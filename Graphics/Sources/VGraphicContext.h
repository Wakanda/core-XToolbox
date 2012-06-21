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
#ifndef __VGraphicContext__
#define __VGraphicContext__

#include "Graphics/Sources/VGraphicsTypes.h"
#include "Graphics/Sources/VGraphicSettings.h"
#include "Graphics/Sources/VRect.h"
#include "Graphics/Sources/VBezier.h"
#if VERSIONWIN
#include <gdiplus.h> 
#else
#include "Graphics/Sources/VQuicktimeSDK.h"
#include "Graphics/Sources/V4DPictureIncludeBase.h"
#include "Graphics/Sources/V4DPictureTools.h"
#endif

BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VAffineTransform;
class VRect;
class VRegion;
class VIcon;
class VPolygon;
class VGraphicPath;
class VFileBitmap;
class VPictureData;
class VTreeTextStyle;
class VStyledTextBox;
// Brush synchronisation flags
typedef struct {
	uWORD	fTextModeChanged : 1;		// Brush state flags (used by VGraphicContext
	uWORD	fDrawingModeChanged : 1;	//	to specify which brushes have been overwritten)
	uWORD	fLineSizeChanged : 1;
	uWORD	fLineBrushChanged : 1;
	uWORD	fFillBrushChanged : 1;
	uWORD	fTextBrushChanged : 1;
	uWORD	fBrushPosChanged : 1;
	uWORD	fFontChanged : 1;
	uWORD	fKerningChanged : 1;
	uWORD	fPixelTransfertChanged : 1;
	uWORD	fTextPosChanged : 1;
	uWORD	fTextRenderingModeChanged : 1;
	uWORD	fTextMeasuringModeChanged : 1;
	uWORD	fUnused : 1;				// Reserved for future use (set to 0)
	uWORD	fCustomFlag1 : 1;			// Reserved for custom use. Those flags are never
	uWORD	fCustomFlag2 : 1;			//	modified by VGraphicContext
	uWORD	fCustomFlag3 : 1;
} VBrushFlags;

typedef enum
{
	eDefaultGraphicContext = 0,
	//on Windows, GDIPlus graphic context
	//on Mac OS, Quartz graphic context
	eAdvancedGraphicContext,
	//on Windows, Direct2D graphic context on Vista+, otherwise GDIPlus graphic context
	//on Mac OS, Quartz graphic context
	//(will switch on software only graphic context if hardware is not available)
	eAdvancedHardwareGraphicContext,
	eCountGraphicContext
}GraphicContextType;

#if VERSIONWIN
//Gdiplus impl: Gdiplus::Graphics *
//Direct2D impl: ID2D1RenderTarget *
typedef void *VGraphicContextNativeRef;
#else
#if VERSIONMAC
typedef CGContextRef VGraphicContextNativeRef;
#endif
#endif

#if VERSIONWIN
/** class used to encapsulate GDI offscreen DC rendering & the underlaying layer 
@remarks
	underlaying BITMAP layer is preserved until class destruction:
	it is resized or recreated only if needed to limit allocation/deallocation

	this class is used for instance to implement double-buffering for form contexts on platform where D2D is not/partially available - as on XP:
	actually double-buffering using GDI is still faster & less buggy if D2D is not available 
	especially because of GDI ClearType (GDIPlus context layers are a bit slower and buggy with texts because of GDI ClearType)
	so GDI is still better if D2D is not available

	recommended usage is to use Begin/End methods which take care of the offscreen logic

	it is encapsulated in class VImageOffScreen in order to be used by gc methods VGraphicContext::BeginLayerOffScreen/EndLayerOffScreen
*/
class VGDIOffScreen : public XBOX::VObject
{
public:
	VGDIOffScreen()
	{
		fHDCOffScreen = NULL;
		fHBMOffScreen = NULL;
		fOldFont = NULL;
		fOldBmp = NULL;
		fActive = false;
	}
	virtual ~VGDIOffScreen()
	{
		xbox_assert(!fActive && !fHDCOffScreen);
		Clear();
	}

	void Clear()
	{
		xbox_assert(!fActive && fHDCOffScreen == NULL && fOldFont == NULL && fOldBmp == NULL);
		fMaxBounds.SetEmpty();
		if (fHBMOffScreen)
		{
			::DeleteBitmap(fHBMOffScreen);
			fHBMOffScreen = NULL;
		}
	}

	/** begin render offscreen 
	@remarks
		returns offscreen hdc or the passed parent hdc if failed 
	*/
	HDC Begin(HDC inHDC, const XBOX::VRect& inBounds, bool *inNewBmp = false, bool inInheritParentClipping = false);

	/** end render offscreen & blit offscreen in parent gc */
	void End(HDC inHDCOffScreen);

	/** return true if fHBMOffScreen needs to be cleared or resized if using inBounds as dest bounds on next call to Draw or Begin */
	bool ShouldClear( HDC inHDC, const XBOX::VRect& inBounds);

	/** draw directly fHBMOffScreen to dest dc at the specified position 
	@remarks

		caution: blitting is done in device space so bounds are transformed to device space before blitting
	*/
	bool Draw( HDC inHDC, const XBOX::VRect& inDestBounds, bool inNeedDeviceBoundsEqualToCurLayerBounds = false);

	/** get GDI bitmap 
	@remarks
		bitmap is owned by the class: do not destroy it...
		this will return a valid handle only if at least Begin has been called once before (the bitmap is created only if needed)
		you should not select the bitmap in a context between Begin/End because it is selected yet in the offscreen context

		normally you should not use this method but for debugging purpose:
		recommended usage is to use Begin/End methods which take care of the offscreen logic
	*/
	HBITMAP GetHBITMAP() const { return fHBMOffScreen; }

private:
	bool fActive;

	XBOX::VRect fMaxBounds;	  //internal bitmap bounds (greater or equal to layer bounds)
	XBOX::VRect fLayerBounds; //layer actual bounds in device space

	HDC fHDCParent;

	HFONT fOldFont;
	HBITMAP fOldBmp;
	XBOX::VRect fBounds;	  //current clip bounds (bounds used for blitting)
	bool fNeedResetCTMParent;
	XFORM fCTMParent;

	HBITMAP fHBMOffScreen; 
	HDC fHDCOffScreen;
};
#endif

/** gc offscreen image class */
class VImageOffScreen: public VObject, public IRefCountable
{
public:
	/** create offscreen image compatible with the specified gc */
	VImageOffScreen( VGraphicContext *inGC, GReal inWidth = 1.0f, GReal inHeight = 1.0f);

	const VRect GetBounds( VGraphicContext *inGC) const;
#if VERSIONWIN
	/** GDIPlus gc compatible offscreen */

	VImageOffScreen(Gdiplus::Bitmap *inBmp);
	void Set(Gdiplus::Bitmap *inBmp);

	bool IsGDIPlusImpl() const { return fGDIPlusBmp != NULL; }
	operator Gdiplus::Bitmap *() const { return fGDIPlusBmp; }

	/** GDI gc compatible offscreen */

	bool IsGDIImpl() const { return fGDIOffScreen.GetHBITMAP() != NULL; }
	VGDIOffScreen *GetGDIOffScreen() { return &fGDIOffScreen; }
	HBITMAP GetHBITMAP() const { return fGDIOffScreen.GetHBITMAP(); }

#if ENABLE_D2D
	/** D2D gc compatible offscreen */

	VImageOffScreen(ID2D1BitmapRenderTarget *inBmpRT, ID2D1RenderTarget *inParentRT = NULL);
	void SetAndRetain(ID2D1BitmapRenderTarget *inBmpRT, ID2D1RenderTarget *inParentRT = NULL);

	bool IsD2DImpl() const { return fD2DBmpRT != NULL; }

	operator ID2D1BitmapRenderTarget *() const { return fD2DBmpRT; }
	ID2D1RenderTarget *GetParentRenderTarget() const { return fD2DParentRT; }
	void ClearParentRenderTarget();
#else
	bool IsD2DImpl() const { return false; }
#endif
#endif

#if VERSIONMAC
	/** Quartz2D gc compatible offscreen */

	VImageOffScreen( CGLayerRef inLayerRef);
	void SetAndRetain(CGLayerRef inLayerRef);

	bool IsD2DImpl() const { return false; }
	bool IsGDIPlusImpl() const { return false; }

	operator CGLayerRef() const { return fLayer; }
#endif

	virtual ~VImageOffScreen();

	void Clear();

private:
#if VERSIONWIN
	/** GDI gc compatible offscreen */
	VGDIOffScreen fGDIOffScreen;
	/** GDIPlus gc compatible offscreen */
	Gdiplus::Bitmap *fGDIPlusBmp;
#if ENABLE_D2D
	/** D2D gc compatible offscreen */
	ID2D1RenderTarget *fD2DParentRT;
	ID2D1BitmapRenderTarget *fD2DBmpRT;
#endif
#endif

#if VERSIONMAC
	/** Quartz2D gc compatible offscreen */
	CGLayerRef fLayer;
#endif
};
typedef VImageOffScreen *VImageOffScreenRef;


/** class used to encapsulate VGraphicContext offscreen layer rendering & the underlaying layer
@remarks 
	underlaying layer is preserved until class destruction:
	it is resized or recreated only if needed to limit allocation/deallocation

	recommended usage is to use Begin/End methods which take care of the offscreen logic
*/
class XTOOLBOX_API VGCOffScreen : public XBOX::VObject
{
public:
	/** delegate to bind a alternate drawing derived gc to the offscreen 
	@param inGC
		parent gc (which is actually the gc bound to the offscreen)
	@param inBounds
		drawing clip bounds
	@param inTransparent
		needs transparency or not
	@param outContextParent
		if delegate returns a valid derived gc, it MUST return in outContextParent = inGC->BeginUsingParentContext() in order to lock (if appropriate) the inGC parent context
		(End() will automatically call inGC->EndUsingParentContext( outContextParent))
	@param inCurDrawingGC
		previous derived gc if it has been preserved in the class (for instance D2D DC gc can be rebound to another DC context): 
		if inCurDrawingGC is not NULL, delegate should decide either to return RetainRefCountable(inCurDrawingGC) or another derived gc
	@param inUserData
		user data passed to SetDrawingGCDelegate
	@remarks
		delegate should return NULL if it failed or does not want to derive the context
	*/
	typedef VGraphicContext* (*fnOffScreenDrawingGCDelegate)(XBOX::VGraphicContext *inGC, const XBOX::VRect& inBounds, bool inTransparent, XBOX::ContextRef &outContextParent, XBOX::VGraphicContext *inCurDrawingGC, void *inUserData);

	VGCOffScreen()
	{
		fLayerOffScreen = NULL;
		fActive = false;
		fDrawingGCDelegate = NULL;
		fDrawingGCDelegateUserData = NULL;
		fDrawingGC = NULL;
	}
	virtual ~VGCOffScreen()
	{
		Clear();
	}

	void Clear()
	{
		ReleaseRefCountable(&fDrawingGC);
		xbox_assert(!fActive);
		XBOX::ReleaseRefCountable(&fLayerOffScreen);
	}

	/** begin render offscreen 
	@remarks
		return true if a new layer is created, false if current layer is used
		(true for instance if current transformed region bounds have changed)

		it is caller responsibility to clear or not the layer after call to Begin
	*/
	bool Begin(XBOX::VGraphicContext *inGC, const XBOX::VRect& inBounds, bool inTransparent = true, bool inLayerInheritParentClipping = false, const XBOX::VRegion *inNewLayerClipRegion = NULL, const XBOX::VRegion *inReUseLayerClipRegion = NULL);

	/** end render offscreen & blit offscreen in parent gc */
	void End(XBOX::VGraphicContext *inGC);

	/** draw internal offscreen layer using the specified bounds 
	@remarks
		this is like calling Begin & End without any drawing between
		so it is faster because only the offscreen bitmap is rendered
	 
		if transformed bounds size is not equal to background layer size, return false and do nothing 
	    so only transformed bounds translation is allowed.
		if the internal offscreen is NULL, return false and do nothing

		ideally if offscreen content is not dirty, you should first call this method
		and only call Begin & End if it returns false
	 */
	bool Draw(XBOX::VGraphicContext *inGC, const VRect& inBounds);
	
	/** return true if the internal offscreen layer needs to be cleared or resized if using inBounds as dest bounds on next call to Draw or Begin/End
	@remarks
		return true if transformed input bounds size is not equal to the offscreen layer size
	    so only transformed bounds translation is allowed.
		return true if the offscreen layer is not compatible with the passed gc
	 */
	bool ShouldClear(XBOX::VGraphicContext *inGC, const VRect& inBounds);


	/** set a delegate to bind a alternate drawing derived gc to the offscreen 
	@remarks
		must be called before Begin()
	*/
	void SetDrawingGCDelegate( fnOffScreenDrawingGCDelegate inDelegate, void *inUserData = NULL)
	{
		if (!testAssert(!fActive))
			return;

		if (fDrawingGCDelegate == inDelegate && fDrawingGCDelegateUserData == inUserData)
			return;

		fDrawingGCDelegate = inDelegate;
		fDrawingGCDelegateUserData = inUserData;
		ReleaseRefCountable(&fDrawingGC);
	}

	/** get current derived gc if any is bound to the offscreen 
	@remarks
		should be used for drawing between Begin/End if not NULL
	*/
	VGraphicContext *GetDrawingGC()
	{
		if (fActive)
			return fDrawingGC;
		else
			return NULL;
	}

protected:
	bool						fActive;
	XBOX::VImageOffScreenRef	fLayerOffScreen;
	bool						fTransparent;

	/** optional delegate to bind a alternate drawing derived gc to the offscreen */
	fnOffScreenDrawingGCDelegate fDrawingGCDelegate;
	void						*fDrawingGCDelegateUserData;

	/** optional derived gc bound to the back buffer for drawing (lifetime is equal to the back buffer lifetime) */
	VGraphicContext				*fDrawingGC; 
	ContextRef					fDrawingGCParentContext;
	
	bool						fNeedRestoreClip;
};


/** line dash pattern type definition */
typedef std::vector<GReal> VLineDashPattern;

class VGraphicImage;
class VGraphicFilterProcess;

/** opaque type for impl-specific bitmap ref:
	D2D graphic context: IWICBitmap *
	Gdiplus graphic context: Gdiplus Bitmap *
	Quartz2D graphic context: CGImageRef
*/
typedef void *NATIVE_BITMAP_REF;

