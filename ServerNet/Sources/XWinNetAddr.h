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
#ifndef __SNET_WIN_NET_ADDR__
#define __SNET_WIN_NET_ADDR__

#include <stdexcept>
#include <iterator>

#include <ws2tcpip.h>

#include "ServerNetTypes.h"


BEGIN_TOOLBOX_NAMESPACE


class XTOOLBOX_API XWinNetAddr
{
public :
	
	//Erreur de link sur ce symbol
	XWinNetAddr();
	
	XWinNetAddr(const sockaddr_storage& inSockAddr);
	
	XWinNetAddr(const sockaddr_in& inSockAddr);

	XWinNetAddr(const sockaddr_in6& inSockAddr);
	
	XWinNetAddr(const sockaddr* inSockAddr);	//To help working with Posix functions
		
	XWinNetAddr(IP4 inIp, PortNumber inPort);	//To help legacy code
		
	VError FromIP4AndPort(IP4 inIp, PortNumber inPort=kBAD_PORT);
	
	VError FromIP6AndPort(IP6 inIp, PortNumber inPort=kBAD_PORT);

	VError FromLocalAddr(Socket inSock);

	VError FromPeerAddr(Socket inSock);
	
	VError FromIpAndPort(const VString& inIP, PortNumber inPort=kBAD_PORT);

	VError FromIpAndPort(sLONG inFamily, const VString& inIP, PortNumber inPort=kBAD_PORT);
	
	void SetAddr(const sockaddr_storage& inSockAddr);
	
	const sockaddr* GetAddr() const;
	
	sLONG GetAddrLen() const;
		
	VString GetIP(sLONG* outVersion=NULL) const;
			
	PortNumber GetPort() const;

	IP4 GetIPv4HostOrder() const; //helps legacy code, do not use !
	
	void FillAddrStorage(sockaddr_storage* outSockAddr) const;

	sLONG GetPfFamily() const;
	
	const sockaddr* GetSockAddr() const;
	
	
private :
	
	sockaddr_storage fSockAddr;	//sizeof(XWinNetAddr)=136
};

typedef XWinNetAddr XNetAddr;


class VNetAddrList;

class XTOOLBOX_API XWinAddrLocalQuery
{
public :	
	XWinAddrLocalQuery(VNetAddrList* outList) : fVAddrList(outList) {}

	VError FillAddrList();
	
private :
	VNetAddrList* fVAddrList;
};

typedef XWinAddrLocalQuery XAddrLocalQuery;


class XTOOLBOX_API XWinAddrDnsQuery
{
public :	
	XWinAddrDnsQuery(VNetAddrList* outList) : fVAddrList(outList) {}
	
	typedef enum {TCP, UDP} Protocol;

	VError FillAddrList(const VString& inDnsName, PortNumber inPort, Protocol inProto=TCP);
	
private :
	VNetAddrList* fVAddrList;
};

typedef XWinAddrDnsQuery XAddrDnsQuery;


#define DECLARE_XNETADDR_FRIENDSHIP friend class XWinAddrLocalQuery; friend class XWinAddrDnsQuery;


END_TOOLBOX_NAMESPACE


#endif
