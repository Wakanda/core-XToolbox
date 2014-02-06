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
#ifndef __VJSValue__
#define __VJSValue__


#include "JS4D.h"
#include "VJSErrors.h"
#include "VJSContext.h"

BEGIN_TOOLBOX_NAMESPACE


//======================================================

/*
	Value holder class.
	
	It's a smart pointer.
	It's always using the same context.
*/

class XTOOLBOX_API VJSValue : public XBOX::VObject
{
public:

#if USE_V8_ENGINE
	explicit					VJSValue( const VJSContext& inContext);
	explicit					VJSValue( const VJSContext& inContext, const XBOX::VJSException& inException );
								VJSValue( const VJSValue& inOther);
			const VJSValue&		operator=( const VJSValue& inOther);
#else
	explicit					VJSValue( const VJSContext& inContext) : fContext( inContext), fValue( NULL)	{}
	explicit					VJSValue( const VJSContext& inContext, const XBOX::VJSException& inException ) : fContext(inContext), fValue((JS4D::ValueRef)inException.GetValue()) {}
								VJSValue( const VJSValue& inOther):fContext( inOther.fContext),fValue( inOther.fValue)	{}

			const VJSValue&		operator=( const VJSValue& inOther)	{
				/*xbox_assert( fContext == inOther.fContext);*/
				fValue = inOther.fValue;
				fContext = inOther.fContext;
				return *this;
			}
#endif

#if USE_V8_ENGINE
			bool				IsUndefined() const;
			bool				IsNull() const;
			bool				IsNumber() const;
			bool				IsBoolean() const;
			bool				IsString() const;
			bool				IsObject() const;
			bool				IsFunction() const;
	inline	bool				IsInstanceOf( const XBOX::VString& inConstructorName, VJSException* outException = NULL) const;
			bool				IsArray() const;
#else

            bool				IsUndefined() const     { return JS4D::ValueIsUndefined( fContext, fValue ); }
			bool				IsNull() const			{ return JS4D::ValueIsNull( fContext, fValue); }
			bool				IsNumber() const		{ return JS4D::ValueIsNumber( fContext, fValue); }
			bool				IsBoolean() const		{ return JS4D::ValueIsBoolean( fContext, fValue); }
			bool				IsString() const		{ return JS4D::ValueIsString( fContext, fValue); }
			bool				IsObject() const		{ return JS4D::ValueIsObject( fContext, fValue); }
			bool				IsFunction() const;
	inline	bool				IsInstanceOf( const XBOX::VString& inConstructorName, VJSException* outException = NULL) const		{
				return IsInstanceOf( inConstructorName, VJSException::GetExceptionRefIfNotNull(outException) );
			}
			bool				IsArray() const			{ return IsInstanceOf("Array", (JS4D::ExceptionRef *)NULL); }
#endif
 
			// Return type of value, note that eTYPE_OBJECT is very broad (an object can be a function, an Array, a Date, a String, etc.).

			JS4D::EType			GetType ()				{ return JS4D::GetValueType( fContext, fValue); }

			// cast value into some type.
			// If cast is not possible before the value type is not compatible, it doesn't throw an error or an exception but simply returns false or NULL.
	inline	bool				GetReal( Real *outValue) const { return GetReal( outValue, NULL); }
	inline	bool				GetLong( sLONG *outValue) const { return GetLong( outValue, NULL); }
	inline	bool				GetLong8( sLONG8 *outValue) const { return GetLong8( outValue, NULL); }
	inline	bool				GetULong( uLONG *outValue) const { return GetULong( outValue, NULL); }
	inline	bool				GetString( XBOX::VString& outString) const { return GetString( outString, NULL); }
	inline	bool				GetJSONValue( VJSONValue& outValue) const {
				return GetJSONValue( outValue, (JS4D::ExceptionRef *)NULL );
			}
	inline	bool				GetJSONValue( VJSONValue& outValue, VJSException* outException) const {
				return GetJSONValue( outValue, VJSException::GetExceptionRefIfNotNull(outException) );
			}

			// if value is a string, make it a url, else returns false
	inline	bool				GetURL( XBOX::VURL& outURL) const {
				return GetURL(outURL,NULL);
			}
	inline	bool				GetBool( bool *outValue) const {
				return GetBool(outValue,NULL);
			}
	inline	bool				GetTime( XBOX::VTime& outTime) const {
				return GetTime(outTime,NULL);
			}
	inline	bool				GetDuration( XBOX::VDuration& outDuration) const {
				return GetDuration(outDuration,NULL);
			}
	inline	XBOX::VFile*		GetFile() const {
				return GetFile(NULL);
			}
	inline	XBOX::VFolder*		GetFolder() const {
				return GetFolder(NULL);
			}
	inline	XBOX::VValueSingle*	CreateVValue(bool simpleDate = false) const	{
				return CreateVValue(NULL,simpleDate);
			}