#if VERSIONWIN
/** static class for GDI transform context save/restore 
@remarks
	used only by Windows impl GDIPlus & Direct2D 
	to ensure GDI context inherits GDIPlus/Direct2D context transform
*/
class HDC_TransformSetter
{
	public:
	HDC_TransformSetter(HDC inDC,VAffineTransform& inMat);
	HDC_TransformSetter(HDC inDC);
	virtual ~HDC_TransformSetter();
	void SetTransform(const VAffineTransform& inMat);
	void ResetTransform();
	private:
	HDC fDC;
	DWORD fOldMode;
	XFORM fOldTransform;
};

class XTOOLBOX_API HDC_MapModeSaverSetter
{
	public:
	HDC_MapModeSaverSetter(HDC inDC,int inMapMode=0);
	virtual ~HDC_MapModeSaverSetter();
	
	private:
	HDC		fDC;
	int		fMapMode;
	SIZE	fWindowExtend;
	SIZE	fViewportExtend;
};


#endif

#if ENABLE_D2D
//FIXME: Direct2D does not allow direct layout update so we fallback to GDI if text is too big
#define VTEXTLAYOUT_DIRECT2D_MAX_TEXT_SIZE	512
#endif

class XTOOLBOX_API VTextLayout;


class XTOOLBOX_API VGraphicContext : public VObject, public IRefCountable
{
	friend class VTextLayout;
protected:
	mutable bool fMaxPerfFlag;
	
	/** high quality antialiasing mode */
	mutable bool fIsHighQualityAntialiased;

	/** high quality text rendering mode */
	mutable TextRenderingMode fHighQualityTextRenderingMode;

	/** current fill rule (used to fill with solid color or patterns) */
	mutable FillRuleType fFillRule;
	
	/** char kerning override: 0.0 for auto kerning (default) 
	@remarks
		auto-kerning if fCharKerning+fCharSpacing == 0.0 (default)
		otherwise kerning is set to fCharKerning+fCharSpacing
	*/	
	mutable GReal		fCharKerning; 
	
	/** char spacing override: 0.0 for normal spacing (default) 
	@remarks
		auto-kerning if fCharKerning+fCharSpacing == 0.0 (default)
		otherwise kerning is set to fCharKerning+fCharSpacing
	*/	
	mutable GReal		fCharSpacing; 

	/** layer implementation */
	std::vector<VRect>						fStackLayerBounds;		//layer bounds stack
	std::vector<bool>						fStackLayerOwnBackground;	//layer back buffer owner
	std::vector<VImageOffScreenRef>			fStackLayerBackground;	//layer back buffer stack
	std::vector<VGraphicContextNativeRef>	fStackLayerGC;			//layer graphic context stack
	std::vector<GReal>						fStackLayerAlpha;	    //layer alpha stack
	std::vector<VGraphicFilterProcess *>	fStackLayerFilter;		//layer filter stack

	/** true: text will use current fill & stroke brush (fill brush for normal & stroke brush for outline)
		false: text will use its own brush (default for best compatibility: no outline support)
	@remarks
		must be set to true in order to apply a gradient to the text
		or to draw text outline	

		TODO: implement outlined text (but for now only SVG component might need it)
	*/
	mutable bool fUseShapeBrushForText;

	/** crispEdges status 
	@remarks
		when crispEdges status is enabled, shape floating point coordinates are modified in transformed space
		in order to align shape coordinates with device pixels 
		(according to pixel offset mode which should be half pixel offset for all impls):
		this mode ensures shapes are drawed in device space at the same pixel position
		on all platforms (actually this is not true for more complex shapes like paths with bezier curves 
		because for complex	shapes it is not possible to get the same rendering on any platform
		but it is true for shapes which are made mainly with lines);
		this mode helps too to reduce antialiasing artifacts on horizontal & vertical lines

		this mode has a performance cost & it can deform shape especially if device resolution is 
		not the same as shape resolution (ie if transform scale from shape user space to device space is not a integer value
		or if shape uses not integer values for coordinates) so default status is disabled;
		ideally it should be enabled while drawing at device resolution when platform compatibility is required

		this rendering mode is not compatible with QD-compatible methods so you should not enable it
		while using these methods.
	*/
	mutable bool fShapeCrispEdgesEnabled;

	/** allow crispEdges algorithm to be applied on paths 
	@remarks
		by default it is set to false

		by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
		it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
		on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
	*/
	mutable bool fAllowCrispEdgesOnPath;

	/** allow crispEdges algorithm to be applied on paths with bezier curves
	@remarks
		by default it is set to false because modifying bezier coordinates can result in bezier curve distortion 
		because of control points error discrepancies
		so caller should only enable it on a per-case basis

		by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
		it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
		on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
	*/
	mutable bool fAllowCrispEdgesOnBezier;

	bool			fHairline;

	bool			fPrinterScale;
	GReal			fPrinterScaleX;
	GReal			fPrinterScaleY;

#if VERSIONWIN
	static void	_DrawLegacyTextBox (HDC inHDC, const VString& inString, const VColor& inColor, AlignStyle inHoriz, AlignStyle inVert, const VRect& inLocalBounds, TextLayoutMode inMode = TLM_NORMAL);
	static void	_GetLegacyTextBoxSize(HDC inHDC, const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode = TLM_NORMAL);

	void	_DrawLegacyTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inLocalBounds, TextLayoutMode inMode = TLM_NORMAL);
	void	_GetLegacyTextBoxSize( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inLayoutMode = TLM_NORMAL) const;

	static void	_DrawLegacyStyledText( HDC inHDC, const VString& inText, VFont *inFont, const VColor& inColor, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	static void _GetLegacyStyledTextBoxBounds( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f);
	static void _GetLegacyStyledTextBoxCaretMetricsFromCharIndex( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	static bool _GetLegacyStyledTextBoxCharIndexFromCoord( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	static void _GetLegacyStyledTextBoxRunBoundsFromRange( HDC inHDC, const VString& inText, VFont *inFont, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);

	void _DrawLegacyStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	void _GetLegacyStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f);
	void _GetLegacyStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	bool _GetLegacyStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
	void _GetLegacyStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
#endif

	/** draw text layout 
	@remarks
		text layout left top origin is set to inTopLeft
		text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
		text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
	*/
#if VERSIONWIN
	virtual void	_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft = VPoint());
#else
	virtual void	_DrawTextLayout( VTextLayout *inTextLayout, const VPoint& inTopLeft = VPoint()) {}
#endif
	
	/** return text layout bounds 
		@remark:
			text layout origin is set to inTopLeft
			on output, outBounds contains text layout bounds including any glyph overhangs

			on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
			(if max width or max height is equal to 0.0f, it is assumed to be infinite)

			if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
			(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
			 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)
	*/
#if VERSIONWIN
	virtual void	_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);
#else
	virtual void	_GetTextLayoutBounds( VTextLayout *inTextLayout, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false) {}
#endif


	/** return text layout run bounds from the specified range 
	@remarks
		text layout origin is set to inTopLeft
	*/
#if VERSIONWIN
	virtual void	_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);
#else
	virtual void	_GetTextLayoutRunBoundsFromRange( VTextLayout *inTextLayout, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1) {}
#endif


	/** return the caret position & height of text at the specified text position in the text layout
	@remarks
		text layout origin is set to inTopLeft

		text position is 0-based

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
		if text layout is drawed at inTopLeft

		by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
		but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
#if VERSIONWIN
	virtual void	_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);
#else
	virtual void	_GetTextLayoutCaretMetricsFromCharIndex( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true) {}
#endif


	/** return the text position at the specified coordinates
	@remarks
		text layout origin is set to inTopLeft (in gc user space)
		the hit coordinates are defined by inPos param (in gc user space)

		output text position is 0-based

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
#if VERSIONWIN
	virtual bool _GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex);
#else
	virtual bool _GetTextLayoutCharIndexFromPos( VTextLayout *inTextLayout, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex) {return false;}
#endif

	/* update text layout
	@remarks
		build text layout according to layout settings & current graphic context settings

		this method is called only from VTextLayout::_UpdateTextLayout
	*/
#if VERSIONWIN
	virtual void _UpdateTextLayout( VTextLayout *inTextLayout);
#else
	virtual void _UpdateTextLayout( VTextLayout *inTextLayout) { xbox_assert(inTextLayout); /*inTextLayout->fLayoutIsValid = false;*/ }	// COMMENTED FOR BUILD FIX
#endif

	/** apply custom alignment to input styles & return new styles 
	@remarks
		return NULL if input style is not modified by new alignment
	*/
	static VTreeTextStyle *_StylesWithCustomAlignment(const VString& inText, VTreeTextStyle *inStyle = NULL, AlignStyle inHoriz = AL_DEFAULT, AlignStyle inVert = AL_DEFAULT);


public:
	/** default filter layer inflating offset 
	@remarks
		filter bounds are inflated (in device space) by this offset 
		in order to reduce border artifacts with some filter effects
		(like gaussian blur)
	*/
	static const GReal sLayerFilterInflatingOffset;

public:
			VGraphicContext ();
			VGraphicContext (const VGraphicContext& inOriginal);
	
	static VGraphicContext* CreateBitmapContext(const VRect& inBounds);
	
	virtual	~VGraphicContext ();
	
	/** create graphic context binded to the specified native context ref:
		on Vista+, create a D2D graphic context
		on XP, create a Gdiplus graphic context
		on Mac OS, create a Quartz2D graphic context
	@param inContextRef
		native context ref
		CAUTION: on Windows, Direct2D cannot draw directly to a printer HDC so while printing & if D2DIsEnabled()
				 you should either call CreateBitmapGraphicContext to draw in a bitmap graphic context
				 & then print the bitmap with DrawBitmapToHDC or use explicitly another graphic context like GDIPlus or GDI
				 (use the desired graphic context constructor for that)
	@param inBounds (used only by D2D impl - see VWinD2DGraphicContext)
		context area bounds 
	@param inPageScale (used only by D2D impl - see remarks below)
		desired page scaling 
		(default is 1.0f)
	@remarks
		if IsD2DImpl() return true, 

			caller can call BindHDC later to bind or rebind new dc.

			all coordinates are expressed in device independent units (DIP) 
			where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
			we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
			(It is because on Windows, 4D app is not dpi-aware and we must keep 
			this limitation for backward compatibility with previous impl)

			render target DPI is computed from inPageScale using the following formula:
			render target DPI = 96.0f*inPageScale 
			(yes render target DPI is a floating point value)

			bounds are expressed in DIP (so with DPI = 96): 
			D2D internally manages page scaling by scaling bounds & coordinates by DPI/96
	*/
	static VGraphicContext* Create(ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale = 1.0f, const bool inTransparent = true, const bool inSoftwareOnly = false);

	/** create a shared graphic context binded initially to the specified native context ref 
	@param inUserHandle
		user handle used to identify the shared graphic context
	@param inContextRef
		native context ref
		CAUTION: on Windows, Direct2D cannot draw directly to a printer HDC so while printing & if D2DIsEnabled()
				 you should either call CreateBitmapGraphicContext to draw in a bitmap graphic context
				 & then print the bitmap with DrawBitmapToHDC or use explicitly another graphic context like GDIPlus or GDI
				 (use the desired graphic context constructor for that)
	@param inBounds (used only by D2D impl - see VWinD2DGraphicContext)
		context area bounds 
	@param inPageScale (used only by D2D impl - see remarks below)
		desired page scaling 
		(default is 1.0f)
	@remarks
		if Direct2D is not available, method will behave the same than VGraphicContext::Create (param inUserHandle is ignored)
		so this method is mandatory only for Direct2D impl (but can be used safely for other impls so you can use same code in any platform)

		this method should be used if you want to re-use the same Direct2D graphic context with different GDI contexts:
		it creates a single Direct2D graphic context mapped to the passed user handle:
		- the first time you call this method, it allocates the graphic context, bind it to the input HDC & store it to a shared internal table (indexed by the user handle)
		- if you call this method again with the same user handle & with a different HDC or bounds, it rebinds the render target to the new HDC & bounds
		  which is more faster than to recreate the render target especially if new bounds are less or equal to the actual bounds

		caution: to release the returned graphic context, call as usual Release method of the graphic context
		
		if you want to remove the returned graphic context from the shared internal table too, call also RemoveShared 
		(actually you do not need to call this method because shared graphic contexts are auto-released by VGraphicContext::DeInit
		 & when needed for allocating video memory)

		For instance, in 4D this method should be called preferably than the previous one in order to avoid to create render target at each frame;

		if IsD2DImpl() return true, 

			all coordinates are expressed in device independent units (DIP) 
			where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
			we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
			(It is because on Windows, 4D app is not dpi-aware and we must keep 
			this limitation for backward compatibility with previous impl)

			render target DPI is computed from inPageScale using the following formula:
			render target DPI = 96.0f*inPageScale 
			(yes render target DPI is a floating point value)

			bounds are expressed in DIP (so with DPI = 96): 
			D2D internally manages page scaling by scaling bounds & coordinates by DPI/96; 
	*/
	static VGraphicContext* CreateShared(sLONG inUserHandle, ContextRef inContextRef, const VRect& inBounds, const GReal inPageScale = 1.0f, const bool inTransparent = true, const bool inSoftwareOnly = false);

	/** release the shared graphic context binded to the specified user handle	
	@see
		CreateShared
	*/
	static void RemoveShared( sLONG inUserHandle);

	/** bind new HDC to the current render target
	@remarks
		do nothing and return false if not IsD2DImpl()
		do nothing and return false if current render target is not a ID2D1DCRenderTarget
	*/
	virtual bool BindContext( ContextRef inContextRef, const VRect& inBounds, bool inResetContext = true);

	/** create graphic context binded to the specified HWND:

		on Vista+, create a D2D graphic context
		on XP, create a Gdiplus graphic context
	@param inHWND
		window handle
	@param inPageScale (used only by D2D impl - see remarks below)
		desired page scaling 
		(default is 1.0f)
	@remarks
		FIXME: for now, it is not recommended to create directly a D2D graphic context from HWND
			   because ID2D1HwndRenderTarget hardware native double-buffering is conflicting with 4D GUI (especially due to Altura/QD wrapping)
			   so you should use either Create or CreateShared instead

		if IsD2DImpl() return true, 

			caller should call WndResize if window is resized
			(otherwise render target content is scaled to fit in window bounds)

			all coordinates are expressed in device independent units (DIP) 
			where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
			we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
			(It is because on Windows, 4D app is not dpi-aware and we must keep 
			this limitation for backward compatibility with previous impl)

			render target DPI is computed from inPageScale using the following formula:
			render target DPI = 96.0f*inPageScale 
			(yes render target DPI is a floating point value)
	*/
#if VERSIONWIN
	static VGraphicContext* Create(HWND inHWND, const GReal inPageScale = 1.0f, const bool inTransparent = true);
