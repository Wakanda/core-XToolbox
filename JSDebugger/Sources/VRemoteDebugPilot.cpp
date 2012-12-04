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

#include "VRemoteDebugPilot.h"

USING_TOOLBOX_NAMESPACE

extern VLogHandler*			sPrivateLogHandler;

const XBOX::VString			K_CHR_DBG_ROOT("/devtools");

VRemoteDebugPilot::PageDescriptor_t*			VRemoteDebugPilot::sPages = NULL;

XBOX::VError VRemoteDebugPilot::Check(WAKDebuggerContext_t inContext)
{
	bool l_res = false;
	unsigned long long	l_min = (unsigned long long)0;
	unsigned long long	l_max = (unsigned long long)(K_NB_MAX_CTXS-1);
	unsigned long long	l_pos = (unsigned long long)inContext;
	l_res = ( (l_pos >= l_min) && (l_pos <= l_max));

	if (!testAssert(l_res))
	{
		DebugMsg("VRemoteDebugPilot::Check bad ctx !!!!\n");
		xbox_assert(false);
	}
	return l_res;
}


VRemoteDebugPilot::VRemoteDebugPilot(
						CHTTPServer*			inHTTPServer,
						const XBOX::VString		inIP,
						sLONG					inPort) : fCompletedSem(0), fPipeInSem(0), fPipeOutSem(0)
{
	VString l_port_str = CVSTR(":");
	l_port_str.AppendLong(inPort);
	VString l_rel_url;
	
	if (!sPages)
	{
		sPages = new PageDescriptor_t[K_NB_MAX_PAGES];
		for( sLONG l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
		{
			sPages[l_i].used = false;
			l_rel_url = CVSTR("/wka_dbg");
			l_rel_url += K_CHR_DBG_ROOT;
			l_rel_url += CVSTR("/devtools.html?host=");
			l_rel_url += inIP;
			l_rel_url += l_port_str;
			l_rel_url += CVSTR("&page=");
			l_rel_url.AppendLong(l_i);
			sPages[l_i].value.Init((uLONG)l_i,inHTTPServer,l_rel_url);
		}
	}
	fWS = NULL;
	fTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &VRemoteDebugPilot::InnerTaskProc);
	if (fTask)
	{
		fTask->SetKindData((sLONG_PTR)this);
		fTask->Run();
	}
}


#define K_DBG_PROTOCOL_CONNECT_STR	"{\"method\":\"connect\",\"id\":\""
#define K_DBG_PROTOCOL_GET_URL_STR	"{\"method\":\"getURL\",\"id\":\""


