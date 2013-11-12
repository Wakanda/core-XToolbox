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
#include "V4DPictureIncludeBase.h"
#include "V4DPictureProxyCache.h"
#include "VGraphicContext.h"
#if VERSIONWIN
#include "XWinGDIPlusGraphicContext.h"
#include "XWinStyledTextBox.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif
#endif
#if VERSIONMAC
#include "XMacQuartzGraphicContext.h"
#endif
#include "VStyledTextBox.h"

#include "VDocBaseLayout.h"
#include "VDocBorderLayout.h"
#include "VDocImageLayout.h"
#include "VDocParagraphLayout.h"
#include "VDocTableLayout.h"
#include "VDocTextLayout.h"

#undef min
#undef max

/** default selection color (Word-like color) */
#define VTEXTLAYOUT_SELECTION_ACTIVE_COLOR XBOX::VColor(81,155,227,128)
#define VTEXTLAYOUT_SELECTION_INACTIVE_COLOR XBOX::VColor(200,200,200,128)

#define VTEXTLAYOUT_HIGHDPI	72*20

/**	VTextLayout constructor
@param inPara
	VDocParagraph source: the passed paragraph is used only for initializing control plain text, styles & paragraph properties
						  but inPara is not retained 
	(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
@param inShouldEnableCache
	enable/disable cache (default is true): see ShouldEnableCache
*/
VTextLayout::VTextLayout(const VDocParagraph *inPara, bool inShouldEnableCache, bool inShouldUseNativeLayout)
{
	xbox_assert(inPara);
	_Init();
	fShouldEnableCache = inShouldEnableCache;
	fShouldUseNativeLayout = inShouldUseNativeLayout;
	if (!fShouldUseNativeLayout)
		fParagraphLayout = new VDocParagraphLayout(this);

	_ReplaceText( inPara);

	VTreeTextStyle *styles = inPara->RetainStyles();
	ApplyStyles( styles);
	ReleaseRefCountable(&styles);
	
	_ApplyParagraphProps( inPara, false);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	ReleaseRefCountable(&fDefaultStyle);
}

/** use native platform for paragraph layout or not

@param inShouldUseNativeLayout
	true (default): compute paragraph layout with native impl (e.g. GDI/RTE/D2D on Windows, CoreText on Mac)
	false: compute paragraph layout with platform independent impl (using ICU & platform independent algorithm) 
		   only layout basic runs (same direction & style) are still computed & rendered with native impl (GDI/D2D on Windows, CoreText on Mac)
@note
	important: complex paragraph features like embedded objects, bullets and so on are supported only with inShouldUseNativeLayout = false 
			   (in V15, it will be used to render new 4D XHTML document format) 
*/
void VTextLayout::ShouldUseNativeLayout(bool inShouldUseNativeLayout) 
{
	VTaskLock protect(&fMutex);
	if (fShouldUseNativeLayout == inShouldUseNativeLayout)
		return;
	fShouldUseNativeLayout = inShouldUseNativeLayout;
	if (!fShouldUseNativeLayout)
	{
		fParagraphLayout = new VDocParagraphLayout( this);
		
		//release native layout if any
		ReleaseRefCountable(&fTextBox);
#if ENABLE_D2D
		if (fLayoutD2D)
		{
			fLayoutD2D->Release();
			fLayoutD2D = NULL;
		}
#endif
		SetMargins( fMarginTWIPS);
	}
	else
	{
		if (fParagraphLayout)
		{
			delete fParagraphLayout;
			fParagraphLayout = NULL;
		}
		SetMargins( fMarginTWIPS);
	}
	ReleaseRefCountable(&fLayerOffScreen);
	fLayerIsDirty = true;
}


/** create and retain paragraph from the current layout plain text, styles & paragraph properties */
VDocParagraph *VTextLayout::CreateParagraph(sLONG inStart, sLONG inEnd)
{
	VDocParagraph *para = new VDocParagraph();

	if (inEnd == -1)
		inEnd = fTextLength;

	//copy plain text
	if (inStart == 0 && inEnd >= fTextLength)
	{
		para->SetText( *fTextPtr);
		if (fTextLength < para->GetTextPtr()->GetLength())
			para->GetTextPtr()->Truncate(fTextLength);
	}
	else
	{
		VString text;
		fTextPtr->GetSubString(inStart+1, inEnd-inStart, text);
		para->SetText( text);
	}

	//copy text character styles
	VTreeTextStyle *styles = _RetainFullTreeTextStyle();
	if (styles)
	{
		xbox_assert(styles == fDefaultStyle);
		//reset text align & back color as they are paragraph props
		styles->GetData()->SetJustification( JST_Notset);
		styles->GetData()->SetHasBackColor( false);

		//bool isDefaultStylesDirty = false;
		//if (styles == fDefaultStyle && styles->GetData()->GetHasBackColor())
		//{
		//	fDefaultStyle->GetData()->SetHasBackColor(false); //ensure to reset back color as it is parent frame back color
		//	isDefaultStylesDirty = true;
		//}

		VTreeTextStyle *stylesOnRange = NULL;
		bool copyStyles = true;
		if (inStart == 0 && inEnd >= fTextLength)
			stylesOnRange = RetainRefCountable( styles);
		else
		{
			stylesOnRange = styles->CreateTreeTextStyleOnRange(inStart, inEnd, true, false);
			copyStyles = false;
		}

	
		if (stylesOnRange->GetChildCount() > 0 && stylesOnRange->GetNthChild(1)->HasSpanRefs())
		{
			xbox_assert(stylesOnRange->GetChildCount() == 1);

			VTreeTextStyle *newStylesOnRange = stylesOnRange->GetNthChild(1);
			newStylesOnRange->Retain();
			stylesOnRange->RemoveChildren();
			if (copyStyles)
			{
				copyStyles = false;
				VTreeTextStyle *newStyles = new VTreeTextStyle( newStylesOnRange);
				CopyRefCountable(&newStylesOnRange, newStyles);
				ReleaseRefCountable(&newStyles);
			}
			newStylesOnRange->GetData()->AppendStyle( stylesOnRange->GetData(), true);
			stylesOnRange->Release();
			stylesOnRange = newStylesOnRange;
		}

		para->SetStyles( stylesOnRange, copyStyles);
		ReleaseRefCountable(&stylesOnRange);
		
		_ReleaseFullTreeTextStyle( styles);
		
		ReleaseRefCountable(&fDefaultStyle); //as we have modified fDefaultStyle, clear it so it is created again next time it is requested
	}

	if (fParagraphLayout)
	{
		fParagraphLayout->SetPropsTo( static_cast<VDocNode *>(para));
		return para;
	}

	//copy paragraph properties
	if (!(VDocProperty(fMarginTWIPS) == para->GetDefaultPropValue( kDOC_PROP_MARGIN)))
		para->SetMargin( fMarginTWIPS);

	if (fHAlign != AL_DEFAULT)
		para->SetTextAlign( (eStyleJust)JustFromAlignStyle(fHAlign));
	if (fVAlign != AL_DEFAULT)
		para->SetVerticalAlign( (eStyleJust)JustFromAlignStyle(fVAlign));

	if (fBackColor.GetRGBAColor() != RGBACOLOR_TRANSPARENT)
		para->SetBackgroundColor( fBackColor.GetRGBAColor());

	return para;
}

/** apply paragraph properties & styles if any to the current layout (passed paragraph text is ignored) 
@param inPara
	the paragraph style
@param inResetStyles
	if true, reset styles to default prior to apply new paragraph style
@remarks
	such paragraph is just used for styling:
	so note that only paragraph first level VTreeTextStyle styles are applied (to the layout full plain text: paragraph style range is ignored)

	also only paragraph properties or character styles which are overriden are applied
	(so to ensure to replace all, caller needs to define explicitly all paragraph & character styles
	 or call ResetParagraphStyle() before)
*/
void VTextLayout::ApplyParagraphStyle( const VDocParagraph *inPara, bool inResetStyles, bool inApplyParagraphPropsOnly)
{
	fLayoutIsValid = false;
	fLayerIsDirty = true;
	ReleaseRefCountable(&fDefaultStyle);

	if (inResetStyles && !inApplyParagraphPropsOnly)
		_ResetStyles( false, true); //reset only char styles

	if (!inApplyParagraphPropsOnly && inPara->GetStyles())
	{
		//apply char styles
		VTextStyle *style = new VTextStyle( inPara->GetStyles()->GetData());
		style->SetRange(0, fTextLength);
		ApplyStyle( style);
		delete style;
	}

	_ApplyParagraphProps( inPara, inResetStyles ? false : true);

	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
}

/** init layout from the passed paragraph 
@param inPara
	paragraph source
@param inResetStyles
	if true, styles & props are reset first to default before initializing from paragraph
	if false (default), only overriden paragraph props & styles override current props & styles
@remarks
	the passed paragraph is used only for initializing layout plain text, styles & paragraph properties but inPara is not retained 
	note here that current styles are reset (see ResetParagraphStyle) & replaced by new paragraph styles
	also current plain text is replaced by the passed paragraph plain text 
*/
void VTextLayout::InitFromParagraph( const VDocParagraph *inPara)
{
	fLayoutIsValid = false;
	fLayerIsDirty = true;

	_ResetStyles( false, true); //reset char styles

	_ReplaceText( inPara);

	VTreeTextStyle *styles = inPara->RetainStyles();
	ApplyStyles( styles);
	ReleaseRefCountable(&styles);

	_ApplyParagraphProps( inPara, false);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
}

/** reset all text properties & character styles to default values */
void VTextLayout::ResetParagraphStyle(bool inResetParagraphPropsOnly)
{
	fLayoutIsValid = false;
	fLayerIsDirty = true;

	_ResetStyles(true, inResetParagraphPropsOnly ? false : true);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
}

void VTextLayout::_ReplaceText( const VDocParagraph *inPara)
{
	if (fTextPtrExternal)
	{
		fTextPtr = fTextPtrExternal;
		*fTextPtrExternal = inPara->GetText();
	}
	else
	{
		fTextPtr = &fText;
		fText = inPara->GetText();
	}
	fTextLength = fTextPtr->GetLength();
	if (fTextPtr->IsEmpty())
	{
		fText.FromCString("x");
		fTextPtr = &fText;
	}
}

void VTextLayout::_ApplyParagraphProps( const VDocParagraph *inPara, bool inIgnoreIfInherited)
{
	//remark: as measure are in TWIPS, we convert from TWIPS to point & then from point to VTextLayout DPI 

	if (fParagraphLayout)
	{
		fParagraphLayout->InitPropsFrom( static_cast<const VDocNode *>(inPara), inIgnoreIfInherited);
		return;
	}

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_MARGIN) && !inPara->IsInherited( kDOC_PROP_MARGIN)))
		SetMargins( inPara->GetMargin());

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_TEXT_ALIGN) && !inPara->IsInherited( kDOC_PROP_TEXT_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inPara->GetTextAlign();
		SetTextAlign( JustToAlignStyle( just));
	}

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_VERTICAL_ALIGN) && !inPara->IsInherited( kDOC_PROP_VERTICAL_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inPara->GetVerticalAlign();
		SetParaAlign( JustToAlignStyle( just));
	}

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_BACKGROUND_COLOR) && !inPara->IsInherited( kDOC_PROP_BACKGROUND_COLOR)))
	{
		RGBAColor color = inPara->GetBackgroundColor();
		VColor xcolor;
		xcolor.FromRGBAColor( color);
		SetBackColor( xcolor, fShouldPaintBackColor);
	}
}

