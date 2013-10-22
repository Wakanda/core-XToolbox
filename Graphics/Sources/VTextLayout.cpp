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

#undef min
#undef max


/**	VTextLayout constructor
@param inPara
	VDocParagraph source: the passed paragraph is used only for initializing control plain text, styles & paragraph properties
						  but inPara is not retained 
	(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
@param inShouldEnableCache
	enable/disable cache (default is true): see ShouldEnableCache
*/
VTextLayout::VTextLayout(const VDocParagraph *inPara, bool inShouldEnableCache, bool inShouldUseNativePlatformForParagraphLayout)
{
	xbox_assert(inPara);
	_Init();
	fShouldEnableCache = inShouldEnableCache;
	fShouldUseNativePlatformForParagraphLayout = inShouldUseNativePlatformForParagraphLayout;
	if (!fShouldUseNativePlatformForParagraphLayout)
		fParagraphLayout = new VParagraphLayout(this);

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

@param inShouldUseNativePlatformForParagraphLayout
	true (default): compute paragraph layout with native impl (e.g. GDI/RTE/D2D on Windows, CoreText on Mac)
	false: compute paragraph layout with platform independent impl (using ICU & platform independent algorithm) 
		   only layout basic runs (same direction & style) are still computed & rendered with native impl (GDI/D2D on Windows, CoreText on Mac)
@note
	important: complex paragraph features like embedded objects, bullets and so on are supported only with inShouldUseNativePlatformForParagraphLayout = false 
			   (in V15, it will be used to render new 4D XHTML document format) 
*/
void VTextLayout::ShouldUseNativePlatformForParagraphLayout(bool inShouldUseNativePlatformForParagraphLayout) 
{
	VTaskLock protect(&fMutex);
	if (fShouldUseNativePlatformForParagraphLayout == inShouldUseNativePlatformForParagraphLayout)
		return;
	fShouldUseNativePlatformForParagraphLayout = inShouldUseNativePlatformForParagraphLayout;
	if (!fShouldUseNativePlatformForParagraphLayout)
	{
		fParagraphLayout = new VParagraphLayout( this);
		
		//release native layout if any
		ReleaseRefCountable(&fTextBox);
#if ENABLE_D2D
		if (fLayoutD2D)
		{
			fLayoutD2D->Release();
			fLayoutD2D = NULL;
		}
#endif
	}
	else
	{
		if (fParagraphLayout)
			delete fParagraphLayout;
		fParagraphLayout = NULL;
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

	if (ShouldUseNativePlatformForParagraphLayout())
		return para;

	//following properties are supported only by new layout impl

	if (fLineHeight != -1)
	{
		if (fLineHeight < 0)
			para->SetLineHeight( (sLONG)-floor((-fLineHeight)*100+0.5)); //percentage
		else
			para->SetLineHeight( ICSSUtil::PointToTWIPS( fLineHeight)); //absolute line height
	}

	if (fPaddingFirstLine != 0)
	{
		if (fPaddingFirstLine >= 0)
			para->SetPaddingFirstLine( ICSSUtil::PointToTWIPS( fPaddingFirstLine)); 
		else
			para->SetPaddingFirstLine( -ICSSUtil::PointToTWIPS( -fPaddingFirstLine));
	}

	uLONG tabStopOffsetTWIPS = (uLONG)ICSSUtil::PointToTWIPS( fTabStopOffset);
	if (tabStopOffsetTWIPS != (uLONG)para->GetDefaultPropValue( kDOC_PROP_TAB_STOP_OFFSET))
		para->SetTabStopOffset( tabStopOffsetTWIPS);
	if (fTabStopType != TTST_LEFT)
		para->SetTabStopType( (eDocPropTabStopType)fTabStopType);

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
		SetBackColor( xcolor, fPaintBackColor);
	}
	
	if (ShouldUseNativePlatformForParagraphLayout())
		return;

	//following properties are supported only by new layout impl

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_DIRECTION) && !inPara->IsInherited( kDOC_PROP_DIRECTION)))
	{
		if (inPara->GetDirection() == kTEXT_DIRECTION_RTL)
			SetRTL( true);
		else
			SetRTL( false);
	}

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_LINE_HEIGHT) && !inPara->IsInherited( kDOC_PROP_LINE_HEIGHT)))
	{
		GReal lineHeight;
		if (inPara->GetLineHeight() >= 0)
			lineHeight = ICSSUtil::TWIPSToPoint(inPara->GetLineHeight());
		else if (inPara->GetLineHeight() == kDOC_PROP_LINE_HEIGHT_NORMAL)
			lineHeight = -1;
		else
		{
			lineHeight = -inPara->GetLineHeight();
			lineHeight = -ICSSUtil::RoundTWIPS(lineHeight*0.01); //percentage/100 multiple of 0.05
		}
		SetLineHeight( lineHeight);
	}

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_PADDING_FIRST_LINE) && !inPara->IsInherited( kDOC_PROP_PADDING_FIRST_LINE)))
	{
		if (inPara->GetPaddingFirstLine() >= 0)
			SetPaddingFirstLine( ICSSUtil::TWIPSToPoint(inPara->GetPaddingFirstLine()));
		else
			SetPaddingFirstLine( -ICSSUtil::TWIPSToPoint(-inPara->GetPaddingFirstLine()));
	}

	bool updateTabStop = false;
	GReal offset = fTabStopOffset;
	eTextTabStopType type = fTabStopType;

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_TAB_STOP_OFFSET) && !inPara->IsInherited( kDOC_PROP_TAB_STOP_OFFSET)))
	{
		updateTabStop = true;
		offset = ICSSUtil::TWIPSToPoint(inPara->GetTabStopOffset());
	}
	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_TAB_STOP_TYPE) && !inPara->IsInherited( kDOC_PROP_TAB_STOP_TYPE)))
	{
		updateTabStop = true;
		type = (eTextTabStopType)inPara->GetTabStopType();
	}
	if (updateTabStop)
		SetTabStop( offset, type);
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


VTextLayout::VTextLayout(bool inShouldEnableCache, bool inShouldUseNativePlatformForParagraphLayout)
{
	_Init();
	fShouldEnableCache = inShouldEnableCache;
	fShouldUseNativePlatformForParagraphLayout = inShouldUseNativePlatformForParagraphLayout;
	if (!fShouldUseNativePlatformForParagraphLayout)
		fParagraphLayout = new VParagraphLayout(this);

	ResetParagraphStyle();
}

void VTextLayout::_Init()
{
	fGC = NULL;
	fLayoutIsValid = false;
	fGCUseCount = 0;
	fCurGCType = eDefaultGraphicContext; //means same as undefined here
	fShouldEnableCache = false;
	fShouldUseNativePlatformForParagraphLayout = true;
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
	fParagraphLayout = NULL;
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
	if (fPaintBackColor && fBackColor.GetAlpha())
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

	if (fParagraphLayout)
		//platform-independent layout: VParagraphLayout::Draw paints back color
		fShouldDrawCharBackColor = false;
	else
		//native layout: depending on gc, background color might be drawed natively or by VTextLayout
		fShouldDrawCharBackColor = !fGC->CanDrawStyledTextBackColor();

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
		return;
	}

	xbox_assert(fLayerOffScreen == NULL);
	fLayerOffScreen = fGC->EndLayerOffScreen();
	fLayerIsDirty = fLayerOffScreen == NULL;
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
	fLayoutIsValid = false;
	fDPI = inDPI;

	sDocPropPaddingMargin margin;
	margin.left			= fMarginLeftTWIPS;
	margin.right		= fMarginRightTWIPS;
	margin.top			= fMarginTopTWIPS;
	margin.bottom		= fMarginBottomTWIPS;
	SetMargins( margin);
}


/** get margins (in TWIPS) */
void VTextLayout::GetMargins( sDocPropPaddingMargin& outMargins)
{
    outMargins.left     = fMarginLeftTWIPS;
    outMargins.right    = fMarginRightTWIPS;
    outMargins.top      = fMarginTopTWIPS;
    outMargins.bottom   = fMarginBottomTWIPS;
}

GReal VTextLayout::_RoundMetrics( VGraphicContext *inGC, GReal inValue)
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
		//context does not align glyphs on pixel boundaries:
		//round to TWIPS precision (so result is multiple of 0.05) (it is as if metrics are computed at 72*20 = 1440 DPI which should achieve enough precision)
		if (inValue >= 0)
			return ICSSUtil::RoundTWIPS( inValue);
		else
			return -ICSSUtil::RoundTWIPS( -inValue);
	}
}

