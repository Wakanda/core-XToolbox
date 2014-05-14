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

#if USE_V8_ENGINE
#include <v8.h>
#include <V4DContext.h>
using namespace v8;

#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#include <4DJavaScriptCore/JS4D_Tools.h>
#else
#include <JavaScriptCore/JavaScript.h>
#include <JavaScriptCore/4D/JS4D_Tools.h>
#endif
#endif

#include "JS4D.h"
#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSJSON.h"
#include "VJSModule.h"

BEGIN_TOOLBOX_NAMESPACE


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
/*VJSContext::VjsScope::VjsScope(const VJSContext&	inContext)
:fHS((V4DContext*)(inContext.fContext->GetData())->GetIsolate())
{
	V4DContext* v8Ctx = (V4DContext*)(inContext.fContext->GetData());

	fContext = new v8::Handle<v8::Context> (
		v8::Handle<v8::Context>::New(
			v8Ctx->GetIsolate(),
			v8Ctx->GetPersistentContext(v8Ctx->GetIsolate())) );
}*/



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


bool VJSContext::EvaluateScript( VFile *inFile, VJSValue *outResult, VJSException& outException, VJSObject* inThisObject) const
{
	return EvaluateScript(	inFile, outResult, outException.GetPtr(), inThisObject );
}

bool VJSContext::EvaluateScript( const VString& inScript, const VURL *inSource, VJSValue *outResult, VJSException& outException, VJSObject* inThisObject) const
{
	return EvaluateScript( inScript, inSource, outResult, outException.GetPtr(), inThisObject );
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
	bool ok = false;
#if USE_V8_ENGINE
	VString	urlString("emptyURL");
	if (inSource)
	{
		inSource->GetAbsoluteURL(urlString,true);
	}
	DebugMsg("VJSContext::EvaluateScript called for <%S>\n",&urlString );
	Isolate*	isolate = Isolate::GetCurrent();
	xbox_assert( isolate == fContext);
	V4DContext* v8Ctx = (V4DContext*)isolate->GetData();
	ok = v8Ctx->EvaluateScript(inScript,urlString,outResult);
/*JS4D::ClassCreate(NULL);
	VStringConvertBuffer	bufferTmp(inScript,VTC_UTF_8);
	Isolate*	isolate = Isolate::GetCurrent();//(Isolate*)fContext;
	//VV8PersistentData* v8PersData = (VV8PersistentData*)isolate->GetData();
	V4DV8Context* v8Ctx = (V4DV8Context*)isolate->GetData();
	HandleScope handle_scope(isolate);
	Local<Context> locCtx = Handle<Context>::New(isolate,v8Ctx->fPersistentCtx);
	Context::Scope context_scope(locCtx);
	Handle<String> scriptStr =
		String::New(bufferTmp.GetCPointer(), bufferTmp.GetSize());
	TryCatch try_catch;
	Handle<Script> compiled_script = Script::Compile(scriptStr);
	if (compiled_script.IsEmpty()) {
		String::Utf8Value error(try_catch.Exception());
		DebugMsg("VJSContext::EvaluateScript error ->%s\n",*error);
    		// The script failed to compile; bail out.
	}
	else
	{
		Handle<Value> result = compiled_script->Run();
		if (result.IsEmpty()) {
			String::Utf8Value error(try_catch.Exception());
			DebugMsg("VJSContext::EvaluateScript exec error ->%s\n",*error);
		}
		else
		{
			ok = true;
		}
	}
*/
#else
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
	
	ok = (result != NULL) && (exception == NULL);
	
	if (outResult != NULL)
	{
		*outResult = VJSValue(fContext,result);
	}

	if (outException != NULL)
	{
		*outException = exception;
	}
#endif
	return ok;
}


