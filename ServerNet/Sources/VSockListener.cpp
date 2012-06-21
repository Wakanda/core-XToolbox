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

#include "VSockListener.h"

#include "Tools.h"
#include "VSslDelegate.h"
#include "VNetAddr.h"

BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


class XTOOLBOX_API XSBind
{
public:

#if WITH_DEPRECATED_IPV4_API
	XSBind(IP4 inAddr, PortNumber inPort, IRequestLogger* inRequestLogger=NULL, sLONG inBoundSock=kBAD_SOCKET) :
	fAddr(inAddr), fPort(inPort), fRequestLogger(inRequestLogger), fIsSSL(false), fBoundSock(inBoundSock), fSock(NULL) { }
#else
	XSBind(const VNetAddress& inAddr, IRequestLogger* inRequestLogger=NULL, sLONG inBoundSock=kBAD_SOCKET) :
	fAddr(inAddr), fRequestLogger(inRequestLogger), fIsSSL(false), fBoundSock(inBoundSock), fSock(NULL) { }
#endif	
	
	virtual ~XSBind()						{ if(fSock!=NULL) fSock->Close(), delete fSock; }

#if WITH_DEPRECATED_IPV4_API
	void SetAddress(IP4 inAddr)				{ fAddr=inAddr; }
	IP4 GetAddress()						{ return fAddr; }
	void SetPort(PortNumber inPort)			{ fPort=inPort; }
	PortNumber GetPort()					{ return fPort; }

#else
	void SetAddress(const VNetAddress& inAddr)	{ fAddr=inAddr; }
	const VString GetIP()					{ return fAddr.GetIP(); }
	PortNumber GetPort()					{ return fAddr.GetPort(); }
#endif	
	
	void SetSSL()							{ fIsSSL=true; }
	
	bool IsSSL()							{ return fIsSSL; }
	
	void SetSock(XTCPSock* iSock)			{ fSock=iSock; }
	
	XTCPSock* GetSock()						{ return fSock; }
	
	VError Publish()
	{
		StTmpErrorContext errCtx;

#if WITH_DEPRECATED_IPV4_API
		XTCPSock* sock=XTCPSock::NewServerListeningSock(GetAddress(), GetPort(), fBoundSock);
#else
		XTCPSock* sock=XTCPSock::NewServerListeningSock(fAddr, fBoundSock);
#endif
		SetSock(sock);
		
		if(sock!=NULL)
		{
			errCtx.Flush();
			return VE_OK;
		}
		
		return vThrowError(VE_SRVR_FAILED_TO_CREATE_LISTENING_SOCKET);
	}
	
	
private:
	
#if WITH_DEPRECATED_IPV4_API
	uLONG			fAddr;
#else
	VNetAddress		fAddr;
#endif
	
	PortNumber		fPort;
	IRequestLogger* fRequestLogger;
	bool			fIsSSL;
	sLONG			fBoundSock;
	XTCPSock*	fSock;
};


VSockListener::VSockListener(IRequestLogger* inRequestLogger) :
fRequestLogger(inRequestLogger), fListenStarted(false), fAcceptTimeout(0), fKeyCertPair(NULL)
{}


VSockListener::~VSockListener()
{
	StopListeningAndClearPorts();
}


#if WITH_DEPRECATED_IPV4_API

bool VSockListener::AddListeningPort(uLONG iAddr, PortNumber iPort, bool iSsl, Socket inBoundSock)
{	
	assert(!fListenStarted);
	
	std::vector<XSBind*> *l_vector = iSsl ? &fSslListens : &fPlainListens;
	
	try
	{
		XSBind*	newBind = new XSBind ( iAddr, iPort, fRequestLogger, inBoundSock );
		
		if(iSsl)
			newBind-> SetSSL();
		
		l_vector-> push_back ( newBind );
	}
	catch (...)
	{
		return false;
	}
	return true;
}

#else

bool VSockListener::AddListeningPort(VString inAddr, PortNumber inPort, bool iSsl, Socket inBoundSock)
{
	return AddListeningPort(VNetAddress(inAddr, inPort));
}

bool VSockListener::AddListeningPort(const VNetAddress& inAddr, bool iSsl, Socket inBoundSock)
{	
	assert(!fListenStarted);
	
	std::vector<XSBind*> *l_vector = iSsl ? &fSslListens : &fPlainListens;
	
	try
	{
		XSBind*	newBind = new XSBind (inAddr, fRequestLogger, inBoundSock);
		
		if(iSsl)
			newBind-> SetSSL();
		
		l_vector-> push_back ( newBind );
	}
	catch (...)
	{
		return false;
	}
	return true;
}

#endif

//Compatibility method : Reading key/cert files shouldn't be ServerNet business.
void VSockListener::SetCertificatePaths (const char* inCertPath, const char* inKeyPath)
{
	VFile certFile(inCertPath);
	
	VMemoryBuffer<> certBuffer;
	
	VError verr=certFile.GetContent(certBuffer);
	
	if(verr!=VE_OK)
		vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
	VFile keyFile(inKeyPath);
	
	VMemoryBuffer<> keyBuffer;
	
	verr=keyFile.GetContent(keyBuffer); 
	
	if(verr!=VE_OK)
		vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
	
	//Do not throw here, just try to be transparent.
	verr=SetKeyAndCertificate(keyBuffer, certBuffer);
}


