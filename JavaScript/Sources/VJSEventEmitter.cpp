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

#include "VJSEventEmitter.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

void VJSEventEmitter::AddListener (const XBOX::VString &inEventName, XBOX::VJSObject inCallback, bool inOnce)
{
	// Note: That it is possible to add a same function several times for a single event.

	SListener	newListener;
/*
	XBOX::DebugMsg("VJSEventEmitter:AddListener(): context %lu: objectref: %lu\n", 
		(unsigned long) inCallback.GetContextRef(), (unsigned long) inCallback.GetObjectRef());
*/
	JS4D::ProtectValue(inCallback.GetContextRef(), inCallback.GetObjectRef());

	newListener.fOnce = inOnce;
	newListener.fCallback = inCallback.GetObjectRef();
	newListener.fContext= inCallback.GetContextRef();
	xbox_assert(newListener.fCallback != NULL);

	if (fAllListeners.find(inEventName) == fAllListeners.end()) {

		SListenerList	newListenerList;

		newListenerList.push_back(newListener);
		fAllListeners.insert(std::pair<XBOX::VString,SListenerList>(inEventName, newListenerList));

	} else

		fAllListeners[inEventName].push_back(newListener);

	if (++fNumberListeners > fMaximumListeners) {

		// TODO: See what NodeJS does exactly (console.warning("too many listeners") ?).

	}

	// TODO: Queue a newListener event. Or not! Is this feature useful?
}

bool VJSEventEmitter::HasListener (const XBOX::VString &inEventName)
{
	if (fAllListeners.find(inEventName) == fAllListeners.end())

		return false;

	else 

		return !fAllListeners[inEventName].empty();
}

void VJSEventEmitter::Emit (XBOX::VJSContext inContext, const XBOX::VString &inEventName, std::vector<XBOX::VJSValue> *inArguments)
{
	xbox_assert(inArguments != NULL);

	std::list<XBOX::JS4D::ObjectRef>	callbackList;
	
	if (_getCallbacks(inEventName, &callbackList, true)) {

		XBOX::VJSObject								globalObject	= inContext.GetGlobalObject();
		XBOX::VJSObject								callbackObject(inContext);
		std::list<XBOX::JS4D::ObjectRef>::iterator	i;

		for (i = callbackList.begin(); i != callbackList.end(); i++) {

//				XBOX::DebugMsg("VJSEventEmitter::emit() context %lu\n", (unsigned long) callbackObject.GetContextRef());

			callbackObject.SetObjectRef(*i);
			globalObject.CallFunction(callbackObject, inArguments, NULL, NULL);

		}

	}
}

VJSEventEmitter::VJSEventEmitter ()
{
	fMaximumListeners = VJSEventEmitter::kDefaultMaximumListeners;
	fNumberListeners = 0;
	fAllListeners.clear();
}

VJSEventEmitter::~VJSEventEmitter ()
{
	SEventMap::iterator		it1;
	SListenerList::iterator	it2;

	for (it1 = fAllListeners.begin(); it1 != fAllListeners.end(); it1++) {

		for (it2 = fAllListeners[it1->first].begin(); it2 != fAllListeners[it1->first].end(); it2++) {

			;	
	
/*
			XBOX::DebugMsg("~VJSEventEmitter() context %lu: objectref: %lu\n", 
				(unsigned long) it2->fContext, (unsigned long) it2->fCallback);

*/

		// Do not do unprotect, the heap has always been cleaned-up by garbage collector.
			
		//	XBOX::JS4D::UnprotectValue(it2->fContext, it2->fCallback);

		}

	}

}

void VJSEventEmitter::SetupEventEmitter (const XBOX::VJSParms_initialize &inParms)
{
	XBOX::VJSObject	callbackObject(inParms.GetContext());
	XBOX::VJSObject	constructedObject(inParms.GetObject());
	
	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _addListener>);
	constructedObject.SetProperty("addListener", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _on>);
	constructedObject.SetProperty("on", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _once>);
	constructedObject.SetProperty("once", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _removeListener>);
	constructedObject.SetProperty("removeListener", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _removeAllListeners>);
	constructedObject.SetProperty("removeAllListeners", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _setMaxListeners>);
	constructedObject.SetProperty("setMaxListeners", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _listeners>);
	constructedObject.SetProperty("listeners", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);

	callbackObject.MakeCallback(XBOX::js_callback<VJSEventEmitter, _emit>);
	constructedObject.SetProperty("emit", callbackObject, XBOX::JS4D::PropertyAttributeDontDelete);
}

void VJSEventEmitter::_addListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);
	_add(ioParms, inEventEmitter, false);
}

void VJSEventEmitter::_on (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);
	_addListener(ioParms, inEventEmitter);
}

void VJSEventEmitter::_once (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);
	_add(ioParms, inEventEmitter, true);
}

void VJSEventEmitter::_removeListener (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;
	XBOX::VJSObject	callbackObject(ioParms.GetContext());

	if (ioParms.CountParams() < 2
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName)
	|| !ioParms.IsObjectParam(2) || !ioParms.GetParamObject(2, callbackObject) || !callbackObject.IsFunction()) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	// If the event has no listener at all, or if the callback object isn't found in the listener list, do nothing. 

	if (inEventEmitter->fAllListeners.find(eventName) == inEventEmitter->fAllListeners.end())

		return;

	// To remove callback in a LIFO way, loop twice to find the last one inserted (reverse_iterator doesn't work with erase()).

	XBOX::JS4D::ObjectRef	callbackObjectRef;
	SListenerList			*listenerList;
	SListenerList::iterator	i;
	sLONG					j, k;

	callbackObjectRef = callbackObject.GetObjectRef();
	listenerList = &inEventEmitter->fAllListeners[eventName];
	for (i = listenerList->begin(), j = 0; i != listenerList->end(); i++) 

		if (i->fCallback == callbackObjectRef) 

			j++;

	if (j)

		for (i = listenerList->begin(), k = 0; i != listenerList->end(); i++) 

			if (++k == j) {

				XBOX::JS4D::UnprotectValue(ioParms.GetContextRef(), i->fCallback);

				listenerList->erase(i);
				xbox_assert(inEventEmitter->fNumberListeners);
				inEventEmitter->fNumberListeners--;
				break;

			}
}

