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

#include "VJSW3CFileSystem.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSEvent.h"
#include "VJSRuntime_file.h"

#define HIDDEN_ATTRIBUTE	(JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeReadOnly)

USING_TOOLBOX_NAMESPACE

const char *VJSLocalFileSystem::sTEMPORARY_PREFIX	= "Temporary";		// Followed by a random number.
const char *VJSLocalFileSystem::sPERSISTENT_NAME	= "Persistent";
const char *VJSLocalFileSystem::sRELATIVE_PREFIX	= "Relative";		// Followed by a path to authorize.

void VJSLocalFileSystem::AddInterfacesSupport (const XBOX::VJSContext &inContext)
{
	XBOX::VJSObject	globalObject(inContext.GetGlobalObject());

	globalObject.SetProperty("TEMPORARY", (sLONG) TEMPORARY, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty("PERSISTENT", (sLONG) PERSISTENT, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	XBOX::VJSObject	function(inContext);

	function.MakeCallback(XBOX::js_callback<void, _requestFileSystem>);
	globalObject.SetProperty("requestFileSystem", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	function.MakeCallback(XBOX::js_callback<void, _fileSystem>);
	globalObject.SetProperty("FileSystem", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	function = globalObject.GetPropertyAsObject("FileSystem");
	_addTypeConstants(function);

	function.MakeCallback(XBOX::js_callback<void, _resolveLocalFileSystemURL>);
	globalObject.SetProperty("resolveLocalFileSystemURL", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);

	function.MakeCallback(XBOX::js_callback<void, _requestFileSystemSync>);
	globalObject.SetProperty("requestFileSystemSync", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	globalObject.SetProperty("FileSystemSync", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	function = globalObject.GetPropertyAsObject("FileSystemSync");
	_addTypeConstants(function);

	function.MakeCallback(XBOX::js_callback<void, _resolveLocalFileSystemSyncURL>);
	globalObject.SetProperty("resolveLocalFileSystemSyncURL", function, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}

VJSLocalFileSystem *VJSLocalFileSystem::RetainLocalFileSystem (XBOX::VJSContext &inContext, const XBOX::VFilePath &inRelativeRoot)
{	
	VJSLocalFileSystem	*newLocalFileSystem;

	if ((newLocalFileSystem = new VJSLocalFileSystem(inContext, inRelativeRoot)) == NULL)

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	return newLocalFileSystem;
}

bool VJSLocalFileSystem::AuthorizePath (const XBOX::VFilePath &inPath)
{
	XBOX::VString	name;

	name = sRELATIVE_PREFIX;
	name.AppendString(inPath.GetPath());

	bool			isOk;
	SMap::iterator	it;
	
	isOk = true;
	for (it = fFileSystems.begin(); it != fFileSystems.end(); it++) 
		
		if (it->first.BeginsWith(name) || name.BeginsWith(it->first)) {

			isOk = false;
			break;

		}

	if (isOk) {

		VJSFileSystem	*fileSystem;

		// RetainRelativeFileSystem() will add to VJSLocalFileSystem's file system map.

		fileSystem = RetainRelativeFileSystem(inPath.GetPath());
		XBOX::ReleaseRefCountable<VJSFileSystem>(&fileSystem);

	}

	return false;
}

void VJSLocalFileSystem::InvalidPath (const XBOX::VFilePath &inPath)
{
	XBOX::VString	name;
	SMap::iterator	it;

	name = sRELATIVE_PREFIX;
	name.AppendString(inPath.GetPath());
	if ((it = fFileSystems.find(name)) == fFileSystems.end()) {

		xbox_assert(false);	// Should never happen.

	} else 

		ReleaseFileSystem(it->second);
}

VJSFileSystem *VJSLocalFileSystem::RetainTemporaryFileSystem (VSize inQuota)
{
	XBOX::VString	name;

	do {

		name = sTEMPORARY_PREFIX;
		name.AppendLong(XBOX::VSystem::Random() & 0x0fffffff);

	} while (fFileSystems.find(name) != fFileSystems.end());
		
	VJSFileSystem	*fileSystem;
	XBOX::VFilePath	root(fProjectPath, XBOX::VString(name).AppendChar('/'), XBOX::FPS_POSIX);

	if ((fileSystem = new VJSFileSystem(name, root, inQuota)) != NULL) {

		XBOX::VFolder	folder(root);
		XBOX::VError	error;

		if ((error = folder.Create() != XBOX::VE_OK)) {
		
			XBOX::vThrowError(error);
			XBOX::ReleaseRefCountable<VJSFileSystem>(&fileSystem);

		} else {

			xbox_assert(folder.Exists());
			fFileSystems[name] = fileSystem;
			
		}	

	} else

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
	
	return fileSystem;
}

VJSFileSystem *VJSLocalFileSystem::RetainPersistentFileSystem (VSize inQuota)
{	  
	VJSFileSystem	*fileSystem;
	SMap::iterator	it;
	
	if ((it = fFileSystems.find(sPERSISTENT_NAME)) == fFileSystems.end()) {
	
		XBOX::VFilePath	root(fProjectPath, XBOX::VString(sPERSISTENT_NAME).AppendChar('/'), XBOX::FPS_POSIX);

		if ((fileSystem = new VJSFileSystem(sPERSISTENT_NAME, root, inQuota)) != NULL) {
			
			XBOX::VFolder	folder(root);
			XBOX::VError	error;

			if (!folder.Exists()) {

				if ((error = folder.Create() != XBOX::VE_OK)) {
		
					XBOX::vThrowError(error);
					XBOX::ReleaseRefCountable<VJSFileSystem>(&fileSystem);

				} else {

					xbox_assert(folder.Exists());
					fFileSystems[sPERSISTENT_NAME] = fileSystem;
			
				}

			} else 

				fFileSystems[sPERSISTENT_NAME] = fileSystem;

		} else 

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	} else {

		fileSystem = it->second;
		XBOX::RetainRefCountable<VJSFileSystem>(fileSystem);

	}

	return fileSystem;
}

VJSFileSystem *VJSLocalFileSystem::RetainRelativeFileSystem (const XBOX::VString &inPath)
{
	XBOX::VString	name;
	VJSFileSystem	*fileSystem;
	SMap::iterator	it;

	name = sRELATIVE_PREFIX;
	name.AppendString(inPath);
	if ((it = fFileSystems.find(name)) == fFileSystems.end()) {
	
		XBOX::VFilePath	root(fProjectPath);

		if ((fileSystem = new VJSFileSystem(inPath, root, 0)) != NULL) {

			fFileSystems[name] = fileSystem;

#if VERSIONDEBUG

			XBOX::VFolder	folder(root);

			xbox_assert(folder.Exists());

#endif

		} else 

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	} else {

		fileSystem = it->second;
		XBOX::RetainRefCountable<VJSFileSystem>(fileSystem);

	}

	return fileSystem;
}

VJSFileSystem *VJSLocalFileSystem::RetainNamedFileSystem (const XBOX::VJSContext &inContext, const XBOX::VString &inFileSystemName)
{
	XBOX::IJSRuntimeDelegate	*rtDeleguate	= inContext.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate();
	VJSFileSystem				*fsPrivateData	= NULL;
	XBOX::VFileSystemNamespace	*fsNameSpace;

	xbox_assert(rtDeleguate != NULL);
			
	if ((fsNameSpace = rtDeleguate->RetainRuntimeFileSystemNamespace()) != NULL) {

		XBOX::VFileSystem	*fileSystem;
	
		if ((fileSystem = fsNameSpace->RetainFileSystem(inFileSystemName)) != NULL) {

			fsPrivateData = new VJSFileSystem(fileSystem);
			XBOX::ReleaseRefCountable<XBOX::VFileSystem>(&fileSystem);
		
		}

		XBOX::ReleaseRefCountable<XBOX::VFileSystemNamespace>(&fsNameSpace);

	}

	return fsPrivateData;
}

void VJSLocalFileSystem::ReleaseFileSystem (VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem !=  NULL);
	xbox_assert(!inFileSystem->GetRefCount());

	// If not already released by ReleaseAllFileSystems(), do actual release.

	SMap::iterator	it;

	if ((it = fFileSystems.find(inFileSystem->GetName())) != fFileSystems.end()) {
	
		VJSFileSystem	*fileSystem	= it->second;

		fileSystem->SetValid(false);
	
		// If a TEMPORARY file system and folder exists, then delete it along with its content.

		if (fileSystem->GetName().BeginsWith(VJSLocalFileSystem::sTEMPORARY_PREFIX, true)) {

			XBOX::VFolder	folder(fileSystem->GetRoot());
			XBOX::VError	error;

			xbox_assert(folder.Exists());
			if ((error = folder.Delete(true)) != XBOX::VE_OK)

				XBOX::vThrowError(error);

		}

		fFileSystems.erase(it);	

	}
}

void VJSLocalFileSystem::ReleaseAllFileSystems ()
{
	SMap::iterator	it;
	bool			isThrown;

	// Refcounts may be kept positive because objects referencing file systems are not garbage collected.
	// This loop will ensure that TEMPORARY file system folders are deleted.

	for (it = fFileSystems.begin(), isThrown = false; it != fFileSystems.end(); it++) {
		
		it->second->SetValid(false);
		if (it->second->GetName().BeginsWith(VJSLocalFileSystem::sTEMPORARY_PREFIX, true)) {

			XBOX::VFolder	folder(it->second->GetRoot());
			XBOX::VError	error;

			if ((error = folder.Delete(true)) != XBOX::VE_OK && !isThrown) {

				XBOX::vThrowError(error);	// Throw first error, then ignore.
				isThrown = true;

			}
	
		}

	}
	
	// This will release refcount of all file systems in map.

	fFileSystems.clear();
}

sLONG VJSLocalFileSystem::RequestFileSystem (const XBOX::VJSContext &inContext, 
	sLONG inType, VSize inQuota, XBOX::VJSObject *outResult, 
	bool inIsSync, const XBOX::VString &inFileSystemName)
{
	xbox_assert(outResult != NULL);

	VJSFileSystem	*fileSystem;
	XBOX::VError	error;

	{
		StErrorContextInstaller	context(false, true);

		switch (inType) {

			case NAMED_FS:		fileSystem = RetainNamedFileSystem(inContext, inFileSystemName); break;
			case TEMPORARY:		fileSystem = RetainTemporaryFileSystem(inQuota); break;
			case PERSISTENT:	fileSystem = RetainPersistentFileSystem(inQuota); break;
			case DATA:			fileSystem = RetainRelativeFileSystem(fProjectPath.GetPath()); break;
			
			default:			xbox_assert(false); break;

		}			
		error = context.GetLastError();
	
	} 

	if (error == XBOX::VE_OK && fileSystem != NULL) {

		xbox_assert(fileSystem != NULL);

		if (inIsSync)

			*outResult = VJSFileSystemSyncClass::GetInstance(inContext, fileSystem);

		else

			*outResult = VJSFileSystemClass::GetInstance(inContext, fileSystem);

		XBOX::ReleaseRefCountable<VJSFileSystem>(&fileSystem);

		return VJSFileErrorClass::OK;

	} else {
	
		if (fileSystem == NULL && inType == NAMED_FS) {

			// Named file system wasn't found.

			*outResult = VJSFileErrorClass::NewInstance(inContext, VJSFileErrorClass::NOT_FOUND_ERR);
			return VJSFileErrorClass::NOT_FOUND_ERR;

		} else {

			// Error is probably out of memory, but use SECURITY_ERR as there is nothing more specific.

			*outResult = VJSFileErrorClass::NewInstance(inContext, VJSFileErrorClass::SECURITY_ERR);
			return VJSFileErrorClass::SECURITY_ERR;

		}

	}
}

sLONG VJSLocalFileSystem::ResolveURL (const XBOX::VJSContext &inContext, const XBOX::VString &inURL, bool inIsSync, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);

	VJSFileSystem	*relativeFileSystem;
	XBOX::VError	error;

	{
		StErrorContextInstaller	context(false, true);

		relativeFileSystem = RetainRelativeFileSystem(fProjectPath.GetPath());
		error = context.GetLastError();
	}

	if (error == XBOX::VE_OK) {

		xbox_assert(relativeFileSystem != NULL);

		sLONG			code;
		XBOX::VFilePath	path(relativeFileSystem->GetRoot());
		bool			isFile;
		
		if ((code = relativeFileSystem->ParseURL(inURL, &path, &isFile)) == VJSFileErrorClass::OK)

			*outResult = VJSEntry::CreateObject(inContext, inIsSync, relativeFileSystem, path, isFile);

		else 
				
			*outResult = VJSFileErrorClass::NewInstance(inContext, code);

		XBOX::ReleaseRefCountable<VJSFileSystem>(&relativeFileSystem);

		return code;

	} else {

		// See comment for RequestFileSystem().

		*outResult = VJSFileErrorClass::NewInstance(inContext, VJSFileErrorClass::SECURITY_ERR);
		return VJSFileErrorClass::SECURITY_ERR;

	}
}

VJSLocalFileSystem::VJSLocalFileSystem (XBOX::VJSContext &inContext, const XBOX::VFilePath &inRelativeRoot)
{
	XBOX::VFolder	*folder;
	
	folder = inContext.GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
	fProjectPath = folder->GetPath();
	ReleaseRefCountable<XBOX::VFolder>(&folder);
}

VJSLocalFileSystem::~VJSLocalFileSystem ()
{
	xbox_assert(fFileSystems.empty());
}

void VJSLocalFileSystem::_requestFileSystem (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	_requestFileSystem(ioParms, false);
}

void VJSLocalFileSystem::_resolveLocalFileSystemURL (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	_resolveLocalFileSystemURL(ioParms, false);
}

void VJSLocalFileSystem::_requestFileSystemSync (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	_requestFileSystem(ioParms, true);
}

void VJSLocalFileSystem::_resolveLocalFileSystemSyncURL (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	_resolveLocalFileSystemURL(ioParms, true);
}

void VJSLocalFileSystem::_fileSystem (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	_requestFileSystem(ioParms, false, true);
}

void VJSLocalFileSystem::_requestFileSystem (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync, bool inFromFunction)
{	
	sLONG			type;
	XBOX::VString	fileSystemName;
	
	if (ioParms.IsStringParam(1)) {

		if (!ioParms.GetStringParam(1, fileSystemName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
			return;

		} else

			type = NAMED_FS;

	} else {

		if (!ioParms.IsNumberParam(1) || !ioParms.GetLongParam(1, &type)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "1");
			return;

		}
		if (type < FS_FIRST_TYPE || type > FS_LAST_TYPE) {

			XBOX::vThrowError(XBOX::VE_JVSC_FS_WRONG_TYPE, "1");
			return;

		}

	}
	
	sLONG	size;
	VSize	quota;
	sLONG	callbackStartIndex;

	if (type == NAMED_FS) {
		
		quota = size = 0;			// Not applicable for named file systems.
		callbackStartIndex = 2;

	} else {

		if (!ioParms.IsNumberParam(2) || !ioParms.GetLongParam(2, &size)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER, "2");
			return;

		}
		if ((quota = size) < 0) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_NUMBER_ARGUMENT, "2");
			return;

		}
		callbackStartIndex = 3;

	}

	XBOX::VJSObject	successCallback(ioParms.GetContext());
	XBOX::VJSObject	errorCallback(ioParms.GetContext());

	if (!inIsSync && !inFromFunction) {

		XBOX::VString	argumentNumber;

		if (!ioParms.IsObjectParam(callbackStartIndex) || !ioParms.GetParamFunc(callbackStartIndex, successCallback)) {

			argumentNumber.AppendLong(callbackStartIndex);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}
		if (ioParms.CountParams() >= callbackStartIndex + 1 
		&& (!ioParms.IsObjectParam(callbackStartIndex + 1) || !ioParms.GetParamFunc(callbackStartIndex + 1, errorCallback))) {

			argumentNumber.AppendLong(callbackStartIndex + 1);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

	}

	VJSWorker			*worker				= VJSWorker::RetainWorker(ioParms.GetContext());
	VJSLocalFileSystem	*localFileSystem	= worker->RetainLocalFileSystem();

	// If worker has failed to create a local file system, that error has already been reported. Ignore.

	if (localFileSystem == NULL) {

		XBOX::ReleaseRefCountable<VJSWorker>(&worker);
		return;	

	}	

	// If inFromFunction is true, this will return an asynchronous FileSystem object. 
	// We use this to implement FileSystem().

	if (inIsSync || inFromFunction) {

		sLONG			code;
		XBOX::VJSObject	resultObject(ioParms.GetContext());

		code = localFileSystem->RequestFileSystem(ioParms.GetContext(), type, quota, &resultObject, inIsSync, fileSystemName);


		if (code == VJSFileErrorClass::OK)

			ioParms.ReturnValue(resultObject);

		else

			ioParms.SetException(resultObject);

	} else {

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::RequestFS(localFileSystem, type, quota, fileSystemName, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else

			worker->QueueEvent(request);

	}
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
	XBOX::ReleaseRefCountable<VJSLocalFileSystem>(&localFileSystem);
}

void VJSLocalFileSystem::_resolveLocalFileSystemURL (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync)
{
	XBOX::VString	url;

	if (!ioParms.IsStringParam(1) || !ioParms.GetStringParam(1, url)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}

	XBOX::VJSObject	successCallback(ioParms.GetContext());
	XBOX::VJSObject	errorCallback(ioParms.GetContext());

	if (!inIsSync) {

		if (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}
		if (ioParms.CountParams() >= 3 
		&& (!ioParms.IsObjectParam(3) || !ioParms.GetParamFunc(3, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "3");
			return;

		}

	}

	VJSWorker			*worker				= VJSWorker::RetainWorker(ioParms.GetContext());
	VJSLocalFileSystem	*localFileSystem	= worker->RetainLocalFileSystem();

	// See comment for _requestFileSystem().
	
	if (localFileSystem == NULL) {

		XBOX::ReleaseRefCountable<VJSWorker>(&worker);
		return;		

	}

	if (inIsSync) {

		XBOX::VJSObject	resultObject(ioParms.GetContext());

		if (localFileSystem->ResolveURL(ioParms.GetContext(), url, true, &resultObject) == VJSFileErrorClass::OK)

			ioParms.ReturnValue(resultObject);

		else

			ioParms.SetException(resultObject);

	} else {

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::ResolveURL(localFileSystem, url, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else

			worker->QueueEvent(request);

	}	
	XBOX::ReleaseRefCountable<VJSWorker>(&worker);
	XBOX::ReleaseRefCountable<VJSLocalFileSystem>(&localFileSystem);
}

void VJSLocalFileSystem::_addTypeConstants (XBOX::VJSObject &inFileSystemObject)
{
	inFileSystemObject.SetProperty("TEMPORARY", (sLONG) TEMPORARY, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inFileSystemObject.SetProperty("PERSISTENT", (sLONG) PERSISTENT, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	inFileSystemObject.SetProperty("DATA", (sLONG) DATA, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
}

VJSFileSystem::VJSFileSystem (const XBOX::VString &inName, const XBOX::VFilePath &inRoot, VSize inQuota)
{
	fFileSystem = new XBOX::VFileSystem(inName, inRoot, true, inQuota);
	xbox_assert(fFileSystem != NULL);

	ClearObjectRefs();
}

VJSFileSystem::VJSFileSystem (XBOX::VFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	fFileSystem = XBOX::RetainRefCountable<XBOX::VFileSystem>(inFileSystem);
	ClearObjectRefs();
}

VJSFileSystem::~VJSFileSystem ()
{
	XBOX::ReleaseRefCountable<XBOX::VFileSystem>(&fFileSystem);
}

void VJSFileSystem::ClearObjectRefs (void) 
{
	fFileSystemObjectRef = fFileSystemSyncObjectRef = NULL;
	fRootEntryObjectRef = fRootEntrySyncObjectRef = NULL;
}

void VJSFileSystem::SetFileSystemObjectRef (bool inIsSync, XBOX::JS4D::ObjectRef inObjectRef)
{
	if (inIsSync)

		fFileSystemSyncObjectRef = inObjectRef;
		
	else
	
		fFileSystemObjectRef = inObjectRef;
}

void VJSFileSystem::SetRootEntryObjectRef (bool inIsSync, XBOX::JS4D::ObjectRef inObjectRef)
{
	if (inIsSync)

		fRootEntrySyncObjectRef = inObjectRef;

	else 

		fRootEntryObjectRef = inObjectRef;
}

sLONG VJSFileSystem::ParseURL (const XBOX::VString &inURL, XBOX::VFilePath *ioPath, bool *outIsFile)
{
	xbox_assert(ioPath != NULL && outIsFile != NULL);

	XBOX::VURL		url(inURL, true);
	XBOX::VFilePath	currentFolder(*ioPath);
	XBOX::VError	error;

	if ((error = fFileSystem->ParseURL(url, currentFolder, ioPath)) == XBOX::VE_OK) {

		*outIsFile = ioPath->IsFile();
		return VJSFileErrorClass::OK;

	} else

		switch (error) {

			case XBOX::VE_FS_PATH_ENCODING_ERROR:	
				
				return VJSFileErrorClass::ENCODING_ERR;

			case XBOX::VE_FS_NOT_AUTHORIZED:

				return VJSFileErrorClass::SECURITY_ERR;

			case XBOX::VE_FS_NOT_FOUND:

				return VJSFileErrorClass::NOT_FOUND_ERR;

			case XBOX::VE_FS_INVALID_STATE_ERROR:

				return VJSFileErrorClass::INVALID_STATE_ERR; 

			default: 

				xbox_assert(false);
				return VJSFileErrorClass::INVALID_STATE_ERR; 

		}
}

void VJSFileSystemClass::GetDefinition (ClassDefinition &outDefinition)
{	
    outDefinition.className		= "FileSystem";
	outDefinition.initialize	= js_initialize<_Initialize>;
	outDefinition.finalize		= js_finalize<_Finalize>;
	outDefinition.hasInstance	= js_hasInstance<_HasInstance>;
}

XBOX::VJSObject VJSFileSystemClass::GetInstance (const XBOX::VJSContext &inContext, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	// Each FileSystem object is unique.

	if (inFileSystem->GetFileSystemObjectRef(false) != NULL)

		return XBOX::VJSObject(inContext, inFileSystem->GetFileSystemObjectRef(false));

	else

		return CreateInstance(inContext, inFileSystem);
}

void VJSFileSystemClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	inFileSystem->Retain();
	inFileSystem->SetFileSystemObjectRef(false, inParms.GetObject().GetObjectRef());	// This will prevent recursion when creating root entry.

	XBOX::VJSObject	rootObject(inParms.GetContext());

	rootObject = VJSEntry::CreateObject(inParms.GetContext(), false, inFileSystem, inFileSystem->GetRoot(), false);

	inParms.GetObject().SetProperty("name", inFileSystem->GetName(), XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("root", rootObject, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
}

void VJSFileSystemClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	// Root entry will be garbaged along.

	inFileSystem->ClearObjectRefs();
	inFileSystem->Release();
}

bool VJSFileSystemClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSFileSystemClass::Class());
}

void VJSFileSystemSyncClass::GetDefinition (ClassDefinition &outDefinition)
{	
    outDefinition.className		= "FileSystemSync";
	outDefinition.initialize	= js_initialize<_Initialize>;
	outDefinition.finalize		= js_finalize<_Finalize>;
	outDefinition.hasInstance	= js_hasInstance<_HasInstance>;
}

XBOX::VJSObject VJSFileSystemSyncClass::GetInstance (const XBOX::VJSContext &inContext, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	// Each FileSystemSync object is unique.

	if (inFileSystem->GetFileSystemObjectRef(true) != NULL)

		return XBOX::VJSObject(inContext, inFileSystem->GetFileSystemObjectRef(true));

	else

		return CreateInstance(inContext, inFileSystem);
}

void VJSFileSystemSyncClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	inFileSystem->Retain();
	inFileSystem->SetFileSystemObjectRef(true, inParms.GetObject().GetObjectRef());

	XBOX::VJSObject	rootObject(inParms.GetContext());

	rootObject = VJSEntry::CreateObject(inParms.GetContext(), true, inFileSystem, inFileSystem->GetRoot(), false);

	inParms.GetObject().SetProperty("name", inFileSystem->GetName(), XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
	inParms.GetObject().SetProperty("root", rootObject, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
}

void VJSFileSystemSyncClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSFileSystem *inFileSystem)
{
	xbox_assert(inFileSystem != NULL);

	inFileSystem->ClearObjectRefs();
	inFileSystem->Release();
}

bool VJSFileSystemSyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSFileSystemSyncClass::Class());
}

XBOX::VJSObject	VJSEntry::CreateObject (
	const XBOX::VJSContext &inContext, bool inIsSync,
	VJSFileSystem *inFileSystem, const XBOX::VFilePath &inPath, bool inIsFile)
{
	xbox_assert(inFileSystem != NULL);

	bool	isRoot = false;

	if (!inIsFile) {
		
		if (inFileSystem->GetRoot() == inPath) {

			// Root entry is unique.
				
			if (inFileSystem->GetRootEntryObjectRef(inIsSync) != NULL) 

				return XBOX::VJSObject(inContext, inFileSystem->GetRootEntryObjectRef(inIsSync));

			else

				isRoot = true;

		}

	} 
	
	XBOX::VJSObject	createdObject(inContext);
	VJSEntry		*entry;

	if ((entry = new VJSEntry(inFileSystem, inPath, inIsFile, inIsSync)) == NULL) {

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		createdObject.SetNull();
		
	} else {

		if (inIsSync) {

			if (inIsFile) 

				createdObject = VJSFileEntrySyncClass::CreateInstance(inContext, entry);

			else {

				createdObject = VJSDirectoryEntrySyncClass::CreateInstance(inContext, entry);
				if (isRoot)

					inFileSystem->SetRootEntryObjectRef(inIsSync, createdObject.GetObjectRef());

			}

		} else {

			if (inIsFile) 

				createdObject = VJSFileEntryClass::CreateInstance(inContext, entry);

			else {

				createdObject = VJSDirectoryEntryClass::CreateInstance(inContext, entry);
				if (isRoot)

					inFileSystem->SetRootEntryObjectRef(inIsSync, createdObject.GetObjectRef());

			}

		}

		createdObject.SetProperty("isFile", inIsFile, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);
		createdObject.SetProperty("isDirectory", !inIsFile, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

		XBOX::VString	string, rootPath;

		if (isRoot) 

			string = "";

		else

			inPath.GetName(string);

		createdObject.SetProperty("name", string, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

		inFileSystem->GetRoot().GetPosixPath(rootPath);
		inPath.GetPosixPath(string);
		string.SubString(rootPath.GetLength(), string.GetLength() - rootPath.GetLength() + 1);		
		createdObject.SetProperty("fullPath", string, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

		XBOX::VJSValue	fileSystemObject(inContext);

		if (inIsSync) 

			fileSystemObject = VJSFileSystemSyncClass::GetInstance(inContext, inFileSystem);

		else

			fileSystemObject = VJSFileSystemClass::GetInstance(inContext, inFileSystem);

		createdObject.SetProperty("filesystem", fileSystemObject, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	}
	
	return createdObject;	
}

sLONG VJSEntry::GetMetadata (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult)
{
	sLONG		code;
	XBOX::VTime	modificationTime;

	if (!fFileSystem->IsValid())

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else if (fIsFile) {

		XBOX::VFile	file(fPath);

		if (!file.Exists()) 

			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if (file.GetTimeAttributes(&modificationTime, NULL, NULL) != XBOX::VE_OK)

			code = VJSFileErrorClass::SECURITY_ERR;

		else

			code = VJSFileErrorClass::OK;

	} else {

		XBOX::VFolder	folder(fPath);
				
		if (!folder.Exists())
						
			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if (folder.GetTimeAttributes(&modificationTime, NULL, NULL) != XBOX::VE_OK) 

			code = VJSFileErrorClass::SECURITY_ERR;

		else 

			code = VJSFileErrorClass::OK;

	}

	if (code == VJSFileErrorClass::OK)

		*outResult = VJSMetadataClass::NewInstance(inContext, modificationTime);

	else

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::MoveTo (const XBOX::VJSContext &inContext, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);
	xbox_assert(inTargetEntry != NULL && !inTargetEntry->fIsFile);

	if (!_IsNameCorrect(inNewName)) {
		
		*outResult = VJSFileErrorClass::NewInstance(inContext, VJSFileErrorClass::ENCODING_ERR);
		return VJSFileErrorClass::ENCODING_ERR;

	}
 
	sLONG			code;
	XBOX::VFilePath	sourceParent;
	XBOX::VFolder	targetFolder(inTargetEntry->fPath);
	XBOX::VFilePath	resultPath;

	if (!fFileSystem->IsValid()) 

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else if (fFileSystem != inTargetEntry->fFileSystem || _IsRoot() || !fPath.GetParent(sourceParent)) {

		// Not same file systems for source and target. 
		// Cannot move the root directory. 
		// Or unable to get parent folder (this should never happen).

		code = VJSFileErrorClass::SECURITY_ERR;	

	} else if (!fIsFile && inTargetEntry->fPath.GetPath().BeginsWith(fPath.GetPath(), true)) {
		
		// It is forbidden to move a directory into itself or one of its child.
	
		code = VJSFileErrorClass::INVALID_MODIFICATION_ERR;

	} else if (!targetFolder.Exists()) {

		// Target folder doesn't exists.

		code = VJSFileErrorClass::NOT_FOUND_ERR;

	} else {

		XBOX::VString	fullPath;

		fullPath = inTargetEntry->fPath.GetPath();
		if (inNewName.GetLength())

			fullPath.AppendString(inNewName);
		
		else {
		
			XBOX::VString	string;
			
			fPath.GetName(string);
			fullPath.AppendString(string);

		}

		XBOX::VString	filePath, folderPath;

		if (fullPath.GetUniChar(fullPath.GetLength()) == FOLDER_SEPARATOR) {
			
			fullPath.GetSubString(1, fullPath.GetLength() - 1, filePath);
			folderPath = fullPath;

		} else {

			filePath = fullPath;
			folderPath = fullPath;
			folderPath.AppendChar(FOLDER_SEPARATOR);

		}

		XBOX::VFile		file(filePath);
		XBOX::VFolder	folder(folderPath);
		bool			isOk, isRename;

		isOk = true;
		isRename = false;
		if (inTargetEntry->fPath.GetPath().EqualToString(sourceParent.GetPath())) {

			// Trying to move on itself?

			if (fIsFile) {

				if (filePath.EqualToString(fPath.GetPath())) 

					isOk = false;

			} else {

				if (folderPath.EqualToString(fPath.GetPath()))

					isOk = false;

			}

			if (isOk) 

				isRename = true;

			else
				
				code = VJSFileErrorClass::INVALID_MODIFICATION_ERR;

		} 
		
		if (isOk) {

			if (file.Exists()) {

				if (!fIsFile || file.Delete() != XBOX::VE_OK) {

					// Forbidden to replace a file by a folder.
					// Or failed to remove existing file.

					code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;
					isOk = false;

				} 

			} else if (folder.Exists()) {

				if (fIsFile || !folder.IsEmpty() || folder.Delete(false) != XBOX::VE_OK) {

					// Forbidden to replace a folder by a file.
					// If replacing a folder, it must be empty. 
					// Deletion must succeed.

					code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;
					isOk = false;

				} 

			} 

		}
			
		// Do actual move.

		if (isOk) {

			if (fIsFile) {

				XBOX::VFile	*file;

				if (isRename) {

					if (XBOX::VFile(fPath).Rename(inNewName, &file) != XBOX::VE_OK)

						code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

					else {

						code = VJSFileErrorClass::OK;
						resultPath = file->GetPath();
						XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

					}

				} else {

					XBOX::VFilePath	path(filePath); 

					if (XBOX::VFile(fPath).Move(path, &file) != XBOX::VE_OK)

						code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

					else {

						code = VJSFileErrorClass::OK;
						resultPath = file->GetPath();
						XBOX::ReleaseRefCountable<XBOX::VFile>(&file);
	
					}

				}

			} else {

				XBOX::VFolder	*folder;

				if (isRename) {

					if (XBOX::VFolder(fPath).Rename(inNewName, &folder) != XBOX::VE_OK)

						code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

					else {

						code = VJSFileErrorClass::OK;
						resultPath = folder->GetPath();
						XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

					}

				} else if (!inNewName.GetLength()) {

					if (XBOX::VFolder(fPath).Move(XBOX::VFolder(inTargetEntry->fPath), &folder) != XBOX::VE_OK)

						code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

					else {

						code = VJSFileErrorClass::OK;
						resultPath = folder->GetPath();
						XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

					}

				} else {

					// Move with a rename.

					XBOX::VFolder	*sourceFolder, *targetFolder;
					bool			nameIsChanged;

					sourceFolder = targetFolder = NULL;
					if (XBOX::VFolder(fPath).Rename(inNewName, &sourceFolder) != XBOX::VE_OK) {

						// Rename failed, may be there is already a folder with same name.

						sLONG			i;
						XBOX::VString	name;

						for (i = 0; i < 5; i++) {

							name = inNewName;
							name.AppendLong(VSystem::Random());

							if (XBOX::VFolder(fPath).Rename(name, &sourceFolder) == XBOX::VE_OK) 

								break;						

						}

						if (i != 5)

							nameIsChanged = true;

						else {

							code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;
							isOk = false;

						}

					} else

						nameIsChanged = false;

					if (isOk) {

						if (sourceFolder->Move(XBOX::VFolder(inTargetEntry->fPath), &targetFolder) != XBOX::VE_OK)

							code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

						else if (nameIsChanged) {

							if (targetFolder->Rename(inNewName, NULL) != XBOX::VE_OK) 

								code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

							else {

								code = VJSFileErrorClass::OK;
								resultPath = targetFolder->GetPath();

							}

						} else {

							code = VJSFileErrorClass::OK;
							resultPath = targetFolder->GetPath();
	
						}
						XBOX::ReleaseRefCountable<XBOX::VFolder>(&sourceFolder);
						XBOX::ReleaseRefCountable<XBOX::VFolder>(&targetFolder);

					}						

				}

			}

		}

	}

	if (code == VJSFileErrorClass::OK)

		*outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, resultPath, fIsFile);

	else

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::CopyTo (const XBOX::VJSContext &inContext, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);
	xbox_assert(inTargetEntry != NULL && !inTargetEntry->fIsFile);

	if (!_IsNameCorrect(inNewName)) {
		 
		*outResult = VJSFileErrorClass::NewInstance(inContext, VJSFileErrorClass::ENCODING_ERR);
		return VJSFileErrorClass::ENCODING_ERR;

	}
	sLONG			code;
	XBOX::VFilePath	sourceParent;
	XBOX::VFolder	targetFolder(inTargetEntry->fPath);
	XBOX::VFilePath	resultPath;
	
	if (!fFileSystem->IsValid()) 

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else if (fFileSystem != inTargetEntry->fFileSystem || !fPath.GetParent(sourceParent)) {

		// Not same file systems for source and target. Or unable to get parent folder (this should never happen).

		code = VJSFileErrorClass::SECURITY_ERR;	

	} else if (!fIsFile && inTargetEntry->fPath.GetPath().BeginsWith(fPath.GetPath(), true)) {
	
		// It is forbidden to copy a directory into itself or one of its child.

		code = VJSFileErrorClass::INVALID_MODIFICATION_ERR;

	} else if (inTargetEntry->fPath.GetPath().EqualToString(sourceParent.GetPath(), true) && !inNewName.GetLength()) {
		
		// A file or directory can be moved to its parent only in case of a rename.

		code = VJSFileErrorClass::INVALID_MODIFICATION_ERR;

	} else if (!targetFolder.Exists()) {

		// Target folder doesn't exists.

		code = VJSFileErrorClass::NOT_FOUND_ERR;

	} else {

		XBOX::VString	fullPath;

		fullPath = inTargetEntry->fPath.GetPath();
		if (inNewName.GetLength())

			fullPath.AppendString(inNewName);
		
		else {
		
			XBOX::VString	string;
			
			fPath.GetName(string);
			fullPath.AppendString(string);

		}

		XBOX::VString	filePath, folderPath;

		if (fullPath.GetUniChar(fullPath.GetLength()) == FOLDER_SEPARATOR) {
			
			fullPath.GetSubString(1, fullPath.GetLength() - 1, filePath);
			folderPath = fullPath;

		} else {

			filePath = fullPath;
			folderPath = fullPath;
			folderPath.AppendChar(FOLDER_SEPARATOR);

		}

		XBOX::VFile		file(filePath);
		XBOX::VFolder	folder(folderPath);
		bool			isOk;

		isOk = true;
		if (file.Exists()) {

			if (!fIsFile || file.Delete() != XBOX::VE_OK) {

				// Forbidden to replace a file by a copy of a folder.
				// Or failed to remove existing file.

				code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;
				isOk = false;

			} 

		} else if (folder.Exists() && fIsFile) {

			// Forbidden to replace a folder by a file.

			code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;
			isOk = false;

		} 

		// Do actual copy.

		if (isOk) {

			if (fIsFile) {
				
				XBOX::VFilePath	path(filePath); 
				XBOX::VFile		*file;

				if (XBOX::VFile(fPath).CopyTo(path, &file, FCP_Overwrite) == XBOX::VE_OK) {

					code = VJSFileErrorClass::OK;
					resultPath = file->GetPath();
					XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

				} else

					code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

			} else {

				XBOX::VFilePath	path(folderPath); 
			
				if ((folder.Exists() || folder.Create() == XBOX::VE_OK) 
				&& XBOX::VFolder(fPath).CopyContentsTo(folder, FCP_Overwrite) == XBOX::VE_OK) {

					code = VJSFileErrorClass::OK;
					resultPath = folder.GetPath();

				} else

					code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

			}

		}

	}

	if (code == VJSFileErrorClass::OK)

		*outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, resultPath, fIsFile);

	else

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::Remove (const XBOX::VJSContext &inContext, XBOX::VJSObject *outException)
{
	xbox_assert(outException != NULL);

	sLONG	code;

	if (!fFileSystem->IsValid())

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else if (fIsFile) {

		XBOX::VFile	file(fPath);
		
		if (!file.Exists())

			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if (file.Delete() != XBOX::VE_OK)

			code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;
			
		else {
		
			xbox_assert(!file.Exists());
			code = VJSFileErrorClass::OK;

		}

	} else {

		XBOX::VFolder	folder(fPath);

		if (_IsRoot()) 

			code = VJSFileErrorClass::SECURITY_ERR;	// Forbidden to remove root directory!

		else if (!folder.Exists())

			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if (!folder.IsEmpty())
		
			code = VJSFileErrorClass::INVALID_MODIFICATION_ERR;

		else if (folder.Delete(false) != XBOX::VE_OK) 

			code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

		else {

			xbox_assert(!folder.Exists());
			code = VJSFileErrorClass::OK;

		}
		
	}

	if (code != VJSFileErrorClass::OK) 

		*outException = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}
	
sLONG VJSEntry::GetParent (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);

	sLONG	code;

	if (!fFileSystem->IsValid()) {

		code = VJSFileErrorClass::INVALID_STATE_ERR;
		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	} else if (_IsRoot()) {

		// Root entry is unique.

		outResult->SetContext(inContext);
		outResult->SetObjectRef(fFileSystem->GetRootEntryObjectRef(fIsSync));

		xbox_assert(outResult->GetObjectRef() != NULL && outResult->IsObject());

		code = VJSFileErrorClass::OK;
	
	} else { 

		XBOX::VFilePath	parentPath;

		fPath.GetParent(parentPath);
		xbox_assert(fFileSystem->IsAuthorized(parentPath));

		*outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, parentPath, false);

		code = VJSFileErrorClass::OK;

	}

	return code;
}

sLONG VJSEntry::GetFile (const XBOX::VJSContext &inContext, const XBOX::VString &inURL, sLONG inFlags, XBOX::VJSObject *outResult)
{
	xbox_assert(!IsFile() && outResult != NULL);

	sLONG		code;
	VFilePath	path(GetPath());
	bool		isFile;
	
	if ((code = fFileSystem->ParseURL(inURL, &path, &isFile)) == VJSFileErrorClass::OK) {

		// Path alredy exists.

		if (inFlags == (eFLAG_CREATE | eFLAG_EXCLUSIVE))

			code = VJSFileErrorClass::PATH_EXISTS_ERR;

		else if (!isFile) {

			// Path is actually a directory.

			if (inFlags & eFLAG_CREATE) {

				XBOX::VFolder	folder(path);
				XBOX::VFile		file(path);

				xbox_assert(folder.Exists());

				// Try to remove the directory and create the file instead.

				if (path == fFileSystem->GetRoot())

					code = VJSFileErrorClass::SECURITY_ERR;

				else if (!folder.IsEmpty() || folder.Delete(false) != XBOX::VE_OK || file.Create() != XBOX::VE_OK)

					code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

				else {

					xbox_assert(!folder.Exists() && file.Exists());

				}

			} else

				code = VJSFileErrorClass::TYPE_MISMATCH_ERR;

		}

	} else if (code == VJSFileErrorClass::NOT_FOUND_ERR && (inFlags & eFLAG_CREATE)) {

		// File doesn't exists and eFLAG_CREATE is set, create it.
		// It is forbidden to create a file with no existing parent folder.

		XBOX::VFilePath	parentPath;
		
		if (path.GetParent(parentPath) && XBOX::VFolder(parentPath).Exists()) {

			XBOX::VFile	file(path);

			if (file.Create() != XBOX::VE_OK)

				code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

			else {

				xbox_assert(file.Exists());
				code = VJSFileErrorClass::OK;

			}

		}

	}

	if (code == VJSFileErrorClass::OK) 

		*outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, path, true);

	else

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::GetDirectory (const XBOX::VJSContext &inContext, const XBOX::VString &inURL, sLONG inFlags, XBOX::VJSObject *outResult)
{
	xbox_assert(!IsFile() && outResult != NULL);

	sLONG		code;
	VFilePath	path(GetPath());
	bool		isFile;

	if ((code = fFileSystem->ParseURL(inURL, &path, &isFile)) == VJSFileErrorClass::OK) {

		// Path already exists.

		if (inFlags == (eFLAG_CREATE | eFLAG_EXCLUSIVE))

			code = VJSFileErrorClass::PATH_EXISTS_ERR;

		else if (isFile) {

			// Path is actually a file.

			if (inFlags & eFLAG_CREATE) {

				XBOX::VFile		file(path);
				XBOX::VString	fullPath;

				path.GetPath(fullPath);
				fullPath.AppendChar(FOLDER_SEPARATOR);

				XBOX::VFolder	folder(fullPath);
				
				xbox_assert(file.Exists());

				// Try to remove the file and create the directory instead.
		
				if (file.Delete() != XBOX::VE_OK || folder.Create() != XBOX::VE_OK)
				
					code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

				else {

					xbox_assert(!file.Exists() && folder.Exists());

				}

			} else

				code = VJSFileErrorClass::TYPE_MISMATCH_ERR;

		}

	} else if (code == VJSFileErrorClass::NOT_FOUND_ERR && (inFlags & eFLAG_CREATE)) {

		// Folder doesn't exists and eFLAG_CREATE is set, create it.
		// Same as file, it is forbidden to create a folder with no existing parent folder.

		XBOX::VFilePath	parentPath;
		
		if (path.GetParent(parentPath) && XBOX::VFolder(parentPath).Exists()) {

			XBOX::VString	fullPath;

			path.GetPath(fullPath);
			if (fullPath.GetUniChar(fullPath.GetLength()) != FOLDER_SEPARATOR) {
				
				fullPath.AppendChar(FOLDER_SEPARATOR);
				path.FromFullPath(fullPath);

			}

			XBOX::VFolder	folder(fullPath);

			if (folder.Create() != XBOX::VE_OK)

				code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

			else {

				xbox_assert(folder.Exists());
				code = VJSFileErrorClass::OK;

			}

		}

	}

	if (code == VJSFileErrorClass::OK) 

		*outResult = VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, path, false);

	else

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::RemoveRecursively (const XBOX::VJSContext &inContext, XBOX::VJSObject *outException)
{
	xbox_assert(outException != NULL);
	xbox_assert(!fIsFile);

	sLONG	code;

	if (!fFileSystem->IsValid())

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else {

		XBOX::VFolder	folder(fPath);

		if (_IsRoot()) 

			code = VJSFileErrorClass::SECURITY_ERR;	// Forbidden to remove root directory!

		else if (!folder.Exists())

			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if (folder.Delete(true) != XBOX::VE_OK) 

			code = VJSFileErrorClass::NO_MODIFICATION_ALLOWED_ERR;

		else {

			xbox_assert(!folder.Exists());
			code = VJSFileErrorClass::OK;

		}
		
	}

	if (code != VJSFileErrorClass::OK) 

		*outException = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::Folder (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);
	xbox_assert(!fIsFile);

	sLONG				code;
	JS4DFolderIterator	*folderIterator;

	if (!fFileSystem->IsValid())

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else {

		VFolder *folder = new VFolder(fPath, fFileSystem->GetFileSystem());

		if (folder == NULL) 

			code = VJSFileErrorClass::SECURITY_ERR;
				
		else if (!folder->Exists())

			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if ((folderIterator = new JS4DFolderIterator(folder)) == NULL)

			code = VJSFileErrorClass::SECURITY_ERR;

		else

			code = VJSFileErrorClass::OK;
	
		XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

	}

	if (code == VJSFileErrorClass::OK) {

		*outResult = VJSFolderIterator::CreateInstance(inContext, folderIterator);
		XBOX::ReleaseRefCountable<JS4DFolderIterator>(&folderIterator);
		
	} else 

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

sLONG VJSEntry::CreateWriter (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);
	xbox_assert(fIsFile);

	//** TODO.
	return VJSFileErrorClass::OK;
}

sLONG VJSEntry::File (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);
	xbox_assert(fIsFile);

	sLONG				code;
	JS4DFileIterator	*fileIterator;

	if (!fFileSystem->IsValid())

		code = VJSFileErrorClass::INVALID_STATE_ERR;

	else {

		XBOX::VFile	*file = new VFile(fPath, fFileSystem->GetFileSystem());

		if (file == NULL) 

			code = VJSFileErrorClass::SECURITY_ERR;
				
		else if (!file->Exists())

			code = VJSFileErrorClass::NOT_FOUND_ERR;

		else if ((fileIterator = new JS4DFileIterator(file)) == NULL)

			code = VJSFileErrorClass::SECURITY_ERR;

		else

			code = VJSFileErrorClass::OK;
	
		XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

	}

	if (code == VJSFileErrorClass::OK) {

		*outResult = VJSFileIterator::CreateInstance(inContext, fileIterator);
		XBOX::ReleaseRefCountable<JS4DFileIterator>(&fileIterator);
		
	} else 

		*outResult = VJSFileErrorClass::NewInstance(inContext, code);

	return code;
}

VJSEntry::VJSEntry (VJSFileSystem *inFileSystem, const XBOX::VFilePath &inPath, bool inIsFile, bool inIsSync)
{
	xbox_assert(inFileSystem != NULL);

	fFileSystem = XBOX::RetainRefCountable<VJSFileSystem>(inFileSystem);
	fPath = inPath;
	fIsFile = inIsFile;
	fIsSync = inIsSync;
}

VJSEntry::~VJSEntry ()
{
	xbox_assert(fFileSystem != NULL);

	XBOX::ReleaseRefCountable<VJSFileSystem>(&fFileSystem);
}

void VJSEntry::_getMetadata (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	if (inEntry->fIsSync) {

		XBOX::VJSObject	resultObject(ioParms.GetContext());

		if (inEntry->GetMetadata(ioParms.GetContext(), &resultObject) == VJSFileErrorClass::OK)

			ioParms.ReturnValue(resultObject);

		else

			ioParms.SetException(resultObject);

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
			return;

		}
		if (ioParms.CountParams() >= 2
		&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::GetMetadata(inEntry, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_moveTo (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_moveOrCopyTo(ioParms, inEntry, true);
}

void VJSEntry::_copyTo (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_moveOrCopyTo(ioParms, inEntry, false);
}

void VJSEntry::_toURL (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	XBOX::VString	mimeType;

	if (ioParms.CountParams() && !ioParms.GetStringParam(1, mimeType)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}
 
	VIndex	rootLength, relativeSize;
	
	rootLength = inEntry->fFileSystem->GetRoot().GetPath().GetLength();

	xbox_assert(inEntry->fPath.GetPath().GetLength() >= rootLength);
	relativeSize = inEntry->fPath.GetPath().GetLength() - rootLength + 1;

	XBOX::VString	relativePath;

	if (relativeSize) 

		inEntry->fPath.GetPath().GetSubString(rootLength, relativeSize, relativePath);
	
	else

		relativePath = ".";

#if VERSIONWIN == 1 || VERSION_MAC == 1

	UniChar	*p;

	for (p = (UniChar *) relativePath.GetCPointer(); *p != 0; p++)

		if (*p == FOLDER_SEPARATOR)

			*p = '/';

#endif

	//** TODO: Add mimeType ! (What is the syntax?)

	XBOX::VString	url;

	url = "file://";
	url.AppendString(relativePath);

	ioParms.ReturnString(url);
}

void VJSEntry::_remove (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	if (inEntry->fIsSync) {

		XBOX::VJSObject	exception(ioParms.GetContext());

		if (inEntry->Remove(ioParms.GetContext(), &exception) != VJSFileErrorClass::OK) 

			ioParms.SetException(exception);

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
			return;

		}
		if (ioParms.CountParams() >= 2
		&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::Remove(inEntry, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_getParent (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	if (inEntry->fIsSync) {

		XBOX::VJSObject	resultObject(ioParms.GetContext());
		
		inEntry->GetParent(ioParms.GetContext(), &resultObject);
		ioParms.ReturnValue(resultObject);

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
			return;

		}
		if (ioParms.CountParams() >= 2
		&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::GetParent(inEntry, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_createReader (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	ioParms.ReturnValue(VJSDirectoryReader::NewReaderObject(ioParms.GetContext(), inEntry));
}

void VJSEntry::_getFile (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_getFileOrDirectory(ioParms, inEntry, true);
}

void VJSEntry::_getDirectory (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	_getFileOrDirectory(ioParms, inEntry, false);	
}

void VJSEntry::_removeRecursively (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	if (inEntry->fIsSync) {

		XBOX::VJSObject	exception(ioParms.GetContext());

		if (inEntry->RemoveRecursively(ioParms.GetContext(), &exception) != VJSFileErrorClass::OK) 

			ioParms.SetException(exception);

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
			return;

		}
		if (ioParms.CountParams() >= 2
		&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}
		
		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::RemoveRecursively(inEntry, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_folder (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	if (inEntry->fIsSync) {

		XBOX::VJSObject	resultObject(ioParms.GetContext());

		if (inEntry->Folder(ioParms.GetContext(), &resultObject))
	 
			ioParms.ReturnValue(resultObject);

		else

			ioParms.SetException(resultObject);		

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
			return;

		}
		if (ioParms.CountParams() >= 2
		&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::Folder(inEntry, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_createWriter (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && inEntry->fIsFile);

	//** TODO
}

void VJSEntry::_file (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && inEntry->fIsFile);

	if (inEntry->fIsSync) {

		XBOX::VJSObject	resultObject(ioParms.GetContext());

		if (inEntry->File(ioParms.GetContext(), &resultObject) == VJSFileErrorClass::OK)
	 
			ioParms.ReturnValue(resultObject);

		else

			ioParms.SetException(resultObject);		

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());

		if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
			return;

		}
		if (ioParms.CountParams() >= 2
		&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
			return;

		}

		VJSW3CFSEvent	*request;

		if ((request = VJSW3CFSEvent::File(inEntry, successCallback, errorCallback)) == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_moveOrCopyTo (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsMoveTo)
{
	xbox_assert(inEntry != NULL);

	XBOX::VJSObject			parentEntry(ioParms.GetContext());
	VJSEntry				*targetEntry;
	XBOX::VString			newName;

	XBOX::VError			error;
	XBOX::JS4D::ClassRef	classRef;

	if (inEntry->fIsSync) {

		error = XBOX::VE_JVSC_FS_EXPECTING_DIR_ENTRY_SYNC;
		classRef = VJSDirectoryEntryClass::Class();
		
	} else {
		
		error = XBOX::VE_JVSC_FS_EXPECTING_DIR_ENTRY;
		classRef = VJSDirectoryEntrySyncClass::Class();

	}
	
	if (!ioParms.GetParamObject(1, parentEntry) && !parentEntry.IsOfClass(classRef)) {

		XBOX::vThrowError(error, "1");
		return;

	}

	if (inEntry->fIsSync) 

		targetEntry = parentEntry.GetPrivateData<VJSDirectoryEntrySyncClass>();

	else

		targetEntry = parentEntry.GetPrivateData<VJSDirectoryEntryClass>();

	xbox_assert(targetEntry != NULL);

	// Get optional new name. An empty new name means none was specified.

	sLONG	index;

	if (ioParms.CountParams() >= 2 && ioParms.IsStringParam(2)) {
		
		if (!ioParms.GetStringParam(2, newName)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			return;

		} else if (!newName.GetLength()) {

			XBOX::vThrowError(XBOX::VE_JVSC_FS_NEW_NAME_IS_EMTPY, "2");
			return;
		
		} else

			index = 3;

	} else 

		index = 2;

	if (inEntry->fIsSync) {

		sLONG			code;
		XBOX::VJSObject	result(ioParms.GetContext());

		if (inIsMoveTo) 
		
			code = inEntry->MoveTo(ioParms.GetContext(), targetEntry, newName, &result);

		else

			code = inEntry->CopyTo(ioParms.GetContext(), targetEntry, newName, &result);

		if (code == VJSFileErrorClass::OK)

			ioParms.ReturnValue(result);

		else

			ioParms.SetException(result);

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());
		XBOX::VString	argumentNumber;

		if (ioParms.CountParams() >= index
		&& (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, successCallback))) {

			argumentNumber.FromLong(index);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

		index++;
		if (ioParms.CountParams() >= index
		&& (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, errorCallback))) {

			argumentNumber.FromLong(index);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

		VJSW3CFSEvent	*request;

		if (inIsMoveTo) 

			request = VJSW3CFSEvent::MoveTo(inEntry, targetEntry, newName, successCallback, errorCallback);

		else

			request = VJSW3CFSEvent::CopyTo(inEntry, targetEntry, newName, successCallback, errorCallback);			

		if (request == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

void VJSEntry::_getFileOrDirectory (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsGetFile)
{
	xbox_assert(inEntry != NULL && !inEntry->fIsFile);

	XBOX::VString	url;
	sLONG			flags;

	if (!ioParms.GetStringParam(1, url)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
		return;

	}
		
	sLONG	index;

	flags = 0;
	if (ioParms.CountParams() >= 2) {
		
		XBOX::VJSObject	options(ioParms.GetContext());

		if (!ioParms.GetParamObject(2, options)) {

			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT, "2");
			return;

		}

		if (!options.IsFunction()) {

			if (options.GetPropertyAsBool("create", NULL, NULL))

				flags |= eFLAG_CREATE;

			if (options.GetPropertyAsBool("exclusive", NULL, NULL))

				flags |= eFLAG_EXCLUSIVE;

			index = 3;

		} else 

			index = 2;
	
	} else 

		index = 2;

	if (inEntry->fIsSync) {

		sLONG			code;
		XBOX::VJSObject	resultObject(ioParms.GetContext());

		if (inIsGetFile)

			code = inEntry->GetFile(ioParms.GetContext(), url, flags, &resultObject);

		else

			code = inEntry->GetDirectory(ioParms.GetContext(), url, flags, &resultObject);
	
		if (code == VJSFileErrorClass::OK)

			ioParms.ReturnValue(resultObject);

		else

			ioParms.SetException(resultObject);

	} else {

		XBOX::VJSObject	successCallback(ioParms.GetContext());
		XBOX::VJSObject	errorCallback(ioParms.GetContext());
		XBOX::VString	argumentNumber;

		if (ioParms.CountParams() >= index
		&& (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, successCallback))) {

			argumentNumber.FromLong(index);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

		index++;
		if (ioParms.CountParams() >= index
		&& (!ioParms.IsObjectParam(index) || !ioParms.GetParamFunc(index, errorCallback))) {

			argumentNumber.FromLong(index);
			XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, argumentNumber);
			return;

		}

		VJSW3CFSEvent	*request;

		if (inIsGetFile) 

			request = VJSW3CFSEvent::GetFile(inEntry, url, flags, successCallback, errorCallback);

		else

			request = VJSW3CFSEvent::GetDirectory(inEntry, url, flags, successCallback, errorCallback);
			
		if (request == NULL)

			XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		else {

			VJSWorker	*worker;
			
			worker = VJSWorker::RetainWorker(ioParms.GetContext());
			worker->QueueEvent(request);
			XBOX::ReleaseRefCountable<VJSWorker>(&worker);

		}

	}
}

// Rudimentary encoding checking. Just make sure there is no directory separator if it is a file.
// Or look that directory separator is at the end for a folder. Further errors are catched when trying
// to execute the operation.

bool VJSEntry::_IsNameCorrect (const XBOX::VString &inName)
{
	VIndex	index	= inName.FindUniChar('/');

	if (!index)

		return true;

	else if (fIsFile)

		return false;

	else

		return index == inName.GetLength() && index != 1;
}

void VJSDirectoryEntryClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryEntryClass, VJSEntry>::StaticFunction functions[] =
	{
		{	"createReader",			js_callStaticFunction<VJSEntry::_createReader>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getFile",				js_callStaticFunction<VJSEntry::_getFile>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getDirectory",			js_callStaticFunction<VJSEntry::_getDirectory>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"removeRecursively",	js_callStaticFunction<VJSEntry::_removeRecursively>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"folder",				js_callStaticFunction<VJSEntry::_folder>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},

		{	"getMetadata",			js_callStaticFunction<VJSEntry::_getMetadata>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",				js_callStaticFunction<VJSEntry::_moveTo>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",				js_callStaticFunction<VJSEntry::_copyTo>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"toURL",				js_callStaticFunction<VJSEntry::_toURL>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"remove",				js_callStaticFunction<VJSEntry::_remove>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getParent",			js_callStaticFunction<VJSEntry::_getParent>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},

		{	0,						0,														0																			},
	};

    outDefinition.className			= "DirectoryEntry";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

void VJSDirectoryEntryClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Retain();
}

void VJSDirectoryEntryClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Release();
}

bool VJSDirectoryEntryClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSDirectoryEntryClass::Class());
}

void VJSDirectoryEntrySyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryEntrySyncClass, VJSEntry>::StaticFunction functions[] =
	{
		{	"createReader",			js_callStaticFunction<VJSEntry::_createReader>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getFile",				js_callStaticFunction<VJSEntry::_getFile>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getDirectory",			js_callStaticFunction<VJSEntry::_getDirectory>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"removeRecursively",	js_callStaticFunction<VJSEntry::_removeRecursively>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"folder",				js_callStaticFunction<VJSEntry::_folder>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},

		{	"getMetadata",			js_callStaticFunction<VJSEntry::_getMetadata>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",				js_callStaticFunction<VJSEntry::_moveTo>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",				js_callStaticFunction<VJSEntry::_copyTo>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"toURL",				js_callStaticFunction<VJSEntry::_toURL>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"remove",				js_callStaticFunction<VJSEntry::_remove>,				JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getParent",			js_callStaticFunction<VJSEntry::_getParent>,			JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},

		{	0,						0,														0																			},
	};

    outDefinition.className			= "DirectoryEntrySync";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSDirectoryEntryClass::_Initialize>;
	outDefinition.finalize			= js_finalize<VJSDirectoryEntryClass::_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

bool VJSDirectoryEntrySyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSDirectoryEntrySyncClass::Class());
}

XBOX::VJSObject VJSDirectoryReader::NewReaderObject (const XBOX::VJSContext &inContext, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL && !inEntry->IsFile());

	XBOX::VJSObject		createdObject(inContext);
	VJSDirectoryReader	*reader;

	if ((reader = new VJSDirectoryReader(inEntry->GetFileSystem(), inEntry->GetPath(), inEntry->IsSync())) == NULL
	|| reader->fFolder == NULL || reader->fFolderIterator == NULL || reader->fFileIterator == NULL) 

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	else {
		
		reader->fFolderIterator->First();
		reader->fFileIterator->First();
		if (inEntry->IsSync())

			createdObject = VJSDirectoryReaderSyncClass::CreateInstance(inContext, reader);

		else 

			createdObject = VJSDirectoryReaderClass::CreateInstance(inContext, reader);

	}
		
	XBOX::ReleaseRefCountable<VJSDirectoryReader>(&reader);
	return createdObject;
}

sLONG VJSDirectoryReader::ReadEntries (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult)
{
	xbox_assert(outResult != NULL);

	sLONG	code;

	if (!fFileSystem->IsValid()) {
	
		code = VJSFileErrorClass::INVALID_STATE_ERR;
		*outResult = VJSFileErrorClass::NewInstance(inContext, code);
	
	} else if (!fFolder->Exists()) { 
	
		code = VJSFileErrorClass::NOT_FOUND_ERR;
		*outResult = VJSFileErrorClass::NewInstance(inContext, code);
	
	} else {

		// Currently folders are always returned first, followed by files. Yet there is no defined order in the spec, 
		// and this (order) can change in the future.

		sLONG			numberEntries;
		XBOX::VJSArray	entriesArray(inContext);
		
		numberEntries = 0;

		for ( ; fFolderIterator->IsValid() && numberEntries < kMaximumEntries; ++*fFolderIterator) {
		
			XBOX::VFolder	*f = fFolderIterator->Current();

			entriesArray.PushValue(VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, fFolderIterator->Current()->GetPath(), false));
			numberEntries++;

		}

		for ( ; fFileIterator->IsValid() && numberEntries < kMaximumEntries; ++*fFileIterator) {
		
			entriesArray.PushValue(VJSEntry::CreateObject(inContext, fIsSync, fFileSystem, fFileIterator->Current()->GetPath(), true));
			numberEntries++;

		}

		numberEntries = (sLONG) entriesArray.GetLength();	//** ???
		*outResult = entriesArray;
		code =  VJSFileErrorClass::OK;

	}

	return code;
}

VJSDirectoryReader::VJSDirectoryReader (VJSFileSystem *inFileSystem, const XBOX::VFilePath &inPath, bool inIsSync)
{
	xbox_assert(inFileSystem != NULL);

	fIsSync = inIsSync;
	fFileSystem = XBOX::RetainRefCountable<VJSFileSystem>(inFileSystem);
	if ((fFolder = new XBOX::VFolder(inPath)) != NULL) {

		fFolderIterator = new XBOX::VFolderIterator(fFolder, FI_ITERATE_DELETE | FI_NORMAL_FOLDERS);
		fFileIterator = new XBOX::VFileIterator(fFolder, FI_ITERATE_DELETE | FI_NORMAL_FILES);

	}
	fIsReading = false;
}

VJSDirectoryReader::~VJSDirectoryReader()
{
	XBOX::ReleaseRefCountable<XBOX::VFolder>(&fFolder);
	if (fFolderIterator != NULL) {

		delete fFolderIterator;
		fFolderIterator = NULL;

	}
	if (fFileIterator != NULL) {

		delete fFileIterator;
		fFileIterator = NULL;

	}

	xbox_assert(fFileSystem != NULL);
	XBOX::ReleaseRefCountable<VJSFileSystem>(&fFileSystem);	
}

void VJSDirectoryReaderClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryReaderClass, VJSDirectoryReader>::StaticFunction functions[] =
	{
		{	"readEntries",	js_callStaticFunction<_readEntries>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	0,				0,										0																			},
	};

    outDefinition.className			= "DirectoryReader";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

void VJSDirectoryReaderClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL);

	inDirectoryReader->Retain();
}

void VJSDirectoryReaderClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL);

	inDirectoryReader->Release();
}

bool VJSDirectoryReaderClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSDirectoryReaderClass::Class());
}

void VJSDirectoryReaderClass::_readEntries (XBOX::VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL);

	if (inDirectoryReader->IsReading()) {

		XBOX::VJSObject	exception(ioParms.GetContext());

		exception = VJSFileErrorClass::NewInstance(ioParms.GetContext(), VJSFileErrorClass::INVALID_STATE_ERR);
		ioParms.SetException(exception);

		return;

	}

	XBOX::VJSObject	successCallback(ioParms.GetContext());
	XBOX::VJSObject	errorCallback(ioParms.GetContext());

	if (!ioParms.IsObjectParam(1) || !ioParms.GetParamFunc(1, successCallback)) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "1");
		return;

	}
	if (ioParms.CountParams() >= 2
	&& (!ioParms.IsObjectParam(2) || !ioParms.GetParamFunc(2, errorCallback))) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION, "2");
		return;

	}

	VJSW3CFSEvent	*request;

	if ((request = VJSW3CFSEvent::ReadEntries(inDirectoryReader, successCallback, errorCallback)) == NULL)

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	else {

		VJSWorker	*worker;
			
		worker = VJSWorker::RetainWorker(ioParms.GetContext());
		inDirectoryReader->SetAsReading();
		worker->QueueEvent(request);
		XBOX::ReleaseRefCountable<VJSWorker>(&worker);

	}

}

void VJSDirectoryReaderSyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSDirectoryReaderSyncClass, VJSDirectoryReader>::StaticFunction functions[] =
	{
		{	"readEntries",	js_callStaticFunction<_readEntries>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	0,				0,										0																			},
	};

    outDefinition.className			= "DirectoryReaderSync";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSDirectoryReaderClass::_Initialize>;
	outDefinition.finalize			= js_finalize<VJSDirectoryReaderClass::_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

bool VJSDirectoryReaderSyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSDirectoryReaderSyncClass::Class());
}

