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

#include "VChromeDebugHandler.h"
#include "VRemoteDebugPilot.h"


USING_TOOLBOX_NAMESPACE

#define WKA_USE_UNIFIED_DBG

const XBOX::VString			K_REMOTE_DEBUGGER_WS("/devtools/remote_debugger_ws");
const XBOX::VString			K_REMOTE_TRACES_WS("/devtools/remote_traces_ws");
const XBOX::VString			K_CHR_DBG_ROOT("/devtools");
const XBOX::VString			K_CHR_DBG_FILTER("/devtools/page/");
const XBOX::VString			K_CHR_DBG_JSON_FILTER("/devtools/json.wka");
const XBOX::VString			K_CHR_DBG_REMDBG_FILTER("/devtools/remote_debugger.html");
const XBOX::VString			K_CHR_DBG_REMTRA_FILTER("/devtools/remote_traces.html");
const XBOX::VString			K_TEMPLATE_WAKSERV_HOST_STR("__TEMPLATE_WAKANDA_SERVER_HOST__");
const XBOX::VString			K_TEMPLATE_WAKSERV_PORT_STR("__TEMPLATE_WAKANDA_SERVER_PORT__");


VChromeDebugHandler*				VChromeDebugHandler::sDebugger=NULL;
XBOX::VCriticalSection				VChromeDebugHandler::sDbgLock;



VLogHandler*			sPrivateLogHandler = NULL;

#if defined(WKA_USE_UNIFIED_DBG)



VChromeDebugHandler::VChromeDebugHandler(
	const VString	inIP,
	sLONG			inPort,
	const VString	inDebuggerHTML,
	const VString	inTracesHTML)
{
	if (!sDebugger)
	{
		fStarted = false;
		fPort = inPort;
		fIPAddr = inIP;
		//fDebuggerSettings = NULL;
		fDebuggerHTML = inDebuggerHTML;
		fTracesHTML = inTracesHTML;
		VString l_port_str = CVSTR(":");
		l_port_str.AppendLong8(fPort);
		VString l_rel_url;

		CHTTPServer *l_http_server = VComponentManager::RetainComponent< CHTTPServer >();
		fPilot = new VRemoteDebugPilot(l_http_server,inIP,inPort);
		if (!fPilot)
		{
			DebugMsg("VChromeDebugHandler::VChromeDebugHandler VRemoteDebugPilot Init pb\n");
			xbox_assert(false);
		}
		else
		{
			fPilot->SetWS(l_http_server->NewHTTPWebsocketHandler());
			sPrivateLogHandler->SetWS(l_http_server->NewHTTPWebsocketHandler());
		}
#if 0
		if (!WebInspDebugContext::Init(l_http_server,inIP,inPort,fMemMgr))
		{
			DebugMsg("VChromeDebugHandler::VChromeDebugHandler DebugContext::Init pb\n");
			xbox_assert(false);
		}
		else
		{
		/*for( int l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
		{
			l_rel_url = CVSTR("/wka_dbg");
			l_rel_url += K_CHR_DBG_ROOT;
			l_rel_url += CVSTR("/devtools.html?host=");
			l_rel_url += fIPAddr;
			l_rel_url += l_port_str;
			l_rel_url += CVSTR("&page=");
			l_rel_url.AppendULong8(l_i);

			fPages[l_i].Init((uLONG)l_i,l_http_server,fMemMgr,l_rel_url);
		}*/
			fPilot->SetWS(l_http_server->NewHTTPWebsocketHandler());
			sPrivateLogHandler->SetWS(l_http_server->NewHTTPWebsocketHandler());
		}
#endif
		l_http_server->Release();

	}
	else
	{
		//fWS = NULL;
		//DebugMsg("VChrmDebugHandler should be a singleton\n");
		xbox_assert(false);
	}
}



VChromeDebugHandler::~VChromeDebugHandler()
{
	xbox_assert(false);
	/*if (fWS)
	{
		if (fWS->Close())
		{
			DebugMsg("~VChrmDebugHandler error closing websocket\n");
		}
		VTask::Sleep(1500);
		s_vchrm_debug_handler_init = false;
		fWS = NULL;
	}*/
}




XBOX::VError VChromeDebugHandler::TreatDebuggerHTML(IHTTPResponse* ioResponse)
{
	XBOX::VError		err;
	VSize				size;
	char*				buff;

	err = VE_OK;
	if (ioResponse->GetRequestMethod() != HTTP_GET)
	{
		err = VE_INVALID_PARAMETER;
	}
	if (!err)
	{
		{
			VStringConvertBuffer	buffer(fDebuggerHTML,VTC_UTF_8);
			size = buffer.GetSize();
			err = ioResponse->SetResponseBody(buffer.GetCPointer(),size);
			if (!err)
			{
				if (!ioResponse->SetContentTypeHeader(CVSTR("text/html"),XBOX::VTC_UTF_8))
				{
					err = VE_INVALID_PARAMETER;
				}
			}
		}
	}
	return err;
}


XBOX::VError VChromeDebugHandler::TreatTracesHTML(IHTTPResponse* ioResponse)
{
	XBOX::VError		err;
	VSize				size;
	char*				buff;

	err = VE_OK;
	if (ioResponse->GetRequestMethod() != HTTP_GET)
	{
		err = VE_INVALID_PARAMETER;
	}
	if (!err)
	{
		{
			VStringConvertBuffer	buffer(fTracesHTML,VTC_UTF_8);
			size = buffer.GetSize();

			err = ioResponse->SetResponseBody(buffer.GetCPointer(),size);
			if (!err)
			{
				if (!ioResponse->SetContentTypeHeader(CVSTR("text/html"),XBOX::VTC_UTF_8))
				{
					err = VE_INVALID_PARAMETER;
				}
			}
		}
	}
	return err;
}

