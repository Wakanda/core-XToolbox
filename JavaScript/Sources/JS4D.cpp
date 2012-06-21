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

#include "VJSValue.h"
#include "VJSRuntime_Image.h"
#include "VJSRuntime_blob.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE


bool JS4D::ValueToString( ContextRef inContext, ValueRef inValue, VString& outString, ValueRef *outException)
{
	JSStringRef jsString = JSValueToStringCopy( inContext, inValue, outException);
	bool ok = StringToVString( jsString, outString);
	if (jsString != NULL)
		JSStringRelease( jsString);
	return ok;
}


JS4D::StringRef JS4D::VStringToString( const VString& inString)
{
	JSStringRef jsString = JSStringCreateWithCharacters( inString.GetCPointer(), inString.GetLength());
	return jsString;
}


bool JS4D::StringToVString( StringRef inJSString, VString& outString)
{
	bool ok;
	if (inJSString != NULL)
	{
		size_t length = JSStringGetLength( inJSString);
		UniChar *p = outString.GetCPointerForWrite( length);
		if (p != NULL)
		{
			::memcpy( p, JSStringGetCharactersPtr( inJSString), length * sizeof( UniChar));
			ok = outString.Validate( length);
		}
		else
		{
			outString.Clear();
			ok = false;
		}
	}
	else
	{
		ok = false;
		outString.Clear();
	}
	return ok;
}


