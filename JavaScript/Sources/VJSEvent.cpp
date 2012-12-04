/*
* This file is part of Wakanda software, licensed by 4D under
*  (i) the GNU General Public License version 3 (GNU GPL v3), or
*  (ii) the Affero General Public License version 3 (AGPL v3) or
*  (iii) a commercial license.
* This file remains the exclusive property of 4D and/or its licensors
* and is protected by national and international legislations.
* In any event, Licensee's compliance with the terms and conditions
* of the applicable license constitutes a prerequisite to any use of this file.
* Except as otherwise expressly stated in the applicable license,
* such license does not include any other license or rights on this file,
* 4D's and/or its licensors' trademarks and/or other proprietary rights.
* Consequently, no title, copyright or other proprietary rights
* other than those specified in the applicable license is granted.
*/
#include "VJavaScriptPrecompiled.h"

#include "VJSEvent.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSDOMEvent.h"
#include "VJSMessagePort.h"
#include "VJSWorker.h"
#include "VJSStructuredClone.h"
#include "VJSBuffer.h"
#include "VJSEventEmitter.h"
#include "VJSNet.h"
#include "VJSNetServer.h"
#include "VJSNetSocket.h"
#include "VJSW3CFileSystem.h"
#include "VJSSystemWorker.h"

USING_TOOLBOX_NAMESPACE

bool IJSEvent::InsertEvent (std::list<IJSEvent *> *ioEventQueue, IJSEvent *inEvent)
{
	std::list<IJSEvent *>::iterator	i;
	bool							isFirstPosition;
		
	for (i = ioEventQueue->begin(); i != ioEventQueue->end(); i++)
		
		if (inEvent->fTriggerTime <= (*i)->fTriggerTime)
			
			break;

	isFirstPosition = i == ioEventQueue->begin();
	ioEventQueue->insert(i, inEvent);

	return isFirstPosition;
}

VJSMessageEvent *VJSMessageEvent::Create (VJSMessagePort *inMessagePort, VJSStructuredClone *inMessage)
{
	xbox_assert(inMessagePort != NULL && inMessage != NULL);

	VJSMessageEvent	*messageEvent;

	messageEvent = new VJSMessageEvent();
	messageEvent->fType = eTYPE_MESSAGE; 
	messageEvent->fTriggerTime.FromSystemTime();	// Service as soon as possible.
	messageEvent->fMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);
	messageEvent->fData = XBOX::RetainRefCountable<VJSStructuredClone>(inMessage);

	return messageEvent;
}

void VJSMessageEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
		
	XBOX::VJSObject		callbackObject(inContext);	

	callbackObject = fMessagePort->GetCallback(inWorker, inContext);
	if (callbackObject.IsFunction()) {

		XBOX::VJSObject				object(inContext);
		XBOX::VJSValue				data = fData->MakeValue(inContext);
		std::vector<XBOX::VJSValue>	callbackArguments;

		object.SetNull();	// Should implement EventTarget interface instead.

		XBOX::VJSObject	messageEventObject	= VJSDOMMessageEventClass::NewInstance(inContext, object, fTriggerTime, data);
					
		callbackArguments.push_back(messageEventObject);

		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL, NULL);

	}

	Discard();
}

void VJSMessageEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fMessagePort);
	XBOX::ReleaseRefCountable<VJSStructuredClone>(&fData);
	Release();
}

void VJSMessageEvent::RemoveMessageFrom (std::list<IJSEvent *> *ioEventQueue, VJSMessagePort *inMessagePort)
{
	std::list<IJSEvent *>::iterator	i;

	for (i = ioEventQueue->begin(); i != ioEventQueue->end(); i++)

		if ((*i)->GetType() == eTYPE_MESSAGE && ((VJSMessageEvent *) (*i))->fMessagePort == inMessagePort) {

			(*i)->Discard();
			ioEventQueue->erase(i);
	
		}
}

VJSConnectionEvent *VJSConnectionEvent::Create (VJSMessagePort *inConnectionPort, VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort)
{
	xbox_assert(inMessagePort != NULL && inConnectionPort != NULL && inErrorPort != NULL);

	VJSConnectionEvent	*connectionEvent;

	connectionEvent = new VJSConnectionEvent();
	connectionEvent->fType = eTYPE_CONNECTION; 
	connectionEvent->fTriggerTime.FromSystemTime();	// Service as soon as possible.
	connectionEvent->fConnectionPort = XBOX::RetainRefCountable<VJSMessagePort>(inConnectionPort);	
	connectionEvent->fMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);	
	connectionEvent->fErrorPort = XBOX::RetainRefCountable<VJSMessagePort>(inErrorPort);
	
	return connectionEvent;
}

void VJSConnectionEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(!inWorker->IsDedicatedWorker());

	XBOX::VJSObject	callbackObject(inContext);

	callbackObject = fConnectionPort->GetCallback(inWorker, inContext);
	if (callbackObject.IsFunction()) { 

		// Connection event is a special MessageEvent with "connect" type (section 4.7.3 of Web Workers specification).

		XBOX::VJSObject	object(inContext);
		XBOX::VJSValue	emptyStringValue(inContext);

		object.SetNull();					// Should implement EventTarget interface instead.
		emptyStringValue.SetString("");
 
		XBOX::VJSObject	messageEventObject	= VJSDOMMessageEventClass::NewInstance(inContext, object, fTriggerTime, emptyStringValue);
		VJSDOMEvent		*domEvent			= messageEventObject.GetPrivateData<VJSDOMMessageEventClass>();

		domEvent->fType = "connect";
				
		XBOX::VJSArray	portsArrayObject(inContext);
		XBOX::VJSObject	messagePort = VJSMessagePortClass::CreateInstance(inContext, fMessagePort);

		fMessagePort->SetObject(inWorker, messagePort);
		fErrorPort->SetObject(inWorker, messagePort);

		portsArrayObject.PushValue(messagePort);
		messageEventObject.SetProperty("ports", portsArrayObject);

		std::vector<XBOX::VJSValue>	callbackArguments;
												
		callbackArguments.push_back(messageEventObject);
		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL, NULL);

	} else {

		// Unable to invoke "onconnect" callback of shared worker. Thus it has no way to communicate back 
		// with other ("outside") javascript thread. So close its side of the port.

		fMessagePort->Close(inWorker);

	}			
	
	Discard();
}

void VJSConnectionEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fConnectionPort);
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fMessagePort);
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fErrorPort);	
	Release();
}

VJSErrorEvent *VJSErrorEvent::Create (VJSMessagePort *inMessagePort, 
										const XBOX::VString &inMessage, 
										const XBOX::VString &inFileName, 
										sLONG inLineNumber)
{
	xbox_assert(inMessagePort != NULL);

	VJSErrorEvent	*errorEvent;

	errorEvent = new VJSErrorEvent();
	errorEvent->fType = eTYPE_ERROR; 
	errorEvent->fTriggerTime.FromSystemTime();	// Service as soon as possible.
	errorEvent->fMessagePort = XBOX::RetainRefCountable<VJSMessagePort>(inMessagePort);
	errorEvent->fMessage = inMessage;
	errorEvent->fFileName = inFileName;
	errorEvent->fLineNumber = inLineNumber;

	return errorEvent;
}


void VJSErrorEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
		
	XBOX::VJSObject	callbackObject(inContext);
	
	callbackObject = fMessagePort->GetCallback(inWorker, inContext);
	if (callbackObject.IsFunction()) {

		XBOX::VJSObject	object(inContext);

		object.SetNull();	// Should implement EventTarget interface instead.

		XBOX::VJSObject	errorEventObject	= VJSDOMErrorEventClass::NewInstance(inContext, object, fTriggerTime, fMessage, fFileName, fLineNumber);
		
		std::vector<XBOX::VJSValue>	callbackArguments;		

		callbackArguments.push_back(errorEventObject);

		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL, NULL);

	} else {

		// No callback to handle error: If inWorker is a dedicated worker, propagate to error its parent.
		// Section 4.6 "Runtime Script Errors" of web workers specification.

		if (inWorker->IsDedicatedWorker())

			inWorker->BroadcastError(fMessage, fFileName, fLineNumber);

	}

	Discard();
}

void VJSErrorEvent::Discard ()
{
	XBOX::ReleaseRefCountable<VJSMessagePort>(&fMessagePort);
	Release();
}
	
