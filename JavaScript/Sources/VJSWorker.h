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
#ifndef __VJS_WORKER__
#define __VJS_WORKER__

#include <list>

#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSTimer.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSMessagePort;
class IJSEvent;
class IJSEventGenerator;
class VJSLocalFileSystem;

class XTOOLBOX_API IJSWorkerDelegate
{
public:

	virtual	VJSGlobalContext*	RetainJSContext (VError& outError, const VJSContext& inParentContext, bool inReusable) = 0;
	virtual	VError				ReleaseJSContext (VJSGlobalContext* inContext) = 0;
};

// Worker execution.

class XTOOLBOX_API VJSWorker : public XBOX::VObject
{
public:

	// The caller is responsible for the destruction of the delegate

	static void			SetDelegate (IJSWorkerDelegate *inDelegate);

	// Create a dedicated worker.

						VJSWorker (XBOX::VJSContext &inParentContext, const XBOX::VString &inURL, bool inReUseContext);
						
	// If shared worker with given URL and name exists, return a pointer to it. If not, create it.

	static VJSWorker	*GetSharedWorker (XBOX::VJSContext &inParentContext,
											const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext, 
											bool *outCreatedWorker);

	// For dedicated workers only, set message port for "onmessage"/postMessage() and "onerror".
 
	void				SetMessagePorts (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort);
	
	// For shared workers only. Connect() will queue an event to trigger the "onconnect" callback, allowing to transfer communication ports.

	void				Connect (VJSMessagePort *inMessagePort, VJSMessagePort *inErrorPort);

	// Add an error port, allowing to report errors back to "outside" worker(s).

	void				AddErrorPort (VJSMessagePort *inErrorPort);
	
	// Send an error event to all error port(s), only one (toward parent) for dedicated workers.

	void				BroadcastError (const XBOX::VString &inMessage, const XBOX::VString &inFileNumber, sLONG inLineNumber);
	
	// Launch worker thread. fGlobalContext must have been set properly.
	
	void				Run ();

	// Process events during inWaitingDuration milliseconds. Zero millisecond means "polling": execute all pending events but do not wait. 
	// Use a negative value for no limit in duration. Set inEventGenerator and inEventType to the event generator object and event type to
	// match. Set inEventGenerator to NULL if no matching is needed. 
	// Return 1 if terminated (close flag is set), return 0 if waiting time elapsed, or return -1 if an event has been matched.
	// Worker threads should use a NULL JSContextRef (XBOX::VJSContext((JS4D::ContextRef) NULL)), WaitFor() will then use fGlobalContext 
	// to create the actual context used to process the events.
		
	sLONG				WaitFor (XBOX::VJSContext &inContext, sLONG inWaitingDuration, IJSEventGenerator *inEventGenerator, sLONG inEventType);

	// Set closing flag to true, this will unconditionaly terminate worker. Can be called several times, as long as object exists.

	void				Terminate ();		
	static void			TerminateAll ();	

	// Retrieve worker from context (then get it from global object). If VJSWorker doesn't exist ("root" javascript execution"), 
	// create one so it can receive message from other workers. Hence, GetWorker() never return NULL.

	static VJSWorker	*GetWorker (const XBOX::VJSContext &inContext);

	// Queue a message on worker's message queue.

	void				QueueEvent (IJSEvent *inEvent);

	// A timer can call UnscheduleTimer() on itself during the triggered callback.
	// Because the timer event is removed from queue before triggering it, this will
	// do nothing.
	
	void				UnscheduleTimer (VJSTimer *inTimer);

	// Add or remove a message port.

	void				AddMessagePort (VJSMessagePort *inMessagePort);
	void				RemoveMessagePort (VJSMessagePort *inMessagePort);

	// Accessors.
	
	bool				IsDedicatedWorker ()	{	return fIsDedicatedWorker;		}

	VJSTimerContext		*GetTimerContext ()		{	return &fTimerContext;			}

	bool				IsInsideWaitFor ()		{	return fInsideWaitCount > 0;	}

	// Global object's methods.

	static void			Close (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void			Wait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	static void			ExitWait (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	
	VJSLocalFileSystem	*GetLocalFileSystem ()	{	return fLocalFileSystem;	}

	// Return wait duration for wait() methods. Negative means infinite duration. 
	// Zero just polling, execute all pending events. Positive, the value to wait 
	// in milliseconds.

	static sLONG		GetWaitDuration (VJSParms_callStaticFunction &ioParms);

private:

	static const uLONG				kSpecificKey	= ('J' << 24) | ('S' << 16) | ('W' << 8) | 'W'; 
	
	static XBOX::VCriticalSection	sMutex;				
	static std::list<VJSWorker *>	sDedicatedWorkers;
	static std::list<VJSWorker *>	sSharedWorkers;
	static std::list<VJSWorker *>	sRootWorkers;				// Several "root" workers can run simultaneously.
	static IJSWorkerDelegate		*sDelegate;

	XBOX::VCriticalSection			fMutex;
	XBOX::VTask						*fVTask;

	XBOX::VJSGlobalContext			*fGlobalContext;			// "Child" (dedicated or shared) workers only. 

	bool							fClosingFlag, fExitWaitFlag;
	XBOX::VSyncEvent				fSyncEvent;
	bool							fIsLockedWaiting;			// True if locked waiting on fSyncEvent.
	std::list<IJSEvent *>			fEventQueue;		
	std::list<VJSMessagePort *>		fMessagePorts;				// List of all message ports (any type).
	VJSTimerContext					fTimerContext;				// All timers.
	
	bool							fIsDedicatedWorker;	
	XBOX::VString					fURL, fName;				// URL and name make an unique couple, which identify each shared worker.

	VJSMessagePort					*fOnMessagePort;			// "onmessage" and postMessage() (dedicated workers only).	
	VJSMessagePort					*fConnectionPort;			// "onconnect" (shared workers only).

	std::list<VJSMessagePort *>		fErrorPorts;				// List of error ports.

	sLONG							fInsideWaitCount;			// Count recursive wait() calls.
	XBOX::VTime						fStartTime;	
	XBOX::VDuration					fTotalIdleDuration;

	VJSLocalFileSystem				*fLocalFileSystem;

	// Create a shared worker.

					VJSWorker (XBOX::VJSContext &inParentContext, const XBOX::VString &inURL, const XBOX::VString &inName, bool inReUseContext);

	// Private constructor, construct an "empty" worker for "root" javascript execution.

					VJSWorker (const XBOX::VJSContext &inContext);
	virtual			~VJSWorker ();

	void			_AddMessagePort (std::list<VJSMessagePort *> *ioMessagePorts, VJSMessagePort *inMessagePort);
	void			_RemoveMessagePort (std::list<VJSMessagePort *> *ioMessagePorts, VJSMessagePort *inMessagePort);

	// Add worker attributes and functions to global object.

	void			_PopulateGlobalObject (XBOX::VJSContext inContext);

	// Load script, evaluate and execute it, then service events.

	static sLONG	_RunProc (XBOX::VTask *inVTask);
	void			_DoRun ();

	// Dedicated workers only.
	
	static void		_PostMessage (VJSParms_callStaticFunction &ioParms, VJSGlobalObject *inGlobalObject);
	
#if VERSIONDEBUG
	
	// Debug information, should make those data available from JS runtime.
	
	void			_DumpExecutionStatistics ();
	
#endif
};

END_TOOLBOX_NAMESPACE

#endif