XBOX::VError VChromeDebugHandler::TreatRemoteTraces(IHTTPResponse* ioResponse)
{
	XBOX::VError		err;

	err = VE_OK;
	sPrivateLogHandler->TreatClient(ioResponse);
	return err;
}


XBOX::VError VChromeDebugHandler::TreatJSONFile(IHTTPResponse* ioResponse)
{
	XBOX::VError		err = VE_UNKNOWN_ERROR;

	xbox_assert(false); // SHOULD NOT BE CALLED anymore !!

	return err;
}


#if 0
RemoteDebugPilot::RemoteDebugPilot() : fCompletedSem(0)
{
	fWS = NULL;
	fFifo.Reset();
	fTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &RemoteDebugPilot::InnerTaskProc);
	if (fTask)
	{
		fTask->SetKindData((sLONG_PTR)this);
		fTask->Run();
	}
}

#define K_DBG_PROTOCOL_CONNECT_STR	"{\"method\":\"connect\",\"id\":\""
#define K_DBG_PROTOCOL_GET_URL_STR	"{\"method\":\"getURL\",\"id\":\""

sLONG RemoteDebugPilot::InnerTaskProc(XBOX::VTask* inTask)
{

	RemoteDebugPilot*						l_this = (RemoteDebugPilot*)inTask->GetKindData();
	XBOX::VString							l_client_id;
	std::map<unsigned int,DebugContext*>	l_hash;

	XBOX::VError	l_err;
	XBOX::VString	l_resp;
	int			l_offset = 0;
	RemoteDebugPilotMsg_t	l_msg;
	bool		l_msg_found;
	uLONG		l_msg_len;
	bool		l_is_msg_terminated;
	char*		l_tmp;
	char*		l_end;
	VSize		l_size;

	//l_this->fFifo.Reset();
	l_this->l_state = STARTED_STATE;

	while(!l_this->fFifo.Get(&l_msg,&l_msg_found,500))
	{
		if (l_msg_found)
		{
			switch(l_msg.type)
			{
			case TREAT_CLIENT_MSG:
				switch(l_this->l_state)
				{
				case STARTED_STATE:
					l_err = l_this->fWS->TreatNewConnection(l_msg.response);
					if (!l_err)
					{
						l_this->l_state = WORKING_STATE;
						l_client_id = VString("");
					}
					else
					{
						l_this->fCompletedSem.Unlock();
					}
					break;
				case STOPPED_STATE:
					l_this->fCompletedSem.Unlock();
					break;
				case WORKING_STATE:
					DebugMsg("RemoteDebugPilot::InnerTaskProc NEW_CLIENT SHOULD NOT HAPPEN in state=%d !!!\n",l_this->l_state);
					xbox_assert(false);
					break;
				}
				break;
			case NEW_CONTEXT_MSG:
				if (l_this->l_state == WORKING_STATE)
				{
					l_resp = CVSTR("{\"method\":\"newContext\",\"contextId\":\"");
					l_resp.AppendLong8(l_msg.ctx->GetId());
					l_resp += CVSTR("\",\"id\":\"");
					l_hash[l_msg.ctx->GetId()] = l_msg.ctx;
					goto send;
				}
				break;
			case REMOVE_CONTEXT_MSG:
				if (l_this->l_state == WORKING_STATE)
				{
					l_resp = CVSTR("{\"method\":\"removeContext\",\"contextId\":\"");
					l_resp.AppendLong8(l_msg.ctx->GetId());
					l_resp += CVSTR("\",\"id\":\"");
					if (!l_hash.erase(l_msg.ctx->GetId()))
					{
						DebugMsg("RemoteDebugPilot::InnerTaskProc map erase pb for ctxid=%d\n",l_msg.ctx->GetId());
					}
					goto send;
				}
				break;
			case UPDATE_CONTEXT_MSG:
				if (l_this->l_state == WORKING_STATE)
				{
					l_resp = CVSTR("{\"method\":\"updateContext\",\"contextId\":\"");
					l_resp.AppendLong8(l_msg.ctx->GetId());
					l_resp += CVSTR("\",\"id\":\"");
					goto send;
				}
				break;
			case SET_URL_CONTEXT_MSG:
				if (l_this->l_state == WORKING_STATE)
				{
					l_resp = CVSTR("{\"method\":\"setURLContext\",\"contextId\":\"");
					l_resp.AppendLong8(l_msg.ctx->GetId());
					l_resp += CVSTR("\",\"url\":\"");//http://");
					l_resp += l_msg.url;
					l_resp += CVSTR("\",\"id\":\"");
					goto send;
				}
				break;
			default:
				break;
			}
		}
		if (l_this->l_state == WORKING_STATE)
		{
			l_msg_len = K_MAX_SIZE;
			l_err = l_this->fWS->ReadMessage(l_this->fData,&l_msg_len,&l_is_msg_terminated);
			if (l_err)
			{
				DebugMsg("RemoteDebugPilot::InnerTaskProc fWS->ReadMessage pb\n");
				xbox_assert(false);
				l_this->fCompletedSem.Unlock();
				l_this->l_state = STARTED_STATE;
				continue;
			}
			if (l_msg_len)
			{

				if (l_msg_len < K_MAX_SIZE)
				{
					l_this->fData[l_msg_len] = 0;
				}
				else
				{
					l_this->fData[K_MAX_SIZE-1] = 0;
				}
				DebugMsg("RemoteDebugPilot::InnerTaskProc received msg='%s'\n",l_this->fData);
				l_tmp = strstr(l_this->fData,K_DBG_PROTOCOL_CONNECT_STR);
				if (l_tmp && !l_client_id.GetLength())
				{
					l_tmp += strlen(K_DBG_PROTOCOL_CONNECT_STR);
					l_end = strchr(l_tmp,'"');
					*l_end = 0;
					l_resp = CVSTR("{\"result\":\"ok\",\"id\":\"");
					l_tmp[K_MAX_SIZE] = 0;
					l_client_id = VString(l_tmp);
send:
					l_resp += l_client_id;
					l_resp += CVSTR("\"}");
					l_size = l_resp.GetLength() * 3; /* overhead between VString's UTF_16 to CHAR* UTF_8 */
					if (l_size >= K_MAX_SIZE)
					{
						l_size = K_MAX_SIZE;
						xbox_assert(false);
					}
					l_size = l_resp.ToBlock(l_this->fData,l_size,VTC_UTF_8,false,false);
					l_err = l_this->fWS->WriteMessage(l_this->fData,l_size,true);
					if (l_err)
					{
						l_this->fCompletedSem.Unlock();
						l_this->l_state = STARTED_STATE;
						continue;
					}
				}
				else
				{
					l_tmp = strstr(l_this->fData,K_DBG_PROTOCOL_GET_URL_STR);
					if (l_tmp && l_client_id.GetLength())
					{
						l_tmp += strlen(K_DBG_PROTOCOL_GET_URL_STR);
						l_end = strchr(l_tmp,'"');
						*l_end = 0;
						unsigned int	l_id = atoi(l_tmp);
						WebInspDebugContext*	l_ctx;
						const std::map<unsigned int,WebInspDebugContext*>::const_iterator l_it = l_hash.find(l_id);
						if (l_it != l_hash.end())
						{
							l_it->second->GetURL();
						}
						else
						{
							DebugMsg("RemoteDebugPilot::InnerTaskProc  unknwon context\n");
							xbox_assert(false);
						}
					}
					else
					{
						DebugMsg("RemoteDebugPilot::InnerTaskProc received msg='%s'\n",l_this->fData);
					}
				}
			}
		}
	}
	DebugMsg("RemoteDebugPilot::InnerTaskProc fifo pb\n");
	xbox_assert(false);
	return 0;
}

