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


/**	VTextLayout constructor
@param inPara
	VDocParagraph source: the passed paragraph is used only for initializing control plain text, styles & paragraph properties
						  but inPara is not retained 
	(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
@param inShouldEnableCache
	enable/disable cache (default is true): see ShouldEnableCache
*/
VTextLayout::VTextLayout(const VDocParagraph *inPara, bool inShouldEnableCache)
{
	xbox_assert(inPara);
	_Init();
	fShouldEnableCache = inShouldEnableCache;

	_ReplaceText( inPara);

	VTreeTextStyle *styles = inPara->RetainStyles();
	ApplyStyles( styles);
	ReleaseRefCountable(&styles);
	
	_ApplyParagraphProps( inPara, false);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	ReleaseRefCountable(&fDefaultStyle);
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
		//if (isDefaultStylesDirty)
		//	ReleaseRefCountable(&fDefaultStyle);
		
		ReleaseRefCountable(&fDefaultStyle); //as we have modified fDefaultStyle, clear it so it is created again next time it is requested
	}

	//copy paragraph properties
	sDocPropPaddingMargin margin;
	margin.left = fMarginLeftTWIPS;
	margin.right = fMarginRightTWIPS;
	margin.top = fMarginTopTWIPS;
	margin.bottom = fMarginBottomTWIPS;
	if (!(VDocProperty(margin) == para->GetDefaultPropValue( kDOC_PROP_MARGIN)))
		para->SetMargin( margin);

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
	//(caution: might be different from document DPI -> document DPI is used only on xml parsing to convert from px to res-independent unit like pt)

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_MARGIN) && !inPara->IsInherited( kDOC_PROP_MARGIN)))
	{
		sDocPropPaddingMargin margin = inPara->GetMargin();
		SetMargins( ICSSUtil::TWIPSToPoint(margin.left), 
					ICSSUtil::TWIPSToPoint(margin.right),
					ICSSUtil::TWIPSToPoint(margin.top),
					ICSSUtil::TWIPSToPoint(margin.bottom));
	}

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


VTextLayout::VTextLayout(bool inShouldEnableCache)
{
	_Init();
	fShouldEnableCache = inShouldEnableCache;

	ResetParagraphStyle();
}

