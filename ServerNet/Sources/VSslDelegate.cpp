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

#include "VSslDelegate.h"

#include "SslStub.h"
#include "Kernel/Sources/VMemoryBuffer.h"


#if VERSION_LINUX
	#include <signal.h>
#endif


BEGIN_TOOLBOX_NAMESPACE


#define WITH_SNET_SSL_LOG 0


const int RSA_PKCS1_PADDING_LEN=11;


static VError vThrowThreadErrorStackRec()
{
	unsigned long err=SSLSTUB::ERR_get_error();
	
	if(err==0)
		return VE_OK;
	
	//We need to recurse to throw the older errors first ! 
	vThrowThreadErrorStackRec();
	
	char buf[200];
	memset(buf, 0, sizeof(buf));

	SSLSTUB::ERR_error_string_n(err, buf, sizeof(buf));
	
	DebugMsg ("[%d] OpenSSL stacked error : %s\n", VTask::GetCurrentID(), buf);
	
	return vThrowError(VE_SSL_STACKED_ERROR, buf);
}


static VError vThrowThreadErrorStack(VError inImplErr)
{
	vThrowThreadErrorStackRec();
	
	SSLSTUB::ERR_clear_error();
	
	return vThrowError(inImplErr);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SslFramework::XContext
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class SslFramework::XContext : public VObject
{
public :
		
	//Callbacks for OpenSSL thread API
	
	static void SNET_CDECL LockingProc(int inMode, int inIndex, const char* /*inFile*/, int /*inLine*/);
	
	static unsigned long SNET_CDECL ThreadIdProc();
	
	//Needed by OpenSSL to create a connection context
	
	SSL_CTX* GetOpenSSLContext();
	
	VError SetDefaultCertificate(const VMemoryBuffer<>& inCertBuffer);
	
	VError SetDefaultPrivateKey(const VMemoryBuffer<>& inKeyBuffer);
	
	VError AddIntermediateCertificate(const VMemoryBuffer<>& inCertBuffer);
	
	VError AddCertificateDirectory(const VFolder& inCertFolder);

	XContext(const XContext& inUnused); 
	
	XContext& operator=(const XContext& inUnused);
	
	//Needed by SslFramework::InitFramework

	XContext();
	
	virtual ~XContext();
	
	VError Init();

	//SSL_CTX (SSL Context)
	//That's the global context structure which is created by a server or client once per program life-time
	//and which holds mainly default values for the SSL structures which are later created for the connections.
	
	SSL_CTX* fOpenSSLContext;

	//Private members needed for OpenSSL thread sync.
	
	XBOX::VCriticalSection* fLocks;

	sLONG fCount;
};


SslFramework::XContext::XContext() : fOpenSSLContext(NULL), fLocks(NULL), fCount(0)
{
	//empty
}


//virtual
SslFramework::XContext::~XContext()
{
	delete[] fLocks;
	fLocks=NULL;
}


//virtual
VError SslFramework::XContext::Init()
{
	if(fLocks==NULL)
	{
		int count=SSLSTUB::CRYPTO_num_locks();

		fLocks=new VCriticalSection[count];
		fCount=count;
	}
	
	if(fOpenSSLContext==NULL)
	{
		fOpenSSLContext=SSLSTUB::SSL_CTX_new(SSLSTUB::SSLv23_method());
	}
	
	return fLocks!=NULL && fOpenSSLContext!=NULL ? VE_OK : VE_SSL_FRAMEWORK_INIT_FAILED;
}


//namespace
void SNET_CDECL SslFramework::XContext::LockingProc(int inMode, int inIndex, const char* /*inFile*/, int /*inLine*/)
{	
	XContext* ctx=GetContext();
	
	xbox_assert(ctx!=NULL);
	
	if(ctx==NULL)
		return;
	
	if (inMode&CRYPTO_LOCK)
	{
		//DebugMsg ("[%d] SslFramework::XContext::LockingProc : lock index=%d\n", VTask::GetCurrentID(), inIndex);
		
		bool res=false;
		
		if(ctx->fLocks!=NULL && inIndex>=0 && inIndex<ctx->fCount)
			res=ctx->fLocks[inIndex].Lock();
		
		xbox_assert(res);
	}
	else
	{
		//DebugMsg ("[%d] SslFramework::XContext::LockingProc : unlock index=%d\n", VTask::GetCurrentID(), inIndex);
		
		bool res=false;
		
		if(ctx->fLocks!=NULL && inIndex>=0 && inIndex<ctx->fCount)
			res=ctx->fLocks[inIndex].Unlock();
		
		xbox_assert(res);
	}
}


//namespace
unsigned long SNET_CDECL SslFramework::XContext::ThreadIdProc()
{	
	VTaskID signedId=VTaskMgr::Get()->GetCurrentTaskID();
	
	//Task manager give us negative Ids for tasks it doesn't own... But we need positive task Ids !
	return signedId>=0 ? signedId : ULONG_MAX+signedId ;
}


SSL_CTX* SslFramework::XContext::GetOpenSSLContext()
{
	xbox_assert(fOpenSSLContext!=NULL);
		
	return fOpenSSLContext;
}


//namespace
VError SslFramework::XContext::SetDefaultCertificate(const VMemoryBuffer<>& inCertBuffer)
{
	BIO* buf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());
	
	xbox_assert(buf!=NULL);
	
	int res=SSLSTUB::BIO_write(buf, inCertBuffer.GetDataPtr(), static_cast<int>(inCertBuffer.GetDataSize()));
	
	xbox_assert(res==inCertBuffer.GetDataSize());
	
    X509* cert=SSLSTUB::PEM_read_bio_X509(buf, NULL, NULL, NULL);
	
	xbox_assert(cert!=NULL);
	
	SSLSTUB::BIO_free(buf);
	
	if(cert==NULL)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_GET_CERTIFICATE);
	
	SSL_CTX* ctx=GetContext()->GetOpenSSLContext();
	
	res=SSLSTUB::SSL_CTX_use_certificate(ctx, cert);
	
	if(res!=1)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_SET_CERTIFICATE);
	
	return VE_OK;
}


