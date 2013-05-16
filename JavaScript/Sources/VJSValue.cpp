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
#else
#include <JavaScriptCore/JavaScript.h>
#endif

#include "VJSValue.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

USING_TOOLBOX_NAMESPACE


bool VJSValue::GetString( VString& outString, JSValueRef *outException) const
{
	return (fValue != NULL) ? JS4D::ValueToString( fContext, fValue, outString, outException) : false;
}


bool VJSValue::GetLong( sLONG *outValue, JSValueRef *outException) const
{
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		sLONG l = (sLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
}


bool VJSValue::GetLong8( sLONG8 *outValue, JSValueRef *outException) const
{
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		sLONG8 l = (sLONG8) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
}


bool VJSValue::GetULong(uLONG *outValue, JS4D::ExceptionRef *outException) const
{
	if (fValue != NULL)
	{
		double r = JSValueToNumber( fContext, fValue, outException);
		uLONG l = (uLONG) r;
		if (l == r)
		{
			*outValue = l;
		}
		else
		{
			// overflow exception ?
			*outValue = 0;
		}
	}
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
}



bool VJSValue::GetReal( Real *outValue, JSValueRef *outException) const
{
	if (fValue != NULL)
		*outValue = JSValueToNumber( fContext, fValue, outException);
	return (fValue != NULL) && ( (outException == NULL) || (*outException == NULL) );
}


bool VJSValue::GetBool( bool *outValue, JSValueRef *outException) const
{
	if (fValue != NULL)
		*outValue = JSValueToBoolean( fContext, fValue);
	return (fValue != NULL);
}


bool VJSValue::GetTime( VTime& outTime, JS4D::ExceptionRef *outException) const
{
	bool ok;
	if (JS4D::ValueIsInstanceOf( fContext, fValue, CVSTR( "Date"), outException))
	{
		JSObjectRef dateObject = JSValueToObject( fContext, fValue, outException);
		ok = JS4D::DateObjectToVTime( fContext, dateObject, outTime, outException, false);
	}
	else
	{
		outTime.SetNull( true);
		ok = false;
	}
	return ok;
}


bool VJSValue::GetDuration( VDuration& outDuration, JS4D::ExceptionRef *outException) const
{
	return JS4D::ValueToVDuration( fContext, fValue, outDuration, outException);
}


bool VJSValue::GetURL( XBOX::VURL& outURL, JS4D::ExceptionRef *outException) const
{
	VString path;
	if (!GetString( path, outException) && !path.IsEmpty())
		return false;
	
	JS4D::GetURLFromPath( path, outURL);

	return true;
}


VFile *VJSValue::GetFile( JS4D::ExceptionRef *outException) const
{
	JS4DFileIterator *fileIter = GetObjectPrivateData<VJSFileIterator>( outException);
	return (fileIter == NULL) ? NULL : fileIter->GetFile();
}


VFolder* VJSValue::GetFolder( JS4D::ExceptionRef *outException) const
{
	JS4DFolderIterator *folderIter = GetObjectPrivateData<VJSFolderIterator>( outException);
	return (folderIter == NULL) ? NULL : folderIter->GetFolder();
}


bool VJSValue::GetJSONValue( VJSONValue& outValue, JS4D::ExceptionRef *outException) const
{
	bool ok = JS4D::ValueToVJSONValue( fContext, fValue, outValue, outException);
	return ok;
}

void VJSValue::GetObject( VJSObject& outObject, JS4D::ExceptionRef *outException) const
{
	outObject.SetObjectRef( ((fValue != NULL) && JSValueIsObject( fContext, fValue)) ? JSValueToObject( fContext, fValue, outException) : NULL);
}


VJSObject VJSValue::GetObject( JS4D::ExceptionRef *outException) const
{
	VJSObject resultObj(fContext);
	if ((fValue != NULL) && JSValueIsObject( fContext, fValue))
		resultObj.SetObjectRef( JSValueToObject( fContext, fValue, outException));
	else
		//resultObj.SetObjectRef(JSValueToObject(fContext, JSValueMakeNull(fContext), outException));
		resultObj.MakeEmpty();
	return resultObj;
}


bool VJSValue::IsFunction() const
{
	JSObjectRef obj = ( (fValue != NULL) && JSValueIsObject( fContext, fValue) ) ? JSValueToObject( fContext, fValue, NULL) : NULL;
	return (obj != NULL) ? JSObjectIsFunction( fContext, obj) : false;
}


//======================================================


VJSObject::VJSObject( const VJSObject& inParent, const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes):fContext(inParent.fContext) // build an empty object as a property of a parent object
{
	MakeEmpty();
	inParent.SetProperty(inPropertyName, *this, inAttributes);
}


VJSObject::VJSObject( const VJSArray& inParent, sLONG inPos):fContext(inParent.GetContextRef()) // build an empty object as an element of an array (-1 means push at the end)
{
	MakeEmpty();
	if (inPos == -1)
		inParent.PushValue(*this);
	else
		inParent.SetValueAt(inPos, *this);
}

void VJSObject::SetNullProperty( const XBOX::VString& inPropertyName, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNull();
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const XBOX::VValueSingle* inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	VJSValue jsval(fContext);
	if (inValue == NULL || inValue->IsNull())
		jsval.SetNull();
	else
		jsval.SetVValue(*inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, sLONG inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}

void VJSObject::SetProperty( const XBOX::VString& inPropertyName, sLONG8 inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, Real inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	VJSValue jsval(fContext);
	jsval.SetNumber(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const XBOX::VString& inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	VJSValue jsval(fContext);
	jsval.SetString(inValue);
	SetProperty(inPropertyName, jsval, inAttributes, outException);
}



bool VJSObject::HasProperty( const VString& inPropertyName) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	bool ok = JSObjectHasProperty( fContext, fObject, jsName);
	JSStringRelease( jsName);
	return ok;
}


void VJSObject::SetProperty( const VString& inPropertyName, const VJSValue& inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inValue, inAttributes, outException);
	JSStringRelease( jsName);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSObject& inObject, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inObject, inAttributes, outException);
	JSStringRelease( jsName);
}