VJSTimerEvent *VJSTimerEvent::Create (VJSTimer *inTimer, XBOX::VTime &inTriggerTime, std::vector<XBOX::VJSValue> *inArguments)
{
	xbox_assert(inTimer != NULL && !inTimer->IsCleared() && inArguments != NULL);

	VJSTimerEvent	*timerEvent;

	timerEvent = new VJSTimerEvent();
	timerEvent->fType = eTYPE_TIMER;
	timerEvent->fTriggerTime = inTriggerTime;
	timerEvent->fTimer = inTimer;
	timerEvent->fArguments = inArguments;

	return timerEvent;
}

void VJSTimerEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(!fTimer->IsCleared());

	XBOX::VJSObject	callbackObject(inContext);
				
	callbackObject.SetObjectRef(fTimer->fFunctionObject);
	xbox_assert(callbackObject.IsFunction());

  	inContext.GetGlobalObject().CallFunction(callbackObject, fArguments, NULL, NULL);

	if (fTimer->IsInterval()) {
		
		if (!fTimer->IsCleared()) {

			// If clearInterval() hasn't been called, re-schedule.
			// Note that same VJSTimerEvent object is re-used, no call to Discard().

			fTriggerTime.AddMilliseconds(fTimer->fInterval);
			inWorker->QueueEvent(this);

		} else

			Discard();

	} else {

		fTimer->_Clear();
		Discard();

	}
}

void VJSTimerEvent::Discard ()
{
	fTimer->_ReleaseIfCleared();
	Release();
}

void VJSTimerEvent::RemoveTimerEvent (std::list<IJSEvent *> *ioEventQueue, VJSTimer *inTimer)
{
	std::list<IJSEvent *>::iterator	i;

	for (i = ioEventQueue->begin(); i != ioEventQueue->end(); i++)

		if ((*i)->GetType() == eTYPE_TIMER 
		&& ((VJSTimerEvent *) (*i))->fTimer == inTimer) {

			(*i)->Discard();
			ioEventQueue->erase(i);
			break;

		}
}

VJSSystemWorkerEvent *VJSSystemWorkerEvent::Create (VJSSystemWorker *inSystemWorker, sLONG inType, XBOX::JS4D::ObjectRef inObjectRef, uBYTE *inData, sLONG inSize)
{
	xbox_assert(inSystemWorker != NULL);
	xbox_assert(inType == eTYPE_STDOUT_DATA || inType == eTYPE_STDERR_DATA || inType == eTYPE_TERMINATION);
	xbox_assert(inObjectRef != NULL && inData != NULL && inSize > 0);
			
	VJSSystemWorkerEvent	*systemWorkerEvent;

	systemWorkerEvent = new VJSSystemWorkerEvent();
	systemWorkerEvent->fSystemWorker = XBOX::RetainRefCountable<VJSSystemWorker>(inSystemWorker);
	systemWorkerEvent->fType = eTYPE_SYSTEM_WORKER;
	systemWorkerEvent->fTriggerTime.FromSystemTime();

	systemWorkerEvent->fSubType = inType;
	systemWorkerEvent->fObjectRef = inObjectRef;
	systemWorkerEvent->fData = inData;
	systemWorkerEvent->fSize = inSize;
		
	return systemWorkerEvent;
}

void VJSSystemWorkerEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	XBOX::VJSObject	proxyObject(inContext, fObjectRef);
	XBOX::VJSObject	callbackObject(inContext);
	XBOX::VJSObject	argumentObject(inContext);

	argumentObject.MakeEmpty();
	switch (fSubType) {

		case eTYPE_STDOUT_DATA: {
	
			callbackObject = proxyObject.GetPropertyAsObject("onmessage");
			if (callbackObject.IsFunction()) {
			
				argumentObject.MakeEmpty();
				argumentObject.SetProperty("type", "message");
				argumentObject.SetProperty("target", proxyObject);
				if (proxyObject.GetPropertyAsBool("isbinary", NULL, NULL)) 

					argumentObject.SetProperty("data", VJSBufferClass::NewInstance(inContext, fSize, fData));

				else {

					XBOX::VString	string((const char *) fData);
				
					argumentObject.SetProperty("data", string);
					::free(fData);					

				}

			} else

				::free(fData);

			// fData has been freed already or is to be freed by Buffer when done, 
			// so set fData to NULL to prevent Discard() from freeing it.

			fData = NULL;
			break;

		}

		case eTYPE_STDERR_DATA: {

			// See comment for eTYPE_STDOUT_DATA

			callbackObject = proxyObject.GetPropertyAsObject("onerror");
			if (callbackObject.IsFunction()) {

				argumentObject.MakeEmpty();
				argumentObject.SetProperty("type", "error");
				argumentObject.SetProperty("target", proxyObject);
				if (proxyObject.GetPropertyAsBool("isbinary", NULL, NULL))

					argumentObject.SetProperty("data", VJSBufferClass::NewInstance(inContext, fSize, fData));
	
				else {

					XBOX::VString	string((const char *) fData);
				
					argumentObject.SetProperty("data", string);
					::free(fData);					

				}

			} else

				::free(fData);

			fData = NULL;
			break;

		}
		
		case eTYPE_TERMINATION: {

			callbackObject = proxyObject.GetPropertyAsObject("onterminated");
			if (callbackObject.IsFunction()) {

				STerminationData	*data;

				data = (STerminationData *) fData;

				argumentObject.MakeEmpty();
				argumentObject.SetProperty("type", "terminate");
				argumentObject.SetProperty("target", proxyObject);
				argumentObject.SetProperty("hasStarted", data->fHasStarted);
				argumentObject.SetProperty("forced", data->fForcedTermination);
				argumentObject.SetProperty("exitStatus", (Real) data->fExitStatus);
				argumentObject.SetProperty("pid", data->fPID);	

			}

			delete fData;
			fData = NULL;

			break;		
		
		}

	}

	if (callbackObject.IsFunction()) {

		std::vector<XBOX::VJSValue>	callbackArguments;

		callbackArguments.push_back(argumentObject);	
		inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL, NULL);

	}		

	Discard();
}

void VJSSystemWorkerEvent::Discard ()
{
	if (fSubType == eTYPE_TERMINATION) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&fSystemWorker->fCriticalSection);

		fSystemWorker->fTerminationEventDiscarded = true;

	}

	XBOX::ReleaseRefCountable<VJSSystemWorker>(&fSystemWorker);

	// If Process() hasn't be called, free fData.

	if (fData != NULL) {

		if (fSubType == eTYPE_TERMINATION) 

			delete fData;

		else

			::free(fData);

		fData = NULL;

	}	
	Release();
}

VJSNetEvent *VJSNetEvent::Create (VJSEventEmitter *inEventEmitter, const XBOX::VString &inEventName)
{
	xbox_assert(inEventEmitter != NULL);
		
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_NO_ARGUMENT;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	netEvent->fEventName = inEventName;
	
	return netEvent;
}

VJSNetEvent *VJSNetEvent::CreateData (VJSNetSocketObject *inSocketObject, uBYTE *inData, sLONG inSize)
{
	xbox_assert(inSocketObject != NULL);
	xbox_assert(inSize >= 0);
	xbox_assert(!(inSize > 0 && inData == NULL));
	
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_DATA;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSNetSocketObject>(inSocketObject);
	netEvent->fData = inData;
	netEvent->fSize = inSize;
	
	return netEvent;
}

VJSNetEvent *VJSNetEvent::CreateError (VJSEventEmitter *inEventEmitter, const XBOX::VString &inExceptionName)
{
	xbox_assert(inEventEmitter != NULL);
			
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_ERROR;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	netEvent->fExceptionName = inExceptionName;
		
	return netEvent;
}

VJSNetEvent *VJSNetEvent::CreateClose (VJSEventEmitter *inEventEmitter, bool inHadError)
{
	xbox_assert(inEventEmitter != NULL);
		
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = eTYPE_CLOSE;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSEventEmitter>(inEventEmitter);
	netEvent->fHadError = inHadError;
		
	return netEvent;
}

VJSNetEvent	*VJSNetEvent::CreateConnection (VJSNetServerObject *inServerObject, XBOX::VTCPEndPoint *inEndPoint, bool inIsSSL)
{
	xbox_assert(inServerObject != NULL && inEndPoint != NULL);
	
	VJSNetEvent	*netEvent;

	netEvent = new VJSNetEvent();
	netEvent->fType = eTYPE_NET;
	netEvent->fTriggerTime.FromSystemTime();

	netEvent->fSubType = inIsSSL ? eTYPE_CONNECTION_SSL : eTYPE_CONNECTION;
	netEvent->fEventEmitter = XBOX::RetainRefCountable<VJSNetServerObject>(inServerObject);
	netEvent->fEndPoint = inEndPoint;
	
	return netEvent;
}

void VJSNetEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);

	XBOX::VString	callbackName;
	bool			isOk;

	isOk = true;
	switch (fSubType) {

		case eTYPE_NO_ARGUMENT:		callbackName = fEventName; break;
		case eTYPE_DATA:			callbackName = "data"; break;
		case eTYPE_ERROR:			callbackName = "error"; break;
		case eTYPE_CLOSE:			callbackName = "close"; break;
		case eTYPE_CONNECTION:		callbackName = "connection"; break;
		case eTYPE_CONNECTION_SSL:	callbackName = "secureConnection"; break;
		default:					xbox_assert(false); break;
		
	}

	std::vector<XBOX::VJSValue>	callbackArguments;

	if (fSubType == eTYPE_DATA) {

		// If encoding is XBOX::VTC_UNKNOWN (default when Socket object is constructed), then data is 
		// considered as binary and no string conversion takes place, returning a Buffer object.

		CharSet	encoding;

		if ((encoding = ((VJSNetSocketObject *) fEventEmitter)->GetEncoding()) == XBOX::VTC_UNKNOWN) {

			callbackArguments.push_back(VJSBufferClass::NewInstance(inContext, fSize, fData));
	
			// This will prevent Discard() from freeing memory.

			fData = NULL;

		} else {

			VJSBufferObject	*buffer;
			VJSValue		value(inContext);
						
			if ((buffer = new VJSBufferObject(fSize, fData)) == NULL) {

				value.SetString("");			
				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

				// Discard() will free fData.

			} else {

				XBOX::VString	decodedString;

				// Ignore conversion failure.

				buffer->ToString(encoding, 0, fSize, &decodedString);
				value.SetString(decodedString);

				// Buffer object destructor will free fData, Discard() doesn't have to do it.

				ReleaseRefCountable(&buffer);
				fData = NULL;

			}

			callbackArguments.push_back(value);

		}

	} else if (fSubType == eTYPE_ERROR) {

		// Should do a "new fExceptionName()" instead of the current null object.
		// Currently, just an error string.

		XBOX::VJSValue	value(inContext);

		value.SetString(fExceptionName);
		callbackArguments.push_back(value);

	} else if (fSubType == eTYPE_CLOSE) {

		XBOX::VJSValue	value(inContext);

		value.SetBool(fHadError);
		callbackArguments.push_back(value);

	} else if (fSubType == eTYPE_CONNECTION || fSubType == eTYPE_CONNECTION_SSL) {

		XBOX::VJSValue	value(inContext);
		
		VJSNetSocketClass::NewInstance(inContext, fEventEmitter, fEndPoint, &value);
		
		// Variable value is not checked for null!

		callbackArguments.push_back(value);

	}

	if (isOk)

		fEventEmitter->Emit(inContext, callbackName, &callbackArguments);

	Discard();
}

void VJSNetEvent::Discard ()
{
	if (fSubType == eTYPE_DATA && fData != NULL)

		::free(fData);

	else if (fSubType == eTYPE_CONNECTION || fSubType == eTYPE_CONNECTION_SSL) {

		xbox_assert(fEndPoint != NULL);

		// If the "connection" or "secureConnection" event has no callback, this will be the "last" release.
		// VTCPEndPoint's destructor will called and socket will be closed.

		ReleaseRefCountable(&fEndPoint);

	}

	XBOX::ReleaseRefCountable<VJSEventEmitter>(&fEventEmitter);

	Release();
}

VJSCallbackEvent *VJSCallbackEvent::Create (XBOX::VJSObject &inObject, const XBOX::VString &inCallbackName, std::vector<XBOX::VJSValue> *inArguments)
{
	xbox_assert(inObject.GetObjectRef() != NULL && inArguments != NULL);
	
	VJSCallbackEvent	*callbackEvent;

	callbackEvent = new VJSCallbackEvent();
	callbackEvent->fType = eTYPE_CALLBACK;
	callbackEvent->fTriggerTime.FromSystemTime();

	callbackEvent->fObjectRef = inObject.GetObjectRef();
	callbackEvent->fCallbackName = inCallbackName;
	callbackEvent->fArguments = inArguments;
	
	return callbackEvent;
}
												
void VJSCallbackEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{	
	xbox_assert(inWorker != NULL);
		
	XBOX::VJSObject	object(inContext, fObjectRef);
	XBOX::VJSObject	callbackObject	= object.GetPropertyAsObject(fCallbackName);

	if (callbackObject.IsFunction())
	
		inContext.GetGlobalObject().CallFunction(callbackObject, fArguments, NULL, NULL);

	Discard();
}

void VJSCallbackEvent::Discard ()
{
	delete fArguments;
	Release();
}

VJSW3CFSEvent *VJSW3CFSEvent::RequestFS (VJSLocalFileSystem *inLocalFileSystem, sLONG inType, VSize inQuota, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inLocalFileSystem != NULL);
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_REQUEST_FILE_SYSTEM, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fLocalFileSystem = XBOX::RetainRefCountable<VJSLocalFileSystem>(inLocalFileSystem);
		fsEvent->fType = inType;
		fsEvent->fQuota = inQuota;

	}
	
	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::ResolveURL (VJSLocalFileSystem *inLocalFileSystem, const XBOX::VString &inURL, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inLocalFileSystem != NULL);
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_RESOLVE_URL, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fLocalFileSystem = XBOX::RetainRefCountable<VJSLocalFileSystem>(inLocalFileSystem);
		fsEvent->fURL = inURL;
		
	}

	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::GetMetadata (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL);
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_RESOLVE_URL, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::MoveTo (VJSEntry *inEntry, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inTargetEntry != NULL && !inTargetEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_MOVE_TO, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fTargetEntry = XBOX::RetainRefCountable<VJSEntry>(inTargetEntry);
		fsEvent->fURL = inNewName;

	}
	
	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::CopyTo (VJSEntry *inEntry, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inTargetEntry != NULL && !inTargetEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_COPY_TO, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fTargetEntry = XBOX::RetainRefCountable<VJSEntry>(inTargetEntry);
		fsEvent->fURL = inNewName;

	}
	
	return fsEvent;
}

VJSW3CFSEvent *VJSW3CFSEvent::Remove (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL);
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_REMOVE, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}


VJSW3CFSEvent *VJSW3CFSEvent::GetParent (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL);
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_GET_PARENT, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::GetFile (VJSEntry *inEntry, const XBOX::VString &inURL, sLONG inFlags, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_GET_FILE, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fURL = inURL;
		fsEvent->fFlags = inFlags;

	}

	return fsEvent;	
}	

VJSW3CFSEvent *VJSW3CFSEvent::GetDirectory (VJSEntry *inEntry, const XBOX::VString &inURL, sLONG inFlags, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_GET_DIRECTORY, inSuccessCallback, inErrorCallback)) != NULL) {

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
		fsEvent->fURL = inURL;
		fsEvent->fFlags = inFlags;

	}

	return fsEvent;	
}	

