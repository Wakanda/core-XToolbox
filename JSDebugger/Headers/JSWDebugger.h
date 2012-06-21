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


class VJSWDebuggerCommand : public VObject, public IJSWDebuggerCommand
{
	public :

		VJSWDebuggerCommand ( IJSWDebugger::JSWD_COMMAND inCommand, const VString & inID, const VString & inContextID, const VString & inParameters );
		virtual ~VJSWDebuggerCommand ( ) { ; }

		virtual IJSWDebugger::JSWD_COMMAND GetType ( ) const { return fCommand; }
		virtual const char* GetParameters ( ) const;
		virtual const char* GetID ( ) const;
		virtual bool HasSameContextID ( uintptr_t inContextID ) const;
		//virtual const char* GetContextID ( ) const;

		virtual void Dispose ( );

	private :

		IJSWDebugger::JSWD_COMMAND			fCommand;
		VString								fID;
		VString								fContextID;
		VString								fParameters;
};


class VJSWDebugger : public VObject, public IJSWDebugger
{
	public :

		static VJSWDebugger* Get ( );

		virtual int StartServer ( );
		virtual short GetServerPort ( );
		virtual bool HasClients ( );
		virtual void SetInfo ( IJSWDebuggerInfo* inInfo );
		virtual void SetSettings ( IJSWDebuggerSettings* inSettings );

/* Most methods will have JS context ID as a parameter. */

		virtual int Write ( const char * inData, long inLength, bool inToUTF8 = false );
		virtual int WriteFileContent ( long inCommandID, uintptr_t inContext, const unsigned short* inFilePath, int inPathSize );
		virtual int WriteSource ( long inCommandID, uintptr_t inContext, const unsigned short* inSource, int inSize );

		void SetSourcesRoot ( char* inRoot, int inLength );
		virtual int SendBreakPoint (
								uintptr_t inContext,
								int inExceptionHandle /* -1 ? notException : ExceptionHandle */,
								char* inURL, int inURLLength /* in bytes */,
								char* inFunction, int inFunctionLength /* in bytes */,
								int inLine,
								char* inMessage = 0, int inMessageLength = 0 /* in bytes */,
								char* inName = 0, int inNameLength = 0 /* in bytes */,
								long inBeginOffset = 0, long inEndOffset = 0 /* in bytes */ );
		virtual int SendContextCreated ( uintptr_t inContext );
		virtual int SendContextDestroyed ( uintptr_t inContext );

		virtual void Reset ( );
		virtual IJSWDebuggerCommand* WaitForClientCommand ( uintptr_t inContext );
		virtual void WakeUpAllWaiters ( );
		virtual IJSWDebuggerCommand* GetNextBreakPointCommand ( );
		virtual IJSWDebuggerCommand* GetNextSuspendCommand ( uintptr_t inContext );
		virtual IJSWDebuggerCommand* GetNextAbortScriptCommand ( uintptr_t inContext );

		virtual char* GetRelativeSourcePath (
										const unsigned short* inAbsoluteRoot, int inRootSize,
										const unsigned short* inAbsolutePath, int inPathSize,
										int& outSize );
		virtual char* GetAbsolutePath (
									const unsigned short* inAbsoluteRoot, int inRootSize,
									const unsigned short* inRelativePath, int inPathSize,
									int& outSize );
		virtual unsigned short* EscapeForJSON (	const unsigned short* inString, int inSize, int & outSize );
		virtual long long GetMilliseconds ( );

		VError AddHandler ( VJSWConnectionHandler* inHandler );
		VError RemoveAllHandlers ( );

	private :

		static VServer*						sServer;
		static VJSWDebugger*				sDebugger;

		VCriticalSection					fHandlersLock;
		std::vector<VJSWConnectionHandler*>	fHandlers;
		IJSWDebuggerSettings*				fSettings;

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
