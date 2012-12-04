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
#ifndef __VCHROME_DEBUG_HANDLER__
#define __VCHROME_DEBUG_HANDLER__


#include "KernelIPC/Sources/VSignal.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "HTTPServer/Interfaces/CHTTPServer.h"

#include "ServerNet/VServerNet.h"

#ifdef JSDEBUGGER_EXPORTS
    #define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif




#if 1//defined(WKA_USE_UNIFIED_DBG)


#endif

class VRemoteDebugPilot;

class JSDEBUGGER_API VChromeDebugHandler :
						public IHTTPRequestHandler,
#if 1//defined(WKA_USE_UNIFIED_DBG)
						public IWAKDebuggerServer,
#endif
						public XBOX::VObject
{
public:

	static VChromeDebugHandler*			Get();

	static VChromeDebugHandler*			Get(
											const XBOX::VString inIP,
											sLONG				inPort,
											const XBOX::VString inDebuggerHTMLSkeleton,
											const XBOX::VString inTracesHTMLSkeleton);
	virtual bool						HasClients();
	virtual XBOX::VError				GetPatterns(XBOX::VectorOfVString* outPatterns) const;
	virtual XBOX::VError				HandleRequest(IHTTPResponse* ioResponse);

#if 1//defined(WKA_USE_UNIFIED_DBG)
	virtual WAKDebuggerType_t			GetType();
	virtual int							StartServer();
	virtual int							StopServer();
	virtual short						GetServerPort();
	virtual void						SetSettings( IWAKDebuggerSettings* inSettings );
	virtual void						SetInfo( IWAKDebuggerInfo* inInfo );
	virtual bool						Lock();
	virtual bool						Unlock();
	virtual WAKDebuggerContext_t		AddContext( uintptr_t inContext );
	virtual bool						RemoveContext( WAKDebuggerContext_t inContext );
	virtual bool						SetState(WAKDebuggerContext_t inContext, WAKDebuggerState_t state);
	virtual bool						SendLookup( WAKDebuggerContext_t inContext, void* inVars, unsigned int inSize );
	virtual bool						SendEval( WAKDebuggerContext_t inContext, void* inVars );
	virtual bool						BreakpointReached(
											WAKDebuggerContext_t	inContext,
											int						inLineNumber,
											int						inExceptionHandle = -1/* -1 ? notException : ExceptionHandle */,
											char*					inURL  = NULL,
											int						inURLLength = 0/* in bytes */,
											char*					inFunction = NULL,
											int 					inFunctionLength = 0 /* in bytes */,
											char*					inMessage = NULL,
											int 					inMessageLength = 0 /* in bytes */,
											char* 					inName = NULL,
											int 					inNameLength = 0 /* in bytes */,
											long 					inBeginOffset = 0,
											long 					inEndOffset = 0 /* in bytes */ );
	virtual void						SetSourcesRoot( char* inRoot, int inLength );
	virtual char*						GetAbsolutePath (
											const unsigned short* inAbsoluteRoot, int inRootSize,
											const unsigned short* inRelativePath, int inPathSize,
											int& outSize );
	virtual char*						GetRelativeSourcePath (
											const unsigned short* inAbsoluteRoot, int inRootSize,
											const unsigned short* inAbsolutePath, int inPathSize,
											int& outSize );
	virtual void						Reset();
	virtual void						WakeUpAllWaiters();
	virtual bool						SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength );
	virtual bool						SendSource( WAKDebuggerContext_t inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen );
	virtual WAKDebuggerServerMessage*	WaitFrom(WAKDebuggerContext_t inContext);
	virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage);
	virtual WAKDebuggerServerMessage*	GetNextBreakPointCommand();
	virtual WAKDebuggerServerMessage*	GetNextSuspendCommand( WAKDebuggerContext_t inContext );
	virtual WAKDebuggerServerMessage*	GetNextAbortScriptCommand ( WAKDebuggerContext_t inContext );
	virtual long long					GetMilliseconds();

	virtual WAKDebuggerUCharPtr_t		EscapeForJSON( const unsigned char* inString, int inSize, int& outSize );
	virtual void						DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr );

	virtual void*						UStringToVString( const void* inString, int inSize );

	virtual int							Write ( const char * inData, long inLength, bool inToUTF8 = false );
	virtual int							WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize );
	virtual int							WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize );

	virtual void						Trace(	WAKDebuggerContext_t	inContext,
												const void*				inString,
												int						inSize,
												WAKDebuggerTraceLevel_t inTraceLevel = WAKDBG_ERROR_LEVEL );

	// to be removed after clean start/stop of dbgr server
	static void							StaticSetSettings( IWAKDebuggerSettings* inSettings );
private:
										VChromeDebugHandler();
										VChromeDebugHandler(
											const XBOX::VString inIP, sLONG inPort, const XBOX::VString inDebuggerHTML, const XBOX::VString inTracesHTML);
	virtual								~VChromeDebugHandler();
										VChromeDebugHandler( const VChromeDebugHandler& inChromeDebugHandler);

	XBOX::VError						TreatJSONFile(IHTTPResponse* ioResponse);
	XBOX::VError						TreatDebuggerHTML(IHTTPResponse* ioResponse);
	XBOX::VError						TreatTracesHTML(IHTTPResponse* ioResponse);

	XBOX::VError						TreatRemoteTraces(IHTTPResponse* ioResponse);

	bool								fStarted;
	VRemoteDebugPilot*					fPilot;
	sLONG								fPort;
	XBOX::VString						fIPAddr;
	XBOX::VString						fDebuggerHTML;
	XBOX::VString						fTracesHTML;
	//IWAKDebuggerSettings*				fDebuggerSettings;

#endif	//WKA_USE_UNIFIED_DBG

	static								VChromeDebugHandler*			sDebugger;
	static								XBOX::VCriticalSection			sDbgLock;
	// to be removed after clean start/stop of dbgr server
	static								IWAKDebuggerSettings*			sStaticSettings;

//#define UNIFIED_DEBUGGER_NET_DEBUG

};

#endif

