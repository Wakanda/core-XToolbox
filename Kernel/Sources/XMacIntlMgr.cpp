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
#include "VTextConverter.h"
#include "VString.h"
#include "VIntlMgr.h"
#include "XMacTextConverter.h"
#include "XMacIntlMgr.h"


XMacIntlMgr::XMacIntlMgr( DialectCode inDialect)
: fDialect( inDialect)
, fCFLocaleRef( NULL)
{
}


XMacIntlMgr::XMacIntlMgr( const XMacIntlMgr& inOther)
: fDialect( inOther.fDialect)
, fCFLocaleRef( NULL)	// CFLocaleRef is supposed to be non thread safe. Don't retain it
{
}


XMacIntlMgr::~XMacIntlMgr()
{
	if (fCFLocaleRef != NULL)
		CFRelease( fCFLocaleRef);
}


/*
	static
*/
CFLocaleRef XMacIntlMgr::CreateCFLocaleRef( DialectCode inDialect)
{
	VString language;
	VIntlMgr::GetBCP47LanguageCode( inDialect, language);
	
	// Create the locale.
	CFStringRef langStr = language.MAC_RetainCFStringCopy();
	CFLocaleRef locale = ::CFLocaleCreate(NULL,langStr);
	::CFRelease( langStr);

	return locale;
}


CFLocaleRef XMacIntlMgr::GetCFLocaleRef() const
{
	if (fCFLocaleRef == NULL)
	{
		fCFLocaleRef = CreateCFLocaleRef( fDialect);
	}
	return fCFLocaleRef;
}


CFLocaleRef XMacIntlMgr::RetainCFLocaleRef() const
{
	CFLocaleRef ref = GetCFLocaleRef();
	if (ref != NULL)
		::CFRetain( ref);
	return ref;
}


bool XMacIntlMgr::ToUpperLowerCase(const UniChar* inSrc, sLONG inSrcLen, UniPtr outDst, sLONG& ioDstLen, bool inIsUpper)
{
	xbox_assert(inSrcLen >= 0); // doit etre verifie par l'appelant
	
	CFMutableStringRef mstr = CFStringCreateMutable( kCFAllocatorDefault, 0);
	
	CFStringAppendCharacters( mstr, inSrc, inSrcLen);

	if ( inIsUpper )
		CFStringUppercase(mstr, NULL);
	else
		CFStringLowercase(mstr, NULL);
	
	CFIndex len = ::CFStringGetLength( mstr );
	CFIndex stringLength = Min<CFIndex>(len, ioDstLen);
	ioDstLen = (sLONG) len;

	const UniChar*	uniPtr = ::CFStringGetCharactersPtr( mstr );

	if (uniPtr != NULL)
	{
		::memcpy( outDst, uniPtr, stringLength * sizeof(UniChar));
	}
	else
	{
		CFIndex	rangeStart = 0;
		UniChar	buffer[1024];
		do
		{
			CFIndex count = Min<CFIndex>(stringLength, sizeof(buffer));
			stringLength -= count;
			
			::CFStringGetCharacters(mstr, CFRangeMake(rangeStart, count), buffer);
			::memcpy( outDst + rangeStart, buffer, count * sizeof( UniChar) );
			
			rangeStart += count;
		}
		while( stringLength > 0);
	}
	CFRelease ( mstr );

	return (ioDstLen != 0) || (inSrcLen == 0);
}


/*
	static
*/
VToUnicodeConverter* XMacIntlMgr::NewToUnicodeConverter(CharSet inCharSet)
{
	XMacToUnicodeConverter* converter = new XMacToUnicodeConverter(inCharSet);
	if (converter != NULL && !converter->IsValid())
	{
		delete converter;
		converter = NULL;
	}
	return converter;
}


/*
	static
*/
VFromUnicodeConverter* XMacIntlMgr::NewFromUnicodeConverter(CharSet inCharSet)
{
	XMacFromUnicodeConverter* converter = new XMacFromUnicodeConverter(inCharSet);
	if (converter != NULL && !converter->IsValid())
	{
		delete converter;
		converter = NULL;
	}
	return converter;
}

