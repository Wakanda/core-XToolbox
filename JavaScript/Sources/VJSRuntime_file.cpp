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

#include "VJSValue.h"
#include "VJSContext.h"
#include "VJSClass.h"
#include "VJSGlobalClass.h"
#include "VJSRuntime_file.h"

#if VERSIONMAC == 1

#include <sys/types.h>
#include <dirent.h>

#endif

USING_TOOLBOX_NAMESPACE

#if VERSIONMAC == 1

XBOX::VCriticalSection	VJSPath::sMutex;
XBOX::VString			VJSPath::sStartupVolumePrefix;

void VJSPath::CleanPath (XBOX::VString *ioPath)
{
	xbox_assert(ioPath != NULL);
	
	if (!ioPath->BeginsWith("/Volumes", true))
		
		return;

	// Determine startup volume, it is the one mounted at root.
	
	if (!sStartupVolumePrefix.GetLength()) {
	
		XBOX::StLocker<XBOX::VCriticalSection>	lock(&sMutex);
		
		// Race-condition possible, recheck.
		
		if (!sStartupVolumePrefix.GetLength()) {
		
			DIR	*dir;
			
			if ((dir = ::opendir("/Volumes")) != NULL) {
				
				struct dirent	*entryStruct;

				while ((entryStruct = ::readdir(dir)) != NULL) {

					// Skip ".", "..", and hidden files or directories.
					
					if (*entryStruct->d_name == '.')
						
						continue;
										
					char	path[PATH_MAX], buffer[PATH_MAX];
					ssize_t	size;

					path[0] = '\0';
					strcat(path, "/Volumes/");
					strcat(path, entryStruct->d_name);
					
					// readlink() will fail if it isn't a link.
					
					if ((size = ::readlink(path, buffer, PATH_MAX)) > 0) {
					
						buffer[size] = '\0';
						if (!strcmp(buffer, "/")) {
												
							sStartupVolumePrefix.FromBlock(path, ::strlen(path), VTC_UTF_8);
							break;
							
						}	
						
					}
						
				}
				::closedir(dir);
				
			}
						
			// Not found, this is impossible!
			
			if (!sStartupVolumePrefix.GetLength()) {
			
				xbox_assert(false);
				
				// Fail safe. This will never be a valid prefix, make this function do nothing.
				
				sStartupVolumePrefix = "#not valid#";
				
			}
			
		}		
		
	} 
	
	// Strip "/Volumes/my_startup_hardrive" from path if needed
		
	if (ioPath->BeginsWith(sStartupVolumePrefix, "true")) {
		
		VSize	numberCharacters;
		
		numberCharacters = ioPath->GetLength() - sStartupVolumePrefix.GetLength();
		ioPath->SubString(sStartupVolumePrefix.GetLength() + 1, numberCharacters);
		
	}
}

#endif

#if VERSIONMAC == 1 || VERSION_LINUX == 1

void VJSPath::ResolvePath (XBOX::VString *ioPath)
{
	xbox_assert(ioPath != NULL);
	
	
	 char	path[PATH_MAX], resolvedPath[PATH_MAX];
	 size_t	size;
	 
	 ioPath->ToBlock(path, PATH_MAX, VTC_UTF_8, true, false);
	 
	 ::realpath(path, resolvedPath);
	 size = ::strlen(resolvedPath);
	 
	 ioPath->FromBlock(resolvedPath, size, VTC_UTF_8);
}

#endif	

bool JS4DFolderIterator::Next()
{
	bool result = false;
	if (fIter == NULL)
	{
		return false;
	}
	else
	{
		if (fBefore)
		{
			fBefore = false;
		}
		else
		{
			++(*fIter);
		}
		result = fIter->IsValid();
	}
	return result;
}


//======================================================


JS4DFileIterator::JS4DFileIterator( XBOX::VFileIterator* inIter)
: VJSBlobValue( CVSTR(""))
, fIter( inIter)
, fFile( NULL)
, fBefore( true)
{
}


JS4DFileIterator::JS4DFileIterator( XBOX::VFile* inFile)
: VJSBlobValue( CVSTR(""))
, fIter( NULL)
, fFile( RetainRefCountable( inFile))
, fBefore( false)
{
}


JS4DFileIterator::JS4DFileIterator( XBOX::VFile* inFile, sLONG8 inStart, sLONG8 inEnd, const VString& inContentType)
: VJSBlobValue( inStart, inEnd, inContentType)
, fIter( NULL)
, fFile( RetainRefCountable( inFile))
, fBefore( false)
{
}

JS4DFileIterator::~JS4DFileIterator()
{
	delete fIter;
	ReleaseRefCountable( &fFile);
}


VFile* JS4DFileIterator::GetFile()
{
	fBefore = false;
	if (fIter == NULL)
		return fFile;
	else
		return fIter->Current();
}


bool JS4DFileIterator::Next()
{
	bool result = false;
	if (fIter == NULL)
	{
		return false;
	}
	else
	{
		if (fBefore)
		{
			fBefore = false;
		}
		else
		{
			++(*fIter);
		}
		result = fIter->IsValid();
	}
	return result;
}

	
sLONG8 JS4DFileIterator::GetMaxSize() const
{
	VFile *file = const_cast<JS4DFileIterator*>( this)->GetFile();
	
	if (file == NULL)
		return 0;
	
	sLONG8 fileSize;
	if (file->GetSize( &fileSize) != VE_OK)
		return 0;

	return fileSize;
}


JS4DFileIterator* JS4DFileIterator::Slice( sLONG8 inStart, sLONG8 inEnd, const VString& inContentType) const
{
	VFile *file = const_cast<JS4DFileIterator*>( this)->GetFile();
	
	if (file == NULL)
		return NULL;

	sLONG8 sliceSize = GetSize();	// use slice size and not file size
	
	// normalize
	sLONG8 start = (inStart >= 0) ? std::min<sLONG8>( inStart, sliceSize) : std::max<sLONG8>( sliceSize + inStart, 0);
	sLONG8 end = (inEnd >= 0) ? (sLONG8) std::min<sLONG8>( inEnd, sliceSize) : std::max<sLONG8>( sliceSize + inEnd, 0);
	sLONG8 size = (end > start) ? (end - start) : 0;

	return new JS4DFileIterator( file, start, size, inContentType);
}


