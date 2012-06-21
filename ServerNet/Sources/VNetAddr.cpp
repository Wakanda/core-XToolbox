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
#include "VServerNetPrecompiled.h"


#include "VNetAddr.h"

#include "Tools.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;

VNetAddress::VNetAddress() {/*vide pour l'instant*/}

VNetAddress::VNetAddress(const sockaddr_storage& inSockAddr) : fNetAddr(inSockAddr) {}

VNetAddress::VNetAddress(const sockaddr_in& inSockAddr) : fNetAddr(inSockAddr) {}

VNetAddress::VNetAddress(const sockaddr_in6& inSockAddr) : fNetAddr(inSockAddr) {}

VNetAddress::VNetAddress(const XNetAddr& inXAddr) : fNetAddr(inXAddr) {}
	
VNetAddress::VNetAddress(IP4 inIpV4, PortNumber inPort) { vThrowError(fNetAddr.FromIP4AndPort(inIpV4, inPort)); }

VNetAddress::VNetAddress(const VString& inIp, PortNumber inPort) { vThrowError(fNetAddr.FromIpAndPort(inIp, inPort)); }

bool VNetAddress::IsV4() const { return true; }

bool VNetAddress::IsV6() const { return true; }

VError VNetAddress::FromLocalAddr(Socket inSock)
{
	return fNetAddr.FromLocalAddr(inSock);
}

VError VNetAddress::FromPeerAddr(Socket inSock)
{
	return fNetAddr.FromPeerAddr(inSock);
}

VError VNetAddress::FromIpAndPort(const VString& inIP, PortNumber inPort)
{
	//return fNetAddr.FromIpAndPort(inIP, inPort);
	return VE_OK;
}

VError VNetAddress::FromAnyIpAndPort(PortNumber inPort)
{
	if(ConvertFromV6())
		return fNetAddr.FromIP6AndPort(in6addr_any, inPort);

	return fNetAddr.FromIP4AndPort(INADDR_ANY, inPort);
}

void VNetAddress::SetAddr(const sockaddr_storage& inSockAddr)
{
	return fNetAddr.SetAddr(inSockAddr);
}

const sockaddr* VNetAddress::GetAddr() const
{
	return fNetAddr.GetAddr();
}

sLONG VNetAddress::GetAddrLen() const
{
	return fNetAddr.GetAddrLen();
}

VString VNetAddress::GetIP(sLONG* outVersion) const
{
	return fNetAddr.GetIP(outVersion);
}

PortNumber VNetAddress::GetPort() const
{
	return fNetAddr.GetPort();
}

void VNetAddress::FillAddrStorage(sockaddr_storage* outSockAddr) const
{
	return fNetAddr.FillAddrStorage(outSockAddr);
}

sLONG VNetAddress::GetPfFamily() const
{
	return fNetAddr.GetPfFamily();
}

VError VNetAddrList::FromLocalInterfaces()
{
	XAddrLocalQuery query(this);
	
	return query.FillAddrList();
}

VError VNetAddrList::FromDnsQuery(const VString& inDnsName, PortNumber inPort)
{
	XAddrDnsQuery query(this);
	
	return query.FillAddrList(inDnsName, inPort);
}

VNetAddrList::const_iterator::const_iterator() {}		

bool VNetAddrList::const_iterator::operator==(const const_iterator& other)
{
	return fAddrIt==other.fAddrIt;
}

bool VNetAddrList::const_iterator::operator!=(const const_iterator& other)
{
	return fAddrIt!=other.fAddrIt;
}

VNetAddrList::const_iterator& VNetAddrList::const_iterator::operator++()
{
	++fAddrIt;

	return *this;
}

VNetAddrList::const_iterator VNetAddrList::const_iterator::operator++(int)
{
	const_iterator tmp(*this);
	
	++fAddrIt;
	
	return tmp;
}

const VNetAddress& VNetAddrList::const_iterator::operator*()
{
	return *fAddrIt;
}

const VNetAddress* VNetAddrList::const_iterator::operator->()
{
	return &(*fAddrIt);
}

VNetAddrList::const_iterator::const_iterator(std::list<VNetAddress>::const_iterator inBeginIt) : fAddrIt(inBeginIt) {}

VNetAddrList::const_iterator VNetAddrList::begin()
{
	return const_iterator(fAddrList.begin());
}

const VNetAddrList::const_iterator VNetAddrList::end()
{
	return(const_iterator(fAddrList.end()));
}

void VNetAddrList::PushXNetAddr(const XNetAddr& inNetAddr)
{
	fAddrList.push_back(VNetAddress(inNetAddr));
}


END_TOOLBOX_NAMESPACE