//namespace
VError SslFramework::XContext::SetDefaultPrivateKey(const VMemoryBuffer<>& inKeyBuffer)
{
	BIO* buf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());
	
	xbox_assert(buf!=NULL);

	int res=SSLSTUB::BIO_write(buf, inKeyBuffer.GetDataPtr(), static_cast<int>(inKeyBuffer.GetDataSize()));
	
	xbox_assert(res==inKeyBuffer.GetDataSize());

	RSA* key=SSLSTUB::PEM_read_bio_RSAPrivateKey(buf, NULL, NULL, NULL);
	
	xbox_assert(key!=NULL);
	
	SSLSTUB::BIO_free(buf);
		
	if(key==NULL)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_GET_PRIVATE_KEY);
	
	SSL_CTX* ctx=GetContext()->GetOpenSSLContext();
	
	res=SSLSTUB::SSL_CTX_use_RSAPrivateKey(ctx, key);
	
	if(res!=1)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_SET_PRIVATE_KEY);
	
	return VE_OK;
}


//namespace
VError SslFramework::XContext::AddIntermediateCertificate(const VMemoryBuffer<>& inCertBuffer)
{
	BIO* buf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());
	
	xbox_assert(buf!=NULL);
	
	int res=SSLSTUB::BIO_write(buf, inCertBuffer.GetDataPtr(), static_cast<int>(inCertBuffer.GetDataSize()));
	
	xbox_assert(res==inCertBuffer.GetDataSize());
	
    X509* cert=SSLSTUB::PEM_read_bio_X509(buf, NULL, NULL, NULL);
	
	xbox_assert(cert!=NULL);
	
	SSLSTUB::BIO_free(buf);
	
	if(cert==NULL)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_GET_CERTIFICATE);
	
	SSL_CTX* ctx=GetContext()->GetOpenSSLContext();

	res=SSLSTUB::SSL_CTX_ctrl(ctx, SSL_CTRL_EXTRA_CHAIN_CERT, 0, (char*)cert);

	if(res!=1)
		return vThrowThreadErrorStack(VE_SSL_FAIL_TO_SET_CERTIFICATE);
	
	return VE_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SslFramework
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


