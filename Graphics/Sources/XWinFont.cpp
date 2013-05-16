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
#include "VFont.h"
#include "XWinGDIGraphicContext.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#endif


#include "richedit.h"



XWinFont::XWinFont (VFont *inFont, const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize):fFont( inFont)
{
	fIsTrueType = false;
#if GRAPHIC_MIXED_GDIPLUS_D2D
	fGDIPlusFont = NULL;
#endif
#if ENABLE_D2D
	fDWriteFont = NULL;
	fDWriteTextFormat = NULL;
	fCachedDWriteTextLayout = NULL;
	fDWriteEllipsisTrimmingSign = NULL;
#endif
	fFontRef = _CreateFontRef(inFontFamilyName, inFace, inSize);
	HDC hDC=CreateCompatibleDC(NULL);
	fIsTrueType = _IsFontTrueType(hDC, fFontRef);
    DeleteDC( hDC );
}


XWinFont::~XWinFont()
{
	::DeleteObject(fFontRef);
#if GRAPHIC_MIXED_GDIPLUS_D2D
	if (fGDIPlusFont)
		delete fGDIPlusFont;
#endif
#if ENABLE_D2D
	if (VWinD2DGraphicContext::IsAvailable())
	{
		VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());
		if (fDWriteFont)
		{
			fDWriteFont->Release();
			fDWriteFont = NULL;
		}
		if (fDWriteTextFormat)
		{
			fDWriteTextFormat->Release();
			fDWriteTextFormat = NULL;
		}
		if (fCachedDWriteTextLayout)
		{
			fCachedDWriteTextLayout->Release();
			fCachedDWriteTextLayout = NULL;
		}
		if (fDWriteEllipsisTrimmingSign)
		{
			fDWriteEllipsisTrimmingSign->Release();
			fDWriteEllipsisTrimmingSign = NULL;
		}
	}
#endif
}

bool XWinFont::_IsFontTrueType(PortRef inPort, FontRef inFontRef)
{
    TEXTMETRICW tm;
	HGDIOBJ	previous = ::SelectObject(inPort, inFontRef);
    GetTextMetricsW(inPort, &tm);
    bool isFontTrueType = (tm.tmPitchAndFamily & TMPF_TRUETYPE) != 0;
	::SelectObject(inPort, previous);
	return isFontTrueType;
}

#if ENABLE_D2D

void XWinFont::_GetStyleFromFontName(const VString &inFontFullName, VString &outFontFamilyName, int& outStyle, int& outWeight, int& outStretch) const
{
	bool hasITC = false;
	outFontFamilyName = inFontFullName;
	VIndex pos;

	do
	{
		pos = outFontFamilyName.FindUniChar(' ',outFontFamilyName.GetLength(),true);
		if (pos >= 1)
		{
			//extract style
			VString styleName;
			outFontFamilyName.GetSubString( pos+1, outFontFamilyName.GetLength()-pos, styleName);
			int weight;
			int style;
			int stretch;
			if (styleName.EqualToString( "ITC"))
			{
				outFontFamilyName.Truncate(pos-1);
				hasITC = true;
			}
			else if (_GetFontWeightFromStyleName( styleName, weight))
			{
				outWeight = weight;
				outFontFamilyName.Truncate(pos-1);

				if (outWeight == DWRITE_FONT_WEIGHT_LIGHT || outWeight == DWRITE_FONT_WEIGHT_BOLD || outWeight == DWRITE_FONT_WEIGHT_BLACK)
				{
					pos = outFontFamilyName.FindUniChar(' ',outFontFamilyName.GetLength(),true);
					if (pos >= 1)
					{
						VString styleName;
						outFontFamilyName.GetSubString( pos+1, outFontFamilyName.GetLength()-pos, styleName);
						if (outWeight == DWRITE_FONT_WEIGHT_LIGHT)
						{
							if (styleName.EqualToString("extra") || styleName.EqualToString("ext"))
							{
								outWeight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
								outFontFamilyName.Truncate(pos-1);
							}
							else if (styleName.EqualToString("ultra") || styleName.EqualToString("ult"))
							{
								outWeight = DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
								outFontFamilyName.Truncate(pos-1);
							}
						}
						else if (outWeight == DWRITE_FONT_WEIGHT_BOLD)
						{
							if (styleName.EqualToString("extra") || styleName.EqualToString("ext"))
							{
								outWeight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
								outFontFamilyName.Truncate(pos-1);
							}
							else if (styleName.EqualToString("ultra") || styleName.EqualToString("ult"))
							{
								outWeight = DWRITE_FONT_WEIGHT_ULTRA_BOLD;
								outFontFamilyName.Truncate(pos-1);
							}
							else if (styleName.EqualToString("demi"))
							{
								outWeight = DWRITE_FONT_WEIGHT_DEMI_BOLD;
								outFontFamilyName.Truncate(pos-1);
							}
							else if (styleName.EqualToString("semi"))
							{
								outWeight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
								outFontFamilyName.Truncate(pos-1);
							}
						}
						else //DWRITE_FONT_WEIGHT_BLACK
						{
							if (styleName.EqualToString("extra") || styleName.EqualToString("ext"))
							{
								outWeight = DWRITE_FONT_WEIGHT_EXTRA_BLACK;
								outFontFamilyName.Truncate(pos-1);
							}
							else if (styleName.EqualToString("ultra") || styleName.EqualToString("ult"))
							{
								outWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
								outFontFamilyName.Truncate(pos-1);
							}
						}
					}
				}
			}
			else if (_GetFontStyleFromStyleName( styleName, style)) 
			{
				outStyle = style;
				outFontFamilyName.Truncate(pos-1);
			}
			else if (_GetFontStretchFromStyleName( styleName, stretch))
			{
				outStretch = stretch;
				outFontFamilyName.Truncate(pos-1);

				pos = outFontFamilyName.FindUniChar(' ',outFontFamilyName.GetLength(),true);
				if (pos >= 1)
				{
					VString styleName;
					outFontFamilyName.GetSubString( pos+1, outFontFamilyName.GetLength()-pos, styleName);
					if (outStretch == DWRITE_FONT_STRETCH_CONDENSED)
					{
						if (styleName.EqualToString("extra") || styleName.EqualToString("ext"))
						{
							outStretch = DWRITE_FONT_STRETCH_EXTRA_CONDENSED;
							outFontFamilyName.Truncate(pos-1);
						}
						else if (styleName.EqualToString("ultra") || styleName.EqualToString("ult"))
						{
							outStretch = DWRITE_FONT_STRETCH_ULTRA_CONDENSED;
							outFontFamilyName.Truncate(pos-1);
						}
						else if (styleName.EqualToString("semi"))
						{
							outStretch = DWRITE_FONT_STRETCH_SEMI_CONDENSED;
							outFontFamilyName.Truncate(pos-1);
						}
					}
					else //DWRITE_FONT_STRETCH_EXPANDED
					{
						if (styleName.EqualToString("extra") || styleName.EqualToString("ext"))
						{
							outStretch = DWRITE_FONT_STRETCH_EXTRA_EXPANDED;
							outFontFamilyName.Truncate(pos-1);
						}
						else if (styleName.EqualToString("ultra") || styleName.EqualToString("ult"))
						{
							outStretch = DWRITE_FONT_STRETCH_ULTRA_EXPANDED;
							outFontFamilyName.Truncate(pos-1);
						}
						else if (styleName.EqualToString("semi"))
						{
							outStretch = DWRITE_FONT_STRETCH_SEMI_EXPANDED;
							outFontFamilyName.Truncate(pos-1);
						}
					}
				}
			}
			else
				pos = 0;
		}
	} while (pos >= 1);
	if (hasITC)
		outFontFamilyName += " ITC";
}