VJSDataSlice* JS4DFileIterator::RetainDataSlice() const
{
	VSize start = (VSize) GetStart();
	VSize size = (VSize) GetSize();
	VSize actualCount = 0;
	void *data = NULL;

	VFileDesc *desc = NULL;
	VError err = const_cast<JS4DFileIterator*>( this)->GetFile()->Open( FA_READ, &desc);
	if (err == VE_OK)
	{
		data = VMemory::NewPtr( size, 'blob');
		if (data != NULL)
		{
			err = desc->GetData( data, size, start, &actualCount);
		}
		else
		{
			vThrowError( VE_MEMORY_FULL);
		}
	}
	delete desc;
	
	VJSBlobData *blobData = new VJSBlobData( data, actualCount);
	VJSDataSlice *slice = new VJSDataSlice( blobData, 0, actualCount);
	ReleaseRefCountable( &blobData);

	return slice;
}


VError JS4DFileIterator::CopyTo( VFile *inDestination, bool inOverwrite)
{
	VError err = VE_OK;
	VFile* file = GetFile();
	if (file != NULL)
	{
		err = file->CopyTo( *inDestination, inOverwrite ? FCP_Overwrite : FCP_Default);
	}	
	return err;
}

//======================================================

JS4DFolderIterator*	VJSFolderIterator::sDummy = new JS4DFolderIterator((VFolderIterator *) NULL);

void VJSFolderIterator::_Initialize( const VJSParms_initialize& inParms, JS4DFolderIterator* inFolderIter)
{
	inFolderIter->Retain();
}


void VJSFolderIterator::_Finalize( const VJSParms_finalize& inParms, JS4DFolderIterator* inFolderIter)
{
	inFolderIter->Release();
}

void VJSFolderIterator::_CallAsFunction (VJSParms_callAsFunction &ioParms)
{
	ioParms.ReturnValue(VJSFolderIterator::_Construct(ioParms));
}

void VJSFolderIterator::_CallAsConstructor (VJSParms_callAsConstructor &ioParms) 
{	
	ioParms.ReturnConstructedObject(VJSFolderIterator::_Construct(ioParms));
}

bool VJSFolderIterator::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
	xbox_assert(inParms.GetContext().GetGlobalObject().GetPropertyAsObject("Folder") == inParms.GetPossibleContructor());
 
	XBOX::VJSObject	objectToTest	= inParms.GetObject();

	return objectToTest.IsOfClass(VJSFolderIterator::Class());
}

XBOX::VJSObject	VJSFolderIterator::_Construct (VJSParms_withArguments &ioParms)
{
	XBOX::VJSObject	constructedObject(ioParms.GetContext());

	constructedObject.SetNull();

	VString folderurl;
	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, folderurl))
	{
		if (!folderurl.IsEmpty() && folderurl[folderurl.GetLength()-1] != '/')
			folderurl += L"/";
		//VURL url(folderurl, eURL_POSIX_STYLE, L"file://");
		VURL url;
		JS4D::GetURLFromPath( folderurl, url);
		VFilePath path;
		if (url.GetFilePath(path) && path.IsFolder())
		{
			VFolder* folder = new VFolder(path);
			constructedObject.MakeFolder( folder);
			ReleaseRefCountable( &folder);
		}
	}
	else
	{
		JS4DFolderIterator* folderiter = ioParms.GetParamObjectPrivateData<VJSFolderIterator>(1);
		if (folderiter != NULL)
		{
			VFolder* folder = folderiter->GetFolder();
			if (folder != NULL)
			{
				VString foldername;
				if (ioParms.IsStringParam(2) && ioParms.GetStringParam(2, foldername))
				{
					VFolder* subfolder = new VFolder(*folder, foldername);
					constructedObject.MakeFolder( subfolder);
					ReleaseRefCountable( &subfolder);
				}
				else
					vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");
			}	
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FOLDER, "1");
	}

	return constructedObject;
}

void VJSFolderIterator::_IsFolder (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *)
{
	/*
	if (ioParms.CountParams() != 1) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}
	*/

	XBOX::VString	path;
	XBOX::VFolder	*folder;
	XBOX::VJSObject	object(ioParms.GetContext());

	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, path)) {

		// An empty string "" path is not a legit folder (WAK0073843).

		if (!path.GetLength()) {
			
			ioParms.ReturnBool(false);
			return;

		}

		// Relative paths will always work, regardless of platform.

		// Absolute POSIX paths ("/tmp") will work on Mac/Linux.
		// Path with drive name ("c:\\temp") will work on Windows.

		if (path.GetUniChar(1) != '/' && !(path.GetLength() >= 2 && path.GetUniChar(2) == ':')) {
			
			XBOX::VString	t;

			folder = ioParms.GetContext().GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
			t = path;
			folder->GetPath(path, XBOX::FPS_POSIX);
			path.AppendString(t);
			XBOX::ReleaseRefCountable(&folder);
		}

		if (path.GetUniChar(path.GetLength()) != '/')

			path.AppendChar('/');
	
		XBOX::VFilePath	filePath(path, XBOX::FPS_POSIX);
		bool			isFolder;

		isFolder = false;
		if (filePath.IsFolder()) {

			folder = new XBOX::VFolder(filePath);

			if (folder != NULL) {

				isFolder = folder->Exists();
				XBOX::ReleaseRefCountable(&folder);

			} else

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		}
		ioParms.ReturnBool(isFolder);
		
	} else if (ioParms.IsObjectParam(1) && ioParms.GetParamObject(1, object) && object.IsInstanceOf("Folder")) {

		// TODO.

	} else 

		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FOLDER, "1");
}

void VJSFolderIterator::_GetName(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString foldername;
	bool noextension = ioParms.GetBoolParam( 1, L"No Extension", "With Extension");
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		/*
		if (noextension)
			inFolderIter->Current()->GetNameWithoutExtension(foldername);
		else
		 */
			folder->GetName(foldername);
	}
	ioParms.ReturnString(foldername);
}


