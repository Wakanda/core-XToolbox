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

#include "VUDPEndPoint.h"


BEGIN_TOOLBOX_NAMESPACE


VUDPEndPoint::VUDPEndPoint(XUDPSock* inSock) : fSock(inSock)
{
	//empty
}


VUDPEndPoint::~VUDPEndPoint ()
{
	delete fSock;
}


void VUDPEndPoint::SetIsBlocking ( bool inIsBlocking )
{	
	if ( !fSock )
		return;
	
	fSock-> SetBlocking ( inIsBlocking );
}


VError VUDPEndPoint::Read(void *outBuffer, uLONG *ioLength, XNetAddr* outSenderInfo)
{
	VError verr=fSock->Read(outBuffer, ioLength, outSenderInfo);

	return verr==VE_OK ? VE_OK : VE_SRVR_READ_FAILED;
}


VError VUDPEndPoint::WriteExactly(void *inBuffer, uLONG inLength)
{
	VError verr=VE_OK;
	
	verr=fSock->Write(inBuffer, inLength, fDestination);
	
	return verr==VE_OK ? VE_OK : VE_SRVR_WRITE_FAILED;
}


VError VUDPEndPoint::Close ()
{
	if(fSock!=NULL)
		fSock->Close();

	delete fSock;

	fSock=NULL;

	return VE_OK;
}


VError VUDPEndPoint::SetDestination(const XNetAddr& inReceiverInfo)
{
	memcpy(&fDestination, &inReceiverInfo, sizeof(fDestination));
	
	return VE_OK;
}


VUDPEndPoint *VUDPEndPointFactory::CreateMulticastEndPoint(uLONG inMulticastIPv4, PortNumber inPort)
{
	uLONG localIp=0;

#if VERSIONWIN

	hostent* entries=gethostbyname(NULL);

	if(entries!=NULL && entries->h_addr_list[0]!=NULL)
	{
		in_addr* addr=reinterpret_cast<in_addr*>(entries->h_addr_list[0]);

		xbox_assert(addr!=NULL);

		localIp=ntohl(addr->S_un.S_addr);
	}
	else
	{
		//Multicast silently fail without a local interface IP.
		return NULL;
	}


#else

	//Let the kernel choose a local interface.
	localIp=INADDR_ANY;

#endif

	return CreateMulticastEndPoint(localIp, inMulticastIPv4, inPort);
}


VUDPEndPoint *VUDPEndPointFactory::CreateMulticastEndPoint(uLONG inLocalIpv4, uLONG inMulticastIPv4, PortNumber inPort)
{
	XUDPSock* xsock=XUDPSock::NewMulticastSock(inLocalIpv4, inMulticastIPv4, inPort);

	if(xsock==NULL)
		return NULL;


	VUDPEndPoint  *ep=new VUDPEndPoint(xsock);
	
	if(ep==NULL)
	{
		xsock->Close();

		delete xsock;

		vThrowError(VE_MEMORY_FULL);
	}

	XNetAddr mcast(inMulticastIPv4, inPort);

	ep->SetDestination(mcast);
	
	return ep;
}


END_TOOLBOX_NAMESPACE