void VJSObject::SetProperty( const XBOX::VString& inPropertyName, const VJSArray& inArray, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, inArray, inAttributes, outException);
	JSStringRelease( jsName);
}


void VJSObject::SetProperty( const VString& inPropertyName, bool inBoolean, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString(inPropertyName);
	JSObjectSetProperty( fContext, fObject, jsName, JSValueMakeBoolean(fContext, inBoolean), inAttributes, outException);
	JSStringRelease(jsName);
}

VJSValue VJSObject::GetProperty( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSValueRef valueRef = JSObjectGetProperty( fContext, fObject, jsName, outException);
	JSStringRelease( jsName);
	return VJSValue( fContext, valueRef);
}


VJSObject VJSObject::GetPropertyAsObject( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	JSValueRef value = JSObjectGetProperty( fContext, fObject, jsName, outException);
	JSObjectRef objectRef = ((value != NULL) && JSValueIsObject( fContext, value)) ? JSValueToObject( fContext, value, outException) : NULL;
	JSStringRelease( jsName);
	return VJSObject( fContext, objectRef);
}


bool VJSObject::GetPropertyAsBool(const VString& inPropertyName, JS4D::ExceptionRef *outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, outException));
	bool res = false;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetBool(&res, outException);
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


sLONG VJSObject::GetPropertyAsLong(const VString& inPropertyName, JS4D::ExceptionRef *outException, bool* outExists) const
{
		VJSValue result(GetProperty(inPropertyName, outException));
	sLONG res = 0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetLong(&res, outException);
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


bool VJSObject::GetPropertyAsString(const VString& inPropertyName, JS4D::ExceptionRef *outException, VString& outResult) const
{
	bool ok = false;
	VJSValue result(GetProperty(inPropertyName, outException));
	outResult.Clear();
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetString(outResult, outException);
	}
	return ok;
}