void VJSFolderIterator::_SetName(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString foldername;
	VFolder* folder = inFolderIter->GetFolder();

	// WAK0072305: setName() should return true or false depending on success of operation, but not a Folder object. 
	
	ioParms.ReturnBool(false);

	if (folder != NULL)
	{
		if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, foldername))
		{
			// WAK0071545: Catch error, prevent an exception to be thrown. If erroneous, return false instead.

			StErrorContextInstaller	context(false, true);	

			VFolder	*resultfolder	= nil;		

			if (folder->Rename(foldername, &resultfolder) == XBOX::VE_OK)

				ioParms.ReturnBool(true);

		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");
	}
}


void VJSFolderIterator::_Valid(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->GetFolder() != NULL);
}


void VJSFolderIterator::_Next(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->Next());
}


void VJSFolderIterator::_FirstSubFolder(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolderIterator* iter = new VFolderIterator(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFolderIterator* result = new JS4DFolderIterator(iter);
		ioParms.ReturnValue(VJSFolderIterator::CreateInstance(ioParms.GetContextRef(), result));
		ReleaseRefCountable( &result);
	}
}


void VJSFolderIterator::_FirstFile(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFileIterator* iter = new VFileIterator(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFileIterator* result = new JS4DFileIterator(iter);
		ioParms.ReturnValue(VJSFileIterator::CreateInstance(ioParms.GetContextRef(), result));
		ReleaseRefCountable( &result);
	}
}


void VJSFolderIterator::_IsVisible(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	bool visible = false;
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		bool hidden;
		folder->IsHidden(hidden);
		visible = !hidden;
	}
	ioParms.ReturnBool(visible);
}


void VJSFolderIterator::_SetVisible(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		if (ioParms.GetBoolParam( 1, L"Visible", L"Invisible"))
		{
			// a faire 
		}
		else
		{
			// a faire
		}
	}
}

void VJSFolderIterator::_IsReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
#if VERSION_LINUX   // Postponed Linux Implementation ! (Need refactoring)
        bool isReadable=folder->ProcessCanRead();
        bool isWritable=folder->ProcessCanWrite();

        bool isRO=isReadable&&!isWritable;
#else
        bool isRO=!folder->IsWritableByAll();
#endif
		ioParms.ReturnBool(isRO);
	}
}


void VJSFolderIterator::_SetReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
#if !VERSION_LINUX  // Postponed Linux Implementation ! (Need refactoring)
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		if (ioParms.GetBoolParam( 1, L"ReadOnly", L"ReadWrite"))
		{
			// a faire 
		}
		else
			folder->MakeWritableByAll();
	}
#endif
}


void VJSFolderIterator::_GetPath(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString folderpath;
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		folder->GetPath(folderpath, FPS_POSIX);
		VJSPath::CleanPath(&folderpath);
	}
	ioParms.ReturnString(folderpath);
}

void VJSFolderIterator::_GetURL(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VString folderurl;
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString path;
		folder->GetPath(path, FPS_POSIX);
		
		VJSPath::CleanPath(&path);
		
		VURL url;
		
		url.FromFilePath(path);
		url.GetAbsoluteURL(folderurl, ioParms.GetBoolParam( 1, L"Encoded", L"Not Encoded"));
	}
	ioParms.ReturnString(folderurl);
}


void VJSFolderIterator::_Delete(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->Delete(true) == VE_OK);
	}
}


void VJSFolderIterator::_DeleteContent(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->DeleteContents(true) == VE_OK);
	}
}


void VJSFolderIterator::_GetFreeSpace(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		sLONG8 freebytes;
		folder->GetVolumeFreeSpace(&freebytes,  ! ioParms.GetBoolParam( 1, L"NoQuotas", L"WithQuotas"));
		ioParms.ReturnNumber(freebytes);
	}
}


void VJSFolderIterator::_GetVolumeSize(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		sLONG8 freebytes = - 1;
		//folder->GetVolumeFreeSpace(&freebytes,  ! ioParms.GetBoolParam( 1, L"NoQuotas", L"WithQuotas"));
		ioParms.ReturnNumber(freebytes);
	}
}


void VJSFolderIterator::_Create(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->CreateRecursive(true) == VE_OK);
	}
}


void VJSFolderIterator::_Exists(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->Exists());
	}
}


void VJSFolderIterator::_GetParent(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolder* parent = folder->RetainParentFolder();
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}
}


void VJSFolderIterator::_eachFile(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			std::vector<VJSValue> params;
			VJSObject objfunc(ioParms.GetContextRef());
			if (ioParms.GetParamFunc(1, objfunc, true))
			{
				VJSValue elemVal(ioParms.GetContextRef());
				params.push_back(elemVal);
				VJSValue indiceVal(ioParms.GetContextRef());
				params.push_back(indiceVal);
				params.push_back(ioParms.GetThis());

				VJSValue thisParamVal(ioParms.GetParamValue(2));
				VJSObject thisParam(thisParamVal.GetObject());

				sLONG counter = 0;
				VFileIterator iter(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
				bool cont = true;
				while (iter.IsValid() && cont)
				{
					params[1].SetNumber(counter);
					VFile* file = iter.Current();
					JS4DFileIterator* ifile = new JS4DFileIterator(file);
					VJSObject thisobj(VJSFileIterator::CreateInstance(ioParms.GetContextRef(), ifile));
					ReleaseRefCountable( &ifile);
					params[0] = thisobj;
					VJSValue result(ioParms.GetContextRef());
					JS4D::ExceptionRef except = nil;
					thisParam.CallFunction(objfunc, &params, &result, &except);
					if (except != nil)
					{
						cont = false;
						ioParms.SetException(except);
					}
					++iter;
					++counter;
				}
			}
		}
	}
	ioParms.ReturnThis();
}


void VJSFolderIterator::_eachFolder(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			std::vector<VJSValue> params;
			VJSObject objfunc(ioParms.GetContextRef());
			if (ioParms.GetParamFunc(1, objfunc, true))
			{
				VJSValue elemVal(ioParms.GetContextRef());
				params.push_back(elemVal);
				VJSValue indiceVal(ioParms.GetContextRef());
				params.push_back(indiceVal);
				params.push_back(ioParms.GetThis());

				VJSValue thisParamVal(ioParms.GetParamValue(2));
				VJSObject thisParam(thisParamVal.GetObject());

				sLONG counter = 0;
				VFolderIterator iter(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
				bool cont = true;
				while (iter.IsValid() && cont)
				{
					params[1].SetNumber(counter);

					VFolder* subfolder = iter.Current();
					JS4DFolderIterator* ifolder = new JS4DFolderIterator(subfolder);
					VJSObject thisobj(VJSFolderIterator::CreateInstance(ioParms.GetContextRef(), ifolder));
					ReleaseRefCountable( &ifolder);

					params[0] = thisobj;
					VJSValue result(ioParms.GetContextRef());
					JS4D::ExceptionRef except = nil;
					thisParam.CallFunction(objfunc, &params, &result, &except);
					if (except != nil)
					{
						cont = false;
						ioParms.SetException(except);
					}
					++iter;
					++counter;
				}
			}
		}
	}
	ioParms.ReturnThis();
}

