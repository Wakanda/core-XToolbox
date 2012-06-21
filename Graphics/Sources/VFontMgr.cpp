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
#include "VFont.h"
#if VERSIONWIN
#include "XWinGDIGraphicContext.h"
#endif


VFontMgr::VFontMgr()
{
}


VFontMgr::~VFontMgr()
{
	fFonts.Destroy(DESTROY);
}

	
xFontnameVector* VFontMgr::GetFontNameList(bool inWithScreenFonts)
{
	//return NULL;	//GetFontNameList(inWithScreenFonts);
	return fFontMgr.GetFontNameList(inWithScreenFonts);
}


VFont* VFontMgr::RetainFont(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI, bool inReturnNULLIfNotExist)
{
	VTaskLock lock(&fCriticalSection);
	
#if VERSIONWIN
	GReal pixelSize = 0.0f;
#endif
	if (inDPI)
	{
	//scale font size to the desired DPI
#if VERSIONWIN
#if ENABLE_D2D
#if D2D_GDI_COMPATIBLE
	if (inDPI == 72)
		pixelSize = inSize; //to avoid math discrepancies with font size in 4D form, we ensure to pass initial pixel font size to a new VFont
							//(to ensure a 4D form 12pt font will actually have a real 12px font size)
	inSize = inSize*inDPI/VWinGDIGraphicContext::GetLogPixelsY();
#else
	inSize = inSize*inDPI/96.0f;
#endif
#else
	inSize = inSize*inDPI/VWinGDIGraphicContext::GetLogPixelsY();
#endif
#else
	inSize = inSize*inDPI/72.0f;
#endif
	}

	VFont*	theFont = NULL;
	
	for(sLONG i = 0 ; i < fFonts.GetCount() ; ++i)
	{
		VFont* oneFont = fFonts[i];
		if(oneFont->Match(inFontFamilyName, inFace, inSize))
		{
			oneFont->Retain();
			theFont = oneFont;
			break;
		}
	}
	
	if(theFont == NULL)
	{
#if VERSIONWIN
		theFont = new VFont(inFontFamilyName, inFace, inSize, STDF_NONE, pixelSize);
#else
		theFont = new VFont(inFontFamilyName, inFace, inSize);
#endif
		if (inReturnNULLIfNotExist)
			if (!theFont->GetImpl().GetFontRef())
			{
				delete theFont;
				return NULL;
			}
		if(fFonts.AddTail(theFont))
		{
			theFont->Retain();
		}
	}
	
	return theFont;
}


VFont* VFontMgr::RetainStdFont(StdFont inFont)
{
	VTaskLock lock(&fCriticalSection);
	
	VFont*	theFont = NULL;
	
	for(sLONG i = 0 ; i < fFonts.GetCount() ; ++i)
	{
		VFont*	oneFont = fFonts[i];
		if(oneFont->Match(inFont))
		{
			oneFont->Retain();
			theFont = oneFont;
			break;
		}
	}
	
	if(theFont == NULL)
	{
		VStr31	name;
		VFontFace	style;
		GReal	size;
		fFontMgr.GetStdFont(inFont, name, style, size);
		theFont = RetainFont(name, style, size);
	}
	
	return theFont;
}



