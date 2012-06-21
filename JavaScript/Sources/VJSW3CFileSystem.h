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
#ifndef __VJS_W3C_FILE_SYSTEM__
#define __VJS_W3C_FILE_SYSTEM__

#include "VJSClass.h"
#include "VJSValue.h"

#include "VJSWorker.h"

// W3C File System API implementation:
//
//	http://www.w3.org/TR/file-system-api/
//
// Objects must not be shared among workers, not thread safe.
// 
// Notes:
//
//	* DirectoryEntry and DirectoryEntrySync have an additional Folder() function not in W3C specification.

BEGIN_TOOLBOX_NAMESPACE

// LocalFileSystem (3.4.1) and LocalFileSystemSync (3.4.2) interfaces. Each worker has a VJSLocalFileSystem 
// object which encapsulates all the "file systems" (sandboxes) available.

class VJSFileSystem;

class XTOOLBOX_API VJSLocalFileSystem : public XBOX::IRefCountable
{
public:

	enum {

		TEMPORARY	= 0,
		PERSISTENT	= 1,

	};

	// File systems names. 

	static const char			*sTEMPORARY_PREFIX;		// Unique for each request.
	static const char			*sPERSISTENT_NAME;		// Shared by all.
	static const char			*sRELATIVE_PREFIX;		// Unique for each authorized path.

	// Add LocalFileSystem and LocalFileSystemSync interfaces support to global class. 
	// Only to be used by VJSGlobalClass.

	static void					AddInterfacesSupport (const XBOX::VJSContext &inContext);

	// RetainLocalFileSystem() is only to be used by VJSWorker. This will create a new VJSLocalFileSystem object. 
	// By default, project's path is authorized.
	
	static VJSLocalFileSystem	*RetainLocalFileSystem (XBOX::VJSContext &inContext, const XBOX::VFilePath &inProjectPath);

	// Release all file systems. This will invalidate all file systems of this local file system. Memory is actually freed 
	// when reference counts are zero. Temporary file systems have their folders removed.

	void						ReleaseAllFileSystems ();
	
	// Authorize a path (all sub-directories are readable/writable), return true if successful. It is forbidden to authorize
	// a path which is a child or a parent of an already authorized path. File systems must be disjoint (section 3.2).

	bool						AuthorizePath (const XBOX::VFilePath &inPath);

	// Invalidate a path. It is an error to invalid a path that has not been authorized by a call to AuthorizePath(). Also it
	// is forbidden to invalid path more than once.

	void						InvalidPath (const XBOX::VFilePath &inPath);

	// Only to be used by VJSLocalFileSystem.

	VJSFileSystem				*RetainTemporaryFileSystem (VSize inQuota);
	VJSFileSystem				*RetainPersistentFileSystem (VSize inQuota);
	VJSFileSystem				*RetainRelativeFileSystem (const XBOX::VString &inPath);
	void						ReleaseFileSystem (VJSFileSystem *inFileSystem);

	// Only to be used by VJSLocalFileSystem and VJSW3CFSEvent

	sLONG						RequestFileSystem (const XBOX::VJSContext &inContext, sLONG inType, VSize inQuota, XBOX::VJSObject *outResult);
	sLONG						ResolveURL (const XBOX::VJSContext &inContext, const XBOX::VString &inURL, bool inIsSync, XBOX::VJSObject *outResult);

private: 

	// VJSFileSystem is a IRefCountable, but is stored as a "simple" pointer. VJSFileSystem's
	// destructor will remove itself. Not thread safe (VJSLocalFileSystem is not to be shared
	// with another thread).

	typedef	std::map<XBOX::VString, VRefPtr<VJSFileSystem>, XBOX::VStringLessCompareStrict>	SMap;
	
	XBOX::VFilePath				fProjectPath;
	SMap						fFileSystems;

								VJSLocalFileSystem (XBOX::VJSContext &inContext, const XBOX::VFilePath &inRelativeRoot);
	virtual						~VJSLocalFileSystem ();

	static void					_requestFileSystem (XBOX::VJSParms_callStaticFunction &ioParms, void *);
    static void					_resolveLocalFileSystemURL (XBOX::VJSParms_callStaticFunction &ioParms, void *);
	static void					_requestFileSystemSync (XBOX::VJSParms_callStaticFunction &ioParms, void *);
    static void					_resolveLocalFileSystemSyncURL (XBOX::VJSParms_callStaticFunction &ioParms, void *);

	static void					_requestFileSystem (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync);

	// resolveLocalFileSystemURL() and resolveLocalFileSystemSyncURL() always return a VJSEntry from the "relative" file system.
	// However the URL can be absolute or relative.