XBOX::VError RemoteDebugPilot::TreatRemoteDbg(IHTTPResponse* ioResponse)
{
	RemoteDebugPilotMsg_t	l_msg;

	if (!fSem.TryToLock())
	{
		DebugMsg("RemoteDebugPilot::TreatRemoteDbg already in use\n"); 
		return VE_UNKNOWN_ERROR;
	}

	l_msg.type = TREAT_CLIENT_MSG;
	l_msg.response = ioResponse;
	if (!fFifo.Put(l_msg))
	{
		fCompletedSem.Lock();
	}
	fSem.Unlock();
	return XBOX::VE_OK;
}
#endif



XBOX::VError VChromeDebugHandler::HandleRequest( IHTTPResponse* ioResponse )
{
	XBOX::VError	l_err;
	sLONG			l_page_nb;
	//ChrmDbgMsg_t	l_msg;

	DebugMsg("VChromeDebugHandler::HandleRequest called ioResponse=%x\n",ioResponse);

	if (!fStarted)
	{
		sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,VString("VChromeDebugHandler::HandleRequest SERVER NOT STARTED!!!"));
		return VE_INVALID_PARAMETER;
	}
	if (ioResponse == NULL)
	{
		sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,VString("VChromeDebugHandler::HandleRequest called NULL ptr!!!"));
		return VE_INVALID_PARAMETER;
	}
	/*VHTTPResponse *				response = dynamic_cast<VHTTPResponse *>(ioResponse);
	VVirtualHost *				virtualHost = (NULL != response) ? dynamic_cast<VVirtualHost *>(response->GetVirtualHost()) : NULL;
	VAuthenticationManager *	authenticationManager = (NULL != virtualHost) ? dynamic_cast<VAuthenticationManager *>(virtualHost->GetProject()->GetAuthenticationManager()) : NULL;

	if (NULL != authenticationManager && (authenticationManager->CheckAdminAccessGranted (ioResponse) != XBOX::VE_OK))
		return VE_HTTP_PROTOCOL_UNAUTHORIZED;
*/
	VString l_trace("VChromeDebugHandler::HandleRequest called for url=");
	l_trace += ioResponse->GetRequest().GetURL();
	sPrivateLogHandler->Put(WAKDBG_INFO_LEVEL,l_trace);

	VString			l_url = ioResponse->GetRequest().GetURL();
	VString			l_tmp;
	VIndex			l_i;
	
	l_err = VE_OK;
	l_i = l_url.Find(K_CHR_DBG_JSON_FILTER);
	if (l_i > 0)
	{
		l_err = TreatJSONFile(ioResponse);
	}
	else
	{
		l_i = l_url.Find(K_CHR_DBG_FILTER);
		if (l_i > 0)
		{
			l_url.GetSubString(
				l_i+K_CHR_DBG_FILTER.GetLength(),
				l_url.GetLength() - K_CHR_DBG_FILTER.GetLength(),
				l_tmp);

			l_page_nb = l_tmp.GetLong();

			if (!sStaticSettings->UserCanDebug(
					ioResponse->GetRequest().GetAuthenticationInfos()))
			{
					l_err = VE_HTTP_PROTOCOL_UNAUTHORIZED;
			}
			if (!l_err)
			{
				l_err = fPilot->TreatWS(ioResponse,l_page_nb);
			}
		}
		else
		{
			l_i = l_url.Find(K_REMOTE_DEBUGGER_WS);
			if (l_i > 0)
			{
				if (!sStaticSettings->UserCanDebug(
						ioResponse->GetRequest().GetAuthenticationInfos()))
				{
					l_err = VE_HTTP_PROTOCOL_UNAUTHORIZED;
				}
				if (!l_err)
				{
					l_err = fPilot->TreatRemoteDbg(ioResponse);
					l_err = XBOX::VE_OK;// set err to OK so that HTTP server considers HTTP request successfully ended
				}
			}
			else
			{
				l_i = l_url.Find(K_CHR_DBG_REMDBG_FILTER);
				if (l_i > 0)
				{
					if (!sStaticSettings->UserCanDebug(
							ioResponse->GetRequest().GetAuthenticationInfos()))
					{
						l_err = VE_HTTP_PROTOCOL_UNAUTHORIZED;
					}
					if (!l_err)
					{
						l_err = TreatDebuggerHTML(ioResponse);
					}
				}
				else
				{
					l_i = l_url.Find(K_CHR_DBG_REMTRA_FILTER);
					if (l_i > 0)
					{
						l_err = TreatTracesHTML(ioResponse);
					}
					else
					{
						l_i = l_url.Find(K_REMOTE_TRACES_WS);
						if (l_i > 0)
						{
							l_err = TreatRemoteTraces(ioResponse);
							l_err = XBOX::VE_OK;// set err to OK so that HTTP server considers HTTP request successfully ended
						}
						else
						{
							l_err = XBOX::VE_INVALID_PARAMETER;
						}
					}
				}
			}
		}
	}
	return l_err;

}


