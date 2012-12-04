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
#include "VGraphicContext.h"
#if VERSIONWIN
#include "XWinGDIGraphicContext.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif
#endif

VFontMgr*	VFont::sManager;


#if VERSIONWIN
VFont::VFont(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, StdFont inStdFont, GReal inPixelSize):
	fPixelSize(inPixelSize),
#else
VFont::VFont(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, StdFont inStdFont):
#endif
	fFont(this, inFontFamilyName, inFace, Abs(inSize))
{
	fStdFont = inStdFont;
	fFamilyName = inFontFamilyName;
	fFace = inFace;
	fSize = Abs(inSize);

	fFontIDValid=false;
	fFontID=0;
}


/** return the best existing font family name which matches the specified generic ID 
@see
	eGenericFont
*/
void VFont::GetGenericFontFamilyName(eGenericFont inGenericID, VString& outFontFamilyName, const VGraphicContext *inGC)
{
#if VERSIONWIN
	const Gdiplus::FontFamily *fontFamily = NULL;
	switch( inGenericID)
	{
	case GENERIC_FONT_SERIF:
		fontFamily = Gdiplus::FontFamily::GenericSerif();
		break;
	case GENERIC_FONT_SANS_SERIF:
		fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		break;
	case GENERIC_FONT_CURSIVE:
		//gdiplus implementation constraint: 
		//try with "Comic Sans MS"
		//and if failed fallback to sans-serif
		{
			if (FontExists( "Comic Sans MS", KFS_NORMAL, 12, 0, inGC))
			{
				outFontFamilyName = "Comic Sans MS";
				return;
			}
			fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		}
		break;
	case GENERIC_FONT_FANTASY:
		//gdiplus implementation constraint: fallback to sans-serif
		fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		break;
	case GENERIC_FONT_MONOSPACE:
		fontFamily = Gdiplus::FontFamily::GenericMonospace();
		break;
	default:
		xbox_assert(false);
		fontFamily = Gdiplus::FontFamily::GenericSansSerif();
		break;
	}
	if (fontFamily && fontFamily->IsAvailable())
	{
		WCHAR  name[LF_FACESIZE];
		Gdiplus::Status status = fontFamily->GetFamilyName( name, LANG_NEUTRAL);
		xbox_assert(sizeof(WCHAR) == sizeof(UniChar));
		outFontFamilyName.FromBlock( name, wcslen( name)*sizeof(UniChar), VTC_UTF_16);
	}
	else
		outFontFamilyName = "Times New Roman";
#if ENABLE_D2D
#if VERSION_DEBUG
	//check if font family is supported by DWrite too
	//(should always be the case because GDIPlus has support only for TrueType fonts
	// & all TrueType fonts are supported by D2D)
	if (VWinD2DGraphicContext::IsAvailable())
	{
		if (inGC && inGC->IsD2DImpl())
			xbox_assert(FontExists( outFontFamilyName, KFS_NORMAL, 12, inGC));
	}
#endif
#endif
#else
	//MAC OS implementation constraint:
	//as there is no system-provided feature that can feed us with generic fonts
	//(of course if you know about such a feature feel free to modify code below)
	//we need to provide ourselves generic font family names
	//so we choose to use font family names that are always installed on MAC OS X

	VString fontFamily;
	switch( inGenericID)
	{
	case GENERIC_FONT_SERIF:
		fontFamily = "Times New Roman";
		break;
	case GENERIC_FONT_SANS_SERIF:
		fontFamily = "Helvetica";
		break;
	case GENERIC_FONT_CURSIVE:
		fontFamily = "Apple Chancery";
		break;
	case GENERIC_FONT_FANTASY:
		fontFamily = "American Typewriter";
		break;
	case GENERIC_FONT_MONOSPACE:
		fontFamily = "Courier New";
		break;
	default:
		xbox_assert(false);
		fontFamily = "Times New Roman";
		break;
	}
	if (FontExists( fontFamily, KFS_NORMAL, 12))
		outFontFamilyName = fontFamily;
	else
		//fallback to "Times Roman" just in case...
		outFontFamilyName = "Times New Roman";
#endif
}


