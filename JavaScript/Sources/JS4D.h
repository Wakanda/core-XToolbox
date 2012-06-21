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
#ifndef __JS4D__
#define __JS4D__


// private type, don't use.
#if VERSIONMAC
#ifndef BUILD_WEBVIEWER
#include <4DJavaScriptCore/JavaScriptCore.h>
#else
#include <JavaScriptCore/JavaScriptCore.h>
#endif
#else
#include <JavaScriptCore/JavaScript.h>
#endif

#include "Graphics/VGraphics.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSValue;
class VJSObject;

#if !VERSIONWIN
	#define __cdecl
#endif


#define nil NULL

class XTOOLBOX_API JS4D
{
public:
	typedef	JSClassRef					ClassRef;
	typedef	JSContextRef				ContextRef;
	typedef	JSGlobalContextRef			GlobalContextRef;
	typedef	JSValueRef					ValueRef;
	typedef	JSValueRef					ExceptionRef;
	typedef	JSObjectRef					ObjectRef;
	typedef	JSStringRef					StringRef;
	typedef unsigned					PropertyAttributes;
	typedef JSPropertyNameArrayRef		PropertyNameArrayRef;

	enum {
		PropertyAttributeNone         = kJSPropertyAttributeNone,
		PropertyAttributeReadOnly     = kJSPropertyAttributeReadOnly,
		PropertyAttributeDontEnum     = kJSPropertyAttributeDontEnum,
		PropertyAttributeDontDelete   = kJSPropertyAttributeDontDelete
	};

	enum { 
		ClassAttributeNone						= kJSClassAttributeNone,
		ClassAttributeNoAutomaticPrototype		= kJSClassAttributeNoAutomaticPrototype
	};

	static	bool					ValueToString( ContextRef inContext, ValueRef inValue, XBOX::VString& outString, ValueRef *outException = NULL);
	static	XBOX::VValueSingle*		ValueToVValue( ContextRef inContext, ValueRef inValue, ValueRef *outException = NULL);

	// produces a js null value if VValueSingle is null
	static	ValueRef				VValueToValue( ContextRef inContext, const XBOX::VValueSingle& inValue, ValueRef *outException);

	// produces a js null value if VFile or VFolder is NULL
	static	ObjectRef				VFileToObject( ContextRef inContext, XBOX::VFile *inFile, ValueRef *outException);
	static	ObjectRef				VFolderToObject( ContextRef inContext, XBOX::VFolder *inFolder, ValueRef *outException);
	static	ObjectRef				VFilePathToObjectAsFileOrFolder( ContextRef inContext, const XBOX::VFilePath& inPath, ValueRef *outException);
	
	// various wakanda apis accepts accepts a file path or an url. This method centralizes the detection algorithm.
	// returns true if the url is valid
	static	bool					GetURLFromPath( const VString& inPath, XBOX::VURL& outURL);
	
	static	StringRef				VStringToString( const XBOX::VString& inString);
	static	bool					StringToVString( StringRef inJSString, XBOX::VString& outString);
	
	// returns false if the string doesn't appear to be a valid positive or negative 32bits integer.
	static	bool					StringToLong( StringRef inJSString, sLONG *outValue);

	// produces a js null value if VString is null
	static	ValueRef				VStringToValue( ContextRef inContext, const XBOX::VString& inString);

#if !VERSION_LINUX  // Postponed Linux Implementation !
	static	ValueRef				VPictureToValue( ContextRef inContext, const XBOX::VPicture& inPict);
#endif

	static	ValueRef				VBlobToValue( ContextRef inContext, const VBlob& inBlob, const VString& inContentType = CVSTR( "application/octet-stream"));

	static	ValueRef				BoolToValue( ContextRef inContext, bool inValue);
	
	template<class Type>
	static	ValueRef				NumberToValue( ContextRef inContext, const Type& inValue)
	{
		return JSValueMakeNumber( inContext, static_cast<double>( inValue));
	}

	// produces a Date object
	// warning: returns NULL and not a js null value if the VTime is null
	static	ObjectRef				VTimeToObject( ContextRef inContext, const XBOX::VTime& inDate, ExceptionRef *outException);

	// fills a VTime from a js object that implements getTime() function.
	// in case of failure, VTime::IsNull() returns true.
	// it's caller responsibility to check inObject is really a Date using ValueIsInstanceOf
	static	bool					DateObjectToVTime( ContextRef inContext, ObjectRef inObject, XBOX::VTime& outTime, ExceptionRef *outException);
	
	// produces a number containing the duration in milliseconds
	// produces a js null value if VDuration is null
	static	ValueRef				VDurationToValue( ContextRef inContext, const XBOX::VDuration& inDuration);
	static	bool					ValueToVDuration( ContextRef inContext, ValueRef inValue, XBOX::VDuration& outDuration, ExceptionRef *outException);

	// evaluates some javascript text and returns the result in outValue.
	static	void					EvaluateScript( ContextRef inContext, const XBOX::VString& inScript, VJSValue& outValue, ExceptionRef *outException);

	// same result as instanceof javascript operator 
	static	bool					ValueIsInstanceOf( ContextRef inContext, ValueRef inValue, const XBOX::VString& inConstructorName, ExceptionRef *outException);

	// creates a function object from some js code as body
	static	ObjectRef				MakeFunction( ContextRef inContext, const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, ExceptionRef *outException);

	// call a function
	static	ValueRef				CallFunction (const ContextRef inContext, const ObjectRef inFunctionObject, const ObjectRef inThisObject, sLONG inNumberArguments, const ValueRef *inArguments, ValueRef *ioException);
	
	/*
		JSC Docmentation extract :
		"JavaScript values that are on the machine stack, in a register, 
		protected by JSValueProtect, set as the global object of an execution context, 
		or reachable from any such value will not be collected.

		During JavaScript execution, you are not required to call this function; the 
		JavaScript engine will garbage collect as needed. JavaScript values created
		within a context group are automatically destroyed when the last reference
		to the context group is released."

		To force garbage collection, call VJSContext::GarbageCollect
 		
	*/
	static	void					ProtectValue( ContextRef inContext, ValueRef inValue)	{ JSValueProtect( inContext, inValue);}
	static	void					UnprotectValue( ContextRef inContext, ValueRef inValue)	{ JSValueUnprotect( inContext, inValue);}

	// if being passed a non null javascript exception, throw an xbox VError that describe the exception.
	// returns true if a VError was thrown.
	static	bool					ThrowVErrorForException( ContextRef inContext, ExceptionRef inException);

	// If the current VTask error context is not empty, and if outException is not NULL, creates a javascript exception object and returns true.
	static	bool					ConvertErrorContextToException( ContextRef inContext, VErrorContext *inErrorContext, ExceptionRef *outException);

	/*
		Utility functions to ease the creation of VJSClass object using operator new in javascript.
		You should not use these functions directly but use instead VJSGlobalClass::AddConstructorObjectStaticValue.
	*/
	static	bool					RegisterConstructor( const char *inClassName, JSObjectCallAsConstructorCallback inConstructorCallBack);
	static	JSValueRef __cdecl		GetConstructorObject( JSContextRef inContext, JSObjectRef inObject, JSStringRef inPropertyName, JSValueRef* outException);
};

END_TOOLBOX_NAMESPACE

#endif
