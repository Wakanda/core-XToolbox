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
using namespace v8;
#include <V4DContext.h>

#else
#if VERSIONMAC
#include <4DJavaScriptCore/JavaScriptCore.h>
#else
#include <JavaScriptCore/JavaScript.h>
#endif
#endif

#include "VJSContext.h"
#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE

#if 0
static VURL * CreateURLFromPath(const VString& inPath);
static VURL * CreateURLFromPath(const VString& inPath)
{
	VURL* result = NULL;

	if(inPath.GetLength() > 0)
	{
		VIndex index = inPath.Find("://");
		if(index > 0)//URL
		{
			result = new VURL(inPath, false);
		}
		else//system path
		{
			result = new VURL(inPath, eURL_POSIX_STYLE, L"file://");
		}
	}

	return result;
}
#endif



//======================================================
#if USE_V8_ENGINE

/*
void VJSParms_construct::CallbackProlog()
{
    xbox::DebugMsg("VJSParms_construct::CallbackProlog called  nbArgs=%d isConstruct=%d InternalFieldCount()=%d\n",
		fV8Arguments->Length(),
		(fV8Arguments->IsConstructCall() ? 1 : 0),
		fV8Arguments->Holder()->InternalFieldCount());
}
void VJSParms_construct::CallbackEpilog()
{
}
void VJSParms_construct::ReturnConstructedObjectProlog()
{
	xbox::DebugMsg("VJSParms_construct::ReturnConstructedObjectProlog called  IsConstructCall()=%d\n",
		(fV8Arguments->IsConstructCall() ? 1 : 0));
}
*/

class XBOX::JS4D::V8ObjectGetPropertyCallback
{
public:
	V8ObjectGetPropertyCallback(XBOX::GetPropertyFn inFunction) { fGetPropertyFn = inFunction; }

	static	void					AccessorGetterCallback(Local<String> property, const PropertyCallbackInfo<Value>& info);

private:
	XBOX::GetPropertyFn				fGetPropertyFn;
};


class XBOX::JS4D::V8ObjectSetPropertyCallback
{
public:
	V8ObjectSetPropertyCallback(XBOX::SetPropertyFn inFunction) { fSetPropertyFn = inFunction; }

	static	void					AccessorSetterCallback(Local<String> property, Local<Value> value, const PropertyCallbackInfo<Value>& info);

private:
	XBOX::SetPropertyFn				fSetPropertyFn;
};

struct V8PropertyHandlerCallback_st
{
	V8PropertyHandlerCallback_st(JS4D::ObjectGetPropertyCallback inGetCbk, JS4D::ObjectSetPropertyCallback inSetCbk) { getCbk = inGetCbk; setCbk = inSetCbk; }
	JS4D::ObjectGetPropertyCallback		getCbk;
	JS4D::ObjectSetPropertyCallback		setCbk;
};

void XBOX::JS4D::V8ObjectGetPropertyCallback::AccessorGetterCallback(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info)
{
	v8::String::Utf8Value	utfVal(property);
	VString	propName(*utfVal);

	XBOX::GetPropertyFn		getPropertyFn = NULL;
	Local<Value> dataVal = info.Data();
	if (dataVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(dataVal);
		V8PropertyHandlerCallback_st*	propertyHandlerCallback = (V8PropertyHandlerCallback_st*)extHdl->Value();
		//JS4D::ObjectGetPropertyCallback getPropertyCallback = (JS4D::ObjectGetPropertyCallback)extHdl->Value();
		if (testAssert(propertyHandlerCallback->getCbk))
		{
			getPropertyFn = propertyHandlerCallback->getCbk()->fGetPropertyFn;//getPropertyCallback()->fGetPropertyFn;
		}
	}
	VJSParms_getProperty	parms(propName, info);
	Local<Object> locObj = info.Holder();
	void* data = JS4D::GetObjectPrivate(info.GetIsolate(), locObj.val_);

	getPropertyFn(parms, data);
}

