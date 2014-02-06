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
#ifndef __VJSClass__
#define __VJSClass__

#include "JS4D.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

#if !VERSIONWIN
	#define __cdecl
#endif

class XTOOLBOX_API VJSParms_withContext
{	
public:
#if USE_V8_ENGINE
	VJSParms_withContext() : fObject((VJSContext*)NULL) {;}
#endif 
	VJSParms_withContext( JS4D::ContextRef inContext, JS4D::ObjectRef inObject)
		: fObject( inContext, inObject)	{}
			
	~VJSParms_withContext();

			const VJSContext&				GetContext() const					{ return fObject.GetContext();}

			void							ProtectValue( JS4D::ValueRef inValue) const;
			void							UnprotectValue( JS4D::ValueRef inValue) const;

			const VJSObject&				GetObject() const					{ return fObject;}

			UniChar							GetWildChar() const					{ return GetContext().GetWildChar(); };
			
protected:
			VJSObject						fObject;
			JS4D::ContextRef				GetContextRef() const				{ return fObject.GetContextRef();}
};


//======================================================
class XTOOLBOX_API VJSParms_withException : public VJSParms_withContext
{
public:
#if USE_V8_ENGINE
	VJSParms_withException() :
		fException(NULL), fTaskContext(NULL), fErrorContext(NULL) {;}
	VJSParms_withException(JS4D::ContextRef inContext) :
		fException(NULL), fTaskContext(NULL), fErrorContext(NULL) , VJSParms_withContext(inContext,NULL) {;}
#endif 
	VJSParms_withException( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::ExceptionRef* outException)
		: VJSParms_withContext( inContext, inObject)
		, fException( outException)
		{
			fTaskContext = VTask::GetCurrent()->GetErrorContext( true);
			fErrorContext = fTaskContext->PushNewContext( false /* do not keep */, true /* silent */);
		}
		
	~VJSParms_withException()
		{
#if USE_V8_ENGINE
#else
			JS4D::ConvertErrorContextToException( GetContextRef(), fErrorContext, fException);
			xbox_assert( fErrorContext == fTaskContext->GetLastContext());
			fTaskContext->PopContext();
			fTaskContext->RecycleContext( fErrorContext);
#endif
		}

			void							EvaluateScript( const XBOX::VString& inScript, XBOX::VValueSingle** outResult);

			JS4D::ExceptionRef*				GetExceptionRefPointer()	{ return fException;}
			void							SetException(JS4D::ExceptionRef inException) {
				if (testAssert(fException))
				{
					*fException = inException;
				}
			}
			void							SetException(const VJSException& inException) {
				SetException(inException.GetValue());
			}
protected:
			JS4D::ExceptionRef*				fException;
			VErrorTaskContext*				fTaskContext;
			VErrorContext*					fErrorContext;
};

//======================================================

class XTOOLBOX_API VJSParms_withReturnValue : public virtual VJSParms_withException
{
public:
#if USE_V8_ENGINE
	VJSParms_withReturnValue( const v8::FunctionCallbackInfo<v8::Value>& args ) : fV8Arguments(&args), fReturnValue(NULL) {;}
#endif 
	VJSParms_withReturnValue( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::ExceptionRef* outException)
		: VJSParms_withException( inContext, inObject, outException)
		, fReturnValue( inContext)
#if USE_V8_ENGINE
		, fV8Arguments(NULL)
#endif 
		{;}

