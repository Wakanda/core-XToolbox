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
#include "VJavaScriptPrecompiled.h"

#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif


#include "JS4D.h"
#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSJSON.h"
#include "VJSModule.h"

USING_TOOLBOX_NAMESPACE



static bool LoadScriptFile( VFile *inFile, VString& outScript)
{
	VFileStream stream( inFile);
	VError err = stream.OpenReading();
	if (err == VE_OK)
	{
		err = stream.GuessCharSetFromLeadingBytes( VTC_DefaultTextExport );		// sc 18/03/2011 instead of VTC_UTF_8
		stream.SetCarriageReturnMode( eCRM_NATIVE );
		if (err == VE_OK)
			err = stream.GetText( outScript);
	}
	stream.CloseReading();
	return err == VE_OK;
}


//======================================================

VJSContext::VJSContext( const VJSGlobalContext *inGlobalContext)
: fGlobalContext( RetainRefCountable( inGlobalContext))
, fContext( inGlobalContext->Use())
, fDebuggerAllowed(true)
{
}


VJSContext::~VJSContext()
{
	if (fGlobalContext != NULL)
	{
		fGlobalContext->Unuse();
		fGlobalContext->Release();
	}
}

VJSContext& VJSContext::operator=( const VJSContext& inOther)
{
	fContext = inOther.fContext;
	fGlobalContext = RetainRefCountable( inOther.fGlobalContext);
	if (fGlobalContext != NULL)
	{
		fGlobalContext->Use();
	}
	return *this;
}



bool VJSContext::EvaluateScript( VFile *inFile, VJSValue *outResult, JS4D::ExceptionRef *outException, VJSObject* inThisObject) const
{
	VString script;
	bool ok = LoadScriptFile( inFile, script);
	
	if (ok)
	{
		VURL url( inFile->GetPath());
		ok = EvaluateScript( script, &url, outResult, outException, inThisObject);
	}

	return ok;
}


bool VJSContext::EvaluateScript( const VString& inScript, const VURL *inSource, VJSValue *outResult, JS4D::ExceptionRef *outException, VJSObject* inThisObject) const
{
	VJSGlobalObject	*globalObject = GetGlobalObjectPrivateInstance();

	// globalObject might be NULL when used on a WebView context.

	JSStringRef jsUrl;
	if (inSource != NULL)
	{
		VString url;
		inSource->GetAbsoluteURL( url, true);
		jsUrl = JS4D::VStringToString( url);
	}
	else
	{
		jsUrl = NULL;	
	}
	
	JSValueRef exception = NULL;
    JSStringRef jsScript = JS4D::VStringToString( inScript);
	int nStartingLineNumber = fDebuggerAllowed ? 0 : -1;
	JSValueRef result = JS4DEvaluateScript( fContext, jsScript, inThisObject == NULL ? NULL : inThisObject->GetObjectRef()/*thisObject*/, jsUrl, nStartingLineNumber, &exception);
    JSStringRelease( jsScript);
	
	if (jsUrl != NULL)
		JSStringRelease( jsUrl);
	
	bool ok = (result != NULL) && (exception == NULL);
	
	if (outResult != NULL)
	{
		outResult->SetValueRef( result);
	}

	if (outException != NULL)
	{
		*outException = exception;
	}

	return ok;
}


bool VJSContext::CheckScriptSyntax (const VString &inScript, const VURL *inSource, JS4D::ExceptionRef *outException) const
{
	bool		ok;
	JSStringRef jsUrl;
	VString		url;

	jsUrl = NULL;
	if (inSource != NULL) {

		inSource->GetAbsoluteURL(url, true);
		jsUrl = JS4D::VStringToString(url);

	}
	
	JSValueRef	exception;
    JSStringRef jsScript; 

	exception = NULL;
	jsScript = JS4D::VStringToString(inScript);
	ok = JSCheckScriptSyntax(fContext, jsScript, jsUrl, 0, &exception);
	JSStringRelease(jsScript);

	if (jsUrl != NULL)

		JSStringRelease(jsUrl);
	
	if (outException != NULL)

		*outException = exception;

	return ok;
}


