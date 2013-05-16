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

sLONG						VRemoteDebugPilot::sRemoteDebugPilotContextId=1;


static bool VRemoteDebugPilotIsInit = false;
VRemoteDebugPilot::VRemoteDebugPilot(
				CHTTPServer*			inHTTPServer) :
		fUniqueClientConnexionSemaphore(1),
		fClientCompletedSemaphore(0),
		fPipeInSem(0), 
		fPipeOutSem(0), 
		fTmpData(NULL), 
		fPendingContextsNumberSem(0,1000000),
		fDebuggingEventsTimeStamp(1)
{
	
	fHTTPServer = inHTTPServer;
	if (!VRemoteDebugPilotIsInit)
	{
		VRemoteDebugPilotIsInit = true;
	}
	fWS = NULL;
	fTask = new XBOX::VTask(this, 64000, XBOX::eTaskStylePreemptive, &VRemoteDebugPilot::InnerTaskProc);
	if (fTask)
	{
		fTask->SetKindData((sLONG_PTR)this);
		fTask->Run();
	}
}


#define K_DBG_PROTOCOL_CONNECT_STR		"{\"method\":\"connect\",\"id\":\""
#define K_DBG_PROTOCOL_GET_URL_STR		"{\"method\":\"getURL\",\"id\":\""
#define K_DBG_PROTOCOL_DISCONNECT_STR	"{\"method\":\"disconnect\",\"id\":\""
#define K_DBG_PROTOCOL_ABORT_STR		"{\"method\":\"abort\",\"id\":\""

void VRemoteDebugPilot::AbortContexts(bool inOnlyDebuggedOnes)
{
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator ctxIt = fContextArray.begin();

	while( ctxIt != fContextArray.end() )
	{
		if (fState == STARTED_STATE)
		{
			if ((*ctxIt).second->fSem)
			{
				(*ctxIt).second->fSem->Unlock();
				ReleaseRefCountable(&(*ctxIt).second->fSem);
			}
		}
		else
		{
			if (!(*ctxIt).second->fPage)
			{
				if ((*ctxIt).second->fSem)
				{
					if (!inOnlyDebuggedOnes)
					{
						(*ctxIt).second->fSem->Unlock();
						ReleaseRefCountable(&(*ctxIt).second->fSem);
					}
				}
				ctxIt++;
				continue;
			}
			VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
			testAssert(page->Abort() == VE_OK);
		}
		ctxIt->second->Release();
		ctxIt++;
	}
	//if (fState == STARTED_STATE)
	{
		fContextArray.clear();
	}
}