Real VJSObject::GetPropertyAsReal(const VString& inPropertyName, JS4D::ExceptionRef *outException, bool* outExists) const
{
	VJSValue result(GetProperty(inPropertyName, outException));
	Real res = 0.0;
	bool ok = false;
	if (!result.IsNull() && !result.IsUndefined())
	{
		ok = result.GetReal(&res, outException);
	}
	if (outExists != NULL)
		*outExists = ok;
	return res;
}


void VJSObject::GetPropertyNames( std::vector<VString>& outNames) const
{
	JSPropertyNameArrayRef array = JSObjectCopyPropertyNames( fContext, fObject);
	if (array != NULL)
	{
		size_t size = JSPropertyNameArrayGetCount( array);
		try
		{
			outNames.resize( size);
			size = 0;
			for( std::vector<VString>::iterator i = outNames.begin() ; i != outNames.end() ; ++i)
			{
				JS4D::StringToVString( JSPropertyNameArrayGetNameAtIndex( array, size++), *i);
			}
		}
		catch(...)
		{
			outNames.clear();
		}
		JSPropertyNameArrayRelease( array);
	}
	else
	{
		outNames.clear();
	}
}


bool VJSObject::DeleteProperty( const VString& inPropertyName, JS4D::ExceptionRef *outException) const
{
	JSStringRef jsName = JS4D::VStringToString( inPropertyName);
	bool ok = JSObjectDeleteProperty( fContext, fObject, jsName, outException);
	JSStringRelease( jsName);
	return ok;
}


bool VJSObject::CallFunction (
	const VJSObject &inFunctionObject, 
	const std::vector<VJSValue> *inValues, 
	VJSValue *outResult, 
	JS4D::ExceptionRef *outException)
{
	bool ok = true;
	
	JS4D::ExceptionRef exception = NULL;
	JSValueRef result = NULL;

	if (inFunctionObject.IsFunction())
	{
		size_t count = ( (inValues == NULL) || inValues->empty() ) ? 0 : inValues->size();
		JSValueRef *values = (count == 0) ? NULL : new JSValueRef[count];
		if (values != NULL)
		{
			size_t j = 0;
			for( std::vector<VJSValue>::const_iterator i = inValues->begin() ; i != inValues->end() ; ++i)
				values[j++] = i->GetValueRef();
		}
		else
		{
			ok = (count == 0);
		}

		result = JSObjectCallAsFunction( fContext, inFunctionObject, fObject, count, values, &exception);

		delete[] values;
	}
	else
	{
		ok = false;
	}

	if (exception != NULL)
		ok = false;
	
	if (outException != NULL)
		*outException = exception;
	
	if (outResult != NULL)
	{
		if (result == NULL)
			outResult->SetUndefined();
		else
			outResult->SetValueRef( result);
	}
	
	return ok;
}

bool VJSObject::CallAsConstructor (const std::vector<VJSValue> *inValues, VJSObject *outCreatedObject, JS4D::ExceptionRef *outException)
{
	xbox_assert(outCreatedObject != NULL);

	if (!JSObjectIsConstructor(GetContextRef(), GetObjectRef()))

		return false;

	uLONG		argumentCount	= inValues == NULL || inValues->empty() ? 0 : (uLONG) inValues->size();
	JSValueRef	*values			= argumentCount == 0 ? NULL : new JSValueRef[argumentCount];

	if (values != NULL) {

		std::vector<VJSValue>::const_iterator	i;
		uLONG									j;

		for (i = inValues->begin(), j = 0; i != inValues->end(); i++, j++)

			values[j] = i->GetValueRef();
		

	}

	JS4D::ExceptionRef	exception;
	JS4D::ObjectRef		createdObject;
	
	exception = NULL;
	createdObject = JSObjectCallAsConstructor(GetContextRef(), GetObjectRef(), argumentCount, values, &exception);
		
	if (values != NULL)

		delete[] values;

	if (outException != NULL)

		*outException = exception;

	outCreatedObject->SetContext(fContext);
	if (exception != NULL || createdObject == NULL) {
		
		outCreatedObject->SetUndefined();
		return false;

	} else {

		outCreatedObject->SetObjectRef(createdObject);
		return true;

	}
}

