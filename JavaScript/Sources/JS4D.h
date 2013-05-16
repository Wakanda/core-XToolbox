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


#include "Graphics/VGraphics.h"

// redeclare JavaScriptCore basic types

struct OpaqueJSContextGroup;
struct OpaqueJSContext;
struct OpaqueJSPropertyNameAccumulator;
struct OpaqueJSValue;

#if VERSIONMAC
namespace WK4D
{
#endif

	struct OpaqueJSClass;
	struct OpaqueJSContext;
	struct OpaqueJSValue;
	struct OpaqueJSString;
	struct OpaqueJSPropertyNameArray;
	struct OpaqueJSPropertyNameAccumulator;

#if VERSIONMAC
};
#endif

#if !VERSIONMAC
#define WK4D
#endif

BEGIN_TOOLBOX_NAMESPACE

class VJSValue;
class VJSObject;

#if !VERSIONWIN
	#define __cdecl
#endif


class XTOOLBOX_API JS4D
{
public:
	typedef	struct WK4D::OpaqueJSClass*						ClassRef;
	typedef	const struct WK4D::OpaqueJSContext*				ContextRef;
	typedef	struct WK4D::OpaqueJSContext*					GlobalContextRef;
	typedef	const struct WK4D::OpaqueJSValue*				ValueRef;
	typedef	const struct WK4D::OpaqueJSValue*				ExceptionRef;
	typedef	struct WK4D::OpaqueJSValue*						ObjectRef;
	typedef	struct WK4D::OpaqueJSString*					StringRef;
	typedef unsigned										ClassAttributes;
	typedef unsigned										PropertyAttributes;
	typedef struct WK4D::OpaqueJSPropertyNameArray*			PropertyNameArrayRef;
	typedef struct WK4D::OpaqueJSPropertyNameAccumulator*	PropertyNameAccumulatorRef;

	enum {
		PropertyAttributeNone         = 0,		// kJSPropertyAttributeNone,
		PropertyAttributeReadOnly     = 1<<1,	// kJSPropertyAttributeReadOnly,
		PropertyAttributeDontEnum     = 1<<2,	// kJSPropertyAttributeDontEnum,
		PropertyAttributeDontDelete   = 1<<3	// kJSPropertyAttributeDontDelete
	};

	enum { 
		ClassAttributeNone						= 0,	// kJSClassAttributeNone,
		ClassAttributeNoAutomaticPrototype		= 1>>1	// kJSClassAttributeNoAutomaticPrototype
	};


	// Types returned by GetType().
	enum EType {
	
		// A VJSValue with a NULL ValueRef is considered as undefined.

		eTYPE_UNDEFINED	= 0,	// kJSTypeUndefined, 

		// null is a special JavaScript value.

		eTYPE_NULL		= 1,	// kJSTypeNull,

		// Primitive types.

    	eTYPE_BOOLEAN	= 2,	// kJSTypeBoolean,
    	eTYPE_NUMBER	= 3,	// kJSTypeNumber,
		eTYPE_STRING	= 4,	// kJSTypeString,

		// An object can be a function, an Array, etc.

		eTYPE_OBJECT	= 5,	// kJSTypeObject 

	};

	typedef void (__cdecl *ObjectInitializeCallback) ( ContextRef ctx, ObjectRef object);
	typedef void (__cdecl *ObjectFinalizeCallback) ( ObjectRef object);
	typedef bool (__cdecl *ObjectHasPropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName);
	typedef ObjectRef (__cdecl *ObjectCallAsConstructorCallback) ( ContextRef ctx, ObjectRef constructor, size_t argumentCount, const ValueRef arguments[], ExceptionRef* exception);
	typedef ValueRef (__cdecl *ObjectGetPropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName, ExceptionRef* exception);
	typedef bool (__cdecl *ObjectSetPropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName, ValueRef value, ExceptionRef* exception);
	typedef bool (__cdecl *ObjectDeletePropertyCallback) ( ContextRef ctx, ObjectRef object, StringRef propertyName, ExceptionRef* exception);
	typedef void (__cdecl *ObjectGetPropertyNamesCallback) ( ContextRef ctx, ObjectRef object, PropertyNameAccumulatorRef propertyNames);
	typedef ValueRef (__cdecl *ObjectCallAsFunctionCallback) ( ContextRef ctx, ObjectRef function, ObjectRef thisObject, size_t argumentCount, const ValueRef arguments[], ExceptionRef* exception);
	typedef bool (__cdecl *ObjectHasInstanceCallback)  ( ContextRef ctx, ObjectRef constructor, ValueRef possibleInstance, ValueRef* exception);
	typedef ValueRef (__cdecl *ObjectConvertToTypeCallback) ( ContextRef ctx, ObjectRef object, EType type, ValueRef* exception);

