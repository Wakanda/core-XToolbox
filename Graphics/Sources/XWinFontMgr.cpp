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
#include "XWinGDIGraphicContext.h"


#if USE_GDIPLUS
using namespace Gdiplus;
#endif

#include "richedit.h"


XWinFontMgr::XWinFontMgr()
{
	// TBD
	BuildFontList();
}


XWinFontMgr::~XWinFontMgr()
{
	// TBD
}


void XWinFontMgr::BuildFontList()
{
	fFontFamilies.SetCount(0);
	//fFontNames.SetCount(0);
	fFontNames.clear();
	// TBD
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));
	lf.lfCharSet = DEFAULT_CHARSET;

	HDC hDC = GetDC(NULL);
	::EnumFontFamiliesEx(hDC, &lf,
				(FONTENUMPROC) EnumFamScreenCallBackEx, (LPARAM) this, NULL);

	ReleaseDC(NULL, hDC);

	std::sort(fFontNames.begin(),fFontNames.end());
	//fFontNames.Sort(0,fFontNames.GetCount()-1);
}

BOOL CALLBACK XWinFontMgr::EnumFamScreenCallBackEx(ENUMLOGFONTEX* pelf, NEWTEXTMETRICEX* /*lpntm*/, int FontType,LPVOID pThis)
{
	XBOX::VString fontName;
	fontName.FromCString((const char*)(pelf->elfFullName));
	((XWinFontMgr *)pThis)->AddFont(fontName);
	
	return 1;
}

void XWinFontMgr::AddFont(const XBOX::VString& inFontName)
{
	XBOX::VString vfontname;
	/*fFontNames.GetString(vfontname,fFontNames.GetCount());
	if(!(vfontname == inFontName))
		fFontNames.AppendString(inFontName);*/
	if( std::find(fFontNames.begin(), fFontNames.end(), inFontName) == fFontNames.end())
		fFontNames.push_back(inFontName);
}

xFontnameVector* XWinFontMgr::GetFontNameList(bool inWithScreenFonts)
{
	// TBD

	return &fFontNames;
}


void XWinFontMgr::GetStdFont(StdFont inFont, VString& outName, VFontFace& outFace, GReal& outSize)
{
	HFONT		hFont=NULL;
	LOGFONTW*	winFont = NULL;
	LOGFONTW	logFont;

	NONCLIENTMETRICSW	metrics;
	metrics.cbSize = sizeof(NONCLIENTMETRICSW);
	
	switch(inFont) 
	{
		case STDF_APPLICATION:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_LIST:
		case STDF_ICON:
			if (::SystemParametersInfoW(SPI_GETICONTITLELOGFONT, 0,(void*)&logFont,0))
				winFont = &logFont;
			break;
		case STDF_MENU:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMenuFont;
			break;
		case STDF_MENU_MINI:
		case STDF_TIP:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfStatusFont;
			break;
		case STDF_CAPTION:
		case STDF_SYSTEM_MINI:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_SYSTEM_SMALL:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_EDIT:
		case STDF_TEXT:
		case STDF_SYSTEM:	
		
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfMessageFont;
			break;
		case STDF_WINDOW_CAPTION:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfCaptionFont;
				break;
		case STDF_WINDOW_SMALL_CAPTION:
			if (::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 0,(void*)&metrics,0))
				winFont = &metrics.lfSmCaptionFont;
				break;
		default:
			assert(false);
			break;
	}
	
	if (winFont != NULL)
	{
		outName.FromUniCString(winFont->lfFaceName);
		outFace = (winFont->lfWeight > 500) ? KFS_BOLD : KFS_NORMAL;
		outFace |= (winFont->lfItalic) ? KFS_ITALIC : KFS_NORMAL;
		outSize = PixelsToPoints(Abs(winFont->lfHeight));
	}
}

