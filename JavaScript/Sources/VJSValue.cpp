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
#include <V4DV8Context.h>
#include <v8.h>
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

#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE

#if USE_V8_ENGINE
VJSValue::~VJSValue()
{
	DebugMsg("VJSValue::~VJSValue( CALLED!!!!\n");
	fPersValue->Dispose();
	delete fPersValue;
}
VJSValue::VJSValue( const VJSContext& inContext) : fContext( inContext), fValue( NULL),
		fPersValue(new Persistent<Value>())
{
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Handle<Value>	tmpVal;
	(*fPersValue).Reset(fContext,tmpVal);
}

VJSValue::VJSValue( const VJSContext& inContext, const XBOX::VJSException& inException ) :
	fContext(inContext), fValue( NULL),
		fPersValue(NULL)
{
	Handle<Value>	tmpVal();
	/*fPersValue = new Persistent<Value>();//fContext,tmpVal);
	fPersValue->Reset(fContext,tmpVal);*/
}

VJSValue::VJSValue( const VJSValue& inOther):
									fContext( inOther.fContext)
									,fValue(inOther.fValue)
									//,fPersValue(new Persistent<Value>(inOther.fContext,*inOther.fPersValue))
{
	fPersValue->Reset(inOther.fContext,*inOther.fPersValue);

}

const VJSValue&		VJSValue::operator=( const VJSValue& inOther)	{
				fValue = inOther.fValue;
				fContext = inOther.fContext;
				fPersValue->Reset(inOther.fContext,*inOther.fPersValue);
				return *this;
			}

VJSValue::VJSValue( JS4D::ContextRef inContext) : fContext( inContext), fValue( NULL),
		fPersValue(new Persistent<Value>())
{
DebugMsg("VJSValue::VJSValue Value const by ContextRef!!!\n");
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Handle<Value>	tmpVal;
	(*fPersValue).Reset(fContext,tmpVal);
}
VJSValue::VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue) : fContext( inContext), fValue(inValue),
		fPersValue(new Persistent<Value>())
{
DebugMsg("VJSValue::VJSValue Value const by ContextRef and ValueRef!!!\n");
	//fPersValue->Reset(inContext,inValue);
}
VJSValue::VJSValue( JS4D::ContextRef inContext, const XBOX::VJSException& inException ) : fContext(inContext)//, fValue((JS4D::ValueRef)inException.GetValue())
{
}

VJSValue::operator JS4D::ValueRef() const
{
	return fPersValue->val_;
}
JS4D::ValueRef		VJSValue::GetValueRef() const 
{
	return fPersValue->val_;
//	return fValue;
}

			
bool VJSValue::IsUndefined() const
{
    return JS4D::ValueIsUndefined( fContext, fPersValue->val_ );
}
bool VJSValue::IsNull() const
{
    return JS4D::ValueIsNull( fContext, fPersValue->val_);
}
bool VJSValue::IsNumber() const		{ return JS4D::ValueIsNumber( fContext, fPersValue->val_); }
bool VJSValue::IsBoolean() const	{ return JS4D::ValueIsBoolean( fContext, fPersValue->val_); }
bool VJSValue::IsString() const		{ return JS4D::ValueIsString( fContext, fPersValue->val_); }
bool VJSValue::IsObject() const		{ return JS4D::ValueIsObject( fContext, fPersValue->val_); }
bool VJSValue::IsArray() const
{
    return fPersValue->val_ ? false : fPersValue->val_->IsArray();
}

bool VJSValue::IsInstanceOf( const XBOX::VString& inConstructorName, VJSException* outException ) const		{
				return false;//IsInstanceOf( inConstructorName, VJSException::GetExceptionRefIfNotNull(outException));
}
#endif

bool VJSValue::GetString( VString& outString, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	HandleScope scope(fContext);
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Handle<Value>	hdl = Handle<Value>::New(fContext,*fPersValue);
	String::Utf8Value  value2(hdl);
	outString = *value2;
	return true;
#else
	return (fValue != NULL) ? JS4D::ValueToString( fContext, fValue, outString, outException) : false;
#endif
}