#endif

	/** resize window render target 
	@remarks
		do nothing if IsD2DImpl() return false
		do nothing if current render target is not a ID2D1HwndRenderTarget
	*/
	virtual void WndResize() {}

	/** create a bitmap graphic context 

		on Vista+, create a D2D WIC bitmap graphic context
		on XP, create a Gdiplus bitmap graphic context
		on Mac OS, create a Quartz2D bitmap graphic context
	@remarks
		use method RetainBitmap() to get bitmap source

		if IsD2DImpl() return true, 

			all coordinates are expressed in device independent units (DIP) 
			where 1 DIP = 1 pixel if render target DPI = 96 so in order to avoid DPI scaling from 96 to DPI
			we force DPI equal to 96 by default (so 1 DIP = 1 Pixel by default).
			(It is because on Windows, 4D app is not dpi-aware and we must keep 
			this limitation for backward compatibility with previous impl)

			render target DPI is computed from inPageScale using the following formula:
			render target DPI = 96.0f*inPageScale 
			(yes render target DPI is a floating point value)
	*/
	static VGraphicContext *CreateBitmapGraphicContext( sLONG inWidth, sLONG inHeight, const GReal inPageScale = 1.0f, const bool inTransparent = true, const VPoint* inOrigViewport = NULL, bool inTransparentCompatibleGDI = true);

#if VERSIONWIN
	/** retain WIC bitmap  
	@remarks
		return NULL if graphic context is not a bitmap context or if context is GDI

		bitmap is still shared by the context so you should not modify it (!)
	*/
	virtual IWICBitmap *RetainWICBitmap() const { return NULL; }

	/** get Gdiplus bitmap  
	@remarks
		return NULL if graphic context is not a bitmap context or if context is GDI

		bitmap is still owned by the context so you should not destroy it (!)
	*/
	virtual Gdiplus::Bitmap *GetGdiplusBitmap() const { return NULL; }

	/** retain or get bitmap associated with graphic context

		D2D graphic context: IWICBitmap * (retained WIC bitmap)
		Gdiplus graphic context: Gdiplus Bitmap * (Gdiplus bitmap)
		GDI context: return NULL

		Quartz2D graphic context: CGImageRef (retained CoreGraphics image)
	@remarks
		NULL if graphic context is not a bitmap graphic context

		bitmap is still shared by the context so you should not modify it (!)
		if it is retainable, you should release it after usage
		if it is not (Gdiplus bitmap), you should NOT destroy it because Gdiplus bitmap is owned by the context
	*/
	virtual NATIVE_BITMAP_REF RetainBitmap() const 
	{ 
		if (IsD2DImpl())
			return (NATIVE_BITMAP_REF)RetainWICBitmap();
		else if (IsGdiPlusImpl())
			return (NATIVE_BITMAP_REF)GetGdiplusBitmap();
		return NULL; 
	}
#else
	/** retain bitmap associated with graphic context

		D2D graphic context: IWICBitmap * (retained WIC bitmap)
		Gdiplus graphic context: Gdiplus Bitmap * (Gdiplus bitmap)
		GDI context: return NULL

		Quartz2D graphic context: CGImageRef (retained CoreGraphics bitmap)
	@remarks
		NULL if graphic context is not a bitmap graphic context

		bitmap is still shared by the context so you should not modify it (!)
		if it is retainable, you should release it after usage
		if it is not (Gdiplus bitmap), you should NOT destroy it because Gdiplus bitmap is owned by the context
	*/
	virtual NATIVE_BITMAP_REF RetainBitmap() const { return NULL; }
#endif

	/** draw the attached bitmap to a GDI device context 
	@remarks
		this method works with printer HDC 

		D2D impl does not support drawing directly to a printer HDC
		so for printing with D2D, you sould render first to a bitmap graphic context 
		and then call this method to draw bitmap to the printer HDC

		do nothing if current graphic context is not a bitmap graphic context
	*/
#if VERSIONWIN
	virtual void DrawBitmapToHDC( HDC hdc, const VRect& inDestBounds, const Real inPageScale = 1.0f, const VRect* inSrcBounds = NULL) {}
#endif

	bool  GetPrinterScale(GReal &outScaleX,GReal &outScaleY) const
	{
		if(fPrinterScale)
		{
			outScaleX=fPrinterScaleX;
			outScaleY=fPrinterScaleY;
		}
		return fPrinterScale;
	}
	void  SetPrinterScale(bool inEnable,GReal inScaleX,GReal inScaleY){fPrinterScale=inEnable;fPrinterScaleX=inScaleX;fPrinterScaleY=inScaleY;};
	
	virtual GReal GetDpiX() const { return 72.0f; }
	virtual GReal GetDpiY() const { return 72.0f; }
	
	static  bool IsD2DAvailable();
	static	void DesktopCompositionModeChanged();
	
	virtual bool IsD2DImpl() const { return false; }
	virtual bool IsGdiPlusImpl() const { return false; }
	virtual bool IsGDIImpl() const { return false; }

	/** enable/disable hardware 
	@remarks
		actually only used by Direct2D impl
	*/
	static void EnableHardware( bool inEnable);

	/** hardware enabling status 
	@remarks
		actually only used by Direct2D impl
	*/
	static bool IsHardwareEnabled();

	/** return true if graphic context uses hardware resources */
	virtual bool IsHardware() const 
	{ 
#if VERSIONWIN
		return false;
#elif VERSIONMAC
		return true;
#else
		return false;
#endif
	}

	/** set to true to force this graphic context to use software only
	@remarks
		if actual graphic context is hardware, 
		internal resources will be rebuilded 

		you should call this method outside BeginUsingContext/EndUsingContext
		otherwise method will do nothing
	@note
		actually this method is mandatory only for Direct2D impl
	*/
	virtual void SetSoftwareOnly( bool inSoftwareOnly) {}

	/** return true if this graphic context should use software only, false otherwise */
	virtual bool IsSoftwareOnly() const
	{
#if VERSIONWIN
		return true;
#elif VERSIONMAC
		return false;
#else
		return false;
#endif
	}

	/** change transparency status
	@remarks
		render target can be rebuilded if transparency status is changed
	*/
	virtual void SetTransparent( bool inTransparent) {}

	/** return true if render target is transparent
	@remarks
		ClearType can only be used on opaque render target
	*/
	virtual bool IsTransparent() const
	{
		return true;
	}

	/** Windows & Direct2D only (but safe for any impl): if true, if GDI context is queried on a transparent surface (bitmap or layer),
		it will ensure GDI surface is prepared for drawing with GDI 
		(it does nothing on a opaque surface)

		It assumes all pixels drawed with GDI are opaque.

		you should call this method on a per-case basis that is before BeginUsingParentContext 
		and reset status with ShouldPrepareTransparentForGDI(false) after EndUsingParentContext
		(if you let the status to be true, surface will be always prepared while querying a hdc which can have a high perf hit
		 especially if hdc is not used for drawing...)

		default if false
	@param inAllow
		enable/disable GDI surface preparation

	@param inBounds
		bounds in gc user space of area where surface needs to be prepared (should be set to intended drawing area)
		if NULL, all surface area will be fixed
	*/
	virtual void ShouldPrepareTransparentForGDI( bool inAllow = true, const VRect *inBounds = NULL) {}
	virtual bool ShouldPrepareTransparentForGDI() const { return false; }

	/** set lock type 
	@remarks
		this method is mandatory only for Direct2D context: for all other contexts it does nothing.

		if context is Direct2D:

		if true, _Lock(), _GetHDC() and BeginUsingParentContext() will always lock & return parent context by default  
				ie the context in which is presented the render target (to which the render target is binded)
		if false (default), _Lock(), _GetHDC() and BeginUsingParentContext() will lock & return a GDI-compatible context created from the render target content
				if called between BeginUsingContext & EndUsingContext, otherwise will lock or return the context binded to the render target

		caution: please do NOT call between _GetHDC()/_ReleaseHDC() or BeginUsingParentContext/EndUsingParentContext
		
	*/
	virtual void SetLockParentContext( bool inLockParentContext) {}
	virtual bool GetLockParentContext() const { return true; }

	/** enable/disable Direct2D implementation at runtime
	@remarks
		if set to false, IsD2DAvailable() will return false even if D2D is available on the platform
		if set to true, IsD2DAvailable() will return true if D2D is available on the platform
	*/
	static void D2DEnable( bool inD2DEnable = true);

	/** return true if Direct2D impl is enabled at runtime, false otherwise */
	static bool D2DIsEnabled();

	/** create VGraphicPath object */
	virtual VGraphicPath *CreatePath(bool inComputeBoundsAccurate = false) const;

	// Pen management

	void	SetFillStdPattern (PatternStyle inStdPattern, VBrushFlags* ioFlags = NULL);
	void	SetLineStdPattern (PatternStyle inStdPattern, VBrushFlags* ioFlags = NULL);
	
	virtual void	SetFillColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	
	virtual void	SetFillPattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) = 0;
	virtual void	SetLineColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	virtual void	SetLinePattern (const VPattern* inPattern, VBrushFlags* ioFlags = NULL) = 0;
	
	void	UseShapeBrushForText( bool inUseShapeBrushForText)
	{
		fUseShapeBrushForText = inUseShapeBrushForText;
	}
	bool	UseShapeBrushForText() const
	{
		return fUseShapeBrushForText;
	}

	/** set line dash pattern
	@param inDashOffset
		offset into the dash pattern to start the line (user units)
	@param inDashPattern
		dash pattern (alternative paint and unpaint lengths in user units)
		for instance {3,2,4} will paint line on 3 user units, unpaint on 2 user units
							 and paint again on 4 user units and so on...
	*/
	virtual void	SetLineDashPattern(GReal inDashOffset, const VLineDashPattern& inDashPattern, VBrushFlags* ioFlags = NULL) = 0;

	/** clear line dash pattern */
	void ClearLineDashPattern(VBrushFlags* ioFlags = NULL)
	{
		SetLineDashPattern( 0.0f, VLineDashPattern(), ioFlags);
	}

	/** set fill rule */
	virtual void	SetFillRule( FillRuleType inFillRule) 
	{ 
		fFillRule = inFillRule; 
	}
	/** get fill rule */
	FillRuleType	GetFillRule() const
	{
		return fFillRule;
	}
	
	virtual void	SetLineWidth (GReal inWidth, VBrushFlags* ioFlags = NULL) = 0;
	virtual GReal	GetLineWidth () const { return 1.0f; }
	
	virtual	bool	SetHairline(bool inSet);
	virtual bool	GetHairLine(){return fHairline;}
	
	virtual void	SetLineStyle (CapStyle inCapStyle, JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL) = 0;
	virtual void	SetLineCap (CapStyle inCapStyle, VBrushFlags* ioFlags = NULL) = 0;
	virtual void	SetLineJoin(JoinStyle inJoinStyle, VBrushFlags* ioFlags = NULL) = 0; 
	/** set line miter limit */
	virtual void	SetLineMiterLimit( GReal inMiterLimit = (GReal) 0.0) {}
	virtual void	SetTextColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	virtual void	GetTextColor (VColor& outColor) const = 0;
	virtual void	SetBackColor (const VColor& inColor, VBrushFlags* ioFlags = NULL) = 0;
	virtual void	GetBrushPos (VPoint& outHwndPos) const = 0;
	virtual void	SetTransparency (GReal inAlpha) = 0;	// Global Alpha - added to existing line or fill alpha
	virtual void	SetShadowValue (GReal inHOffset, GReal inVOffset, GReal inBlurness) = 0;	// Need a call to SetDrawingMode to update changes
	
	virtual DrawingMode	SetDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL) = 0;
	
	void	SetMaxPerfFlag(bool inMaxPerfFlag) 
	{
		if (inMaxPerfFlag == fMaxPerfFlag)
			return;

		fMaxPerfFlag = inMaxPerfFlag;

		if (fMaxPerfFlag)
		{
			//disable antialiasing for shapes & text

			//backup high quality status (inverse of max performance status)
			bool highQualityAntialiased = fIsHighQualityAntialiased;
			TextRenderingMode highQualityTextRenderingMode = fHighQualityTextRenderingMode;

			//here as fMaxPerfFlag == true, calling following methods will actually disable smoothing text & antialiasing
			SetTextRenderingMode( fHighQualityTextRenderingMode);
			DisableAntiAliasing();

			//keep high quality status for later restore (restored if max perf is disabled)
			fIsHighQualityAntialiased = highQualityAntialiased;
			fHighQualityTextRenderingMode = highQualityTextRenderingMode;
		}
		else
		{
			//restore high quality antialiasing as set with SetTextRenderingMode & EnableAntiAliasing

			SetTextRenderingMode( fHighQualityTextRenderingMode);
			if (fIsHighQualityAntialiased)
				EnableAntiAliasing();
			else 
				DisableAntiAliasing();
		}
	}
	bool	GetMaxPerfFlag() const {return fMaxPerfFlag;}

	// Anti-aliasing for everything but Text
	virtual void	EnableAntiAliasing () = 0;
	virtual void	DisableAntiAliasing () = 0;
	virtual bool	IsAntiAliasingAvailable () = 0;
	/** return current antialiasing status (dependent on max perf flag status) */
	virtual bool	IsAntiAliasingEnabled() const { return fIsHighQualityAntialiased && !fMaxPerfFlag; }

	/** set desired antialiasing status */
	virtual void	SetDesiredAntiAliasing( bool inEnable)	
	{ 
		if (inEnable)
			EnableAntiAliasing();
		else
			DisableAntiAliasing();
	}
	/** return desired antialiasing status (as requested by EnableAntiAliasing or DisableAntiAliasing) */
	virtual bool	GetDesiredAntiAliasing() const { return fIsHighQualityAntialiased ; }
	
	/** enable crispEdges rendering mode
	@remarks
		when crispEdges status is enabled, shape floating point coordinates are modified in transformed space
		in order to align shape coordinates with device pixels 
		(according to pixel offset mode which should be half pixel offset for all impls):
		this mode ensures shapes are drawed in device space at the same pixel position
		on all platforms (actually this is not true for more complex shapes like paths with bezier curves 
		because for complex	shapes it is not possible to get the same rendering on any platform
		but it is true for shapes which are made mainly with lines);
		this mode helps too to reduce antialiasing artifacts on horizontal & vertical lines

		this mode has a performance cost & it can deform shape especially if device resolution is 
		not the same as shape resolution (ie if transform scale from shape user space to device space is not a integer value
		or if shape uses not integer values for coordinates) so default status is disabled;
		ideally it should be enabled while drawing at device resolution when platform compatibility is required
		(like in 4D forms for drawing controls)

		this rendering mode is not compatible with QD-compatible methods so you should not enable it
		while using these methods.
	*/
	void EnableShapeCrispEdges( bool inShapeCrispEdgesEnabled)
	{
		fShapeCrispEdgesEnabled = inShapeCrispEdgesEnabled;
	}
	
	/** crisEdges rendering mode accessor */
	bool IsShapeCrispEdgesEnabled() const 
	{
		return fShapeCrispEdgesEnabled;
	}

	/** allow crispEdges algorithm to be applied on paths 
	@remarks
		by default it is set to false

		by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
		it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
		on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
	*/
	void AllowCrispEdgesOnPath( bool inAllowCrispEdgesOnPath)
	{
		fAllowCrispEdgesOnPath = inAllowCrispEdgesOnPath;
	}

	/** return true if crispEdges can be applied on paths */
	bool IsAllowedCrispEdgesOnPath() const 
	{
		return fAllowCrispEdgesOnPath;
	} 

	/** allow crispEdges algorithm to be applied on paths with bezier curves
	@remarks
		by default it is set to false because modifying bezier coordinates can result in bezier curve distortion 
		because of control points error discrepancies
		so caller should only enable it on a per-case basis

		by default crispEdges algorithm is applied only on lines, rects, round rects, ellipses & polygons;
		it is applied also on paths which contain only lines if fAllowCrispEdgesOnPaths if equal to true &
		on all paths if fAllowCrispEdgesOnPaths & fAllowCrispEdgesOnBezier are both equal to true.
	*/
	void AllowCrispEdgesOnBezier( bool inAllowCrispEdgesOnBezier)
	{
		if (inAllowCrispEdgesOnBezier)
			fAllowCrispEdgesOnPath = true;
		fAllowCrispEdgesOnBezier = inAllowCrispEdgesOnBezier;
	}

	/** return true if crispEdges can be applied on paths with bezier curves */
	bool IsAllowedCrispEdgesOnBezier() const 
	{
		return fAllowCrispEdgesOnPath && fAllowCrispEdgesOnBezier;
	} 

	/** adjust point coordinates in transformed space 
	@remarks
		this method should only be called if fShapeCrispEdgesEnabled is equal to true
	*/
	void _CEAdjustPointInTransformedSpace( VPoint& ioPos, bool inFillOnly = false, VPoint *outPosTransformed = NULL);

	/** adjust rect coordinates in transformed space 
	@remarks
		this method should only be called if fShapeCrispEdgesEnabled is equal to true
	*/
	void _CEAdjustRectInTransformedSpace( VRect& ioRect, bool inFillOnly = false);

	// Text management
	virtual void	SetFont( VFont *inFont, GReal inScale = (GReal) 1.0) = 0;
	virtual VFont*	RetainFont() = 0;
	virtual void	SetTextPosBy (GReal inHoriz, GReal inVert) = 0;
	virtual void	SetTextPosTo (const VPoint& inHwndPos) = 0;
	virtual void	SetTextAngle (GReal inAngle) = 0;
	virtual void	GetTextPos (VPoint& outHwndPos) const = 0;
	
	virtual void	SetSpaceKerning (GReal inSpaceExtra)
	{
		fCharSpacing = inSpaceExtra;
	}
	GReal GetSpaceKerning() const
	{
		return fCharSpacing;
	}

	virtual void	SetCharKerning (GReal inSpaceExtra)
	{
		fCharKerning = inSpaceExtra;
	}
	GReal GetCharKerning() const
	{
		return fCharKerning;
	}
	
	/** return sum of char kerning and spacing (actual kerning) */
	GReal GetCharActualKerning() const
	{
		return fCharKerning+fCharSpacing;
	}
	
	virtual DrawingMode	SetTextDrawingMode (DrawingMode inMode, VBrushFlags* ioFlags = NULL) = 0;
	
	// Anti-aliasing for Text, kerning, ligatures etc...
	virtual TextRenderingMode	SetTextRenderingMode( TextRenderingMode inMode);
	/** return current text rendering mode (dependent on max perf flag status) */
	virtual TextRenderingMode	GetTextRenderingMode() const;

	/** return desired text rendering mode (as requested by SetTextRenderingMode) */
	virtual TextRenderingMode	GetDesiredTextRenderingMode() const { return fHighQualityTextRenderingMode; }

	// Pixel management
	virtual uBYTE	SetAlphaBlend (uBYTE inAlphaBlend) = 0;
	
	virtual TransferMode	SetPixelTransferMode (TransferMode inMode) = 0;
	
	// Graphic context storage
	virtual void	NormalizeContext () = 0;
	virtual void	SaveContext () = 0;
	virtual void	RestoreContext () = 0;
	
	// Graphic layer management
	//
	//@remarks
	// layers are graphic containers which have their own graphic context
	// so effects like transparency, shadow or custom filters can be applied globally on a set of graphic primitives;
	// to define a layer, start a graphic container with BeginLayer...() - either the transparency/shadow or filter version - 
	// draw any graphic primitives and then close layer with EndLayer(): 
	// when you call EndLayer, group of primitives is drawed using transparency, shadow or filter you have defined in BeginLayer();
	// you can define a layer inside another layer so you can call again
	// BeginLayer...()/EndLayer() inside BeginLayer...()/EndLayer()
	// 
	//@note
	//	on Windows OS, layers are implemented using a bitmap graphic context and software filters for transparency/shadow or filter layers
	//  on Mac OS, transparency/shadow layers are implemented as transparency/shadow quartz layers 
	//			   but filter layers are implemented using a bitmap graphic context and CoreImage filters
	//			   (because CoreImage filters need at least one bitmap graphic context)


	// Create a transparent layer graphic context
	// with specified bounds and transparency
	void BeginLayerTransparency(const VRect& inBounds, GReal inAlpha)
	{
		_BeginLayer( inBounds, inAlpha, NULL);
	}

	// Create a filter layer graphic context
	// with specified bounds and filter
	// @remark : retain the specified filter until EndLayer is called
	void BeginLayerFilter(const VRect& inBounds, VGraphicFilterProcess *inFilterProcess)
	{
		_BeginLayer( inBounds, (GReal) 1.0, inFilterProcess);
	}

	// Create a shadow layer graphic context
	// with specified bounds and transparency
	virtual void BeginLayerShadow(const VRect& inBounds, const VPoint& inShadowOffset = VPoint(8,8), GReal inShadowBlurDeviation = (GReal) 1.0, const VColor& inShadowColor = VColor(0,0,0,170), GReal inAlpha = (GReal) 1.0) {}
	
	/** create a background layer 
	@param inBounds
		layer bounds
	@param inBackBufferRef
		if NULL, create new background layer with size equal to current context transformed bounds size
				 
		if <>NULL, retain inBackBufferRef & resize it to current context transformed bounds size if needed
	@param inInheritParentClipping
		if true, offscreen layer bounds will be equal to the intersection of inBounds with gc clipping bounds (transformed by ctm)
		otherwise layer bounds will be equal to inBounds (transformed by ctm)
	@param inTransparent
		true for transparent layer, false otherwise
	@return
		true if new background layer is created or if inBackBufferRef is resized (you should redraw all the layer content)
		false if inBackBufferRef is not modified (you should redraw only layer modified content)
	@remarks
		this layer is suitable for offscreen compositing (for instance for SVG component)
		as background layer is owned by caller, caller should only redraw invalidated areas
		if method returns false 
	*/
	virtual bool BeginLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBufferRef = NULL, bool inInheritParentClipping = true, bool inTransparent = true)
	{
		if (inBackBufferRef == NULL)
			inBackBufferRef = (VImageOffScreenRef)0xFFFFFFFF;
		return _BeginLayer( inBounds, 1.0f, NULL, inBackBufferRef, inInheritParentClipping, inTransparent);
	}

	/** draw offscreen layer using the specified bounds 
	@remarks
		this is like calling BeginLayerOffScreen & EndLayerOffScreen without any drawing between
		so it is faster because only the offscreen bitmap is rendered
	 
		if transformed bounds size is not equal to background layer size, return false and do nothing 
	    so only transformed bounds translation is allowed.
		if inBackBuffer is NULL, return false and do nothing

		ideally if offscreen content is not dirty, you should first call this method
		and only call BeginLayerOffScreen & EndLayerOffScreen if it returns false
	 */
	virtual bool DrawLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer) { return false; }
	
	/** return true if offscreen layer needs to be cleared or resized on next call to DrawLayerOffScreen or BeginLayerOffScreen/EndLayerOffScreen
		if using inBounds as dest bounds
	@remarks
		return true if transformed input bounds size is not equal to the background layer size
	    so only transformed bounds translation is allowed.
		return true the offscreen layer is not compatible with the current gc
	 */
	virtual bool ShouldClearLayerOffScreen(const VRect& inBounds, VImageOffScreenRef inBackBuffer) { return true; }

	/** return index of current layer 
	@remarks
		index is zero-based
		return -1 if there is no layer
	*/
	virtual sLONG GetIndexCurrentLayer() const
	{
		if (fStackLayerGC.size() > 0)
			return (sLONG)(fStackLayerGC.size()-1);
		else 
			return -1;
	}
	
	
	// Create a filter or alpha layer graphic context
	// with specified bounds and filter
	// @remark : retain the specified filter until EndLayer is called
