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
#ifndef __VFileSystem__
#define __VFileSystem__

#include "Kernel/Sources/VFilePath.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VURL.h"

BEGIN_TOOLBOX_NAMESPACE

/* To implement sandboxing, any file or folder belongs to a "file system".
 *
 * A file system is made of a name, has a root folder, where everything "below" is authorized for access, 
 * along with a writable flag which allows writing, and a quota size (use zero if no limit).
 *
 * Note that this is a "logical" sandboxing: A file or folder authorized for read or/and write access, 
 * may not have its read or/and write bit(s) set on the actual "physical" file system. Also, there may be
 * enough space in quota, but not actual space available on disk.
 */

class XTOOLBOX_API VFileSystem : public VObject, public IRefCountable
{
public:

	// When created, a file system is valid by default. Use zero for inQuota if the file system does not have size limit. 
	// Set inUsedSpace (default is zero) to specify already used space, it is ignored if inQuota is zero.
	
							VFileSystem( const VString &inName, 
										const VFilePath &inRoot, 
										bool inIsWritable, 
										VSize inQuota,
										VSize inUsedSpace = 0);
	
			const VString&			GetName () const					{	return fName;	}
			const VFilePath&		GetRoot () const					{	return fRoot;	}	
	
	// A file system can be set to "not valid", this will disable actions on all entries from it (VE_FS_INVALID_STATE_ERROR).
	
			void					SetValid (bool inIsValid);
			bool					IsValid() const						{	return (fFlags & FLAG_VALID) != 0;	}

	// A file system always allow reading but writing is controllable.

			void					SetWritable (bool inIsWritable);
			bool					IsWritable () const					{	return (fFlags & FLAG_WRITABLE) != 0;	}

			bool					IsValidAndWritable () const			{	return fFlags == (FLAG_VALID | FLAG_WRITABLE);	}

	// If quota is zero, this file system doesn't have limitation.

			void					SetQuota (VSize inQuota)			{	fQuota = inQuota;	}
			VSize					GetQuota () const					{	return fQuota;		}
			bool					HasQuota () const					{	return fQuota > 0;	}

			VSize					GetAvailable () const				{	return fAvailable;	}	

	// Request or relinquish bytes from file system, return appropriate error code.	

			VError					Request (VSize inSize);
			VError					Relinquish (VSize inSize);

	// Return VE_OK if the file or folder with given path is accessible in current file system.
	// Remember that it doesn't check actual physical permissions.
	// This method can be used to check access for "true absolute" path.
	
			VError					IsAuthorized( const VFilePath &inPath);

	// Parse an URL or a POSIX path, determine if it is a folder or file, then check for existence and if (read) access is authorized. 
	//
	// The URL or POSIX path can be absolute or relative. If an absolute path is used, it is actually absolute to the root of the file 
	// system. Hence there can be no drive letter on Windows platform. If a relative path is used, it is relative to the current folder
	// (inCurrentFolder), which must itself be below the root of the file system.
	// 
	// Method return the appropriate error code. If ok, use VFilePath's IsFile() and IsFolder() methods to determine if the
	// full path is a file or folder.

			VError					ParsePosixPath (const VString &inPosixPath, const VFilePath &inCurrentFolder, VFilePath *outFullPath);
			VError					ParseURL (const VURL &inUrl, const VFilePath &inCurrentFolder, VFilePath *outFullPath);	

private: 

	enum FLAGS
	{
		FLAG_VALID		= 1 << 0,
		FLAG_WRITABLE	= 1 << 1,
	};

	virtual							~VFileSystem ()	{}
									VFileSystem( const VFileSystem&);	// forbidden
			VFileSystem&			operator=(const VFileSystem&);	// forbidden
			
			VString					fName;
			VFilePath				fRoot;	
			VSize					fQuota;

			uLONG					fFlags;	
			VSize					fAvailable;
};


class XTOOLBOX_API VFileSystemNamespace : public VObject, public IRefCountable
{
public:
									VFileSystemNamespace( VFileSystemNamespace *inParent = NULL);
	
			void					RegisterFileSystem( VFileSystem *inFileSystem);
			void					UnregisterFileSystem( const VString& inName);
			
			VFileSystem*			RetainFileSystem( const VString& inName);

			VError					LoadFromDefinitionFile( VFile *inFile);
	
private:
	virtual							~VFileSystemNamespace();
									VFileSystemNamespace( const VFileSystemNamespace&);		// forbidden
			VFileSystemNamespace&	operator=( const VFileSystemNamespace&);	// forbidden

	typedef	unordered_map_VString<VRefPtr<VFileSystem> >	MapOfFileSystem;
	
			VCriticalSection		fMutex;
			MapOfFileSystem			fMap;
			VFileSystemNamespace*	fParent;
};

END_TOOLBOX_NAMESPACE

#endif