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

#include "Tools.h"

#include "ICriticalError.h"
#include "VSslDelegate.h"
#include "XML/VXML.h" /* For VLocalizationManager */


#if VERSIONMAC || VERSION_LINUX
	#include <netdb.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <ifaddrs.h>
#endif


BEGIN_TOOLBOX_NAMESPACE


VServerNetManager*	VServerNetManager::sInstance=NULL;


VServerNetManager::VServerNetManager() :
	fCriticalError(NULL), fDefaultClientIdleTimeOut(20000 /*20s*/),
	fSelectIOInactivityTimeOut(300 /*ms*/), fEndPointCounter(0), fIpPolicy(DefaultPolicy)
#if VERSIONWIN
	,fInetPtoN(NULL), fInetNtoP(NULL)
#endif
{	
	VError verr=SslFramework::Init();
	xbox_assert(verr==VE_OK);

#if VERSIONWIN
	fInetPtoN=(InetPtoNPtr)::GetProcAddress(GetModuleHandle("Ws2_32.dll"), "inet_pton");
	fInetNtoP=(InetNtoPPtr)::GetProcAddress(GetModuleHandle("Ws2_32.dll"), "inet_ntop");
#endif
}


VServerNetManager::~VServerNetManager()
{
	VErrorBase::UnregisterLocalizer(kSERVER_NET_SIGNATURE);
	
	VError verr=SslFramework::DeInit();
	xbox_assert(verr==VE_OK);
}


#if VERSIONWIN

sLONG VServerNetManager::InetPtoN(sLONG inFamily, const char* inSrc, void* outDst) const
{
	if(fInetPtoN==NULL)
		return -1;	// error code for 'some system error occurred'
	
	return fInetPtoN(inFamily, inSrc, outDst);
}


const char* VServerNetManager::InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, size_t inDstLen) const
{
	if(fInetNtoP==NULL)
		return NULL;	// generic error
	
	return fInetNtoP(inFamily, inSrc, outDst, inDstLen);
}

#endif


//static
VServerNetManager* VServerNetManager::Get()
{
	if(sInstance==NULL)
		sInstance=new VServerNetManager();
		
	xbox_assert(sInstance!=NULL);
	
	return sInstance;
}


//static
void VServerNetManager::Init(ICriticalError* inCriticalError, IpPolicy inIpPolicy)
{
	//Call Get() to force new VServerNetManager() and SslFramework::Init()
	VServerNetManager* manager=VServerNetManager::Get();
	xbox_assert(manager!=NULL);
	
	if(manager!=NULL && inCriticalError!=NULL && manager->fCriticalError==NULL)
		manager->fCriticalError=inCriticalError;
	
	if(manager!=NULL)
		manager->fIpPolicy=inIpPolicy;
}


//static
void VServerNetManager::DeInit()
{
	//ILocalizer and ICriticalError have no destructor ; nothing to do
}


sLONG VServerNetManager::GetDefaultClientIdleTimeOut()
{
	return fDefaultClientIdleTimeOut; 
}


void VServerNetManager::SetDefaultClientIdleTimeOut(sLONG inTimeOut)
{
	VInterlocked::Exchange(&fDefaultClientIdleTimeOut, inTimeOut);
}

	
sLONG VServerNetManager::GetSelectIODelay()
{
	return fSelectIOInactivityTimeOut; 
}


void VServerNetManager::SetSelectIODelay(sLONG inDelay)
{
	VInterlocked::Exchange(&fSelectIOInactivityTimeOut, inDelay); 
}


sLONG VServerNetManager::GetNextSimpleID()
{
	return VInterlocked::Increment(&fEndPointCounter);
}


//static
VError VServerNetManager::SignalCriticalError(VError inError)
{
	xbox_assert(fCriticalError!=NULL);
	
	if(fCriticalError!=NULL)
		fCriticalError->SignalCriticalNetworkError(inError);
		
	return inError;
}


IpPolicy VServerNetManager::GetIpPolicy() const
{
	return fIpPolicy;
}


