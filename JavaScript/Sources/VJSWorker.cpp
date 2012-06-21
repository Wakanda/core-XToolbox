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

#include "VJSWorker.h"

#include "VJSClass.h"
#include "VJSGlobalClass.h"

#include "VJSEvent.h"
#include "VJSMessagePort.h"
#include "VJSWorkerProxy.h"

#include "VJSW3CFileSystem.h"

USING_TOOLBOX_NAMESPACE

XBOX::VCriticalSection	VJSWorker::sMutex;				
std::list<VJSWorker *>	VJSWorker::sDedicatedWorkers;	
std::list<VJSWorker *>	VJSWorker::sSharedWorkers;
std::list<VJSWorker *>	VJSWorker::sRootWorkers;
IJSWorkerDelegate*		VJSWorker::sDelegate = NULL;

void VJSWorker::SetDelegate (IJSWorkerDelegate *inDelegate)
{
	// Set to NULL when finishing.

	sDelegate = inDelegate;
}

VJSWorker::VJSWorker (XBOX::VJSContext &inParentContext, const XBOX::VString &inURL, bool inReUseContext)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	XBOX::VError							error;
	
	fStartTime.FromSystemTime();	
	
	fInsideWaitCount = 0;
	fTotalIdleDuration.Clear();
	
	fGlobalContext = sDelegate->RetainJSContext(error, inParentContext, inReUseContext);
	
	fVTask = new XBOX::VTask(NULL, 0, XBOX::eTaskStylePreemptive, _RunProc);
	fVTask->SetKindData((sLONG_PTR) this);
	fClosingFlag = fIsLockedWaiting = fExitWaitFlag = false;

	fIsDedicatedWorker = true;
	fURL = inURL;
	fOnMessagePort = NULL;

	fLocalFileSystem = NULL;
			
	_PopulateGlobalObject(XBOX::VJSContext(fGlobalContext));

	sDedicatedWorkers.push_back(this);
}

VJSWorker *VJSWorker::GetSharedWorker (XBOX::VJSContext &inParentContext,	
										const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext, 
										bool *outCreatedWorker)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);	
	std::list<VJSWorker *>::iterator		i;
	
	for (i = sSharedWorkers.begin(); i != sSharedWorkers.end(); i++) {

		xbox_assert(!(*i)->fIsDedicatedWorker);
		if ((*i)->fURL.EqualToString(inURL) && (*i)->fName.EqualToString(inName)) {

			*outCreatedWorker = false;
			return *i;

		}

	}

	*outCreatedWorker = true;
	return new VJSWorker(inParentContext, inURL, inName, inReUseContext);
}

void VJSWorker::SetMessagePorts (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort)
{
	xbox_assert(IsDedicatedWorker() && inMessagePort != NULL && inErrorPort != NULL);

	fOnMessagePort = inMessagePort;	

	XBOX::VJSContext	context(fGlobalContext);
	XBOX::VJSObject		globalObject	= context.GetGlobalObject();

	inMessagePort->SetObject(this, globalObject);
	inErrorPort->SetObject(this, globalObject);
}
	
void VJSWorker::Connect (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort)
{
	xbox_assert(!IsDedicatedWorker() && fConnectionPort != NULL && inMessagePort != NULL && inErrorPort != NULL);
	
	QueueEvent(VJSConnectionEvent::Create(fConnectionPort, inMessagePort, inErrorPort));
}

void VJSWorker::AddErrorPort (VJSMessagePort *inErrorPort)
{
	xbox_assert(inErrorPort != NULL);

	inErrorPort->Retain();

	// Clean-up first.

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);	
	std::list<VJSMessagePort *>::iterator	i;

	for (i = fErrorPorts.begin(); i != fErrorPorts.end(); )

		if ((*i)->IsOtherClosed(this)) {

			(*i)->Release();
			i = fErrorPorts.erase(i);

		} else

			i++;

	// Then add error port.
	
	fErrorPorts.push_back(inErrorPort);
}