void VJSObject::MakeEmpty()
{
	fObject = JSObjectMake( fContext, NULL, NULL);
}

void VJSObject::MakeCallback( JS4D::ObjectCallAsFunctionCallback inCallbackFunction)
{
	JS4D::ContextRef	contextRef;
	
	if ((contextRef = fContext) != NULL && inCallbackFunction != NULL) 

		// Use NULL for function name (anonymous).

		fObject = JSObjectMakeFunctionWithCallback( contextRef, NULL, inCallbackFunction);

	else

		SetNull();
}

void VJSObject::MakeConstructor( JS4D::ClassRef inClassRef, JS4D::ObjectCallAsConstructorCallback inConstructor)
{
	JS4D::ContextRef	contextRef;
		
	if ((contextRef = fContext) != NULL && inClassRef != NULL && inConstructor != NULL) 
		
		fObject = JSObjectMakeConstructor( contextRef, inClassRef, inConstructor);

	else

		SetNull();
}

XBOX::VJSObject VJSObject::GetPrototype (const XBOX::VJSContext &inContext) 
{
	if (IsObject())

		return XBOX::VJSObject( inContext, JS4D::ValueToObject( inContext, JSObjectGetPrototype(inContext, fObject), NULL));

	else

		return XBOX::VJSObject( inContext, NULL);
}

//======================================================

VJSPropertyIterator::VJSPropertyIterator( const VJSObject& inObject)
: fObject( inObject)
, fIndex( 0)
{
	fNameArray = (fObject != NULL) ? JSObjectCopyPropertyNames( inObject.GetContextRef(), fObject) : NULL;
	fCount = (fNameArray != NULL) ? JSPropertyNameArrayGetCount( fNameArray) : 0;
}


VJSPropertyIterator::~VJSPropertyIterator()
{
	if (fNameArray != NULL)
		JSPropertyNameArrayRelease( fNameArray);
}


bool VJSPropertyIterator::GetPropertyNameAsLong( sLONG *outValue) const
{
	if (testAssert( fIndex < fCount))
		return JS4D::StringToLong( JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outValue);
	else
		return false;
}


void VJSPropertyIterator::GetPropertyName( VString& outName) const
{
	if (testAssert( fIndex < fCount))
		JS4D::StringToVString( JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outName);
	else
		outName.Clear();
}


VJSObject VJSPropertyIterator::GetPropertyAsObject( JS4D::ExceptionRef *outException) const
{
	JS4D::ValueRef value = _GetProperty(  outException);
	return VJSObject( fObject.GetContextRef(), ((value != NULL) && JSValueIsObject( fObject.GetContextRef(), value)) ? JSValueToObject( fObject.GetContextRef(), value, outException) : NULL);
}


JS4D::ValueRef VJSPropertyIterator::_GetProperty( JS4D::ExceptionRef *outException) const
{
	JSValueRef value;
	if (testAssert( fIndex < fCount))
		value = JSObjectGetProperty( fObject.GetContextRef(), fObject, JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), outException);
	else
		value = NULL;
	return value;
}


void VJSPropertyIterator::_SetProperty( JS4D::ValueRef inValue, JS4D::PropertyAttributes inAttributes, JS4D::ExceptionRef *outException) const
{
	if (testAssert( fIndex < fCount))
		JSObjectSetProperty( fObject.GetContextRef(), fObject, JSPropertyNameArrayGetNameAtIndex( fNameArray, fIndex), inValue, inAttributes, outException);
}


//======================================================

