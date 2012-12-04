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
#ifndef __VIntlMgr__
#define __VIntlMgr__

#include <list> 

#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/VTextConverter.h"
#include "Kernel/Sources/VFolder.h"
#include "Kernel/Sources/VBlob.h"

class IMecabModel;

BEGIN_TOOLBOX_NAMESPACE

enum RFC3066CodeCompareResult
{
	RFC3066_CODES_NOT_VALID = -1,
	RFC3066_CODES_ARE_NOT_EQUAL = 0, /**< examples: en <-> it, sv-fi <-> ro */
	RFC3066_CODES_LANGUAGE_REGION_VARIANT, /**< example: en-us <-> en-gb */
	RFC3066_CODES_GLOBAL_LANGUAGE_AND_SPECIFIC_LANGUAGE_REGION_VARIANT, /**< example: en and en-au */
	RFC3066_CODES_SPECIFIC_LANGUAGE_REGION_AND_GLOBAL_LANGUAGE_VARIANT, /**< example: en-au and en */
	RFC3066_CODES_ARE_EQUAL
};

enum EOSFormats
{
	eOS_SHORT_FORMAT=0,
	eOS_MEDIUM_FORMAT=1,
	eOS_LONG_FORMAT=2
};

enum EMonthDayList
{
	eDayOfWeekList=0,
	eMonthList,
	eAbbreviatedDayOfWeekList,
	eAbbreviatedMonthList
};

enum EFindStringMode
{
	eFindWholeWords = 3,	// the match must not begin nor end in the middle of a word
	eFindContains = 4,		// the match can be anywhere
	eFindStartsWith = 5,	// the match must not begin in the middle of a word
	eFindEndsWith = 6		// the match must not end in the middle of a word
};

enum
{
	eKW_ConsiderOnlyDeadChars	= 1,
	eKW_UseICU					= 2,
	eKW_UseMeCab				= 4,
	eKW_UseLineBreakingRule		= 8
};
typedef uLONG	EKeyWordOptions;

END_TOOLBOX_NAMESPACE

#if VERSIONMAC
#include "Kernel/Sources/XMacIntlMgr.h"
#elif VERSION_LINUX
#include "Kernel/Sources/XLinuxIntlMgr.h"
#elif VERSIONWIN
#include "Kernel/Sources/XWinIntlMgr.h"
#endif

#if USE_ICU

namespace xbox_icu
{
	class Locale;
	class Transliterator;
	class BreakIterator;
}
#endif

#if USE_MECAB

namespace MeCab
{
    class Lattice;
};
#endif

BEGIN_TOOLBOX_NAMESPACE

#define DC_MAKEDIALECT(p, s)      ((((uWORD  )(s)) << 10) | (uWORD  )(p))
#define DC_PRIMARYLANGID(lgid)    ((uWORD  )(lgid) & 0x3ff)
#define DC_SUBLANGID(lgid)        ((uWORD  )(lgid) >> 10)

// Needed declarations
class VArrayLong;
class VArrayString;
class VIntlMgr;
class VCollator;

typedef std::vector<std::pair<VString,DialectCode> >	VectorOfNamedDialect;
typedef std::vector<std::pair<VIndex,VIndex> >	VectorOfStringSlice;

typedef std::vector<DialectCode> VectorOfDialect;

class XTOOLBOX_API VIntlMgr : public VObject, public IRefCountable
{
public:
	virtual	~VIntlMgr ();

	static	VIntlMgr*			Create( DialectCode inDialect, DialectCode inDialectForCollator, CollatorOptions inCollatorOptions);
	static	VIntlMgr*			Create( DialectCode inDialect, CollatorOptions inCollatorOptions = COL_ICU) { return Create( inDialect, inDialect, inCollatorOptions);}
			
			/** @brief the only thread-safe method **/
								VIntlMgr*		Clone();

			// Collation info
			bool				IsUsingICUCollator()	{ return fUseICUCollator;}

	// String comparision
	