bool JS4D::StringToLong( StringRef inJSString, sLONG *outResult)
{
	bool ok;
	if (inJSString != NULL)
	{
		const JSChar* p = JSStringGetCharactersPtr( inJSString);
		const JSChar* p_end = p + JSStringGetLength( inJSString);

		if (p != p_end)
		{
			// neg sign ?
			ok = true;
			bool neg;
			if (*p != CHAR_HYPHEN_MINUS)
			{
				neg = false;
			}
			else
			{
				neg = true;
				++p;
			}
			
			sLONG8 result = 0;
			
			while(p != p_end)
			{
				if (*p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
				{
					result *= 10;
					result += *p - CHAR_DIGIT_ZERO;
					++p;
				}
				else
				{
					ok = false;
					break;
				}
			}
			
			// overflow?
			if (result > MaxLongInt)
				ok = false;
			
			if (ok && neg)
				result = -result;
			
			if (ok && (outResult != NULL))
				*outResult = static_cast<sLONG>( result);
		}
		else
		{
			// empty string
			ok = false;
		}
	}
	return ok;
}


JS4D::ValueRef JS4D::VStringToValue( ContextRef inContext, const VString& inString)
{
	if (inString.IsNull())
		return JSValueMakeNull( inContext);

	JSStringRef jsString = JSStringCreateWithCharacters( inString.GetCPointer(), inString.GetLength());
	ValueRef value = JSValueMakeString( inContext, jsString);
	JSStringRelease( jsString);
	return value;
}


#if !VERSION_LINUX   // Postponed Linux Implementation !
JS4D::ValueRef JS4D::VPictureToValue( ContextRef inContext, const VPicture& inPict)
{
	if (inPict.IsNull())
		return JSValueMakeNull( inContext);
	VPicture* newPict = nil;
	if ((&inPict) != nil)
		newPict = new VPicture(inPict);
	VJSPictureContainer* xpic = new VJSPictureContainer(newPict, /*true,*/ inContext);
	ValueRef value = VJSImage::CreateInstance(inContext, xpic);
	xpic->Release();
	return value;
}
#endif


JS4D::ValueRef JS4D::VBlobToValue( ContextRef inContext, const VBlob& inBlob, const VString& inContentType)
{
	if (inBlob.IsNull())
		return JSValueMakeNull( inContext);

	VJSBlobValue_Slice* blobValue = VJSBlobValue_Slice::Create( inBlob, inContentType);
	if (blobValue == NULL)
		return JSValueMakeNull( inContext);
		
	ValueRef value = VJSBlob::CreateInstance( inContext, blobValue);
	ReleaseRefCountable( &blobValue);

	return value;
}


JS4D::ValueRef JS4D::BoolToValue( ContextRef inContext, bool inValue)
{
	return JSValueMakeBoolean( inContext, inValue);
}


JS4D::ObjectRef JS4D::VTimeToObject( ContextRef inContext, const VTime& inTime, ExceptionRef *outException)
{
	if (inTime.IsNull())
		return NULL;	// can't return JSValueMakeNull as an object
	
	sWORD year, month, day, hour, minute, second, millisecond;
	
	inTime.GetLocalTime( year, month, day, hour, minute, second, millisecond);

	JSValueRef	args[6];
	args[0] = JSValueMakeNumber( inContext, year);
	args[1] = JSValueMakeNumber( inContext, month-1);
	args[2] = JSValueMakeNumber( inContext, day);
	args[3] = JSValueMakeNumber( inContext, hour);
	args[4] = JSValueMakeNumber( inContext, minute);
	args[5] = JSValueMakeNumber( inContext, second);

	#if NEW_WEBKIT
	JSObjectRef date = JSObjectMakeDate( inContext, 6, args, outException);
	#else
    JSStringRef jsClassName = JSStringCreateWithUTF8CString("Date");
    JSObjectRef constructor = JSValueToObject( inContext, JSObjectGetProperty( inContext, JSContextGetGlobalObject( inContext), jsClassName, NULL), NULL);
    JSStringRelease( jsClassName);
    JSObjectRef date = JSObjectCallAsConstructor( inContext, constructor, 6, args, outException);
	#endif

	return date;
}


JS4D::ValueRef JS4D::VDurationToValue( ContextRef inContext, const VDuration& inDuration)
{
	if (inDuration.IsNull())
		return JSValueMakeNull( inContext);

	return JSValueMakeNumber( inContext, inDuration.ConvertToMilliseconds());
}


bool JS4D::ValueToVDuration( ContextRef inContext, ValueRef inValue, VDuration& outDuration, ExceptionRef *outException)
{
	bool ok;
	double r = JSValueToNumber( inContext, inValue, outException);
	sLONG8 n = (sLONG8) r;
	if (n == r)
	{
		outDuration.FromNbMilliseconds( n);
		ok = true;
	}
	else
	{
		outDuration.SetNull( true);
		ok = false;
	}
	return ok;
}


bool JS4D::DateObjectToVTime( ContextRef inContext, ObjectRef inObject, VTime& outTime, ExceptionRef *outException)
{
	// it's caller responsibility to check inObject is really a Date using ValueIsInstanceOf
	
	// call getTime()
	bool ok = false;
    JSStringRef jsString = JSStringCreateWithUTF8CString( "getTime");
	JSValueRef getTime = JSObjectGetProperty( inContext, inObject, jsString, outException);
	JSObjectRef getTimeFunction = JSValueToObject( inContext, getTime, outException);
    JSStringRelease( jsString);
	JSValueRef result = (getTime != NULL) ? JSObjectCallAsFunction( inContext, getTimeFunction, inObject, 0, NULL, outException) : NULL;
	if (result != NULL)
	{
		// The getTime() method returns the number of milliseconds since midnight of January 1, 1970.
		double r = JSValueToNumber( inContext, result, outException);
		sLONG8 n = (sLONG8) r;
		if (n == r)
		{
			outTime.FromUTCTime( 1970, 1, 1, 0, 0, 0, 0);
			outTime.AddMilliseconds( n);
			ok = true;
		}
		else
		{
			outTime.SetNull( true);
		}
	}
	else
	{
		outTime.SetNull( true);
	}
	return ok;
}


VValueSingle *JS4D::ValueToVValue( ContextRef inContext, ValueRef inValue, ValueRef *outException)
{
	if (inValue == NULL)
		return NULL;
	
	VValueSingle *value;
	JSType type = JSValueGetType( inContext, inValue);
	switch( type)
	{
		case kJSTypeUndefined:
		case kJSTypeNull:
			value = NULL;	//new VEmpty;
			break;
		
		case kJSTypeBoolean:
			value = new VBoolean( JSValueToBoolean( inContext, inValue));
			break;

		case kJSTypeNumber:
			value = new VReal( JSValueToNumber( inContext, inValue, outException));
			break;

		case kJSTypeString:
			{
				JSStringRef jsString = JSValueToStringCopy( inContext, inValue, outException);
				if (jsString != NULL)
				{
					VString *s = new VString;
					size_t length = JSStringGetLength( jsString);
					UniChar *p = (s != NULL) ? s->GetCPointerForWrite( length) : NULL;
					if (p != NULL)
					{
						::memcpy( p, JSStringGetCharactersPtr( jsString), length * sizeof( UniChar));
						s->Validate( length);
						value = s;
					}
					else
					{
						delete s;
						value = NULL;
					}
					JSStringRelease( jsString);
				}
				else
				{
					value = NULL;
				}
				break;
			}
		
		case kJSTypeObject:
			{
				if (ValueIsInstanceOf( inContext, inValue, CVSTR( "Date"), outException))
				{
					VTime *time = new VTime;
					if (time != NULL)
					{
						JSObjectRef dateObject = JSValueToObject( inContext, inValue, outException);
						DateObjectToVTime( inContext, dateObject, *time, outException);
						value = time;
					}
					else
					{
						value = NULL;
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSFolderIterator::Class()))
				{
					value = NULL;
					JS4DFolderIterator* container = static_cast<JS4DFolderIterator*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( container != NULL))
					{
						VFolder* folder = container->GetFolder();
						if (folder != NULL)
						{
							VString *s = new VString;
							if (s != NULL)
								folder->GetPath( *s, FPS_POSIX);
							value = s;
						}
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSFileIterator::Class()))
				{
					value = NULL;
					JS4DFileIterator* container = static_cast<JS4DFileIterator*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( container != NULL))
					{
						VFile* file = container->GetFile();
						if (file != NULL)
						{
							VString *s = new VString;
							if (s != NULL)
								file->GetPath( *s, FPS_POSIX);
							value = s;
						}
					}
				}
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSBlob::Class()))
				{
					// remember File inherits from Blob so one must check File before Blob
					VJSBlob::PrivateDataType* blobValue = static_cast<VJSBlob::PrivateDataType*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( blobValue != NULL))
					{
						VJSDataSlice *slice = blobValue->RetainDataSlice();
						VBlobWithPtr *blob = new VBlobWithPtr;
						if ( (blob != NULL) && (slice != NULL) && (slice->GetDataSize() > 0) )
						{
							if (blob->PutData( slice->GetDataPtr(), slice->GetDataSize()) != VE_OK)
							{
								delete blob;
								blob = NULL;
							}
						}
						value = blob;
						ReleaseRefCountable( &slice);
					}
					else
					{
						value = NULL;
					}
				}
