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
#include "VString.h"
#include "XWinSystem.h"

#include <pdhmsg.h>
#include <pdh.h>
#include <imagehlp.h>
#include <psapi.h>
#include <tlhelp32.h>



static HMODULE GetPSAPIModule()
{
	static HMODULE sModule = NULL;
	if (sModule == NULL)
	{
		HMODULE h = LoadLibrary( "psapi.dll");
		if (h != NULL)
		{
			xbox_assert( sizeof( HMODULE) == sizeof( void*));
			void* oldVal = ::InterlockedCompareExchangePointer( (void**) &sModule, h, NULL);
			if (oldVal != NULL)
				FreeLibrary( h);
		}
	}
	return sModule;
}

class VDisplayNotificationParam : public VObject
{
public:
	VDisplayNotificationParam( const VString& inTitle, const VString& inMessage, UINT inFlags, HWND inParent)
		: fTitle( inTitle), fMessage( inMessage), fFlags( inFlags), fParent( inParent)	{}

	VString	fTitle;
	VString	fMessage;
	UINT	fFlags;
	HWND	fParent;
};


static DWORD WINAPI DisplayNotificationProc( LPVOID inParam)
{
	VDisplayNotificationParam *param = reinterpret_cast<VDisplayNotificationParam*>( inParam);

	int r = ::MessageBoxExW( param->fParent, param->fMessage.GetCPointer(), param->fTitle.GetCPointer(), param->fFlags, 0);
	
	delete param;
	return 1;
}


bool XWinSystem::DisplayNotification( const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions, ENotificationResponse *outResponse)
{
	bool ok;
	
	UINT flags = MB_TASKMODAL | MB_SERVICE_NOTIFICATION;
	flags |= (inOptions & EDN_StyleCritical) ? MB_ICONWARNING : MB_ICONINFORMATION;

	switch( inOptions & EDN_LayoutMask)
	{
		case EDN_OK:
			flags |= MB_OK;
			break;
		
		case EDN_OK_Cancel:
			flags |= MB_OKCANCEL;
			break;
		
		case EDN_Yes_No:
			flags |= MB_YESNO;
			break;
		
		case EDN_Yes_No_Cancel:
			flags |= MB_YESNOCANCEL;
			break;
		
		case EDN_Abort_Retry_Ignore:
			flags |= MB_ABORTRETRYIGNORE;
			break;
	}

	switch( inOptions & EDN_DefaultMask)
	{
		case EDN_Default1:
			flags |= MB_DEFBUTTON1;
			break;

		case EDN_Default2:
			flags |= MB_DEFBUTTON2;
			break;

		case EDN_Default3:
			flags |= MB_DEFBUTTON3;
			break;
	}
	
	if (outResponse == NULL)
	{
		/*
			we need a modeless asynchronous message.
			so we create a thread.
		*/

		// MB_SERVICE_NOTIFICATION est adapte a notre cas (face-less app) mais donne un look Win95 tres moche, et surtout ca serialise les messages !!


		VDisplayNotificationParam *param = new VDisplayNotificationParam( inTitle, inMessage, flags, NULL);
		
		DWORD threadID;
		HANDLE thread = ::CreateThread( NULL, 0, DisplayNotificationProc, param, 0, &threadID);

		ok = (thread != NULL);

		if (thread != NULL)
			CloseHandle( thread);
	}
	else
	{
		int r = ::MessageBoxExW( NULL, inMessage.GetCPointer(), inTitle.GetCPointer(), flags, 0);
		ok = r != 0;
		switch( r)
		{
			case IDYES:			*outResponse = ERN_Yes; break;
			case IDNO:			*outResponse = ERN_No; break;
			case IDRETRY:		*outResponse = ERN_Retry; break;
			case IDIGNORE:		*outResponse = ERN_Ignore; break;
			case IDABORT:		*outResponse = ERN_Abort; break;
			case IDOK:			*outResponse = ERN_OK; break;
			case IDCANCEL:
			default:			*outResponse = ERN_Cancel; break;
		}
	}

	return ok;
}


