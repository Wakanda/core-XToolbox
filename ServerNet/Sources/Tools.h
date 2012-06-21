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
#include "ServerNetTypes.h"


#ifndef __SNET_TOOLS__
#define __SNET_TOOLS__


BEGIN_TOOLBOX_NAMESPACE

class ICriticalError;
class VLocalizationManager;


namespace ServerNetTools
{
	IpPolicy XTOOLBOX_API GetIpPolicy();

	bool XTOOLBOX_API AcceptFromV6();
	bool XTOOLBOX_API AcceptFromV4();

	bool XTOOLBOX_API ConnectToV6();
	bool XTOOLBOX_API ConnectToV4();

	bool XTOOLBOX_API ConvertFromV6();
	bool XTOOLBOX_API ConvertFromV4();
	
	bool XTOOLBOX_API ResolveToV6();
	bool XTOOLBOX_API ResolveToV4();
	
	bool XTOOLBOX_API ListV6();
	bool XTOOLBOX_API ListV4();
	
	void XTOOLBOX_API SetDefaultSSLPrivateKey(char* inKey, uLONG inSize);
	void XTOOLBOX_API SetDefaultSSLCertificate(char* inCertificate, uLONG inSize);

	sLONG XTOOLBOX_API GetSelectIODelay();
	void XTOOLBOX_API SetSelectIODelay(sLONG inDelay);
	
	sLONG XTOOLBOX_API GetDefaultClientIdleTimeOut();
	void XTOOLBOX_API SetDefaultClientIdleTimeOut(sLONG inTimeOut);
	
	//TODO - Legacy code ; Need rewrite.
	uLONG XTOOLBOX_API GetEncryptedPKCS1DataSize ( uLONG inKeySize /* 128 for 1024 RSA; X/8 for X RSA*/, uLONG inDataSize );
	VError XTOOLBOX_API Encrypt (uCHAR* inPrivateKeyPEM, uLONG inPrivateKeyPEMSize, uCHAR* inData, uLONG inDataSize, uCHAR* ioEncryptedData, uLONG* ioEncryptedDataSize);

	sLONG XTOOLBOX_API GetNextSimpleID();
	
	VError XTOOLBOX_API ThrowNetError(VError inError, VString const* inKey1=0, VString const* inKey2=0);
	VError XTOOLBOX_API vThrowNativeCombo(VError inImplErr, int inErrno);
	VError XTOOLBOX_API SignalNetError(VError inError);
	
	sLONG XTOOLBOX_API FillDumpBag(VValueBag* ioBag, const void* inPayLoad, sLONG inPayLoadLen, sLONG inOffset);

	XTOOLBOX_API sLONG InetPtoN(sLONG inFamily, const char* inSrc, void* outDst);
	XTOOLBOX_API const char* InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, sLONG inDstLen);


	////////////////////////////////////////////////////////////////////////////////
	//
	// Deprecated IP v4 only stuff
	//
	////////////////////////////////////////////////////////////////////////////////
	
#if WITH_DEPRECATED_IPV4_API
	sLONG XTOOLBOX_API GetLocalIPv4Addresses (std::vector<IP4>& outIPv4Addresses);
	uLONG XTOOLBOX_API GetIPAddress ( VString const & inDottedIPAddress );
	void XTOOLBOX_API GetIPAdress( IP4 inIPAdress, XBOX::VString& outDottedIPAddress );
	void XTOOLBOX_API GetLocalIPAdresses( std::vector< XBOX::VString >& outAddresses );	
	long XTOOLBOX_API ResolveAddress (const XBOX::VString& inHostName, XBOX::VString *outIPv4String = NULL);
#endif	

};


//StErrorContextInstaller(bool inKeepingErrors, bool inSilentContext)

//An error context for loop, to flush errors on each iteration.
class StDropErrorContext : public StErrorContextInstaller
{
public :

	StDropErrorContext() : StErrorContextInstaller(false /*we drop errors*/, false /*popups are allowed*/) { }
};


//An error context fore place where we are interested in errors, but don't want popups
class StSilentErrorContext : public StErrorContextInstaller
{
public :

	StSilentErrorContext() : StErrorContextInstaller(true /*we keep errors*/, true /*from now on, popups are forbidden*/) { }
};


//An error context for places where we are not interested in errors that may preceed a success
class StTmpErrorContext : public StErrorContextInstaller
{
public :
	
	StTmpErrorContext() : StErrorContextInstaller(true /*we keep errors*/, true /*from now on, popups are forbidden*/) { }
};


class XTOOLBOX_API VServerNetManager : public VObject
{
public :
	
	static VServerNetManager* Get();
	
	static void Init(ICriticalError* inCriticalError=NULL, IpPolicy inIpPolicy=DefaultPolicy);
	static void DeInit();

	sLONG GetDefaultClientIdleTimeOut();
	void SetDefaultClientIdleTimeOut(sLONG inTimeOut);
	
	sLONG GetSelectIODelay();
	void SetSelectIODelay(sLONG inDelay);
	
	sLONG GetNextSimpleID();
	
	//Returns inError
	VError SignalCriticalError(VError inError);	
		
	IpPolicy GetIpPolicy() const;
	
#if VERSIONWIN
	//Do not call directly ; use ServerNetTools::InetPtoN and ServerNetTools::InetNtoP instead
	sLONG InetPtoN(sLONG inFamily, const char* inSrc, void* outDst) const;
	const char* InetNtoP(sLONG inFamily, const void* inSrc, char* outDst, size_t inDstLen) const;
#endif

private :
	
	VServerNetManager();
	~VServerNetManager();
	
	
	static VServerNetManager* sInstance;
	
	ICriticalError*	fCriticalError;
	
	sLONG fDefaultClientIdleTimeOut;
	sLONG fSelectIOInactivityTimeOut;
	sLONG fEndPointCounter;
	
	IpPolicy fIpPolicy;

#if VERSIONWIN

	//Pointer to inet_pton, need vista, 2008 server or better
	typedef int (*InetPtoNPtr)(int, const char*, void*);
	InetPtoNPtr fInetPtoN;

	//Pointer to inet_ntop, need vista, 2008 server or better
	typedef const char* (*InetNtoPPtr)(int, const void*, char*, size_t);
	InetNtoPPtr fInetNtoP;

#endif
};


END_TOOLBOX_NAMESPACE


#endif
