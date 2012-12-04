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
#ifndef __VFont__
#define __VFont__

#include "Graphics/Sources/VFontMgr.h"

// Native declarations
#if VERSIONMAC
    #include "Graphics/Sources/XMacFont.h"
	typedef CTFontRef VFontNativeRef;
#elif VERSIONWIN
    #include "Graphics/Sources/XWinFont.h"
	#include <gdiplus.h> 
	/** opaque ref: 
		Gdiplus::Font* if GRAPHIC_MIXED_GDIPLUS_D2D is equal to 1
		IDWriteFont* if GRAPHIC_MIXED_GDIPLUS_D2D is equal to 0
	*/
	typedef void *VFontNativeRef;
#endif

BEGIN_TOOLBOX_NAMESPACE

// Needed declarations
class VFont;
class VFontMetrics;
class VGraphicContext;

/** generic font type definition (as defined by CSS2 W3C: see http://www.w3.org/TR/CSS2/fonts.html#generic-font-families) */
typedef enum eGenericFont
{
	GENERIC_FONT_SERIF = 0,			//for instance "Times New Roman" (proportional)
	GENERIC_FONT_SANS_SERIF = 1,	//for instance "Arial"			(proportional)
	GENERIC_FONT_CURSIVE = 2,		//for instance "Adobe Poetica"	(handwritten-like)
	GENERIC_FONT_FANTASY = 3,		//for instance "Critter"	(mixing characters and decorations)
	GENERIC_FONT_MONOSPACE = 4		//for instance "Courier" (not proportional)

} eGenericFont;

class XTOOLBOX_API VFont : public VObject, public IRefCountable
{
friend class VFontMgr;
public:
#if VERSIONWIN
								VFont (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, StdFont inStdFont = STDF_NONE, GReal inPixelSize = 0.0f);
#else
								VFont (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, StdFont inStdFont = STDF_NONE);
#endif
			virtual						~VFont ();
	
			/** return true if the specified font exists on the current system 
			@remarks
				does not create font
			*/
	static	bool				FontExists(const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI = 0, const VGraphicContext *inGC = NULL);

			/** return the best existing font family name on the current system which matches the specified generic ID 
			@see
				eGenericFont
			*/
	static	void				GetGenericFontFamilyName(eGenericFont inGenericID, VString& outFontFamilyName, const VGraphicContext *inGC = NULL);

	// Creation 
	static	VFont*				RetainFont (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize, const GReal inDPI = 0, bool inReturnNULLIfNotExist = false) { assert(sManager != NULL); return sManager->RetainFont(inFontFamilyName, inFace, inSize, inDPI, inReturnNULLIfNotExist); };
	static	VFont*				RetainStdFont (StdFont inFont) { assert(sManager != NULL); return sManager->RetainStdFont(inFont); }

	static	VFont*				RetainFontFromStream ( VStream *inStream);
			VError				WriteToStream( VStream *ioStream, sLONG inParam = 0) const;

			VFont*				DeriveFontSize (GReal inSize, const GReal inDPI = 0) const;
			VFont*				DeriveFontFace (const VFontFace& inFace) const;

			void				ApplyToPort (PortRef inPort, GReal inScale);
	
			// Font matching
			bool				Match (const VString& inFontFamilyName, const VFontFace& inFace, GReal inSize) const;
			bool				Match (StdFont inFont) const;
	
			// Accessors
			const VString&		GetName () const		{ return fFamilyName; }
			const VFontFace&	GetFace () const		{ return fFace; }

			GReal				GetSize () const		{ return fSize; }

#if VERSIONWIN
			GReal				GetPixelSize () const;
#else
			GReal				GetPixelSize () const		{ return fSize; }
#endif

			bool				GetFontID(sWORD& outID)const	{outID=fFontID;return fFontIDValid;}
			void				SetFontID(sWORD inID)			{fFontID = inID;fFontIDValid=true;}

			
			FontRef				GetFontRef () const		{ return fFont.GetFontRef(); }
			Boolean				IsTrueTypeFont() { return fFont.IsTrueTypeFont();}

			/**	create text style from the current font
			@remarks
				range is not set
			*/
			VTextStyle		   *CreateTextStyle( const GReal inDPI = 72.0f) const;	

			/**	create text style from the specified font name, font size & font face
			@remarks
				if font name is empty, returned style inherits font name from parent
				if font face is equal to UNDEFINED_STYLE, returned style inherits style from parent
				if font size is equal to UNDEFINED_STYLE, returned style inherits font size from parent
			*/
			static VTextStyle  *CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI = 0);		