void VTextLayout::_ResetStyles( bool inResetParagraphProps, bool inResetCharStyles)
{
	//reset to default character style 
	if (inResetCharStyles)
	{
		ReleaseRefCountable(&fStyles);
		ReleaseRefCountable(&fExtStyles);
		ReleaseRefCountable(&fDefaultFont);
		fDefaultFont = VFont::RetainStdFont(STDF_TEXT);
		fHasDefaultTextColor = true;
		fDefaultTextColor = VColor(0,0,0);
		ReleaseRefCountable(&fDefaultStyle); //ensure it is rebuild next time it is required
	}

	//reset to default paragraph props
	if (inResetParagraphProps)
	{
		VDocParagraph *paraDefault = new VDocParagraph();
		_ApplyParagraphProps( paraDefault, false);
		paraDefault->Release();
	}
}


VTextLayout::VTextLayout(bool inShouldEnableCache, bool inShouldUseNativeLayout)
{
	_Init();
	fShouldEnableCache = inShouldEnableCache;
	fShouldUseNativeLayout = inShouldUseNativeLayout;
	if (!fShouldUseNativeLayout)
		fParagraphLayout = new VDocParagraphLayout(this);

	ResetParagraphStyle();
}

void VTextLayout::_Init()
{
	fGC = NULL;
	fLayoutIsValid = false;
	fGCUseCount = 0;
	fCurGCType = eDefaultGraphicContext; //means same as undefined here
	fShouldEnableCache = false;
	fShouldUseNativeLayout = true;
	fShouldDrawOnLayer = false;
	fCurLayerOffsetViewRect = VPoint(-100000.0f, 0); 
	fLayerOffScreen = NULL;
	fLayerIsDirty = true;
	fTextLength = 0;
	//if text is empty, we need to set layout text to a dummy "x" text otherwise 
	//we cannot get correct metrics for caret or for text layout bounds  
	//(we use fTextLength to know the actual text length & stay consistent)
	fText.FromCString("x");
	fTextPtrExternal = NULL;
	fTextPtr = &fText;
	fDefaultFont = VFont::RetainStdFont(STDF_TEXT);
	fHasDefaultTextColor = true;
	fDefaultTextColor = VColor(0,0,0);
	fBackColor = VColor(0xff,0xff,0xff);
	fShouldPaintBackColor = false;
	fShouldPaintSelection = false;
	fSelActiveColor = VTEXTLAYOUT_SELECTION_ACTIVE_COLOR;
	fSelInactiveColor = VTEXTLAYOUT_SELECTION_INACTIVE_COLOR;
	fSelIsActive = true;
	fDefaultStyle = NULL;
	fCurFont = NULL;
	fCurKerning = 0.0f;
	fCurTextRenderingMode = TRM_NORMAL;
	fCurWidth = 0.0f;
	fCurHeight = 0.0f;
	fCurLayoutWidth = 0.0f;
	fCurLayoutHeight = 0.0f;
	fCurOverhang.left = 0.0f;
	fCurOverhang.right = 0.0f;
	fCurOverhang.top = 0.0f;
	fCurOverhang.bottom = 0.0f;
	fStyles = fExtStyles = NULL;
	fMaxWidth = fMaxWidthInit = 0.0f;
	fMaxHeight = fMaxHeightInit = 0.0f;
	fNeedUpdateBounds = false;

	fMarginTWIPS.left = fMarginTWIPS.right = fMarginTWIPS.top = fMarginTWIPS.bottom = 0;
	fMargin.left = fMargin.right = fMargin.top = fMargin.bottom = 0.0f;
	fHAlign = AL_DEFAULT;
	fVAlign = AL_DEFAULT;
	fLineHeight = -1;
	fTextIndent = 0;
	fTabStopOffset = ICSSUtil::CSSDimensionToPoint(1.25, kCSSUNIT_CM);
	fTabStopType = TTST_LEFT;
	fLayoutMode = TLM_NORMAL;

	fShowRefs = false;
	fHighlightRefs = false;

	fDPI = 72.0f;
	fUseFontTrueTypeOnly = false;
	fStylesUseFontTrueTypeOnly = true;
	fParagraphLayout = NULL;
	fTextBox = NULL;
#if ENABLE_D2D
	fLayoutD2D = NULL;
#endif
	fBackupFont = NULL;
	fIsPrinting = false;
	fSkipDrawLayer = false;
	fShouldPaintCharBackColor = false;
	fShouldPaintCharBackColorExtStyles = false;
	fCharBackgroundColorDirty = true;
}

VTextLayout::~VTextLayout()
{
	xbox_assert(fGCUseCount == 0);
	fLayoutIsValid = false;
	ReleaseRefCountable(&fDefaultStyle);
	ReleaseRefCountable(&fBackupFont);
	ReleaseRefCountable(&fLayerOffScreen);
	ReleaseRefCountable(&fDefaultFont);
	ReleaseRefCountable(&fCurFont);
	ReleaseRefCountable(&fExtStyles);
	ReleaseRefCountable(&fStyles);
	ReleaseRefCountable(&fTextBox);
	if (fParagraphLayout)
		delete fParagraphLayout;
#if ENABLE_D2D
	if (fLayoutD2D)
	{
		fLayoutD2D->Release();
		fLayoutD2D = NULL;
	}
#endif
}

void VTextLayout::ShouldEnableCache( bool inShouldEnableCache)
{
	VTaskLock protect(&fMutex);
	if (fShouldEnableCache == inShouldEnableCache)
		return;
	fShouldEnableCache = inShouldEnableCache;
	ReleaseRefCountable(&fLayerOffScreen);
	fLayerIsDirty = true;
}


/** allow/disallow offscreen text rendering (default is false)
@remarks
	text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
	you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
@see
	ShouldEnableCache
*/
void VTextLayout::ShouldDrawOnLayer( bool inShouldDrawOnLayer)
{
	VTaskLock protect(&fMutex);
	if (fShouldDrawOnLayer == inShouldDrawOnLayer)
		return;
	fShouldDrawOnLayer = inShouldDrawOnLayer;
	fLayerIsDirty = true;
	if (fGC && !fShouldDrawOnLayer && !fIsPrinting)
		_ResetLayer();
}

/** paint native selection or custom selection (using a color with opacity - as in Word) */
void VTextLayout::ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor, const VColor* inInactiveColor)
{
	VTaskLock protect(&fMutex);
	if (fShouldPaintSelection == inPaintSelection 
		&&
		(!inPaintSelection
		||
		((!inActiveColor || fSelActiveColor == *inActiveColor)
		 &&
		 (!inInactiveColor || fSelInactiveColor == *inInactiveColor))))
		return;

	fShouldPaintSelection = inPaintSelection;
	fSelectionRenderInfo.clear();
	fSelectionIsDirty = fSelStart < fSelEnd;
	if (inActiveColor)
		fSelActiveColor = *inActiveColor;
	if (inInactiveColor)
		fSelInactiveColor = *inInactiveColor;
	fLayerIsDirty = true;
}

void VTextLayout::ChangeSelection( sLONG inStart, sLONG inEnd, bool inActive)
{
	VTaskLock protect(&fMutex);

	if (fSelIsActive == inActive 
		&&
		(fSelStart == inStart && (fSelEnd == inEnd || (inEnd == -1 && fSelEnd == fTextLength))))
		return;

	fSelIsActive = inActive;
	fSelStart = inStart;
	fSelEnd = inEnd;
	if (inEnd == -1)
		fSelEnd = fTextLength;
	if (!fShouldPaintSelection)
		return;
	fSelectionRenderInfo.clear();
	fSelectionIsDirty = fSelStart < fSelEnd;
	if (!fLayerIsDirty)
		//layer needs to be redrawed only if color is opaque, otherwise color is painted after the layer
		fLayerIsDirty = fSelActiveColor.GetAlpha() == 255 || fSelInactiveColor.GetAlpha() == 255;
}

/** set default text color 
@remarks
	by default, base text color is current gc text color BUT: 
	it is highly recommended to call this method to avoid to inherit text color from gc
	& bind a default text color to the text layout
	otherwise if gc text color has changed, text layout needs to be computed again & if there is no default text color defined
*/
void VTextLayout::SetDefaultTextColor( const VColor &inTextColor, bool inEnable)
{
	VTaskLock protect(&fMutex);
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
	ReleaseRefCountable(&fDefaultStyle);
}

/** get default text color */
bool VTextLayout::GetDefaultTextColor(VColor& outColor) const 
{ 
	VTaskLock protect(&fMutex);
	if (!fHasDefaultTextColor)
		return false;
	outColor = fDefaultTextColor;
	return true; 
}

/** set back color 
@param  inColor
	layout back color
@param	inPaintBackColor
	if false (default), layout frame is not filled with back color (for instance if parent frame paints back color yet)
		but it is still used to ignore character background color if character back color is equal to the frame back color
		(unless back color is transparent color)
@remarks
	default is white and back color is not painted
*/
void VTextLayout::SetBackColor( const VColor& inColor, bool inPaintBackColor) 
{
	VTaskLock protect(&fMutex);
	if (fBackColor != inColor || fShouldPaintBackColor != inPaintBackColor)
	{
		fShouldPaintBackColor = inPaintBackColor;
		fBackColor = inColor;
		fLayoutIsValid = false;
		ReleaseRefCountable(&fDefaultStyle);
		fCharBackgroundColorDirty = true;
	}
}

const VString& VTextLayout::GetText() const
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



/** set max text width (0 for infinite) - in pixel (pixel relative to the layout DPI)
@remarks
	it is also equal to text box width 
	so you should specify one if text is not aligned to the left border
	otherwise text layout box width will be always equal to formatted text width
*/ 
void VTextLayout::SetMaxWidth(GReal inMaxWidth) 
{
	VTaskLock protect(&fMutex);

	if (fParagraphLayout)
	{
		//TODO: to be fixed later when VDocLayout is implemented
		if (fParagraphLayout->GetWidth())
			inMaxWidth = VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint(fParagraphLayout->GetWidth(true)) * fDPI / 72);	
		if (fParagraphLayout->GetMinWidth())
		{
			GReal minWidth = VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint(fParagraphLayout->GetMinWidth(true)) * fDPI / 72);
			if (inMaxWidth < minWidth)
				inMaxWidth = minWidth;
		}
	}

	inMaxWidth = std::ceil(inMaxWidth);

	if (inMaxWidth != 0.0f && inMaxWidth < fMargin.left+fMargin.right)
		inMaxWidth = fMargin.left+fMargin.right;

	if (fMaxWidthInit == inMaxWidth)
		return;

	if (fParagraphLayout)
		fNeedUpdateBounds = true;
	else
		fLayoutIsValid = false;
	fMaxWidth = fMaxWidthInit = inMaxWidth;

	if (fMaxWidth != 0.0f)
	{
		fMaxWidth = fMaxWidthInit-(fMargin.left+fMargin.right);
		if (fMaxWidth <= 0)
			fMaxWidth = 0.00001f;
	}
}

/** set max text height (0 for infinite) - in pixel
@remarks
	it is also equal to text box height 
	so you should specify one if paragraph is not aligned to the top border
	otherwise text layout box height will be always equal to formatted text height
*/ 
void VTextLayout::SetMaxHeight(GReal inMaxHeight) 
{
	VTaskLock protect(&fMutex);

	if (fParagraphLayout)
	{
		//TODO: to be fixed later when VDocLayout is implemented
		if (fParagraphLayout->GetHeight())
			inMaxHeight = VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint(fParagraphLayout->GetHeight(true)) * fDPI / 72);	
		if (fParagraphLayout->GetMinHeight())
		{
			GReal minHeight = VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint(fParagraphLayout->GetMinHeight(true)) * fDPI / 72);
			if (inMaxHeight < minHeight)
				inMaxHeight = minHeight;
		}
	}

	inMaxHeight = std::ceil(inMaxHeight);

	if (inMaxHeight != 0.0f && inMaxHeight < fMargin.top+fMargin.bottom)
		inMaxHeight = fMargin.top+fMargin.bottom;

	if (fMaxHeightInit == inMaxHeight)
		return;

	if (fParagraphLayout)
		fNeedUpdateBounds = true;
	else
		fLayoutIsValid = false;
	fMaxHeight = fMaxHeightInit = inMaxHeight;

	if (fMaxHeight != 0.0f)
	{
		fMaxHeight = fMaxHeightInit-(fMargin.top+fMargin.bottom);
		if (fMaxHeight <= 0)
			fMaxHeight = 0.00001f;
	}
}

