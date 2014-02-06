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
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif
#endif
#if VERSIONMAC
#include "XMacQuartzGraphicContext.h"
#endif

#include "VDocBaseLayout.h"
#include "VDocBorderLayout.h"
#include "VDocImageLayout.h"
#include "VDocParagraphLayout.h"

#undef min
#undef max

VDocParagraphLayout::VDocParagraphLayout(bool inShouldEnableCache):VDocBaseLayout()
{
	_Init();
	fShouldEnableCache = inShouldEnableCache;
	ResetParagraphStyle();
}

/**	VDocParagraphLayout constructor
@param inPara
	VDocParagraph source: the passed paragraph is used only for initializing control plain text, styles & paragraph properties
						  but inPara is not retained 
	(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
@param inShouldEnableCache
	enable/disable cache (default is true): see ShouldEnableCache
*/
VDocParagraphLayout::VDocParagraphLayout(const VDocParagraph *inPara, bool inShouldEnableCache):VDocBaseLayout()
{
	_Init();
	fShouldEnableCache = inShouldEnableCache;
	if (!inPara)
	{
		ResetParagraphStyle();
		return;
	}

	_ReplaceText( inPara);

	const VTreeTextStyle *styles = inPara->GetStyles();
	ApplyStyles( styles);
	
	_ApplyParagraphProps( inPara, false);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
	ReleaseRefCountable(&fDefaultStyle);
}

VDocParagraphLayout::~VDocParagraphLayout()
{
	xbox_assert(fGCUseCount == 0);
	ReleaseRefCountable(&fDefaultStyle);
	ReleaseRefCountable(&fBackupFont);
	ReleaseRefCountable(&fLayerOffScreen);
	ReleaseRefCountable(&fDefaultFont);
	ReleaseRefCountable(&fExtStyles);
	ReleaseRefCountable(&fStyles);
}


void VDocParagraphLayout::_Init()
{
	fLayoutIsValid					= false;
	fShouldEnableCache				= false;
	fShouldDrawOnLayer				= false;
	fShowRefs						= kSRCTT_Value;
	fHighlightRefs					= false;
	
	fIsPassword						= false;

	fGC								= NULL;
	fGCUseCount						= 0;
	fGCType							= eDefaultGraphicContext; //means same as undefined here

	fLayerIsDirty					= true;
	fLayerOffsetViewRect			= VPoint(-100000.0f, 0); 
	fLayerOffScreen					= NULL;

	fStyles = fExtStyles			= NULL;
	fUseFontTrueTypeOnly			= false;
	fStylesUseFontTrueTypeOnly		= true;
	
	fDefaultFont					= VFont::RetainStdFont(STDF_TEXT);
	fDefaultTextColor				= VColor(0,0,0);
	fDefaultStyle					= NULL;
	
	fShouldPaintSelection			= false;
	fSelActiveColor					= VTEXTLAYOUT_SELECTION_ACTIVE_COLOR;
	fSelInactiveColor				= VTEXTLAYOUT_SELECTION_INACTIVE_COLOR;
	fSelIsActive					= true;
	fSelStart						= fSelEnd = 0;
	fSelIsDirty						= false;

	fLayoutKerning					= 0.0f;
	fLayoutTextRenderingMode		= TRM_NORMAL;
	fLayoutBoundsDirty				= false;

	fRenderMaxWidth					= 0.0f;
	fRenderMaxHeight				= 0.0f;
	fMaxWidthTWIPS					= 0;

	fRenderMargin.left				= fRenderMargin.right = fRenderMargin.top = fRenderMargin.bottom = 0;
	fMargin.left					= fMargin.right = fMargin.top = fMargin.bottom = 0.0f;
	
	fLineHeight						= -1;
	fTextIndent						= 0;
	fTabStopOffset					= ICSSUtil::CSSDimensionToPoint(1.25, kCSSUNIT_CM);
	fTabStopType					= TTST_LEFT;
	fLayoutMode						= TLM_NORMAL;

	fBackupFont						= NULL;
	fBackupParentGC					= NULL;
	fIsPrinting						= false;
	fSkipDrawLayer					= false;
}

/** set dirty */
void VDocParagraphLayout::SetDirty(bool /*inApplyToChildren*/, bool inApplyToParent) 
{ 
	fLayoutIsValid = false; 
	if (fParent && inApplyToParent) 
	{ 
		ITextLayout *textLayout = fParent->QueryTextLayoutInterface(); 
		if (textLayout) 
			textLayout->SetDirty(); 
	}
}

void VDocParagraphLayout::ShouldEnableCache( bool inShouldEnableCache)
{
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
void VDocParagraphLayout::ShouldDrawOnLayer( bool inShouldDrawOnLayer)
{
	if (fShouldDrawOnLayer == inShouldDrawOnLayer)
		return;
	fShouldDrawOnLayer = inShouldDrawOnLayer;
	fLayerIsDirty = true;
	if (fGC && !fShouldDrawOnLayer && !fIsPrinting)
		_ResetLayer();
}

/** paint native selection or custom selection  */
void VDocParagraphLayout::ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor, const VColor* inInactiveColor)
{
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
	fSelIsDirty = fSelStart < fSelEnd;
	if (inActiveColor)
		fSelActiveColor = *inActiveColor;
	if (inInactiveColor)
		fSelInactiveColor = *inInactiveColor;
	fLayerIsDirty = true;
}

/** set layout DPI (default is 4D form DPI = 72) */
void VDocParagraphLayout::SetDPI( const GReal inDPI)
{
	if (fDPI == inDPI && fDPI == fRenderDPI)
		return;
	VDocBaseLayout::SetDPI( inDPI);
	fLayoutIsValid = false;
}

/** set inside margins	(in twips) - CSS padding */
void VDocParagraphLayout::SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin)
{
	if (fMarginInTWIPS == inPadding)
		return;
	VDocBaseLayout::SetPadding( inPadding, inCalcAllMargin);
	fLayoutBoundsDirty = true;
}

/** set outside margins	(in twips) - CSS margin */
void VDocParagraphLayout::SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin)
{
	if (fMarginOutTWIPS == inMargins)
		return;
	VDocBaseLayout::SetMargins( inMargins, inCalcAllMargin);
	fLayoutBoundsDirty = true;
}


/** set text align - CSS text-align (-1 for not defined) */
void VDocParagraphLayout::SetTextAlign( const AlignStyle inHAlign) 
{ 
	if (fTextAlign == inHAlign)
		return;
	VDocBaseLayout::SetTextAlign( inHAlign);
	fLayoutBoundsDirty = true;
	if (inHAlign == AL_JUST)
		SetDirty();
}

/** set vertical align - CSS vertical-align (-1 for not defined) */
void VDocParagraphLayout::SetVerticalAlign( const AlignStyle inVAlign) 
{ 
	if (fVerticalAlign == inVAlign)
		return;
	VDocBaseLayout::SetVerticalAlign( inVAlign);
	fLayoutBoundsDirty = true;
}

/** set width (in twips) - CSS width (0 for auto) */
void VDocParagraphLayout::SetWidth(const uLONG inWidth)
{
	if (fWidthTWIPS == inWidth)
		return;
	VDocBaseLayout::SetWidth( inWidth ? (inWidth > GetMinWidth() ? inWidth : GetMinWidth()) : 0);
	fRenderMaxWidth = inWidth ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetWidth(true))*fRenderDPI/72) : 0;
	fLayoutBoundsDirty = true;
}

/** set min width (in twips) - CSS min-width (0 for auto) */
void VDocParagraphLayout::SetMinWidth(const uLONG inWidth)
{
	if (fMinWidthTWIPS == inWidth)
		return;
	VDocBaseLayout::SetMinWidth( inWidth);
	if (fWidthTWIPS && fWidthTWIPS < fMinWidthTWIPS) 
		SetWidth( inWidth);
	fLayoutBoundsDirty = true;
}


/** set height (in twips) - CSS height (0 for auto) */
void VDocParagraphLayout::SetHeight(const uLONG inHeight)
{
	if (fHeightTWIPS == inHeight)
		return;
	VDocBaseLayout::SetHeight( inHeight ? (inHeight > GetMinHeight() ? inHeight : GetMinHeight()) : 0);
	fRenderMaxHeight = inHeight ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetHeight(true))*fRenderDPI/72) : 0;
	fLayoutBoundsDirty = true;
}

/** set min height (in twips) - CSS min-height (0 for auto) */
void VDocParagraphLayout::SetMinHeight(const uLONG inHeight)
{
	if (fMinHeightTWIPS == inHeight)
		return;
	VDocBaseLayout::SetMinHeight( inHeight);
	if (fHeightTWIPS && fHeightTWIPS < fMinHeightTWIPS) 
		SetHeight( inHeight);
	fLayoutBoundsDirty = true;
}

/** set background color - CSS background-color (transparent for not defined) */
void VDocParagraphLayout::SetBackgroundColor( const VColor& inColor)
{
	if (fBackgroundColor == inColor)
		return;
	VDocBaseLayout::SetBackgroundColor( inColor);
	fLayerIsDirty = true;
}


/** set background clipping - CSS background-clip */
void VDocParagraphLayout::SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip)
{
	if (fBackgroundClip == inBackgroundClip)
		return;
	VDocBaseLayout::SetBackgroundClip( inBackgroundClip);
	fLayerIsDirty = true;
}

/** set background origin - CSS background-origin */
void VDocParagraphLayout::SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin)
{
	if (fBackgroundOrigin == inBackgroundOrigin)
		return;
	VDocBaseLayout::SetBackgroundOrigin( inBackgroundOrigin);
	SetDirty();
}

/** set borders 
@remarks
	if inLeft is NULL, left border is same as right border 
	if inBottom is NULL, bottom border is same as top border
	if inRight is NULL, right border is same as top border
	if inTop is NULL, top border is default border (solid, medium width & black color)
*/
void VDocParagraphLayout::SetBorders( const sDocBorder *inTop, const sDocBorder *inRight, const sDocBorder *inBottom, const sDocBorder *inLeft)
{
	VDocBaseLayout::SetBorders( inTop, inRight, inBottom, inLeft);
	SetDirty();
}

/** clear borders */
void VDocParagraphLayout::ClearBorders()
{
	if (!fBorderLayout)
		return;
	VDocBaseLayout::ClearBorders();
	SetDirty();
}

/** set background image 
@remarks
	see VDocBackgroundImageLayout constructor for parameters description */
void VDocParagraphLayout::SetBackgroundImage(	const VString& inURL,	
												const eStyleJust inHAlign, const eStyleJust inVAlign, 
												const eDocPropBackgroundRepeat inRepeat,
												const GReal inWidth, const GReal inHeight,
												const bool inWidthIsPercentage,  const bool inHeightIsPercentage)
{
	if (inURL.IsEmpty())
	{
		ClearBackgroundImage();
		return;
	}
	VDocBaseLayout::SetBackgroundImage( inURL, inHAlign, inVAlign, inRepeat, inWidth, inHeight, inWidthIsPercentage, inHeightIsPercentage);
	SetDirty();
}

/** clear background image */
void VDocParagraphLayout::ClearBackgroundImage()
{
	if (!fBackgroundImageLayout)
		return;
	VDocBaseLayout::ClearBackgroundImage();
	SetDirty();
}


/** set node properties from the current properties */
void VDocParagraphLayout::SetPropsTo( VDocNode *outNode)
{
	VDocBaseLayout::SetPropsTo( outNode);

	VDocParagraph *para = dynamic_cast<VDocParagraph *>(outNode);
	if (!testAssert(para))
		return;

	//for inheritable properties, always set it

	if (IsRTL())
		para->SetDirection( kDOC_DIRECTION_RTL);
	else
		para->SetDirection( kDOC_DIRECTION_LTR);

	//if (fLineHeight != -1)
	{
		if (fLineHeight < 0)
			para->SetLineHeight( (sLONG)-floor((-fLineHeight)*100+0.5)); //percentage
		else
			para->SetLineHeight( ICSSUtil::PointToTWIPS( fLineHeight)); //absolute line height
	}

	//if (fTextIndent != 0)
	{
		if (fTextIndent >= 0)
			para->SetTextIndent( ICSSUtil::PointToTWIPS( fTextIndent)); 
		else
			para->SetTextIndent( -ICSSUtil::PointToTWIPS( -fTextIndent));
	}

	uLONG tabStopOffsetTWIPS = (uLONG)ICSSUtil::PointToTWIPS( fTabStopOffset);
	if (tabStopOffsetTWIPS != (uLONG)para->GetDefaultPropValue( kDOC_PROP_TAB_STOP_OFFSET))
		para->SetTabStopOffset( tabStopOffsetTWIPS);
	if (fTabStopType != TTST_LEFT)
		para->SetTabStopType( (eDocPropTabStopType)fTabStopType);
}