			VJSObject			GetObject( VJSException* outException = NULL) const;


			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType*	GetObjectPrivateData( VJSException *outException = NULL) const
			{
				JS4D::ClassRef classRef = NATIVE_CLASS::Class();
				return static_cast<typename NATIVE_CLASS::PrivateDataType*>(
													JS4D::ValueIsObjectOfClass(
														fContext,
														fValue,
														classRef) ?	JS4D::GetObjectPrivate(
																			JS4D::ValueToObject(
																				fContext,
																				fValue,
																				VJSException::GetExceptionRefIfNotNull(outException) ) ) : NULL);
			}
			template<class Type>
#if USE_V8_ENGINE
			void				SetNumber( const Type& inValue, VJSException *outException = NULL)
				{ JS4D::NumberToValue(fContext,inValue,fPersValue); }
			void				SetString( const XBOX::VString& inString, VJSException *outException = NULL)
				{ JS4D::VStringToValue( fContext, inString,fPersValue); }
			void				SetBool( bool inValue, VJSException *outException = NULL)
				{ JS4D::BoolToValue( fContext, inValue, fPersValue); }
			void				SetTime( const XBOX::VTime& inTime, VJSException *outException = NULL)
				{ JS4D::VTimeToObject( fContext, inTime, fPersValue, VJSException::GetExceptionRefIfNotNull(outException) ); }
			void				SetDuration( const XBOX::VDuration& inDuration, VJSException *outException = NULL)
				{ JS4D::VDurationToValue( fContext, inDuration,fPersValue); }
			void				SetFile( XBOX::VFile *inFile, VJSException *outException = NULL)
				{ JS4D::VFileToObject( fContext, inFile, fPersValue, VJSException::GetExceptionRefIfNotNull(outException)); }

			void				SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException = NULL)
				{ JS4D::VFilePathToObjectAsFileOrFolder(
								fContext,
								inPath,
								fPersValue,
								VJSException::GetExceptionRefIfNotNull(outException)); }

#else
			void				SetNumber( const Type& inValue, VJSException *outException = NULL)
				{ fValue = JS4D::NumberToValue( fContext, inValue); }
			void				SetString( const XBOX::VString& inString, VJSException *outException = NULL)
				{ fValue = JS4D::VStringToValue( fContext, inString); }
			void				SetBool( bool inValue, VJSException *outException = NULL)	{
				fValue = JS4D::BoolToValue( fContext, inValue); 
			}
			void				SetTime( const XBOX::VTime& inTime, VJSException *outException = NULL)	{
				fValue = JS4D::VTimeToObject( fContext, inTime, VJSException::GetExceptionRefIfNotNull(outException) );
			}
			void				SetDuration( const XBOX::VDuration& inDuration, VJSException *outException = NULL)	{
				fValue = JS4D::VDurationToValue( fContext, inDuration);
			}
			void				SetFile( XBOX::VFile *inFile, VJSException *outException = NULL)	{
				fValue = JS4D::ObjectToValue(
							fContext,
							JS4D::VFileToObject( fContext, inFile, VJSException::GetExceptionRefIfNotNull(outException)));
			}
			void				SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException = NULL)	{
				fValue = JS4D::ObjectToValue(
							fContext,
							JS4D::VFilePathToObjectAsFileOrFolder(
								fContext,
								inPath,
								VJSException::GetExceptionRefIfNotNull(outException)));
			}
#endif

			void				SetVValue( const XBOX::VValueSingle& inValue, VJSException *outException = NULL) {
				fValue = JS4D::VValueToValue( fContext, inValue, VJSException::GetExceptionRefIfNotNull(outException) );
			}
			void				SetJSONValue( const VJSONValue& inValue, VJSException *outException = NULL)				{
				fValue = JS4D::VJSONValueToValue( fContext, inValue, VJSException::GetExceptionRefIfNotNull(outException));
			}

			void				SetFolder( XBOX::VFolder *inFolder, VJSException *outException = NULL)	{
				fValue = JS4D::ObjectToValue( 
							fContext,
							JS4D::VFolderToObject( fContext, inFolder, VJSException::GetExceptionRefIfNotNull(outException)));
			}

#if USE_V8_ENGINE
			void				SetUndefined( VJSException *outException = NULL)
				{ JS4D::MakeUndefined( fContext, fPersValue ); }
			void				SetNull( VJSException *outException = NULL)
				{ JS4D::MakeNull( fContext, fPersValue ); }
