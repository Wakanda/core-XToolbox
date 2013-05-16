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

#include "VKernelPrecompiled.h"
#include "VFileSystem.h"
#include "VFilePath.h"
#include "VFile.h"
#include "VFolder.h"
#include "VJSONTools.h"



VFileSystem::VFileSystem( const VString &inName, const VFilePath &inRoot, bool inIsWritable, VSize inQuota, VSize inUsedSpace)
{	
	xbox_assert(inQuota >= inUsedSpace);

	fName = inName;
	fRoot = inRoot;
	fQuota = inQuota;

	fFlags = FLAG_VALID | (inIsWritable ? FLAG_WRITABLE : 0);	
	fAvailable = inQuota - inUsedSpace;
}

void VFileSystem::SetValid (bool inIsValid)
{
	if (inIsValid) 

		fFlags |= FLAG_VALID;

	else

		fFlags &= ~FLAG_VALID;
}

void VFileSystem::SetWritable (bool inIsWritable)
{
	if (inIsWritable) 

		fFlags |= FLAG_WRITABLE;

	else

		fFlags &= ~FLAG_WRITABLE;
}

VError VFileSystem::Request (VSize inSize)
{
	if (!(fFlags & FLAG_VALID))

		return VE_FS_INVALID_STATE_ERROR;

	else if (!(fFlags & FLAG_WRITABLE))

		return VE_FS_NOT_WRITABLE;

	else if (!fQuota) 

		return VE_OK;	// No limit

	else if (inSize <= fAvailable) {

		fAvailable -= inSize;
		return VE_OK;

	} else 

		return VE_FS_OUT_OF_SPACE;
}

VError VFileSystem::Relinquish (VSize inSize)
{
	if (!(fFlags & FLAG_VALID))

		return VE_FS_INVALID_STATE_ERROR;

	if (!(fFlags & FLAG_WRITABLE))

		return VE_FS_NOT_WRITABLE;

	if (fQuota) {

		fAvailable += inSize;
		xbox_assert(fAvailable <= fQuota);

	} 

	return VE_OK;
}

VError VFileSystem::IsAuthorized (const VFilePath &inPath)
{
	if (!(fFlags & FLAG_VALID))

		return VE_FS_INVALID_STATE_ERROR;

	else {

		// VFilePath::IsChildOf() is another way to do (slower).

		const VString	&root	= fRoot.GetPath();
		const VString &path	= inPath.GetPath();
			
		return path.BeginsWith(root, true) ? VE_OK : VE_FS_NOT_AUTHORIZED;

	}
}

VError VFileSystem::ParsePosixPath (const VString &inPosixPath, const VFilePath &inCurrentFolder, VFilePath *outFullPath)
{
	xbox_assert(inCurrentFolder.IsValid() && inCurrentFolder.IsFolder() && (IsAuthorized(inCurrentFolder) == VE_OK) );
	xbox_assert(outFullPath != NULL);

	// Make a full path from an "absolute" path from root of file system or from a "relative" path from
	// current folder (which must be below the root of file system).

	if (inPosixPath.IsEmpty())

		return VE_FS_PATH_ENCODING_ERROR;

	else if (inPosixPath.GetUniChar(1) == '/') {

		VIndex	length;
				
		if ((length = inPosixPath.GetLength()) == 1)

			outFullPath->FromFilePath(fRoot);

		else {	

			VString	posixPath;

			inPosixPath.GetSubString(2, length - 1, posixPath);
			outFullPath->FromRelativePath(fRoot, posixPath, FPS_POSIX);

		}
			
	} else

		outFullPath->FromRelativePath(inCurrentFolder, inPosixPath, FPS_POSIX);

	if (!outFullPath->IsValid())

		return VE_FS_PATH_ENCODING_ERROR;

	// Check that path is authorized. For instance, with a relative path, it can try to 
	// back up its parent "above" the root of file system.

	VError	error;

	if ((error = IsAuthorized(*outFullPath)) != VE_OK)

		return error;

	// Check for existence of the file or folder.

	if (outFullPath->IsFolder()) {

		VFolder	folder(*outFullPath);

		error = folder.Exists() ? VE_OK : VE_FS_NOT_FOUND;
			
	} else {
		
		VFile	file(*outFullPath);
			
		if (file.Exists())

			error = VE_OK;

		else {

			// Allow user to specify a folder without a terminating '/' separator.

			VString	fullPath;

			fullPath = outFullPath->GetPath();
			fullPath.AppendChar(FOLDER_SEPARATOR);

			VFolder	folder(fullPath);

			if (folder.Exists()) {

				outFullPath->FromFullPath(fullPath);
				error = VE_OK;

			} else 

				error = VE_FS_NOT_FOUND;

		}

	}
	return error;
}