/** get margins (in TWIPS) */
void VTextLayout::SetMargins( const sDocPropPaddingMargin& inMargins)
{
	fMarginLeftTWIPS	= inMargins.left;
	fMarginRightTWIPS	= inMargins.right;
	fMarginTopTWIPS		= inMargins.top;
	fMarginBottomTWIPS	= inMargins.bottom;
    
	fMarginLeft		= _RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginLeftTWIPS) * fDPI/72);
	fMarginRight	= _RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginRightTWIPS) * fDPI/72);
	fMarginTop		= _RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginTopTWIPS) * fDPI/72);
	fMarginBottom	= _RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginBottomTWIPS) * fDPI/72);
    
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

	sDocPropPaddingMargin margin;
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
			fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fCurFont->IsTrueTypeFont();
			if (fParagraphLayout)
				//platform-independent impl
				fParagraphLayout->UpdateLayout();
			else
				//native impl
				fGC->_UpdateTextLayout( this);
			fNeedUpdateBounds = false;
		}
	}
	else
	{
		ReleaseRefCountable(&fTextBox);

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
		if (fParagraphLayout)
		{
			//platform-independent impl
			fParagraphLayout->UpdateLayout();
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


VTextLayout::VParagraphLayout::Run::Run(Line *inLine, const sLONG inIndex, const sLONG inStart, const sLONG inEnd, const eDirection inDirection, VFont *inFont, const VColor *inTextColor, const VColor *inBackColor)
{
	fLine = inLine;
	fIndex = inIndex;
	fStart = inStart;
	fEnd = inEnd;
	fDirection = inDirection;
	fFont = RetainRefCountable( inFont);
	fAscent = fDescent = 0;
	if (inTextColor)
		fTextColor = *inTextColor;
	else
		fTextColor.SetAlpha(0);
	if (inBackColor)
		fBackColor = *inBackColor;
	else
		fBackColor.SetAlpha(0);
}

VTextLayout::VParagraphLayout::Run::Run(const Run& inRun)
{
	fLine = inRun.fLine;
	fIndex = inRun.fIndex;
	fStart = inRun.fStart;
	fEnd = inRun.fEnd;
	fDirection = inRun.fDirection;
	fFont = NULL;
	CopyRefCountable(&fFont, inRun.fFont);
	fStartPos = inRun.fStartPos;
	fEndPos = inRun.fEndPos;
	fAscent = inRun.fAscent;
	fDescent = inRun.fDescent;
	fTypoBounds = inRun.fTypoBounds;
	fHitBounds = inRun.fHitBounds;
	fTypoBoundsWithTrailingSpaces = inRun.fTypoBoundsWithTrailingSpaces;
	fTextColor = inRun.fTextColor;
	fBackColor = inRun.fBackColor;
}

VTextLayout::VParagraphLayout::Run& VTextLayout::VParagraphLayout::Run::operator=(const Run& inRun)
{
	if (this == &inRun)
		return *this;

	fLine = inRun.fLine;
	fIndex = inRun.fIndex;
	fStart = inRun.fStart;
	fEnd = inRun.fEnd;
	fDirection = inRun.fDirection;
	CopyRefCountable(&fFont, inRun.fFont);
	fStartPos = inRun.fStartPos;
	fEndPos = inRun.fEndPos;
	fAscent = inRun.fAscent;
	fDescent = inRun.fDescent;
	fTypoBounds = inRun.fTypoBounds;
	fHitBounds = inRun.fHitBounds;
	fTypoBoundsWithTrailingSpaces = inRun.fTypoBoundsWithTrailingSpaces;
	fTextColor = inRun.fTextColor;
	fBackColor = inRun.fBackColor;

	return *this;
}

bool VTextLayout::VParagraphLayout::Run::IsTab() const
{
	if (fEnd-fStart != 1)
		return false;

	VString *textPtr = fLine->fParagraph->fParaLayout->fLayout->fTextPtr;
	return ((*textPtr)[fStart] == '\t');
}


VGraphicContext* VTextLayout::VParagraphLayout::_RetainComputingGC( VGraphicContext *inGC)
{
	xbox_assert(inGC);
#if VERSIONWIN
	//with GDI, measures depend on ctm & screen dpi so ensure we use computing GDI gc as we compute measures in point & with screen DPI 
	//if (inGC->IsGDIImpl() 
	//	|| 
	//	(inGC->GetTextRenderingMode() & TRM_LEGACY_ON)) 
	//	return VGraphicContext::CreateForComputingMetrics( eWinGDIGraphicContext);
	//else
#endif
		return RetainRefCountable( inGC);
}

const VTextLayout::VParagraphLayout::CharOffsets& VTextLayout::VParagraphLayout::Run::GetCharOffsets()
{
	if (fCharOffsets.size() != fEnd-fStart)
	{
		if (fCharOffsets.size() > fEnd-fStart)
		{
			fCharOffsets.resize( fEnd-fStart);
			return fCharOffsets;
		}
		
		bool needReleaseGC = false; 
		VGraphicContext *gc = fLine->fParagraph->fParaLayout->fComputingGC;
		if (!gc)
		{
			//might happen if called later while requesting run or caret metrics 
			gc = fLine->fParagraph->fParaLayout->_RetainComputingGC( fLine->fParagraph->fParaLayout->fLayout->fGC);
			gc->BeginUsingContext(true);
			needReleaseGC = true;
		}

		xbox_assert(gc && fFont);

		VFontMetrics *metrics = new VFontMetrics( fFont, gc);

		VString runtext;
		VString *textPtr = fLine->fParagraph->fParaLayout->fLayout->fTextPtr;
		textPtr->GetSubString( fStart+1, fEnd-fStart, runtext);

		//replace CR by whitespace to ensure correct offset for end of line character 
		if (runtext.GetUniChar(runtext.GetLength()) == 13)
			runtext.SetUniChar(runtext.GetLength(), ' ');

		fCharOffsets.reserve(runtext.GetLength());
		fCharOffsets.clear();

		metrics->MeasureText( runtext, fCharOffsets);	//TODO: check in all impls that MeasureText returned offsets take account pair kerning between adjacent characters, otherwise modify method (with a option)
														// returned offsets should be also absolute offsets from start position & not from left border (for RTL)
		while (fCharOffsets.size() < fEnd-fStart)
			fCharOffsets.push_back(fCharOffsets.back()); 
		
		CharOffsets::iterator itOffset = fCharOffsets.begin();
		for (;itOffset != fCharOffsets.end(); itOffset++)
			*itOffset = VTextLayout::_RoundMetrics( gc, *itOffset);
		delete metrics;

		if (needReleaseGC)
		{
			gc->EndUsingContext();
			gc->Release();
		}
	}
	return fCharOffsets;
}

/** first pass: compute only run start & end positions (it is used only by VParagraph for run length) */
void VTextLayout::VParagraphLayout::Run::PreComputeMetrics(bool inIgnoreTrailingSpaces)
{
	xbox_assert(!inIgnoreTrailingSpaces || fLine->fParagraph->fMaxWidth > 0); //we should ignore trailing spaces if and only if word-wrap is enabled 
																			  //(otherwise trailing spaces need to be taken account for layout width)

	VGraphicContext *gc = fLine->fParagraph->fParaLayout->fComputingGC;
	xbox_assert(gc && fFont);

	VFontMetrics *metrics = new VFontMetrics( fFont, gc);
	//ensure rounded ascent & ascent+descent is greater or equal to design ascent & ascent+descent
	//(as we use ascent to compute line box y origin from y baseline & ascent+descent to compute line box height)
	fAscent = VTextLayout::_RoundMetrics(gc, metrics->GetAscent());
	fDescent = VTextLayout::_RoundMetrics(gc, metrics->GetAscent()+metrics->GetDescent())-fAscent;
	xbox_assert(fDescent >= 0);
	
	VString *textPtr = fLine->fParagraph->fParaLayout->fLayout->fTextPtr;
	sLONG paraEnd = fLine->fParagraph->fEnd;
	bool endOfLine = fEnd >= paraEnd;

	VString runtext;
	if (fEnd > fStart)
	{
		textPtr->GetSubString( fStart+1, fEnd-fStart, runtext);

		//replace CR by whitespace to ensure correct width for end of line character (as it seems to be 0 otherwise with some platform impl)
		if (runtext.GetUniChar(runtext.GetLength()) == 13)
		{
			runtext.SetUniChar(runtext.GetLength(), ' ');
			endOfLine = true;
		}
	}
	bool isTab = runtext.GetLength() == 1 && runtext[0] == '\t';
	if (isTab)
		inIgnoreTrailingSpaces = false;

	if (inIgnoreTrailingSpaces && !runtext.IsEmpty())
	{
		eDirection paraDirection = fLine->fParagraph->fDirection;
		if (paraDirection == fDirection)
		{
			//trim trailing spaces
			const UniChar *c = runtext.GetCPointer() + runtext.GetLength();
			while ((c-1) >= runtext.GetCPointer() && VIntlMgr::GetDefaultMgr()->IsSpace( *(c-1)))
				c--;
			VIndex newLength = c-runtext.GetCPointer();
			if (newLength < runtext.GetLength())
				runtext.Truncate(newLength);
		}
		else
		{
			//trim leading spaces
			const UniChar *c = runtext.GetCPointer();
			while (VIntlMgr::GetDefaultMgr()->IsSpace( *c))
				c++;
			VIndex numChar = c-runtext.GetCPointer();
			if (numChar)
				runtext.Remove(1, numChar);
		}
	}

	GReal width;
	if (isTab)
	{
		//compute absolute tab decal 
		GReal start = fStartPos.GetX();
		GReal end = start;
		fLine->fParagraph->_ApplyTab( end, fFont); 
		width = std::abs(end-start);
	}
	else if (fEnd > fStart)
		width = ceil(metrics->GetTextWidth( runtext)); //typo width
	else
		width = 0;

	GReal widthWithKerning;
	bool isLastRun = fIndex+1 >= fLine->fRuns.size();
	const Run *nextRun = NULL;
	if (!isLastRun)
		nextRun = fLine->fRuns[fIndex+1];
	sLONG runTextLength = runtext.GetLength();
	if (!endOfLine && !isTab && !inIgnoreTrailingSpaces && fEnd > fStart && (isLastRun || nextRun->GetDirection() == fDirection))
	{
		//next run has the same direction -> append next run first char (beware of surrogates so 2 code points if possible) to run text in order to determine next run start offset = this run end offset & take account kerning
		
		//add next two code points (for possible surrogates)
		VString nextRunText;
		if (fEnd+2 <= paraEnd)
			textPtr->GetSubString( fEnd+1, 2, nextRunText);
		else
			textPtr->GetSubString( fEnd+1, 1, nextRunText);
		runtext.AppendString(nextRunText);

		fCharOffsets.reserve(runtext.GetLength());
		fCharOffsets.clear();
		metrics->MeasureText( runtext, fCharOffsets);	//TODO: check in all impls that MeasureText returned offsets take account pair kerning between adjacent characters, otherwise modify method (with a option)
														// returned offsets should be also absolute offsets from start position & not from left border (for RTL)

		CharOffsets::iterator itOffset = fCharOffsets.begin();
		for (;itOffset != fCharOffsets.end(); itOffset++)
			*itOffset = VTextLayout::_RoundMetrics( gc, *itOffset); //width with kerning so round to nearest integral (GDI) or TWIPS boundary (other sub-pixel positionning capable context)

		widthWithKerning = fCharOffsets[runTextLength-1]; 
		xbox_assert(fCharOffsets.size() == runtext.GetLength());
	}
	else
	{
		//next run has not the same direction or it is end of line or tab: ignore kerning between runs -> width with kerning = typo width
		//(as next run first logical char is not adjacent to this run last logical char)
		widthWithKerning = width;
	}
	if (inIgnoreTrailingSpaces)
		fTypoBounds.SetWidth( width);
	else
	{
		fTypoBoundsWithTrailingSpaces.SetWidth(width);
		if (fTypoBounds.GetWidth() == 0.0f)
			fTypoBounds.SetWidth( width);
	}

	fEndPos = fStartPos;
	fEndPos.x += widthWithKerning;

	fHitBounds.SetWidth( widthWithKerning);

	delete metrics;
}



/** compute final metrics according to bidi */
void VTextLayout::VParagraphLayout::Run::ComputeMetrics(GReal& inBidiCurDirStartOffset, GReal& inBidiCurDirWidth, sLONG& inBidiCurDirStartIndex)
{
	eDirection paraDirection = fLine->fParagraph->fDirection;
	bool isLastRun = fIndex == fLine->fRuns.size()-1;
	bool wordwrap = fLine->fParagraph->fMaxWidth != 0;

	if (fLine->fParagraph->fDirections.empty())
	{
		//only one direction
		
		if (paraDirection == kPARA_RTL)
		{
			//just neg start & end x position
			fStartPos.x = -fStartPos.x;
			fEndPos.x = -fEndPos.x;

			fHitBounds.SetCoords( fEndPos.x, -fAscent, fHitBounds.GetWidth(), fAscent+fDescent);
			fTypoBoundsWithTrailingSpaces.SetCoords( fStartPos.x-fTypoBoundsWithTrailingSpaces.GetWidth(), -fAscent, fTypoBoundsWithTrailingSpaces.GetWidth(), fAscent+fDescent);
			if (isLastRun && wordwrap)
				fTypoBounds.SetCoords( fStartPos.x-fTypoBounds.GetWidth(), -fAscent, fTypoBounds.GetWidth(), fAscent+fDescent);
			else
				fTypoBounds = fTypoBoundsWithTrailingSpaces;
		}
		else
		{
			fHitBounds.SetCoords( fStartPos.x, -fAscent, fHitBounds.GetWidth(), fAscent+fDescent);
			fTypoBoundsWithTrailingSpaces.SetCoords( fStartPos.x, -fAscent, fTypoBoundsWithTrailingSpaces.GetWidth(), fAscent+fDescent);
			if (isLastRun && wordwrap)
				fTypoBounds.SetCoords( fStartPos.x, -fAscent, fTypoBounds.GetWidth(), fAscent+fDescent);
			else
				fTypoBounds = fTypoBoundsWithTrailingSpaces;
		}
	}
	else
	{
		//complex bidi text

		if (fIndex == 0)
		{
			inBidiCurDirStartOffset = 0;
			inBidiCurDirWidth = 0;
			inBidiCurDirStartIndex = fIndex;
		}

		eDirection nextRunDir = fIndex+1 < fLine->fRuns.size() ? (fLine->fRuns[fIndex+1])->fDirection : fDirection;
		if (nextRunDir != fDirection || fIndex+1 >= fLine->fRuns.size())
		{
			//last run with the same direction -> adjust runs & compute final metrics for runs from inBidiCurDirStartIndex to the current (the runs whith the same direction)

			if (wordwrap && fIndex == fLine->fRuns.size()-1)
				inBidiCurDirWidth += fTypoBounds.GetWidth();
			else
				inBidiCurDirWidth += fHitBounds.GetWidth();

			GReal nextRunStartOffset = inBidiCurDirStartOffset;

			if (paraDirection == kPARA_LTR)
			{
				if (fDirection == kPARA_RTL)
					inBidiCurDirStartOffset += inBidiCurDirWidth;

				nextRunStartOffset += inBidiCurDirWidth;
			}
			else
			{
				//(paraDirection == kPARA_RTL)

				if (fDirection == kPARA_LTR)
					inBidiCurDirStartOffset -= inBidiCurDirWidth;

				nextRunStartOffset -= inBidiCurDirWidth;
			}

			for (int i = inBidiCurDirStartIndex; i <= fIndex; i++)
			{
				Run* run = fLine->fRuns[i];
				isLastRun = i == fLine->fRuns.size()-1;

				if (fDirection == kPARA_LTR)
				{
					run->fStartPos.x = inBidiCurDirStartOffset;
					run->fEndPos.x = inBidiCurDirStartOffset+run->fHitBounds.GetWidth();
					
					run->fHitBounds.SetCoords( inBidiCurDirStartOffset, -fAscent, run->fHitBounds.GetWidth(), fAscent+fDescent);
					run->fTypoBoundsWithTrailingSpaces.SetCoords( inBidiCurDirStartOffset, -fAscent, run->fTypoBoundsWithTrailingSpaces.GetWidth(), fAscent+fDescent);
					if (isLastRun && wordwrap)
						run->fTypoBounds.SetCoords( inBidiCurDirStartOffset, -fAscent, run->fTypoBounds.GetWidth(), fAscent+fDescent);
					else
						run->fTypoBounds = run->fTypoBoundsWithTrailingSpaces;
										
					inBidiCurDirStartOffset = run->fEndPos.x;
				}
				else
				{
					run->fStartPos.x = inBidiCurDirStartOffset;
					run->fEndPos.x = inBidiCurDirStartOffset-run->fHitBounds.GetWidth();
					
					run->fHitBounds.SetCoords( inBidiCurDirStartOffset-run->fHitBounds.GetWidth(), -fAscent, run->fHitBounds.GetWidth(), fAscent+fDescent);
					run->fTypoBoundsWithTrailingSpaces.SetCoords( inBidiCurDirStartOffset-run->fTypoBoundsWithTrailingSpaces.GetWidth(), -fAscent, run->fTypoBoundsWithTrailingSpaces.GetWidth(), fAscent+fDescent);
					if (isLastRun && wordwrap)
						run->fTypoBounds.SetCoords( inBidiCurDirStartOffset-run->fTypoBounds.GetWidth(), -fAscent, run->fTypoBounds.GetWidth(), fAscent+fDescent);
					else
						run->fTypoBounds = run->fTypoBoundsWithTrailingSpaces;
										
					inBidiCurDirStartOffset = run->fEndPos.x;
				}
			}

			if (fIndex+1 < fLine->fRuns.size())
			{
				//prepare the next direction first run
				inBidiCurDirStartOffset = nextRunStartOffset;
				inBidiCurDirWidth = 0;
				inBidiCurDirStartIndex = fIndex+1;
			}
		}
		else
			inBidiCurDirWidth += fHitBounds.GetWidth();
	}
}

VTextLayout::VParagraphLayout::Line::Line(VParagraph *inParagraph, const sLONG inIndex, const sLONG inStart, const sLONG inEnd)
{
	fParagraph = inParagraph;
	fIndex = inIndex;
	fStart = inStart;
	fEnd = inEnd;
	fAscent = fDescent = fLineHeight = 0;
}
VTextLayout::VParagraphLayout::Line::Line(const Line& inLine)
{
	fParagraph = inLine.fParagraph;
	fIndex = inLine.fIndex;
	fStart = inLine.fStart;
	fEnd = inLine.fEnd;
	fStartPos = inLine.fStartPos;
	fRuns = inLine.fRuns;
	fAscent = inLine.fAscent;
	fDescent = inLine.fDescent;
	fLineHeight = inLine.fLineHeight;
	fTypoBounds = inLine.fTypoBounds;
	fTypoBoundsWithTrailingSpaces = inLine.fTypoBoundsWithTrailingSpaces;
}
VTextLayout::VParagraphLayout::Line& VTextLayout::VParagraphLayout::Line::operator=(const Line& inLine)
{
	if (this == &inLine)
		return *this;

	fParagraph = inLine.fParagraph;
	fIndex = inLine.fIndex;
	fStart = inLine.fStart;
	fEnd = inLine.fEnd;
	fStartPos = inLine.fStartPos;
	fRuns = inLine.fRuns;
	fAscent = inLine.fAscent;
	fDescent = inLine.fDescent;
	fLineHeight = inLine.fLineHeight;
	fTypoBounds = inLine.fTypoBounds;
	fTypoBoundsWithTrailingSpaces = inLine.fTypoBoundsWithTrailingSpaces;
	return *this;
}

VTextLayout::VParagraphLayout::Line::~Line()
{
	Runs::const_iterator itRun = fRuns.begin();
	for(;itRun != fRuns.end(); itRun++)
		delete (*itRun);
}

/** compute metrics according to start pos, start & end indexs & run info */
void VTextLayout::VParagraphLayout::Line::ComputeMetrics()
{
	//first compute run final metrics 

	VGraphicContext *gc = fParagraph->fParaLayout->fComputingGC;
	xbox_assert(gc);

	GReal bidiStartOffset = 0;
	GReal bidiWidth = 0;
	sLONG bidiRunIndex = 0;
	Runs::iterator itRunWrite = fRuns.begin();
	for (;itRunWrite != fRuns.end(); itRunWrite++)
		(*itRunWrite)->ComputeMetrics( bidiStartOffset, bidiWidth, bidiRunIndex);

	//then compute local metrics

	fAscent = fDescent = fLineHeight = 0.0f;
	fTypoBounds.SetEmpty();
	fTypoBoundsWithTrailingSpaces.SetEmpty();

	fEnd = fStart;
	Runs::const_iterator itRunRead = fRuns.begin();
	VIndex runSize = fRuns.size();
	sLONG runIndex = 0;
	for (;itRunRead != fRuns.end(); itRunRead++, runIndex++)
	{
		fAscent = std::max( fAscent, (*itRunRead)->GetAscent());
		fDescent = std::max( fDescent, (*itRunRead)->GetDescent());

		if (runIndex == runSize-1)
		{
			fTypoBoundsWithTrailingSpaces = fTypoBounds;
			if (fTypoBoundsWithTrailingSpaces.IsEmpty())
				fTypoBoundsWithTrailingSpaces = (*itRunRead)->GetTypoBounds(true);
			else
				fTypoBoundsWithTrailingSpaces.Union( (*itRunRead)->GetTypoBounds(true));

			if (fTypoBounds.IsEmpty())
				fTypoBounds = (*itRunRead)->GetTypoBounds(false);
			else
				fTypoBounds.Union( (*itRunRead)->GetTypoBounds(false));
		}
		else
			if (fTypoBounds.IsEmpty())
				fTypoBounds = (*itRunRead)->GetTypoBounds(true);
			else
				fTypoBounds.Union( (*itRunRead)->GetTypoBounds(true));
		
		fEnd = (*itRunRead)->GetEnd();
	}

	fLineHeight = fParagraph->fParaLayout->fLayout->GetLineHeight();
	if (fLineHeight >= 0)
		fLineHeight = VTextLayout::_RoundMetrics(gc, fLineHeight*fParagraph->fParaLayout->fLayout->fDPI/72);
	else
	{
		fLineHeight = (fAscent+fDescent)*(-fLineHeight);
		fLineHeight = VTextLayout::_RoundMetrics( gc, fLineHeight);
	}
	fTypoBounds.SetHeight( fLineHeight);
	fTypoBoundsWithTrailingSpaces.SetHeight( fLineHeight);
}


VTextLayout::VParagraphLayout::VParagraph::VParagraph(VTextLayout::VParagraphLayout *inParaLayout, const sLONG inIndex, const sLONG inStart, const sLONG inEnd):VObject(),IRefCountable()
{
	fParaLayout	= inParaLayout;
	fIndex	= inIndex;
	fStart	= inStart;
	fEnd	= inEnd;
	fMaxWidth = 0;
	fDirection = kPARA_LTR;
	fWordWrapOverflow = false;
	fDirty	= true;
}

VTextLayout::VParagraphLayout::VParagraph::~VParagraph()
{
	//free sequential uniform styles 
	Styles::iterator itStyle = fStyles.begin();
	for (;itStyle != fStyles.end(); itStyle++)
	{
		if (*itStyle)
			delete *itStyle;
	}
	fStyles.clear();

	Lines::const_iterator itLine = fLines.begin();
	for (;itLine != fLines.end(); itLine++)
		delete (*itLine);
}

void VTextLayout::VParagraphLayout::VParagraph::_ApplyTab( GReal& pos, VFont *font)
{
	//compute tab absolute pos using TWIPS 

	VGraphicContext *gc = fParaLayout->fComputingGC;
	xbox_assert(gc);

	xbox_assert(pos >= 0);
	sLONG abspos = ICSSUtil::PointToTWIPS(pos);
	sLONG taboffset = ICSSUtil::PointToTWIPS( fParaLayout->fLayout->GetTabStopOffset()*fParaLayout->fLayout->fDPI/72);
	if (taboffset)
	{
		//compute absolute tab pos
		sLONG div = abspos/taboffset;
		pos = VTextLayout::_RoundMetrics(gc, ICSSUtil::TWIPSToPoint( (div+1)*taboffset));  
	}
	else
	{
		//if tab offset is 0 -> tab width = whitespace width
		VGraphicContext *gc = fParaLayout->fComputingGC;
		xbox_assert(gc);
		VFontMetrics *metrics = new VFontMetrics( font, gc);
		pos += VTextLayout::_RoundMetrics(gc, metrics->GetCharWidth( ' '));
		delete metrics;
	}
}

void VTextLayout::VParagraphLayout::VParagraph::_NextRun( Run* &run, Line* line, eDirection dir, VFont* font, VColor textColor, VColor backColor, GReal& lineWidth, sLONG i, bool inIsTab)
{
	xbox_assert( font);

	if (inIsTab && run->GetEnd() > run->GetStart()+1)
	{
		//last char of current run is a tab char -> split in two runs, first one without last char & second one for absolute tab

		//terminate run: truncate without tab 
		run->SetEnd( i-1);
		
		//add run for the absolute tab
		run = new Run( line, run->GetIndex()+1, i-1, i, run->GetDirection(), run->GetFont(), &(run->GetTextColor()), &(run->GetBackColor()));
		line->thisRuns().push_back( run);
		
		Run *runPrev = line->thisRuns()[run->GetIndex()-1];
		runPrev->PreComputeMetrics(); //update previous run metrics 
		lineWidth = std::abs( runPrev->GetEndPos().GetX()); //get prev run last offset

		run->SetStartPos( VPoint( lineWidth, 0));
		run->PreComputeMetrics(); //calc absolute tab width
		lineWidth = std::abs( run->GetEndPos().GetX());
	}
	else
	{
		if (run->GetEnd() == run->GetStart())
		{
			//empty run: just update font, dir  & line width
			run->SetFont( font);
			run->SetDirection ( dir);
			run->SetTextColor( textColor);
			run->SetBackColor( backColor);
			lineWidth = std::abs( run->GetEndPos().GetX());
			return;
		}

		//terminate run
		run->SetEnd( i);
	}

	//now add new run
	if (i < fEnd)
	{
		run = new Run( line, run->GetIndex()+1, i, i, dir, font, &textColor, &backColor);
		line->thisRuns().push_back( run);

		Run *runPrev = line->thisRuns()[run->GetIndex()-1];
		runPrev->PreComputeMetrics(); //update previous run metrics 
		lineWidth = std::abs( runPrev->GetEndPos().GetX());

		run->SetStartPos( VPoint( lineWidth, 0));
		run->SetEndPos( VPoint( lineWidth, 0));
	}
}

void VTextLayout::VParagraphLayout::VParagraph::_NextRun( Run* &run, Line *line, GReal& lineWidth, sLONG i, bool isTab)
{
	_NextRun( run, line, run->GetDirection(), run->GetFont(), run->GetTextColor(), run->GetBackColor(), lineWidth, i, isTab);
}

void VTextLayout::VParagraphLayout::VParagraph::_NextRun( Run* &run, Line *line, eDirection dir, GReal& lineWidth, sLONG i, bool isTab)
{
	_NextRun( run, line, dir, run->GetFont(), run->GetTextColor(), run->GetBackColor(), lineWidth, i, isTab);
}

void VTextLayout::VParagraphLayout::VParagraph::_NextRun( Run* &run, Line *line, eDirection dir, VFont* &font, VTextStyle* style, GReal& lineWidth, sLONG i)
{
	xbox_assert( !style->GetFontName().IsEmpty());
	xbox_assert(!style->IsSpanRef()); //VParagraphLayout::UpdateLayout has merged yet span references styles & discarded it (local storage only)

	ReleaseRefCountable(&font);
	font = VFont::RetainFont(		style->GetFontName(),
									 _TextStyleToFontFace(style),
									 style->GetFontSize() > 0.0f ? style->GetFontSize() : 12,
									 fParaLayout->fLayout->fDPI); //fonts are pre-scaled by layout DPI (in order metrics to be computed at the desired DPI)
										  
	VColor textColor, backColor;
	if (style->GetHasForeColor())
		textColor.FromRGBAColor( style->GetColor());
	else
		textColor = run->fLine->fParagraph->fParaLayout->fLayout->fDefaultTextColor;
	
	if (style->GetHasBackColor() && style->GetBackGroundColor() != RGBACOLOR_TRANSPARENT)
		backColor.FromRGBAColor( style->GetBackGroundColor());
	else
		backColor = VColor(0,0,0,0);

	_NextRun( run, line, dir, font, textColor, backColor, lineWidth, i, false);
}


void VTextLayout::VParagraphLayout::VParagraph::_NextLine( Run* &run, Line* &line, sLONG i)
{
	xbox_assert(run == (line->thisRuns().back()));

	VGraphicContext *gc = fParaLayout->fComputingGC;
	xbox_assert(gc);

	eDirection runDir = run->GetDirection();
	VFont *runFont = run->GetFont();
	VColor runTextColor = run->GetTextColor();
	VColor runBackColor = run->GetBackColor();

	//terminate run
	if (run->GetEnd() == run->GetStart() && line->GetRuns().size() > 1)
		line->thisRuns().pop_back();
	else
	{
		run->SetEnd( i);
		run->PreComputeMetrics(); //update run metrics 
	}	

	//finalize line
	line->ComputeMetrics();

	//start next line
	if (i < fEnd)
	{
		line = new Line( this, line->GetIndex()+1, i, i);
		fLines.push_back( line);
		
		run = new Run(line, 0, i, i, runDir, runFont, &runTextColor, &runBackColor);
		line->thisRuns().push_back( run);

		//take account negative line padding if any (lines but the first are decaled with -fParaLayout->fLayout->GetPaddingFirstLine())
		if (fParaLayout->fLayout->GetPaddingFirstLine() < 0)
		{
			GReal startLineX = VTextLayout::_RoundMetrics(gc, -fParaLayout->fLayout->GetPaddingFirstLine()*fParaLayout->fLayout->fDPI/72);
			run->SetStartPos( VPoint( startLineX, 0));
			run->SetEndPos( VPoint( startLineX, 0));
		}
	}
}


/** extract font style from VTextStyle */
VFontFace VTextLayout::VParagraphLayout::VParagraph::_TextStyleToFontFace( const VTextStyle *inStyle)
{
	VFontFace fontFace = KFS_NORMAL;
	if (inStyle->GetBold() == TRUE)
		fontFace |= KFS_BOLD;
	if (inStyle->GetItalic() == TRUE)
		fontFace |= KFS_ITALIC;
	if (inStyle->GetUnderline() == TRUE)
		fontFace |= KFS_UNDERLINE;
	if (inStyle->GetStrikeout() == TRUE)
		fontFace |= KFS_STRIKEOUT;
	return fontFace;
}


void VTextLayout::VParagraphLayout::VParagraph::ComputeMetrics(bool inUpdateBoundsOnly, const GReal inMaxWidth, 
															   const GReal inMarginLeft, const GReal inMarginRight, const GReal inMarginTop, const GReal inMarginBottom)
{
	if (fDirty)
		inUpdateBoundsOnly = false;

	VGraphicContext *gc = fParaLayout->fComputingGC;
	xbox_assert(gc);
	
	if (!inUpdateBoundsOnly)
	{
		//determine bidi 
		//TODO: for now assume LTR (need to use ICU)

		fDirection = fParaLayout->fLayout->IsRTL() ? kPARA_RTL : kPARA_LTR;
		fDirections.clear();

		//compute sequential uniform styles on paragraph range (& store locally to speed up update later)

		Styles::iterator itStyle = fStyles.begin();
		for (;itStyle != fStyles.end(); itStyle++)
		{
			if (*itStyle)
				delete *itStyle;
		}
		fStyles.clear();
		if (fParaLayout->fLayout->fStyles || fParaLayout->fLayout->fExtStyles)
		{
			VTreeTextStyle *layoutStyles = fParaLayout->fLayout->fStyles ? fParaLayout->fLayout->fStyles->CreateTreeTextStyleOnRange( fStart, fEnd, false, fEnd > fStart) :
										   fParaLayout->fLayout->fExtStyles->CreateTreeTextStyleOnRange( fStart, fEnd, false, fEnd > fStart);
			if (layoutStyles)
			{
				if (layoutStyles->HasSpanRefs())
				{
					//ensure we merge span style with span reference style & discard span reference (only local copy is discarded)
					for (int i = 1; i <= layoutStyles->GetChildCount(); i++)
					{
						VTextStyle *style = layoutStyles->GetNthChild(i)->GetData();
						const VTextStyle *styleSource = NULL;
						if (style->IsSpanRef())
						{
							//we need to get the original span ref in order to get proper mouse over status
							//(as CreateTreeTextStyleOnRange does not preserve mouse over status - which is dependent at runtime on control events)
							sLONG rangeStart, rangeEnd;
							style->GetRange( rangeStart, rangeEnd);
							styleSource = fParaLayout->fLayout->GetStyleRefAtRange( rangeStart, rangeEnd);
							const VTextStyle *styleSpanRef = testAssert(styleSource) ? styleSource->GetSpanRef()->GetStyle() : NULL;
							if (styleSpanRef)
								style->MergeWithStyle( styleSpanRef);
							style->SetSpanRef(NULL); //discard span ref (useless now)						}
						}
					}
				}

				if (fParaLayout->fLayout->fExtStyles && fParaLayout->fLayout->fStyles)
					layoutStyles->ApplyStyles( fParaLayout->fLayout->fExtStyles);

				VTextStyle *style = VTextLayout::CreateTextStyle( fParaLayout->fLayout->fDefaultFont, 72);
				style->SetRange(fStart, fEnd);
				VTreeTextStyle *layoutStylesWithDefault = new VTreeTextStyle( style);
				if (layoutStylesWithDefault)
				{
					layoutStylesWithDefault->AddChild( layoutStyles);
					layoutStylesWithDefault->BuildSequentialStylesFromCascadingStyles( fStyles, NULL, false);
				}
				ReleaseRefCountable(&layoutStyles);
				ReleaseRefCountable(&layoutStylesWithDefault);
			}
		}
	}

	GReal maxWidth = 0.0f;
	if (inMaxWidth)
	{
		maxWidth = inMaxWidth-inMarginLeft-inMarginRight; 
		if (maxWidth <= 0.00001)
			maxWidth = 0.00001;
	}

	if (fLines.empty() || !inUpdateBoundsOnly || maxWidth != fMaxWidth)
	{
		//compute lines: take account styles, direction, line breaking (if max width != 0), tabs & first line start offset to build lines & runs

		Lines::const_iterator itLine = fLines.begin();
		for (;itLine != fLines.end(); itLine++)
			delete (*itLine);
		fLines.clear();

		fMaxWidth = maxWidth;

		//backup default font
		VFont *fontBackup = RetainRefCountable(fParaLayout->fLayout->fCurFont);
		xbox_assert(fontBackup);
		VFont *font = RetainRefCountable( fontBackup);

		//parse text 
		const UniChar *c = fParaLayout->fLayout->fTextPtr->GetCPointer()+fStart;
		VIndex lenText = fEnd-fStart;

		Line *line = new Line( this, 0, fStart, fStart);
		fLines.push_back( line);
		
		Run *run = new Run(line, 0, fStart, fStart, fDirection, fontBackup, &(fParaLayout->fLayout->fDefaultTextColor));
		line->thisRuns().push_back( run);

		if (!lenText)
		{
			//empty text
			if (fStyles.size())
			{
				//even if empty, we need to apply style if any (as run is empty, _NextRun does not create new run but only updates current run)
				GReal lineWidth = 0;
				_NextRun( run, line, fDirection, font, fStyles[0], lineWidth, 0);
			}
			run->PreComputeMetrics();
			line->ComputeMetrics();
		}
		else
		{
			VectorOfStringSlice words;
			if (fMaxWidth)
			{
				//word-wrap: get text words (using ICU line breaking rule)
				VString text;
				fParaLayout->fLayout->fTextPtr->GetSubString( fStart+1, lenText, text);

				VIntlMgr::GetDefaultMgr()->GetWordBoundariesWithOptions(text, words, eKW_UseICU | eKW_UseLineBreakingRule);
			}

			sLONG wordIndex = 0;
			sLONG wordStart, wordEnd;
			if (wordIndex < words.size())
			{
				wordStart = fStart+words[wordIndex].first-1;
				wordEnd = wordStart+words[wordIndex].second;
			}
			else
			{
				//no more word or no word-wrap: make wordStart & wordEnd consistent
				wordStart = wordEnd = fEnd+2;
			}
			sLONG wordIndexBackup = 0;

			eDirection dir = fDirection;
			sLONG dirIndex = -1;
			sLONG dirStart, dirEnd;
			
			sLONG styleIndex = -1;
			sLONG styleStart, styleEnd;
			
			sLONG startLineX = 0;
			sLONG startLine = fStart;
			sLONG endLine = fStart;
			GReal lineWidth = 0.0f;

			sLONG runsCountBackup = 0;
			sLONG dirIndexBackup = -1;

			sLONG styleIndexBackup = -1;

			sLONG endLineBackup = 0;
			sLONG lineWidthBackup = 0.0f;

			fWordWrapOverflow = false;

			if (!fDirections.empty())
			{
				dirIndex = 0;
				dirStart = fDirections[0].first;
				dirEnd = 1 < fDirections.size() ? fDirections[1].first : fEnd;
				dirIndexBackup = 0;
			}
			if (!fStyles.empty())
			{
				styleIndex = 0;
				fStyles[0]->GetRange( styleStart, styleEnd);
				styleIndexBackup = 0;
			}
			
			bool includeLeadingSpaces = true;

			//first take account first line padding if any
			if (fParaLayout->fLayout->GetPaddingFirstLine() > 0)
			{
				startLineX = VTextLayout::_RoundMetrics(gc, fParaLayout->fLayout->GetPaddingFirstLine()*fParaLayout->fLayout->fDPI/72);
				lineWidth = startLineX;
				run->SetStartPos( VPoint( startLineX, 0));
				run->SetEndPos( VPoint( startLineX, 0));
			}

			bool isPreviousValidForLineBreak = true;
			bool isValidForLineBreak = true;
			for (int i = fStart; i < fEnd+1; i++, c++, isPreviousValidForLineBreak = isValidForLineBreak)
			{
				isValidForLineBreak = i <= wordStart || i >= wordEnd;
				bool isSpaceChar = i >= fEnd || VIntlMgr::GetDefaultMgr()->IsSpace( *c);
				
				endLine = i;
				run->SetEnd( i);
				run->SetEndPos( run->GetStartPos());

				if (i > fStart && *(c-1) == '\t')
				{
					//we use one run only per tab (as run width is depending both on tab offset(s) & last run offset) so terminate tab run and start new one
					_NextRun( run, line, lineWidth, i, true);
					includeLeadingSpaces = true;
					isValidForLineBreak = true;
					isPreviousValidForLineBreak = false;
				}

				/* //useless because each VParagraph contains only one paragraph
				if (	i < fEnd 
						&&
						i > fStart 
						&& 
						(*(c-1) == 13))
					)
				{
					//explicit end of line: close run & line & start next line
					
					endOfLine = true;
					bool closeLine = true;
					if (fMaxWidth)
					{
						run->PreComputeMetrics(true); //compute metrics without trailing spaces
						lineWidth = std::abs(run->GetEndPos().GetX());
						if (lineWidth > fMaxWidth)
						{
							if (endLineBackup-startLine > 0.0f)
								closeLine = false;
							else
								fWordWrapOverflow = true;
						}
					}

					if (closeLine)
					{
						_NextLine( run, line, i);
						
						if (i >= fEnd)
							break;

						//start new line
						isPreviousValidForLineBreak = true;
						startLine = endLine = i;
						lineWidth = std::abs(run->GetEndPos().GetX());
						includeLeadingSpaces = true; 
					}
				}
				*/
				bool charCR = *c == 13;
				if (i < fEnd && run->GetEnd() > run->GetStart() && (*c == '\t' || *c == 13))
					//we use one run only per tab & end of line (as run width is depending both on tab offset(s) & last run offset) so start new run for tab or end of line
					_NextRun( run, line, lineWidth, i);

				//if direction is modified, terminate run & start new run
				if (dirIndex >= 0 && ((run->GetStart() == run->GetEnd()) || i < fEnd))
				{
					if (i == dirStart)
					{
						//start new run

						//set new direction
						dir = fDirections[dirIndex].second;
						_NextRun( run, line, dir, lineWidth, i);
					}
					if (i == dirEnd && i < fEnd)
					{
						//start new run

						dirIndex++;
						if (dirIndex >= fDirections.size())
						{
							dirIndex = -1;
							//rollback to default direction
							dir = fDirection;
						}
						else
						{
							dirStart = fDirections[dirIndex].first;
							dirEnd = (dirIndex+1) < fDirections.size() ? fDirections[dirIndex+1].first : fEnd;
							if (i == dirStart)
								//set new direction
								dir = fDirections[dirIndex].second;
							else
								//rollback to default direction
								dir = fDirection;
						}
						_NextRun( run, line, dir, lineWidth, i);
					}
				}

				//if style is modified, terminate run & start new run
				if (styleIndex >= 0 && ((run->GetStart() == run->GetEnd()) || i < fEnd))
				{
					if (i == styleStart)
					{
						//start new run with new style
						VTextStyle *style = fStyles[styleIndex];
						_NextRun( run, line, dir, font, style, lineWidth, i); //here font is updated from style too
					}
					if (i == styleEnd && i < fEnd)
					{
						//end current style: start new run

						styleIndex++;
						if (styleIndex >= fStyles.size())
						{
							styleIndex = -1;
							//rollback to default style
							CopyRefCountable(&font, fontBackup);
							_NextRun( run, line, dir, font, fParaLayout->fLayout->fDefaultTextColor, VColor(0,0,0,0), lineWidth, i);
						}
						else
						{
							fStyles[styleIndex]->GetRange( styleStart, styleEnd);
							if (i == styleStart)
							{
								//start new run with new style
								VTextStyle *style = fStyles[styleIndex];
								_NextRun( run, line, dir, font, style, lineWidth, i); //here font is updated from style too
							}
							else
							{
								//rollback to default style
								CopyRefCountable(&font, fontBackup);
								_NextRun( run, line, dir, font, fParaLayout->fLayout->fDefaultTextColor, VColor(0,0,0,0), lineWidth, i);
							}
						}
					}
				}

				bool endOfLine = false;
				if (i >= fEnd)
				{
					//close line: terminate run & line
					
					endOfLine = true;
					bool closeLine = true;
					if (fMaxWidth)
					{
						run->PreComputeMetrics(true); //compute metrics without trailing spaces
						lineWidth = std::abs(run->GetEndPos().GetX());
						if (lineWidth > fMaxWidth)
						{
							if (endLineBackup-startLine > 0.0f)
								closeLine = false;
							else
								fWordWrapOverflow = true;
						}
					}

					if (closeLine)
					{
						_NextLine( run, line, i);
						break; //terminate parsing
					}
				}

				bool breakLine = false;
				if (!includeLeadingSpaces && !isSpaceChar && i < fEnd)
				{
					//first character of first word of new line:
					//make start of line here
					
					startLine = endLine = i;
					endLineBackup = endLine;
					includeLeadingSpaces = true;
					isPreviousValidForLineBreak = true;
					line->SetStart( i);
					line->SetEnd( i);
					run->SetStart( i);
					run->SetEnd( i);
				}

				while (i >= wordEnd)
				{
					//end of word: iterate on next word
					wordIndex++;
					if (wordIndex < words.size())
					{
						wordStart = fStart+words[wordIndex].first-1;
						wordEnd = wordStart+words[wordIndex].second;
					}
					else
					{
						//no more word: make wordStart & wordEnd consistent
						wordStart = wordEnd = fEnd+2;
						break;
					}
				}

				if (fMaxWidth && (endOfLine || (isValidForLineBreak && !isPreviousValidForLineBreak && *c != 13)))
				{
					//word-wrap 

					if (!endOfLine && run->GetEnd() > run->GetStart())
						run->PreComputeMetrics(true); //ignore trailing spaces
					lineWidth = std::abs(run->GetEndPos().GetX());
					if (lineWidth > fMaxWidth)
					{
						//end of line: break on previous word otherwise make line with current word
						if (endLineBackup-startLine > 0.0f)
						{
							//one word at least was inside bounds: restore to end of last word inside bounds

							//restore line info
							endLine = endLineBackup;
							lineWidth = lineWidthBackup;
							
							//restore style
							styleIndex = styleIndexBackup;
							if (styleIndex >= 0)
								fStyles[styleIndex]->GetRange( styleStart, styleEnd);

							//restore direction
							dirIndex = dirIndexBackup;
							if (dirIndex >= 0)
							{
								dirStart = fDirections[dirIndex].first;
								dirEnd = (dirIndex+1) < fDirections.size() ? fDirections[dirIndex+1].first : fEnd;
								dir = fDirections[dirIndex].second;
							}
							else
								dir = fDirection;

							if (runsCountBackup < line->GetRuns().size())
							{
								//restore run
								xbox_assert( runsCountBackup);
								if (runsCountBackup < line->GetRuns().size())
								{
									while (runsCountBackup < line->GetRuns().size())
										line->thisRuns().pop_back();
									run = (line->thisRuns().back());
									xbox_assert(dir == run->GetDirection());
									CopyRefCountable(&font, run->GetFont());
								}
								run->SetEnd( endLine);
								run->PreComputeMetrics(true);
								lineWidth = std::abs(run->GetEndPos().GetX());
							}
							
							//restore word info
							wordIndex = wordIndexBackup;
							if (wordIndex < words.size())
							{
								wordStart = fStart+words[wordIndex].first-1;
								wordEnd = wordStart+words[wordIndex].second;
							}
							else
							{
								//no more word: make wordStart & wordEnd consistent
								wordStart = wordEnd = fEnd+2;
							}

							//restore parsing position
							i = endLine;
							c = fParaLayout->fLayout->fTextPtr->GetCPointer()+endLine;
							isSpaceChar = i >= fEnd || VIntlMgr::GetDefaultMgr()->IsSpace( *c);
						}
						else 
							fWordWrapOverflow = true;

						//close line: terminate run, update line & start next line
						_NextLine( run, line, i);
						
						//start new line
						isPreviousValidForLineBreak = true;
						startLine = endLine = i;
						endLineBackup = endLine;
						lineWidth = std::abs(run->GetEndPos().GetX());
						breakLine = true;
						includeLeadingSpaces = false; //do not include next leading spaces
					}

					//we are still inside bounds: backup current line including last word (as we are line breakable here)
					if (lineWidth <= fMaxWidth)
					{
						xbox_assert(endLine == i);
						endLineBackup = endLine;
						lineWidthBackup = lineWidth;
						dirIndexBackup = dirIndex;
						runsCountBackup = line->GetRuns().size();
						styleIndexBackup = styleIndex;
						wordIndexBackup = wordIndex;
					}
				}
				if (!isSpaceChar)
					includeLeadingSpaces = true;
			}
		}
		ReleaseRefCountable(&font);
		ReleaseRefCountable(&fontBackup);
	}

	//compute local metrics 

	fTypoBounds.SetEmpty();
	fTypoBoundsWithTrailingSpaces.SetEmpty();

	GReal layoutWidth = fMaxWidth; //max width minus margins

	//determine lines start position (only y here)
	GReal y = inMarginTop;
	Lines::iterator itLine = fLines.begin();
	for (;itLine != fLines.end(); itLine++)
	{
		y += (*itLine)->GetAscent();

		(*itLine)->SetStartPos( VPoint(0, y));

		y += (*itLine)->GetLineHeight()-(*itLine)->GetAscent();
	}
	GReal layoutHeight = y+inMarginBottom;

	//compute typographic bounds 
	//(relative to x = 0 & y = paragraph top origin)
	itLine = fLines.begin();
	for (;itLine != fLines.end(); itLine++)
	{
		//typo bounds without trailing spaces 
		VRect boundsTypo = (*itLine)->GetTypoBounds(false);
		boundsTypo.SetPosBy( 0, (*itLine)->GetStartPos().GetY());
		if (fTypoBounds.IsEmpty())
			fTypoBounds = boundsTypo;
		else
			fTypoBounds.Union( boundsTypo);

		//typo bounds with trailing spaces
		boundsTypo = (*itLine)->GetTypoBounds(true);
		boundsTypo.SetPosBy( 0, (*itLine)->GetStartPos().GetY());
		if (fTypoBoundsWithTrailingSpaces.IsEmpty())
			fTypoBoundsWithTrailingSpaces = boundsTypo;
		else
			fTypoBoundsWithTrailingSpaces.Union( boundsTypo);
	}
	xbox_assert(fTypoBounds.GetHeight() == layoutHeight-inMarginTop-inMarginBottom);

	if (!layoutWidth)
		layoutWidth = fTypoBounds.GetWidth();
	layoutWidth += inMarginLeft+inMarginRight;
	fLayoutBounds.SetCoords(0, 0, layoutWidth, layoutHeight);

	fDirty = false;
}

VTextLayout::VParagraphLayout::VParagraphLayout(VTextLayout *inLayout):VObject()
{
	fLayout = inLayout;

	fMarginLeft = fMarginRight = fMarginTop = fMarginBottom = 0;

	fComputingGC = NULL;
}

VTextLayout::VParagraphLayout::~VParagraphLayout()
{
	xbox_assert(fComputingGC == NULL);
}

/** update text layout metrics */ 
void VTextLayout::VParagraphLayout::UpdateLayout()
{
	if (fLayout->fLayoutIsValid)
		return;

	xbox_assert(fComputingGC == NULL);
	fComputingGC = _RetainComputingGC(fLayout->fGC);
	xbox_assert(fComputingGC);
	fComputingGC->BeginUsingContext(true);
#if VERSIONDEBUG && VERSIONWIN
	if (fComputingGC->IsGDIImpl())
	{
		VAffineTransform ctm;
		fComputingGC->GetTransform(ctm);
		xbox_assert(ctm.IsIdentity()); 
	}
#endif

	GReal maxWidth		= fLayout->fMaxWidthInit; 
	GReal maxHeight		= fLayout->fMaxHeightInit; 

	fMarginLeft			= _RoundMetrics(fComputingGC,fLayout->fMarginLeft); 
	fMarginRight		= _RoundMetrics(fComputingGC,fLayout->fMarginRight); 
	fMarginTop			= _RoundMetrics(fComputingGC,fLayout->fMarginTop); 
	fMarginBottom		= _RoundMetrics(fComputingGC,fLayout->fMarginBottom); 

	//determine paragraph(s)

	//clear & rebuild

	fParagraphs.reserve(10);
	fParagraphs.clear();

	const UniChar *c = fLayout->fTextPtr->GetCPointer();
	sLONG start = 0;
	sLONG i = start;

	sLONG index = 0;
	while (*c)
	{
		if (i > start && (*(c-1) == 13))
		{
			VRefPtr<VParagraph> para( new VParagraph( this, index++, start, i), false);
			fParagraphs.push_back( para);
			start = i;
		}
		c++;i++;
	}
	VRefPtr<VParagraph> para( new VParagraph( this, index++, start, i), false);
	fParagraphs.push_back( para);
	if (i > start && (*(c-1) == 13))
	{
		VRefPtr<VParagraph> para( new VParagraph( this, index, i, i), false);
		fParagraphs.push_back( para);
	}

	//compute paragraph(s) metrics

	GReal typoWidth = 0.0f;
	GReal typoHeight = 0.0f;

	Paragraphs::iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		VParagraph *para = itPara->Get();
		para->ComputeMetrics(false, maxWidth, fMarginLeft, fMarginRight, fMarginTop, fMarginBottom); 
		para->SetStartPos(VPoint(0,typoHeight));
		typoWidth = std::max(typoWidth, para->GetTypoBounds(false).GetWidth());
		typoHeight = typoHeight+para->GetLayoutBounds().GetHeight();
	}
	typoWidth += fMarginLeft+fMarginRight;

	GReal layoutWidth = 0.0f;
	GReal layoutHeight = 0.0f;
	if (maxWidth)
		layoutWidth = maxWidth;
	else
		layoutWidth = typoWidth;
	if (maxHeight)
		layoutHeight = maxHeight;
	else
		layoutHeight = typoHeight;

	fTypoBounds.SetCoords( 0, 0, typoWidth, typoHeight);
	fLayoutBounds.SetCoords( 0, 0, layoutWidth, layoutHeight);

	//here we update VTextLayout metrics using the desired DPI (because VTextLayout stores fDPI metrics while VParagraphLayout computes & stores metrics in point - with TWIPS precision)

	fLayout->fCurWidth = ceil(typoWidth);
	fLayout->fCurHeight = ceil(typoHeight);

	if (fLayout->fMaxWidthInit != 0.0f && fLayout->fCurWidth > fLayout->fMaxWidthInit)
		fLayout->fCurWidth = std::floor(fLayout->fMaxWidthInit); 
	if (fLayout->fMaxHeightInit != 0.0f && fLayout->fCurHeight > fLayout->fMaxHeightInit)
		fLayout->fCurHeight = std::floor(fLayout->fMaxHeightInit); 
	
	if (fLayout->fMaxWidthInit == 0.0f)
		fLayout->fCurLayoutWidth = fLayout->fCurWidth;
	else
		fLayout->fCurLayoutWidth = std::floor(fLayout->fMaxWidthInit); 
	
	if (fLayout->fMaxHeightInit == 0.0f)
		fLayout->fCurLayoutHeight = fLayout->fCurHeight;
	else
		fLayout->fCurLayoutHeight = std::floor(fLayout->fMaxHeightInit); 

	fComputingGC->EndUsingContext();
	ReleaseRefCountable(&fComputingGC);
}
					
void VTextLayout::VParagraphLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw)
{
	//we build transform to the target device coordinate space
	//if painting, we apply the transform to the passed gc & return in outCTM the previous transform which is restored in _EndLocalTransform
	//if not painting (get metrics), we return in outCTM the transform from res-independent coord space to the device space but we do not modify the passed gc transform

	if (inDraw)
	{
		inGC->UseReversedAxis();
		inGC->GetTransform( outCTM);
	}

	VAffineTransform xform;
	VPoint origin( inTopLeft);

	//apply vertical align offset
	switch (fLayout->GetParaAlign())
	{
	case AL_BOTTOM:
		xform.Translate(0, fLayoutBounds.GetHeight()-fTypoBounds.GetHeight()); 
		break;
	case AL_CENTER:
		xform.Translate(0, fLayoutBounds.GetHeight()*0.5-fTypoBounds.GetHeight()*0.5);
		break;
	default:
		break;
	}

	//apply printer scaling if appropriate
	//TODO: to avoid rounding artifacts at run boundaries while scaling
	//		we should re-compute run & line metrics at fLayout->fDPI*printer dpi/72 
	//      (but do not recalc wordwrap to keep the same line-breaks...)
		
	GReal scalex = 1, scaley = 1;
	if (inGC->HasPrinterScale()) 
	{
		xbox_assert( inGC->IsGDIImpl()); 
		GReal scalex, scaley;
		inGC->GetPrinterScale( scalex, scaley);
		origin.x = origin.x*scalex; //if printing, top left position is scaled also
		origin.y = origin.y*scaley;
	}

	if (scalex != 1 || scaley != 1)
		xform.Scale( scalex, scaley, VAffineTransform::MatrixOrderAppend);
	
	//stick (0,0) to inTopLeft

	xform.Translate( origin.GetX(), origin.GetY(), VAffineTransform::MatrixOrderAppend);
	
	//set new transform
	if (inDraw)
	{
		xform.Multiply( outCTM, VAffineTransform::MatrixOrderAppend);
		inGC->UseReversedAxis();
		inGC->SetTransform( xform);
	}
	else
		outCTM = xform;
}

