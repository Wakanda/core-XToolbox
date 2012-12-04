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

	explicit					VJSValue( JS4D::ContextRef inContext) : fContext( inContext), fValue( NULL)	{}
	explicit					VJSValue( JS4D::ContextRef inContext, JS4D::ValueRef inValue) : fContext( inContext), fValue( inValue)	{}
								VJSValue( const VJSValue& inOther):fContext( inOther.fContext), fValue( inOther.fValue)	{}
			
								operator JS4D::ValueRef() const		{ return fValue;}
			const VJSValue&		operator=( const VJSValue& inOther)	{ /*xbox_assert( fContext == inOther.fContext);*/ fValue = inOther.fValue; fContext = inOther.fContext; return *this; }

			bool				IsUndefined() const		{ return JS4D::ValueIsUndefined( fContext, fValue); }
			bool				IsNull() const			{ return JS4D::ValueIsNull( fContext, fValue); }
			bool				IsNumber() const		{ return JS4D::ValueIsNumber( fContext, fValue); }
			bool				IsBoolean() const		{ return JS4D::ValueIsBoolean( fContext, fValue); }
			bool				IsString() const		{ return JS4D::ValueIsString( fContext, fValue); }
			bool				IsObject() const		{ return JS4D::ValueIsObject( fContext, fValue); }
			bool				IsFunction() const;
			bool				IsInstanceOf( const XBOX::VString& inConstructorName, JS4D::ExceptionRef *outException = NULL) const		{ return JS4D::ValueIsInstanceOf( fContext, fValue, inConstructorName, outException);}
			bool				IsArray() const			{ return IsInstanceOf("Array"); }

			// Return type of value, note that eTYPE_OBJECT is very broad (an object can be a function, an Array, a Date, a String, etc.).

			JS4D::EType			GetType ()				{ return JS4D::GetValueType( fContext, fValue); }

			// cast value into some type.
			// If cast is not possible before the value type is not compatible, it doesn't throw an error or an exception but simply returns false or NULL.
			bool				GetReal( Real *outValue, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetLong( sLONG *outValue, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetLong8( sLONG8 *outValue, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetULong( uLONG *outValue, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetString( XBOX::VString& outString, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetJSONValue( VJSONValue& outValue, JS4D::ExceptionRef *outException = NULL) const;

			// if value is a string, make it a url, else returns false
			bool				GetURL( XBOX::VURL& outURL, JS4D::ExceptionRef *outException = NULL) const;

			bool				GetBool( bool *outValue, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetTime( XBOX::VTime& outTime, JS4D::ExceptionRef *outException = NULL) const;
			bool				GetDuration( XBOX::VDuration& outDuration, JS4D::ExceptionRef *outException = NULL) const;
			XBOX::VFile*		GetFile( JS4D::ExceptionRef *outException = NULL) const;
			XBOX::VFolder*		GetFolder( JS4D::ExceptionRef *outException = NULL) const;
			XBOX::VValueSingle*	CreateVValue( JS4D::ExceptionRef *outException = NULL) const										{ return JS4D::ValueToVValue( fContext, fValue, outException); }
			void				GetObject( VJSObject& outObject, JS4D::ExceptionRef *outException = NULL) const;
			VJSObject			GetObject( JS4D::ExceptionRef *outException = NULL) const;

			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType*	GetObjectPrivateData( JS4D::ExceptionRef *outException = NULL) const
			{
				JS4D::ClassRef classRef = NATIVE_CLASS::Class();
				return static_cast<typename NATIVE_CLASS::PrivateDataType*>( JS4D::ValueIsObjectOfClass( fContext, fValue, classRef) ? JS4D::GetObjectPrivate( JS4D::ValueToObject( fContext, fValue, outException) ) : NULL);
			}

			template<class Type>
			void				SetNumber( const Type& inValue, JS4D::ExceptionRef *outException = NULL)						{ fValue = JS4D::NumberToValue( fContext, inValue); }
			void				SetString( const XBOX::VString& inString, JS4D::ExceptionRef *outException = NULL)				{ fValue = JS4D::VStringToValue( fContext, inString); }
			void				SetBool( bool inValue, JS4D::ExceptionRef *outException = NULL)									{ fValue = JS4D::BoolToValue( fContext, inValue); }
			void				SetTime( const XBOX::VTime& inTime, JS4D::ExceptionRef *outException = NULL)					{ fValue = JS4D::VTimeToObject( fContext, inTime, outException); }
			void				SetDuration( const XBOX::VDuration& inDuration, JS4D::ExceptionRef *outException = NULL)		{ fValue = JS4D::VDurationToValue( fContext, inDuration); }
			void				SetVValue( const XBOX::VValueSingle& inValue, JS4D::ExceptionRef *outException = NULL)			{ fValue = JS4D::VValueToValue( fContext, inValue, outException); }
			void				SetJSONValue( const VJSONValue& inValue, JS4D::ExceptionRef *outException = NULL)				{ fValue = JS4D::VJSONValueToValue( fContext, inValue, outException); }
			void				SetFile( XBOX::VFile *inFile, JS4D::ExceptionRef *outException = NULL)							{ fValue = JS4D::VFileToObject( fContext, inFile, outException); }
			void				SetFolder( XBOX::VFolder *inFolder, JS4D::ExceptionRef *outException = NULL)					{ fValue = JS4D::VFolderToObject( fContext, inFolder, outException); }
			void				SetFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, JS4D::ExceptionRef *outException = NULL)		{ fValue = JS4D::VFilePathToObjectAsFileOrFolder( fContext, inPath, outException); }

			void				SetUndefined( JS4D::ExceptionRef *outException = NULL)											{ fValue = JS4D::MakeUndefined( fContext);}
			void				SetNull( JS4D::ExceptionRef *outException = NULL)												{ fValue = JS4D::MakeNull( fContext);}

			/*
				Any value saved elsewhere than on the stack must be protected so that it doesn't get garbage collected.
				Warning: be careful to balance calls to Protect/Unprotect on same ValueRef.
			*/
			void				Protect() const																					{ JS4D::ProtectValue( fContext, fValue);}
			void				Unprotect() const																				{ JS4D::UnprotectValue( fContext, fValue);}
			
			void				SetValueRef( JS4D::ValueRef inValue)															{ fValue = inValue;}
			JS4D::ValueRef		GetValueRef() const																				{ return fValue;}

			VJSContext			GetContext() const																				{ return XBOX::VJSContext(fContext); }
			JS4D::ContextRef	GetContextRef() const																			{ return fContext;}			
private:
								VJSValue();	// forbidden (always need a context)
								
			JS4D::ContextRef	fContext;
			JS4D::ValueRef		fValue;
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
	explicit					VJSObject( JS4D::ContextRef inContext) : fContext( inContext), fObject( NULL)	{}
	explicit					VJSObject( JS4D::ContextRef inContext, JS4D::ObjectRef inObject) : fContext( inContext), fObject( inObject)	{}
								VJSObject( const VJSObject& inOther):fContext( inOther.fContext), fObject( inOther.fObject)	{}
								VJSObject( const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone); // build an empty object as a property of a parent object
								VJSObject( const VJSArray& inParent, sLONG inPos); // build an empty object as an element of an array (-1 means push at the end)

								operator JS4D::ObjectRef() const		{ return fObject;}
								operator VJSValue() const				{ return VJSValue( fContext, fObject);}
			const VJSObject&	operator=( const VJSObject& inOther)	{ /*xbox_assert( fContext == inOther.fContext);*/ fObject = inOther.fObject; fContext = inOther.fContext; return *this; }

			bool				IsInstanceOf( const XBOX::VString& inConstructorName, JS4D::ExceptionRef *outException = NULL) const		{ return JS4D::ValueIsInstanceOf( fContext, fObject, inConstructorName, outException);}
			bool				IsFunction() const							{ return (fObject != NULL) ? JS4D::ObjectIsFunction( fContext, fObject) : false; }	// sc 03/09/2009 crash if fObject is null
			bool				IsArray() const								{ return IsInstanceOf("Array"); }
			bool				IsObject() const							{ return fObject != NULL; }
			bool				IsOfClass (JS4D::ClassRef inClassRef) const	{ return JS4D::ValueIsObjectOfClass( fContext, fObject, inClassRef); }

			void				MakeEmpty();	
			void				MakeFile( XBOX::VFile *inFile, JS4D::ExceptionRef *outException = NULL)									{ fObject = JS4D::VFileToObject( fContext, inFile, outException); }
			void				MakeFolder( XBOX::VFolder *inFolder, JS4D::ExceptionRef *outException = NULL)							{ fObject = JS4D::VFolderToObject( fContext, inFolder, outException); }
			void				MakeFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, JS4D::ExceptionRef *outException = NULL)		{ fObject = JS4D::VFilePathToObjectAsFileOrFolder( fContext, inPath, outException); }
	
			bool				HasProperty( const XBOX::VString& inPropertyName) const;
			bool				DeleteProperty( const XBOX::VString& inPropertyName, JS4D::ExceptionRef *outException = NULL) const;
	
			VJSValue			GetProperty( const XBOX::VString& inPropertyName, JS4D::ExceptionRef *outException = NULL) const;
			VJSObject			GetPropertyAsObject( const XBOX::VString& inPropertyName, JS4D::ExceptionRef *outException = NULL) const;
			void				GetPropertyNames( std::vector<XBOX::VString>& outNames) const;

			bool				GetPropertyAsString(const VString& inPropertyName, JS4D::ExceptionRef *outException, VString& outResult) const;
			sLONG				GetPropertyAsLong(const VString& inPropertyName, JS4D::ExceptionRef *outException, bool* outExists) const;
			Real				GetPropertyAsReal(const VString& inPropertyName, JS4D::ExceptionRef *outException, bool* outExists) const;
			bool				GetPropertyAsBool(const VString& inPropertyName, JS4D::ExceptionRef *outException, bool* outExists) const;


			void				SetNullProperty( const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;

			void				SetProperty( const XBOX::VString& inPropertyName, const VJSValue& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const VJSObject& inObject, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const VJSArray& inArray, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const XBOX::VValueSingle* inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, sLONG inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, sLONG8 inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, Real inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, const XBOX::VString& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;
			void				SetProperty( const XBOX::VString& inPropertyName, bool inBoolean, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const;

			// call a member function on this object.
			// returns false if the function was not found.
			// If inFullPath is not NULL, set it as the "source" of the executing script.
			// This is needed if files are referred relative to the script containing the function, require() for example.

			bool				CallFunction(
									const VJSObject& inFunctionObject, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									JS4D::ExceptionRef *outException,
									const XBOX::VFilePath *inFullPath = NULL);
			bool				CallMemberFunction(
									const XBOX::VString& inFunctionName, 
									const std::vector<VJSValue> *inValues, 
									VJSValue *outResult, 
									JS4D::ExceptionRef *outException,
									const XBOX::VFilePath *inFullPath = NULL)
									{
										return CallFunction(GetPropertyAsObject(inFunctionName, outException), inValues, outResult, outException, inFullPath);
									}

			// Call object as a constructor, return true if successful. outCreatedObject must not be NULL.

			bool				CallAsConstructor (const std::vector<VJSValue> *inValues, VJSObject *outCreatedObject, JS4D::ExceptionRef *outException);

			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType*	GetPrivateData() const
			{
				JS4D::ClassRef classRef = NATIVE_CLASS::Class();
				return static_cast<typename NATIVE_CLASS::PrivateDataType*>( JS4D::ValueIsObjectOfClass( fContext, fObject, classRef) ? JS4D::GetObjectPrivate( fObject) : NULL);
			}

			void				SetUndefined( JS4D::ExceptionRef *outException = NULL)		{ fObject = JS4D::ValueToObject( fContext, JS4D::MakeUndefined( fContext), outException);}
			void				SetNull (JS4D::ExceptionRef *outException = NULL)			{ fObject = JS4D::ValueToObject( fContext, JS4D::MakeNull(fContext), outException); }

			/*
				any value saved elsewhere than on the stack must be protected so that it doesn't get garbage collected.
				Warning: be careful to balance calls to Protect/Unprotect on same ValueRef.
			*/
			void				Protect() const												{ JS4D::ProtectValue( fContext, fObject);}
			void				Unprotect() const											{ JS4D::UnprotectValue( fContext, fObject);}

			void				SetObjectRef( JS4D::ObjectRef inObject)						{ fObject = inObject;}
			JS4D::ObjectRef		GetObjectRef() const										{ return fObject;}

			const VJSContext&	GetContext() const											{ return fContext;}
			JS4D::ContextRef	GetContextRef() const										{ return fContext;}

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
	
private:
								VJSObject();	// forbidden (always need a context)

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
			
			VJSValue						GetProperty( JS4D::ExceptionRef *outException = NULL) const			{ return VJSValue( fObject.GetContextRef(), _GetProperty( outException));}
			void							SetProperty( const VJSValue& inValue, JS4D::PropertyAttributes inAttributes = JS4D::PropertyAttributeNone, JS4D::ExceptionRef *outException = NULL) const	{ _SetProperty( inValue, inAttributes, outException);}

			VJSObject						GetPropertyAsObject( JS4D::ExceptionRef *outException = NULL) const;

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
			explicit			VJSArray( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException = NULL);
			
								// create a new array with a list of values
								// this is a lot more efficient than looping around PushValue
			explicit			VJSArray( JS4D::ContextRef inContext, const std::vector<VJSValue>& inValues, JS4D::ExceptionRef *outException = NULL);
			
								// uses an object as an Array
								// if inCheckInstanceOfArray is true, VJSObject::IsInstanceOf is called to make sure it is actually an array.
			explicit			VJSArray( const VJSObject& inArrayObject, bool inCheckInstanceOfArray = true);
								VJSArray( const VJSArray& inOther): fContext( inOther.fContext), fObject( inOther.fObject)	{}

			explicit			VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray = true);

			explicit			VJSArray( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException, bool refIsNil)
								{
									fContext = inContext;
									fObject = NULL;
								}

								operator JS4D::ObjectRef() const		{ return fObject;}
								operator VJSObject() const				{ return VJSObject( fContext, fObject);}
								operator VJSValue() const				{ return VJSValue( fContext, fObject);}
			
			// warning: indexes starts at zero just like in javascript
			size_t				GetLength() const;
			VJSValue			GetValueAt( size_t inIndex, JS4D::ExceptionRef *outException = NULL) const;
			void				SetValueAt( size_t inIndex, const VJSValue& inValue, JS4D::ExceptionRef *outException = NULL) const;
			
			void				PushValue( const VJSValue& inValue, JS4D::ExceptionRef *outException = NULL) const;
			void				PushValue( const VValueSingle& inValue, JS4D::ExceptionRef *outException = NULL) const;
			template<class Type>
			void				PushNumber( Type inValue , JS4D::ExceptionRef *outException = NULL) const;
			void				PushString( const VString& inValue , JS4D::ExceptionRef *outException = NULL) const;
			void				PushFile( XBOX::VFile *inFile, JS4D::ExceptionRef *outException = NULL) const;
			void				PushFolder( XBOX::VFolder *inFolder, JS4D::ExceptionRef *outException = NULL) const;
			void				PushFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, JS4D::ExceptionRef *outException = NULL) const;
			void				PushValues( const std::vector<VJSValue>& inValues, JS4D::ExceptionRef *outException = NULL) const	{ _Splice( GetLength(), 0, &inValues, outException); }
			void				PushValues( const std::vector<const XBOX::VValueSingle*> inValues, JS4D::ExceptionRef *outException = NULL) const;
			
			// remove inCount elements starting at element inWhere
			void				Remove( size_t inWhere, size_t inCount, JS4D::ExceptionRef *outException = NULL) const				{ _Splice( inWhere, inCount, NULL, outException); }
			
			// remove all elements
			void				Clear( JS4D::ExceptionRef *outException = NULL) const												{ _Splice( 0, GetLength(), NULL, outException); }

			// The splice() method remove inCount elements starting with element at inWhere and insert values at inWhere
			void				Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue>& inValues, JS4D::ExceptionRef *outException = NULL) const	{ _Splice( inWhere, inCount, &inValues, outException); }

			void				SetObjectRef( JS4D::ObjectRef inObject)						{ fObject = inObject;}
			JS4D::ObjectRef		GetObjectRef() const										{ return fObject;}

			JS4D::ContextRef	GetContextRef() const
			{
				return fContext;
			}
								
private:
								VJSArray();	// forbidden (always need a context)
			void				_Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue> *inValues, JS4D::ExceptionRef *outException) const;

			JS4D::ContextRef	fContext;
			JS4D::ObjectRef		fObject;
};


//======================================================

#if !VERSION_LINUX  // Postponed Linux Implementation !

class XTOOLBOX_API VJSPictureContainer : public XBOX::VObject, public XBOX::IRefCountable
{
public:
	VJSPictureContainer(XBOX::VValueSingle* inPict, /*bool ownsPict,*/ JS4D::ContextRef inContext);

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

	void SetMetaInfo(JS4D::ValueRef inMetaInfo, JS4D::ContextRef inContext);

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
	JS4D::ContextRef fContext;
	const XBOX::VValueBag* fMetaBag;
	bool fMetaInfoIsValid;
	//bool fOwnsPict;

};

#endif


//======================================================

/*
	facility class to call a function in context with parameters.
	
	the function name can actually be any valid JavaScript expression that evaluates to a function object.
*/
class XTOOLBOX_API VJSFunction : public XBOX::VObject
{
public:
									VJSFunction( const VString& inFuncName, JS4D::ContextRef inContext):fContext( inContext), fResult( inContext), fFuncName( inFuncName), fExcept( NULL)		{}
	virtual							~VJSFunction();

			void					AddParam( const VValueSingle* inVal);
			void					AddParam( const VJSValue& inVal);
			void					AddParam( const VJSONValue& inVal);
			void					AddParam( const VString& inVal);
			void					AddParam( sLONG inVal);
			void					AddBoolParam( bool inVal);
			void					AddLong8Param( sLONG8 inVal);

			bool					Call();

			bool					IsNullResult() const			{ return fResult.IsNull() || fResult.IsUndefined();}

			bool					GetResultAsBool() const;
			sLONG					GetResultAsLong() const;
			sLONG8					GetResultAsLong8() const;
			void					GetResultAsString( VString& outResult) const;
			bool					GetResultAsJSONValue( VJSONValue& outResult) const;
			const VJSValue&			GetResult() const				{ return fResult; }
			
			JS4D::ExceptionRef		GetException() const			{ return fExcept; }

			void					ClearParamsAndResult();

private:
			std::vector<VJSValue>	fParams;
			VString					fFuncName;
			VJSValue				fResult;
			VJSContext				fContext;
			JS4D::ExceptionRef		fExcept;
};

END_TOOLBOX_NAMESPACE


#endif
