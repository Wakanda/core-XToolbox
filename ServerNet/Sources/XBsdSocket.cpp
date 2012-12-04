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


#include "XBsdSocket.h"

#include "Tools.h"
#include "VNetAddr.h"
#include "VSslDelegate.h"

#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>


#define SNET_HAVE_GROUP_REQ 0


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


#define WITH_SNET_SOCKET_LOG 0


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XBsdTCPSocket
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//virtual
XBsdTCPSocket::~XBsdTCPSocket()
{
	//Close();

#if VERSIONDEBUG
	if(fSock!=kBAD_SOCKET)
		DebugMsg ("[%d] XBsdTCPSocket::~XBsdTCPSocket() : socket %d not closed yet !\n", VTask::GetCurrentID(), fSock);
#endif
	
	if(fSslDelegate!=NULL)
		delete fSslDelegate;
}


#if WITH_DEPRECATED_IPV4_API
IP4 XBsdTCPSocket::GetIPv4HostOrder()
{
	XBsdNetAddr addr;
	
	if(fProfile==ClientSock)
		addr.FromPeerAddr(fSock);
	else
		addr.FromLocalAddr(fSock);
	
	return addr.GetIPv4HostOrder();
}
#endif


VString XBsdTCPSocket::GetIP() const
{
	XBsdNetAddr addr;
	
	if(fProfile==ClientSock)
		addr.FromPeerAddr(fSock);
	else
		addr.FromLocalAddr(fSock);
	
	return addr.GetIP();
}


PortNumber XBsdTCPSocket::GetPort() const
{
	return GetServicePort();
}


PortNumber XBsdTCPSocket::GetServicePort() const
{
	return fServicePort;
}


VError XBsdTCPSocket::SetServicePort(PortNumber inServicePort)
{
	fServicePort=inServicePort;

	return VE_OK;
}


Socket XBsdTCPSocket::GetRawSocket() const
{
	return fSock;
}


VError XBsdTCPSocket::Close(bool inWithRecvLoop)
{
	StSilentErrorContext errCtx;
	
	sLONG invalidSocket=kBAD_SOCKET;
	sLONG realSocket=XBOX::VInterlocked::Exchange(&fSock, invalidSocket);
	
	if(fSslDelegate!=NULL)
		fSslDelegate->Shutdown();

	//Changed behavior : We are a well-behaved server now !
	//(And we don't want our peer's OS to discard valid but unread data)
	int err=shutdown(realSocket, SHUT_RDWR);
			
	if(inWithRecvLoop)
	{
		// Now we treat some remaining data that might have arrived late
		// They are lost, but it helps preventing a connection RESET
		// to be sent
		
		//jmo - We try to protect IE clients from a page loading failure, with
		//a 30s "security" timeout. IMHO we should get rid of that !
			
		StDropErrorContext errCtxRcvLoop;

		char garbage[512];
		uLONG len=sizeof(garbage);
		
		ReadWithTimeout(garbage, &len, 30);
	}

	// Finally, we close the socket.
	if(realSocket!=kBAD_SOCKET)
	{
		do
			err=close(realSocket);
		while(err==-1 && errno==EINTR);
	}
	
	return err==0 ? VE_OK : vThrowNativeError(errno);
}


VError XBsdTCPSocket::SetBlocking(bool inBlocking)
{
	int flags=fcntl(fSock, F_GETFL);
	
	if(flags==-1)
		return vThrowNativeError(errno);
	
	
	if(inBlocking)
		flags&=~O_NONBLOCK;
	else
		flags|=O_NONBLOCK;
	
	
	int err=fcntl(fSock, F_SETFL, flags);

	if(err==-1)
		return vThrowNativeError(errno);

	return VE_OK;
}


bool XBsdTCPSocket::IsBlocking()
{
	int flags=fcntl(fSock, F_GETFL);
	
	if(flags==-1)
	{
		vThrowNativeError(errno);
		return false;
	}
		
	return ((flags&O_NONBLOCK)==0);
}


VError XBsdTCPSocket::WaitFor(Socket inFd, FdSet inSet, sLONG inMsTimeout, sLONG* outMsSpent)
{
	// - outMsSpent is optional but always computed (on success and error) if not NULL. It may be > to inMsTimeout,
	//   reflecting the fact that the call was (slightly) longer than the requested timeout.
	// - Caller should deal with special error VE_SOCK_TIMEOUT

	if(inMsTimeout<0)
		return VE_SOCK_TIMED_OUT;
		
	sLONG start=VSystem::GetCurrentTime();
	sLONG stop=start+inMsTimeout;
	sLONG now=start;
	
	for(;;)
	{
		sLONG msTimeout=stop-now;

		timeval timeout={0};
				
		timeout.tv_sec=msTimeout/1000;
		timeout.tv_usec=1000*(msTimeout%1000);
		
        fd_set set;
        FD_ZERO(&set);
        FD_SET(inFd, &set);

		fd_set* rs=(inSet==kREAD_SET) ? &set : NULL;
		fd_set* ws=(inSet==kWRITE_SET) ? &set : NULL;
		fd_set* es=(inSet==kERROR_SET) ? &set : NULL;

        int res=select(inFd+1, rs, ws, es, &timeout);
		
		if(res==-1 && errno==EINTR)
		{
			now=VSystem::GetCurrentTime();
			continue;
		}
		
		//Prepare to return and update spent time if caller asked for it
		
		if(outMsSpent!=NULL)
		{
			now=VSystem::GetCurrentTime();
			*outMsSpent=now-start;
		}
		
		if(res==0)
			return VE_SOCK_TIMED_OUT;
		
		if(res==1)
			return VE_OK;
		
		return vThrowNativeError(errno);
	}
}