void VJSEventEmitter::_removeAllListeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	SEventMap::iterator		it1;
	SListenerList::iterator	it2;

	// If no event name argument, remove all listeners for all events.

	if (!ioParms.CountParams()) {

		for (it1 = inEventEmitter->fAllListeners.begin(); it1 != inEventEmitter->fAllListeners.end(); it1++)

			for (it2 = inEventEmitter->fAllListeners[it1->first].begin(); it2 != inEventEmitter->fAllListeners[it1->first].end(); it2++)
			
				XBOX::JS4D::UnprotectValue(ioParms.GetContextRef(), it2->fCallback);

		inEventEmitter->fAllListeners.clear();
		inEventEmitter->fNumberListeners = 0;

	} else {

		XBOX::VString	eventName;

		if (ioParms.CountParams() < 1
		|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName))	{

			XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
			return;

		}

		if ((it1 = inEventEmitter->fAllListeners.find(eventName)) == inEventEmitter->fAllListeners.end())

			return;

		sLONG	numberListeners;

		numberListeners = (sLONG) it1->second.size();

		for (it2 = inEventEmitter->fAllListeners[it1->first].begin(); it2 != inEventEmitter->fAllListeners[it1->first].end(); it2++)
			
			XBOX::JS4D::UnprotectValue(ioParms.GetContextRef(), it2->fCallback);

		inEventEmitter->fAllListeners.erase(it1);

		xbox_assert(inEventEmitter->fNumberListeners >= numberListeners);
		inEventEmitter->fNumberListeners -= numberListeners;	

	}
}

void VJSEventEmitter::_setMaxListeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	sLONG	maximum;

	if (ioParms.CountParams() < 1
	|| !ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &maximum) || maximum < 0) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	inEventEmitter->fMaximumListeners = maximum;
}

void VJSEventEmitter::_listeners (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;

	if (ioParms.CountParams() < 1
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName))	{

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	XBOX::VJSArray						arrayObject(ioParms.GetContext());
	std::list<XBOX::JS4D::ObjectRef>	callbackList;
	
	if (inEventEmitter->_getCallbacks(eventName, &callbackList, false)) {

		std::list<XBOX::JS4D::ObjectRef>::iterator	i;

		for (i = callbackList.begin(); i != callbackList.end(); i++) {

			XBOX::VJSValue	value(ioParms.GetContextRef(), (XBOX::JS4D::ValueRef) *i);

			arrayObject.PushValue(value);

		}

	}	

	ioParms.ReturnValue(arrayObject);
}

void VJSEventEmitter::_emit (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;

	if (ioParms.CountParams() < 1
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName))	{

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	std::vector<XBOX::VJSValue>	arguments;
	sLONG						i;

	for (i = 2; i <= ioParms.CountParams(); i++) 

		arguments.push_back(ioParms.GetParamValue(i));

	inEventEmitter->Emit(ioParms.GetContext(), eventName, &arguments);
}

void VJSEventEmitter::_add (XBOX::VJSParms_callStaticFunction &ioParms, VJSEventEmitter *inEventEmitter, bool inOnce)
{
	xbox_assert(inEventEmitter != NULL);

	XBOX::VString	eventName;
	XBOX::VJSObject	callbackObject(ioParms.GetContext());

	if (ioParms.CountParams() < 2
	|| !ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, eventName)
	|| !ioParms.IsObjectParam(2) || !ioParms.GetParamObject(2, callbackObject) || !callbackObject.IsFunction()) 

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		
	else

		inEventEmitter->AddListener(eventName, callbackObject, inOnce);
}

sLONG VJSEventEmitter::_getCallbacks (const XBOX::VString &inEventName, std::list<XBOX::JS4D::ObjectRef> *outList, bool inRemoveOnce)
{
	xbox_assert(outList != NULL && outList->empty());

	if (fAllListeners.find(inEventName) == fAllListeners.end())

		return 0;

	SListenerList			*listeners	= &fAllListeners[inEventName];
	SListenerList::iterator	i;
	sLONG					n;

	if (inRemoveOnce) 

		for (i = listeners->begin(), n = 0; i != listeners->end(); n++) {

/*			XBOX::DebugMsg("VJSEventEmitter::getcallback(): context %lu: objectref: %lu\n", 
				(unsigned long) i->fContext, (unsigned long) i->fCallback); */


			outList->push_back(i->fCallback);
			if (i->fOnce) {

				i = listeners->erase(i);
				xbox_assert(fNumberListeners);
				fNumberListeners--;

			} else

				i++;

		}

	else 

		for (i = listeners->begin(), n = 0; i != listeners->end(); i++, n++) {
/*
			XBOX::DebugMsg("VJSEventEmitter::getcallback(): context %lu: objectref: %lu\n", 
				(unsigned long) i->fContext, (unsigned long) i->fCallback);
*/
			outList->push_back(i->fCallback);

		}

	return n;
}