void VTextLayout::_Init()
{
	fGC = NULL;
	fLayoutIsValid = false;
	fGCUseCount = 0;
	fShouldEnableCache = false;
	fShouldDrawOnLayer = true;
	fShouldAllowClearTypeOnLayer = true;
	fLayerIsForMetricsOnly = false;
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
	fPaintBackColor = false;
	fDefaultStyle = NULL;
	fCurFont = NULL;
	fCurKerning = 0.0f;
	fCurTextRenderingMode = TRM_NORMAL;
	fCurWidth = 0.0f;
	fCurHeight = 0.0f;
	fCurLayoutWidth = 0.0f;
	fCurLayoutHeight = 0.0f;
	fCurOverhangLeft = 0.0f;
	fCurOverhangRight = 0.0f;
	fCurOverhangTop = 0.0f;
	fCurOverhangBottom = 0.0f;
	fStyles = fExtStyles = NULL;
	fMaxWidth = fMaxWidthInit = 0.0f;
	fMaxHeight = fMaxHeightInit = 0.0f;
	fNeedUpdateBounds = false;

	fMarginLeftTWIPS = fMarginRightTWIPS = fMarginTopTWIPS = fMarginBottomTWIPS = 0;
	fMarginLeft = fMarginRight = fMarginTop = fMarginBottom = 0.0f;
	fHAlign = AL_DEFAULT;
	fVAlign = AL_DEFAULT;
	fLineHeight = -1;
	fPaddingFirstLine = 0;
	fTabStopOffset = ICSSUtil::CSSDimensionToPoint(1.25, kCSSUNIT_CM);
	fTabStopType = TTST_LEFT;
	fLayoutMode = TLM_NORMAL;

	fShowRefs = false;
	fHighlightRefs = false;

	fDPI = 72.0f;
	fUseFontTrueTypeOnly = false;
	fStylesUseFontTrueTypeOnly = true;
	fTextBox = NULL;
#if ENABLE_D2D
	fLayoutD2D = NULL;
#endif
	fBackupFont = NULL;
	fIsPrinting = false;
	fSkipDrawLayer = false;
	fShouldDrawCharBackColor = false;
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
#if ENABLE_D2D
	if (fLayoutD2D)
	{
		fLayoutD2D->Release();
		fLayoutD2D = NULL;
	}
#endif
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


void VTextLayout::_ResetLayer(VGraphicContext *inGC, bool inAlways)
{
	if (!inGC)
		inGC = fGC;
	xbox_assert(inGC);

	fLayerIsDirty = true;
	if (!fShouldEnableCache)
	{
		fLayerIsForMetricsOnly = true;
		ReleaseRefCountable(&fLayerOffScreen);
		return;
	}
	if (!fLayerOffScreen || !fLayerIsForMetricsOnly || inAlways)
	{
		fLayerIsForMetricsOnly = true;
		ReleaseRefCountable(&fLayerOffScreen);
#if VERSIONWIN
		if (!inGC->IsGDIImpl())
#endif
			fLayerOffScreen = new VImageOffScreen( inGC); //reset layer to 1x1 size (this compatible layer is used only for metrics)
	}
}


void VTextLayout::_UpdateBackgroundColorRenderInfo(const VPoint& inTopLeft, const VRect& inClipBounds)
{
	xbox_assert(fGC);

	if (!fShouldDrawCharBackColor)
		return;
	if (!fCharBackgroundColorDirty)
		return;

	fCharBackgroundColorDirty = false;
	fCharBackgroundColorRenderInfo.clear();

	VIndex start;
	GetCharIndexFromPos( fGC, inTopLeft, inClipBounds.GetTopLeft(), start);
	
	VIndex end;
	GetCharIndexFromPos( fGC, inTopLeft, inClipBounds.GetBotRight(), end);

	if (start >= end)
		return;

	if (fStyles)
		_UpdateBackgroundColorRenderInfoRec( inTopLeft, fStyles, start, end);
	if (fExtStyles)
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
	if (fPaintBackColor)
	{
		fGC->SetFillColor( fBackColor);
		fGC->FillRect( inBoundsLayout);
	}

	if (!fShouldDrawCharBackColor)
		return;

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

bool VTextLayout::_CheckLayerBounds(const VPoint& inTopLeft, VRect& outLayerBounds)
{
	outLayerBounds = VRect( inTopLeft.GetX(), inTopLeft.GetY(), fCurLayoutWidth, fCurLayoutHeight); //on default, layer bounds = layout bounds

	fShouldDrawCharBackColor = !fGC->CanDrawStyledTextBackColor() && ((fGC->GetTextRenderingMode() & TRM_LEGACY_OFF) || fUseFontTrueTypeOnly);	

	if ((!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl()) //not draw on layer
		&& 
		!fShouldDrawCharBackColor) //not draw background color
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
			//layout has scrolled in view window: mark it as dirty
			fCurLayerOffsetViewRect = offset;
			fLayerIsDirty = true;
			fCharBackgroundColorDirty = true;
		}
	}

	if (outLayerBounds.IsEmpty())
	{
		//do not draw at all if layout is fully clipped by fLayerViewRect
		_ResetLayer();
	
		fCharBackgroundColorRenderInfo.clear();
		fCharBackgroundColorDirty = false;
		return false; 
	}

	_UpdateBackgroundColorRenderInfo(inTopLeft, outLayerBounds);
	return true;
}


bool VTextLayout::_BeginDrawLayer(const VPoint& inTopLeft)
{
	xbox_assert(fGC && fSkipDrawLayer == false);

	VRect boundsLayout;
	bool doDraw = _CheckLayerBounds( inTopLeft, boundsLayout);

	if (!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl())
	{
		if (doDraw)
			_DrawBackgroundColor( inTopLeft, boundsLayout);
		return doDraw;
	}

	if (!fLayerOffScreen)
		fLayerIsDirty = true;

	bool isLayerTransparent = !fPaintBackColor || fBackColor.GetAlpha() != 255;

#if VERSIONWIN
	if (fShouldAllowClearTypeOnLayer && isLayerTransparent && !fGC->IsGDIImpl())
	{
		TextRenderingMode trm = fGC->fMaxPerfFlag ? TRM_WITHOUT_ANTIALIASING : fGC->fHighQualityTextRenderingMode;
		if (!(trm & TRM_WITHOUT_ANTIALIASING))
			if (!(trm & TRM_WITH_ANTIALIASING_NORMAL))
			{
				//if ClearType is enabled, do not draw on layer GC but draw on parent GC
				//(but keep layer in order to preserve the layout)
				fSkipDrawLayer = true;
				_ResetLayer();
				if (doDraw)
					_DrawBackgroundColor( inTopLeft, boundsLayout);
				return doDraw;
			}
	}
#elif VERSIONMAC
    if (isLayerTransparent)
    {
        fSkipDrawLayer = true;
        _ResetLayer();
        if (doDraw)
            _DrawBackgroundColor( inTopLeft, boundsLayout);
        return doDraw;
    }
#endif

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
		else
			fLayerIsForMetricsOnly = false;
		_DrawBackgroundColor( inTopLeft, boundsLayout);
	}
	else
		fSkipDrawLayer = true;
	return doRedraw;
}

