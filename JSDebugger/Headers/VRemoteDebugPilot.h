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
									VRemoteDebugPilot(	CHTTPServer*			inHTTPServer);

									~VRemoteDebugPilot() {;}
	
			void					GetStatus(bool&	outPendingContexts,bool& outConnected,sLONG& outDebuggingEventsTimeStamp);

			bool					IsActive() {
										return (fState == STARTED_STATE || fState == CONNECTING_STATE || fState == CONNECTED_STATE); }

			void					SetWS(IHTTPWebsocketHandler* inWS) { fWS = inWS; }
	
			XBOX::VError			TreatRemoteDbg(IHTTPResponse* ioResponse, bool inNeedsAuthentication);

			XBOX::VError			TreatWS(IHTTPResponse* ioResponse, uLONG inPageNb );

			void					SetSolutionName(const XBOX::VString& inSolutionName) {fSolutionName = inSolutionName;}

			WAKDebuggerServerMessage*		WaitFrom(OpaqueDebuggerContext inContext);

			XBOX::VError			SendEval( OpaqueDebuggerContext inContext, const XBOX::VString& inEvaluationResult, const XBOX::VString& inRequestId );

			XBOX::VError			SendLookup( OpaqueDebuggerContext inContext, const XBOX::VString& inLookupResult );

			XBOX::VError			SendCallStack(
											OpaqueDebuggerContext				inContext,
											const XBOX::VString&				inCallstackStr,
											const XBOX::VString&				inExceptionInfosStr,
											const CallstackDescriptionMap&		inCallstackDesc);
	
			XBOX::VError			BreakpointReached(
											OpaqueDebuggerContext		inContext,
											int							inLineNumber,
											RemoteDebuggerPauseReason	inDebugReason,
											XBOX::VString&				inException,
											XBOX::VString&				inSourceUrl,
											XBOX::VectorOfVString&		inSourceData);

			XBOX::VError			NewContext(OpaqueDebuggerContext* outContext);
	
			XBOX::VError			ContextRemoved(OpaqueDebuggerContext inContext);

			XBOX::VError			DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly );

			XBOX::VError			Start();

			XBOX::VError			Stop();
			
			XBOX::VError			AbortAll();

private:
			XBOX::VString						fSolutionName;
			XBOX::VSemaphore					fPendingContextsNumberSem;
			sLONG								fDebuggingEventsTimeStamp;

	class VContextDebugInfos : public XBOX::VObject {
	public:
		VContextDebugInfos() : fLineNb(0) {;} 
		sLONG					fLineNb;
		XBOX::VString			fFileName;
		XBOX::VString			fReason;
		XBOX::VString			fComment;
		XBOX::VString			fSourceLine;
	};

	virtual XBOX::VError			ContextUpdated(	OpaqueDebuggerContext		inContext,
													const VContextDebugInfos&	inDebugInfos,
													XBOX::VSemaphore**			outSem);

#define K_NB_MAX_CTXS		(1024)

typedef enum RemoteDebugPilotMsgType_enum {
		NEW_CONTEXT_MSG=10000,
		REMOVE_CONTEXT_MSG,
		DEACTIVATE_CONTEXT_MSG,
		UPDATE_CONTEXT_MSG,
		TREAT_WS_MSG,
		BREAKPOINT_REACHED_MSG,
		WAIT_FROM_MSG,
		TREAT_NEW_CLIENT_MSG,
		START_MSG,
		STOP_MSG,
		ABORT_ALL_MSG,
} RemoteDebugPilotMsgType_t;

typedef struct RemoteDebugPilotMsg_st {
	RemoteDebugPilotMsgType_t		type;
	OpaqueDebuggerContext			ctx;
	OpaqueDebuggerContext*			outctx;
	XBOX::VString					url;
	IHTTPResponse*					response;
	WAKDebuggerState_t				state;
	uLONG							pagenb;
	VChromeDbgHdlPage**				outpage;
	XBOX::VSemaphore**				outsem;
	XBOX::VString					datastr;
	XBOX::VectorOfVString			datavectstr;
	XBOX::VString					urlstr;
	int								linenb;
	intptr_t						srcid;
	bool							needsAuthentication;
	bool							hideOnly;
	VContextDebugInfos				debugInfos;
} RemoteDebugPilotMsg_t;