#if !VERSION_LINUX  // Postponed Linux Implementation !
				else if (JSValueIsObjectOfClass( inContext, inValue, VJSImage::Class()))
				{
					VJSImage::PrivateDataType* piccontainer = static_cast<VJSImage::PrivateDataType*>(JSObjectGetPrivate( JSValueToObject( inContext, inValue, outException) ));
					if (testAssert( piccontainer != NULL))
					{
						VPicture *vpic = piccontainer->GetPict();
						value = (vpic != NULL) ? new VPicture(*vpic) : NULL;
					}
					else
					{
						value = NULL;
					}
				}
#endif
				else
				{
					value = NULL;
				}
				break;
			}
		
		default:
			value = NULL;
			break;
	}
	return value;
}


JS4D::ValueRef JS4D::VValueToValue( ContextRef inContext, const VValueSingle& inValue, ValueRef *outException)
{
	ValueRef jsValue;
	switch( inValue.GetValueKind())
	{
		case VK_BYTE:
		case VK_WORD:
		case VK_LONG:
		case VK_LONG8:
		case VK_REAL:
		case VK_FLOAT:
		case VK_SUBTABLE_KEY:
			jsValue = inValue.IsNull() ? JSValueMakeNull( inContext) : JSValueMakeNumber( inContext, inValue.GetReal());
			break;

		case VK_BOOLEAN:
			jsValue = inValue.IsNull() ? JSValueMakeNull( inContext) : JSValueMakeBoolean( inContext, inValue.GetBoolean() != 0);
			break;
			
		case VK_UUID:
			if (inValue.IsNull())
				jsValue = JSValueMakeNull( inContext) ;
			else
			{
				VString tempS;
				inValue.GetString(tempS);
				jsValue = VStringToValue( inContext, tempS);
			}
			break;
		
		case VK_TEXT:
		case VK_STRING:
#if VERSIONDEBUG
			jsValue = VStringToValue( inContext, dynamic_cast<const VString&>( inValue));
#else
			jsValue = VStringToValue( inContext, (const VString&)( inValue));
#endif
			break;
		
		case VK_TIME:
#if VERSIONDEBUG
			jsValue = inValue.IsNull() ? JSValueMakeNull( inContext) : VTimeToObject( inContext, dynamic_cast<const VTime&>( inValue), outException);
#else
			jsValue = inValue.IsNull() ? JSValueMakeNull( inContext) : VTimeToObject( inContext, (const VTime&)( inValue), outException);
#endif
			break;
		
		case VK_DURATION:
#if VERSIONDEBUG
			jsValue = VDurationToValue( inContext, dynamic_cast<const VDuration&>( inValue));
#else
			jsValue = VDurationToValue( inContext, (const VDuration&)( inValue));
#endif
			break;

		case VK_BLOB:
			jsValue = VBlobToValue( inContext, (const VBlob&)( inValue));
			break;

		case VK_IMAGE:
#if !VERSION_LINUX  // Postponed Linux Implementation !
			jsValue = VPictureToValue(inContext, (const VPicture&)( inValue));
#endif
			break;

		default:
			jsValue = JSValueMakeUndefined( inContext);	// exception instead ?
			break;
	}

	return jsValue;
}