#else
			void				SetUndefined( VJSException *outException = NULL) {
				fValue = JS4D::MakeUndefined( fContext);
			}
			void				SetNull( VJSException *outException = NULL)	{
				fValue = JS4D::MakeNull( fContext);
			}
#endif

			/*
				Any value saved elsewhere than on the stack must be protected so that it doesn't get garbage collected.
				Warning: be careful to balance calls to Protect/Unprotect on same ValueRef.
			*/
			void				Protect() const																					{ JS4D::ProtectValue( fContext, fValue);}
			void				Unprotect() const																				{ JS4D::UnprotectValue( fContext, fValue);}
			
			VJSContext			GetContext() const																				{ return XBOX::VJSContext(fContext); }

#if  USE_V8_ENGINE
			~VJSValue();
#endif


protected:
								VJSValue();	// forbidden (always need a context)
#if USE_V8_ENGINE
	explicit					VJSValue( JS4D::ContextRef inContext);
	explicit					VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue);
	explicit					VJSValue( JS4D::ContextRef inContext, const XBOX::VJSException& inException );
#else
	explicit					VJSValue( JS4D::ContextRef inContext) : fContext( inContext), fValue( NULL)	{;}
	explicit					VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue) : fContext( inContext), fValue( inValue)	{;}
	explicit					VJSValue( JS4D::ContextRef inContext, const XBOX::VJSException& inException ) : fContext(inContext), fValue((JS4D::ValueRef)inException.GetValue()) {}
#endif

#if  USE_V8_ENGINE
								operator JS4D::ValueRef() const;

			JS4D::ContextRef	GetContextRef() const																			{ return fContext;}			
			
			JS4D::ValueRef		GetValueRef() const;
#else
								operator JS4D::ValueRef() const		{ return fValue;}
			JS4D::ContextRef	GetContextRef() const																			{ return fContext;}			
			JS4D::ValueRef		GetValueRef() const																				{ return fValue;}
#endif
			void				SetValueRef( JS4D::ValueRef inValue)															{ fValue = inValue;}

			bool				IsInstanceOf( const XBOX::VString& inConstructorName, JS4D::ExceptionRef *outException) const	{
				return JS4D::ValueIsInstanceOf( fContext, fValue, inConstructorName, outException);
			}

	inline	void				GetObject(VJSObject& outObject) const { GetObject(outObject,NULL); }
			void				GetObject( VJSObject& outObject, JS4D::ExceptionRef *outException) const;
			VJSObject			GetObject( JS4D::ExceptionRef *outException ) const;

			bool				GetReal( Real *outValue, JS4D::ExceptionRef *outException) const;
			bool				GetLong( sLONG *outValue, JS4D::ExceptionRef *outException) const;
			bool				GetLong8( sLONG8 *outValue, JS4D::ExceptionRef *outException) const;
			bool				GetULong( uLONG *outValue, JS4D::ExceptionRef *outException) const;
			bool				GetString( XBOX::VString& outString, JS4D::ExceptionRef *outException ) const;
			bool				GetJSONValue( VJSONValue& outValue, JS4D::ExceptionRef *outException) const;
			// if value is a string, make it a url, else returns false
			bool				GetURL( XBOX::VURL& outURL, JS4D::ExceptionRef *outException) const;
			bool				GetBool( bool *outValue, JS4D::ExceptionRef *outException) const;
			bool				GetTime( XBOX::VTime& outTime, JS4D::ExceptionRef *outException) const;
			bool				GetDuration( XBOX::VDuration& outDuration, JS4D::ExceptionRef *outException) const;
			XBOX::VFile*		GetFile( JS4D::ExceptionRef *outException) const;
			XBOX::VFolder*		GetFolder( JS4D::ExceptionRef *outException) const;
			XBOX::VValueSingle*	CreateVValue( JS4D::ExceptionRef *outException, bool simpleDate) const										{ return JS4D::ValueToVValue( fContext, fValue, outException, simpleDate); }


friend class VJSParms_withContext;
friend class VJSParms_callAsConstructor;
friend class VJSParms_callStaticFunction;
friend class VJSParms_callAsFunction;
friend class VJSParms_construct;
friend class VJSParms_withArguments;
friend class VJSParms_withException;
friend class VJSParms_withReturnValue;
friend class VJSParms_setProperty;
friend class VJSParms_getProperty;
friend class VJSObject;
friend class VJSPropertyIterator;
friend class VJSFunction;
friend class VJSArray;
friend class VJSJSON;
friend class VJSImage;
friend class VJSWebSocketEvent;
friend class VJSStructuredClone;
friend class VJSEventEmitter;
friend class VJSW3CFSEvent;
friend class VJSModuleState;
friend class VJSRequireClass;
friend class VJSTerminationEvent;
friend class VJSWorker;
friend class VJSContext;
friend class VJSNewListenerEvent;
friend class XMLHttpRequest;
friend class VJSXMLHttpRequest;
friend class VJSObject;