			CompareResult		CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);
			CompareResult		CompareString_Like (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);
			
			CompareResult		CompareString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, const VCompareOptions& inOptions);

			bool				EqualString (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);
			bool				EqualString_Like (const UniChar* inText1, sLONG inSize1, const UniChar* inText2, sLONG inSize2, bool inWithDiacritics);

			void				ToUpperLowerCase( VString& ioText, bool inStripDiac, bool inIsUpper);
			
			VCollator*			GetCollator() const			{ return fCollator;}
			UniChar				GetWildChar() const;
	
	// Char analyse utilities
			bool				IsAlpha( UniChar inChar) const;
			bool				IsDigit( UniChar inChar) const;
			bool				IsSpace( UniChar inChar) const;
			bool				IsPunctuation( UniChar inChar) const;

			void				NormalizeString( VString& ioString, NormalizationMode inMode);
	
	// returns start (1-based) and length of each words
			bool				GetWordBoundariesWithOptions( const VString& inText, VectorOfStringSlice& outBoundaries, EKeyWordOptions inOptions);
			bool				GetWordBoundaries( const VString& inText, VectorOfStringSlice& outBoundaries)	{ return GetWordBoundariesWithOptions( inText, outBoundaries, fKeyWordOptions);}
			bool				FindWord( const VString& inText, const VString& inPattern, const VCompareOptions& inOptions);
	
	// Find some pattern in given text accordingly to common text editors rules.
	// outMatches contains the start (1-based) and the length of each match.
			void				FindStrings( const VString& inText, const VString& inPattern, EFindStringMode inOperator, bool inWithDiacritics, VectorOfStringSlice& outMatches);

	// Standalone version - may be called at anytime in debug mode
	static	bool				DebugConvertString (void* inSrcText, sLONG inSrcBytes, CharSet inSrcCharSet, void* inDstText, sLONG& inDstMaxBytes, CharSet inDstCharSet);

			void				GetDialectsHavingCollator( VectorOfNamedDialect& outCollatorDisplayNames);

	static	void				GetAllDialects( VectorOfDialect& outDialects);

	/** @brief DialectCode -> ISO639-1 Language Code (2 letters).*/
	static	bool				GetISO6391LanguageCode( DialectCode inDialect, VString& outLanguageCode );
	static	const char*			GetISO6391LanguageCode( DialectCode inDialect);
	
	/** @brief DialectCode -> ISO639-2 Language Code (3 letters).*/
	static	bool				GetISO6392LanguageCode( DialectCode inDialect, VString& outLanguageCode );
	
	/** @brief DialectCode -> ISO3166 Region Code.*/
	static	bool				GetISO3166RegionCode( DialectCode inDialect, VString& outRegionCode );
	static	const char*			GetISO3166RegionCode( DialectCode inDialect);
	
	/** @brief DialectCode -> RFC3066Bis Subtag Code. This code can be different from an ISO3166 code, because RF3066Bis accepts exceptions ( http://www.iana.org/assignments/language-tags )  */
	static	bool				GetRFC3066BisSubTagRegionCode( DialectCode inDialect, VString& outRFC3066SubTag );
	
	/** @brief DialectCode -> ISO Language Name.*/
	static	bool				GetISOLanguageName( DialectCode inDialect, VString& outLanguageName );
	
	/** @brief DialectCode -> language name in user-friendly localized text. Needs ICU */
			bool				GetLocalizedLanguageName( DialectCode inDialect, VString& outLanguageName);

	/** @brief DialectCode -> RFC3066Bis Language Code.*/
	static	bool				GetRFC3066BisLanguageCodeWithDialectCode( DialectCode inDialectCode, VString& outRFC3066LanguageCode,char inSeparator='-');
	
	/** @brief RFC3066Bis Language Code -> DialectCode (case unsensitive).*/
	static	bool				GetDialectCodeWithRFC3066BisLanguageCode( const char *inISO6391Code, const char *inRFC3066SubTag, DialectCode& outDialectCode, bool searchForISO6391CodeIfNothingFound, bool calculateDialectIfNoSubTag = true);
	static	bool				GetDialectCodeWithRFC3066BisLanguageCode( const VString& inRFC3066BisLanguageCode, DialectCode& outDialectCode, bool searchForISO6391CodeIfNothingFound = true, bool calculateDialectIfNoSubTag = true);

	/** BCP47 combines ISO639 language code with ISO3166 region code separated with "-" **/
	/** examples are fr-FR de-de ja-JP **/
	/** CFLocale on mac uses BCP47 **/
	static	bool				GetBCP47LanguageCode( DialectCode inDialect, VString& outLanguageCode);

	/** @brief ISO Language Name -> DialectCode.*/
	static	bool				GetDialectCodeWithISOLanguageName(const VString& inISOLanguageName, DialectCode& outDialectCode);

	static	bool				GetLocalizationFoldersWithDialect( DialectCode inDialectCode, const VFilePath & inLocalizationFoldersPath, VectorOfVFolder& outRetainedLocalizationFoldersList);

	/** @brief looks for a localization file in specified dialect using GetLocalizationFoldersWithDialect */
	static	VFile*				RetainLocalizationFile( const VFilePath& inResourcesFolderPath, const VString& inFileName, DialectCode inPreferredDialect, bool *outAtLeastOneFolderScaned = NULL);

	/** @brief looks if the dialect is found in sDialectsList */
	static	bool				IsDialectCode( DialectCode inDialect);
	
	// tells if primary language of given dialect is a right to left
	static	bool				IsRightToLeftDialect( DialectCode inDialect);

	/**
	* @brief Make a comparison between two RFC3066 language codes.
	* The comparison is made in the order first -> second, i.e. a result like RFC3066_CODES_GLOBAL_LANGUAGE_AND_SPECIFIC_LANGUAGE_REGION_VARIANT means the first language code is the global code of the second one.
	* @param firstRFC3066Code A valid and supported RFC-3066 language code
	* @param secondRFC3066Code A valid and supported RFC-3066 language code
	* @return A comparison result. If at least one language code is not supported, a RFC3066_CODES_NOT_VALID result is returned.
	*/
	static	RFC3066CodeCompareResult	Compare2RFC3066LanguageCodes(const VString & inFirstRFC3066Code, const VString & inSecondRFC3066Code);