/** set text horizontal alignment (default is AL_DEFAULT) */
void VTextLayout::SetTextAlign( AlignStyle inHAlign)
{
	VTaskLock protect(&fMutex);
	if (fHAlign == inHAlign)
		return;
	fHAlign = inHAlign;
	if (fParagraphLayout)
	{
		fNeedUpdateBounds = true;
		if (inHAlign == AL_JUST)
			fLayoutIsValid = false;
		fParagraphLayout->SetTextAlign( JustFromAlignStyle(inHAlign));
	}
	else
		fLayoutIsValid = false;
}

/** set text paragraph alignment (default is AL_DEFAULT) */
void VTextLayout::SetParaAlign( AlignStyle inVAlign)
{
	VTaskLock protect(&fMutex);
	if (fVAlign == inVAlign)
		return;
	fVAlign = inVAlign;
	if (fParagraphLayout)
	{
		fNeedUpdateBounds = true;
		fParagraphLayout->SetVerticalAlign( JustFromAlignStyle(inVAlign));
	}
	else
		fLayoutIsValid = false;
}

/** set paragraph line height 

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height 
*/
void VTextLayout::SetLineHeight( const GReal inLineHeight) 
{
	VTaskLock protect(&fMutex);
	if (fLineHeight == inLineHeight)
		return;
	fLayoutIsValid = false;
	fLineHeight = inLineHeight;
}


/** set first line padding offset in point 
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
void VTextLayout::SetTextIndent(const GReal inPadding)
{
	VTaskLock protect(&fMutex);
	if (fTextIndent == inPadding)
		return;
	fTextIndent = inPadding;
	fLayoutIsValid = false;
}

/** set tab stop (offset in point) */
void VTextLayout::SetTabStop(const GReal inOffset, const eTextTabStopType& inType)
{
	VTaskLock protect(&fMutex);
	if (!testAssert(inOffset > 0.0f))
		return;
	if (fTabStopOffset == inOffset && fTabStopType == inType)
		return;
	fTabStopOffset = inOffset;
	fTabStopType = inType;
	fLayoutIsValid = false;
}


/** set text layout mode (default is TLM_NORMAL) */
void VTextLayout::SetLayoutMode( TextLayoutMode inLayoutMode)
{
	VTaskLock protect(&fMutex);
	if (fLayoutMode == inLayoutMode)
		return;
	fLayoutIsValid = false;
	fLayoutMode = inLayoutMode;
}



/** draw text layout in the specified graphic context 
@remarks
	text layout left top origin is set to inTopLeft
	text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
	text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
*/
void VTextLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft)
{
	if (!testAssert(fGC == NULL || fGC == inGC))
		return;
	xbox_assert(inGC);
	if (fParagraphLayout)
		//platform-independent impl
		fParagraphLayout->Draw( inGC, inTopLeft);
	else
		//native impl
		inGC->_DrawTextLayout( this, inTopLeft);
}

/** return formatted text bounds (typo bounds) including margins
@remarks
	text typo bounds origin is set to inTopLeft

	as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
	because layout bounds are equal to the text box design bounds and not to the formatted text bounds
	you should use this method to get formatted text width & height 
*/
void VTextLayout::GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	//call GetLayoutBounds in order to update both layout & typo metrics
	VTaskLock protect(&fMutex);
	GetLayoutBounds( inGC, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
	outBounds.SetWidth( fCurWidth);
	outBounds.SetHeight( fCurHeight);
}

/** return text layout bounds including margins
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
void VTextLayout::GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	VTaskLock protect(&fMutex);
	if (inGC)
	{
		if (!testAssert(fGC == NULL || fGC == inGC))
			return;
		if (fParagraphLayout)
			//platform-independent impl
			fParagraphLayout->GetLayoutBounds( inGC, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
		else
			//native impl
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
void VTextLayout::GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	if (!testAssert(fGC == NULL || fGC == inGC))
		return;
	xbox_assert(inGC);
	if (fParagraphLayout)
		//platform-independent impl
		fParagraphLayout->GetRunBoundsFromRange( inGC, outRunBounds, inTopLeft, inStart, inEnd);
	else
		//native impl
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
void VTextLayout::GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	if (!testAssert(fGC == NULL || fGC == inGC))
		return;
	xbox_assert(inGC);
	if (fParagraphLayout)
		//platform-independent impl
		fParagraphLayout->GetCaretMetricsFromCharIndex( inGC, inTopLeft, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics);
	else
		//native impl
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
bool VTextLayout::GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength)
{
	if (!testAssert(fGC == NULL || fGC == inGC))
		return false;
	xbox_assert(inGC);
	bool inside;
	if (fParagraphLayout)
		//platform-independent impl
		inside	= fParagraphLayout->GetCharIndexFromPos( inGC, inTopLeft, inPos, outCharIndex, outRunLength);
	else
	{
		//native impl
		inside	= inGC->_GetTextLayoutCharIndexFromPos( this, inTopLeft, inPos, outCharIndex);
		if (outRunLength)
			*outRunLength = 0;
	}
	return inside; 
}


void VTextLayout::ReleaseFullTreeTextStyle(const VTreeTextStyle *inStyles) const
{
	xbox_assert(inStyles == fDefaultStyle);

	if (fDefaultStyle->GetChildCount() > 0)
	{
		xbox_assert(fDefaultStyle->GetChildCount() == 1 && fStyles);
		fDefaultStyle->RemoveChildAt(1);
	}
	fDefaultStyle->Release();

	fMutex.Unlock();
}

AlignStyle VTextLayout::JustToAlignStyle( const eStyleJust inJust) 
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
	default:
		return AL_DEFAULT;
		break;
	}
}

eStyleJust VTextLayout::JustFromAlignStyle( const AlignStyle inJust) 
{
	switch (inJust)
	{
	case AL_LEFT:
		return JST_Left;
		break;
	case AL_RIGHT:
		return JST_Right;
		break;
	case AL_CENTER:
		return JST_Center;
		break;
	case AL_JUST:
		return JST_Justify;
		break;
	default:
		return JST_Default;
		break;
	}
}


/** begin using text layout for the specified gc */
void VTextLayout::BeginUsingContext( VGraphicContext *inGC, bool inNoDraw)
{
	xbox_assert(inGC);
	
	if (fGCUseCount > 0)
	{
		xbox_assert(fGC == inGC);
		fGC->BeginUsingContext( inNoDraw);
		//if caller has updated some layout settings, we need to update again the layout
		if (!fLayoutIsValid || fNeedUpdateBounds)
			_UpdateTextLayout();
		fGCUseCount++;
		return;
	}
	
	fMutex.Lock();
	
	fGCUseCount++;
	inGC->BeginUsingContext( inNoDraw);
	inGC->UseReversedAxis();

	//reset layout & layer if we are printing
	GReal scaleX, scaleY;
	fIsPrinting = inGC->GetPrinterScale( scaleX, scaleY);
	if (fIsPrinting)
	{
		fLayoutIsValid = false;
		ReleaseRefCountable(&fLayerOffScreen);
		fLayerIsDirty = true;
	}

	_SetGC( inGC);
	xbox_assert(fGC == inGC);

	//set current font & text color
	xbox_assert(fBackupFont == NULL);
	fBackupFont = inGC->RetainFont();
	inGC->GetTextColor( fBackupTextColor);
	
	if (fLayoutIsValid)
	{
		if (fBackupFont != fCurFont)
			inGC->SetFont( fCurFont);
	
		if (fBackupTextColor != fCurTextColor)
			inGC->SetTextColor( fCurTextColor);
	}
	else
	{
		xbox_assert(fDefaultFont);
		fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fDefaultFont->IsTrueTypeFont();
	}

#if VERSIONWIN
	fBackupTRM = fGC->GetDesiredTextRenderingMode();
	fNeedRestoreTRM = false;
	if (!fGC->IsGDIImpl() && (!(fGC->GetTextRenderingMode() & TRM_LEGACY_ON)) && (!(fGC->GetTextRenderingMode() & TRM_LEGACY_OFF)) && (!fUseFontTrueTypeOnly))
	{
		//force legacy if not set yet if text uses at least one font not truetype (as neither D2D or GDIPlus are capable to display fonts but truetype)
		fGC->SetTextRenderingMode( (fBackupTRM & (~TRM_LEGACY_OFF)) | TRM_LEGACY_ON);
		fNeedRestoreTRM = true;
	}
#endif

	//update text layout (only if it is no longer valid)
	_UpdateTextLayout();

}

/** end using text layout */
void VTextLayout::EndUsingContext()
{
	fGCUseCount--;
	xbox_assert(fGCUseCount >= 0);
	if (fGCUseCount > 0)
	{
		fGC->EndUsingContext();
		return;
	}

#if VERSIONWIN
	if (fNeedRestoreTRM)
		fGC->SetTextRenderingMode( fBackupTRM);
#endif

	//restore font & text color
	fGC->SetTextColor( fBackupTextColor);

	VFont *font = fGC->RetainFont();
	if (font != fBackupFont)
		fGC->SetFont( fBackupFont);
	ReleaseRefCountable(&font);
	ReleaseRefCountable(&fBackupFont);

	fGC->EndUsingContext();
	_SetGC( NULL);
	
	fMutex.Unlock();
}

void VTextLayout::_BeginUsingStyles()
{
	fMutex.Lock();
	if (fExtStyles)
	{
		//attach extra styles to current styles
		if (fStyles == NULL)
			fStyles = fExtStyles;
		else
			fStyles->AddChild( fExtStyles);
	}
}

void VTextLayout::_EndUsingStyles()
{
	if (fExtStyles)
	{
		//detach extra styles from current styles
		if (fStyles == fExtStyles)
			fStyles = NULL;
		else
			fStyles->RemoveChildAt(fStyles->GetChildCount());
	}
	fMutex.Unlock();
}


void VTextLayout::_ResetLayer()
{
	fLayerIsDirty = true;
	ReleaseRefCountable(&fLayerOffScreen);
}


/** get character range which intersects the passed bounds (in gc user space) 
@remarks
	text layout origin is set to inTopLeft (in gc user space)
*/
void VTextLayout::GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd)
{
	BeginUsingContext( inGC, true);

	sLONG startRunLength;
	sLONG endRunLength;
	if (IsRTL())
	{
		GetCharIndexFromPos( inGC, inTopLeft, inBounds.GetTopRight(), outRangeStart, &startRunLength);
		GetCharIndexFromPos( inGC, inTopLeft, inBounds.GetBotLeft(), outRangeEnd, &endRunLength);
	}
	else
	{
		GetCharIndexFromPos( inGC, inTopLeft, inBounds.GetTopLeft(), outRangeStart, &startRunLength);
		GetCharIndexFromPos( inGC, inTopLeft, inBounds.GetBotRight(), outRangeEnd, &endRunLength);
	}
	if (outRangeStart == outRangeEnd)
	{
		if (!startRunLength && !endRunLength)
		{
			EndUsingContext();
			return;
		}
	}

	//adjust indexs: round to run limit

	if (outRangeStart > outRangeEnd)
	{
		sLONG startBackup = outRangeStart;
		outRangeStart = outRangeEnd;
		outRangeEnd = startBackup;

		startBackup = startRunLength;
		startRunLength = endRunLength;
		endRunLength = startBackup;
	}
	if (startRunLength)
	{
		outRangeStart -= startRunLength;
		if (outRangeStart < 0)
			outRangeStart = 0;
	}
	if (endRunLength)
	{
		outRangeEnd += endRunLength;
		if (outRangeEnd > fTextLength)
			outRangeEnd = fTextLength;
	}

	EndUsingContext();
}


