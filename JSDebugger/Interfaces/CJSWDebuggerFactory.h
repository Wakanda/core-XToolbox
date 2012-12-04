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

#if 0//!WKA_USE_UNIFIED_DBG
#include "IJSWDebugger.h"
#else
#include "4DDebuggerServer.h"
#endif


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
#if 0
		IJSWDebugger* Get ( );
#else
		IWAKDebuggerServer*	Get();
		IWAKDebuggerServer*	GetChromeDebugHandler();
		IWAKDebuggerServer*	GetChromeDebugHandler(
			const XBOX::VString inIP,
			uLONG				inPort,
			const XBOX::VString inDebuggerHTMLSkeleton,
			const XBOX::VString inTracesHTMLSkeleton);
#endif
	private:
};


class IAuthenticationInfos;
#if 0 //!WKA_USE_UNIFIED_DBG
class JSDEBUGGER_API IJSWDebuggerSettings
#else
class JSDEBUGGER_API IWAKDebuggerSettings
#endif
{
	public:

		virtual bool NeedsAuthentication ( ) const = 0;
		/* Returns 'true' if a given user can debug with this JS debugger. Returns 'false' otherwise. */
		virtual bool UserCanDebug ( const UniChar* inUserName, const UniChar* inUserPassword ) const = 0;
		virtual bool UserCanDebug(IAuthenticationInfos* inAuthenticationInfos) const = 0;
		virtual bool HasDebuggerUsers ( ) const = 0;

	protected:
#if 0//!defined(WKA_USE_UNIFIED_DBG)	
		IJSWDebuggerSettings ( ) { ; }
		virtual ~IJSWDebuggerSettings ( ) { ; }
#else
		IWAKDebuggerSettings ( ) { ; }
		virtual ~IWAKDebuggerSettings ( ) { ; }
#endif
};


#endif