protected:
	virtual bool _BeginLayer(const VRect& inBounds, 
							 GReal inAlpha, 
							 VGraphicFilterProcess *inFilterProcess,
							 VImageOffScreenRef inBackBufferRef = NULL,
							 bool inInheritParentClipping = true,
							 bool inTransparent = true) { return true; }
	virtual VImageOffScreenRef _EndLayer() { return NULL; }
public:
	
	/** 
	   Draw the background image in the container graphic context
	   
	   @return
			background image
	   @remarks
			must only be called after BeginLayerOffScreen
	*/
	virtual VImageOffScreenRef EndLayerOffScreen() 
	{
		return _EndLayer();
	}

	// Draw the layer graphic context in the container graphic context 
	// using the filter specified in BeginLayer
	// (container graphic context can be either the main graphic context or another layer graphic context)
	virtual void	EndLayer()
	{
		VImageOffScreenRef offscreen = _EndLayer();
		if (offscreen)
			offscreen->Release();
	}

	// clear all layers and restore main graphic context
	virtual void	ClearLayers() {}

	/** return current clipping bounding rectangle 
	 @remarks
		bounding rectangle is expressed in VGraphicContext normalized coordinate space 
	    (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */  
	virtual void	GetClipBoundingBox( VRect& outBounds) { outBounds.SetEmpty(); }
	
	/** return current graphic context transformation matrix
	    in VGraphicContext normalized coordinate space
		(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */
	virtual void GetTransformNormalized(VAffineTransform &outTransform) { GetTransform( outTransform); }
	
	/** set current graphic context transformation matrix
		in VGraphicContext normalized coordinate space
		(ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	 */
	virtual void SetTransformNormalized(const VAffineTransform &inTransform) { SetTransform( inTransform); }
	
	/** return user space to main graphic context space transformation matrix
	@remarks
		unlike GetTransform which return current graphic context transformation 
		- which can be the current layer transformation if called between BeginLayer() and EndLayer() - 
		this method return current user space to the main graphic context space transformation
		taking account all intermediate layer transformations if any
		(of course useful only if you need to map user space to/from main graphic context space 
		 - main graphic context is the first one created or set with this class constructor)
	 
		returned matrix is compliant with VGraphicContext coordinate space
	    (ie coordinate space with y axis pointing down - compliant with gdiplus and quickdraw coordinate spaces)
	*/
	virtual void	GetTransformToScreen(VAffineTransform &outTransform) 	{ GetTransformToLayer( outTransform, -1); }

	virtual void	GetTransformToLayer(VAffineTransform &outTransform, sLONG inIndexLayer) { GetTransform( outTransform); }

	/** get layer filter inflating offset 
	@remark
		filter bounds are inflated (in device space) by this offset 
		in order to reduce border artifacts with some filter effects
		(like gaussian blur)
	*/
	static GReal GetLayerFilterInflatingOffset()
	{
		return sLayerFilterInflatingOffset;
	}

	//return current graphic context native reference
	virtual VGraphicContextNativeRef GetNativeRef() const { return NULL; }
	
	// Transform
	virtual void GetTransform(VAffineTransform &outTransform)=0;
	virtual void SetTransform(const VAffineTransform &inTransform)=0;
	virtual void ConcatTransform(const VAffineTransform &inTransform)=0;
	virtual void RevertTransform(const VAffineTransform &inTransform)=0;
	virtual void ResetTransform()=0;
	
	// Text measurement
	virtual GReal	GetCharWidth (UniChar inChar) const  = 0;
	virtual GReal	GetTextWidth (const VString& inString) const = 0;
	virtual TextMeasuringMode	SetTextMeasuringMode(TextMeasuringMode inMode) = 0;
	virtual TextMeasuringMode	GetTextMeasuringMode() = 0;

	/** return text bounds including additional spacing
		due to layout formatting
		@remark: if you want the real bounds use GetTextBoundsTypographic
	*/
	virtual void	GetTextBounds( const VString& inString, VRect& oRect) const;

	/** Returns ascent + descent [+ external leading] */
	virtual GReal	GetTextHeight (bool inIncludeExternalLeading = false) const = 0;

	/** Returns ascent + descent [+ external leading] each info is normalized to int before operation*/
	virtual sLONG	GetNormalizedTextHeight (bool inIncludeExternalLeading = false) const = 0;


	/** draw text box */
	virtual void	DrawTextBox (const VString& inString, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL) = 0;
	
	/** return text box bounds
		@remark:
			if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
			if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
	*/
	virtual void	GetTextBoxBounds( const VString& inString, VRect& ioHwndBounds, TextLayoutMode inMode = TLM_NORMAL) {}
	
	virtual void	DrawText (const VString& inString, TextLayoutMode inMode = TLM_NORMAL) = 0;

#if VERSIONWIN
	virtual void	DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		_DrawLegacyStyledText( inText, inStyles, inHoriz, inVert, inHwndBounds, inMode, inRefDocDPI);
	}
#else
	virtual void	DrawStyledText( const VString& inText, VTreeTextStyle *inStyles, AlignStyle inHoriz, AlignStyle inVert, const VRect& inHwndBounds, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) {}
