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


#include "VLogHandler.h"

USING_TOOLBOX_NAMESPACE


VLogHandler::VLogHandler() : fSem(1), fFifo(32,false)
{
	fWS = NULL;
	fState = STOPPED_STATE;
}

void VLogHandler::SetWS(IHTTPWebsocketHandler* inWS)
{
	fWS = inWS;
	/*fTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &LogHandler::InnerTaskProc);
	if (fTask)
	{
		fTask->SetKindData((sLONG_PTR)this);
		fTask->Run();
	}*/
}

void VLogHandler::Put(WAKDebuggerTraceLevel_t inTraceLevel, const XBOX::VString& inStr)
{
	LogHandlerMsg_t		l_msg;
	if (fState != WORKING_STATE)
	{
		return;
	}
	l_msg.type = (LogHandlerMsgType_t)inTraceLevel;
	l_msg.value = inStr;
	if (fFifo.Put(l_msg) !=  VE_OK )
	{
		DebugMsg("LogHandler::PutE trace lost...\n");
	}
}


void VLogHandler::TreatClient(IHTTPResponse* ioResponse)
{
	LogHandlerMsg_t		l_msg;
	bool				l_msg_found;
	char				l_data[1024];
	VSize				l_size;
	VString				l_trace;

	if (!fSem.TryToLock())
	{
		DebugMsg("LogHandler::TreatClient already in use\n"); 
		return;
	}
	XBOX::VError			l_err;
	fState = STARTED_STATE;
	while (fState != STOPPED_STATE)
	{
		switch(fState)
		{
		case STARTED_STATE:
			l_err = fWS->TreatNewConnection(ioResponse);
			xbox_assert( l_err == VE_OK );
			if (l_err == VE_OK)
			{
				fState = WORKING_STATE;
			}
			else
			{
				fState = STOPPED_STATE;
			}
			break;
		case WORKING_STATE:
			l_err = fFifo.Get(&l_msg,&l_msg_found,500);
			xbox_assert( l_err == VE_OK );
			if (l_err != VE_OK)
			{
				fWS->Close();
				fState = STOPPED_STATE;
			}
			else
			{
				if (l_msg_found)
				{
					VString l_trace_clean;
					l_msg.value.GetJSONString(l_trace_clean);
					switch(l_msg.type)
					{
					case PUT_ERROR_TRACE_MSG:
						l_trace = "{\"type\":\"ERROR\",\"value\":\"";
						l_trace += l_trace_clean;
						l_trace += "\"}";
						break;
#if 1//defined(UNIFIED_DEBUGGER_NET_DEBUG_INFO_LEVEL)
					case PUT_INFO_TRACE_MSG:
						l_trace = "{\"type\":\"INFO\",\"value\":\"";
						l_trace += l_trace_clean;
						l_trace += "\"}";
						break;
#endif
					default:
						l_trace = "";
						break;
					}
					if (l_trace.GetLength() > 0)
					{
						l_size = l_trace.ToBlock(l_data,1024,VTC_UTF_8,false,false);
						l_err = fWS->WriteMessage(l_data,l_size,true);
						xbox_assert( l_err == VE_OK );
						if (l_err != VE_OK)
						{
							fWS->Close();
							fState = STOPPED_STATE;
						}
					}
				}
			}
			break;
		case STOPPED_STATE:
			break;
		default:
			break;
		}
	}

	fSem.Unlock();
}

/*sLONG LogHandler::InnerTaskProc(XBOX::VTask* inTask)
{
	typedef enum TraceState_enum {
		STOPPED_STATE,
		STARTED_STATE,
		WORKING_STATE,
	} TraceState_t;

	TraceState_t			l_state = STARTED_STATE;
	LogHandler*				l_this = (LogHandler*)inTask->GetKindData();
	XBOX::VError			l_err;

	switch(l_state)
	{
	case STARTED_STATE:
		l_err = l_this->fWS->TreatNewConnection(l_msg.response);
		if (!l_err)
		{
				l_state = WORKING_STATE;
		}
		break;
	default:
	}
	return 0;
}*/