VError XBsdTCPSocket::WaitForConnect(sLONG inMsTimeout, sLONG* outMsSpent)
{
	//connect : When the connection has been established asynchronously, select() shall indicate that 
	//          the file descriptor for the socket is ready for writing.
	
	return WaitFor(GetRawSocket(), kWRITE_SET, inMsTimeout, outMsSpent);
}


VError XBsdTCPSocket::WaitForAccept(sLONG inMsTimeout, sLONG* outMsSpent)
{
	//accept : When a connection is available, select() indicates that
	//         the file descriptor for the socket is ready for reading.
	return WaitFor(GetRawSocket(), kREAD_SET, inMsTimeout, outMsSpent);
}


VError XBsdTCPSocket::WaitForRead(sLONG inMsTimeout, sLONG* outMsSpent)
{
	return WaitFor(GetRawSocket(), kREAD_SET, inMsTimeout, outMsSpent);
}


VError XBsdTCPSocket::WaitForWrite(sLONG inMsTimeout, sLONG* outMsSpent)
{
	return WaitFor(GetRawSocket(), kWRITE_SET, inMsTimeout, outMsSpent);
}


VError XBsdTCPSocket::Connect(const VNetAddress& inAddr, sLONG inMsTimeout)
{
	xbox_assert(fProfile==NewSock);
	
	VError verr=SetBlocking(false);
	
	if(verr!=VE_OK)
		return verr;
	
	int err=0;
	
	do
		err=connect(fSock, inAddr.GetAddr(), inAddr.GetAddrLen());
	while(err==-1 && errno==EINTR);
	
	if(err!=0 && errno!=EINPROGRESS && errno!=EALREADY)
		return vThrowNativeError(errno);

	//If for any reason the socket is already connected, we take a shortcut to save a select syscall.
	if(err==0)
		return verr;
	
	verr=WaitForConnect(inMsTimeout);
	
	if(verr!=VE_OK)
		return verr;
	
	//Wait returns, but it doesn't mean the connection succed ! Retry connect to retrieve the status
	do
		err=connect(fSock, inAddr.GetAddr(), inAddr.GetAddrLen());
	while(err==-1 && errno==EINTR);
	
	if(err==-1 && errno!=EISCONN)
		return vThrowNativeError(errno);
		
	fProfile=ClientSock;
	
	verr=SetBlocking(true);
	
	return verr;
}


VError XBsdTCPSocket::Listen(const VNetAddress& inAddr, bool inAlreadyBound)
{
	xbox_assert(fProfile==NewSock);

	int err=0;
	
	if(!inAlreadyBound && inAddr.IsV6())
	{
		int opt=!WithV4mappedV6();

		err=setsockopt(fSock, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt));
		
		if(err!=0)
			return vThrowNativeError(errno);
	}
		
	if(!inAlreadyBound)
	{	
		int opt=true;
		err=setsockopt(fSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
		
		if(err!=0)
			return vThrowNativeError(errno);
		
		err=bind(fSock, inAddr.GetAddr(), inAddr.GetAddrLen());
		
		if(err!=0)
			return vThrowNativeError(errno);
	}
	
	err=listen(fSock, SOMAXCONN);
	
	if(err==0)
	{
		fProfile=ServiceSock;
		
		return VE_OK;
	}
	
	return vThrowNativeError(errno);
}


XBsdTCPSocket* XBsdTCPSocket::Accept(uLONG inMsTimeout)
{
	xbox_assert(fProfile==ServiceSock);

	VError verr=VE_OK;
	
	if(inMsTimeout>0)
	{
		verr=WaitForAccept(inMsTimeout);
		
		if(verr!=VE_OK)
			return NULL;
	}
	
	//Should not be necessary...
	verr=SetBlocking(false);

	if(verr!=VE_OK)
		return NULL;

	sockaddr_storage sa_storage;
	socklen_t len=sizeof(sa_storage);
	memset(&sa_storage, 0, len);
	
	sockaddr* sa=reinterpret_cast<sockaddr*>(&sa_storage);
	
	int sock=kBAD_SOCKET;
	
	do
		sock=accept(GetRawSocket(), sa, &len);
	while(sock==kBAD_SOCKET && errno==EINTR);

	
	if(sock==kBAD_SOCKET)
	{
		vThrowNativeError(errno);
		return NULL;
	}
		
	bool ok=true;

	if(ok && sa->sa_family!=AF_INET && sa->sa_family!=AF_INET6)
		ok=false;
	
	int opt=true;
	int err=0;

	if(ok)
	{
		err=setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
		
		if(err!=0)
			{ ok=false ; vThrowNativeError(errno); }
	}

	if(ok)
	{
		err=setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

		if(err!=0)
			{ ok=false ; vThrowNativeError(errno); }
	}
	
#if !VERSION_LINUX
	if(ok)
	{
		err=setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
		
		if(err!=0)
			{ ok=false ; vThrowNativeError(errno); }
	}
#endif

	XBsdTCPSocket* xsock=NULL;
	
	if(ok)
	{
		xsock=new XBsdTCPSocket(sock);

		if(xsock==NULL)
			{ ok=false ; vThrowError(VE_MEMORY_FULL); }
	}

	if(ok)
		xsock->fProfile=ConnectedSock;
		
	if(ok)
	{
		verr=xsock->SetBlocking(true);
		
		if(verr!=VE_OK)
			ok=false;
	}
	
	if(ok)
	{
		verr=xsock->SetServicePort(GetPort());

		if(verr!=VE_OK)
			ok=false;
	}

	if(!ok)
	{
		if(xsock!=NULL)
		{
			xsock->Close(false);
		
			delete xsock;
		
			xsock=NULL;	
		}	
		else
		{
			do
				err=close(sock);
			while(err==-1 && errno==EINTR);
		}
	}	
	
	return xsock;
}