#endif

	/** return styled text box bounds
		@remark:
			if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
			if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)
	*/
	virtual void	GetStyledTextSize( const VString& inText, VTreeTextStyle *inStyles, GReal& ioWidth, GReal &ioHeight)
	{
		VRect bounds(0,0,ioWidth,ioHeight);
		GetStyledTextBoxBounds( inText, inStyles, bounds);
		ioWidth = bounds.GetWidth();
		ioHeight = bounds.GetHeight();
	}

	/** return styled text box bounds
		@remark:
			if input bounds width is not null, use it as max line wrapping width (or max text height in vertical layout mode)
			if input bounds height is not null, use it as max text height (or max line wrapping width in vertical layout mode)

			inRefDocDPI is used only on Windows to adjust font size in point according to 4D form dpi which is 72 by default
				(otherwise real font size would be scaled to screen dpi/72 which is wrong in 4D form)
				it can be used to scale font size according to any reference DPI 
				it is only needed for styled text because otherwise it is assumed that current selected font is already scaled according to 4D form DPI
	*/
#if VERSIONWIN
	virtual void	GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f)
	{
		_GetLegacyStyledTextBoxBounds( inText, inStyles, ioBounds, inRefDocDPI);
	}
#else
	virtual void	GetStyledTextBoxBounds( const VString& inText, VTreeTextStyle *inStyles, VRect& ioBounds, const GReal inRefDocDPI = 72.0f) {}
#endif

	/** return styled text box run bounds from the specified range */
#if VERSIONWIN
	virtual void	GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		_GetLegacyStyledTextBoxRunBoundsFromRange( inText, inStyles, inBounds, outRunBounds, inStart, inEnd, inHAlign, inVAlign, inMode, inRefDocDPI);
	}
#else
	virtual void	GetStyledTextBoxRunBoundsFromRange( const VString& inText, VTreeTextStyle *inStyles, const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) {}
#endif

	/** for the specified styled text box, return the caret position & height of text at the specified text position
	@remarks
		text position is 0-based

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

		by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
		but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
#if VERSIONWIN
	virtual void	GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		_GetLegacyStyledTextBoxCaretMetricsFromCharIndex( inText, inStyles, inHwndBounds, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics, inHAlign, inVAlign, inMode, inRefDocDPI);
	}
#else
	virtual void	GetStyledTextBoxCaretMetricsFromCharIndex( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) {}
#endif

	/** for the specified text box, return the character bounds at the specified text position
	@remarks
		text position is 0-based

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

		by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
		but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
	virtual void	GetTextBoxCaretMetricsFromCharIndex( const VString& inText, const VRect& inHwndBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		GetStyledTextBoxCaretMetricsFromCharIndex( inText, NULL, inHwndBounds, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics, inHAlign, inVAlign, inMode, inRefDocDPI);
	}
	
	/** for the specified styled text box, return the text position at the specified coordinates
	@remarks
		output text position is 0-based

		input coordinates are in graphic context user space

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
#if VERSIONWIN
	virtual bool GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		return _GetLegacyStyledTextBoxCharIndexFromCoord( inText, inStyles, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
	}
#else
	virtual bool GetStyledTextBoxCharIndexFromCoord( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f) { return false; }
#endif

	virtual bool GetStyledTextBoxCharIndexFromPos( const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		return GetStyledTextBoxCharIndexFromCoord( inText, inStyles, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
	}

	/** for the specified text box, return the text position at the specified coordinates
	@remarks
		output text position is 0-based

		input coordinates are in graphic context user space

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
	virtual bool GetTextBoxCharIndexFromCoord( const VString& inText, const VRect& inHwndBounds, const VPoint& inPos, VIndex& outCharIndex, AlignStyle inHAlign = AL_DEFAULT, AlignStyle inVAlign = AL_DEFAULT, TextLayoutMode inMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f)
	{
		return GetStyledTextBoxCharIndexFromCoord( inText, NULL, inHwndBounds, inPos, outCharIndex, inHAlign, inVAlign, inMode, inRefDocDPI);
	}

	/**	get text bounds 
		
		@param inString
			text string
		@param oRect
			text layout bounds (out param)
		@param inLayoutMode
		  layout mode

		@remarks
			this method return the typographic text line bounds 
		@note
			this method is used by SVG component
	*/
	virtual void	GetTextBoundsTypographic( const VString& inString, VRect& oRect, TextLayoutMode inLayoutMode = TLM_NORMAL) const;

	/** extract text lines & styles from a wrappable text using the specified passed max width
	@param inText
		text to split into lines
	@param inMaxWidth
		text max width (used for automatic wrapping)
	@param outTextLines
		output text per line
	@param inStyles
		input styles
	@param outTextLinesStyles
		output text styles per line
	@param inRefDocDPI
		input DPI (default is 72 for 4D form compliancy)
	@param inNoBreakWord
		if true (default), words are not breakable so there will be at least one word per line: in that case text can overflow max width
			(here we need to make manually the wrapping because native API should eventually break words if width is too small) 
		if false, depending on native API, words can eventually be breaked into lines if max width is too small
	@param outLinesOverflow
		if inNoBreakWord is true: return true if some output lines width is greater than max width otherwise false
		if inNoBreakWord is false, return always false
	@remarks 
		layout mode is assumed to be equal to default = TLM_NORMAL
	*/
	virtual void GetTextBoxLines( const VString& inText, const GReal inMaxWidth, std::vector<VString>& outTextLines, VTreeTextStyle *inStyles = NULL, std::vector<VTreeTextStyle *> *outTextLinesStyles = NULL, const GReal inRefDocDPI = 72.0f, const bool inNoBreakWord = true, bool *outLinesOverflow = NULL);
	
	/** extract font style from VTextStyle */
	VFontFace _TextStyleToFontFace( const VTextStyle *inStyle);

	/** return true if current graphic context or inStyles uses only true type font(s) */
	virtual bool	UseFontTrueTypeOnly( VTreeTextStyle *inStyles = NULL);

	/** return true if inStyles uses only true type font(s) */
	static bool		UseFontTrueTypeOnlyRec( VTreeTextStyle *inStyles = NULL);

	/**	draw text at the specified position
		
		@param inString
			text string
		@param inLayoutMode
		  layout mode (here only TLM_VERTICAL and TLM_RIGHT_TO_LEFT are used)
		
		@remark: 
			this method does not make any layout formatting 
			text is drawed on a single line from the specified starting coordinate
			(which is relative to the first glyph horizontal or vertical baseline)
			useful when you want to position exactly a text at a specified coordinate
			without taking account extra spacing due to layout formatting (which is implementation variable)
		@note
			this method is used by SVG component
	*/
	virtual void	DrawText (const VString& inString, const VPoint& inPos, TextLayoutMode inLayoutMode = TLM_NORMAL){}

	virtual void	DrawRect (const VRect& inHwndRect) = 0;
	virtual void	DrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void	DrawOval (const VRect& inHwndRect) = 0;
	virtual void	DrawPolygon (const VPolygon& inHwndPolygon) = 0;
	virtual void	DrawRegion (const VRegion& inHwndRegion) = 0;
	virtual void	DrawBezier (VGraphicPath& inHwndBezier) = 0;
	virtual void	DrawPath (VGraphicPath& inHwndPath) = 0;

	virtual void	FillRect (const VRect& inHwndRect) = 0;
	virtual void	FillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void	FillOval (const VRect& inHwndRect) = 0;
	virtual void	FillPolygon (const VPolygon& inHwndPolygon) = 0;
	virtual void	FillRegion (const VRegion& inHwndRegion) = 0;
	virtual void	FillBezier (VGraphicPath& inHwndBezier) = 0;
	virtual void	FillPath (VGraphicPath& inHwndPath) = 0;

	virtual void	FrameRect (const VRect& inHwndRect) = 0;
	virtual void	FrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight);
	virtual void	FrameOval (const VRect& inHwndRect) = 0;
	virtual void	FramePolygon (const VPolygon& inHwndPolygon) = 0;
	virtual void	FrameRegion (const VRegion& inHwndRegion) = 0;
	virtual void	FrameBezier (const VGraphicPath& inHwndBezier) = 0;
	virtual void	FramePath (const VGraphicPath& inHwndPath) = 0;

	/** paint arc stroke 
	@param inCenter
		arc ellipse center
	@param inStartDeg
		starting angle in degree
	@param inEndDeg
		ending angle in degree
	@param inRX
		arc ellipse radius along x axis
	@param inRY
		arc ellipse radius along y axis
		if inRY <= 0, inRY = inRX
	@param inDrawPie (default true)
		true: draw ellipse pie between start & end
		false: draw only ellipse arc between start & end
	@remarks
		shape is painted using counterclockwise direction
		(so if inStartDeg == 0 & inEndDeg == 90, method paint up right ellipse quarter)
	*/
	virtual void	FrameArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY = 0.0f, bool inDrawPie = true);

	/** paint a arc interior shape 
	@param inCenter
		arc ellipse center
	@param inStartDeg
		starting angle in degree
	@param inEndDeg
		ending angle in degree
	@param inRX
		arc ellipse radius along x axis
	@param inRY
		arc ellipse radius along y axis
		if inRY <= 0, inRY = inRX
	@param inDrawPie (default true)
		true: draw ellipse pie between start & end
		false: draw only ellipse arc between start & end
	@remarks
		shape is painted using counterclockwise direction
		(so if inStartDeg == 0 & inEndDeg == 90, method paint up right ellipse quarter)
	*/
	virtual void	FillArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY = 0.0f, bool inDrawPie = true);

	/** paint a arc (interior & stroke)
	@param inCenter
		arc ellipse center
	@param inStartDeg
		starting angle in degree
	@param inEndDeg
		ending angle in degree
	@param inRX
		arc ellipse radius along x axis
	@param inRY
		arc ellipse radius along y axis
		if inRY <= 0, inRY = inRX
	@param inDrawPie (default true)
		true: draw ellipse pie between start & end
		false: draw only ellipse arc between start & end
	@remarks
		shape is painted using counterclockwise direction
		(so if inStartDeg == 0 & inEndDeg == 90, method paint up right ellipse quarter)
	*/
	virtual void	DrawArc (const VPoint& inCenter, GReal inStartDeg, GReal inEndDeg, GReal inRX, GReal inRY = 0.0f, bool inDrawPie = true);

	virtual void	EraseRect (const VRect& inHwndRect) = 0;
	virtual void	EraseRegion (const VRegion& inHwndRegion) = 0;

	virtual void	InvertRect (const VRect& inHwndRect) = 0;
	virtual void	InvertRegion (const VRegion& inHwndRegion) = 0;

	virtual void	DrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd) = 0;
	virtual void	DrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY) = 0;
	virtual void	DrawLines (const GReal* inCoords, sLONG inCoordCount) = 0;
	virtual void	DrawLineTo (const VPoint& inHwndEnd) = 0;
	virtual void	DrawLineBy (GReal inWidth, GReal inHeight) = 0;
	
	virtual void	MoveBrushTo (const VPoint& inHwndPos) = 0;
	virtual void	MoveBrushBy (GReal inWidth, GReal inHeight) = 0;
	
	virtual void	DrawPicture (const class VPicture& inPicture,const VRect& inHwndBounds,class VPictureDrawSettings *inSet=0) = 0;
	//virtual void	DrawPicture (const VOldPicture& inPicture, PictureMosaic inMosaicMode, const VRect& inHwndBounds, TransferMode inDrawingMode = TM_COPY) = 0;
	virtual void	DrawFileBitmap(const VFileBitmap *inFileBitmap, const VRect& inHwndBounds) = 0;
	virtual void	DrawPictureFile (const VFile& inFile, const VRect& inHwndBounds) = 0;
	virtual void	DrawIcon (const VIcon& inIcon, const VRect& inHwndBounds, PaintStatus inState = PS_NORMAL, GReal inScale = (GReal) 1.0) = 0; // JM 050507 ACI0049850 rajout inScale

	/** QD compatibile drawing primitive (pen mode inset) **/
	
	virtual void	QDFrameRect (const VRect& inHwndRect) = 0;
	virtual void	QDFrameRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)=0;
	virtual void	QDFrameOval (const VRect& inHwndRect) = 0;
	
	virtual void	QDDrawRect (const VRect& inHwndRect) = 0;
	virtual void	QDDrawRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)=0;
	virtual void	QDDrawOval (const VRect& inHwndRect) = 0;

	virtual void	QDFillRect (const VRect& inHwndRect) = 0;
	virtual void	QDFillRoundRect (const VRect& inHwndRect,GReal inOvalWidth,GReal inOvalHeight)=0;
	virtual void	QDFillOval (const VRect& inHwndRect) = 0;

	virtual void	QDDrawLine(const GReal inHwndStartX,const GReal inHwndStartY, const GReal inHwndEndX,const GReal inHwndEndY)=0;
	virtual void	QDMoveBrushTo (const VPoint& inHwndPos) = 0;
	virtual void	QDDrawLine (const VPoint& inHwndStart, const VPoint& inHwndEnd) = 0;
	virtual void	QDDrawLineTo (const VPoint& inHwndEnd) = 0;
	virtual void	QDDrawLines (const GReal* inCoords, sLONG inCoordCount) = 0;
	//draw picture data
	virtual void    DrawPictureData (const VPictureData *inPictureData, const VRect& inHwndBounds,VPictureDrawSettings *inSet=0) {}

	/** clear current graphic context area
	@remarks
		Quartz2D impl: inBounds cannot be equal to NULL (do nothing if it is the case)
		GDIPlus impl: inBounds can be NULL if and only if graphic context is a bitmap graphic context
		Direct2D impl: inBounds can be NULL (in that case the render target content is cleared)
	*/
	virtual void	Clear( const VColor& inColor = VColor(0,0,0,0), const VRect *inBounds = NULL, bool inBlendCopy = true) {}

	// Clipping
	//
	//@remarks
	//	by default, new clipping region intersects current clipping region 
	//@note
	//	MAC OS Quartz2D implementation is restricted to intersection (API constraint)
	//  (so Quartz2D assumes that inIntersectToPrevious is always true no matter what you set)

	virtual void	ClipRect (const VRect& inHwndRect, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) = 0;

	//add or intersect the specified clipping path with the current clipping path
	virtual void	ClipPath (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) {}

	/** add or intersect the specified clipping path outline with the current clipping path 
	@remarks
		this will effectively create a clipping path from the path outline 
		(using current stroke brush) and merge it with current clipping path,
		making possible to constraint drawing to the path outline
	*/
	virtual void	ClipPathOutline (const VGraphicPath& inPath, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) {}

	virtual void	ClipRegion (const VRegion& inHwndRgn, Boolean inAddToPrevious = false, Boolean inIntersectToPrevious = true) = 0;
	virtual void	GetClip (VRegion& outHwndRgn) const = 0;
	
	virtual void	SaveClip () = 0;
	virtual void	RestoreClip () = 0;

	// Content bliting primitives
	virtual void	CopyContentTo (VGraphicContext* inDestContext, const VRect* inSrcBounds = NULL, const VRect* inDestBounds = NULL, TransferMode inMode = TM_COPY, const VRegion* inMask = NULL) = 0;
	virtual VPictureData* CopyContentToPictureData()=0;
	virtual void	Flush () = 0;
	
	// Pixel drawing primitives (supported only for offscreen contexts)
	virtual void	FillUniformColor (const VPoint& inPixelPos, const VColor& inColor, const VPattern* inPattern = NULL) = 0;
	virtual void	SetPixelColor (const VPoint& inPixelPos, const VColor& inColor) = 0;
	virtual VColor*	GetPixelColor (const VPoint& inPixelPos) const = 0;
	//virtual VOldPicture*	GetPicture () const  = 0;

	/** Port setting and restoration (use before and after any drawing suite)
	@param inNoDraw
		true: use context for other purpose than drawing (like get metrics etc, set or get font, set or get fill color, etc...)
				(for some impl like D2D, you should never set to false if you do not intend to draw between BeginUsingContext/EndUsingContext
				 - which seems to happen quite often within form context & virtual interface...)
		false: use context for drawing (default)
	*/
	virtual	void	BeginUsingContext (bool inNoDraw = false) = 0;
	virtual	void	EndUsingContext () = 0;

	// pp same as _GetParentPort/_ReleaseParentPort but special case for gdi+/gdi 
	// hdc clipregion is not sync with gdi+ clip region, to sync clip use this function
	virtual	ContextRef	BeginUsingParentContext()const{return _GetParentContext();};
	virtual	void	EndUsingParentContext(ContextRef inContextRef)const{_ReleaseParentContext(inContextRef);};
	
	virtual	PortRef	BeginUsingParentPort()const{return _GetParentPort();};
	virtual	void	EndUsingParentPort(PortRef inPortRef)const{_ReleaseParentPort(inPortRef);};

	virtual	PortRef		_GetParentPort () const = 0;
	virtual	ContextRef	_GetParentContext () const = 0;
	
	virtual	void	_ReleaseParentPort (PortRef inPortRef) const {;};
	virtual	void	_ReleaseParentContext (ContextRef inContextRef) const {;};
	
