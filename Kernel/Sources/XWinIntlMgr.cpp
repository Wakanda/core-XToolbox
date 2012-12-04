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
#include "VKernelPrecompiled.h"
#include "XWinIntlMgr.h"
#include "XWinTextConverter.h"

#include <mlang.h>

XWinIntlMgr::XWinIntlMgr( DialectCode inDialect)
: fDialect( inDialect)
{
}


XWinIntlMgr::XWinIntlMgr( const XWinIntlMgr& inOther)
: fDialect( inOther.fDialect)
{
}


XWinIntlMgr::~XWinIntlMgr()
{
}


bool XWinIntlMgr::ToUpperLowerCase(const UniChar *inSrc, sLONG inSrcLen, UniPtr outDst, sLONG& ioDstLen, bool inIsUpper)
{
	xbox_assert(inSrcLen > 0); // doit etre verifie par l'appelant
	
	DWORD flags = LCMAP_LINGUISTIC_CASING + (inIsUpper ? LCMAP_UPPERCASE : LCMAP_LOWERCASE);	// it'd be simpler to use CharUpper, but there's at least a chance that LCMapString will properly handle characters like LATIN SMALL LETTER SHARP S
	
	DWORD count = ::LCMapStringW(LOCALE_USER_DEFAULT /*locale*/, flags, (LPCWSTR) inSrc, inSrcLen, (LPWSTR) outDst, ioDstLen);
	xbox_assert(count > 0);
	ioDstLen = count;
	return count > 0;
}


/*
	static
*/
IMultiLanguage2 *XWinIntlMgr::RetainMultiLanguage()
{
	IMultiLanguage2 *multilanguage = NULL;
	HRESULT hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void**) &multilanguage);
	if (hr == CO_E_NOTINITIALIZED)
	{
		hr = ::OleInitialize(NULL);	// J.A. : COINIT_MULTITHREADED doesn't work with OLE objects
		hr = ::CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2, (void**) &multilanguage);
	}
	
	return multilanguage;
}


/*
	static
*/
VToUnicodeConverter* XWinIntlMgr::NewToUnicodeConverter(CharSet inCharSet)
{
	XWinToUnicodeConverter* converter = NULL;
	IMultiLanguage2* multiLanguage = RetainMultiLanguage();
	if (multiLanguage != NULL)
	{
		converter = new XWinToUnicodeConverter( multiLanguage, inCharSet);
		if (converter != NULL && !converter->IsValid())
		{
			delete converter;
			converter = NULL;
		}
		multiLanguage->Release();
	}
	return converter;
}


/*
	static
*/
VFromUnicodeConverter* XWinIntlMgr::NewFromUnicodeConverter(CharSet inCharSet)
{
	XWinFromUnicodeConverter* converter = NULL;
	IMultiLanguage2* multiLanguage = RetainMultiLanguage();
	if (multiLanguage != NULL)
	{
		converter = new XWinFromUnicodeConverter( multiLanguage, inCharSet);
		if (converter != NULL && !converter->IsValid())
		{
			delete converter;
			converter = NULL;
		}
		multiLanguage->Release();
	}
	return converter;
}