void VTextLayout::_EndDrawLayer()
{
#if VERSIONWIN
	if (!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl() || fSkipDrawLayer)
	{
		fSkipDrawLayer = false;
		return;
	}
#else
	if (!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fSkipDrawLayer)
    {
		fSkipDrawLayer = false;
		return;
    }
#endif

	xbox_assert(fLayerOffScreen == NULL);
	fLayerOffScreen = fGC->EndLayerOffScreen();
	fLayerIsDirty = fLayerOffScreen == NULL;
}


/** set text layout graphic context 
@remarks
	it is the graphic context to which is bound the text layout
	if gc is changed and offscreen layer is disabled or gc is not compatible with the actual offscreen layer gc, text layout needs to be computed & redrawed again 
	so it is recommended to enable the internal offscreen layer in order to preserve layout on multiple frames
	(and to not redraw text layout at every frame) because offscreen layer & text content are preserved as long as it is compatible with the attached gc & text content is not dirty
*/
void VTextLayout::_SetGC( VGraphicContext *inGC)
{
	if (fGC == inGC)
		return;
	if (fLayoutIsValid && inGC == NULL && fGC && fShouldEnableCache && !fLayerOffScreen && !fIsPrinting
#if VERSIONWIN
		&& !fGC->IsGDIImpl()
#endif
		)
	{
		//here we are about to detach a gc from the text layout:
		//in order to preserve the layout, if there is not yet a offscreen layer,
		//we create a dummy layer compatible with the last used gc 
		//which will be used to preserve layout until text is drawed first time or if text is not pre-rendered (on Windows, text is not pre-rendered if fUseClearTypeOnLayer == true && current text rendering mode is set to ClearType or system default)
		//(it is necessary if caller calls many metric methods for instance before drawing first time)
		_ResetLayer();
	}

#if VERSIONWIN	
	if (inGC)
	{
		//ensure current layout impl is consistent with current kind of gc
		if (!inGC->IsD2DImpl())
		{
			if (fLayoutD2D)
			{
				fLayoutD2D->Release();
				fLayoutD2D = NULL;
			}
			if (!fTextBox)
				fLayoutIsValid = false;
		}
	}

	if ((inGC && inGC->IsGDIImpl()) //attaching GDI context
		|| 
		(!inGC && (fGC && fGC->IsGDIImpl()))) //detaching GDI context
		//for GDI we do not take account offscreen layer for layout validity if we are not printing
		//(we assume we use always a GDI context compatible with the screen DPI if we are not printing: 
		// caller should use VTextLayout::SetDPI to change layout dpi rather than changing GDI device dpi if caller needs a dpi not equal to screen dpi;
		// doing that way allows VTextLayout to cache text layout for a fixed dpi & dpi scaling is applied internally by scaling fonts rather than device dpi)
		fLayoutIsValid = fLayoutIsValid && fShouldEnableCache && !fIsPrinting;
	else
#endif
		fLayoutIsValid = fLayoutIsValid && fShouldEnableCache && fLayerOffScreen && !fIsPrinting;
#if VERSIONWIN
	//we need to check layer offscreen compatibility with new gc to determine if layer is still suitable
	//(because on Windows there are up to 3 kind of graphic context depending on platform availability...)
	//
	//note that normally the gc is capable of determining that itself & update layer impl seamlessly while drawing with layer 
	//but we need to determine compatibility here in order to reset text layout too because text layout metrics are also bound to the kind of graphic context
	//(otherwise text layout metrics would be not consistent with gc if kind of gc is changed)
	if (inGC && fLayerOffScreen)
	{
		if (inGC->IsGDIImpl())
		{
			//disable offscreen for GDI 
			ReleaseRefCountable(&fLayerOffScreen);
			fLayerIsDirty = true;
			if (fIsPrinting)
				fLayoutIsValid = false;
		}
#if ENABLE_D2D
		else if (inGC->IsGdiPlusImpl())
		{
			if (!fLayerOffScreen->IsGDIPlusImpl())
			{
				//layer is not a GDIPlus layer: reset layer & layout
				_ResetLayer( inGC, true);
				fLayoutIsValid = false;
			}
		}
		else if (inGC->IsD2DImpl())
		{
			if (!fLayerOffScreen->IsD2DImpl())
			{
				//layer is not a D2D layer: reset layer & layout
				_ResetLayer( inGC, true);
				fLayoutIsValid = false;
			}
			else 
			{
				//check if D2D layer uses same resource domain than current gc
				//caution: here gc render target should have been initialized otherwise gc rt resource domain is undetermined
				ID2D1BitmapRenderTarget *bmpRT = (ID2D1BitmapRenderTarget *)(*fLayerOffScreen);
				xbox_assert(bmpRT);
				
				D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
					inGC->IsHardware() ? D2D1_RENDER_TARGET_TYPE_HARDWARE : D2D1_RENDER_TARGET_TYPE_SOFTWARE,
					D2D1::PixelFormat(),
					0.0f,
					0.0f,
					D2D_RENDER_TARGET_USE_GDI_COMPATIBLE_DC ? D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE : D2D1_RENDER_TARGET_USAGE_NONE,
					inGC->IsHardware() ? D2D1_FEATURE_LEVEL_10 : D2D1_FEATURE_LEVEL_DEFAULT
					);
				if (!bmpRT->IsSupported( props))
				{
					//layer resource domain is not equal to gc resource domain: reset layer & impl layout 
					_ResetLayer( inGC, true);
					fLayoutIsValid = false;
				}
			}
		}
#endif
	}
