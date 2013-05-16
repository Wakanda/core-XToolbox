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
#include "VProxyManager.h"
#include "HTTPTools.h"
#include "Tools.h"

USING_TOOLBOX_NAMESPACE
using namespace HTTPTools;


#if VERSIONMAC
#include <SystemConfiguration/SystemConfiguration.h>
#endif


//--------------------------------------------------------------------------------------------------

const sLONG	DEFAULT_HTTP_PORT	= 80;


#if VERSIONMAC

/*	code partially inspired from Technical Q&A QA1234 
	http://developer.apple.com/qa/qa2001/qa1234.html
*/

static
bool MAC_GetHTTPProxySettings (XBOX::VString& outProxyName, sLONG& outProxyPort, XBOX::VectorOfVString *outExceptionsList)
{
	bool				result = false;
	CFDictionaryRef		proxyDict;
	CFNumberRef			enableNum;
	int					enable;
	CFStringRef			hostStr;
	CFNumberRef			portNum;
	int					portInt;

	// Get the dictionary.

	proxyDict = SCDynamicStoreCopyProxies(NULL);
	result = (proxyDict != NULL);

	// Get the enable flag.	 This isn't a CFBoolean, but a CFNumber.
	if (result)
	{
		enableNum = (CFNumberRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesHTTPEnable);
		result = (enableNum != NULL) && (CFGetTypeID(enableNum) == CFNumberGetTypeID());
	}

	if (result)
		result = CFNumberGetValue(enableNum, kCFNumberIntType, &enable) && (enable != 0);

	// Get the proxy host.	DNS names must be in ASCII.	 If you 
	// put a non-ASCII character  in the "Web Proxy"
	// field in the Network preferences panel, the CFStringGetCString
	// function will fail and this function will return false.
	
	if (result)
	{
		hostStr = (CFStringRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesHTTPProxy);
		result = (hostStr != NULL) && (CFGetTypeID(hostStr) == CFStringGetTypeID());
	}

	if (result)
		outProxyName.MAC_FromCFString (hostStr);
	
	// Get the proxy port.

	if (result)
	{
		portNum = (CFNumberRef) CFDictionaryGetValue(proxyDict, kSCPropNetProxiesHTTPPort);
		result = (portNum != NULL) && (CFGetTypeID(portNum) == CFNumberGetTypeID());
	}

	if (result)
		result = CFNumberGetValue (portNum, kCFNumberIntType, &portInt);

	if (result)
		outProxyPort = (sLONG)portInt;

	if (result && (NULL != outExceptionsList))
	{
		CFArrayRef cfExceptionsList = (CFArrayRef)CFDictionaryGetValue (proxyDict, kSCPropNetProxiesExceptionsList);
		if (cfExceptionsList)
		{
			XBOX::VString string;
			for (CFIndex idx = 0; idx < CFArrayGetCount (cfExceptionsList) && result; idx++)
			{
				CFStringRef cfException = (CFStringRef)CFArrayGetValueAtIndex (cfExceptionsList, idx);
				result = (cfException != NULL) && (CFGetTypeID(cfException) == CFStringGetTypeID());
				
				if (result)
				{
					string.MAC_FromCFString (cfException);
					outExceptionsList->push_back (string);
				}
				else
				{
					string.Clear();
				}
			}
		}
	}

	// Clean up.
	if (proxyDict != NULL)
		CFRelease (proxyDict);

	if  (!result)
	{
		outProxyName.Clear();
		outProxyPort = 0;
		if (NULL != outExceptionsList)
			outExceptionsList->clear();
	}

	return result;
}


#elif VERSIONWIN


