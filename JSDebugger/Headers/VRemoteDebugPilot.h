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
#ifndef __REMOTE_DEBUG_PILOT_H__
#define __REMOTE_DEBUG_PILOT_H__

#include "KernelIPC/Sources/VSignal.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "HTTPServer/Interfaces/CHTTPServer.h"
#include "VChromeDbgHdlPage.h"


class VRemoteDebugPilot : public XBOX::VObject {

public:
									VRemoteDebugPilot(	CHTTPServer*			inHTTPServer,
														const XBOX::VString		inIP,
														sLONG					inPort);

									~VRemoteDebugPilot() {;}
	
	bool							IsActive() { return (fState == WORKING_STATE); }

	XBOX::VError					Check(WAKDebuggerContext_t inContext);

	void							SetWS(IHTTPWebsocketHandler* inWS) { fWS = inWS; }
	
	XBOX::VError					TreatRemoteDbg(IHTTPResponse* ioResponse);

	XBOX::VError					TreatWS(IHTTPResponse* ioResponse, uLONG inPageNb );

	WAKDebuggerServerMessage*		WaitFrom(WAKDebuggerContext_t inContext);

	XBOX::VError					SendEval( WAKDebuggerContext_t inContext, void* inVars );

	XBOX::VError					SendSource(	WAKDebuggerContext_t inContext,
												intptr_t inSrcId,
												const char *inData,
												int inLength,
												const char* inUrl,
												unsigned int inUrlLen );

	XBOX::VError					SendLookup( WAKDebuggerContext_t inContext, void* inVars, unsigned int inSize );

	XBOX::VError					SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength );

	XBOX::VError					SetState(WAKDebuggerContext_t inContext, WAKDebuggerState_t state);
	
	XBOX::VError					BreakpointReached(
											WAKDebuggerContext_t	inContext,
											int						inLineNumber,
											int						inExceptionHandle,
											char*					inURL,
											int						inURLLength,
											char*					inFunction,
											int 					inFunctionLength,
											char*					inMessage,
											int 					inMessageLength,
											char* 					inName,
											int 					inNameLength,
											long 					inBeginOffset,
											long 					inEndOffset );

	XBOX::VError					NewContext(WAKDebuggerContext_t* outContext) {
		RemoteDebugPilotMsg_t	l_msg;
		*outContext = NULL;
		l_msg.type = NEW_CONTEXT_MSG;
		l_msg.outctx = outContext;
		XBOX::VError l_err = Send( l_msg );
		if (!l_err)
		{
			return l_err;
		}
		xbox_assert(false);//TBC
		return l_err;
	}
	
	XBOX::VError					ContextRemoved(WAKDebuggerContext_t inContext) {
		RemoteDebugPilotMsg_t	l_msg;
		l_msg.type = REMOVE_CONTEXT_MSG;
		l_msg.ctx = inContext;
		return Send( l_msg );
	}


private:
	
	XBOX::VError					ContextUpdated(WAKDebuggerContext_t inContext, XBOX::VSemaphore* inSem, bool* outWait) {
		RemoteDebugPilotMsg_t	l_msg;
		l_msg.type = UPDATE_CONTEXT_MSG;
		l_msg.ctx = inContext;
		l_msg.sem = inSem;
		l_msg.outwait = outWait;
		return Send( l_msg );
	}

#define K_NB_MAX_CTXS		(1024)

typedef enum RemoteDebugPilotMsgType_enum {
		NEW_CONTEXT_MSG=10000,
		REMOVE_CONTEXT_MSG,
		UPDATE_CONTEXT_MSG,
		SET_STATE_MSG,
		TREAT_WS_MSG,
		BREAKPOINT_REACHED_MSG,
		WAIT_FROM_MSG,
		SEND_CALLSTACK_MSG,
		SEND_LOOKUP_MSG,
		SEND_EVAL_MSG,
		TREAT_CLIENT_MSG,
} RemoteDebugPilotMsgType_t;

typedef struct RemoteDebugPilotMsg_st {
	RemoteDebugPilotMsgType_t		type;
	WAKDebuggerContext_t			ctx;
	WAKDebuggerContext_t*			outctx;
	XBOX::VString					url;
	IHTTPResponse*					response;
	WAKDebuggerState_t				state;
	XBOX::VSemaphore*				sem;
	uLONG							pagenb;
	VChromeDbgHdlPage**				outpage;
	XBOX::VSemaphore**				outsem;
	XBOX::VString					datastr;
	XBOX::VString					urlstr;
	int								linenb;
	intptr_t						srcid;
	bool*							outwait;
} RemoteDebugPilotMsg_t;

typedef enum RemoteDebugPilotState_enum {
		STOPPED_STATE,
		STARTED_STATE,
		WORKING_STATE,
} RemoteDebugPilotState_t;

static sLONG InnerTaskProc(XBOX::VTask* inTask);

	XBOX::VCppMemMgr*							fMemMgr;
	IHTTPWebsocketHandler*						fWS;
	XBOX::VTask*								fTask;
	XBOX::VSemaphore							fSem;
	XBOX::VSemaphore							fCompletedSem;
	sBYTE										fData[K_MAX_SIZE];
	RemoteDebugPilotState_t						fState;
	XBOX::VCriticalSection						fPipeLock;
	XBOX::VSemaphore							fPipeInSem;
	XBOX::VSemaphore							fPipeOutSem;
	RemoteDebugPilotMsg_t						fPipeMsg;
	XBOX::VError								fPipeStatus;

	XBOX::VError Send( const RemoteDebugPilotMsg_t& inMsg ) {
		XBOX::VError	l_err;
		
		fPipeLock.Lock();
		fPipeMsg = inMsg;
		fPipeInSem.Unlock();
		fPipeOutSem.Lock();
		l_err = fPipeStatus;
		xbox_assert( l_err == XBOX::VE_OK );
		fPipeLock.Unlock();
		return l_err;
	}
	XBOX::VError Get(RemoteDebugPilotMsg_t* outMsg, bool* outFound, uLONG inTimeoutMs) {
		bool			l_ok;
		XBOX::VError	l_err;

		*outFound = false;
		l_err = XBOX::VE_UNKNOWN_ERROR;

		if (!inTimeoutMs)
		{
			l_ok = fPipeInSem.Lock();
		}
		else
		{
			l_ok = fPipeInSem.Lock((sLONG)inTimeoutMs);
		}
		if (l_ok)
		{
			*outMsg = fPipeMsg;
			*outFound = true;
			l_err = XBOX::VE_OK;
		}
		else
		{
			if (inTimeoutMs)
			{
				l_err = XBOX::VE_OK;
			}
		}
		return l_err;
	}

static XBOX::VError SendToBrowser( VRemoteDebugPilot* inThis, const XBOX::VString& inResp )
	{
		XBOX::VSize	l_size;
		l_size = inResp.GetLength() * 3; /* overhead between VString's UTF_16 to CHAR* UTF_8 */
		if (l_size >= K_MAX_SIZE)
		{
				l_size = K_MAX_SIZE;
				xbox_assert(false);
		}
		l_size = inResp.ToBlock(inThis->fData,l_size,XBOX::VTC_UTF_8,false,false);
		XBOX::VError l_err = inThis->fWS->WriteMessage((void*)inThis->fData,l_size,true);
		return l_err;
	}

#define K_NB_MAX_PAGES		(8)

typedef struct PageDescriptor_st {
	bool				used;
	VChromeDbgHdlPage	value;
} PageDescriptor_t;

	static PageDescriptor_t*						sPages;

};

#endif