	static void					_resolveLocalFileSystemURL (XBOX::VJSParms_callStaticFunction &ioParms, bool inIsSync);
};

// FileSystem (section 5.1) and FileSystemSync (section 6.1) objects. Any file or folder belongs to a "file system" (sandbox).

class XTOOLBOX_API VJSFileSystem : public XBOX::IRefCountable
{
public:

	// When created a file system is valid by default. 

							VJSFileSystem (VJSLocalFileSystem *inLocalFileSystem, const XBOX::VString &inName, const XBOX::VFilePath &inRoot, VSize inQuota);
	
	const XBOX::VString		&GetName () const		{	return fName;	}
	const XBOX::VFilePath	&GetRoot () const		{	return fRoot;	}	

	// A file system can be set to "not valid", this will disable actions on all entries from it (INVALID_STATE_ERR).
	
	void					SetValid (bool inYesNo)	{	fIsValid = inYesNo;	}
	bool					IsValid () const		{	return fIsValid;	}

	// If quota is zero, this file system doesn't have limitation.

	VSize					GetQuota () const		{	return fQuota;		}
	VSize					GetAvailable () const	{	return fAvailable;	}
	bool					HasQuota () const		{	return fQuota > 0;	}

	// Request or relinquish bytes from file system. Request() return true if successful.

	bool					Request (VSize inSize);
	void					Relinquish (VSize inSize);

	// Parse URL, determine if it is a folder or file, then check for existence and if access is authorized. Return appropriate 
	// VJSFileError code. *ioPath on input is the root path for absolute "adressing". *ioPath and *outIsFile are always set on 
	// output. Returned *ioPath may be invalid (VJSFileErrorClass::ENCODING_ERR). *outIsFile is always correct, as long as returned 
	// code isn't VJSFileErrorClass::ENCODING_ERR or VJSFileErrorClass::SECURITY_ERR.

	sLONG					ParseURL (const XBOX::VString &inUrl, XBOX::VFilePath *ioPath, bool *outIsFile);

	// Return true if the folder or file with given path is accessible in current file system.
	// Note that it doesn't check if the folder or file can actually be read (permissions).
	
	bool					IsAuthorized (const XBOX::VFilePath &inPath);

	// Return true if it is an absolute POSIX path. On Windows platform, drive letter is accepted.

	static bool				IsAbsolutePath (const XBOX::VString &inPath);

	// Set or get ObjectRef of root entry, it must be unique.

	void					SetRootEntryObjectRef (bool inIsSync, JS4D::ObjectRef inObjectRef);
	JS4D::ObjectRef			GetRootEntryObjectRef (bool inIsSync) const				{	return inIsSync ? fRootEntrySyncObjectRef : fRootEntryObjectRef;	}

	// If a FileSystem or FileSystemSync object is garbage collected, null the object references.
	// Note that root entry will be garbage collected a long.

	void					ClearObjectRefs (void);

private: 

friend class VJSFileSystemClass;
friend class VJSFileSystemSyncClass;

	virtual					~VJSFileSystem ();
								
	bool					fIsValid;
	VJSLocalFileSystem		*fLocalFileSystem;

	XBOX::VString			fName;
	XBOX::VFilePath			fRoot;	

	VSize					fQuota;
	VSize					fAvailable;

	JS4D::ObjectRef			fFileSystemObjectRef;
	JS4D::ObjectRef			fFileSystemSyncObjectRef;

	JS4D::ObjectRef			fRootEntryObjectRef;
	JS4D::ObjectRef			fRootEntrySyncObjectRef;
};

class XTOOLBOX_API VJSFileSystemClass : public XBOX::VJSClass<VJSFileSystemClass, VJSFileSystem>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);	

	// Always return same FileSystem object if it exists.

	static XBOX::VJSObject	GetInstance (const XBOX::VJSContext &inContext, VJSFileSystem *inFileSystem);

private:

	static void				_Initialize (const XBOX::VJSParms_initialize &inParms, VJSFileSystem *inFileSystem);
	static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSFileSystem *inFileSystem);
	static bool				_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSFileSystemSyncClass : public XBOX::VJSClass<VJSFileSystemSyncClass, VJSFileSystem>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);	

	// Always return same FileSystemSync object if it exists.

	static XBOX::VJSObject	GetInstance (const XBOX::VJSContext &inContext, VJSFileSystem *inFileSystem);

private:

	static void				_Initialize (const XBOX::VJSParms_initialize &inParms, VJSFileSystem *inFileSystem);
	static void				_Finalize (const XBOX::VJSParms_finalize &inParms, VJSFileSystem *inFileSystem);
	static bool				_HasInstance (const VJSParms_hasInstance &inParms);
};

