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

class ChrmDbgHdlPage	{

public:
	ChrmDbgHdlPage();
	virtual							~ChrmDbgHdlPage();

	void							Init(uLONG inPageNb,CHTTPServer* inHTTPSrv,XBOX::VCppMemMgr* inMemMgr);
	XBOX::VError					CheckInputData();

	XBOX::VError					TreatMsg();
	XBOX::VError					SendResult(const XBOX::VString& inValue);
	XBOX::VError					SendMsg(const XBOX::VString& inValue);
	XBOX::VError					TreatPageReload();
	XBOX::VError					TreatWS(IHTTPResponse* ioResponse);
	uLONG							GetPageNb() {return fPageNb;};
	bool							fActive;
	bool							fUsed;
	XBOX::VectorOfVString			fSource;
	IHTTPWebsocketHandler*			fWS;
	int								fLineNb;
	XBOX::VString					fURL;
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

};

class JSDEBUGGER_API VChrmDebugHandler : public IHTTPRequestHandler, public IJSWChrmDebugger, public XBOX::VObject
{
public:
	static VChrmDebugHandler*		Get();
	virtual bool					HasClients();
	virtual void*					GetCtx();
	virtual bool					ReleaseCtx(void* inContext);
	virtual int 					WaitForClientCommand( void* inContext );
	virtual bool					SetSource( void* inContext, const char *inData, int inLength, const char* inUrl, unsigned int inURLLen );
	virtual bool					BreakpointReached(void* inContext, int inLineNb);

	virtual XBOX::VError			GetPatterns(XBOX::VectorOfVString* outPatterns) const;
	virtual XBOX::VError			HandleRequest(IHTTPResponse* ioResponse);

private:
	VChrmDebugHandler();
	virtual							~VChrmDebugHandler();
	
static VChrmDebugHandler*			sDebugger;



#define K_NB_MAX_PAGES		(16)
	ChrmDbgHdlPage					fPages[K_NB_MAX_PAGES];
	XBOX::VCriticalSection			fLock;
	XBOX::VCppMemMgr*				fMemMgr;

	XBOX::VError					TreatJSONFile(IHTTPResponse* ioResponse);
};

#endif