void VTextLayout::VParagraphLayout::_EndLocalTransform( VGraphicContext *inGC, const VAffineTransform& inCTM, bool inDraw)
{
	if (!inDraw)
		return;
	inGC->UseReversedAxis();
	inGC->SetTransform( inCTM);
}

/** get line start position relative to text box left top origin */
void VTextLayout::VParagraphLayout::_GetLineStartPos( const Line* inLine, VPoint& outPos)
{
	outPos = inLine->GetStartPos(); //here it is relative to start of paragraph for y but 0 for x
	xbox_assert(outPos.GetX() == 0.0f); //should be 0 as we compute line x position only here
	GReal dx = 0, dy = 0;
	
	//adjust y 
	dy = inLine->fParagraph->GetStartPos().GetY();
	xbox_assert(inLine->fParagraph->GetStartPos().GetX() == 0.0f); 

	//adjust x 
	eDirection paraDirection = inLine->fParagraph->fDirection;
	switch (fLayout->GetTextAlign())
	{
	case AL_RIGHT:
		if (paraDirection == kPARA_RTL)
			dx = fLayoutBounds.GetWidth()-fMarginRight;
		else
			dx = fLayoutBounds.GetWidth()-fMarginRight-inLine->GetTypoBounds(false).GetWidth();
		break;
	case AL_CENTER:
		{
		GReal layoutWidth = fLayoutBounds.GetWidth()-fMarginLeft-fMarginRight;
		if (paraDirection == kPARA_RTL)
			dx = fMarginLeft+VTextLayout::_RoundMetrics(fComputingGC, (layoutWidth+inLine->GetTypoBounds(false).GetWidth())*0.5);
		else
			dx = fMarginLeft+VTextLayout::_RoundMetrics(fComputingGC, (layoutWidth-inLine->GetTypoBounds(false).GetWidth())*0.5);
		}
		break;
	default: //AL_LEFT & AL_JUSTIFY
		if (paraDirection == kPARA_RTL)
			dx = fMarginLeft+inLine->GetTypoBounds(false).GetWidth();
		else
			dx = fMarginLeft;
		break;
	}
	outPos.SetPosBy( dx, dy);
}

