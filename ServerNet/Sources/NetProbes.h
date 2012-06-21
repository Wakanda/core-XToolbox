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
#include "DTraceProbes.h"


#ifndef __SNET_PROBES__
#define __SNET_PROBES__


BEGIN_TOOLBOX_NAMESPACE


namespace NetProbes
{	
	inline bool ReadConnection(const XBsdTCPSocket* inThis)
	{
		
#if WITH_DTRACE
		
		if(inThis!=NULL && WAKANDA_READ_CONNECTION_ENABLED())
		{	
			IP localIp, peerIp;
			
			inThis->GetLocalIP(&localIp);
			inThis->GetPeerIP(&peerIp);
			
			WAKANDA_READ_CONNECTION(inThis->GetRawSocket(), localIp, inThis->GetLocalPort(), peerIp, inThis->GetPeerPort());
			
			return true;
		}
		
#endif

		return false;
	};
	
	
	inline bool ReadStart(const XBsdTCPSocket* inThis, uLONG inMaxLen)
	{
		
#if WITH_DTRACE
		
		if(inThis!=NULL && WAKANDA_READ_START_ENABLED())
		{
			WAKANDA_READ_START(inThis->GetRawSocket(), inMaxLen);
			
			return true;
		}
		
#endif		
		
		return false;
	};
	
	
	inline bool ReadDone(const XBsdTCPSocket* inThis, uLONG inReadLen)
	{
		
#if WITH_DTRACE		
		
		if(inThis!=NULL && WAKANDA_READ_DONE_ENABLED())
		{
			WAKANDA_READ_DONE(inThis->GetRawSocket(), inReadLen);
			
			return true;
		}
		
#endif		
		
		return false;
	};
	

	inline bool ReadDump(const XBsdTCPSocket* inThis, const void* inBuf, uLONG inLen)
	{
		
#if WITH_DTRACE
		
		if(inLen>0 && inThis!=NULL && WAKANDA_READ_START_ENABLED())
		{
			const char* start=reinterpret_cast<const char*>(inBuf);
			const char* p1=start;
			const char* past=start+inLen;
		
			const int DUMP_SIZE=100;

			char buf[DUMP_SIZE+1];
			memset(buf, 0, sizeof(buf));

			char* p2=buf;
			
			while(p1<past)
			{
				if(p2-buf==DUMP_SIZE)
				{
					WAKANDA_READ_DUMP(inThis->GetRawSocket(), p1-start, buf);
					
					memset(buf, 0, sizeof(buf));
					p2=buf;
				}
				
				*p2=(isprint(*p1) ? *p1 : '.');
				p1++, p2++;
			}
				
			WAKANDA_READ_DUMP(inThis->GetRawSocket(), p1-start, buf);
			
			return true;
		};

#endif		
		
		return false;
	};

	inline bool WriteConnection(const XBsdTCPSocket* inThis)
	{
		
#if WITH_DTRACE
		
		if(inThis!=NULL && WAKANDA_WRITE_CONNECTION_ENABLED())
		{
			IP localIp, peerIp;
			
			inThis->GetLocalIP(&localIp);
			inThis->GetPeerIP(&peerIp);
			
			WAKANDA_WRITE_CONNECTION(inThis->GetRawSocket(), localIp, inThis->GetLocalPort(), peerIp, inThis->GetPeerPort());
			
			return true;
		}

#endif
		
		return false;
	};
	
	
	inline bool WriteStart(const XBsdTCPSocket* inThis, uLONG inMaxLen)
	{
		
#if WITH_DTRACE		
		
		if(inThis!=NULL && WAKANDA_WRITE_START_ENABLED())
		{
			WAKANDA_WRITE_START(inThis->GetRawSocket(), inMaxLen);
			
			return true;
		}

#endif		
		
		return false;
	};
	
	
	inline bool WriteDone(const XBsdTCPSocket* inThis, uLONG inReadLen)
	{
		
#if WITH_DTRACE
		
		if(inThis!=NULL && WAKANDA_WRITE_DONE_ENABLED())
		{
			WAKANDA_WRITE_DONE(inThis->GetRawSocket(), inReadLen);
			
			return true;
		}
		
#endif		
		
		return false;
	};
	
	
	inline bool WriteDump(const XBsdTCPSocket* inThis, const void* inBuf, uLONG inLen)
	{
		
#if WITH_DTRACE
		
		if(inLen>0 && inThis!=NULL && WAKANDA_WRITE_START_ENABLED())
		{
			const char* start=reinterpret_cast<const char*>(inBuf);
			const char* p1=start;
			const char* past=start+inLen;
			
			const int DUMP_SIZE=100;
			
			char buf[DUMP_SIZE+1];
			memset(buf, 0, sizeof(buf));

			char* p2=buf;
			
			while(p1<past)
			{
				if(p2-buf==DUMP_SIZE)
				{
					WAKANDA_WRITE_DUMP(inThis->GetRawSocket(), p1-start, buf);
					
					memset(buf, 0, sizeof(buf));
					p2=buf;
				}
				
				*p2=(isprint(*p1) ? *p1 : '.');
				
				p1++, p2++;
			}
			
			WAKANDA_WRITE_DUMP(inThis->GetRawSocket(), p1-start, buf);
			
			return true;
		};

#endif
		
		return false;
	};
};

END_TOOLBOX_NAMESPACE


#endif