XBOX::VError VChromeDebugHandler::GetPatterns(XBOX::VectorOfVString *outPatterns) const
{
	if (NULL == outPatterns)
		return VE_HTTP_INVALID_ARGUMENT;
	outPatterns->clear();
	outPatterns->push_back(K_CHR_DBG_ROOT);
	return XBOX::VE_OK;
}


bool VChromeDebugHandler::HasClients()
{
	return fPilot->IsActive();
/*
	bool	l_res;
	l_res = false;
	fLock.Lock();
	for( int l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
	{
		if (fPages[l_i].fActive)// && !fPages[l_i].fUsed)
		{
			l_res = true;
			break;
		}
	}
	fLock.Unlock();
	return l_res;
*/
}


WAKDebuggerContext_t VChromeDebugHandler::AddContext( uintptr_t inContext )
{
	WAKDebuggerContext_t	l_res;

	l_res = NULL;

	//WebInspDebugContext*	l_ctx = WebInspDebugContext::Get();
#if 0//TBC
	l_res = (void*)l_ctx;
	if (!l_res)
	{
		DebugMsg("VChromeDebugHandler::AddContext not enough resources\n");
		xbox_assert(false);
	}
	else
	{
#endif
	if (fPilot->NewContext(&l_res) != VE_OK)
	{
		return (WAKDebuggerContext_t)-1;
	}

	return l_res;
}

bool VChromeDebugHandler::RemoveContext( WAKDebuggerContext_t inContext )
{
	bool	l_res;
	l_res = (fPilot->ContextRemoved(inContext) == VE_OK);

	return l_res;
}

void VChromeDebugHandler::DisposeMessage(WAKDebuggerServerMessage* inMessage)
{
	if (inMessage)
	{
		free(inMessage);
	}
}

WAKDebuggerServerMessage* VChromeDebugHandler::WaitFrom(WAKDebuggerContext_t inContext)
{
	WAKDebuggerServerMessage*		l_res;
	l_res = fPilot->WaitFrom(inContext);
#if 0 //TBC
	ChrmDbgMsg_t					l_msg;
	bool							l_msg_found;

	l_res = (WAKDebuggerServerMessage*)fMemMgr->Malloc( sizeof(WAKDebuggerServerMessage), false, 'memb');
	if (testAssert( l_res != NULL ))
	{
		memset(l_res,0,sizeof(*l_res));
		l_res->fType = WAKDebuggerServerMessage::NO_MSG;
	}
	else
	{
		return NULL;
	}

#if 1
	DebugContext*	l_ctx = (DebugContext*)inContext;
	if (DebugContext::Check(l_ctx))
	{
		ChrmDbgHdlPage	*l_page;
		l_page = l_ctx->GetPage();

		//xbox_assert(l_page->fActive);
		if (l_page->fState == PAUSE_STATE)
		{
			l_page->fOutFifo.Get(&l_msg,&l_msg_found,0);
			if (!l_msg_found)
			{
				l_msg.type = NO_MSG;
			}
			if (l_msg.type == SEND_CMD_MSG)
			{
				memcpy( l_res, &l_msg.data.Msg, sizeof(*l_res) );
			}
			else
			{
				xbox_assert(false);
			}
			/*l_page->fEvt.Lock();
			l_page->fEvt.Reset();
			l_res = l_page->fCmd;*/
		}
		else
		{
			xbox_assert(false);
		}
	}
#endif
#endif
	return l_res;
}

long long VChromeDebugHandler::GetMilliseconds()
{
	xbox_assert(false);// should not pass here
	return -1;
}
void VChromeDebugHandler::Reset()
{
	xbox_assert(false);//TBC
}
void VChromeDebugHandler::WakeUpAllWaiters()
{
	xbox_assert(false);//TBC
}

WAKDebuggerServerMessage* VChromeDebugHandler::GetNextBreakPointCommand()
{
	xbox_assert(false);// should not pass here
	return NULL;
}
WAKDebuggerServerMessage* VChromeDebugHandler::GetNextSuspendCommand( WAKDebuggerContext_t inContext )
{
	xbox_assert(false);// should not pass here
	return NULL;
}
WAKDebuggerServerMessage* VChromeDebugHandler::GetNextAbortScriptCommand ( WAKDebuggerContext_t inContext )
{
	xbox_assert(false);// should not pass here
	return NULL;
}

bool VChromeDebugHandler::SetState(WAKDebuggerContext_t inContext, WAKDebuggerState_t state)
{

	bool l_res = (fPilot->SetState(inContext,state) == VE_OK);
#if 0//TBC
	if (!l_res)
	{
		return l_res;
	}

	ChrmDbgHdlPage	*l_page;
	//fLock.Lock();
	l_page = l_ctx->GetPage();
	if (l_page)
	{
		l_page->fState = state;
		l_page->fFifo.Reset();
		l_page->fOutFifo.Reset();
	}
	l_res = true;
	//fLock.Unlock();
#endif
	return l_res;
}

void VChromeDebugHandler::SetSourcesRoot( char* inRoot, int inLength )
{
	xbox_assert(false);//should not pass here
}
char* VChromeDebugHandler::GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize )
{
	xbox_assert(false);//should not pass here
	return NULL;
}
char* VChromeDebugHandler::GetRelativeSourcePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inAbsolutePath, int inPathSize,
									int& outSize )
{
	xbox_assert(false);//should not pass here
	return NULL;
}
bool VChromeDebugHandler::BreakpointReached(	WAKDebuggerContext_t	inContext,
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
											long 					inEndOffset )
{
	VError	l_err;
	l_err = fPilot->BreakpointReached(
				inContext,
				inLineNumber,
				inExceptionHandle,
				inURL,
				inURLLength,
				inFunction,
				inFunctionLength,
				inMessage,
				inMessageLength,
				inName,
				inNameLength,
				inBeginOffset,
				inEndOffset);
#if 0 //TBC
	VString			l_trace;
/*
	l_trace = CVSTR("VChromeDebugHandler::BreakpointReached called, CheckContext=");
	l_trace += (l_res ? "1" : "0" );
	sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);

	if (!l_res)
	{
		return l_res;
	}
	ChrmDbgHdlPage*	l_page = l_ctx->fPage;*/
	//ChrmDbgMsg_t	l_msg;
	/*if (l_page->GetPageNb() >= K_NB_MAX_PAGES)
	{
		DebugMsg("VChromeDebugHandler::BreakpointReached corrupted data\n");
		xbox_assert(false);
		return l_res;
	}
	if (!l_page->fActive)
	{
		// to activate the page and make the browser open the URL (thus connecting to our WS)
		fPilot.ContextUpdated(l_ctx);

		for( int l_i=0; l_i<40; l_i++ )
		{
			VTask::Sleep(100);
			if (l_page->fActive && l_page->fEnabled)
			{
				break;
			}
		}
		xbox_assert((l_page->fActive && l_page->fEnabled));
		if (!l_page->fActive || !l_page->fEnabled )
		{
			l_trace = VString("VChromeDebugHandler::BreakpointReached dbgr still not connected!!!!");
			sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,l_trace);
			return false;
		}
	}*/

	l_res = ( fPilot->ContextUpdated(l_ctx) == VE_OK );
	bool	l_already_activated;
	l_res = l_ctx->WaitUntilActivated(&l_already_activated);
	if (!l_res)
	{
		l_trace = VString("VChromeDebugHandler::BreakpointReached ctx activation pb for ctxid=");
		l_trace.AppendLong8(l_ctx->GetId());
		l_trace += (l_res ? "1" : "0" );
		sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);
	}
	else
	{
		ChrmDbgHdlPage*	l_page = l_ctx->GetPage();
		l_res = (l_page != NULL);
		if (l_res)
		{
			DebugMsg(	"VChromeDebugHandler::BreakpointReached page=%d, alrdy_act=%s\n",
						l_page->GetPageNb(),
						(l_already_activated ? "1" : "0") );
			if (!l_already_activated)
			{
				fPilot->SetURLContext(l_ctx,l_page->fRelURL);

				while (!l_page->fEnabled)
				{
					VTask::Sleep(200);
				}
			}
			DebugMsg(	"VChromeDebugHandler::BreakpointReached page=%d enabled\n",l_page->GetPageNb());
			l_msg.type = BRKPT_REACHED_MSG;
			l_msg.data.Msg.fLineNumber = inLineNumber;
			l_msg.data.Msg.fSrcId = *(intptr_t *)inURL;// we use the URL param to pass sourceId (to not modify interface of server)
			l_msg.data._dataStr = VString( inFunction, inFunctionLength*2,  VTC_UTF_16 );
			l_msg.data._urlStr = VString( inMessage, inMessageLength*2,  VTC_UTF_16 );
			l_msg.data._pilot = fPilot;

			l_page->fState = PAUSE_STATE;
			l_res = (l_page->fFifo.Put(l_msg) == VE_OK);
			l_trace = VString("VChromeDebugHandler::BreakpointReached fFifo.put status=");
			l_trace += (l_res ? "1" : "0" );
			sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);
		}
	}
