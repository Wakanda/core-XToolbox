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

#include "VJSW3CArrayBuffer.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

VJSArrayBufferObject::VJSArrayBufferObject (VSize inLength, void *inBuffer)
{
	xbox_assert(inLength >= 0);
	xbox_assert(!(inLength > 0 && inBuffer == NULL));

	if ((fBufferObject = new VJSBufferObject(inLength, inBuffer)) == NULL)

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
}

VJSArrayBufferObject::VJSArrayBufferObject (VJSBufferObject *inBufferObject)
{
	xbox_assert(inBufferObject != NULL);

	inBufferObject->Retain();
	fBufferObject = inBufferObject;
}

VJSArrayBufferObject::VJSArrayBufferObject (VSize inLength)
{
	xbox_assert(inLength >= 0);

	if ((fBufferObject = new VJSBufferObject(inLength)) == NULL) 
	
		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
}

VJSArrayBufferObject::VJSArrayBufferObject ()
{
	fBufferObject = NULL;
}

VJSArrayBufferObject::~VJSArrayBufferObject ()
{
	if (fBufferObject != NULL)

		XBOX::ReleaseRefCountable<VJSBufferObject>(&fBufferObject);	
}

void VJSArrayBufferClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSArrayBufferClass, VJSArrayBufferObject>::StaticFunction functions[] =
	{
		{	"slice",	js_callStaticFunction<_slice>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"toBuffer",	js_callStaticFunction<_toBuffer>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,									0																		},
	};

    outDefinition.className			= "ArrayBuffer";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.getProperty		= js_getProperty<_GetProperty>;
}

XBOX::VJSObject	VJSArrayBufferClass::MakeConstructor (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	arrayBufferConstructor(inContext);

	arrayBufferConstructor.MakeConstructor(Class(), js_constructor<_Construct>);

	return arrayBufferConstructor;
}

XBOX::VJSObject	VJSArrayBufferClass::NewInstance (XBOX::VJSContext inContext, VSize inLength, void *inBuffer)
{
	XBOX::VJSObject			createdObject(inContext);
	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = new VJSArrayBufferObject(inLength, inBuffer)) == NULL) {

		createdObject.SetUndefined();
		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
				
	} else {

		// Check if a fBufferObject allocation is successful.			

		if (arrayBuffer->fBufferObject == NULL) 
		
			createdObject.SetUndefined();

		else

			createdObject = VJSArrayBufferClass::CreateInstance(inContext, arrayBuffer);
		
		arrayBuffer->Release();

	}

	return createdObject;
}

void VJSArrayBufferClass::_Construct (XBOX::VJSParms_callAsConstructor &ioParms)
{
	sLONG	size;

	if (!ioParms.GetLongParam(1, &size)) {
					
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
		return;	

	}
	if (size < 0) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "1");
		return;

	}

	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = new VJSArrayBufferObject(size)) == NULL) {

		ioParms.ReturnUndefined();
		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		
	} else if (arrayBuffer->fBufferObject == NULL) {

		// Allocation of fBufferObject failed.

		ioParms.ReturnUndefined();
		arrayBuffer->Release();
		
	} else {
	
		ioParms.ReturnConstructedObject(VJSArrayBufferClass::CreateInstance(ioParms.GetContextRef(), arrayBuffer));
		arrayBuffer->Release();

	}
}

void VJSArrayBufferClass::_Initialize (const VJSParms_initialize &inParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	inArrayBuffer->Retain();
}

void VJSArrayBufferClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	inArrayBuffer->Release();
}

void VJSArrayBufferClass::_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	XBOX::VString	name;

	if (ioParms.GetPropertyName(name) && name.EqualToUSASCIICString("byteLength")) {

		if (inArrayBuffer->fBufferObject == NULL)

			ioParms.ReturnNumber<sLONG>(0);

		else 

			ioParms.ReturnNumber<VSize>(inArrayBuffer->fBufferObject->GetDataSize());

	} else 
		
		ioParms.ForwardToParent();
}

void VJSArrayBufferClass::_slice (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);

	if (inArrayBuffer->fBufferObject == NULL) {

		ioParms.ReturnNullValue();
		return;

	}

	sLONG	dataSize, begin, end, size;

	dataSize = (sLONG) inArrayBuffer->fBufferObject->GetDataSize(); 
	if (!ioParms.CountParams()) {

		begin = 0;
		end = dataSize;

	} else {

		if (!ioParms.GetLongParam(1, &begin)) {
					
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;	

		}

		if (begin < 0 && (begin = dataSize - begin) < 0)

			begin = 0;

		if (ioParms.CountParams() >= 2) {

			if (!ioParms.GetLongParam(2, &end)) {
					
				XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
				return;	

			}

			if (end > dataSize)

				end = dataSize;

			if (end < 0 && (end = dataSize - end) < 0)

				end = 0;

		} else 

			end = dataSize;

	}

	if (begin > end)

		size = 0;

	else

		size = end - begin;

	VJSArrayBufferObject	*arrayBuffer;

	if ((arrayBuffer = size ? new VJSArrayBufferObject(size) : new VJSArrayBufferObject()) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		ioParms.ReturnUndefinedValue();
				
	} else {
				
		if (!size) 
	
			ioParms.ReturnValue(VJSArrayBufferClass::CreateInstance(ioParms.GetContextRef(), arrayBuffer));

		else if (arrayBuffer->fBufferObject != NULL) {

			uBYTE	*p;

			p = (uBYTE *) inArrayBuffer->fBufferObject->GetDataPtr();
			::memcpy(arrayBuffer->fBufferObject->GetDataPtr(), &p[begin], size);
			ioParms.ReturnValue(VJSArrayBufferClass::CreateInstance(ioParms.GetContextRef(), arrayBuffer));

		} else

			ioParms.ReturnUndefinedValue();		// fBufferObject allocation failed!
			
		arrayBuffer->Release();	
		
	}
}

void VJSArrayBufferClass::_toBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer)
{
	xbox_assert(inArrayBuffer != NULL);
	
	if (inArrayBuffer->fBufferObject == NULL) {

		// If ArrayBuffer has been "neutered", return null.

		ioParms.ReturnNullValue();

	} else {

		// Created Buffer object will do retain() on inArrayBuffer->fBufferObject.

		ioParms.ReturnValue(VJSBufferClass::CreateInstance(ioParms.GetContext(), inArrayBuffer->fBufferObject));

	}
}