void VTextLayout::_UpdateSelectionRenderInfo( const VPoint& inTopLeft, const VRect& inClipBounds)
{
	xbox_assert(fGC);

	if (!fShouldPaintSelection)
		return;
	if (!fSelectionIsDirty)
		return;

	fSelectionIsDirty = false;
	fSelectionRenderInfo.clear();

	if (fSelStart >= fSelEnd)
		return;

	sLONG start, end;
	GetCharRangeFromRect( fGC, inTopLeft, inClipBounds, start, end);

	start = std::max( fSelStart, start);
	end = std::min( fSelEnd, end);

	if (start >= end)
		return;

	GetRunBoundsFromRange( fGC, fSelectionRenderInfo, VPoint(), start, end);
}


void VTextLayout::_DrawSelection(const VPoint& inTopLeft)
{
	if (!fShouldPaintSelection)
		return;

	{
		StEnableShapeCrispEdges crispEdges( fGC);

		fGC->SetFillColor( fSelIsActive ? fSelActiveColor : fSelInactiveColor);
		
		VectorOfRect::const_iterator it = fSelectionRenderInfo.begin();
		for (; it != fSelectionRenderInfo.end(); it++)
		{
			VRect bounds = *it;
			bounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
			fGC->FillRect( bounds);
		}

	}
}


void VTextLayout::_UpdateBackgroundColorRenderInfo(const VPoint& inTopLeft, const VRect& inClipBounds)
{
	xbox_assert(fGC);

	if (!fShouldPaintCharBackColor && !fShouldPaintCharBackColorExtStyles)
		return;
	if (!fCharBackgroundColorDirty)
		return;

	fCharBackgroundColorDirty = false;
	fCharBackgroundColorRenderInfo.clear();

	sLONG start, end;
	GetCharRangeFromRect( fGC, inTopLeft, inClipBounds, start, end);

	if (start >= end)
		return;

	if (fShouldPaintCharBackColor && fStyles)
		_UpdateBackgroundColorRenderInfoRec( inTopLeft, fStyles, start, end);
	if (fShouldPaintCharBackColorExtStyles && fExtStyles)
		_UpdateBackgroundColorRenderInfoRec( inTopLeft, fExtStyles, start, end);
}

void VTextLayout::_UpdateBackgroundColorRenderInfoRec( const VPoint& inTopLeft, VTreeTextStyle *inStyles, const sLONG inStart, const sLONG inEnd, bool inHasParentBackgroundColor)
{
	sLONG start, end;
	inStyles->GetData()->GetRange( start, end);

	if (start < inStart)
		start  = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start < end)
	{
		bool hasBackColor = inHasParentBackgroundColor;
		VTextStyle *style = inStyles->GetData();
		while (style)
		{
			if (style->GetHasBackColor())
			{
				if (hasBackColor || style->GetBackGroundColor() != RGBACOLOR_TRANSPARENT)
				{
					VectorOfRect runBounds;
					GetRunBoundsFromRange( fGC, runBounds, VPoint(), start, end); //we compute with layout origin = (0,0) because while we draw background color, we add inTopLeft
					//(and we do not want to compute again render info if layout has been translated only)
					
					XBOX::VColor backcolor;
					backcolor.FromRGBAColor(style->GetBackGroundColor() == RGBACOLOR_TRANSPARENT ? fBackColor.GetRGBAColor() : style->GetBackGroundColor());
					if (backcolor.GetAlpha())
						fCharBackgroundColorRenderInfo.push_back(BoundsAndColor( runBounds, backcolor));
					hasBackColor = style->GetBackGroundColor() != RGBACOLOR_TRANSPARENT;
				}
			}
			
			if (style->IsSpanRef())
			{
				const VTextStyle *styleRef = style->GetSpanRef()->GetStyle();
				if (styleRef && styleRef->GetHasBackColor())
				{
					xbox_assert(!styleRef->IsSpanRef()); 
					
					style = new VTextStyle( styleRef);
					style->SetRange( start, end);
				}
				else 
					style = NULL;
			}
			else
			{
				if (style != inStyles->GetData())
					delete style;
				style = NULL;
			}
		}
		
		sLONG childCount = inStyles->GetChildCount();
		for (int i = 1; i <= childCount; i++)
			_UpdateBackgroundColorRenderInfoRec( inTopLeft, inStyles->GetNthChild( i), start, end, hasBackColor);
	}
}


void VTextLayout::_DrawBackgroundColor(const VPoint& inTopLeft, const VRect& inBoundsLayout)
{
	if (!fParagraphLayout && fShouldPaintBackColor && fBackColor.GetAlpha())
	{
		fGC->SetFillColor( fBackColor);
		fGC->FillRect( inBoundsLayout);
	}

	if (!fShouldPaintCharBackColor && !fShouldPaintCharBackColorExtStyles)
		return;

	{
		StEnableShapeCrispEdges crispEdges( fGC);
			
		VectorOfBoundsAndColor::const_iterator it = fCharBackgroundColorRenderInfo.begin();
		for (; it != fCharBackgroundColorRenderInfo.end(); it++)
		{
			fGC->SetFillColor( it->second);

			VectorOfRect::const_iterator itRect = it->first.begin();
			for (; itRect != it->first.end(); itRect++)
			{
				VRect bounds = *itRect;
				bounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
				
				fGC->FillRect( bounds);
			}
		}
	}
}

bool VTextLayout::_CheckLayerBounds(const VPoint& inTopLeft, VRect& outLayerBounds)
{
	outLayerBounds = VRect( inTopLeft.GetX(), inTopLeft.GetY(), fCurLayoutWidth, fCurLayoutHeight); //on default, layer bounds = layout bounds

	if (fParagraphLayout)
	{
		//platform-independent layout: 
		
		//VDocParagraphLayout::Draw paints back color 
		fShouldPaintCharBackColor = false;
		fShouldPaintCharBackColorExtStyles = false; 
	}
	else
		//native layout: depending on gc, background color might be drawed natively or by VTextLayout
		fShouldPaintCharBackColor = fShouldPaintCharBackColorExtStyles = !fGC->CanDrawStyledTextBackColor();

	if ((!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl()) //not draw on layer
		&& 
		!fShouldPaintCharBackColor && !fShouldPaintCharBackColorExtStyles && !fShouldPaintSelection) //not draw background color
		return true;

	VAffineTransform ctm;	
	fGC->UseReversedAxis();
	fGC->GetTransformToScreen(ctm); //in case there is a pushed layer, we explicitly request transform from user space to hwnd space
									//(because fLayerViewRect is in hwnd user space)
	if (ctm.GetScaling() != fCurLayerCTM.GetScaling()
		||
		ctm.GetShearing() != fCurLayerCTM.GetShearing())
	{
		//if scaling or rotation has changed, mark layer and background color as dirty
		fLayerIsDirty = true;
		fCharBackgroundColorDirty = true;
		fSelectionIsDirty = true;
		fCurLayerCTM = ctm;
	}

	//determine layer bounds
	if (!fLayerViewRect.IsEmpty())
	{
		//constraint layer bounds with view rect bounds

		outLayerBounds.Intersect( fLayerViewRect);
		
		//clip layout bounds with view rect bounds in gc local user space
		/*
		if (ctm.IsIdentity())
			outLayerBounds.Intersect( fLayerViewRect);
		else
		{
#if VERSIONMAC
			//fLayerViewRect is window rectangle in QuickDraw coordinates so we need to convert it to Quartz2D coordinates
			//(because transformed space is equal to Quartz2D space)
			
			boundsViewRect( fLayerViewRect);
			QDToQuartz2D( boundsViewRect);
			VRect boundsViewLocal = ctm.Inverse().TransformRect( boundsViewRect);
#else
			VRect boundsViewLocal = ctm.Inverse().TransformRect( fLayerViewRect);
#endif
			boundsViewLocal.NormalizeToInt();
			outLayerBounds.Intersect( boundsViewLocal);
		}
		*/

		VPoint offset = outLayerBounds.GetTopLeft() - inTopLeft;
		if (offset != fCurLayerOffsetViewRect)
		{
			//layout has scrolled in view window: mark layer as dirty
			//FIXME: maybe we should redraw the layer here only if layout size is greater than some reasonable limit (and make the layer size = layout size if not)
			fCurLayerOffsetViewRect = offset;
			fLayerIsDirty = true;
			fCharBackgroundColorDirty = true;
			fSelectionIsDirty = true;
		}
	}

	if (outLayerBounds.IsEmpty())
	{
		//do not draw at all if layout is fully clipped by fLayerViewRect
		_ResetLayer();
	
		fCharBackgroundColorRenderInfo.clear();
		fCharBackgroundColorDirty = false;
		fSelectionRenderInfo.clear();
		fSelectionIsDirty = false;
		return false; 
	}

	_UpdateBackgroundColorRenderInfo(inTopLeft, outLayerBounds);
	_UpdateSelectionRenderInfo(inTopLeft, outLayerBounds);
	return true;
}


bool VTextLayout::_BeginDrawLayer(const VPoint& inTopLeft)
{
	xbox_assert(fGC && fSkipDrawLayer == false);

	VRect boundsLayout;
	bool doDraw = _CheckLayerBounds( inTopLeft, boundsLayout);
	fCurTopLeft = inTopLeft;
	fCurLayerBounds = boundsLayout;

	if (!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl())
	{
		if (doDraw)
			_DrawBackgroundColor( inTopLeft, boundsLayout);
		return doDraw;
	}

	if (!fLayerOffScreen)
		fLayerIsDirty = true;

	bool isLayerTransparent = !fShouldPaintBackColor || fBackColor.GetAlpha() != 255;

	if (isLayerTransparent && !fGC->ShouldDrawTextOnTransparentLayer())
	{
		fSkipDrawLayer = true;
		_ResetLayer();
		if (doDraw)
			_DrawBackgroundColor( inTopLeft, boundsLayout);
		return doDraw;
	}

	if (!doDraw)
	{
		fSkipDrawLayer = true;
		return false;
	}

	bool doRedraw = true; 
	if (!fLayerIsDirty)
		doRedraw = !fGC->DrawLayerOffScreen( boundsLayout, fLayerOffScreen);
	if (doRedraw)
	{
		fLayerIsDirty = true;
		bool doClear = !fGC->BeginLayerOffScreen( boundsLayout, fLayerOffScreen, false, isLayerTransparent);
		ReleaseRefCountable(&fLayerOffScreen);
		if (doClear)
		{
			//clear layer if offscreen layer is preserved
			//(otherwise current frame would be painted over last frame)
			if (isLayerTransparent)
			{
				if (fGC->IsD2DImpl() || fGC->IsGdiPlusImpl())
					fGC->Clear(VColor(0,0,0,0));
				else
					fGC->Clear(VColor(0,0,0,0), &boundsLayout);
			}
		}
		_DrawBackgroundColor( inTopLeft, boundsLayout);
	}
	else
		fSkipDrawLayer = true;
	return doRedraw;
}

void VTextLayout::_EndDrawLayer()
{
	if (!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl() || fSkipDrawLayer)
	{
		fSkipDrawLayer = false;

		//we draw selection above the layer 
		if (!fParagraphLayout)
			_DrawSelection( fCurTopLeft);
		return;
	}

	xbox_assert(fLayerOffScreen == NULL);
	fLayerOffScreen = fGC->EndLayerOffScreen();
	fLayerIsDirty = fLayerOffScreen == NULL;

	//we draw selection above the layer 
	if (!fParagraphLayout)
		_DrawSelection( fCurTopLeft);
}