void VRemoteDebugPilot::TreatStartMsg(RemoteDebugPilotMsg_t& ioMsg,VPilotLogListener* inLogListener)
{
	switch(fState)
	{
	case STOPPED_STATE:
		fState = STARTED_STATE;
		VProcess::Get()->GetLogger()->AddLogListener(inLogListener);
	case STARTED_STATE:
		break;
	case CONNECTED_STATE:
	case CONNECTING_STATE:
	case DISCONNECTING_STATE:
		// should not happen
		break;
	}
	fPipeStatus = VE_OK;
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::TreatAbortAllMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	switch(fState)
	{
	case STOPPED_STATE:
		break;
	case CONNECTED_STATE:
	case CONNECTING_STATE:
	case STARTED_STATE:
		AbortContexts(false);
	}
	fPipeStatus = VE_OK;
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::TreatStopMsg(RemoteDebugPilotMsg_t& ioMsg,VPilotLogListener* inLogListener)
{
	XBOX::VError	err;
	switch(fState)
	{
	case STOPPED_STATE:
		break;
	case CONNECTED_STATE:
	case CONNECTING_STATE:
	case DISCONNECTING_STATE:
		VProcess::Get()->GetLogger()->RemoveLogListener(inLogListener);
		VRemoteDebugPilot::AbortContexts(false);
		err = fWS->Close();
		fState = STOPPED_STATE;
		fClientCompletedSemaphore.Unlock();// there is only a caller to unlock on thyese states
		break;
	case STARTED_STATE:
		VProcess::Get()->GetLogger()->RemoveLogListener(inLogListener);
		VRemoteDebugPilot::AbortContexts(false);
		fState = STOPPED_STATE;
		break;
	}
	fPipeStatus = VE_OK;
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::TreatNewClientMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	XBOX::VError			err;
	
	switch(fState)
	{
	case STARTED_STATE:
		err = fWS->TreatNewConnection(ioMsg.response);
		if (!err)
		{
			fNeedsAuthentication = ioMsg.needsAuthentication;
			fState = CONNECTING_STATE;
			fClientId = VString("");
			fPipeStatus = VE_OK;
		}
		else
		{
			fClientCompletedSemaphore.Unlock();
		}
		break;
	case STOPPED_STATE:
		DebugMsg("VRemoteDebugPilot::TreatNewClientMsg NEW_CLIENT SHOULD NOT HAPPEN in state=%d !!!\n",fState);
		fClientCompletedSemaphore.Unlock();
		break;
	case CONNECTED_STATE:
	case DISCONNECTING_STATE:
	case CONNECTING_STATE:
		DebugMsg("VRemoteDebugPilot::TreatNewClientMsg NEW_CLIENT SHOULD NOT HAPPEN in state=%d !!!\n",fState);
		xbox_assert(false);
		break;
	}
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::TreatNewContextMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	XBOX::VError			err;
	XBOX::VString			resp;

	if (fState != STOPPED_STATE)
	{
		if ( testAssert(fContextArray.size() < K_NB_MAX_CTXS) )
		{

			VContextDescriptor*		newCtx = new VContextDescriptor();
			*(ioMsg.outctx) = (OpaqueDebuggerContext)sRemoteDebugPilotContextId;
			sRemoteDebugPilotContextId++;
			std::pair< OpaqueDebuggerContext, VContextDescriptor* >	l_pair( *(ioMsg.outctx), newCtx);
			std::pair< std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator, bool >	newElt =
					fContextArray.insert( l_pair );

			if (testAssert(newElt.second))
			{
				if (fState == CONNECTED_STATE)
				{
					resp = CVSTR("{\"method\":\"newContext\",\"contextId\":\"");
					resp.AppendLong8((sLONG8)(*(ioMsg.outctx)));
					resp += CVSTR("\",\"id\":\"");
					resp += fClientId;
					resp += CVSTR("\"}");

					err = SendToBrowser( resp );
					if (err)
					{
						fWS->Close();
						fState = DISCONNECTING_STATE;
					}
				}
				else
				{
					err = VE_OK;
				}
				if (!err)
				{
					fPipeStatus = VE_OK;
				}
			}
		}
	}
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::RemoveContext(std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator& inCtxIt)
{
	XBOX::VError			err;
	XBOX::VString			resp;

	if (fState != STOPPED_STATE)
	{
		// ctx could already have been removed by ABORT
		if (inCtxIt != fContextArray.end())
		{
			if ((*inCtxIt).second->fPage)
			{
				ReleaseRefCountable(&(*inCtxIt).second->fPage);
			}

			if (fState == CONNECTED_STATE)
			{
				resp = CVSTR("{\"method\":\"removeContext\",\"contextId\":\"");
				resp.AppendLong8((sLONG8)((*inCtxIt).first));
				resp += CVSTR("\",\"id\":\"");
				resp += fClientId;
				resp += CVSTR("\"}");
				err = SendToBrowser( resp );
				if (err)
				{
					fWS->Close();
					fState = DISCONNECTING_STATE;
				}
			}
			inCtxIt->second->Release();
			fContextArray.erase(inCtxIt);
		}
	}
}

void VRemoteDebugPilot::TreatDeactivateContextMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator ctxIt = fContextArray.find(ioMsg.ctx);
	XBOX::VError			err;
	XBOX::VString			resp;

	if (fState != STOPPED_STATE)
	{
		if (ctxIt != fContextArray.end())
		{
			if (!ioMsg.hideOnly)
			{
				VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
				if (page)
				{
					page->Stop();
					ReleaseRefCountable(&(*ctxIt).second->fPage);
				}
			}
			fPipeStatus = VE_OK;

			if ( (fState == CONNECTED_STATE) && ((*ctxIt).second->fUpdated) )
			{
				resp = CVSTR("{\"method\":\"hideContext\",\"contextId\":\"");
				resp.AppendLong8((sLONG8)((*ctxIt).first));
				resp += CVSTR("\",\"id\":\"");
				resp += fClientId;
				resp += CVSTR("\"}");
				err = SendToBrowser( resp );
				if (err)
				{
					fWS->Close();
					fState = DISCONNECTING_STATE;
				}
			}
			(*ctxIt).second->fUpdated = false;
		}
	}
	fPipeOutSem.Unlock();
}
void VRemoteDebugPilot::TreatRemoveContextMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator ctxIt = fContextArray.find(ioMsg.ctx);
	RemoveContext(ctxIt);
	fPipeStatus = VE_OK;
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::DisconnectBrowser()
{
	fWS->Close();
	if (fState > STARTED_STATE)
	{
		fClientCompletedSemaphore.Unlock();
	}
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.begin();
	while( ctxIt != fContextArray.end()) 
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if (page)
		{
			page->Stop();
			ReleaseRefCountable(&(*ctxIt).second->fPage);
		}
		ctxIt++;
	}
	fState = STARTED_STATE;

}

void VRemoteDebugPilot::TreatUpdateContextMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	XBOX::VError			err;
	XBOX::VString			resp;

	*ioMsg.outsem = NULL;

	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(ioMsg.ctx);

	if ( ctxIt != fContextArray.end())
	{
		(*ctxIt).second->fUpdated = true;

		switch(fState)
		{
		case STOPPED_STATE:
			xbox_assert(false);
			break;

		case CONNECTING_STATE:
		case DISCONNECTING_STATE:
		case STARTED_STATE:
			(*ctxIt).second->fDebugInfos = ioMsg.debugInfos;
			testAssert( (*ctxIt).second->fSem == NULL );
			(*ctxIt).second->fSem = new XBOX::VSemaphore(0);
			(*ctxIt).second->fSem->Retain();
			*ioMsg.outsem = (*ctxIt).second->fSem;
			fPipeStatus = VE_OK;
			break;

		case CONNECTED_STATE:
			if ( !(*ctxIt).second->fPage )
			{
				(*ctxIt).second->fDebugInfos = ioMsg.debugInfos;
				testAssert((*ctxIt).second->fSem == NULL);
				err = SendContextToBrowser(ctxIt);

				if (err)
				{
					fWS->Close();
					fState = DISCONNECTING_STATE;
				}
				(*ctxIt).second->fSem = new XBOX::VSemaphore(0);
				(*ctxIt).second->fSem->Retain();
				*ioMsg.outsem = (*ctxIt).second->fSem;
				fPipeStatus = VE_OK;
			}
			else
			{
				testAssert( (*ctxIt).second->fSem == NULL );
				(*ctxIt).second->fDebugInfos = ioMsg.debugInfos;
				err = SendContextToBrowser(ctxIt);
				if (err)
				{
					fWS->Close();
					fState = DISCONNECTING_STATE;
				}
				fPipeStatus = VE_OK;
			}
			break;
		}
	}
	fPipeOutSem.Unlock();
	//DisplayContexts();	
}


