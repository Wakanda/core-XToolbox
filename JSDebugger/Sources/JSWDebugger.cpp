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
// JSDebugger.cpp : Defines the exported functions for the DLL application.
//

#include "Kernel/VKernel.h"
#include "ServerNet/VServerNet.h"

#include "CJSWDebuggerFactory.h"
#include "JSWDebugger.h"
#include "JSWServer.h"

#define WKA_USE_UNIFIED_DBG

VServer*								VJSWDebugger::sServer = 0;
VJSWDebugger*							VJSWDebugger::sDebugger = 0;


#if !defined(WKA_USE_UNIFIED_DBG)
#else
XBOX::VCriticalSection					VJSWDebugger::sDbgLock;
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
XBOX::XTCPSock*							VJSWDebugger::sSck=NULL;
#endif

#endif

#if 0
// This is an example of an exported variable
JSDEBUGGER_API int nJSDebugger=0;

// This is an example of an exported function.
JSDEBUGGER_API int fnJSDebugger(void)
{
	return 42;
}
#endif

#if !defined(WKA_USE_UNIFIED_DBG)
VJSWDebuggerCommand::VJSWDebuggerCommand ( IJSWDebugger::JSWD_COMMAND inCommand, const VString & inID, const VString & inContextID, const VString & inParameters )
#else
VJSWDebuggerCommand::VJSWDebuggerCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID, const VString & inParameters )
#endif
{
	fCommand = inCommand;

	fID. FromString ( inID );
	if ( fID. GetLength ( ) == 0 )
		fID. AppendCString ( "NOID" );

	fContextID. FromString ( inContextID );
	if ( fContextID. GetLength ( ) == 0 )
		fContextID. AppendCString ( "UNKNOWN" );

	fParameters. FromString ( inParameters );
}

const char* VJSWDebuggerCommand::GetParameters ( ) const
{
	VIndex		nBufferLength = fParameters. GetLength ( ) + 1;
	char*		szchResult = new char [ nBufferLength ];
	fParameters. ToCString ( szchResult, nBufferLength );

	return szchResult;
}

const char* VJSWDebuggerCommand::GetID ( ) const
{
	VIndex		nBufferLength = fID. GetLength ( ) + 1;
	char*		szchResult = new char [ nBufferLength ];
	fID. ToCString ( szchResult, nBufferLength );

	return szchResult;
}

void VJSWDebuggerCommand::Dispose ( )
{
	delete this;
}


bool VJSWDebuggerCommand::HasSameContextID ( uintptr_t inContextID ) const
{
	if ( fContextID. EqualToString ( CVSTR ( "UNDEFINED" ) ) )
		return true;

	VString			vstrTestID;
	vstrTestID. FromLong8 ( inContextID );

	return fContextID. EqualToString ( vstrTestID );
}


#if !defined(WKA_USE_UNIFIED_DBG)
void VJSWDebugger::SetSettings ( IJSWDebuggerSettings* inSettings )
#else
void VJSWDebugger::SetSettings( IWAKDebuggerSettings* inSettings )
#endif
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
	{
		fSettings = inSettings;

		return;
	}

	if ( inSettings == 0 )
		fSettings = inSettings;

	cHandler-> SetSettings ( inSettings );
	cHandler-> Release ( );
}

#if !defined(WKA_USE_UNIFIED_DBG)
void VJSWDebugger::SetInfo ( IJSWDebuggerInfo* inInfo )
#else
void VJSWDebugger::SetInfo( IWAKDebuggerInfo* inInfo )
#endif
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> SetInfo ( inInfo );
	cHandler-> Release ( );
}

VJSWDebugger* VJSWDebugger::Get ( )
{
	if ( sDebugger == 0 )
	{
		sDebugger = new VJSWDebugger ( );
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
		if (!sSck)
		{
			sSck = XTCPSock::NewClientConnectedSock(K_NETWORK_DEBUG_HOST,K_NETWORK_DEBUG_BASE,100);
		}
#endif
	}
	return sDebugger;
}

int VJSWDebugger::StopServer ( )
{

	if ( sServer == 0 )
	{
		xbox_assert(false);
		return 0;
	}
	//sServer-> Stop ( );

	//jswchFactory-> RemoveAllPorts ( );
	return 0;
}