#endif
	//attach new gc or dispose actual gc (if inGC == NULL)
	CopyRefCountable(&fGC, inGC);
	if (fGC && fShouldEnableCache && !fIsPrinting && (!fLayerOffScreen || !fShouldDrawOnLayer))
		_ResetLayer(); //ensure a compatible layer is created if cache is enabled & we are not printing
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
	if (fDPI == inDPI)
		return;
	fLayoutIsValid = false;
	fDPI = inDPI;
}


/** replace margins (in point) */
void	VTextLayout::SetMargins(const GReal inMarginLeft, const GReal inMarginRight, const GReal inMarginTop, const GReal inMarginBottom)	
{
	//to avoid math discrepancies from res-independent point to current DPI, we always store in TWIPS & convert from TWIPS

	fMarginLeftTWIPS	= ICSSUtil::PointToTWIPS( inMarginLeft);
	fMarginRightTWIPS	= ICSSUtil::PointToTWIPS( inMarginRight);
	fMarginTopTWIPS		= ICSSUtil::PointToTWIPS( inMarginTop);
	fMarginBottomTWIPS	= ICSSUtil::PointToTWIPS( inMarginBottom);

	fMarginLeft		= floor(ICSSUtil::TWIPSToPoint( fMarginLeftTWIPS) * fDPI/72 +0.5f);
	fMarginRight	= floor(ICSSUtil::TWIPSToPoint( fMarginRightTWIPS) * fDPI/72 +0.5f);
	fMarginTop		= floor(ICSSUtil::TWIPSToPoint( fMarginTopTWIPS) * fDPI/72 +0.5f);
	fMarginBottom	= floor(ICSSUtil::TWIPSToPoint( fMarginBottomTWIPS) * fDPI/72 +0.5f);

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

/** get margins (in point) */
void VTextLayout::GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom)
{
	if (outMarginLeft)
		*outMarginLeft = ICSSUtil::TWIPSToPoint( fMarginLeftTWIPS);
	if (outMarginRight)
		*outMarginRight = ICSSUtil::TWIPSToPoint( fMarginRightTWIPS);
	if (outMarginTop)
		*outMarginTop = ICSSUtil::TWIPSToPoint( fMarginTopTWIPS);
	if (outMarginBottom)
		*outMarginBottom = ICSSUtil::TWIPSToPoint( fMarginBottomTWIPS);
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
#if VERSIONMAC
	else if (inText.GetUniChar(fTextLength) == 13)
	{
		//if last character is a CR, we need to append some dummy whitespace character otherwise 
		//we cannot get correct metrics after last CR (CoreText bug)
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromString(inText);
		fText.AppendChar(' ');
		fTextPtr = &fText;
	}
	else
#endif
	{
		if (&inText != &fText)
			fText.FromString(inText);
		fTextPtr = &fText;
	}

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
#if VERSIONMAC
	else if (inText->GetUniChar(fTextLength) == 13)
	{
		//if last character is a CR, we need to append some dummy whitespace character otherwise 
		//we cannot get correct metrics after last CR (CoreText bug)
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromString(*inText);
		fText.AppendChar(' ');
		fTextPtr = &fText;
	}
	else
#endif
	{
		if (inCopyText)
		{
			fText.FromString(*inText);
			fTextPtr = &fText;
		}
		else
			fTextPtr = inText;
	}

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
		//NDJQ: here we can check only the attached gc because offscreen layer (if enabled) will inherit the attached gc context settings when BeginLayerOffScreen is called
		//		(layer surface compatiblity with fGC has been already checked in _SetGC)

		//check if font has changed
		if (!fCurFont)
			fLayoutIsValid = false;
		else 
		{
			if (!testAssert(fDefaultFont)) //forbidden now
			{
				//check if gc font has changed
				VFont *font = fGC->RetainFont();
				if (!font)
					font = VFont::RetainStdFont(STDF_TEXT);
				if (font != fCurFont)
					fLayoutIsValid = false;
				ReleaseRefCountable(&font);
			}
#if VERSION_DEBUG
			else
				xbox_assert(fDefaultFont == fCurFont);
#endif
		}
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
#if VERSION_DEBUG
			else
				xbox_assert(fDefaultTextColor == fCurTextColor);
#endif
			//check if text rendering mode has changed
			if (fLayoutIsValid && fCurTextRenderingMode != fGC->GetTextRenderingMode())
				fLayoutIsValid = false;
#if VERSIONMAC
			//check if kerning has changed (only on Mac OS: kerning is not used for text box layout on other impls)
			if (fLayoutIsValid && fCurKerning != fGC->GetCharActualKerning())
				fLayoutIsValid = false;
#endif
		}
	}

	if (fLayoutIsValid)
	{
		if (fNeedUpdateBounds) //update only bounds
		{
#if VERSIONWIN
			fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fCurFont->IsTrueTypeFont();
#else
			fUseFontTrueTypeOnly = true;
#endif
			fGC->_UpdateTextLayout( this);
		}
	}
	else
	{
		ReleaseRefCountable(&fTextBox);

		//set cur font
		if (testAssert(fDefaultFont))
			CopyRefCountable(&fCurFont, fDefaultFont);
		else 
		{
			VFont *font = fGC->RetainFont();
			if (!font)
				font = VFont::RetainStdFont(STDF_TEXT);
			CopyRefCountable(&fCurFont, font);
			ReleaseRefCountable(&font);
		}
		xbox_assert(fCurFont);
#if VERSIONWIN
		fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fCurFont->IsTrueTypeFont();
#else
		fUseFontTrueTypeOnly = true;
#endif
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
		fCurOverhangLeft = 0.0f;
		fCurOverhangRight = 0.0f;
		fCurOverhangTop = 0.0f;
		fCurOverhangBottom = 0.0f;
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
		_BeginUsingStyles();
		fGC->_UpdateTextLayout( this);
		_EndUsingStyles();

		if (fTextBox)
			fLayoutIsValid = true;
#if ENABLE_D2D
		else if (fLayoutD2D)
			fLayoutIsValid = true;
#endif
		else
			fLayoutIsValid = false;

		fCharBackgroundColorDirty = true;
	}
}

