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


VJSParms_withContext::~VJSParms_withContext()
{
	GetContext().EndOfNativeCodeExecution();
}


void VJSParms_withContext::ProtectValue( JS4D::ValueRef inValue) const
{
#if USE_V8_ENGINE

#else
	JSValueProtect( GetContextRef(), inValue);
#endif
}


void VJSParms_withContext::UnprotectValue( JS4D::ValueRef inValue) const
{
#if USE_V8_ENGINE

#else
	JSValueUnprotect( GetContextRef(), inValue);
#endif
}



//======================================================


void VJSParms_withException::EvaluateScript( const VString& inScript, VValueSingle** outResult)
{
	VJSValue value( GetContextRef());
	JS4D::EvaluateScript( GetContextRef(), inScript, value, fException);
	if (outResult != NULL)
		*outResult = value.CreateVValue(fException,false);
}

//======================================================
#if USE_V8_ENGINE
VJSParms_withArguments::VJSParms_withArguments( const v8::FunctionCallbackInfo<v8::Value>& args )
		: fV8Arguments(&args)
		, fArgumentCount(args.Length())
		, VJSParms_withException(args.GetIsolate())
{
	;
}

#endif

VJSValue VJSParms_withArguments::GetParamValue( size_t inIndex) const
{
#if USE_V8_ENGINE
	HandleScope handle_scope((*fV8Arguments).GetIsolate());
//DebugMsg("VJSParms_withArguments::GetParamValue i=%d isol=%x\n",inIndex,(*fV8Arguments).GetIsolate());
	//Handle<Value> val = (*fV8Arguments)[inIndex-1];
//String::Utf8Value valStr((*fV8Arguments)[inIndex-1]l);
//DebugMsg("VJSParms_withArguments::GetParamValue '%s'\n",*valStr);

	xbox_assert( GetContextRef() == (*fV8Arguments).GetIsolate() );
	Handle<Value>	locVal = (*fV8Arguments)[inIndex-1];
// GH trouver pourquoi necessaire alor que transporter le handle suffit
String::Utf8Value  value2( locVal->ToString() );
//DebugMsg("VJSParms_withArguments::GetParamValue -> %s\n",*value2);
	VJSValue	result(GetContextRef());//,locVal);
	return result;
	//return VJSValue((*fV8Arguments).GetIsolate(),*((*fV8Arguments)[inIndex-1]));
#else
	if (inIndex > 0 && inIndex <= fArgumentCount)	// sc 05/11/2009 inIndex is 1 based
		return VJSValue( GetContextRef(), fArguments[inIndex-1]);
	else
		return VJSValue( GetContextRef(), NULL);
#endif
}


VValueSingle *VJSParms_withArguments::CreateParamVValue( size_t inIndex, bool simpleDate) const
{
#if USE_V8_ENGINE
	xbox_assert(false);//GH
#else
	if (inIndex >= 0 && inIndex <= fArgumentCount)
		return JS4D::ValueToVValue( GetContextRef(), fArguments[inIndex-1], fException, simpleDate);
#endif
	return NULL;
}


bool VJSParms_withArguments::GetParamObject( size_t inIndex, VJSObject& outObject) const
{
	bool okobj = false;
	
#if USE_V8_ENGINE

#else
	if (inIndex >= 0 && inIndex <= fArgumentCount && JSValueIsObject( GetContextRef(), fArguments[inIndex-1]) )
	{
		outObject.SetObjectRef( JSValueToObject( GetContextRef(), fArguments[inIndex-1], fException));
		okobj = true;
	}
	else
		outObject.SetObjectRef( NULL);
#endif	
	return okobj;
}


bool VJSParms_withArguments::GetParamJSONValue( size_t inIndex, VJSONValue& outValue) const
{
	bool ok;
	
	if (inIndex >= 0 && inIndex <= fArgumentCount)
	{
#if USE_V8_ENGINE
		xbox_assert(false);//GH
		ok = false;
#else
		ok = JS4D::ValueToVJSONValue( GetContextRef(), fArguments[inIndex-1], outValue, fException);
#endif
	}
	else
	{
		outValue.SetUndefined();
		ok = false;
	}
	
	return ok;
}


