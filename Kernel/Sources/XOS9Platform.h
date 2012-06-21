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
#ifndef __XOS9Platform__
#define __XOS9Platform__

	// Platform & compiler macros
	#define VERSIONMAC	1
	#define VERSIONWIN	0
	#define VERSION_MACHO	0
	#define VERSION_DARWIN	0
	
	//Penser à tester pour les architectures 64 bits
	#if defined (__LP64__) || (_LP64) || ( __WORDSIZE == 64)
		#define ARCH_32			0
		#define ARCH_64			1
	#else
		#define ARCH_32			1
		#define ARCH_64			0
	#endif

	#define COMPIL_VISUAL 0
	#define COMPIL_CODEWARRIOR 1
	#define COMPIL_GCC 0

	// Processor & memory macros
	#define BIGENDIAN	1
	#define SMALLENDIAN	0
	#define NATIVE_64_BITS	0

	#define VTC_WCHAR	VTC_UTF_16
	#define WCHAR_IS_UNICHAR	0

	// Compiler relative macros & settings
	#define EXPORT_API	__declspec (export)
	#define IMPORT_API	__declspec (import)
	#define EXPORT_UNMANGLED_API(_return_type_)	extern "C" __declspec (export) _return_type_

	// Required includes
	#include <ansi_parms.h>
	#include <div_t.h>
	#include <stat.h>
	#include <null.h>
	#include <size_t.h>
	#include <unistd.h>
	#include <utime.h>
	#include <va_list.h>
	#include <wchar.h>

	#include <ctype.h>
	#include <errno.h>
	#include <fcntl.h>
	#include <float.h>
	#include <limits.h>
	#include <locale.h>

#ifndef __SANE__
	#include <math.h>
#endif

	#include <setjmp.h>
	#include <signal.h>
	#include <stdarg.h>
	#include <stddef.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <time.h>

	#include <typeinfo.h>

	#define TARGET_API_MAC_CARBON	1
	#define PM_USE_SESSION_APIS		0
	
	#include "Carbon.h"

#endif