bool JS4D::GetURLFromPath( const VString& inPath, XBOX::VURL& outURL)
{
	VIndex index = inPath.FindRawString( CVSTR( "://"));
	if (index > 0)//URL
	{
		outURL.FromString( inPath, true);
	}
	else if (!inPath.IsEmpty()) //system path
	{
		outURL.FromFilePath( inPath, eURL_POSIX_STYLE);
	}
	else
	{
		outURL.Clear();
	}

	return outURL.IsConformRFC();
}


JS4D::ObjectRef JS4D::VFileToObject( ContextRef inContext, XBOX::VFile *inFile, ValueRef *outException)
{
	JS4DFileIterator* file = (inFile == NULL) ? NULL : new JS4DFileIterator( inFile);
	ObjectRef value;
	if (file != NULL)
		value = JSObjectMake( inContext, VJSFileIterator::Class(), file);
	else
		value = JSValueToObject( inContext, JSValueMakeNull( inContext), outException);
	ReleaseRefCountable( &file);
	return value;
}


JS4D::ObjectRef JS4D::VFolderToObject( ContextRef inContext, XBOX::VFolder *inFolder, ValueRef *outException)
{
	JS4DFolderIterator* folder = (inFolder == NULL) ? NULL : new JS4DFolderIterator( inFolder);
	ObjectRef value;
	if (folder != NULL)
		value = JSObjectMake( inContext, VJSFolderIterator::Class(), folder);
	else
		value = JSValueToObject( inContext, JSValueMakeNull( inContext), outException);
	ReleaseRefCountable( &folder);
	return value;
}


JS4D::ObjectRef JS4D::VFilePathToObjectAsFileOrFolder( ContextRef inContext, const XBOX::VFilePath& inPath, ValueRef *outException)
{
	ObjectRef value;
	if (inPath.IsFolder())
	{
		VFolder *folder = new VFolder( inPath);
		value = VFolderToObject( inContext, folder, outException);
		ReleaseRefCountable( &folder);
	}
	else if (inPath.IsFile())
	{
		VFile *file = new VFile( inPath);
		value = VFileToObject( inContext, file, outException);
		ReleaseRefCountable( &file);
	}
	else
	{
		value = JSValueToObject( inContext, JSValueMakeNull( inContext), outException);	// exception instead ?
	}
	return value;
}


void JS4D::EvaluateScript( ContextRef inContext, const VString& inScript, VJSValue& outValue, ExceptionRef *outException)
{
	JSStringRef jsScript = JS4D::VStringToString( inScript);
	
	outValue.SetValueRef( (jsScript != NULL) ? JSEvaluateScript( inContext, jsScript, NULL /*thisObject*/, NULL /*jsUrl*/, 0 /*startingLineNumber*/, outException) : NULL);
	
	JSStringRelease( jsScript);
}


bool JS4D::ValueIsInstanceOf( ContextRef inContext, ValueRef inValue, const VString& inConstructorName, ExceptionRef *outException)
{
	JSStringRef jsString = JS4D::VStringToString( inConstructorName);
	JSObjectRef globalObject = JSContextGetGlobalObject( inContext);
	JSObjectRef constructor = JSValueToObject( inContext, JSObjectGetProperty( inContext, globalObject, jsString, outException), outException);
	xbox_assert( constructor != NULL);
	JSStringRelease( jsString);

	return (constructor != NULL) && (inValue != NULL) && JSValueIsInstanceOfConstructor( inContext, inValue, constructor, outException);
}