// Lexicographic ordering (much like strcmp() but on XBOX::VString objects).
// This is for coherency with JavaScript runtimes.
// For instance :
//
//	var b = "AAA" < "zz";
//
// Will result in b having a value of true. XBOX::VString::CompareToString()
// doesn't have same behavior.

class CLexCompare 
{
public:

	bool		operator() (const XBOX::VString &a, const XBOX::VString &b) const	{ return isSmaller(a, b); }
	static bool isSmaller (const XBOX::VString &a, const XBOX::VString &b);
};	

bool CLexCompare::isSmaller (const XBOX::VString &a, const XBOX::VString &b)
{ 
	const UniChar	*p = a.GetCPointer();
	const UniChar	*q = b.GetCPointer();

	for ( ; ; )

		if (*p == 0)

			return true;

		else if (*q == 0)

			return false;

		else if (*p < *q)

			return true;

		else if (*p > *q)

			return false;

		else {

			p++;
			q++;

		}
}

// Do a lexicographic depth-first traversal.

bool parseFolder (VFolder* folder, VJSParms_callStaticFunction& ioParms, VJSObject& objfunc, std::vector<VJSValue>& params, sLONG& counter, VJSObject& thisParam)
{
	typedef	std::map<XBOX::VString, VRefPtr<XBOX::VFolder>, CLexCompare>	SFolderMap;
	typedef	std::map<XBOX::VString, VRefPtr<XBOX::VFile>, CLexCompare>		SFileMap;

	// Look up all folders and files of current folder.

	SFolderMap		folderMap;
	VFolderIterator iterFolder(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
	
	while (iterFolder.IsValid()) {

		XBOX::VFolder	*subFolder = iterFolder.Current();
		XBOX::VString	folderName;

		subFolder->GetName(folderName);
		folderMap[folderName] = subFolder;		
		++iterFolder;

	}

	SFileMap		fileMap;
	VFileIterator	iterFile(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);

	while (iterFile.IsValid()) {

		XBOX::VFile		*file	= iterFile.Current();
		XBOX::VString	fileName;

		file->GetName(fileName);
		fileMap[fileName] = file;
		++iterFile;

	}

	// Do the traversal.

	JS4DFolderIterator	*ifolder = new JS4DFolderIterator(folder);
	VJSObject		folderObject(VJSFolderIterator::CreateInstance(ioParms.GetContextRef(), ifolder));
	ReleaseRefCountable( &ifolder);
			
	XBOX::JS4D::ProtectValue(ioParms.GetContextRef(), folderObject.GetObjectRef());

	SFolderMap::iterator	folderMapIter	= folderMap.begin();
	SFileMap::iterator		fileMapIter		= fileMap.begin();
	bool					cont			= true;

	while (cont && (folderMapIter != folderMap.end() || fileMapIter	!= fileMap.end())) {

		bool	doHandleFile;

		if (folderMapIter == folderMap.end())

			doHandleFile = true;

		else if (fileMapIter == fileMap.end()) 

			doHandleFile = false;

		else

			doHandleFile = CLexCompare::isSmaller(fileMapIter->first, folderMapIter->first);

		if (doHandleFile) {

			JS4DFileIterator	*ifile	= new JS4DFileIterator(fileMapIter->second);
			VJSObject			thisobj(VJSFileIterator::CreateInstance(ioParms.GetContextRef(), ifile));
			ReleaseRefCountable( &ifile);
			VJSValue			result(ioParms.GetContextRef());
			JS4D::ExceptionRef	except = nil;

			params[0] = thisobj;
			params[1].SetNumber(counter);
			params[2] = folderObject;

			thisParam.CallFunction(objfunc, &params, &result, &except);
			if (except != nil) {

				cont = false;
				ioParms.SetException(except);

			}

			fileMapIter++;

			++counter;

		} else {

			cont = parseFolder(folderMapIter->second, ioParms, objfunc, params, counter, thisParam);
			folderMapIter++;
			
		}

	}

	XBOX::JS4D::UnprotectValue(ioParms.GetContextRef(), folderObject.GetObjectRef());

	return cont;
}

void VJSFolderIterator::_parse(VJSParms_callStaticFunction& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			std::vector<VJSValue> params;
			VJSObject objfunc(ioParms.GetContextRef());
			if (ioParms.GetParamFunc(1, objfunc, true))
			{
				VJSValue elemVal(ioParms.GetContextRef());
				params.push_back(elemVal);
				VJSValue indiceVal(ioParms.GetContextRef());
				params.push_back(indiceVal);

				params.push_back(ioParms.GetThis());	// Will be overwritten by actual current folder at each callback.

				VJSValue thisParamVal(ioParms.GetParamValue(2));
				VJSObject thisParam(thisParamVal.GetObject());

				sLONG counter = 0;
				parseFolder(folder, ioParms, objfunc,params, counter, thisParam);
			}
		}
	}
	ioParms.ReturnThis();
}



void VJSFolderIterator::_files(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VJSArray result(ioParms.GetContextRef());
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			VFileIterator iter(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
			while (iter.IsValid())
			{
				result.PushFile( iter);
				++iter;
			}
		}
	}
	ioParms.ReturnValue(result);
}


void VJSFolderIterator::_folders(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VJSArray result(ioParms.GetContextRef());
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFilePath path;
		folder->GetPath(path);
		if (path.IsFolder())
		{
			VFolderIterator iter(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
			while (iter.IsValid())
			{
				result.PushFolder( iter);
				++iter;
			}
		}
	}
	ioParms.ReturnValue(result);
}