bool VJSContext::GetFunction(const VString& inFunctionRef, VJSObject& outFuncObj)
{
	bool found = false;
	VJSValue result(*this);

	EvaluateScript(inFunctionRef, nil, &result, nil, nil);

	if (result.IsObject())
	{
		VJSObject funcobj(result.GetObject());
		if (funcobj.IsFunction())
		{
			outFuncObj = funcobj;
			found = true;
		}
	}
	return found;
}



VJSObject VJSContext::MakeFunction( const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, JS4D::ExceptionRef *outException)
{
	return VJSObject( fContext, JS4D::MakeFunction( fContext, inName, inParamNames, inBody, inUrl, inStartingLineNumber, outException));
}

VJSGlobalObject *VJSContext::GetGlobalObjectPrivateInstance() const
{
	return static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)));
}


VJSObject VJSContext::GetGlobalObject() const
{
	return VJSObject( fContext, JSContextGetGlobalObject( fContext));
}


void VJSContext::EndOfNativeCodeExecution() const
{
	VJSGlobalObject *privateInstance = GetGlobalObjectPrivateInstance();
	if (privateInstance != NULL)
		privateInstance->GarbageCollectIfNecessary();
}


void VJSContext::GarbageCollect() const
{
	GetGlobalObjectPrivateInstance()->GarbageCollect();
}


UniChar VJSContext::GetWildChar() const
{ 
	return fGlobalContext->GetWildChar(); 
}


//======================================================


VJSGlobalContext::~VJSGlobalContext()
{
	// Pending fix. (replace with xbox_assert(fContext != NULL)).

	if (fContext != NULL) {

		// require() function object is protected from garbage collection.
		// Unprotect it before the global context is released.

		XBOX::VJSContext	context(fContext);
		XBOX::VJSObject		globalObject(context.GetGlobalObject());
		XBOX::VJSObject		requireObject(context);
		
		requireObject = globalObject.GetPropertyAsObject("require");
		if (requireObject.IsOfClass(VJSRequireClass::Class())) {

			VJSModuleState	*moduleState;

			moduleState = requireObject.GetPrivateData<VJSRequireClass>();
			xbox_assert(moduleState);

			JS4D::UnprotectValue(fContext, moduleState->GetRequireFunctionRef());

		}
	
		JSGlobalContextRelease(fContext);

	}
}


/*
	static
*/
VJSGlobalContext *VJSGlobalContext::Create( IJSRuntimeDelegate *inRuntimeDelegate)
{
	return VJSGlobalContext::Create( inRuntimeDelegate, VJSGlobalClass::Class());
}


/*
	static
*/
VJSGlobalContext *VJSGlobalContext::Create( IJSRuntimeDelegate *inRuntimeDelegate, JS4D::ClassRef inGlobalClassRef)
{
	JSGlobalContextRef ref = JSGlobalContextCreateInGroup( NULL, inGlobalClassRef);
	if (ref != NULL)
	{
		VJSGlobalObject *globalObject = new VJSGlobalObject( ref, NULL, inRuntimeDelegate);
		JSObjectSetPrivate( JSContextGetGlobalObject( ref), globalObject);

		return new VJSGlobalContext( ref);
	}
	else
		return NULL;
}

/*
	static
*/
void VJSGlobalContext::SetSourcesRoot ( const VFolder & inRootFolder )
{
	VFilePath		vfPathRoot;
	inRootFolder. GetPath ( vfPathRoot );
	
	VString			vstrRoot;
	vfPathRoot. GetPosixPath ( vstrRoot );

    JSStringRef		jsstrRoot = JS4D::VStringToString ( vstrRoot );
	if ( jsstrRoot != 0 )
	{
		JSSetSourcesRoot ( jsstrRoot );
		JSStringRelease ( jsstrRoot );
	}
}

bool VJSGlobalContext::IsDebuggerPaused ( )
{
	return JSIsDebuggerPaused ( );
}

bool VJSGlobalContext::IsDebuggerActive ( )
{
	return JSIsDebuggerActive ( );
}

void VJSGlobalContext::AbortAllDebug ( )
{
	JSAbortAllDebug ( );
}

void VJSGlobalContext::AllowDebuggerLaunch ( )
{
	JSAllowDebuggerLaunch ( );
}

void VJSGlobalContext::ForbidDebuggerLaunch ( )
{
	JSForbidDebuggerLaunch ( );
}