/** return true if layout styles uses truetype font only */
bool VTextLayout::_StylesUseFontTrueTypeOnly() const
{
#if VERSIONMAC
	return true;
#else
#if ENABLE_D2D
	if (fTextLength > VTEXTLAYOUT_DIRECT2D_MAX_TEXT_SIZE)
		return false;
#endif
	bool useTrueTypeOnly;
	if (fStyles)
		useTrueTypeOnly = VGraphicContext::UseFontTrueTypeOnlyRec( fStyles);
	else
		useTrueTypeOnly = true;
	if (fExtStyles && fStylesUseFontTrueTypeOnly)
		useTrueTypeOnly = useTrueTypeOnly && VGraphicContext::UseFontTrueTypeOnlyRec( fExtStyles);
	return useTrueTypeOnly;
#endif
}

/** return true if layout uses truetype font only */
bool VTextLayout::UseFontTrueTypeOnly(VGraphicContext *inGC) const
{
#if VERSIONMAC
	return true;
#else
	//FIXME: for now we fallback to GDI impl if text is too big because Direct2D does not allow for now direct layout update
#if ENABLE_D2D
	if (fTextLength > VTEXTLAYOUT_DIRECT2D_MAX_TEXT_SIZE)
		return false;
#endif

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
#endif
}

