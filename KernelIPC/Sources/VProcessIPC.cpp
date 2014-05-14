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
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS





VProcessIPC* VProcessIPC::sInstance = NULL;





VProcessIPC::VProcessIPC()
	: fImpl(NULL)
	, fInitCalled(false)
	, fInitOK(false)
	, fFileSystemNotifier(NULL)
	, fSleepNotifier(NULL)
	, fExitStatus(EXIT_FAILURE) // error by default
{
	xbox_assert(sInstance == NULL);
	sInstance = this;
}


VProcessIPC::~VProcessIPC()
{
	delete fFileSystemNotifier;
	delete fSleepNotifier;

	delete fImpl;
	fImpl = NULL;

	xbox_assert(sInstance == this);
	sInstance = NULL;
}


void VProcessIPC::DoRun()
{
	while (IsRunning())
	{
		VTask::ExecuteMessagesWithTimeout( 100, true);
	}
}


VCommandLineParser::Error  VProcessIPC::DoParseCommandLine(VCommandLineParser&)
{
	// empty implementation on purpose
	return VCommandLineParser::eOK;
}


bool VProcessIPC::Init(VProcess::InitOptions inOptions)
{
	if (!fInitCalled)
	{
		if (inherited::Init(inOptions))
		{
			fInitCalled = true;

			// Parse the command line arguments
			{
				// At this point of execution, the command line arguments must have been
				// initialized by the caller `main.cpp`
				#if WITH_NEW_XTOOLBOX_GETOPT != 0
				xbox_assert(!fCommandLineArguments.empty()
					&& "VProcessIPC::SetCommandLineArguments() must be called before Init()");
				#endif

				// the method `DoParseCommandLine` should always be called, even
				// if obviously not necessary. The user may implement some specific
				// code for initializing its application or part of it
				VCommandLineParser cmdparser(fCommandLineArguments);
				switch (DoParseCommandLine(cmdparser))
				{
					case VCommandLineParser::eOK:
					{
						fExitStatus = EXIT_SUCCESS;
						fInitOK = true;
						break;
					}
					case VCommandLineParser::eShouldExit:
					{
						fExitStatus = EXIT_SUCCESS;
						fInitOK = false; // force program shutdown
						break;
					}
					default:
					{
						// error in any cases
						fExitStatus = EXIT_FAILURE;
						fInitOK = false;
					}
				}

				// From now on, the command line arguments are no longer usefull
				// clearing it (clear-and-minimize)
				std::vector<VString>().swap(fCommandLineArguments);
			}

			if (fInitOK)
			{
				fImpl = new XProcessIPCImpl(this);
				fInitOK = (fImpl != NULL) ? fImpl->Init() : false;
			}

			if (fInitOK)
			{
				fFileSystemNotifier = new VFileSystemNotifier;
				fSleepNotifier      = new VSleepNotifier;

				#if VERSIONMAC || VERSION_LINUX
				VError verr = fSleepNotifier->Init();
				xbox_assert(verr == VE_OK);
				#endif

				#if VERSIONDEBUG
				//A debug sleep handler which produce some log on sleep events
				VSleepNotifier::Instance()->RetainSleepHandler(new VSleepDebugHandler);
				#endif

				// fExitStatus will remain to an error value
				// it is the responsability of `main()` to properly reset to EXIT_SUCCESS
				// if the application has been properly initialized.
			}
		}
	}
	return fInitOK;
}




void VProcessIPC::SetCommandLineArguments(sLONG inArgc, const char** inArgv)
{
	fCommandLineArguments.clear();
	for (sLONG i = 0; i < inArgc; ++i)
		fCommandLineArguments.push_back(inArgv[i]); // trusting VString for UTF8 conversion
}


void VProcessIPC::SetCommandLineArguments(sLONG inArgc, const wchar_t** inArgv)
{
	fCommandLineArguments.clear();
	for (sLONG i = 0; i < inArgc; ++i)
		fCommandLineArguments.push_back(inArgv[i]);
}