void VJSFolderIterator::_creationDate(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VTime dd;
		folder->GetTimeAttributes(nil, &dd, nil);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setcreationDate(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		folder->SetTimeAttributes(nil, &dd, nil);
	}
	return true;
}


void VJSFolderIterator::_modificationDate(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VTime dd;
		folder->GetTimeAttributes(&dd, nil, nil);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setmodificationDate(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		folder->SetTimeAttributes(&dd, nil, nil);
	}
	return true;
}




void VJSFolderIterator::_visible(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		bool visible = true;
		// a faire
		ioParms.ReturnBool(visible);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setvisible(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		bool visible;
		ioParms.GetPropertyValue().GetBool(&visible);
		// a faire
	}
	return true;
}



void VJSFolderIterator::_extension(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString extension;
		folder->GetExtension(extension);
		ioParms.ReturnString(extension);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setextension(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VString name;
		VString ext;
		ioParms.GetPropertyValue().GetString(ext);
		folder->GetNameWithoutExtension(name);
		folder->Rename(name+L"."+ext, nil);
	}
	return true;
}


void VJSFolderIterator::_name(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString filename;
		folder->GetName(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setname(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		folder->Rename(filename, nil);
	}
	return true;
}


void VJSFolderIterator::_nameNoExt(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString filename;
		folder->GetNameWithoutExtension(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setnameNoExt(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		VString ext;
		folder->GetExtension(ext);
		folder->Rename(filename+L"."+ext, nil);
	}
	return true;
}


void VJSFolderIterator::_next(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->Next());
}


void VJSFolderIterator::_valid(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	ioParms.ReturnBool(inFolderIter->GetFolder() != NULL);
}



void VJSFolderIterator::_readOnly(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
#if !VERSION_LINUX  // Postponed Linux Implementation ! (Need refactoring)
		bool readonly = folder->IsWritableByAll();
#else
        bool readonly=false;
#endif
		ioParms.ReturnBool(readonly);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFolderIterator::_setreadOnly(VJSParms_setProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
#if !VERSION_LINUX  // Postponed Linux Implementation ! (Need refactoring)
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != nil)
	{
		bool readonly;
		ioParms.GetPropertyValue().GetBool(&readonly);
		if (readonly)
		{
			// a faire
		}
		else
			folder->MakeWritableByAll();
	}
#endif
	return true;
}


void VJSFolderIterator::_path(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VString folderpath;
		folder->GetPath(folderpath, FPS_POSIX);
				
		VJSPath::CleanPath(&folderpath);
						
		ioParms.ReturnString(folderpath);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_exists(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		ioParms.ReturnBool(folder->Exists());
	}
	else
		ioParms.ReturnNullValue();
}



void VJSFolderIterator::_parent(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolder* parent = folder->RetainParentFolder();
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_firstFile(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFileIterator* iter = new VFileIterator(folder, FI_NORMAL_FILES | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFileIterator* result = new JS4DFileIterator(iter);
		ioParms.ReturnValue(VJSFileIterator::CreateInstance(ioParms.GetContextRef(), result));
		ReleaseRefCountable( &result);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFolderIterator::_firstFolder(VJSParms_getProperty& ioParms, JS4DFolderIterator* inFolderIter)
{
	VFolder* folder = inFolderIter->GetFolder();
	if (folder != NULL)
	{
		VFolderIterator* iter = new VFolderIterator(folder, FI_NORMAL_FOLDERS | FI_WANT_INVISIBLES | FI_ITERATE_DELETE);
		JS4DFolderIterator* result = new JS4DFolderIterator(iter);
		ioParms.ReturnValue(VJSFolderIterator::CreateInstance(ioParms.GetContextRef(), result));
		ReleaseRefCountable( &result);
	}
	else
		ioParms.ReturnNullValue();
}

void VJSFolderIterator::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "getName", js_callStaticFunction<_GetName>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setName", js_callStaticFunction<_SetName>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "valid", js_callStaticFunction<_Valid>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "next", js_callStaticFunction<_Next>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "firstSubFolder", js_callStaticFunction<_FirstSubFolder>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "FirstFolder", js_callStaticFunction<_FirstSubFolder>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "FirstFile", js_callStaticFunction<_FirstFile>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "isVisible", js_callStaticFunction<_IsVisible>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setVisible", js_callStaticFunction<_SetVisible>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "isReadOnly", js_callStaticFunction<_IsReadOnly>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setReadOnly", js_callStaticFunction<_SetReadOnly>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getPath", js_callStaticFunction<_GetPath>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getURL", js_callStaticFunction<_GetURL>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "remove", js_callStaticFunction<_Delete>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "removeContent", js_callStaticFunction<_DeleteContent>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "drop", js_callStaticFunction<_Delete>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "dropContent", js_callStaticFunction<_DeleteContent>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getFreeSpace", js_callStaticFunction<_GetFreeSpace>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getVolumeSize", js_callStaticFunction<_GetVolumeSize>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "create", js_callStaticFunction<_Create>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "exists", js_callStaticFunction<_Exists>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "each", js_callStaticFunction<_eachFile>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "forEachFile", js_callStaticFunction<_eachFile>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "forEachFolder", js_callStaticFunction<_eachFolder>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getParent", js_callStaticFunction<_GetParent>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "parse", js_callStaticFunction<_parse>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "creationDate", js_getProperty<_creationDate>, js_setProperty<_setcreationDate>, JS4D::PropertyAttributeDontDelete },
		{ "modificationDate", js_getProperty<_modificationDate>, js_setProperty<_setmodificationDate>, JS4D::PropertyAttributeDontDelete },
		{ "visible", js_getProperty<_visible>, js_setProperty<_setvisible>, JS4D::PropertyAttributeDontDelete },
		{ "extension", js_getProperty<_extension>, js_setProperty<_setextension>, JS4D::PropertyAttributeDontDelete },
		{ "name", js_getProperty<_name>, js_setProperty<_setname>, JS4D::PropertyAttributeDontDelete },
		{ "nameNoExt", js_getProperty<_nameNoExt>, js_setProperty<_setnameNoExt>, JS4D::PropertyAttributeDontDelete },
		//{ "next", js_getProperty<_next>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		//{ "valid", js_getProperty<_valid>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "readOnly", js_getProperty<_readOnly>, js_setProperty<_setreadOnly>, JS4D::PropertyAttributeDontDelete },
		{ "path", js_getProperty<_path>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "exists", js_getProperty<_exists>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "parent", js_getProperty<_parent>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "firstFile", js_getProperty<_firstFile>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "firstFolder", js_getProperty<_firstFolder>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "files", js_getProperty<_files>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "folders", js_getProperty<_folders>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ 0, 0, 0, 0}
	};

	outDefinition.className			= "Folder";
	outDefinition.initialize		= js_initialize<_Initialize>;
	outDefinition.finalize			= js_finalize<_Finalize>;
	outDefinition.hasInstance		= js_hasInstance<_HasInstance>;
	outDefinition.callAsFunction	= js_callAsFunction<_CallAsFunction>;
	outDefinition.callAsConstructor	= js_callAsConstructor<_CallAsConstructor>;
	outDefinition.staticFunctions	= functions;
	outDefinition.staticValues		= values;
}

XBOX::VJSObject	VJSFolderIterator::MakeConstructor (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	folderConstructor(inContext);

	folderConstructor = CreateInstance(inContext, sDummy);

	XBOX::VJSObject	isFolderObject(inContext);
	
	isFolderObject.MakeCallback(XBOX::js_callback<VObject, _IsFolder>);
	
	folderConstructor.SetProperty("isFolder", isFolderObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	
	return folderConstructor;
}

void VJSFolderIterator::do_Folder (VJSParms_callStaticFunction& ioParms) 
{
	ioParms.ReturnValue(VJSFolderIterator::_Construct(ioParms));
}

//======================================================


JS4DFileIterator*	VJSFileIterator::sDummy = new JS4DFileIterator((VFileIterator *) NULL);

void VJSFileIterator::_Initialize( const VJSParms_initialize& inParms, JS4DFileIterator* inFileIter)
{
	// rien a faire pour l'instant
	inFileIter->Retain();
}


void VJSFileIterator::_Finalize( const VJSParms_finalize& inParms, JS4DFileIterator* inFileIter)
{
	inFileIter->Release();
}

void VJSFileIterator::_CallAsFunction (VJSParms_callAsFunction &ioParms)
{
	ioParms.ReturnValue(VJSFileIterator::_Construct(ioParms));
}

void VJSFileIterator::_CallAsConstructor (VJSParms_callAsConstructor &ioParms) 
{	
	ioParms.ReturnConstructedObject(VJSFileIterator::_Construct(ioParms));
}

bool VJSFileIterator::_HasInstance (const VJSParms_hasInstance &inParms) 
{
	xbox_assert(inParms.GetObject().GetObjectRef() == inParms.GetObjectToTest());
	xbox_assert(inParms.GetContext().GetGlobalObject().GetPropertyAsObject("File").GetObjectRef() == inParms.GetPossibleContructor());

	XBOX::VJSObject	objectToTest	= inParms.GetObject();

	return objectToTest.IsOfClass(VJSFileIterator::Class());
}

void VJSFileIterator::_IsFile (XBOX::VJSParms_callStaticFunction &ioParms, XBOX::VObject *)
{
	/*
	if (ioParms.CountParams() != 1) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}
	*/

	XBOX::VString	path;
	XBOX::VJSObject	object(ioParms.GetContext());

	if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, path)) {

		// See comment for VJSFolderIterator::IsFolder().

		if (path.GetUniChar(1) != '/' && !(path.GetLength() >= 2 && path.GetUniChar(2) == ':')) {
		
			XBOX::VFolder	*folder;
			XBOX::VString	t;

			folder = ioParms.GetContext().GetGlobalObjectPrivateInstance()->GetRuntimeDelegate()->RetainScriptsFolder();
			t = path;
			folder->GetPath(path, XBOX::FPS_POSIX);
			path.AppendString(t);
			XBOX::ReleaseRefCountable<XBOX::VFolder>(&folder);

		}

		XBOX::VFilePath	filePath(path, XBOX::FPS_POSIX);
		bool			isFile;

		isFile = false;
		if (filePath.IsFile()) {

			XBOX::VFile	*file = new XBOX::VFile(filePath);

			if (file != NULL) {

				isFile = file->Exists();
				XBOX::ReleaseRefCountable<XBOX::VFile>(&file);

			} else

				XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

		}
		ioParms.ReturnBool(isFile);
		
	} else if (ioParms.IsObjectParam(1) && ioParms.GetParamObject(1, object) && object.IsInstanceOf("File")) {

		// TODO

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);

	} else 
		vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
		
}


