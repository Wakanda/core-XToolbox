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

#ifndef __VJS_W3C_ARRAY_BUFFER__
#define __VJS_W3C_ARRAY_BUFFER__

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VJSArrayBufferObject : public XBOX::IRefCountable
{
public:

					// Create an ArrayBuffer from binary data.

					VJSArrayBufferObject (VSize inLength, void *inBuffer);

					// From Buffer's object toArrayBuffer() method.

					VJSArrayBufferObject (VJSBufferObject *inBufferObject);

	VJSBufferObject	*GetBufferObject () const	{	return fBufferObject;	}


	VSize			GetDataSize () const		{	return fBufferObject != NULL ? fBufferObject->GetDataSize() : 0;	}
	void			*GetDataPtr () const		{	return fBufferObject != NULL ? fBufferObject->GetDataPtr() : 0;	}

private:

friend class VJSArrayBufferClass;

					// From ArrayBuffer's constructor.

					VJSArrayBufferObject (VSize inLength);

					// If slice() method ends up with a zero size slice, create a zero sized ArrayBuffer.

					VJSArrayBufferObject ();

	virtual			~VJSArrayBufferObject ();
	
	// ArrayBuffer objects can be "neutered" after transfer (from one process to another, see cloning). 
	// If so, fBufferObject is NULL then.

	VJSBufferObject	*fBufferObject;		
};

class XTOOLBOX_API VJSArrayBufferClass : public XBOX::VJSClass<VJSArrayBufferClass, VJSArrayBufferObject>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);

	static XBOX::VJSObject	MakeConstructor (XBOX::VJSContext inContext);

	// Create an ArrayBuffer object from binary data.

	static XBOX::VJSObject	NewInstance (XBOX::VJSContext inContext, VSize inLength, void *inBuffer);

private:

	static void				_Construct (XBOX::VJSParms_callAsConstructor &ioParms);

	static void				_Initialize (const VJSParms_initialize &inParms, VJSArrayBufferObject *inArrayBuffer);
	static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSArrayBufferObject *inArrayBuffer);	

	static void				_GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSArrayBufferObject *inArrayBuffer);

	static void				_slice (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer);
	static void				_toBuffer (XBOX::VJSParms_callStaticFunction &ioParms, VJSArrayBufferObject *inArrayBuffer);
};

END_TOOLBOX_NAMESPACE

#endif