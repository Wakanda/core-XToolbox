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
#include "VJSJSON.h"

USING_TOOLBOX_NAMESPACE


VJSJSON::VJSJSON( const XBOX::VJSContext& inContext, XBOX::VJSException *outException)
: fContext( inContext)
, fJSONObject( _GetJSON( inContext, VJSException::GetExceptionRefIfNotNull(outException)))
, fParseFunction( NULL)
, fStringifyFunction( NULL)
{
}


VJSValue VJSJSON::Parse( const VString& inJSON, XBOX::VJSException *outException)
{
#if USE_V8_ENGINE
	return VJSValue( fContext );
#else
	JSStringRef jsString = JS4D::VStringToString( inJSON);
	JS4D::ValueRef value = _Parse( jsString, VJSException::GetExceptionRefIfNotNull(outException) );
	JSStringRelease( jsString);
	return VJSValue( fContext, value);
#endif
}


VJSValue VJSJSON::Parse( const VString& inJSON, JS4D::ObjectCallAsFunctionCallback inReviverFunction, XBOX::VJSException *outException)
{
#if USE_V8_ENGINE
	return VJSValue( fContext );
#else
	JSObjectRef reviverFunction = JSObjectMakeFunctionWithCallback( fContext, NULL, inReviverFunction);
	JSStringRef jsString = JS4D::VStringToString( inJSON);
	JS4D::ValueRef value = _Parse( jsString, reviverFunction, VJSException::GetExceptionRefIfNotNull(outException) );
	JSStringRelease( jsString);
	return VJSValue( fContext, value);
#endif
}


JS4D::ValueRef VJSJSON::_Parse( JS4D::StringRef inJSON, JS4D::ExceptionRef *outException)
{
#if USE_V8_ENGINE
	return NULL;
#else
	JSObjectRef parseFunction = _GetParseFunction( outException);
	if ( (parseFunction == NULL) || (inJSON == NULL) )
		return NULL;

	JSValueRef arguments[] = { JSValueMakeString( fContext, inJSON)};
	return JS4DObjectCallAsFunction( fContext, parseFunction, fJSONObject, 1, arguments, outException);
#endif
}


JS4D::ValueRef VJSJSON::_Parse( JS4D::StringRef inJSON, JS4D::ObjectRef inReviverFunction, JS4D::ExceptionRef *outException)
{
	JS4D::ObjectRef parseFunction = _GetParseFunction( outException);
#if USE_V8_ENGINE
	return NULL;
#else
	if ( (parseFunction == NULL) || (inJSON == NULL) || (inReviverFunction == NULL) )
		return NULL;

	JSValueRef arguments[] = { JSValueMakeString( fContext, inJSON), inReviverFunction };
	return JS4DObjectCallAsFunction( fContext, parseFunction, fJSONObject, 2, arguments, outException);
#endif
}


void VJSJSON::Stringify( const VJSValue& inValue, VString& outJSON, XBOX::VJSException *outException)
{
	VJSValue xnull(inValue);
	xnull.SetNull();
	_Stringify( inValue, xnull, xnull, outJSON, VJSException::GetExceptionRefIfNotNull(outException));
}


void VJSJSON::StringifyWithSpaces( const VJSValue& inValue, sLONG inSpaces, VString& outJSON, JS4D::ExceptionRef *outException)
{
#if USE_V8_ENGINE

#else
	VJSValue xnull(inValue);
	xnull.SetNull();
	_Stringify( inValue, xnull, JSValueMakeNumber( fContext, inSpaces), outJSON, outException);
#endif
}


void VJSJSON::_Stringify( const XBOX::VJSValue& inValue, JS4D::ValueRef inReplacer, JS4D::ValueRef inSpace, XBOX::VString& outJSON, JS4D::ExceptionRef *outException)
{
	JS4D::ObjectRef stringifyFunction = _GetStringifyFunction( outException);
	if (stringifyFunction == NULL)
		outJSON.Clear();
	else
	{
#if USE_V8_ENGINE

#else
		JSValueRef arguments[] = { inValue, inReplacer, inSpace };

		JS4D::ExceptionRef except = NULL;
		JS4D::ExceptionRef* outExcept = outException;
		if (outExcept == NULL)
			outExcept = &except;
		JSValueRef result = JS4DObjectCallAsFunction( fContext, stringifyFunction, fJSONObject, 3, arguments, outExcept);

		if (result == NULL)
			result = *outExcept;

		JS4D::ValueToString( fContext, result, outJSON, NULL);
#endif
	}
}


/*
	static
*/
JS4D::ObjectRef VJSJSON::_GetJSON( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException)
{
#if USE_V8_ENGINE
	return NULL;
#else
	JSStringRef jsString = JSStringCreateWithUTF8CString( "JSON");
	JSValueRef jsonValue = JSObjectGetProperty( inContext, JSContextGetGlobalObject( inContext), jsString, outException);
	JSObjectRef jsonObject = ((jsonValue != NULL) && JSValueIsObject( inContext, jsonValue)) ? JSValueToObject( inContext, jsonValue, outException) : NULL;
    JSStringRelease( jsString);
	return jsonObject;
#endif
}


JS4D::ObjectRef VJSJSON::_GetParseFunction( JS4D::ExceptionRef *outException)
{
#if USE_V8_ENGINE
	return NULL;
#else
	if (fParseFunction == NULL)
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString( "parse");
		JSValueRef functionValue = JSObjectGetProperty( fContext, fJSONObject, jsString, outException);
		fParseFunction = ((functionValue != NULL) && JSValueIsObject( fContext, functionValue)) ? JSValueToObject( fContext, functionValue, outException) : NULL;
		JSStringRelease( jsString);
	}
	return fParseFunction;
#endif
}


JS4D::ObjectRef VJSJSON::_GetStringifyFunction( JS4D::ExceptionRef *outException)
{
#if USE_V8_ENGINE
	return NULL;
#else
	if (fStringifyFunction == NULL)
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString( "stringify");
		JSValueRef functionValue = JSObjectGetProperty( fContext, fJSONObject, jsString, outException);
		fStringifyFunction = ((functionValue != NULL) && JSValueIsObject( fContext, functionValue)) ? JSValueToObject( fContext, functionValue, outException) : NULL;
		JSStringRelease( jsString);
	}
	return fStringifyFunction;
#endif
}


