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
#ifndef __VJSValue__H__
#define __VJSValue__H__


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
	explicit					VJSValue(const VJSContext& inContext);
	explicit					VJSValue(const VJSContext& inContext, const XBOX::VJSException& inException);
								VJSValue( const VJSValue& inOther);
								~VJSValue();
								
			VJSValue&			operator=( const VJSValue& inOther);
			// operators needed because VJSStructuredClone uses VJSValues as map indexes
			bool				operator==(const VJSValue& inOther ) const { return fValue == inOther.fValue; }
			bool				operator<(const VJSValue& inOther) const { return fValue < inOther.fValue; }
			bool				operator<=(const VJSValue& inOther) const { return fValue <= inOther.fValue; }
#else
	explicit					VJSValue( const VJSContext& inContext) : fContext( inContext), fValue( NULL)	{}
	explicit					VJSValue( const VJSContext& inContext, const XBOX::VJSException& inException ) : fContext(inContext), fValue((JS4D::ValueRef)inException.GetValue()) {}
								VJSValue( const VJSValue& inOther):fContext( inOther.fContext),fValue( inOther.fValue)	{}

			VJSValue&			operator=( const VJSValue& inOther ){ fValue = inOther.fValue; fContext=inOther.fContext;/*xbox_assert( fContext == inOther.fContext);*/return *this;}
			// operators needed because VJSStructuredClone uses VJSValues as map indexes
			bool				operator==(const VJSValue& inOther ) const { return fValue == inOther.fValue; }
			bool				operator<(const VJSValue& inOther) const { return fValue < inOther.fValue; }
			bool				operator<=(const VJSValue& inOther) const { return fValue <= inOther.fValue; }
#endif

			bool				IsUndefined() const;
			bool				IsNull() const;
			bool				IsNumber() const;
			bool				IsBoolean() const;
			bool				IsString() const;
			bool				IsObject() const;
			bool				IsFunction() const;
			bool				IsInstanceOf( const XBOX::VString& inConstructorName, VJSException* outException = NULL) const;
			bool				IsInstanceOfBoolean() const;
			bool				IsInstanceOfNumber() const;
			bool				IsInstanceOfString() const;
			bool				IsArray() const;
			bool				IsInstanceOfDate() const;
			bool				IsInstanceOfRegExp() const;
 
			// Return type of value, note that eTYPE_OBJECT is very broad (an object can be a function, an Array, a Date, a String, etc.).

			JS4D::EType			GetType ()				{ return JS4D::GetValueType( fContext, fValue); }

			// cast value into some type.
			// If cast is not possible before the value type is not compatible, it doesn't throw an error or an exception but simply returns false or NULL.
			bool				GetReal( Real *outValue) const { return GetReal( outValue, NULL); }
			bool				GetLong( sLONG *outValue) const { return GetLong( outValue, NULL); }
			bool				GetLong8( sLONG8 *outValue) const { return GetLong8( outValue, NULL); }
			bool				GetULong( uLONG *outValue) const { return GetULong( outValue, NULL); }
			bool				GetString( XBOX::VString& outString) const { return GetString( outString, NULL); }
			bool				GetJSONValue( VJSONValue& outValue) const;
			bool				GetJSONValue( VJSONValue& outValue, VJSException* outException) const;

			// if value is a string, make it a url, else returns false
			bool				GetURL( XBOX::VURL& outURL) const {return GetURL(outURL,NULL);}
			bool				GetBool( bool *outValue) const {return GetBool(outValue,NULL);}
			bool				GetTime( XBOX::VTime& outTime) const {return GetTime(outTime,NULL);}
			bool				GetDuration( XBOX::VDuration& outDuration) const {return GetDuration(outDuration,NULL);}
			XBOX::VFile*		GetFile() const {return GetFile(NULL);}
			XBOX::VFolder*		GetFolder() const {return GetFolder(NULL);}
			XBOX::VValueSingle*	CreateVValue(bool simpleDate = false) const	{return CreateVValue(NULL,simpleDate);}

			VJSObject			GetObject( VJSException* outException = NULL) const;

			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType*	GetObjectPrivateData(VJSException *outException = NULL) const;