int VJSWDebugger::StartServer ( )
{
	/* TODO: Need multi-threading protection */

	if ( sServer != 0 )
		return 0;

	VError									vError = VE_OK;

	sServer = new VServer ( );
	VTCPConnectionListener*					vtcpcListener = new VTCPConnectionListener ( );
	VWorkerPool*							vwPool = new VWorkerPool ( 2, 5, 60, 2, 3 );

	PortNumber								nPort = 1919;
	VJSWConnectionHandlerFactory*			jswchFactory = new VJSWConnectionHandlerFactory ( );

#if WITH_DEPRECATED_IPV4_API
	jswchFactory-> SetIP ( 0 /* ALL */ );
#else
	VString									vstrIPAll = VNetAddress::GetAnyIP ( );
	jswchFactory-> SetIP ( vstrIPAll );
#endif

	VFolder*								vResourcesFolder =  VProcess::Get ( )-> RetainFolder ( VProcess::eFS_Resources );
	xbox_assert ( vResourcesFolder != 0 );

	VFilePath								vfPathKey;
	VFilePath								vfPathCertificate;
	
	if ( vResourcesFolder != 0 )
	{
		VFilePath							vfPathResources;
		vResourcesFolder-> GetPath ( vfPathResources );
		VFilePath&							vfPathSSL = vfPathResources. ToSubFolder ( CVSTR ( "Default Solution" ) ). ToSubFolder ( CVSTR ( "Admin" ) );
		vfPathCertificate=vfPathSSL. ToSubFile ( CVSTR ( "cert.pem" ) );
		vfPathSSL. ToFolder ( );
		vfPathKey=vfPathSSL. ToSubFile ( CVSTR ( "key.pem" ) );
		vResourcesFolder-> Release ( );
	}

	jswchFactory-> SetIsSSL ( true );
	vtcpcListener-> SetSSLCertificatePaths ( vfPathCertificate, vfPathKey );
	jswchFactory-> SetDebugger ( this );

	/*
		Look for the next available port.
		The code could be generalized and put directly into VServer and VListener themselves.
		In that case a special care and logic is needed to deal with listeners on multiple 
		ports and cases when listeners from different "projects" register within the same server
		to be able to tell which port gets which new value.
	*/
	while ( nPort <= 65535 )
	{
		vtcpcListener-> AddWorkerPool ( vwPool );
		jswchFactory-> AddNewPort ( nPort );
		vtcpcListener-> AddConnectionHandlerFactory ( jswchFactory );
		sServer-> AddConnectionListener ( vtcpcListener );

		vError = sServer-> Start ( );
		if ( vError == VE_OK )
			break;

		sServer-> Stop ( );

		jswchFactory-> RemoveAllPorts ( );
		nPort++;
	}

	vwPool-> Release ( );
	jswchFactory-> Release ( );

	/* TODO: Error handling */

	return 0;
}

VJSWDebugger::VJSWDebugger ( )
{
	fSettings = 0;
}

short VJSWDebugger::GetServerPort ( )
{
	if ( sServer == 0 )
		return -1;

	std::vector<PortNumber>						vctrPorts;
	sServer-> GetPublishedPorts ( vctrPorts );

	xbox_assert ( vctrPorts. size ( ) == 1 );

	return vctrPorts [ 0 ];
}

bool VJSWDebugger::HasClients ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return false;

	bool		bResult = cHandler-> IsHandling ( );
	cHandler-> Release ( );

	return bResult;
}

int VJSWDebugger::Write ( const char * inData, long inLength, bool inToUTF8 )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> Write ( inData, inLength, inToUTF8 );
	cHandler-> Release ( );

	return nResult;
}

int VJSWDebugger::WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> WriteFileContent ( inCommandID, inContext, inFilePath, inPathSize );
	cHandler-> Release ( );

	return nResult;
}

int VJSWDebugger::WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> WriteSource ( inCommandID, inContext, inSource, inSize );
	cHandler-> Release ( );

	return nResult;
}

#if !defined(WKA_USE_UNIFIED_DBG)
int VJSWDebugger::SendBreakPoint (
						uintptr_t inContext,
						int inExceptionHandle /* -1 ? notException : ExceptionHandle */,
						char* inURL, int inURLLength,
						char* inFunction, int inFunctionLength,
						int inLineNumber,
						char* inMessage, int inMessageLength /* in bytes */,
						char* inName, int inNameLength /* in bytes */,
						long inBeginOffset, long inEndOffset /* in bytes */ )
