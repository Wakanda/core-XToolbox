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
#ifndef __VJS_MODULE__
#define __VJS_MODULE__

#include "VJSClass.h"
#include "VJSValue.h"

BEGIN_TOOLBOX_NAMESPACE

// VJSModuleState is not the state of a module but the state of the entire module system of a JavaScript execution.

class VJSModuleState : public XBOX::VObject
{
public:

	// Get module system state of current execution. Unless out of memory, this function will always succeed (return
	// non NULL). However, if JavaScript code for require() failed to load, GetRequireFunctionRef() will then always
	// return NULL.

	static VJSModuleState			*GetModuleState (XBOX::VJSContext &inContext);

	// Retrieve ObjectRef to JavaScript code for require() function.

	XBOX::JS4D::ObjectRef			GetRequireFunctionRef ()	{	return fRequireFunctionRef;	}

	// Load a script file, inURLString must be a full path URL.

	static XBOX::VError				LoadScript (const XBOX::VString &inURLString, XBOX::VURL *outURL, XBOX::VString *outScript);

private:

	static const uLONG				kSpecificKey	= ('J' << 24) | ('S' << 16) | ('M' << 8) | 'S'; 

	static XBOX::VCriticalSection	sMutex;
	static bool						sIsCodeLoaded;
	static XBOX::VURL				sRequireFunctionURL;
	static XBOX::VString			sRequireFunctionScript;

	XBOX::JS4D::ObjectRef			fRequireFunctionRef;

	
									VJSModuleState (XBOX::JS4D::ObjectRef inRequireFunctionRef);
	virtual							~VJSModuleState ();

	// Retrieve the URL string of requireFunction.js. Currently only implemented for Wakanda Studio.
	// ** (Wakanda Server uses different path) **.

	static void						_GetRequireFunctionURL (XBOX::VString *outURLString);	
};

// The require object is implemented using both C++ and JavaScript. The require() function is implemented using JavaScript 
// (loaded by VJSModuleState).

class XTOOLBOX_API VJSRequireClass : public XBOX::VJSClass<VJSRequireClass, void>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	MakeObject (XBOX::VJSContext &inContext);
	
private:

	static void				_Initialize (const VJSParms_initialize &inParms, void *);
	static bool				_SetProperty (XBOX::VJSParms_setProperty &ioParms, void *);
	static void				_CallAsFunction (VJSParms_callAsFunction &ioParms);

	static void				_evaluate (VJSParms_callStaticFunction &ioParms, void *);
	static void				_getCurrentPath (VJSParms_callStaticFunction &ioParms, void *);
};

END_TOOLBOX_NAMESPACE

#endif