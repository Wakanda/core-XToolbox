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

#include "VJSValue.h"
#include "VJSJSON.h"

USING_TOOLBOX_NAMESPACE


VJSJSON::VJSJSON( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException)
: fContext( inContext)
, fJSONObject( _GetJSON( inContext, outException))
, fParseFunction( NULL)
, fStringifyFunction( NULL)
{
}


VJSValue VJSJSON::Parse( const VString& inJSON, JS4D::ExceptionRef *outException)
{
	JSStringRef jsString = JS4D::VStringToString( inJSON);
	JS4D::ValueRef value = _Parse( jsString, outException);
	JSStringRelease( jsString);
	return VJSValue( fContext, value);
}


VJSValue VJSJSON::Parse( const VString& inJSON, JS4D::ObjectCallAsFunctionCallback inReviverFunction, JS4D::ExceptionRef *outException)
{
	JSObjectRef reviverFunction = JSObjectMakeFunctionWithCallback( fContext, NULL, inReviverFunction);
	JSStringRef jsString = JS4D::VStringToString( inJSON);
	JS4D::ValueRef value = _Parse( jsString, reviverFunction, outException);
	JSStringRelease( jsString);
	return VJSValue( fContext, value);
}


JS4D::ValueRef VJSJSON::_Parse( JS4D::StringRef inJSON, JS4D::ExceptionRef *outException)
{
	JSObjectRef parseFunction = _GetParseFunction( outException);
	if ( (parseFunction == NULL) || (inJSON == NULL) )
		return NULL;

	JSValueRef arguments[] = { JSValueMakeString( fContext, inJSON)};
	return JS4DObjectCallAsFunction( fContext, parseFunction, fJSONObject, 1, arguments, outException);
}


JS4D::ValueRef VJSJSON::_Parse( JS4D::StringRef inJSON, JS4D::ObjectRef inReviverFunction, JS4D::ExceptionRef *outException)
{
	JSObjectRef parseFunction = _GetParseFunction( outException);
	if ( (parseFunction == NULL) || (inJSON == NULL) || (inReviverFunction == NULL) )
		return NULL;

	JSValueRef arguments[] = { JSValueMakeString( fContext, inJSON), inReviverFunction };
	return JS4DObjectCallAsFunction( fContext, parseFunction, fJSONObject, 2, arguments, outException);
}


void VJSJSON::Stringify( const VJSValue& inValue, VString& outJSON, JS4D::ExceptionRef *outException)
{
	VJSValue xnull(inValue);
	xnull.SetNull();
	_Stringify( inValue, xnull, xnull, outJSON, outException);
}


void VJSJSON::StringifyWithSpaces( const VJSValue& inValue, sLONG inSpaces, VString& outJSON, JS4D::ExceptionRef *outException)
{
	VJSValue xnull(inValue);
	xnull.SetNull();
	_Stringify( inValue, xnull, JSValueMakeNumber( fContext, inSpaces), outJSON, outException);
}


void VJSJSON::_Stringify( JS4D::ValueRef inValue, JS4D::ValueRef inReplacer, JS4D::ValueRef inSpace, XBOX::VString& outJSON, JS4D::ExceptionRef *outException)
{
	JSObjectRef stringifyFunction = _GetStringifyFunction( outException);
	if (stringifyFunction == NULL)
		outJSON.Clear();
	else
	{
		JSValueRef arguments[] = { inValue, inReplacer, inSpace };

		JS4D::ExceptionRef except = NULL;
		JS4D::ExceptionRef* outExcept = outException;
		if (outExcept == NULL)
			outExcept = &except;
		JSValueRef result = JS4DObjectCallAsFunction( fContext, stringifyFunction, fJSONObject, 3, arguments, outExcept);

		if (result == NULL)
			result = *outExcept;

		JS4D::ValueToString( fContext, result, outJSON, NULL);
	}
}


/*
	static
*/
JS4D::ObjectRef VJSJSON::_GetJSON( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException)
{
    JSStringRef jsString = JSStringCreateWithUTF8CString( "JSON");
	JSValueRef jsonValue = JSObjectGetProperty( inContext, JSContextGetGlobalObject( inContext), jsString, outException);
	JSObjectRef jsonObject = ((jsonValue != NULL) && JSValueIsObject( inContext, jsonValue)) ? JSValueToObject( inContext, jsonValue, outException) : NULL;
    JSStringRelease( jsString);
	return jsonObject;
}


JS4D::ObjectRef VJSJSON::_GetParseFunction( JS4D::ExceptionRef *outException)
{
	if (fParseFunction == NULL)
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString( "parse");
		JSValueRef functionValue = JSObjectGetProperty( fContext, fJSONObject, jsString, outException);
		fParseFunction = ((functionValue != NULL) && JSValueIsObject( fContext, functionValue)) ? JSValueToObject( fContext, functionValue, outException) : NULL;
		JSStringRelease( jsString);
	}
	return fParseFunction;
}


JS4D::ObjectRef VJSJSON::_GetStringifyFunction( JS4D::ExceptionRef *outException)
{
	if (fStringifyFunction == NULL)
	{
		JSStringRef jsString = JSStringCreateWithUTF8CString( "stringify");
		JSValueRef functionValue = JSObjectGetProperty( fContext, fJSONObject, jsString, outException);
		fStringifyFunction = ((functionValue != NULL) && JSValueIsObject( fContext, functionValue)) ? JSValueToObject( fContext, functionValue, outException) : NULL;
		JSStringRelease( jsString);
	}
	return fStringifyFunction;
}