/** insert text at the specified position */
void VTextLayout::InsertText( sLONG inPos, const VString& inText)
{
	if (!inText.GetLength())
		return;

	if (fStyles)
	{
		fStyles->AdjustPos( inPos, false);

		if (fStyles->HasSpanRefs() && inPos > 0 && fLayoutIsValid && fTextBox && fStyles->IsOverSpanRef( inPos-1))
			fLayoutIsValid = false;
	}

	VTaskLock protect(&fMutex);
	xbox_assert(fTextPtr);
	sLONG lengthPrev = fTextLength;
	sLONG lengthPrevImpl = fTextPtr->GetLength();
#if VERSIONMAC
	bool needAddWhitespace = fTextPtr->GetLength() == fTextLength; //if not equal, a ending whitespace has been added yet to the impl layout
#endif
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
		fTextPtr->Insert( inText, inPos+1);
		if (fStyles)
			fStyles->ExpandAtPosBy( inPos, inText.GetLength());
		if (fExtStyles)
			fExtStyles->ExpandAtPosBy( inPos, inText.GetLength());
	}
	fTextLength = fTextPtr->GetLength();
#if VERSIONMAC
	bool addWhitespace = false;
	if (fTextPtr->GetUniChar(fTextLength) == 13)
	{
		//if last character is a CR, we need to append some dummy whitespace character otherwise 
		//we cannot get correct metrics after last CR (CoreText bug)
		//(we use fTextLength to know the actual text length & stay consistent)
		if (fTextPtr != &fText)
			fText.FromString(*fTextPtr);
		fText.AppendChar(' ');
		fTextPtr = &fText;
		addWhitespace = true;
	}
#endif

#if VERSIONWIN
	bool useFontTrueTypeOnly = fStylesUseFontTrueTypeOnly;
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	if (fStylesUseFontTrueTypeOnly != useFontTrueTypeOnly)
		fLayoutIsValid = false;
#else
	fStylesUseFontTrueTypeOnly = true;
#endif

	if (fLayoutIsValid && fTextBox)
	{
#if VERSIONWIN
		//FIXME: if we insert a single CR with RichTextEdit, we need to compute all layout otherwise RTE seems to not take account 
		//CR and screws up styles too (if inText contains other characters but CR it is ok: quite annoying...)
		if ((inText.GetLength() == 1 && inText.GetUniChar(1) == 13)
			||
			(inPos > 0 && (*fTextPtr)[inPos-1] == 13)
			)
			fLayoutIsValid = false;
		else
		{
#endif
			//direct update impl layout
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->InsertText( inPos, inText);
			
			if (lengthPrev == 0)
			{
#if VERSIONMAC				
				needAddWhitespace = true;
#endif
				fTextBox->DeleteText(fTextLength, fTextLength+1); //remove dummy "x" for empty text
			}
			
#if VERSIONMAC
			if (addWhitespace && needAddWhitespace)
				fTextBox->InsertText( fTextLength, VString(" "));
#endif
			fNeedUpdateBounds = true;
#if VERSIONWIN
		}
#endif
	}
	else
		//layout direct update not supported are not implemented: compute all layout
		//FIXME: Direct2D layout cannot be directly updated so for now caller should backfall to GDI if text is big to prevent annoying perf issues
		fLayoutIsValid = false;
	
	fCharBackgroundColorDirty = true;
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

		if (fStyles->HasSpanRefs() && inStart > 0 && fLayoutIsValid && fTextBox && fStyles->IsOverSpanRef( inStart-1))
			fLayoutIsValid = false;
	}
	xbox_assert(fTextPtr);
	sLONG lengthPrev = fTextLength;
	sLONG lengthPrevImpl = fTextPtr->GetLength();
#if VERSIONMAC
	bool needAddWhitespace = fTextPtr->GetLength() == fTextLength; //if not equal, a ending whitespace has been added yet to the impl layout