void VJSFileIterator::_GetName( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VString filename;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		bool noextension = ioParms.GetBoolParam( 1, L"No Extension", "With Extension");
		if (noextension)
			file->GetNameWithoutExtension(filename);
		else
			file->GetName(filename);
	}
	ioParms.ReturnString(filename);
}



void VJSFileIterator::_SetName( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool ok = false;
	VString filename;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, filename))
		{
			VError err = file->Rename(filename);
			if (err == VE_OK)
				ok = true;
		}
	}
	ioParms.ReturnBool(ok);
}


void VJSFileIterator::_Valid( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->GetFile() != NULL);
}


void VJSFileIterator::_Next( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->Next());
}


void VJSFileIterator::_IsVisible(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool visible = false;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		visible = ((attrib & FATT_HidenFile) == 0);
	}
	ioParms.ReturnBool(visible);
}


void VJSFileIterator::_IsReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool readonly = false;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		readonly = ((attrib & FATT_LockedFile) != 0);
	}
	ioParms.ReturnBool(readonly);
}


void VJSFileIterator::_SetVisible(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool visible = ioParms.GetBoolParam( 1, L"Visible", L"Invisible");
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (visible)
			attrib = attrib ^ FATT_HidenFile;
		else
			attrib = attrib | FATT_HidenFile;
		file->SetFileAttributes(attrib);
	}
}


void VJSFileIterator::_SetReadOnly(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	bool readonly = ioParms.GetBoolParam( 1, L"ReadOnly", L"ReadWrite");
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (readonly)
			attrib = attrib | FATT_LockedFile;
		else
			attrib = attrib ^ FATT_LockedFile;
		file->SetFileAttributes(attrib);
	}
}


