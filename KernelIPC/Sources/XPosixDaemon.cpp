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
#include "VDaemon.h"
#include "XPosixDaemon.h"



XPosixProcessIPC::XPosixProcessIPC( VProcessIPC *inDaemon)
{
}


XPosixProcessIPC::~XPosixProcessIPC()
{
}


void SIGTERM_handler(int inSigNum)
{
	VProcess::Get()->QuitAsynchronously();
}


void SIGINT_handler(int inSigNum)
{
	VProcess::Get()->QuitAsynchronously();
}


void SIGQUIT_handler(int inSigNum)
{
	VProcess::Get()->QuitAsynchronously();
}


bool XPosixProcessIPC::Init()
{
	struct sigaction sa;
	
	// install handler for SIGTERM signal
	sa.sa_handler = SIGTERM_handler;	
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask, SIGINT);
	sigaddset( &sa.sa_mask, SIGQUIT);
	sa.sa_flags = SA_RESTART;
	
	if (sigaction( SIGTERM, &sa, NULL) != 0)
		return false;
		
	// install handler for SIGINT signal
	sa.sa_handler = SIGINT_handler;
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask, SIGTERM);
	sigaddset( &sa.sa_mask, SIGQUIT);
	sa.sa_flags = SA_RESTART;
	
	if (sigaction( SIGINT, &sa, NULL) != 0)
		return false;

	// install handler for SIGQUIT signal
	sa.sa_handler = SIGQUIT_handler;	
	sigemptyset( &sa.sa_mask);
	sigaddset( &sa.sa_mask, SIGTERM);
	sigaddset( &sa.sa_mask, SIGINT);
	sa.sa_flags = SA_RESTART | SA_RESETHAND;

	if (sigaction( SIGQUIT, &sa, NULL) != 0)
		return false;
	
	return true;
}


//=======================================



XPosixDaemon::XPosixDaemon( VDaemon *inDaemon)
{
}


XPosixDaemon::~XPosixDaemon()
{
}


bool XPosixDaemon::Init()
{
	return true;
}