namespace SslFramework
{
	SslFramework::XContext* gContext=NULL;
}


//namespace
VError SslFramework::Init()
{
	if(gContext!=NULL)
		return VE_OK;
	
	SSLSTUB::SSL_library_init();
	
	SSLSTUB::SSL_load_error_strings();
	
	gContext=new XContext;

	if(gContext==NULL)
		return vThrowError(VE_MEMORY_FULL);
	
	VError verr=gContext->Init();
	
	if(verr!=VE_OK)
		return VE_SSL_FRAMEWORK_INIT_FAILED;
	
	SSLSTUB::CRYPTO_set_id_callback(SslFramework::XContext::ThreadIdProc);

	SSLSTUB::CRYPTO_set_locking_callback(SslFramework::XContext::LockingProc);
	
#if VERSION_LINUX
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));

	sa.sa_handler=SIG_IGN;

	int rv=sigaction(SIGPIPE, &sa, NULL);

	xbox_assert(rv==0);
#endif
	
	return VE_OK;
}


//namespace
VError SslFramework::DeInit()
{
	//jmo - TODO : Terminer ! (tout liberer, y compris la pile d'erreurs, passer le context à NULL, etc.)
	
	SSLSTUB::ERR_free_strings();
	
	SSL_CTX* sslCtx=GetContext()->GetOpenSSLContext();
	
	if(sslCtx!=NULL)
	{
		SSLSTUB::SSL_CTX_free(sslCtx);
	}
	
	return VE_OK;
}


//namespace
VError SslFramework::SetDefaultPrivateKey(const VMemoryBuffer<>& inKeyBuffer)
{
	if(gContext==NULL)
		return VE_INVALID_PARAMETER;
	
	return gContext->SetDefaultPrivateKey(inKeyBuffer);
}


//namespace
VError SslFramework::SetDefaultCertificate(const VMemoryBuffer<>& inCertBuffer)
{
	if(gContext==NULL)
		return VE_INVALID_PARAMETER;
	
	return gContext->SetDefaultCertificate(inCertBuffer);
}


//namespace
VError SslFramework::AddCertificateDirectory(const VFolder& inCertFolder)
{
	const VString pem("pem");

	//VFileIterator fit(&inCertFolder);

	VError verr=VE_OK;

	//while(fit.IsValid())
	for(VFileIterator fit(&inCertFolder) ; fit.IsValid() ; ++fit)
	{
		VString ext;
		
		fit->GetExtension(ext);

		if(ext.CompareToString(pem, false /*not case sensitive*/)==CR_EQUAL)
		{
			const VFilePath path=fit->GetPath();

			VString name;
			fit->GetName(name);

			VMemoryBuffer<> buffer;
	
			if(fit->GetContent(buffer)!=VE_OK)
			{
						DebugMsg ("[%d] AddCertificateDirectory : Fail to load buffer for %S\n",
								  VTask::GetCurrentID(), &name);

						verr=vThrowError(VE_SSL_FAIL_TO_SET_CERTIFICATE);

						continue;
			}

			if(gContext->AddIntermediateCertificate(buffer)!=VE_OK)
			{
				DebugMsg ("[%d] AddCertificateDirectory : Fail to set certificate %S\n",
								  VTask::GetCurrentID(), &name);

				verr=VE_SSL_FAIL_TO_SET_CERTIFICATE;
	
				continue;
			}

			DebugMsg ("[%d] AddCertificateDirectory : Added intermediate certificate %S\n",
					  VTask::GetCurrentID(), &name);
		}
	}

	return verr;
}