VJSArray::VJSArray( JS4D::ContextRef inContext, JS4D::ExceptionRef *outException)
: fContext( inContext)
{
	// build an array
	fObject = JSObjectMakeArray( fContext, 0, NULL, outException);
}


VJSArray::VJSArray( JS4D::ContextRef inContext, const std::vector<VJSValue>& inValues, JS4D::ExceptionRef *outException)
: fContext( inContext)
{
	// build an array
	if (inValues.empty())
	{
		#if NEW_WEBKIT
		fObject = JSObjectMakeArray( fContext, 0, NULL, outException);
		#else
		fObject = NULL;
		#endif
	}
	else
	{
		JSValueRef *values = new JSValueRef[inValues.size()];
		if (values != NULL)
		{
			size_t j = 0;
			for( std::vector<VJSValue>::const_iterator i = inValues.begin() ; i != inValues.end() ; ++i)
				values[j++] = i->GetValueRef();
		}
		#if NEW_WEBKIT
		fObject = JSObjectMakeArray( fContext, inValues.size(), values, outException);
		#else
		fObject = NULL;
		#endif
		delete[] values;
	}
}


VJSArray::VJSArray( const VJSObject& inArrayObject, bool inCheckInstanceOfArray)
: fContext( inArrayObject.GetContextRef())
{
	if (inCheckInstanceOfArray && !inArrayObject.IsInstanceOf( CVSTR( "Array")) )
		fObject = NULL;
	else
		fObject = inArrayObject.GetObjectRef();
}


VJSArray::VJSArray( const VJSValue& inArrayValue, bool inCheckInstanceOfArray)
: fContext( inArrayValue.GetContextRef())
{
	if (inCheckInstanceOfArray && !inArrayValue.IsInstanceOf( CVSTR( "Array")) )
		fObject = NULL;
	else
	{
		VJSObject jobj(inArrayValue.GetObject());
		inArrayValue.GetObject(jobj);
		fObject = jobj.GetObjectRef();
	}
}



size_t VJSArray::GetLength() const
{
	size_t length;
	JSStringRef jsString = JSStringCreateWithUTF8CString( "length");
	JSValueRef result = JSObjectGetProperty( fContext, fObject, jsString, NULL);
	JSStringRelease( jsString);
	if (testAssert( result != NULL))
	{
		double r = JSValueToNumber( fContext, result, NULL);
		length = (size_t) r;
	}
	else
	{
		length = 0;
	}
	return length;
}


VJSValue VJSArray::GetValueAt( size_t inIndex, JS4D::ExceptionRef *outException) const
{
	return VJSValue( fContext, JSObjectGetPropertyAtIndex( fContext, fObject, inIndex, outException));
}


void VJSArray::SetValueAt( size_t inIndex, const VJSValue& inValue, JS4D::ExceptionRef *outException) const
{
	JSObjectSetPropertyAtIndex( fContext, fObject, inIndex, inValue, outException);
}


void VJSArray::PushValue( const VJSValue& inValue, JS4D::ExceptionRef *outException) const
{
	SetValueAt( GetLength(), inValue, outException);
}


void VJSArray::PushValue( const VValueSingle& inValue, JS4D::ExceptionRef *outException) const
{
	VJSValue val(fContext);
	val.SetVValue(inValue, outException);
	PushValue(val, outException);
}

template<class Type>
void VJSArray::PushNumber( Type inValue , JS4D::ExceptionRef *outException) const
{
	VJSValue val(fContext);
	val.SetNumber(inValue, outException);
	PushValue(val, outException);
}


void VJSArray::PushString( const VString& inValue , JS4D::ExceptionRef *outException) const
{
	VJSValue val(fContext);
	val.SetString(inValue, outException);
	PushValue(val, outException);
}


void VJSArray::PushFile( XBOX::VFile *inFile, JS4D::ExceptionRef *outException) const
{
	VJSValue val(fContext);
	val.SetFile( inFile, outException);
	PushValue(val, outException);
}