void VDocParagraphLayout::InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited)
{
	VDocBaseLayout::InitPropsFrom(inNode, inIgnoreIfInherited);

	const VDocParagraph *inPara = dynamic_cast<const VDocParagraph *>(inNode);
	if (!testAssert(inPara))
		return;

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_DIRECTION) && !inPara->IsInherited( kDOC_PROP_DIRECTION)))
	{
		if (inPara->GetDirection() == kDOC_DIRECTION_RTL)
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
		(inPara->IsOverriden( kDOC_PROP_TEXT_INDENT) && !inPara->IsInherited( kDOC_PROP_TEXT_INDENT)))
	{
		if (inPara->GetTextIndent() >= 0)
			SetTextIndent( ICSSUtil::TWIPSToPoint(inPara->GetTextIndent()));
		else
			SetTextIndent( -ICSSUtil::TWIPSToPoint(-inPara->GetTextIndent()));
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


VDocParagraphLayout::Run::Run(Line *inLine, const sLONG inIndex, const sLONG inBidiVisualIndex, const sLONG inStart, const sLONG inEnd, const eDirection inDirection, const GReal inFontSize, VFont *inFont, const VColor *inTextColor, const VColor *inBackColor)
{
	fLine = inLine;
	fIndex = inIndex;
	fBidiVisualIndex = inBidiVisualIndex;
	fStart = inStart;
	fEnd = inEnd;
	fDirection = inDirection;
	fFontSize = inFontSize;
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
	fIsLeadingSpace = false;
	fTabOffset = 0;
	fNodeLayout = NULL;
}

VDocParagraphLayout::Run::Run(const Run& inRun)
{
	fLine = inRun.fLine;
	fIndex = inRun.fIndex;
	fBidiVisualIndex = inRun.fBidiVisualIndex;
	fStart = inRun.fStart;
	fEnd = inRun.fEnd;
	fDirection = inRun.fDirection;
	fFontSize = inRun.fFontSize;
	fFont = NULL;
	CopyRefCountable(&fFont, inRun.fFont);
	fStartPos = inRun.fStartPos;
	fEndPos = inRun.fEndPos;
	fAscent = inRun.fAscent;
	fDescent = inRun.fDescent;
	fTypoBounds = inRun.fTypoBounds;
	fTypoBoundsWithKerning = inRun.fTypoBoundsWithKerning;
	fTypoBoundsWithTrailingSpaces = inRun.fTypoBoundsWithTrailingSpaces;
	fTextColor = inRun.fTextColor;
	fBackColor = inRun.fBackColor;
	fIsLeadingSpace = inRun.fIsLeadingSpace;
	fTabOffset = inRun.fTabOffset;
	fNodeLayout = inRun.fNodeLayout;
}

VDocParagraphLayout::Run& VDocParagraphLayout::Run::operator=(const Run& inRun)
{
	if (this == &inRun)
		return *this;

	fLine = inRun.fLine;
	fIndex = inRun.fIndex;
	fBidiVisualIndex = inRun.fBidiVisualIndex;
	fStart = inRun.fStart;
	fEnd = inRun.fEnd;
	fDirection = inRun.fDirection;
	fFontSize = inRun.fFontSize;
	CopyRefCountable(&fFont, inRun.fFont);
	fStartPos = inRun.fStartPos;
	fEndPos = inRun.fEndPos;
	fAscent = inRun.fAscent;
	fDescent = inRun.fDescent;
	fTypoBounds = inRun.fTypoBounds;
	fTypoBoundsWithKerning = inRun.fTypoBoundsWithKerning;
	fTypoBoundsWithTrailingSpaces = inRun.fTypoBoundsWithTrailingSpaces;
	fTextColor = inRun.fTextColor;
	fBackColor = inRun.fBackColor;
	fIsLeadingSpace = inRun.fIsLeadingSpace;
	fTabOffset = inRun.fTabOffset;
	fNodeLayout = inRun.fNodeLayout;

	return *this;
}

bool VDocParagraphLayout::Run::IsTab() const
{
	if (fEnd-fStart != 1)
		return false;

	return ((fLine->fParagraph->fParaLayout->fText)[fStart] == '\t');
}


/** update internal font using design font size */
void VDocParagraphLayout::Run::NotifyChangeDPI()
{
	//update font according to new DPI
	xbox_assert(fFont);
	VString fontName = fFont->GetName();
	VFontFace fontFace = fFont->GetFace();
	ReleaseRefCountable(&fFont);
	fFont = VFont::RetainFont(		fontName,
									fontFace,
									fFontSize,
									fLine->fParagraph->fCurDPI); //fonts are pre-scaled by layout DPI (in order metrics to be computed at the desired DPI)

	if (fNodeLayout)
	{
		//update embedded object layout according to new DPI

		fNodeLayout->SetDPI( fLine->fParagraph->fCurDPI);
		fNodeLayout->UpdateLayout( fLine->fParagraph->fParaLayout->fGC);
	}
}


VGraphicContext* VDocParagraphLayout::_RetainComputingGC( VGraphicContext *inGC)
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

const VDocParagraphLayout::CharOffsets& VDocParagraphLayout::Run::GetCharOffsets()
{
	if (fCharOffsets.size() != fEnd-fStart)
	{
		if (fCharOffsets.size() > fEnd-fStart)
		{
			fCharOffsets.resize( fEnd-fStart);
			return fCharOffsets;
		}
		
		if (fNodeLayout)
		{
			xbox_assert(fEnd-fStart <= 1);
			fCharOffsets.clear();
			fCharOffsets.push_back( fNodeLayout->GetLayoutBounds().GetWidth());
			return fCharOffsets;
		}

		bool needReleaseGC = false; 
		VGraphicContext *gc = fLine->fParagraph->fParaLayout->fGC;
		if (!gc)
		{
			//might happen if called later while requesting run or caret metrics 
			gc = fLine->fParagraph->fParaLayout->_RetainComputingGC( fLine->fParagraph->fParaLayout->fGC);
			xbox_assert(gc);
			gc->BeginUsingContext(true);
			needReleaseGC = true;
		}

		xbox_assert(gc && fFont);

		VFontMetrics *metrics = new VFontMetrics( fFont, gc);

		VString runtext;
		VString *textPtr = &(fLine->fParagraph->fParaLayout->fText);
		textPtr->GetSubString( fStart+1, fEnd-fStart, runtext);

		//replace CR by whitespace to ensure correct offset for end of line character 
		if (runtext.GetUniChar(runtext.GetLength()) == 13)
			runtext.SetUniChar(runtext.GetLength(), ' ');

		fCharOffsets.reserve(runtext.GetLength());
		fCharOffsets.clear();

		metrics->MeasureText( runtext, fCharOffsets, fDirection == kPARA_RTL);	//returned offsets should be also absolute offsets from start position (which must be anchored to the right with RTL, and left with LTR)
		while (fCharOffsets.size() < fEnd-fStart)
			fCharOffsets.push_back(fCharOffsets.back()); 
		
		CharOffsets::iterator itOffset = fCharOffsets.begin();
		for (;itOffset != fCharOffsets.end(); itOffset++)
			*itOffset = RoundMetrics( gc, *itOffset);
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
void VDocParagraphLayout::Run::PreComputeMetrics(bool inIgnoreTrailingSpaces)
{
	VGraphicContext *gc = fLine->fParagraph->fParaLayout->fGC;
	xbox_assert(gc && fFont);

	if (fNodeLayout)
	{
		//run is a embedded object (image for instance)

		fAscent = fDescent = 0.0001; //it is computed later in Line::ComputeMetrics as embedded node vertical alignment is dependent on line text runs metrics (HTML compliant)
									 //(we do not set to 0 as it needs to be not equal to 0 from here for fTypoBounds->IsEmpty() not equal to true - otherwise it would screw up line metrics computing)
		GReal width = fNodeLayout->GetLayoutBounds().GetWidth();
		
		fCharOffsets.clear();
		fCharOffsets.push_back( width);

		fTypoBounds.SetWidth( width);
		fTypoBoundsWithTrailingSpaces.SetWidth(width);
		fTypoBoundsWithKerning.SetWidth( width);

		fEndPos = fStartPos;
		fEndPos.x += width;

		fIsLeadingSpace = false;
		return;
	}

	VFontMetrics *metrics = new VFontMetrics( fFont, gc);
	//ensure rounded ascent & ascent+descent is greater or equal to design ascent & ascent+descent
	//(as we use ascent to compute line box y origin from y baseline & ascent+descent to compute line box height)
	fAscent = RoundMetrics(gc, metrics->GetAscent());
	fDescent = RoundMetrics(gc, metrics->GetAscent()+metrics->GetDescent())-fAscent;
	xbox_assert(fDescent >= 0);
	
	VString *textPtr = &(fLine->fParagraph->fParaLayout->fText);
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
		sLONG paraDirection = fLine->fParagraph->fDirection;
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
		fCharOffsets.clear();
		fCharOffsets.push_back( width);
	}
	else if (fEnd > fStart)
		width = RoundMetrics( gc, metrics->GetTextWidth( runtext, fDirection == kPARA_RTL)); //typo width
	else
		width = 0;

	GReal widthWithKerning;
	bool isLastRun = fIndex+1 >= fLine->fRuns.size();
	const Run *nextRun = NULL;
	if (!isLastRun)
		nextRun = fLine->fRuns[fIndex+1];
	sLONG runTextLength = runtext.GetLength();
	if (nextRun
		&& !endOfLine 
		&& !isTab 
		&& !inIgnoreTrailingSpaces 
		&& fEnd > fStart 
		&& nextRun->GetDirection() == fDirection
		&& nextRun->GetBidiVisualIndex() == fBidiVisualIndex
		)
	{
		//next run has the same direction & the same bidi visual index -> append next run first char (beware of surrogates so 2 code points if possible) to run text in order to determine next run start offset = this run end offset & take account kerning
		
		//add next two code points (for possible surrogates)
		VString nextRunText;
		if (fEnd+2 <= paraEnd)
			textPtr->GetSubString( fEnd+1, 2, nextRunText);
		else
			textPtr->GetSubString( fEnd+1, 1, nextRunText);
		runtext.AppendString(nextRunText);

		fCharOffsets.reserve(runtext.GetLength());
		fCharOffsets.clear();
		metrics->MeasureText( runtext, fCharOffsets, fDirection == kPARA_RTL);	//returned offsets are absolute offsets from start position & not from left border (for RTL)

		CharOffsets::iterator itOffset = fCharOffsets.begin();
		for (;itOffset != fCharOffsets.end(); itOffset++)
			*itOffset = RoundMetrics( gc, *itOffset); //width with kerning so round to nearest integral (GDI) or TWIPS boundary (other sub-pixel positionning capable context)

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
		fTypoBoundsWithTrailingSpaces.SetWidth(widthWithKerning);

	fEndPos = fStartPos;
	fEndPos.x += widthWithKerning;

	fTypoBoundsWithKerning.SetWidth( widthWithKerning);

	if (inIgnoreTrailingSpaces)
		fIsLeadingSpace = width == 0.0f && (fIndex == 0 || fLine->fRuns[fIndex-1]->fIsLeadingSpace);

	if (isTab && !inIgnoreTrailingSpaces)
		//store res-independent tab offset (with TWIPS precision - passing NULL gc)
		//remark: it is used to recalc after the offset when DPI is changed (so it is always accurate with the DPI and gc glyph positionning)
		fTabOffset = RoundMetrics(NULL,fEndPos.x*72/fLine->fParagraph->fCurDPI);

	delete metrics;
}



/** compute final metrics according to bidi */
void VDocParagraphLayout::Run::ComputeMetrics(GReal& inBidiCurDirStartOffset)
{
	eDirection paraDirection = fLine->fParagraph->fDirection;

	//if paragraph direction (line direction) is LTR, we compute metrics from the first visual run to the last
	//if paragraph direction (line direction) is RTL, we compute metrics from the last visual run to the first

	if (fDirection == kPARA_LTR) 
	{
		//run is LTR

		if (paraDirection == kPARA_RTL)
			inBidiCurDirStartOffset -= fTypoBoundsWithKerning.GetWidth();

		fStartPos.x = inBidiCurDirStartOffset;
		fEndPos.x = inBidiCurDirStartOffset+fTypoBoundsWithKerning.GetWidth();

		fTypoBoundsWithKerning.SetCoords( inBidiCurDirStartOffset, -fAscent, fTypoBoundsWithKerning.GetWidth(), fAscent+fDescent);
		fTypoBoundsWithTrailingSpaces.SetCoords( inBidiCurDirStartOffset, -fAscent, fTypoBoundsWithTrailingSpaces.GetWidth(), fAscent+fDescent);
		fTypoBounds.SetCoords( inBidiCurDirStartOffset, -fAscent, fTypoBounds.GetWidth(), fAscent+fDescent);

		if (paraDirection == kPARA_LTR)
			inBidiCurDirStartOffset = fEndPos.x;
	}
	else //run is RTL
	{
		if (paraDirection == kPARA_LTR)
			inBidiCurDirStartOffset += fTypoBoundsWithKerning.GetWidth();

		fStartPos.x = inBidiCurDirStartOffset;
		fEndPos.x = inBidiCurDirStartOffset-fTypoBoundsWithKerning.GetWidth();
		
		fTypoBoundsWithKerning.SetCoords( inBidiCurDirStartOffset-fTypoBoundsWithKerning.GetWidth(), -fAscent, fTypoBoundsWithKerning.GetWidth(), fAscent+fDescent);
		fTypoBoundsWithTrailingSpaces.SetCoords( inBidiCurDirStartOffset-fTypoBoundsWithTrailingSpaces.GetWidth(), -fAscent, fTypoBoundsWithTrailingSpaces.GetWidth(), fAscent+fDescent);
		fTypoBounds.SetCoords( inBidiCurDirStartOffset-fTypoBounds.GetWidth(), -fAscent, fTypoBounds.GetWidth(), fAscent+fDescent);
		
		if (paraDirection == kPARA_RTL)
			inBidiCurDirStartOffset = fEndPos.x;
	}
}

VDocParagraphLayout::Line::Line(VParagraph *inParagraph, const sLONG inIndex, const sLONG inStart, const sLONG inEnd)
{
	fParagraph = inParagraph;
	fIndex = inIndex;
	fStart = inStart;
	fEnd = inEnd;
	fAscent = fDescent = fLineHeight = 0;
}
VDocParagraphLayout::Line::Line(const Line& inLine)
{
	fParagraph = inLine.fParagraph;
	fIndex = inLine.fIndex;
	fStart = inLine.fStart;
	fEnd = inLine.fEnd;
	fStartPos = inLine.fStartPos;
	fRuns = inLine.fRuns;
	fVisualRuns = inLine.fVisualRuns;
	fAscent = inLine.fAscent;
	fDescent = inLine.fDescent;
	fLineHeight = inLine.fLineHeight;
	fTypoBounds = inLine.fTypoBounds;
	fTypoBoundsWithTrailingSpaces = inLine.fTypoBoundsWithTrailingSpaces;
}
VDocParagraphLayout::Line& VDocParagraphLayout::Line::operator=(const Line& inLine)
{
	if (this == &inLine)
		return *this;

	fParagraph = inLine.fParagraph;
	fIndex = inLine.fIndex;
	fStart = inLine.fStart;
	fEnd = inLine.fEnd;
	fStartPos = inLine.fStartPos;
	fRuns = inLine.fRuns;
	fVisualRuns = inLine.fVisualRuns;
	fAscent = inLine.fAscent;
	fDescent = inLine.fDescent;
	fLineHeight = inLine.fLineHeight;
	fTypoBounds = inLine.fTypoBounds;
	fTypoBoundsWithTrailingSpaces = inLine.fTypoBoundsWithTrailingSpaces;
	return *this;
}

VDocParagraphLayout::Line::~Line()
{
	Runs::const_iterator itRun = fRuns.begin();
	for(;itRun != fRuns.end(); itRun++)
		delete (*itRun);
}

/** for run visual order sorting */
bool VDocParagraphLayout::Line::CompareVisual(Run* inRun1, Run* inRun2)
{
	//sort first using bidi visual order 
	if (inRun1->fBidiVisualIndex < inRun2->fBidiVisualIndex)
		return true;
	if (inRun1->fBidiVisualIndex > inRun2->fBidiVisualIndex)
		return false;
	//runs have same bidi visual order (which means they belong to the same bidi run and so have the same direction - but have different style) 
	xbox_assert( inRun1->fDirection == inRun2->fDirection);
	if (inRun1->fDirection == kPARA_RTL)
		//if runs are RTL, the run greatest in logical order comes first
		return inRun1->fIndex > inRun2->fIndex;
	else
		//if runs are LTR, the run lowest in logical order comes first
		return inRun1->fIndex < inRun2->fIndex;
}

/** compute metrics according to start pos, start & end indexs & run info */
void VDocParagraphLayout::Line::ComputeMetrics()
{
	//first compute runs final metrics...

	//build visual ordered runs 
	fVisualRuns = fRuns;
	if (fVisualRuns.size() > 1)
		std::sort(fVisualRuns.begin(), fVisualRuns.end(), CompareVisual);
	
	//parse runs to adjust typo bounds with or without trailing spaces 
	//and to adjust whitespace runs width (if justification is required)
	bool wordwrap = fParagraph->fWordWrap;
	bool justify = wordwrap && fParagraph->fParaLayout->GetTextAlign() == AL_JUST && fIndex+1 < fParagraph->fLines.size(); //justify only lines but the last one
	bool isParaRTL = fParagraph->fDirection == kPARA_RTL; 
	Runs runsWhiteSpace;
	bool addRunsWhitespace = justify;
	GReal lineWidthWords = 0.0f;
	GReal lineWidthSpaces = 0.0f;
	GReal startIndent = 0.0f;
	if (fParagraph->fTextIndent != 0)
	{
		if (fParagraph->fTextIndent > 0)
		{
			//indent first line
			if (fIndex == 0)
				startIndent = fParagraph->fTextIndent;
		}
		else
		{
			//indent second line, third line, ...
			if (fIndex > 0)
				startIndent = -fParagraph->fTextIndent;
		}
	}
	GReal startOffset = startIndent;
	if (fVisualRuns.size() > 1)
	{
		//we need to take account paragraph direction as it dictates on which side of the line spaces are trimmed 
		if (isParaRTL)
		{
			//RTL paragraph direction 
			Runs::iterator itRunWrite;
			bool isTrailingSpaceRun = true;
			for (itRunWrite = fVisualRuns.begin();itRunWrite != fVisualRuns.end(); itRunWrite++)
			{
				Run *run = *itRunWrite;
				if (!isTrailingSpaceRun)
				{
					if (addRunsWhitespace)
					{
						if (run->IsTab() || run->IsLeadingSpace())
						{
							addRunsWhitespace = false; //cannot justify beyond the last tab or leading space (in direction order)
							//FIXME: maybe we should recalc tab offset with _ApplyTab because visual ordering might have decaled the offset computed by VParagraph (if complex bidi)
							startOffset = run->GetEndPos().GetX(); //decal offset to last tab or leading space end pos
						}
						else if (run->fTypoBounds.GetWidth() == 0.0f)
						{
							xbox_assert(run->fEnd == run->fStart+1); //for justify, each run (but trailing or leading) should be only one space character size
							lineWidthSpaces += run->fTypoBoundsWithTrailingSpaces.GetWidth();
							runsWhiteSpace.push_back( run);
						}
						else
							lineWidthWords += run->fTypoBoundsWithTrailingSpaces.GetWidth();
					}
					run->fTypoBounds = run->fTypoBoundsWithTrailingSpaces;
				}
				else if (run->fTypoBounds.GetWidth() != 0)
				{
					isTrailingSpaceRun = false; //this run has a width without trailing spaces not null -> it contains characters but spaces 
												//-> for runs before this one in paragraph direction order, use only typo bounds with trailing spaces
												//(to ensure proper bounds union for LTR or RTL line metrics)
												//and start to scan for runs to justify
					if (addRunsWhitespace)
					{
						if (run->IsTab())
							addRunsWhitespace = false;
						else
							lineWidthWords += run->fTypoBounds.GetWidth();
					}
				}
			}
		}
		else 
		{
			//LTR paragraph direction (reverse parse)
			Runs::reverse_iterator itRunWrite;
			bool isTrailingSpaceRun = true;
			for (itRunWrite = fVisualRuns.rbegin();itRunWrite != fVisualRuns.rend(); itRunWrite++)
			{
				Run *run = *itRunWrite;
				if (!isTrailingSpaceRun)
				{
					if (addRunsWhitespace)
					{
						if (run->IsTab() || run->IsLeadingSpace())
						{
							addRunsWhitespace = false; //cannot justify beyond the last tab or leading space (in direction order)
							//FIXME: maybe we should recalc tab offset with _ApplyTab because visual ordering might have decaled the offset computed by VParagraph (if complex bidi)
							startOffset = run->GetEndPos().GetX(); //decal offset to last tab or leading space end pos
						}
						else if (run->fTypoBounds.GetWidth() == 0.0f)
						{
							xbox_assert(run->fEnd == run->fStart+1); //for justify, each run (but trailing or leading) should be only one space character size
							lineWidthSpaces += run->fTypoBoundsWithTrailingSpaces.GetWidth();
							runsWhiteSpace.push_back( run);
						}
						else
							lineWidthWords += run->fTypoBoundsWithTrailingSpaces.GetWidth();
					}
					run->fTypoBounds = run->fTypoBoundsWithTrailingSpaces;
				}
				else if (run->fTypoBounds.GetWidth() != 0)
				{
					isTrailingSpaceRun = false; //this run has a width without trailing spaces not null -> it contains characters but spaces 
												//-> for runs before this one in paragraph direction order, use only typo bounds with trailing spaces
												//(to ensure proper bounds union for LTR or RTL line metrics)
												//and start to scan for runs to justify
					if (addRunsWhitespace)
					{
						if (run->IsTab())
							addRunsWhitespace = false;
						else
							lineWidthWords += run->fTypoBounds.GetWidth();
					}
				}
			}
		}
	}
	
	VGraphicContext *gc = fParagraph->fParaLayout->fGC;
	xbox_assert(gc);

	if (lineWidthWords > 0 && runsWhiteSpace.size() > 0 && fParagraph->fMaxWidth > startOffset+lineWidthWords+lineWidthSpaces)
	{
		//justification: compute final whitespace runs width 
		sLONG whiteSpaceWidth;
		sLONG modWhiteSpaceWidth;
		if (gc->IsFontPixelSnappingEnabled())
		{
			//context is not sub-pixel positioning capable: round whitespace width to integer precision (deal with division error by distributing modulo to whitespace runs)

			sLONG value = fParagraph->fMaxWidth-startOffset-lineWidthWords-lineWidthSpaces;
			whiteSpaceWidth = value / runsWhiteSpace.size();
			modWhiteSpaceWidth = value % runsWhiteSpace.size();

			if (whiteSpaceWidth*runsWhiteSpace.size()+modWhiteSpaceWidth > 0) //ensure we do not fall below minimal whitespace run width
			{
				Runs::const_iterator itRun = runsWhiteSpace.begin();
				for(; itRun != runsWhiteSpace.end(); itRun++)
				{
					Run *run = *itRun;
					GReal width = whiteSpaceWidth+run->fTypoBoundsWithTrailingSpaces.GetWidth();
					if (modWhiteSpaceWidth)
					{
						width++; 
						modWhiteSpaceWidth--;
					}
					run->fTypoBounds.SetWidth( width);
					run->fTypoBoundsWithTrailingSpaces.SetWidth( width);
					run->fTypoBoundsWithKerning.SetWidth( width);
					run->fCharOffsets.clear();
					run->fCharOffsets.push_back(width);
				}
			}
		}
		else
		{
			//context is sub-pixel positioning capable: round whitespace width to twips precision

			sLONG value = ICSSUtil::PointToTWIPS(fParagraph->fMaxWidth-startOffset-lineWidthWords-lineWidthSpaces);
			whiteSpaceWidth = value / runsWhiteSpace.size();
			modWhiteSpaceWidth = value % runsWhiteSpace.size();

			if (whiteSpaceWidth*runsWhiteSpace.size()+modWhiteSpaceWidth > 0) //ensure we do not fall below minimal whitespace run width
			{
				Runs::const_iterator itRun = runsWhiteSpace.begin();
				for(; itRun != runsWhiteSpace.end(); itRun++)
				{
					Run *run = *itRun;
					GReal width = whiteSpaceWidth;
					if (modWhiteSpaceWidth)
					{
						width++; 
						modWhiteSpaceWidth--;
					}
					width = ICSSUtil::TWIPSToPoint( width)+run->fTypoBoundsWithTrailingSpaces.GetWidth();

					run->fTypoBounds.SetWidth( width);
					run->fTypoBoundsWithTrailingSpaces.SetWidth( width);
					run->fTypoBoundsWithKerning.SetWidth( width);
					run->fCharOffsets.clear();
					run->fCharOffsets.push_back(width);
				}
			}
		}
	}

	//compute run final metrics for visual display 
	//(note: run start & end positions are anchored according to run direction - and not paragraph direction
	//		 but line is anchored according to paragraph direction)

	GReal bidiStartOffset = startIndent;
	if (isParaRTL)
	{
		//RTL: compute from last visual run to the first (x = bidiStartOffset = 0 -> line right border)
		bidiStartOffset = -bidiStartOffset; //reverse text-indent
		Runs::reverse_iterator itRunWrite = fVisualRuns.rbegin();
		for (;itRunWrite != fVisualRuns.rend(); itRunWrite++)
			(*itRunWrite)->ComputeMetrics( bidiStartOffset);
	}
	else
	{
		//LTR: compute from first visual run to the last (x = bidiStartOffset = 0 -> line left border)
		Runs::iterator itRunWrite = fVisualRuns.begin();
		for (;itRunWrite != fVisualRuns.end(); itRunWrite++)
			(*itRunWrite)->ComputeMetrics( bidiStartOffset);
	}

	//now we can compute line metrics

	fAscent = fDescent = fLineHeight = 0.0f;
	fTypoBounds.SetEmpty();
	fTypoBoundsWithTrailingSpaces.SetEmpty();

	xbox_assert(fRuns.size() > 0);
	fEnd = fRuns.back()->GetEnd(); //beware to use runs in logical order here...

	//iterate to compute proper typo bounds without trailing spaces & typo bounds including trailing spaces
	Runs runsWithNodeLayout;
	VectorOfFontMetrics fontMetrics;

	if (isParaRTL)
	{
		Run *runPrev = NULL;
		Runs::const_reverse_iterator itRunRead = fVisualRuns.rbegin();
		for (;itRunRead != fVisualRuns.rend(); itRunRead++)
		{
			Run *run = *itRunRead;

			fAscent = std::max( fAscent, run->GetAscent());
			fDescent = std::max( fDescent, run->GetDescent());

			if (run->GetNodeLayout())
			{
				if (runPrev)
					fontMetrics.push_back( FontMetrics(runPrev->fAscent, runPrev->fDescent));
				else
					fontMetrics.push_back( FontMetrics(run->fAscent, run->fDescent));
				runsWithNodeLayout.push_back( run);
			}

			//update bounds including trailing spaces
			if (itRunRead == fVisualRuns.rbegin())
				fTypoBoundsWithTrailingSpaces = run->GetTypoBounds(true);
			else if (!fTypoBoundsWithTrailingSpaces.IsEmpty() && !run->GetTypoBounds(true).IsEmpty())
					fTypoBoundsWithTrailingSpaces.Union( run->GetTypoBounds(true));
			
			//update bounds without trailing spaces
			if (itRunRead == fVisualRuns.rbegin())
				fTypoBounds = run->GetTypoBounds(false);
			else if (!fTypoBounds.IsEmpty() && (!run->GetTypoBounds(false).IsEmpty()))
				fTypoBounds.Union( run->GetTypoBounds(false));

			runPrev = run;
		}
	}
	else //LTR paragraph 
	{
		Run *runPrev = NULL;
		Runs::const_iterator itRunRead = fVisualRuns.begin();
		for (;itRunRead != fVisualRuns.end(); itRunRead++)
		{
			Run *run = *itRunRead;

			fAscent = std::max( fAscent, run->GetAscent());
			fDescent = std::max( fDescent, run->GetDescent());

			if (run->GetNodeLayout())
			{
				if (runPrev)
					fontMetrics.push_back( FontMetrics(runPrev->fAscent, runPrev->fDescent));
				else
					fontMetrics.push_back( FontMetrics(run->fAscent, run->fDescent));
				runsWithNodeLayout.push_back( run);
			}

			//update bounds including trailing spaces
			if (itRunRead == fVisualRuns.begin())
				fTypoBoundsWithTrailingSpaces = run->GetTypoBounds(true);
			else if (!fTypoBoundsWithTrailingSpaces.IsEmpty() && !run->GetTypoBounds(true).IsEmpty())
					fTypoBoundsWithTrailingSpaces.Union( run->GetTypoBounds(true));
			
			//update bounds without trailing spaces
			if (itRunRead == fVisualRuns.begin())
				fTypoBounds = run->GetTypoBounds(false);
			else if (!fTypoBounds.IsEmpty() && (!run->GetTypoBounds(false).IsEmpty()))
				fTypoBounds.Union( run->GetTypoBounds(false));
			
			runPrev = run;
		}
	}

	//for runs with embedded nodes, compute now ascent & descent (embedded nodes vertical alignment is dependent on line & text ascent & descent - excluding embedded nodes ascent & descent)
	if (runsWithNodeLayout.size())
	{
		Runs::const_iterator itRunNode = runsWithNodeLayout.begin();
		VectorOfFontMetrics::const_iterator itFontMetrics = fontMetrics.begin();
		GReal ascent = fAscent, descent = fDescent;
		for(;itRunNode != runsWithNodeLayout.end(); itRunNode++, itFontMetrics++)
		{
			//update run ascent & descent
			Run *run = (*itRunNode);

			IDocNodeLayout *nodeLayout = run->GetNodeLayout();
			nodeLayout->UpdateAscent( ascent, descent, itFontMetrics->first, itFontMetrics->second);
			run->fAscent = nodeLayout->GetAscent();
			run->fDescent = nodeLayout->GetDescent();
			run->fTypoBounds.SetY(-run->fAscent);
			run->fTypoBounds.SetHeight(run->fAscent+run->fDescent);
			run->fTypoBoundsWithTrailingSpaces.SetY(-run->fAscent);
			run->fTypoBoundsWithTrailingSpaces.SetHeight(run->fAscent+run->fDescent);
			run->fTypoBoundsWithKerning.SetY(-run->fAscent);
			run->fTypoBoundsWithKerning.SetHeight(run->fAscent+run->fDescent);

			//update line ascent & descent
			fAscent = std::max( fAscent, run->fAscent);
			fDescent = std::max( fDescent, run->fDescent);
		}
		fTypoBounds.SetY(-fAscent);
		fTypoBounds.SetHeight(fAscent+fDescent);
		fTypoBoundsWithTrailingSpaces.SetY(-fAscent);
		fTypoBoundsWithTrailingSpaces.SetHeight(fAscent+fDescent);
	}

	//update line height & finalize typo bounds

	fLineHeight = fParagraph->fParaLayout->GetLineHeight();
	if (fLineHeight >= 0)
		fLineHeight = RoundMetrics(gc, fLineHeight*fParagraph->fCurDPI/72);
	else
	{
		fLineHeight = (fAscent+fDescent)*(-fLineHeight);
		fLineHeight = RoundMetrics( gc, fLineHeight);
	}
	fTypoBounds.SetHeight( fLineHeight);
	fTypoBoundsWithTrailingSpaces.SetHeight( fLineHeight);
}


VDocParagraphLayout::VParagraph::VParagraph(VDocParagraphLayout *inParaLayout, const sLONG inIndex, const sLONG inStart, const sLONG inEnd):VObject(),IRefCountable()
{
	fParaLayout	= inParaLayout;
	fIndex	= inIndex;
	fStart	= inStart;
	fEnd	= inEnd;
	fMaxWidth = 0;
	fWordWrap = true;
	fDirection = kPARA_LTR;
	fWordWrapOverflow = false;
	fDirty	= true;
	fCurDPI = 72;
}

VDocParagraphLayout::VParagraph::~VParagraph()
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

void VDocParagraphLayout::VParagraph::_ApplyTab( GReal& pos, VFont *font)
{
	//compute tab absolute pos using TWIPS 

	VGraphicContext *gc = fParaLayout->fGC;
	xbox_assert(gc);

	xbox_assert(pos >= 0);
	sLONG abspos = ICSSUtil::PointToTWIPS(pos);
	sLONG taboffset = ICSSUtil::PointToTWIPS( fParaLayout->GetTabStopOffset()*fCurDPI/72);
	if (taboffset)
	{
		//compute absolute tab pos
		sLONG div = abspos/taboffset;
		GReal oldpos = pos;
		pos = RoundMetrics(gc, ICSSUtil::TWIPSToPoint( (div+1)*taboffset));  
		if (pos == oldpos) //might happen because of pixel snapping -> pick next tab
			pos = RoundMetrics(gc, ICSSUtil::TWIPSToPoint( (div+2)*taboffset));  
	}
	else
	{
		//if tab offset is 0 -> tab width = whitespace width
		VGraphicContext *gc = fParaLayout->fGC;
		xbox_assert(gc);
		VFontMetrics *metrics = new VFontMetrics( font, gc);
		pos += RoundMetrics(gc, metrics->GetCharWidth( ' '));
		delete metrics;
	}
}

void VDocParagraphLayout::VParagraph::_NextRun( Run* &run, Line* line, eDirection dir, sLONG bidiVisualIndex, const GReal inFontSize, VFont* font, VColor textColor, VColor backColor, GReal& lineWidth, sLONG i, bool inIsTab)
{
	xbox_assert( font);

	if (inIsTab && run->GetEnd() > run->GetStart()+1)
	{
		xbox_assert(false);
	}
	else
	{
		if (run->GetEnd() == run->GetStart())
		{
			//empty run: just update font, dir  & line width
			run->SetFontSize( inFontSize);
			run->SetFont( font);
			run->SetDirection ( dir);
			run->SetBidiVisualIndex( bidiVisualIndex);
			run->SetTextColor( textColor);
			run->SetBackColor( backColor);
			return;
		}

		//terminate run
		run->SetEnd( i);
		run->PreComputeMetrics(true);
		if (fWordWrap && run->GetTypoBounds(false).GetWidth() > 0)
			lineWidth = run->GetEndPos().GetX();
	}

	//now add new run
	if (i < fEnd)
	{
		run = new Run( line, run->GetIndex()+1, bidiVisualIndex, i, i, dir, inFontSize, font, &textColor, &backColor);
		line->thisRuns().push_back( run);

		Run *runPrev = line->thisRuns()[run->GetIndex()-1];
		runPrev->PreComputeMetrics(); //we precompute metrics after new run is added to ensure we take account bidiVisualIndex of previous and new run 
									  //(in order to compute proper run bounds with kerning)

		run->SetStartPos( VPoint( runPrev->GetEndPos().GetX(), 0));
		run->SetEndPos( VPoint( runPrev->GetEndPos().GetX(), 0));
	}
	else
		//last run
		run->PreComputeMetrics();
}

void VDocParagraphLayout::VParagraph::_NextRun( Run* &run, Line *line, GReal& lineWidth, sLONG i, bool isTab)
{
	_NextRun( run, line, run->GetDirection(), run->GetBidiVisualIndex(), run->GetFontSize(), run->GetFont(), run->GetTextColor(), run->GetBackColor(), lineWidth, i, isTab);
}

void VDocParagraphLayout::VParagraph::_NextRun( Run* &run, Line *line, eDirection dir, sLONG bidiVisualIndex, GReal& lineWidth, sLONG i, bool isTab)
{
	_NextRun( run, line, dir, bidiVisualIndex, run->GetFontSize(), run->GetFont(), run->GetTextColor(), run->GetBackColor(), lineWidth, i, isTab);
}

void VDocParagraphLayout::VParagraph::_NextRun( Run* &run, Line *line, eDirection dir, sLONG bidiVisualIndex, GReal& fontSize, VFont* &font, VTextStyle* style, GReal& lineWidth, sLONG i)
{
	xbox_assert( !style->GetFontName().IsEmpty());

	fontSize = style->GetFontSize() > 0.0f ? style->GetFontSize() : 12;
	ReleaseRefCountable(&font);
	font = VFont::RetainFont(		style->GetFontName(),
									 _TextStyleToFontFace(style),
									 fontSize,
									 fCurDPI); //fonts are pre-scaled by layout DPI (in order metrics to be computed at the desired DPI)
										  
	VColor textColor, backColor;
	if (style->GetHasForeColor())
		textColor.FromRGBAColor( style->GetColor());
	else
		textColor = run->fLine->fParagraph->fParaLayout->fDefaultTextColor;
	
	if (style->GetHasBackColor() && style->GetBackGroundColor() != RGBACOLOR_TRANSPARENT)
		backColor.FromRGBAColor( style->GetBackGroundColor());
	else
		backColor = VColor(0,0,0,0);

	_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, textColor, backColor, lineWidth, i, false);

	//bind span ref node layout to this run (if it is a embedded object)
	if (style->IsSpanRef() && testAssert(i < fEnd && style->GetSpanRef()->GetType() == kSpanTextRef_Image && (style->GetSpanRef()->ShowRef() & kSRTT_Value)))
		run->SetNodeLayout( dynamic_cast<IDocNodeLayout *>( style->GetSpanRef()->GetLayoutDelegate()));
}


void VDocParagraphLayout::VParagraph::_NextLine( Run* &run, Line* &line, sLONG i)
{
	xbox_assert(run == (line->thisRuns().back()));

	VGraphicContext *gc = fParaLayout->fGC;
	xbox_assert(gc);

	eDirection runDir = run->GetDirection();
	sLONG bidiVisualIndex = run->GetBidiVisualIndex();
	GReal runFontSize = run->GetFontSize();
	VFont *runFont = run->GetFont();
	VColor runTextColor = run->GetTextColor();
	VColor runBackColor = run->GetBackColor();
	IDocNodeLayout *runNodeLayout = run->GetNodeLayout();

	//terminate current line last run
	run->SetEnd(i);
	if (run->GetEnd() == run->GetStart() && testAssert(line->GetRuns().size() > 1))
	{
		line->thisRuns().pop_back();
		Run *run = line->thisRuns().back();
		run->PreComputeMetrics(true);
		run->PreComputeMetrics();
	}
	else
		run->PreComputeMetrics(); 

	//finalize current line
	Line *prevLine = line;

	//start next line (only if not at the end)
	if (i < fEnd)
	{
		line = new Line( this, line->GetIndex()+1, i, i);
		fLines.push_back( line);
		
		run = new Run(line, 0, bidiVisualIndex, i, i, runDir, runFontSize, runFont, &runTextColor, &runBackColor);
		run->SetNodeLayout( runNodeLayout);

		line->thisRuns().push_back( run);

		//take account negative line padding if any (lines but the first are decaled with -fParaLayout->GetTextIndent())
		if (fTextIndent < 0)
		{
			run->SetStartPos( VPoint( -fTextIndent, 0));
			run->SetEndPos( VPoint( -fTextIndent, 0));
		}
	}
	prevLine->ComputeMetrics();
}


/** extract font style from VTextStyle */
VFontFace VDocParagraphLayout::VParagraph::_TextStyleToFontFace( const VTextStyle *inStyle)
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

/** local typedef for map of image layout per run start index */
typedef std::map<sLONG, VDocImageLayout *> MapOfImageLayout;

void VDocParagraphLayout::VParagraph::ComputeMetrics(bool inUpdateBoundsOnly, bool inUpdateDPIMetricsOnly, const GReal inMaxWidth)
{
	if (fDirty)
	{
		inUpdateBoundsOnly = false;
		inUpdateDPIMetricsOnly = false;
		fMaxWidth = 0;
	}
	if (!inUpdateBoundsOnly)
	{
		inUpdateDPIMetricsOnly = false;
		fMaxWidth = 0;
	}

	VGraphicContext *gc = fParaLayout->fGC;
	xbox_assert(gc);
	
	if (fCurDPI != fParaLayout->fDPI)
		fMaxWidth = 0;
	fCurDPI = fParaLayout->fDPI;

	const UniChar *textPtr = fParaLayout->fText.GetCPointer()+fStart;
	if (!inUpdateBoundsOnly)
	{
		//determine bidi 

		fDirection = fParaLayout->IsRTL() ? kPARA_RTL : kPARA_LTR; //it is desired base direction 
		VIntlMgr::GetDefaultMgr()->GetBidiRuns( textPtr, fEnd-fStart, fBidiRuns, fBidiVisualToLogicalMap, (sLONG)fDirection, true);
		
		//compute sequential uniform styles on paragraph range (& store locally to speed up update later)

		Styles::iterator itStyle = fStyles.begin();
		for (;itStyle != fStyles.end(); itStyle++)
		{
			if (*itStyle)
				delete *itStyle;
		}
		fStyles.clear();
		if (fParaLayout->fStyles || fParaLayout->fExtStyles)
		{
			VTreeTextStyle *layoutStyles = fParaLayout->fStyles ? fParaLayout->fStyles->CreateTreeTextStyleOnRange( fStart, fEnd, false, fEnd > fStart) :
										   fParaLayout->fExtStyles->CreateTreeTextStyleOnRange( fStart, fEnd, false, fEnd > fStart);
			if (layoutStyles)
			{
				MapOfImageLayout mapImageLayout;

				if (layoutStyles->HasSpanRefs())
				{
					//ensure we merge span style with span reference style & discard span reference (only local copy is discarded)
					for (int i = 1; i <= layoutStyles->GetChildCount(); i++)
					{
						VTextStyle *style = layoutStyles->GetNthChild(i)->GetData();
						const VTextStyle *styleSource = NULL;
						if (style->IsSpanRef())
						{
							//we need to get the original span ref in order to get the style according to the proper mouse event status
							//(as CreateTreeTextStyleOnRange does not preserve mouse over status - which is dependent at runtime on control events)
							sLONG rangeStart, rangeEnd;
							style->GetRange( rangeStart, rangeEnd);
							styleSource = fParaLayout->GetStyleRefAtRange( rangeStart, rangeEnd);
							const VTextStyle *styleSpanRef = testAssert(styleSource) ? styleSource->GetSpanRef()->GetStyle() : NULL;
							if (styleSpanRef)
								style->MergeWithStyle( styleSpanRef);
							if (styleSource->GetSpanRef()->GetType() == kSpanTextRef_Image && (styleSource->GetSpanRef()->ShowRef() & kSRTT_Value))
							{
								//create image layout & store to temporary map
										
								VDocImageLayout *imageLayout = NULL;
								if (styleSource->GetSpanRef()->GetLayoutDelegate())
								{
									//retain source span ref image layout
									imageLayout = dynamic_cast<VDocImageLayout*>(styleSource->GetSpanRef()->GetLayoutDelegate());
									xbox_assert(imageLayout);
									imageLayout->Retain();
								}
								else
								{
									//create new image layout & store it in source span ref 
									imageLayout = new VDocImageLayout( styleSource->GetSpanRef()->GetImage());
									imageLayout->Retain();
									styleSource->GetSpanRef()->SetLayoutDelegate( static_cast<IDocSpanTextRefLayout *>(imageLayout));
								}
								imageLayout->SetDPI( fCurDPI);
								imageLayout->UpdateLayout( gc);
								mapImageLayout[rangeStart] = imageLayout;
							}
							else
								style->SetSpanRef(NULL); //discard span ref if not a image (useless now)						
						}
					}
				}

				if (fParaLayout->fExtStyles && fParaLayout->fStyles)
					layoutStyles->ApplyStyles( fParaLayout->fExtStyles);

				VTextStyle *style = CreateTextStyle( fParaLayout->fDefaultFont, 72);
				style->SetRange(fStart, fEnd);
				VTreeTextStyle *layoutStylesWithDefault = new VTreeTextStyle( style);
				if (layoutStylesWithDefault)
				{
					layoutStylesWithDefault->AddChild( layoutStyles);
					layoutStylesWithDefault->BuildSequentialStylesFromCascadingStyles( fStyles, NULL, false);
				}
				ReleaseRefCountable(&layoutStyles);
				ReleaseRefCountable(&layoutStylesWithDefault);

				//attach now previously build image layout(s) to the remaining span image references
				MapOfImageLayout::iterator itImageLayout = mapImageLayout.begin();
				if (itImageLayout != mapImageLayout.end())
				{
					Styles::const_iterator itStyle = fStyles.begin();
					for (;itStyle != fStyles.end(); itStyle++)
					{
						VDocSpanTextRef *spanRef = (*itStyle)->GetSpanRef();
						if (spanRef)
						{
							xbox_assert( spanRef->GetType() == kSpanTextRef_Image);

							sLONG start, end;
							(*itStyle)->GetRange( start, end);
							xbox_assert( start == itImageLayout->first);

							//bind image layout to the span reference
							VDocImageLayout *imageLayout = itImageLayout->second;
							spanRef->SetLayoutDelegate( static_cast<IDocSpanTextRefLayout *>(imageLayout));
							
							itImageLayout++;
							if (itImageLayout == mapImageLayout.end())
								break;
						}
					}
					xbox_assert(itImageLayout == mapImageLayout.end());
				}
			}
		}
	}

	fWordWrap = inMaxWidth && (!(fParaLayout->GetLayoutMode() & TLM_DONT_WRAP));
	GReal maxWidth = floor(inMaxWidth);

	if (fLines.empty() || !inUpdateBoundsOnly || (!inUpdateDPIMetricsOnly && fWordWrap && maxWidth != fMaxWidth))
	{
		//compute lines: take account styles, direction, line breaking (if max width != 0), tabs & text indent to build lines & runs

		Lines::const_iterator itLine = fLines.begin();
		for (;itLine != fLines.end(); itLine++)
			delete (*itLine);
		fLines.clear();

		fMaxWidth = maxWidth;
		fTextIndent = RoundMetrics(gc, fParaLayout->GetTextIndent()*fCurDPI/72);

		//backup default font
		GReal fontSizeBackup = fParaLayout->fDefaultFont->GetPixelSize();
		VFont *fontBackup = fParaLayout->RetainDefaultFont( fCurDPI);
		xbox_assert(fontBackup);
		GReal fontSize = fontSizeBackup;
		VFont *font = RetainRefCountable( fontBackup);

		//parse text 
		const UniChar *c = textPtr;
		VIndex lenText = fEnd-fStart;

		sLONG bidiVisualIndex = fDirection == kPARA_RTL ? 1000000 : -1;

		Line *line = new Line( this, 0, fStart, fStart);
		fLines.push_back( line);
		
		Run *run = new Run(line, 0, bidiVisualIndex, fStart, fStart, fDirection, fontSize, font, &(fParaLayout->fDefaultTextColor));
		line->thisRuns().push_back( run);

		if (!lenText)
		{
			//empty text

			//take account text indent if any
			GReal lineWidth = 0;
			if (fTextIndent > 0)
			{
				lineWidth = fTextIndent;
				run->SetStartPos( VPoint( fTextIndent, 0));
				run->SetEndPos( VPoint( fTextIndent, 0));
			}

			if (fStyles.size())
			{
				//even if empty, we need to apply style if any (as run is empty, _NextRun does not create new run but only updates current run)
				_NextRun( run, line, fDirection, bidiVisualIndex, fontSize, font, fStyles[0], lineWidth, 0);
			}
			run->PreComputeMetrics();
			line->ComputeMetrics();
		}
		else
		{
			bool justify = fWordWrap && fParaLayout->GetTextAlign() == AL_JUST;

			VectorOfStringSlice words;
			if (fWordWrap)
			{
				//word-wrap: get text words (using ICU line breaking rule)
				VString text;
				fParaLayout->fText.GetSubString( fStart+1, lenText, text);

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
			
			sLONG startLine = fStart;
			sLONG endLine = fStart;
			GReal lineWidth = 0.0f;

			sLONG runsCountBackup = 0;
			sLONG dirIndexBackup = -1;
			sLONG bidiVisualIndexBackup = bidiVisualIndex;

			sLONG styleIndexBackup = -1;

			sLONG endLineBackup = 0;
			GReal lineWidthBackup = 0.0f;

			fWordWrapOverflow = false;

			if (!fBidiRuns.empty())
			{
				dirIndex = 0;
				dirStart = fStart+fBidiRuns[0].fStart;
				dirEnd = fStart+fBidiRuns[0].fEnd;
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
			if (fTextIndent > 0)
			{
				lineWidth = fTextIndent;
				run->SetStartPos( VPoint( fTextIndent, 0));
				run->SetEndPos( VPoint( fTextIndent, 0));
			}

			bool isPreviousValidForLineBreak = true;
			bool isValidForLineBreak = true;
			bool wasSpaceChar = false;
			bool isSpaceChar = false;
			for (int i = fStart; i < fEnd+1; i++, c++, wasSpaceChar = isSpaceChar)
			{
				isValidForLineBreak = i <= wordStart || i >= wordEnd;
				isSpaceChar = i >= fEnd || VIntlMgr::GetDefaultMgr()->IsSpace( *c);
				
				endLine = i;
				run->SetEnd( i);
				run->SetEndPos( run->GetStartPos());

				if (i > fStart && *(c-1) == '\t')
				{
					//we use one run only per tab (as run width is depending both on tab offset(s) & last run offset) so terminate tab run and start new one
					if (dirIndex >= 0)
					{
						//restore direction
						dir = (fBidiRuns[dirIndex].fBidiLevel & 1) ? kPARA_RTL : kPARA_LTR;
						bidiVisualIndex = fBidiRuns[dirIndex].fVisualIndex;
					}
					_NextRun( run, line, dir, bidiVisualIndex, lineWidth, i, true);
					includeLeadingSpaces = true;
					isValidForLineBreak = true;
					isPreviousValidForLineBreak = false;
				}

				//if direction is modified, terminate run & start new run
				if (dirIndex >= 0 && ((run->GetStart() == run->GetEnd()) || i < fEnd))
				{
					if (i == dirStart)
					{
						//start new run
						
						//set new direction
						dir = (fBidiRuns[dirIndex].fBidiLevel & 1) ? kPARA_RTL : kPARA_LTR;
						bidiVisualIndex = fBidiRuns[dirIndex].fVisualIndex;
						_NextRun( run, line, dir, bidiVisualIndex, lineWidth, i);
					}
					if (i == dirEnd && i < fEnd)
					{
						//start new run

						dirIndex++;
						if (dirIndex >= fBidiRuns.size())
						{
							dirIndex = -1;
							//rollback to default direction
							dir = fDirection;
							if (fDirection == kPARA_RTL)
								bidiVisualIndex = -1;
							else
								bidiVisualIndex++;
						}
						else
						{
							dirStart = fStart+fBidiRuns[dirIndex].fStart;
							dirEnd = fStart+fBidiRuns[dirIndex].fEnd;
							if (i == dirStart)
							{
								//set new direction
								dir = (fBidiRuns[dirIndex].fBidiLevel & 1) ? kPARA_RTL : kPARA_LTR;
								bidiVisualIndex = fBidiRuns[dirIndex].fVisualIndex;
							}
							else
							{
								xbox_assert(false); //bidi runs should be always contiguous
								//rollback to default direction
								dir = fDirection;
							}
						}
						_NextRun( run, line, dir, bidiVisualIndex, lineWidth, i);
					}
				}

				bool isCharCR = *c == 13;
				if (i < fEnd && (run->GetEnd() > run->GetStart() || run->GetDirection() != fDirection) && (*c == '\t' || isCharCR))
				{
					//we use one run only per tab & end of line (as run width is depending both on tab offset(s) & last run offset) so start new run for tab or end of line
					
					//for CR, ensure proper direction according to paragraph direction 
					if (isCharCR)
					{
						dir = fDirection;
						dirIndex = -1; //end of line so terminate parsing direction table
						if (fDirection == kPARA_RTL)
							bidiVisualIndex = -1; //ensure CR comes first
						else
							bidiVisualIndex++; //ensure CR comes last
					}
					_NextRun( run, line, dir, bidiVisualIndex, lineWidth, i);
				}
				if (justify && includeLeadingSpaces && i < fEnd && run->GetEnd() > run->GetStart())
				{
					//for justification, we need one run per whitespace as whitespace runs width might be determined later from max width
					if (isSpaceChar || wasSpaceChar)
						_NextRun( run, line, lineWidth, i);
				}

				//if style is modified, terminate run & start new run
				if (styleIndex >= 0 && ((run->GetStart() == run->GetEnd()) || i < fEnd))
				{
					if (i == styleStart)
					{
						//start new run with new style
						VTextStyle *style = fStyles[styleIndex];
						_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, style, lineWidth, i); //here font is updated from style too
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
							fontSize = fontSizeBackup;
							_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, fParaLayout->fDefaultTextColor, VColor(0,0,0,0), lineWidth, i);
						}
						else
						{
							fStyles[styleIndex]->GetRange( styleStart, styleEnd);
							if (i == styleStart)
							{
								//start new run with new style
								VTextStyle *style = fStyles[styleIndex];
								_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, style, lineWidth, i); //here font is updated from style too
							}
							else
							{
								//rollback to default style
								CopyRefCountable(&font, fontBackup);
								fontSize = fontSizeBackup;
								_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, fParaLayout->fDefaultTextColor, VColor(0,0,0,0), lineWidth, i);
							}
						}
					}
				}

				if (isSpaceChar)
					isSpaceChar = run->GetNodeLayout() == NULL; //embedded object should not be assumed to be a whitespace

				bool endOfLine = false;
				if (i >= fEnd)
				{
					//close line: terminate run & line
					
					endOfLine = true;
					bool closeLine = true;
					run->PreComputeMetrics(true); //compute metrics without trailing spaces
					if (fWordWrap)
					{
						if (run->GetTypoBounds(false).GetWidth() > 0)
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
					}
				}

				if (fWordWrap && (endOfLine || (isValidForLineBreak && !isPreviousValidForLineBreak && !isCharCR)))
				{
					//word-wrap 

					if (!endOfLine && run->GetEnd() > run->GetStart())
					{
						run->PreComputeMetrics(true); //ignore trailing spaces
						if (run->GetTypoBounds(false).GetWidth() > 0)
							lineWidth = std::abs(run->GetEndPos().GetX());
					}
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
								dirStart = fStart+fBidiRuns[dirIndex].fStart;
								dirEnd = fStart+fBidiRuns[dirIndex].fEnd;
								dir = (fBidiRuns[dirIndex].fBidiLevel & 1) ? kPARA_RTL : kPARA_LTR;
								bidiVisualIndex = fBidiRuns[dirIndex].fVisualIndex;
							}
							else
							{
								dir = fDirection;
								bidiVisualIndex = bidiVisualIndexBackup;
							}

							if (runsCountBackup < line->GetRuns().size())
							{
								//restore run
								xbox_assert( runsCountBackup);
								if (runsCountBackup < line->GetRuns().size())
								{
									while (runsCountBackup < line->GetRuns().size())
										line->thisRuns().pop_back();
									run = (line->thisRuns().back());
									fontSize = run->GetFontSize();
									CopyRefCountable(&font, run->GetFont());
								}
							}
							run->SetEnd( endLine);
							run->PreComputeMetrics(true);
							xbox_assert(lineWidth <= fMaxWidth);

							xbox_assert(dir == run->GetDirection() && bidiVisualIndex == run->GetBidiVisualIndex()); 

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
							c = fParaLayout->fText.GetCPointer()+endLine;
							isSpaceChar = i >= fEnd || VIntlMgr::GetDefaultMgr()->IsSpace( *c);
							wasSpaceChar = i > fStart && VIntlMgr::GetDefaultMgr()->IsSpace( *(c-1));
						}
						else 
							fWordWrapOverflow = true;

						//close line: terminate run, update line & start next line
						_NextLine( run, line, i);
						
						//start new line
						if (run->GetNodeLayout())
							isSpaceChar = false;
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
						bidiVisualIndexBackup = bidiVisualIndex;
						runsCountBackup = line->GetRuns().size();
						styleIndexBackup = styleIndex;
						wordIndexBackup = wordIndex;
					}
				}
				if (isSpaceChar)
					isPreviousValidForLineBreak = isValidForLineBreak;
				else
				{
					isPreviousValidForLineBreak = false; //always reset if not whitespace - so if this char trailing is line-breakable, line width will be checked on next iteration for word-wrap 
														 //(workaround for ideographic where all chars are line-breakable - words have 1 character size)
					includeLeadingSpaces = true;
				}
			}
		}
		ReleaseRefCountable(&font);
		ReleaseRefCountable(&fontBackup);
	}
	else
	{
		fMaxWidth = maxWidth;

		if (inUpdateDPIMetricsOnly)
		{
			//only DPI has changed: just recalc run & line metrics according to new DPI

			fTextIndent = RoundMetrics(gc, fParaLayout->GetTextIndent()*fCurDPI/72);
			
			Lines::iterator itLine = fLines.begin();
			for(;itLine != fLines.end(); itLine++)
			{
				Line *line = (*itLine);
				line->fVisualRuns.clear();

				Runs::iterator itRun = line->fRuns.begin();
				GReal offset = fTextIndent;
				for(;itRun != line->fRuns.end(); itRun++)
				{
					Run *run = (*itRun);
					run->SetStartPos( VPoint(offset,0));
					run->SetEndPos( run->GetStartPos());
					run->fCharOffsets.clear();
					run->NotifyChangeDPI();
					if (run->IsTab())
					{
						GReal tabOffset = RoundMetrics(gc, run->fTabOffset*fCurDPI/72);
						if (tabOffset < offset) //might happen because of pixel snapping
							tabOffset = offset; 
						run->SetEndPos( VPoint(tabOffset,0));
						GReal width = tabOffset-offset;
						run->fTypoBounds.SetWidth( width);
						run->fTypoBoundsWithTrailingSpaces.SetWidth( width);
						run->fTypoBoundsWithKerning.SetWidth( width);
						run->fCharOffsets.clear();
						run->fCharOffsets.push_back(width);
					}
					else if (run->GetNodeLayout())
						run->PreComputeMetrics();
					else
					{
						run->PreComputeMetrics(true);
						run->PreComputeMetrics();
					}
					offset = run->GetEndPos().GetX();
				}

				line->ComputeMetrics();
			}
		}
	}

	//compute local metrics 

	fTypoBounds.SetEmpty();
	fTypoBoundsWithTrailingSpaces.SetEmpty();

	GReal layoutWidth = fMaxWidth; 

	//determine lines start position (only y here)
	GReal y = 0;
	Lines::iterator itLine = fLines.begin();
	for (;itLine != fLines.end(); itLine++)
	{
		y += (*itLine)->GetAscent();

		(*itLine)->SetStartPos( VPoint(0, y));

		y += (*itLine)->GetLineHeight()-(*itLine)->GetAscent();
	}
	GReal layoutHeight = y;

	//compute typographic bounds 
	//(relative to x = 0 & y = paragraph top origin)
	itLine = fLines.begin();
	for (;itLine != fLines.end(); itLine++)
	{
		//typo bounds without trailing spaces 
		VRect boundsTypo = (*itLine)->GetTypoBounds(false);
		boundsTypo.SetPosBy( 0, (*itLine)->GetStartPos().GetY());
		if ((*itLine)->fIndex == 0)
			fTypoBounds = boundsTypo;
		else 
			fTypoBounds.Union( boundsTypo);

		//typo bounds with trailing spaces
		boundsTypo = (*itLine)->GetTypoBounds(true);
		boundsTypo.SetPosBy( 0, (*itLine)->GetStartPos().GetY());
		if ((*itLine)->fIndex == 0)
			fTypoBoundsWithTrailingSpaces = boundsTypo;
		else 
			fTypoBoundsWithTrailingSpaces.Union( boundsTypo);
	}
	//xbox_assert(fTypoBounds.GetHeight() == layoutHeight-inMarginTop-inMarginBottom);

	if (!layoutWidth)
		layoutWidth = fTypoBounds.GetWidth();
	fLayoutBounds.SetCoords(0, 0, layoutWidth, layoutHeight);

	fDirty = false;
}


