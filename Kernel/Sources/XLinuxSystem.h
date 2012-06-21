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
#ifndef __XLinuxSystem__
#define __XLinuxSystem__

#include "Kernel/Sources/VKernelTypes.h"



BEGIN_TOOLBOX_NAMESPACE


class VString;
//class VValueBag;

typedef pid_t pid;


class XTOOLBOX_API XLinuxSystem 
{
public:
	
	static void		Beep();

	static void		GetOSVersionString(VString &outStr);

	static sLONG	Random(bool inComputeFast);

	static bool     DisplayNotification(const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse);
	static bool     IsSystemVersionOrAbove(SystemVersion inSystemVersion);

    static VSize    GetVMPageSize();
    static void*    VirtualAlloc(VSize inNbBytes, const void *inHintAddress);
 	static void*	VirtualAllocPhysicalMemory(VSize inNbBytes, const void *inHintAddress, bool *outCouldLock);
    static void     VirtualFree(void* inBlock, VSize inNbBytes, bool inPhysicalMemory);

	static void		LocalToUTCTime(sWORD ioVals[7]);
	static void     UTCToLocalTime(sWORD ioVals[7]);

	static void		GetSystemUTCTime(sWORD outVals[7]);
	static void		GetSystemLocalTime(sWORD outVals[7]);

	static uLONG	GetCurrentTicks();
	static uLONG    GetCurrentTime();

	static void		GetProfilingCounter(sLONG8& outVal);
	static sLONG8   GetProfilingFrequency();

	static void		Delay(sLONG inMilliseconds);

	static sLONG    GetNumberOfProcessors();
	static bool		GetProcessorsPercentageUse(Real& outPercentageUse);
	static void		GetProcessorTypeString(VString& outProcessorType, bool inCleanupSpaces);
	static Real		GetApplicationProcessorsPercentageUse();

	static sLONG8	GetPhysicalMemSize();
	static sLONG8	GetPhysicalFreeMemSize();

 	static sLONG8	GetApplicationPhysicalMemSize();
 	static VSize	GetApplicationVirtualMemSize();
	static VSize	VirtualMemoryUsedSize();

	
	static bool		GetInOutNetworkStats(std::vector<Real>& outNetworkStats);
	static bool     AllowedToGetSystemInfo();

 	static void     GetLoginUserName( VString& outName); //todo
	static void     GetHostName( VString& outName); //todo
	static void		GetProcessList(std::vector<pid> &outPIDs); //todo

// 	static bool     VirtualQuery( const void *inAddress, VSize *outSize, VMStatus *outStatus, const void **outBaseAddress); // duno
// 	static bool     AllowedToGetSystemInfo();	// duno

// 	static bool		GetInOutNetworkStats(std::vector< Real >& outNetworkStats); // Ouch !
// 	static void     DemangleSymbol( const char *inMangledSymbol, VString& outDemangledSymbol); // Ouch !

};


typedef XLinuxSystem XSystemImpl;



END_TOOLBOX_NAMESPACE

#endif