void VJSArray::PushFolder( XBOX::VFolder *inFolder, JS4D::ExceptionRef *outException) const
{
	VJSValue val(fContext);
	val.SetFolder( inFolder, outException);
	PushValue(val, outException);
}


void VJSArray::PushFilePathAsFileOrFolder( const XBOX::VFilePath& inPath, JS4D::ExceptionRef *outException) const
{
	VJSValue val(fContext);
	val.SetFilePathAsFileOrFolder( inPath, outException);
	PushValue(val, outException);
}


void VJSArray::PushValues( const std::vector<const XBOX::VValueSingle*> inValues, JS4D::ExceptionRef *outException) const
{
	for (std::vector<const XBOX::VValueSingle*>::const_iterator cur = inValues.begin(), end = inValues.end(); cur != end; cur++)
	{
		const VValueSingle* val = *cur;
		if (val == NULL)
		{
			VJSValue xval(fContext);
			xval.SetNull();
			PushValue(xval, outException);
		}
		else
			PushValue(*val, outException);
	}
}


void VJSArray::_Splice( size_t inWhere, size_t inCount, const std::vector<VJSValue> *inValues, JS4D::ExceptionRef *outException) const
{
    JSStringRef jsString = JSStringCreateWithUTF8CString( "splice");
	JSValueRef splice = JSObjectGetProperty( fContext, fObject, jsString, outException);
	JSObjectRef spliceFunction = JSValueToObject( fContext, splice, outException);
    JSStringRelease( jsString);

	JSValueRef *values = new JSValueRef[2 + ((inValues != NULL) ? inValues->size() : 0)];
	if (values != NULL)
	{
		size_t j = 0;
		values[j++] = JSValueMakeNumber( fContext, inWhere);
		values[j++] = JSValueMakeNumber( fContext, inCount);
		if (inValues != NULL)
		{
			for( std::vector<VJSValue>::const_iterator i = inValues->begin() ; i != inValues->end() ; ++i)
				values[j++] = i->GetValueRef();
		}
	
		JSObjectCallAsFunction( fContext, spliceFunction, fObject, j, values, outException);
	}
	delete[] values;
}


// -------------------------------------------------------------------


VJSPictureContainer::VJSPictureContainer(XBOX::VValueSingle* inPict,/* bool ownsPict,*/ JS4D::ContextRef inContext)
{
	assert(inPict == NULL || inPict->GetValueKind() == VK_IMAGE);
	fPict = (VPicture*)inPict;
	fMetaInfoIsValid = false;
	//fOwnsPict = ownsPict;
	fContext = inContext;
	fMetaBag = NULL;
}


VJSPictureContainer::~VJSPictureContainer()
{
	if (/*fOwnsPict &&*/ fPict != NULL)
		delete fPict;
	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
	QuickReleaseRefCountable(fMetaBag);
}


void VJSPictureContainer::SetMetaInfo(JS4D::ValueRef inMetaInfo, JS4D::ContextRef inContext)
{
#if !VERSION_LINUX

	if (fMetaInfoIsValid)
	{
		JS4D::UnprotectValue(fContext, fMetaInfo);
	}
	JS4D::ProtectValue(inContext, inMetaInfo);
	fMetaInfo = inMetaInfo;
	fContext = inContext;
	fMetaInfoIsValid = true;

#else

	// Postponed Linux Implementation !
	vThrowError(VE_UNIMPLEMENTED);
	xbox_assert(false);

#endif
}


const XBOX::VValueBag* VJSPictureContainer::RetainMetaBag()
{
#if !VERSION_LINUX

	if (fMetaBag == NULL)
	{
		if (fPict != NULL)
		{
			const VPictureData* picdata = fPict->RetainNthPictData(1);
			if (picdata != NULL)
			{
				fMetaBag = picdata->RetainMetadatas();
				picdata->Release();
			}
		}
	}
	return RetainRefCountable(fMetaBag);

#else

	// Postponed Linux Implementation !
	vThrowError(VE_UNIMPLEMENTED);
	xbox_assert(false);

	return NULL;

#endif
}