#else
bool VJSWDebugger::BreakpointReached(	WAKDebuggerContext_t	inContext,
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
#endif
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> SendBreakPoint (
											(uintptr_t)inContext,
											inExceptionHandle,
											inURL, inURLLength,
											inFunction, inFunctionLength,
											inLineNumber,
											inMessage, inMessageLength,
											inName, inNameLength,
											inBeginOffset, inEndOffset );
	cHandler-> Release ( );

	return nResult;
}

void VJSWDebugger::SetSourcesRoot ( char* inRoot, int inLength )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> SetSourcesRoot ( inRoot, inLength );
	cHandler-> Release ( );
}


void VJSWDebugger::Reset ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> Reset ( );
	cHandler-> Release ( );
}


void VJSWDebugger::WakeUpAllWaiters ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> WakeUpAllWaiters ( );
	cHandler-> Release ( );
}


long long VJSWDebugger::GetMilliseconds ( )
{
	return VSystem::GetCurrentTime ( );
}


char* VJSWDebugger::GetRelativeSourcePath (
										const unsigned short* inAbsoluteRoot, int inRootSize,
										const unsigned short* inAbsolutePath, int inPathSize,
										int& outSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	char*		szchResult = cHandler-> GetRelativeSourcePath ( inAbsoluteRoot, inRootSize, inAbsolutePath, inPathSize, outSize );
	cHandler-> Release ( );

	return szchResult;
}


char* VJSWDebugger::GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	char*		szchResult = cHandler-> GetAbsolutePath ( inAbsoluteRoot, inRootSize, inRelativePath, inPathSize, outSize );
	cHandler-> Release ( );

	return szchResult;
}

VJSWConnectionHandler* VJSWDebugger::_RetainFirstHandler ( )
{
	VJSWConnectionHandler*		cHandler = 0;

	fHandlersLock. Lock ( );

		std::vector<VJSWConnectionHandler*>::iterator	iter = fHandlers. begin ( );
		while ( iter != fHandlers. end ( ) )
		{
			if ( ( *iter )-> IsHandling ( ) )
			{
				cHandler = *iter;
				cHandler-> Retain ( );

				break;
			}
			else if ( ( *iter )-> IsDone ( ) )
			{
				( *iter )-> Release ( );
				iter = fHandlers. erase ( iter );
			}
			else
			{
				++iter;
			}
		}

	fHandlersLock. Unlock ( );

	return cHandler;
}

VError VJSWDebugger::RemoveAllHandlers ( )
{
	fHandlersLock. Lock ( );

		std::vector<VJSWConnectionHandler*>::iterator	iter = fHandlers. begin ( );
		while ( iter != fHandlers. end ( ) )
		{
			( *iter )-> Release ( );

			iter++;
		}
		fHandlers. clear ( );

	fHandlersLock. Unlock ( );

	return VE_OK;
}

/*VJSWDebuggerCommand::VJSWDebuggerCommand ( IJSWDebugger::JSWD_COMMAND inCommand ) :
														fParameters ( )
{
	fCommand = inCommand;
}*/

#if !defined(WKA_USE_UNIFIED_DBG)
IJSWDebuggerCommand* VJSWDebugger::WaitForClientCommand ( uintptr_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	IJSWDebuggerCommand*		dcResult = cHandler-> WaitForClientCommand ( inContext );
	cHandler-> Release ( );

	return dcResult;
}
#else
WAKDebuggerServerMessage* VJSWDebugger::WaitFrom(WAKDebuggerContext_t inContext)
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler == 0 )
		return 0;

	WAKDebuggerServerMessage*	dcResult = cHandler->WaitForClientCommand( (uintptr_t)inContext );
	cHandler->Release();

	return dcResult;
}
void VJSWDebugger::DisposeMessage(WAKDebuggerServerMessage* inMessage)
{
	xbox_assert(false);// appeler le free correspondant a cHandler->WaitForClientCommand 
}