bool XWinFont::_GetFontWeightFromStyleName( const VString& inStyleName, int& outFontWeight) const
{
	if (inStyleName.EqualToString( "regular"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_REGULAR;
		return true;
	}
	else if (inStyleName.EqualToString( "light"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_LIGHT;
		return true;
	}
	else if (inStyleName.EqualToString( "ExtraLight"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
		return true;
	}
	else if (inStyleName.EqualToString( "UltraLight"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_ULTRA_LIGHT;
		return true;
	}
	else if (inStyleName.EqualToString( "bold"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_BOLD;
		return true;
	}
	else if (inStyleName.EqualToString( "SemiBold") || inStyleName.EqualToString( "Semi"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_SEMI_BOLD;
		return true;
	}
	else if (inStyleName.EqualToString( "DemiBold") || inStyleName.EqualToString( "Demi"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_DEMI_BOLD;
		return true;
	}
	else if (inStyleName.EqualToString( "ExtraBold") || inStyleName.EqualToString( "Extra"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_EXTRA_BOLD;
		return true;
	}
	else if (inStyleName.EqualToString( "UltraBold") || inStyleName.EqualToString( "Ultra"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_ULTRA_BOLD;
		return true;
	}
	else if (inStyleName.EqualToString( "Medium"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_MEDIUM;
		return true;
	}
	else if (inStyleName.EqualToString( "Thin"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_THIN;
		return true;
	}
	else if (inStyleName.EqualToString( "Black"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_BLACK;
		return true;
	}
	else if (inStyleName.EqualToString( "ExtraBlack"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_EXTRA_BLACK;
		return true;
	}
	else if (inStyleName.EqualToString( "UltraBlack"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_ULTRA_BLACK;
		return true;
	}
	else if (inStyleName.EqualToString( "Heavy"))
	{
		outFontWeight = DWRITE_FONT_WEIGHT_HEAVY;
		return true;
	}
	return false;
}

bool XWinFont::_GetFontStyleFromStyleName( const VString& inStyleName, int& outFontStyle) const
{
	if (inStyleName.EqualToString( "italic"))
	{
		outFontStyle = DWRITE_FONT_STYLE_ITALIC;
		return true;
	}
	else if (inStyleName.EqualToString( "oblique"))
	{
		outFontStyle = DWRITE_FONT_STYLE_OBLIQUE;
		return true;
	}
	else if (inStyleName.EqualToString( "normal"))
	{
		outFontStyle = DWRITE_FONT_STYLE_NORMAL;
		return true;
	}
	return false;
}

bool XWinFont::_GetFontStretchFromStyleName( const VString& inStyleName, int& outFontStretch) const
{
	if (inStyleName.EqualToString( "condensed") || inStyleName.EqualToString( "cond") || inStyleName.EqualToString( "compressed") || inStyleName.EqualToString( "narrow"))
	{
		outFontStretch = DWRITE_FONT_STRETCH_CONDENSED;
		return true;
	}
	else if (inStyleName.EqualToString( "expanded") || inStyleName.EqualToString( "exp") || inStyleName.EqualToString( "extended"))
	{
		outFontStretch = DWRITE_FONT_STRETCH_EXPANDED;
		return true;
	}
	return false;
}

#endif

FontRef XWinFont::_CreateFontRef (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize) 
{
	LOGFONTW logfont;
	//JQ 30/06/2010: avoid to lock screen
	HDC hDC=CreateCompatibleDC(NULL);
	
	//convert size to pixel
	GReal inSizePx = fFont->_GetPixelSize();
	if (inSizePx == 0.0f)
		inSizePx = PointsToPixels(inSize);
	if (inSizePx == 0)
		inSizePx = 1; //ensure valid font is created for all impls

	::memset(&logfont, 0, sizeof(logfont));

	logfont.lfHeight = -inSizePx;
	logfont.lfWidth = 0;
	logfont.lfEscapement = 0;
	logfont.lfOrientation = 0;
	logfont.lfWeight = (inFace & KFS_BOLD) ? FW_BOLD : FW_NORMAL;
	logfont.lfItalic = (inFace & KFS_ITALIC) ? TRUE : FALSE;
	logfont.lfUnderline = (inFace & KFS_UNDERLINE) ? TRUE : FALSE;
	logfont.lfStrikeOut = (inFace & KFS_STRIKEOUT) ? TRUE : FALSE;
	logfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	//JQ 22/06/2010: allow more available sizes and enable synthetization of styles
	//				 when styles are not available for the specified font
	//logfont.lfQuality = CLEARTYPE_QUALITY;
	logfont.lfQuality = DEFAULT_QUALITY;
	logfont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	inFontFamilyName.ToBlock(logfont.lfFaceName, sizeof(logfont.lfFaceName), VTC_UTF_16, true, false);
	logfont.lfCharSet=DEFAULT_CHARSET;
	::EnumFontFamiliesExW(hDC, &logfont,(FONTENUMPROCW)EnumFontFamiliesExProc, 0, 0 );

	//JQ 22/06/2010: store locally native font

	VString sFontFamilyDefault = "Times New Roman";

#if GRAPHIC_MIXED_GDIPLUS_D2D
	//create GDIPlus native font

	if (fGDIPlusFont)
		delete fGDIPlusFont;
	fGDIPlusFont = new Gdiplus::Font( hDC, &logfont);
#if VERSIONDEBUG
	Gdiplus::Unit unit = fGDIPlusFont->GetUnit();
#endif
	if (!fGDIPlusFont->IsAvailable())
	{
		//probably not TrueType font:
		//allow gdiplus to deal with substitution
		delete fGDIPlusFont;

		int gdiplusface = Gdiplus::FontStyleRegular;

		if (inFace & KFS_ITALIC)
		{
			if (inFace & KFS_BOLD)
				gdiplusface = Gdiplus::FontStyleBoldItalic;
			else
				gdiplusface = Gdiplus::FontStyleItalic;
		}
		else
			if (inFace & KFS_BOLD)
				gdiplusface = Gdiplus::FontStyleBold;

		if (inFace & KFS_UNDERLINE)
			gdiplusface |= Gdiplus::FontStyleUnderline;
		if (inFace & KFS_STRIKEOUT)
			gdiplusface |= Gdiplus::FontStyleStrikeout;
		
		fGDIPlusFont = new Gdiplus::Font(inFontFamilyName.GetCPointer(), inSizePx, gdiplusface, Gdiplus::UnitPixel);
		xbox_assert(fGDIPlusFont->IsAvailable());
	}

#if ENABLE_D2D
	//set D2D font family name substitute equal to GDIPlus font family name
	Gdiplus::FontFamily *fontFamily = new Gdiplus::FontFamily();
	fGDIPlusFont->GetFamily( fontFamily);
	if (fontFamily->IsAvailable())
	{
		WCHAR  name[LF_FACESIZE];
		Gdiplus::Status status = fontFamily->GetFamilyName( name, LANG_NEUTRAL);
		xbox_assert(sizeof(WCHAR) == sizeof(UniChar));
		sFontFamilyDefault.FromBlock( name, wcslen( name)*sizeof(UniChar), VTC_UTF_16);
	}
	delete fontFamily;
#endif

	LOGFONTW logfont2;
	::memset(&logfont2, 0, sizeof(logfont2));
	{
	Gdiplus::Graphics graphics( hDC);
	fGDIPlusFont->GetLogFontW( &graphics, &logfont2);

	//if font family is not modified, we still need to update GDI logfont style from Gdiplus logfont style
	//because if gdiplus has returned another style, it is because font does not have this style either
	//and so we need to use gdiplus logfont to create the GDI font to ensure GDI font will have the proper supported styles
	//(for instance, if we request for Aharoni font, il will return a Aharoni bold font which is supported but not regular font
	// and so lfWeight needs to be set to bold)
	if (wcscmp( logfont.lfFaceName, logfont2.lfFaceName) == 0)
		memcpy(&logfont, &logfont2, sizeof(LOGFONTW));
	}
#endif
    DeleteDC( hDC );

#if ENABLE_D2D
	//create DWrite native font
	if (VWinD2DGraphicContext::IsAvailable())
	{
		VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());

		if (fDWriteFont)
		{
			fDWriteFont->Release();
			fDWriteFont = NULL;
		}
		if (fDWriteTextFormat)
		{
			fDWriteTextFormat->Release();
			fDWriteTextFormat = NULL;
		}
		if (fCachedDWriteTextLayout)
		{
			fCachedDWriteTextLayout->Release();
			fCachedDWriteTextLayout = NULL;
		}
		if (fDWriteEllipsisTrimmingSign)
		{
			fDWriteEllipsisTrimmingSign->Release();
			fDWriteEllipsisTrimmingSign = NULL;
		}

		DWRITE_FONT_STYLE fontStyle = (inFace & KFS_ITALIC) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
		
		DWRITE_FONT_WEIGHT fontWeight;
		if (inFace & KFS_BOLD)
			fontWeight = DWRITE_FONT_WEIGHT_BOLD;
		else
			fontWeight = DWRITE_FONT_WEIGHT_NORMAL;
		
		DWRITE_FONT_STRETCH fontStretch;
		if (inFace & KFS_CONDENSED)
			fontStretch = DWRITE_FONT_STRETCH_CONDENSED;
		else if (inFace & KFS_EXTENDED)
			fontStretch = DWRITE_FONT_STRETCH_EXPANDED;
		else
			fontStretch = DWRITE_FONT_STRETCH_NORMAL;

		static VString gLocalName;
		if (gLocalName.IsEmpty())
		{
			WCHAR temp[40];
			LCID lcid = ::GetUserDefaultLCID();
			::GetLocaleInfoW( lcid, LOCALE_SISO639LANGNAME, temp, sizeof(temp) );
			gLocalName.FromUniCString( temp);
			gLocalName.AppendUniChar('-');
			::GetLocaleInfoW( lcid, LOCALE_SISO3166CTRYNAME, temp, sizeof(temp) );
			gLocalName.AppendUniCString( temp);
		}

		//create DWrite font
		CComPtr<IDWriteFontCollection> fontCollection;
		HRESULT hr = VWinD2DGraphicContext::GetDWriteFactory()->GetSystemFontCollection(&fontCollection);
		xbox_assert(SUCCEEDED(hr));
		BOOL exists = FALSE;
		UINT32 index = 0;
		VString sActualFontFamilyName;
		fontCollection->FindFamilyName( inFontFamilyName.GetCPointer(), &index, &exists);
		if (exists)
			sActualFontFamilyName = inFontFamilyName;
		else
		{
			//can be a font family with style so extract style & font family name
			int style = DWRITE_FONT_STYLE_NORMAL, weight = DWRITE_FONT_WEIGHT_REGULAR, stretch = DWRITE_FONT_STRETCH_UNDEFINED;
			_GetStyleFromFontName( inFontFamilyName, sActualFontFamilyName, style, weight, stretch);
			if (style != DWRITE_FONT_STYLE_NORMAL && fontStyle == DWRITE_FONT_STYLE_NORMAL)
				fontStyle = (DWRITE_FONT_STYLE)style;
			if (weight != DWRITE_FONT_WEIGHT_REGULAR && fontWeight == DWRITE_FONT_WEIGHT_REGULAR)
				fontWeight = (DWRITE_FONT_WEIGHT)weight;
			if (stretch != DWRITE_FONT_STRETCH_UNDEFINED && fontStretch == DWRITE_FONT_STRETCH_NORMAL)
				fontStretch = (DWRITE_FONT_STRETCH)stretch; 
			index = 0;
			exists = FALSE;
			fontCollection->FindFamilyName( sActualFontFamilyName.GetCPointer(), &index, &exists);
			if (!exists)
			{
				//default to GDIPlus font name
				index = 0;
				exists = FALSE;
				fontCollection->FindFamilyName( sFontFamilyDefault.GetCPointer(), &index, &exists);
				if (exists)
					sActualFontFamilyName = sFontFamilyDefault;
				else
				{
					//unknown font: default to sans serif
					VFont::GetGenericFontFamilyName( GENERIC_FONT_SANS_SERIF, sActualFontFamilyName);
					index = 0;
					exists = FALSE;
					fontCollection->FindFamilyName( sActualFontFamilyName.GetCPointer(), &index, &exists);
					xbox_assert(exists);
				}
			}
		}
		if (exists == TRUE)
		{
			CComPtr<IDWriteFontFamily> fontFamily;
			if (SUCCEEDED(fontCollection->GetFontFamily(index, &fontFamily)))
			{
				if (SUCCEEDED(fontFamily->GetFirstMatchingFont( fontWeight, fontStretch, fontStyle, &fDWriteFont)))
				{
					//pre-create DWrite text format (blueprint to create text layout)

					HRESULT hr = VWinD2DGraphicContext::GetDWriteFactory()->CreateTextFormat(
						sActualFontFamilyName.GetCPointer(),                
						NULL,                       
						fontWeight,
						fontStyle,
						fontStretch,
						inSize != 0.0f ? inSizePx : 1,	//prevent from creation failure if font size is equal to 0
						gLocalName.GetCPointer(),
						&fDWriteTextFormat
						);
					xbox_assert(SUCCEEDED(hr));

					//create trimming ellipsis sign
					hr = VWinD2DGraphicContext::GetDWriteFactory()->CreateEllipsisTrimmingSign( fDWriteTextFormat, &fDWriteEllipsisTrimmingSign);
				}
			}
		}
	}
#endif
	//create GDI font
	return (::CreateFontIndirectW(&logfont));
}

#if ENABLE_D2D
IDWriteTextLayout *XWinFont::_GetCachedTextLayout( const VString& inText, const GReal inWidth, const GReal inHeight, AlignStyle inHoriz, AlignStyle inVert, TextLayoutMode inMode, const bool isAntialiased, const GReal inRefDocDPI)
{
	if (fCachedDWriteTextLayout
		&& 
		inWidth == fCachedWidth
		&&
		inHeight == fCachedHeight
		&&
		inHoriz == fCachedHAlign
		&&
		inVert == fCachedVAlign
		&&
		inMode == fCachedLayoutMode
		&&
		inRefDocDPI == fCachedDPI
		&&
		inText.EqualToStringRaw(fCachedText)
		/*
		&&
		isAntialiased == fCachedAntialiased*/)
	{
		fCachedDWriteTextLayout->AddRef();
		return fCachedDWriteTextLayout;
	}
	return NULL;
}

void XWinFont::_ApplyMultiStyle( ID2D1RenderTarget *inRT, IDWriteTextLayout *inTextLayout, VTreeTextStyle *inStyles, VSize inTextLength, const GReal inRefDocDPI)
{
	if (!inStyles)
		return;
	const VTextStyle *style = inStyles->GetData();
	if (!style)
		return;

	//apply current style

	sLONG start,end;
	style->GetRange(start, end);
	DWRITE_TEXT_RANGE textRange = { start, end-start};
	if (end > start)
	{
		do
		{
			if (start == 0 && end >= inTextLength) //text alignment can only be applied on all text
			{
				//set text alignment 
				DWRITE_READING_DIRECTION readingDirection = inTextLayout->GetReadingDirection();
				DWRITE_TEXT_ALIGNMENT textAlignment = inTextLayout->GetTextAlignment();
				switch (style->GetJustification())
				{
					case JST_Center:
						textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
						break;
						
					case JST_Right:
						textAlignment = readingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_TRAILING;
						break;
						
					case JST_Left:
						textAlignment = readingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING;
						break;

					case JST_Default:
						textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
						break;

					default:
						break;
				}
				if (inTextLayout->GetTextAlignment() != textAlignment)
					inTextLayout->SetTextAlignment( textAlignment);
			}

			//set font family
			if (!style->GetFontName().IsEmpty())
				inTextLayout->SetFontFamilyName( style->GetFontName().GetCPointer(), textRange);

			//set color
			if (style->GetHasForeColor())
			{
				RGBAColor color = style->GetColor();
				ID2D1SolidColorBrush *brushColor = NULL;
				if (SUCCEEDED(inRT->CreateSolidColorBrush( D2D1::ColorF(color & 0xFFFFFF, (color >> 24)/255.0f), 
															&brushColor)))
				{
					IUnknown *drawingEffect = NULL;
					brushColor->QueryInterface( __uuidof(IUnknown), (void **)&drawingEffect);
					inTextLayout->SetDrawingEffect( drawingEffect, textRange);
					drawingEffect->Release();
					brushColor->Release();
				}
			}

			//set font size
			Real fontSize = style->GetFontSize();
			if (fontSize != -1)
			{
	#if D2D_GDI_COMPATIBLE
				if (inRefDocDPI == 72.0f)
					inTextLayout->SetFontSize( fontSize, textRange);
				else
	#endif
				{
	#if ENABLE_D2D
	#if D2D_GDI_COMPATIBLE
					fontSize = fontSize * inRefDocDPI / VWinGDIGraphicContext::GetLogPixelsY();
	#else
					fontSize = fontSize * inRefDocDPI / 96.0f;
	#endif
	#else
					fontSize = fontSize * inRefDocDPI / VWinGDIGraphicContext::GetLogPixelsY();
	#endif

					inTextLayout->SetFontSize( D2D_PointToDIP( fontSize), textRange);
				}
			}

			//set bold
			if (style->GetBold() == TRUE)
				inTextLayout->SetFontWeight( DWRITE_FONT_WEIGHT_BOLD, textRange);
			else if (style->GetBold() == FALSE)
				inTextLayout->SetFontWeight( DWRITE_FONT_WEIGHT_NORMAL, textRange);

			//set italic
			if (style->GetItalic() == TRUE)
				inTextLayout->SetFontStyle( DWRITE_FONT_STYLE_ITALIC, textRange);
			else if (style->GetItalic() == FALSE)
				inTextLayout->SetFontStyle( DWRITE_FONT_STYLE_NORMAL, textRange);

			//set underline
			if (style->GetUnderline() == TRUE)
				inTextLayout->SetUnderline( TRUE, textRange);
			else if (style->GetUnderline() == FALSE)
				inTextLayout->SetUnderline( FALSE, textRange);

			//set strikeout
			if (style->GetStrikeout() == TRUE)
				inTextLayout->SetStrikethrough( TRUE, textRange);
			else if (style->GetStrikeout() == FALSE)
				inTextLayout->SetStrikethrough( FALSE, textRange);

			if (style->IsSpanRef())
				//apply span ref override style
				style = style->GetSpanRef()->GetStyle();
			else
				style = NULL;

		} while(style);
	}

	//apply child styles
	sLONG numStyle = inStyles->GetChildCount();
	for (int i = 1; i <= numStyle; i++)
	{
		VTreeTextStyle *styles = inStyles->GetNthChild( i);
		_ApplyMultiStyle( inRT, inTextLayout, styles, inTextLength, inRefDocDPI);
	}

}

/** do not try to cache text strings longer than the following length */
#define DWRITE_CTL_CACHE_MAX_TEXT_LENGTH 128

/** create text layout from the specified text string & options */

IDWriteTextLayout *XWinFont::CreateTextLayout(
	ID2D1RenderTarget *inRT, 
	const VString& inText, 
	const bool isAntialiased, 
	const GReal inWidth, 
	const GReal inHeight, 
	AlignStyle inHAlign, 
	AlignStyle inVAlign, 
	TextLayoutMode inMode, 
	VTreeTextStyle *inStyles, 
	const GReal inRefDocDPI, 
	const bool inUseCache)
{
	if (VWinD2DGraphicContext::IsAvailable())
	{
		VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());

		IDWriteTextLayout *textLayout = inStyles == NULL && inUseCache && inText.GetLength() <= DWRITE_CTL_CACHE_MAX_TEXT_LENGTH ? _GetCachedTextLayout( inText, inWidth, inHeight, inHAlign, inVAlign, inMode, isAntialiased, inRefDocDPI) : NULL;
		if (textLayout)
			return textLayout;

//#if D2D_GDI_COMPATIBLE
//		HRESULT hr = VWinD2DGraphicContext::GetDWriteFactory()->CreateGdiCompatibleTextLayout(
//			inText.GetCPointer(),		// The string to be laid out and formatted.
//			inText.GetLength(),			// The length of the string.
//			fDWriteTextFormat,			// The text format to apply to the string (contains font information, etc).
//			inWidth,					// The width of the layout box.
//			inHeight,					// The height of the layout box.
//			96.0f,
//			NULL,
//			isAntialiased ? TRUE : FALSE,
//			&textLayout					// The IDWriteTextLayout interface pointer.
//			);
//#else
		HRESULT hr = VWinD2DGraphicContext::GetDWriteFactory()->CreateTextLayout(
			inText.GetCPointer(),		// The string to be laid out and formatted.
			inText.GetLength(),			// The length of the string.
			fDWriteTextFormat,			// The text format to apply to the string (contains font information, etc).
			inWidth,					// The width of the layout box.
			inHeight,					// The height of the layout box.
			&textLayout					// The IDWriteTextLayout interface pointer.
			);
//#endif
		if (SUCCEEDED(hr))
		{
			//set main style
			DWRITE_TEXT_RANGE textRange = { 0, inText.GetLength() };

			//set underline
			if (fFont->GetFace() & KFS_UNDERLINE)
				textLayout->SetUnderline( TRUE, textRange);

			//set strikeout
			if (fFont->GetFace() & KFS_STRIKEOUT)
				textLayout->SetStrikethrough( TRUE, textRange);

			//set reading direction
			DWRITE_READING_DIRECTION readingDirection = inMode & TLM_RIGHT_TO_LEFT ? DWRITE_READING_DIRECTION_RIGHT_TO_LEFT : DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
			if (textLayout->GetReadingDirection() != readingDirection)
				textLayout->SetReadingDirection( readingDirection);

			//set text alignment
			DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
			switch (inHAlign)
			{
				case AL_CENTER:
					textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
					break;
					
				case AL_RIGHT:
					textAlignment = inMode & TLM_RIGHT_TO_LEFT ? DWRITE_TEXT_ALIGNMENT_LEADING : DWRITE_TEXT_ALIGNMENT_TRAILING;
					break;
					
				case AL_LEFT:
					textAlignment = inMode & TLM_RIGHT_TO_LEFT ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING;
					break;

				case AL_DEFAULT:
					break;

				case AL_JUST:
					//unimplemented
					break;

				default:
					xbox_assert(false);
					break;
			}


			if (textLayout->GetTextAlignment() != textAlignment)
				textLayout->SetTextAlignment( textAlignment);
				
			//set paragraph alignment
			DWRITE_PARAGRAPH_ALIGNMENT paraAlignment = DWRITE_PARAGRAPH_ALIGNMENT_NEAR;
			switch (inVAlign)
			{
				case AL_CENTER:
					paraAlignment = DWRITE_PARAGRAPH_ALIGNMENT_CENTER;
					break;
					
				case AL_RIGHT:
					paraAlignment = DWRITE_PARAGRAPH_ALIGNMENT_FAR;
					break;
					
				case AL_LEFT:
				case AL_DEFAULT:
					break;

				case AL_JUST:
					// tbd
					break;

				default:
					xbox_assert(false);
					break;
			}
			if (textLayout->GetParagraphAlignment() != paraAlignment)
				textLayout->SetParagraphAlignment( paraAlignment);

			//set wrapping mode
			DWRITE_WORD_WRAPPING wordWrapping = ((inMode & TLM_DONT_WRAP) != 0) ? DWRITE_WORD_WRAPPING_NO_WRAP : DWRITE_WORD_WRAPPING_WRAP;
			if (wordWrapping != textLayout->GetWordWrapping())
				textLayout->SetWordWrapping( wordWrapping);

			//set trimming
			IDWriteInlineObject* inlineObject = NULL;
            DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_NONE, 0, 0 };
            textLayout->GetTrimming(&trimming, &inlineObject);
            DWRITE_TRIMMING_GRANULARITY granularity = ((inMode & (TLM_TRUNCATE_MIDDLE_IF_NECESSARY|TLM_TRUNCATE_END_IF_NECESSARY)) != 0) //FIXME: for now, D2D does trimming only at the end
													 ? DWRITE_TRIMMING_GRANULARITY_CHARACTER
													 : DWRITE_TRIMMING_GRANULARITY_NONE;
			if (granularity != trimming.granularity 
				|| 
				(granularity != DWRITE_TRIMMING_GRANULARITY_NONE && inlineObject == NULL))
			{
				trimming.granularity = granularity;
				if (granularity != DWRITE_TRIMMING_GRANULARITY_NONE && inlineObject == NULL)
				{
					inlineObject = fDWriteEllipsisTrimmingSign;
					inlineObject->AddRef();
				}
				textLayout->SetTrimming(&trimming, inlineObject);
			}
			if (inlineObject)
				inlineObject->Release();

			if (inStyles)
				//set child styles
				_ApplyMultiStyle( inRT, textLayout, inStyles, inText.GetLength(), inRefDocDPI);

			if (inStyles == NULL && inUseCache && inText.GetLength() <= DWRITE_CTL_CACHE_MAX_TEXT_LENGTH)
			{
				fCachedText = inText;
				fCachedWidth = inWidth;
				fCachedHeight = inHeight;
				fCachedHAlign = inHAlign;
				fCachedVAlign = inVAlign;
				fCachedLayoutMode = inMode;
				fCachedDPI = inRefDocDPI;
				fCachedAntialiased = isAntialiased;
				if (fCachedDWriteTextLayout)
					fCachedDWriteTextLayout->Release();
				fCachedDWriteTextLayout = textLayout;
				textLayout->AddRef();
			}
			return textLayout;
		}
	}
	return NULL;
}

/**	create text style for the specified text position 
@remarks
	range of returned text style is set to the range of text which has same format as text at specified position
*/
VTextStyle *XWinFont::CreateTextStyle( IDWriteTextLayout *inLayout, const sLONG inTextPos)
{
	if (!inLayout)
		return NULL;

	//get style run length 
	DWRITE_TEXT_RANGE range;
	UINT32 nameLength = 0;
	HRESULT hr = inLayout->GetFontFamilyNameLength( inTextPos, &nameLength, &range);
	if (FAILED(hr))
		return NULL;

	VTextStyle *textStyle = new VTextStyle();
	textStyle->SetRange( range.startPosition, range.startPosition+range.length);

	//get font name
	if (nameLength+1 <= 80)
	{
		WCHAR familyName[80];
		hr = inLayout->GetFontFamilyName( inTextPos, familyName, nameLength+1);
		if (FAILED(hr))
		{
			delete textStyle;
			return NULL;
		}
		textStyle->SetFontName( VString(familyName));
	}
	else
	{
		WCHAR *familyName = new WCHAR[nameLength+1];
		hr = inLayout->GetFontFamilyName( inTextPos, familyName, nameLength+1);
		if (FAILED(hr))
		{
			delete [] familyName;
			delete textStyle;
			return NULL;
		}
		delete [] familyName;
	}

	//get font size
	FLOAT fontSize;
	hr = inLayout->GetFontSize( inTextPos, &fontSize);
	if (FAILED(hr))
	{
		delete textStyle;
		return NULL;
	}
	textStyle->SetFontSize( fontSize);

	//TODO: add stretch style support to VTextStyle ?
	/* 
	DWRITE_FONT_STRETCH fontStretch;
	hr = inLayout->GetFontStretch( inTextPos, &fontStetch);
	if (FAILED(hr))
	{
		delete textStyle;
		return NULL;
	}
	*/

	//get font weight
	DWRITE_FONT_WEIGHT fontWeight;
	hr = inLayout->GetFontWeight( inTextPos, &fontWeight);
	if (FAILED(hr))
	{
		delete textStyle;
		return NULL;
	}
	textStyle->SetBold( fontWeight >= DWRITE_FONT_WEIGHT_BOLD ? TRUE : FALSE);

	//get font style
	DWRITE_FONT_STYLE fontStyle;
	hr = inLayout->GetFontStyle( inTextPos, &fontStyle);
	if (FAILED(hr))
	{
		delete textStyle;
		return NULL;
	}
	textStyle->SetItalic( fontStyle == DWRITE_FONT_STYLE_ITALIC ? TRUE : FALSE);

	//get underline status
	BOOL hasUnderline;
	hr = inLayout->GetUnderline( inTextPos, &hasUnderline);
	if (FAILED(hr))
	{
		delete textStyle;
		return NULL;
	}
	textStyle->SetUnderline( hasUnderline);
	
	//get strikethrough status
	BOOL hasStrikeThrough;
	hr = inLayout->GetStrikethrough( inTextPos, &hasStrikeThrough);
	if (FAILED(hr))
	{
		delete textStyle;
		return NULL;
	}
	textStyle->SetStrikeout( hasStrikeThrough);

	//get color
	CComPtr<IUnknown> drawEffect;
	hr = inLayout->GetDrawingEffect( inTextPos, &drawEffect);
	if (SUCCEEDED(hr) && ((IUnknown *)drawEffect) != NULL)
	{
		ID2D1SolidColorBrush *color = NULL;
		hr = drawEffect->QueryInterface( __uuidof(ID2D1SolidColorBrush), (void **)&color);
		if (SUCCEEDED(hr))
		{
			D2D1_COLOR_F colD2D = color->GetColor();
			VColor xcolor( (uBYTE)(colD2D.r*255.0f), (uBYTE)(colD2D.g*255.0f), (uBYTE)(colD2D.b*255.0f), (uBYTE)(colD2D.a*255.0f));
			color->Release();
			textStyle->SetHasForeColor(true);
			textStyle->SetColor( xcolor.GetRGBAColor());
		}
	}
	return textStyle;
}

/**	create text styles tree for the specified text range */
VTreeTextStyle *XWinFont::CreateTreeTextStyle( IDWriteTextLayout *inLayout, const sLONG inTextPos, const sLONG inTextLen)
{
	sLONG start = inTextPos;
	sLONG end = inTextPos+inTextLen;
	VTextStyle *parentStyle = NULL;
	VTreeTextStyle *treeStyles = NULL;
	VTreeTextStyle *curTreeStyle = NULL;
	do 
	{
		VTextStyle *style = CreateTextStyle( inLayout, start);
		if (!style)
			break;
		
		sLONG s, e;
		style->GetRange( s, e);
		s = start;
		start = e;
		xbox_assert(s >= inTextPos);
		if (end-s == 0)
		{
			delete style;
			style = NULL;
		}
		else
			style->SetRange( s-inTextPos, end-inTextPos);
		
		if (style)
		{
			if (treeStyles == NULL)
			{
				//get justification
				DWRITE_READING_DIRECTION readingDir = inLayout->GetReadingDirection();
				DWRITE_TEXT_ALIGNMENT textAlign = inLayout->GetTextAlignment();
				switch (textAlign)
				{
					case DWRITE_TEXT_ALIGNMENT_LEADING:
						style->SetJustification( JST_Notset); //not set: use default based on reading dir
						break;
					case DWRITE_TEXT_ALIGNMENT_TRAILING:
						style->SetJustification( readingDir == DWRITE_READING_DIRECTION_LEFT_TO_RIGHT ? JST_Right : JST_Left);
						break;
					case DWRITE_TEXT_ALIGNMENT_CENTER:
						style->SetJustification( JST_Center);
						break;
					default:
						xbox_assert(false);
						break;
				}
			}

			VTextStyle *parentStyle = curTreeStyle ? curTreeStyle->GetData() : NULL;
			if (parentStyle)
			{
				//ensure we redefine only styles which are different from parent styles
				sLONG numEqual = 0;
				if (style->GetFontName().EqualToString( parentStyle->GetFontName(), true))
				{ 
					style->SetFontName(VString(""));
					numEqual++;
				}
				if (style->GetBold() == parentStyle->GetBold())
				{ 
					style->SetBold( UNDEFINED_STYLE);
					numEqual++;
				}
				if (style->GetFontSize() == parentStyle->GetFontSize())
				{ 
					style->SetFontSize( UNDEFINED_STYLE);
					numEqual++;
				}
				if (style->GetItalic() == parentStyle->GetItalic())
				{ 
					style->SetItalic( UNDEFINED_STYLE);
					numEqual++;
				}
				if (style->GetUnderline() == parentStyle->GetUnderline())
				{ 
					style->SetUnderline( UNDEFINED_STYLE);
					numEqual++;
				}
				if (style->GetStrikeout() == parentStyle->GetStrikeout())
				{ 
					style->SetStrikeout( UNDEFINED_STYLE);
					numEqual++;
				}
				if (style->GetHasForeColor() == parentStyle->GetHasForeColor()
						&&
						style->GetColor() == parentStyle->GetColor()
						)
				{ 
					style->SetHasForeColor( false);
					numEqual++;
				}
				if (numEqual >= 7)
				{
					//style is exactly the same as the parent style: release it
					delete style;
					style = NULL;
				}
			}

			if (style)
			{
				VTreeTextStyle *parentTreeStyle = curTreeStyle;
				curTreeStyle = new VTreeTextStyle( style);

				if (parentTreeStyle)
				{
					parentTreeStyle->AddChild( curTreeStyle);
					curTreeStyle->Release();
				}
				else
					treeStyles = curTreeStyle;
			}
		}
		if (start >= end)
			break;
	} while (true);

	return treeStyles;
}
#endif


int XWinFont::GetCharSetFromFontName(const VString& inFontFamilyName) const
{
	VString str,str2;
	str.FromCString("SimSun");
	str2.FromCString("MS Mincho");
	if(inFontFamilyName.EqualToString(str))
	{
		return CHINESEBIG5_CHARSET;
	}
	else if(inFontFamilyName.EqualToString(str2))
	{
		return SHIFTJIS_CHARSET;
	}
	else
	   return DEFAULT_CHARSET;
}

int CALLBACK XWinFont::EnumFontFamiliesExProc(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme,DWORD FontType,LPARAM lParam)
{   
	BYTE charset;
	charset=lpelfe->elfLogFont.lfCharSet;
	return 1;
}


void XWinFont::ApplyToPort(PortRef inPort, GReal inScale) 
{
	if (::GetObjectType(fFontRef) == 0)
		DebugMsg("BAD Font Object\r\n");

	//caution here: ensure we do not modify local native fonts !!
	//(here it is better to use VFontManager do derive current font so we do not store it locally
	// and we defer to VFontManager the cache management)
	HFONT	scaledFont;
	if (inScale != 1.0)
	{
		VFont *font = fFont->DeriveFontSize( inScale * fFont->GetSize());
		scaledFont = font->GetFontRef();
		font->Release();
	}
	else
		scaledFont = fFontRef;
		
	HGDIOBJ	previous = ::SelectObject(inPort, scaledFont);
}


DWORD XWinFont::FontFaceToWinFontEffect(const VFontFace& inFace)
{
	DWORD	effect = (inFace & KFS_BOLD) ? CFE_BOLD : 0;
	effect |= (inFace & KFS_ITALIC) != 0 ? CFE_ITALIC : 0;
	effect |= (inFace & KFS_UNDERLINE) != 0 ? CFE_UNDERLINE : 0;
	effect |= (inFace & KFS_SHADOW) != 0 ? CFE_SHADOW : 0;
	effect |= (inFace & KFS_OUTLINE) != 0 ? CFE_OUTLINE : 0;
	effect |= (inFace & KFS_STRIKEOUT) != 0 ? CFE_STRIKEOUT : 0;
	
	return effect;
}


void XWinFont::WinFontEffectToFontFace(DWORD inEffect, VFontFace& outFace)
{
	outFace = KFS_NORMAL;
	outFace |= (inEffect & CFE_BOLD) != 0 ? KFS_BOLD : 0;
	outFace |= (inEffect & CFE_ITALIC) != 0 ? KFS_ITALIC : 0;
	outFace |= (inEffect & CFE_UNDERLINE) != 0 ? KFS_UNDERLINE : 0;
	outFace |= (inEffect & CFE_SHADOW) != 0 ? KFS_SHADOW : 0;
	outFace |= (inEffect & CFE_OUTLINE) != 0 ? KFS_OUTLINE : 0;
	outFace |= (inEffect & CFE_STRIKEOUT) != 0 ? KFS_STRIKEOUT : 0;
}


//===================================================================



void XWinFontMetrics::GetMetrics(GReal& outAscent, GReal& outDescent, GReal& outLeading) const
{
	//remark: ascent includes internal leading (cf MSDN documentation)
	GReal internalLeading;
	GetMetrics( outAscent, outDescent, internalLeading, outLeading);
}


void XWinFontMetrics::GetMetrics(GReal& outAscent, GReal& outDescent, GReal& outInternalLeading, GReal& outExternalLeading) const
{
	if ( fMetrics->GetContext() == NULL )
	{
		outAscent = outDescent = outInternalLeading = outExternalLeading = 0;
		return;
	}
#if ENABLE_D2D
	if ( fMetrics->GetContext()->IsD2DImpl() && !fMetrics->UseLegacyMetrics())
	{
		//get DWrite metrics
		VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());

		if (fMetrics->GetFont()->GetDWriteFont())
		{
			DWRITE_FONT_METRICS fontMetrics;
			fMetrics->GetFont()->GetDWriteFont()->GetMetrics(&fontMetrics);
			
			GReal fontSizeDIP = fMetrics->GetFont()->GetPixelSize();
			outAscent = fontMetrics.ascent*fontSizeDIP/fontMetrics.designUnitsPerEm;
			outDescent = fontMetrics.descent*fontSizeDIP/fontMetrics.designUnitsPerEm;
			outInternalLeading = 0.0f;
			outExternalLeading = fontMetrics.lineGap*fontSizeDIP/fontMetrics.designUnitsPerEm;
			return;
		}
		else
			xbox_assert(false);
		return;
	}
#endif
	//get GDI metrics

	TEXTMETRICW tm;
	 
	StParentContextNoDraw nodraw( fMetrics->GetContext());

	PortRef port = fMetrics->GetContext()->GetParentPort();
	HGDIOBJ oldobj;
	oldobj = ::SelectObject(port, fMetrics->GetFont()->GetFontRef() );
	::GetTextMetricsW(port, &tm);
	::SelectObject(port, oldobj );

//	fTrueTypeFont = (tm.tmPitchAndFamily & TMPF_TRUETYPE)!=0;
	fMetrics->GetContext()->ReleaseParentPort(port);
	outInternalLeading = tm.tmInternalLeading;
	outExternalLeading = tm.tmExternalLeading;
	outAscent = tm.tmAscent;
	outDescent = tm.tmDescent;
}


GReal XWinFontMetrics::GetTextWidth(const VString& inText) const
{
	if (fMetrics->GetContext() == NULL )
		return 0.0f;
#if ENABLE_D2D
	//blindage: call DoUseLegacyMetrics in order to take account new text rendering mode if it has changed
	fMetrics->DoUseLegacyMetrics( fMetrics->GetDesiredUseLegacyMetrics());
	if (fMetrics->GetContext()->IsD2DImpl() && !fMetrics->UseLegacyMetrics())
	{
		//get DWrite metrics
		VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());

		if (fMetrics->GetFont()->GetDWriteTextFormat())
		{
			IDWriteTextLayout *textLayout = fMetrics->GetFont()->CreateTextLayout( (ID2D1RenderTarget *)fMetrics->GetContext()->GetNativeRef(), inText, (!(fMetrics->GetContext()->GetTextRenderingMode() & TRM_WITHOUT_ANTIALIASING)) );
			DWRITE_TEXT_METRICS textMetrics;
			textLayout->GetMetrics( &textMetrics);
			textLayout->Release();
			return textMetrics.widthIncludingTrailingWhitespace;
		}
		else
			xbox_assert(false);
		return 0.0f;
	}
#endif
	//use GDI metrics
	SIZE	sz;

	StParentContextNoDraw nodraw( fMetrics->GetContext());

	PortRef port = fMetrics->GetContext()->GetParentPort();
	HGDIOBJ oldobj;
	oldobj = ::SelectObject(port, fMetrics->GetFont()->GetFontRef() );
	
	LOGFONTW lf;
	GetObjectW(fMetrics->GetFont()->GetFontRef(),sizeof(LOGFONTW),&lf);
	sLONG extra=GetTextCharacterExtra(port);
	SetMapMode(port,MM_TEXT);
	if (::GetTextExtentPoint32W(port, (LPCWSTR)inText.GetCPointer(), inText.GetLength(), &sz) == 0)
		sz.cx = 0;
	::SelectObject(port, oldobj );
	fMetrics->GetContext()->ReleaseParentPort(port);
	return sz.cx;
}

GReal XWinFontMetrics::GetCharWidth(UniChar inChar) const
{
	VStr15 str(inChar);
	
	return GetTextWidth(str);
}


void XWinFontMetrics::MeasureText( const VString& inText, std::vector<GReal>& outOffsets) const
{
	if (fMetrics->GetContext() == NULL )
	{
		sLONG len = inText.GetLength();
		for(sLONG i = 0 ; i < len ; ++i)
			outOffsets.push_back( 0.0f);
	}

#if ENABLE_D2D
	//blindage: call DoUseLegacyMetrics in order to take account new text rendering mode if it has changed
	fMetrics->DoUseLegacyMetrics( fMetrics->GetDesiredUseLegacyMetrics());
	if (fMetrics->GetContext()->IsD2DImpl() && !fMetrics->UseLegacyMetrics())
	{
		//get DWrite metrics
		VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());

		if (fMetrics->GetFont()->GetDWriteTextFormat())
		{
			IDWriteTextLayout *textLayout = fMetrics->GetFont()->CreateTextLayout( (ID2D1RenderTarget *)fMetrics->GetContext()->GetNativeRef(), inText, (!(fMetrics->GetContext()->GetTextRenderingMode() & TRM_WITHOUT_ANTIALIASING)));

			sLONG len = inText.GetLength();
			FLOAT pointX;
			FLOAT pointY;
			DWRITE_HIT_TEST_METRICS hitTestMetrics;
			for (sLONG index = 0; index < len; index++)
			{
				textLayout->HitTestTextPosition(index, TRUE, &pointX, &pointY, &hitTestMetrics);
				outOffsets.push_back(pointX);
			}
			textLayout->Release();
		}
		else
			xbox_assert(false);
		return;
	}
#endif

	sLONG len = inText.GetLength();

	INT array[ 256 ];
	INT *ptArray;
	
	if (len > 256 )
		ptArray = (INT*) vMalloc(sizeof(INT) * len, 'mtex' );
	else
		ptArray = array;

	if (ptArray )
	{
		StParentContextNoDraw nodraw( fMetrics->GetContext());
		
		PortRef port = fMetrics->GetContext()->GetParentPort();
		SIZE sz = {0,0};
		HGDIOBJ oldobj = ::SelectObject(port, fMetrics->GetFont()->GetFontRef()  );
		::GetTextExtentExPointW(port, (LPCWSTR) inText.GetCPointer(), len, 0, 0, ptArray, &sz);
		::SelectObject(port, oldobj );
		fMetrics->GetContext()->ReleaseParentPort(port);
		for(long i = 0 ; i < len ; ++i)
			outOffsets.push_back( ptArray[i]);
	}

	if (len > 256 )
		vFree(ptArray );
}


bool XWinFontMetrics::MousePosToTextPos(const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos) 
{
	bool found = false;

	std::vector<GReal> offsets;

	MeasureText(inString, offsets);

	GReal left = 0;
	GReal pos = inMousePos - ioPixPos;

	for(std::vector<GReal>::const_iterator i = offsets.begin() ; i != offsets.end() ; ++i)
	{
		GReal right = *i;
		if (pos >= left && pos < right )
		{
			// regarde si la position est plutot a gauche ou a droite de la lettre
			if (pos >(left + right ) / 2 )
			{
				ioPixPos += right;
				ioTextPos += (i - offsets.begin()) + 1;
			}
			else
			{
				ioPixPos += left;
				ioTextPos += (i - offsets.begin());
			}
			found = true;
			break;
		}
		left = right;
	}

	if (!found && !offsets.empty())
	{
		ioPixPos += offsets.back();
		ioTextPos += inString.GetLength();
	}
	
	return found;
}