#endif

	return testAssert(l_err == VE_OK);

}

bool VChromeDebugHandler::SendEval( WAKDebuggerContext_t inContext, void* inVars )
{
	bool	l_res;

	l_res = (fPilot->SendEval(inContext,inVars) == VE_OK);

#if 0//TBC
	DebugContext*	l_ctx = (DebugContext*)inContext;

	l_ctx = DebugContext::Check(l_ctx);
	l_res = (l_ctx != NULL);
	if (!l_res)
	{
		return l_res;
	}
#if 1 //  TBC
	ChrmDbgHdlPage*	l_page = l_ctx->GetPage();
	ChrmDbgMsg_t	l_msg;

	if (l_page->GetPageNb() >= K_NB_MAX_PAGES)
	{
		DebugMsg("VChromeDebugHandler::SendEval corrupted data\n");
		xbox_assert(false);
		return l_res;
	}
	l_msg.type = EVAL_MSG;
	l_msg.data._dataStr = *(VString*)(inVars);
	delete (VString*)(inVars);
	l_res = (l_page->fFifo.Put(l_msg) == VE_OK);
#endif
#endif
	return l_res;
}

bool VChromeDebugHandler::SendLookup( WAKDebuggerContext_t inContext, void* inVars, unsigned int inSize )
{
	bool	l_res = (fPilot->SendLookup(inContext,inVars,inSize) == VE_OK);

#if 0
	WebInspDebugContext*	l_ctx = (DebugContext*)inContext;

	l_ctx = WebInspDebugContext::Check(l_ctx);
	l_res = (l_ctx != NULL);
	if (!l_res)
	{
		return l_res;
	}
#if 1 // to be changed
	ChrmDbgMsg_t	l_msg;
	ChrmDbgHdlPage*	l_page = l_ctx->GetPage();
	if (l_page->GetPageNb() >= K_NB_MAX_PAGES)
	{
		DebugMsg("VChromeDebugHandler::SendLookup corrupted data\n");
		xbox_assert(false);
		return l_res;
	}
	l_msg.type = LOOKUP_MSG;
	l_msg.data._dataStr = *((VString*)inVars);
	delete (VString*)(inVars);
//	const XBOX::VString				K_RUN_GET_PRO_STR1("{\"result\":{\"result\":[
//	{\"value\":{\"type\":\"number\",\"value\":123,\"description\":\"123\"},\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"test1\"},
//	{\"value\":{\"type\":\"number\",\"value\":3,\"description\":\"3\"},    \"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"test\"}]},\"id\":");
	/*l_msg.data._dataStr = VString("{\"result\":{\"result\":[");
	bool l_first = true;
	for( int l_i=0; l_i<inSize; l_i++ )
	{
		VString l_str = *((VString*)inVars[l_i].name_str);
		delete (VString *)inVars[l_i].name_str;

		if (!l_first)
		{
			l_msg.data.dataStr += VString(",");
		}
		else
		{
			l_first = false;
		}
		VString		l_tmp;
		if (inVars[l_i].value_str)
		{

			l_tmp = *(VString*)(inVars[l_i].value_str);
			delete (VString*)(inVars[l_i].value_str);
			l_msg.data.dataStr += l_tmp;
			l_msg.data.dataStr += VString(",\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"");
			//if (l_str.Find(VString("argumen")) > 0)
			//{
			//	continue;
			//}
			l_msg.data.dataStr += l_str;
			l_msg.data.dataStr += VString("\"}");
		}
		else
		{
			l_msg.data.dataStr += VString("{\"value\":{\"type\":\"number\",\"value\":");
			l_tmp = VString("1234567");
			l_msg.data.dataStr += l_tmp;
			l_msg.data.dataStr += VString(",\"description\":\"");
			l_msg.data.dataStr += l_tmp;
			l_msg.data.dataStr += VString("\"},\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"");
			l_msg.data.dataStr += l_str;
			l_msg.data.dataStr += VString("\"}");
		}
	}
	l_msg.data.dataStr += VString("]},\"id\":");*/
	l_res = (l_page->fFifo.Put(l_msg) == VE_OK);
#endif
#endif
	return l_res;
}