VError XBsdTCPSocket::Read(void* outBuff, uLONG* ioLen)
{
	VError verr=DoRead(outBuff, ioLen);

	return verr;
}


VError XBsdTCPSocket::DoRead(void* outBuff, uLONG* ioLen)
{
	// - outBuff and ioLen are mandatory ; ioLen is always modified (set to 0 on error)
	// - Caller should deal with special errors VE_SOCK_WOULD_BLOCK and VE_SOCK_PEER_OVER
	
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	if(fSslDelegate!=NULL)
		return fSslDelegate->Read(outBuff, ioLen);
	
	ssize_t n=0;
	
	do
		n=recv(fSock, outBuff, *ioLen, 0 /*flags*/);
	while(n==-1 && errno==EINTR);

#if VERSIONDEBUG && WITH_SNET_SOCKET_LOG
	DebugMsg ("[%d] XBsdTCPSocket::Read : socket=%d len=%d res=%d\n",
			  VTask::GetCurrentID(), GetRawSocket(), *ioLen, n);
#endif	
	
	if(n>0)
	{
		*ioLen=static_cast<uLONG>(n);
		
		return VE_OK;
	}
	
	//We have an error...
	*ioLen=0;
	
	if(n==0)
	{
		//If no messages are available to be received and the peer has performed an orderly shutdown,
		//recv() shall return 0.
		
		return VE_SOCK_PEER_OVER;
	}
	
	if(errno==EWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(errno==ECONNRESET || errno==ENOTSOCK || errno==EBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, errno);
	
	return vThrowNativeCombo(VE_SOCK_READ_FAILED, errno);
}


VError XBsdTCPSocket::Write(const void* inBuff, uLONG* ioLen, bool /*inWithEmptyTail*/)
{
	VError verr=DoWrite(inBuff, ioLen);
		
	return verr;
}


VError XBsdTCPSocket::DoWrite(const void* inBuff, uLONG* ioLen)
{
	// - inBuff and ioLen are mandatory ; ioLen is always modified (set to 0 on error)
	// - Caller should deal with special error VE_SOCK_WOULD_BLOCK
	
	if(inBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	if(fSslDelegate!=NULL)
		return fSslDelegate->Write(inBuff, ioLen);

    int flags=0;

#if VERSION_LINUX
    flags|=MSG_NOSIGNAL;
#endif

	ssize_t n=send(fSock, inBuff, *ioLen, flags);
	
	//No need for windows-client bWithEmptyTail fix here
	
	if(n>=0)
	{
		*ioLen=static_cast<uLONG>(n);
		return VE_OK;
	}
	
	//We have an error...
	*ioLen=0;
		
	if(errno==EWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(errno==ECONNRESET || errno==ENOTSOCK || errno==EBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, errno);
	
	return vThrowNativeCombo(VE_SOCK_WRITE_FAILED, errno);
}


VError XBsdTCPSocket::ReadWithTimeout(void* outBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent)
{
	VError verr=DoReadWithTimeout(outBuff, ioLen, inMsTimeout, outMsSpent);
	
	return verr;
}


VError XBsdTCPSocket::DoReadWithTimeout(void* outBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent)
{
	// - outBuff and ioLen are mandatory ; ioLen is always modified (set to 0 on error)
	// - inMsTimeout <= 0 results in some form of polling
	// - outMsSpent is optional but always computed (on success and error) if not NULL. It may be > to inMsTimeout,
	//   reflecting the fact that the call was (slightly) longer than the requested timeout.
	// - Caller should deal with special errors VE_SOCK_TIMEOUT and VE_SOCK_PEER_OVER
	// - Partial read before end of timeout is not an error ; VE_OK means something was read.
	// - this method might change and restore the socket blocking state
	
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	
	VError verr=VE_OK;

	uLONG len=*ioLen;

	if(fSslDelegate!=NULL)
	{		
		sLONG spentTotal=0;
		
		sLONG available=fSslDelegate->GetBufferedDataLen();
		
		if(available>=len)
		{
			//First case : OpenSSL has enough buffered data ; No socket IO should occur.
			
			verr=fSslDelegate->Read(outBuff, &len);
			
			spentTotal=0;
		}
		else
		{	
			//Second case : We have to handle SSL protocol IO that might occur before having applicative data
		
			bool wasBlocking=false;
		
			if(IsBlocking())
			{
				SetBlocking(false);
				wasBlocking=true;
			}
				
			sLONG timeout=inMsTimeout;
		
			do
			{
				len=*ioLen;
				
				verr=fSslDelegate->Read(outBuff, &len);

				if(len>0)	//We have some data ; that's enough for now.
					break;
		
				//We have no data ; perhaps we need waiting for something...
			
				sLONG spentOnStep=0;
				
				if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantRead())
					verr=WaitForRead(timeout, &spentOnStep);
				else if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantWrite())
					verr=WaitForWrite(timeout, &spentOnStep);
		
				timeout-=spentOnStep;
				spentTotal+=spentOnStep;
			}
			while(verr==VE_OK);

			if(wasBlocking)
				SetBlocking(true);
		}	
		
		if(outMsSpent!=NULL)
			*outMsSpent=spentTotal;
	}
	else
	{
		verr=WaitForRead(inMsTimeout, outMsSpent);

		if(verr==VE_OK)
			verr=DoRead(outBuff, &len);
		else
			len=0;
	}
	
	if(verr==VE_OK && len==0)
	{
		//Having nothing to read should result in VE_SOCK_TIMED_OUT. If read is OK but len is 0, it really means the peer
		//did a shutdown (or ssl equivalent). We return a special error so the caller dont try to call us in a mad loop.
		
		verr=VE_SOCK_PEER_OVER;
	}
	
	//Check len and err code are consistent
	xbox_assert((len==0 && verr!=VE_OK) || (len>0 && verr==VE_OK));
		
	//sLONG errCode=ERRCODE_FROM_VERROR(verr);
	
	//Check this error never escape !
	xbox_assert(verr!=VE_SOCK_WOULD_BLOCK);
	
	*ioLen=len;
	
	if(verr==VE_OK || verr==VE_SOCK_TIMED_OUT || verr==VE_SOCK_PEER_OVER)
		return verr;
		
	return vThrowError(verr);	//might be VE_OK, which throws nothing.
}


VError XBsdTCPSocket::WriteWithTimeout(const void* inBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent, bool /*unusedWithEmptyTail*/)
{
	VError verr=DoWriteWithTimeout(inBuff, ioLen, inMsTimeout, outMsSpent);
		
	return verr;
}


VError XBsdTCPSocket::DoWriteWithTimeout(const void* inBuff, uLONG* ioLen, sLONG inMsTimeout, sLONG* outMsSpent)
{
	// - inBuff and ioLen are mandatory ; ioLen is always modified (set to 0 on error)
	// - inMsTimeout <= 0 results in some form of polling
	// - outMsSpent is optional but always computed (on success and error) if not NULL. It may be > to inMsTimeout,
	//   reflecting the fact that the call was (slightly) longer than the requested timeout.
	// - Caller should deal with special error VE_SOCK_TIMED_OUT
	// - Partial write before end of timeout is not an error ; VE_OK means something was sent.
	// - this method might change and restore the socket blocking state
	
	if(inBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	
	VError verr=VE_OK;
	
	uLONG len=*ioLen;
	
	if(fSslDelegate!=NULL)
	{
		sLONG spentTotal=0;
		
		bool wasBlocking=false;
		
		if(IsBlocking())
		{
			SetBlocking(false);
			wasBlocking=true;
		}
		
		sLONG timeout=inMsTimeout;
		
		do
		{
			len=*ioLen;
			
			verr=fSslDelegate->Write(inBuff, &len);
			
			if(len>0)	//We sent some data ; that's enough for now.
				break;
			
			//No data were sent ; perhaps we need waiting for something...
			
			sLONG spentOnStep=0;
			
			if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantRead())
				verr=WaitForRead(timeout, &spentOnStep);
			else if(verr==VE_SOCK_WOULD_BLOCK && fSslDelegate->WantWrite())
				verr=WaitForWrite(timeout, &spentOnStep);
			
			timeout-=spentOnStep;
			spentTotal+=spentOnStep;
		}
		while(verr==VE_OK);
		
		if(wasBlocking)
			SetBlocking(true);
		
	}
	else
	{
		verr=WaitForWrite(inMsTimeout, outMsSpent);
	
		if(verr==VE_OK)
			verr=DoWrite(inBuff, &len);
	}

	//Check len and err code are consistent
	xbox_assert((len==0 && verr!=VE_OK) || (len>0 && verr==VE_OK));
	
	//Check those errors never escape !
	xbox_assert(verr!=VE_SOCK_WOULD_BLOCK);
	xbox_assert(verr!=VE_SOCK_PEER_OVER);
	
	*ioLen=len;
	
	return vThrowError(verr);	//might be VE_OK, which throws nothing.
}


//static
XBsdTCPSocket* XBsdTCPSocket::NewClientConnectedSock(const VString& inDnsName, PortNumber inPort, sLONG inMsTimeout)
{
	XBsdTCPSocket* xsock=NULL;
	
	VNetAddressList addrs;
	
	VError verr=addrs.FromDnsQuery(inDnsName, inPort);
	
	VNetAddressList::const_iterator cit;
	
	for(cit=addrs.begin() ; cit!=addrs.end() ; cit++)
	{
		//We have a list of address structures ; we choose the first one that connects successfully.
	
		int sock=socket(cit->GetPfFamily(), SOCK_STREAM, IPPROTO_TCP);
		
		if(sock==kBAD_SOCKET)
		{
			vThrowNativeError(errno);
		}
		else
		{
			int opt=true;
			int err=setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
			
			if(err!=0)
			{
				vThrowNativeError(errno);
				
				do
					err=close(sock);
				while(err==-1 && errno==EINTR);
				
				return NULL;
			}
			
			err=setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
			
			if(err!=0)
			{
				vThrowNativeError(errno);
				
				do
					err=close(sock);
				while(err==-1 && errno==EINTR);
				
				return NULL;
			}
			
#if !VERSION_LINUX
			err=setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &opt, sizeof(opt));
			
			if(err!=0)
			{
				vThrowNativeError(errno);
				
				do
					err=close(sock);
				while(err==-1 && errno==EINTR);
				
				return NULL;
			}
#endif
			
			xsock=new XBsdTCPSocket(sock);
			
			if(xsock==NULL)
			{
				do
					err=close(sock);
				while(err==-1 && errno==EINTR);
				
				vThrowError(VE_MEMORY_FULL);
				
				break;
			}
			
			verr=xsock->Connect(*cit, inMsTimeout);
			
			if(verr!=VE_OK)
			{
				xsock->Close();
				
				delete xsock;
				
				xsock=NULL;
			}
			else
			{				
				//DEPRECATED ? - We have to set the service port (duplicated stuff on client sock).
				PortNumber resolvedPort=cit->GetPort();
				
				xbox_assert(resolvedPort==inPort);
				
				xsock->SetServicePort(resolvedPort);
			}
		}
	}
	
	
#if VERSIONDEBUG && 0
	if(xsock!=NULL)
	{
		VString msg("New client socket :");
		msg.AppendString(" task=").AppendLong(VTask::GetCurrentID());
		msg.AppendString(" sock=").AppendLong(xsock->GetRawSocket());
		msg.AppendString(" client=").AppendString(xsock->GetClientIP()).AppendString(":").AppendLong(xsock->GetClientPort());
		msg.AppendString(" server=").AppendString(xsock->GetServerIP()).AppendString(":").AppendLong(xsock->GetServerPort());
		msg.AppendString("\n");
		DebugMsg(msg);
	}
#endif
	
	return xsock;
}