/** set text layout graphic context 
@remarks
	it is the graphic context to which is bound the text layout
	if offscreen layer is disabled text layout needs to be redrawed again at every frame
	so it is recommended to enable the internal offscreen layer if you intend to preserve the same rendered layout on multiple frames
	because offscreen layer & text content are preserved as long as offscreen layer is compatible with the attached gc & text content is not dirty
*/
void VTextLayout::_SetGC( VGraphicContext *inGC)
{
	if (fGC == inGC)
		return;

	if (inGC)
	{
		fLayoutIsValid = fLayoutIsValid && fShouldEnableCache && !fIsPrinting;

		//if new gc is not compatible with last used gc, we need to compute again text layout & metrics and/or reset layer

		if (fLayoutIsValid && !inGC->IsCompatible( fCurGCType, true)) //check if current layout is compatible with new gc: here we ignore hardware or software status 
			fLayoutIsValid = false;//gc kind has changed: inval layout

		if (fLayerOffScreen && (inGC->IsGDIImpl() || !fShouldDrawOnLayer)) //check if layer is still enabled
			_ResetLayer(); 

		if (fLayerOffScreen && !inGC->IsCompatible( fCurGCType, false)) //check is layer is compatible with new gc: here we take account hardware or software status
			_ResetLayer(); //gc kind has changed: clear layer so it is created from new gc on next _BeginLayer

		fCurGCType = inGC->GetGraphicContextType(); //backup current gc type
		
		if (!fLayoutIsValid) 
			fLayerIsDirty = true;
	}

	//attach new gc or dispose actual gc (if inGC == NULL)
	CopyRefCountable(&fGC, inGC);
}


/** set default font 
@remarks
	by default, base font is current gc font BUT: 
	it is highly recommended to call this method to avoid to inherit default font from gc
	& bind a default font to the text layout
	otherwise if gc font has changed, text layout needs to be computed again if there is no default font defined

	by default, input font is assumed to be 4D form-compliant font (created with dpi = 72)
*/
void VTextLayout::SetDefaultFont( const VFont *inFont, GReal inDPI)
{
	if (fDefaultFont == inFont && inDPI == 72)
		return;

	fLayoutIsValid = false;
	if (inFont == NULL)
	{
		ReleaseRefCountable(&fDefaultFont);
		fDefaultFont = VFont::RetainStdFont(STDF_TEXT);
		ReleaseRefCountable(&fDefaultStyle);
		return;
	}

	if (inDPI == 72)
		CopyRefCountable(&fDefaultFont, (VFont *)inFont);
	else
	{
		//we always keep unscaled font (font with dpi = 72)
		VFont *font = VFont::RetainFont(inFont->GetName(), inFont->GetFace(), inFont->GetPixelSize(), 72.0f*72.0f/inDPI); 
		CopyRefCountable(&fDefaultFont, font);
		ReleaseRefCountable(&font);
	}
	ReleaseRefCountable(&fDefaultStyle);
}


/** get & retain default font 
@remarks
	by default, return 4D form-compliant font (with dpi = 72) so not the actual fDPI-scaled font 
	(for consistency with SetDefaultFont & because fDPI internal scaling should be transparent for caller)
*/
VFont *VTextLayout::RetainDefaultFont(GReal inDPI) const
{
	if (!testAssert(fDefaultFont))
	{
		fDefaultFont = VFont::RetainStdFont(STDF_TEXT);
		ReleaseRefCountable(&fDefaultStyle);
	}

	if (inDPI == 72.0f)
		return RetainRefCountable(fDefaultFont);
	else
		return VFont::RetainFont(fDefaultFont->GetName(), fDefaultFont->GetFace(), fDefaultFont->GetPixelSize(), inDPI); 
}


/** set layout DPI (default is 4D form DPI = 72) */
void VTextLayout::SetDPI( GReal inDPI)
{
	inDPI = inDPI; 
	if (fDPI == inDPI)
		return;
	fDPI = inDPI;

	if (fParagraphLayout)
	{
		fParagraphLayout->SetDPI( fDPI);
		fNeedUpdateBounds = true;
	}
	else
	{
		fLayoutIsValid = false;
		//recalc margins in px
		SetMargins( fMarginTWIPS);
	}
}


/** get margins (in TWIPS) */
void VTextLayout::GetMargins( sDocPropRect& outMargins)
{
	outMargins = fMarginTWIPS;
}

GReal VTextLayout::RoundMetrics( VGraphicContext *inGC, GReal inValue)
{
	if (inGC && inGC->IsFontPixelSnappingEnabled())
	{
		//context aligns glyphs to pixel boundaries:
		//stick to nearest integral (as GDI does)
		//(note that as VTextLayout pixel is dependent on fixed fDPI, metrics need to be computed if fDPI is changed)
		if (inValue >= 0)
			return floor( inValue+0.5);
		else
			return ceil( inValue-0.5);
	}
	else
	{
		//context does not align glyphs on pixel boundaries (or it is not for text metrics - passing NULL as inGC) :
		//round to TWIPS precision (so result is multiple of 0.05) (it is as if metrics are computed at 72*20 = 1440 DPI which should achieve enough precision)
		if (inValue >= 0)
			return ICSSUtil::RoundTWIPS( inValue);
		else
			return -ICSSUtil::RoundTWIPS( -inValue);
	}
}

/** get margins (in TWIPS) */
void VTextLayout::SetMargins( const sDocPropRect& inMargins)
{
	fMarginTWIPS = inMargins;

	fMargin.left	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginTWIPS.left) * fDPI/72);
	fMargin.right	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginTWIPS.right) * fDPI/72);
	fMargin.top		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginTWIPS.top) * fDPI/72);
	fMargin.bottom	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginTWIPS.bottom) * fDPI/72);

	if (fParagraphLayout)
	{
		fParagraphLayout->SetMargins( inMargins);

		if( fMaxWidthInit != 0.0f)
		{
			GReal maxWidth = fMaxWidthInit;
			fMaxWidthInit = 0.0f;
			SetMaxWidth( maxWidth);
		}
		if( fMaxHeightInit != 0.0f)
		{
			GReal maxHeight = fMaxHeightInit;
			fMaxHeightInit = 0.0f;
			SetMaxHeight( maxHeight);
		}

		fNeedUpdateBounds = true;
		fLayerIsDirty = true;
		return;
	}

	if( fMaxWidthInit != 0.0f)
	{
		GReal maxWidth = fMaxWidthInit;
		fMaxWidthInit = 0.0f;
		SetMaxWidth( maxWidth);
	}
	if( fMaxHeightInit != 0.0f)
	{
		GReal maxHeight = fMaxHeightInit;
		fMaxHeightInit = 0.0f;
		SetMaxHeight( maxHeight);
	}

	fLayerIsDirty = true;
	fLayoutIsValid = false;
}


/** replace margins (in point) */
void VTextLayout::SetMargins(const GReal inMarginLeft, const GReal inMarginRight, const GReal inMarginTop, const GReal inMarginBottom)
{
	//to avoid math discrepancies from res-independent point to current DPI, we always store in TWIPS & convert from TWIPS

	sDocPropRect margin;
	margin.left			= ICSSUtil::PointToTWIPS( inMarginLeft);
	margin.right		= ICSSUtil::PointToTWIPS( inMarginRight);
	margin.top			= ICSSUtil::PointToTWIPS( inMarginTop);
	margin.bottom		= ICSSUtil::PointToTWIPS( inMarginBottom);
	SetMargins( margin);
}

/** get margins (in point) */
void VTextLayout::GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom)
{
	if (outMarginLeft)
		*outMarginLeft = ICSSUtil::TWIPSToPoint( fMarginTWIPS.left);
	if (outMarginRight)
		*outMarginRight = ICSSUtil::TWIPSToPoint( fMarginTWIPS.right);
	if (outMarginTop)
		*outMarginTop = ICSSUtil::TWIPSToPoint( fMarginTWIPS.top);
	if (outMarginBottom)
		*outMarginBottom = ICSSUtil::TWIPSToPoint( fMarginTWIPS.bottom);
}


void VTextLayout::_CheckStylesConsistency()
{
	//ensure initial text range == initial styles range (otherwise merging styles later would be constrained to initial range)
	if (fStyles)
	{
		sLONG start, end;
		fStyles->GetData()->GetRange( start, end);
		bool doUpdate = false;
		if (start < 0)
		{
			start = 0;
			doUpdate = true;
		}
		if (end > fTextLength)
		{
			end = fTextLength;
			doUpdate = true;
		}
		if (doUpdate)
			fStyles->GetData()->SetRange( start, end);
		if (start > 0 || end < fTextLength)
		{
			//expand styles to contain text range
			VTreeTextStyle *styles = new VTreeTextStyle( new VTextStyle());
			styles->GetData()->SetRange( 0, fTextLength);
			styles->AddChild( fStyles);
			fStyles->Release();
			fStyles = styles;
		}
	}

	if (fExtStyles)
	{
		//for fExtStyles, we do not need to expand styles to text range because we do not merge other styles with it:
		//we just check if range is included in text range
		sLONG start, end;
		fExtStyles->GetData()->GetRange( start, end);
		bool doUpdate = false;
		if (start < 0)
		{
			start = 0;
			doUpdate = true;
		}
		if (end > fTextLength)
		{
			end = fTextLength;
			doUpdate = true;
		}
		if (doUpdate)
			fExtStyles->GetData()->SetRange( start, end);
	}
}

/** replace current text & styles 
@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false, styles are retained: in that case, if you modify passed styles you should call this method again 
	
*/
void VTextLayout::SetText( const VString& inText, VTreeTextStyle *inStyles, bool inCopyStyles)
{
	VTaskLock protect(&fMutex);

	fLayoutIsValid = false;
	fTextLength = inText.GetLength();
	fTextPtrExternal = NULL;
	if (fTextLength == 0)
	{
		//if text is empty, we need to set layout text to a dummy "x" text otherwise 
		//we cannot get correct metrics for caret or for text layout bounds  
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromCString("x");
		fTextPtr = &fText;
	}
    else
    {
		if (&inText != &fText)
			fText.FromString(inText);
		fTextPtr = &fText;
	}

	_PostReplaceText();

	if (inStyles != fStyles || inCopyStyles)
	{
		if (inCopyStyles && inStyles)
		{
			VTreeTextStyle *styles = fStyles;
			fStyles = new VTreeTextStyle( inStyles);
			ReleaseRefCountable(&styles);
		}
		else
			CopyRefCountable(&fStyles, inStyles);

		_CheckStylesConsistency();

		fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	}
}


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
void VTextLayout::SetText( VString* inText, VTreeTextStyle *inStyles, bool inCopyStyles, bool inCopyText)
{
	VTaskLock protect(&fMutex);

	fLayoutIsValid = false;
	fTextLength = inText->GetLength();
	fTextPtrExternal = inCopyText ? NULL : inText;
	if (fTextLength == 0)
	{
		//if text is empty, we need to set layout text to a dummy "x" text otherwise 
		//we cannot get correct metrics for caret or for text layout bounds  
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromCString("x");
		fTextPtr = &fText;
	}
	else
	{
		if (inCopyText)
		{
			fText.FromString(*inText);
			fTextPtr = &fText;
		}
		else
			fTextPtr = inText;
	}
	_PostReplaceText();

	if (inStyles != fStyles || inCopyStyles)
	{
		if (inCopyStyles && inStyles)
		{
			VTreeTextStyle *styles = fStyles;
			fStyles = new VTreeTextStyle( inStyles);
			ReleaseRefCountable(&styles);
		}
		else
			CopyRefCountable(&fStyles, inStyles);

		_CheckStylesConsistency();

		fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	}
}