void XBOX::JS4D::V8ObjectSetPropertyCallback::AccessorSetterCallback(Local<String> property, Local<Value> value, const PropertyCallbackInfo<v8::Value>& info)
{
	v8::String::Utf8Value	utfVal(property);
	VString	propName(*utfVal);

	XBOX::SetPropertyFn		setPropertyFn = NULL;
	Local<Value> dataVal = info.Data();
	if (dataVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(dataVal);
		V8PropertyHandlerCallback_st*	propertyHandlerCallback = (V8PropertyHandlerCallback_st*)extHdl->Value();
		if (testAssert(propertyHandlerCallback->setCbk))
		{
			setPropertyFn = propertyHandlerCallback->setCbk()->fSetPropertyFn;
		}
	}
	VJSParms_setProperty	parms(propName, value.val_, info);
	Local<Object> locObj = info.Holder();
	void* data = JS4D::GetObjectPrivate(info.GetIsolate(), locObj.val_);

	setPropertyFn(parms, data);
}

void* XBOX::VJSClass__GetV8PropertyGetterCallback(JS4D::V8ObjectGetPropertyCallback* inCbk)
{
	return (void*)inCbk->AccessorGetterCallback;
}

void* XBOX::VJSClass__GetV8PropertySetterCallback(JS4D::V8ObjectSetPropertyCallback* inCbk)
{
	return (void*)inCbk->AccessorSetterCallback;
}

XBOX::JS4D::V8ObjectGetPropertyCallback* XBOX::VJSClass__GetPropertyGetterCallback(XBOX::GetPropertyFn inFunction)
{
	XBOX::JS4D::V8ObjectGetPropertyCallback*	result = NULL;
	result = new XBOX::JS4D::V8ObjectGetPropertyCallback(inFunction);
	return result;
}

XBOX::JS4D::V8ObjectSetPropertyCallback* XBOX::VJSClass__GetPropertySetterCallback(XBOX::SetPropertyFn inFunction)
{
	XBOX::JS4D::V8ObjectSetPropertyCallback*	result = NULL;
	result = new XBOX::JS4D::V8ObjectSetPropertyCallback(inFunction);
	return result;
}
void* XBOX::VJSClass__GetV8PropertyHandlerCallback(JS4D::ObjectGetPropertyCallback cbkG, JS4D::ObjectSetPropertyCallback cbkS)
{
	return new V8PropertyHandlerCallback_st(cbkG, cbkS);
}




void XBOX::VJSClassImpl::CallStaticFunctionEpilog(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	//DebugMsg("VJSClassImpl::CallStaticFunctionEpilog called IsConstructCall=%d\n",
	//    args.IsConstructCall() ? 1 : 0);
}

bool XBOX::VJSClassImpl::CallAsConstructor(JS4D::ClassRef inClassRef, VJSParms_callAsConstructor& parms, fn_callAsConstructor fn)
{
	bool isOK = false;
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	VJSObject	constructedObject = parms.GetConstructedObject();
	VJSContext	vjsCtx = constructedObject.GetContext();
	JS4D::ObjectRef objRef = parms.GetConstructedObjectRef();
#if 0
	if (parms.CountParams() >= 16)
	{
		isOK = true;
		if (parms.CountParams() >= 32)
		{
			return true;
		}

		//HandleScope handle_scope();
		//v8::Persistent<v8::Context>& 		persistentContext = V4DContext::GetPersistentContext(vjsCtx);
		//Context::Scope context_scope(vjsCtx,persistentContext);
		//Handle<Context> context = Handle<Context>::New(vjsCtx,persistentContext);
		DebugMsg("VJSClassImpl::CallAsConstructor adding private value %x for classDef=%s \n",
			NULL,
			classDef->className);
		if (objRef->IsObject())
		{
			Local<Object>		locObj = objRef->ToObject();
			VJSValue 			extVal = parms.GetParamValue(1);
			JS4D::ValueRef		valRef = extVal.GetValueRef();
			if (valRef && valRef->IsExternal())
			{
				External* extPtr = External::Cast(valRef);
				void* prvData = extPtr->Value();
				Local<String>		constrName = locObj->GetConstructorName();
				String::Utf8Value	utf8Name(constrName);

				DebugMsg("VJSClassImpl::CallAsConstructor adding private value %x for classDef=%s hash=%d objRef->isFunc=%d\n",
					prvData,
					classDef->className,
					locObj->GetIdentityHash(),
					locObj->IsFunction());

				DebugMsg("VJSClassImpl::CallAsConstructor adding private value %x for innerClass '%s' objRef->isFunc=%d\n",
					prvData,
					*utf8Name,
					locObj->IsFunction());
				Local<External> extHdl = External::New(prvData);
				locObj->SetHiddenValue(String::NewSymbol(K_WAK_PRIVATE_DATA_ID), extHdl);
			}
		}
		//persistentContext.Reset(vjsCtx,context);

	}
	else
#endif
	if (parms.IsConstructCall())
	{
		if (parms.CountParams() >= 32)
		{
		}
		else
		{
			fn(parms);
			JS4D::ContextRef	ctxRef = vjsCtx;
			if (classDef->initialize)
			{
				classDef->initialize(ctxRef, objRef);
			}
		}
	}
	return isOK;
}