IpPolicy ServerNetTools::GetIpPolicy()
{
	return VServerNetManager::Get()->GetIpPolicy();
}


bool ServerNetTools::AcceptFromV6()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV6;
}

bool ServerNetTools::AcceptFromV4()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV4;
}


bool ServerNetTools::ConnectToV6()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV6;
}


bool ServerNetTools::ConnectToV4()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV4;
}


bool ServerNetTools::ConvertFromV6()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV6;
}


bool ServerNetTools::ConvertFromV4()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV4;
}


bool ServerNetTools::ResolveToV6()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV6;
}


bool ServerNetTools::ResolveToV4()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV4;
}


bool ServerNetTools::ListV6()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV6;
}


bool ServerNetTools::ListV4()
{
	IpPolicy policy=GetIpPolicy();
	
	return policy==DualStack || policy==ForceV4;
}


//TODO : Legacy code ; Need rewrite
uLONG ServerNetTools::GetEncryptedPKCS1DataSize(uLONG inKeySize /* 128 for 1024 RSA; X/8 for X RSA*/, uLONG inDataSize)
{
	return SslFramework::GetEncryptedPKCS1DataSize(inKeySize, inDataSize);
}
	
	
VError ServerNetTools::Encrypt(uCHAR* inPrivateKeyPEM, uLONG inPrivateKeyPEMSize, uCHAR* inData, uLONG inDataSize,
							   uCHAR* ioEncryptedData, uLONG* ioEncryptedDataSize)
{
	
	if(inPrivateKeyPEM==NULL || inPrivateKeyPEMSize<886 || inData==NULL || inDataSize==0 ||
	   ioEncryptedData==NULL || ioEncryptedDataSize==0)
		return VE_SRVR_INVALID_PARAMETER;
	
	//Copy/Paste from Security::Encrypt
	int nTenIndex = 31;

	while( nTenIndex <= 811 )
	{
		inPrivateKeyPEM [ nTenIndex ] = 10;
		nTenIndex += 65;
	}
	
	inPrivateKeyPEM [ 856 ] = 10;
	inPrivateKeyPEM [ 886 ] = 10;
	
	VError verr=SslFramework::Encrypt(inPrivateKeyPEM, inPrivateKeyPEMSize, inData, inDataSize, ioEncryptedData, ioEncryptedDataSize);
	
	return verr;
}


sLONG ServerNetTools::GetSelectIODelay()
{
	return VServerNetManager::Get()->GetSelectIODelay();
}


void ServerNetTools::SetSelectIODelay(sLONG inDelay)
{
	VServerNetManager::Get()->SetSelectIODelay(inDelay);
}


sLONG ServerNetTools::GetNextSimpleID()
{
	return VServerNetManager::Get()->GetNextSimpleID();
}


VError ServerNetTools::ThrowNetError(VError inError, VString const* inKey1, VString const* inKey2)
{	
	VErrorBase*	verrBase=new VErrorBase(inError, 0);
	
	if(inKey1!=NULL)
	{
		VValueBag* vvBagParameters=verrBase->GetBag();
		
		if(vvBagParameters!=NULL)
		{
			vvBagParameters->SetString("key1", *inKey1);
			
			if(inKey2!=NULL)
				vvBagParameters->SetString("key2", *inKey2);
		}
	}
	
	VTask::GetCurrent()->PushRetainedError(verrBase);
	
	return inError;
}


VError ServerNetTools::vThrowNativeCombo(VError inImplErr, int inErrno)
{
	VErrorTaskContext* errCtx=VTask::GetErrorContext(false);

	if(errCtx!=NULL)
	{		
		//Do not push an error that is arlready in the stack. Prevent uncontrolled stack growth in mad loops.
		bool foundImplErr=(errCtx->Find(inImplErr)!=NULL);
	
		if(!foundImplErr)
		{
			//Exact native error - ie ECONNRESET, EBADF or ENOTSOCK (these are not the same !)
			vThrowNativeError(inErrno);
	
			//Value added error usefull for VStuff.cpp - ie VE_SOCK_READ_FAILED
			return vThrowError(inImplErr);
		}
	}
	
	return inImplErr;
}


