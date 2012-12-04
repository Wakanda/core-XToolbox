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

#include "VJSModule.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

XBOX::VCriticalSection	VJSModuleState::sMutex;
bool					VJSModuleState::sIsCodeLoaded;
XBOX::VURL				VJSModuleState::sRequireFunctionURL;
XBOX::VString			VJSModuleState::sRequireFunctionScript;

VJSModuleState *VJSModuleState::GetModuleState (XBOX::VJSContext &inContext)
{
	VJSModuleState	*moduleState;

	if ((moduleState = (VJSModuleState *) inContext.GetGlobalObjectPrivateInstance()->GetSpecific((VJSSpecifics::key_type) kSpecificKey)) == NULL) {

		XBOX::VError	error;
		XBOX::VString	urlString;
		
		error = XBOX::VE_OK;
		_GetRequireFunctionURL(&urlString);

		// If needed, load the JavaScript code of the require() function.

		if (!sIsCodeLoaded) {

			// Avoid race condition.

			XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
		
			if (!sIsCodeLoaded) {

				error = LoadScript(urlString, &sRequireFunctionURL, &sRequireFunctionScript);

				// Even if loading fails, set boolean as true, so as not to try again.

				sIsCodeLoaded = true;

			}
		
		}

		// Make require() function.

		XBOX::JS4D::ObjectRef	requireFunctionRef	= NULL;

		if (error != XBOX::VE_OK) {

			if (error == XBOX::VE_JVSC_SCRIPT_NOT_FOUND)

				XBOX::vThrowError(XBOX::VE_JVSC_SCRIPT_NOT_FOUND, urlString);

			else

				XBOX::vThrowError(error);
			
		} else if (!inContext.CheckScriptSyntax(sRequireFunctionScript, &sRequireFunctionURL, NULL)) {

			// Syntax error.

			XBOX::vThrowError(XBOX::VE_JVSC_SYNTAX_ERROR, urlString);
			
		} else {
			
			XBOX::VJSValue				result(inContext);
			XBOX::JS4D::ExceptionRef	exception;

			if (!inContext.EvaluateScript(sRequireFunctionScript, &sRequireFunctionURL, &result, &exception)) {

				// Should be more specific instead.
						
				XBOX::vThrowError(XBOX::VE_JVSC_SYNTAX_ERROR, urlString);

			}

			// Must return a function object for require().

			xbox_assert(result.IsFunction());

			requireFunctionRef = (XBOX::JS4D::ObjectRef) result.GetValueRef();
			JS4D::ProtectValue(inContext, requireFunctionRef);

		}

		// Set module state.

		if ((moduleState = new VJSModuleState(requireFunctionRef)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else 

			inContext.GetGlobalObjectPrivateInstance()->SetSpecific(
				(VJSSpecifics::key_type) kSpecificKey, 
				moduleState, 
				VJSSpecifics::DestructorVObject);

	}

	return moduleState;
}

XBOX::VError VJSModuleState::LoadScript (const XBOX::VString &inURLString, XBOX::VURL *outURL, XBOX::VString *outScript)
{
	xbox_assert(outURL != NULL && outScript != NULL);

	outURL->FromFilePath(inURLString, eURL_POSIX_STYLE, CVSTR("file://"));

	XBOX::VError	error;
	XBOX::VFilePath	path;
					
	if (outURL->GetFilePath(path) && path.IsFile()) {

		XBOX::VFile			file(path);			
		XBOX::VFileStream	stream(&file);
				
		if ((error = stream.OpenReading()) == XBOX::VE_OK
		&& (error = stream.GuessCharSetFromLeadingBytes(XBOX::VTC_DefaultTextExport)) == XBOX::VE_OK) {

			stream.SetCarriageReturnMode(XBOX::eCRM_NATIVE);
			error = stream.GetText(*outScript);

		}
		stream.CloseReading();	// Ignore closing error, if any.

	} else

		error = XBOX::VE_JVSC_SCRIPT_NOT_FOUND;

	return error;
}

VJSModuleState::VJSModuleState (XBOX::JS4D::ObjectRef inRequireFunctionRef)
{
	// inRequireFunctionRef can be NULL if the require() JavaScript file didn't load properly.

	fRequireFunctionRef = inRequireFunctionRef;	
}

VJSModuleState::~VJSModuleState ()
{
}

void VJSModuleState::_GetRequireFunctionURL (XBOX::VString *outURLString)
{
	XBOX::VFolder	*folder = XBOX::VFolder::RetainSystemFolder(eFK_Executable, false);

	xbox_assert(folder != NULL);	
	
#if VERSIONMAC
	
	XBOX::VFolder	*parentFolder = folder->RetainParentFolder();
	
	XBOX::VFilePath	path(parentFolder->GetPath(), "Resources/Web Components/walib/WAF/Core/Native/requireFunction.js", FPS_POSIX);
	XBOX::VFile		file(path);

	if (!file.Exists()) {

		path.FromRelativePath(parentFolder->GetPath(), "walib/WAF/Core/Native/requireFunction.js", FPS_POSIX);

	}
	
	XBOX::ReleaseRefCountable<XBOX::VFolder>(&parentFolder);
	
#else
	
	XBOX::VFilePath	path(folder->GetPath(), "Resources/Web Components/walib/WAF/Core/Native/requireFunction.js", FPS_POSIX);
	XBOX::VFile		file(path);

	if (!file.Exists()) 

		path.FromRelativePath(folder->GetPath(), "walib/WAF/Core/Native/requireFunction.js", FPS_POSIX);
	
#endif	
	
	path.GetPosixPath(*outURLString);

	XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);
}

void VJSRequireClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSRequireClass, void>::StaticFunction functions[] =
	{
		{	"_evaluate",		js_callStaticFunction<_evaluate>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete	},
		{	"_getCurrentPath",	js_callStaticFunction<_getCurrentPath>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete	},
		{	0,					0,										0																										},
	};

    outDefinition.className			= "require";
	outDefinition.staticFunctions	= functions;
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.callAsFunction	= js_callAsFunction<_CallAsFunction>;
}