/** return true if font with the specified font family name, style and size exists on the current system */
bool VFont::FontExists(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI, const VGraphicContext *inGC)
{
#if VERSIONWIN
	bool exists = false;
	//JQ 21/06/2010: fixed and optimized FontExists (was not coherent with font creation)
	VFont *font = RetainFont( inFontFamilyName, inFace, inSize, inDPI, true);
	if (font)
	{
		if (!font->IsTrueTypeFont())
		{
			//not TrueType fonts are drawed with GDI
			//so return true if a valid HFONT is set

			HFONT hFont = font->GetFontRef();
			exists = hFont != NULL;
			font->Release();
			return exists;
		}
#if ENABLE_D2D
		if (inGC && inGC->IsD2DImpl())
		{
			//if DWrite font family name is not equal to inFontFamilyName
			//DWrite has returned a compatible font 
			//so return true only if DWrite font family name matches inFontFamilyName

			VTaskLock lock(&VWinD2DGraphicContext::GetMutexDWriteFactory());
			exists = false;
			if (font->GetDWriteTextFormat() != NULL)
			{
				UINT32 nameLength = font->GetDWriteTextFormat()->GetFontFamilyNameLength();
				if (nameLength+1 <= 80)
				{
					WCHAR familyName[80];
					HRESULT hr = font->GetDWriteTextFormat()->GetFontFamilyName( familyName, nameLength+1);
					exists = wcscmp( inFontFamilyName.GetCPointer(), familyName) == 0;
				}
				else
				{
					WCHAR *familyName = new WCHAR[nameLength+1];
					font->GetDWriteTextFormat()->GetFontFamilyName( familyName, nameLength+1);
					exists = wcscmp( inFontFamilyName.GetCPointer(), familyName) == 0;
					delete [] familyName;
				}
			}
			font->Release();
			return exists;
		}
#endif
		//if gdiplus font family name is not equal to inFontFamilyName
		//gdiplus has returned a compatible font 
		//so return true only if gdiplus font family name matches inFontFamilyName
		Gdiplus::FontFamily *family = new Gdiplus::FontFamily();
		Gdiplus::Status status = font->GetGDIPlusFont()->GetFamily(family);
		WCHAR familyName[LF_FACESIZE];
		family->GetFamilyName( familyName);
		exists = wcscmp( inFontFamilyName.GetCPointer(), familyName) == 0;
		delete family;

		font->Release();
	}
	return exists;
#else
	bool exists = false;
	VFont *font = RetainFont( inFontFamilyName, inFace, inSize, inDPI, true);
	if (font)
	{
		//ensure returned font has the requested family name (here we check only the family name but not the full name)
		//otherwise it is a font substitute
		CTFontRef ctfont = font->GetFontRef();
		CFStringRef name = CTFontCopyFamilyName(ctfont);
		VString xname;
		xname.MAC_FromCFString( name);
		CFRelease(name);
		VString inname( inFontFamilyName);
		inname.Truncate( xname.GetLength());
		return inname.EqualToString( xname);
	}
	return exists;
#endif
}


#if VERSIONWIN
GReal	VFont::GetPixelSize () const
{
	if (fPixelSize != 0.0f)
		return fPixelSize;

#if ENABLE_D2D
	fPixelSize = D2D_PointToDIP( fSize);
#else
	fPixelSize = PointsToPixels( fSize);
#endif
	return fPixelSize;
}
#endif

/**	create text style from the current font
@remarks
	range is not set
*/
VTextStyle *VFont::CreateTextStyle(const GReal inDPI) const			
{
	VTextStyle *style = new VTextStyle();
	style->SetFontName( fFamilyName);
	//here we must pass pixel font size because VTextStyle stores 4D form 72 dpi font size where 1px = 1pt so actually it is real pixel font size
	//(size is scaled automatically later to 72/device dpi to be compliant with 4D form dpi)
	style->SetFontSize( GetPixelSize()*72.0f/inDPI);
	if (fFace & KFS_ITALIC)
		style->SetItalic( TRUE);
	else
		style->SetItalic( FALSE);
	if (fFace & KFS_BOLD)
		style->SetBold( TRUE);
	else
		style->SetBold( FALSE);
	if (fFace & KFS_UNDERLINE)
		style->SetUnderline( TRUE);
	else
		style->SetUnderline( FALSE);
	if (fFace & KFS_STRIKEOUT)
		style->SetStrikeout( TRUE);
	else
		style->SetStrikeout( FALSE);
	return style;
}


