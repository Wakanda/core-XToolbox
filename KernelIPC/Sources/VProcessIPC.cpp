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
#include "VFileSystemNotification.h"
#include "VSleepNotifier.h"
#include "VProcessIPC.h"
#if VERSIONMAC || VERSION_LINUX
#include "KernelIPC/Sources/XPosixDaemon.h"
#elif VERSIONWIN
#include "KernelIPC/Sources/XWinDaemon.h"
#endif


VProcessIPC* VProcessIPC::sInstance = NULL;


VProcessIPC::VProcessIPC()
: fImpl(NULL)
, fInitCalled(false)
, fInitOK(false)
, fFileSystemNotifier(NULL)
#if !VERSION_LINUX
, fSleepNotifier(NULL)
#endif
{
	xbox_assert(sInstance == NULL);
	sInstance = this;
}


VProcessIPC::~VProcessIPC()
{
	delete fFileSystemNotifier;

#if!VERSION_LINUX
	delete fSleepNotifier;
#endif

	delete fImpl;
	fImpl = NULL;

	xbox_assert(sInstance == this);
	sInstance = NULL;
}


void VProcessIPC::DoRun()
{
	while(IsRunning())
	{
		VTask::ExecuteMessagesWithTimeout( 100, true);
	}
}


bool VProcessIPC::Init( VProcess::InitOptions inOptions)
{
	if (!fInitCalled)
	{
		if (inherited::Init( inOptions))
		{
			fInitCalled = true;
			fImpl = new XProcessIPCImpl( this);
			if (fImpl != NULL)
			{
				fInitOK = fImpl->Init();
			}
			
			if (fInitOK)
			{
				fFileSystemNotifier = new VFileSystemNotifier;

#if !VERSION_LINUX

				fSleepNotifier = new VSleepNotifier;

	#if VERSIONMAC
				VError verr=fSleepNotifier->Init();
				xbox_assert(verr==VE_OK);
	#endif

	#if VERSIONDEBUG
				//A debug sleep handler which produce some log on sleep events
				VSleepNotifier::Instance()->RetainSleepHandler(new VSleepDebugHandler);
	#endif

#endif

			}
		}
	}
	return fInitOK;
}