void VJSWorker::BroadcastError (const XBOX::VString &inMessage, const XBOX::VString &inFileNumber, sLONG inLineNumber)
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);	
	std::list<VJSMessagePort *>::iterator	i;

	for (i = fErrorPorts.begin(); i != fErrorPorts.end(); ) {

		if ((*i)->IsOtherClosed(this)) {

			// Clean-up.

			(*i)->Release();
			i = fErrorPorts.erase(i);

		} else {

			(*i)->PostError((*i)->GetOther(this), inMessage, inFileNumber, inLineNumber); 
			i++;

		}
		
	}
}

void VJSWorker::Run ()
{
	xbox_assert(fVTask != NULL);

	fVTask->Run();
}

sLONG VJSWorker::WaitFor (XBOX::VJSContext &inContext, sLONG inWaitingDuration, IJSEventGenerator *inEventGenerator, sLONG inEventType)
{
	fInsideWaitCount++;

	bool		isEventMatched;
	XBOX::VTime	endTime, currentTime;
	
	isEventMatched = false;		
	if (inWaitingDuration >= 0) {

		endTime.FromSystemTime();
		endTime.AddMilliseconds(inWaitingDuration);

	}

	fMutex.Lock();
	currentTime.FromSystemTime();
	while (!fClosingFlag && !fExitWaitFlag) {
		
		// If there is an event to be triggered, process it.
		
		if (!fEventQueue.empty() && currentTime >= fEventQueue.front()->GetTriggerTime()) {

			IJSEvent	*event;

			event = fEventQueue.front();
			fEventQueue.pop_front();

			// If an event generator and event type has been specified, check if the event to process is matching.

			if (inEventGenerator != NULL && inEventGenerator->MatchEvent(event, inEventType))

				isEventMatched = true;

			// Process event.

			fMutex.Unlock();

			if ((JS4D::ContextRef) inContext == NULL) {
			
				VJSContext	context(fGlobalContext);

				event->Process(context, this);

			} else

				event->Process(inContext, this);

			fMutex.Lock();

			// Event has been matched, exit.

			if (isEventMatched)

				break;

			currentTime.FromSystemTime();

			// If waiting time has elapsed, exit. If polling (inWaitingDuration is zero), execute all pending events.
			// Note: Place a limit on polling executions ? If no limit (always execute all pending events), the wait() 
			// may never end (events keep adding new events).

			if (inWaitingDuration > 0 && currentTime >= endTime) 
	
				break;	

			else

				continue;

		}

		// Check if we are polling. If so, do not wait, just exit.

		if (!inWaitingDuration)

			break;

		// Compute time to wait until then next event.
		
		bool			isTimedOutWait;
		XBOX::VDuration	duration;
		uLONG			waitDuration;
						
		isTimedOutWait = true;
		if (fEventQueue.empty()) {

			if (inWaitingDuration > 0) {

				if (endTime > currentTime) {

					endTime.Substract(currentTime, duration);
					waitDuration = duration.ConvertToMilliseconds();

				} else 

					break;	// Waiting duration elapsed.

			} else

				isTimedOutWait = false;
				
		} else {

			XBOX::VTime	minimum;

			minimum = fEventQueue.front()->GetTriggerTime();
			if (inWaitingDuration > 0 && minimum > endTime)

				minimum = endTime;
		
			minimum.Substract(currentTime, duration);	
			waitDuration = duration.ConvertToMilliseconds();
			
		}

		// Wait until it's time to trigger next event.
		
		XBOX::VTime	waitStartTime;
		bool		syncBoolean;
		
		waitStartTime = currentTime;
		fIsLockedWaiting = true;
		fMutex.Unlock();
		if (isTimedOutWait) {
		
			if (waitDuration)

				syncBoolean = fSyncEvent.Lock(waitDuration);

			else

				syncBoolean = false;		
		
		} else {

			syncBoolean = fSyncEvent.Lock();

			// Stopped waiting because fClosingFlag has been set to true or an event has been queued.
			// Cannot be a timeout.

			xbox_assert(syncBoolean);	

		}

		// If an fSyncEvent.Unlock() is sent, it will be ignored.

		fIsLockedWaiting = false;
		fMutex.Lock();

		fSyncEvent.Reset();

		// Find out the actual time idled for statistics.
		
		XBOX::VDuration	actualWaitDuration;
		
		currentTime.FromSystemTime();
		currentTime.Substract(waitStartTime, actualWaitDuration);
		
		fTotalIdleDuration.Add(actualWaitDuration);		
		
		// If timeout, check if waiting time has elapsed.
				
		if (!syncBoolean && inWaitingDuration > 0 && currentTime >= endTime) 
	
			break;	

	}
	fMutex.Unlock();
	
	fInsideWaitCount--;
	xbox_assert(fInsideWaitCount >= 0);

	fExitWaitFlag = false;

	if (inEventGenerator != NULL && isEventMatched)

		return -1;

	else 

		return fClosingFlag ? 1 : 0;
}