void VJSDirectoryReaderSyncClass::_readEntries (XBOX::VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader)
{
	xbox_assert(inDirectoryReader != NULL && inDirectoryReader->IsSync());

	sLONG			code;
	XBOX::VJSObject	resultObject(ioParms.GetContext());

	if ((code = inDirectoryReader->ReadEntries(ioParms.GetContext(), &resultObject)) == VJSFileErrorClass::OK)

		ioParms.ReturnValue(resultObject);

	else

		ioParms.SetException(resultObject);
}

void VJSFileEntryClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSFileEntryClass, VJSEntry>::StaticFunction functions[] =
	{
		{	"createWriter",	js_callStaticFunction<VJSEntry::_createWriter>,	JS4D::PropertyAttributeDontDelete											},
		{	"file",			js_callStaticFunction<VJSEntry::_file>,			JS4D::PropertyAttributeDontDelete											},

		{	"getMetadata",	js_callStaticFunction<VJSEntry::_getMetadata>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",		js_callStaticFunction<VJSEntry::_moveTo>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",		js_callStaticFunction<VJSEntry::_copyTo>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"toURL",		js_callStaticFunction<VJSEntry::_toURL>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"remove",		js_callStaticFunction<VJSEntry::_remove>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getParent",	js_callStaticFunction<VJSEntry::_getParent>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},

		{	0,				0,												0																			},
	};

    outDefinition.className			= "FileEntry";
	outDefinition.staticFunctions	= functions;
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

void VJSFileEntryClass::_Initialize (const XBOX::VJSParms_initialize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Retain();
}

void VJSFileEntryClass::_Finalize (const XBOX::VJSParms_finalize &inParms, VJSEntry *inEntry)
{
	xbox_assert(inEntry != NULL);

	inEntry->Release();
}

bool VJSFileEntryClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSFileEntryClass::Class());
}

void VJSFileEntrySyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static VJSClass<VJSFileEntrySyncClass, VJSEntry>::StaticFunction functions[] =
	{
		{	"createWriter",	js_callStaticFunction<VJSEntry::_createWriter>,	JS4D::PropertyAttributeDontDelete											},
		{	"file",			js_callStaticFunction<VJSEntry::_file>,			JS4D::PropertyAttributeDontDelete											},

		{	"getMetadata",	js_callStaticFunction<VJSEntry::_getMetadata>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"moveTo",		js_callStaticFunction<VJSEntry::_moveTo>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"copyTo",		js_callStaticFunction<VJSEntry::_copyTo>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"toURL",		js_callStaticFunction<VJSEntry::_toURL>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"remove",		js_callStaticFunction<VJSEntry::_remove>,		JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	"getParent",	js_callStaticFunction<VJSEntry::_getParent>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},

		{	0,				0,												0																			},
	};

    outDefinition.className			= "FileEntrySync";
	outDefinition.staticFunctions	= functions;	
	outDefinition.initialize		= js_initialize<VJSFileEntryClass::_Initialize>;
	outDefinition.finalize			= js_finalize<VJSFileEntryClass::_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
}

bool VJSFileEntrySyncClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSFileEntrySyncClass::Class());
}