void VRemoteDebugPilot::TreatTreatWSMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	if (fState == CONNECTED_STATE)
	{
		std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.begin();
		while( ctxIt != fContextArray.end() )
		{
			if ((*ctxIt).second->fPage)
			{
				if ((*ctxIt).second->fPage->GetPageNb() == ioMsg.pagenb)
				{
					fPipeStatus = VE_OK;
					(*ctxIt).second->fPage->Retain();
					*ioMsg.outpage = (*ctxIt).second->fPage;
					*ioMsg.outsem = (*ctxIt).second->fSem;
					(*ctxIt).second->fSem = NULL;
					break;
				}
			}
			ctxIt++;
		}
	}
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::TreatBreakpointReachedMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(ioMsg.ctx);

	if (testAssert(ctxIt != fContextArray.end()))
	{
		if (fState == CONNECTED_STATE)
		{
			if ( testAssert((*ctxIt).second->fPage) )
			{
				VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
				{
					fPipeStatus = page->BreakpointReached(ioMsg.linenb,ioMsg.datavectstr,ioMsg.urlstr,ioMsg.datastr);
				}
			}
		}
		//testAssert(fPipeStatus == VE_OK);
	}
	fPipeOutSem.Unlock();
}


void VRemoteDebugPilot::TreatWaitFromMsg(RemoteDebugPilotMsg_t& ioMsg)
{
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(ioMsg.ctx);

	if (testAssert(ctxIt != fContextArray.end()))
	{
		if (fState == CONNECTED_STATE)
		{
			if ( (*ctxIt).second->fPage )
			{
				fPipeStatus = VE_OK;
				*ioMsg.outpage = (*ctxIt).second->fPage;
				(*ctxIt).second->fPage->Retain();
			}
		}
	}
	fPipeOutSem.Unlock();
}

void VRemoteDebugPilot::TreatTraces()
{
	fTracesLock.Lock();
	if (fState == STOPPED_STATE)
	{
		for( std::vector<const VValueBag*>::size_type idx = 0; idx < fTraces.size(); idx++ )
		{
			fTraces[idx]->Release();
		}
	}
	else
	{
		VChromeDbgHdlPage*	page = NULL;

		//std::map< VChromeDbgHdlPage*, std::vector<const VValueBag*> >	tracesMap;
		for( std::vector<const VValueBag*>::size_type idx = 0; idx < fTraces.size(); idx++ )
		{
			sLONG8	debugContext;
			if (ILoggerBagKeys::debug_context.Get( fTraces[idx], debugContext) )
			{
				std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find((OpaqueDebuggerContext)debugContext);
				if (ctxIt != fContextArray.end())
				{
					/*page = (*ctxIt).second->fPage;
					if (page)
					{
						std::map< VChromeDbgHdlPage*, std::vector<const VValueBag*> >::iterator itContext = tracesMap.find(page);
						if (itContext == tracesMap.end())
						{
							tracesMap.insert(std::pair< VChromeDbgHdlPage*, std::vector<const VValueBag*> >( page, std::vector<const VValueBag*>() ) );
							itContext = tracesMap.find(page);
						}
						if (testAssert(itContext != tracesMap.end()))
						{
							//(*ioMsg.values)[idx]->Retain();
							(*itContext).second.push_back(fTraces[idx]);
							fTraces[idx] = NULL;
						}
					}*/
					(*ctxIt).second->fTraceContainer->Put(fTraces[idx]);
					fTraces[idx] = NULL;
				}
			}
			if (fTraces[idx])
			{
				fTraces[idx]->Release();
			}
		}
		/*std::map< VChromeDbgHdlPage*, std::vector<const VValueBag*> >::iterator itDebugContexts = tracesMap.begin();
		while(itDebugContexts != tracesMap.end() )
		{
			(*itDebugContexts).first->SendTraces((*itDebugContexts).second);
			itDebugContexts++;
		}*/
	
	}
	fTraces.clear();
	fTracesLock.Unlock();
}