XBOX::VJSObject	VJSRequireClass::MakeObject (XBOX::VJSContext &inContext)
{
	return VJSRequireClass::CreateInstance(inContext, NULL);
}

void VJSRequireClass::_Initialize (const VJSParms_initialize &inParms, void *)
{
	XBOX::VJSObject	thisObject(inParms.GetObject());
	XBOX::VJSArray	arrayObject(inParms.GetContext());
	XBOX::VFolder	*folder = XBOX::VFolder::RetainSystemFolder(eFK_Executable, false);

	xbox_assert(folder != NULL);	
	
#if VERSIONMAC
	
	XBOX::VFolder	*parentFolder = folder->RetainParentFolder();
	
	XBOX::VFilePath	path(parentFolder->GetPath(), "Modules/", FPS_POSIX);
	
	XBOX::ReleaseRefCountable<XBOX::VFolder>(&parentFolder);
	
#else
	
	XBOX::VFilePath	path(folder->GetPath(), "Modules/", FPS_POSIX);
	
#endif	

	XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

	XBOX::VString	defaultPath;

	path.GetPosixPath(defaultPath);

	thisObject.SetProperty("paths", arrayObject, JS4D::PropertyAttributeDontDelete);
	arrayObject.PushString(defaultPath);
}

void VJSRequireClass::_CallAsFunction (VJSParms_callAsFunction &ioParms)
{
	XBOX::VString	idString;

	if (!ioParms.GetStringParam(1, idString)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	XBOX::VJSContext		context(ioParms.GetContext());
	VJSModuleState			*moduleState;
	XBOX::JS4D::ObjectRef	objectRef;

	if ((moduleState = VJSModuleState::GetModuleState(context)) == NULL
	|| (objectRef = moduleState->GetRequireFunctionRef()) == NULL)

		return;

	XBOX::VJSObject				requireFunction(ioParms.GetContext(), objectRef);
	std::vector<XBOX::VJSValue>	arguments;
	XBOX::VJSValue				result(context);
	XBOX::JS4D::ExceptionRef	exception;

	for (sLONG i = 0; i < ioParms.CountParams(); i++)
		
		arguments.push_back(ioParms.GetParamValue(i + 1));

	if (!context.GetGlobalObject().CallFunction(requireFunction, &arguments, &result, &exception)) 

		ioParms.SetException(exception);

	else 

		ioParms.ReturnValue(result);	
}

void VJSRequireClass::_evaluate (VJSParms_callStaticFunction &ioParms, void *)
{
	XBOX::VJSContext	context(ioParms.GetContext());
	XBOX::VString		fullPath;
	XBOX::VJSObject		exportsObject(context), moduleObject(context);

	if (!ioParms.GetStringParam(1, fullPath)) {
		
		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	if (!ioParms.GetParamObject(2, exportsObject)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "2");
		return;

	}

	if (!ioParms.GetParamObject(3, moduleObject)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "3");
		return;

	}

	XBOX::VError	error;
	XBOX::VURL		url;
	XBOX::VString	script;
		
	if ((error = VJSModuleState::LoadScript(fullPath, &url, &script)) != XBOX::VE_OK) {

		if (error == XBOX::VE_JVSC_SCRIPT_NOT_FOUND) {

			// Should not happen as require() does already check for existence.

			XBOX::vThrowError(XBOX::VE_JVSC_SCRIPT_NOT_FOUND, fullPath);

		} else

			XBOX::vThrowError(error);
			
	} else {

		XBOX::VFilePath	path(fullPath, FPS_POSIX);
		XBOX::VFile		*file;
			
		if ((file = new XBOX::VFile(path)) == NULL) {

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
			
		} else {

			context.GetGlobalObjectPrivateInstance()->RegisterIncludedFile(file);

			XBOX::VString				functionScript;
			XBOX::VJSValue				functionResult(context);
			XBOX::JS4D::ExceptionRef	exception;			

			// Make a function of the module script, then call it.
			
			functionScript.AppendString("(function (exports, module) {");
			functionScript.AppendString(script);
			functionScript.AppendString("})");

			if (!context.EvaluateScript(functionScript, &url, &functionResult, &exception)) {
						
				ioParms.SetException(exception);	// Syntax error.

			} else {

				xbox_assert(functionResult.IsFunction());

				XBOX::VJSObject				functionObject(context);
				std::vector<XBOX::VJSValue>	arguments;
				XBOX::VJSValue				result(context);

				functionObject.SetObjectRef((XBOX::JS4D::ObjectRef) functionResult.GetValueRef());
	
				arguments.push_back(exportsObject);
				arguments.push_back(moduleObject);

				if (!context.GetGlobalObject().CallFunction(functionObject, &arguments, &result, &exception, &path)) 

					ioParms.SetException(exception);

				else 

					ioParms.ReturnValue(result);

			}

			XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

		}

	}	
}

// Actually returns the directory of the currently executing script.

void VJSRequireClass::_getCurrentPath (VJSParms_callStaticFunction &ioParms, void *)
{
	VJSGlobalObject	*globalObject	= ioParms.GetContext().GetGlobalObjectPrivateInstance();
	
	xbox_assert(globalObject != NULL);

	XBOX::VFilePath	*path	= (XBOX::VFilePath *) globalObject->GetSpecific(VJSContext::kURLSpecificKey);
	
	xbox_assert(path != NULL);

	XBOX::VFilePath	parent;

	if (!path->GetParent(parent)) {

		ioParms.ReturnString("/");

	} else {

		XBOX::VString	posixPath;

		parent.GetPosixPath(posixPath);

		ioParms.ReturnString(posixPath);

	}
}