//namespace
SslFramework::XContext* SslFramework::GetContext()
{
	return gContext;
}


//namespace
VError SslFramework::Encrypt(uCHAR* inPrivateKeyPEM, uLONG inPrivateKeyPEMSize, uCHAR* inData, uLONG inDataSize, uCHAR* ioEncryptedData, uLONG* ioEncryptedDataSize)
{
	if(ioEncryptedData==NULL)
		return VE_SRVR_INVALID_PARAMETER;
	
	
	//Unfold RSAKey::PEMToPrivateKey and SslFramework::XContext::SetDefaultPrivateKey
	BIO* buf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());
	
	xbox_assert(buf!=NULL);
	
	int res=SSLSTUB::BIO_write(buf, inPrivateKeyPEM, inPrivateKeyPEMSize);
	
	xbox_assert(res==inPrivateKeyPEMSize);
	
	RSA* key=SSLSTUB::PEM_read_bio_RSAPrivateKey(buf, NULL, NULL, NULL);
	
	xbox_assert(key!=NULL);
	
	SSLSTUB::BIO_free(buf);
	
	if(key==NULL)
		return VE_SRVR_INVALID_PARAMETER;

	
	//Unfold RSAKey::PrivateEncrypt	
	int	max_chunk, chunk;
	
	max_chunk = SSLSTUB::RSA_size (key) - RSA_PKCS1_PADDING_LEN;
	
	int	inpos = 0, outpos = 0;
	
	unsigned char* from=inData;
	int fromlen=inDataSize;
	
	unsigned char* to=ioEncryptedData;
	int tolen=*ioEncryptedDataSize;	
	
	VError verr=VE_OK;
	
	while ( fromlen )
	{
		if ( fromlen > max_chunk )
			chunk = max_chunk;
		else
			chunk = fromlen;
		
		if(outpos+max_chunk>tolen)
		{	
			verr=VE_SRVR_INVALID_PARAMETER;
			break;
		}
			
		outpos += SSLSTUB::RSA_private_encrypt (chunk, from+inpos, to+outpos, key, RSA_PKCS1_PADDING);
	
		inpos += chunk;
		fromlen -= chunk;
	}
	
	*ioEncryptedDataSize=outpos;
	SSLSTUB::RSA_free(key);
	
	return verr;
}