bool VJSValue::GetLong( sLONG *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Local<Number>	nb = fPersValue->val_->ToNumber(); 
	double r = nb->Value();
	sLONG l = (sLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	return true;
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		sLONG l = (sLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
#endif
}


bool VJSValue::GetLong8( sLONG8 *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Local<Number>	nb = fPersValue->val_->ToNumber(); 
	double r = nb->Value();
	sLONG8 l = (sLONG8) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	return true;
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		sLONG8 l = (sLONG8) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
#endif
}


bool VJSValue::GetULong(uLONG *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Local<Number>	nb = fPersValue->val_->ToNumber(); 
	double r = nb->Value();
	uLONG l = (uLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	return true;
#else
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		uLONG l = (uLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
#endif
}



bool VJSValue::GetReal( Real *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Local<Number>	nb = fPersValue->val_->ToNumber(); 
	*outValue = nb->Value(); 
	return true;
#else
	if (fValue != NULL)
		*outValue = JSValueToNumber( fContext, fValue, outException);
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
#endif
}


bool VJSValue::GetBool( bool *outValue, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Local<v8::Boolean>	boolVal = fPersValue->val_->ToBoolean(); 
	*outValue = boolVal->Value(); 
	return true;
#else
	if (fValue != NULL)
		*outValue = JSValueToBoolean( fContext, fValue);
	return (fValue != NULL);
#endif
}


bool VJSValue::GetTime( VTime& outTime, JS4D::ExceptionRef *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Handle<Value> value = Handle<Value>::New(fContext,*fPersValue);
	if (value->IsDate())//fPersValue->val_->IsDate())
	{
		//v8::Date*	dateObject = v8::Date::Cast(fPersValue->val_); 
		ok = JS4D::DateObjectToVTime( fContext, value, outTime, outException, false);
	}
#else
	if (JS4D::ValueIsInstanceOf( fContext, fValue, CVSTR( "Date"), outException))
	{
		JSObjectRef dateObject = JSValueToObject( fContext, fValue, outException);
		ok = JS4D::DateObjectToVTime( fContext, dateObject, outTime, outException, false);
	}
#endif
	else
	{
		outTime.SetNull( true);
		ok = false;
	}

	return ok;
}


bool VJSValue::GetDuration( VDuration& outDuration, JS4D::ExceptionRef *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE
	HandleScope scope(fContext);
	V4DV8Context* v8Ctx = (V4DV8Context*)fContext->GetData();
	Context::Scope context_scope(fContext,v8Ctx->fPersistentCtx);
	Handle<Value> value = Handle<Value>::New(fContext,*fPersValue);
	ok = JS4D::ValueToVDuration( fContext, value, outDuration, outException);

#else
	ok = JS4D::ValueToVDuration( fContext, fValue, outDuration, outException);
#endif
	return ok;
}


bool VJSValue::GetURL( XBOX::VURL& outURL, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	return false;
#else
	VString path;
	if (!GetString( path, outException) && !path.IsEmpty())
		return false;
	
	JS4D::GetURLFromPath( path, outURL);

	return true;
#endif
}


VFile *VJSValue::GetFile( JS4D::ExceptionRef *outException) const
{
	JS4DFileIterator *fileIter = NULL;
#if USE_V8_ENGINE
	return NULL;
#else
	if (outException)
	{
		VJSException	except;
		fileIter = GetObjectPrivateData<VJSFileIterator>(&except);
		*outException = except.GetValue();
	}
	else
	{
		fileIter = GetObjectPrivateData<VJSFileIterator>(NULL);
	}
	return (fileIter == NULL) ? NULL : fileIter->GetFile();
#endif
}


VFolder* VJSValue::GetFolder( JS4D::ExceptionRef *outException) const
{
	JS4DFolderIterator *folderIter = NULL;
#if USE_V8_ENGINE
	return NULL;
#else
	if (outException)
	{
		VJSException	except;
		folderIter = GetObjectPrivateData<VJSFolderIterator>(&except);
		*outException = except.GetValue();
	}
	else
	{
		folderIter = GetObjectPrivateData<VJSFolderIterator>(NULL);
	}
	return (folderIter == NULL) ? NULL : folderIter->GetFolder();
#endif
}


bool VJSValue::GetJSONValue( VJSONValue& outValue, JS4D::ExceptionRef *outException) const
{
	bool ok = JS4D::ValueToVJSONValue( fContext, fValue, outValue, outException);
	return ok;
}

void VJSValue::GetObject( VJSObject& outObject, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE

#else
	xbox_assert( outObject.fContext.fContext == outObject.fContext.fContext );
	outObject.SetObjectRef( ((fValue != NULL) && JSValueIsObject( fContext, fValue)) ? JSValueToObject( fContext, fValue, outException) : NULL);
#endif
}


VJSObject VJSValue::GetObject( VJSException* outException ) const
{
	return GetObject( VJSException::GetExceptionRefIfNotNull(outException) );
}

VJSObject VJSValue::GetObject( JS4D::ExceptionRef *outException) const
{
	VJSObject resultObj(fContext);
#if USE_V8_ENGINE

#else
	if ((fValue != NULL) && JSValueIsObject( fContext, fValue))
		resultObj.SetObjectRef( JSValueToObject( fContext, fValue, outException));
	else
		//resultObj.SetObjectRef(JSValueToObject(fContext, JSValueMakeNull(fContext), outException));
		resultObj.MakeEmpty();
#endif
	return resultObj;
}


bool VJSValue::IsFunction() const
{
#if USE_V8_ENGINE
    return ( fPersValue->val_ ? false : fPersValue->val_->IsFunction() );
#else
	JSObjectRef obj = ( (fValue != NULL) && JSValueIsObject( fContext, fValue) ) ? JSValueToObject( fContext, fValue, NULL) : NULL;
	return (obj != NULL) ? JSObjectIsFunction( fContext, obj) : false;
#endif
}


//======================================================


VJSObject::VJSObject( const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes):fContext(inParent.fContext) // build an empty object as a property of a parent object
{
	MakeEmpty();
	inParent.SetProperty(inPropertyName, *this, inAttributes);
}


VJSObject::VJSObject( const VJSArray& inParent, sLONG inPos):fContext(inParent.GetContext()) // build an empty object as an element of an array (-1 means push at the end)
{
	MakeEmpty();
	if (inPos == -1)
		inParent.PushValue(*this);
	else
		inParent.SetValueAt(inPos, *this);
}

void VJSObject::SetNullProperty( const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNull();
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const XBOX::VValueSingle* inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	if (inValue == NULL || inValue->IsNull())
		jsval.SetNull();
	else
		jsval.SetVValue(*inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, sLONG inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, sLONG8 inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, Real inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const XBOX::VString& inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
	VJSValue jsval(fContext);
	jsval.SetString(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}



bool VJSObject::HasProperty( const VString& inPropertyName) const
{
	bool ok = false;
#if USE_V8_ENGINE

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	ok = JSObjectHasProperty( fContext, fObject, jsName);
	JSStringRelease( jsName);
#endif
	return ok;
}


void VJSObject::SetProperty( const VString& inPropertyName, const VJSValue& inValue, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inValue, inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSObject& inObject, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inObject, inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSArray& inArray, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inArray, inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease( jsName);
#endif
}


void VJSObject::SetProperty( const VString& inPropertyName, bool inBoolean, JS4D::PropertyAttributes inAttributes, VJSException* outException) const
{
#if USE_V8_ENGINE

#else
	JSStringRef jsName = JS4D::VStringToString(inPropertyName);
	JSObjectSetProperty(
		fContext, 
		fObject, 
		jsName, 
		JSValueMakeBoolean(fContext, inBoolean), inAttributes, VJSException::GetExceptionRefIfNotNull(outException));
	JSStringRelease(jsName);
#endif
}

VJSValue VJSObject::GetProperty( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	return VJSValue(fContext);
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSValueRef valueRef = JSObjectGetProperty( fContext, fObject, jsName, outException);
	JSStringRelease( jsName);
	return VJSValue( fContext, valueRef);
#endif
}


VJSObject VJSObject::GetPropertyAsObject( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE
	return VJSObject(fContext);
#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSValueRef value = JSObjectGetProperty( fContext, fObject, jsName, outException);
	JSObjectRef objectRef = ((value != NULL) && JSValueIsObject( fContext, value)) ? JSValueToObject( fContext, value, outException) : NULL;
	JSStringRelease( jsName);
	return VJSObject( fContext, objectRef);
#endif
}


bool VJSObject::GetPropertyAsBool(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	bool res = false;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetBool(&res, VJSException::GetExceptionRefIfNotNull((outException)));
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


sLONG VJSObject::GetPropertyAsLong(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	sLONG res = 0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetLong(&res, VJSException::GetExceptionRefIfNotNull(outException));
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


bool VJSObject::GetPropertyAsString(const VString& inPropertyName, VJSException* outException, VString& outResult) const
{
	bool ok = false;
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	outResult.Clear();
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetString(outResult, VJSException::GetExceptionRefIfNotNull(outException));
	}
	return ok;
}



Real VJSObject::GetPropertyAsReal(const VString& inPropertyName, VJSException* outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, VJSException::GetExceptionRefIfNotNull(outException)));
	Real res = 0.0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetReal(&res, VJSException::GetExceptionRefIfNotNull(outException));
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


void VJSObject::GetPropertyNames( std::vector<VString>& outNames) const
{
#if USE_V8_ENGINE

#else
	JSPropertyNameArrayRef array = JSObjectCopyPropertyNames( fContext, fObject);
	if (array != NULL)
	{
		size_t size = JSPropertyNameArrayGetCount( array);
		try
		{
			outNames.resize( size);
			size = 0;
			for( std::vector<VString>::iterator i = outNames.begin() ; i != outNames.end() ; ++i)
			{
				JS4D::StringToVString( JSPropertyNameArrayGetNameAtIndex( array, size++), *i);
			}
		}
		catch(...)
		{
			outNames.clear();
		}
		JSPropertyNameArrayRelease( array);
	}
	else
	{
		outNames.clear();
	}
#endif
}


bool VJSObject::DeleteProperty( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
	bool ok = false;
#if USE_V8_ENGINE

#else
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	ok = JSObjectDeleteProperty( fContext, fObject, jsName, outException);
	JSStringRelease( jsName);
#endif
	return ok;
}


bool VJSObject::CallFunction (
	const VJSObject &inFunctionObject, 
	const std::vector<VJSValue> *inValues, 
	VJSValue *outResult, 
	JS4D::ExceptionRef *outException)
{
	bool ok = true;
#if USE_V8_ENGINE

#else	
	JS4D::ExceptionRef exception = NULL;
	JSValueRef result = NULL;

	if (inFunctionObject.IsFunction())
	{
		size_t count = ( (inValues == NULL) || inValues->empty() ) ? 0 : inValues->size();
		JSValueRef *values = (count == 0) ? NULL : new JSValueRef[count];
		if (values != NULL)
		{
			size_t j = 0;
			for( std::vector<VJSValue>::const_iterator i = inValues->begin() ; i != inValues->end() ; ++i)
				values[j++] = i->GetValueRef();
		}
		else
		{
			ok = (count == 0);
		}

		result = JS4DObjectCallAsFunction( fContext, inFunctionObject, fObject, count, values, &exception);

		delete[] values;
	}
	else
	{
		ok = false;
	}

	if (exception != NULL)
		ok = false;
	
	if (outException != NULL)
		*outException = exception;
	
	if (outResult != NULL)
	{
		if (result == NULL)
			outResult->SetUndefined();
		else
			outResult->SetValueRef( result);
	}
#endif
	return ok;
}

bool VJSObject::CallAsConstructor (const std::vector<VJSValue> *inValues, VJSObject *outCreatedObject, JS4D::ExceptionRef *outException)
{
	xbox_assert(outCreatedObject != NULL);
#if USE_V8_ENGINE
	return false;
#else
	if (!JSObjectIsConstructor(GetContextRef(), GetObjectRef()))

		return false;

	uLONG		argumentCount	= inValues == NULL || inValues->empty() ? 0 : (uLONG) inValues->size();
	JSValueRef	*values			= argumentCount == 0 ? NULL : new JSValueRef[argumentCount];

	if (values != NULL) {

		std::vector<VJSValue>::const_iterator	i;
		uLONG									j;

		for (i = inValues->begin(), j = 0; i != inValues->end(); i++, j++)

			values[j] = i->GetValueRef();
		

	}

	JS4D::ExceptionRef	exception;
	JS4D::ObjectRef		createdObject;
	
	exception = NULL;
	createdObject = JSObjectCallAsConstructor(GetContextRef(), GetObjectRef(), argumentCount, values, &exception);
		
	if (values != NULL)

		delete[] values;

	if (outException != NULL)

		*outException = exception;

	outCreatedObject->SetContext(fContext);
	if (exception != NULL || createdObject == NULL) {
		
		outCreatedObject->SetUndefined();
		return false;

	} else {

		outCreatedObject->SetObjectRef(createdObject);
		return true;

	}
#endif
}

void VJSObject::MakeEmpty()
{
#if USE_V8_ENGINE

#else
	fObject = JSObjectMake( fContext, NULL, NULL);
#endif
}

void VJSObject::MakeCallback( JS4D::ObjectCallAsFunctionCallback inCallbackFunction)
{
#if USE_V8_ENGINE
	SetNull();
#else
	JS4D::ContextRef	contextRef;
	
	if ((contextRef = fContext) != NULL && inCallbackFunction != NULL) 

		// Use NULL for function name (anonymous).

		fObject = JSObjectMakeFunctionWithCallback( contextRef, NULL, inCallbackFunction);

	else

		SetNull();
#endif
}

void VJSObject::MakeConstructor( JS4D::ClassRef inClassRef, JS4D::ObjectCallAsConstructorCallback inConstructor)
{
#if USE_V8_ENGINE
	SetNull();
#else
	JS4D::ContextRef	contextRef;
		
	if ((contextRef = fContext) != NULL && inClassRef != NULL && inConstructor != NULL) 
		
		fObject = JSObjectMakeConstructor( contextRef, inClassRef, inConstructor);

	else

		SetNull();
#endif
}

XBOX::VJSObject VJSObject::GetPrototype (const XBOX::VJSContext &inContext) 
{
#if USE_V8_ENGINE

#else
	if (IsObject())

		return XBOX::VJSObject( inContext, JS4D::ValueToObject( inContext, JSObjectGetPrototype(inContext, fObject), NULL));

	else
#endif
		return XBOX::VJSObject( inContext, NULL);
}

//======================================================

VJSPropertyIterator::VJSPropertyIterator( const VJSObject& inObject)
: fObject( inObject)
, fIndex( 0)
{
#if USE_V8_ENGINE
	fNameArray = NULL;
	fCount = 0;
#else
	fNameArray = (fObject != NULL) ? JSObjectCopyPropertyNames( inObject.GetContextRef(), fObject) : NULL;
	fCount = (fNameArray != NULL) ? JSPropertyNameArrayGetCount( fNameArray) : 0;
#endif
}


VJSPropertyIterator::~VJSPropertyIterator()
{
#if USE_V8_ENGINE

#else
	if (fNameArray != NULL)
		JSPropertyNameArrayRelease( fNameArray);
#endif
}


bool VJSPropertyIterator::GetPropertyNameAsLong( sLONG *outValue) const
{
#if USE_V8_ENGINE

#else
	if (testAssert( fIndex < fCount))
		return JS4D::StringToLong( JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outValue);
	else
#endif
		return false;
}


void VJSPropertyIterator::GetPropertyName( VString& outName) const
{
#if USE_V8_ENGINE

#else
	if (testAssert( fIndex < fCount))
		JS4D::StringToVString( JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outName);
	else
#endif

		outName.Clear();
}


VJSObject VJSPropertyIterator::GetPropertyAsObject( VJSException *outException) const
{
#if USE_V8_ENGINE
	return VJSObject( fObject.GetContextRef() );
#else
	JS4D::ValueRef value = _GetProperty( VJSException::GetExceptionRefIfNotNull(outException) );
	return VJSObject(
		fObject.GetContext(),
		((value != NULL) && JSValueIsObject( fObject.GetContextRef(), value)) ?
		JSValueToObject( fObject.GetContextRef(), value, VJSException::GetExceptionRefIfNotNull(outException) ) : NULL);
#endif
}


JS4D::ValueRef VJSPropertyIterator::_GetProperty( JS4D::ExceptionRef *outException) const
{
	JS4D::ValueRef value = NULL;
#if USE_V8_ENGINE

#else
	if (testAssert( fIndex < fCount))
		value = JSObjectGetProperty( fObject.GetContextRef(), fObject, JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outException);
	else
		value = NULL;
#endif
	return value;
}


void VJSPropertyIterator::_SetProperty( JS4D::ValueRef inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
#if USE_V8_ENGINE

#else
	if (testAssert( fIndex < fCount))
		JSObjectSetProperty( fObject.GetContextRef(), fObject, JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), inValue, inAttributes, outException);
#endif
}


//======================================================

/*VJSArray::VJSArray( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException)
: fContext( inContext)
{
#if USE_V8_ENGINE
	fObject = NULL;
#else
	// build an array
	fObject = JSObjectMakeArray( fContext, 0, NULL, outException);
#endif
}*/

VJSArray::VJSArray( const VJSContext& inContext, VJSException* outException)
: fContext( inContext)
{
#if USE_V8_ENGINE
	fObject = NULL;
#else
	// build an array
	fObject = JSObjectMakeArray( fContext, 0, NULL, VJSException::GetExceptionRefIfNotNull(outException));
#endif
}

VJSArray::VJSArray( const VJSContext& inContext, const std::vector<VJSValue>& inValues, VJSException* outException)
: fContext( inContext)
{
#if USE_V8_ENGINE
	fObject = NULL;
#else	// build an array
	JS4D::ExceptionRef*  except = VJSException::GetExceptionRefIfNotNull(outException);
	if (inValues.empty())
	{
		#if NEW_WEBKIT
		fObject = JSObjectMakeArray( fContext, 0, NULL, except);
		#else
		fObject = NULL;
		#endif
	}
	else
	{
		JSValueRef *values = new JSValueRef[inValues.size()];
		if (values != NULL)
		{
			size_t j = 0;
			for( std::vector<VJSValue>::const_iterator i = inValues.begin() ; i != inValues.end() ; ++i)
				values[j++] = i->GetValueRef();
		}
		#if NEW_WEBKIT
		fObject = JSObjectMakeArray( fContext, inValues.size(), values, except);
		#else
		fObject = NULL;
		#endif
		delete[] values;
	}
#endif
}




VJSArray::VJSArray( const VJSObject& inArrayObject, bool inCheckInstanceOfArray)
: fContext( inArrayObject.GetContext())
{
	if (inCheckInstanceOfArray && !inArrayObject.IsInstanceOf( CVSTR( "Array")) )
		fObject = NULL;
	else
		fObject = inArrayObject.GetObjectRef();
}


VJSArray::VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray)
: fContext( inArrayValue.GetContext())
{
	if (inCheckInstanceOfArray && !inArrayValue.IsInstanceOf( CVSTR( "Array")) )
		fObject = NULL;
	else
	{
		VJSObject jobj(inArrayValue.GetObject());
		inArrayValue.GetObject(jobj);
		fObject = jobj.GetObjectRef();
	}
}



size_t VJSArray::GetLength() const
{
	size_t length = 0;
#if USE_V8_ENGINE

#else
	JSStringRef jsString = JSStringCreateWithUTF8CString( "length");
	JSValueRef result = JSObjectGetProperty( fContext, fObject, jsString, NULL);
	JSStringRelease( jsString);
	if (testAssert( result != NULL))
	{
		double r = JSValueToNumber( fContext, result, NULL);
		length = (size_t) r;
	}
	else
	{
		length = 0;
	}
#endif
	return length;
}


VJSValue VJSArray::GetValueAt( size_t inIndex, VJSException *outException) const
{
#if USE_V8_ENGINE
	return VJSValue( fContext );
#else
	return VJSValue(
		fContext,
		JSObjectGetPropertyAtIndex( fContext, fObject, inIndex, VJSException::GetExceptionRefIfNotNull(outException) ));
#endif
}


void VJSArray::SetValueAt( size_t inIndex, const VJSValue& inValue, VJSException *outException) const
{
#if USE_V8_ENGINE

#else
	JSObjectSetPropertyAtIndex( fContext, fObject, inIndex, inValue, VJSException::GetExceptionRefIfNotNull(outException) );
#endif
}


void VJSArray::PushValue( const VJSValue& inValue, VJSException *outException) const
{
	SetValueAt( GetLength(), inValue, outException);
}


void VJSArray::PushValue( const VValueSingle& inValue, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetVValue(inValue, outException);
	PushValue(val, outException);
}

template<class Type>
void VJSArray::PushNumber( Type inValue , VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetNumber(inValue, outException);
	PushValue(val, outException);
}


void VJSArray::PushString( const VString& inValue , VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetString(inValue, outException);
	PushValue(val, outException);
}


void VJSArray::PushFile( XBOX::VFile *inFile, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetFile( inFile, outException);
	PushValue(val, outException);
}


void VJSArray::PushFolder( XBOX::VFolder *inFolder, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetFolder( inFolder, outException);
	PushValue(val, outException);
}


void VJSArray::PushFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException) const
{
	VJSValue val(fContext);
	val.SetFilePathAsFileOrFolder( inPath, outException);
	PushValue(val, outException);
}


void VJSArray::PushValues( const std::vector<const XBOX::VValueSingle*> inValues, VJSException *outException) const
{
	for (std::vector<const XBOX::VValueSingle*>::const_iterator cur = inValues.begin(), end = inValues.end(); cur != end; cur++)
	{
		const VValueSingle* val = *cur;
		if (val == NULL)
		{
			VJSValue xval(fContext);
			xval.SetNull();
			PushValue(xval, outException);
		}
		else
			PushValue(*val, outException);
	}
}


void VJSArray::_Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue> *inValues, VJSException *outException) const
{
#if USE_V8_ENGINE

#else
	JS4D::ExceptionRef*		jsException = VJSException::GetExceptionRefIfNotNull(outException);
	JSStringRef jsString = JSStringCreateWithUTF8CString( "splice");
	JSValueRef splice = JSObjectGetProperty( fContext, fObject, jsString, jsException);
	JSObjectRef spliceFunction = JSValueToObject( fContext, splice, jsException);
    JSStringRelease( jsString);

	JSValueRef *values = new JSValueRef[2 + ((inValues != NULL) ? inValues->size() : 0)];
	if (values != NULL)
	{
		size_t j = 0;
		values[j++] = JSValueMakeNumber( fContext, inWhere);
		values[j++] = JSValueMakeNumber( fContext, inCount);
		if (inValues != NULL)
		{
			for( std::vector<VJSValue>::const_iterator i = inValues->begin() ; i != inValues->end() ; ++i)
				values[j++] = i->GetValueRef();
		}
	
		JS4DObjectCallAsFunction( fContext, spliceFunction, fObject, j, values, jsException);
	}
	delete[] values;
#endif
}


// -------------------------------------------------------------------


VJSPictureContainer::VJSPictureContainer(XBOX::VValueSingle* inPict,/* bool ownsPict,*/ const VJSContext& inContext) :
fContext(inContext)
{
	assert(inPict == NULL || inPict->GetValueKind() == VK_IMAGE);
	fPict = (VPicture*)inPict;
	fMetaInfoIsValid = false;
	//fOwnsPict = ownsPict;
	fMetaBag = NULL;
}


VJSPictureContainer::~VJSPictureContainer()
{
	if (/*fOwnsPict &&*/ fPict != NULL)
		delete fPict;
	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
	QuickReleaseRefCountable(fMetaBag);
}


void VJSPictureContainer::SetMetaInfo(JS4D::ValueRef inMetaInfo, const VJSContext& inContext) 
{
#if !VERSION_LINUX

	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
	JS4D::ProtectValue(inContext, inMetaInfo);
	fMetaInfo = inMetaInfo;
	fMetaInfoIsValid = true;
	fContext = inContext;
#else

	// Postponed Linux Implementation !
	vThrowError(VE_UNIMPLEMENTED);
	xbox_assert(false);

#endif
}


const XBOX::VValueBag* VJSPictureContainer::RetainMetaBag()
{
#if !VERSION_LINUX

	if (fMetaBag == NULL)
	{
		if (fPict != NULL)
		{
			const VPictureData* picdata = fPict->RetainNthPictData(1);
			if (picdata != NULL)
			{
				fMetaBag = picdata->RetainMetadatas();
				picdata->Release();
			}
		}
	}
	return RetainRefCountable(fMetaBag);

#else

	// Postponed Linux Implementation !
	vThrowError(VE_UNIMPLEMENTED);
	xbox_assert(false);

	return NULL;

#endif
}


void VJSPictureContainer::SetMetaBag(const XBOX::VValueBag* metaBag)
{
	QuickReleaseRefCountable(fMetaBag);
	fMetaBag = RetainRefCountable(metaBag);
}


// -------------------------------------------------------------------

VJSFunction::~VJSFunction()
{
	ClearParamsAndResult();
}


void VJSFunction::ClearParamsAndResult()
{
	for( std::vector<VJSValue>::const_iterator i = fParams.begin() ; i != fParams.end() ; ++i)
		i->Unprotect();
	fParams.clear();

	fResult.Unprotect();
	fResult.SetValueRef( NULL);
	
	JS4D::UnprotectValue( fContext, fException.GetValue());
	fException.SetValue(NULL);
}


void VJSFunction::AddParam( const VValueSingle* inVal)
{
	VJSValue val(fContext);
	if (inVal == NULL)
		val.SetNull();
	else
		val.SetVValue(*inVal);
	AddParam( val);
}


void VJSFunction::AddParam( const VJSValue& inVal)
{
	inVal.Protect();
	fParams.push_back( inVal);
}


void VJSFunction::AddParam( const VJSONValue& inVal)
{
	VJSValue val( fContext);
	val.SetJSONValue( inVal);
	AddParam( val);
}


void VJSFunction::AddParam( const VString& inVal)
{
	VJSValue val(fContext);
	val.SetString(inVal);
	AddParam( val);
}


void VJSFunction::AddParam(sLONG inVal)
{
	VJSValue val( fContext);
	val.SetNumber(inVal);
	AddParam( val);
}


void VJSFunction::AddUndefinedParam()
{
	VJSValue val( fContext);
	val.SetUndefined();
	AddParam( val);
}


void VJSFunction::AddNullParam()
{
	VJSValue val( fContext);
	val.SetNull();
	AddParam( val);
}


void VJSFunction::AddBoolParam( bool inVal)
{
	VJSValue val( fContext);
	val.SetBool( inVal);
	AddParam( val);
}


void VJSFunction::AddLong8Param( sLONG8 inVal)
{
	VJSValue val( fContext);
	val.SetNumber( inVal);
	AddParam( val);
}


bool VJSFunction::Call( bool inThrowIfUndefinedFunction)
{
	bool called = false;

	VJSObject functionObject( fFunctionObject);
	
	if (!functionObject.IsObject() && !fFunctionName.IsEmpty())
	{
		VJSValue result( fContext);
		fContext.EvaluateScript( fFunctionName, NULL, &result, NULL, NULL);
		functionObject = result.GetObject();
	}

	if (functionObject.IsFunction())
	{
		JS4D::ExceptionRef exception = NULL;

		VJSValue result( fContext);
		fContext.GetGlobalObject().CallFunction( functionObject, &fParams, &result, &exception);

		result.Protect();	// protect on stack before storing it in class field
		JS4D::ProtectValue( fContext, exception);

		fResult.Unprotect();	// unprotect previous result we may have if Call() is being called twice.
		JS4D::UnprotectValue( fContext, fException.GetValue());
		
		fResult = result;
		fException.SetValue(exception);
		called = exception == NULL;
	}
	else
	{
		if (fFunctionName.IsEmpty())
			vThrowError( VE_JVSC_UNDEFINED_FUNCTION);
		else
			vThrowError( VE_JVSC_UNDEFINED_FUNCTION_BY_NAME, fFunctionName);
	}

	return called;
}

bool VJSFunction::GetResultAsBool() const
{
	bool res = false;
	fResult.GetBool(&res);
	return res;
}

sLONG VJSFunction::GetResultAsLong() const
{
	sLONG res = 0;
	fResult.GetLong(&res);
	return res;
}


sLONG8 VJSFunction::GetResultAsLong8() const
{
	sLONG8 res = 0;
	fResult.GetLong8(&res);
	return res;
}


void VJSFunction::GetResultAsString(VString& outResult) const
{
	fResult.GetString(outResult);
}


bool VJSFunction::GetResultAsJSONValue( VJSONValue& outResult) const
{
	if (fException.GetValue() != NULL)
	{
		outResult.SetUndefined();
		return false;
	}
	return fResult.GetJSONValue( outResult, fException.GetPtr());
}