JS4D::ObjectRef JS4D::MakeFunction( ContextRef inContext, const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, ExceptionRef *outException)
{
	JSStringRef jsName = VStringToString( inName);
	JSStringRef jsBody = VStringToString( inBody);
	bool ok = (jsName != NULL) && (jsBody != NULL);

	JSStringRef jsUrl;
	VString url;
	if (inUrl != NULL)
	{
		inUrl->GetAbsoluteURL( url, true);
		jsUrl = VStringToString( url);
	}
	else
	{
		jsUrl = NULL;
	}


	size_t countNames = ( !ok || (inParamNames == NULL) || inParamNames->empty() ) ? 0 : inParamNames->size();
	JSStringRef *names = (countNames == 0) ? NULL : new JSStringRef[countNames];
	if (names != NULL)
	{
		size_t j = 0;
		for( VectorOfVString::const_iterator i = inParamNames->begin() ; i != inParamNames->end() ; ++i)
		{
			JSStringRef s = VStringToString( *i);
			if (s == NULL)
				ok = false;
			names[j++] = s;
			
		}
	}
	else
	{
		if (countNames != 0)
		{
			ok = false;
			countNames = 0;
		}
	}
	
	JSObjectRef functionObject = ok ? JSObjectMakeFunction( inContext, jsName, countNames, names, jsBody, jsUrl, inStartingLineNumber, outException) : NULL;

	for( JSStringRef *i = names ; i != names + countNames ; ++i)
	{
		if (*i != NULL)
			JSStringRelease( *i);
	}
	
	if (jsName != NULL)
		JSStringRelease( jsName);

	if (jsBody != NULL)
		JSStringRelease( jsBody);

	if (jsUrl != NULL)
		JSStringRelease( jsUrl);
	
	return functionObject;
}

JS4D::ValueRef JS4D::CallFunction (const ContextRef inContext, const ObjectRef inFunctionObject, const ObjectRef inThisObject, sLONG inNumberArguments, const ValueRef *inArguments, ValueRef *ioException)
{
	return JSObjectCallAsFunction(inContext, inFunctionObject, inThisObject, inNumberArguments, inArguments, ioException);
}


bool JS4D::ThrowVErrorForException( ContextRef inContext, ExceptionRef inException)
{
	bool errorThrown;
	if ( (inException != NULL) && !JSValueIsUndefined( inContext, inException))
	{
		VTask::GetCurrent()->GetDebugContext().DisableUI();

		bool bHasBeenAborted = false;

		// first throw an error for javascript
		VString description;
		if (ValueToString( inContext, inException, description, /*outException*/ NULL))
		{
			if ( description. BeginsWith ( CVSTR ( "SyntaxError" ) ) && JSValueIsObject ( inContext, inException ) ) // Just to be super safe at the moment to avoid breaking everything
			{
				JSObjectRef				jsExceptionObject = JSValueToObject ( inContext, inException, NULL );
				if ( jsExceptionObject != 0 )
				{
					JSStringRef			jsLine = JSStringCreateWithUTF8CString ( "line" );
					if ( JSObjectHasProperty ( inContext, jsExceptionObject, jsLine ) )
					{
						JSValueRef		jsValLine = JSObjectGetProperty ( inContext, jsExceptionObject, jsLine, NULL );
						if ( jsValLine != 0 )
						{
							double nLine = JSValueToNumber( inContext, jsValLine, NULL );
							VString		vstrLine ( "Error on line " );
							vstrLine. AppendReal ( nLine );
							vstrLine. AppendCString ( ". " );
							description. Insert ( vstrLine, 1 );
						}
					}
					JSStringRelease ( jsLine );
				}
			}
			else if ( description. Find ( CVSTR ( "Error: Execution aborted by caller." ) ) > 0 && JSValueIsObject ( inContext, inException ) )
				bHasBeenAborted = true;

			if ( !bHasBeenAborted )
			{
				StThrowError<>	error( MAKE_VERROR( 'JS4D', 1));
				error->SetString( "error_description", description);
			}
		}
		
		VTask::GetCurrent()->GetDebugContext().EnableUI();
		errorThrown = !bHasBeenAborted;
	}
	else
	{
		errorThrown = false;
	}
	return errorThrown;
}