friend void JS4D::EvaluateScript( JS4D::ContextRef inContext, const XBOX::VString& inScript, XBOX::VJSValue& outValue, JS4D::ExceptionRef *outException);

private:
			JS4D::ContextRef	fContext;
#if  !USE_V8_ENGINE
			JS4D::ValueRef		fValue;
#else
			JS4D::ValueRef		fValue;
			v8::Persistent< v8::Value >*	fPersValue;
//			v8::Persistent< v8::Value , v8::NonCopyablePersistentTraits<v8::Value> >*	fValue;
/*
			void Reset();
			bool IsEmpty() const { return fValue == 0; }
			void Copy(const VJSValue& that);*/
#endif
};


//======================================================

/*
	Object holder class.
	
	It's a smart pointer.
	It's always using the same context.
*/

class VJSArray;
class VJSParms_callStaticFunction;

class XTOOLBOX_API VJSObject : public XBOX::VObject
{
public:
#if USE_V8_ENGINE
	bool fIsGlobalObject;
	bool	IsGlobalObject() const { return fIsGlobalObject || !fObject; }
#endif
	explicit					VJSObject( const VJSContext* inContext) : fContext((JS4D::ContextRef)NULL), fObject( NULL) {
									if (inContext) {
										fContext = VJSContext(*inContext);
									}
								}
	explicit					VJSObject( const VJSContext& inContext) : fContext( inContext), fObject( NULL)	{}

								VJSObject( const VJSObject& inOther):fContext( inOther.fContext), fObject( inOther.fObject)	{}
								VJSObject( const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone); // build an empty object as a property of a parent object
								VJSObject( const VJSArray& inParent, sLONG inPos); // build an empty object as an element of an array (-1 means push at the end)

								operator VJSValue() const				{ return VJSValue( fContext, fObject);}
			const VJSObject&	operator=( const VJSObject& inOther)	{ /*xbox_assert( fContext == inOther.fContext);*/ fObject = inOther.fObject; fContext = inOther.fContext; return *this; }

			bool				IsEmpty() const { return fObject == NULL; }
			bool				IsInstanceOf( const XBOX::VString& inConstructorName, VJSException *outException = NULL) const {
				return JS4D::ValueIsInstanceOf(
						fContext,
						fObject,
						inConstructorName,
						VJSException::GetExceptionRefIfNotNull(outException) );}
			bool				IsFunction() const							{ return (fObject != NULL) ? JS4D::ObjectIsFunction( fContext, fObject) : false; }	// sc 03/09/2009 crash if fObject is null
			bool				IsArray() const								{ return IsInstanceOf("Array"); }
			bool				IsObject() const							{ return fObject != NULL; }
			bool				IsOfClass (JS4D::ClassRef inClassRef) const	{ return JS4D::ValueIsObjectOfClass( fContext, fObject, inClassRef); }

			void				MakeEmpty();	
			void				MakeObject( JS4D::ClassRef inClassRef, void* inPrivateData = NULL)										{ fObject = JS4D::MakeObject( fContext, inClassRef, inPrivateData); }
			
	inline	void				MakeFilePathAsFileOrFolder( const VFilePath& inPath)	{
				MakeFilePathAsFileOrFolder(inPath,NULL);
			}
	
			bool				HasProperty( const XBOX::VString& inPropertyName) const;
	
	inline	VJSValue			GetProperty( const XBOX::VString& inPropertyName) const {
				return GetProperty(inPropertyName,NULL);
			}
	inline	VJSValue			GetProperty( const XBOX::VString& inPropertyName, VJSException& outException) const {
				return GetProperty(inPropertyName,outException.GetPtr());
			}
	inline	VJSObject			GetPropertyAsObject( const XBOX::VString& inPropertyName ) const {
				return GetPropertyAsObject( inPropertyName, NULL );
			}
	inline	VJSObject			GetPropertyAsObject( const XBOX::VString& inPropertyName, VJSException& outException) const {
				return GetPropertyAsObject( inPropertyName, outException.GetPtr() );
			}
			void				GetPropertyNames( std::vector<XBOX::VString>& outNames) const;

			bool				GetPropertyAsString(const VString& inPropertyName, VJSException* outException, VString& outResult ) const;
			sLONG				GetPropertyAsLong(const VString& inPropertyName, VJSException* outException, bool* outExists) const;
			Real				GetPropertyAsReal(const VString& inPropertyName, VJSException* outException, bool* outExists) const;
			bool				GetPropertyAsBool(const VString& inPropertyName, VJSException* outException, bool* outExists) const;