void VJSMetadataClass::GetDefinition (ClassDefinition &outDefinition)
{
	outDefinition.className		= "Metadata";
	outDefinition.hasInstance	= js_hasInstance<_HasInstance>;
}

bool VJSMetadataClass::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
 
	return inParms.GetObject().IsOfClass(VJSMetadataClass::Class());
}

XBOX::VJSObject VJSMetadataClass::NewInstance (const XBOX::VJSContext &inContext, const XBOX::VTime &inModificationTime)
{
	XBOX::VJSObject	newMetadata	= CreateInstance(inContext, NULL);
	XBOX::VJSValue	modificationDate(inContext);

	modificationDate.SetTime(inModificationTime);
	newMetadata.SetProperty("modificationTime", modificationDate, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	return newMetadata;
}

const char	*VJSFileErrorClass::sErrorNames[NUMBER_CODES] = {

	"NOT_FOUND_ERR",
	"SECURITY_ERR", 
	"ABORT_ERR",
	"NOT_READABLE_ERR",
	"ENCODING_ERR",
	"NO_MODIFICATION_ALLOWED_ERR",
	"INVALID_STATE_ERR",
	"SYNTAX_ERR",
	"INVALID_MODIFICATION_ERR",
	"QUOTA_EXCEEDED_ERR",
	"TYPE_MISMATCH_ERR",
	"PATH_EXISTS",

};

void VJSFileErrorClass::GetDefinition (ClassDefinition &outDefinition)
{
    static XBOX::VJSClass<VJSFileErrorClass, void>::StaticFunction functions[] =
	{
		{	"toString()",	js_callStaticFunction<_toString>,	JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly	},
		{	0,				0,									0																			},
	};

	outDefinition.className			= "FileException";
	outDefinition.staticFunctions	= functions;	
}

XBOX::VJSObject VJSFileErrorClass::NewInstance (const XBOX::VJSContext &inContext, sLONG inCode)
{
	xbox_assert(inCode >= FIRST_CODE && inCode <= LAST_CODE);

	XBOX::VJSObject	createdObject = CreateInstance(inContext, NULL);

	createdObject.SetProperty("code", inCode, XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	sLONG	i;

	for (sLONG i = 0; i < NUMBER_CODES; i++) 
	
		createdObject.SetProperty(sErrorNames[i], (sLONG) (FIRST_CODE + i), XBOX::JS4D::PropertyAttributeDontDelete | XBOX::JS4D::PropertyAttributeReadOnly);

	return createdObject;
}

void VJSFileErrorClass::_toString (XBOX::VJSParms_callStaticFunction &ioParms, void *)
{
	bool	propertyExits;
	sLONG	code;

	code = ioParms.GetThis().GetPropertyAsLong("code", NULL, &propertyExits);

	xbox_assert(propertyExits && code >= FIRST_CODE && code <= LAST_CODE);

	ioParms.ReturnString(sErrorNames[code]);
}