/** replace text with span text reference on the specified range
@remarks
	it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on show ref or not flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VTextLayout::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inNoUpdateRef,
									    void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	VTaskLock protect(&fMutex);
	inSpanRef->ShowHighlight(fHighlightRefs);
	inSpanRef->ShowRef(fShowRefs);
	if (VTreeTextStyle::ReplaceAndOwnSpanRef( fTextPtrExternal ? *fTextPtrExternal : fText, fStyles, inSpanRef, inStart, inEnd, 
											  inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inNoUpdateRef,
											  inUserData, inBeforeReplace, inAfterReplace))
	{
		fLayoutIsValid = false;
		if (fTextPtrExternal)
			SetText(fTextPtrExternal, fStyles, false, false);
		else
		{
			if (fTextLength < fText.GetLength())
				fText.Truncate(fTextLength);
			SetText(fText, fStyles, false);
		}
		return true;
	}
	return false;
}

/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
bool VTextLayout::UpdateSpanRefs(sLONG inStart, sLONG inEnd, void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	VTaskLock protect(&fMutex);
	if (fStyles)
	{
		if (VTreeTextStyle::UpdateSpanRefs( fTextPtrExternal ? *fTextPtrExternal : fText, *fStyles, inStart, inEnd, inUserData, inBeforeReplace, inAfterReplace))
		{
			fLayoutIsValid = false;
			if (fTextPtrExternal)
				SetText(fTextPtrExternal, fStyles, false, false);
			else
			{
				if (fTextLength < fText.GetLength())
					fText.Truncate(fTextLength);
				SetText(fText, fStyles, false);
			}
			return true;
		}
	}
	return false;
}

/** eval 4D expressions  
@remarks
	note that it only updates span 4D expression references but does not update plain text:
	call UpdateSpanRefs after to update layout plain text

	return true if any 4D expression has been evaluated
*/
bool VTextLayout::EvalExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, VDocCache4DExp *inCache4DExp)
{
	VTaskLock protect(&fMutex);
	if (fStyles)
		return fStyles->EvalExpressions( inLC, inStart, inEnd, inCache4DExp);
	return false;
}


/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
bool VTextLayout::FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	VTaskLock protect(&fMutex);
	if (fStyles)
	{
		if (VTreeTextStyle::FreezeExpressions( inLC, fTextPtrExternal ? *fTextPtrExternal : fText, *fStyles, inStart, inEnd, inUserData, inBeforeReplace, inAfterReplace))
		{
			fLayoutIsValid = false;
			if (fTextPtrExternal)
				SetText(fTextPtrExternal, fStyles, false, false);
			else
			{
				if (fTextLength < fText.GetLength())
					fText.Truncate(fTextLength);
				SetText(fText, fStyles, false);
			}
			return true;
		}
	}
	return false;
}


/** set text styles 
@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
void VTextLayout::SetStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	VTaskLock protect(&fMutex);
	if (!inStyles && !fStyles)
		return;

	if (inStyles != fStyles || inCopyStyles)
	{
		fLayoutIsValid = false;

		if (inCopyStyles && inStyles)
		{
			VTreeTextStyle *styles = fStyles;
			fStyles = new VTreeTextStyle( inStyles);
			ReleaseRefCountable(&styles);
		}
		else
			CopyRefCountable(&fStyles, inStyles);

		_CheckStylesConsistency();

		fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	}
}

/** set extra text styles 
@remarks
	it can be used to add a temporary style effect to the text layout without modifying the main text styles
	(for instance to add a text selection effect: see VStyledTextEditView)

	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
void VTextLayout::SetExtStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	VTaskLock protect(&fMutex);
	if (!inStyles && !fExtStyles)
		return;

	if (!testAssert(inStyles == NULL || inStyles != fStyles))
		return;

	fLayoutIsValid = false;

	if (inCopyStyles && inStyles)
	{
		VTreeTextStyle *styles = fExtStyles;
		fExtStyles = new VTreeTextStyle( inStyles);
		ReleaseRefCountable(&styles);
	}
	else
		CopyRefCountable(&fExtStyles, inStyles);

	_CheckStylesConsistency();

	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
}



/** internally called while replacing text in order to update impl-specific stuff */
void VTextLayout::_PostReplaceText()
{
	if (fGC) //deal with reentrance 
		fGC->_PostReplaceText( this);
	else
	{
		VGraphicContext *gc = VGraphicContext::CreateForComputingMetrics(fCurGCType); 
		gc->_PostReplaceText( this);
		ReleaseRefCountable(&gc);
	}
}



/** update text layout if it is not valid */
void VTextLayout::_UpdateTextLayout()
{
	VTaskLock protect(&fMutex);
	if (!fGC)
	{
		//text layout is invalid if gc is not defined
		fLayoutIsValid = false;
		ReleaseRefCountable(&fTextBox);
#if ENABLE_D2D
		if (fLayoutD2D)
		{
			fLayoutD2D->Release();
			fLayoutD2D = NULL;
		}
#endif
		return;
	}
	if (fLayoutIsValid)
	{
		//check if font has changed
		if (!fCurFont)
			fLayoutIsValid = false;

		//check if text color has changed
		if (fLayoutIsValid)
		{
			if (!fHasDefaultTextColor)
			{
				//check if gc text color has changed
				VColor color;
				fGC->GetTextColor(color);
				if (color != fCurTextColor)
					fLayoutIsValid = false;
			}
#if VERSIONDEBUG
			else
				xbox_assert(fDefaultTextColor == fCurTextColor);
#endif
			//check if text rendering mode has changed
			if (fLayoutIsValid && fCurTextRenderingMode != fGC->GetTextRenderingMode())
				fLayoutIsValid = false;

			//check if kerning has changed (only on Mac OS: kerning is not used for text box layout on other impls)
			if (fLayoutIsValid && fCurKerning != fGC->GetCharActualKerning())
				fLayoutIsValid = false;
		}
	}

	if (fLayoutIsValid)
	{
		if (fNeedUpdateBounds) //update only bounds
		{
			fLayerIsDirty = true;
			fSelectionIsDirty = true;
			fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fCurFont->IsTrueTypeFont();
			if (fParagraphLayout)
				//platform-independent impl
				fParagraphLayout->UpdateLayout(fGC);
			else
				//native impl
				fGC->_UpdateTextLayout( this);
			fNeedUpdateBounds = false;
		}
	}
	else
	{
		ReleaseRefCountable(&fTextBox);
		fNeedUpdateBounds = false;

		//set cur font
		xbox_assert(fDefaultFont);
		ReleaseRefCountable(&fCurFont);
		fCurFont = RetainDefaultFont( fDPI);
		fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fCurFont->IsTrueTypeFont();

		//set cur text color
		if (fHasDefaultTextColor)
			fCurTextColor = fDefaultTextColor;
		else
		{
			VColor color;
			fGC->GetTextColor(color);
			fCurTextColor = color;
		}
		//set cur kerning
		fCurKerning = fGC->GetCharActualKerning();

		//set cur text rendering mode
		fCurTextRenderingMode = fGC->GetTextRenderingMode();

		fCurWidth = 0.0f;
		fCurHeight = 0.0f;
		fCurLayoutWidth = 0.0f;
		fCurLayoutHeight = 0.0f;
		fCurOverhang.left = 0.0f;
		fCurOverhang.right = 0.0f;
		fCurOverhang.top = 0.0f;
		fCurOverhang.bottom = 0.0f;
		fLayerIsDirty = true;

		//update font & text color
		VFont *fontOld = fGC->RetainFont();
		if (fontOld != fCurFont)
			fGC->SetFont( fCurFont);
		ReleaseRefCountable(&fontOld);
		
		VColor textColorOld;
		fGC->GetTextColor( textColorOld);
		if (fCurTextColor != textColorOld)
			fGC->SetTextColor( fCurTextColor);

		//compute text layout according to VTextLayout settings
		if (fParagraphLayout)
		{
			//platform-independent impl
			fParagraphLayout->UpdateLayout(fGC);
			fLayoutIsValid = true; 
		}
		else
		{
			//native impl
			_BeginUsingStyles();
			
			fGC->_UpdateTextLayout( this);

			if (fTextBox)
				fLayoutIsValid = true;
	#if ENABLE_D2D
			else if (fLayoutD2D)
				fLayoutIsValid = true;
	#endif
			else
				fLayoutIsValid = false;
			
			_EndUsingStyles();
		}


		fCharBackgroundColorDirty = true;
		fSelectionIsDirty = true;
	}
}

/** return true if layout styles uses truetype font only */
bool VTextLayout::_StylesUseFontTrueTypeOnly() const
{
	bool useTrueTypeOnly;
	if (fStyles)
		useTrueTypeOnly = VGraphicContext::UseFontTrueTypeOnlyRec( fStyles);
	else
		useTrueTypeOnly = true;
	if (fExtStyles && useTrueTypeOnly)
		useTrueTypeOnly = useTrueTypeOnly && VGraphicContext::UseFontTrueTypeOnlyRec( fExtStyles);
	return useTrueTypeOnly;
}

/** return true if layout uses truetype font only */
bool VTextLayout::UseFontTrueTypeOnly(VGraphicContext *inGC) const
{
	xbox_assert(inGC);
	bool curFontIsTrueType = false;
	if (testAssert(fDefaultFont))
		curFontIsTrueType = fDefaultFont->IsTrueTypeFont() != FALSE ? true : false;
	else 
	{
		VFont *font = inGC->RetainFont();
		if (!font)
			font = VFont::RetainStdFont(STDF_TEXT);
		curFontIsTrueType = font->IsTrueTypeFont() != FALSE ? true : false;
		ReleaseRefCountable(&font);
	}
	return fStylesUseFontTrueTypeOnly && curFontIsTrueType;
}

/** move character index to the nearest character index on the line before/above the character line 
@remarks
	return false if character is on the first line (ioCharIndex is not modified)
*/
bool VTextLayout::MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	if (!testAssert(fGC == NULL || fGC == inGC))
		return false;
	xbox_assert(inGC);
	bool inside;
	if (fParagraphLayout)
		//platform-independent impl
		return fParagraphLayout->MoveCharIndexUp( inGC, inTopLeft, ioCharIndex);
	else
	{
		BeginUsingContext( inGC, true);

		sLONG charIndex = ioCharIndex;
		VPoint pos;
		GReal height;
		GetCaretMetricsFromCharIndex( inGC, inTopLeft, ioCharIndex, pos, height, true, false);
		pos.SetPosBy(0,-4);
		GetCharIndexFromPos( inGC, inTopLeft, pos, ioCharIndex);
		
		EndUsingContext();
		
		return ioCharIndex != charIndex;
	}
}

/** move character index to the nearest character index on the line after/below the character line
@remarks
	return false if character is on the last line (ioCharIndex is not modified)
*/
bool VTextLayout::MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	if (!testAssert(fGC == NULL || fGC == inGC))
		return false;
	xbox_assert(inGC);
	bool inside;
	if (fParagraphLayout)
		//platform-independent impl
		return fParagraphLayout->MoveCharIndexDown( inGC, inTopLeft, ioCharIndex);
	else
	{
		BeginUsingContext( inGC, true);

		sLONG charIndex = ioCharIndex;
		VPoint pos;
		GReal height;
		GetCaretMetricsFromCharIndex( inGC, inTopLeft, ioCharIndex, pos, height, true, false);
		pos.SetPosBy(0,height+4);
		GetCharIndexFromPos( inGC, inTopLeft, pos, ioCharIndex);
		
		EndUsingContext();
		
		return ioCharIndex != charIndex;
	}
}