bool JS4D::ConvertErrorContextToException( ContextRef inContext, VErrorContext *inErrorContext, ExceptionRef *outException)
{
	bool exceptionGenerated;
	if ( (inErrorContext != NULL) && !inErrorContext->IsEmpty() && (outException != NULL) )
	{
		exceptionGenerated = true;
		VErrorBase *error = inErrorContext->GetLast();
		
		if ( (error != NULL) && (*outException == NULL) )	// don't override existing exception
		{
			VString description;
			for( VErrorStack::const_reverse_iterator i = inErrorContext->GetErrorStack().rbegin() ; description.IsEmpty() && (i != inErrorContext->GetErrorStack().rend()) ; ++i)
			{
				(*i)->GetErrorDescription( description);
			}
		
			JSStringRef jsString = JS4D::VStringToString( description);
			JSValueRef argumentsErrorValues[] = { JSValueMakeString( inContext, jsString) };
			JSStringRelease( jsString);

			*outException = JSObjectMakeError( inContext, 1, argumentsErrorValues, NULL);
			if ( *outException != NULL )
			{
				VJSObject e(inContext, JSValueToObject(inContext, *outException, nil));
				VJSArray arr(inContext);

				for( VErrorStack::const_reverse_iterator i = inErrorContext->GetErrorStack().rbegin() ; (i != inErrorContext->GetErrorStack().rend()) ; ++i)
				{
					(*i)->GetErrorDescription( description);
					VJSValue jsdesc(inContext);
					jsdesc.SetString(description);
					arr.PushValue(jsdesc);
				}

				e.SetProperty("messages", arr, PropertyAttributeNone);
			}
		}
	}
	else
	{
		exceptionGenerated = false;
	}
	return exceptionGenerated;
}



/*================================================================================

	To create a VJSClass object in javascript using operator new,
	one need to create first a special constructor object added
	as property in the global object or another.
	
	Here we define the class "GenericConstructor" for these constructor objects
	as a convenience.

================================================================================*/

typedef unordered_map_VString<JSObjectCallAsConstructorCallback>	MapOfConstructorCallback;
static MapOfConstructorCallback		sMapOfConstructorCallback;


static JSObjectRef __cdecl GenericConstructor_callAsConstructor( JSContextRef inContext, JSObjectRef inConstructor, size_t inArgumentCount, const JSValueRef inArguments[], JSValueRef* outException)
{
	JSObjectCallAsConstructorCallback constructor = (JSObjectCallAsConstructorCallback) JSObjectGetPrivate( inConstructor);
	return (*constructor)( inContext, inConstructor, inArgumentCount, inArguments, outException);
}


static JSClassRef GetGenericConstructorClassRef()
{
	static JSClassRef sGenericConstructorClassRef = NULL;
	if (sGenericConstructorClassRef == NULL)
	{
		JSClassDefinition def = kJSClassDefinitionEmpty;
		def.className = "GenericConstructor";
		def.callAsConstructor = GenericConstructor_callAsConstructor;
		sGenericConstructorClassRef = JSClassCreate( &def);
	}
	return sGenericConstructorClassRef;
}


JSValueRef JS4D::GetConstructorObject( JSContextRef inContext, JSObjectRef inObject, JSStringRef inPropertyName, JSValueRef* outException)
{
	JSValueRef result = NULL;
	VString name;
	if (JS4D::StringToVString( inPropertyName, name))
	{
		MapOfConstructorCallback::const_iterator i = sMapOfConstructorCallback.find( name);
		if (testAssert( i != sMapOfConstructorCallback.end()))
		{
			result = JSObjectMake( inContext, GetGenericConstructorClassRef(), (void*) i->second);
		}
	}

	return result;
}


bool JS4D::RegisterConstructor( const char *inClassName, JSObjectCallAsConstructorCallback inConstructorCallBack)
{
	VString name;
	name.FromBlock( inClassName, strlen( inClassName), VTC_UTF_8);

	// already registered ?
	xbox_assert( sMapOfConstructorCallback.find( name) == sMapOfConstructorCallback.end());

	try
	{
		sMapOfConstructorCallback.insert( MapOfConstructorCallback::value_type( name, inConstructorCallBack));
	}
	catch(...)
	{
		return false;
	}
	return true;
}




// -----------------------------------------------------------------------------------------------------