/**	create text style from the specified font name, font size & font face
@remarks
	if font name is empty, returned style inherits font name from parent
	if font face is equal to UNDEFINED_STYLE, returned style inherits style from parent
	if font size is equal to UNDEFINED_STYLE, returned style inherits font size from parent
*/
VTextStyle *VFont::CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI)
{
	VTextStyle *style = new VTextStyle();
	style->SetFontName( inFontFamilyName);

	if (inFontSize != UNDEFINED_STYLE)
	{
#if VERSIONWIN
		if (inDPI == 72)
			style->SetFontSize( inFontSize);
		else
#endif
		{
#if VERSIONWIN
#if D2D_GDI_COMPATIBLE
			GReal dpi = inDPI ? inDPI : VWinGDIGraphicContext::GetLogPixelsY();
#else
			GReal dpi = inDPI ? inDPI : 96.0f;
#endif
#else
			GReal dpi = inDPI ? inDPI : 72.0f;
#endif
			//here we must pass pixel font size because VTextStyle stores 4D form 72 dpi font size where 1px = 1pt so actually it is real pixel font size
			//(size is scaled automatically later to 72/device dpi to be compliant with 4D form dpi)
			style->SetFontSize( inFontSize*dpi/72.0f);
		}
	}

	if (inFace != UNDEFINED_STYLE)
	{
		if (inFace & KFS_ITALIC)
			style->SetItalic( TRUE);
		else
			style->SetItalic( FALSE);
		if (inFace & KFS_BOLD)
			style->SetBold( TRUE);
		else
			style->SetBold( FALSE);
		if (inFace & KFS_UNDERLINE)
			style->SetUnderline( TRUE);
		else
			style->SetUnderline( FALSE);
		if (inFace & KFS_STRIKEOUT)
			style->SetStrikeout( TRUE);
		else
			style->SetStrikeout( FALSE);
	}
	return style;
}


VFont* VFont::DeriveFontSize(GReal inSize, const GReal inDPI) const
{
	VFont* result=sManager->RetainFont(fFamilyName, fFace, inSize, inDPI, false);
	if(result && fFontIDValid)
		result->SetFontID(fFontID);
	return result;
}


VFont* VFont::DeriveFontFace(const VFontFace& inFace) const
{
	VFont* result = sManager->RetainFont(fFamilyName, inFace, fSize, 0, false);
	if(result && fFontIDValid)
		result->SetFontID(fFontID);
	return result;
}


void VFont::DeInit()
{
	if(sManager != NULL)
	{
		delete sManager;
		sManager = NULL;
	}
}

VFont *VFont::RetainFontFromStream(VStream *inStream)
{	
	xbox_assert(sManager != NULL);
	
	VStr63 familyName;
	familyName.ReadFromStream(inStream);
	VFontFace face =(VFontFace) inStream->GetLong();
	GReal size = (GReal) inStream->GetReal();
	
	VFont *theFont;
	if(inStream->GetLastError() == VE_OK)
		theFont = sManager->RetainFont(familyName, face, size);
	else
		theFont = sManager->RetainStdFont(STDF_CAPTION);
	
	return theFont;
}


VError	VFont::WriteToStream(VStream *ioStream, sLONG /*inParam*/) const
{
	fFamilyName.WriteToStream(ioStream);
	ioStream->PutLong(fFace);
	return ioStream->PutReal(fSize);
}



bool VFont::Match(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize) const
{
	return fFamilyName == inFontFamilyName && inFace == fFace && inSize == fSize;
}


void VFont::ApplyToPort(PortRef inPort, GReal inScale) 
{
	fFont.ApplyToPort(inPort, inScale);
}


bool VFont::Match(StdFont inFont) const
{
	return fStdFont == inFont;
}


bool VFont::Init()
{
	if(sManager == NULL)
		sManager = new VFontMgr;
		
	return(sManager != NULL);
}


VFont::~VFont()
{
}


#pragma mark-

VFontMetrics::VFontMetrics( VFont* inFont, const VGraphicContext* inContext)
: fUseLegacyMetrics(false)
, fDesiredUseLegacyMetrics(false)
, fFont( inFont)
, fContext( inContext)
, fMetrics( this)
{
	// sc 11/05/2007 the graphic context is the owner of the VFontMetrics object, so never retain the graphic context.
	xbox_assert(inFont != NULL);

	fFont->Retain();
#if VERSIONWIN
	if (inContext)
	{
		if (!inContext->IsD2DImpl())
			fUseLegacyMetrics = true; //GDIPlus metrics = GDI metrics = legacy metrics
		else
		{
			fUseLegacyMetrics = !fFont->IsTrueTypeFont();	//by default, use GDI metrics for not truetype font, otherwise use D2D metrics
															//(because D2D impl draws not truetype font using legacy GDI as it cannot draw natively these kind of font)
			TextRenderingMode mode = inContext->GetTextRenderingMode();
			if (mode & TRM_LEGACY_ON)
				fUseLegacyMetrics = true; //force only GDI metrics (because text is rendered only with GDI)
			else if (mode & TRM_LEGACY_OFF)
				fUseLegacyMetrics = false; //force only impl metrics (because text is rendered only with D2D impl)
		}
	}
	else
		//you should always pass a context (otherwise metrics are set to 0) but just in case...
		fUseLegacyMetrics = true;
#else
	fUseLegacyMetrics = true;
	fDesiredUseLegacyMetrics = true;
#endif
	fMetrics.GetMetrics( fAscent, fDescent, fInternalLeading, fExternalLeading);

#if VERSIONMAC
	if ( inContext )
		fMetrics.GetTextFactory()->SetRenderingMode( inContext->GetTextRenderingMode());
#endif
}


