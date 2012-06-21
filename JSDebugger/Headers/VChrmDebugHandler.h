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
#ifndef __VCHRM_DEBUG_HANDLER__
#define __VCHRM_DEBUG_HANDLER__

#include "KernelIPC/Sources/VSignal.h"
#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "HTTPServer/Interfaces/CHTTPServer.h"

#ifdef JSDEBUGGER_EXPORTS
    #define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif

#define K_MAX_SIZE					(4096)


#if defined(WKA_USE_CHR_REM_DBG)

typedef enum ChrmDbgMsgType_enum {
		NO_MSG,
		SEND_CMD_MSG,
		CALLSTACK_MSG,
		LOOKUP_MSG,
		EVAL_MSG,
		SET_SOURCE_MSG,
		BRKPT_REACHED_MSG
} ChrmDbgMsgType_t;
typedef struct ChrmDbgMsgData_st {
		XBOX::VString	_urlStr;
		XBOX::VString	_dataStr;
		WAKDebuggerServerMessage				Msg;
} ChrmDbgMsgData_t;
typedef struct ChrmDbgMsg_st {
	ChrmDbgMsgType_t				type;
	ChrmDbgMsgData_t				data;
} ChrmDbgMsg_t;

class ChrmDbgHdlPage	{

public:
class ChrmDbgFifo	{
public:
	std::queue<ChrmDbgMsg_t>		fMsgs;
	ChrmDbgFifo();
	void							Reset();
	XBOX::VError					Put(ChrmDbgMsg_t inMsg);
	// NULL timeout means infinite, if timeout and nothing returned type is NO_MSG
	XBOX::VError					Get(ChrmDbgMsg_t* outMsg, uLONG inTimeoutMs);
	/*
	XBOX::VError					Put(ChrmDbgMsg_t inMsg);
	// NULL timeout means infinite
	bool							Get(ChrmDbgMsg_t* outMsg, uLONG inTimeoutMs);*/
private:

#define K_NB_MAX_MSGS	(4)
	XBOX::VCriticalSection			fLock;
	XBOX::VSemaphore				fSem;
};
	ChrmDbgHdlPage();
	virtual							~ChrmDbgHdlPage();

	void							Init(uLONG inPageNb,CHTTPServer* inHTTPSrv,XBOX::VCppMemMgr* inMemMgr);
	XBOX::VError					CheckInputData();

	XBOX::VError					TreatMsg();
	XBOX::VError					SendResult(const XBOX::VString& inValue);
	XBOX::VError					SendMsg(const XBOX::VString& inValue);
	XBOX::VError					TreatPageReload(XBOX::VString* str);
	XBOX::VError					SendDbgPaused(XBOX::VString* str);
	XBOX::VError					TreatWS(IHTTPResponse* ioResponse);
	XBOX::VError					GetBrkptParams(char* inStr, char** outUrl, char** outColNb, char **outLineNb);
	uLONG							GetPageNb() {return fPageNb;};
	void							Clear() {fSource.clear();};
	bool							fActive;// true when chrome displays the page
	bool							fUsed;// true when the page is fed with 4D debug data
	IHTTPWebsocketHandler*			fWS;
	WAKDebuggerState_t				fState;
	ChrmDbgFifo						fFifo;
	ChrmDbgFifo						fOutFifo;

private:
	char							fMsgData[K_MAX_SIZE];
	uLONG							fMsgLen;
	bool							fIsMsgTerminated;
	int								fOffset; 
	XBOX::VString					fMethod;
	char*							fParams;
	char*							fId;
	uLONG8							fBodyNodeId;
	XBOX::VCppMemMgr*				fMemMgr;
	uLONG							fPageNb;
	XBOX::VectorOfVString			fSource;
	intptr_t						fSrcId;
	int								fLineNb;
	bool							fSrcSent;
	XBOX::VString					fFileName;
	XBOX::VString					fURL;

};
#endif

class JSDEBUGGER_API VChrmDebugHandler :
						public IHTTPRequestHandler,
#if defined(WKA_USE_CHR_REM_DBG)
						public IWAKDebuggerServer,
#endif
						public XBOX::VObject
{
public:

static VChrmDebugHandler*				Get();
	virtual bool						HasClients();
	virtual XBOX::VError				GetPatterns(XBOX::VectorOfVString* outPatterns) const;
	virtual XBOX::VError				HandleRequest(IHTTPResponse* ioResponse);

#if defined(WKA_USE_CHR_REM_DBG)
	virtual WAKDebuggerType_t			GetType();
	virtual int							StartServer();
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
	virtual char* GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize );
	virtual char* GetRelativeSourcePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inAbsolutePath, int inPathSize,
									int& outSize );
	virtual bool						SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength );
	virtual bool						SendSource( WAKDebuggerContext_t inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen );
	virtual WAKDebuggerServerMessage*	WaitFrom(WAKDebuggerContext_t inContext);
	virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage);
	virtual WAKDebuggerServerMessage* GetNextBreakPointCommand();
	virtual WAKDebuggerServerMessage* GetNextSuspendCommand( WAKDebuggerContext_t inContext );
	virtual WAKDebuggerServerMessage* GetNextAbortScriptCommand ( WAKDebuggerContext_t inContext );
	virtual long long					GetMilliseconds();

	virtual WAKDebuggerUCharPtr_t		EscapeForJSON( const unsigned char* inString, int inSize, int& outSize );
	virtual void						DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr );

	virtual void*						UStringToVString( const void* inString, int inSize );

	virtual int							Write ( const char * inData, long inLength, bool inToUTF8 = false );
	virtual int							WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize );
	virtual int							WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize );

	virtual void						Trace(const void* inString, int inSize );


private:
	VChrmDebugHandler();
	virtual							~VChrmDebugHandler();

	XBOX::VError					TreatJSONFile(IHTTPResponse* ioResponse);

#define K_NB_MAX_PAGES		(4)
	ChrmDbgHdlPage					fPages[K_NB_MAX_PAGES];
	XBOX::VCriticalSection			fLock;
	XBOX::VCppMemMgr*				fMemMgr;

#endif	//WKA_USE_CHR_REM_DBG

static VChrmDebugHandler*			sDebugger;
static XBOX::VCriticalSection		sDbgLock;

};

#endif

