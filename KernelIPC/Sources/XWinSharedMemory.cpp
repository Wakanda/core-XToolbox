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
#include "VKernelIPCPrecompiled.h"
#include "XWinSharedMemory.h"

BEGIN_TOOLBOX_NAMESPACE

XWinSharedMemory::XWinSharedMemory(const uLONG inName) :
    fMapFile(NULL), fMapping(NULL), fSize(0), fName(inName)
{
}

XWinSharedMemory::~XWinSharedMemory()
{
	Detach();
}

VError XWinSharedMemory::Create(VSize inSize, FileAccess inMode)
{
	if (Exists())
	{
		return VE_SHM_EXISTS;
	}

	fSize = inSize;
	
	char stringName[10];
	sprintf(stringName, "%lu", fName);

	fMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,    // use paging file
		NULL,                    // default security 
		PAGE_READWRITE,          // read/write access
		0,                       // maximum object size (high-order DWORD) 
		fSize,                // maximum object size (low-order DWORD)  
		(LPCSTR)stringName);  // name of mapping object
	
	return (fMapFile != NULL ? VE_OK : VE_SHM_CANNOT_CREATE);
}

int XWinSharedMemory::GetCreateRightsFromFileAccess(FileAccess inMode)
{
	int rights = 0;
	
	switch (inMode)
	{
		case FA_READ_WRITE:
			rights = PAGE_READWRITE;
			break;
		case FA_READ:
			rights = PAGE_READONLY;
			break;
	}
	
	return rights;
}

int XWinSharedMemory::GetAttachRightsFromFileAccess(FileAccess inMode)
{
	int rights = 0;
	
	switch (inMode)
	{
		case FA_READ_WRITE:
			rights = FILE_MAP_WRITE;
			break;
		case FA_READ:
			rights = FILE_MAP_READ;
			break;
	}
	
	return rights;
}

bool XWinSharedMemory::Exists() const
{
	return (fMapFile != NULL);
}

VError XWinSharedMemory::Attach(FileAccess inMode)
{
	if (IsAttached())
	{
		return VE_SHM_ALREADY_ATTACHED;
	}

	fMapping = MapViewOfFile(fMapFile,   // handle to map object
		FILE_MAP_ALL_ACCESS, // read/write permission
		0,
		0,
		fSize);

	return (fMapping != NULL ? VE_OK : VE_SHM_CANNOT_ATTACH);
}

bool XWinSharedMemory::IsAttached() const
{
	return (fMapping != NULL);
}

VError XWinSharedMemory::Detach()
{
	int res = UnmapViewOfFile(fMapping);

	if (res != 0)
	{
		fMapping = NULL;
	}
	
	return ((res != 0) ? VE_OK : VE_SHM_CANNOT_DETACH);
}

VError XWinSharedMemory::Delete()
{
	return (CloseHandle(fMapFile) != 0 ? VE_OK : VE_SHM_CANNOT_DELETE);
}

END_TOOLBOX_NAMESPACE