void VJSClassImpl::CreateInstanceEpilog(const XBOX::VJSContext& inContext, void *inPrivateData, JS4D::ObjectRef inObjectRef, JS4D::ClassRef inClassRef)
{
	HandleScope handle_scope(inContext);
	v8::Persistent<v8::Context>& 		persistentContext = V4DContext::GetPersistentContext(inContext);
	Context::Scope context_scope(inContext, persistentContext);
	Handle<Context> context = Handle<Context>::New(inContext, persistentContext);
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	if (inObjectRef->IsObject())
	{
		Local<Object>		locObj = inObjectRef->ToObject();
		DebugMsg("VJSClassImpl::CreateInstanceEpilog adding private value %x for %s\n", inPrivateData, classDef->className);
		Handle<External> extPtr = External::New(inPrivateData);
		bool	isOK = locObj->SetHiddenValue(String::NewSymbol(K_WAK_PRIVATE_DATA_ID), extPtr);

		//if (inClassDef.staticValues)
		{
			/*for( const JS4D::StaticValue *i = inClassDef.staticValues ; i->name != NULL ; ++i)
			{
			DebugMsg("VJSClassImpl::CreateInstanceEpilog treat static value %s for %s\n",i->name,inClassDef.className);
			JS4D::ObjectGetPropertyCallback cbk = i->getProperty;
			locObj->SetAccessor(v8::String::New(i->name),cbk()->AccessorGetterCallback);
			}*/
		}
		JS4D::ContextRef	ctxRef = inContext;
		if (classDef->initialize)
		{
			classDef->initialize(ctxRef, inObjectRef);
		}
	}
	persistentContext.Reset(inContext, context);

	/*if (inClassDef.staticFunctions)
	{
	for( const JS4D::StaticFunction *i = inClassDef.staticFunctions ; i->name != NULL ; ++i)
	{
	DebugMsg("::CreateInstanceEpilog treat static function %s for %s\n",i->name,inClassDef.className);

	}
	}*/
}
VJSObject VJSClassImpl::CreateInstanceFromPrivateData(const XBOX::VJSContext& inContext, JS4D::ClassRef inClassRef, void *inPrivateData)
{
	HandleScope handle_scope(inContext);
	v8::Persistent<v8::Context>& 		persistentContext = V4DContext::GetPersistentContext(inContext);
	Context::Scope context_scope(inContext, persistentContext);
	Handle<Context> context = Handle<Context>::New(inContext, persistentContext);
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	V4DContext*	v8Ctx = (V4DContext*)(v8::Isolate::GetCurrent()->GetData());

	Persistent<FunctionTemplate>*	pFT = v8Ctx->GetImplicitFunctionConstructor(classDef->className);
	Local<Function> locFunc = (pFT->val_)->GetFunction();

	/*Handle<Value> valuesArray[32];
	for( int idx=0; idx<32; idx++ )
	{
	valuesArray[idx] = v8::Integer::New(0);
	}*/
	Local<Value> locVal = locFunc->CallAsConstructor(0, NULL);//32 , valuesArray);
	if (!strcmp(classDef->className, "requireClass"))
	{
		if (locVal->IsObject())
		{
			int a = 1;
		}
		if (locVal->IsFunction())
		{
			int a = 1;
		}
	}

	V4DContext::SetObjectPrivateData(v8::Isolate::GetCurrent(), locVal.val_, inPrivateData);
	VJSObject	result(inContext, locVal.val_);
	return result;
}

#else


#endif


//======================================================

/*
void VJSParms_finalize::UnprotectValue( JS4D::ValueRef inValue) const
{
	JSValueUnprotect( NULL, inValue);
}
*/


//======================================================



