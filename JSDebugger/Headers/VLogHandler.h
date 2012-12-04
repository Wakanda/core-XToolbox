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
#ifndef __VCHROME_DEBUG_LOG_HANDLER__
#define __VCHROME_DEBUG_LOG_HANDLER__

#include "VChromeDebugHandler.h"

template <class Elt>
class TemplProtectedFifo	{
public:
	#define K_NB_MAX_MSGS	(4)
	
	TemplProtectedFifo(unsigned int inMaxSize=K_NB_MAX_MSGS, bool inAssertWhenFull=true) :
					fSem(0,inMaxSize+1), fMaxSize(inMaxSize), fAssertWhenFull(inAssertWhenFull) {;}

	void							Reset()	{
		if (!testAssert(fLock.Lock()))
		{
			// trace
		}
		while (!fMsgs.empty())
		{
			fMsgs.pop();
		}
		while(fSem.TryToLock())
		{
		}
		if (!testAssert(fLock.Unlock()))
		{
			// trace
		}
	}
	XBOX::VError					Put(Elt& inMsg)
	{
		XBOX::VError	l_err;
		size_t			l_size;
		l_err = XBOX::VE_UNKNOWN_ERROR;
		if (!testAssert(fLock.Lock()))
		{
			// trace
		}
		l_size = fMsgs.size();
		if (l_size < fMaxSize)
		{
			fMsgs.push(inMsg);
			if (!fSem.Unlock())
			{
				xbox_assert(false);
			}
			else
			{
				l_err = XBOX::VE_OK;
			}
		}
		else
		{
			l_err = XBOX::VE_MEMORY_FULL;
			if (fAssertWhenFull)
			{
				xbox_assert(false);
			}
		}
		if (!testAssert(fLock.Unlock()))
		{
			// trace
		}
		return l_err;
	}
	XBOX::VError					Get(Elt* outMsg, bool* outFound, uLONG inTimeoutMs) {
		bool			l_ok;
		XBOX::VError	l_err;

		*outFound = false;
		l_err = XBOX::VE_UNKNOWN_ERROR;

		if (!inTimeoutMs)
		{
			l_ok = fSem.Lock();
		}
		else
		{
			l_ok = fSem.Lock((sLONG)inTimeoutMs);
		}
		if (l_ok)
		{
			if (!testAssert(fLock.Lock()))
			{
				// trace
			}
			*outMsg = (Elt)fMsgs.front();
			fMsgs.pop();
			if (!testAssert(fLock.Unlock()))
			{
				// trace
			}
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
private:
	unsigned int					fMaxSize;
	bool							fAssertWhenFull;
	std::queue<Elt>					fMsgs;
	XBOX::VCriticalSection			fLock;
	XBOX::VSemaphore				fSem;
};

class VLogHandler : public XBOX::VObject {
public:
	VLogHandler();
	void SetWS(IHTTPWebsocketHandler* inWS);
	void Put(WAKDebuggerTraceLevel_t inTraceLevel, const XBOX::VString& inStr);
	void TreatClient(IHTTPResponse* ioResponse);
private:
	~VLogHandler() {;}
	IHTTPWebsocketHandler*		fWS;
	XBOX::VSemaphore			fSem;
	typedef enum TraceState_enum {
		STOPPED_STATE,
		STARTED_STATE,
		WORKING_STATE,
	} TraceState_t;

	TraceState_t				fState;

typedef enum LogHandlerMsgType_enum {
		PUT_ERROR_TRACE_MSG = WAKDBG_ERROR_LEVEL,
		PUT_INFO_TRACE_MSG = WAKDBG_INFO_LEVEL,
} LogHandlerMsgType_t;

typedef struct LogHandlerMsg_st {
	LogHandlerMsgType_t		type;
	XBOX::VString			value;
} LogHandlerMsg_t;
	TemplProtectedFifo<LogHandlerMsg_t>	fFifo;

};


#endif