void VJSFileIterator::_GetPath(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VString folderpath;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		file->GetPath(folderpath, FPS_POSIX);
		VJSPath::CleanPath(&folderpath);		
	}
	ioParms.ReturnString(folderpath);
}


void VJSFileIterator::_GetURL(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VString folderurl;
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString	path;
		file->GetPath(path, FPS_POSIX);
		
		VJSPath::CleanPath(&path);
		
		VURL url;
		
		url.FromFilePath(path);
		url.GetAbsoluteURL(folderurl, ioParms.GetBoolParam( 1, L"Encoded", L"Not Encoded"));
	}
	ioParms.ReturnString(folderurl);
}


void VJSFileIterator::_Delete(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Delete() == VE_OK);
	}
}


void VJSFileIterator::_GetExtension(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString extension;
		file->GetExtension(extension);
		ioParms.ReturnString(extension);
	}
}


void VJSFileIterator::_GetSize(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		sLONG8 outsize;
		file->GetSize(&outsize);
		ioParms.ReturnNumber(outsize);
	}	
}


void VJSFileIterator::_GetFreeSpace(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		sLONG8 freebytes, totalbytes;
		file->GetVolumeFreeSpace(&freebytes, &totalbytes, ! ioParms.GetBoolParam( 1, L"NoQuotas", L"WithQuotas"), 0);
		ioParms.ReturnNumber(freebytes);
	}	
}


void VJSFileIterator::_GetVolumeSize(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		sLONG8 freebytes, totalbytes;
		file->GetVolumeFreeSpace(&freebytes, &totalbytes, ! ioParms.GetBoolParam( 1, L"NoQuotas", L"WithQuotas"), 0);
		ioParms.ReturnNumber(totalbytes);
	}	
}


void VJSFileIterator::_Create(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Create() == VE_OK);
	}	
}


void VJSFileIterator::_Exists(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Exists());
	}	
}


void VJSFileIterator::_GetParent(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VFolder* parent = file->RetainParentFolder();
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}	
}


void VJSFileIterator::_MoveTo(VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VFile* dest = ioParms.RetainFileParam( 1);
		if (dest != NULL)
		{
			VFilePath path;
			dest->GetPath(path);
			VError err = file->Move(path, NULL, ioParms.GetBoolParam( 2, L"OverWrite", L"KeepExisting") ? FCP_Overwrite : FCP_Default);
			dest->Release();
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FILE, "1");
	}	
}


void VJSFileIterator::_Slice( VJSParms_callStaticFunction& ioParms, JS4DFileIterator* inFileIter)
{
	sLONG8 paramStart;
	if (!ioParms.GetLong8Param( 1, &paramStart))
		paramStart = 0;

	sLONG8 paramEnd;
	if (!ioParms.GetLong8Param( 2, &paramEnd))
		paramEnd = kMAX_sLONG8;

	VString contentType;
	ioParms.GetStringParam( 3, contentType);	// w3c says slice doesn't inherit master blob mimetype

	JS4DFileIterator *slice = inFileIter->Slice( paramStart, paramEnd, contentType);
	if (slice != NULL)
	{
		ioParms.ReturnValue( CreateInstance( ioParms.GetContextRef(), slice));
	}
	else
	{
		ioParms.ReturnNullValue();
	}
	
	ReleaseRefCountable( &slice);
}


void VJSFileIterator::_creationDate(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VTime dd;
		file->GetTimeAttributes(nil, &dd);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setcreationDate(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		file->SetTimeAttributes(nil, &dd);
	}
	return true;
}


void VJSFileIterator::_lastModifiedDate (VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VTime dd;
		file->GetTimeAttributes(&dd);
		ioParms.ReturnTime(dd);
	}
	else
		ioParms.ReturnNullValue();
}

void VJSFileIterator::_type (VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnString(inFileIter->GetContentType());		
}


bool VJSFileIterator::_setLastModifiedDate(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VTime dd;
		ioParms.GetPropertyValue().GetTime(dd);
		file->SetTimeAttributes(&dd);
	}
	return true;
}




void VJSFileIterator::_visible(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		bool visible = ((attrib & FATT_HidenFile) == 0);
		ioParms.ReturnBool(visible);
	}
	else
		ioParms.ReturnNullValue();
}

bool VJSFileIterator::_setvisible(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		bool visible;
		ioParms.GetPropertyValue().GetBool(&visible);
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (visible)
			attrib = attrib ^ FATT_HidenFile;
		else
			attrib = attrib | FATT_HidenFile;
		file->SetFileAttributes(attrib);
	}
	return true;
}


void VJSFileIterator::_extension(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString extension;
		file->GetExtension(extension);
		ioParms.ReturnString(extension);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setextension(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VString name;
		VString ext;
		ioParms.GetPropertyValue().GetString(ext);
		file->GetNameWithoutExtension(name);
		file->Rename(name+L"."+ext);
	}
	return true;
}


void VJSFileIterator::_name(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString filename;
		file->GetName(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setname(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		file->Rename(filename);
	}
	return true;
}


void VJSFileIterator::_nameNoExt(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString filename;
		file->GetNameWithoutExtension(filename);
		ioParms.ReturnString(filename);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setnameNoExt(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		VString filename;
		ioParms.GetPropertyValue().GetString(filename);
		VString ext;
		file->GetExtension(ext);
		file->Rename(filename+L"."+ext);
	}
	return true;
}


void VJSFileIterator::_next(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->Next());
}


void VJSFileIterator::_valid(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	ioParms.ReturnBool(inFileIter->GetFile() != NULL);
}



void VJSFileIterator::_readOnly(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		bool readonly = ((attrib & FATT_LockedFile) != 0);
		ioParms.ReturnBool(readonly);
	}
	else
		ioParms.ReturnNullValue();
}


bool VJSFileIterator::_setreadOnly(VJSParms_setProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != nil)
	{
		bool readonly;
		ioParms.GetPropertyValue().GetBool(&readonly);
		EFileAttributes attrib;
		file->GetFileAttributes(attrib);
		if (readonly)
			attrib = attrib | FATT_LockedFile;
		else
			attrib = attrib ^ FATT_LockedFile;
		file->SetFileAttributes(attrib);
	}
	return true;
}


