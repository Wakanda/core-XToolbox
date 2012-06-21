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
#include "XWinSharedSemaphore.h"


BEGIN_TOOLBOX_NAMESPACE


XWinSharedSemaphore::XWinSharedSemaphore(uLONG inName)
{
	char stringName[10];
	sprintf(stringName, "%lu", inName);

	/*
	char*	utf-8
	wchar_t*	utf-32

	StStringConverter<wchart_t> buffer( inName, VTC_WCHAR_T);
	buffer.GetCPointer();
*/
	fSem = CreateSemaphoreW( 
        NULL,           // default security attributes
        1,  // initial count
        1,  // maximum count
		(LPCWSTR)stringName);

    if (fSem == NULL) 
    {
        printf("CreateSemaphore error: %d\n", GetLastError());
        return;
    }
}

XWinSharedSemaphore::~XWinSharedSemaphore()
{
}

bool XWinSharedSemaphore::Lock(sLONG inTimeoutMilliseconds)
{
	return WaitForSingleObject(fSem, inTimeoutMilliseconds);
}

bool XWinSharedSemaphore::Unlock()
{
	return ReleaseSemaphore(fSem, 1, NULL);
}

END_TOOLBOX_NAMESPACE