/** update text layout metrics */ 
void VDocParagraphLayout::UpdateLayout(VGraphicContext *inGC)
{
	xbox_assert(inGC);

	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}

	BeginUsingContext( inGC, true); //will call _UpdateLayout & _UpdateLayoutInternal
	EndUsingContext();
}

/** update text layout if it is not valid */
void VDocParagraphLayout::_UpdateLayout()
{
	if (!testAssert(fGC))
	{
		//layout is invalid if gc is NULL
		fLayoutIsValid = false;
		return;
	}

	if (fLayoutIsValid)
	{
		if (fLayoutBoundsDirty) //update only bounds (and maybe lines if wordwrap)
		{
			xbox_assert(fDefaultFont);
			fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fDefaultFont->IsTrueTypeFont();
			
			_UpdateLayoutInternal();
			
			fLayoutIsValid = true; 
			fLayerIsDirty = true;
			fSelIsDirty = true;
			fLayoutBoundsDirty = false;
		}
	}
	else
	{
		xbox_assert(fDefaultFont);
		fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fDefaultFont->IsTrueTypeFont();

		//set cur kerning
		fLayoutKerning = fGC->GetCharActualKerning();

		//set cur text rendering mode
		fLayoutTextRenderingMode = fGC->GetTextRenderingMode();

		//compute text layout according to VDocParagraphLayout settings

		bool displayPlaceHolder = false;
		bool releaseExtStyles = false;
		VTreeTextStyle *placeHolderStyles = NULL;
		if (fShouldRestoreText && fTextBackup.GetLength() == 0)							
		{
			//locally modify styles for placeholder

			displayPlaceHolder = true;
			if (fStyles)
				fStyles->ExpandAtPosBy(0, fPlaceHolder.GetLength());

			//override color using extended styles
			if (!fExtStyles)
			{
				fExtStyles = new VTreeTextStyle( new VTextStyle());
				releaseExtStyles = true;
			}
			fExtStyles->ExpandAtPosBy(0, fPlaceHolder.GetLength());
			placeHolderStyles = new VTreeTextStyle( new VTextStyle());
			placeHolderStyles->GetData()->SetRange(0, fPlaceHolder.GetLength());
			placeHolderStyles->GetData()->SetHasForeColor(true);
			placeHolderStyles->GetData()->SetColor( VTEXTLAYOUT_COLOR_PLACEHOLDER.GetRGBAColor());
			fExtStyles->AddChild(placeHolderStyles);
		}
		
		_BeginUsingStyles();
		
		_UpdateLayoutInternal();

		_EndUsingStyles();

		if (displayPlaceHolder)
		{
			//restore styles
			if (fExtStyles)
			{
				xbox_assert(placeHolderStyles);
				fExtStyles->RemoveChildAt( fExtStyles->GetChildCount());
				placeHolderStyles->Release();
				if (releaseExtStyles)
					ReleaseRefCountable(&fExtStyles);
				else
					fExtStyles->Truncate(0, fPlaceHolder.GetLength());
			}
			if (fStyles)
				fStyles->Truncate(0, fPlaceHolder.GetLength());
		}

		fLayoutIsValid = true; 
		fLayerIsDirty = true;
		fSelIsDirty = true;
		fLayoutBoundsDirty = false;
	}
}