			template<class Type>
			void				ReturnNumber( const Type& inValue)					{
				VJSException	except;
				fReturnValue.SetNumber( inValue, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnBool( bool inValue)							{
				VJSException	except;
				fReturnValue.SetBool( inValue, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnString( const XBOX::VString& inString)		{
				VJSException	except;
				fReturnValue.SetString( inString, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnTime( const XBOX::VTime& inTime)				{
				VJSException	except;
				fReturnValue.SetTime( inTime, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnDuration( const XBOX::VDuration& inDuration)	{ 
				VJSException	except;
				fReturnValue.SetDuration( inDuration, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnVValue( const XBOX::VValueSingle& inValue)	{ 
				VJSException	except;
				fReturnValue.SetVValue( inValue, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnFile( XBOX::VFile *inFile)					{ 
				VJSException	except;
				fReturnValue.SetFile( inFile, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnFolder( XBOX::VFolder *inFolder)				{ 
				VJSException	except;
				fReturnValue.SetFolder( inFolder, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
			void				ReturnFilePathAsFileOrFolder( const XBOX::VFilePath& inPath)	{ 
				VJSException	except;
				fReturnValue.SetFilePathAsFileOrFolder( inPath, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}
				
			void				ReturnValue( const VJSValue& inValue)				{ fReturnValue = inValue; }
			void				ReturnJSONValue( const VJSONValue& inValue)			{ 
				VJSException	except;
				fReturnValue.SetJSONValue( inValue, &except);
				if (fException)
				{
					*fException = except.GetValue();
				}
			}

			void				ReturnNullValue()									{ fReturnValue.SetNull(); }
			void				ReturnUndefinedValue()								{ fReturnValue.SetUndefined(); }

			const VJSValue&		GetReturnValue() const								{ return fReturnValue;}

protected:
			VJSValue			fReturnValue;
#if USE_V8_ENGINE
			const v8::FunctionCallbackInfo<v8::Value>*	fV8Arguments;
#endif
};

//======================================================


class XTOOLBOX_API VJSParms_initialize : public VJSParms_withContext
{
public:
	VJSParms_initialize( JS4D::ContextRef inContext, JS4D::ObjectRef inObject)
		: VJSParms_withContext( inContext, inObject)	{;}
	
private:
};


//======================================================


class XTOOLBOX_API VJSParms_finalize
{
public:
	VJSParms_finalize( JS4D::ObjectRef inObject)
		: fObject( inObject)	{;}

/*
			// UnprotectValue() crashes because there is no context. 
			// JSValueUnprotect(NULL, someValueRef) doesn't work.
			
			void	UnprotectValue( JS4D::ValueRef inValue) const;
*/
	
private:
			JS4D::ObjectRef			fObject;
};


//======================================================


class XTOOLBOX_API VJSParms_withArguments : public virtual VJSParms_withException
{
public:
#if USE_V8_ENGINE
	VJSParms_withArguments( const v8::FunctionCallbackInfo<v8::Value>& args );
#else
	VJSParms_withArguments( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
		: VJSParms_withException( inContext, inObject, outException)
		, fArgumentCount( inArgumentCount)
		, fArguments( inArguments)
			{
			;
			}
#endif

			size_t				CountParams() const					{
#if USE_V8_ENGINE
			return fArgumentCount;
#else
			return fArgumentCount;
#endif
			}
			
			// cast specified param into some type
			// returns false if index out of bound
			// index starts at 1.
			bool				GetStringParam( size_t inIndex, XBOX::VString& outString) const;
			bool				GetLongParam( size_t inIndex, sLONG *outValue) const;
			bool				GetLong8Param( size_t inIndex, sLONG8 *outValue) const;
			bool				GetULongParam( size_t inIndex, uLONG *outValue) const;
			bool				GetRealParam( size_t inIndex, Real *outValue) const;
			bool				GetBoolParam( size_t inIndex, bool *outValue) const;
			bool				GetBoolParam( size_t inIndex, const XBOX::VString& inNameForTrue, const XBOX::VString& inNameForFalse, size_t* outIndex = NULL) const;

			XBOX::VFile*		RetainFileParam( size_t inIndex, bool allowPath = true) const;
			XBOX::VFolder*		RetainFolderParam( size_t inIndex, bool allowPath = true) const;

			VJSValue			GetParamValue( size_t inIndex) const;
			XBOX::VValueSingle*	CreateParamVValue( size_t inIndex, bool simpleDate = false) const;
			bool				GetParamObject( size_t inIndex, VJSObject& outObject) const;
			bool				GetParamArray( size_t inIndex, VJSArray& outArray) const;
			bool				GetParamFunc( size_t inIndex, VJSObject& outObject, bool allowString = false) const;
			bool				GetParamJSONValue( size_t inIndex, VJSONValue& outValue) const;
			
			// returns true if param doesn't exist or is null (no exception is thrown)
			bool				IsNullParam( size_t inIndex) const;
			
			// tells how is defined the javascript param
			bool				IsNumberParam( size_t inIndex) const;
			bool				IsBooleanParam( size_t inIndex) const;
			bool				IsStringParam( size_t inIndex) const;
			bool				IsObjectParam( size_t inIndex) const;
			bool				IsArrayParam( size_t inIndex) const;
			bool				IsFunctionParam( size_t inIndex) const;

			// returns the private data of specified param if it is a native object of class NATIVE_CLASS, NULL if it is not.
			// exception if index out of bound.
			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType *GetParamObjectPrivateData( size_t inIndex) const
			{
				VJSValue value( GetParamValue( inIndex));
				return value.GetObjectPrivateData<NATIVE_CLASS>();
			}
			
private:
#if USE_V8_ENGINE
			const v8::FunctionCallbackInfo<v8::Value>* 	fV8Arguments; 
#else
			const JS4D::ValueRef*	fArguments;
#endif
			size_t				fArgumentCount;
};


//======================================================

class XTOOLBOX_API VJSParms_callStaticFunction : public VJSParms_withReturnValue, public VJSParms_withArguments
{
public:
#if USE_V8_ENGINE
	VJSParms_callStaticFunction( const v8::FunctionCallbackInfo<v8::Value>& args )
	//VJSParms_callStaticFunction::VJSParms_callStaticFunction( const FunctionCallbackInfo<Value>& args )	:
		: VJSParms_withArguments(args)
		, VJSParms_withReturnValue( args)
		, VJSParms_withException( )
		, fThis((VJSContext*)NULL)
{
	;
}

#else
	VJSParms_callStaticFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
		: VJSParms_withArguments( inContext, inFunction, inArgumentCount, inArguments, outException)
		, VJSParms_withReturnValue( inContext, inFunction, outException)
		, VJSParms_withException( inContext, inFunction, outException)
		, fThis( inContext, inThis)
			{
			;
			}
			 
			JS4D::ValueRef		GetValueRefOrNull(JS4D::ContextRef inContext) {
				return (GetReturnValue() != NULL) ? GetReturnValue() : JS4D::MakeNull(inContext);
			}
#endif
			const VJSObject&	GetThis() const						{ return fThis;}
			void				ReturnThis()						{ fReturnValue = fThis;}
private:
			VJSObject			fThis;
};


//======================================================

class XTOOLBOX_API VJSParms_callAsFunction : public VJSParms_withReturnValue, public VJSParms_withArguments
{
public:
#if USE_V8_ENGINE
	VJSParms_callAsFunction( const v8::FunctionCallbackInfo<v8::Value>& args ) 
		: VJSParms_withArguments(args)
		, VJSParms_withReturnValue( args)
		, VJSParms_withException( )
		, fFunction (NULL,NULL)
		, fThis( NULL, NULL)
			{ 
				xbox_assert(false);
			}
#else
	VJSParms_callAsFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
		: VJSParms_withArguments( inContext, inFunction, inArgumentCount, inArguments, outException)
		, VJSParms_withReturnValue( inContext, inFunction, outException)
		, VJSParms_withException( inContext, inFunction, outException)
		, fFunction (inContext, inFunction)
		, fThis( inContext, inThis)
			{
			;
			}
			JS4D::ValueRef		GetValueRefOrNull(JS4D::ContextRef inContext) {
				return (GetReturnValue() != NULL) ? GetReturnValue() : JS4D::MakeNull(inContext);
			}
#endif
			// Use the returned (function) object to retrieve private C++ data.

			VJSObject			GetFunction ()						{ return fFunction;	}


			const VJSObject&	GetThis() const						{ return fThis;}
			void				ReturnThis()						{ fReturnValue = fThis;}

private:

			VJSObject			fFunction;		// Object which is called as a function.
			VJSObject			fThis;
};

//======================================================

class XTOOLBOX_API VJSParms_callAsConstructor : public VJSParms_withArguments
{
public:
#if USE_V8_ENGINE
	VJSParms_callAsConstructor(const v8::FunctionCallbackInfo<v8::Value>& args )
		: VJSParms_withArguments(args)
		, VJSParms_withException( )
		, fConstructedObject((VJSContext*)NULL )
			{ 
				xbox_assert(false);
			}
#else
	VJSParms_callAsConstructor( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
		: VJSParms_withArguments( inContext, inConstructor, inArgumentCount, inArguments, outException)
		, VJSParms_withException( inContext, inConstructor, outException)
		, fConstructedObject( inContext)
			{
			;
			}
#endif			
			void				ReturnConstructedObject( const VJSObject& inObject)		{ fConstructedObject = inObject;}
			void				ReturnUndefined()										{ fConstructedObject.SetUndefined(); }
			const VJSObject&	GetConstructedObject() const							{ return fConstructedObject;}

			JS4D::ObjectRef		GetConstructedObjectRef() const	{ return fConstructedObject.GetObjectRef(); }
private:
			VJSObject			fConstructedObject;
};


//======================================================

class XTOOLBOX_API VJSParms_construct : public VJSParms_withArguments
{
/*
	Used for instantiating an object with new operator VJSGlobalClass::AddConstructorObjectStaticValue has been used.
*/
public:
	typedef void (*fn_construct)( VJSParms_construct&);
	template<fn_construct fn>
#if USE_V8_ENGINE
	static void __cdecl Callback(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
	}
#else
static JS4D::ObjectRef __cdecl Callback( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
	{
		VJSParms_construct parms( inContext, inConstructor, inArgumentCount, inArguments, outException);
		fn( parms);
		return parms.GetConstructedObject();
	}
#endif

#if USE_V8_ENGINE
VJSParms_construct(const v8::FunctionCallbackInfo<v8::Value>& args)
		: VJSParms_withArguments(args)
		, VJSParms_withException()
		, fConstructedObject((VJSContext*)NULL)
			{
				xbox_assert(false);
			}

#else
VJSParms_construct( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ValueRef* outException)
		: VJSParms_withArguments( inContext, inConstructor, inArgumentCount, inArguments, outException)
		, VJSParms_withException( inContext, inConstructor, outException)
		, fConstructedObject( inContext)
			{
			;
			}
#endif

			void				ReturnConstructedObject( const VJSObject& inObject)		{ fConstructedObject = inObject;}
			void				ReturnUndefined()										{ fConstructedObject.SetUndefined(); }
			const VJSObject&	GetConstructedObject() const							{ return fConstructedObject;}


private:
			VJSObject			fConstructedObject;
};

//======================================================


class XTOOLBOX_API VJSParms_getProperty : public VJSParms_withReturnValue
{
public:
	VJSParms_getProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
		: VJSParms_withReturnValue( inContext, inObject, outException)
		, VJSParms_withException( inContext, inObject, outException)
		, fPropertyName( inPropertyName)
		{
		#if VERSIONDEBUG
			JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
		#endif
		}

			bool				GetPropertyName( XBOX::VString& outPropertyName) const	{ return JS4D::StringToVString( fPropertyName, outPropertyName);}
			bool				GetPropertyNameAsLong( sLONG *outValue) const			{ return JS4D::StringToLong( fPropertyName, outValue);}
			const UniChar*		GetPropertyNamePtr( XBOX::VIndex *outLength) const;

			// If you don't call any Returnxxx() function nor throw any exception, JSC will call parent class getProperty accessor.
			// Call ForwardToParent() if you think a Returnxxx() function has been called and you want to cancel it.
			void				ForwardToParent () 										{ fReturnValue.SetValueRef(NULL);	} 
			
			JS4D::ValueRef		GetValueRefOrNull(JS4D::ContextRef inContext) {
				return GetReturnValue();
			}			
private:
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
};


//======================================================


class XTOOLBOX_API VJSParms_setProperty : public VJSParms_withException
{
public:
	VJSParms_setProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ValueRef inValue, JS4D::ExceptionRef* outException)
		: VJSParms_withException( inContext, inObject, outException)
		, fPropertyName( inPropertyName)
		, fValue( inContext, inValue)
		{
		#if VERSIONDEBUG
			JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
		#endif
		}
	
			bool				GetPropertyName( XBOX::VString& outPropertyName) const	{ return JS4D::StringToVString( fPropertyName, outPropertyName);}
			bool				GetPropertyNameAsLong( sLONG *outValue) const			{ return JS4D::StringToLong( fPropertyName, outValue);}

			const VJSValue&		GetPropertyValue() const								{ return fValue;}
			
			XBOX::VValueSingle*	CreatePropertyVValue(bool simpleDate) const							{ return fValue.CreateVValue( fException, simpleDate);}

			// returns the private data of the property if it is a native object of class NATIVE_CLASS, NULL if it is not.
			template<class NATIVE_CLASS>
			typename NATIVE_CLASS::PrivateDataType *GetPropertyObjectPrivateData( ) const
			{
				if (fValue != NULL)
				{
					JS4D::ClassRef classRef = NATIVE_CLASS::Class();
					return static_cast<typename NATIVE_CLASS::PrivateDataType*>( JS4D::ValueIsObjectOfClass( GetContextRef(), fValue, classRef) ? JS4D::GetObjectPrivate( JS4D::ValueToObject( GetContextRef(), fValue, fException)) : NULL);
				}
				else
					return NULL;
			}

			bool				IsValueNull() const				{ return fValue.IsNull();}
			bool				IsValueNumber() const			{ return fValue.IsNumber();}
			bool				IsValueBoolean() const			{ return fValue.IsBoolean();}
			bool				IsValueString() const			{ return fValue.IsString();}
			bool				IsValueObject() const			{ return fValue.IsObject();}
			
private:
			JS4D::StringRef		fPropertyName;
			VJSValue			fValue;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
};


//======================================================


class XTOOLBOX_API VJSParms_getPropertyNames : public VJSParms_withContext
{
public:
	VJSParms_getPropertyNames( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::PropertyNameAccumulatorRef inPropertyNames)
		: VJSParms_withContext( inContext, inObject)
		, fPropertyNames( inPropertyNames)
		{
		}
	
			void	ReturnNumericalRange( size_t inBegin, size_t inEnd);
			void	AddPropertyName( const XBOX::VString& inPropertyName);

private:
	JS4D::PropertyNameAccumulatorRef	fPropertyNames;
};

//======================================================

class XTOOLBOX_API VJSParms_hasProperty : public VJSParms_withContext
{
public:
	VJSParms_hasProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName)
		: VJSParms_withContext( inContext, inObject)
		, fPropertyName( inPropertyName)
		, fReturnedBoolean(false)			// Default is to forward to parent.
		{
		#if VERSIONDEBUG
			JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
		#endif
		}

			bool				GetPropertyName( XBOX::VString& outPropertyName) const	{ return JS4D::StringToVString( fPropertyName, outPropertyName);}
			bool				GetPropertyNameAsLong( sLONG *outValue) const			{ return JS4D::StringToLong( fPropertyName, outValue);}
			const UniChar*		GetPropertyNamePtr( XBOX::VIndex *outLength) const;

			// Return true if object has the property. Returning false will "forward to parent", it will checks static values and so on.

			void				ReturnBoolean (bool inBoolean)							{ fReturnedBoolean = inBoolean;	}
			void				ReturnTrue ()											{ fReturnedBoolean = true;		}
			void				ReturnFalse ()											{ fReturnedBoolean = false;		}

			void				ForwardToParent ()										{ ReturnFalse();				}

			bool				GetReturnedBoolean ()									{ return fReturnedBoolean;		}
			
private:			
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
			bool				fReturnedBoolean;
};

//======================================================


class XTOOLBOX_API VJSParms_deleteProperty : public VJSParms_withException
{
public:
	VJSParms_deleteProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
		: VJSParms_withException( inContext, inObject, outException)
		, fPropertyName( inPropertyName)
		, fReturnedBoolean(true)			// Default is successful deletion.
		{
		#if VERSIONDEBUG
			JS4D::StringRefToUTF8CString( inPropertyName, fDebugPropertyName, sizeof( fDebugPropertyName));
		#endif
		}

			bool				GetPropertyName( XBOX::VString& outPropertyName) const	{ return JS4D::StringToVString( fPropertyName, outPropertyName);}
			bool				GetPropertyNameAsLong( sLONG *outValue) const			{ return JS4D::StringToLong( fPropertyName, outValue);}
			const UniChar*		GetPropertyNamePtr( XBOX::VIndex *outLength) const;

			void				ReturnBoolean (bool inBoolean)							{ fReturnedBoolean = inBoolean;	}
			void				ReturnTrue ()											{ fReturnedBoolean = true;		}
			void				ReturnFalse ()											{ fReturnedBoolean = false;		}

			bool				GetReturnedBoolean ()									{ return fReturnedBoolean;		}
			
private:
			JS4D::StringRef		fPropertyName;
			#if VERSIONDEBUG
			char				fDebugPropertyName[256];
			#endif
			bool				fReturnedBoolean;
};

class XTOOLBOX_API VJSParms_hasInstance : public VJSParms_withException
{
public:

	// Callback for "instanceof".
	//
	//		inObjectToTest instanceof inPossibleConstructor
	//
	// Note that you should have assert(inObjectToTest == GetObject.GetObjectRef()).

	VJSParms_hasInstance(JS4D::ContextRef inContext, JS4D::ObjectRef inPossibleConstructor, JS4D::ValueRef inObjectToTest, JS4D::ExceptionRef *outException)
	: VJSParms_withException(inContext, (JS4D::ObjectRef) inObjectToTest, outException)
	, fPossibleConstructor((JS4D::ObjectRef) inPossibleConstructor), fObjectToTest((JS4D::ObjectRef) inObjectToTest)
	{
	}

	XBOX::JS4D::ObjectRef	GetPossibleContructor ()	const	{	return fPossibleConstructor;	}	
	XBOX::JS4D::ObjectRef	GetObjectToTest ()			const	{	return fObjectToTest;	}	
			
private:

	XBOX::JS4D::ObjectRef	fPossibleConstructor;
	XBOX::JS4D::ObjectRef	fObjectToTest;
};

//======================================================

// Template for creating JSObjectCallAsFunctionCallback callbacks.

template<class PRIVATE_DATA_TYPE, void (*callback) (VJSParms_callStaticFunction &, PRIVATE_DATA_TYPE *)>
#if USE_V8_ENGINE
static void __cdecl js_callback( const v8::FunctionCallbackInfo<v8::Value>& args )
{
}
#else
static JS4D::ValueRef __cdecl js_callback (JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
{
	XBOX::VJSParms_callStaticFunction	parms(inContext, inFunction, inThis, inArgumentCount, inArguments, outException);

	callback(parms, (PRIVATE_DATA_TYPE *) JS4D::GetObjectPrivate(inThis));

	return parms.GetValueRefOrNull(inContext);
}
#endif

// Template for creating JSObjectCallAsConstructorCallback callbacks.

template<void (*constructor) (VJSParms_callAsConstructor &)>
#if USE_V8_ENGINE
static void __cdecl js_constructor( const v8::FunctionCallbackInfo<v8::Value>& args )
{
xbox::DebugMsg("js_constructor called !!\n");
	VJSParms_callAsConstructor	parms(args);
#else
static JS4D::ObjectRef __cdecl js_constructor (JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
{
	VJSParms_callAsConstructor	parms(inContext, inConstructor, inArgumentCount, inArguments, outException);

#endif
	constructor(parms);
	
#if USE_V8_ENGINE
#else
	return parms.GetConstructedObjectRef();	
#endif
}

//======================================================

// to force single instantiation of sClass static variable (doesn't work on inline static member function, that's why one need another non-member function)
#if COMPIL_GCC
#define GCC_VISIBILITY_DEFAULT __attribute__((__visibility__("default")))
#else
#define GCC_VISIBILITY_DEFAULT
#endif

template<typename DERIVED> JS4D::ClassRef JS4DGetClassRef() GCC_VISIBILITY_DEFAULT;
template<typename DERIVED> JS4D::ClassRef JS4DGetClassRef()
{
	static JS4D::ClassRef sClass = NULL;
	if (sClass == NULL)
	{
		JS4D::ClassDefinition def = {0};
		DERIVED::GetDefinition( def);
		sClass = JS4D::ClassCreate( &def);
	}
	return sClass;
}


template<class DERIVED, class PRIVATE_DATA_TYPE>
class VJSClass
{
	typedef void (*fn_initialize)( const VJSParms_initialize&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_finalize)( const VJSParms_finalize&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_getProperty)( VJSParms_getProperty&, PRIVATE_DATA_TYPE*);
	typedef bool (*fn_setProperty)( VJSParms_setProperty&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_getPropertyNames)( VJSParms_getPropertyNames&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_hasProperty)( VJSParms_hasProperty&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_deleteProperty)( VJSParms_deleteProperty&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_callStaticFunction)( VJSParms_callStaticFunction&, PRIVATE_DATA_TYPE*);
	typedef void (*fn_callAsFunction)( VJSParms_callAsFunction&);
	typedef void (*fn_callAsConstructor)( VJSParms_callAsConstructor&);
	typedef bool (*fn_hasInstance) (const VJSParms_hasInstance &);
	
public:
	typedef	JS4D::StaticValue			StaticValue;
	typedef	JS4D::StaticFunction		StaticFunction;
	typedef	JS4D::ClassDefinition		ClassDefinition;
	typedef	PRIVATE_DATA_TYPE			PrivateDataType;

	template<fn_initialize fn>
	static void __cdecl js_initialize( JS4D::ContextRef inContext, JS4D::ObjectRef inObject)
	{
		VJSParms_initialize parms( inContext, inObject);
#if  USE_V8_ENGINE
		fn( parms, (PrivateDataType*)inObject); 
#else
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject)); 
#endif
	}

	template<fn_finalize fn>
	static void __cdecl js_finalize( JS4D::ObjectRef inObject)
	{
		VJSParms_finalize parms( inObject);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
	}

	template<fn_getProperty fn>
	static JS4D::ValueRef __cdecl js_getProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
	{
		VJSParms_getProperty parms( inContext, inObject, inPropertyName, outException);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
		return parms.GetValueRefOrNull(inContext);
	}

	template<fn_setProperty fn>
	static bool __cdecl js_setProperty( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ValueRef inValue, JS4D::ExceptionRef* outException)
	{
		VJSParms_setProperty parms( inContext, inObject, inPropertyName, inValue, outException);
		return fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
	}

	template<fn_getPropertyNames fn>
	static void __cdecl js_getPropertyNames( JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::PropertyNameAccumulatorRef inPropertyNames)
	{
		VJSParms_getPropertyNames parms( inContext, inObject, inPropertyNames);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inObject));
	}
  
	template<fn_hasProperty fn>
	static bool __cdecl js_hasProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName)
	{
		VJSParms_hasProperty parms(inContext, inObject, inPropertyName);
		fn(parms, (PrivateDataType *) JS4D::GetObjectPrivate(inObject));
		return parms.GetReturnedBoolean();
	}

	template<fn_deleteProperty fn>
	static bool __cdecl js_deleteProperty(JS4D::ContextRef inContext, JS4D::ObjectRef inObject, JS4D::StringRef inPropertyName, JS4D::ExceptionRef* outException)
	{
		VJSParms_deleteProperty parms(inContext, inObject, inPropertyName, outException);
		fn(parms, (PrivateDataType *) JS4D::GetObjectPrivate(inObject));
		return parms.GetReturnedBoolean();
	}
			
	template<fn_callStaticFunction fn>
#if USE_V8_ENGINE
	static void js_callStaticFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
VJSParms_callStaticFunction parms(args);
fn(parms,NULL);
	}
#else
	static JS4D::ValueRef __cdecl js_callStaticFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{
		VJSParms_callStaticFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		fn( parms, (PrivateDataType*) JS4D::GetObjectPrivate( inThis));
		return parms.GetValueRefOrNull(inContext);
	}
#endif
	template<fn_callAsFunction fn>
#if USE_V8_ENGINE
	static void __cdecl js_callAsFunction(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
	}
#else
	static JS4D::ValueRef __cdecl js_callAsFunction( JS4D::ContextRef inContext, JS4D::ObjectRef inFunction, JS4D::ObjectRef inThis, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{
		VJSParms_callAsFunction parms( inContext, inFunction, inThis, inArgumentCount, inArguments, outException);
		fn( parms);
		return parms.GetValueRefOrNull(inContext);
	}
#endif
	template<fn_callAsConstructor fn>
#if USE_V8_ENGINE
	static void __cdecl js_callAsConstructor(const v8::FunctionCallbackInfo<v8::Value>& args)
	{
xbox::DebugMsg("js_callAsConstructor called !!!\n");
		VJSParms_callAsConstructor parms( args );
		fn(parms);
	}
#else
static JS4D::ObjectRef __cdecl js_callAsConstructor( JS4D::ContextRef inContext, JS4D::ObjectRef inConstructor, size_t inArgumentCount, const JS4D::ValueRef inArguments[], JS4D::ExceptionRef* outException)
	{
		VJSParms_callAsConstructor parms( inContext, inConstructor, inArgumentCount, inArguments, outException);
		fn( parms);
		return parms.GetConstructedObjectRef();
	}
#endif

	template<fn_hasInstance fn>
	static bool __cdecl js_hasInstance (JS4D::ContextRef inContext, JS4D::ObjectRef inPossibleConstructor, JS4D::ValueRef inObjectToTest, JS4D::ExceptionRef *outException)
	{
		VJSParms_hasInstance	parms(inContext, inPossibleConstructor, inObjectToTest, outException);
	
		return fn(parms);
	}

	static	JS4D::ClassRef Class()
	{
		return JS4DGetClassRef<DERIVED>();
	}
	static	VJSObject CreateInstance( const VJSContext& inContext, PrivateDataType *inData)
	{
#if USE_V8_ENGINE
xbox::DebugMsg("VJSClass::CreateInstance called !!!\n");

		VJSObject	newObj(inContext);
		return newObj;
#else
	return VJSObject( inContext, JS4D::MakeObject( inContext, Class(), inData));
#endif
}

};


END_TOOLBOX_NAMESPACE


#endif
