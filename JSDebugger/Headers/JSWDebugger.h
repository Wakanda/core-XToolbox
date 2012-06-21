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
#ifndef __JSW_DEBUGGER_IMPL__
#define __JSW_DEBUGGER_IMPL__

#include "JSDebugger/Interfaces/CJSWDebuggerFactory.h"
#include "ServerNet/VServerNet.h"
#include "VChrmDebugHandler.h"

class VJSWConnectionHandler;

USING_TOOLBOX_NAMESPACE


class VJSWDebuggerCommand :
			public VObject
#if !defined(WKA_USE_UNIFIED_DBG)
			,	public IJSWDebuggerCommand
#else
			,	public IWAKDebuggerCommand
#endif
{
	public :

#if !defined(WKA_USE_UNIFIED_DBG)
		VJSWDebuggerCommand ( IJSWDebugger::JSWD_COMMAND inCommand, const VString & inID, const VString & inContextID, const VString & inParameters );
#else
		VJSWDebuggerCommand( IWAKDebuggerCommand::WAKDebuggerServerMsgType_t inCommand, const VString & inID, const VString & inContextID, const VString & inParameters );
#endif

		virtual ~VJSWDebuggerCommand ( ) { ; }

#if !defined(WKA_USE_UNIFIED_DBG)
		virtual IJSWDebugger::JSWD_COMMAND GetType ( ) const { return fCommand; }
#else
		virtual IWAKDebuggerCommand::WAKDebuggerServerMsgType_t GetType ( ) const { return fCommand; }
		//virtual WAKDebuggerServerMessage::WAKDebuggerServerMsgType_t GetType ( ) const { return fCommand; }
#endif
		virtual const char* GetParameters ( ) const;
		virtual const char* GetID ( ) const;
		virtual void Dispose ( );
		virtual bool HasSameContextID ( uintptr_t inContextID ) const;
		//virtual const char* GetContextID ( ) const;

	private :

#if !defined(WKA_USE_UNIFIED_DBG)
		IJSWDebugger::JSWD_COMMAND								fCommand;
#else
		IWAKDebuggerCommand::WAKDebuggerServerMsgType_t			fCommand;
		//WAKDebuggerServerMessage::WAKDebuggerServerMsgType_t	fCommand;
#endif
		VString								fID;
		VString								fContextID;
		VString								fParameters;
};


class VJSWDebugger :
	public VObject,
#if !defined(WKA_USE_UNIFIED_DBG)
	public IJSWDebugger
#else
	public IWAKDebuggerServer
#endif
{
	public :

		static VJSWDebugger* Get ( );
		virtual bool HasClients ( );
		virtual short GetServerPort ( );
		virtual int StartServer ( );
		VError RemoveAllHandlers ( );
		virtual int Write ( const char * inData, long inLength, bool inToUTF8 = false );
		virtual int WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize );
		virtual int WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize );
		virtual VError AddHandler ( VJSWConnectionHandler* inHandler );
		virtual void SetSourcesRoot ( char* inRoot, int inLength );
		virtual char* GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize );
		virtual char* GetRelativeSourcePath (
										const unsigned short* inAbsoluteRoot, int inRootSize,
										const unsigned short* inAbsolutePath, int inPathSize,
										int& outSize );
		virtual long long GetMilliseconds ( );
#if !defined(WKA_USE_UNIFIED_DBG)

		virtual void SetInfo ( IJSWDebuggerInfo* inInfo );
		virtual void SetSettings ( IJSWDebuggerSettings* inSettings );
		virtual int SendContextCreated ( uintptr_t inContext );
		virtual int SendContextDestroyed ( uintptr_t inContext );
/* Most methods will have JS context ID as a parameter. */


		virtual int SendBreakPoint (
								uintptr_t inContext,
								int inExceptionHandle /* -1 ? notException : ExceptionHandle */,
								char* inURL, int inURLLength /* in bytes */,
								char* inFunction, int inFunctionLength /* in bytes */,
								int inLineNumber,
								char* inMessage = 0, int inMessageLength = 0 /* in bytes */,
								char* inName = 0, int inNameLength = 0 /* in bytes */,
								long inBeginOffset = 0, long inEndOffset = 0 /* in bytes */ );

		virtual void Reset ( );
		virtual IJSWDebuggerCommand* WaitForClientCommand ( uintptr_t inContext );
		virtual void WakeUpAllWaiters ( );
		virtual IJSWDebuggerCommand* GetNextBreakPointCommand ( );
		virtual IJSWDebuggerCommand* GetNextSuspendCommand ( uintptr_t inContext );
		virtual IJSWDebuggerCommand* GetNextAbortScriptCommand ( uintptr_t inContext );


		virtual unsigned short* EscapeForJSON (	const unsigned short* inString, int inSize, int & outSize );

#else
		virtual WAKDebuggerType_t			GetType();
		virtual void						SetInfo( IWAKDebuggerInfo* inInfo );
		virtual void						SetSettings( IWAKDebuggerSettings* inSettings );
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
		virtual bool						SendCallStack( WAKDebuggerContext_t inContext, const char *inData, int inLength );
		virtual bool						SendSource( WAKDebuggerContext_t inContext, intptr_t inSrcId, const char *inData, int inLength, const char* inUrl, unsigned int inUrlLen );
		virtual WAKDebuggerServerMessage*	WaitFrom(WAKDebuggerContext_t inContext);
		virtual void						DisposeMessage(WAKDebuggerServerMessage* inMessage);
		virtual WAKDebuggerServerMessage* GetNextBreakPointCommand();
		virtual WAKDebuggerServerMessage* GetNextSuspendCommand( WAKDebuggerContext_t inContext );
		virtual WAKDebuggerServerMessage* GetNextAbortScriptCommand ( WAKDebuggerContext_t inContext );

		virtual WAKDebuggerUCharPtr_t		EscapeForJSON( const unsigned char* inString, int inSize, int& outSize );
		virtual void						DisposeUCharPtr( WAKDebuggerUCharPtr_t inUCharPtr );

		virtual void*						UStringToVString( const void* inString, int inSize );

		virtual void						Trace(const void* inString, int inSize );

#endif
	private :

		static VServer*						sServer;
		static VJSWDebugger*				sDebugger;
		VCriticalSection					fHandlersLock;
		std::vector<VJSWConnectionHandler*>	fHandlers;
#if !defined(WKA_USE_UNIFIED_DBG)
		IJSWDebuggerSettings*				fSettings;
#else
		IWAKDebuggerSettings*				fSettings;
		static XBOX::VCriticalSection		sDbgLock;
#endif
		VJSWDebugger ( );

		VJSWConnectionHandler* _RetainFirstHandler ( ); // To be used as a shortcut while debugger is single-connection.
};

/*
Should inherit from a pure virtual class, exposed to JavaScript Core debugger when a new context event is sent.
This would avoid a mutex lock each time a context needs to know if there is a new pending runtime event for it.

class VJSWContextRunTimeInfo : public VObject
{
	public :

		VJSWContextRunTimeInfo ( uintptr_t inContext );

		uintptr_t GetID ( ) { return fContext; }

		bool HasNewRunTimeInterrupt ( ); // Has either a "pause" or an "abort" commands pending.
		bool HasNewBreakPoint ( );

		void AddRunTimeInterrupt ( );
		void AddBreakPoint ( );

	private :

		uintptr_t							fContext;
		sLONG								fInterruptCount;
		sLONG								fBreakPointCount;
};
*/


#endif //__JSW_DEBUGGER__