void VJSPictureContainer::SetMetaBag(const XBOX::VValueBag* metaBag)
{
	QuickReleaseRefCountable(fMetaBag);
	fMetaBag = RetainRefCountable(metaBag);
}


// -------------------------------------------------------------------

VJSFunction::~VJSFunction()
{
	ClearParamsAndResult();
}


void VJSFunction::ClearParamsAndResult()
{
	for( std::vector<VJSValue>::const_iterator i = fParams.begin() ; i != fParams.end() ; ++i)
		i->Unprotect();
	fParams.clear();

	fResult.Unprotect();
	fResult.SetValueRef( NULL);
	
	JS4D::UnprotectValue( fContext, fException);
	fException = NULL;
}


void VJSFunction::AddParam( const VValueSingle* inVal)
{
	VJSValue val(fContext);
	if (inVal == NULL)
		val.SetNull();
	else
		val.SetVValue(*inVal);
	AddParam( val);
}


void VJSFunction::AddParam( const VJSValue& inVal)
{
	inVal.Protect();
	fParams.push_back( inVal);
}


void VJSFunction::AddParam( const VJSONValue& inVal)
{
	VJSValue val( fContext);
	val.SetJSONValue( inVal);
	AddParam( val);
}


void VJSFunction::AddParam( const VString& inVal)
{
	VJSValue val(fContext);
	val.SetString(inVal);
	AddParam( val);
}


void VJSFunction::AddParam(sLONG inVal)
{
	VJSValue val( fContext);
	val.SetNumber(inVal);
	AddParam( val);
}


void VJSFunction::AddUndefinedParam()
{
	VJSValue val( fContext);
	val.SetUndefined();
	AddParam( val);
}


void VJSFunction::AddNullParam()
{
	VJSValue val( fContext);
	val.SetNull();
	AddParam( val);
}


void VJSFunction::AddBoolParam( bool inVal)
{
	VJSValue val( fContext);
	val.SetBool( inVal);
	AddParam( val);
}


void VJSFunction::AddLong8Param( sLONG8 inVal)
{
	VJSValue val( fContext);
	val.SetNumber( inVal);
	AddParam( val);
}


bool VJSFunction::Call()
{
	bool called = false;

	VJSObject functionObject( fFunctionObject);
	
	if (!functionObject.IsObject() && !fFunctionName.IsEmpty())
	{
		VJSValue result( fContext);
		fContext.EvaluateScript( fFunctionName, NULL, &result, NULL, NULL);
		functionObject = result.GetObject();
	}

	if (functionObject.IsFunction())
	{
		JS4D::ExceptionRef exception = NULL;

		VJSValue result( fContext);
		fContext.GetGlobalObject().CallFunction( functionObject, &fParams, &result, &exception);

		result.Protect();	// protect on stack before storing it in class field
		JS4D::ProtectValue( fContext, exception);

		fResult.Unprotect();	// unprotect previous result we may have if Call() is being called twice.
		JS4D::UnprotectValue( fContext, fException);
		
		fResult = result;
		fException = exception;
		called = fException == NULL;
	}

	return called;
}

bool VJSFunction::GetResultAsBool() const
{
	bool res = false;
	fResult.GetBool(&res);
	return res;
}

sLONG VJSFunction::GetResultAsLong() const
{
	sLONG res = 0;
	fResult.GetLong(&res);
	return res;
}


sLONG8 VJSFunction::GetResultAsLong8() const
{
	sLONG8 res = 0;
	fResult.GetLong8(&res);
	return res;
}


void VJSFunction::GetResultAsString(VString& outResult) const
{
	fResult.GetString(outResult);
}


bool VJSFunction::GetResultAsJSONValue( VJSONValue& outResult) const
{
	if (fException != NULL)
	{
		outResult.SetUndefined();
		return false;
	}
	return fResult.GetJSONValue( outResult, &fException);
}