VError VFileSystem::ParseURL (const VURL &inUrl, const VFilePath &inCurrentFolder, VFilePath *outFullPath)
{
	VString	posixPath;

	inUrl.GetRelativePath(posixPath);

	return ParsePosixPath(posixPath, inCurrentFolder, outFullPath);
}


//=================================================================


VFileSystemNamespace::VFileSystemNamespace( VFileSystemNamespace *inParent)
: fParent( RetainRefCountable( inParent))
{
}


VFileSystemNamespace::~VFileSystemNamespace()
{
	ReleaseRefCountable( &fParent);
}


void VFileSystemNamespace::RegisterFileSystem( VFileSystem *inFileSystem)
{
	if (inFileSystem != NULL)
	{
		VTaskLock lock( &fMutex);
		
		fMap[inFileSystem->GetName()] = inFileSystem;
	}
}


void VFileSystemNamespace::UnregisterFileSystem( const VString& inName)
{
	VTaskLock lock( &fMutex);
	
	fMap.erase( inName);
}

			
VFileSystem *VFileSystemNamespace::RetainFileSystem( const VString& inName)
{
	VTaskLock lock( &fMutex);
	
	MapOfFileSystem::const_iterator i = fMap.find( inName);

	return (i == fMap.end()) ? NULL : i->second.Retain();
}


VError VFileSystemNamespace::LoadFromDefinitionFile( VFile *inFile)
{
	if (!testAssert( inFile != NULL))
		return vThrowError( VE_UNKNOWN_ERROR);

	VJSONValue	value;
	VError err = VJSONImporter::ParseFile( inFile, value, VJSONImporter::EJSI_Strict);

	if ( (err == VE_OK) && testAssert( value.IsArray()) )
	{
		VJSONArray *array = value.GetArray();
		
		size_t count = array->GetCount();
		for( size_t i = 0 ; i < count ; ++i)
		{
			VJSONValue fileSystemValue = (*array)[i];
			if (testAssert( fileSystemValue.IsObject()))
			{
				VJSONObject *fileSystemObject = fileSystemValue.GetObject();
				
				VString name;
				fileSystemObject->GetPropertyAsString( CVSTR( "name"), name);

				VString pathString;
				fileSystemObject->GetPropertyAsString( CVSTR( "path"), pathString);

				VString parentFileSystemName;
				fileSystemObject->GetPropertyAsString( CVSTR( "fileSystem"), parentFileSystemName);

				bool writable = false;
				fileSystemObject->GetPropertyAsBool( CVSTR( "writable"), &writable);
				
				// we need at least a non empty name and path
				if ( (testAssert( !name.IsEmpty() && !pathString.IsEmpty())) )
				{
					VFilePath path;
					
					// be nice and add the terminating '/' if the user forgot
					if (pathString[pathString.GetLength()-1] != '/')
						pathString += '/';

					// resolve path against parent file system if any
					if (parentFileSystemName.IsEmpty())
					{
						path.FromFullPath( pathString, FPS_POSIX);
					}
					else
					{
						VFileSystem *parentFileSystem = RetainFileSystem( parentFileSystemName);
						if (testAssert( parentFileSystem != NULL) )
						{
							path.FromRelativePath( parentFileSystem->GetRoot(), pathString, FPS_POSIX);
						}
						ReleaseRefCountable( &parentFileSystem);
					}
					if (testAssert( path.IsFolder()))
					{
						VFileSystem *fileSystem = new VFileSystem( name, path, writable, 0);
						RegisterFileSystem( fileSystem);
						ReleaseRefCountable( &fileSystem);
					}
				}
			}
		}
	}
	return err;
}