void VDocParagraphLayout::_BeginUsingStyles()
{
	if (fExtStyles)
	{
		//attach extra styles to current styles
		if (fStyles == NULL)
			fStyles = fExtStyles;
		else
			fStyles->AddChild( fExtStyles);
	}
}

void VDocParagraphLayout::_EndUsingStyles()
{
	if (fExtStyles)
	{
		//detach extra styles from current styles
		if (fStyles == fExtStyles)
			fStyles = NULL;
		else
			fStyles->RemoveChildAt(fStyles->GetChildCount());
	}
}

/** update text layout metrics */ 
void VDocParagraphLayout::_UpdateLayoutInternal()
{
	if (fLayoutIsValid && !fLayoutBoundsDirty)
		return;

	xbox_assert(fGC);
	fGC->BeginUsingContext(true);

	bool layoutIsValid = fLayoutIsValid;
	_AdjustComputingDPI(fGC);
	fLayoutIsValid = layoutIsValid;

	sLONG widthTWIPS = _GetWidthTWIPS();
	sLONG heightTWIPS = _GetHeightTWIPS();

	GReal maxWidth		= widthTWIPS ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(widthTWIPS)*fDPI/72) : 0; 
	GReal maxHeight		= heightTWIPS ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(heightTWIPS)*fDPI/72) : 0; 

	fMargin.left		= RoundMetrics(fGC,fAllMargin.left); 
	fMargin.right		= RoundMetrics(fGC,fAllMargin.right); 
	fMargin.top			= RoundMetrics(fGC,fAllMargin.top); 
	fMargin.bottom		= RoundMetrics(fGC,fAllMargin.bottom); 


	//determine paragraph(s)

	//clear & rebuild

	if (!fLayoutIsValid)
	{
		fMaxWidthTWIPS = 0;

		fParagraphs.reserve(10);
		fParagraphs.clear();

		const UniChar *c = fText.GetCPointer();
		sLONG start = 0;
		sLONG i = start;

		sLONG index = 0;
		while (*c && i < fText.GetLength())
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
		if (IsLastChild(true))
		{
			//if instance is last child node, we should add a empty paragraph if last character is CR (otherwise we cannot select after last CR)
			if (i > start && (*(c-1) == 13))
			{
				VRefPtr<VParagraph> para( new VParagraph( this, index, i, i), false);
				fParagraphs.push_back( para);
			}
		}
	}

	//compute paragraph(s) metrics

	GReal typoWidth = 0.0f;
	GReal typoHeight = 0.0f;

	//determine if we recalc only run & line metrics because of DPI change
	bool updateDPIMetricsOnly = false;
	sLONG maxWidthTWIPS = ICSSUtil::PointToTWIPS(maxWidth*72/fDPI);
	if (fLayoutIsValid && !fGC->IsFontPixelSnappingEnabled())
	{
		if (fLayoutMode & TLM_DONT_WRAP)
		{
			updateDPIMetricsOnly = fMaxWidthTWIPS == 0;
			fMaxWidthTWIPS = 0;
		}
		else
		{
			if (fMaxWidthTWIPS == maxWidthTWIPS)
				updateDPIMetricsOnly = true;
			else
				fMaxWidthTWIPS = maxWidthTWIPS;
		}
	}
	else
		fMaxWidthTWIPS = (fLayoutMode & TLM_DONT_WRAP) != 0 ? 0 : maxWidthTWIPS;

	Paragraphs::iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		VParagraph *para = itPara->Get();
		para->ComputeMetrics(fLayoutIsValid, updateDPIMetricsOnly, maxWidth); 
		para->SetStartPos(VPoint(0,typoHeight));
		typoWidth = std::max(typoWidth, para->GetTypoBounds(false).GetWidth());
		typoHeight = typoHeight+para->GetLayoutBounds().GetHeight();
	}
	GReal layoutWidth = typoWidth;
	GReal layoutHeight = typoHeight;

	if (maxWidth) 
		layoutWidth = maxWidth;
	if (maxHeight)
		layoutHeight = maxHeight;

	_FinalizeTextLocalMetrics(	fGC, typoWidth, typoHeight, layoutWidth, layoutHeight, 
								fMargin, fRenderMaxWidth, fRenderMaxHeight, 
								fTypoBounds, fRenderTypoBounds, fRenderLayoutBounds, fRenderMargin);

	_SetLayoutBounds( fGC, fLayoutBounds); //here we pass bounds relative to fDPI
	
	fGC->EndUsingContext();
}

					
void VDocParagraphLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw)
{
	//apply vertical align offset
	GReal y = 0;
	if (fLayoutBounds.GetHeight() != fTypoBounds.GetHeight())
		switch (GetVerticalAlign())
		{
		case AL_BOTTOM:
			y = fLayoutBounds.GetHeight()-fTypoBounds.GetHeight(); 
			break;
		case AL_CENTER:
			y = fLayoutBounds.GetHeight()*0.5-fTypoBounds.GetHeight()*0.5;
			break;
		default:
			break;
		}
	VDocBaseLayout::_BeginLocalTransform( inGC, outCTM, inTopLeft, inDraw, VPoint(0,y)); 
}