static
bool WIN_GetHTTPProxySettings (XBOX::VString& outProxyName, sLONG& outProxyPort, XBOX::VectorOfVString *outExceptionsList)
{
	LPCTSTR	lpSubKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings";
	HKEY	resHandle = NULL;
	char *	proxOverValue = NULL;
	char *	proxEnabValue = NULL;
	char *	proxServValue = NULL;			
	DWORD	proxOverSize = 0;
	DWORD	proxEnabSize = 0;
	DWORD	proxServSize = 0;			
	DWORD	maxLen = 0;	
	sLONG	err = 0;
	LSTATUS	proxErr = 0;

	//open the registry's key
	proxErr = RegOpenKeyEx (HKEY_CURRENT_USER, lpSubKey, (DWORD)0, (REGSAM)KEY_READ, (PHKEY)&resHandle);
	if (0 == proxErr)
	{
		//gets the max size for a value in this key
		proxErr = RegQueryInfoKey (resHandle, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, (LPDWORD)&maxLen, NULL, NULL);
	}
	if (0 == maxLen)
	{
		err = 1;
	}
	if (0 == proxErr)
	{
		proxServSize = proxOverSize = proxEnabSize = maxLen;	

		proxEnabValue = (char *)malloc (proxEnabSize);
		if (NULL == proxEnabValue)
			err = 1;
		proxServValue = (char *)malloc (proxServSize);
		if (NULL == proxServValue)
			err = 1;
		proxOverValue = (char *)malloc (proxOverSize);
		if (NULL == proxOverValue)
			err = 1;
	}

	if ((0 == err) && (0 == proxErr))
	{
		//Check that the proxy option is enabled
		proxErr = RegQueryValueEx (resHandle, TEXT ("ProxyEnable"), 0, NULL, (LPBYTE)proxEnabValue, &proxEnabSize);				
		if ((0 == err) && (0 == proxErr) && (NULL != proxEnabValue) && *proxEnabValue)
		{
			proxErr = RegQueryValueEx (resHandle, TEXT ("ProxyServer"), 0, NULL, (LPBYTE)proxServValue, &proxServSize);				
			if ((0 == err) && (NULL != proxServSize))
			{			
				//let's see if there are some domains for which the proxy settings should be overriden
				proxServSize--;//looks like a bug	

				XBOX::VString proxyParse;
				XBOX::VString proxyandPort;
				XBOX::VString proxyandPortClean;
				XBOX::VString proxNameBG;
				XBOX::VString portNbBG;

				proxyParse.FromBlock (proxServValue, proxServSize, XBOX::VTC_StdLib_char);

				//looking for the http proxy value
				sLONG startProxyStr = FindASCIICString (proxyParse, "http=");
				if (startProxyStr > 0)
				{
					XBOX::VString proxyTrimLeft;
					proxyParse.GetSubString (startProxyStr, proxyParse.GetLength(), proxyTrimLeft);

					sLONG endProxyPos = FindASCIICString (proxyTrimLeft, ";\0");
					if (0 == endProxyPos)
						endProxyPos = proxyTrimLeft.GetLength() + 1;

					proxyTrimLeft.GetSubString (5+1, endProxyPos-5-1, proxyandPort);
				}
				else if ((FindASCIICString (proxyParse, "https=") == 0) && (FindASCIICString (proxyParse, "ftp=") == 0) && (FindASCIICString (proxyParse, "socks=") == 0))
				{
					proxyandPort = proxyParse;
				}
				else
				{
					proxyandPort.Clear();
					err = 1; // No Proxy for HTTP
				}

				if (err == 0)
				{
					//Make sure there is no such things as "http://" or "https://" before the proxy name
					sLONG isGarbage = FindASCIICString (proxyandPort, "http://");
					if (isGarbage > 0)
					{
						proxyandPort.GetSubString (8, proxyandPort.GetLength() - 7, proxyandPortClean);
					}
					else
					{
						isGarbage = FindASCIICString (proxyandPort, "https://");
						if (isGarbage > 0)
						{
							proxyandPort.GetSubString (9, proxyandPort.GetLength() - 8, proxyandPortClean);
						}
						else
						{
							proxyandPortClean = proxyandPort;							
						}
					}

					sLONG delimiter = FindASCIICString (proxyandPortClean, ":\0");
					if (delimiter > 0)	// m.c, default value support
					{
						proxyandPortClean.GetSubString (1, delimiter-1, proxNameBG);
						if  (proxyandPortClean.GetLength() > delimiter)
							proxyandPortClean.GetSubString (delimiter+1, proxyandPortClean.GetLength()-delimiter, portNbBG);
					}
					else
					{
						proxNameBG = proxyandPortClean;
					}

					if(0 == err)
					{
						outProxyName.FromString (proxNameBG);
						outProxyPort = portNbBG.GetLength() ? portNbBG.GetLong() : DEFAULT_HTTP_PORT;
					}

					if (outExceptionsList != NULL)
					{						
						proxErr = RegQueryValueEx (resHandle, TEXT ("ProxyOverride"), 0, NULL, (LPBYTE)proxOverValue, &proxOverSize);
						if((0 == err) && (0 == proxErr) && (NULL != proxOverSize))
						{
							XBOX::VString exceptionsString ((char *)proxOverValue, proxOverSize, XBOX::VTC_StdLib_char);
							exceptionsString.GetSubStrings (CHAR_SEMICOLON, *outExceptionsList, false);
						}
					}
				}
			}
			else
			{
				err = 1;//no proxy
			}
		}
		else
		{
			err = 1;
		}
	}

	if (NULL != proxEnabValue)
		free (proxEnabValue);

	if (NULL != proxServValue)
		free (proxServValue);

	if (NULL != proxOverValue)
		free (proxOverValue);

	return (err == 0);
}

#elif VERSION_LINUX