	static	bool					ValueToString( ContextRef inContext, ValueRef inValue, XBOX::VString& outString, ExceptionRef *outException = NULL);
	static	XBOX::VValueSingle*		ValueToVValue( ContextRef inContext, ValueRef inValue, ExceptionRef *outException = NULL, bool simpleDate = false);

	// produces a js null value if VValueSingle is null
	static	ValueRef				VValueToValue( ContextRef inContext, const XBOX::VValueSingle& inValue, ExceptionRef *outException);
	
	// converts a JavaScriptCore value into a xbox VJSONValue.
	static	ValueRef				VJSONValueToValue( ContextRef inContext, const XBOX::VJSONValue& inValue, ExceptionRef *outException);
	static	bool					ValueToVJSONValue( ContextRef inContext, ValueRef inValue, VJSONValue& outJSONValue, ExceptionRef *outException);

	// produces a js null value if VFile or VFolder is NULL
	static	ObjectRef				VFileToObject( ContextRef inContext, XBOX::VFile *inFile, ValueRef *outException);
	static	ObjectRef				VFolderToObject( ContextRef inContext, XBOX::VFolder *inFolder, ValueRef *outException);
	static	ObjectRef				VFilePathToObjectAsFileOrFolder( ContextRef inContext, const XBOX::VFilePath& inPath, ExceptionRef *outException);
	
	// various wakanda apis accepts accepts a file path or an url. This method centralizes the detection algorithm.
	// returns true if the url is valid
	static	bool					GetURLFromPath( const VString& inPath, XBOX::VURL& outURL);
	
	static	StringRef				VStringToString( const XBOX::VString& inString);
	static	bool					StringToVString( StringRef inJSString, XBOX::VString& outString);
	
	// returns false if the string doesn't appear to be a valid positive or negative 32bits integer.
	static	bool					StringToLong( StringRef inJSString, sLONG *outValue);

	// produces a js null value if VString is null
	static	ValueRef				VStringToValue( ContextRef inContext, const XBOX::VString& inString);

	static	ValueRef				VPictureToValue( ContextRef inContext, const XBOX::VPicture& inPict);

	static	ValueRef				VBlobToValue( ContextRef inContext, const VBlob& inBlob, const VString& inContentType = CVSTR( "application/octet-stream"));

	static	ValueRef				BoolToValue( ContextRef inContext, bool inValue);
	
	static	ValueRef				DoubleToValue( ContextRef inContext, double inValue);
	
	static	sLONG8					GetDebugContextId( ContextRef inContext );


	template<class Type>
	static	ValueRef				NumberToValue( ContextRef inContext, const Type& inValue)	{ return DoubleToValue( inContext, static_cast<double>( inValue));}

	// produces a Date object
	// warning: returns NULL and not a js null value if the VTime is null
	static	ObjectRef				VTimeToObject( ContextRef inContext, const XBOX::VTime& inDate, ExceptionRef *outException);

	// fills a VTime from a js object that implements getTime() function.
	// in case of failure, VTime::IsNull() returns true.
	// it's caller responsibility to check inObject is really a Date using ValueIsInstanceOf
	static	bool					DateObjectToVTime( ContextRef inContext, ObjectRef inObject, XBOX::VTime& outTime, ExceptionRef *outException, bool simpleDate);
	
	// produces a number containing the duration in milliseconds
	// produces a js null value if VDuration is null
	static	ValueRef				VDurationToValue( ContextRef inContext, const XBOX::VDuration& inDuration);
	static	bool					ValueToVDuration( ContextRef inContext, ValueRef inValue, XBOX::VDuration& outDuration, ExceptionRef *outException);

	// evaluates some javascript text and returns the result in outValue.
	static	void					EvaluateScript( ContextRef inContext, const XBOX::VString& inScript, VJSValue& outValue, ExceptionRef *outException);

	// Tests whether a JavaScript value's type is the undefined type.
	static	bool					ValueIsUndefined( ContextRef inContext, ValueRef inValue);
	static	ValueRef				MakeUndefined( ContextRef inContext);

	// Tests whether a JavaScript value's type is the null type.
	static	bool					ValueIsNull( ContextRef inContext, ValueRef inValue);
	static	ValueRef				MakeNull( ContextRef inContext);