/** get line start position relative to text box left top origin */
void VDocParagraphLayout::_GetLineStartPos( const Line* inLine, VPoint& outPos)
{
	outPos = inLine->GetStartPos(); //here it is relative to start of paragraph for y but 0 for x
	xbox_assert(outPos.GetX() == 0.0f); //should be 0 as we compute line x position only here
	GReal dx = 0, dy = 0;
	
	//adjust y 
	dy = inLine->fParagraph->GetStartPos().GetY()+fMargin.top;
	xbox_assert(inLine->fParagraph->GetStartPos().GetX() == 0.0f); 

	//adjust x 
	eDirection paraDirection = inLine->fParagraph->fDirection;
	AlignStyle align = GetTextAlign();
	if (align == AL_DEFAULT)
		align = paraDirection == kPARA_RTL ? AL_RIGHT : AL_LEFT;

	//if paragraph direction is RTL, bounds are negative (origin (where x=0) is anchored to start of RTL line which is line right border) otherwise positive
	//(it ensures proper alignment without trailing spaces with both directions)
	switch (align)
	{
	case AL_RIGHT:
		if (paraDirection == kPARA_RTL)
			dx = fLayoutBounds.GetWidth()-fMargin.right;
		else
			dx = fLayoutBounds.GetWidth()-fMargin.right-inLine->GetTypoBounds(false).GetRight();
		break;
	case AL_CENTER:
		{
		GReal layoutWidth = fLayoutBounds.GetWidth()-fMargin.left-fMargin.right;
		if (paraDirection == kPARA_RTL)
			dx = fMargin.left+RoundMetrics(fGC, (layoutWidth-inLine->GetTypoBounds(false).GetLeft())*0.5);
		else
			dx = fMargin.left+RoundMetrics(fGC, (layoutWidth-inLine->GetTypoBounds(false).GetRight())*0.5);
		}
		break;
	default: //AL_LEFT & AL_JUSTIFY
		if (paraDirection == kPARA_RTL)
			dx = fMargin.left-inLine->GetTypoBounds(false).GetLeft();
		else
			dx = fMargin.left;
		break;
	}
	outPos.SetPosBy( dx, dy);
}

VDocParagraphLayout::VParagraph*	VDocParagraphLayout::_GetParagraphFromCharIndex(const sLONG inCharIndex)
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

VDocParagraphLayout::Line* VDocParagraphLayout::_GetLineFromCharIndex(const sLONG inCharIndex)
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

VDocParagraphLayout::Run* VDocParagraphLayout::_GetRunFromCharIndex(const sLONG inCharIndex)
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

void VDocParagraphLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft, const GReal inOpacity)
{
	xbox_assert(inGC);

	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}

	BeginUsingContext(inGC); 

	if (!testAssert(fLayoutIsValid))
	{
		EndUsingContext();
		return;
	}

	VColor selPaintColor;
	if (fShouldPaintSelection)
	{
		if (fSelIsActive)
			selPaintColor = fSelActiveColor;
		else
			selPaintColor = fSelInactiveColor;
		if (inGC->IsGDIImpl())
			//GDI has no transparency support
			selPaintColor.SetAlpha(255);
	}

	if (!_BeginDrawLayer( inTopLeft))
	{
		//text has been refreshed from layer: we do not need to redraw text content
		_EndDrawLayer();
		EndUsingContext();
		return;
	}

	{
	StEnableShapeCrispEdges crispEdges( inGC);
	
	VAffineTransform ctm;
	_BeginLocalTransform( inGC, ctm, inTopLeft, true); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space pixel is mapped to fDPI (72 on default so on default 1px = 1pt)
	//disable shape printer scaling as for printing, we draw at printer dpi yet - design metrics are computed at printer dpi (mandatory only for GDI)
	bool shouldScaleShapesWithPrinterScale = inGC->ShouldScaleShapesWithPrinterScale(false);

	//apply transparency if appropriate
	_BeginLayerOpacity( inGC, inOpacity);

	//clip bounds used to clip only background color drawing on the left & right
	VRect boundsClipMargin;
	boundsClipMargin.SetLeft( floor(fMargin.left));
	GReal marginRight = fLayoutBounds.GetWidth()-fMargin.right;
	if (marginRight < 0)
		marginRight  = 0;
	boundsClipMargin.SetRight( floor(marginRight));
	boundsClipMargin.SetTop( floor(fMargin.top));
	GReal marginBottom = fLayoutBounds.GetHeight()-fMargin.bottom;
	if (marginBottom < 0)
		marginBottom = 0;
	boundsClipMargin.SetBottom(floor(marginBottom));

	//draw common layout decoration: borders, etc...
	//(as layout origin is sticked yet to 0,0 because of current transform, we do not need to translate)
	VDocBaseLayout::Draw( inGC, VPoint());

	//ensure text does not overflow border
	if (fBorderLayout)
	{
		inGC->SaveClip();
		VRect boundsClipContent( fMarginOut.left+fBorderWidth.left, fMarginOut.top+fBorderWidth.top,
								 fLayoutBounds.GetWidth()-fMarginOut.left-fBorderWidth.left-fMarginOut.right-fBorderWidth.right,
								 fLayoutBounds.GetHeight()-fMarginOut.top-fBorderWidth.top-fMarginOut.bottom-fBorderWidth.bottom);
		inGC->ClipRect( boundsClipContent);
	}

	VRect clipBounds;
	inGC->GetClipBoundingBox( clipBounds);
	clipBounds.NormalizeToInt();

	VColor textColor;
	inGC->GetTextColor( textColor);
	VFont *font = inGC->RetainFont();
	VColor fillColor = VColor(0,0,0,0);
	
	//determine if we pre-decal RTL start position or let gc do it (not necessary but it avoids a costly text width calc by gc with some impls)
	bool rtlAnchorPreDecal = !inGC->IsQuartz2DImpl() && (!inGC->IsD2DImpl() || ((inGC->GetTextRenderingMode() & TRM_LEGACY_ON) != 0));
	
	for (sLONG iPass = 0; iPass < 2; iPass++)
	{
		bool drawCharBackColor = iPass == 0;
		bool drawSelection = iPass == 1;
		bool drawText = iPass == 1;

		if (drawSelection && fShouldPaintSelection && fSelStart < fSelEnd)
		{
			//draw selection color: we paint sel color after char back color but before the text (Word & Mac-like)
			_EndLocalTransform( inGC, ctm, true);
			_DrawSelection( inTopLeft);
			_BeginLocalTransform( inGC, ctm, inTopLeft, true); 
			fillColor = VColor(0,0,0,0); //reset cur fill color
		}
		
		Paragraphs::const_iterator itPara = fParagraphs.begin();
		for (;itPara != fParagraphs.end(); itPara++)
		{
			//draw paragraph
			VParagraph *para = itPara->Get();
			if (fMargin.top+para->GetStartPos().GetY() >= clipBounds.GetBottom())
				break;
			if (fMargin.top+para->GetStartPos().GetY()+para->GetLayoutBounds().GetHeight() < clipBounds.GetY())
				continue;

			Lines::const_iterator itLine = para->GetLines().begin();
			for (;itLine != para->GetLines().end(); itLine++)
			{
				//draw line
				Line *line = *itLine;

				VPoint linePos;
				_GetLineStartPos( line, linePos);

				if (linePos.GetY()+line->GetTypoBounds().GetY() >= clipBounds.GetBottom())
					break;
				if (linePos.GetY()+line->GetTypoBounds().GetBottom() < clipBounds.GetY())
					continue;

				VPoint linePosText = linePos;
				if (drawText && !inGC->IsFontPixelSnappingEnabled())
					//we need to snap baseline position vertically to pixel boundary otherwise CoreText or DWrite might align itself vertically differently per run depending on run style
					//(because both snap baseline vertically to pixel boundary - maybe to better align decorations like underline in device space)
					inGC->CEAdjustPointInTransformedSpace( linePosText, true);

				VRect boundsBackColor;
				VColor backColor(0,0,0,0);

				//iterate on visual-ordered runs (draw runs from left to right visual order)
				Runs::const_iterator itRun = line->fVisualRuns.begin();
				for (;itRun != line->fVisualRuns.end(); itRun++)
				{
					//draw run

					Run* run = *itRun;

					if (run->GetEnd() == run->GetStart())
						continue;
					if (run->GetTypoBounds().GetX()+linePos.x >= clipBounds.GetRight())
						continue;
					if (run->GetTypoBounds().GetRight()+linePos.x < clipBounds.GetX())
						continue;

					//draw background

					if (drawCharBackColor)
					{
						if (run->fBackColor.GetAlpha() && !run->fTypoBounds.IsEmpty())
						{
							if (backColor.GetAlpha() && run->fBackColor != backColor)
							{
								if (backColor != fillColor)
								{
									inGC->SetFillColor( backColor);
									fillColor = backColor;
								}
								inGC->FillRect( boundsBackColor);
								boundsBackColor.SetEmpty();
								backColor.SetAlpha(0);
							}

							VRect bounds = VRect( run->fTypoBounds.GetX(), -run->fLine->fAscent, run->fTypoBounds.GetWidth(), run->fLine->GetLineHeight());
							bounds.SetPosBy( linePos.x, linePos.y);
							bounds.Intersect(boundsClipMargin);

							if (boundsBackColor.IsEmpty())
								boundsBackColor = bounds;
							else
								boundsBackColor.Union( bounds);
							backColor = run->fBackColor;
						}
						else
						{
							if (backColor.GetAlpha())
							{
								if (backColor != fillColor)
								{
									inGC->SetFillColor( backColor);
									fillColor = backColor;
								}
								inGC->FillRect( boundsBackColor);
								boundsBackColor.SetEmpty();
								backColor.SetAlpha(0);
							}
						}
					}
					

					if (drawText)
					{
						if (run->GetNodeLayout())
						{
							//it is a embedded node (image for instance)

							IDocNodeLayout *nodeLayout = run->GetNodeLayout();

							//here pos is sticked to x = node layout bounds left & y = baseline (IDocNodeLayout::Draw will adjust y according to ascent as while drawing text)
							VPoint pos = linePosText + run->GetStartPos();
							bool isRTL = run->GetDirection() == kPARA_RTL;
							if (isRTL)
								pos.SetPosBy(-nodeLayout->GetLayoutBounds().GetWidth(),0);
	
							GReal opacity = 1.0f;
							if (run->fStart >= fSelStart && run->fEnd <= fSelEnd)
								//if embedded node is selected, highlight it (Word-like highlighting)
								opacity = 0.5f;

							nodeLayout->Draw( inGC, pos, opacity);
						}
						else if (!run->IsTab())
						{
							//it is a text run 

							//change text color
							if (run->fTextColor.GetAlpha() && run->fTextColor != textColor)
							{
								inGC->SetTextColor( run->fTextColor);
								textColor = run->fTextColor;
							}

							//set font
							VFont *runfont = run->GetFont();
							if (runfont != font)
							{
								CopyRefCountable(&font, runfont);
								inGC->SetFont( font);
							}

							//draw text
							VPoint pos = linePosText + run->GetStartPos();
							VString runtext;
							fText.GetSubString( run->GetStart()+1, run->GetEnd()-run->GetStart(), runtext);
							
							bool isRTL = run->GetDirection() == kPARA_RTL;
							bool isRTLAnchorRight = isRTL && !rtlAnchorPreDecal;
							if (isRTL && !isRTLAnchorRight)
								//optimization: to avoid gc to recalc run width in order to decal RTL anchor, we pre-decal start position here
								//(as we know run width yet)
								pos.SetPosBy( -run->GetTypoBoundsWithKerning().GetWidth(), 0);
							
							inGC->DrawText( runtext, pos,	
											TLM_D2D_IGNORE_OVERHANGS | 
											(isRTL ? TLM_RIGHT_TO_LEFT : 0) | 
											TLM_TEXT_POS_UPDATE_NOT_NEEDED, 
											isRTLAnchorRight);		//for D2D, as we draw runs at kerning position, we need to ignore overhangs
																	//(otherwise kerning inter-runs would be screwed up...)
						}
					}                                                                                                                                                  
				}
			
				//draw run(s) background 
				if (backColor.GetAlpha())
				{
					if (backColor != fillColor)
					{
						inGC->SetFillColor( backColor);
						fillColor = backColor;
					}
					inGC->FillRect( boundsBackColor);
				}
			}
		}
	}
	
	ReleaseRefCountable(&font);

	if (fBorderLayout)
		inGC->RestoreClip();

	_EndLayerOpacity( inGC, inOpacity);

	inGC->ShouldScaleShapesWithPrinterScale(shouldScaleShapesWithPrinterScale);

	_EndLocalTransform( inGC, ctm, true);
	}

	_EndDrawLayer();
	EndUsingContext();
}

/** return formatted text bounds (typo bounds) including margins
@remarks
	text typo bounds origin is set to inTopLeft

	as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
	because layout bounds are equal to the text box design bounds and not to the formatted text bounds
	you should use this method to get formatted text width & height 
*/
void VDocParagraphLayout::GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	//call GetLayoutBounds in order to update both layout & typo metrics
	GetLayoutBounds( inGC, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
	if (fLayoutIsValid)
	{
		outBounds.SetWidth( fRenderTypoBounds.GetWidth());
		outBounds.SetHeight( fRenderTypoBounds.GetHeight());
	}
}