VTextLayout::VParagraphLayout::VParagraph*	VTextLayout::VParagraphLayout::_GetParagraphFromCharIndex(const sLONG inCharIndex)
{
	Paragraphs::iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		VParagraph *para = itPara->Get();
		if (inCharIndex < para->GetEnd())
			return para;
	}
	return fParagraphs.back().Get();
}

VTextLayout::VParagraphLayout::Line* VTextLayout::VParagraphLayout::_GetLineFromCharIndex(const sLONG inCharIndex)
{
	VParagraph *para = _GetParagraphFromCharIndex( inCharIndex);

	Lines::iterator itLine = para->fLines.begin();
	for (;itLine != para->fLines.end(); itLine++)
	{
		if (inCharIndex < (*itLine)->GetEnd())
			return (*itLine);
	}
	return para->fLines.back();
}

VTextLayout::VParagraphLayout::Run* VTextLayout::VParagraphLayout::_GetRunFromCharIndex(const sLONG inCharIndex)
{
	Line *line = _GetLineFromCharIndex( inCharIndex);

	Runs::iterator itRun = line->fRuns.begin();
	for (;itRun != line->fRuns.end(); itRun++)
	{
		if (inCharIndex < (*itRun)->GetEnd())
			return (*itRun);
	}
	return line->fRuns.back();
}