			void				SetNullProperty( const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;

			void				SetProperty( const XBOX::VString& inPropertyName, const VJSValue& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const VJSObject& inObject, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const VJSArray& inArray, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const XBOX::VValueSingle* inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, sLONG inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, sLONG8 inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, Real inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const XBOX::VString& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, bool inBoolean, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException* outException = NULL) const;

			bool				DeleteProperty( const XBOX::VString& inPropertyName, VJSException& outException ) const {
				return DeleteProperty( inPropertyName, outException.GetPtr() );
			}

			// call a member function on this object.
			// returns false if the function was not found.
			
			bool				CallFunction(	// WARNING: you need to protect the VJSValue arguments 
									const VJSObject& inFunctionObject, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult) {
				return CallFunction(inFunctionObject,inValues,outResult,NULL);
			}

			bool				CallFunction(	// WARNING: you need to protect the VJSValue arguments 
									const VJSObject& inFunctionObject, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									VJSException& outException) {
				return CallFunction(inFunctionObject,inValues,outResult,outException.GetPtr());
			}

			bool				CallMemberFunction(	
									const XBOX::VString& inFunctionName, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									VJSException& outException) {
				return CallMemberFunction(inFunctionName,inValues,outResult,outException.GetPtr());
			}
			bool				CallMemberFunction(	// WARNING: you need to protect the VJSValue arguments 
									const XBOX::VString& inFunctionName, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult)
			{
				return CallMemberFunction(inFunctionName, inValues, outResult, NULL);
			}


			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType*	GetPrivateData() const
			{
				JS4D::ClassRef classRef = NATIVE_CLASS::Class();
				return static_cast<typename NATIVE_CLASS::PrivateDataType*>( JS4D::ValueIsObjectOfClass( fContext, fObject, classRef) ? JS4D::GetObjectPrivate( fObject) : NULL);
			}

	inline	void				SetUndefined()		{ SetUndefined(NULL); }
	inline	void				SetNull ()			{ SetNull(NULL); }

			/*
				any value saved elsewhere than on the stack must be protected so that it doesn't get garbage collected.
				Warning: be careful to balance calls to Protect/Unprotect on same ValueRef.
			*/
			void				Protect() const												{ JS4D::ProtectValue( fContext, fObject);}
			void				Unprotect() const											{ JS4D::UnprotectValue( fContext, fObject);}

			void				SetObjectRef( const VJSObject& inObject)					{ fObject = inObject.fObject; }

			const VJSContext&	GetContext() const											{ return fContext;}

			void				SetContext(const VJSContext& context)
			{
				fContext = context;
			}
		
			// Make object a C++ callback. Use js_callback<> template to set inCallbackFunction (see VJSClass.h).

			void				MakeCallback( JS4D::ObjectCallAsFunctionCallback inCallbackFunction);

			// Make constructor object. Use js_constructor<> template to set inConstructor (see VJSClass.h).

			void				MakeConstructor( JS4D::ClassRef inClassRef, JS4D::ObjectCallAsConstructorCallback inConstructor);

			// Return prototype of object.

			XBOX::VJSObject		GetPrototype (const XBOX::VJSContext &inContext);
	
	static	XBOX::VJSObject		CreateFromPrivateRef( const XBOX::VJSContext &inContext, void* inData ) {
				return VJSObject( inContext, (JS4D::ObjectRef)inData );
			}

			//only for  VWebViewerComponentWebkit::GetJSFilePath
			void*				GetPrivateRef() {
				return (void*)fObject;
			}

protected:

// expose the following constructor as less as possible anf only for libJS classes
	explicit					VJSObject( JS4D::ContextRef inContext) : fContext(inContext), fObject( NULL)	{}
	explicit					VJSObject( JS4D::ContextRef inContext, JS4D::ObjectRef inObject) : fContext(inContext), fObject( inObject)	{}	
	explicit					VJSObject( const VJSContext& inContext, JS4D::ObjectRef inObject) : fContext( inContext), fObject( inObject)	{}

			//only available inside XTlibJS, GetPrivateRef is available for VWebViewerComponentWebkit::GetJSFilePath
			operator JS4D::ObjectRef() const		{ return fObject;}

			JS4D::ContextRef	GetContextRef() const										{ return fContext;}
			JS4D::ObjectRef		GetObjectRef() const										{ return fObject;}
			void				SetObjectRef( JS4D::ObjectRef inObject)						{ fObject = inObject;}
			VJSValue			GetProperty( const XBOX::VString& inPropertyName, JS4D::ExceptionRef *outException) const;
			VJSObject			GetPropertyAsObject( const XBOX::VString& inPropertyName, JS4D::ExceptionRef *outException) const;
			bool				DeleteProperty( const XBOX::VString& inPropertyName, JS4D::ExceptionRef *outException = NULL) const;

			bool				CallFunction(	// WARNING: you need to protect the VJSValue arguments 
									const VJSObject& inFunctionObject, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									JS4D::ExceptionRef *outException);

			bool				CallMemberFunction(	// WARNING: you need to protect the VJSValue arguments 
									const XBOX::VString& inFunctionName, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									JS4D::ExceptionRef *outException)
			{
				return CallFunction(GetPropertyAsObject(inFunctionName, outException), inValues, outResult, outException);
			}

			// Call object as a constructor, return true if successful. outCreatedObject must not be NULL.
			// WARNING: you need to protect the VJSValue arguments 
			bool				CallAsConstructor (const std::vector<VJSValue> *inValues, VJSObject *outCreatedObject, JS4D::ExceptionRef *outException);

			void				SetUndefined( JS4D::ExceptionRef *outException )		{
#if USE_V8_ENGINE
				testAssert(false);
				fObject = NULL;
#else				
				fObject = JS4D::ValueToObject( fContext, JS4D::MakeUndefined( fContext), outException);
#endif
			}

			void				SetNull (JS4D::ExceptionRef *outException )				{
#if USE_V8_ENGINE
				testAssert(false);
				fObject = NULL;
#else	
				fObject = JS4D::ValueToObject( fContext, JS4D::MakeNull(fContext), outException);
#endif
			}


			// because a NULL ObjectRef is invalid, MakeFile, MakeFolder and MakeFilePathAsFileOrFolder will do nothing if beging passed a NULL file or folder
			void				MakeFile( VFile *inFile, JS4D::ExceptionRef *outException = NULL)
			{
				if (testAssert( inFile != NULL))
				{
#if USE_V8_ENGINE
					JS4D::VFileToObject( fContext, inFile, fPersValue, outException);
#else
					fObject = JS4D::VFileToObject( fContext, inFile, outException);
#endif
				}
			}
			
			void				MakeFolder( VFolder *inFolder, JS4D::ExceptionRef *outException = NULL)
			{ 
				if (testAssert( inFolder != NULL))
				{
					fObject = JS4D::VFolderToObject( fContext, inFolder, outException);
				}
			}
			
			void				MakeFilePathAsFileOrFolder( const VFilePath& inPath, JS4D::ExceptionRef *outException)
			{ 
				if (testAssert( inPath.IsValid()))
				{
#if USE_V8_ENGINE
					JS4D::VFilePathToObjectAsFileOrFolder( fContext, inPath, fPersValue, outException);
#else
					fObject = JS4D::VFilePathToObjectAsFileOrFolder( fContext, inPath, outException);
#endif			
				}
			}


template<class DRVD, class PRV_DATA_TYPE> friend class VJSClass;

friend class VJSParms_withContext;
friend class VJSParms_callAsConstructor;
friend class VJSParms_callStaticFunction;
friend class VJSParms_callAsFunction;
friend class VJSValue;
friend class VJSParms_construct;
friend class VJSParms_withArguments;
friend class VJSParms_withException;
friend class VJSPropertyIterator;
friend class VJSArray;
friend class VJSWebSocketObject;
friend class VJSDirectoryEntryClass;
friend class VJSDirectoryEntrySyncClass;
friend class VJSFileSystemClass;
friend class VJSNetSocketClass;
friend class VJSNetClass;
friend class VJSNetServerSyncClass;
friend class VJSEventEmitter;
friend class VJSNetSocketSyncClass;
friend class VJSTimer;
friend class VJSStructuredClone;
friend class VJSSystemWorkerClass;
friend class VJSSystemWorkerEvent;
friend class VJSWebSocketEvent;
friend class VJSCallbackEvent;
friend class VJSW3CFSEvent;
friend class VJSMessagePort;
friend class VJSConnectionEvent;
friend class VJSFileIterator;
friend class VJSFileEntrySyncClass;
friend class VJSFileSystemSyncClass;
friend class VJSTextStream;
friend class VJSDirectoryReaderClass;
friend class VJSFileEntryClass;
friend class VJSDirectoryReaderSyncClass;
friend class VJSMetadataClass;
friend class VJSFolderIterator;
friend class VJSTimerEvent;
friend class VJSContext;
friend class XMLHttpRequest;
friend class VJSRequireClass;
friend class VJSTerminationEvent;
friend class VJSGlobalObject;
friend class VJSBufferClass;
friend class VJSArrayBufferClass;
friend class VJSFunction;
friend class VJSGlobalClass;
friend class VJSDOMErrorEventClass;


friend JS4D::ValueRef JS4D::VPictureToValue( JS4D::ContextRef inContext, const XBOX::VPicture& inPict);
friend JS4D::ValueRef JS4D::VBlobToValue( JS4D::ContextRef inContext, const XBOX::VBlob& inBlob, const XBOX::VString& inContentType);
friend bool JS4D::ConvertErrorContextToException( JS4D::ContextRef inContext, XBOX::VErrorContext *inErrorContext, JS4D::ExceptionRef *outException);

#if USE_V8_ENGINE
friend class V4DV8Context;
#endif

private:
								VJSObject();	// forbidden (always need a context)

			VJSContext			fContext;
			JS4D::ObjectRef		fObject;
#if  USE_V8_ENGINE
			v8::Persistent< v8::Value >*	fPersValue;
#endif
};


//======================================================


/*
	Object properties iterator.
	
	You must make sure properties count does not change while iterating.
	
	for( VJSPropertyIterator i( object) ; i.IsValid() ; ++i)
	{
		XBOX::VString name;
		i.GetPropertyName( name);
	}
	
*/
class XTOOLBOX_API VJSPropertyIterator : public XBOX::VObject
{
public:
											VJSPropertyIterator( const VJSObject& inObject);
											~VJSPropertyIterator();