bool VChromeDebugHandler::SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength )
{
	bool	l_res = (fPilot->SendCallStack( inContext, inData, inLength ) == VE_OK);
#if 0
	WebInspDebugContext*	l_ctx = (DebugContext*)inContext;

	l_ctx = WebInspDebugContext::Check(l_ctx);
	l_res = (l_ctx != NULL);
	if (!l_res)
	{
		return l_res;
	}

	ChrmDbgHdlPage*	l_page = l_ctx->GetPage();
	ChrmDbgMsg_t	l_msg;
	if (l_page->GetPageNb() >= K_NB_MAX_PAGES)
	{
		DebugMsg("VChromeDebugHandler::SendCallStack corrupted data\n");
		xbox_assert(false);
		return l_res;
	}
	l_msg.type = CALLSTACK_MSG;
	//l_msg.data._dataStr += CVSTR(",\"params\":{\"callFrames\":[");
	l_msg.data._dataStr = VString( inData, inLength*2,  VTC_UTF_16 );
	l_msg.data._debugContext = l_ctx;

#if 0 // TBC
	bool l_first = true;
	for( int l_i=0; l_i<l_page->fVectFrames.size(); )
	{

		l_msg.data.dataStr += CVSTR("{\"callFrameId\":\"{\\\"ordinal\\\":");
		l_msg.data.dataStr.AppendLong((sLONG)l_i);
		l_msg.data.dataStr += CVSTR(",\\\"injectedScriptId\\\":3}\",\"functionName\":\"");
		l_msg.data.dataStr += *((VString*)l_page->fVectFrames[l_i].name_str);
		l_msg.data.dataStr += VString("\",\"location\":{\"scriptId\":\"" K_JS_SCRIPTID_VALUE "\",\"lineNumber\":");
		l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].line_nb));
		l_msg.data.dataStr += CVSTR(",\"columnNumber\":");
		l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].col_nb));
		l_msg.data.dataStr += CVSTR("},\"scopeChain\":[");

		if (l_page->fVectFrames[l_i].available_scopes == IJSWChrmDebugger::SCOPE_GLOBAL_ONLY)
		{
			l_msg.data.dataStr += CVSTR("{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":");
			l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].id));//scope_ids[GLOBAL_SCOPE]));
			l_msg.data.dataStr += CVSTR("}\",\"className\":\"Application\",\"description\":\"Application\"},\"type\":\"global\"}");//GH
			l_msg.data.dataStr += CVSTR("],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":");
			l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].id));//scope_ids[GLOBAL_SCOPE]));
			l_msg.data.dataStr += CVSTR("}\",\"className\":\"Application\",\"description\":\"Application\"}");//GH
		}
		else
		{
		//if ( ((unsigned int)l_page->fVectFrames[l_i].scope_ids[LOCAL_SCOPE] != (unsigned int)K_UNDEFINED_SCOPE_ID) )
		//{
			l_msg.data.dataStr += CVSTR("{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":");
			l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].scope_ids[LOCAL_SCOPE]));
			l_msg.data.dataStr += CVSTR("}\",\"className\":\"Object\",\"description\":\"Object\"},\"type\":\"local\"}");
			l_first = false;
		//}
			if ( ((unsigned int)l_page->fVectFrames[l_i].scope_ids[CLOSURE_SCOPE] != (unsigned int)K_UNDEFINED_SCOPE_ID) )
			{
			//if (!l_first)
				{
				l_msg.data.dataStr += CVSTR(",");
				}
			//else
			//{
				l_first = false;
			//}
				DebugMsg("closure id=%d\n",(unsigned int)l_page->fVectFrames[l_i].scope_ids[CLOSURE_SCOPE]);
				l_msg.data.dataStr += CVSTR("{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":");
				l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].scope_ids[CLOSURE_SCOPE]));
				l_msg.data.dataStr += CVSTR("}\",\"className\":\"Object\",\"description\":\"Object\"},\"type\":\"closure\"}");
			}
			if ( ((unsigned int)l_page->fVectFrames[l_i].scope_ids[GLOBAL_SCOPE] != (unsigned int)K_UNDEFINED_SCOPE_ID) )
			{
			//if (!l_first)
			//{
				l_msg.data.dataStr += CVSTR(",");
				//}
				l_msg.data.dataStr += CVSTR("{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":");
				l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].scope_ids[GLOBAL_SCOPE]));
				l_msg.data.dataStr += CVSTR("}\",\"className\":\"Application\",\"description\":\"Application\"},\"type\":\"global\"}");//GH
			}
			l_msg.data.dataStr += CVSTR("],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":");
			l_msg.data.dataStr.AppendLong((sLONG)(l_page->fVectFrames[l_i].id));
			l_msg.data.dataStr += CVSTR("}\",\"className\":\"Application\",\"description\":\"Application\"}");//GH
		}
		delete (VString*)l_page->fVectFrames[l_i].name_str;
		l_i++;
		if (l_i >= l_size)
		{
			l_msg.data.dataStr += CVSTR("}],");
		}
		else
		{
			l_msg.data.dataStr += CVSTR("},");
		}
	}