IRemoteDebuggerServer*		VJSGlobalContext::sWAKDebuggerServer = NULL;
XBOX::VCriticalSection		VJSGlobalContext::sWAKDebuggerServerLock;

void VJSGlobalContext::SetDebuggerServer(	IWAKDebuggerServer*		inDebuggerServer,
											IChromeDebuggerServer*	inChromeDebuggerServer,
											IRemoteDebuggerServer*	inNoDebugger)
{
	sWAKDebuggerServerLock.Lock();
	JS4DSetDebuggerServer( inDebuggerServer,inChromeDebuggerServer, inNoDebugger);
	sWAKDebuggerServer =
		(inDebuggerServer ?
			(IRemoteDebuggerServer*)inDebuggerServer :
			(inChromeDebuggerServer ? (IRemoteDebuggerServer*)inChromeDebuggerServer : inNoDebugger ) );
	sWAKDebuggerServerLock.Unlock();
}

IRemoteDebuggerServer* VJSGlobalContext::GetDebuggerServer( )
{
	IRemoteDebuggerServer*	result = NULL;
	
	sWAKDebuggerServerLock.Lock();
	result = sWAKDebuggerServer;
	sWAKDebuggerServerLock.Unlock();

	return result;
}


JS4D::ContextRef VJSGlobalContext::Use() const
{
	static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)))->UseContext();
	return fContext;
}


void VJSGlobalContext::Unuse() const
{
	static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)))->UnuseContext();
}


UniChar VJSGlobalContext::GetWildChar() const
{
	// right now only use the VIntlMgr set in the task
	VIntlMgr* intl = VTask::GetCurrentIntlManager();
	if (intl == nil)
		return '*';
	else
		return intl->GetCollator()->GetWildChar();
}



bool VJSGlobalContext::EvaluateScript( VFile *inFile, VValueSingle **outResult, bool inJSONresult ) const
{
	VString script;
	bool ok = LoadScriptFile( inFile, script);
	if (ok)
	{
		VURL url( inFile->GetPath());
		ok = EvaluateScript( script, &url, outResult, inJSONresult );
	}

	return ok;
}


bool VJSGlobalContext::EvaluateScript( const VString& inScript, const VURL *inSource, VValueSingle **outResult, bool inJSONresult ) const
{
	VJSContext context( fContext);

	Use();

	VJSValue result( fContext, NULL);
	JS4D::ExceptionRef exception = NULL;

	bool ok = context.EvaluateScript( inScript, inSource, &result, &exception);
	
	// first throw an error for javascript if necessary
	if (JS4D::ThrowVErrorForException( fContext, exception))
	{
		xbox_assert( !ok);
		// then rethrow an error to explain wat we were doing
		VTask::GetCurrent()->GetDebugContext().DisableUI();
		{
			VString url;
			if (inSource != NULL)
				inSource->GetAbsoluteURL( url, false);
			if (!url.IsEmpty())
			{
				StThrowError<>	error( MAKE_VERROR( 'JS4D', 2));
				error->SetString( "url", url);
				error->SetString( "error_description", CVSTR( "Error evaluating javascript {url}"));
			}
			else
			{
				StThrowError<>	error( MAKE_VERROR( 'JS4D', 3));
				error->SetString( "error_description", CVSTR( "Error evaluating javascript"));
			}
		}
		VTask::GetCurrent()->GetDebugContext().EnableUI();
		if (outResult != NULL)
			*outResult = NULL;
	}
	else
	{
		xbox_assert( ok);
		if (outResult != NULL)
		if ( inJSONresult )
		{
			VJSJSON stringifier( fContext );
			VString* ptStr = new VString;
			// The "result" value's fValue is initialized to 0 when a script is aborted using the JS debugger. In this case js core crashes while stringifying, thus the if-else
			// The if-else can be moved directly into ::Stringify if "null" is acceptable as a generic return value
			if ( result. IsNull ( ) )
				ptStr-> AppendCString ( "null" );
			else
				stringifier.Stringify( result, *ptStr );
			*outResult = ptStr;
		}
		else
			*outResult = result.CreateVValue( NULL);
	}

	Unuse();
	
	return ok;
}

