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
#ifndef __JSW_DEBUGGER_FACTORY__
#define __JSW_DEBUGGER_FACTORY__

#include "4DDebuggerServer.h"

#ifdef JSDEBUGGER_EXPORTS
    #define JSDEBUGGER_API __declspec(dllexport)
#else
	#ifdef __GNUC__
		#define JSDEBUGGER_API __attribute__((visibility("default")))
	#else
		#define JSDEBUGGER_API __declspec(dllimport)
	#endif
#endif


class JSDEBUGGER_API JSWDebuggerFactory
{
	public:
		JSWDebuggerFactory ( ) { ; }

		IWAKDebuggerServer*	Get();
		IChromeDebuggerServer*	GetChromeDebugHandler(const XBOX::VString& inSolutionName);
		IChromeDebuggerServer*	GetChromeDebugHandler(
			const XBOX::VString&	inSolutionName,
			const XBOX::VString&	inTracesHTMLSkeleton);

	private:
};


class IAuthenticationInfos;

class JSDEBUGGER_API IWAKDebuggerSettings
{
	public:

		virtual bool	NeedsAuthentication ( ) const = 0;
		/* Returns 'true' if a given user can debug with this JS debugger. Returns 'false' otherwise. */
		virtual bool	UserCanDebug ( const UniChar* inUserName, const UniChar* inUserPassword ) const = 0;
		virtual bool	UserCanDebug(IAuthenticationInfos* inAuthenticationInfos) const = 0;
		virtual bool	HasDebuggerUsers ( ) const = 0;

		virtual	bool	AddBreakPoint( OpaqueDebuggerContext inContext, const XBOX::VString& inUrl, intptr_t inSrcId, int inLineNumber ) = 0;
		virtual	bool	AddBreakPoint( const XBOX::VString& inUrl, int inLineNumber ) = 0;
		virtual	bool	RemoveBreakPoint( const XBOX::VString& inUrl, int inLineNumber ) = 0;
		virtual	bool	RemoveBreakPoint( OpaqueDebuggerContext inContext, const XBOX::VString& inUrl, intptr_t inSrcId, int inLineNumber ) = 0;
		virtual void	Set(OpaqueDebuggerContext	inContext, const XBOX::VString& inUrl, intptr_t inSrcId, const XBOX::VString& inData) = 0;
		virtual void	Add(OpaqueDebuggerContext inContext) = 0;
		virtual void	Remove(OpaqueDebuggerContext	inContext) = 0;
		virtual bool	HasBreakpoint(OpaqueDebuggerContext	inContext,intptr_t inSrcId, unsigned lineNumber) = 0;
		virtual bool	GetData(OpaqueDebuggerContext inContext, intptr_t inSrcId, XBOX::VString& outSourceUrl, XBOX::VectorOfVString& outSourceData) = 0;
		virtual void	GetJSONBreakpoints(XBOX::VString& outJSONBreakPoints) = 0;

	protected:

		IWAKDebuggerSettings ( ) { ; }
		virtual ~IWAKDebuggerSettings ( ) { ; }

};


#endif
