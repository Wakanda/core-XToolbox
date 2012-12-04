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
#ifndef __VCHROME_DEBUG_HANDLER_PAGE__
#define __VCHROME_DEBUG_HANDLER_PAGE__

#include "VLogHandler.h"


#define K_MAX_SIZE					(4096)

class RemoteDebugPilot;
class DebugContext;
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
		XBOX::VString					_urlStr;
		XBOX::VString					_dataStr;
		WAKDebuggerServerMessage		Msg;
		RemoteDebugPilot*				_pilot;
		DebugContext*					_debugContext;
} ChrmDbgMsgData_t;
typedef struct ChrmDbgMsg_st {
	ChrmDbgMsgType_t				type;
	ChrmDbgMsgData_t				data;
} ChrmDbgMsg_t;



class VChromeDbgHdlPage : public XBOX::VObject{

public:

									VChromeDbgHdlPage();
	virtual							~VChromeDbgHdlPage();

	void							Init(sLONG inPageNb,CHTTPServer* inHTTPSrv,XBOX::VString inAbsUrl);
	XBOX::VError					CheckInputData();

	XBOX::VError					TreatMsg();
	XBOX::VError					SendResult(const XBOX::VString& inValue);
	XBOX::VError					SendMsg(const XBOX::VString& inValue);
	XBOX::VError					TreatPageReload(XBOX::VString* inStr, intptr_t inSrcId);
	XBOX::VError					SendDbgPaused(XBOX::VString* inStr, intptr_t inSrcId);
	XBOX::VError					TreatWS(IHTTPResponse* ioResponse);
	XBOX::VError					GetBrkptParams(sBYTE* inStr, sBYTE** outUrl, sBYTE** outColNb, sBYTE **outLineNb);
	sLONG							GetPageNb() {return fPageNb;};
	void							Clear() {/*fSource.clear();*/};
	bool							fEnabled;// true when chrome front-end connected and initialized
	//bool							fActive;// true when chrome displays the page
	//bool							fUsed;// true when the page is fed with 4D debug data
	IHTTPWebsocketHandler*			fWS;
	WAKDebuggerState_t				fState;
	TemplProtectedFifo<ChrmDbgMsg_t>	fFifo;
	TemplProtectedFifo<ChrmDbgMsg_t>	fOutFifo;
	XBOX::VString					fRelURL;
private:
	sBYTE							fMsgData[K_MAX_SIZE];
	sLONG							fMsgLen;
	bool							fIsMsgTerminated;
	sLONG							fOffset; 
	XBOX::VString					fMethod;
	sBYTE*							fParams;
	sBYTE*							fId;
	sLONG							fBodyNodeId;
	sLONG							fPageNb;
	intptr_t						fSrcId;
	sLONG							fLineNb;
	XBOX::VString					fFileName;
	XBOX::VectorOfVString			fSource;
	XBOX::VSemaphore				fSem;
};

#endif