void VRemoteDebugPilot::HandleDisconnectingState()
{
	XBOX::VError			err;
	VSize					msgLen;
	bool					isMsgTerminated;
	sBYTE*					tmpStr;

	msgLen = K_MAX_SIZE;
	tmpStr = NULL;

	err = fWS->ReadMessage((void*)fTmpData,msgLen,isMsgTerminated);
	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleDisconnectingState fWS->ReadMessage pb\n");
		//AbortContexts(true);
		DisconnectBrowser();
		return;
	}
	if (msgLen)
	{
		if (msgLen < K_MAX_SIZE)
		{
			fTmpData[msgLen] = 0;
		}
		else
		{
			fTmpData[K_MAX_SIZE-1] = 0;
		}
		DebugMsg("VRemoteDebugPilot::HandleDisconnectingState received msg(len=%d)='%s'\n",msgLen,fTmpData);
		tmpStr = (sBYTE*)strstr((const char*)fTmpData,K_DBG_PROTOCOL_DISCONNECT_STR);
	}
	else
	{
		XBOX::VTime			curtTime;
		XBOX::VDuration		disconnectDuration;
		curtTime.Substract(fDisconnectTime,disconnectDuration);
		if (disconnectDuration.ConvertToSeconds() > 2)
		{
			tmpStr = fTmpData;
		}
	}
	if (tmpStr)
	{
		//AbortContexts(true);
		DisconnectBrowser();
		return;
	}
}

void VRemoteDebugPilot::HandleConnectingState()
{
	XBOX::VError			err;
	VSize					msgLen;
	bool					isMsgTerminated;
	sBYTE*					tmpStr;
	sBYTE*					endStr;
	unsigned long long		id;
	XBOX::VString			resp;

	msgLen = K_MAX_SIZE;
	err = fWS->ReadMessage((void*)fTmpData,msgLen,isMsgTerminated);
	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleConnectingState fWS->ReadMessage pb\n");
	}

	if (!err && msgLen)
	{
		if (msgLen < K_MAX_SIZE)
		{
			fTmpData[msgLen] = 0;
		}
		else
		{
			fTmpData[K_MAX_SIZE-1] = 0;
		}
		DebugMsg("VRemoteDebugPilot::HandleConnectingState received msg(len=%d)='%s'\n",msgLen,fTmpData);
		tmpStr = (sBYTE*)strstr((const char*)fTmpData,K_DBG_PROTOCOL_CONNECT_STR);
		if (tmpStr && !fClientId.GetLength())
		{
			tmpStr += strlen(K_DBG_PROTOCOL_CONNECT_STR);
			endStr = (sBYTE*)strchr((const char*)tmpStr,'"');
			if (endStr)
			{
				*endStr = 0;
				resp = CVSTR("{\"result\":\"ok\",\"solution\":\"");
				resp += fSolutionName;
				resp += ("\",\"needsAuthentication\":");
				resp += ( fNeedsAuthentication ? CVSTR("true") : CVSTR("false") );
				resp += (",\"id\":\"");
				fClientId = VString(tmpStr);
				resp += fClientId;
				resp += CVSTR("\"}");
				err = SendToBrowser( resp );
				if (!err)
				{
					if (fNeedsAuthentication)
					{
						fState = DISCONNECTING_STATE;
						fDisconnectTime.FromSystemTime();
						return;
					}
					else
					{
						//DisplayContexts();

						// here the client connection has been accepted without need of authentication
						// we send the already existing contexts
						std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.begin();
						while( !err && (ctxIt != fContextArray.end()) )
						{
							resp = CVSTR("{\"method\":\"newContext\",\"contextId\":\"");
							resp.AppendLong8((sLONG8)((*ctxIt).first));
							resp += CVSTR("\",\"id\":\"");
							resp += fClientId;
							resp += CVSTR("\"}");
							err = SendToBrowser( resp );

							if ( !err && (*ctxIt).second->fUpdated )
							{
								if ((*ctxIt).second->fSem == NULL)
								{
									(*ctxIt).second->fSem = new XBOX::VSemaphore(0);
									(*ctxIt).second->fSem->Retain();
								}
								err = SendContextToBrowser(ctxIt);
							}
							ctxIt++;
						}
						if (!err)
						{
							fState = CONNECTED_STATE;
						}
					}
				}
			}
			else
			{
				err = VE_INVALID_PARAMETER;
			}
		}
		else
		{
			DebugMsg("VRemoteDebugPilot::HandleConnectingState first message shoud be 'connect'\n");
			err = VE_INVALID_PARAMETER;
		}
	}

	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleConnectingState fWS->ReadMessage pb\n");
		fWS->Close();
		fState = DISCONNECTING_STATE;
	}
}