sLONG VRemoteDebugPilot::InnerTaskProc(XBOX::VTask* inTask)
{
	unsigned long long						l_id;
	VRemoteDebugPilot*						l_this = (VRemoteDebugPilot*)inTask->GetKindData();
	XBOX::VString							l_client_id;
	typedef struct ContextDesc_st {
		bool				used;
		XBOX::VSemaphore*	sem;
		PageDescriptor_t*	page;
	} ContextDesc_t;
	//std::map<unsigned int,PageDescriptor_t*>	l_hash;
	ContextDesc_t*		l_ctxs;

	XBOX::VError	l_err;
	XBOX::VString	l_resp;
	RemoteDebugPilotMsg_t	l_msg;
	bool		l_msg_found;
	uLONG		l_msg_len;
	bool		l_is_msg_terminated;
	sBYTE*		l_tmp;
	sBYTE*		l_end;
	VSize		l_size;

	l_ctxs = new ContextDesc_t[K_NB_MAX_CTXS];
	for( int l_i=0; l_i<K_NB_MAX_CTXS; l_i++ )
	{
		l_ctxs[l_i].used = false;
		l_ctxs[l_i].sem = NULL;
		l_ctxs[l_i].page = NULL;
	}

wait_connection:
	l_this->fState = STARTED_STATE;
	// clean the pipe
	l_this->Get(&l_msg,&l_msg_found,100);
	while(!l_this->Get(&l_msg,&l_msg_found,500))
	{
		if (l_msg_found)
		{
			l_this->fPipeStatus = VE_INVALID_PARAMETER;

			switch(l_msg.type)
			{
			
			case TREAT_CLIENT_MSG:
				switch(l_this->fState)
				{
				case STARTED_STATE:
					l_err = l_this->fWS->TreatNewConnection(l_msg.response);
					if (!l_err)
					{
						l_this->fState = WORKING_STATE;
						l_client_id = VString("");
						l_this->fPipeStatus = VE_OK;
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
					DebugMsg("VRemoteDebugPilot::InnerTaskProc NEW_CLIENT SHOULD NOT HAPPEN in state=%d !!!\n",l_this->fState);
					xbox_assert(false);
					break;
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case NEW_CONTEXT_MSG:
					l_id = K_NB_MAX_CTXS+1;
					for( unsigned int l_i=0; l_i<K_NB_MAX_CTXS; l_i++ )
					{
						//const std::map<unsigned int,PageDescriptor_t*>::const_iterator l_it = l_hash.find((unsigned int)l_i);
						//if (l_it == l_hash.end())
						if (!l_ctxs[l_i].used)
						{
							l_ctxs[l_i].used = true;
							l_ctxs[l_i].page = NULL;
							l_id = l_i;
							l_this->fPipeStatus = VE_OK;
							break;
						}
					}
					if (l_this->fState == WORKING_STATE)
					{
						if (l_id < K_NB_MAX_CTXS)
						{
							l_resp = CVSTR("{\"method\":\"newContext\",\"contextId\":\"");
							l_resp.AppendLong8(l_id);
							l_resp += CVSTR("\",\"id\":\"");
							l_resp += l_client_id;
							l_resp += CVSTR("\"}");

							*l_msg.outctx = (WAKDebuggerContext_t)l_id;
							VRemoteDebugPilot::SendToBrowser( l_this, l_resp );
						}
					}
					l_this->fPipeOutSem.Unlock();
				break;
			case REMOVE_CONTEXT_MSG:
				{
				//const std::map<unsigned int,PageDescriptor_t*>::const_iterator l_it = l_hash.find((unsigned int)l_msg.ctx);
				//if (l_it != l_hash.end())
				{
					l_id = (unsigned long long)l_msg.ctx;
					if ( l_id < K_NB_MAX_PAGES)
					{
						sPages[(unsigned long long)l_msg.ctx].used = false;
						if (l_ctxs[l_id].page)
						{
							l_ctxs[l_id].page->value.fFifo.Reset();
							l_ctxs[l_id].page->value.fOutFifo.Reset();
							l_ctxs[l_id].page->used = false;
							l_ctxs[l_id].page = NULL;
						}
						/*if (!l_hash.erase((unsigned int)l_msg.ctx))
						{
							DebugMsg("VRemoteDebugPilot::InnerTaskProc map erase pb for ctxid=%d\n",(unsigned int)l_msg.ctx);
						}*/
						l_ctxs[l_id].used = false;
						l_this->fPipeStatus = VE_OK;
					}
					if (l_this->fState == WORKING_STATE)
					{
						l_resp = CVSTR("{\"method\":\"removeContext\",\"contextId\":\"");
						l_resp.AppendLong8(l_id);
						l_resp += CVSTR("\",\"id\":\"");
						l_resp += l_client_id;
						l_resp += CVSTR("\"}");
						VRemoteDebugPilot::SendToBrowser( l_this, l_resp );
					}
				}
				l_this->fPipeOutSem.Unlock();
				}
				break;
			case UPDATE_CONTEXT_MSG:
				*l_msg.outwait = false;
				if (l_this->fState == WORKING_STATE)
				{
					l_id = (unsigned long long)l_msg.ctx;
					if (!l_ctxs[l_id].used)
					{
						xbox_assert(false);
					}
					if (!l_ctxs[l_id].page)
					{
						l_resp = CVSTR("{\"method\":\"updateContext\",\"contextId\":\"");
						l_resp.AppendULong8(l_id);
						l_resp += CVSTR("\",\"id\":\"");
						l_resp += l_client_id;
						l_resp += CVSTR("\"}");
						l_ctxs[l_id].sem = l_msg.sem;
						VRemoteDebugPilot::SendToBrowser( l_this, l_resp );
						*l_msg.outwait = true;
						l_this->fPipeStatus = VE_OK;
					}
					else
					{
						if (!l_ctxs[l_id].page->used || !l_ctxs[l_id].page->value.fEnabled)
						{
							xbox_assert(false);
						}
						else
						{
							l_this->fPipeStatus = VE_OK;
						}
					}
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case SET_STATE_MSG:
				{
					//const std::map<unsigned int,PageDescriptor_t*>::const_iterator l_it = l_hash.find((unsigned int)l_msg.ctx);
					//if (l_it != l_hash.end())
					l_id = (unsigned long long)l_msg.ctx;
					{
						if ( (l_ctxs[l_id].page) && (l_ctxs[l_id].page->used) )
						{
							VChromeDbgHdlPage*	l_page = &l_ctxs[l_id].page->value;
							l_page->fState = l_msg.state;
							l_page->fFifo.Reset();
							l_page->fOutFifo.Reset();
						}
						l_this->fPipeStatus = VE_OK;
					}
					l_this->fPipeOutSem.Unlock();
				}
				break;
			case SEND_EVAL_MSG:
				l_id = (unsigned long long)l_msg.ctx;
				if (l_id < K_NB_MAX_PAGES)
				{
					VChromeDbgHdlPage*	l_page = &l_ctxs[l_id].page->value;
					ChrmDbgMsg_t		l_page_msg;

					l_page_msg.type = EVAL_MSG;
					l_page_msg.data._dataStr = l_msg.datastr;

					bool l_res = (l_page->fFifo.Put(l_page_msg) == VE_OK);
					VString l_trace = VString("VRemoteDebugPilot::InnerTaskProc (SEND_EVAL_MSG) fFifo.put status=");
					l_trace += (l_res ? "1" : "0" );
					sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);
					l_this->fPipeStatus = VE_OK;
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case SEND_LOOKUP_MSG:
				l_id = (unsigned long long)l_msg.ctx;
				if (l_id < K_NB_MAX_PAGES)
				{
					VChromeDbgHdlPage*	l_page = &l_ctxs[l_id].page->value;
					ChrmDbgMsg_t		l_page_msg;

					l_page_msg.type = LOOKUP_MSG;
					l_page_msg.data._dataStr = l_msg.datastr;

					bool l_res = (l_page->fFifo.Put(l_page_msg) == VE_OK);
					VString l_trace = VString("VRemoteDebugPilot::InnerTaskProc (SEND_LOOKUP_MSG) fFifo.put status=");
					l_trace += (l_res ? "1" : "0" );
					sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);
					l_this->fPipeStatus = VE_OK;
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case SEND_CALLSTACK_MSG:
				l_id = (unsigned long long)l_msg.ctx;
				if (l_id < K_NB_MAX_PAGES)
				{
					VChromeDbgHdlPage*	l_page = &l_ctxs[l_id].page->value;
					ChrmDbgMsg_t		l_page_msg;

					l_page_msg.type = CALLSTACK_MSG;
					l_page_msg.data._dataStr = l_msg.datastr;

					bool l_res = (l_page->fFifo.Put(l_page_msg) == VE_OK);
					VString l_trace = VString("VRemoteDebugPilot::InnerTaskProc (SEND_CALLSTACK_MSG) fFifo.put status=");
					l_trace += (l_res ? "1" : "0" );
					sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);
					l_this->fPipeStatus = VE_OK;
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case TREAT_WS_MSG:
				l_id = K_NB_MAX_CTXS+1;
				for( int l_i=0; l_i<K_NB_MAX_CTXS; l_i++ )
				{
					if (l_ctxs[l_i].used && l_ctxs[l_i].page)
					{
						if (l_ctxs[l_i].page->value.GetPageNb() == l_msg.pagenb)
						{
							l_id = l_i;
							l_this->fPipeStatus = VE_OK;
							*l_msg.outpage = &l_ctxs[l_id].page->value;
							*l_msg.outsem = l_ctxs[l_id].sem;
							break;
						}
					}
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case BREAKPOINT_REACHED_MSG:
				l_id = (unsigned long long)l_msg.ctx;
				if (l_id < K_NB_MAX_PAGES)
				{
					VChromeDbgHdlPage*	l_page = &l_ctxs[l_id].page->value;
					ChrmDbgMsg_t		l_page_msg;

					l_page_msg.type = BRKPT_REACHED_MSG;
					l_page_msg.data.Msg.fLineNumber = l_msg.linenb;
					l_page_msg.data.Msg.fSrcId = l_msg.srcid;
					l_page_msg.data._dataStr = l_msg.datastr;
					l_page_msg.data._urlStr = l_msg.urlstr;
					l_page_msg.type = BRKPT_REACHED_MSG;

					l_page->fState = PAUSE_STATE;
					bool l_res = (l_page->fFifo.Put(l_page_msg) == VE_OK);
					VString l_trace = VString("VRemoteDebugPilot::InnerTaskProc (BREAKPOINT_REACHED_MSG) fFifo.put status=");
					l_trace += (l_res ? "1" : "0" );
					sPrivateLogHandler->Put( (!l_res ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);
					l_this->fPipeStatus = VE_OK;
				}
				l_this->fPipeOutSem.Unlock();
				break;
			case WAIT_FROM_MSG:
				l_id = (unsigned long long)l_msg.ctx;
				if (l_id < K_NB_MAX_PAGES)
				{
					if (l_ctxs[l_id].used && l_ctxs[l_id].page)
					{
						{
							l_this->fPipeStatus = VE_OK;
							*l_msg.outpage = &l_ctxs[l_id].page->value;
						}
					}
				}
				l_this->fPipeOutSem.Unlock();
				break;
			default:
				DebugMsg("VRemoteDebugPilot::InnerTaskProc  unknown msg type=%d\n",l_msg.type);
				xbox_assert(false);
				break;
			}
		}
		if (l_this->fState == WORKING_STATE)
		{
			l_msg_len = K_MAX_SIZE;
			l_err = l_this->fWS->ReadMessage((void*)l_this->fData,&l_msg_len,&l_is_msg_terminated);
			if (l_err)
			{
				DebugMsg("VRemoteDebugPilot::InnerTaskProc fWS->ReadMessage pb\n");
				xbox_assert(false);
				l_this->fCompletedSem.Unlock();
				l_this->fState = STARTED_STATE;
				goto wait_connection;
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
				DebugMsg("VRemoteDebugPilot::InnerTaskProc received msg='%s'\n",l_this->fData);
				l_tmp = (sBYTE*)strstr((const char*)l_this->fData,K_DBG_PROTOCOL_CONNECT_STR);
				if (l_tmp && !l_client_id.GetLength())
				{
					l_tmp += strlen(K_DBG_PROTOCOL_CONNECT_STR);
					l_end = (sBYTE*)strchr((const char*)l_tmp,'"');
					*l_end = 0;
					l_resp = CVSTR("{\"result\":\"ok\",\"id\":\"");
					l_tmp[K_MAX_SIZE] = 0;
					l_client_id = VString(l_tmp);
					l_resp += l_client_id;
					l_resp += CVSTR("\"}");
					/*l_size = l_resp.GetLength() * 3; // overhead between VString's UTF_16 to CHAR* UTF_8 
					if (l_size >= K_MAX_SIZE)
					{
						l_size = K_MAX_SIZE;
						xbox_assert(false);
					}
					l_size = l_resp.ToBlock(l_this->fData,l_size,VTC_UTF_8,false,false);
					l_err = l_this->fWS->WriteMessage(l_this->fData,l_size,true);*/
					l_err = VRemoteDebugPilot::SendToBrowser( l_this, l_resp );
					if (l_err)
					{
						l_this->fCompletedSem.Unlock();
						l_this->fState = STARTED_STATE;
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
						l_id = atoi(l_tmp);
						//std::map<unsigned int,PageDescriptor_t*>::iterator l_it = l_hash.find(l_tmp_id);
						if (l_ctxs[l_id].used)
						{
							if (l_ctxs[l_id].page)
							{
								xbox_assert(false);
							}
							else
							{
								for( int l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
								{
									if (!sPages[l_i].used)
									{
										sPages[l_i].used = true;
										l_ctxs[l_id].page = sPages + l_i;
										sPages[l_i].value.fFifo.Reset();
										sPages[l_i].value.fOutFifo.Reset();
										l_resp = CVSTR("{\"method\":\"setURLContext\",\"contextId\":\"");
										l_resp.AppendULong8(l_id);
										l_resp += CVSTR("\",\"url\":\"");//http://");
										l_resp += sPages[l_i].value.fRelURL;
										l_resp += CVSTR("\",\"id\":\"");
										l_resp += l_client_id;
										l_resp += CVSTR("\"}");
										VRemoteDebugPilot::SendToBrowser( l_this, l_resp );
										break;
									}
								}
							}
						}
						else
						{
							DebugMsg("VRemoteDebugPilot::InnerTaskProc  unknwon context\n");
							xbox_assert(false);
						}
					}
					else
					{
						DebugMsg("VRemoteDebugPilot::InnerTaskProc received msg='%s'\n",l_this->fData);
					}
				}
			}
		}
	}
	DebugMsg("VRemoteDebugPilot::InnerTaskProc fifo pb\n");
	xbox_assert(false);
	return 0;
}

XBOX::VError VRemoteDebugPilot::TreatRemoteDbg(IHTTPResponse* ioResponse)
{
	RemoteDebugPilotMsg_t	l_msg;

	if (!fSem.TryToLock())
	{
		DebugMsg("VRemoteDebugPilot::TreatRemoteDbg already in use\n"); 
		return VE_UNKNOWN_ERROR;
	}

	l_msg.type = TREAT_CLIENT_MSG;
	l_msg.response = ioResponse;
	if (!Send(l_msg))
	{
		fCompletedSem.Lock();
	}
	fSem.Unlock();
	return XBOX::VE_OK;
}


WAKDebuggerServerMessage* VRemoteDebugPilot::WaitFrom(WAKDebuggerContext_t inContext)
{
	//VTask::Sleep(2000);
	WAKDebuggerServerMessage*	l_res = NULL;
	RemoteDebugPilotMsg_t		l_msg;
	VChromeDbgHdlPage*			l_page;	
	

	//memset(&l_msg,0,sizeof(l_msg));
	l_msg.type = WAIT_FROM_MSG;
	l_msg.ctx = inContext;
	l_msg.outpage = &l_page;

	VError	l_err = Send(l_msg);

	if (!l_err)
	{
		while (!l_page->fEnabled)
		{
			VTask::Sleep(200);
		}

		ChrmDbgMsg_t					l_page_msg;
		bool							l_msg_found;

		l_res = (WAKDebuggerServerMessage*)malloc(sizeof(WAKDebuggerServerMessage));

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
		//xbox_assert(l_page->fActive);
		if (l_page && l_page->fState == PAUSE_STATE)
		{
			l_page->fOutFifo.Get(&l_page_msg,&l_msg_found,0);
			if (!l_msg_found)
			{
				l_page_msg.type = NO_MSG;
			}
			if (l_page_msg.type == SEND_CMD_MSG)
			{
				memcpy( l_res, &l_page_msg.data.Msg, sizeof(*l_res) );
			}
			else
			{
				xbox_assert(false);
			}

		}
		else
		{
			//xbox_assert(false);
		}
	}
#endif
	return l_res;
}

XBOX::VError VRemoteDebugPilot::SendEval( WAKDebuggerContext_t inContext, void* inVars )
{
	VError	l_err = VRemoteDebugPilot::Check(inContext);
	if (!l_err)
	{
		return l_err;
	}
	RemoteDebugPilotMsg_t	l_msg;
	l_msg.type = SEND_EVAL_MSG;
	l_msg.ctx = inContext;
	l_msg.datastr = *(VString*)(inVars);

	delete (VString*)(inVars);

	l_err = Send( l_msg );
	return l_err;
}

XBOX::VError VRemoteDebugPilot::SendSource(		WAKDebuggerContext_t inContext,
												intptr_t inSrcId,
												const char *inData,
												int inLength,
												const char* inUrl,
												unsigned int inUrlLen )
{

	VError	l_err;
	xbox_assert(false);// should not pass here
	return l_err;
}

XBOX::VError VRemoteDebugPilot::SendLookup( WAKDebuggerContext_t inContext, void* inVars, unsigned int inSize )
{
	VError	l_err = VRemoteDebugPilot::Check(inContext);
	if (!l_err)
	{
		return l_err;
	}
	RemoteDebugPilotMsg_t	l_msg;
	l_msg.type = SEND_LOOKUP_MSG;
	l_msg.ctx = inContext;

	l_msg.datastr = *((VString*)inVars);
	delete (VString*)(inVars);
	l_err = Send( l_msg );
	return l_err;
}

XBOX::VError VRemoteDebugPilot::SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength )
{
	VError	l_err = VRemoteDebugPilot::Check(inContext);
	if (!l_err)
	{
		return l_err;
	}
	RemoteDebugPilotMsg_t	l_msg;
	l_msg.type = SEND_CALLSTACK_MSG;
	l_msg.ctx = inContext;

	l_msg.datastr = VString( inData, inLength*2,  VTC_UTF_16 );
	l_err = Send( l_msg );
	return l_err;
}


XBOX::VError VRemoteDebugPilot::SetState(WAKDebuggerContext_t inContext, WAKDebuggerState_t state)
{
	VError	l_err = VRemoteDebugPilot::Check(inContext);
	if (!l_err)
	{
		return l_err;
	}
	RemoteDebugPilotMsg_t	l_msg;
	l_msg.type = SET_STATE_MSG;
	l_msg.ctx = inContext;
	l_msg.state = state;
	l_err = Send( l_msg );
	return l_err;
}

XBOX::VError VRemoteDebugPilot::BreakpointReached(
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
											long 					inEndOffset )
{

	VError	l_err = VRemoteDebugPilot::Check(inContext);
	if (!l_err)
	{
		return l_err;
	}

	bool	l_wait;
	XBOX::VSemaphore*	l_sem = new VSemaphore(0);

	l_err = ContextUpdated(inContext,l_sem,&l_wait);
	if (!l_err)
	{
		if (l_wait)
		{
			l_sem->Lock();
			VTask::Sleep(2500);
			delete l_sem;
		}
		RemoteDebugPilotMsg_t	l_msg;
		l_msg.type = BREAKPOINT_REACHED_MSG;
		l_msg.ctx = inContext;
		l_msg.linenb = inLineNumber;
		l_msg.srcid = *(intptr_t *)inURL;// we use the URL param to pass sourceId (to not modify interface of server)
		l_msg.datastr = VString( inFunction, inFunctionLength*2,  VTC_UTF_16 );
		l_msg.urlstr = VString( inMessage, inMessageLength*2,  VTC_UTF_16 );
		l_err = Send( l_msg );
	}
	VString l_trace = VString("VRemoteDebugPilot::BreakpointReached ctx activation pb for ctxid=");
	l_trace.AppendLong8((unsigned long long)inContext);
	l_trace += " ,ok=";
	l_trace += (!l_err ? "1" : "0" );
	sPrivateLogHandler->Put( (l_err ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);

	return l_err;
}

XBOX::VError VRemoteDebugPilot::TreatWS(IHTTPResponse* ioResponse, uLONG inPageNb )
{

	if (inPageNb >= K_NB_MAX_PAGES)
	{
		xbox_assert(false);
		return VE_INVALID_PARAMETER;
	}
	
	VError					l_err;
	RemoteDebugPilotMsg_t	l_msg;
	VChromeDbgHdlPage*		l_page;
	XBOX::VSemaphore*		l_sem = NULL;
	//memset(&l_msg,0,sizeof(l_msg));
	l_msg.type = TREAT_WS_MSG;
	l_msg.pagenb = inPageNb;
	l_msg.outpage = &l_page;
	l_msg.outsem = &l_sem;
	l_err = Send( l_msg );
	if (!l_err)
	{
		// attention : debloquer aussi si err
		l_sem->Unlock();
		l_page->TreatWS(ioResponse);
	}
	else
	{
		xbox_assert(false);
	}

	return VE_OK;
}