#endif
	//l_msg.data._dataStr += CVSTR("\"reason\":\"other\"}}");
	l_res = (l_page->fFifo.Put(l_msg) == VE_OK);
#endif
	return l_res;
}

bool VChromeDebugHandler::SendSource( WAKDebuggerContext_t inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen )
{
	bool	l_res = (fPilot->SendSource(inContext,inSrcId,inData,inLength,inUrl,inUrlLen) == VE_OK);
#if 0//TBC
	DebugContext*	l_ctx = (DebugContext*)inContext;

	l_ctx = DebugContext::Check(l_ctx);
	l_res = (l_ctx != NULL);
	if (!l_res)
	{
		return l_res;
	}

	ChrmDbgHdlPage*	l_page = l_ctx->GetPage();
	ChrmDbgMsg_t	l_msg;

	if (l_page->GetPageNb() < K_NB_MAX_PAGES)
	{
		l_msg.type = SET_SOURCE_MSG;
		l_msg.data.Msg.fSrcId = inSrcId;
		l_msg.data._dataStr = VString( inData, inLength*2,  VTC_UTF_16 );
		l_msg.data._urlStr = VString( inUrl, inUrlLen*2,  VTC_UTF_16 );
		l_res = (l_page->fFifo.Put(l_msg) == VE_OK);
		VTask::Sleep(200);
		xbox_assert(l_res);
	}
	else
	{
		DebugMsg("VChromeDebugHandler::SetSource corrupted data\n");
		xbox_assert(false);
	}
#endif
	return l_res;
}


WAKDebuggerUCharPtr_t VChromeDebugHandler::EscapeForJSON( const unsigned char* inString, int inSize, int & outSize )
{
	VString				vstrInput;
	vstrInput.AppendBlock( inString, 2*inSize, VTC_UTF_16 );

	VString				vstrOutput;
	vstrInput.GetJSONString( vstrOutput );

	outSize = vstrOutput.GetLength();
	WAKDebuggerUCharPtr_t	l_res = new unsigned char[2*(outSize + 1)];
	vstrOutput.ToBlock( l_res, 2*(outSize + 1), VTC_UTF_16, true, false );

	return l_res;
}