#if USE_V8_ENGINE
			template<class Type>
			void				SetNumber( const Type& inValue, VJSException *outException = NULL){	JS4D::NumberToValue(fContext,inValue,*this); }
#else
			template<class Type>
			void				SetNumber(const Type& inValue, VJSException *outException = NULL) { fValue = JS4D::NumberToValue(fContext, inValue); }
#endif

			void				SetString( const XBOX::VString& inString, VJSException *outException = NULL);

			void				SetBool( bool inValue, VJSException *outException = NULL);

			void				SetTime( const XBOX::VTime& inTime, VJSException *outException = NULL, bool simpleDate = false);

			void				SetDuration( const XBOX::VDuration& inDuration, VJSException *outException = NULL);

			void				SetFile( XBOX::VFile *inFile, VJSException *outException = NULL);

			void				SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, VJSException *outException = NULL);

			void				SetVValue(const XBOX::VValueSingle& inValue, VJSException *outException = NULL, bool simpleDate = false);

			void				SetJSONValue( const VJSONValue& inValue, VJSException *outException = NULL);

			void				SetFolder( XBOX::VFolder *inFolder, VJSException *outException = NULL);


#if USE_V8_ENGINE
			void				SetUndefined( VJSException *outException = NULL);
			void				SetNull( VJSException *outException = NULL);
            // unfortunately should be temporarily made visible since static implementation js_internalCallAsConstructor needs it
            JS4D::ValueRef		GetValueRef() const;
#else
			void				SetUndefined( VJSException *outException = NULL);
			void				SetNull( VJSException *outException = NULL);
#endif

			/*
				Any value saved elsewhere than on the stack must be protected so that it doesn't get garbage collected.
				Warning: be careful to balance calls to Protect/Unprotect on same ValueRef.
			*/
			void				Protect() const			{ JS4D::ProtectValue( fContext, fValue);}
			void				Unprotect() const		{ JS4D::UnprotectValue( fContext, fValue);}
			
			VJSContext			GetContext() const		{ return XBOX::VJSContext(fContext); }


protected:
								VJSValue();	// forbidden (always need a context)
#if USE_V8_ENGINE
	explicit					VJSValue( JS4D::ContextRef inContext);
	explicit					VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue);
	explicit					VJSValue( JS4D::ContextRef inContext, JS4D::HandleValueRef inValue);

								operator JS4D::ValueRef() const;

			JS4D::ContextRef	GetContextRef() const	{ return fContext;}			
			
           void*				InternalGetObjectPrivateData( 	JS4D::ClassRef 	inClassRef,
                                                                VJSException*	outException=NULL,
                                                                JS4D::ValueRef	inValueRef = NULL ) const;
#else
	explicit					VJSValue( JS4D::ContextRef inContext) : fContext( inContext), fValue( NULL)	{;}
	explicit					VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue) : fContext( inContext), fValue( inValue)	{;}
	explicit					VJSValue( JS4D::ContextRef inContext, const XBOX::VJSException& inException ) : fContext(inContext), fValue((JS4D::ValueRef)inException.GetValue()) {}

								operator JS4D::ValueRef() const			{ return fValue;}
			JS4D::ContextRef	GetContextRef() const					{ return fContext;}			

#endif

			bool				IsInstanceOf( const XBOX::VString& inConstructorName, JS4D::ExceptionRef *outException) const;


			void				GetObject(VJSObject& outObject) const { GetObject(outObject,NULL); }
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
#if USE_V8_ENGINE
friend class V4DContext;
friend class VJSConstructor;
#else
friend class VJSParms_construct;
#endif
friend class VJSParms_withArguments;
friend class VJSParms_withException;
friend class VJSParms_withGetPropertyCallbackReturnValue;
friend class VJSParms_withSetPropertyCallbackReturnValue;
friend class VJSParms_setProperty;
friend class VJSParms_getProperty;
friend class VJSParms_hasProperty;
friend class VJSPropertyIterator;
friend class VJSFunction;
friend class VJSPictureContainer;
friend class VJSJSON;
friend class VJSContext;
friend class JS4D;