void VJSWorker::Terminate ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	fClosingFlag = true;

	// If locked waiting for an event, send signal.

	if (fIsLockedWaiting)

		fSyncEvent.Unlock();
}

void VJSWorker::TerminateAll ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	std::list<VJSWorker *>::iterator		i;

	for (i = sDedicatedWorkers.begin(); i != sDedicatedWorkers.end(); i++)

		(*i)->Terminate();

	for (i = sSharedWorkers.begin(); i != sSharedWorkers.end(); i++)

		(*i)->Terminate();

	for (i = sRootWorkers.begin(); i != sRootWorkers.end(); i++)

		(*i)->Terminate();
}

VJSWorker *VJSWorker::GetWorker (const XBOX::VJSContext &inContext)
{
	XBOX::VJSGlobalObject	*globalObject;
	VJSWorker				*worker;

	globalObject = inContext.GetGlobalObjectPrivateInstance();
	xbox_assert(globalObject != NULL);

	// Only "root" javascript execution can return NULL because it hasn't been created as a dedicated or shared worker. 
	// In which case, an empty (without vtask and no postMessage() method or onmessage attribute globals) worker is created.

	if ((worker = (VJSWorker *) globalObject->GetSpecific((VJSSpecifics::key_type) kSpecificKey)) == NULL) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
	
		// Re-check with mutex acquired (race condition avoidance).

		if ((worker = (VJSWorker *) globalObject->GetSpecific((VJSSpecifics::key_type) kSpecificKey)) == NULL) 

			worker = new VJSWorker(inContext);

	}

	return worker;
}

void VJSWorker::QueueEvent (IJSEvent *inEvent)
{
	xbox_assert(inEvent != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);
	std::list<IJSEvent *>::iterator			i;
	
	for (i = fEventQueue.begin(); i != fEventQueue.end(); i++) 

		// If two events have identical trigger time, first queued will be processed first.
		
		if ((*i)->GetTriggerTime() > inEvent->GetTriggerTime())

			break;

	fEventQueue.insert(i, inEvent);	

	// If locked waiting for an event, send signal.

	if (fIsLockedWaiting)

		fSyncEvent.Unlock();
}

void VJSWorker::UnscheduleTimer (VJSTimer *inTimer)
{
	xbox_assert(inTimer != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	VJSTimerEvent::RemoveTimerEvent(&fEventQueue, inTimer);	
}

void VJSWorker::AddMessagePort (VJSMessagePort *inMessagePort)
{
	xbox_assert(inMessagePort != NULL);

	_AddMessagePort(&fMessagePorts, inMessagePort);
}

void VJSWorker::RemoveMessagePort (VJSMessagePort *inMessagePort)
{
	xbox_assert(inMessagePort != NULL);

	_RemoveMessagePort(&fMessagePorts, inMessagePort);
}

void VJSWorker::Close (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker::GetWorker(ioParms.GetContext())->Terminate();
}

void VJSWorker::Wait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	XBOX::VJSContext	context(ioParms.GetContext());
	sLONG				waitDuration;
		
	waitDuration = GetWaitDuration(ioParms);
	ioParms.ReturnBool(VJSWorker::GetWorker(context)->WaitFor(context, waitDuration, NULL, 0) == 1);
}