VError VSockListener::SetKeyAndCertificate(const VMemoryBuffer<>& inKey, const VMemoryBuffer<>& inCertificate)
{
	SslFramework::ReleaseKeyCertificatePair(fKeyCertPair);
	
	fKeyCertPair=SslFramework::RetainKeyCertificatePair(inKey, inCertificate);
	
	if(fKeyCertPair==NULL)
		return vThrowError(VE_SRVR_FAILED_TO_SET_KEY_AND_CERTIFICATE);
	
	return VE_OK;
}

VError VSockListener::SetKeyAndCertificate (const XBOX::VString &inKey, const XBOX::VString &inCertificate)
{
	char			*keyData, *certificateData;
	XBOX::VError	error;

	keyData = certificateData = NULL;		
	if ((keyData = new char[inKey.GetLength() + 1]) != NULL 
	&& (certificateData = new char[inCertificate.GetLength() + 1]) != NULL) {

		VMemoryBuffer<> key, certificate;

		inKey.ToCString(keyData, inKey.GetLength() + 1);
		inCertificate.ToCString(certificateData, inCertificate.GetLength() + 1);

		key.SetDataPtr(keyData, inKey.GetLength(), inKey.GetLength() + 1);
		certificate.SetDataPtr(certificateData, inCertificate.GetLength(), inCertificate.GetLength() + 1);

		error = this->SetKeyAndCertificate(key, certificate);

		key.ForgetData();
		certificate.ForgetData();

	} else 

		error = VE_MEMORY_FULL;

	if (keyData != NULL)

		delete[] keyData;

	if (certificateData != NULL)

		delete[] certificateData;

	return error;
}

bool VSockListener::StartListening()
{
	bool l_res = false;
	
	if (!fListenStarted)
	{
		l_res = true;
		std::vector<XSBind*>::iterator		iterBind = fPlainListens. begin ( );
		while ( iterBind != fPlainListens. end ( ) )
		{
			if ( !( l_res = ( ( *iterBind )-> Publish ( ) == VE_OK ) ) )
				break;
			
			fAcceptIterator.AddServiceSocket((*iterBind)->GetSock());
			
			iterBind++;
		}
		if ( l_res )
		{
			iterBind = fSslListens. begin ( );
			while ( iterBind != fSslListens. end ( ) )
			{
				VError verr=(*iterBind)->Publish();
				
				if(verr!=VE_OK)
				{
					l_res=false;
					break;
				}
				
				fAcceptIterator.AddServiceSocket((*iterBind)->GetSock());
				
				iterBind++;
			}
		}
		
		fListenStarted = true;	/* To indicate that there may be some cleaning up to do. */
	}
	
	return l_res;
}

void VSockListener::StopListeningAndClearPorts()
{
	std::for_each(fPlainListens.begin(), fPlainListens.end(), del_fun<XSBind>()); 
	fPlainListens.clear();
	
	std::for_each(fSslListens.begin(), fSslListens.end(), del_fun<XSBind>()); 
	fSslListens.clear();
	
	fAcceptIterator.ClearServiceSockets();
	
	fListenStarted = false;
}


void VSockListener::setAcceptTimeout(uLONG inMsTimeout)
{
	fAcceptTimeout=inMsTimeout;
}


bool VSockListener::SetBlocking (bool isBlocking)
{
	for (uLONG i = 0; i < fPlainListens.size(); ++i)
	{
		XSBind *bind = dynamic_cast<XSBind*> (fPlainListens[i]);
		
		if (bind)
		{
			if (bind->GetSock()->SetBlocking(isBlocking) != VE_OK)
				return false;
		}
	}
	
	return true;
}


XTCPSock* VSockListener::GetNewConnectedSocket(sLONG inMsTimeout)
{
	StTmpErrorContext errCtx;
	
	XTCPSock* sock=NULL;
	
	VError verr=fAcceptIterator.GetNewConnectedSocket(&sock, inMsTimeout);
	
	if(sock!=NULL && verr==VE_OK)
	{
		for(XSBindCIt cit=fSslListens.begin() ; cit!=fSslListens.end() ; ++cit)
		{
			if((*cit)->GetPort()==sock->GetServicePort())
			{
				verr=sock->PromoteToSSL(fKeyCertPair);
				break;
			}
		}
	}
	
	if(verr==VE_OK || verr==VE_SOCK_TIMED_OUT)
	{
		errCtx.Flush();
		
		return sock; 
	}
	
	vThrowError(VE_SRVR_FAILED_TO_CREATE_CONNECTED_SOCKET);
	
	delete sock;
	
	return NULL;
}


void VSockListener::ReleaseConnection(XTCPSock *iConnection)
{
	delete iConnection;
}


END_TOOLBOX_NAMESPACE