bool VJSWDebugger::SetState(WAKDebuggerContext_t inContext, WAKDebuggerState_t state)
{
	xbox_assert(false);//TBC
	return true;
}
bool VJSWDebugger::SendLookup( WAKDebuggerContext_t inContext, void* inVars, unsigned int inSize )
{
	xbox_assert(false);//TBC
	return true;
}
bool VJSWDebugger::SendEval( WAKDebuggerContext_t inContext, void* inVars )
{
	xbox_assert(false);//TBC
	return true;
}

bool VJSWDebugger::SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength )
{
	xbox_assert(false);//TBC
	return true;
}
bool VJSWDebugger::SendSource( WAKDebuggerContext_t inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen )
{
	xbox_assert(false);//TBC
	return true;
}

void VJSWDebugger::Trace(WAKDebuggerContext_t inContext, const void* inString, int inSize )
{
	VString		l_trace(inString,(VSize)inSize,VTC_UTF_16);
	VString l_tmp = CVSTR("DEBUG >>>> ");
	l_tmp += l_trace;
	l_tmp += CVSTR(" '\n");
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
	if (!sSck)
	{
#endif
		DebugMsg(l_tmp);
#if defined(UNIFIED_DEBUGGER_NET_DEBUG)
	}
	else
	{
		StErrorContextInstaller errCtx(false, true);

		char	l_data[1024];
		l_tmp.ToCString(l_data,1024);
		uLONG	l_len=strlen(l_data);
		if (sSck->Write(l_data, &l_len, false))
		{
			sSck->Close();
			delete sSck;
			sSck = NULL;
		}
	}
#endif
}
#endif

#if !defined(WKA_USE_UNIFIED_DBG)
IJSWDebuggerCommand* VJSWDebugger::GetNextBreakPointCommand ( )
#else
WAKDebuggerServerMessage* VJSWDebugger::GetNextBreakPointCommand()
#endif
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

#if !defined(WKA_USE_UNIFIED_DBG)
	IJSWDebuggerCommand*		dcResult = cHandler-> GetNextBreakPointCommand ( );
#else
	WAKDebuggerServerMessage*	dcResult = cHandler-> GetNextBreakPointCommand ( );
#endif
	cHandler-> Release ( );

	return dcResult;
}

#if !defined(WKA_USE_UNIFIED_DBG)
IJSWDebuggerCommand* VJSWDebugger::GetNextSuspendCommand ( uintptr_t inContext )
#else
WAKDebuggerServerMessage* VJSWDebugger::GetNextSuspendCommand( WAKDebuggerContext_t inContext )
#endif
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

#if !defined(WKA_USE_UNIFIED_DBG)
	IJSWDebuggerCommand*		dcResult = cHandler-> GetNextSuspendCommand ( inContext );
#else
	WAKDebuggerServerMessage*	dcResult = cHandler-> GetNextSuspendCommand ( (uintptr_t)inContext );
#endif
	cHandler-> Release ( );

	return dcResult;
}

#if !defined(WKA_USE_UNIFIED_DBG)
IJSWDebuggerCommand* VJSWDebugger::GetNextAbortScriptCommand ( uintptr_t inContext )
#else
WAKDebuggerServerMessage* VJSWDebugger::GetNextAbortScriptCommand ( WAKDebuggerContext_t inContext )
#endif
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

#if !defined(WKA_USE_UNIFIED_DBG)
	IJSWDebuggerCommand*		dcResult = cHandler-> GetNextAbortScriptCommand ( inContext );
#else
	WAKDebuggerServerMessage*	dcResult = cHandler-> GetNextAbortScriptCommand ( (uintptr_t)inContext );
#endif
	cHandler-> Release ( );

	return dcResult;
}

#if !defined(WKA_USE_UNIFIED_DBG)
unsigned short* VJSWDebugger::EscapeForJSON ( const unsigned short* inString, int inSize, int & outSize )
{
	VString				vstrInput;
	vstrInput. AppendBlock ( inString, inSize * sizeof ( unsigned short ), VTC_UTF_16 );

	VString				vstrOutput;
	vstrInput. GetJSONString ( vstrOutput );

	outSize = vstrOutput. GetLength ( );
	unsigned short*		szusResult = new unsigned short [ outSize + 1 ];
	vstrOutput. ToBlock ( szusResult, ( outSize + 1 ) * sizeof ( unsigned short ), VTC_UTF_16, true, false );

	return szusResult;
}
#else