#if VERSIONWIN
#if ENABLE_D2D
#if GRAPHIC_MIXED_GDIPLUS_D2D
			//GDIPlus & D2D impl

			VFontNativeRef		GetFontNative() const	
			{ 
				return (VFontNativeRef)fFont.GetGDIPlusFont();
			}
			Gdiplus::Font*		GetGDIPlusFont() const	{ return fFont.GetGDIPlusFont(); }
			IDWriteFont*		GetDWriteFont() const	{ return fFont.GetDWriteFont(); }
			IDWriteTextFormat*	GetDWriteTextFormat() const { return fFont.GetDWriteTextFormat(); }

			/** create text layout from the specified text string & options 
			@remarks
				inWidth & inHeight are in DIP
			*/
			IDWriteTextLayout *CreateTextLayout(ID2D1RenderTarget *inRT, const VString& inText, const bool isAntialiased = true, const GReal inWidth = DWRITE_TEXT_LAYOUT_MAX_WIDTH, const GReal inHeight = DWRITE_TEXT_LAYOUT_MAX_HEIGHT, AlignStyle inHoriz = AL_LEFT, AlignStyle inVert = AL_TOP, TextLayoutMode inMode = TLM_NORMAL, VTreeTextStyle *inStyles = NULL, const GReal inRefDocDPI = 72.0f, const bool inUseCache = true)
			{
				return fFont.CreateTextLayout( inRT, inText, isAntialiased, inWidth, inHeight, inHoriz, inVert, inMode, inStyles, inRefDocDPI, inUseCache);
			}
#else
			//D2D impl only

			VFontNativeRef		GetFontNative() const	
			{ 
				return (VFontNativeRef)fFont.GetDWriteFont();
			}
			Gdiplus::Font*		GetGDIPlusFont() const	{ xbox_assert(false); return NULL; }
			IDWriteFont*		GetDWriteFont() const	{ return fFont.GetDWriteFont(); }
			IDWriteTextFormat*	GetDWriteTextFormat() const { return fFont.GetDWriteTextFormat(); }

			/** create text layout from the specified text string & options 
			@remarks
				inWidth & inHeight are in DIP
			*/
			IDWriteTextLayout *CreateTextLayout(ID2D1RenderTarget *inRT, const VString& inText, const bool isAntialiased = true, const GReal inWidth = DWRITE_TEXT_LAYOUT_MAX_WIDTH, const GReal inHeight = DWRITE_TEXT_LAYOUT_MAX_HEIGHT, AlignStyle inHoriz = AL_LEFT, AlignStyle inVert = AL_TOP, TextLayoutMode inMode = TLM_NORMAL, VTreeTextStyle *inStyles = NULL, const GReal inRefDocDPI = 72.0f, const bool inUseCache = true)
			{
				return fFont.CreateTextLayout( inRT, inText, isAntialiased, inWidth, inHeight, inHoriz, inVert, inMode, inStyles, inRefDocDPI, inUseCache);
			}
#endif
			/**	create text style for the specified text position 
			@remarks
				range of returned text style is set to the range of text which has same format as text at specified position
			*/
			VTextStyle *CreateTextStyle( IDWriteTextLayout *inLayout, const sLONG inTextPos = 0.0f)
			{
				return fFont.CreateTextStyle( inLayout, inTextPos);
			}

			/**	create text styles tree for the specified text range 
			@remarks
				text styles ranges will be relative to input start pos
			*/
			VTreeTextStyle *CreateTreeTextStyle( IDWriteTextLayout *inLayout, const sLONG inTextPos = 0.0f, const sLONG inTextLen = 1.0f)
			{
				return fFont.CreateTreeTextStyle( inLayout, inTextPos, inTextLen);
			}
#else
			//GDIPlus impl only

			VFontNativeRef		GetFontNative() const	
			{ 
				return (VFontNativeRef)fFont.GetGDIPlusFont();
			}
			Gdiplus::Font*		GetGDIPlusFont() const	{ return fFont.GetGDIPlusFont(); }
			IDWriteFont*		GetDWriteFont() const	{ xbox_assert(false); return NULL; }
			IDWriteTextFormat*	GetDWriteTextFormat() const { xbox_assert(false); return NULL;  }
			
			/** create text layout from the specified text string & options 
			@remarks
				inWidth & inHeight are in DIP
			*/
			IDWriteTextLayout *CreateTextLayout(ID2D1RenderTarget *inRT, const VString& inText, const bool isAntialiased = true, 
												const GReal inWidth = DWRITE_TEXT_LAYOUT_MAX_WIDTH, const GReal inHeight = DWRITE_TEXT_LAYOUT_MAX_HEIGHT, 
												AlignStyle inHoriz = AL_LEFT, AlignStyle inVert = AL_TOP, 
												TextLayoutMode inMode = TLM_NORMAL, VTreeTextStyle *inStyles = NULL, 
												const GReal inRefDocDPI = 72.0f, const bool inUseCache = true)
			{
				xbox_assert(false); return NULL;
			}
#endif
#else
			VFontNativeRef		GetFontNative() const	{ return fFont.GetFontRef(); }