			const VJSPropertyIterator&		operator++()					{ ++fIndex; return *this;}

			bool							IsValid() const					{ return fIndex < fCount;}
			
			VJSValue						GetProperty( VJSException *outException = NULL) const {
				return VJSValue(
					fObject.GetContext(),
					_GetProperty( VJSException::GetExceptionRefIfNotNull(outException) ));
			}
			void							SetProperty( const VJSValue& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException *outException = NULL) const {
				_SetProperty(
					inValue,
					inAttributes,
					VJSException::GetExceptionRefIfNotNull(outException));
			}

			VJSObject						GetPropertyAsObject( VJSException *outException = NULL) const;

			void							GetPropertyName( XBOX::VString& outName) const;
			bool							GetPropertyNameAsLong( sLONG *outValue) const;
			
private:
			VJSPropertyIterator( const VJSPropertyIterator&);	// forbidden
			const VJSPropertyIterator&		operator=( const VJSPropertyIterator&);	// forbidden

			JS4D::ValueRef					_GetProperty( JS4D::ExceptionRef *outException) const;
			void							_SetProperty( JS4D::ValueRef inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const;

			VJSObject						fObject;
			JS4D::PropertyNameArrayRef		fNameArray;
			size_t							fIndex;
			size_t							fCount;
};


//======================================================


class XTOOLBOX_API VJSArray : public XBOX::VObject
{
public:
								// create a new empty Array
			explicit			VJSArray( const VJSContext& inContext, VJSException* outException = NULL);
			