bool VJSParms_withArguments::GetParamFunc( size_t inIndex, VJSObject& outObject, bool allowString) const
{
	bool okfunc = false;

	VJSValue valfunc(GetParamValue(inIndex));
	if (!valfunc.IsUndefined() && valfunc.IsObject())
	{
		VJSObject objfunc(GetContextRef());
		valfunc.GetObject(objfunc);
		if (objfunc.IsFunction())
		{
			outObject = objfunc;
			okfunc = true;
		}
	}
	/*
	if (!okfunc && allowString)
	{
		if (IsStringParam(inIndex))
		{
		}
	}
	*/

	return okfunc;
}



bool VJSParms_withArguments::GetParamArray( size_t inIndex, VJSArray& outArray) const
{
	bool okarray = false;

	if (IsArrayParam( inIndex))
    {
#if USE_V8_ENGINE

#else
		outArray.SetObjectRef( JSValueToObject( GetContextRef(), fArguments[inIndex-1], fException));
#endif
		okarray=true;
    }
	else
		outArray.SetObjectRef( NULL);

    return okarray;
}


bool VJSParms_withArguments::IsNullParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return (inIndex >= 0 && inIndex <= fArgumentCount) ? JSValueIsNull( GetContextRef(), fArguments[inIndex-1]) : true;
#endif
}