#endif
			// Native operator
			operator FontRef () const					{ return GetFontRef(); }
	
			// Utilities
	static	bool				Init ();
	static	void				DeInit ();
	
	static	xFontnameVector*		GetFontNameList (bool inWithScreenFonts = true) { xbox_assert(sManager != NULL); return sManager->GetFontNameList(inWithScreenFonts); };	

			const XFontImpl&	GetImpl () const { return fFont; };

#if VERSIONWIN
			GReal _GetPixelSize() const { return fPixelSize; };
#endif
protected:
	static VFontMgr*			sManager;
	
			VString				fFamilyName;
			VFontFace			fFace;
			GReal				fSize;
#if VERSIONWIN
			mutable GReal		fPixelSize;
#endif
			StdFont				fStdFont;
			
			// optimization pour l'ancien code qui utilsie tj des fontid quickdraw
			// pour eviter d'appler GetFnum trop souvent, on stock l'id quickdraw dans la vfont une fois pour toute.
			// Cet id est conservé lors de la derivation de la fonte, il n'est jamais utilisé en interne. 
			bool				fFontIDValid; 
			sWORD				fFontID;
			
			XFontImpl			fFont;
};

/** font metrics
@remarks
	for D2D/DWrite impl, metrics are in DIP (1/96e inches)
	for GDIPlus & GDI, metrics are in pixels
	for Quartz2D, metrics are in pt = 1/72e inches
*/
class XTOOLBOX_API VFontMetrics : public VObject
{
public:
							// Copies the TextRenderingMode set in VGraphicContext
							VFontMetrics( VFont* inFont, const VGraphicContext* inContext);
	virtual					~VFontMetrics();
	
							/** copy constructor */
							VFontMetrics( const VFontMetrics& inFontMetrics);

	// Computation
	GReal					GetTextWidth( const VString& inString)			{ return inString.IsEmpty() ? (GReal) 0.0f : fMetrics.GetTextWidth( inString);}
	GReal					GetCharWidth( UniChar inChar)					{ return fMetrics.GetCharWidth( inChar);}

	// return true if the inMousePos is in the text
	// if the MousePos is in the text :
	//		ioTextPos is added by the character position nearest of inMousePos
	//		ioPixPos is added by this character position in pixels.
	// else
	//		ioTextPos is added by the len of the text in chars
	//		ioPixPos is added by the len of the text in pixels

	bool					MousePosToTextPos( const VString& inString, GReal inMousePos, sLONG& ioTextPos, GReal& ioPixPos);

	void					MeasureText( const VString& inString, std::vector<GReal>& outOffsets);
	
	GReal					GetAscent() const				{ return fAscent;}
	GReal					GetDescent() const				{ return fDescent;}
	GReal					GetLeading() const				{ return fExternalLeading;}			
	GReal					GetInternalLeading() const		{ return fInternalLeading;} //remark: internal leading is included in ascent
	GReal					GetExternalLeading() const		{ return fExternalLeading;}

	GReal					GetTextHeight() const			{ return fAscent + fDescent;}
	GReal					GetLineHeight() const			{ return fAscent + fDescent + fExternalLeading;}
	
	// donne la hauteur de la police en utilisant la meme (presque) metode d'arrondi quickdraw 
	sLONG					GetNormalizedTextHeight() const;
	sLONG					GetNormalizedLineHeight() const	;
	
	// Accessors
	VFont*					GetFont() const					{ return fFont; }
	const VGraphicContext*		GetContext() const				{ return fContext; }
	
	
	/** scale metrics by the specified factor */
	void					Scale( GReal inScaling); 
	
	XFontMetricsImpl&		GetImpl()						{ return fMetrics; }

	/** use legacy (GDI) context metrics or actuel context metrics 
	@remarks
		if actual context is D2D: if true, use GDI font metrics otherwise use Direct2D font metrics 
		any other context: use GDI metrics on Windows

		if context text rendering mode has TRM_LEGACY_ON, inUseLegacyMetrics is replaced by true
		if context text rendering mode has TRM_LEGACY_OFF, inUseLegacyMetrics is replaced by false
		(so context text rendering mode overrides this setting)
	*/
	void					DoUseLegacyMetrics( bool inUseLegacyMetrics = false);

	bool					UseLegacyMetrics() const
	{
		return fUseLegacyMetrics;
	}

	bool					GetDesiredUseLegacyMetrics() const
	{
		return fDesiredUseLegacyMetrics;
	}
private:
	const VGraphicContext*		fContext;
	VFont*					fFont;
	GReal					fAscent;
	GReal					fDescent;
	GReal					fInternalLeading;
	GReal					fExternalLeading;
	XFontMetricsImpl		fMetrics;
	bool					fDesiredUseLegacyMetrics;
	bool					fUseLegacyMetrics;
};


END_TOOLBOX_NAMESPACE

#endif