private:
friend class VJSObject;
friend class VJSArray;
friend class VJSParms_withReturnValue;
friend class VJSPersistentValue;

			JS4D::ContextRef	fContext;
			JS4D::ValueRef		fValue;

#if  !USE_V8_ENGINE
			JS4D::ValueRef		GetValueRef() const						{ return fValue;}
#else
			void				Dispose();
			void				CopyFrom(const VJSValue& inOther);
			void				CopyFrom(JS4D::ValueRef inOther);
			void				Set(JS4D::ValueRef inValue);
#endif
};

#if 0 //USE_V8_ENGINE
class XTOOLBOX_API VJSPersistentValue : public XBOX::VObject
{
public:

	explicit					VJSPersistentValue(const VJSContext& inContext);
				void			SetValue(const VJSValue& inValue);
				VJSValue		GetValue();
				void			SetNull();

private:
	JS4D::ContextRef	fContext;
	void*				fPersistentValue;
};
#endif

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
								~VJSObject();
#endif
	explicit					VJSObject( const VJSContext* inContext);

	explicit					VJSObject( const VJSContext& inContext);

								VJSObject( const VJSObject& inOther);
								VJSObject( const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone); // build an empty object as a property of a parent object
								VJSObject( const VJSArray& inParent, sLONG inPos); // build an empty object as an element of an array (-1 means push at the end)
#if USE_V8_ENGINE
			bool				HasSameRef(JS4D::ObjectRef inObjRef) const { return false; }// GH fPersValue == inObjRef; }
			bool				operator==(const VJSObject& inOther ) const { return false; }// GH fObject == inOther.fObject; }
#else
			bool				operator==(const VJSObject& inOther ) const { return fObject == inOther.fObject; }
			bool				HasSameRef(JS4D::ObjectRef inObjRef) const { return fObject == inObjRef; }
#endif

								operator VJSValue() const;
			VJSObject&			operator=( const VJSObject& inOther);

			// has a valid ObjectRef
			bool				HasRef() const;
			void				ClearRef();

			bool				IsInstanceOf( const XBOX::VString& inConstructorName, VJSException *outException = NULL) const;

			bool				IsFunction() const;
			bool				IsArray() const;
			bool				IsObject() const;
			bool				IsOfClass (JS4D::ClassRef inClassRef) const;

			// makes an empty JavaScriptObject {}
			void				MakeEmpty();

			void				MakeObject( JS4D::ClassRef inClassRef, void* inPrivateData = NULL);
			
			void				MakeFilePathAsFileOrFolder( const VFilePath& inPath) {MakeFilePathAsFileOrFolder(inPath,NULL);}

			void				MakeFile( VFile *inFile ) { MakeFile(inFile,NULL); }
			void				MakeFolder( VFolder *inFolder) { MakeFolder(inFolder,NULL); }

			bool				HasProperty( const XBOX::VString& inPropertyName) const;
	
			VJSValue			GetProperty( const XBOX::VString& inPropertyName) const {return GetProperty(inPropertyName,NULL);}

			VJSValue			GetProperty( const XBOX::VString& inPropertyName, VJSException& outException) const {return GetProperty(inPropertyName,outException.GetPtr());}
			VJSObject			GetPropertyAsObject( const XBOX::VString& inPropertyName ) const {return GetPropertyAsObject( inPropertyName, NULL );}
			VJSObject			GetPropertyAsObject( const XBOX::VString& inPropertyName, VJSException& outException) const {return GetPropertyAsObject( inPropertyName, outException.GetPtr() );}
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

			bool				DeleteProperty( const XBOX::VString& inPropertyName, VJSException& outException ) const {return DeleteProperty( inPropertyName, outException.GetPtr() );}

			// call a member function on this object.
			// returns false if the function was not found.
			
			bool				CallFunction(	// WARNING: you need to protect the VJSValue arguments 
												const VJSObject& inFunctionObject, 
												const std::vector<VJSValue> *inValues, 
												VJSValue *outResult);
			