/** copy constructor */
VFontMetrics::VFontMetrics( const VFontMetrics& inFontMetrics):fMetrics( this)
{
	fContext = inFontMetrics.fContext;
	fFont = NULL;

	CopyRefCountable(&fFont, inFontMetrics.fFont);

	fAscent = inFontMetrics.fAscent;
	
	fDescent = inFontMetrics.fDescent;
	fInternalLeading = inFontMetrics.fInternalLeading;
	fExternalLeading = inFontMetrics.fExternalLeading;
	fUseLegacyMetrics = inFontMetrics.fUseLegacyMetrics;
	fDesiredUseLegacyMetrics = inFontMetrics.fDesiredUseLegacyMetrics;
}


/** use legacy (GDI) context metrics or actuel context metrics 
@remarks
	if actual context is D2D: if true, use GDI font metrics otherwise use Direct2D font metrics 
	any other context: use GDI metrics on Windows

	if context text rendering mode has TRM_LEGACY_ON, inUseLegacyMetrics is replaced by true
	if context text rendering mode has TRM_LEGACY_OFF, inUseLegacyMetrics is replaced by false
	(so context text rendering mode overrides this setting)
*/
void VFontMetrics::DoUseLegacyMetrics( bool inUseLegacyMetrics)
{
#if VERSIONWIN
	fDesiredUseLegacyMetrics = inUseLegacyMetrics;
	if ((!fContext) || (!fContext->IsD2DImpl()))
	{
		//GDIPlus metrics = GDI metrics = legacy metrics
		if (!fUseLegacyMetrics)
		{
			fUseLegacyMetrics = true;
			fMetrics.GetMetrics( fAscent, fDescent, fInternalLeading, fExternalLeading);
		}
		return;
	}

	TextRenderingMode mode = fContext->GetTextRenderingMode();
	if (mode & TRM_LEGACY_ON)
		inUseLegacyMetrics = true;
	else if (mode & TRM_LEGACY_OFF)
		inUseLegacyMetrics = false;
	if (fUseLegacyMetrics == inUseLegacyMetrics)
		return;

	fUseLegacyMetrics = inUseLegacyMetrics;
	fMetrics.GetMetrics( fAscent, fDescent, fInternalLeading, fExternalLeading);
#endif
}

VFontMetrics::~VFontMetrics()
{
	fFont->Release();
}


bool VFontMetrics::MousePosToTextPos(const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos) 
{
	if (inString.IsEmpty())
		return false;
	
	return fMetrics.MousePosToTextPos( inString, inMousePos, ioTextPos, ioPixPos);
}


void VFontMetrics::MeasureText( const VString& inString, std::vector<GReal>& outOffsets)
{
	outOffsets.clear();
	if (!inString.IsEmpty())
		fMetrics.MeasureText( inString, outOffsets);
}


/** scale metrics by the specified factor */
void VFontMetrics::Scale( GReal inScaling)
{
	fAscent *= inScaling;
	fDescent *= inScaling;
	fInternalLeading *= inScaling;
	fExternalLeading *= inScaling;
}

// donne la hauteur de la police en utilisant la meme (presque) metode d'arrondi quickdraw 

static sLONG _round(XBOX::GReal inValue)
{
	XBOX::GReal v1;
	XBOX::GReal v = std::modf(inValue,&v1);
	if(v>0.5)
		return std::ceil(inValue);
	else 
		return v1;
}

sLONG VFontMetrics::GetNormalizedTextHeight() const			
{ 
	return _round(fAscent) + _round(fDescent);
}
sLONG VFontMetrics::GetNormalizedLineHeight() const			
{ 
	return _round(fAscent) + _round(fDescent) + _round(fExternalLeading);
}