void VDocParagraphLayout::GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	if (inGC)
	{
		if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
			return;
		if (inGC == fBackupParentGC)
		{
			xbox_assert(fGC);
			inGC = fGC;
		}
		xbox_assert(inGC);
		if (fText.GetLength() == 0 && !inReturnCharBoundsForEmptyText)
		{
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
			return;
		}

		BeginUsingContext(inGC, true);

		if (testAssert(fLayoutIsValid))
		{
			outBounds = fRenderLayoutBounds;
			outBounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
		}
		else
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);

		EndUsingContext();
	}
	else
	{
		//return stored metrics

		if (fText.GetLength() == 0 && !inReturnCharBoundsForEmptyText)
		{
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
			return;
		}
		if (fLayoutIsValid)
		{
			outBounds = fRenderLayoutBounds;
			outBounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
		}
		else
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
	}
}

void VDocParagraphLayout::GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	_NormalizeRange( inStart, inEnd);

	if (fText.GetLength() == 0 || inStart >= inEnd)
	{
		outRunBounds.clear();
		return;
	}

	outRunBounds.clear();

	BeginUsingContext(inGC, true);
	if (!testAssert(fLayoutIsValid))
	{
		EndUsingContext();
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
	{
		sLONG i = inStart-1-run->fStart;
		if (i >= run->GetCharOffsets().size())
			startOffset = run->fTypoBoundsWithKerning.GetWidth();
		else
			startOffset = run->GetCharOffsets()[i];
	}
	GReal endOffset = run->fTypoBoundsWithKerning.GetWidth();	
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
						boundsRun = VRect( run->fTypoBoundsWithKerning.GetX(), -run->fLine->fAscent, run->fTypoBoundsWithKerning.GetWidth(), run->fLine->GetLineHeight());
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

	EndUsingContext();
}

void VDocParagraphLayout::GetCaretMetricsFromCharIndex(	VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex _inCharIndex, 
														VPoint& outCaretPos, GReal& outTextHeight, 
														const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	sLONG inCharIndex = _inCharIndex;
	if (inCharIndex < 0)
		inCharIndex = 0;

	BeginUsingContext(inGC, true);
	if (!testAssert(fLayoutIsValid))
	{
		EndUsingContext();
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
	else if (inCharIndex >= run->fStart)
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

	EndUsingContext();
}

/** get paragraph from local position (relative to text box origin+vertical align decal & computing dpi)*/
VDocParagraphLayout::VParagraph* VDocParagraphLayout::_GetParagraphFromLocalPos(const VPoint& inPos)
{
	xbox_assert(fParagraphs.size() > 0);

	Paragraphs::iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		VParagraph *para = itPara->Get();
		if (inPos.GetY() < fMargin.top+para->GetStartPos().GetY()+para->GetLayoutBounds().GetHeight())
			return para;
	}
	return fParagraphs.back().Get();
}

VDocParagraphLayout::Line* VDocParagraphLayout::_GetLineFromLocalPos(const VPoint& inPos)
{
	VParagraph* para = _GetParagraphFromLocalPos( inPos);
	xbox_assert(para);

	VPoint pos = inPos;
	pos.y -= fMargin.top+para->GetStartPos().GetY();

	Lines::iterator itLine = para->fLines.begin();
	for (;itLine != para->fLines.end(); itLine++)
	{
		if (pos.GetY() < (*itLine)->GetStartPos().GetY()+(*itLine)->GetLineHeight()-(*itLine)->GetAscent())
			return (*itLine);
	}
	return para->fLines.back();
}

VDocParagraphLayout::Run* VDocParagraphLayout::_GetRunFromLocalPos(const VPoint& inPos, VPoint& outLineLocalPos)
{
	Line *line = _GetLineFromLocalPos( inPos);

	VPoint lineStartPos;
	_GetLineStartPos( line, lineStartPos);
	VPoint pos = inPos;
	pos -= lineStartPos;
	outLineLocalPos = pos;

	GReal minx = kMAX_SmallReal;
	GReal maxx = kMIN_SmallReal; 
	Runs::iterator itRun = line->fVisualRuns.begin();
	for (;itRun != line->fVisualRuns.end(); itRun++)
	{
		if (pos.GetX() < (*itRun)->fTypoBoundsWithKerning.GetRight())
			return (*itRun);
	}
	return (line->fVisualRuns.back());
}


bool VDocParagraphLayout::GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return false;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (fText.GetLength() == 0)
	{
		outCharIndex = 0;
		if (outRunLength)
			*outRunLength = 0;
		return false;
	}

	BeginUsingContext(inGC, true);
	if (!testAssert(fLayoutIsValid))
	{
		EndUsingContext();
		outCharIndex = 0;
		if (outRunLength)
			*outRunLength = 0;
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

	if (outRunLength)
		*outRunLength = run->GetEnd()-run->GetStart();

	//adjust with end of line (as we point on the current line, we cannot return a index after CR  - which is on the next line - but just before the CR) 
	if (outCharIndex > run->fLine->GetStart() && outCharIndex == run->GetEnd() && fText[outCharIndex-1] == 13)
		outCharIndex--;

	_EndLocalTransform( inGC, xform, false);

	EndUsingContext();

	return true;
}


/** get character range which intersects the passed bounds (in gc user space) 
@remarks
	text layout origin is set to inTopLeft (in gc user space)
*/
void VDocParagraphLayout::GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

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
		if (outRangeEnd > fText.GetLength())
			outRangeEnd = fText.GetLength();
	}

	EndUsingContext();
}


/** move character index to the nearest character index on the line before/above the character line 
@remarks
	return false if character is on the first line (ioCharIndex is not modified)
*/
bool VDocParagraphLayout::MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return false;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (fText.GetLength() == 0)
		return false;

	BeginUsingContext(inGC, true);
	if (!testAssert(fLayoutIsValid))
	{
		EndUsingContext();
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

	EndUsingContext();

	return result;
}

/** move character index to the nearest character index on the line after/below the character line
@remarks
	return false if character is on the last line (ioCharIndex is not modified)
*/
bool VDocParagraphLayout::MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return false;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (fText.GetLength() == 0)
		return false;

	BeginUsingContext(inGC, true);
	if (!testAssert(fLayoutIsValid))
	{
		EndUsingContext();
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

	EndUsingContext();

	return result;
}

/** create and retain document from the current layout  
@param inStart
	range start
@param inEnd
	range end

@remarks
	if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
	with a single child node initialized from the current layout
*/
VDocText* VDocParagraphLayout::CreateDocument(sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraph *para = CreateParagraph( inStart, inEnd);
	if (para)
	{
		VDocText *doc = new VDocText();
		if (!doc)
		{
			para->Release();
			return NULL;
		}
#if ENABLE_VDOCTEXTLAYOUT
		VDocBaseLayout *parent = _GetTopLevelParentLayout();
		if (parent && parent->GetDocType() == kDOC_NODE_TYPE_DOCUMENT)
		{
			VDocTextLayout *docLayout = dynamic_cast<VDocTextLayout *>(parent);
			if (testAssert(docLayout))
				docLayout->SetPropsTo( static_cast<VDocNode *>(doc));
		}
#endif
		doc->AddChild( para);
		para->Release();
		return doc;
	}
	return NULL;
}