#endif
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
	{
		if (inEnd > inStart)
			fStyles->Truncate( inStart, inEnd-inStart);
		if (inText.GetLength() > 0)
			fStyles->ExpandAtPosBy( inStart, inText.GetLength());
	}
	if (fExtStyles)
	{
		if (inEnd > inStart)
			fExtStyles->Truncate( inStart, inEnd-inStart);
		if (inText.GetLength() > 0)
			fExtStyles->ExpandAtPosBy( inStart, inText.GetLength());
	}
	
	if (inEnd > inStart)
		fTextPtr->Replace(inText, inStart+1, inEnd-inStart);
	else 
		fTextPtr->Insert(inText, inStart+1);
	
	fTextLength = fTextPtr->GetLength();
#if VERSIONMAC	
	bool addWhitespace = false;
#endif
	if (fTextLength == 0)
	{
		//if text is empty, we need to set layout text to a dummy "x" text otherwise 
		//we cannot get correct metrics for caret or for text layout bounds  
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromCString("x");
		fTextPtr = &fText;
	}
#if VERSIONMAC
	else
	{
		if (fTextPtr->GetUniChar(fTextLength) == 13)
		{
			//if last character is a CR, we need to append some dummy whitespace character otherwise 
			//we cannot get correct metrics after last CR (CoreText bug)
			//(we use fTextLength to know the actual text length & stay consistent)
			if (fTextPtr != &fText)
				fText.FromString(*fTextPtr);
			fText.AppendChar(' ');
			fTextPtr = &fText;
			addWhitespace = true;
		}
	}
#endif
	
#if VERSIONWIN
	bool useFontTrueTypeOnly = fStylesUseFontTrueTypeOnly;
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	if (fStylesUseFontTrueTypeOnly != useFontTrueTypeOnly)
		fLayoutIsValid = false;
#else
	fStylesUseFontTrueTypeOnly = true;
#endif
	
	if (fLayoutIsValid && fTextBox)
	{
#if VERSIONWIN
		//FIXME: if we insert a single CR with RichTextEdit, we need to compute all layout otherwise RTE seems to not take account 
		//CR and screws up styles too (if inText contains other characters but CR it is ok: quite annoying...)
		if (inText.GetLength() == 1 && inText.GetUniChar(1) == 13)
			fLayoutIsValid = false;
		else
		{
#endif
			//direct update impl layout
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->ReplaceText(inStart, inEnd, inText);
			
			if (lengthPrev == 0 && fTextLength > 0)
			{
#if VERSIONMAC				
				needAddWhitespace = true;
#endif
				fTextBox->DeleteText(fTextLength, fTextLength+1); //remove dummy "x" for empty text
			}
			
			if (fTextLength == 0 && lengthPrev > 0)
				//add dummy "x" for empty text
				fTextBox->InsertText(0, CVSTR("x"));
#if VERSIONMAC
			if (addWhitespace && needAddWhitespace)
				fTextBox->InsertText( fTextLength, VString(" "));
#endif
			fNeedUpdateBounds = true;
#if VERSIONWIN
		}
#endif
	}
	else
		//layout direct update not supported are not implemented: compute all layout
		//FIXME: Direct2D layout cannot be directly updated so for now caller should backfall to GDI if text is big to prevent perf issues
		fLayoutIsValid = false;
	
	fCharBackgroundColorDirty = true;
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
	sLONG lengthPrevImpl = fTextPtr->GetLength();
#if VERSIONMAC
	bool needAddWhitespace = fTextPtr->GetLength() == fTextLength; //if not equal, a ending whitespace has been added yet to the impl layout
#endif
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
	if (fExtStyles)
		fExtStyles->Truncate(inStart, inEnd-inStart);
	fTextPtr->Remove( inStart+1, inEnd-inStart);
	fTextLength = fTextPtr->GetLength();
#if VERSIONMAC	
	bool addWhitespace = false;
#endif
	if (fTextLength == 0)
	{
		//if text is empty, we need to set layout text to a dummy "x" text otherwise 
		//we cannot get correct metrics for caret or for text layout bounds  
		//(we use fTextLength to know the actual text length & stay consistent)
		fText.FromCString("x");
		fTextPtr = &fText;
	}
#if VERSIONMAC
	else
	{
		if (fTextPtr->GetUniChar(fTextLength) == 13)
		{
			//if last character is a CR, we need to append some dummy whitespace character otherwise 
			//we cannot get correct metrics after last CR (CoreText bug)
			//(we use fTextLength to know the actual text length & stay consistent)
			if (fTextPtr != &fText)
				fText.FromString(*fTextPtr);
			fText.AppendChar(' ');
			fTextPtr = &fText;
			addWhitespace = true;
		}
	}
#endif