//namespace
uLONG SslFramework::GetEncryptedPKCS1DataSize(uLONG inKeySize /* 128 for 1024 RSA; X/8 for X RSA*/, uLONG inDataSize)
{
	//From Security::EncryptedPKCS1DataSize
	uLONG len = 0;
	
	if ( inDataSize < ( inKeySize - RSA_PKCS1_PADDING_LEN ) )
		len = inKeySize + RSA_PKCS1_PADDING_LEN ;
	else 
		len = ((inDataSize / ( inKeySize - RSA_PKCS1_PADDING_LEN )) + 1) * ( inKeySize + RSA_PKCS1_PADDING_LEN );
		
	return len;	
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VSslDelegate::XConnection
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class VSslDelegate::XConnection : public VObject
{
public :

	XConnection();
	~XConnection();
	
	SSL* GetConnection();
	
private :

	XConnection(const XConnection& inUnused);
	XConnection& operator=(const XConnection& inUnused);

	//SSL (SSL Connection)
    //That's the main SSL/TLS structure which is created by a server or client per established connection. 
	//This actually is the core structure in the SSL API. Under run-time the application usually deals with
	//this structure which has links to mostly all other structures.

	SSL* fConnection;
};


VSslDelegate::XConnection::XConnection()
{
	SSL_CTX* implCtx=SslFramework::GetContext()->GetOpenSSLContext();
	
	fConnection=SSLSTUB::SSL_new(implCtx);
	
#if VERSIONDEBUG && WITH_SNET_SSL_LOG
		DebugMsg ("[%d] VSslDelegate::XConnection::XConnection() : connection=%d\n", VTask::GetCurrentID(), fConnection);
#endif
}


VSslDelegate::XConnection::~XConnection()
{
	
#if VERSIONDEBUG && WITH_SNET_SSL_LOG
		DebugMsg ("[%d] VSslDelegate::XConnection::~XConnection() : connection=%d\n", VTask::GetCurrentID(), fConnection);
#endif
	
	if(fConnection!=NULL)
		SSLSTUB::SSL_free(fConnection);
}

SSL* VSslDelegate::XConnection::GetConnection()
{
	return fConnection;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VKeyCertChain
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


class VKeyCertChain : public IRefCountable
{
  private : 
	
	VKeyCertChain(const VKeyCertChain&);	//No copy !

	~VKeyCertChain();						//Release !

	RSA*	fPrivateKey;
	X509*	fCertificate;
	
	std::vector<X509*> fChain;
	
  public :

	VKeyCertChain() : fPrivateKey(NULL), fCertificate(NULL) {}
		
	VError Init(const VMemoryBuffer<>& inKeyBuffer, const VMemoryBuffer<>& inCertBuffer);
	
	VError PushIntermediateCertificate(const VMemoryBuffer<>& inCertBuffer);
	
	VError LoadIntoConnection(SSL* inConn);
};


VKeyCertChain::~VKeyCertChain()
{
	SSLSTUB::RSA_free(fPrivateKey);
	SSLSTUB::X509_free(fCertificate);
	
	std::vector<X509*>::iterator it;
	
	for(it=fChain.begin() ; it!=fChain.end() ; ++it)
		SSLSTUB::X509_free(*it);
}


VError VKeyCertChain::Init(const VMemoryBuffer<>& inKeyBuffer, const VMemoryBuffer<>& inCertBuffer)
{
	VError verr=VE_OK;
	
	BIO* buf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());
	
	if(buf==NULL)
		verr=VE_MEMORY_FULL;
	
	if(verr==VE_OK)
	{
		int res=SSLSTUB::BIO_write(buf, inKeyBuffer.GetDataPtr(), static_cast<int>(inKeyBuffer.GetDataSize()));
		
		if(res!=inKeyBuffer.GetDataSize())
			verr=VE_SSL_FAIL_TO_GET_PRIVATE_KEY;
	}
	
	if(verr==VE_OK)
	{
		RSA* key=SSLSTUB::PEM_read_bio_RSAPrivateKey(buf, NULL, NULL, NULL);
		
		if(key!=NULL)
			fPrivateKey=key;
		else
			verr=VE_SSL_FAIL_TO_GET_PRIVATE_KEY;
	}	
	
	if(verr==VE_OK)
	{		
		int res=SSLSTUB::BIO_write(buf, inCertBuffer.GetDataPtr(), static_cast<int>(inCertBuffer.GetDataSize()));
		
		if(res!=inCertBuffer.GetDataSize())
			verr=VE_SSL_FAIL_TO_GET_CERTIFICATE;
	}
	
	if(verr==VE_OK)
	{		
		X509* cert=SSLSTUB::PEM_read_bio_X509(buf, NULL, NULL, NULL);
		
		if(cert!=NULL)
			fCertificate=cert;
		else
			verr=VE_SSL_FAIL_TO_GET_CERTIFICATE;
	}	
	
	SSLSTUB::BIO_free(buf);
	
	if(verr!=VE_OK)
	{
		SSLSTUB::RSA_free(fPrivateKey);
		SSLSTUB::X509_free(fCertificate);
	}
	
	return vThrowThreadErrorStack(verr);
}


VError VKeyCertChain::PushIntermediateCertificate(const VMemoryBuffer<>& inCertBuffer)
{
	VError verr=VE_OK;
	
	BIO* buf=SSLSTUB::BIO_new(SSLSTUB::BIO_s_mem());
	
	if(buf==NULL)
		verr=VE_MEMORY_FULL;
	
	if(verr==VE_OK)
	{		
		int res=SSLSTUB::BIO_write(buf, inCertBuffer.GetDataPtr(), static_cast<int>(inCertBuffer.GetDataSize()));
		
		if(res!=inCertBuffer.GetDataSize())
			verr=VE_SSL_FAIL_TO_GET_CERTIFICATE;
	}
	
	if(verr==VE_OK)
	{		
		X509* cert=SSLSTUB::PEM_read_bio_X509(buf, NULL, NULL, NULL);
		
		if(cert!=NULL)
			fChain.push_back(cert);
		else
			verr=VE_SSL_FAIL_TO_GET_CERTIFICATE;
	}	
	
	SSLSTUB::BIO_free(buf);
	
	return vThrowThreadErrorStack(verr);
}


VError VKeyCertChain::LoadIntoConnection(SSL* inConn)
{
	if(inConn==NULL)
		return VE_INVALID_PARAMETER;
	
	VError verr=VE_OK;
	
	int res=SSLSTUB::SSL_use_certificate(inConn, fCertificate);
	
	if(res!=1)
		verr=VE_SSL_FAIL_TO_SET_CERTIFICATE;
	else
	{
		res=SSLSTUB::SSL_use_RSAPrivateKey(inConn, fPrivateKey);
		
		if(res!=1)
			verr=VE_SSL_FAIL_TO_SET_PRIVATE_KEY;
		else
		{
			std::vector<X509*>::const_iterator cit;
			
			for(cit=fChain.begin() ; cit!=fChain.end() && verr==VE_OK ; cit++)
			{
				//As it turns out, SSL_xxx_client_CA fonctions (mostly undocumented) have nothing to do with client, nor CA
				//OpenSLL encrypted sens of humour ?
				res=SSLSTUB::SSL_add_client_CA(inConn, *cit);

				if(res!=1)
					verr=VE_SSL_FAIL_TO_SET_CERTIFICATE;
			}
		}
	}
	
	//TODO : Vérifier que le tout est apparié !
	
	return vThrowError(verr);
}


//namespace
VKeyCertChain* SslFramework::RetainKeyCertificateChain(const VMemoryBuffer<>& inKeyBuffer, const VMemoryBuffer<>& inCertBuffer)
{
	VKeyCertChain* kcc=new VKeyCertChain();
	
	VError verr=kcc->Init(inKeyBuffer, inCertBuffer);
	
	if(verr!=VE_OK)
		return NULL;
	
	return kcc;
}


//namespace
void SslFramework::ReleaseKeyCertificateChain(VKeyCertChain** inKeyCertChain)
{
	ReleaseRefCountable(inKeyCertChain);
}


//namespace
VError SslFramework::PushIntermediateCertificate(VKeyCertChain* inKeyCertChain, const VMemoryBuffer<>& inCertBuffer)
{
	if(inKeyCertChain!=NULL)
		return inKeyCertChain->PushIntermediateCertificate(inCertBuffer);
	
	return VE_SRVR_INVALID_PARAMETER;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// VSslDelegate
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VSslDelegate::VSslDelegate() : fConnection(NULL), fKeyCertChain(NULL)
{
	fConnection=new XConnection();
}


//virtual 
VSslDelegate::~VSslDelegate()
{
	if(fKeyCertChain!=NULL)
		fKeyCertChain->Release();

	if(fConnection!=NULL)
		delete fConnection;
}


//static
VSslDelegate* VSslDelegate::NewDelegate(Socket inRawSocket)
{
	VSslDelegate* delegate=new VSslDelegate();
	
	if(delegate->fConnection==NULL)
	{
		vThrowError(VE_SSL_NEW_CONTEXT_FAILED);

		return NULL;
	}
	
	SSL* conn=delegate->fConnection->GetConnection();
	
	int res=SSLSTUB::SSL_set_fd(conn, inRawSocket);
	
	if(res==0)
	{
		vThrowError(VE_SSL_NEW_CONTEXT_FAILED);
		
		delete delegate;
		delegate=NULL;
	}
	
#if VERSIONDEBUG && WITH_SNET_SSL_LOG
	DebugMsg ("[%d] VSslDelegate::NewDelegate() : connection=%d with socket %d\n",
			  VTask::GetCurrentID(), conn, inRawSocket);
#endif
	
	return delegate;

}


//static
VSslDelegate* VSslDelegate::NewClientDelegate(Socket inRawSocket)
{
	SSLSTUB::ERR_clear_error();
	
	VSslDelegate* delegate=NewDelegate(inRawSocket);

	if(delegate!=NULL)
	{
		SSL* conn=delegate->fConnection->GetConnection();
		
		SSLSTUB::SSL_set_connect_state(conn);
	}
	
	return delegate;
}


//static
VSslDelegate* VSslDelegate::NewServerDelegate(Socket inRawSocket, VKeyCertChain* inKeyCertChain)
{
	SSLSTUB::ERR_clear_error();

	VSslDelegate* delegate=NewDelegate(inRawSocket);
	
	VError verr=VE_OK;
	
	if(delegate!=NULL)
	{
		SSL* conn=delegate->fConnection->GetConnection();

		SSLSTUB::SSL_set_accept_state(conn);
		
		if(inKeyCertChain!=NULL)
		{		
			verr=inKeyCertChain->LoadIntoConnection(conn);
			
			if(verr==VE_OK)
			{
				delegate->fKeyCertChain=inKeyCertChain;
				delegate->fKeyCertChain->Retain();
			}
		}
	}
	
	if(verr!=VE_OK)
	{
		delete delegate;
		delegate=NULL;
	}
	
	return delegate;
}

// Only to be used by VJSNet at SSL socket creation (SSJS implementation).

VError VSslDelegate::HandShake ()
{
	int	r;

	// SSL sockets can be asynchronous, so an SSL_ERROR_WANT_READ is ok 
	// because connection negociation is pending. It will complete in an
	// asynchronous way.

	if ((r = SSLSTUB::SSL_connect(fConnection->GetConnection())) == 1
	|| SSLSTUB::SSL_get_error(fConnection->GetConnection(), r) == SSL_ERROR_WANT_READ)
		
		return XBOX::VE_OK;

	else

		return XBOX::VE_ACCESS_DENIED;		
}

sLONG VSslDelegate::GetBufferedDataLen()
{
	SSL* conn=fConnection->GetConnection();
	
	sLONG len=SSLSTUB::SSL_pending(conn);
	
	return len;
}


VError VSslDelegate::Read(void* outBuff, uLONG* ioLen)
{
	// - outBuff and ioLen are mandatory ; ioLen is always modified (set to 0 on error)
	// - Caller should deal with special errors VE_SOCK_WOULD_BLOCK and VE_SOCK_PEER_OVER
	// - Should be consistent with XBsdTCPSocket::Read(), except WOULD_BLOCK may go with WantWrite()
	
	SSLSTUB::ERR_clear_error(); fIOState=kBlank;
	
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	SSL* conn=fConnection->GetConnection();
		
	int res=SSLSTUB::SSL_read(conn, outBuff,*ioLen);
	
#if VERSIONDEBUG && WITH_SNET_SSL_LOG
	DebugMsg ("[%d] VSslDelegate::Read() : connection=%d socket=%d len=%d res=%d\n",
			  VTask::GetCurrentID(), conn, SSLSTUB::SSL_get_fd(conn), *ioLen, res);
#endif
	
	*ioLen=0;
	
	if(res>0)
	{
		*ioLen=res;

	#if VERSIONDEBUG && WITH_SNET_SSL_LOG
		
		VString msg;
		msg.AppendCString("<<<").AppendBlock(outBuff, *ioLen, VTC_UTF_8).AppendCString(">>>\n");
	
		DebugMsg(msg);
		
	#endif
		
		return VE_OK;
	}
	
	int errCode=SSLSTUB::SSL_get_error(conn, res);
	
	if(res==0 && (errCode==SSL_ERROR_ZERO_RETURN || errCode==SSL_ERROR_SYSCALL))
	{
		//The TLS/SSL connection has been closed. If the protocol version is SSL 3.0 or TLS 1.0, this code is
		//returned if the connection has been closed cleanly. Note that in this case SSL_ERROR_ZERO_RETURN does
		//not necessarily indicate that the underlying transport has been closed.
		
		fIOState=kOver;

		//As long as we don't support downgrading an ssl socket, we consider the undelying socket was shutdown().
		
		return VE_SOCK_PEER_OVER;
	}	

	if(res==-1 && errCode==SSL_ERROR_WANT_READ)
	{
		fIOState=kWantRead;
		
		return VE_SOCK_WOULD_BLOCK;
	}

	if(res==-1 && errCode==SSL_ERROR_WANT_WRITE)
	{
		fIOState=kWantWrite;

		return VE_SOCK_WOULD_BLOCK;
	}

	//We have an error...
	
	return vThrowThreadErrorStack(VE_SSL_READ_FAILED);
}


VError VSslDelegate::Write(const void* inBuff, uLONG* ioLen)
{
	// - inBuff and ioLen are mandatory ; ioLen is always modified (set to 0 on error)
	// - Caller should deal with special error VE_SOCK_WOULD_BLOCK 
	// - Should be consistent with XBsdTCPSocket::Write(), except WOULD_BLOCK may go with WantRead()
	
	SSLSTUB::ERR_clear_error();
	
	if(inBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	SSL* conn=fConnection->GetConnection();

	int res=SSLSTUB::SSL_write(conn, inBuff, *ioLen);
		
#if VERSIONDEBUG && WITH_SNET_SSL_LOG
	DebugMsg ("[%d] VSslDelegate::Write() : connection=%d, len=%d res=%d\n",
			  VTask::GetCurrentID(), conn, *ioLen, res);
#endif
	
	*ioLen=0;
	
	if(res>0)
	{
		*ioLen=res;
		
		return VE_OK;
	}
	
	int errCode=SSLSTUB::SSL_get_error(conn, res);
	
	if(res==-1 && errCode==SSL_ERROR_WANT_READ)
	{
		fIOState=kWantRead;
		
		return VE_SOCK_WOULD_BLOCK;
	}
	
	if(res==-1 && errCode==SSL_ERROR_WANT_WRITE)
	{
		fIOState=kWantWrite;
	
		return VE_SOCK_WOULD_BLOCK;
	}
	
	//OpenSSL strange behavior observed in ssjs sockets. According to OpenSSL doc, we should never see
	//this error because all the sockets we use are already (tcp) connected...
	xbox_assert(errCode!=SSL_ERROR_WANT_CONNECT);

	if(res==-1 && errCode==SSL_ERROR_WANT_CONNECT)
	{
		fIOState=kWantWrite;	//connect -> write set in select call
	
		return VE_SOCK_WOULD_BLOCK;
	}
	
	//We have an error...
	
	//Clean SSL protocol termination ? I'd like to see this one !
	xbox_assert(errCode!=SSL_ERROR_ZERO_RETURN);
		
	return vThrowThreadErrorStack(VE_SSL_WRITE_FAILED);
}


VError VSslDelegate::Shutdown()
{
	SSLSTUB::ERR_clear_error();

	SSL* conn=fConnection->GetConnection();	

	int res=SSLSTUB::SSL_shutdown(conn);
	
	if(res==0 /*unidirectional*/ || res==1 /*bidirectional*/)
		return VE_OK;
	
	//We have an error...

	return vThrowThreadErrorStack(VE_SSL_SHUTDOWN_FAILED);
}


END_TOOLBOX_NAMESPACE