								// create a new array with a list of values
								// this is a lot more efficient than looping around PushValue
			explicit			VJSArray( const VJSContext& inContext, const std::vector<VJSValue>& inValues, VJSException* outException = NULL);
			
								// uses an object as an Array
								// if inCheckInstanceOfArray is true, VJSObject::IsInstanceOf is called to make sure it is actually an array.
			explicit			VJSArray( const VJSObject& inArrayObject, bool inCheckInstanceOfArray = true);
								VJSArray( const VJSArray& inOther): fContext( inOther.fContext), fObject( inOther.fObject)	{}

			explicit			VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray = true);

			explicit			VJSArray( const VJSContext& inContext, VJSException* outException, bool refIsNil) :fContext(inContext), fObject(NULL) {}
								operator JS4D::ObjectRef() const		{ return fObject;}
								operator VJSObject() const				{ return VJSObject( fContext, fObject);}
								operator VJSValue() const				{ return VJSValue( fContext, fObject);}
			
			// warning: indexes starts at zero just like in javascript
			size_t				GetLength() const;
			VJSValue			GetValueAt( size_t inIndex, VJSException *outException = NULL) const;
			void				SetValueAt( size_t inIndex, const VJSValue& inValue, VJSException *outException = NULL) const;
			
			void				PushValue( const VJSValue& inValue, VJSException *outException = NULL) const;
			void				PushValue( const VValueSingle& inValue, VJSException *outException = NULL) const;
			template<class Type>
			void				PushNumber( Type inValue , VJSException *outException = NULL) const;
			void				PushString( const VString& inValue , VJSException *outException = NULL) const;
			void				PushFile( XBOX::VFile *inFile, VJSException *outException = NULL) const;
			void				PushFolder( XBOX::VFolder *inFolder, VJSException *outException = NULL) const;
			void				PushFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException = NULL) const;
			void				PushValues( const std::vector<VJSValue>& inValues, VJSException *outException = NULL) const	{
				_Splice( GetLength(), 0, &inValues, outException);
			}
			void				PushValues( const std::vector<const XBOX::VValueSingle*> inValues, VJSException *outException = NULL) const;
			