// Files and folders have a shared set of attributes and methods.

class XTOOLBOX_API VJSEntry : public IRefCountable
{
public:

	// Flags for getFile() and getDirectory().

	enum {
	
		eFLAG_CREATE	= 0x01,		// If doesn't exists, create it.
		eFLAG_EXCLUSIVE	= 0x02,		// If eFLAG_CREATE is set and already exists, fail.
		
	};
	
	// Create a DirectoryEntry/DirectoryEntrySync or FileEntry/FileEntrySync object.
	// It is up to reator to determine if the entry is for a folder or a file, and that it is valid.	

	static XBOX::VJSObject	CreateObject (const XBOX::VJSContext &inContext, bool inIsSync,
											VJSFileSystem *inFileSystem, const XBOX::VFilePath &inPath, bool inIsFile);

	VJSFileSystem			*GetFileSystem ()	{	return fFileSystem;	}
	bool					IsSync()			{	return fIsSync;		}
	bool					IsFile ()			{	return fIsFile;		}
	const XBOX::VFilePath	&GetPath ()			{	return fPath;		}

	// Execute operation. Return error code (see VJSFileErrorClass) along with appropriate object (success) or exception (failure).

	sLONG					GetMetadata (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult);
	sLONG					MoveTo (const XBOX::VJSContext &inContext, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, XBOX::VJSObject *outResult);
	sLONG					CopyTo (const XBOX::VJSContext &inContext, VJSEntry *inTargetEntry, const XBOX::VString &inNewName, XBOX::VJSObject *outResult);
	sLONG					Remove (const XBOX::VJSContext &inContext, XBOX::VJSObject *outException);
	sLONG					GetParent (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult);

	// DirectoryEntry and DirectoryEntrySync specific.

	sLONG					GetFile (const XBOX::VJSContext &inContext, VJSEntry *inFolderEntry, const XBOX::VString &inURL, sLONG inFlags, XBOX::VJSObject *outResult);
	sLONG					GetDirectory (const XBOX::VJSContext &inContext, VJSEntry *inFolderEntry, const XBOX::VString &inURL, sLONG inFlags, XBOX::VJSObject *outResult);
	sLONG					RemoveRecursively (const XBOX::VJSContext &inContext, XBOX::VJSObject *outException);

	sLONG					Folder (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult);

	// FileEntry and FileEntrySync specific.

	sLONG					CreateWriter (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult);
	sLONG					File (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult);

private:

friend class VJSDirectoryEntryClass;
friend class VJSFileEntryClass;

friend class VJSDirectoryEntrySyncClass;
friend class VJSFileEntrySyncClass;

	VJSFileSystem	*fFileSystem;
	XBOX::VFilePath	fPath;
	bool			fIsFile;
	bool			fIsSync;

					VJSEntry (VJSFileSystem *inFileSystem, const XBOX::VFilePath &inPath, bool inIsFile, bool inIsSync);
	virtual			~VJSEntry ();

	// Entry/EntrySync interface functions.
		