#if VERSIONMAC
			CFLocaleRef			MAC_RetainCFLocale()	{ return fImpl.RetainCFLocaleRef(); }
	static	DialectCode			MAC_LanguageCodeToDialectCode( sLONG inLanguageCode);
#endif	// VERSIONMAC

#if VERSIONWIN && VERSIONDEBUG
			void				WIN_BuildDialectTMPL ();
			void				WIN_BuildFilesFromResources ();
#endif	// VERSIONWIN
	
	// Class utilities
	static	void				GetBoolString( VString& outString, bool inValue);
	
	static	bool				IsAMacCharacter( uBYTE inChar, uBYTE inCharSet);
	static	bool				IsUsingAmPm()			{ return GetDefaultMgr()->fUsingAmPm; }
	
	static	const VString&		GetAMString()			{ return GetDefaultMgr()->fAMString; }
	static	const VString&		GetPMString()			{ return GetDefaultMgr()->fPMString; }
	
	static	DateOrder			GetDateOrder()			{ return GetDefaultMgr()->fDateOrder; }

	static	const VString&		GetDateSeparator()		{ return GetDefaultMgr()->fDateSeparator; }
	static	const VString&		GetTimeSeparator()		{ return GetDefaultMgr()->fTimeSeparator; }

			const VString&		GetThousandsSeparator() { return fThousandsSeparator; }
			const VString&		GetDecimalSeparator()	{ return fDecimalSeparator; }
			const VString&		GetCurrency()			{ return fCurrency;}
	
	static	const VString&		GetLongDatePattern()	{ return GetDefaultMgr()->fLongDatePattern; }
	static	const VString&		GetMediumDatePattern()	{ return GetDefaultMgr()->fMediumDatePattern; }
	static	const VString&		GetShortDatePattern()	{ return GetDefaultMgr()->fShortDatePattern; }
	static	const VString&		GetLongTimePattern()	{ return GetDefaultMgr()->fLongTimePattern; }
	static	const VString&		GetMediumTimePattern()	{ return GetDefaultMgr()->fMediumTimePattern; }
	static	const VString&		GetShortTimePattern()	{ return GetDefaultMgr()->fShortTimePattern; }
	
	static	void				GetMonthDayNames( EMonthDayList inListType,VArrayString* outArray);

			void				FormatDateByOS( const VTime& inDate,VString& outDate,EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay);
			void				FormatTimeByOS( const VTime& inDate,VString& outDate,EOSFormats inFormat, bool inUseGMTTimeZoneForDisplay);
			void				FormatDurationByOS( const VDuration& inDuration,VString& outTime,EOSFormats inFormat);
			void				FormatNumberByOS( const VValueSingle& inValue,VString& outNumber);

			DialectCode			GetDialectCode() const	{ return fDialect;}
	static	DialectCode			GetCurrentDialectCode()	{ VIntlMgr *cur = GetDefaultMgr(); if (cur != NULL) return cur->GetDialectCode(); else return DC_ENGLISH_US; }
	
	static	DialectCode			GetSystemUIDialectCode();
	
	static	VIntlMgr*			GetDefaultMgr ()		{ return VTask::GetCurrentIntlManager();}
	
	static	MultiByteCode		DialectCodeToScriptCode( DialectCode inDialect);
	
			const XIntlMgrImpl&	GetImpl () const		{ return fImpl; }
			XIntlMgrImpl&		GetImpl ()				{ return fImpl; }


	static	bool				ICU_Available()			{ return sICU_Available;}

			// restore default settings
			void				Reset();
	
			// resolve DC_USER DC_SYSTEM DC_NONE dialect codes
	static	DialectCode			ResolveDialectCode( DialectCode inDialect);
		