#if WITH_DEPRECATED_IPV4_API

//static
XBsdTCPSocket* XBsdTCPSocket::NewServerListeningSock(uLONG inIPv4, PortNumber inPort, Socket inBoundSock)
{
	sockaddr_in v4={0};
	v4.sin_family=AF_INET;
	v4.sin_addr.s_addr=htonl(inIPv4);
	v4.sin_port=htons(inPort);

	
	bool alreadyBound=(inBoundSock!=kBAD_SOCKET) ? true : false;
	
	int sock=(alreadyBound) ? inBoundSock : socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if(sock==kBAD_SOCKET)
	{
		vThrowNativeError(errno);
		return NULL;
	}

	XBsdTCPSocket* xsock=new XBsdTCPSocket(sock);
	
	if(xsock==NULL)
	{
		vThrowError(VE_MEMORY_FULL);
		return NULL;
	}
	
	xsock->SetServicePort(inPort);
	
	VNetAddress addr(v4);
	
	VError verr=xsock->Listen(addr, alreadyBound);
	
	if(verr!=VE_OK)
	{
		delete xsock;
		return NULL;
	}
		
	verr=xsock->SetBlocking(false);
	
	if(verr!=VE_OK)
	{
		delete xsock;
		return NULL;
	}

	return xsock;
}