	// Tests whether a JavaScript value's type is the boolean type.
	static	bool					ValueIsBoolean( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the number type.
	static	bool					ValueIsNumber( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the string type.
	static	bool					ValueIsString( ContextRef inContext, ValueRef inValue);

	// Tests whether a JavaScript value's type is the object type.
	static	bool					ValueIsObject( ContextRef inContext, ValueRef inValue);
	static	ObjectRef				ValueToObject( ContextRef inContext, ValueRef inValue, ExceptionRef* outException);
	static	ObjectRef				MakeObject( ContextRef inContext, ClassRef inClassRef, void* inPrivateData);

	// Tests whether a JavaScript value is an object with a given class in its class chain.
	// true if value is an object and has jsClass in its class chain, otherwise false.
	static	bool					ValueIsObjectOfClass( ContextRef inContext, ValueRef inValue, ClassRef inClassRef);

	// same result as instanceof javascript operator 
	static	bool					ValueIsInstanceOf( ContextRef inContext, ValueRef inValue, const XBOX::VString& inConstructorName, ExceptionRef *outException);

	// Return type of value, note that eTYPE_OBJECT is very broad (an object can be a function, an Array, a Date, a String, etc.).
	static	EType					GetValueType( ContextRef inContext, ValueRef inValue);

	// creates a function object from some js code as body
	static	ObjectRef				MakeFunction( ContextRef inContext, const VString& inName, const VectorOfVString *inParamNames, const VString& inBody, const VURL *inUrl, sLONG inStartingLineNumber, ExceptionRef *outException);
	static	bool					ObjectIsFunction( ContextRef inContext, ObjectRef inObjectRef);

	// call a function
	static	ValueRef				CallFunction (const ContextRef inContext, const ObjectRef inFunctionObject, const ObjectRef inThisObject, sLONG inNumberArguments, const ValueRef *inArguments, ExceptionRef *ioException);
	
	/*
		JSC Docmentation extract :
		"JavaScript values that are on the machine stack, in a register, 
		protected by JSValueProtect, set as the global object of an execution context, 
		or reachable from any such value will not be collected.

		During JavaScript execution, you are not required to call this function; the 
		JavaScript engine will garbage collect as needed. JavaScript values created
		within a context group are automatically destroyed when the last reference
		to the context group is released."

		A value may be protected multiple times and must be unprotected an equal number of times before becoming eligible for garbage collection.

		To force garbage collection, call VJSContext::GarbageCollect
 		
	*/
	static	void					ProtectValue( ContextRef inContext, ValueRef inValue);
	static	void					UnprotectValue( ContextRef inContext, ValueRef inValue);

	// if being passed a non null javascript exception, throw an xbox VError that describe the exception.
	// returns true if a VError was thrown.
	static	bool					ThrowVErrorForException( ContextRef inContext, ExceptionRef inException);

	// If the current VTask error context is not empty, and if outException is not NULL, creates a javascript exception object and returns true.
	static	bool					ConvertErrorContextToException( ContextRef inContext, VErrorContext *inErrorContext, ExceptionRef *outException);

	/*
		Utility functions to ease the creation of VJSClass object using operator new in javascript.
		You should not use these functions directly but use instead VJSGlobalClass::AddConstructorObjectStaticValue.
	*/
	static	bool					RegisterConstructor( const char *inClassName, ObjectCallAsConstructorCallback inConstructorCallBack);
	static	ValueRef __cdecl		GetConstructorObject( ContextRef inContext, ObjectRef inObject, StringRef inPropertyName, ExceptionRef* outException);

	// Gets an object's private data.
	static	void*					GetObjectPrivate( ObjectRef inObject);

	/*
		Converts a JavaScript string into a null-terminated UTF8 string, 
		and copies the result into an external byte buffer.
		The destination byte buffer into which to copy a null-terminated 
		UTF8 representation of string. On return, buffer contains a UTF8 string 
		representation of string. If bufferSize is too small, buffer will contain only 
		partial results. If buffer is not at least bufferSize bytes in size, 
		behavior is undefined. 
		returns the number of bytes written into buffer (including the null-terminator byte).
	*/
	static	size_t					StringRefToUTF8CString( StringRef inStringRef, void *outBuffer, size_t inBufferSize);


	typedef struct {
		const char* name;
		ObjectGetPropertyCallback getProperty;
		ObjectSetPropertyCallback setProperty;
		PropertyAttributes attributes;
	} StaticValue;

	typedef struct {
		const char* name;
		ObjectCallAsFunctionCallback callAsFunction;
		PropertyAttributes attributes;
	} StaticFunction;

	typedef struct {
		ClassAttributes						attributes;

		const char*							className;
		ClassRef							parentClass;
			
		const StaticValue*					staticValues;
		const StaticFunction*				staticFunctions;
		
		ObjectInitializeCallback			initialize;
		ObjectFinalizeCallback				finalize;
		ObjectHasPropertyCallback			hasProperty;
		ObjectGetPropertyCallback			getProperty;
		ObjectSetPropertyCallback			setProperty;
		ObjectDeletePropertyCallback		deleteProperty;
		ObjectGetPropertyNamesCallback		getPropertyNames;
		ObjectCallAsFunctionCallback		callAsFunction;
		ObjectCallAsConstructorCallback		callAsConstructor;
		ObjectHasInstanceCallback			hasInstance;
	} ClassDefinition;

	static	ClassRef				ClassCreate( const ClassDefinition* inDefinition);
};

END_TOOLBOX_NAMESPACE

#endif
