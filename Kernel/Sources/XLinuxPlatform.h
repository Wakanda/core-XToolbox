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
#ifndef __XLinuxPlatform__
#define __XLinuxPlatform__

// Platform & compiler macros
#define VERSIONMAC      0
#define VERSIONWIN      0
#define VERSION_LINUX   1
#define VERSION_DARWIN  0
#define VERSION_MACHO   0

//We want to compile as much Linux code as possible on XCode, but sometimes the code is really plateform specific,
//and we have to use VERSION_LINUX_STRICT to keep XCode away... Of course, when compiled this way the code may lack
//some features. Also note that some Linux code may compile with XCode but not work (code that read /proc and /sys for
//ex.). The point is : we want the Linux code to compile on XCode. It's better if it works, but it's not required.
//Although VERSION_LINUX_ON_XCODE is not necessary, in some cases it helps to keep #if stuff short and readable.

#ifdef __MACH__
	#define VERSION_LINUX_STRICT	0
    #define VERSION_LINUX_ON_XCODE  1
#else
	#define VERSION_LINUX_STRICT	1
	#define VERSION_LINUX_ON_XCODE	0
#endif 

//Test de LP64 pour ppc64 et x86_64
#if defined (__x86_64__)
    #define ARCH_32     0
    #define ARCH_64     1
    #define ARCH_386    1
    #define ARCH_POWER  0
    #define BIGENDIAN   0
    #define SMALLENDIAN	1
#else
    #error Need x86_64 support !
#endif

#if defined(__GNUC__)
    #define COMPIL_VISUAL       0
    #define COMPIL_CODEWARRIOR	0
    #define COMPIL_GCC          1
    #else
    #error Need gcc !
#endif

// Guess VERSIONDEBUG if not defined yet
#ifndef VERSIONDEBUG
    #if COMPIL_GCC
	#if __OPTIMIZE__
	    #define VERSIONDEBUG    0
	#else
	    #define VERSIONDEBUG    1
	#endif
    #endif
#endif

// define NDEBUG to disable assert macro in ANSI header file
#if !VERSIONDEBUG
    #define NDEBUG
#endif

//Pas sur pour ceux la !
#define VTC_WCHAR           VTC_UTF_32_RAW
#define WCHAR_IS_UNICHAR    0

//#define U_DISABLE_RENAMING 1    //Prevent ICU from renaming symbols
//#define icu icu_3_8

// Compiler relative macros & settings
#if defined(__GNUC__)
    #define EXPORT_API __attribute__((visibility("default")))
    #define IMPORT_API __attribute__((visibility("default")))
    // GCC 4.0.1 does not support yet the visibility tag on instantiated templates
    #define EXPORT_TEMPLATE_API
    #define IMPORT_TEMPLATE_API
    #define EXPORT_UNMANGLED_API(_return_type_) extern "C" __attribute__((visibility("default"))) _return_type_
#endif

#if COMPIL_GCC
    #include <stdbool.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/param.h>
    #include <sys/mount.h>
    #include <pthread.h>
    #include <semaphore.h>
    #include <iterator>
    #include <typeinfo>
#endif

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

// STL
#include <vector>

#if COMPIL_GCC
    #include <ext/hash_map>
    // namespace for non-standard yet hash_map
    #define STL_HASH_MAP ::__gnu_cxx
#endif

#if COMPIL_GCC
    #undef Free
#endif

//Pas sur pour la suite

#ifndef PM_USE_SESSION_APIS
    #define PM_USE_SESSION_APIS	0
#endif

#ifndef _MSL_ISO646_H
    #define _MSL_ISO646_H   // ciso646.h
#endif

#ifndef _MSL_TGMATH_H
    #define _MSL_TGMATH_H   // tgmath.h
#endif

#ifndef _MSL_WCHAR_H
    #define _MSL_WCHAR_H    // wchar.h
#endif

#ifndef _MSL_WCTYPE_H
    #define _MSL_WCTYPE_H   // wctype.h
#endif

#endif