void VRemoteDebugPilot::HandleConnectedState()
{
	XBOX::VError			err;
	VSize					msgLen;
	bool					isMsgTerminated;
	sBYTE*					tmpStr;
	sBYTE*					endStr;
	unsigned long long		id;
	XBOX::VString			resp;

	msgLen = K_MAX_SIZE;
	err = fWS->ReadMessage((void*)fTmpData,msgLen,isMsgTerminated);
	if (err)
	{
		DebugMsg("VRemoteDebugPilot::HandleConnectedState fWS->ReadMessage pb\n");
		fWS->Close();
		fState = DISCONNECTING_STATE;
		return;
	}
	if (msgLen)
	{
		if (msgLen < K_MAX_SIZE)
		{
			fTmpData[msgLen] = 0;
		}
		else
		{
			fTmpData[K_MAX_SIZE-1] = 0;
		}
		DebugMsg("VRemoteDebugPilot::HandleConnectedState received msg(len=%d)='%s'\n",msgLen,fTmpData);

		{
			tmpStr = strstr(fTmpData,K_DBG_PROTOCOL_ABORT_STR);
			if (tmpStr && fClientId.GetLength())
			{
				tmpStr += strlen(K_DBG_PROTOCOL_ABORT_STR);
				endStr = strchr(tmpStr,'"');
				*endStr = 0;
				id = atoi(tmpStr);
				std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find((OpaqueDebuggerContext)id);
				if ( testAssert(ctxIt != fContextArray.end()) )
				{
					if (!(*ctxIt).second->fPage)
					{
						if ((*ctxIt).second->fSem)
						{
							(*ctxIt).second->fSem->Unlock();
							ReleaseRefCountable(&(*ctxIt).second->fSem);
						}
						RemoveContext(ctxIt);
					}
					else
					{
						VChromeDbgHdlPage*	l_page = (*ctxIt).second->fPage;
						testAssert(l_page->Abort() == VE_OK);
					}
					//RemoveContext(id);
				}
			}
			else
			{
				tmpStr = strstr(fTmpData,K_DBG_PROTOCOL_GET_URL_STR);
				if (tmpStr && fClientId.GetLength())
				{
					tmpStr += strlen(K_DBG_PROTOCOL_GET_URL_STR);
					endStr = strchr(tmpStr,'"');
					*endStr = 0;
					id = atoi(tmpStr);
					std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find((OpaqueDebuggerContext)id);
					if ( testAssert(ctxIt != fContextArray.end()) )
					{
						// test if a a page has already been attributed to this context
						if (testAssert(!(*ctxIt).second->fPage))
						{
							xbox_assert((*ctxIt).second->fTraceContainer);
							(*ctxIt).second->fPage = new VChromeDbgHdlPage((*ctxIt).second->fTraceContainer);
							(*ctxIt).second->fPage->Init(id,fHTTPServer,NULL);
							err = (*ctxIt).second->fPage->Start((OpaqueDebuggerContext)id);
							/*sPages[l_i].value.fFifo.Reset();
							sPages[l_i].value.fOutFifo.Reset();*/
						}
						if (!err)
						{
							resp = CVSTR("{\"method\":\"setURLContext\",\"contextId\":\"");
							resp.AppendLong8(id);
							resp += CVSTR("\",\"url\":\"");
							resp.AppendLong8((*ctxIt).second->fPage->GetPageNb());
							resp += CVSTR("\",\"id\":\"");
							resp += fClientId;
							resp += CVSTR("\"}");
							err = SendToBrowser( resp );
						}

						if (err)
						{
							fWS->Close();
							fState = DISCONNECTING_STATE;
							return;
						}
					}
					else
					{
						DebugMsg("VRemoteDebugPilot::HandleConnectedState unknown context = %d\n",id);
					}
				}
				else
				{
					DebugMsg("VRemoteDebugPilot::HandleConnectedState received msg2(len=%d)='%s'\n",msgLen,fTmpData);
				}
			}
		}
	}
}

VRemoteDebugPilot::VPilotLogListener::VPilotLogListener(VRemoteDebugPilot* inPilot)
{
	fPilot = inPilot;
}

void VRemoteDebugPilot::VPilotLogListener::Put( std::vector<const VValueBag*>& inValuesVector )
{
	RemoteDebugPilotMsg_t	msg;
	if (inValuesVector.size() > 0)
	{
		fPilot->fTracesLock.Lock();
		//msg.values = new std::vector<const VValueBag*>;
		for( std::vector<const VValueBag*>::size_type idx = 0; idx < inValuesVector.size(); idx++ )
		{
			inValuesVector[idx]->Retain();
			fPilot->fTraces.push_back(inValuesVector[idx]);
			//msg.values->push_back(inValuesVector[idx]);
		}
		fPilot->fTracesLock.Unlock();
		//msg.type = PUT_TRACES_MSG;
		//fPilot->Send(msg);
	}
}