			// remove inCount elements starting at element inWhere
			void				Remove( size_t inWhere, size_t inCount, VJSException *outException = NULL) const				{ _Splice( inWhere, inCount, NULL, outException); }
			
			// remove all elements
			void				Clear( VJSException *outException = NULL) const												{ _Splice( 0, GetLength(), NULL, outException); }

			// The splice() method remove inCount elements starting with element at inWhere and insert values at inWhere
			void				Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue>& inValues, VJSException *outException = NULL) const	{
				_Splice( inWhere, inCount, &inValues, outException); }

			void				SetObjectRef( JS4D::ObjectRef inObject)						{ fObject = inObject;}
			JS4D::ObjectRef		GetObjectRef() const										{ return fObject;}

			const VJSContext&	GetContext() const { return fContext; }	
							
private:
								VJSArray();	// forbidden (always need a context)
			void				_Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue> *inValues, VJSException *outException) const;

			VJSContext			fContext;
			JS4D::ObjectRef		fObject;
};


//======================================================

class XTOOLBOX_API VJSPictureContainer : public XBOX::VObject, public XBOX::IRefCountable
{
public:
	VJSPictureContainer(XBOX::VValueSingle* inPict, /*bool ownsPict,*/ const XBOX::VJSContext& inContext);

	/*
	virtual void SetModif()
	{
		// rien ici, est gere par DB4D pour les images qui proviennent de champs
	}
	*/

	bool MetaInfoInited() const
	{
		return fMetaInfoIsValid;
	}

	JS4D::ValueRef GetMetaInfo() const
	{
		return fMetaInfo;
	}

	void SetMetaInfo(JS4D::ValueRef inMetaInfo, const XBOX::VJSContext& inContext);

	XBOX::VPicture* GetPict()
	{
		return fPict;
	}

	const XBOX::VValueBag* RetainMetaBag();
	void SetMetaBag(const XBOX::VValueBag* metaBag);
	
protected:
	virtual ~VJSPictureContainer();

	XBOX::VPicture* fPict;
	JS4D::ValueRef fMetaInfo;
	XBOX::VJSContext		fContext;
	const XBOX::VValueBag* fMetaBag;
	bool fMetaInfoIsValid;
	//bool fOwnsPict;

};

//======================================================

/*
	facility class to call a function in context with parameters.
	
	the function name can actually be any valid JavaScript expression that evaluates to a function object.
*/
class XTOOLBOX_API VJSFunction : public XBOX::VObject
{
public:

									VJSFunction( const VJSObject& inFunctionObject, const VJSContext& inContext):fContext( inContext), fResult( inContext), fFunctionObject( inFunctionObject), fException( NULL)		{}
									VJSFunction( const VString& inFunctionName, const VJSContext&  inContext):fContext( inContext), fResult( inContext), fFunctionObject( inContext), fFunctionName( inFunctionName), fException( NULL)		{}

	virtual							~VJSFunction();

			void					AddParam( const VValueSingle* inVal);
			void					AddParam( const VJSValue& inVal);
			void					AddParam( const VJSONValue& inVal);
			void					AddParam( const VString& inVal);
			void					AddParam( sLONG inVal);
			void					AddUndefinedParam();
			void					AddNullParam();
			void					AddBoolParam( bool inVal);
			void					AddLong8Param( sLONG8 inVal);

			bool					Call( bool inThrowIfUndefinedFunction = false);

			bool					IsNullResult() const			{ return fResult.IsNull() || fResult.IsUndefined();}

			bool					GetResultAsBool() const;
			sLONG					GetResultAsLong() const;
			sLONG8					GetResultAsLong8() const;
			void					GetResultAsString( VString& outResult) const;
			bool					GetResultAsJSONValue( VJSONValue& outResult) const;
			const VJSValue&			GetResult() const				{ return fResult; }
			
			void					GetException(VJSException& ioException) const { 
				ioException = fException;
			}

			void					ClearParamsAndResult();

private:
			std::vector<VJSValue>	fParams;
			VString					fFunctionName;
			VJSObject				fFunctionObject;
			VJSValue				fResult;
			VJSContext				fContext;
			VJSException			fException;
};

END_TOOLBOX_NAMESPACE


#endif