#else

//static
XBsdTCPSocket* XBsdTCPSocket::NewServerListeningSock(const VNetAddress& inAddr, Socket inBoundSock)
{
	bool alreadyBound=(inBoundSock!=kBAD_SOCKET) ? true : false;
	
	int sock=(alreadyBound) ? inBoundSock : socket(inAddr.GetPfFamily(), SOCK_STREAM, IPPROTO_TCP);

	VError verr=VE_OK;
	
	if(sock==kBAD_SOCKET)
		verr=vThrowNativeError(errno);
	
	XBsdTCPSocket* xsock=NULL;
		
	if(verr==VE_OK)
	{	
		xsock=new XBsdTCPSocket(sock);

		if(xsock==NULL)
			verr=vThrowError(VE_MEMORY_FULL);
	}
	
	if(verr==VE_OK)
	{
		xsock->SetServicePort(inAddr.GetPort());
		
		verr=xsock->Listen(inAddr, alreadyBound);
	}
	
	if(verr==VE_OK)
		verr=xsock->SetBlocking(false);
	
	if(verr!=VE_OK)
	{
		delete xsock;
		xsock=NULL;
		
#if VERSIONDEBUG

		VString ip=inAddr.GetIP();
		PortNumber port=inAddr.GetPort();
		
		DebugMsg ("[%d] XBsdTCPSocket::NewServerListeningSock() : Fail to get listening socket for ip %S on port %d\n",
			VTask::GetCurrentID(), &ip, port);
#endif
	}
	
	return xsock;
}