sLONG VRemoteDebugPilot::InnerTaskProc(XBOX::VTask* inTask)
{
	bool					msgFound;
	RemoteDebugPilotMsg_t	msg;

	VRemoteDebugPilot*		l_this = (VRemoteDebugPilot*)inTask->GetKindData();

	VPilotLogListener		logListener(l_this);

	l_this->fTmpData = new sBYTE[K_MAX_SIZE];
	l_this->fState = STOPPED_STATE;

wait_connection:

	while(!l_this->Get(msg,msgFound,500))
	{
		if (msgFound)
		{
			l_this->fLock.Lock();

			l_this->fPipeStatus = VE_INVALID_PARAMETER;

			switch(msg.type)
			{

			case START_MSG:
				l_this->TreatStartMsg(msg,&logListener);
				break;

			case ABORT_ALL_MSG:
				l_this->TreatAbortAllMsg(msg);
				break;

			case STOP_MSG:
				l_this->TreatStopMsg(msg,&logListener);
				break;

			case TREAT_NEW_CLIENT_MSG:
				l_this->TreatNewClientMsg(msg);
				break;

			case NEW_CONTEXT_MSG:
				l_this->TreatNewContextMsg(msg);
				break;

			case REMOVE_CONTEXT_MSG:
				l_this->TreatRemoveContextMsg(msg);
				break;

			case DEACTIVATE_CONTEXT_MSG:
				l_this->TreatDeactivateContextMsg(msg);
				break;

			case UPDATE_CONTEXT_MSG:
				l_this->fDebuggingEventsTimeStamp++;
				l_this->TreatUpdateContextMsg(msg);
				break;

			case TREAT_WS_MSG:
				l_this->TreatTreatWSMsg(msg);
				break;

			case BREAKPOINT_REACHED_MSG:
				l_this->TreatBreakpointReachedMsg(msg);
				break;

			case WAIT_FROM_MSG:
				l_this->TreatWaitFromMsg(msg);
				break;

			//case PUT_TRACES_MSG:
			//	l_this->TreatTracesMsg(msg);
			//	break;

			default:
				DebugMsg("VRemoteDebugPilot::InnerTaskProc  unknown msg type=%d\n",msg.type);
				xbox_assert(false);
				l_this->fPipeOutSem.Unlock();
				break;
			}
			l_this->TreatTraces();
			l_this->fLock.Unlock();
		}

		if (l_this->fState == DISCONNECTING_STATE)
		{
			l_this->fLock.Lock();
			l_this->HandleDisconnectingState();
			l_this->fLock.Unlock();
		}
		if (l_this->fState == CONNECTING_STATE)
		{
			l_this->fLock.Lock();
			l_this->HandleConnectingState();
			l_this->fLock.Unlock();
		}
		if (l_this->fState == CONNECTED_STATE)
		{
			l_this->fLock.Lock();
			l_this->HandleConnectedState();
			l_this->fLock.Unlock();
		}
	}
	DebugMsg("VRemoteDebugPilot::InnerTaskProc fifo pb\n");
	xbox_assert(false);
	return 0;
}

XBOX::VError VRemoteDebugPilot::TreatRemoteDbg(IHTTPResponse* ioResponse, bool inNeedsAuthentication)
{
	RemoteDebugPilotMsg_t	msg;

	if (!fUniqueClientConnexionSemaphore.TryToLock())
	{
		DebugMsg("VRemoteDebugPilot::TreatRemoteDbg already in use\n"); 
		return VE_UNKNOWN_ERROR;
	}

	IAuthenticationInfos*	authInfos = ioResponse->GetRequest().GetAuthenticationInfos();
	VString					userName;
	authInfos->GetUserName(userName);
	msg.type = TREAT_NEW_CLIENT_MSG;
	msg.response = ioResponse;
	msg.needsAuthentication = inNeedsAuthentication;
	if (!Send(msg))
	{
		fClientCompletedSemaphore.Lock();
		DebugMsg("VRemoteDebugPilot::TreatRemoteDbg ended!!!\n");
	}
	fUniqueClientConnexionSemaphore.Unlock();
	return XBOX::VE_OK;
}


WAKDebuggerServerMessage* VRemoteDebugPilot::WaitFrom(OpaqueDebuggerContext inContext)
{
	WAKDebuggerServerMessage*	res = NULL;
	RemoteDebugPilotMsg_t		msg;
	VChromeDbgHdlPage*			page = NULL;	

	msg.type = WAIT_FROM_MSG;
	msg.ctx = inContext;
	msg.outpage = &page;

	VError	err = Send(msg);

	if (!err && page)
	{
		fPendingContextsNumberSem.Unlock();
		res = page->WaitFrom();
		fLock.Lock();
		if (res->fType == WAKDebuggerServerMessage::SRV_CONNEXION_INTERRUPTED)
		{
			std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);
			if (ctxIt != fContextArray.end())
			{
				ReleaseRefCountable(&(*ctxIt).second->fPage);
			}
		}
		ReleaseRefCountable(&page);
		fLock.Unlock();
		fPendingContextsNumberSem.Lock();
	}
	else
	{
		//int a=1;
	}
	return res;
}

XBOX::VError VRemoteDebugPilot::SendEval( OpaqueDebuggerContext inContext, const XBOX::VString& inEvaluationResult, const XBOX::VString& inRequestId )
{
	VError	err = VE_INVALID_PARAMETER;

	fLock.Lock();
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);
	
	if (testAssert(ctxIt != fContextArray.end()))
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if ( page  )
		{
			err = page->Eval(inEvaluationResult,inRequestId);
		}
	}

	fLock.Unlock();
	return err;
}


XBOX::VError VRemoteDebugPilot::SendLookup( OpaqueDebuggerContext inContext, const XBOX::VString& inLookupResult )
{
	VError	err = VE_INVALID_PARAMETER;

	fLock.Lock();
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);

	if (ctxIt != fContextArray.end())
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if ( page )
		{
			err = page->Lookup(inLookupResult);
		}
	}
	fLock.Unlock();

	return err;
}