#if VERSIONWIN
	bool useFontTrueTypeOnly = fStylesUseFontTrueTypeOnly;
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	if (fStylesUseFontTrueTypeOnly != useFontTrueTypeOnly)
		fLayoutIsValid = false;
#else
	fStylesUseFontTrueTypeOnly = true;
#endif

	if (fLayoutIsValid && fTextBox)
	{
		//direct update impl layout
		fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
		fTextBox->DeleteText( inStart, inEnd);
		
		if (fTextLength == 0 && lengthPrev > 0)
			//add dummy "x" for empty text
			fTextBox->InsertText(0, CVSTR("x"));
		
#if VERSIONMAC
		if (addWhitespace && needAddWhitespace)
			fTextBox->InsertText( fTextLength, VString(" "));
#endif
		fNeedUpdateBounds = true;
	}
	else
		//layout direct update not supported are not implemented: compute all layout
		//FIXME: Direct2D layout cannot be directly updated so for now caller should backfall to GDI if text is big to prevent perf issues
		fLayoutIsValid = false;
	
	fCharBackgroundColorDirty = true;
}


/** set min line height on the specified range 
 @remarks
	it is used while editing to prevent text wobble while editing with IME
 */
void VTextLayout::SetMinLineHeight(const GReal inMinHeight, sLONG inStart, sLONG inEnd)
{
#if VERSIONMAC	
	if (fLayoutIsValid && fTextBox)
	{
		fTextBox->SetMinLineHeight(inMinHeight, inStart, inEnd);
		fNeedUpdateBounds = true;
	}
#endif
}


VTreeTextStyle *VTextLayout::_RetainDefaultTreeTextStyle() const
{
	if (!fDefaultStyle)
	{
		VTextStyle *style = testAssert(fDefaultFont) ? fDefaultFont->CreateTextStyle() : new VTextStyle();

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
#if VERSIONWIN
		if (!inStyles->GetData()->GetFontName().IsEmpty())
			fLayoutIsValid = false;
#endif
		if (fLayoutIsValid && fTextBox && inFast)
		{
			//direct update impl layout
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->ApplyStyle( inStyles->GetData());
			fNeedUpdateBounds = true;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			//FIXME: Direct2D layout cannot be directly updated so for now caller should backfall to GDI if text is big to prevent perf issues
			fLayoutIsValid = false;
		fCharBackgroundColorDirty = true;
	}
	
	VIndex count = inStyles->GetChildCount();
	for (int i = 1; i <= count; i++)
		update = _ApplyStylesRec( inStyles->GetNthChild(i)) || update;
	
	return update;
}


/** apply style (use style range) */
bool VTextLayout::ApplyStyle( const VTextStyle* inStyle, bool inFast)
{
	VTaskLock protect(&fMutex);
	if (_ApplyStyle( inStyle))
	{
		if (inStyle->IsSpanRef())
			fLayoutIsValid = false;
#if VERSIONWIN
		if (!inStyle->GetFontName().IsEmpty())
			fLayoutIsValid = false;
#endif

		if (fLayoutIsValid && fTextBox && inFast)
		{
			//direct update impl layout
			fTextBox->SetDrawContext(NULL); //reset drawing context (it is not used for layout update)
			fTextBox->ApplyStyle( inStyle);
			fNeedUpdateBounds = true;
		}
		else
			//layout direct update not supported are not implemented: compute all layout
			//FIXME: Direct2D layout cannot be directly updated so for now caller should backfall to GDI if text is big to prevent perf issues
			fLayoutIsValid = false;
		fCharBackgroundColorDirty = true;
		return true;
	}
	return false;
}



bool  VTextLayout::_ApplyStyle(const VTextStyle *inStyle)
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

			fStyles->ApplyStyle( inStyle); //we apply only on fStyles

			_ReleaseFullTreeTextStyle( styles); //detach fStyles from fDefaultStyle

			if (fStyles->GetChildCount() == 0 && fStyles->GetData()->IsUndefined())
				ReleaseRefCountable(&fStyles);
		}
		else
		{
			fStyles = new VTreeTextStyle( new VTextStyle());
			fStyles->GetData()->SetRange(0, fTextLength);

			VTreeTextStyle *styles = _RetainFullTreeTextStyle(); //to ensure fStyles will use fDefaultStyle as parent 
																 //(so only styles which are different from default uniform style will be overriden)
			fStyles->ApplyStyle( inStyle); //we apply only on fStyles

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