void VTextLayout::VParagraphLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft)
{
	xbox_assert(inGC);

	StUseContext_NoRetain context(inGC);
	fLayout->BeginUsingContext(inGC); 

	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		return;
	}
	if (!fLayout->_BeginDrawLayer( inTopLeft))
	{
		//text has been refreshed from layer: we do not need to redraw text content
		fLayout->_EndDrawLayer();
		fLayout->EndUsingContext();
		return;
	}

	VAffineTransform ctm;
	_BeginLocalTransform( inGC, ctm, inTopLeft, true); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space is mapped to point (dpi = 72)

	VColor textColor;
	inGC->GetTextColor( textColor);
	VColor fillColor = VColor(0,0,0,0);

	Paragraphs::const_iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		//draw paragraph
		VParagraph *para = itPara->Get();

		Lines::const_iterator itLine = para->GetLines().begin();
		for (;itLine != para->GetLines().end(); itLine++)
		{
			//draw line
			VPoint linePos;
			_GetLineStartPos( *itLine, linePos);

			Runs::const_iterator itRun = (*itLine)->GetRuns().begin();
			for (;itRun != (*itLine)->GetRuns().end(); itRun++)
			{
				//draw run

				Run* run = *itRun;

				if (run->GetEnd() == run->GetStart())
					continue;

				//draw background
				if (run->fBackColor.GetAlpha())
				{
					VRect bounds = VRect( run->fTypoBoundsWithTrailingSpaces.GetX(), -run->fLine->fAscent, run->fTypoBoundsWithTrailingSpaces.GetWidth(), run->fLine->GetLineHeight());
					bounds.SetPosBy( linePos.x, linePos.y);

					if (run->fBackColor != fillColor)
					{
						inGC->SetFillColor( run->fBackColor);
						fillColor = run->fBackColor;
					}
					inGC->FillRect( bounds);
				}
				//change text color
				if (run->fTextColor.GetAlpha() && run->fTextColor != textColor)
				{
					inGC->SetTextColor( run->fTextColor);
					textColor = run->fTextColor;
				}

				//set font
				VFont *font = run->GetFont();
				inGC->SetFont( font);

				//draw text
				VPoint pos = linePos + run->GetStartPos();
				VString runtext;
				fLayout->fTextPtr->GetSubString( run->GetStart()+1, run->GetEnd()-run->GetStart(), runtext);
				inGC->SetTextPosTo( pos);
				inGC->DrawText( runtext);
			}
		}
	}

	_EndLocalTransform( inGC, ctm, true);

	fLayout->_EndDrawLayer();
	fLayout->EndUsingContext();
}