XBOX::VError VRemoteDebugPilot::SendCallStack(
											OpaqueDebuggerContext				inContext,
											const XBOX::VString&				inCallstackStr,
											const XBOX::VString&				inExceptionInfosStr,
											const CallstackDescriptionMap&		inCallstackDesc )
{
	VError	err = VE_INVALID_PARAMETER;

	fLock.Lock();
	std::map< OpaqueDebuggerContext, VContextDescriptor* >::iterator	ctxIt = fContextArray.find(inContext);

	if (testAssert(ctxIt != fContextArray.end()))
	{
		VChromeDbgHdlPage*	page = (*ctxIt).second->fPage;
		if ( page )
		{
			err = page->Callstack(inCallstackStr,inExceptionInfosStr,inCallstackDesc);
		}
	}
	fLock.Unlock();
	return err;
}


XBOX::VError VRemoteDebugPilot::BreakpointReached(
											OpaqueDebuggerContext		inContext,
											int							inLineNumber,
											RemoteDebuggerPauseReason	inDebugReason,
											XBOX::VString&				inException,
											XBOX::VString&				inSourceUrl,
											XBOX::VectorOfVString&		inSourceData)
{

	VError	err = VE_INVALID_PARAMETER;

	XBOX::VSemaphore*	sem = NULL;
	VContextDebugInfos	debugInfos;
	
	debugInfos.fFileName = inSourceUrl;

	VURL::Decode( debugInfos.fFileName );
	VIndex				idx = debugInfos.fFileName.FindUniChar( L'/', debugInfos.fFileName.GetLength(), true );
	if (idx  > 1)
	{
		VString			tmpStr;
		debugInfos.fFileName.GetSubString(idx+1,debugInfos.fFileName.GetLength()-idx,tmpStr);
		debugInfos.fFileName = tmpStr;
	}

	debugInfos.fLineNb = inLineNumber+1;
	debugInfos.fReason = ( inDebugReason == EXCEPTION_RSN ? "exception" : (inDebugReason == BREAKPOINT_RSN ? "breakpoint" : "dbg statement" ) );
	debugInfos.fComment = "To Be Completed";
	debugInfos.fSourceLine = ( inLineNumber < inSourceData.size() ? inSourceData[inLineNumber] : " NO SOURCE AVAILABLE  " );
	err = ContextUpdated(inContext,debugInfos,&sem);

	if (err == VE_OK)
	{
		fPendingContextsNumberSem.Unlock();
		// if sem not NULL then wait for a browser page granted for debug
		if (sem)
		{
			sem->Lock();
			ReleaseRefCountable(&sem);
		}

		RemoteDebugPilotMsg_t	msg;
		msg.type = BREAKPOINT_REACHED_MSG;
		msg.ctx = inContext;
		msg.linenb = inLineNumber;
		msg.datavectstr = inSourceData;
		msg.datastr = inException;
		msg.urlstr = inSourceUrl;
		err = Send( msg );
		//testAssert(err == VE_OK);
		fPendingContextsNumberSem.Lock();
	}
	VString trace = VString("VRemoteDebugPilot::BreakpointReached ctx activation pb for ctxid=");
	trace.AppendLong8((unsigned long long)inContext);
	trace += " ,ok=";
	trace += (!err ? "1" : "0" );
	sPrivateLogHandler->Put( (err ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),trace);

	return err;
}

XBOX::VError VRemoteDebugPilot::TreatWS(IHTTPResponse* ioResponse, uLONG inPageNb )
{
	
	VError					err;
	RemoteDebugPilotMsg_t	msg;
	VChromeDbgHdlPage*		page = NULL;
	XBOX::VSemaphore*		sem = NULL;

	msg.type = TREAT_WS_MSG;
	msg.pagenb = inPageNb;
	msg.outpage = &page;
	msg.outsem = &sem;
	err = Send( msg );
	if (testAssert(err == VE_OK))
	{
		if (testAssert(sem && page))
		{
			// page and sem are already retained
			// the debugging page WebSocket is handled thanks to the (HTTP server) calling thread
			if (page->TreatWS(ioResponse,sem))
			{
				
			}
			//fLock.Lock();
			ReleaseRefCountable(&sem);
			ReleaseRefCountable(&page);
			//fLock.Unlock();
		}
	}

	return VE_OK;
}

XBOX::VError VRemoteDebugPilot::NewContext(OpaqueDebuggerContext* outContext) {
	RemoteDebugPilotMsg_t	msg;
	msg.type = NEW_CONTEXT_MSG;
	msg.outctx = outContext;
	XBOX::VError l_err = Send( msg );
	if (!l_err)
	{
		return l_err;
	}
	xbox_assert(false);//TBC
	return l_err;
}

XBOX::VError VRemoteDebugPilot::ContextUpdated(	OpaqueDebuggerContext		inContext,
												const VContextDebugInfos&	inDebugInfos,
												XBOX::VSemaphore**			outSem)
{
	RemoteDebugPilotMsg_t	msg;
	msg.type = UPDATE_CONTEXT_MSG;
	msg.ctx = inContext;
	msg.outsem = outSem;
	msg.debugInfos = inDebugInfos;
	return Send( msg );
}
XBOX::VError VRemoteDebugPilot::ContextRemoved(OpaqueDebuggerContext inContext)
{
	RemoteDebugPilotMsg_t	msg;
	msg.type = REMOVE_CONTEXT_MSG;
	msg.ctx = inContext;
	return Send( msg );
}
XBOX::VError VRemoteDebugPilot::DeactivateContext( OpaqueDebuggerContext inContext, bool inHideOnly )
{
	RemoteDebugPilotMsg_t	msg;
	msg.type = DEACTIVATE_CONTEXT_MSG;
	msg.ctx = inContext;
	msg.hideOnly = inHideOnly;
	return Send( msg );
}
XBOX::VError VRemoteDebugPilot::Start()
{
	RemoteDebugPilotMsg_t		msg;
	msg.type = START_MSG;
	return Send( msg );
}