VError ServerNetTools::SignalNetError(VError inError)
{
 	return VServerNetManager::Get()->SignalCriticalError(inError);
}


sLONG ServerNetTools::GetDefaultClientIdleTimeOut()
{
	return VServerNetManager::Get()->GetDefaultClientIdleTimeOut();
}


void ServerNetTools::SetDefaultClientIdleTimeOut(sLONG inTimeOut)
{
	return VServerNetManager::Get()->SetDefaultClientIdleTimeOut(inTimeOut);
}


void ServerNetTools::SetDefaultSSLPrivateKey ( char* inKey, uLONG inSize )
{
	VMemoryBuffer<> keyBuffer;
	
	keyBuffer.PutData(0 /*offset in mem buffer*/, inKey, inSize);
	
	VError verr=SslFramework::SetDefaultPrivateKey(keyBuffer);
	
	xbox_assert(verr==VE_OK);
}


void ServerNetTools::SetDefaultSSLCertificate ( char* inCertificate, uLONG inSize )
{
	VMemoryBuffer<> certBuffer;
	
	certBuffer.PutData(0 /*offset in mem buffer*/, inCertificate, inSize);
	
	VError verr=SslFramework::SetDefaultCertificate(certBuffer);
	
	xbox_assert(verr==VE_OK);
}


sLONG ServerNetTools::FillDumpBag(VValueBag* ioBag, const void* inPayLoad, sLONG inPayLoadLen, sLONG inOffset)
{	
	if(ioBag==NULL || inPayLoad==NULL || inPayLoadLen<=0 || inOffset>=inPayLoadLen)
		return kMAX_sLONG;

	const char* plStart=reinterpret_cast<const char*>(inPayLoad)+inOffset;
	const char* plPast=plStart+(inPayLoadLen-inOffset<100 ? inPayLoadLen-inOffset : 100);
	
	int len=plPast-plStart;
	
	char bufStart[100];

	char *bufPast=bufStart+len;
	
	const char* plPos=NULL;
	char* bufPos=NULL;
	
#if VERSIONWIN
	for(plPos=plStart, bufPos=bufStart ; bufPos<bufPast ; plPos++, bufPos++)
		*bufPos=(isprint((unsigned char)(*plPos)) ? *plPos : '.');
#else
	for(plPos=plStart, bufPos=bufStart ; bufPos<bufPast ; plPos++, bufPos++)
		*bufPos=(isprint(*plPos) ? *plPos : '.');
#endif

	VString dump(bufStart, len, VTC_US_ASCII);

	ILoggerBagKeys::dump_offset.Set(ioBag, inOffset);
	ILoggerBagKeys::dump_buffer.Set(ioBag, dump);
	
	return inOffset+len;
}


sLONG XTOOLBOX_API ServerNetTools::InetPtoN(sLONG inFamily, const char* inSrc, void* outDst)
{
	sLONG res=0;

#if VERSIONWIN
	res=VServerNetManager::Get()->InetPtoN(inFamily, inSrc, outDst);
#else
	res=inet_pton(inFamily, inSrc, outDst);
#endif

	return res;
}


XTOOLBOX_API const char* ServerNetTools::InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, sLONG inDstLen)
{
	const char* res=NULL;

#if VERSIONWIN
	res=VServerNetManager::Get()->InetNtoP(inFamily, inSrc, outDst, inDstLen);
#else
	res=inet_ntop(inFamily, inSrc, outDst, inDstLen);
#endif

	return res;
}


////////////////////////////////////////////////////////////////////////////////
//
// Deprecated IP v4 only stuff
//
////////////////////////////////////////////////////////////////////////////////


#if WITH_DEPRECATED_IPV4_API