void VTextLayout::VParagraphLayout::GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	xbox_assert(inGC);
	if (fLayout->fTextLength == 0 && !inReturnCharBoundsForEmptyText)
	{
		outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
		return;
	}

	StUseContext_NoRetain_NoDraw context(inGC);
	fLayout->BeginUsingContext(inGC, true);

	outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
	if (testAssert(fLayout->fLayoutIsValid))
	{
		outBounds.SetWidth( fLayout->fCurLayoutWidth);
		outBounds.SetHeight( fLayout->fCurLayoutHeight);
	}
	fLayout->EndUsingContext();
}

void VTextLayout::VParagraphLayout::GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	xbox_assert(inGC);

	if (inEnd < 0)
		inEnd = fLayout->fTextLength;
	if (fLayout->fTextLength == 0 || inStart >= inEnd)
	{
		outRunBounds.clear();
		return;
	}

	outRunBounds.clear();

	fLayout->BeginUsingContext(inGC, true);
	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		return;
	}


	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space is mapped to point (dpi = 72)

	VRect boundsRun;
	Run *run = _GetRunFromCharIndex( inStart);

	VPoint lineStartPos;
	_GetLineStartPos( run->fLine, lineStartPos);

	GReal startOffset = 0.0f;
	if (inStart > run->fStart)
		startOffset = run->GetCharOffsets()[inStart-1-run->fStart];
	GReal endOffset = run->fHitBounds.GetWidth();	
	if (inEnd < run->fEnd)
	{
		if (inEnd > run->fStart)
			endOffset = run->GetCharOffsets()[inEnd-1-run->fStart];
		else
			endOffset = 0.0f;
	}
	if (startOffset < endOffset)
	{
		if (run->fDirection == kPARA_LTR)
			boundsRun = VRect( run->GetStartPos().GetX()+startOffset, -run->fLine->fAscent, endOffset-startOffset, run->fLine->GetLineHeight());
		else
			boundsRun = VRect( run->GetStartPos().GetX()-endOffset, -run->fLine->fAscent, endOffset-startOffset, run->fLine->GetLineHeight());
		boundsRun.SetPosBy( lineStartPos.x, lineStartPos.y);
		boundsRun = xform.TransformRect( boundsRun);
		outRunBounds.push_back( boundsRun);
	}
	if (inEnd > run->fEnd)
	{
		Line *line = run->fLine;
		VParagraph *para = run->fLine->fParagraph;
		if (run->fIndex+1 < line->fRuns.size())
			run = line->fRuns[run->fIndex+1];
		else
			run  = NULL;
		while (para)
		{
			while (line)
			{
				while (run)
				{
					if (inEnd < run->fEnd)
					{
						xbox_assert(inEnd > run->fStart);
						endOffset = run->GetCharOffsets()[inEnd-1-run->fStart];
						if (run->fDirection == kPARA_LTR)
							boundsRun = VRect( run->GetStartPos().GetX(), -run->fLine->fAscent, endOffset, run->fLine->GetLineHeight());
						else
							boundsRun = VRect( run->GetStartPos().GetX()-endOffset, -run->fLine->fAscent, endOffset, run->fLine->GetLineHeight());
					}
					else
						boundsRun = VRect( run->fHitBounds.GetX(), -run->fLine->fAscent, run->fHitBounds.GetWidth(), run->fLine->GetLineHeight());
					boundsRun.SetPosBy( lineStartPos.x, lineStartPos.y);
					boundsRun = xform.TransformRect( boundsRun);
					outRunBounds.push_back( boundsRun);

					if (inEnd > run->fEnd && run->fIndex+1 < line->fRuns.size())
						run = line->fRuns[run->fIndex+1];
					else
						run = NULL;
				}
				if (inEnd <= line->fEnd)
					break;
				else if (line->fIndex+1 < para->fLines.size())
				{
					line = para->fLines[line->fIndex+1];
					_GetLineStartPos( line, lineStartPos);
					run = line->fRuns[0];
				}
				else
					line = NULL;
			}
			if (inEnd <= para->GetEnd())
				break;
			else if (para->fIndex+1 < fParagraphs.size())
			{
				para = fParagraphs[para->fIndex+1].Get();
				line = para->fLines[0];
				_GetLineStartPos( line, lineStartPos);
				run = line->fRuns[0];
			}
			else
				para = NULL;
		}
	}
		
	_EndLocalTransform( inGC, xform, false);

	fLayout->EndUsingContext();
}