#if 0 //USE_V8_ENGINE
			bool				CallFunction(	// WARNING: you need to protect the VJSValue arguments 
				const VJSObject& inFunctionObject,
				const std::vector<VJSValue> *inValues,
				VJSPersistentValue& outResult,
				VJSException& outException);
#endif

			bool				CallFunction(	// WARNING: you need to protect the VJSValue arguments 
												const VJSObject& inFunctionObject, 
												const std::vector<VJSValue> *inValues, 
												VJSValue *outResult, 
												VJSException& outException);

			bool				CallMemberFunction(	const XBOX::VString& inFunctionName, 
													const std::vector<VJSValue> *inValues, 
													VJSValue *outResult, 
													VJSException& outException);

			bool				CallMemberFunction(	// WARNING: you need to protect the VJSValue arguments 
													const XBOX::VString& inFunctionName, 
													const std::vector<VJSValue> *inValues, 
													VJSValue *outResult);

			// Call object as a constructor, return true if successful. outCreatedObject must not be NULL.
			// WARNING: you need to protect the VJSValue arguments 
			bool				CallAsConstructor (const std::vector<VJSValue> *inValues, VJSObject *outCreatedObject,VJSException* outException=NULL);


			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType*	GetPrivateData() const;
	
#if USE_V8_ENGINE
			void				SetSpecifics(JS4D::ClassDefinition& inClassDef,void* inData) const;
			void				SetPrivateData(void* inData) const {
				JS4D::SetObjectPrivate(fContext,fObject,(void *)inData);
			}
#endif

			/*
				any value saved elsewhere than on the stack must be protected so that it doesn't get garbage collected.
				Warning: be careful to balance calls to Protect/Unprotect on same ValueRef.
			*/
			void				Protect() const;
			void				Unprotect() const;

			const VJSContext&	GetContext() const							{ return fContext;}

			void				SetContext(const VJSContext& context)		{fContext = context;}
		
			// Make object a C++ callback. Use js_callback<> template to set inCallbackFunction (see VJSClass.h).

			void				MakeCallback( JS4D::ObjectCallAsFunctionCallback inCallbackFunction);

			// Return prototype of object.

			XBOX::VJSObject		GetPrototype (const XBOX::VJSContext &inContext);
	
	static	XBOX::VJSObject		CreateFromPrivateRef( const XBOX::VJSContext &inContext, void* inData ) {return VJSObject( inContext, (JS4D::ObjectRef)inData );}

			//only for  VWebViewerComponentWebkit::GetJSFilePath
			void*				GetPrivateRef();

protected:

// expose the following constructor as less as possible anf only for libJS classes
	explicit					VJSObject( JS4D::ContextRef inContext);
	explicit					VJSObject( JS4D::ContextRef inContext, JS4D::ObjectRef inObject);	
	explicit					VJSObject( const VJSContext& inContext, JS4D::ObjectRef inObject);
#if USE_V8_ENGINE
	explicit					VJSObject(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, bool inPersistent);
#endif
			//only available inside XTlibJS, GetPrivateRef is available for VWebViewerComponentWebkit::GetJSFilePath

			JS4D::ContextRef	GetContextRef() const										{ return fContext;}

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
									JS4D::ExceptionRef *outException);

			// because a NULL ObjectRef is invalid, MakeFile, MakeFolder and MakeFilePathAsFileOrFolder will do nothing if beging passed a NULL file or folder
			void				MakeFile( VFile *inFile, JS4D::ExceptionRef *outException );
			
			void				MakeFolder( VFolder *inFolder, JS4D::ExceptionRef *outException );
	
			void				MakeFilePathAsFileOrFolder( const VFilePath& inPath, JS4D::ExceptionRef *outException);

#if USE_V8_ENGINE
			void				SetInternalData(void* inData);
			void*				GetInternalData();