void VJSWorker::ExitWait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker;

	worker = VJSWorker::GetWorker(ioParms.GetContext());
	if (worker->fInsideWaitCount > 0) {

		XBOX::StLocker<XBOX::VCriticalSection>	lock(&worker->fMutex);

		worker->fExitWaitFlag = true;
		if (worker->fIsLockedWaiting)

			worker->fSyncEvent.Unlock();

	}
}

sLONG VJSWorker::GetWaitDuration (VJSParms_callStaticFunction &ioParms)
{
	// If no duration (in milliseconds) or if it is invalid (negative), wait duration is set to -1 (infinite).

	if (!ioParms.CountParams())

		return -1;

	Real	duration;

	if (!ioParms.GetRealParam(1, &duration))

		return -1;

	// If negative, infinite, or NaN, return -1 (infinite).

	if (duration < 0.0 || duration > XBOX::kMAX_Real || duration != duration)

		return -1;

	else

		return (sLONG) duration;	
}

// sMutex must be locked before creating a new shared worker.

VJSWorker::VJSWorker (XBOX::VJSContext &inParentContext, const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext)
{
	XBOX::VError	error;
	
	fStartTime.FromSystemTime();	
	
	fInsideWaitCount = 0;
	fTotalIdleDuration.Clear();
	
	fGlobalContext = sDelegate->RetainJSContext(error, inParentContext, inReUseContext);

	fVTask = new XBOX::VTask(NULL, 0, XBOX::eTaskStylePreemptive, _RunProc);
	fVTask->SetKindData((sLONG_PTR) this);
	fClosingFlag = fIsLockedWaiting = fExitWaitFlag = false;

	fIsDedicatedWorker = false;	
	fURL = inURL;
	fName = inName;

	XBOX::VJSContext	context(fGlobalContext);
	XBOX::VJSObject		globalObject	= context.GetGlobalObject();
	
	fConnectionPort = VJSMessagePort::Create(this, globalObject, "onconnect");

	fLocalFileSystem = NULL;
		
	_PopulateGlobalObject(context);

	sSharedWorkers.push_back(this);
}

// sMutex must be locked before creating an "empty" worker for "root".

VJSWorker::VJSWorker (const XBOX::VJSContext &inContext) 
{
	fGlobalContext = NULL;

	fStartTime.FromSystemTime();	
	
	fInsideWaitCount = 0;
	fTotalIdleDuration.Clear();

	fURL = "";	//** TODO: Set proper name?

	fVTask = NULL;
	fClosingFlag = fIsLockedWaiting = fExitWaitFlag = false;

	fIsDedicatedWorker = false;			// "Root" is not a dedicated worker of another worker.

	fLocalFileSystem = NULL;

	_PopulateGlobalObject(inContext);

	sRootWorkers.push_back(this);
}

// See comment in _PopulateGlobalObject() about destructor call.

VJSWorker::~VJSWorker ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);

	// Release all error ports, requesting termination of "child" dedicated workers if needed.

	std::list<VJSMessagePort *>::iterator	i;

	for (i = fErrorPorts.begin(); i != fErrorPorts.end(); i++) {

		if (!(*i)->IsInside(this)) {

			// Current worker is the "outside" of the other worker. If the "inside" worker is
			// a dedicated worker, its parent has terminated, so request termination. This will
			// terminate the "worker tree" of dedicated workers. Shared workers are "free" 
			// running.

			if (!(*i)->IsOtherClosed(this) && (*i)->GetOther(this)->IsDedicatedWorker())

				(*i)->GetOther(this)->Terminate();

		}
		(*i)->Release();

	}
	fErrorPorts.clear();

	// Close all message ports. 
	
	VJSMessagePort::CloseAll(this, &fMessagePorts);

	// Mark all timers as "cleared" (they will be freed).

	fTimerContext.ClearAll();
	
	// Discard all pending events. 

	while (!fEventQueue.empty()) {

		fEventQueue.front()->Discard();
		fEventQueue.pop_front();

	}

	std::list<VJSWorker *>	*list;
	
	if (fVTask != NULL) {

		// If not "root", release VTask and context.

		XBOX::ReleaseRefCountable<XBOX::VTask>(&fVTask);

		xbox_assert(fGlobalContext != NULL);

		sDelegate->ReleaseJSContext(fGlobalContext);

		list = fIsDedicatedWorker ? &sDedicatedWorkers : &sSharedWorkers;

	} else {

		xbox_assert(fGlobalContext == NULL);

		list = &sRootWorkers;

	}	

	// Remove worker from dedicated, shared, or root worker list.
	
	std::list<VJSWorker *>::iterator	j;

	for (j = list->begin(); j != list->end(); j++)

		if (*j == this) 

			break;

	if (j != list->end())

		list->erase(j);

	else

		xbox_assert(0);		// Impossible!

	// Release file systems, if any.

	fLocalFileSystem->ReleaseAllFileSystems(); 
	ReleaseRefCountable<VJSLocalFileSystem>(&fLocalFileSystem);
	