// the order of declaration is important: STPPED then STARTED then all the others states which are sub-states of STARTED
typedef enum RemoteDebugPilotState_enum {
		// pilot is stopped, waits to be started
		STOPPED_STATE,
		// pilot is started, waits for a (browser) client connection
		STARTED_STATE,
		// pilot waits connected client disconnection
		DISCONNECTING_STATE,
		// client has created a WS to communicate but connexion os not complete (e.g. authentication needed)
		CONNECTING_STATE,
		// pilot is working for a connected client, treats the requests coming through the WS
		CONNECTED_STATE
} RemoteDebugPilotState_t;

	static	sLONG					InnerTaskProc(XBOX::VTask* inTask);

	#define K_NB_MAX_PAGES		(16)

	class VContextDescriptor : public XBOX::VObject {
	public:
		VContextDescriptor() : fSem(NULL) , fPage(NULL), fUpdated(false) {;}
		~VContextDescriptor() {
			fUpdated = false;
			//testAssert( (void*)fSem == NULL );
			//ReleaseRefCountable(&fSem);
			(void*)fPage;
		};
		XBOX::VSemaphore*		fSem;
		VChromeDbgHdlPage*		fPage;
		bool					fUpdated;
		VContextDebugInfos		fDebugInfos;
	};

	// perform an hysteresis on status to compensate WAKstudio polling mecanism
	class VStatusHysteresis : public XBOX::VObject {
	public:
		VStatusHysteresis();
		void					Get(bool& ioConnected);
	private:
		XBOX::VCriticalSection	fLock;
		XBOX::VTime				fLastConnectedTime;
	};

			XBOX::VCppMemMgr*							fMemMgr;
			IHTTPWebsocketHandler*						fWS;
			XBOX::VTask*								fTask;
			XBOX::VSemaphore							fUniqueClientConnexionSemaphore;
			XBOX::VSemaphore							fClientCompletedSemaphore;
			RemoteDebugPilotState_t						fState;
			XBOX::VCriticalSection						fPipeLock;
			XBOX::VSemaphore							fPipeInSem;
			XBOX::VSemaphore							fPipeOutSem;
			RemoteDebugPilotMsg_t						fPipeMsg;
			XBOX::VError								fPipeStatus;
			XBOX::VTime									fDisconnectTime;
			bool										fNeedsAuthentication;
			std::map< OpaqueDebuggerContext, VContextDescriptor>	fContextArray;
			sBYTE*										fTmpData;
			XBOX::VString								fClientId;
			CHTTPServer*								fHTTPServer;
			VStatusHysteresis							fStatusHysteresis;
			XBOX::VCriticalSection						fLock;

			XBOX::VError					Send( const RemoteDebugPilotMsg_t& inMsg );

			XBOX::VError					Get(RemoteDebugPilotMsg_t& outMsg, bool& outFound, uLONG inTimeoutMs);

			XBOX::VError					SendToBrowser( const XBOX::VString& inResp );
	
			void							DisconnectBrowser();

			void							RemoveContext(std::map< OpaqueDebuggerContext, VContextDescriptor >::iterator&	inCtxIt);

			void							TreatStartMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatAbortAllMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatStopMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatNewClientMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatNewContextMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatRemoveContextMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatDeactivateContextMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatUpdateContextMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatTreatWSMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatBreakpointReachedMsg(RemoteDebugPilotMsg_t& ioMsg);
			void							TreatWaitFromMsg(RemoteDebugPilotMsg_t& ioMsg);

			void							HandleDisconnectingState();
			void							HandleConnectedState();
			void							HandleConnectingState();


			XBOX::VError					SendContextToBrowser(
														std::map< OpaqueDebuggerContext,VContextDescriptor >::iterator& inCtxIt);

			void							AbortContexts(bool inOnlyDebuggedOnes);

			void DisplayContexts() {
				XBOX::VString	K_EMPTY("empty");
				XBOX::DebugMsg("\ndisplay ctxs\n");
				std::map< OpaqueDebuggerContext, VContextDescriptor >::iterator	ctxIt = fContextArray.begin();
				while( ctxIt != fContextArray.end() )
				{
					sLONG8 id = (sLONG8)((*ctxIt).first);
					int updated = 0;
					if ((*ctxIt).second.fUpdated)
					{
						updated = 1;
					}
					XBOX::VString tmp = (*ctxIt).second.fDebugInfos.fFileName;
					if (tmp.GetLength() <= 0)
					{
						XBOX::DebugMsg("  id->%lld , name=empty, updated=%d\n",
								id,
								updated );
					}
					else
					{
						XBOX::DebugMsg("  id->%lld , name=%S, updated=%d\n",
								id,
								&tmp,
								updated );
					}
					ctxIt++;
				}
				XBOX::DebugMsg("\n\n");
			}

	static	sLONG							sRemoteDebugPilotContextId;

};

#endif