/** create and retain paragraph from the current layout plain text, styles & paragraph properties */
VDocParagraph *VDocParagraphLayout::CreateParagraph(sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraph *para = new VDocParagraph();

	//copy plain text
	if (inStart == 0 && inEnd >= fText.GetLength())
		para->SetText( fText);
	else
	{
		VString text;
		fText.GetSubString(inStart+1, inEnd-inStart, text);
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
		if (inStart == 0 && inEnd >= fText.GetLength())
			stylesOnRange = RetainRefCountable( styles);
		else
		{
			stylesOnRange = styles->CreateTreeTextStyleOnRange(inStart, inEnd, true, inEnd > inStart);
			copyStyles = false;
		}

	
		if (stylesOnRange && stylesOnRange->GetChildCount() > 0 && stylesOnRange->GetNthChild(1)->HasSpanRefs())
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

	SetPropsTo( static_cast<VDocNode *>(para));
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
void VDocParagraphLayout::ApplyParagraphStyle( const VDocParagraph *inPara, bool inResetStyles, bool inApplyParagraphPropsOnly, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	SetDirty();
	fLayerIsDirty = true;
	ReleaseRefCountable(&fDefaultStyle);

	if (inResetStyles && !inApplyParagraphPropsOnly)
		_ResetStyles( false, true); //reset only char styles

	if (!inApplyParagraphPropsOnly && inPara->GetStyles())
	{
		//apply char styles
		VTextStyle *style = new VTextStyle( inPara->GetStyles()->GetData());
		style->SetRange(0, fText.GetLength());
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
void VDocParagraphLayout::InitFromParagraph( const VDocParagraph *inPara)
{
	SetDirty();
	fLayerIsDirty = true;

	_ResetStyles( false, true); //reset char styles

	_ReplaceText( inPara);

	const VTreeTextStyle *styles = inPara->GetStyles();
	ApplyStyles( styles);

	_ApplyParagraphProps( inPara, false);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
}

/** reset all text properties & character styles to default values */
void VDocParagraphLayout::ResetParagraphStyle(bool inResetParagraphPropsOnly, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	SetDirty();
	fLayerIsDirty = true;

	_ResetStyles(true, inResetParagraphPropsOnly ? false : true);

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();
}

void VDocParagraphLayout::_ReplaceText( const VDocParagraph *inPara)
{
	fText = inPara->GetText();

	_UpdateTextInfoEx(true);
}

void VDocParagraphLayout::_ApplyParagraphProps( const VDocParagraph *inPara, bool inIgnoreIfInherited)
{
	InitPropsFrom( static_cast<const VDocNode *>(inPara), inIgnoreIfInherited);
}

void VDocParagraphLayout::_ResetStyles( bool inResetParagraphProps, bool inResetCharStyles)
{
	//reset to default character style 
	if (inResetCharStyles)
	{
		ReleaseRefCountable(&fStyles);
		ReleaseRefCountable(&fExtStyles);
		ReleaseRefCountable(&fDefaultFont);
		fDefaultFont = VFont::RetainStdFont(STDF_TEXT);
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

void VDocParagraphLayout::ChangeSelection( sLONG inStart, sLONG inEnd, bool inActive)
{
	_NormalizeRange( inStart, inEnd);

	if (fSelIsActive == inActive 
		&&
		(fSelStart == inStart && fSelEnd == inEnd))
		return;

	fSelIsActive = inActive;
	fSelStart = inStart;
	fSelEnd = inEnd;
	if (!fShouldPaintSelection)
		return;
	fSelectionRenderInfo.clear();
	fSelIsDirty = fSelStart < fSelEnd;
	fLayerIsDirty = true;
}

/** set default text color 
@remarks
	black color on default (it is applied prior styles set with SetText)
*/
void VDocParagraphLayout::SetDefaultTextColor( const VColor &inTextColor, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	if (fDefaultTextColor == inTextColor)
		return;
	SetDirty();
	fDefaultTextColor = inTextColor;
	ReleaseRefCountable(&fDefaultStyle);
}

/** get default text color */
bool VDocParagraphLayout::GetDefaultTextColor(VColor& outColor, sLONG /*inStart*/) const 
{ 
	outColor = fDefaultTextColor;
	return true; 
}

const VString& VDocParagraphLayout::GetText() const
{
	return fText;
}



/** set max text width (0 for infinite) - in pixel (pixel relative to the layout DPI)
@remarks
	it is also equal to text box width 
	so you should specify one if text is not aligned to the left border
	otherwise text layout box width will be always equal to formatted text width
*/ 
void VDocParagraphLayout::SetMaxWidth(GReal inMaxWidth) 
{
	if (inMaxWidth)
	{
		sLONG width = ICSSUtil::PointToTWIPS(inMaxWidth*72/fRenderDPI);
		width -= _GetAllMarginHorizontalTWIPS(); //subtract margins
		if (width <= 0)
			width = 1;
		VDocParagraphLayout::SetWidth( width);
	}
	else
		VDocParagraphLayout::SetWidth( 0);
}

/** set max text height (0 for infinite) - in pixel (pixel relative to the layout DPI)
@remarks
	it is also equal to text box height 
	so you should specify one if paragraph is not aligned to the top border
	otherwise text layout box height will be always equal to formatted text height
*/ 
void VDocParagraphLayout::SetMaxHeight(GReal inMaxHeight) 
{
	if (inMaxHeight)
	{
		sLONG height = ICSSUtil::PointToTWIPS(inMaxHeight*72/fRenderDPI);
		height -= _GetAllMarginVerticalTWIPS(); //subtract margins
		if (height <= 0)
			height = 1;
		VDocParagraphLayout::SetHeight( height);
	}
	else
		VDocParagraphLayout::SetHeight( 0);
}



/** set paragraph line height 

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height 
*/
void VDocParagraphLayout::SetLineHeight( const GReal inLineHeight, sLONG /*inStart*/, sLONG /*inEnd*/) 
{
	if (fLineHeight == inLineHeight)
		return;
	SetDirty();
	fLineHeight = inLineHeight;
}

/** set first line padding offset in point 
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
void VDocParagraphLayout::SetTextIndent(const GReal inPadding, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	if (fTextIndent == inPadding)
		return;
	fTextIndent = inPadding;
	SetDirty();
}

/** set tab stop (offset in point) */
void VDocParagraphLayout::SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	if (!testAssert(inOffset > 0.0f))
		return;
	if (fTabStopOffset == inOffset && fTabStopType == inType)
		return;
	fTabStopOffset = inOffset;
	fTabStopType = inType;
	SetDirty();
}


/** set text layout mode (default is TLM_NORMAL) */
void VDocParagraphLayout::SetLayoutMode( TextLayoutMode inLayoutMode, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	if (fLayoutMode == inLayoutMode)
		return;
	SetDirty();
	fLayoutMode = inLayoutMode;
}


/** begin using text layout for the specified gc */
void VDocParagraphLayout::BeginUsingContext( VGraphicContext *inGC, bool inNoDraw)
{
	xbox_assert(inGC);
	
	if (fGCUseCount > 0)
	{
		xbox_assert(fGC == inGC || inGC == fBackupParentGC);
		if (!fParent)
			fGC->BeginUsingContext( inNoDraw);
		//if caller has updated some layout settings, we need to update again the layout
		if (!fLayoutIsValid || fLayoutBoundsDirty)
			_UpdateLayout();
		fGCUseCount++;
		return;
	}

	fGCUseCount++;

	if (!fParent)
	{
		inGC->BeginUsingContext( inNoDraw);
		inGC->UseReversedAxis();
	}

	//reset layout & layer if we are printing
	fIsPrinting = inGC->IsPrintingContext();
	if (fIsPrinting)
	{
		fLayoutIsValid = false;
		ReleaseRefCountable(&fLayerOffScreen);
		fLayerIsDirty = true;
	}


#if VERSIONWIN
	if (!fParent)
	{
		//if GDI and not printing, derive to GDIPlus for enhanced graphic features onscreen (for new layout only)
		xbox_assert(fBackupParentGC == NULL);
		if (!fIsPrinting && inGC->IsGDIImpl())
		{
			CopyRefCountable(&fBackupParentGC, inGC);
			inGC = inGC->BeginUsingGdiplus();
		}
	}
#endif

	_SetGC( inGC);
	xbox_assert(fGC == inGC);

	//set current font & text color
	if (!fParent)
	{
		xbox_assert(fBackupFont == NULL);
		fBackupFont = inGC->RetainFont();
		inGC->GetTextColor( fBackupTextColor);
	}

	xbox_assert(fDefaultFont);
	fUseFontTrueTypeOnly = fStylesUseFontTrueTypeOnly && fDefaultFont->IsTrueTypeFont();

#if VERSIONWIN
	fNeedRestoreTRM = false;
	if (!fParent)
	{
		fBackupTRM = fGC->GetDesiredTextRenderingMode();

		if ( fGC->IsGdiPlusImpl() && (!(fGC->GetTextRenderingMode() & TRM_LEGACY_ON)) && (!(fGC->GetTextRenderingMode() & TRM_LEGACY_OFF)))//force always legacy for GDIPlus -  as native text layout is allowed only for D2D - with DWrite (and actually only with v15+)
		{
			//force legacy if not set yet if text uses at least one font not truetype (as neither D2D or GDIPlus are capable to display fonts but truetype)
			fGC->SetTextRenderingMode( (fBackupTRM & (~TRM_LEGACY_OFF)) | TRM_LEGACY_ON);
			fNeedRestoreTRM = true;
		}
	}
#endif

	if (!fParent && fLayoutIsValid)
	{
		//check if text rendering mode has changed
		if (fLayoutTextRenderingMode != fGC->GetTextRenderingMode())
			fLayoutIsValid = false;
		//check if kerning has changed (only on Mac OS: kerning is not used for text box layout on other impls)
		else if (fLayoutKerning != fGC->GetCharActualKerning())
			fLayoutIsValid = false;
	}

	//manage placeholder or password text replacement (we need fText to be always consistent with text used to compute layout)

	fShouldRestoreText = false;
	if (fText.GetLength() == 0 && !fPlaceHolder.IsEmpty())
	{
		fTextBackup = fText;
		fText = fPlaceHolder;
		fShouldRestoreText = true;
	}
	else if (fIsPassword && fText.GetLength() > 0)
	{
		if (fTextPassword.GetLength() > fText.GetLength())
			fTextPassword.Truncate( fText.GetLength());
		else if (fTextPassword.GetLength() < fText.GetLength())
		{
			VIndex	curLength = fTextPassword.GetLength();
			fTextPassword.EnsureSize( fText.GetLength());
			for (int i = curLength; i < fText.GetLength(); i++)
				fTextPassword.AppendUniChar( '*');
		}
		fTextBackup = fText;
		fText = fTextPassword;
		fShouldRestoreText = true;
	}

	if (fLayoutIsValid && !fLayoutBoundsDirty)
		return;

	//update text layout (only if it is no longer valid)
	_UpdateLayout();
}

/** end using text layout */
void VDocParagraphLayout::EndUsingContext()
{
	fGCUseCount--;
	xbox_assert(fGCUseCount >= 0);
	if (fGCUseCount > 0)
	{
		if (!fParent)
			fGC->EndUsingContext();
		return;
	}

	if (fShouldRestoreText)
	{
		fText = fTextBackup;
		xbox_assert(fText.GetLength() == fTextLength); //it should never happen -> plain text should not be modified while using context
	}

#if VERSIONWIN
	if (fNeedRestoreTRM) 
		fGC->SetTextRenderingMode( fBackupTRM);
#endif

	//restore font & text color
	if (!fParent)
	{
		fGC->SetTextColor( fBackupTextColor);

		VFont *font = fGC->RetainFont();
		if (font != fBackupFont)
			fGC->SetFont( fBackupFont);
		ReleaseRefCountable(&font);
		ReleaseRefCountable(&fBackupFont);

#if VERSIONWIN
		if (fBackupParentGC)
		{
			fBackupParentGC->EndUsingGdiplus();
			fBackupParentGC->EndUsingContext();
			ReleaseRefCountable(&fBackupParentGC);
		}
		else
#endif
			fGC->EndUsingContext();
	}

	_SetGC( NULL);
}


void VDocParagraphLayout::_ResetLayer()
{
	fLayerIsDirty = true;
	ReleaseRefCountable(&fLayerOffScreen);
}


void VDocParagraphLayout::_UpdateSelectionRenderInfo( const VPoint& inTopLeft, const VRect& inClipBounds)
{
	xbox_assert(fGC);

	if (!fShouldPaintSelection)
		return;
	if (!fSelIsDirty)
		return;

	fSelIsDirty = false;
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


void VDocParagraphLayout::_DrawSelection(const VPoint& inTopLeft)
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



bool VDocParagraphLayout::_CheckLayerBounds(const VPoint& inTopLeft, VRect& outLayerBounds)
{
	outLayerBounds = VRect( inTopLeft.GetX(), inTopLeft.GetY(), fRenderLayoutBounds.GetWidth(), fRenderLayoutBounds.GetHeight()); //on default, layer bounds = layout bounds

	if ((!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl()) //not draw on layer
		&& 
		!fShouldPaintSelection) //not draw selection
		return true;

	VAffineTransform ctm;	
	fGC->UseReversedAxis();
	fGC->GetTransformToScreen(ctm); //in case there is a pushed layer, we explicitly request transform from user space to hwnd space
									//(because fLayerViewRect is in hwnd user space)
	if (ctm.GetScaling() != fLayerCTM.GetScaling()
		||
		ctm.GetShearing() != fLayerCTM.GetShearing())
	{
		//if scaling or rotation has changed, mark layer and background color as dirty
		fLayerIsDirty = true;
		fSelIsDirty = true;
		fLayerCTM = ctm;
	}

	//determine layer bounds
	VRect boundsViewLocal = outLayerBounds;
	if (!fLayerViewRect.IsEmpty())
	{
		//clip layout bounds with view rect bounds in gc local user space
		VPoint offset;
		if (ctm.IsIdentity())
		{
			boundsViewLocal.Intersect( outLayerBounds);
		}
		else
		{
			if (fGC->IsQuartz2DImpl())
			{
				//fLayerViewRect is window rectangle in QuickDraw coordinates so we need to convert it to Quartz2D coordinates
				//(because transformed space is equal to Quartz2D space)
				
				VRect boundsViewRect( fLayerViewRect);
				fGC->QDToQuartz2D( boundsViewRect);
				boundsViewLocal = ctm.Inverse().TransformRect( boundsViewRect);
			}
			else
				boundsViewLocal = ctm.Inverse().TransformRect( fLayerViewRect);

			boundsViewLocal.NormalizeToInt();
			boundsViewLocal.Intersect( outLayerBounds);
		}
	}

	if (boundsViewLocal.IsEmpty())
	{
		//do not draw at all & clear layer if layout is fully clipped by fLayerViewRect
		_ResetLayer();
	
		fSelectionRenderInfo.clear();
		fSelIsDirty = false;
		return false; 
	}

	_UpdateSelectionRenderInfo(inTopLeft, outLayerBounds);
	return true;
}


bool VDocParagraphLayout::_BeginDrawLayer(const VPoint& inTopLeft)
{
	xbox_assert(fGC && fSkipDrawLayer == false);

	VRect boundsLayout;
	bool doDraw = _CheckLayerBounds( inTopLeft, boundsLayout);
	fLayerTopLeft = inTopLeft;

	if (!fShouldDrawOnLayer || !fShouldEnableCache || fIsPrinting || fGC->IsGDIImpl())
	{
		fSkipDrawLayer = true; 
		return doDraw;
	}

	if (!fLayerOffScreen)
		fLayerIsDirty = true;

	bool isLayerTransparent = true; //for consistency, assume layer is always transparent
	
	if (isLayerTransparent && !fGC->ShouldDrawTextOnTransparentLayer())
	{
		fSkipDrawLayer = true;
		_ResetLayer();
		return doDraw;
	}

	if (!doDraw)
	{
		fSkipDrawLayer = true;
		return false;
	}

	//draw on layer

	//ensure contiguous layers do not overlap in device space
	fGC->CEAdjustRectInTransformedSpace( boundsLayout, true);

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
	}
	else
		fSkipDrawLayer = true;
	return doRedraw;
}

void VDocParagraphLayout::_EndDrawLayer()
{
	if (fSkipDrawLayer)
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
void VDocParagraphLayout::_SetGC( VGraphicContext *inGC)
{
	if (fGC == inGC)
		return;

	if (inGC)
	{
		fLayoutIsValid = fLayoutIsValid && fShouldEnableCache && !fIsPrinting;

		//if new gc is not compatible with last used gc, we need to compute again text layout & metrics and/or reset layer

		if (!fParent && !inGC->IsCompatible( fGCType, true)) //check if current layout is compatible with new gc: here we ignore hardware or software status 
			fLayoutIsValid = false;//gc kind has changed: inval layout

		if (fLayerOffScreen && (inGC->IsGDIImpl() || !fShouldDrawOnLayer)) //check if layer is still enabled
			_ResetLayer(); 

		if (!fParent && fLayerOffScreen && !inGC->IsCompatible( fGCType, false)) //check is layer is compatible with new gc: here we take account hardware or software status
			_ResetLayer(); //gc kind has changed: clear layer so it is created from new gc on next _BeginLayer

		fGCType = inGC->GetGraphicContextType(); //backup current gc type
		
		if (!fLayoutIsValid) 
			fLayerIsDirty = true;
	}

	//attach new gc or dispose actual gc (if inGC == NULL)
	CopyRefCountable(&fGC, inGC);
}


/** set default font */
void VDocParagraphLayout::SetDefaultFont( const VFont *inFont, GReal inDPI, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	xbox_assert(inFont != NULL);
	if (fDefaultFont == inFont && inDPI == 72)
		return;

	SetDirty();
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
VFont *VDocParagraphLayout::RetainDefaultFont(GReal inDPI, sLONG /*inStart*/) const
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


void VDocParagraphLayout::_CheckStylesConsistency()
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
		if (end > fText.GetLength())
		{
			end = fText.GetLength();
			doUpdate = true;
		}
		if (doUpdate)
			fStyles->GetData()->SetRange( start, end);
		if (start > 0 || end < fText.GetLength())
		{
			//expand styles to contain text range
			VTreeTextStyle *styles = new VTreeTextStyle( new VTextStyle());
			styles->GetData()->SetRange( 0, fText.GetLength());
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
		if (end > fText.GetLength())
		{
			end = fText.GetLength();
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
void VDocParagraphLayout::SetText( const VString& inText, VTreeTextStyle *inStyles, bool inCopyStyles)
{
	SetDirty();

	uLONG previousTextLength = fText.GetLength();

	if (&fText != &inText)
		fText.FromString(inText);

	fTextLength = fText.GetLength(); //actual length

#if VERSIONDEBUG
	if (fStyles && inStyles == fStyles)
	{
		sLONG start, end;
		fStyles->GetData()->GetRange(start, end);
		xbox_assert(start == 0 && end == fTextLength);
	}
#endif
	if (inStyles != fStyles || inCopyStyles)
	{
		if (inStyles != fStyles)
		{
			if (fStyles)
			{
				//preserve uniform style (first character style)
				if (previousTextLength > 0)
				{
					fStyles->Truncate(1); //truncate all styles but first char style
					fStyles->Truncate(0,-1,false); //set range to 0,0 (preserving first char style)
				}
				fStyles->ExpandAtPosBy(0, fTextLength); //now expand to new text range
			}
			//apply new styles
			_ApplyStylesRec( inStyles);
		}
		else
		{
			VTreeTextStyle *styles = inStyles ? new VTreeTextStyle( inStyles) : NULL;
			CopyRefCountable(&fStyles, styles);
			ReleaseRefCountable(&styles);
		}
	}

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();

	_UpdateTextInfoEx(true);
}

/** replace current text & styles
@remarks	
	here passed styles are not retained
*/
void VDocParagraphLayout::SetText(	const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd, 
									bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, 
									const VDocSpanTextRef * inPreserveSpanTextRef, 
									void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	_NormalizeRange( inStart, inEnd);

	if (inBeforeReplace && inAfterReplace)
	{
		fReplaceTextUserData = inUserData;
		fReplaceTextBefore = inBeforeReplace;
		fReplaceTextAfter = inAfterReplace;

		VTreeTextStyle::ReplaceStyledText(	fText, fStyles, inText, inStyles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle,
											inPreserveSpanTextRef, this, _BeforeReplace, _AfterReplace);
	}
	else
	{
		if (VTreeTextStyle::ReplaceStyledText(	fText, fStyles, inText, inStyles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle,
												inPreserveSpanTextRef, NULL, NULL, NULL))
		{
			SetText( fText, fStyles, false);
		}
	}
}

void VDocParagraphLayout::GetText(VString& outText, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	if (inStart == 0 && inEnd == fText.GetLength())
	{
		outText = fText;
		return;
	}

	if (inStart < 0)
		inStart = 0;
	if (inEnd == -1 || inEnd > fText.GetLength())
		inEnd = fText.GetLength();
	if (inStart < inEnd)
		fText.GetSubString( inStart+1, inEnd-inStart, outText);
	else
		outText.SetEmpty();
}

VTreeTextStyle* VDocParagraphLayout::RetainDefaultStyle(sLONG inStart, sLONG inEnd) const 
{ 
	VTreeTextStyle* styles = _RetainDefaultTreeTextStyle(); 
	styles->GetData()->SetJustification( JST_Notset); //do not keep styles inherited from paragraph props (as we want only character styles here)
	styles->GetData()->SetHasBackColor(false);
	ReleaseRefCountable(&fDefaultStyle);
	return styles;
}

VTreeTextStyle*	VDocParagraphLayout::RetainStyles(sLONG inStart, sLONG inEnd, bool inRelativeToStart, bool inIncludeDefaultStyle, bool inCallerOwnItAlways) const
{
	_NormalizeRange( inStart, inEnd);

	VTreeTextStyle *styles = NULL;
	if (fStyles)
	{
		if (inStart == 0 && inEnd >= fText.GetLength())
			styles = inCallerOwnItAlways ? new VTreeTextStyle(fStyles) : RetainRefCountable(fStyles);
		else
			styles = fStyles->CreateTreeTextStyleOnRange( inStart, inEnd, inRelativeToStart, inEnd > inStart);
	}

	if (inIncludeDefaultStyle)
	{
		VTreeTextStyle *stylesBase = RetainDefaultStyle();

		if (inRelativeToStart)
			stylesBase->GetData()->SetRange( 0, inEnd-inStart);
		else
			stylesBase->GetData()->SetRange( inStart, inEnd);
		if (styles)
		{
			if (styles == fStyles)
			{
				styles->Release();
				styles = new VTreeTextStyle( fStyles);
			}
			if (styles->HasSpanRefs())
				stylesBase->ApplyStyles( styles, false); //to ensure styles span references are attached to stylesBase
			else
				stylesBase->AddChild( styles);
			styles->Release();
		}
		return stylesBase;
	}
	return styles;
}


/** replace text with span text reference on the specified range
@remarks
	it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

	you should no more use or destroy passed inSpanRef even if method returns false

	IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
			   so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
			   (passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens 
			    - if display type is set to normalized source)
*/
bool VDocParagraphLayout::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inNoUpdateRef,
									    void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	xbox_assert(inSpanRef);

	_NormalizeRange( inStart, inEnd);

	if (inStart > inEnd)
	{
		delete inSpanRef;
		return false;
	}

	inSpanRef->ShowHighlight(fHighlightRefs);
	inSpanRef->ShowRef(inSpanRef->CombinedTextTypeMaskToTextTypeMask( fShowRefs));

	if (inBeforeReplace && inAfterReplace)
	{
		fReplaceTextUserData = inUserData;
		fReplaceTextBefore = inBeforeReplace;
		fReplaceTextAfter = inAfterReplace;

		if (VTreeTextStyle::ReplaceAndOwnSpanRef( fText, fStyles, inSpanRef, inStart, inEnd, 
												  inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inNoUpdateRef,
												  this, _BeforeReplace, _AfterReplace, inLC))
		{
			return true;
		}
	}
	else
		if (VTreeTextStyle::ReplaceAndOwnSpanRef( fText, fStyles, inSpanRef, inStart, inEnd, 
												  inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inNoUpdateRef,
												  NULL, NULL, NULL, inLC))
		{
			SetText(fText, fStyles, false);
			return true;
		}
	return false;
}

void VDocParagraphLayout::_BeforeReplace(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText)
{
	VDocParagraphLayout *layout = (VDocParagraphLayout *)inUserData;

	if (layout->fReplaceTextBefore)
		layout->fReplaceTextBefore(layout->fReplaceTextUserData, ioStart, ioEnd, ioText);
}

void VDocParagraphLayout::_AfterReplace(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced)
{
	VDocParagraphLayout *layout = (VDocParagraphLayout *)inUserData;

	layout->SetText(layout->fText, layout->fStyles, false); //ensure layout is invalidated before we call after replace handler
	
	if (layout->fReplaceTextAfter)
		layout->fReplaceTextAfter( layout->fReplaceTextUserData, inStartReplaced, inEndReplaced);
}

/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
bool VDocParagraphLayout::UpdateSpanRefs(sLONG inStart, sLONG inEnd, void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	if (fStyles && fStyles->HasSpanRefs())
	{
		_NormalizeRange( inStart, inEnd);

		if (inBeforeReplace && inAfterReplace)
		{
			fReplaceTextUserData = inUserData;
			fReplaceTextBefore = inBeforeReplace;
			fReplaceTextAfter = inAfterReplace;
			
			if (VTreeTextStyle::UpdateSpanRefs( fText, *fStyles, inStart, inEnd, this, _BeforeReplace, _AfterReplace, inLC))
				return true;
		}
		else 
			if (VTreeTextStyle::UpdateSpanRefs( fText, *fStyles, inStart, inEnd, NULL, NULL, NULL, inLC))
			{
				SetText( fText, fStyles, false);
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
bool VDocParagraphLayout::EvalExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, VDocCache4DExp *inCache4DExp)
{
	if (fStyles)
	{
		_NormalizeRange( inStart, inEnd);
		return fStyles->EvalExpressions( inLC, inStart, inEnd, inCache4DExp);
	}
	return false;
}


/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
bool VDocParagraphLayout::FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	if (fStyles)
	{
		_NormalizeRange( inStart, inEnd);

		if (inBeforeReplace && inAfterReplace)
		{
			fReplaceTextUserData = inUserData;
			fReplaceTextBefore = inBeforeReplace;
			fReplaceTextAfter = inAfterReplace;

			if (VTreeTextStyle::FreezeExpressions( inLC, fText, *fStyles, inStart, inEnd, this, _BeforeReplace, _AfterReplace))
			{
				return true;
			}
		}
		else
			if (VTreeTextStyle::FreezeExpressions( inLC, fText, *fStyles, inStart, inEnd, NULL, NULL, NULL))
			{
				SetText(fText, fStyles, false);
				return true;
			}
	}
	return false;
}


/** set extra text styles 
@remarks
	it can be used to add a temporary style effect to the text layout without modifying the main text styles
	(for instance to add a text selection effect: see VStyledTextEditView)

	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
void VDocParagraphLayout::SetExtStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	if (!inStyles && !fExtStyles)
		return;

	if (!testAssert(inStyles == NULL || inStyles != fStyles))
		return;

	if (!testAssert(!inStyles || !inStyles->HasSpanRefs())) //span references are forbidden in extended styles
		return;

	if (inStyles && fExtStyles && (*inStyles) == (*fExtStyles)) //avoid to inval layout if new styles are equal to the actual styles
		return;

	SetDirty();

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



/** return true if layout styles uses truetype font only */
bool VDocParagraphLayout::_StylesUseFontTrueTypeOnly() const
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
bool VDocParagraphLayout::UseFontTrueTypeOnly(VGraphicContext *inGC) const
{
	xbox_assert(inGC);
	xbox_assert(fDefaultFont);
	
	bool curFontIsTrueType = fDefaultFont->IsTrueTypeFont() != FALSE ? true : false;
	return fStylesUseFontTrueTypeOnly && curFontIsTrueType;
}


/** insert text at the specified position */
void VDocParagraphLayout::InsertPlainText( sLONG inPos, const VString& inText)
{
	if (!inText.GetLength())
		return;
	ReplacePlainText( inPos, inPos, inText);
}


/** replace text */
void VDocParagraphLayout::ReplacePlainText( sLONG inStart, sLONG inEnd, const VString& inText)
{
	SetText( inText, NULL, inStart, inEnd);
}


/** delete text range */
void VDocParagraphLayout::DeletePlainText( sLONG inStart, sLONG inEnd)
{
	ReplacePlainText( inStart, inEnd, CVSTR(""));
}



VTreeTextStyle *VDocParagraphLayout::_RetainDefaultTreeTextStyle() const
{
	if (!fDefaultStyle)
	{
		VTextStyle *style = testAssert(fDefaultFont) ? VDocParagraphLayout::CreateTextStyle( fDefaultFont) : new VTextStyle();

		style->SetHasForeColor(true);
		style->SetColor( fDefaultTextColor.GetRGBAColor());

		if (fTextAlign != AL_DEFAULT)
			//avoid character style to override paragraph align style if it is equal to paragraph align style
			style->SetJustification( VDocParagraphLayout::JustFromAlignStyle( fTextAlign));

		//avoid layout character styles to store background color if it is equal to this element or parent background color
		uLONG bkColor = fBackgroundColor.GetRGBAColor();
		VDocBaseLayout *parent = fParent;
		while (parent && bkColor == RGBACOLOR_TRANSPARENT)
		{
			bkColor = parent->GetBackgroundColor().GetRGBAColor();
			parent = parent->fParent;
		}

		if (bkColor != RGBACOLOR_TRANSPARENT)
		{
			style->SetHasBackColor(true);
			style->SetBackGroundColor(bkColor);
		}
		fDefaultStyle = new VTreeTextStyle( style);
	}

	fDefaultStyle->GetData()->SetRange(0, fText.GetLength());
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
VTreeTextStyle *VDocParagraphLayout::_RetainFullTreeTextStyle() const
{
	if (!fStyles)
		return _RetainDefaultTreeTextStyle();

	VTreeTextStyle *styles = _RetainDefaultTreeTextStyle();
	if (testAssert(styles))
		styles->AddChild(fStyles);
	return styles;
}

void VDocParagraphLayout::_ReleaseFullTreeTextStyle( VTreeTextStyle *inStyles) const
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

bool VDocParagraphLayout::ApplyStyles( const VTreeTextStyle * inStyles, bool inFast)
{
	return _ApplyStylesRec( inStyles, inFast);
}

bool VDocParagraphLayout::_ApplyStylesRec( const VTreeTextStyle * inStyles, bool inFast)
{
	if (!inStyles)
		return false;
	bool update = _ApplyStyle( inStyles->GetData());
	if (update)
	{
		SetDirty();
		fSelIsDirty = true;
	}
	
	VIndex count = inStyles->GetChildCount();
	for (int i = 1; i <= count; i++)
		update = _ApplyStylesRec( inStyles->GetNthChild(i)) || update;
	
	return update;
}


/** apply style (use style range) */
bool VDocParagraphLayout::ApplyStyle( const VTextStyle* inStyle, bool inFast, bool inIgnoreIfEmpty)
{
	if (_ApplyStyle( inStyle, inIgnoreIfEmpty))
	{
		SetDirty();
		fSelIsDirty = true;
		return true;
	}
	return false;
}



bool  VDocParagraphLayout::_ApplyStyle(const VTextStyle *inStyle, bool inIgnoreIfEmpty)
{
	if (!inStyle)
		return false;
	sLONG start, end;
	inStyle->GetRange( start, end);
	if (start > end || inStyle->IsUndefined())
		return false;
	if (inIgnoreIfEmpty && GetTextLength() > 0)
		if (start == end)
			return false;

	bool needUpdate = false;
	VTextStyle *newStyle = NULL;
	if (start <= 0 && end >= GetTextLength())
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
			}
			font->Release();
		}
		if (inStyle->GetHasForeColor())
		{
			//modify current default text color
			VColor color;
			color.FromRGBAColor( inStyle->GetColor());
			if (color != fDefaultTextColor)
			{
				fDefaultTextColor = color;
				ReleaseRefCountable(&fDefaultStyle);
				needUpdate = true;
			}
		}
		if (inStyle->GetJustification() != JST_Notset)
		{
			//modify current default horizontal justification
			AlignStyle align = JustToAlignStyle( inStyle->GetJustification());
			if (align != fTextAlign)
			{
				SetTextAlign( align);
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
		SpanRefTextTypeMask textTypeMask = spanRef->CombinedTextTypeMaskToTextTypeMask( fShowRefs);
		if (spanRef->ShowRef() != textTypeMask || spanRef->ShowHighlight() != fHighlightRefs)
		{
			if (inStyle != newStyle)
			{
				newStyle = new VTextStyle( inStyle);
				inStyle = newStyle;
			}
			newStyle->GetSpanRef()->ShowRef( textTypeMask);
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
			}
		}
		else
		{
			fStyles = new VTreeTextStyle( new VTextStyle());
			fStyles->GetData()->SetRange(0, GetTextLength());

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

	if (needUpdate && fStyles && start < end && end < fText.GetLength() && fText[end] == 13)
	{
		//ensure terminating CR will inherit style if it is contiguous to replaced range (terminating CR should always inherit style from last line character)
		fStyles->ExpandAtPosBy(end, 1);
		fStyles->Truncate(fText.GetLength());
	}
	return needUpdate;
}

void VDocParagraphLayout::_UpdateTextInfo()
{
	VDocBaseLayout::_UpdateTextInfo(); //update fTextStart

	fTextLength = fText.GetLength();
}

VTextStyle*	VDocParagraphLayout::CreateTextStyle( const VFont *inFont, const GReal inDPI)
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
VTextStyle*	VDocParagraphLayout::CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI)
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

/** consolidate end of lines
@remarks
	if text is multiline:
		we convert end of lines to a single CR 
	if text is singleline:
		if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
		if inEscaping, we escape line endings and tab 
*/
bool VDocParagraphLayout::ConsolidateEndOfLines(bool inIsMultiLine, bool inEscaping, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	VString text;
	if (ITextLayout::ConsolidateEndOfLines( fText, text, fStyles, inIsMultiLine, inEscaping))
	{
		fText = text;
		SetDirty();
		fSelIsDirty = true;
		return true;
	}
	else
		return false;
}


/** adjust selection 
@remarks
	you should call this method to adjust selection if styles contain span references 
*/
void VDocParagraphLayout::AdjustRange(sLONG& ioStart, sLONG& ioEnd) const 
{ 
	if (ioEnd < 0)
		ioEnd = fText.GetLength();
	if (ioStart == ioEnd)
	{
		//ensure to keep range empty 
		AdjustPos(ioStart,false);
		ioEnd = ioStart;
		return;
	}
	ITextLayout::AdjustWithSurrogatePairs( ioStart, ioEnd, fText);
	if (fStyles) 
		fStyles->AdjustRange( ioStart, ioEnd); 
}

/** adjust char pos
@remarks
	you should call this method to adjust char pos if styles contain span references 
*/
void VDocParagraphLayout::AdjustPos(sLONG& ioPos, bool inToEnd) const 
{ 
	ITextLayout::AdjustWithSurrogatePairs( ioPos, fText, inToEnd);
	if (fStyles) 
		fStyles->AdjustPos( ioPos, inToEnd); 
}

/** unmerge paragraph(s) on the specified range 
@remarks
	any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR, 
	resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)

	remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
	you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
*/

typedef std::pair<sLONG,sLONG> ParaRange;
typedef std::vector<ParaRange> VectorOfParaRange;

bool VDocParagraphLayout::UnMergeParagraphs( sLONG inStart, sLONG inEnd)
{
	if (!fParent)
		return false; //only if it is has a parent

	_NormalizeRange( inStart, inEnd);
	if (inStart >= inEnd)
		return false;

	VectorOfParaRange ranges;
	const UniChar *c = fText.GetCPointer();
	sLONG index = 0;
	sLONG rangeStart = 0;
	bool isLast = IsLastChild(true);
	while (*c)
	{
		if (*c == 13 && index >= inStart && index < inEnd) //we break only if CR is on the range
		{
			if (isLast || index+1 < fText.GetLength()) //if not a paragraph terminating CR
			{
				ranges.push_back(ParaRange(rangeStart, index+1));
				rangeStart = index+1;
			}
		}
		c++;
		index++;
	}
	if (ranges.size() > 0)
		ranges.push_back(ParaRange(rangeStart, index));

	if (ranges.size() <= 1)
		return false;
	
	//first add paragraphs

	VDocParagraph *para = new VDocParagraph();
	SetPropsTo( para);

	bool noUpdateTextInfo = fNoUpdateTextInfo;
	fNoUpdateTextInfo = true;

	VectorOfParaRange::const_iterator itRange = ranges.begin();
	itRange++;
	sLONG childIndex = fChildIndex+1;
	for (;itRange != ranges.end(); itRange++)
	{
		VDocParagraphLayout *paraLayout = new VDocParagraphLayout(para, fShouldEnableCache);
		VString text;
		if (itRange->second > itRange->first)
			fText.GetSubString(itRange->first+1, itRange->second-itRange->first, text);
		VTreeTextStyle *styles = RetainStyles( itRange->first, itRange->second, true);
		paraLayout->SetText( text, styles, false);
		ReleaseRefCountable(&styles);
		if (fExtStyles)
		{
			VTreeTextStyle *styles = fExtStyles->CreateTreeTextStyleOnRange( itRange->first, itRange->second);
			if (styles && !styles->IsUndefined())
				paraLayout->SetExtStyles( styles, false);
			else
				paraLayout->SetExtStyles( NULL);
			ReleaseRefCountable(&styles);
		}
		
		fParent->InsertChild(static_cast<IDocBaseLayout *>(static_cast<VDocBaseLayout *>(paraLayout)), childIndex++);
	}
	ReleaseRefCountable(&para);

	fNoUpdateTextInfo = noUpdateTextInfo;

	//now modify this paragraph

	fText.Truncate(ranges[0].second);
	if (fStyles)
		fStyles->Truncate(ranges[0].second);
	if (fExtStyles)
		fExtStyles->Truncate(ranges[0].second);

	fTextLength = fText.GetLength(); //actual length

	_CheckStylesConsistency();
	fStylesUseFontTrueTypeOnly = _StylesUseFontTrueTypeOnly();

	_UpdateTextInfoEx(true);

	SetDirty();

	return true;
}


/** consolidate end of lines
@remarks
	if text is multiline:
		we convert end of lines to a single CR 
	if text is singleline:
		if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
		if inEscaping, we escape line endings and tab 
*/
bool ITextLayout::ConsolidateEndOfLines(const VString &inText, VString &outText, VTreeTextStyle *ioStyles, bool inIsMultiLine, bool inEscaping)
{
	if (inText.IsEmpty())
		return false;

	outText.SetEmpty();
	outText.EnsureSize(inText.GetLength());

	UniChar charPrev = 0;
	const UniChar *c = inText.GetCPointer();
	sLONG index = 0;
	bool doUpdate = false;
	if (inIsMultiLine)
		//multiline: consolidate end of lines to CR
		while (*c)
		{
			if (charPrev == 13 && *c == 10) //CRLF
			{
				if (ioStyles)
					ioStyles->Truncate(index, 1);
				charPrev = 9; //to avoid to skip next 13 if contiguous
				c++;
				doUpdate = true;
			}
			else if (charPrev == 10 && *c == 13) //LFCR (inversed)
			{
				if (ioStyles)
					ioStyles->Truncate(index, 1);
				charPrev = 9; //to avoid to skip next 10 if contiguous
				c++;
				doUpdate = true;
			}
			else if (*c == 11 || *c == 12) //end of paragraph & vertical end 
			{
				charPrev = *c; 
				outText.AppendUniChar(13);
                c++;
				doUpdate = true;
			}
			else if (*c == 10)
			{
				charPrev = 10;
				outText.AppendUniChar(13);
                c++;
				doUpdate = true;
			}
			else
			{
				charPrev = *c;
				outText.AppendUniChar(*c++);
				index++;
			}
		}
	else
		//singleline
		if (inEscaping)
		{
			//escape control chars
			while (*c)
			{
				if ((*c >= 9 && *c <= 13) || *c == '\\')
				{
					if (*c == '\n')
					{
						//line feed
						outText.AppendString("\\n");
						if (ioStyles)
							ioStyles->ExpandAtPosBy( index, 1);
						index++;
					}
					else if (*c == '\r')
					{
						//carriage return
						outText.AppendString("\\r");
						if (ioStyles)
							ioStyles->ExpandAtPosBy( index, 1);
						index++;
					}
					else if (*c == '\f')
					{
						//form feed
						outText.AppendString("\\f");
						if (ioStyles)
							ioStyles->ExpandAtPosBy( index, 1);
						index++;
					}
					else if (*c == '\t')
					{
						//horizontal tab
						outText.AppendString("\\t");
						if (ioStyles)
							ioStyles->ExpandAtPosBy( index, 1);
						index++;
					}
					else if (*c == '\\')
					{
						//escape char
						outText.AppendString("\\\\");
						if (ioStyles)
							ioStyles->ExpandAtPosBy( index, 1);
						index++;
					}
					else 
					{
						//vertical tab
						outText.AppendString("\\v");
						if (ioStyles)
							ioStyles->ExpandAtPosBy( index, 1);
						index++;
					}

					c++;
					doUpdate = true;
				}
				else
					outText.AppendUniChar(*c++);
				index++;
			}
		}
		else 
			//replace line endings with whitespaces
			while (*c)
			{
				if (*c >= 10 && *c <= 13)
				{
					if (charPrev == ' ')
					{
						//truncate end of line if previous char is a whitespace yet
						if (ioStyles)
							ioStyles->Truncate(index, 1);
					}
					else
					{
						//replace end of line with whitespace
						outText.AppendUniChar(' ');
						charPrev = ' ';
						index++;
					}
					c++;
					doUpdate = true;
				}
				else
				{
					charPrev = *c;
					outText.AppendUniChar(*c++);
					index++;
				}
			}

	return doUpdate;
}


void ITextLayout::AdjustWithSurrogatePairs( sLONG& ioStart, sLONG &ioEnd, const VString& inText)
{
	//round unicode range to surrogate pair limits (cf http://en.wikipedia.org/wiki/UTF-16)
	if (ioEnd < 0)
		ioEnd = inText.GetLength();
	xbox_assert(ioStart <= ioEnd);
	if (ioStart == ioEnd)
	{
		AdjustWithSurrogatePairs(ioStart, inText, false);
		ioEnd = ioStart;
	}
	else
	{
		AdjustWithSurrogatePairs(ioStart, inText, false);
		AdjustWithSurrogatePairs(ioEnd, inText, true);
	}
}


void ITextLayout::AdjustWithSurrogatePairs( sLONG& ioPos, const VString& inText, bool inToEnd)
{
	//round unicode index to surrogate pair limits (cf http://en.wikipedia.org/wiki/UTF-16)

	if (ioPos <= 0 || ioPos >= inText.GetLength())
		return;
	UniChar c = inText.GetUniChar(ioPos+1);
	if (c >= 0xDC00 && c <= 0xDFFF) //trail surrogate -> we might be on the middle of a surrogate pair -> check previous code for lead surrogate
	{
		c = inText.GetUniChar(ioPos);
		if (c >= 0xD800 && c <= 0xDBFF) //lead surrogate -> position is on the middle of a surrogate -> round according to direction 
		{
			if (inToEnd)
				ioPos++;
			else
				ioPos--;
		}
	}
}


/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) at the specified character index */
void ITextLayout::GetWordRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces, bool inTrimPunctuation)
{
	GetWordOrParagraphRange( inIntlMgr, GetText(), inOffset, outStart, outEnd, false, inTrimSpaces, inTrimPunctuation);
}

/**	get paragraph range at the specified character index 
@remarks
	if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
	(for VTextLayout, parameter is ignored)
*/
void ITextLayout::GetParagraphRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange)
{
	GetWordOrParagraphRange( inIntlMgr, GetText(), inOffset, outStart, outEnd, true, false, false);
}

/**	get word (or space or punctuation) or paragraph range at the specified character offset */
void ITextLayout::GetWordOrParagraphRange( VIntlMgr *inIntlMgr, const VString& inText, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inGetParagraph, bool inTrimSpaces, bool inTrimPunctuation)
{
	//select a word, punctuation, white space or paragraph

	if (inGetParagraph)
	{
		inTrimSpaces = false;
		inTrimPunctuation = false;
	}

	VIndex lenText = inText.GetLength();
	if (lenText == 0 || !inIntlMgr)
	{
		outStart = outEnd = 0;
		return;
	}

	if (inOffset < 0)
		inOffset = 0;
	if (inOffset > lenText)
		inOffset = lenText;

	sLONG charIndex = inOffset;
	const UniChar *c = inText.GetCPointer()+charIndex;
	if (*c == 0 && charIndex > 0)
	{
		charIndex--;
		c--;
	}

	if (*c)
	{
		if (inTrimSpaces && inTrimPunctuation)
		{
			//skip spaces and punctuation
			if (inIntlMgr->IsSpace(*c) || inIntlMgr->IsPunctuation( *c))
			{
				while (charIndex > 0 && *(c-1) != 13 && *(c-1) != 10 && (inIntlMgr->IsSpace(*c) || inIntlMgr->IsPunctuation( *(c))))
				{
					charIndex--;
					c--;
				}
				while (*c && (inIntlMgr->IsSpace(*c) || inIntlMgr->IsPunctuation( *(c))))
				{
					charIndex++;
					c++;
				}
			}
		}
		else if (inTrimSpaces)
		{
			//skip spaces 
			if (inIntlMgr->IsSpace(*c))
			{
				while (charIndex > 0 && *(c-1) != 13 && *(c-1) != 10 && inIntlMgr->IsSpace(*c))
				{
					charIndex--;
					c--;
				}
				while (*c && inIntlMgr->IsSpace(*c))
				{
					charIndex++;
					c++;
				}
			}
		}
		else if (inTrimPunctuation)
		{
			//skip punctuation
			if (inIntlMgr->IsPunctuation( *c))
			{
				while (charIndex > 0 && *(c-1) != 13 && *(c-1) != 10 && inIntlMgr->IsPunctuation( *(c)))
				{
					charIndex--;
					c--;
				}
				while (*c && inIntlMgr->IsPunctuation( *(c)))
				{
					charIndex++;
					c++;
				}
			}
		}
	}

	if (*c)
	{
		if (!inTrimSpaces && !inGetParagraph && charIndex < lenText && inIntlMgr->IsSpace(*c))
		{
			//get space range

			sLONG start = charIndex;
			sLONG end = charIndex;
			const UniChar *cstart = c;
			const UniChar *cend = c;
			while (start > 0 && inIntlMgr->IsSpace(*(cstart-1)) && *(cstart-1) != 13 && *(cstart-1) != 10)
			{
				cstart--;
				start--;
			}
			while (*cend && *cend != 13 && *cend != 10 && inIntlMgr->IsSpace(*cend))
			{
				end++;
				cend++;
			}
			if (*cend == 13 || *cend == 10)
				end++;

			outStart = start;
			outEnd = end;

			xbox_assert(outStart >= 0 && outStart <= lenText);
			xbox_assert(outEnd >= 0 && outEnd <= lenText);
			return;
		}

		if (!inTrimPunctuation && !inGetParagraph && charIndex < lenText && inIntlMgr->IsPunctuation( *c))
		{
			//get punctuation range

			outStart = charIndex;
			outEnd = charIndex+1;

			xbox_assert(outStart >= 0 && outStart <= lenText);
			xbox_assert(outEnd >= 0 && outEnd <= lenText);
			return;
		}

		//extract current paragraph
		sLONG start = charIndex;
		sLONG end = charIndex;
		const UniChar *cstart = c;
		const UniChar *cend = c;
		while (start > 0 && *(cstart-1) != 13 && *(cstart-1) != 10)
		{
			cstart--;
			start--;
		}
		while (*cend && *cend != 13 && *cend != 10)
		{
			end++;
			cend++;
		}
		if (*cend == 13 || *cend == 10)
			end++;

		if (inGetParagraph)
		{
			//select current paragraph

			outStart = start;
			outEnd = end;

			xbox_assert(outStart >= 0 && outStart <= lenText);
			xbox_assert(outEnd >= 0 && outEnd <= lenText);
			return;
		}


		VString paragraph;
		inText.GetSubString( start+1, end-start, paragraph);

		//get word boundaries
		VectorOfStringSlice words;
		inIntlMgr->GetWordBoundariesWithOptions( paragraph, words, eKW_UseICU);	// force ICU use instead of MeCab in Japanese

		//search selected word
		charIndex = charIndex-start+1;
		VectorOfStringSlice::const_iterator it = words.begin();
		for (;it != words.end(); it++)
		{
			if (charIndex < (*it).first+(*it).second)
			{
				outStart = (*it).first-1+start;
				outEnd = (*it).first+(*it).second-1+start;

				xbox_assert(outStart >= 0 && outStart <= lenText);
				xbox_assert(outEnd >= 0 && outEnd <= lenText);
				return;
			}
		}

		//empty selection
		outStart = outEnd = inOffset;
	}
}