#if VERSIONDEBUG
	
	_DumpExecutionStatistics();
	
#endif
}

void VJSWorker::_AddMessagePort (std::list<VJSMessagePort *> *ioMessagePorts, VJSMessagePort *inMessagePort)
{
	xbox_assert(ioMessagePorts != NULL && inMessagePort != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	ioMessagePorts->push_back(inMessagePort);
}

void VJSWorker::_RemoveMessagePort (std::list<VJSMessagePort *> *ioMessagePorts, VJSMessagePort *inMessagePort)
{
	xbox_assert(ioMessagePorts != NULL && inMessagePort != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	std::list<VJSMessagePort *>::iterator	i;

	for (i = ioMessagePorts->begin(); i != ioMessagePorts->end(); i++) 

		if (*i == inMessagePort)

			break;

	// Check only if "main" message port list fMessagePorts.

//	xbox_assert(!(ioMessagePorts == &fMessagePorts && i != ioMessagePorts->end()));	// assert() à remettre.

	if (i != ioMessagePorts->end())

		ioMessagePorts->erase(i);
}

void VJSWorker::_PopulateGlobalObject (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	globalObject	= inContext.GetGlobalObject();

	if (fIsDedicatedWorker) {

		XBOX::VJSObject	postMessageObject(inContext);

		postMessageObject.MakeCallback(XBOX::js_callback<VJSGlobalObject, _PostMessage>);
		globalObject.SetProperty("postMessage", postMessageObject);

	}

	globalObject.SetProperty("self", globalObject);
	globalObject.SetProperty("location", VJSWorkerLocationClass::CreateInstance(inContext, new VJSWorkerLocationObject(fURL)));

	// Setup W3C File System API.

	xbox_assert(fLocalFileSystem == NULL);

	XBOX::VFolder	*folder;
	
	folder = inContext.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
	fLocalFileSystem = VJSLocalFileSystem::RetainLocalFileSystem(inContext, folder->GetPath());
	ReleaseRefCountable<XBOX::VFolder>(&folder);

	// For dedicated and shared workers, VJSWorker object is deleted explicitely at end of execution.
	// For "root" workers, it is deleted when global object is garbage collected.

	inContext.GetGlobalObjectPrivateInstance()->SetSpecific(
		(VJSSpecifics::key_type) kSpecificKey, 
		this, 
		fVTask != NULL ? VJSSpecifics::DestructorVoid : VJSSpecifics::DestructorVObject);
}

sLONG VJSWorker::_RunProc (XBOX::VTask *inVTask)
{
	((VJSWorker *) inVTask->GetKindData())->_DoRun();	
	return 0;
}

void VJSWorker::_DoRun ()
{
	xbox_assert(fGlobalContext != NULL);

	XBOX::VString		fullPath;
		
	if (fURL.GetUniChar(1) == '/') 

		fullPath = fURL;			

	else {

		XBOX::VJSContext	context(fGlobalContext);
		XBOX::VFolder		*folder;
		XBOX::VString		path;		
		
		folder = context.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
		folder->GetPath(path, XBOX::FPS_POSIX);
		XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);
		fullPath = path + fURL;

	}

	XBOX::VFile			file(fullPath, XBOX::FPS_POSIX);
	XBOX::VFileStream	stream(&file);
	XBOX::VError		error;
	XBOX::VString		script;

	if ((error = stream.OpenReading()) == XBOX::VE_OK
	&& (error = stream.GuessCharSetFromLeadingBytes(XBOX::VTC_DefaultTextExport)) == XBOX::VE_OK) {

		stream.SetCarriageReturnMode(XBOX::eCRM_NATIVE);
		error = stream.GetText(script);

	}
	stream.CloseReading();

	bool	isOk;
	
	isOk = false;
	if (error != XBOX::VE_OK) {

		// Fail reading script file, advise parent.

		BroadcastError("Unable to read script file!", fURL, 0);

	} else {

		XBOX::VJSContext	context(fGlobalContext);
		XBOX::VURL			url(fullPath, eURL_NATIVE_STYLE, CVSTR ( "file://" ));
		JS4D::ExceptionRef	exception;
		
		exception = NULL;				
		if (!context.CheckScriptSyntax(script, &url, &exception)) {

			// Syntax error!

			if (exception != NULL) {

				XBOX::VJSValue	value(context); 

				value.SetValueRef((JS4D::ValueRef) exception);
				if (value.IsObject()) {

					XBOX::VJSObject	errorObject(context);
					XBOX::VString	message;
					sLONG			lineNumber;

					errorObject = value.GetObject();

					message = "";
					if (errorObject.HasProperty("message"))

						errorObject.GetPropertyAsString("message", NULL, message);

					lineNumber = 0;
					if (errorObject.HasProperty("line")) {

						bool	ok;

						lineNumber = errorObject.GetPropertyAsLong("line", NULL, &ok);
						xbox_assert(ok);

					}

					BroadcastError(message, fURL, lineNumber);

				}

			}

		} else if (!context.EvaluateScript(script, &url, NULL, &exception, &context.GetGlobalObject())) {

			//** TODO:	Faire la différence entre une exception runtime de type Error 
			//			(avec message + numéro de ligne), et une exception quelconque.

			if (exception != NULL)

				BroadcastError("Uncaught exception!", fURL, 0);

		} else
			
			isOk = true;
					
	}

	if (isOk) {

		XBOX::VJSContext	nullContext((JS4D::ContextRef) NULL);
		
		WaitFor(nullContext, -1, NULL, 0);
		
	}

	delete this;
}

void VJSWorker::_PostMessage (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject)
{
	VJSWorker	*worker	= VJSWorker::GetWorker(ioParms.GetContext());

	xbox_assert(worker->IsDedicatedWorker() && worker->fOnMessagePort != NULL);

	VJSMessagePort::PostMessageMethod(ioParms, worker->fOnMessagePort);
}

#if VERSIONDEBUG

void VJSWorker::_DumpExecutionStatistics ()
{
	XBOX::VTime		currentTime;
	XBOX::VDuration	totalDuration;
	
	currentTime.FromSystemTime();
	currentTime.Substract(fStartTime, totalDuration);
		
	sLONG8	total;
	Real	totalInSeconds, idlePercentage, executionPercentage;
	
	total = totalDuration.GetLong8();	// In milliseconds;
	totalInSeconds = total / 1000.0;
	if (total) {

		idlePercentage = (fTotalIdleDuration.GetLong8() * 100.0) / total;	
		executionPercentage = 100.0 - idlePercentage;
	
	} else 
		
		idlePercentage = executionPercentage = 0.0;
			
	XBOX::VString	message;
	
	message.AppendString("Total execution time: ");
	message.AppendReal(totalInSeconds);
	message.AppendString(" second(s): execution: ");
	message.AppendReal(executionPercentage);
	message.AppendString("% idle: ");
	message.AppendReal(idlePercentage);
	message.AppendString("%\n");
	
	XBOX::VDebugMgr::Get()->DebugMsg(message);
}

#endif