void VChromeDebugHandler::DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr )
{
	if (inUCharPtr)
	{
		delete [] inUCharPtr;
	}
}

void* VChromeDebugHandler::UStringToVString( const void* inString, int inSize )
{
	return (void*) new VString(inString,(VSize)inSize,VTC_UTF_16);
}

bool VChromeDebugHandler::Lock()
{
	bool	l_res;

	l_res = sDbgLock.Lock();

	xbox_assert(l_res);
	return l_res;
}
bool VChromeDebugHandler::Unlock()
{
	bool	l_res;

	l_res = sDbgLock.Unlock();

	xbox_assert(l_res);
	return l_res;
}

WAKDebuggerType_t VChromeDebugHandler::GetType()
{
	return WEB_INSPECTOR_TYPE;
}

int	VChromeDebugHandler::StartServer()
{
	fStarted = true;
	return 0;
}
int	VChromeDebugHandler::StopServer()
{
	fStarted = false;
	return 0;
}
short VChromeDebugHandler::GetServerPort()
{
	xbox_assert(false);
	return -1;
}

IWAKDebuggerSettings* VChromeDebugHandler::sStaticSettings = NULL;

void VChromeDebugHandler::StaticSetSettings( IWAKDebuggerSettings* inSettings )
{
	sStaticSettings = inSettings;
}

void VChromeDebugHandler::SetSettings( IWAKDebuggerSettings* inSettings )
{
	//fDebuggerSettings = inSettings;
	xbox_assert(false);
}
void VChromeDebugHandler::SetInfo( IWAKDebuggerInfo* inInfo )
{
	xbox_assert(false);
}

int VChromeDebugHandler::Write ( const char * inData, long inLength, bool inToUTF8 )
{
	xbox_assert(false);
	return 0;
}
int	VChromeDebugHandler::WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize )
{
	xbox_assert(false);
	return 0;
}
int VChromeDebugHandler::WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize )
{
	xbox_assert(false);
	return 0;
}


void VChromeDebugHandler::Trace(	WAKDebuggerContext_t	inContext,
								const void*				inString,
								int						inSize,
								WAKDebuggerTraceLevel_t inTraceLevel )
{
	bool			l_res = (fPilot->Check(inContext) == VE_OK);

#if 1//defined(UNIFIED_DEBUGGER_NET_DEBUG)

	//ChrmDbgHdlPage	*l_page = l_ctx->GetPage();

	VString		l_trace(inString,(VSize)inSize,VTC_UTF_16);
	VString l_tmp = CVSTR("WEBINSP id=");
	if (l_res)
	{
		l_tmp.AppendLong8((sLONG8)inContext);//l_ctx->GetId());
	}
	else
	{
		l_tmp += "BAD_CTX ";
	}
	l_tmp += CVSTR(">>>>' ");
	l_tmp += l_trace;
	l_tmp += CVSTR("'\n");
	sPrivateLogHandler->Put(inTraceLevel,l_tmp);

	DebugMsg(l_tmp);

#endif

}

VChromeDebugHandler* VChromeDebugHandler::Get()
{
	if ( sDebugger == 0 )
	{
		//xbox_assert(false);
	}
	return sDebugger;
}


VChromeDebugHandler* VChromeDebugHandler::Get(
						const XBOX::VString inIP,
						sLONG				inPort,
						const XBOX::VString inDebuggerHTMLSkeleton,
						const XBOX::VString inTracesHTMLSkeleton)
{
	if ( sDebugger == 0 )
	{
		VString		l_port_str("");
		VString		l_content = inDebuggerHTMLSkeleton;
		VString		l_traces_content = inTracesHTMLSkeleton;
		VIndex		l_idx;
		
		l_port_str.AppendLong( inPort );
		
		l_idx = l_traces_content.Find(K_TEMPLATE_WAKSERV_HOST_STR);
		if (l_idx > 1)
		{
			l_traces_content.Replace(inIP,l_idx,K_TEMPLATE_WAKSERV_HOST_STR.GetLength());
			l_idx = l_traces_content.Find(K_TEMPLATE_WAKSERV_PORT_STR);
			if (l_idx > 1)
			{
				l_traces_content.Replace(l_port_str,l_idx,K_TEMPLATE_WAKSERV_PORT_STR.GetLength());
			}
		}
		l_idx = l_content.Find(K_TEMPLATE_WAKSERV_HOST_STR);
		if (l_idx > 1)
		{
			l_content.Replace(inIP,l_idx,K_TEMPLATE_WAKSERV_HOST_STR.GetLength());
			l_idx = l_content.Find(K_TEMPLATE_WAKSERV_PORT_STR);
			if (l_idx > 1)
			{
				l_content.Replace(l_port_str,l_idx,K_TEMPLATE_WAKSERV_PORT_STR.GetLength());
				sPrivateLogHandler = new VLogHandler();
				sDebugger = new VChromeDebugHandler(inIP,inPort,l_content,l_traces_content);
				if (sDebugger)
				{
					XBOX::RetainRefCountable(sDebugger);
				}
			}
		}
	}
	return sDebugger;
}

#else // minimal stubs when not WKA_USE_UNIFIED_DBG

XBOX::VError VChromeDebugHandler::HandleRequest( IHTTPResponse* ioResponse )
{

	return VE_UNKNOWN_ERROR;

}


XBOX::VError VChromeDebugHandler::GetPatterns(XBOX::VectorOfVString *outPatterns) const
{

	return VE_HTTP_INVALID_ARGUMENT;
}
bool VChromeDebugHandler::HasClients()
{
	return false;
}

#endif	//WKA_USE_UNIFIED_DBG