bool XWinSystem::GUIDisplayNotification( HWND inParent, const VString& inTitle, const VString& inMessage, EDisplayNotificationOptions inOptions)
{
	UINT flags = MB_OK | MB_TASKMODAL;
	flags |= (inOptions & EDN_StyleCritical) ? MB_ICONWARNING : MB_ICONINFORMATION;

	VDisplayNotificationParam *param = new VDisplayNotificationParam( inTitle, inMessage, flags, inParent);
	
	DWORD threadID;
	HANDLE thread = ::CreateThread( NULL, 0, DisplayNotificationProc, param, 0, &threadID);

	bool ok = (thread != NULL);

	if (thread != NULL)
		CloseHandle( thread);

	return ok;
}


sLONG8 XWinSystem::GetApplicationPhysicalMemSize()
{
	sLONG8 result = 0;

	typedef	BOOL (WINAPI *PGPMI)( HANDLE Process, PPROCESS_MEMORY_COUNTERS ppsmemCounters,DWORD cb);
	static PGPMI sPGPMI = NULL;
	if (sPGPMI == NULL)
	{
		sPGPMI = (PGPMI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "K32GetProcessMemoryInfo ");
		if (sPGPMI == NULL)
		{
			sPGPMI = (PGPMI) GetProcAddress(GetPSAPIModule(), "GetProcessMemoryInfo");
		}
	}

	if (sPGPMI != NULL)
	{
		PROCESS_MEMORY_COUNTERS info;
		::memset( &info, 0, sizeof( info));
		if (sPGPMI( ::GetCurrentProcess(), &info, sizeof( info)))
		{
			result = (sLONG8) info.WorkingSetSize;
		}
	}
	return result;
}


SystemVersion XWinSystem::GetSystemVersion()
{
	static SystemVersion sWinVersion = WIN_UNKNOWN;
	if (sWinVersion == WIN_UNKNOWN)
	{
		OSVERSIONINFO vers;
		OSVERSIONINFOEX OSversion;
		DWORDLONG dwlConditionMask = 0;

		vers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

		VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
		VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );
		// Initialize the OSVERSIONINFOEX structure.
		ZeroMemory(&OSversion, sizeof(OSVERSIONINFOEX));
		OSversion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
		
		// set Windows Seven
		OSversion.dwMajorVersion = 6;
		OSversion.dwMinorVersion = 1;
		if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
		{
			sWinVersion = WIN_SEVEN;
			
			if(::GetVersionEx((LPOSVERSIONINFO)&OSversion))
			{
				if(OSversion.wProductType!=VER_NT_WORKSTATION)
					sWinVersion = WIN_SERVER_2008R2;
			}
		}
		else
		{
			// set Windows Vista Version
			OSversion.dwMajorVersion = 6;
			OSversion.dwMinorVersion = 0;
			if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
			{
				sWinVersion = WIN_VISTA;
				
				if(::GetVersionEx((LPOSVERSIONINFO)&OSversion))
				{
					if(OSversion.wProductType!=VER_NT_WORKSTATION)
						sWinVersion = WIN_SERVER_2008;
				}
			}
			else
			{
				// set Windows 2003 Server
				OSversion.dwMajorVersion = 5;
				OSversion.dwMinorVersion = 2;
				if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
					sWinVersion = WIN_SERVER_2003;
				else
				{
					// set Windows 2003 Server
					OSversion.dwMajorVersion = 5;
					OSversion.dwMinorVersion = 1;
					if ( VerifyVersionInfo(&OSversion, VER_MAJORVERSION | VER_MINORVERSION , dwlConditionMask) )
						sWinVersion = WIN_XP;
				}
			}
		}
		
		if(sWinVersion == WIN_UNKNOWN)
		{
			OSVERSIONINFO vers;
			vers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

			if (::GetVersionEx(&vers))
			{
				switch(vers.dwPlatformId)
				{
					case VER_PLATFORM_WIN32_NT:
						switch(vers.dwMajorVersion)
						{
						case 5:
							if(vers.dwMinorVersion==1)
								sWinVersion = WIN_XP;
							else if(vers.dwMinorVersion==2)
								sWinVersion = WIN_SERVER_2003;
							break;
						case 6:
							if(vers.dwMinorVersion==0)
								sWinVersion = WIN_VISTA;
							else if(vers.dwMinorVersion==1)
								sWinVersion = WIN_SEVEN;
							break;
						}
						break;
				}
			}
		}
	}
	xbox_assert(sWinVersion != WIN_UNKNOWN);
	return sWinVersion;
}