/** insert text at the specified position */
void VTextLayout::InsertText( sLONG inPos, const VString& inText)
{
	if (!inText.GetLength())
		return;

	if (fStyles)
	{
		fStyles->AdjustPos( inPos, false);

		if (!fParagraphLayout && fStyles->HasSpanRefs() && inPos > 0 && fLayoutIsValid && fStyles->IsOverSpanRef( inPos-1))
			fLayoutIsValid = false;
	}

	VTaskLock protect(&fMutex);
	xbox_assert(fTextPtr);
	sLONG lengthPrev = fTextLength;
	if (lengthPrev == 0)
		fLayoutIsValid = false;

	if (fTextPtr == &fText)
	{
		//restore actual text
		if (fTextPtrExternal)
		{
			fTextPtr = fTextPtrExternal;
			try
			{
				fTextLength = fTextPtrExternal->GetLength();
			}
			catch(...)
			{
				fTextPtrExternal = NULL;
				fTextPtr = &fText;
				if (fText.GetLength() > fTextLength)
					fText.Truncate(fTextLength);
			}
		}
		else if (fText.GetLength() > fTextLength)
			fText.Truncate(fTextLength);
	}

	bool isStartOfParagraph = false;
	bool isInsertingCR = false;
	bool needPropagateStyleToNextCR = false;

	if (inPos < 0)
		inPos = 0;
	if (inPos > fTextLength)
		inPos = fTextLength;
	if (!fTextLength)
	{
		if (fStyles)
			fStyles->Truncate(0);
		if (fExtStyles)
			fExtStyles->Truncate(0);
		*fTextPtr = inText;
		if (fStyles)
			fStyles->ExpandAtPosBy( 0, inText.GetLength());
		if (fExtStyles)
			fExtStyles->ExpandAtPosBy( 0, inText.GetLength());
		fLayoutIsValid = false;
	}
	else
	{
		if (inPos < fTextPtr->GetLength() && (inPos <= 0 || (*fTextPtr)[inPos-1] == 13)) 
		{
			//ensure inserted text will inherit style from paragraph first char if inserted at start of paragraph
			isStartOfParagraph = true;
			fLayoutIsValid = false;
		}
		//while inserting a CR or a string terminated with CR just before another CR, we need to propagate style to next CR in order next line to inherit current line style (standard Word/text edit behavior)
		isInsertingCR = inText.GetLength() >= 1 && inText.GetUniChar(inText.GetLength()) == 13;
		needPropagateStyleToNextCR = isInsertingCR && inPos > 0 && (fStyles || fExtStyles) && inPos < fTextLength && (*fTextPtr)[inPos] == 13;
		if (isInsertingCR)
			fLayoutIsValid = false;

		fTextPtr->Insert( inText, inPos+1);
		if (fStyles)
			fStyles->ExpandAtPosBy( inPos, inText.GetLength(), NULL, NULL, false, isStartOfParagraph);
		if (fExtStyles)
			fExtStyles->ExpandAtPosBy( inPos, inText.GetLength(), NULL, NULL, false, isStartOfParagraph);
	}
	fTextLength = fTextPtr->GetLength();

	_PostReplaceText();

	bool useFontTrueTypeOnly = fStylesUseFontTrueTypeOnly;
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	if (fStylesUseFontTrueTypeOnly != useFontTrueTypeOnly)
		fLayoutIsValid = false;

	if (fLayoutIsValid)
	{
		//direct update impl layout
		if (fParagraphLayout)
		{
			fParagraphLayout->InsertText( inPos, inText);
			fNeedUpdateBounds = true;
		}
		else if (fTextBox)
		{
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->InsertText( inPos, inText);
			fNeedUpdateBounds = true;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			fLayoutIsValid = false;
	}

	fCharBackgroundColorDirty = true;
	fSelectionIsDirty = true;

	if (needPropagateStyleToNextCR)
	{
		//propagate last line character style to CR following inserted text
		if (fStyles)
		{
			VTextStyle *style = fStyles->CreateUniformStyleOnRange( inPos-1, inPos, true);
			fStyles->ApplyStyle( style, true, inPos+inText.GetLength());
			delete style;
		}
		if (fExtStyles)
		{
			VTextStyle *style = fExtStyles->CreateUniformStyleOnRange( inPos-1, inPos, true);
			fExtStyles->ApplyStyle( style, true, inPos+inText.GetLength());
			delete style;
		}
	}
}


/** replace text */
void VTextLayout::ReplaceText( sLONG inStart, sLONG inEnd, const VString& inText)
{
	VTaskLock protect(&fMutex);
	if (inStart < 0)
		inStart = 0;
	if (inStart > fTextLength)
		inStart = fTextLength;
	if (inEnd < 0)
		inEnd = fTextLength;
	else if (inEnd > fTextLength)
		inEnd = fTextLength;
	
	if (fStyles)
	{
		fStyles->AdjustRange( inStart, inEnd);

		if (!fParagraphLayout && fStyles->HasSpanRefs() && inStart > 0 && fLayoutIsValid && fStyles->IsOverSpanRef( inStart-1))
			fLayoutIsValid = false;
	}
	xbox_assert(fTextPtr);
	sLONG lengthPrev = fTextLength;
	if (lengthPrev == 0)
		fLayoutIsValid = false; //first inserted text: rebuild full layout
	if (inStart != inEnd && !inText.IsEmpty()) //low-level impl might apply differently replacement of not empty text with not empty text so rebuild full layout (only insert/remove text seems to behave the same with any impl)
		fLayoutIsValid = false;

	if (fTextPtr == &fText)
	{
		//restore actual text
		if (fTextPtrExternal)
		{
			fTextPtr = fTextPtrExternal;
			try
			{
				fTextLength = fTextPtrExternal->GetLength();
			}
			catch(...)
			{
				fTextPtrExternal = NULL;
				fTextPtr = &fText;
				if (fText.GetLength() > fTextLength)
					fText.Truncate(fTextLength);
			}
		}
		else if (fText.GetLength() > fTextLength)
			fText.Truncate(fTextLength);
	}

	bool isStartOfParagraph = false;
	if (inEnd < fTextPtr->GetLength() && (inStart <= 0 || (*fTextPtr)[inStart-1] == 13)) 
	{	
		//ensure inserted text will inherit style from paragraph first char if inserted at start of paragraph
		isStartOfParagraph = true;
		fLayoutIsValid = false;
	}
	//while inserting a CR or a string terminated with CR just before another CR, we need to propagate style to next CR in order next line to inherit current line style (standard Word/text edit behavior)
	bool isInsertingCR = inText.GetLength() >= 1 && inText.GetUniChar(inText.GetLength()) == 13;
	bool needPropagateStyleToNextCR = isInsertingCR && (fStyles || fExtStyles) && inEnd < fTextLength && (*fTextPtr)[inEnd] == 13;
	if (isInsertingCR)
		fLayoutIsValid = false;

	bool done = false;
	if (inEnd > inStart && inText.GetLength() > 0) //same as at start of paragraph as we want new text to inherit previous text uniform style and not style from previous character
	{
		isStartOfParagraph = true; 
		fLayoutIsValid = false;

		if (inEnd-inStart > 1)
		{
			//style might not be uniform on the specified range: ensure we inherit only the first char style

			if (fStyles)
			{
				fStyles->FreezeSpanRefs(inStart, inEnd); //reset span references but keep normal style (we need to reset before as we truncate here not the full replaced range)
				fStyles->Truncate( inStart+1, inEnd-(inStart+1)); 
				if (inText.GetLength() > 1)
					fStyles->ExpandAtPosBy( inStart+1, inText.GetLength()-1, NULL, NULL, false, false);
			}
			if (fExtStyles)
			{
				fExtStyles->Truncate( inStart+1, inEnd-(inStart+1)); 
				if (inText.GetLength() > 1)
					fExtStyles->ExpandAtPosBy( inStart+1, inText.GetLength()-1, NULL, NULL, false, false);
			}
			done = true;
		}
	}
	if (!done)
	{
		if (fStyles)
		{
			if (inEnd > inStart)
				fStyles->Truncate( inStart, inEnd-inStart, inText.GetLength() == 0);
			if (inText.GetLength() > 0)
				fStyles->ExpandAtPosBy( inStart, inText.GetLength(), NULL, NULL, false, isStartOfParagraph);
		}
		if (fExtStyles)
		{
			if (inEnd > inStart)
				fExtStyles->Truncate( inStart, inEnd-inStart, inText.GetLength() == 0);
			if (inText.GetLength() > 0)
				fExtStyles->ExpandAtPosBy( inStart, inText.GetLength(), NULL, NULL, false, isStartOfParagraph);
		}
	}

	if (fStyles && fStyles->GetChildCount() == 0 && fStyles->GetData()->IsUndefined())
	{
		ReleaseRefCountable(&fStyles);
		fLayoutIsValid = false; 
	}

	if (inEnd > inStart)
		fTextPtr->Replace(inText, inStart+1, inEnd-inStart);
	else 
		fTextPtr->Insert(inText, inStart+1);
	
	fTextLength = fTextPtr->GetLength();
	if (fTextLength == 0)
	{
		//if text is empty, we need to set layout text to a dummy "x" text otherwise 
		//we cannot get correct metrics for caret or for text layout bounds  
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromCString("x");
		fTextPtr = &fText;
        fLayoutIsValid = false;
	}
	_PostReplaceText();
	
	bool useFontTrueTypeOnly = fStylesUseFontTrueTypeOnly;
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	if (fStylesUseFontTrueTypeOnly != useFontTrueTypeOnly)
		fLayoutIsValid = false;
	
	if (fLayoutIsValid)
	{
		//direct update impl layout
		if (fParagraphLayout)
		{
			fParagraphLayout->ReplaceText(inStart, inEnd, inText);
			fNeedUpdateBounds = true;
		}
		else if (fTextBox)
		{
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->ReplaceText(inStart, inEnd, inText);
			fNeedUpdateBounds = true;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			fLayoutIsValid = false;
	}
	
	if (needPropagateStyleToNextCR && inStart+inText.GetLength() > 0)
	{
		//propagate last line character style to CR following inserted text
		if (fStyles)
		{
			VTextStyle *style = fStyles->CreateUniformStyleOnRange( inStart+inText.GetLength()-1, inStart+inText.GetLength(), true);
			fStyles->ApplyStyle( style, true, inStart+inText.GetLength());
			delete style;
		}
		if (fExtStyles)
		{
			VTextStyle *style = fExtStyles->CreateUniformStyleOnRange( inStart+inText.GetLength()-1, inStart+inText.GetLength(), true);
			fExtStyles->ApplyStyle( style, true, inStart+inText.GetLength());
			delete style;
		}
	}

	fCharBackgroundColorDirty = true;
	fSelectionIsDirty = true;
}


/** delete text range */
void VTextLayout::DeleteText( sLONG inStart, sLONG inEnd)
{
	VTaskLock protect(&fMutex);
	if (inStart < 0)
		inStart = 0;
	if (inStart > fTextLength)
		inStart = fTextLength;
	if (inEnd < 0)
		inEnd = fTextLength;
	else if (inEnd > fTextLength)
		inEnd = fTextLength;
	if (inStart >= inEnd)
		return;

	if (fStyles)
		fStyles->AdjustRange( inStart, inEnd);

	xbox_assert(fTextPtr);
	sLONG lengthPrev = fTextLength;
	if (fTextPtr == &fText)
	{
		//restore actual text
		if (fTextPtrExternal)
		{
			fTextPtr = fTextPtrExternal;
			try
			{
				fTextLength = fTextPtrExternal->GetLength();
			}
			catch(...)
			{
				fTextPtrExternal = NULL;
				fTextPtr = &fText;
				if (fText.GetLength() > fTextLength)
					fText.Truncate(fTextLength);
			}
		}
		else if (fText.GetLength() > fTextLength)
			fText.Truncate(fTextLength);
	}

	if (fStyles)
		fStyles->Truncate(inStart, inEnd-inStart);
	if (fStyles->GetChildCount() == 0 && fStyles->GetData()->IsUndefined())
	{
		ReleaseRefCountable(&fStyles);
		fLayoutIsValid = false;
	}

	if (fExtStyles)
		fExtStyles->Truncate(inStart, inEnd-inStart);
	fTextPtr->Remove( inStart+1, inEnd-inStart);
	fTextLength = fTextPtr->GetLength();
	if (fTextLength == 0)
	{
		//if text is empty, we need to set layout text to a dummy "x" text otherwise 
		//we cannot get correct metrics for caret or for text layout bounds  
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromCString("x");
		fTextPtr = &fText;
        fLayoutIsValid = false;
	}
	_PostReplaceText();

	bool useFontTrueTypeOnly = fStylesUseFontTrueTypeOnly;
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	if (fStylesUseFontTrueTypeOnly != useFontTrueTypeOnly)
		fLayoutIsValid = false;

	if (fLayoutIsValid)
	{
		//direct update impl layout
		if (fParagraphLayout)
		{
			fParagraphLayout->DeleteText( inStart, inEnd);
			fNeedUpdateBounds = true;
		}
		else if (fTextBox)
		{
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->DeleteText( inStart, inEnd);
			fNeedUpdateBounds = true;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			fLayoutIsValid = false;
	}
	
	fCharBackgroundColorDirty = true;
	fSelectionIsDirty = true;
}



VTreeTextStyle *VTextLayout::_RetainDefaultTreeTextStyle() const
{
	if (!fDefaultStyle)
	{
		VTextStyle *style = testAssert(fDefaultFont) ? VTextLayout::CreateTextStyle( fDefaultFont) : new VTextStyle();

		if (fHasDefaultTextColor)
		{
			style->SetHasForeColor(true);
			style->SetColor( fDefaultTextColor.GetRGBAColor());
		}
		if (fHAlign != AL_DEFAULT)
			//avoid character style to override paragraph align style if it is equal to paragraph align style
			style->SetJustification( VTextLayout::JustFromAlignStyle( fHAlign));

		if (fBackColor.GetRGBAColor() != RGBACOLOR_TRANSPARENT)
		{
			//avoid layout character styles to store background color if it is equal to parent frame back color
			style->SetHasBackColor(true);
			style->SetBackGroundColor(fBackColor.GetRGBAColor());
		}
		fDefaultStyle = new VTreeTextStyle( style);
	}

	fDefaultStyle->GetData()->SetRange(0, fTextLength);
	fDefaultStyle->Retain();
	return fDefaultStyle;
}

/** retain full styles
@remarks
	use this method to get the full styles applied to the text content
	(but not including the graphic context default font: this font is already applied internally by gc methods before applying styles)

	caution: please do not modify fStyles between _RetainFullTreeTextStyle & _ReleaseFullTreeTextStyle

	(in monostyle, it is generally equal to _RetainDefaultTreeTextStyle)
*/
VTreeTextStyle *VTextLayout::_RetainFullTreeTextStyle() const
{
	if (!fStyles)
		return _RetainDefaultTreeTextStyle();

	VTreeTextStyle *styles = _RetainDefaultTreeTextStyle();
	if (testAssert(styles))
		styles->AddChild(fStyles);
	return styles;
}

void VTextLayout::_ReleaseFullTreeTextStyle( VTreeTextStyle *inStyles) const
{
	if (!inStyles)
		return;

	if (testAssert(inStyles == fDefaultStyle))
	{
		if (inStyles->GetChildCount() > 0)
		{
			xbox_assert(inStyles->GetChildCount() == 1);
			inStyles->RemoveChildAt(1);
		}
	}
	inStyles->Release();
}

bool VTextLayout::ApplyStyles( const VTreeTextStyle * inStyles, bool inFast)
{
	VTaskLock protect(&fMutex);
	return _ApplyStylesRec( inStyles, inFast);
}

bool VTextLayout::_ApplyStylesRec( const VTreeTextStyle * inStyles, bool inFast)
{
	if (!inStyles)
		return false;
	bool update = _ApplyStyle( inStyles->GetData());
	if (update)
	{
		if (!inStyles->GetData()->GetFontName().IsEmpty())
			fLayoutIsValid = false;

		if (fLayoutIsValid && inFast)
		{
			//direct update impl layout
			if (fParagraphLayout)
			{
				fParagraphLayout->ApplyStyle( inStyles->GetData());
				fNeedUpdateBounds = true;
			}
			else if (fTextBox)
			{
				fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
				fTextBox->ApplyStyle( inStyles->GetData());
				fLayoutIsValid = !fTextBox->ShouldResetLayout();
				fNeedUpdateBounds = true;
			}
			else
				//layout direct update not supported are not implemented: compute all layout
				fLayoutIsValid = false;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			fLayoutIsValid = false;

		fCharBackgroundColorDirty = true;
		fSelectionIsDirty = true;
	}
	
	VIndex count = inStyles->GetChildCount();
	for (int i = 1; i <= count; i++)
		update = _ApplyStylesRec( inStyles->GetNthChild(i)) || update;
	
	return update;
}


/** apply style (use style range) */
bool VTextLayout::ApplyStyle( const VTextStyle* inStyle, bool inFast, bool inIgnoreIfEmpty)
{
	VTaskLock protect(&fMutex);
	if (_ApplyStyle( inStyle, inIgnoreIfEmpty))
	{
		if (inStyle->IsSpanRef())
			fLayoutIsValid = false;
		
		if (!inStyle->GetFontName().IsEmpty())
			fLayoutIsValid = false;

		if (fLayoutIsValid && inFast)
		{
			//direct update impl layout
			if (fParagraphLayout)
			{
				fParagraphLayout->ApplyStyle( inStyle);
				fNeedUpdateBounds = true;
			}
			else if (fTextBox)
			{
				fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
				fTextBox->ApplyStyle( inStyle);
				fLayoutIsValid = !fTextBox->ShouldResetLayout();
				fNeedUpdateBounds = true;
			}
			else
				//layout direct update not supported are not implemented: compute all layout
				fLayoutIsValid = false;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			fLayoutIsValid = false;
		fCharBackgroundColorDirty = true;
		fSelectionIsDirty = true;
		return true;
	}
	return false;
}



bool  VTextLayout::_ApplyStyle(const VTextStyle *inStyle, bool inIgnoreIfEmpty)
{
	if (!inStyle)
		return false;
	sLONG start, end;
	inStyle->GetRange( start, end);
	if (start > end || inStyle->IsUndefined())
		return false;

	bool needUpdate = false;
	VTextStyle *newStyle = NULL;
	if (start <= 0 && end >= fTextLength)
	{
		//update default uniform style: this is optimization to avoid to create or update unnecessary fStyles (this speeds up rendering too)
		if (!inStyle->GetFontName().IsEmpty()
			||
			(fDefaultFont
			&&
				(
				inStyle->GetItalic() != UNDEFINED_STYLE
				||
				inStyle->GetBold() != UNDEFINED_STYLE
				||
				inStyle->GetUnderline() != UNDEFINED_STYLE
				||
				inStyle->GetStrikeout() != UNDEFINED_STYLE
				||
				inStyle->GetFontSize() != UNDEFINED_STYLE
				)
			)
			)
		{
			//set new default font as combination of current default font (if any) & inStyle

			VString fontname;
			GReal fontsize = 12; 
			VFontFace fontface = 0;

			if (fDefaultFont)
			{
				fontname = fDefaultFont->GetName();
				fontsize = fDefaultFont->GetPixelSize(); 
				fontface = fDefaultFont->GetFace();
			}
			if (!inStyle->GetFontName().IsEmpty())
				fontname = inStyle->GetFontName();
			if (inStyle->GetFontSize() > 0)
				fontsize = inStyle->GetFontSize();
			if (inStyle->GetItalic() == TRUE)
				fontface |= KFS_ITALIC;
			else if (inStyle->GetItalic() == FALSE)
				fontface &= ~KFS_ITALIC;
			if (inStyle->GetStrikeout() == TRUE)
				fontface |= KFS_STRIKEOUT;
			else if (inStyle->GetStrikeout() == FALSE)
				fontface &= ~KFS_STRIKEOUT;
			if (inStyle->GetUnderline() == TRUE)
				fontface |= KFS_UNDERLINE;
			else if (inStyle->GetUnderline() == FALSE)
				fontface &= ~KFS_UNDERLINE;
			if (inStyle->GetBold() == TRUE)
				fontface |= KFS_BOLD;
			else if (inStyle->GetBold() == FALSE)
				fontface &= ~KFS_BOLD;
			VFont *font = VFont::RetainFont( fontname, fontface, fontsize, 72.0f);
			if (font != fDefaultFont)
			{
				CopyRefCountable(&fDefaultFont, font);		
				ReleaseRefCountable(&fDefaultStyle);
				needUpdate = true;
				fLayoutIsValid = false; //recompute all layout
			}
			font->Release();
		}
		if (inStyle->GetHasForeColor())
		{
			//modify current default text color
			VColor color;
			color.FromRGBAColor( inStyle->GetColor());
			if (!fHasDefaultTextColor || color != fDefaultTextColor)
			{
				fHasDefaultTextColor = true;
				fDefaultTextColor = color;
				ReleaseRefCountable(&fDefaultStyle);
				needUpdate = true;
				fLayoutIsValid = false; //recompute all layout
			}
		}
		if (inStyle->GetJustification() != JST_Notset)
		{
			//modify current default horizontal justification
			AlignStyle align = JustToAlignStyle( inStyle->GetJustification());
			if (align != fHAlign)
			{
				fHAlign = align;
				needUpdate = true;
			}
			newStyle = new VTextStyle( inStyle);
			inStyle = newStyle;
			newStyle->SetJustification( JST_Notset);
		}
	}
	else
	{
		if (inStyle->GetJustification() != JST_Notset)
		{
			//for now we can only update justification globally

			newStyle = new VTextStyle( inStyle);
			inStyle = newStyle;
			newStyle->SetJustification( JST_Notset);
		}
	}

	if (inStyle->IsSpanRef())
	{
		VDocSpanTextRef *spanRef = inStyle->GetSpanRef();
		if (spanRef->ShowRef() != fShowRefs || spanRef->ShowHighlight() != fHighlightRefs)
		{
			if (inStyle != newStyle)
			{
				newStyle = new VTextStyle( inStyle);
				inStyle = newStyle;
			}
			newStyle->GetSpanRef()->ShowRef( fShowRefs);
			newStyle->GetSpanRef()->ShowHighlight( fHighlightRefs);
		}
	}

	if (!inStyle->IsUndefined())
	{
		if (fStyles)
		{
			needUpdate = true;

			VTreeTextStyle *styles = _RetainFullTreeTextStyle(); //to ensure fStyles will use fDefaultStyle as parent 
																 //(so only styles which are different from default uniform style will be overriden)

			fStyles->ApplyStyle( inStyle, inIgnoreIfEmpty); //we apply only on fStyles

			_ReleaseFullTreeTextStyle( styles); //detach fStyles from fDefaultStyle

			if (fStyles->GetChildCount() == 0 && fStyles->GetData()->IsUndefined())
			{
				ReleaseRefCountable(&fStyles);
				fLayoutIsValid = false;
			}
		}
		else
		{
			fStyles = new VTreeTextStyle( new VTextStyle());
			fStyles->GetData()->SetRange(0, fTextLength);

			VTreeTextStyle *styles = _RetainFullTreeTextStyle(); //to ensure fStyles will use fDefaultStyle as parent 
																 //(so only styles which are different from default uniform style will be overriden)
			fStyles->ApplyStyle( inStyle, inIgnoreIfEmpty); //we apply only on fStyles

			_ReleaseFullTreeTextStyle( styles); //detach fStyles from fDefaultStyle

			if (fStyles->GetChildCount() == 0 && fStyles->GetData()->IsUndefined())
			{
				//can happen if input style does not override current default font or style
				ReleaseRefCountable(&fStyles);
			}
			else
				needUpdate = true;
		}
	}
	if (inStyle == newStyle)
		delete newStyle;
	return needUpdate;
}


VTextStyle*	VTextLayout::CreateTextStyle( const VFont *inFont, const GReal inDPI)
{
	//here we must pass pixel font size because VTextStyle stores 4D form 72 dpi font size where 1px = 1pt so actually it is real pixel font size
	//(size is scaled automatically later to 72/device dpi to be compliant with 4D form dpi)
	return CreateTextStyle( inFont->GetName(), inFont->GetFace(), inFont->GetPixelSize()*72.0f/inDPI, 72.0f);
}

/**	create text style from the specified font name, font size & font face
@remarks
	if font name is empty, returned style inherits font name from parent
	if font face is equal to UNDEFINED_STYLE, returned style inherits style from parent
	if font size is equal to UNDEFINED_STYLE, returned style inherits font size from parent
*/
VTextStyle*	VTextLayout::CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI)
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