static
bool LINUX_GetHTTPProxySettings (XBOX::VString& outProxyName, sLONG& outProxyPort, XBOX::VectorOfVString *outExceptionsList)
{
	outProxyName=VString();
	outProxyPort=0;

	//Exceptions are not implemented on Linux

	bool haveProxy=false;

	const char* proxy=getenv("http_proxy");
	
	if(proxy==NULL)
		proxy=getenv("HTTP_PROXY");

	if(proxy!=NULL)
	{
		VURL url(VString(proxy), false /*not encoded*/);

		url.GetHostName(outProxyName, false /*not encoded*/);

		if(!outProxyName.IsEmpty())
			haveProxy=true;

		VString port;
		url.GetPortNumber(port, false /*not encoded*/);

		if(!port.IsEmpty())
			outProxyPort=port.GetLong();
	}

	return haveProxy;
}


#endif // #if VERSIONMAC


template <typename T>
bool StrMatch (T* s, T* mask)
{
	T *cp = NULL;
	T *mp = NULL;

	for (; (*s) && (*mask != (T)'*'); mask++, s++)
		if ((*mask != *s) && (*mask != (T)'?'))
			return false;
	for (;;)
	{
		if (!*s)
		{
			while (*mask == (T)'*')
				mask++;
			return !*mask;
		}
		if (*mask == (T)'*')
		{
			if (!*++mask)
				return true;
			mp = mask;
			cp = s + 1;
			continue;
		}
		if ((*mask == *s) || (*mask == (T)'?'))
		{
			mask++;
			s++;
			continue;
		}
		mask = mp;
		s = cp++;
	}
}


//--------------------------------------------------------------------------------------------------


bool					VProxyManager::fUseProxy  = false;
XBOX::VString			VProxyManager::fProxyServerAddress;
PortNumber				VProxyManager::fProxyServerPort = DEFAULT_HTTP_PORT;
XBOX::VectorOfVString	VProxyManager::fProxyExceptionsList;


/* static */
bool VProxyManager::GetInfos (XBOX::VString& outProxyName, sLONG& outProxyPort, XBOX::VectorOfVString *outExceptionsList)
{
#if VERSIONMAC
	return MAC_GetHTTPProxySettings (outProxyName, outProxyPort, outExceptionsList);
#elif VERSIONWIN
	return WIN_GetHTTPProxySettings (outProxyName, outProxyPort, outExceptionsList);
#elif VERSION_LINUX
	return LINUX_GetHTTPProxySettings (outProxyName, outProxyPort, outExceptionsList);
#endif
}


/* static */
VProxyInfos *VProxyManager::CreateProxyInfos()
{
	XBOX::VString	host;
	PortNumber		port;
	GetInfos (host, port, NULL);
	return new VProxyInfos (host, port);
}


/* static */
void VProxyManager::Init()
{
	fUseProxy = GetInfos (fProxyServerAddress, fProxyServerPort, &fProxyExceptionsList);

#if VERSIONDEBUG
	XBOX::DebugMsg ("***** VProxyManager::GetProxyInfo() *****\n\tProxy: %S\n", &fProxyServerAddress);
	XBOX::DebugMsg ("\tProxy Port: %d\n", fProxyServerPort);
	XBOX::DebugMsg ("\tNum Of Exceptions: %d\n", fProxyExceptionsList.size());

	sLONG i = 0;
	for (XBOX::VectorOfVString::iterator it = fProxyExceptionsList.begin(); it != fProxyExceptionsList.end(); ++it)
	{
		XBOX::DebugMsg ("\t\tException #%d: %S\n", i++, &(*it));
	}
#endif
}


/* static */
void VProxyManager::DeInit()
{
	fProxyExceptionsList.erase (fProxyExceptionsList.begin(), fProxyExceptionsList.end());
}


/* static */
bool VProxyManager::MatchProxyException (const XBOX::VString& inDomain)
{
	if (!fProxyExceptionsList.size())
		return false;

	for (XBOX::VectorOfVString::const_iterator it = fProxyExceptionsList.begin(); it != fProxyExceptionsList.end(); ++it)
	{
		if (StrMatch (inDomain.GetCPointer(), (*it).GetCPointer()))
			return true;
	}

	return false;
}


/* static */
bool VProxyManager::ByPassProxyOnLocalhost (const XBOX::VString& inDomain)
{
	return !inDomain.IsEmpty() && (EqualASCIIVString (inDomain, "localhost") 
			                       || ServerNetTools::IsLocalInterface(inDomain)
								   || ServerNetTools::IsLoopBack(inDomain));
}


//--------------------------------------------------------------------------------------------------


XBOX::VString VProxyInfos::GetHttpProxyHost() const
{
	return fHost;
}


PortNumber VProxyInfos::GetHttpProxyPort() const
{
	return fPort;
}


void VProxyInfos::Release()
{
	delete this;
}


