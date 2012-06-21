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
#ifndef __SNET_V_NET_ADDR__
#define __SNET_V_NET_ADDR__

#include <stdexcept>
#include <iterator>
#include <list>


#include "ServerNetTypes.h"

#if VERSIONWIN
	#include "XWinNetAddr.h"
#else
	#include <sys/socket.h>
	#include <netinet/in.h>

	#include "XBsdNetAddr.h"
#endif

#include "Tools.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


class XTOOLBOX_API VNetAddress : public VObject
{
public :
	
	VNetAddress();
	VNetAddress(const sockaddr_storage& inSockAddr);
	VNetAddress(const sockaddr_in& inSockAddr);
	VNetAddress(const sockaddr_in6& inSockAddr);
	VNetAddress(const XNetAddr& inXAddr);
	VNetAddress(IP4 inIpV4, PortNumber inPort=kBAD_PORT); 
	VNetAddress(const VString& inIp, PortNumber inPort=kBAD_PORT);

	VError FromLocalAddr(Socket inSock);
	VError FromPeerAddr(Socket inSock);
	VError FromIpAndPort(const VString& inIP, PortNumber inPort=kBAD_PORT);
	VError FromAnyIpAndPort(PortNumber inPort);

	void SetAddr(const sockaddr_storage& inSockAddr);
	const sockaddr* GetAddr() const;
	sLONG GetAddrLen() const;

	VString GetIP(sLONG* outVersion=NULL) const;
	bool IsV4() const;
	bool IsV6() const;
	PortNumber GetPort() const;

	void FillAddrStorage(sockaddr_storage* outSockAddr) const;
	
	//To help socket creation - need some work :)
	
	sLONG GetPfFamily() const;
	
	//sLONG GetSockType() {return SOCK_STREAM; /*todo*/}
	
	//sLONG GetProtocol {return 0; /*IPPROTO_TCP todo*/}
	
private :
	
	XNetAddr fNetAddr;
};


class XTOOLBOX_API VNetAddrList
{
public :

	VError FromLocalInterfaces();
	
	VError FromDnsQuery(const VString& inDnsName, PortNumber inPort);
	
	class const_iterator : public std::iterator<std::forward_iterator_tag, VNetAddress>
	{
	public :
		const_iterator();		
		
		bool operator==(const const_iterator& other);
		bool operator!=(const const_iterator& other);
		
		const_iterator& operator++();
		const_iterator operator++(int);
		
		const VNetAddress& operator*();	
		const VNetAddress* operator->();
		
	private :
		
		friend class VNetAddrList;
		const_iterator(std::list<VNetAddress>::const_iterator inBeginIt);
		
		std::list<VNetAddress>::const_iterator fAddrIt;
	};
	
	const_iterator begin();
	
	const const_iterator end();
	
private :
	
	DECLARE_XNETADDR_FRIENDSHIP //jmo - Pas de friend sur un typedef ? C'est NUL !

	void PushXNetAddr(const XNetAddr& inNetAddr);
	
	std::list<VNetAddress> fAddrList;

};


END_TOOLBOX_NAMESPACE


#endif