VJSW3CFSEvent *VJSW3CFSEvent::RemoveRecursively (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_REMOVE_RECURSIVELY, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::Folder (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_FOLDER, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::CreateWriter (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_CREATE_WRITER, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::File (VJSEntry *inEntry, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inEntry != NULL && inEntry->IsFile());
	
	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_FILE, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fEntry = XBOX::RetainRefCountable<VJSEntry>(inEntry);
	
	return fsEvent;	
}

VJSW3CFSEvent *VJSW3CFSEvent::ReadEntries (VJSDirectoryReader *inDirectoryReader, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	xbox_assert(inDirectoryReader != NULL && !inDirectoryReader->IsReading());

	VJSW3CFSEvent	*fsEvent;
	
	if ((fsEvent = new VJSW3CFSEvent(eOPERATION_READ_ENTRIES, inSuccessCallback, inErrorCallback)) != NULL)

		fsEvent->fDirectoryReader= XBOX::RetainRefCountable<VJSDirectoryReader>(inDirectoryReader);
	
	return fsEvent;	
}

void VJSW3CFSEvent::Process (XBOX::VJSContext inContext, VJSWorker *inWorker)
{
	xbox_assert(inWorker != NULL);
	
	sLONG			code;
	XBOX::VJSObject	resultObject(inContext);	
		
	switch (fSubType) {

		case eOPERATION_REQUEST_FILE_SYSTEM:	code = fLocalFileSystem->RequestFileSystem(inContext, fType, fQuota, &resultObject); break;
		case eOPERATION_RESOLVE_URL:			code = fLocalFileSystem->ResolveURL(inContext, fURL, false, &resultObject); break;

		case eOPERATION_GET_METADATA:			code = fEntry->GetMetadata(inContext, &resultObject); break;
		case eOPERATION_MOVE_TO:				code = fEntry->MoveTo(inContext, fTargetEntry, fURL, &resultObject); break;
		case eOPERATION_COPY_TO:				code = fEntry->CopyTo(inContext, fTargetEntry, fURL, &resultObject); break;
		case eOPERATION_REMOVE:					code = fEntry->Remove(inContext, &resultObject); break;
		case eOPERATION_GET_PARENT:				code = fEntry->GetParent(inContext, &resultObject); break;

		case eOPERATION_GET_FILE:				code = fEntry->GetFile(inContext, fURL, fFlags, &resultObject); break;
		case eOPERATION_GET_DIRECTORY:			code = fEntry->GetDirectory(inContext, fURL, fFlags, &resultObject); break;
		case eOPERATION_REMOVE_RECURSIVELY:		code = fEntry->RemoveRecursively(inContext, &resultObject); break;
		case eOPERATION_FOLDER:					code = fEntry->Folder(inContext, &resultObject); break;
		
		case eOPERATION_CREATE_WRITER:			code = fEntry->CreateWriter(inContext, &resultObject); break;
		case eOPERATION_FILE:					code = fEntry->File(inContext, &resultObject); break;

		case eOPERATION_READ_ENTRIES: {

			if ((code = fDirectoryReader->ReadEntries(inContext, &resultObject)) == VJSFileErrorClass::OK) {

				xbox_assert(resultObject.IsArray());

				XBOX::VJSArray	entries(resultObject, false);	// No check, already an assert before.

				// If array is empty, there are no more entries to read. If not, reschedule event.

				if (entries.GetLength()) 

					inWorker->QueueEvent(this);

				else

					Discard();

			} else

				Discard();	

			break;

		}

		default:								xbox_assert(false);
			
	}

	std::vector<XBOX::VJSValue>	callbackArguments;

	callbackArguments.push_back(resultObject);
	if (code == VJSFileErrorClass::OK) {
		
		if (fSuccessCallback != NULL) {

			XBOX::VJSObject	callbackObject(inContext, fSuccessCallback);

			inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL, NULL);

		}
	
	} else {

		if (fErrorCallback != NULL) {

			XBOX::VJSObject	callbackObject(inContext, fErrorCallback);

			inContext.GetGlobalObject().CallFunction(callbackObject, &callbackArguments, NULL, NULL);

		}
		
	}	

	if (fSubType != eOPERATION_READ_ENTRIES)

		Discard();
}

void VJSW3CFSEvent::Discard ()
{
	if (fSubType == eOPERATION_REQUEST_FILE_SYSTEM || fSubType == eOPERATION_RESOLVE_URL) {
	
		xbox_assert(fLocalFileSystem != NULL);
		
		XBOX::ReleaseRefCountable<VJSLocalFileSystem>(&fLocalFileSystem);
	
	} else if (fSubType == eOPERATION_READ_ENTRIES) {

		xbox_assert(fDirectoryReader != NULL);

		XBOX::ReleaseRefCountable<VJSDirectoryReader>(&fDirectoryReader);

	} else {
	
		xbox_assert(fEntry != NULL);

		XBOX::ReleaseRefCountable<VJSEntry>(&fEntry);		
		if (fSubType == eOPERATION_MOVE_TO || fSubType == eOPERATION_COPY_TO) {
			
			xbox_assert(fTargetEntry != NULL);
			
			XBOX::ReleaseRefCountable<VJSEntry>(&fTargetEntry);
			
		}		

	}
	Release();
}

VJSW3CFSEvent::VJSW3CFSEvent (sLONG inSubType, const XBOX::VJSObject &inSuccessCallback, const XBOX::VJSObject &inErrorCallback)
{
	fType = eTYPE_W3C_FS;
	fTriggerTime.FromSystemTime();	
	
	fSubType = inSubType;
	fSuccessCallback = inSuccessCallback.GetObjectRef();
	fErrorCallback = inErrorCallback.GetObjectRef();	
}