bool XWinSystem::GetMajorAndMinorFromSystemVersion( SystemVersion inSystemVersion, DWORD *outMajor, DWORD *outMinor)
{
	bool ok = true;
	switch( inSystemVersion)
	{
	case WIN_XP:			*outMajor = 5; *outMinor = 1; break;
	case WIN_SERVER_2003:	*outMajor = 5; *outMinor = 2; break;
	case WIN_VISTA:			*outMajor = 6; *outMinor = 0; break;
	case WIN_SERVER_2008:	*outMajor = 6; *outMinor = 0; break;
	case WIN_SERVER_2008R2:	*outMajor = 6; *outMinor = 1; break;
	case WIN_SEVEN:			*outMajor = 6; *outMinor = 1; break;
	default:
		xbox_assert( false);
		ok = false;
	}
	return ok;
}


bool XWinSystem::IsSystemVersionOrAbove( SystemVersion inSystemVersion)
{
	OSVERSIONINFOEXW version;

	::memset(&version, 0, sizeof(version));
	version.dwOSVersionInfoSize = sizeof(version);
	
	DWORDLONG conditionMask = 0;
	VER_SET_CONDITION( conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL );
	VER_SET_CONDITION( conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL );

	if (!GetMajorAndMinorFromSystemVersion( inSystemVersion, &version.dwMajorVersion, &version.dwMinorVersion))
		return false;

	return ::VerifyVersionInfoW( &version, VER_MAJORVERSION | VER_MINORVERSION , conditionMask) != 0;
}

void XWinSystem::GetProcessList(std::vector<pid> &pids)
{
	pids.clear();

	// It is a good idea to use a large array, because it is hard to predict how
	// many processes there will be at the time you call EnumProcesses. 
	pid processIds[2048];
	DWORD bytesReturned;

	static ENUMPROCESSES sPGPMI = NULL;

	if (sPGPMI == NULL)
	{
#if PSAPI_VERSION >= 2
		sPGPMI = (ENUMPROCESSES) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "K32EnumProcesses");
#else
		sPGPMI = (ENUMPROCESSES) GetProcAddress(GetModuleHandle(TEXT("psapi.dll")), "EnumProcesses");
#endif
	}

	if (sPGPMI != NULL)
	{
		if (sPGPMI(processIds, 2048 * sizeof(pid), &bytesReturned))
		{
			for (unsigned int i = 0; i < (bytesReturned / sizeof(pid)); ++i)
			{
				pids.push_back(processIds[i]);
			}
		}
	}
}

void XWinSystem::GetProcessName(uLONG processID, VString &outProcessName)
{
	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;
	DWORD dwFlags = TH32CS_SNAPPROCESS;

	hProcessSnap = CreateToolhelp32Snapshot(dwFlags, 0);

	if (NULL != hProcessSnap)
	{
		pe32.dwSize = sizeof( PROCESSENTRY32 );
		if (Process32First( hProcessSnap, &pe32 ))
		{
			do 
			{
				if (pe32.th32ProcessID == processID)
				{
					outProcessName = pe32.szExeFile;
					break;
				}
			} while( Process32Next( hProcessSnap, &pe32 ) );

		}
		CloseHandle(hProcessSnap);
	}
}

void XWinSystem::GetProcessPath(uLONG processID, VString &outProcessPath)
{
	HANDLE hProcessSnap;
	MODULEENTRY32 lpme32;
	DWORD dwFlags = TH32CS_SNAPMODULE; 

	hProcessSnap = CreateToolhelp32Snapshot(dwFlags, processID);

	if (NULL != hProcessSnap)
	{
		lpme32.dwSize = sizeof( MODULEENTRY32 );
		if (Module32First( hProcessSnap, &lpme32 ))
		{
			do 
			{
				if (lpme32.th32ProcessID == processID)
				{
					outProcessPath = lpme32.szExePath;
					break;
				}
			} while( Module32Next( hProcessSnap, &lpme32 ) );

		}
		CloseHandle(hProcessSnap);
	}
}

bool XWinSystem::KillProcess(uLONG inProcessID)
{
	bool result = false;

	HANDLE	hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, inProcessID );

	if ( NULL != hProcess )
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
		result = true;
	}

	return result;
}