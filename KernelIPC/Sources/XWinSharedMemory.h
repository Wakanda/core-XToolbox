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
#ifndef __XWinSharedMemory__
#define __XWinSharedMemory__

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API XWinSharedMemory : public VObject
{
public:
	// Constructor
	XWinSharedMemory(const uLONG inName);

	// Destructor
	~XWinSharedMemory();

	VError Create(VSize inSize, FileAccess inMode);
	
	int GetCreateRightsFromFileAccess(FileAccess inMode);

	bool Exists() const;

	VError Attach(FileAccess inMode);
	
	int GetAttachRightsFromFileAccess(FileAccess inMode);
	
	bool IsAttached() const;
	
	VError Detach();
	
	// Delete the system shared memory block (not the logical object !!)
	VError Delete();

	inline void *GetDataPtr();

	inline const void *GetDataPtr() const;

	inline VSize GetDataSize() const;

private:
	// Name of the shared memory block.
	uLONG fName;

	// Size of the shared memory block.
	VSize fSize;

	// Address of the shared memory block.
	void *fMapping;

	// Raw pointer to the shared memory block.
	HANDLE fMapFile;
};

void *XWinSharedMemory::GetDataPtr()
{
	return fMapping;
}

const void *XWinSharedMemory::GetDataPtr() const
{
	return static_cast<const void *>(fMapping);
}

VSize XWinSharedMemory::GetDataSize() const
{
	return fSize;
}

typedef XWinSharedMemory          XSharedMemoryImpl;

END_TOOLBOX_NAMESPACE

#endif