#endif


//static
XBsdTCPSocket* XBsdTCPSocket::NewServerListeningSock(PortNumber inPort, Socket inBoundSock)
{
#if WITH_DEPRECATED_IPV4_API
	return NewServerListeningSock(INADDR_ANY, inPort, inBoundSock);
#else
	VNetAddress anyAddr;
	VError verr=anyAddr.FromAnyIpAndPort(inPort);
	
	xbox_assert(verr==VE_OK);
	
	return NewServerListeningSock(anyAddr, inBoundSock);
#endif	
}


XBOX::VError XBsdTCPSocket::SetNoDelay(bool inYesNo)
{
	int	opt	= inYesNo;
	int err	= 0;

	err = setsockopt(fSock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
	
	return !err ? XBOX::VE_OK : vThrowNativeError(errno);
}


XBOX::VError XBsdTCPSocket::PromoteToSSL(VKeyCertChain* inKeyCertChain)
{
	VSslDelegate* delegate=NULL;
	
	switch(fProfile)
	{
	case ConnectedSock :
			
			delegate=VSslDelegate::NewServerDelegate(GetRawSocket(), inKeyCertChain);
			break;
			
	case ClientSock :
			
			delegate=VSslDelegate::NewClientDelegate(GetRawSocket()/*, inKeyCertChain*/);
			break;
			
	default :

			xbox_assert(fProfile!=ConnectedSock && fProfile!=ClientSock);
	}
	
	if(delegate==NULL)
		return vThrowError(VE_SOCK_FAIL_TO_ADD_SSL);

	fSslDelegate=delegate;
	
	return VE_OK;
}


bool XBsdTCPSocket::IsSSL()
{
	return fSslDelegate!=NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XBsdAcceptIterator
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XBsdAcceptIterator::XBsdAcceptIterator()
{
	fSockIt=fSocks.end();
	FD_ZERO(&fReadSet);
}


VError XBsdAcceptIterator::AddServiceSocket(XBsdTCPSocket* inSock)
{
	if(inSock==NULL)
		return VE_INVALID_PARAMETER;

	fSocks.push_back(inSock);

	//Push back may invalidate the collection iterator...
	fSockIt=fSocks.end();
	
	return VE_OK;
}


VError XBsdAcceptIterator::ClearServiceSockets()
{
	fSocks.clear();
	
	//clear will invalidate the collection iterator...
	fSockIt=fSocks.end();
	
	return VE_OK;
}


VError XBsdAcceptIterator::GetNewConnectedSocket(XBsdTCPSocket** outSock, sLONG inMsTimeout)
{
	if(outSock==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
	
	bool shouldRetry=false;
	
	VError verr=GetNewConnectedSocket(outSock, inMsTimeout, &shouldRetry);
	
	if(verr==VE_OK && *outSock==NULL && shouldRetry)
		verr=GetNewConnectedSocket(outSock, inMsTimeout, &shouldRetry /*ignored*/);		
	
	return verr;
}


VError XBsdAcceptIterator::GetNewConnectedSocket(XBsdTCPSocket** outSock, sLONG inMsTimeout, bool* outShouldRetry)
{
	if(outSock==NULL || outShouldRetry==NULL)
		return vThrowError(VE_INVALID_PARAMETER);
		
	*outSock=NULL;
	*outShouldRetry=true;	//Indicate wether the select call is done (we should not retry) or not (we should retry).
	
	if(inMsTimeout<0)
		return VE_SOCK_TIMED_OUT;
	
	
	if(fSockIt==fSocks.end())
	{
		//We need to issue a new select call, with timeout
		*outShouldRetry=false;

        FD_ZERO(&fReadSet);
		
		sLONG maxFd=-1;
		
		SockPtrColl::const_iterator cit;
		
		for(cit=fSocks.begin() ; cit!=fSocks.end() ; ++cit)
		{
			sLONG fd=(*cit)->GetRawSocket();

			if(fd>maxFd)
				maxFd=fd;
			
			FD_SET(fd, &fReadSet);
		}
		
		sLONG now=VSystem::GetCurrentTime();
		sLONG stop=now+inMsTimeout;
		
		for(;;)
		{
			sLONG msTimeout=stop-now;
			
			timeval timeout={0};
			
			timeout.tv_sec=msTimeout/1000;
			timeout.tv_usec=1000*(msTimeout%1000);
		
			int res=select(maxFd+1, &fReadSet, NULL, NULL, &timeout);
	
			if(res==0)
				return VE_SOCK_TIMED_OUT;
			
			if(res>=1)
				break;
		
			if(res==-1 && errno==EINTR)
			{
				now=VSystem::GetCurrentTime();
				continue;
			}
		
			return vThrowNativeError(errno);
		}
		
		fSockIt=fSocks.begin();
	}
	
	//We have to handle remaining service sockets ; find next ready one
		
	while(fSockIt!=fSocks.end())
	{		
		sLONG fd=(*fSockIt)->GetRawSocket();
		
		if(FD_ISSET(fd, &fReadSet))
		{
			*outSock=(*fSockIt)->Accept(0 /*No timeout*/);
						
			++fSockIt;	//move to next socket ; prefer equity over perf !
			
			return VE_OK;
		}
		else
		{			
			++fSockIt;	//move to next socket ; prefer equity over perf !
		}
	}

	//No luck ; None of the remaining sockets were ready to accept... The caller 'shouldRetry' to call us !
	return VE_OK;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// XBsdUDPSocket
//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

XBsdUDPSocket::~XBsdUDPSocket()
{
	//Close();
	
#if VERSIONDEBUG
	if(fSock!=kBAD_SOCKET)
		DebugMsg ("[%d] XBsdUDPSocket::~XBsdUDPSocket() : socket %d not closed yet !\n", VTask::GetCurrentID(), fSock);
#endif
}


VError XBsdUDPSocket::SetBlocking(uBOOL inBlocking)
{
	int flags=fcntl(fSock, F_GETFL);
	
	if(flags==-1)
		return vThrowNativeError(errno);
	
	
	if(inBlocking)
		flags&=~O_NONBLOCK;
	else
		flags|=O_NONBLOCK;
	
	
	int err=fcntl(fSock, F_SETFL, flags);
	
	if(err==-1)
		return vThrowNativeError(errno);
	
	return VE_OK;
}


VError XBsdUDPSocket::Close()
{
	int err=0;
	 
	do
		err=close(fSock);
	while(err==-1 && errno==EINTR);
	
	fSock=kBAD_SOCKET;
	
	return err==0 ? VE_OK : vThrowNativeError(errno);
}


VError XBsdUDPSocket::Read(void* outBuff, uLONG* ioLen, VNetAddress* outSenderInfo)
{
	if(outBuff==NULL || ioLen==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

	sockaddr_storage addr;
	socklen_t addrLen=sizeof(addr);

	ssize_t n=0;

	do
		n=recvfrom(fSock, outBuff, *ioLen, 0 /*flags*/, reinterpret_cast<sockaddr*>(&addr), &addrLen);
	while(n==-1 && errno==EINTR);

	
	if(n>=0)
	{
		*ioLen=static_cast<uLONG>(n);

		if(outSenderInfo!=NULL)
			outSenderInfo->SetAddr(addr);

		return VE_OK;
	}
	
	//We have an error...
	*ioLen=0;
	
	//En principe on ne gere pas le mode non bloquant avec UDP
	
	if(errno==EWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(errno==ECONNRESET || errno==ENOTSOCK || errno==EBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, errno);
	
	return vThrowNativeCombo(VE_SOCK_READ_FAILED, errno);
}


VError XBsdUDPSocket::Write(const void* inBuff, uLONG inLen, const VNetAddress& inReceiverInfo)
{
	if(inBuff==NULL)
		return vThrowError(VE_INVALID_PARAMETER);

    int flags=0;

#if VERSION_LINUX
    flags|=MSG_NOSIGNAL;
#endif

	ssize_t n=sendto(fSock, inBuff, inLen, flags, inReceiverInfo.GetAddr(), inReceiverInfo.GetAddrLen());
   	
	if(n>=0)
		return VE_OK;
	
	//En principe on ne gere pas le mode non bloquant avec UDP

	if(errno==EWOULDBLOCK)
		return VE_SOCK_WOULD_BLOCK;
	
	if(errno==ECONNRESET || errno==ENOTSOCK || errno==EBADF)
		return vThrowNativeCombo(VE_SOCK_CONNECTION_BROKEN, errno);
	
	return vThrowNativeCombo(VE_SOCK_WRITE_FAILED, errno);
}

#if WITH_DEPRECATED_IPV4_API

VError XBsdUDPSocket::SubscribeMulticast(uLONG inLocalIpv4, uLONG inMulticastIPv4)
{
	//Membership is dropped on close or process termination. The OS take care of it.
	
	ip_mreq	mreq;	
	memset(&mreq, 0, sizeof(mreq));
	
	mreq.imr_multiaddr.s_addr=htonl(inMulticastIPv4);
	mreq.imr_interface.s_addr=htonl(inLocalIpv4);
	
	int err=setsockopt(fSock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
	
	return err==0 ? VE_OK : vThrowNativeCombo(VE_SRVR_CANT_SUBSCRIBE_MULTICAST, errno);
}


VError XBsdUDPSocket::Bind(const VNetAddress& inBindAddr)
{
	int opt=true;
	
#if VERSION_LINUX
	
    //SO_REUSEPORT doesn't exist on Linux ; multicast may not work properly.
    //See this comment in /usr/include/asm-generic/socket.h :
    /* To add :#define SO_REUSEPORT 15 */
	
    int err=setsockopt(fSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
#else
	
	int err=setsockopt(fSock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	
#endif	//VERSION_LINUX
	
	if(err!=0)
		return vThrowNativeError(errno);
		
	err=bind(fSock, inBindAddr.GetAddr(), inBindAddr.GetAddrLen());
	
	return err==0 ? VE_OK : vThrowNativeError(errno);
}

//static
XBsdUDPSocket* XBsdUDPSocket::NewMulticastSock(uLONG inLocalIpv4, uLONG inMulticastIPv4, PortNumber inPort)
{	
	int sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	
	if(sock==kBAD_SOCKET)
	{
		vThrowNativeError(errno);
		return NULL;
	}
	
	XBsdUDPSocket* xsock=new XBsdUDPSocket(sock);
	
	if(xsock==NULL)
	{
		vThrowError(VE_MEMORY_FULL);
		return NULL;
	}

	VError verr=xsock->SubscribeMulticast(inLocalIpv4, inMulticastIPv4);
	
	if(verr!=VE_OK)
	{
		delete xsock;
		return NULL;
	}
	
	VNetAddress anyIP(inPort);
	
	verr=xsock->Bind(anyIP);
	
	if(verr!=VE_OK)
	{
		delete xsock;
		return NULL;
	}

	return xsock;
}

#else	//WITH_DEPRECATED_IPV4_API

//static
XBsdUDPSocket* XBsdUDPSocket::NewMulticastSock(const VString& inMultiCastIP, PortNumber inPort)
{
	VNetAddress localAddr(inPort);
	VNetAddress mcastAddr(inMultiCastIP, inPort);
	
	int sock=socket(localAddr.GetPfFamily(), SOCK_DGRAM, IPPROTO_UDP);
		
	if(sock==kBAD_SOCKET)
	{
		vThrowNativeError(errno);
		return NULL;
	}
	
	XBsdUDPSocket* xsock=new XBsdUDPSocket(sock);
		
	if(xsock==NULL)
		vThrowError(VE_MEMORY_FULL);

	int err=0;
	
	if(xsock!=NULL && err==0)
	{			
		int opt=true;

#if VERSION_LINUX
	
		//SO_REUSEPORT doesn't exist on Linux ; multicast may not work properly.
		//See this comment in /usr/include/asm-generic/socket.h :
		/* To add :#define SO_REUSEPORT 15 */
	
		err=setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
#else
		
		err=setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	
#endif
	}
		
	if(xsock!=NULL && err==0)
	{
		if(ListV6() && mcastAddr.IsV6())
		{
			//IPv6-TODO : Simplifier (débugger) tout ça !
			std::map<VString, sLONG> interfaces;
			
			VNetAddressList addrs;
			addrs.FromLocalInterfaces();
			
			VNetAddressList::const_iterator addr;
			
			for(addr=addrs.begin() ; addr!=addrs.end() ; ++addr)
				if(addr->IsV6() && !addr->IsLocal() && !addr->IsLoopBack())
					interfaces[addr->GetName()]=addr->GetIndex();
			
			std::map<VString, sLONG>::const_iterator interface;
		
			for(interface=interfaces.begin() ; interface!=interfaces.end() ; ++interface)
			{			
				int ifCount=0;

				ipv6_mreq mreq;	
				memset(&mreq, 0, sizeof(mreq));
					
				mreq.ipv6mr_interface=interface->second;
				mcastAddr.FillIpV6(&mreq.ipv6mr_multiaddr);
					
				err=setsockopt(sock, IPPROTO_IPV6, IPV6_JOIN_GROUP, &mreq, sizeof(mreq));
				
				if(err==0)
					err=setsockopt(sock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &mreq.ipv6mr_interface, sizeof(mreq.ipv6mr_interface));
				
				if(err==0)
				{
					ifCount++;
					
					DebugMsg("Subscribed multicast on interface %S\n", &interface->first);
				}
				else
				{
					DebugMsg("Fail to subscribe to multicast on interface %S\n", &interface->first);
				}
			}
		}
		else if(ListV4() && mcastAddr.IsV4())
		{
			ip_mreq	mreq;	
			memset(&mreq, 0, sizeof(mreq));
	
			mreq.imr_interface.s_addr=INADDR_ANY;	//Let the OS choose an interface
			mcastAddr.FillIpV4((IP4*)&(mreq.imr_multiaddr.s_addr));
		
			err=setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
		}
		else
		{
			err=1;
			vThrowError(VE_INVALID_PARAMETER);
		}
			
		xbox_assert(err!=1);
		
		if(err==-1)
			vThrowNativeCombo(VE_SRVR_CANT_SUBSCRIBE_MULTICAST, errno);
		else if(err==1)
			vThrowError(VE_SRVR_CANT_SUBSCRIBE_MULTICAST);
			
	}
		
	if(xsock!=NULL && err==0)
	{		
		err=bind(sock, localAddr.GetAddr(), localAddr.GetAddrLen());
		
		if(err!=0)
			vThrowNativeCombo(VE_SRVR_CANT_SUBSCRIBE_MULTICAST, errno);
	}
	
	if(xsock==NULL || err!=0)
	{
		do
			err=close(sock);
		while(err==-1 && errno==EINTR);
		
		delete xsock;
		
		return NULL;
	}
	
	return xsock;
}


#endif	//WITH_DEPRECATED_IPV4_API


END_TOOLBOX_NAMESPACE