bool VJSParms_withArguments::IsBooleanParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return (inIndex >= 0 && inIndex <= fArgumentCount) ? JSValueIsBoolean( GetContextRef(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsNumberParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return (inIndex >= 0 && inIndex <= fArgumentCount) ? JSValueIsNumber( GetContextRef(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsStringParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return (inIndex >= 0 && inIndex <= fArgumentCount) ? JSValueIsString( GetContextRef(), fArguments[inIndex-1]) : false;
#endif
}


bool VJSParms_withArguments::IsObjectParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return !IsNullParam(inIndex) && ((inIndex >= 0 && inIndex <= fArgumentCount) ? JSValueIsObject( GetContextRef(), fArguments[inIndex-1]) : false);
#endif
}


bool VJSParms_withArguments::IsFunctionParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return IsObjectParam( inIndex) && JSObjectIsFunction( GetContextRef(), JSValueToObject( GetContextRef(), fArguments[inIndex-1], NULL));
#endif
}


bool VJSParms_withArguments::IsArrayParam( size_t inIndex) const
{
#if USE_V8_ENGINE
	return false;
#else
	return (inIndex >= 0 && inIndex <= fArgumentCount) ? JS4D::ValueIsInstanceOf( GetContextRef(), fArguments[inIndex-1], CVSTR( "Array"), NULL) : false;
#endif
}


bool VJSParms_withArguments::GetStringParam( size_t inIndex, VString& outString) const
{
#if USE_V8_ENGINE
	DebugMsg("VJSParms_withArguments::GetStringParam for idx=%d\n",inIndex);
#endif
	VJSValue value( GetParamValue( inIndex));
	return value.GetString( outString, fException);
}


bool VJSParms_withArguments::GetLongParam( size_t inIndex, sLONG *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
	return value.GetLong( outValue, fException);
}


bool VJSParms_withArguments::GetLong8Param( size_t inIndex, sLONG8 *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
	return value.GetLong8( outValue, fException);
}


bool VJSParms_withArguments::GetULongParam( size_t inIndex, uLONG *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
	return value.GetULong( outValue, fException);
}


bool VJSParms_withArguments::GetRealParam( size_t inIndex, Real *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
	return value.GetReal( outValue, fException);
}


bool VJSParms_withArguments::GetBoolParam( size_t inIndex, bool *outValue) const
{
	VJSValue value( GetParamValue( inIndex));
	return value.GetBool( outValue, fException);
}


bool VJSParms_withArguments::GetBoolParam( size_t inIndex, const VString& inNameForTrue, const VString& inNameForFalse, size_t* outIndex) const
{
	bool result = false;
	
	if (CountParams() >= inIndex)
	{
		if (IsStringParam( inIndex))
		{
			if (outIndex != NULL)
				*outIndex = inIndex + 1;
			VString s;
			GetStringParam( inIndex, s);
			if (s == inNameForTrue)
				result = true;
		}
		else
		{
			if (IsBooleanParam( inIndex))
			{
				if (outIndex != NULL)
					*outIndex = inIndex + 1;
				GetBoolParam( inIndex, &result);
			}
		}
	}
	
	return result;
}


VFile* VJSParms_withArguments::RetainFileParam( size_t inIndex, bool allowPath) const
{
#if USE_V8_ENGINE
	DebugMsg("VJSParms_withArguments::GetStringParam for idx=%d\n",inIndex);
#endif
	VFile* result = NULL;
	
	if ( (inIndex >= 0) && (CountParams() >= inIndex) )
	{
#if USE_V8_ENGINE
	 	VJSValue paramValue(GetContextRef(),*((*fV8Arguments)[inIndex-1]));
		result = paramValue.GetFile( fException);
	xbox_assert(result);
	xbox_assert(false);
#else
		VJSValue paramValue( GetContextRef(), fArguments[inIndex-1]);
		result = paramValue.GetFile( fException);
#endif
		if (result != NULL)
		{
			result->Retain();
		}
		else
		{
			if (allowPath)
			{
				VURL url;
#if USE_V8_ENGINE
				xbox_assert(false);
#else
				if (paramValue.GetURL( url, fException))
				{
					VFilePath path;
					if (url.GetFilePath(path) && path.IsFile())
					{
						result = new VFile( path);
					}
					else
					{
						vThrowError( VE_FILE_BAD_NAME);
					}
				}
#endif
			}
				
		}
	}
	return result;
}



VFolder* VJSParms_withArguments::RetainFolderParam( size_t inIndex, bool allowPath) const
{
	VFolder* result = NULL;
	
	if (CountParams() >= inIndex)
	{
#if USE_V8_ENGINE
		xbox_assert(false);
#else
		VJSValue paramValue( GetContextRef(), fArguments[inIndex-1]);
		result = paramValue.GetFolder( fException);
#endif
		if (result != NULL)
		{
			result->Retain();
		}
		else
		{
			if (allowPath)
			{
				VURL url;
#if USE_V8_ENGINE
		xbox_assert(false);
#else
				if (paramValue.GetURL( url, fException))
				{
					VFilePath path;
					if (url.GetFilePath(path) && path.IsFolder())
					{
						result = new VFolder(path);
					}
					else
					{
						vThrowError( VE_FILE_BAD_NAME);
					}
				}
#endif			
			}
		}
	}
	return result;
}

//======================================================

/*
void VJSParms_finalize::UnprotectValue( JS4D::ValueRef inValue) const
{
	JSValueUnprotect( NULL, inValue);
}
*/


//======================================================


const UniChar *VJSParms_getProperty::GetPropertyNamePtr( VIndex *outLength) const
{
#if USE_V8_ENGINE
	return NULL;
#else
	if (outLength != NULL)
		*outLength = JSStringGetLength( fPropertyName);
	return JSStringGetCharactersPtr( fPropertyName);
#endif
}

//======================================================


void VJSParms_getPropertyNames::AddPropertyName(const VString& inPropertyName)
{
#if USE_V8_ENGINE

#else
	JSStringRef jsString = JS4D::VStringToString(inPropertyName);
	JSPropertyNameAccumulatorAddName( fPropertyNames, jsString);
	JSStringRelease( jsString);
#endif
}


void VJSParms_getPropertyNames::ReturnNumericalRange( size_t inBegin, size_t inEnd)
{
#if USE_V8_ENGINE

#else
	for( size_t i = inBegin ; i != inEnd ; ++i)
	{
		size_t index = i;
		size_t length = 0;
		JSChar	temp[20];
		JSChar* current = &temp[20];
		*--current = 0;
		while(index > 0)
		{
			*--current = (JSChar) (CHAR_DIGIT_ZERO + index%10L);
			index /= 10L;
			length++;
		}

		JSStringRef jsString = JSStringCreateWithCharacters( current, length);

		JSPropertyNameAccumulatorAddName( fPropertyNames, jsString);

		JSStringRelease( jsString);
	}
#endif
}