bool VJSContext::CheckScriptSyntax (const VString &inScript, const VURL *inSource, VJSException *outException) const
{
	bool		ok = false;
#if USE_V8_ENGINE
	Isolate*	isolate = Isolate::GetCurrent();
	xbox_assert( isolate == fContext);
	V4DContext* 	v8Ctx = (V4DContext*)isolate->GetData();
	VString	urlString("emptyURL");
	if (inSource)
	{
		inSource->GetAbsoluteURL(urlString,true);
	}
	ok = v8Ctx->CheckScriptSyntax(inScript,urlString);

#else
	JSStringRef jsUrl;
	VString		url;

	jsUrl = NULL;
	if (inSource != NULL) {

		inSource->GetAbsoluteURL(url, true);
		jsUrl = JS4D::VStringToString(url);

	}
	
    JSStringRef jsScript; 

	jsScript = JS4D::VStringToString(inScript);
	ok = JSCheckScriptSyntax(fContext, jsScript, jsUrl, 0, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease(jsScript);

	if (jsUrl != NULL)

		JSStringRelease(jsUrl);

#endif
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
#if USE_V8_ENGINE
	for(;;)
		DebugMsg("%s  NOT IMPL\n",__PRETTY_FUNCTION__ );
#else
	VJSContext	vjsContext(fContext);
	return VJSObject(
			vjsContext,
			JS4D::MakeFunction( fContext, inName, inParamNames, inBody, inUrl, inStartingLineNumber, outException));
#endif
}

VJSGlobalObject *VJSContext::GetGlobalObjectPrivateInstance() const
{
#if USE_V8_ENGINE
	Isolate*	isolate = Isolate::GetCurrent();
	xbox_assert( isolate == fContext);
	V4DContext* 	v8Ctx = (V4DContext*)isolate->GetData();
	return static_cast<VJSGlobalObject*>(v8Ctx->GetGlobalObjectPrivateInstance());
#else
	return static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)));
#endif
}


VJSObject VJSContext::GetGlobalObject() const
{
#if USE_V8_ENGINE
	xbox_assert(fContext == Isolate::GetCurrent());
	HandleScope handle_scope(fContext);
	V4DContext* 	v8Ctx = (V4DContext*)fContext->GetData();
	Handle<Context> context = Handle<Context>::New(fContext,v8Ctx->GetPersistentContext(fContext));
	Handle<Object> global = context->Global();
	return VJSObject( fContext, global.val_ );
#else
	VJSContext	vjsContext(fContext);
	return VJSObject( vjsContext, JSContextGetGlobalObject( fContext));
#endif
}


void VJSContext::EndOfNativeCodeExecution() const
{
#if USE_V8_ENGINE
	//xbox_assert(false);//GH
#else
	VJSGlobalObject *privateInstance = GetGlobalObjectPrivateInstance();
	if (privateInstance != NULL)
		privateInstance->GarbageCollectIfNecessary();
#endif
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
	if (fContext != NULL) {
#if USE_V8_ENGINE
		fContext->Exit();
		V4DContext* v8Ctx = (V4DContext*)fContext->GetData();
		if (v8Ctx)
		{
			delete v8Ctx;
		}
		fContext->Dispose();
		fContext = NULL;;
#else
	// Pending fix. (replace with xbox_assert(fContext != NULL)).


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

			moduleState->GetRequireFunction().Unprotect();

		}
	
		JSGlobalContextRelease(fContext);
#endif

	}
}


/*
	static
*/
VJSGlobalContext *VJSGlobalContext::Create( IJSRuntimeDelegate *inRuntimeDelegate)
{
#if 0
DebugMsg("VJSGlobalContext::Create called\n");
#endif
	VJSGlobalClass::Class();
	return VJSGlobalContext::Create( inRuntimeDelegate, VJSGlobalClass::Class());
}