#endif
friend class VJSParms_construct;
friend class VJSParms_withContext;
friend class VJSParms_callStaticFunction;
friend class VJSParms_callAsFunction;
friend class VJSParms_withArguments;
friend class VJSParms_withException;
friend class VJSParms_withReturnValue;
friend class VJSPropertyIterator;
friend class VJSArray;
friend class VJSContext;
friend class VJSGlobalObject;
friend class VJSFunction;
friend class VJSGlobalClass;
friend class JS4D;
friend class VJSClassImpl;
friend class VJSConstructor;
friend class VJSGlobalContext;
#if USE_V8_ENGINE
friend class V4DContext;
#else
template<class DRVD, class PRV_DATA_TYPE> friend class VJSClass;
#endif

private:

friend class VJSValue;
friend class VJSParms_callAsConstructor;
#if USE_V8_ENGINE
#else
friend class VJSParms_construct;
#endif
friend class VJSJSON;

#if USE_V8_ENGINE
template<class DRVD, class PRV_DATA_TYPE> friend class VJSClass;
			void				Dispose();
			void				CopyFrom(const VJSObject& inOther);
			void				CopyFrom(JS4D::ValueRef inOther);
			void				Set(JS4D::ObjectRef inValue);

#endif
								VJSObject();	// forbidden (always need a context)
			void				SetObjectRef( JS4D::ObjectRef inObject);
			JS4D::ObjectRef		GetObjectRef() const;

			VJSContext			fContext;

			JS4D::ObjectRef		fObject;

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
			
			VJSValue						GetProperty(VJSException *outException = NULL) const;
			void							SetProperty( const VJSValue& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, VJSException *outException = NULL) const;

			VJSObject						GetPropertyAsObject( VJSException *outException = NULL) const;

			void							GetPropertyName( XBOX::VString& outName) const;
			bool							GetPropertyNameAsLong( sLONG *outValue) const;
			
private:
			VJSPropertyIterator( const VJSPropertyIterator&);	// forbidden
			const VJSPropertyIterator&		operator=( const VJSPropertyIterator&);	// forbidden

			VJSValue						_GetProperty(JS4D::ExceptionRef *outException) const;
			void							_SetProperty( JS4D::ValueRef inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const;

	mutable	VJSObject						fObject;
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
								VJSArray(const VJSArray& inOther);

			explicit			VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray = true);

			explicit			VJSArray( const VJSContext& inContext, VJSException* outException, bool refIsNil) :fContext(inContext), fObject(NULL) {}

			VJSArray&			operator=(const VJSArray& inOther);

								~VJSArray();

								operator VJSObject() const;
								operator VJSValue() const;

			void				Protect() const;
			void				Unprotect() const;
			
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
			void				PushValues( const std::vector<VJSValue>& inValues, VJSException *outException = NULL) const	{_Splice( GetLength(), 0, &inValues, outException);}
			void				PushValues( const std::vector<const XBOX::VValueSingle*> inValues, VJSException *outException = NULL) const;
			
			// remove inCount elements starting at element inWhere
			void				Remove( size_t inWhere, size_t inCount, VJSException *outException = NULL) const	{ _Splice( inWhere, inCount, NULL, outException); }
			
			// remove all elements
			void				Clear( VJSException *outException = NULL) const				{ _Splice( 0, GetLength(), NULL, outException); }

			// The splice() method remove inCount elements starting with element at inWhere and insert values at inWhere
			void				Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue>& inValues, VJSException *outException = NULL) const	{_Splice( inWhere, inCount, &inValues, outException); }

			void				SetNull();
			const VJSContext&	GetContext() const { return fContext; }
private:
friend class VJSObject;
#if USE_V8_ENGINE
friend class VJSObject;
#endif
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

			bool					MetaInfoInited() const {return fMetaInfoIsValid;}

			XBOX::VJSValue			GetMetaInfo() const	{return VJSValue(fContext,fMetaInfo); }

			void					SetMetaInfo(JS4D::ValueRef inMetaInfo, const XBOX::VJSContext& inContext);

			XBOX::VPicture*			GetPict() {	return fPict; }

	const	XBOX::VValueBag*		RetainMetaBag();
			void					SetMetaBag(const XBOX::VValueBag* metaBag);
	