#if VERSIONMAC	
	virtual void	GetParentPortBounds( VRect& outBounds) {}
#endif

	/** set to true to inform context returned by _GetHDC()/BeginUsingParentContext will not be used for drawing 
	@remarks
		used only by Direct2D impl
	*/
	virtual void	_SetParentContextNoDraw(bool inStatus) const {}
	virtual bool	_GetParentContextNoDraw() const { return false; }

	// Utilities
	virtual	Boolean	UseEuclideanAxis () = 0;
	virtual	Boolean	UseReversedAxis () = 0;
	
	void _BuildRoundRectPath(const VRect inBounds,GReal inWidth,GReal inHeight,VGraphicPath& outPath, bool inFillOnly = false);
	
	// Debug Utilities

// This is a utility function used when debugging update
// mecanism. It try to remain as independant as possible
// from the framework. Use with care.
// TO BE MOVED INTO GUI
	#if VERSIONMAC
	static	void	_RevealUpdate(WindowRef inWindow);
	#elif VERSIONWIN
	static	void	_RevealUpdate(HWND inWindow);
	#endif

	static void	_RevealClipping (ContextRef inContext);
	static void	_RevealBlitting (ContextRef inContext, const RgnRef inRegion);
	static void	_RevealInval (ContextRef inContext, const RgnRef inRegion);
	
	static bool	sDebugNoClipping;		// Set to true to disable automatic clipping
	static bool	sDebugNoOffscreen;		// Set to true to disable offscreen drawing
	static bool	sDebugRevealClipping;	// Set to true to flash the clip region
	static bool	sDebugRevealUpdate;		// Set to true to flash the update region
	static bool	sDebugRevealBlitting;	// Set to true to flash the blit region
	static bool	sDebugRevealInval;		// Set to true to flash every inval region

	// Class initialization
	static Boolean	Init ();
	static void 	DeInit ();

	// Utilities
	
	static sWORD	_GetRowBytes (sWORD inWidth, sBYTE inDepth, sBYTE inPadding);
	static sBYTE*	_RotatePixels (GReal inRadian, sBYTE* ioBits, sWORD inRowBytes, sBYTE inDepth, sBYTE inPadding, const VRect& inSrcBounds, VRect& outDestBounds, uBYTE inFillByte = 0xFF);
	static void		_ApplyStandardPattern (PatternStyle inPatternStyle, ContextRef inContext);

};



/** text layout class
@remarks
	this class manages a text layout & pre-rendering of its content in a offscreen layer (if offscreen layer is enabled)

	for big text, it is recommended to use this class to draw text or get text metrics in order to optimize speed
				  because layout is preserved as long as layout properties or passed gc is compatible with internal layer

	You should call at least SetText method to define the text & optionally styles; all other layout properties have default values;

	to draw to a VGraphicContext, use method 'Draw( gc, posTopLeft)'
	you can get computed layout metrics for a specified gc with 'GetLayoutBounds( gc, bounds, posTopLeft)' 
	you can get also computed run metrics for a specified gc with 'GetRunBoundsFromRange( gc, vectorOfBounds, posTopLeft, start, end)';
	there are also methods to get caret metrics from pos or get char index from pos.

	If layout cache is enabled, this class manages a internal offscreen layer which is used to preserve layout metrics & eventually pre-render the text layout;
	this layer is a compatible layer created from the gc you pass to drawing or metrics methods; so as long as the layer is compatible with the passed gc,
	the impl layout or metrics do not need to be computed again & the offscreen layer might store the pre-rendered text which means metrics are computed again or text is redrawed
	only if text content has been modified or if the passed gc is no longer compatible with the internal layer.
	On default, text is pre-rendered on layer but you can change that with ShouldDrawOnLayer(false); if ShouldEnableCache() & !ShouldDrawOnLayer(), then layer is used only to preserve text layout (& metrics)
	Note that on Windows, if ShouldAllowClearTypeOnLayer() == true & ShouldDrawOnLayer() == true, then text is rendered on layer if & only if current text antialiasing is not set to ClearType.

	On Windows, it is safe to draw a VTextLayout to any graphic context but it is recommended whenever possible to draw to the same kind of graphic context 
	(ie a GDI, a GDIPlus, a hardware Direct2D gc or a software Direct2D gc) if offscreen layer is enabled otherwise as layer needs to be computed again if gc kind has changed, 
	offscreen layer in that case is not much useful; 

	This class is NOT thread-safe (as 4D GUI is single-threaded, it should not be a pb as this class should be used only by GUI component & GUI users)
*/
class XTOOLBOX_API VTextLayout: public VObject, public IRefCountable
{
public:
	/** VTextLayout constructor 
	
	@param inShouldEnableCache
		enable/disable cache (default is true): see ShouldEnableCache
    @note
		on Windows, if graphic context is GDI graphic context, offscreen layer is disabled because offscreen layers are not implemented in this context (FIXME ?)
	*/
	VTextLayout(bool inShouldEnableCache = true);
	virtual ~VTextLayout();

	/** return true if layout is valid/not dirty */
	bool IsValid() const { return fLayoutIsValid; }

	/** set dirty
	@remarks
		use this method if text owner is caller and caller has modified text without using VTextLayout methods
		or if styles owner is caller and caller has modified styles without using VTextLayout methods
	*/
	void SetDirty() { fLayoutIsValid = false; }

	bool IsDirty() const { return !fLayoutIsValid; }

	/** enable/disable cache
	@remarks
		true (default): VTextLayout will cache impl text layout & optionally rendered text content on multiple frames
						in that case, impl layout & text content are computed/rendered again only if offscreen layer is no longer compatible with graphic context 
						or if text layout is dirty;
						On default, text is pre-rendered on layer but you can change that with ShouldDrawOnLayer(false); if ShouldEnableCache() & !ShouldDrawOnLayer(), then layer is used only to preserve text layout (& metrics)
						Note that on Windows, if ShouldAllowClearTypeOnLayer() == true & ShouldDrawOnLayer() == true, then text is rendered on layer if & only if current text antialiasing is not set to ClearType.

		false:			disable layout cache
		
		if VGraphicContext::fPrinterScale == true - ie while printing, cache is automatically disabled so you can enable cache even if VTextLayout instance is used for both screen drawing & printing
	*/
	void ShouldEnableCache( bool inShouldEnableCache)
	{
		if (fShouldEnableCache == inShouldEnableCache)
			return;
		fShouldEnableCache = inShouldEnableCache;
		ReleaseRefCountable(&fLayerOffScreen);
		fLayerIsDirty = true;
	}
	bool ShouldEnableCache() const { return fShouldEnableCache; }


	/** allow/disallow offscreen text rendering (default is true)
	@remarks
		text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
		you should call ShouldDrawOnLayer(false) only if you want to cache only text layout & metrics but not text rendering
		
		Note that on Windows, if ShouldEnableCache() == true, ShouldAllowClearTypeOnLayer() == true & ShouldDrawOnLayer() == true, 
		then text is rendered on layer if & only if current text antialiasing is not set to ClearType.
	@see
		ShouldEnableCache
	*/
	void ShouldDrawOnLayer( bool inShouldDrawOnLayer)
	{
		if (fShouldDrawOnLayer == inShouldDrawOnLayer)
			return;
		fShouldDrawOnLayer = inShouldDrawOnLayer;
		fLayerIsDirty = true;
		if (fGC && !fShouldDrawOnLayer && !fIsPrinting)
			_ResetLayer();
	}
	bool ShouldDrawOnLayer() const { return fShouldDrawOnLayer; }

	/** enable ClearType antialiasing on layer (default is false)
	@remarks
		this flag is only mandatory on Windows platform & if offscreen layer is enabled

		On Windows, ClearType antialiasing is disabled on default (replaced by grayscale antialiasing) while rendering in offscreen layer bitmap
		(because offscreen bitmap bilinear or bicubic interpolation would screw up ClearType which is tightly dependent on monitor color channels order)

		if you enable ClearType on layer & if ClearType is enabled in passed gc, 
		offscreen layer will be used only to compute & preserve text layout & metrics
		but text will be redrawed at each frame & not pre-rendered in offscreen layer in order to allow ClearType antialiasing to be properly applied
		(if offscreen layer is enabled & ClearType is disabled in passed gc, text will be pre-rendered in offscreen layer no matter this flag status)
	*/
	void ShouldAllowClearTypeOnLayer( bool inShouldAllowClearTypeOnLayer = true) 
	{
		if (fShouldAllowClearTypeOnLayer == inShouldAllowClearTypeOnLayer)
			return;
		fShouldAllowClearTypeOnLayer = inShouldAllowClearTypeOnLayer;
		ReleaseRefCountable(&fLayerOffScreen);
		fLayerIsDirty = true;
	}
	bool ShouldAllowClearTypeOnLayer() const
	{
		return fShouldAllowClearTypeOnLayer;
	}

	/** set layer view rectangle (user space is VView window user space - hwnd user space)
	@remarks
		useful only if offscreen layer is enabled

		by default, layer surface size is equal to text layout size in pixels
		but it can be useful to constraint layer surface size to not more than the visible area size of the text layout container
		in order to avoid to draw offscreen all the text layout (especially if the text size is great...)

		for instance if VTextLayout container is a VView-derived class & fTextLayout is the text layout attached or used in the VView-derived class,
		in the VView-derived class DoDraw method & prior to call VTextLayout::Draw, you should insert this code to set the layer view rect:

		[code/]

		VRect boundsHwnd;
		GetHwndBounds(boundsHwnd);
		fTextLayout->SetLayerViewRect( boundsHwnd);

		[/code]
		
		with that code, layer size will be not greater than min of layout size & VView visible area
		(see class VStyledTextEdtiView for a sample)

		You can still reset to VRect() to use layout size as layer size if you intend to pre-render all text layout 
		(only if you have called before SetLayerViewRect with a not empty VRect)
	*/
	void SetLayerViewRect( const VRect& inRectRelativeToWnd) 
	{
		if (fLayerViewRect == inRectRelativeToWnd)
			return;
		fLayerViewRect = inRectRelativeToWnd;
		fLayerIsDirty = true;
	}
	const VRect& GetLayerViewRect() const { return fLayerViewRect; }

	/** insert text at the specified position */
	void InsertText( sLONG inPos, const VString& inText);

	/** delete text range */
	void DeleteText( sLONG inStart, sLONG inEnd);

	/** apply style (use style range) */
	bool ApplyStyle( VTextStyle* inStyle);


	/** set default font 
	@remarks
		by default, base font is current gc font BUT: 
		it is highly recommended to call this method to avoid to inherit default font from gc
		& bind a default font to the text layout
		otherwise if gc font has changed, text layout needs to be computed again if there is no default font defined

		by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
	*/
	void SetDefaultFont( VFont *inFont, GReal inDPI = 72.0f);

	/** get & retain default font 
	@remarks
		by default, return 4D form-compliant font (with dpi = 72) so not the actual fDPI-scaled font 
		(for consistency with SetDefaultFont & because fDPI internal scaling should be transparent for caller)
	*/
	VFont *RetainDefaultFont(GReal inDPI = 72.0f) const;

	/** set default text color 
	@remarks
		by default, base text color is current gc text color BUT: 
		it is highly recommended to call this method to avoid to inherit text color from gc
		& bind a default text color to the text layout
		otherwise if gc text color has changed, text layout needs to be computed again & if there is no default text color defined
	*/
	void SetDefaultTextColor( const VColor &inTextColor, bool inEnable = true)
	{
		if (fHasDefaultTextColor == inEnable)
		{
			if (fHasDefaultTextColor && fDefaultTextColor == inTextColor)
				return;
			if (!fHasDefaultTextColor)
				return;
		}
		fLayoutIsValid = false;
		fHasDefaultTextColor = inEnable;
		if (inEnable)
			fDefaultTextColor = inTextColor;
	}
	/** get default text color */
	bool GetDefaultTextColor(VColor& outColor) const 
	{ 
		if (!fHasDefaultTextColor)
			return false;
		outColor = fDefaultTextColor;
		return true; 
	}
	

	/** replace current text & styles 
	@remarks
		if inCopyStyles == true, styles are copied
		if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
	*/
	void SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

