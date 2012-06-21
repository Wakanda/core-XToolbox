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
#include "VGraphicsPrecompiled.h"
#include "VFontMgr.h"
#include "XMacFont.h"


XMacFontMgr::XMacFontMgr()
{
	BuildFontList();
}


XMacFontMgr::~XMacFontMgr()
{
}


void XMacFontMgr::BuildFontList()
{
	fFontFamilies.SetCount(0);
	//fFontNames.SetCount(0);
	fFontNames.clear();
	/*
	FMFontFamilyIterator	iterator;
	OSErr	error = ::FMCreateFontFamilyIterator(NULL, NULL, kFMDefaultOptions, &iterator);
	if (error == noErr)
	{
		FMFontFamily	family;
		
		assert(sizeof(FMFontFamily) == sizeof(sWORD));
		while (::FMGetNextFontFamily(&iterator, &family) == noErr)
		{
			Str255 spFontName;
			if (testAssert(::FMGetFontFamilyName(family, spFontName) == noErr))
			{
				VStr255	name;
				CFStringRef cfname;
				ATSFontFamilyRef ffref = ::ATSFontFamilyFindFromQuickDrawName(spFontName);
				::ATSFontFamilyGetName(ffref, kATSOptionFlagsDefault, &cfname);
				//name.MAC_FromMacPString(spFontName);
				name.MAC_FromCFString(cfname);
				if(name[0] != '.' && name[0] != '%')
				{	
					fFontNames.AppendString(name);
					fFontFamilies.AppendWord((sWORD) family);
				}
				//CFRelease(cfname);
			}
		}
		
		::FMDisposeFontFamilyIterator(&iterator);
	}*/
	
	CTFontCollectionRef FontCollection = ::CTFontCollectionCreateFromAvailableFonts(0);//kCTFontCollectionRemoveDuplicatesOption
	if(FontCollection)
    {
		CFArrayRef fonts = CTFontCollectionCreateMatchingFontDescriptors(FontCollection);
		if(fonts)
		{
			const int numFonts = CFArrayGetCount(fonts);
			for(int i = 0; i < numFonts; ++i) 
			{
				VString	vname;
				CTFontDescriptorRef vfont = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fonts, i);
				CFStringRef family_name = (CFStringRef)CTFontDescriptorCopyAttribute(vfont, kCTFontFamilyNameAttribute);
				if(family_name!=NULL)
				{
					vname.MAC_FromCFString(family_name);
				
					if( std::find(fFontNames.begin(), fFontNames.end(), vname) == fFontNames.end())
						fFontNames.push_back(vname);
				}
				//CFRelease(family_name);
			}
		}
	}	
	
	//fFontNames.SynchronizedSort(false, &fFontFamilies);
	//fFontNames.Sort(0, fFontNames.GetCount()-1);
	std::sort(fFontNames.begin(),fFontNames.end());
}

xFontnameVector* XMacFontMgr::GetFontNameList(bool inWithScreenFonts)
{
	return &fFontNames;
}

void XMacFontMgr::GetStdFont( StdFont inFont, VString& outName, VFontFace& outFace, GReal& outSize)
{
	// on risque de perdre des infos, il faudrait retourner une VFont
	
	CTFontUIFontType fontType = StdFontToThemeFontID( inFont);
	CTFontRef fontRef = ::CTFontCreateUIFontForLanguage( fontType, 0 /*size*/, NULL /*language*/ );

	if (testAssert( fontRef != NULL))
	{
		outSize = ::CTFontGetSize( fontRef);

		CFStringRef cfName = ::CTFontCopyFamilyName( fontRef);
		if (cfName != NULL)
		{
			outName.MAC_FromCFString( cfName);
			::CFRelease( cfName);
		}
		else
		{
			outName.Clear();
		}
		
		CTFontSymbolicTraits traits = ::CTFontGetSymbolicTraits( fontRef);
		outFace = 0;
		
		if (traits & kCTFontItalicTrait)
			outFace |= KFS_ITALIC;
		
		if (traits & kCTFontBoldTrait)
			outFace |= KFS_BOLD;
		
		if (traits & kCTFontExpandedTrait)
			outFace |= KFS_EXTENDED;
		
		if (traits & kCTFontCondensedTrait)
			outFace |= KFS_CONDENSED;

		CFRelease( fontRef); 
	}
	else
	{
		outName.Clear();
		outSize = 0;
		outFace = 0;
	}
}


CTFontUIFontType XMacFontMgr::StdFontToThemeFontID(StdFont inFont)
{
	CTFontUIFontType	macFont;
	switch (inFont)
	{
		case STDF_TEXT:
			macFont = kCTFontUserFontType;
			break;
			
		case STDF_MENU:
			macFont = kCTFontMenuItemFontType;
			break;
			
		case STDF_MENU_MINI:
			macFont = kCTFontSmallEmphasizedSystemFontType;
			break;
			
		case STDF_SYSTEM:
			macFont = kCTFontSystemFontType;
			break;
		case STDF_APPLICATION:
			macFont = kCTFontApplicationFontType;
			break;	
		case STDF_CAPTION:
			macFont = kCTFontLabelFontType;
			break;
		
		case STDF_SYSTEM_SMALL:
			macFont = kCTFontSmallSystemFontType;
			break;
			
		case STDF_SYSTEM_MINI:
			macFont = kCTFontMiniSystemFontType;
			break;
			
		case STDF_PUSH_BUTTON:
			macFont = kCTFontPushButtonFontType;
			break;
			
		case STDF_ICON:
			macFont = kCTFontToolbarFontType;
			break;
			
		case STDF_EDIT:
		case STDF_LIST:
			macFont = kCTFontViewsFontType;
			break;
			
		case STDF_TIP:
			macFont = kCTFontToolTipFontType;
			break;
			
		case STDF_WINDOW_SMALL_CAPTION:
			macFont = kCTFontUtilityWindowTitleFontType;
			break;
			
		case STDF_WINDOW_CAPTION:
			macFont = kCTFontWindowTitleFontType;
			break;
			
		case STDF_NONE:
			macFont = kCTFontSystemFontType;
			break;
			
		default:
			xbox_assert(false);
			macFont = kCTFontSystemFontType;
			break;
	}
	
	return macFont;
}