XBOX::VError VRemoteDebugPilot::Stop()
{
	RemoteDebugPilotMsg_t	msg;
	msg.type = STOP_MSG;
	return Send( msg );
}

XBOX::VError VRemoteDebugPilot::AbortAll() {
	RemoteDebugPilotMsg_t	msg;
	msg.type = ABORT_ALL_MSG;
	return Send( msg );
}

VRemoteDebugPilot::VStatusHysteresis::VStatusHysteresis()
{
	fLastConnectedTime.FromSystemTime();
	XBOX::VDuration		tenSecs(10000);
	fLastConnectedTime.Substract(tenSecs);
}

void VRemoteDebugPilot::VStatusHysteresis::Get(bool& ioConnected)
{
	XBOX::VTime			curtTime;

	curtTime.FromSystemTime();
	fLock.Lock();
	if (ioConnected)
	{
		fLastConnectedTime = curtTime;
	}
	else
	{
		XBOX::VDuration		disconnectedDuration;
		curtTime.Substract(fLastConnectedTime,disconnectedDuration);
		// if disconnection quite recent (< approx 1 s) consider it always connected
		if (disconnectedDuration.ConvertToMilliseconds() < 1100)
		{
			ioConnected = true;
		}
	}
	fLock.Unlock();
	
}

void VRemoteDebugPilot::GetStatus(bool&	outPendingContexts,bool& outConnected,sLONG& outDebuggingEventsTimeStamp)
{

	outConnected = (fState == CONNECTED_STATE);
	
	fStatusHysteresis.Get(outConnected);

	outPendingContexts = fPendingContextsNumberSem.TryToLock();
	outDebuggingEventsTimeStamp = fDebuggingEventsTimeStamp;
	if (outPendingContexts)
	{
		fPendingContextsNumberSem.Unlock();
	}
}

XBOX::VError VRemoteDebugPilot::Send( const RemoteDebugPilotMsg_t& inMsg ) {
	XBOX::VError	err;
	
	fPipeLock.Lock();
	fPipeStatus = VE_INVALID_PARAMETER;
	fPipeMsg = inMsg;
	fPipeInSem.Unlock();
	fPipeOutSem.Lock();
	err = fPipeStatus;
	//xbox_assert( l_err == XBOX::VE_OK );
	fPipeLock.Unlock();
	return err;
}

XBOX::VError VRemoteDebugPilot::Get(RemoteDebugPilotMsg_t& outMsg, bool& outFound, uLONG inTimeoutMs) {
	bool			ok;
	XBOX::VError	err;

	outFound = false;
	err = XBOX::VE_UNKNOWN_ERROR;

	if (!inTimeoutMs)
	{
		ok = fPipeInSem.Lock();
	}
	else
	{
		ok = fPipeInSem.Lock((sLONG)inTimeoutMs);
	}
	if (ok)
	{
		outMsg = fPipeMsg;
		outFound = true;
		err = XBOX::VE_OK;
	}
	else
	{
		if (inTimeoutMs)
		{
			err = XBOX::VE_OK;
		}
	}
	return err;
}

XBOX::VError VRemoteDebugPilot::SendToBrowser( const XBOX::VString& inResp )
{
	XBOX::VStringConvertBuffer		buffer(inResp,XBOX::VTC_UTF_8);
	XBOX::VSize						size = buffer.GetSize();
	//DebugMsg("SendToBrowser -> msg='%S'\n",&inResp);
	XBOX::VError					err = fWS->WriteMessage((void*)buffer.GetCPointer(),size,true);
	return err;
}

XBOX::VError VRemoteDebugPilot::SendContextToBrowser(
												std::map< OpaqueDebuggerContext,VContextDescriptor* >::iterator& inCtxIt)
{
	XBOX::VString				resp;
	XBOX::VString				brkptsStr;

	VChromeDebugHandler::GetJSONBreakpoints(brkptsStr);

	resp = CVSTR("{\"method\":\"updateContext\",\"contextId\":\"");
	resp.AppendLong8((sLONG8)(*inCtxIt).first);
	resp += CVSTR("\",\"debugLineNb\":");
	resp.AppendLong((*inCtxIt).second->fDebugInfos.fLineNb);
	resp += CVSTR(",\"debugFileName\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fFileName;
	resp += CVSTR("\",\"debugReason\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fReason;
	resp += CVSTR("\",\"debugComment\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fComment;
	resp += CVSTR("\",\"sourceLine\":\"");
	resp += (*inCtxIt).second->fDebugInfos.fSourceLine;
	resp += CVSTR("\",\"id\":\"");
	resp += fClientId;
	resp += "\",\"breakpoints\":";
	resp += brkptsStr;
	resp += CVSTR("}");
	return SendToBrowser( resp );
}