private:

									VIntlMgr( DialectCode inDialect, EKeyWordOptions inKeyWordOptions);
									VIntlMgr( const VIntlMgr& inMgr);
									VIntlMgr& operator = (const VIntlMgr&);
									
	static	void					_InitICU();
	static	bool					sICU_Available;

	static	void					_InitMeCab();
	static	IMecabModel*			_GetMecabModel();
	static	bool					sMeCab_Available;
			
			#if USE_MECAB
	static	IMecabModel*			sMecabModel;
			MeCab::Lattice*			fMecabLattice;
			#endif
	
			XIntlMgrImpl			fImpl;
			
			bool					fUseICUCollator;
			EKeyWordOptions			fKeyWordOptions;
			VCollator*				fCollator;
			DialectCode				fDialect;
			VString					fAMString;
			VString					fPMString;
			bool					fUsingAmPm;
			VString					fTrue;
			VString					fFalse;

			VString					fDecimalSeparator;
			VString					fThousandsSeparator;
			VString					fDateSeparator;
			VString					fTimeSeparator;
			VString					fCurrency;

			VString					fShortDatePattern;
			VString					fMediumDatePattern;
			VString					fLongDatePattern;

			VString					fShortTimePattern;
			VString					fMediumTimePattern;
			VString					fLongTimePattern;

			DateOrder				fDateOrder;

			#if USE_ICU
			xbox_icu::Locale*			fLocale;
			xbox_icu::Transliterator*	fUpperStripDiacriticsTransformation;
			xbox_icu::Transliterator*	fLowerStripDiacriticsTransformation;
			xbox_icu::BreakIterator*	fBreakIterator;
			xbox_icu::BreakIterator*	fBreakLineIterator;
			
			xbox_icu::Locale*			_GetICUCollatorLocale();
			#endif
	
	// Private utils
			bool					_Init ();
			bool					_InitLocaleVariables ();
			void					_InitSortTables ();
			bool					_InitCollator( DialectCode inDialectForCollator, CollatorOptions inCollatorOptions);
	
			bool					_CharTest (UniChar inChar, const UniChar* inRangesArray) const;
			
			void					_FillRangesArrays ();
			UniChar*				_FillOneRangesArray (UniChar* inArray,uBYTE inMatchFlag);

			bool					_IsItaDeadChar( UniChar c);
			bool					_HasApostropheSecondFollowedByVowel( const VString& inText, VIndex inStart, VIndex inLength);
};


#if VERSIONWIN && USE_MAC_RESFILES
void	WIN_BuildUnicodeResources ();
#endif


END_TOOLBOX_NAMESPACE

#endif