	/** replace current text & styles 
	@remarks
		here VTextLayout does not copy the input text but only keeps a reference on it if inCopyText == false (default): 
			caller still owns the layout text so caller should not destroy the referenced text before VTextLayout is destroyed
			if caller modifies inText, it should call this method again
			Also, InsertText & DeleteText will update inText (so caller does not need to update it if it uses these methods)

		if inCopyStyles == true, styles are copied
		if inCopyStyles == false, styles are retained: in that case, if you modify passed styles you should call this method again 
								  Also, ApplyStyle will update inStyles (so caller does not need to update it if it uses ApplyStyle)
	*/
	void SetText( VString* inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false, bool inCopyText = false);

	const VString& GetText() const
	{
#if VERSIONDEBUG
		xbox_assert(fTextPtr);
		if (fTextPtr != &fText)
		{
			//check fTextPtr validity: prevent access exception in case external VString ref has been released prior to VTextLayout
			try
			{
				sTextLayoutCurLength = fTextPtr->GetLength();
			}
			catch(...)
			{
				xbox_assert(false);
				static VString sTextDummy = "READ ACCESS ERROR: VTextLayout::GetText text pointer is invalid";
				return sTextDummy;
			}
		}
#endif
		return *fTextPtr;
	}

	VString *GetExternalTextPtr() const { return fTextPtrExternal; }

	/** set text styles 
	@remarks
		if inCopyStyles == true, styles are copied
		if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
	*/
	void SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

	VTreeTextStyle *GetStyles() const
	{
		return fStyles;
	}
	VTreeTextStyle *RetainStyles() const
	{
		return RetainRefCountable(fStyles);
	}

	/** set extra text styles 
	@remarks
		it can be used to add a temporary style effect to the text layout without modifying the main text styles
		(for instance to add a text selection effect: see VStyledTextEditView)

		if inCopyStyles == true, styles are copied
		if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
	*/
	void SetExtStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

	VTreeTextStyle *GetExtStyles() const
	{
		VTaskLock protect(&fMutex); //as styles can be retained, we protect access 
		return fExtStyles;
	}
	VTreeTextStyle *RetainExtStyles() const
	{
		VTaskLock protect(&fMutex); //as styles can be retained, we protect access 
		return RetainRefCountable(fExtStyles);
	}


	/** set max text width (0 for infinite) 
	@remarks
		it is also equal to text box width 
		so you should specify one if text is not aligned to the left border
		otherwise text layout box width will be always equal to formatted text width
	*/ 
	void SetMaxWidth(GReal inMaxWidth = 0.0f) 
	{
		if (fMaxWidth == inMaxWidth)
			return;
		fLayoutIsValid = false;
		fMaxWidth = inMaxWidth;
	}
	GReal GetMaxWidth() const { return fMaxWidth; }

	/** set max text height (0 for infinite) 
	@remarks
		it is also equal to text box height 
		so you should specify one if paragraph is not aligned to the top border
		otherwise text layout box height will be always equal to formatted text height
	*/ 
	void SetMaxHeight(GReal inMaxHeight = 0.0f) 
	{
		if (fMaxHeight == inMaxHeight)
			return;
		fLayoutIsValid = false;
		fMaxHeight = inMaxHeight;
	}
	GReal GetMaxHeight() const { return fMaxHeight; }

	/** set text horizontal alignment (default is AL_DEFAULT) */
	void SetTextAlign( AlignStyle inHAlign = AL_DEFAULT)
	{
		if (fHAlign == inHAlign)
			return;
		fLayoutIsValid = false;
		fHAlign = inHAlign;
	}
	/** get text horizontal aligment */
	AlignStyle GetTextAlign() const { return fHAlign; }

	/** set text paragraph alignment (default is AL_DEFAULT) */
	void SetParaAlign( AlignStyle inVAlign = AL_DEFAULT)
	{
		if (fVAlign == inVAlign)
			return;
		fLayoutIsValid = false;
		fVAlign = inVAlign;
	}
	/** get text paragraph aligment */
	AlignStyle GetParaAlign() const { return fVAlign; }


	/** set text layout mode (default is TLM_NORMAL) */
	void SetLayoutMode( TextLayoutMode inLayoutMode = TLM_NORMAL)
	{
		if (fLayoutMode == inLayoutMode)
			return;
		fLayoutIsValid = false;
		fLayoutMode = inLayoutMode;
	}
	/** get text layout mode */
	TextLayoutMode GetLayoutMode() const { return fLayoutMode; }

	/** set font DPI (default is 4D form DPI = 72) */
	void SetDPI( GReal inDPI = 72.0f);

	/** get font DPI */
	GReal GetDPI() const { return fDPI; }

	/** begin using text layout for the specified gc (reentrant)
	@remarks
		for best performance, it is recommended to call this method prior to more than one call to Draw or metrics get methods & terminate with EndUsingContext
		otherwise Draw or metrics get methods will check again if layout needs to be updated at each call to Draw or metric get method
		But you do not need to call BeginUsingContext & EndUsingContext if you call only once Draw or a metrics get method.

		CAUTION: if you call this method, you must then pass the same gc to Draw or metrics get methods before EndUsingContext is called
				 otherwise Draw or metrics get methods will do nothing (& a assert failure is raised in debug)

		If you do not call Draw method, you should pass inNoDraw = true to avoid to create a drawing context on some impls (actually for now only Direct2D impl uses this flag)
	*/
	void BeginUsingContext( VGraphicContext *inGC, bool inNoDraw = false);

	/** draw text layout in the specified graphic context 
	@remarks
		text layout left top origin is set to inTopLeft
		text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
		text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
	*/
	void Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint())
	{
		if (!testAssert(fGC == NULL || fGC == inGC))
			return;
		xbox_assert(inGC);
		inGC->_DrawTextLayout( this, inTopLeft);
	}
	
	/** return formatted text bounds (typo bounds)
	@remarks
		text typo bounds origin is set to inTopLeft

	    as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
		because layout bounds are equal to the text box design bounds and not to the formatted text bounds
		you should use this method to get formatted text width & height 
	*/
	void GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false)
	{
		//call GetLayoutBounds in order to update both layout & typo metrics
		GetLayoutBounds( inGC, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
		outBounds.SetWidth( fCurWidth);
		outBounds.SetHeight( fCurHeight);
	}

	/** return text layout bounds 
	@remarks
		text layout origin is set to inTopLeft
		on output, outBounds contains text layout bounds including any glyph overhangs; outBounds left top origin should be equal to inTopLeft

		on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
		(if max width or max height is equal to 0.0f, it is assumed to be infinite & so layout bounds will be equal to formatted text bounds)
		output layout width will be equal to max width if maw width != 0 otherwise to formatted text width
		output layout height will be equal to max height if maw height != 0 otherwise to formatted text height
		(see CAUTION remark below)

		if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
		(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
		 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)

	    CAUTION: layout bounds can be greater than formatted text bounds (if max width or max height is not equal to 0)
				 because layout bounds are equal to the text box design bounds and not to the formatted text bounds: 
				 you should use GetTypoBounds in order to get formatted text width & height
	*/
	void GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false)
	{
		if (inGC)
		{
			if (!testAssert(fGC == NULL || fGC == inGC))
				return;
			inGC->_GetTextLayoutBounds( this, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
		}
		else
		{
			//return stored metrics

			if (fTextLength == 0 && !inReturnCharBoundsForEmptyText)
			{
				outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
				return;
			}
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), fCurLayoutWidth, fCurLayoutHeight);
		}
	}

	/** return text layout run bounds from the specified range 
	@remarks
		text layout origin is set to inTopLeft
	*/
	void GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1)
	{
		if (!testAssert(fGC == NULL || fGC == inGC))
			return;
		xbox_assert(inGC);
		inGC->_GetTextLayoutRunBoundsFromRange( this, outRunBounds, inTopLeft, inStart, inEnd);
	}

	/** return the caret position & height of text at the specified text position in the text layout
	@remarks
		text layout origin is set to inTopLeft

		text position is 0-based

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
		if text layout is drawed at inTopLeft

		by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
		but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
	void GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true)
	{
		if (!testAssert(fGC == NULL || fGC == inGC))
			return;
		xbox_assert(inGC);
		inGC->_GetTextLayoutCaretMetricsFromCharIndex( this, inTopLeft, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics);
	}


	/** return the text position at the specified coordinates
	@remarks
		text layout origin is set to inTopLeft (in gc user space)
		the hit coordinates are defined by inPos param (in gc user space)

		output text position is 0-based

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
	bool GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex)
	{
		if (!testAssert(fGC == NULL || fGC == inGC))
			return false;
		xbox_assert(inGC);
		return inGC->_GetTextLayoutCharIndexFromPos( this, inTopLeft, inPos, outCharIndex);
	}

	/** return true if layout uses truetype font only */
	bool UseFontTrueTypeOnly(VGraphicContext *inGC) const;

	/** end using text layout (reentrant) */
	void EndUsingContext();

public:
	bool _ApplyStyle(VTextStyle *inStyle);

	/** retain default uniform style 
	@remarks
		if fStyles == NULL, theses styles are text content uniform styles (monostyle)
	*/
	VTreeTextStyle *_RetainDefaultTreeTextStyle() const;

	/** retain full styles
	@remarks
		use this method to get the full styles applied to the text content
		(but not including the graphic context default font: this font is already applied internally by gc methods before applying styles)

		caution: please do not modify fStyles between _RetainFullTreeTextStyle & _ReleaseFullTreeTextStyle

		(in monostyle, it is generally equal to _RetainDefaultTreeTextStyle)
	*/
	VTreeTextStyle *_RetainFullTreeTextStyle() const;

	void _ReleaseFullTreeTextStyle( VTreeTextStyle *inStyles) const;

	AlignStyle _ToAlignStyle( justificationStyle inJust) const
	{
		switch (inJust)
		{
		case JST_Left:
			return AL_LEFT;
			break;
		case JST_Right:
			return AL_RIGHT;
			break;
		case JST_Center:
			return AL_CENTER;
			break;
		case JST_Justify:
			return AL_JUST;
			break;
		case JST_Mixed:
		case JST_Default:
		default:
			return AL_DEFAULT;
			break;
		}
	}

	void _CheckStylesConsistency();

	bool _BeginDrawLayer(const VPoint& inTopLeft = VPoint());
	void _EndDrawLayer();

	/** set text layout graphic context 
	@remarks
		it is the graphic context to which is bound the text layout
		if gc is changed and offscreen layer is disabled or gc is not compatible with the actual offscreen layer gc, text layout needs to be computed & redrawed again 
		so it is recommended to enable the internal offscreen layer in order to preserve layout on multiple frames
		(and to not redraw text layout at every frame) because offscreen layer & text content is preserved as long as layer is compatible with the attached gc & text content is not dirty
	*/
	void _SetGC( VGraphicContext *inGC);
	
	/** return the graphic context bound to the text layout 
	@remarks
		gc is not retained
	*/
	VGraphicContext *_GetGC() { return fGC; }

	/** update text layout if it is not valid */
	void _UpdateTextLayout();

	void _BeginUsingStyles();
	void _EndUsingStyles();

	void _ResetLayer(VGraphicContext *inGC = NULL, bool inAlways = false);

	/** return true if layout styles uses truetype font only */
	bool _StylesUseFontTrueTypeOnly() const;

	bool fLayoutIsValid;
	
	bool fShouldEnableCache;
	bool fShouldDrawOnLayer;
	bool fShouldAllowClearTypeOnLayer;
	VImageOffScreen *fLayerOffScreen;
	VRect fLayerViewRect;
	VPoint fCurLayerOffsetViewRect;
	bool fLayerIsDirty;
	bool fLayerIsForMetricsOnly;

	VGraphicContext *fGC;
	sLONG fGCUseCount;

	VFont *fDefaultFont;
	VColor fDefaultTextColor;
	bool fHasDefaultTextColor;
	mutable VTreeTextStyle *fDefaultStyle;

	VFont *fCurFont;
	VColor fCurTextColor;
	GReal fCurKerning;
	TextRenderingMode fCurTextRenderingMode;
	/** width of formatted text */
	GReal fCurWidth; 
	/** height of formatted text */
	GReal fCurHeight;
	/** width of text layout box 
	@remarks
		it is equal to fMaxWidth if fMaxWidth != 0.0f
		otherwise it is equal to fCurWidth
	*/
	GReal fCurLayoutWidth;
	/** height of text layout box 
	@remarks
		it is equal to fMaxHeight if fMaxHeight != 0.0f
		otherwise it is equal to fCurHeight
	*/
	GReal fCurLayoutHeight;

	GReal fCurOverhangLeft;
	GReal fCurOverhangRight;
	GReal fCurOverhangTop;
	GReal fCurOverhangBottom;

	sLONG fTextLength;
	VString fText;
	VString *fTextPtrExternal;
	const VString *fTextPtr;
	VTreeTextStyle *fStyles;
	VTreeTextStyle *fExtStyles;
	
	GReal fMaxWidth;
	GReal fMaxHeight;

	AlignStyle fHAlign;
	AlignStyle fVAlign;

	TextLayoutMode fLayoutMode;
	GReal fDPI;
	
	bool fUseFontTrueTypeOnly;
	bool fStylesUseFontTrueTypeOnly;

	bool fNeedUpdateBounds;

	/** text box layout used by Quartz2D or GDIPlus/GDI (Windows legacy impl) */
	VStyledTextBox *fTextBox;
#if ENABLE_D2D
	/** DWrite text layout used by Direct2D impl only (in not legacy mode)*/
	IDWriteTextLayout *fLayoutD2D;
#endif

	VFont *fBackupFont;
	VColor fBackupTextColor;
	bool fSkipDrawLayer;

	bool fIsPrinting;

#if VERSIONDEBUG
	mutable sLONG sTextLayoutCurLength;
#endif

	mutable VCriticalSection fMutex;
};


/** class used to deal gracefully with path crispEdges status */
class StUsePath
{
public:
	StUsePath( VGraphicContext *inGC, const VGraphicPath *inPath, bool inFillOnly = false)
	{
		fIsOwned = false;
		fPathConst = fPath = NULL;
		if (inGC->IsShapeCrispEdgesEnabled() && inGC->IsAllowedCrispEdgesOnPath())
		{
			fPath = inPath->CloneWithCrispEdges( inGC, inFillOnly);
			if (fPath)
			{
				fPathConst = fPath;
				fIsOwned = true;
			}
			else
				fPathConst = inPath;
		}
		else
			fPathConst = inPath;
	}
	StUsePath( VGraphicContext *inGC, VGraphicPath *inPath, bool inFillOnly = false)
	{
		fIsOwned = false;
		fPathConst = fPath = NULL;
		if (inGC->IsShapeCrispEdgesEnabled() && inGC->IsAllowedCrispEdgesOnPath())
		{
			fPath = inPath->CloneWithCrispEdges( inGC, inFillOnly);
			if (fPath)
				fIsOwned = true;
			else
				fPath = inPath;
		}
		else
			fPath = inPath;
		fPathConst = fPath;
	}
	virtual ~StUsePath()
	{
		if (fIsOwned)
			delete fPath;
	}