protected:
	virtual ~VJSPictureContainer();

	XBOX::VPicture*				fPict;
	JS4D::ValueRef				fMetaInfo;
	XBOX::VJSContext			fContext;
	const XBOX::VValueBag*		fMetaBag;
	bool						fMetaInfoIsValid;
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
			
			void					GetException(VJSException& ioException) const { ioException = fException;}

			void					ClearParamsAndResult();

private:
			std::vector<VJSValue>	fParams;
			VString					fFunctionName;
			VJSObject				fFunctionObject;
			VJSValue				fResult;
			VJSContext				fContext;
			VJSException			fException;
};



#if USE_V8_ENGINE
#else
inline	bool				VJSValue::IsUndefined() const			{ return JS4D::ValueIsUndefined(fContext, fValue); }
inline	bool				VJSValue::IsNull() const				{ return JS4D::ValueIsNull(fContext, fValue); }
inline	bool				VJSValue::IsNumber() const				{ return JS4D::ValueIsNumber(fContext, fValue); }
inline	bool				VJSValue::IsBoolean() const				{ return JS4D::ValueIsBoolean(fContext, fValue); }
inline	bool				VJSValue::IsString() const				{ return JS4D::ValueIsString(fContext, fValue); }
inline	bool				VJSValue::IsObject() const				{ return JS4D::ValueIsObject(fContext, fValue); }
inline	bool				VJSValue::IsInstanceOfBoolean() const	{ return IsInstanceOf("Boolean", (JS4D::ExceptionRef *)NULL); }
inline	bool				VJSValue::IsInstanceOfNumber() const	{ return IsInstanceOf("Number", (JS4D::ExceptionRef *)NULL); }
inline	bool				VJSValue::IsInstanceOfString() const	{ return IsInstanceOf("String", (JS4D::ExceptionRef *)NULL); }
inline	bool				VJSValue::IsArray() const				{ return IsInstanceOf("Array", (JS4D::ExceptionRef *)NULL); }
inline	bool				VJSValue::IsInstanceOfDate() const		{ return IsInstanceOf("Date", (JS4D::ExceptionRef *)NULL); }
inline	bool				VJSValue::IsInstanceOfRegExp() const	{ return IsInstanceOf("RegExp", (JS4D::ExceptionRef *)NULL); }
#endif




template<class NATIVE_CLASS>
inline typename NATIVE_CLASS::PrivateDataType*	VJSValue::GetObjectPrivateData(VJSException *outException) const
#if USE_V8_ENGINE
{
	JS4D::ClassRef classRef = NATIVE_CLASS::Class();
	if (fValue && JS4D::ValueIsObjectOfClass(fContext, fValue, classRef))
	{
		return static_cast<typename NATIVE_CLASS::PrivateDataType*>(
			InternalGetObjectPrivateData(classRef, outException, fValue));
	}
	return NULL;
}
#else
{
	JS4D::ClassRef classRef = NATIVE_CLASS::Class();
	if (JS4D::ValueIsObjectOfClass(fContext, fValue, classRef))
	{
		return static_cast<typename NATIVE_CLASS::PrivateDataType*>(
			JS4D::GetObjectPrivate(
				JS4D::ValueToObject(fContext, fValue, VJSException::GetExceptionRefIfNotNull(outException))));
	}
	return NULL;
}
#endif



template<class NATIVE_CLASS>
typename NATIVE_CLASS::PrivateDataType*	VJSObject::GetPrivateData() const
{
	JS4D::ClassRef classRef = NATIVE_CLASS::Class();
	if (JS4D::ValueIsObjectOfClass(fContext, fObject, classRef))
	{
		return static_cast<typename NATIVE_CLASS::PrivateDataType*>(
#if USE_V8_ENGINE
			JS4D::GetObjectPrivate(fContext, fObject));
#else
			JS4D::GetObjectPrivate(fObject));
#endif
	}
	return NULL;
}

END_TOOLBOX_NAMESPACE


#endif