void VTextLayout::VParagraphLayout::GetCaretMetricsFromCharIndex(	VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex _inCharIndex, 
														VPoint& outCaretPos, GReal& outTextHeight, 
														const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	xbox_assert(inGC);

	sLONG inCharIndex = _inCharIndex;
	if (inCharIndex < 0)
		inCharIndex = fLayout->fTextLength;
	if (fLayout->fTextPtr->GetLength() == 0)
	{
		//should never happen as VTextLayout ensures fTextPtr is not empty in order to get proper metrics for empty text.
		//(the real text length is fLayout->fTextLength)
		xbox_assert(false);
		return;
	}

	fLayout->BeginUsingContext(inGC, true);
	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		return;
	}


	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space is mapped to point (dpi = 72)

	VRect boundsRun;
	Run *run = _GetRunFromCharIndex( inCharIndex);

	VPoint lineStartPos;
	_GetLineStartPos( run->fLine, lineStartPos);

	GReal charOffset = 0.0f;
	if (inCaretLeading)
	{
		if (inCharIndex > run->fStart)
			charOffset = run->GetCharOffsets()[inCharIndex-1-run->fStart];
	}
	else
		charOffset = run->GetCharOffsets()[inCharIndex-run->fStart];

	if (run->fDirection == kPARA_LTR)
		outCaretPos.SetX( run->GetStartPos().GetX()+charOffset);
	else
		outCaretPos.SetX( run->GetStartPos().GetX()-charOffset);

	if (inCaretUseCharMetrics)
	{
		if (inCaretLeading && inCharIndex == run->fStart && run->fIndex > 0)
		{
			//get previous run caret metrics
			Run *runPrev = run->fLine->fRuns[run->fIndex-1];

			outCaretPos.y = -runPrev->fAscent;
			outTextHeight = runPrev->fAscent+runPrev->fDescent;
		}
		else
		{
			outCaretPos.y =  -run->fAscent;
			outTextHeight = run->GetAscent()+run->GetDescent();
		}
	}
	else
	{
		outCaretPos.SetY( -run->fLine->fAscent);
		outTextHeight = run->fLine->GetLineHeight();
	}

	outCaretPos.SetPosBy( lineStartPos.x, lineStartPos.y);

	VPoint caretBottom( outCaretPos.x, outCaretPos.y+outTextHeight);
	outCaretPos = xform.TransformPoint( outCaretPos);
	caretBottom = xform.TransformPoint( caretBottom);
	outTextHeight = std::abs(caretBottom.y - outCaretPos.y);

	_EndLocalTransform( inGC, xform, false);

	fLayout->EndUsingContext();
}