	const VGraphicPath *Get() const
	{
		return fPathConst;
	}
	VGraphicPath *GetModifiable() 
	{
		return fPath;
	}
private:
	const VGraphicPath *fPathConst;
	VGraphicPath *fPath;
	bool fIsOwned;
};

// This class automate SaveContext/SaveContext calls.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StSaveContext
{
public:
	StSaveContext (const VGraphicContext* inContext)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		fContext->Retain(); 
		fContext->SaveContext();
	};
	
	virtual ~StSaveContext ()
	{
		fContext->RestoreContext();
		fContext->Release();
	};
	
private:
	VGraphicContext*	fContext;
};


// This class automate SaveContext/SaveContext calls.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StSaveContext_NoRetain
{
public:
	StSaveContext_NoRetain (const VGraphicContext* inContext)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		fContext->SaveContext();
	};
	
	virtual ~StSaveContext_NoRetain ()
	{
		fContext->RestoreContext();
	};
	
private:
	VGraphicContext*	fContext;
};


// This class automate SetClip/RestoreClip calls.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StSetClipping
{
public:
	StSetClipping (const VGraphicContext* inContext)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		if ( fContext )
		{
			fContext->Retain(); 
			fContext->SaveClip();
		}
	};
	
	StSetClipping (const VGraphicContext* inContext, const VRegion& inHwndRegion, Boolean inAddToPrevious = false)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		if ( fContext )
		{
			fContext->Retain(); 
			fContext->SaveClip();
			fContext->ClipRegion(inHwndRegion, inAddToPrevious);
		}
	};
	
	StSetClipping (const VGraphicContext* inContext, const VRect& inHwndRect, Boolean inAddToPrevious = false)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		if ( fContext )
		{
			fContext->Retain(); 
			fContext->SaveClip();
			fContext->ClipRect(inHwndRect, inAddToPrevious);
		}
	};
			
	virtual ~StSetClipping ()
	{
		if ( fContext )
		{
			fContext->RestoreClip();
			fContext->Release();
		}
	};
	
private:
	VGraphicContext*	fContext;
	
#if VERSIONDEBUG	// Don't allocate in heap !
	StSetClipping () {};
#endif
};



// Same as StSetContext but with no Retain/Release. Reserved for VGraphiccontext internal use.
//
class StUseContext_NoRetain
{
public:
	StUseContext_NoRetain (const VGraphicContext* inContext) { fContext = const_cast<VGraphicContext*>(inContext); fContext->BeginUsingContext(); };	
	virtual ~StUseContext_NoRetain () { fContext->EndUsingContext(); };
	
#if VERSIONMAC
	operator PortRef () { return fContext->_GetParentPort(); };
#endif

	operator ContextRef () { return fContext->_GetParentContext(); };
	
private:
	VGraphicContext*	fContext;
	
#if VERSIONDEBUG	// Don't allocate in heap !
	StUseContext_NoRetain () {};
#endif
};


// Same as StSetContext but with no Retain/Release. Reserved for VGraphiccontext internal use.
//
class StUseContext_NoRetain_NoDraw
{
public:
	StUseContext_NoRetain_NoDraw (const VGraphicContext* inContext) { fContext = const_cast<VGraphicContext*>(inContext); fContext->BeginUsingContext(true); };	
	virtual ~StUseContext_NoRetain_NoDraw () { fContext->EndUsingContext(); };
	
#if VERSIONMAC
	operator PortRef () { return fContext->_GetParentPort(); };
#endif

	operator ContextRef () { return fContext->_GetParentContext(); };
	
private:
	VGraphicContext*	fContext;
	
#if VERSIONDEBUG	// Don't allocate in heap !
	StUseContext_NoRetain_NoDraw () {};
#endif
};





// This class automate BeginUsingContext/EndUsingContext calls without saving the settings.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StUseContext
{
public:
	StUseContext (const VGraphicContext* inContext)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		fContext->Retain();
		fContext->BeginUsingContext();
	};
	
	virtual ~StUseContext ()
	{
		fContext->EndUsingContext();
		fContext->Release();
	};
	
	virtual VGraphicContext*	GetGraphicContext () { return fContext; };
	
#if VERSIONMAC
	operator PortRef () { return fContext->_GetParentPort(); };
#endif

	operator ContextRef () { return fContext->_GetParentContext(); };
	
private:
	VGraphicContext*	fContext;

#if VERSIONDEBUG	// Don't allocate in heap !
	StUseContext () {};
#endif
};


// This class automate BeginUsingContext/EndUsingContext calls without saving the settings.
// As for all 'St' (stack based) classes you don't instantiate with 'new' operator.
//
class StUseContext_NoDraw
{
public:
	StUseContext_NoDraw (const VGraphicContext* inContext)
	{
		fContext = const_cast<VGraphicContext*>(inContext);
		fContext->Retain();
		fContext->BeginUsingContext(true);
	};
	
	virtual ~StUseContext_NoDraw ()
	{
		fContext->EndUsingContext();
		fContext->Release();
	};
	
	virtual VGraphicContext*	GetGraphicContext () { return fContext; };
	
#if VERSIONMAC
	operator PortRef () { return fContext->_GetParentPort(); };
#endif

	operator ContextRef () { return fContext->_GetParentContext(); };
	
private:
	VGraphicContext*	fContext;

#if VERSIONDEBUG	// Don't allocate in heap !
	StUseContext_NoDraw () {};
#endif
};


#if VERSIONWIN
class StGDIUseGraphicsAdvanced
{
public:
	StGDIUseGraphicsAdvanced(HDC hdc, bool inSaveTransform = false)
	{
		fHDC = hdc;
		fOldMode = ::GetGraphicsMode( fHDC);
		if (fOldMode != GM_ADVANCED)
			::SetGraphicsMode( fHDC, GM_ADVANCED);

		fRestoreCTM = inSaveTransform;
		if (fRestoreCTM)
		{
			BOOL ok = ::GetWorldTransform(fHDC, &fCTM);
			if (!ok)
				fRestoreCTM = false;
		}
	}
	virtual ~StGDIUseGraphicsAdvanced()
	{
		if (fRestoreCTM)
		{
			BOOL ok = ::SetWorldTransform(fHDC, &fCTM);
			xbox_assert(ok == TRUE);
		}
		if (fOldMode != GM_ADVANCED)
			::SetGraphicsMode( fHDC, fOldMode);
	}

private:
	HDC fHDC;
	int fOldMode;
	bool fRestoreCTM;
	XFORM fCTM;
};
#endif



/** will force automatic legacy text rendering on Windows:
	if font is truetype, use impl native text rendering otherwise use GDI for text rendering
@remarks
	caution: you should use same setting both while getting metrics (text bounds) & rendering
*/
class StUseTextAutoLegacy
{
public:
#if VERSIONWIN
	StUseTextAutoLegacy(VGraphicContext *inGC)
	{
		xbox_assert(inGC);
		inGC->Retain();
		fGC = inGC;
		fTextRenderingMode = inGC->GetDesiredTextRenderingMode();
		fGC->SetTextRenderingMode( fTextRenderingMode & ~(TRM_LEGACY_OFF | TRM_LEGACY_ON));
	}
	virtual ~StUseTextAutoLegacy()
	{
		fGC->SetTextRenderingMode( fTextRenderingMode);
		fGC->Release();
	}
#else
	StUseTextAutoLegacy(VGraphicContext */*inGC*/)
	{
	}
	virtual ~StUseTextAutoLegacy()
	{
	}
#endif

private:
	VGraphicContext *fGC;
	TextRenderingMode fTextRenderingMode;
};

/** will force only GDI for text rendering on Windows 
@remarks
	caution: you should use same setting both while getting metrics (text bounds) & rendering
*/
class StUseTextLegacy
{
public:
#if VERSIONWIN
	StUseTextLegacy(VGraphicContext *inGC)
	{
		xbox_assert(inGC);
		inGC->Retain();
		fGC = inGC;
		fTextRenderingMode = inGC->GetDesiredTextRenderingMode();
		fGC->SetTextRenderingMode( (fTextRenderingMode & ~TRM_LEGACY_OFF) | TRM_LEGACY_ON);
	}
	virtual ~StUseTextLegacy()
	{
		fGC->SetTextRenderingMode( fTextRenderingMode);
		fGC->Release();
	}
#else
	StUseTextLegacy(VGraphicContext */*inGC*/)
	{
	}
	virtual ~StUseTextLegacy()
	{
	}
#endif

private:
	VGraphicContext *fGC;
	TextRenderingMode fTextRenderingMode;
};


/** will force only impl native text rendering on Windows (never use GDI for text rendering)
@remarks
	caution: you should use same setting both while getting metrics (text bounds) & rendering
*/
class StUseTextNoLegacy
{
public:
#if VERSIONWIN
	StUseTextNoLegacy(VGraphicContext *inGC)
	{
		xbox_assert(inGC);
		inGC->Retain();
		fGC = inGC;
		fTextRenderingMode = inGC->GetDesiredTextRenderingMode();
		fGC->SetTextRenderingMode( (fTextRenderingMode & ~TRM_LEGACY_ON) | TRM_LEGACY_OFF);
	}
	virtual ~StUseTextNoLegacy()
	{
		fGC->SetTextRenderingMode( fTextRenderingMode);
		fGC->Release();
	}
#else
	StUseTextNoLegacy(VGraphicContext */*inGC*/)
	{
	}
	virtual ~StUseTextNoLegacy()
	{
	}
#endif

private:
	VGraphicContext *fGC;
	TextRenderingMode fTextRenderingMode;
};


class StDisableShapeCrispEdges
{
public:
	StDisableShapeCrispEdges(VGraphicContext *inGC)
	{
		xbox_assert(inGC);
		inGC->Retain();
		fGC = inGC;
		fShapeCrispEdgesEnabled = fGC->IsShapeCrispEdgesEnabled();
		fGC->EnableShapeCrispEdges( false);
	}
	virtual ~StDisableShapeCrispEdges()
	{
		fGC->EnableShapeCrispEdges( fShapeCrispEdgesEnabled);
		fGC->Release();
	}

private:
	VGraphicContext *fGC;
	bool fShapeCrispEdgesEnabled;
};

class StEnableShapeCrispEdges
{
public:
	StEnableShapeCrispEdges(VGraphicContext *inGC)
	{
		xbox_assert(inGC);
		inGC->Retain();
		fGC = inGC;
		fShapeCrispEdgesEnabled = fGC->IsShapeCrispEdgesEnabled();
		fGC->EnableShapeCrispEdges( true);
	}
	virtual ~StEnableShapeCrispEdges()
	{
		fGC->EnableShapeCrispEdges( fShapeCrispEdgesEnabled);
		fGC->Release();
	}

private:
	VGraphicContext *fGC;
	bool fShapeCrispEdgesEnabled;
};

class StSetAntiAliasing
{
public:
	StSetAntiAliasing(VGraphicContext *inGC,bool inEnable)
	{
		xbox_assert(inGC);
		inGC->Retain();
		fGC = inGC;
		//we must save the desired antialiasing (which is not modified by max perf flag)
		fAntiAliasing = fGC->GetDesiredAntiAliasing();
		if(inEnable)
			fGC->EnableAntiAliasing();
		else 
			fGC->DisableAntiAliasing();
				
	}
	virtual ~StSetAntiAliasing()
	{
		if(fAntiAliasing)
			fGC->EnableAntiAliasing();
		else 
			fGC->DisableAntiAliasing();

		fGC->Release();
	}
	
private:
	VGraphicContext *fGC;
	bool fAntiAliasing;
};

class StSetAntiAliasing_NoRetain
{
public:
	StSetAntiAliasing_NoRetain(VGraphicContext *inGC,bool inEnable)
	{
		xbox_assert(inGC);
		
		fGC = inGC;
		//we must save the desired antialiasing (which is not modified by max perf flag)
		fAntiAliasing = fGC->GetDesiredAntiAliasing();
		if(inEnable)
			fGC->EnableAntiAliasing();
		else 
			fGC->DisableAntiAliasing();
		
	}
	virtual ~StSetAntiAliasing_NoRetain()
	{
		if(fAntiAliasing)
			fGC->EnableAntiAliasing();
		else 
			fGC->DisableAntiAliasing();
		
	}
	
private:
	VGraphicContext *fGC;
	bool fAntiAliasing;
};


/** static class used to inform next BeginUsingParentContext that the returned parent context will NOT be used for drawing */
class StParentContextNoDraw : public VObject
{
	public:
	StParentContextNoDraw(const VGraphicContext* inGC)
	{
		fGC = inGC;
		fParentContextNoDraw = inGC->_GetParentContextNoDraw();
		inGC->_SetParentContextNoDraw(true);
	}
	virtual ~StParentContextNoDraw()
	{
		fGC->_SetParentContextNoDraw(fParentContextNoDraw);
	}

	private:
	const VGraphicContext* fGC;
	bool fParentContextNoDraw;
};


/** static class used to inform next BeginUsingParentContext that the returned parent context will be used for drawing */
class StParentContextDraw : public VObject
{
	public:
	StParentContextDraw(const VGraphicContext* inGC, bool inDraw = true)
	{
		fGC = inGC;
		fParentContextNoDraw = inGC->_GetParentContextNoDraw();
		inGC->_SetParentContextNoDraw(!inDraw);
	}
	virtual ~StParentContextDraw()
	{
		fGC->_SetParentContextNoDraw(fParentContextNoDraw);
	}

	private:
	const VGraphicContext* fGC;
	bool fParentContextNoDraw;
};



inline Real	CmsToTwips (Real x) { return (x * 1440.0 / 2.54); };
inline Real	TwipsToCms (Real x) { return (x * 2.54 / 1440.0); };
inline Real	InchesToTwips (Real x) { return (x * 1440.0); };
inline Real	TwipsToInches (Real x) { return (x / 1440.0); };
inline Real	PointsToTwips (Real x) { return (x * 20.0); };
inline Real	TwipsToPoints (Real x) { return (x / 20.0); };

END_TOOLBOX_NAMESPACE

#if VERSIONMAC
#include "Graphics/Sources/XMacQuartzGraphicContext.h"
#endif

#if VERSIONWIN
#include "Graphics/Sources/XWinGDIGraphicContext.h"
#include "Graphics/Sources/XWinGDIPlusGraphicContext.h"
#endif

#endif