WAKDebuggerUCharPtr_t VJSWDebugger::EscapeForJSON( const unsigned char* inString, int inSize, int& outSize )
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

void VJSWDebugger::DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr )
{
	if (inUCharPtr)
	{
		delete [] inUCharPtr;
	}
}

void* VJSWDebugger::UStringToVString( const void* inString, int inSize )
{
	xbox_assert(false);//should not be used in this dbg srv
	return NULL;
}

#endif


VError VJSWDebugger::AddHandler ( VJSWConnectionHandler* inHandler )
{
	xbox_assert ( inHandler != 0 );
	if ( inHandler == 0 )
		return VE_INVALID_PARAMETER;

	fHandlersLock. Lock ( );
		inHandler-> Retain ( );
		fHandlers. push_back ( inHandler );
		if ( fSettings != 0 )
			inHandler-> SetSettings ( fSettings );
	fHandlersLock. Unlock ( );

	return VE_OK;
}

#if !defined(WKA_USE_UNIFIED_DBG)

int VJSWDebugger::SendContextCreated ( uintptr_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> SendContextCreated ( inContext );
	cHandler-> Release ( );

	return nResult;
}

int VJSWDebugger::SendContextDestroyed ( uintptr_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> SendContextDestroyed ( inContext );
	cHandler-> Release ( );

	return nResult;
}

#else
WAKDebuggerContext_t VJSWDebugger::AddContext( uintptr_t inContext  )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler == 0 )
		return NULL;

	int		nResult = cHandler->SendContextCreated( inContext );
	cHandler->Release();

	if (!nResult)
	{
		return (WAKDebuggerContext_t)inContext;
	}
	return NULL;
}

bool VJSWDebugger::RemoveContext( WAKDebuggerContext_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler();
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler->SendContextDestroyed( (uintptr_t)inContext );
	cHandler->Release();

	return (nResult == 0);
}

WAKDebuggerType_t VJSWDebugger::GetType()
{
	return REGULAR_DBG_TYPE;
}
bool VJSWDebugger::Lock()
{
	bool	l_res;

	l_res = sDbgLock.Lock();

	xbox_assert(l_res);
	return l_res;
}
bool VJSWDebugger::Unlock()
{
	bool	l_res;

	l_res = sDbgLock.Unlock();

	xbox_assert(l_res);
	return l_res;
}

#endif


#if 0
IJSWDebugger* JSWDebuggerFactory::Get ( )
{
	return VJSWDebugger::Get ( );
}

#else
IWAKDebuggerServer* JSWDebuggerFactory::Get()
{
	return VJSWDebugger::Get();
}

IWAKDebuggerServer* JSWDebuggerFactory::GetChromeDebugHandler()
{
	return VChromeDebugHandler::Get();
}

IWAKDebuggerServer* JSWDebuggerFactory::GetChromeDebugHandler(
		const XBOX::VString inIP,
		uLONG				inPort,
		const XBOX::VString inDebuggerHTMLSkeleton,
		const XBOX::VString inTracesHTMLSkeleton)
{
#if defined(WKA_USE_UNIFIED_DBG)
	return VChromeDebugHandler::Get(inIP,inPort,inDebuggerHTMLSkeleton,inTracesHTMLSkeleton);
#else
	return NULL;
#endif
}

#endif


/*VJSWContextRunTimeInfo::VJSWContextRunTimeInfo ( uintptr_t inContext ) :
											fContext ( inContext )
											, fInterruptCount ( 0 )
											, fBreakPointCount ( 0 )
{
	;
}

bool VJSWContextRunTimeInfo::HasNewRunTimeInterrupt ( )
{
	sLONG		nCount = VInterlocked::AtomicGet ( &fInterruptCount );

	return nCount > 0;
}

bool VJSWContextRunTimeInfo::HasNewBreakPoint ( )
{
	sLONG		nCount = VInterlocked::AtomicGet ( &fBreakPointCount );

	return nCount > 0;
}

void VJSWContextRunTimeInfo::AddRunTimeInterrupt ( )
{
	VInterlocked::Increment ( &fBreakPointCount );
}

void VJSWContextRunTimeInfo::AddBreakPoint ( )
{
	VInterlocked::Decrement ( &fBreakPointCount );
}

*/
