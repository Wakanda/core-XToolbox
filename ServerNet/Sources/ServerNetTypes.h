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
#include "ServerNetArchTypes.h"
#include "ServerNetSystemTypes.h"


#ifndef __SNET_TYPES__
#define __SNET_TYPES__


BEGIN_TOOLBOX_NAMESPACE


class VEndPoint;
class VTCPEndPoint;
class VTCPClientSession;
class VTCPServerSession;

class VConnectionHandler;
class VConnectionHandlerQueue;
class VConnectionHandlerFactory;
class VTCPConnectionHandlerFactory;
class VWorkerPool;
class VTCPSelectIOPool;

typedef enum {DualStack, ForceV4, ForceV6, DefaultPolicy=ForceV4} IpPolicy;


#define WITH_SHARED_WORKERS 0


enum
{
	kServerNetTaskKind = 'SNET'
};

enum
{
	kSNET_ConnectionListenerTaskKindData = 1,
	kSNET_SessionManagerTaskKindData = 2
};

enum
{
	kWorkerPool_InUseTaskKind = 'WKPL',
	kWorkerPool_SpareTaskKind = 'WKPS',
};

// classic functor for quick deletion of containers objects (m.c)
template<class T> struct del_fun_t 
{
	del_fun_t& operator()(T* p)
	{ 
		delete p;
		return *this;
	}
};

template<class T> del_fun_t<T> del_fun() 
{ 
	return del_fun_t<T>(); 
};


END_TOOLBOX_NAMESPACE

class IRequestLogger;


//#define INET6_ADDRSTRLEN	46
//typedef char IP[46]; //DEPRECATED !


#ifndef WITH_DEPRECATED_IPV4_API
	#define WITH_DEPRECATED_IPV4_API 1
#endif

#ifndef DEPRECATED_IPV4_API_SHOULD_NOT_COMPILE
	#define DEPRECATED_IPV4_API_SHOULD_NOT_COMPILE 0
#endif

/*
#if WITH_DEPRECATED_IPV4_API
	old code
#elif DEPRECATED_IPV4_API_SHOULD_NOT_COMPILE
	#error NEED AN IP V6 UPDATE
#endif
*/

//#if WITH_DEPRECATED_IPV4_API
//typedef uLONG IpAliasType;
//#else
//typedef XBOX::VString IpAliasType;
//#endif

#endif