/*
	static
*/
VJSGlobalContext *VJSGlobalContext::Create( IJSRuntimeDelegate *inRuntimeDelegate, JS4D::ClassRef inGlobalClassRef)
{
#if USE_V8_ENGINE
	Isolate*	ref = Isolate::New();
	if (testAssert(ref))
	{
		ref->Enter();

		DebugMsg("VJSGlobalContext::Create2 called classRef=%s\n",((JS4D::ClassDefinition*)inGlobalClassRef)->className);

		V4DContext*			v8Ctx = new V4DContext(ref,inGlobalClassRef);
		VJSGlobalObject*	globalObject = new VJSGlobalObject( ref, NULL, inRuntimeDelegate);
		v8Ctx->SetGlobalObjectPrivateInstance(globalObject);

		HandleScope handle_scope(ref);		
		Handle<Context> context = Handle<Context>::New(ref,V4DContext::GetPersistentContext(ref));
		Context::Scope context_scope(context);
		Local<Object> global = context->Global();
		((JS4D::ClassDefinition*)inGlobalClassRef)->initialize(ref,*global);
		return new VJSGlobalContext(ref);
#else
	JSGlobalContextRef ref = JSGlobalContextCreateInGroup( NULL, inGlobalClassRef);
	if (ref != NULL)
	{
		VJSGlobalObject *globalObject = new VJSGlobalObject( ref, NULL, inRuntimeDelegate);
		JSObjectSetPrivate( JSContextGetGlobalObject( ref), globalObject);

		return new VJSGlobalContext( ref);
#endif
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
#if USE_V8_ENGINE
	//xbox_assert(false);

#else
    JSStringRef		jsstrRoot = JS4D::VStringToString ( vstrRoot );
	if ( jsstrRoot != 0 )
	{
		JSSetSourcesRoot ( jsstrRoot );
		JSStringRelease ( jsstrRoot );
	}
#endif
}

bool VJSGlobalContext::IsDebuggerPaused ( )
{
#if USE_V8_ENGINE
	xbox_assert(false);
	return false;
#else
	return JSIsDebuggerPaused ( );
#endif
}

bool VJSGlobalContext::IsDebuggerActive ( )
{
#if USE_V8_ENGINE
	//xbox_assert(false);
	return false;
#else
	return JSIsDebuggerActive ( );
#endif
}

void VJSGlobalContext::AbortAllDebug ( )
{
#if USE_V8_ENGINE
	//xbox_assert(false);
#else
	JSAbortAllDebug ( );
#endif
}

void VJSGlobalContext::AllowDebuggerLaunch ( )
{
#if USE_V8_ENGINE
	//xbox_assert(false);
#else
	JSAllowDebuggerLaunch ( );
#endif
}

void VJSGlobalContext::ForbidDebuggerLaunch ( )
{
#if USE_V8_ENGINE
	xbox_assert(false);
#else
	JSForbidDebuggerLaunch ( );
#endif
}

IRemoteDebuggerServer*		VJSGlobalContext::sWAKDebuggerServer = NULL;
XBOX::VCriticalSection		VJSGlobalContext::sWAKDebuggerServerLock;

void VJSGlobalContext::SetDebuggerServer(	IWAKDebuggerServer*		inDebuggerServer,
											IChromeDebuggerServer*	inChromeDebuggerServer,
											IRemoteDebuggerServer*	inNoDebugger)
{
#if USE_V8_ENGINE
	//xbox_assert(false);
#else
	
	sWAKDebuggerServerLock.Lock();
	JS4DSetDebuggerServer( inDebuggerServer,inChromeDebuggerServer, inNoDebugger);
	sWAKDebuggerServer =
		(inDebuggerServer ?
			(IRemoteDebuggerServer*)inDebuggerServer :
			(inChromeDebuggerServer ? (IRemoteDebuggerServer*)inChromeDebuggerServer : inNoDebugger ) );
	sWAKDebuggerServerLock.Unlock();
#endif
}

IRemoteDebuggerServer* VJSGlobalContext::GetDebuggerServer( )
{
	IRemoteDebuggerServer*	result = NULL;
#if USE_V8_ENGINE
	xbox_assert(false);
#else
		
	sWAKDebuggerServerLock.Lock();
	result = sWAKDebuggerServer;
	sWAKDebuggerServerLock.Unlock();
#endif
	return result;
}


JS4D::ContextRef VJSGlobalContext::Use() const
{
#if USE_V8_ENGINE
	//xbox_assert(false);
#else
	
	static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)))->UseContext();
#endif
	return fContext;
}


void VJSGlobalContext::Unuse() const
{
#if USE_V8_ENGINE
	//xbox_assert(false);
#else	
	static_cast<VJSGlobalObject*>( JSObjectGetPrivate( JSContextGetGlobalObject( fContext)))->UnuseContext();
#endif
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

	VJSValue result(context);
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
		{
			if ( inJSONresult )
			{
				VJSJSON stringifier( context );
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
				*outResult = result.CreateVValue(false);
		}
	}

	Unuse();
	
	return ok;
}

END_TOOLBOX_NAMESPACE
