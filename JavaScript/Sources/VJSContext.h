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
#ifndef __VJSContext__
#define __VJSContext__

#include "JS4D.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSGlobalObject;
class VJSGlobalContext;
class IJSRuntimeDelegate;

//======================================================

/*
	not thread safe.
	
*/
class XTOOLBOX_API VJSContextGroup : public XBOX::VObject, public XBOX::IRefCountable
{
public:
											VJSContextGroup();
											~VJSContextGroup();
	
private:
											VJSContextGroup( const VJSContextGroup&);	// forbidden
											VJSContextGroup& operator=( const VJSContextGroup&);	// forbidden

};

//======================================================


/*
	not thread safe.

	It's a smarter pointer & wrapper class.
	
	The context is not retained to avoid cyclic redundancy (children don't retain their parent)
*/
class XTOOLBOX_API VJSContext : public XBOX::VObject
{
friend class VJSParms_withContext;
public:
											VJSContext( JS4D::ContextRef inContext):fContext( inContext),fGlobalContext(NULL),fDebuggerAllowed(true)		{}
											VJSContext( const VJSContext& inOther):fContext( inOther.fContext),fGlobalContext(NULL),fDebuggerAllowed(true)	{}
											VJSContext( const VJSGlobalContext *inGlobalContext);
											~VJSContext();

			// evaluates some script an optionnally returns the result (outResult may be NULL) and the exception (outException may be NULL).
			// return true if no exception occured.
			bool							EvaluateScript( VFile *inFile, VJSValue *outResult, JS4D::ExceptionRef *outException, VJSObject* inThisObject = NULL) const;
			bool							EvaluateScript( const VString& inScript, const VURL *inSource, VJSValue *outResult, JS4D::ExceptionRef *outException, VJSObject* inThisObject = NULL) const;
			
			// Check syntax of the script. Return false if erroneous, in which case outException contains details. 

			bool							CheckScriptSyntax (const VString &inScript, const VURL *inSource, JS4D::ExceptionRef *outException) const;

			VJSObject						MakeFunction( const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, JS4D::ExceptionRef *outException);
			bool							GetFunction(const VString& inFunctionRef, VJSObject& outFuncObj);
	
			VJSObject						GetGlobalObject() const;
			VJSGlobalObject*				GetGlobalObjectPrivateInstance() const;
			const VJSGlobalContext*			GetGlobalContext() const
			{
				return fGlobalContext;
			}

			void							GarbageCollect() const;

			void							EnableDebugger()									{ fDebuggerAllowed = true; }
			void							DisableDebugger()									{ fDebuggerAllowed = false; }

											operator JS4D::ContextRef() const					{ return fContext;}
											VJSContext& operator=( const VJSContext& inOther);

			UniChar							GetWildChar() const;

private:
			void							EndOfNativeCodeExecution() const;

			JS4D::ContextRef				fContext;
			const VJSGlobalContext*			fGlobalContext;
			bool							fDebuggerAllowed;
};


//======================================================

class XTOOLBOX_API VJSGlobalContext : public XBOX::VObject, public XBOX::IRefCountable
{
public:
	static	VJSGlobalContext*				Create( IJSRuntimeDelegate *inRuntimeDelegate);

			// evaluates some script in global context and throw an xbox VError if a javascript exception occured.
			// you should not uses these if being called from inside some javascript code (prefer VJSContext variants instead)
			// return true if no exception occured.
			// you'll have to delete any result returned.
			bool							EvaluateScript( VFile *inFile, VValueSingle **outResult, bool inJSONresult = false ) const;
			bool							EvaluateScript( const VString& inScript, const VURL *inSource, VValueSingle **outResult, bool inJSONresult = false ) const;

			// Defines the root of the JavaScript source files for the current JS runtime. At the moment, this is essentially the 
			// root of the current solution. Source root is used by the debugger to sync file paths between the debug server and
			// debug client - they both use JS file paths relative to a given source root.
	static	void							SetSourcesRoot ( const VFolder & inRootFolder );
#if defined(WKA_USE_CHR_REM_DBG)
//#error "GH!!!"
	static	void							SetChrmDebuggerServer( IJSWChrmDebugger* inDebuggerServer );
#else
	static	void							SetDebuggerServer ( IJSWDebugger* inDebuggerServer );
//#error GH
#endif
			// Returns true if debugger is currently paused on either breakpoint or an exception, false otherwise.
	static	bool							IsDebuggerPaused ( );

			// Returns true if there is at least one debug client connected, false otherwise.
	static	bool							IsDebuggerActive ( );

			// Aborts all JS contexts execution blocked in debug on a JS statement because of a break point or whatever other reason.
	static	void							AbortAllDebug ( );

			// Allows or forbids debugging server launch. Typically, debug server is not launched in the studio but is launched on the server.
	static	void							AllowDebuggerLaunch ( );
	static	void							ForbidDebuggerLaunch ( );

			// mark the context as being in use for execution.
			// Note that EvaluateScript and VJSContext( const VJSGlobalContext) call Use/Unuse, so you shouldn't have to call these yourselves.
			JS4D::ContextRef				Use() const;
			void							Unuse() const;

			UniChar							GetWildChar() const;

private:

											VJSGlobalContext( JS4D::GlobalContextRef inGlobalContext):fContext( inGlobalContext)	{;}
											~VJSGlobalContext();

											VJSGlobalContext( const VJSGlobalContext&);	// forbidden
											VJSGlobalContext& operator=( const VJSGlobalContext&);	// forbidden

			JS4D::GlobalContextRef			fContext;
};


END_TOOLBOX_NAMESPACE

#endif