/** get paragraph from local position (relative to text box origin+vertical align decal & res-independent - 72dpi)*/
VTextLayout::VParagraphLayout::VParagraph* VTextLayout::VParagraphLayout::_GetParagraphFromLocalPos(const VPoint& inPos)
{
	xbox_assert(fParagraphs.size() > 0);

	Paragraphs::iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		VParagraph *para = itPara->Get();
		if (inPos.GetY() < para->GetStartPos().GetY()+para->GetLayoutBounds().GetHeight())
			return para;
	}
	return fParagraphs.back().Get();
}

VTextLayout::VParagraphLayout::Line* VTextLayout::VParagraphLayout::_GetLineFromLocalPos(const VPoint& inPos)
{
	VParagraph* para = _GetParagraphFromLocalPos( inPos);
	xbox_assert(para);

	VPoint pos = inPos;
	pos.y -= para->GetStartPos().GetY();

	Lines::iterator itLine = para->fLines.begin();
	for (;itLine != para->fLines.end(); itLine++)
	{
		if (pos.GetY() < (*itLine)->GetStartPos().GetY()+(*itLine)->GetLineHeight()-(*itLine)->GetAscent())
			return (*itLine);
	}
	return para->fLines.back();
}

VTextLayout::VParagraphLayout::Run* VTextLayout::VParagraphLayout::_GetRunFromLocalPos(const VPoint& inPos, VPoint& outLineLocalPos)
{
	Line *line = _GetLineFromLocalPos( inPos);

	VPoint lineStartPos;
	_GetLineStartPos( line, lineStartPos);
	VPoint pos = inPos;
	pos -= lineStartPos;
	outLineLocalPos = pos;

	GReal minx = kMAX_SmallReal;
	GReal maxx = kMIN_SmallReal; 
	Runs::iterator itRun = line->fRuns.begin();
	for (;itRun != line->fRuns.end(); itRun++)
	{
		if (pos.GetX() >= (*itRun)->fHitBounds.GetLeft() && pos.GetX() < (*itRun)->fHitBounds.GetRight())
			return (*itRun);
		minx = std::min( (*itRun)->fHitBounds.GetLeft(), minx);
		maxx = std::max( (*itRun)->fHitBounds.GetRight(), maxx);
	}
	//if we do not point inside a run, we return the first or last run depending on paragraph direction and if we point before line start or after line end
	//(FIXME:maybe we should adjust with run visual ordering too)
	if (pos.GetX() < minx)
	{
		if (line->fParagraph->fDirection == kPARA_LTR)
			return (line->fRuns[0]);
		else
			return (line->fRuns.back());
	}
	else 
	{
		xbox_assert( pos.GetX() >= maxx);
		if (line->fParagraph->fDirection == kPARA_LTR)
			return (line->fRuns.back());
		else
			return (line->fRuns[0]);
	}
}


bool VTextLayout::VParagraphLayout::GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex)
{
	xbox_assert(inGC);

	if (fLayout->fTextLength == 0)
	{
		outCharIndex = 0;
		return false;
	}

	fLayout->BeginUsingContext(inGC, true);
	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		outCharIndex = 0;
		return false;
	}


	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space is mapped to point (dpi = 72)

	VPoint pos( inPos);
	pos = xform.Inverse().TransformPoint( pos); //transform from device space to local space

	VPoint lineLocalPos;
	Run *run = _GetRunFromLocalPos( pos, lineLocalPos);
	xbox_assert(run);
	
	CharOffsets::const_iterator itOffset = run->GetCharOffsets().begin();	   
	sLONG index = 0;
	if (run->fDirection == kPARA_LTR)
	{
		GReal lastOffset = 0.0f;
		for (;itOffset != run->GetCharOffsets().end(); itOffset++, index++)
		{
			if (lineLocalPos.GetX()-run->GetStartPos().GetX() < (*itOffset+lastOffset)*0.5f)
				break;
			else
				lastOffset = *itOffset;
		}
	}
	else
	{
		GReal lastOffset = 0.0f;
		for (;itOffset != run->GetCharOffsets().end(); itOffset++, index++)
		{
			if (run->GetStartPos().GetX()-lineLocalPos.GetX() < (*itOffset+lastOffset)*0.5f)
				break;
			else
				lastOffset = *itOffset;
		}
	}
	outCharIndex = run->GetStart()+index;

	//adjust with end of line (as we point on the current line, we cannot return a index after CR  - which is on the next line - but just before the CR) 
	if (outCharIndex > 0 && outCharIndex == run->GetEnd() && (*fLayout->fTextPtr)[outCharIndex-1] == 13)
		outCharIndex--;

	_EndLocalTransform( inGC, xform, false);

	fLayout->EndUsingContext();

	return true;
}

/** move character index to the nearest character index on the line before/above the character line 
@remarks
	return false if character is on the first line (ioCharIndex is not modified)
*/
bool VTextLayout::VParagraphLayout::MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	xbox_assert(inGC);

	if (fLayout->fTextLength == 0)
		return false;

	fLayout->BeginUsingContext(inGC, true);
	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		return false;
	}

	//get previous line
	bool result = true;
	Line *line = _GetLineFromCharIndex( ioCharIndex);
	if (line->fIndex == 0)
	{
		if (line->fParagraph->fIndex > 0)
			line = (line->fParagraph->fParaLayout->fParagraphs[line->fParagraph->fIndex-1]).Get()->fLines.back();
		else
			result = false;
	}
	else 
		line = line->fParagraph->fLines[line->fIndex-1];

	if (result)
	{
		//get line position in gc space
		VAffineTransform xform;
		_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space is mapped to point (dpi = 72)

		VPoint lineStartPos;
		_GetLineStartPos( line, lineStartPos);
		VPoint posLine = xform.TransformPoint( lineStartPos);

		_EndLocalTransform( inGC, xform, false);

		//finally get new character index using current character x position & new line y position (both in gc space)
		VPoint charPos;
		GReal textHeight;
		GetCaretMetricsFromCharIndex( inGC, inTopLeft, ioCharIndex, charPos, textHeight, true, false);  
		sLONG charIndex = ioCharIndex;
		GetCharIndexFromPos( inGC, inTopLeft, VPoint(charPos.GetX(), posLine.GetY()), ioCharIndex);
		result = ioCharIndex != charIndex;
	}

	fLayout->EndUsingContext();

	return result;
}

/** move character index to the nearest character index on the line after/below the character line
@remarks
	return false if character is on the last line (ioCharIndex is not modified)
*/
bool VTextLayout::VParagraphLayout::MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	xbox_assert(inGC);

	if (fLayout->fTextLength == 0)
		return false;

	fLayout->BeginUsingContext(inGC, true);
	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		return false;
	}

	//get next line
	bool result = true;
	Line *line = _GetLineFromCharIndex( ioCharIndex);
	if (line->fIndex >= line->fParagraph->fLines.size()-1)
	{
		if (line->fParagraph->fIndex >= line->fParagraph->fParaLayout->fParagraphs.size()-1)
			result = false;
		else
			line = (line->fParagraph->fParaLayout->fParagraphs[line->fParagraph->fIndex+1]).Get()->fLines[0];
	}
	else 
		line = line->fParagraph->fLines[line->fIndex+1];

	if (result)
	{
		//get line position in gc space
		VAffineTransform xform;
		_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space is mapped to point (dpi = 72)

		VPoint lineStartPos;
		_GetLineStartPos( line, lineStartPos);
		VPoint posLine = xform.TransformPoint( lineStartPos);

		_EndLocalTransform( inGC, xform, false);

		//finally get new character index using current character x position & new line y position (both in gc space)
		VPoint charPos;
		GReal textHeight;
		GetCaretMetricsFromCharIndex( inGC, inTopLeft, ioCharIndex, charPos, textHeight, true, false);  
		sLONG charIndex = ioCharIndex;
		GetCharIndexFromPos( inGC, inTopLeft, VPoint(charPos.GetX(), posLine.GetY()), ioCharIndex);
		result = ioCharIndex != charIndex;
	}

	fLayout->EndUsingContext();

	return result;
}


//following methods are useful for fast text replacement (only the modified paragraph(s) are fully computed again)


/** replace text range */
void VTextLayout::VParagraphLayout::ReplaceText( sLONG inStart, sLONG inEnd, const VString& inText)
{
	//TODO (for now rebuild all paragraphs)
	fLayout->fLayoutIsValid = false;
}

/** apply character style (use style range) */
void VTextLayout::VParagraphLayout::ApplyStyle( const VTextStyle* inStyle)
{
	//TODO (for now rebuild all paragraphs)
	fLayout->fLayoutIsValid = false;
}