long ServerNetTools::ResolveAddress (const XBOX::VString& inHostName, XBOX::VString *outIPv4String)
{
	char			hostName[256];
	struct hostent	*hostEnt = NULL;
	unsigned long	ip = 0;
	
	inHostName.ToCString (hostName, sizeof(hostName));
	
	if ((hostEnt = gethostbyname (hostName)) != NULL)
	{
		ip = ntohl (*((unsigned long *)(hostEnt->h_addr_list[0])));
		
		if (outIPv4String)
		{
			struct in_addr	inAddr = {0};
			
			inAddr.s_addr = *((unsigned long *)(hostEnt->h_addr_list[0]));
			
			char *szchIP = inet_ntoa (inAddr);
			if (szchIP)
				outIPv4String->AppendCString (szchIP);
		}
	}
	
	return ip;
}


void ServerNetTools::GetLocalIPAdresses( std::vector< XBOX::VString >& outAddresses )
{
	std::vector<IP4> ipAddresses;
	GetLocalIPv4Addresses( ipAddresses );
	for ( int i = 0; i < ipAddresses.size(); i++ )
	{
		VString str;
		GetIPAdress( ipAddresses[ i ], str );
		outAddresses.push_back( str );
	}
}


uLONG ServerNetTools::GetIPAddress ( VString const & inDottedIPAddress )
{
	char			szchIP [ 64 ];
	inDottedIPAddress. ToCString( szchIP, 64 );
	
	return htonl ( inet_addr ( szchIP ) );
}


void ServerNetTools::GetIPAdress( IP4 inIPAdress, XBOX::VString& outDottedIPAddress)
{
	struct in_addr			inAddr;
	char*					szchIP;
	
	inAddr. s_addr = htonl ( inIPAdress );
	szchIP = inet_ntoa ( inAddr );
	
	outDottedIPAddress.Clear();
	
	if ( szchIP )
		outDottedIPAddress. AppendCString ( szchIP );
	else
		outDottedIPAddress. AppendCString ( "Null IP" );
}


#if VERSIONWIN

sLONG ServerNetTools::GetLocalIPv4Addresses (std::vector<IP4>& outIPv4Addresses)
{
	char	hostName[256];
	
	outIPv4Addresses.clear();
	if (gethostname (hostName, sizeof(hostName)) != -1)
	{
		struct	hostent	*hostEnt = NULL;
		struct in_addr	addr;
		
		if ((hostEnt = gethostbyname (hostName)) != NULL)
		{
			for (sLONG i = 0; (hostEnt->h_addr_list[i] != NULL); ++i)
			{
				memcpy (&addr, hostEnt->h_addr_list[i], sizeof(struct in_addr));

				outIPv4Addresses.push_back (ntohl (addr.S_un.S_addr));
			}
		}
	}
	
	return static_cast<sLONG>(outIPv4Addresses.size());
}

#else

sLONG ServerNetTools::GetLocalIPv4Addresses (std::vector<IP4>& outIPv4Addresses)
{
	ifaddrs* ifList=NULL;
	
	if(getifaddrs(&ifList)==-1)
	{
		vThrowNativeError(errno);
		
		return 0;
	}
	
	ifaddrs* ifPtr=NULL;
	
	for (ifPtr=ifList ; ifPtr!=NULL ; ifPtr=ifPtr->ifa_next)
	{
		if (ifPtr->ifa_addr==NULL)
			continue;
		
		int family=ifPtr->ifa_addr->sa_family;
		
		if(family==AF_INET)
		{
			/*char host[NI_MAXHOST];
			 
			 int res=getnameinfo(ifPtr->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			 
			 if(res!=0)
			 {
			 vThrowNativeError(errno);
			 continue;
			 }*/
			
			sockaddr_in* addr=reinterpret_cast<sockaddr_in*>(ifPtr->ifa_addr);
			
			IP4 ip=static_cast<IP4>(addr->sin_addr.s_addr);
			
			outIPv4Addresses.push_back(ntohl(ip));
		}
	}
	
	freeifaddrs(ifList);
	
	return static_cast<sLONG>(outIPv4Addresses.size());
}

#endif

#endif


END_TOOLBOX_NAMESPACE
