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


//#include "JSWServer.h"


VServer*								VJSWDebugger::sServer = 0;
VJSWDebugger*							VJSWDebugger::sDebugger = 0;

// This is an example of an exported variable
JSDEBUGGER_API int nJSDebugger=0;

// This is an example of an exported function.
JSDEBUGGER_API int fnJSDebugger(void)
{
	return 42;
}


/*VJSWDebuggerCommand::VJSWDebuggerCommand ( IJSWDebugger::JSWD_COMMAND inCommand ) :
														fParameters ( )
{
	fCommand = inCommand;
}*/

VJSWDebuggerCommand::VJSWDebuggerCommand ( IJSWDebugger::JSWD_COMMAND inCommand, const VString & inID, const VString & inContextID, const VString & inParameters )
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

bool VJSWDebuggerCommand::HasSameContextID ( uintptr_t inContextID ) const
{
	if ( fContextID. EqualToString ( CVSTR ( "UNDEFINED" ) ) )
		return true;

	VString			vstrTestID;
	vstrTestID. FromLong8 ( inContextID );

	return fContextID. EqualToString ( vstrTestID );
}

void VJSWDebuggerCommand::Dispose ( )
{
	delete this;
}

IJSWDebugger* JSWDebuggerFactory::Get ( )
{
	return VJSWDebugger::Get ( );
}
IJSWChrmDebugger* JSWDebuggerFactory::GetCD()
{
	return VChrmDebugHandler::Get();
}

VJSWDebugger::VJSWDebugger ( )
{
	fSettings = 0;
}

VJSWDebugger* VJSWDebugger::Get ( )
{
	if ( sDebugger == 0 )
		sDebugger = new VJSWDebugger ( );

	return sDebugger;
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
#elif DEPRECATED_IPV4_API_SHOULD_NOT_COMPILE
	#error NEED AN IP V6 UPDATE
#endif

	jswchFactory-> SetIsSSL ( false );
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

void VJSWDebugger::SetInfo ( IJSWDebuggerInfo* inInfo )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> SetInfo ( inInfo );
	cHandler-> Release ( );
}

void VJSWDebugger::SetSettings ( IJSWDebuggerSettings* inSettings )
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

void VJSWDebugger::SetSourcesRoot ( char* inRoot, int inLength )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> SetSourcesRoot ( inRoot, inLength );
	cHandler-> Release ( );
}

int VJSWDebugger::SendBreakPoint (
						uintptr_t inContext,
						int inExceptionHandle /* -1 ? notException : ExceptionHandle */,
						char* inURL, int inURLLength,
						char* inFunction, int inFunctionLength,
						int inLine,
						char* inMessage, int inMessageLength /* in bytes */,
						char* inName, int inNameLength /* in bytes */,
						long inBeginOffset, long inEndOffset /* in bytes */ )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	int		nResult = cHandler-> SendBreakPoint (
											inContext,
											inExceptionHandle,
											inURL, inURLLength,
											inFunction, inFunctionLength,
											inLine,
											inMessage, inMessageLength,
											inName, inNameLength,
											inBeginOffset, inEndOffset );
	cHandler-> Release ( );

	return nResult;
}

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

void VJSWDebugger::Reset ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> Reset ( );
	cHandler-> Release ( );
}

IJSWDebuggerCommand* VJSWDebugger::WaitForClientCommand ( uintptr_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	IJSWDebuggerCommand*		dcResult = cHandler-> WaitForClientCommand ( inContext );
	cHandler-> Release ( );

	return dcResult;
}

void VJSWDebugger::WakeUpAllWaiters ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return;

	cHandler-> WakeUpAllWaiters ( );
	cHandler-> Release ( );
}

IJSWDebuggerCommand* VJSWDebugger::GetNextBreakPointCommand ( )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	IJSWDebuggerCommand*		dcResult = cHandler-> GetNextBreakPointCommand ( );
	cHandler-> Release ( );

	return dcResult;
}

IJSWDebuggerCommand* VJSWDebugger::GetNextSuspendCommand ( uintptr_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	IJSWDebuggerCommand*		dcResult = cHandler-> GetNextSuspendCommand ( inContext );
	cHandler-> Release ( );

	return dcResult;
}

IJSWDebuggerCommand* VJSWDebugger::GetNextAbortScriptCommand ( uintptr_t inContext )
{
	VJSWConnectionHandler*		cHandler = _RetainFirstHandler ( );
	if ( cHandler == 0 )
		return 0;

	IJSWDebuggerCommand*		dcResult = cHandler-> GetNextAbortScriptCommand ( inContext );
	cHandler-> Release ( );

	return dcResult;
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

long long VJSWDebugger::GetMilliseconds ( )
{
	return VSystem::GetCurrentTime ( );
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
		}

	fHandlersLock. Unlock ( );

	return cHandler;
}

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