void VJSFileIterator::_path(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VString folderpath;
		file->GetPath(folderpath, FPS_POSIX);
				
		VJSPath::CleanPath(&folderpath);
				
		ioParms.ReturnString(folderpath);
	}
	else
		ioParms.ReturnNullValue();
}


void VJSFileIterator::_exists(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		ioParms.ReturnBool(file->Exists());
	}
	else
		ioParms.ReturnNullValue();
}



void VJSFileIterator::_parent(VJSParms_getProperty& ioParms, JS4DFileIterator* inFileIter)
{
	VFile* file = inFileIter->GetFile();
	if (file != NULL)
	{
		VFolder* parent = file->RetainParentFolder();
		ioParms.ReturnFolder( parent);
		ReleaseRefCountable( &parent);
	}
	else
		ioParms.ReturnNullValue();
}

XBOX::VJSObject VJSFileIterator::_Construct (VJSParms_withArguments &ioParms)
{
	XBOX::VJSObject	constructedObject(ioParms.GetContext());

	constructedObject.SetNull();

	JS4DFolderIterator* folderiter = ioParms.GetParamObjectPrivateData<VJSFolderIterator>(1);
	if (folderiter == NULL)
	{
		VString folderurl;
		if (ioParms.IsStringParam(1) && ioParms.GetStringParam(1, folderurl) && ! folderurl.IsEmpty() )
		{
			//VURL url(folderurl, eURL_POSIX_STYLE, L"file://");
			VURL url;
			JS4D::GetURLFromPath(folderurl, url);
			VFilePath path;
			if (url.GetFilePath(path) && path.IsFile())
			{
				VFile* file = new VFile( url);
				constructedObject.MakeFile( file);
				ReleaseRefCountable( &file);
			}
		}
		else
			vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_FOLDER, "1");
	}
	else
	{
		VFolder* folder = folderiter->GetFolder();
		if (folder != NULL)
		{
			VString filename;
			if (ioParms.IsStringParam(2) && ioParms.GetStringParam(2, filename))
			{
				VFile* file = new VFile(*folder, filename);
				constructedObject.MakeFile( file);
				ReleaseRefCountable( &file);
			}
			else 
				vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "2");

		}
	}
	
	return constructedObject;
}

void VJSFileIterator::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] = 
	{
		{ "getName", js_callStaticFunction<_GetName>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setName", js_callStaticFunction<_SetName>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "valid", js_callStaticFunction<_Valid>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "next", js_callStaticFunction<_Next>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "isVisible", js_callStaticFunction<_IsVisible>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setVisible", js_callStaticFunction<_SetVisible>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "isReadOnly", js_callStaticFunction<_IsReadOnly>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "setReadOnly", js_callStaticFunction<_SetReadOnly>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getPath", js_callStaticFunction<_GetPath>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getURL", js_callStaticFunction<_GetURL>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "remove", js_callStaticFunction<_Delete>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "drop", js_callStaticFunction<_Delete>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getExtension", js_callStaticFunction<_GetExtension>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getSize", js_callStaticFunction<_GetSize>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getFreeSpace", js_callStaticFunction<_GetFreeSpace>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getVolumeSize", js_callStaticFunction<_GetVolumeSize>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "create", js_callStaticFunction<_Create>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "exists", js_callStaticFunction<_Exists>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "getParent", js_callStaticFunction<_GetParent>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "moveTo", js_callStaticFunction<_MoveTo>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },
		{ "slice", js_callStaticFunction<_Slice>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontEnum | JS4D::PropertyAttributeDontDelete },	// inherited from Blob
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] = 
	{
		{ "creationDate", js_getProperty<_creationDate>, js_setProperty<_setcreationDate>, JS4D::PropertyAttributeDontDelete },
		{ "lastModifiedDate", js_getProperty<_lastModifiedDate>, js_setProperty<_setLastModifiedDate>, JS4D::PropertyAttributeDontDelete },
		{ "type", js_getProperty<_type>, NULL, JS4D::PropertyAttributeDontDelete },	// inherited from Blob
		{ "visible", js_getProperty<_visible>, js_setProperty<_setvisible>, JS4D::PropertyAttributeDontDelete },
		{ "extension", js_getProperty<_extension>, js_setProperty<_setextension>, JS4D::PropertyAttributeDontDelete },
		{ "name", js_getProperty<_name>, js_setProperty<_setname>, JS4D::PropertyAttributeDontDelete },
		{ "nameNoExt", js_getProperty<_nameNoExt>, js_setProperty<_setnameNoExt>, JS4D::PropertyAttributeDontDelete },
		//{ "next", js_getProperty<_next>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		//{ "valid", js_getProperty<_valid>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "readOnly", js_getProperty<_readOnly>, js_setProperty<_setreadOnly>, JS4D::PropertyAttributeDontDelete },
		{ "path", js_getProperty<_path>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		//{ "size", js_getProperty<_size>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},	// inherited from Blob
		{ "exists", js_getProperty<_exists>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ "parent", js_getProperty<_parent>, nil, JS4D::PropertyAttributeDontDelete |  JS4D::PropertyAttributeReadOnly},
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "File";
	outDefinition.parentClass = VJSBlob::Class();
	outDefinition.initialize = js_initialize<_Initialize>;
	outDefinition.finalize = js_finalize<_Finalize>;
	outDefinition.callAsFunction = js_callAsFunction<_CallAsFunction>;
	outDefinition.callAsConstructor = js_callAsConstructor<_CallAsConstructor>;
	outDefinition.hasInstance = js_hasInstance<_HasInstance>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}

XBOX::VJSObject	VJSFileIterator::MakeConstructor (XBOX::VJSContext inContext)
{
	XBOX::VJSObject	fileConstructor(inContext);

	fileConstructor = CreateInstance(inContext, sDummy);

	XBOX::VJSObject	isFileObject(inContext);
	
	isFileObject.MakeCallback(XBOX::js_callback<VObject, _IsFile>);
	
	fileConstructor.SetProperty("isFile", isFileObject, JS4D::PropertyAttributeDontDelete | JS4D::PropertyAttributeReadOnly);
	
	return fileConstructor;
}

void VJSFileIterator::do_File (VJSParms_callStaticFunction &ioParms)
{
	ioParms.ReturnValue(VJSFileIterator::_Construct(ioParms));
}