	static void		_getMetadata (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static void		_moveTo (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static void		_copyTo (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static void		_toURL (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static void		_remove (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static void		_getParent (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	// DirectoryEntry/DirectoryEntrySync interface functions.

	static void		_createReader (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static void		_getFile (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static void		_getDirectory (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
    static void		_removeRecursively (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	static void		_folder (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	// FileEntry/FileEntrySync interface functions.

	static void		_createWriter (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);
	static void		_file (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry);

	// Helper functions.

	static void		_moveOrCopyTo (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsMoveTo);
	static void		_getFileOrDirectory (XBOX::VJSParms_callStaticFunction &ioParms, VJSEntry *inEntry, bool inIsGetFile);

	bool			_IsRoot ()	{	return fPath.GetPath().EqualToString(fFileSystem->GetRoot().GetPath());	}

	bool			_IsNameCorrect (const XBOX::VString &inName);
};

// DirectoryEntry (section 5.3) and DirectoryEntrySync (section 6.3) objects.

class XTOOLBOX_API VJSDirectoryEntryClass : public XBOX::VJSClass<VJSDirectoryEntryClass, VJSEntry>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
		
private: 

friend class VJSDirectoryEntrySyncClass;

	static void	_Initialize (const XBOX::VJSParms_initialize &inParms, VJSEntry *inEntry);
	static void	_Finalize (const XBOX::VJSParms_finalize &inParms, VJSEntry *inEntry);
	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSDirectoryEntrySyncClass : public XBOX::VJSClass<VJSDirectoryEntrySyncClass, VJSEntry>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);	

private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSDirectoryReader : public XBOX::IRefCountable
{
public:

	// Maximum number of entries returned by readEntries() at once.

	static const sLONG		kMaximumEntries	= 128;

	static XBOX::VJSObject	NewReaderObject (const XBOX::VJSContext &inContext, VJSEntry *inEntry);

	sLONG					ReadEntries (const XBOX::VJSContext &inContext, XBOX::VJSObject *outResult);

	bool					IsSync ()		{	return fIsSync;		}
	bool					IsReading ()	{	return fIsReading;	}

	void					SetAsReading ()	{	fIsReading = true;	}	

private:

	bool					fIsSync;
	VJSFileSystem			*fFileSystem;

	XBOX::VFolder			*fFolder;
	XBOX::VFolderIterator	*fFolderIterator;
	XBOX::VFileIterator		*fFileIterator;

	bool					fIsReading;		// Only for asynchronous operation, prevents calling readEntries() several times.
	
							VJSDirectoryReader (VJSFileSystem *inFileSystem, const XBOX::VFilePath &inPath, bool inIsSync);
	virtual					~VJSDirectoryReader();
};

class XTOOLBOX_API VJSDirectoryReaderClass : public XBOX::VJSClass<VJSDirectoryReaderClass, VJSDirectoryReader>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
	
private:

friend class VJSDirectoryReaderSyncClass;

	static void	_Initialize (const XBOX::VJSParms_initialize &inParms, VJSDirectoryReader *inDirectoryReader);
	static void	_Finalize (const XBOX::VJSParms_finalize &inParms, VJSDirectoryReader *inDirectoryReader);
	static bool	_HasInstance (const VJSParms_hasInstance &inParms);

	static void	_readEntries (XBOX::VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader);
};

class XTOOLBOX_API VJSDirectoryReaderSyncClass : public XBOX::VJSClass<VJSDirectoryReaderSyncClass, VJSDirectoryReader>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
		
private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
	static void	_readEntries (XBOX::VJSParms_callStaticFunction &ioParms, VJSDirectoryReader *inDirectoryReader);
};

// FileEntry (section 5.5) and FileEntrySync objects.

class XTOOLBOX_API VJSFileEntryClass : public XBOX::VJSClass<VJSFileEntryClass, VJSEntry>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);
	
private:

friend class VJSFileEntrySyncClass;

	static void	_Initialize (const XBOX::VJSParms_initialize &inParms, VJSEntry *inEntry);
	static void	_Finalize (const XBOX::VJSParms_finalize &inParms, VJSEntry *inEntry);
	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

class XTOOLBOX_API VJSFileEntrySyncClass : public XBOX::VJSClass<VJSFileEntrySyncClass, VJSEntry>
{
public:

	static void	GetDefinition (ClassDefinition &outDefinition);

private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

// Metadata object, see section 4.1.

class XTOOLBOX_API VJSMetadataClass : public XBOX::VJSClass<VJSMetadataClass, void>
{
public:

	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	NewInstance (const XBOX::VJSContext &inContext, const XBOX::VTime &inModificationTime);

private:

	static bool	_HasInstance (const VJSParms_hasInstance &inParms);
};

// FileError exception object, see section 7.

class XTOOLBOX_API VJSFileErrorClass : public XBOX::VJSClass<VJSFileErrorClass, void>
{
public:

	enum {

		OK							= 0,

		NOT_FOUND_ERR				= 1,
		SECURITY_ERR				= 2, 
		ABORT_ERR					= 3,
		NOT_READABLE_ERR			= 4,
		ENCODING_ERR				= 5,
		NO_MODIFICATION_ALLOWED_ERR = 6,
		INVALID_STATE_ERR			= 7,
		SYNTAX_ERR					= 8,
		INVALID_MODIFICATION_ERR	= 9,
		QUOTA_EXCEEDED_ERR			= 10,
		TYPE_MISMATCH_ERR			= 11,
		PATH_EXISTS_ERR				= 12,

		FIRST_CODE					= NOT_FOUND_ERR,
		LAST_CODE					= PATH_EXISTS_ERR,
		NUMBER_CODES				= LAST_CODE - FIRST_CODE + 1,

	};
	
	static void				GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject	NewInstance (const XBOX::VJSContext &inContext, sLONG inCode);

private:

	static const char		*sErrorNames[NUMBER_CODES];

	static void				_toString (XBOX::VJSParms_callStaticFunction &ioParms, void *);
};

END_TOOLBOX_NAMESPACE

#endif