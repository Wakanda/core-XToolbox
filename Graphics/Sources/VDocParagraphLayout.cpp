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

VDocParagraphLayout::VDocParagraphLayout(VTextLayout *inLayout):VDocBaseLayout()
{
	fLayout = inLayout;

	fComputingGC = NULL;

	fDPI = fTargetDPI = inLayout->fDPI;

	fCurMaxWidthTWIPS = 0;

	fRecCount = 0;
}

VDocParagraphLayout::~VDocParagraphLayout()
{
	xbox_assert(fComputingGC == NULL && fRecCount == 0);
}


/** set outside margins	(in twips) - CSS margin */
void VDocParagraphLayout::SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin)
{
	fRecCount++;
	if (fRecCount == 1)
	{
		VDocBaseLayout::SetMargins( inMargins, inCalcAllMargin);
		fLayout->SetMargins( inMargins);
	}
	fRecCount--;
}

/** set text align - CSS text-align (-1 for not defined) */
void VDocParagraphLayout::SetTextAlign( const eStyleJust inHAlign) 
{ 
	fRecCount++;
	if (fRecCount == 1)
	{
		VDocBaseLayout::SetTextAlign( inHAlign);
		fLayout->SetTextAlign( VTextLayout::JustToAlignStyle(inHAlign));
	}
	fRecCount--;
}

/** set vertical align - CSS vertical-align (-1 for not defined) */
void VDocParagraphLayout::SetVerticalAlign( const eStyleJust inVAlign) 
{ 
	fRecCount++;
	if (fRecCount == 1)
	{
		VDocBaseLayout::SetVerticalAlign( inVAlign);
		fLayout->SetParaAlign( VTextLayout::JustToAlignStyle(inVAlign));
	}
	fRecCount--;
}

/** set width (in twips) - CSS width (0 for auto) */
void VDocParagraphLayout::SetWidth(const uLONG inWidth)
{
	//TODO: to be fixed when VDocLayout is implemented
	fRecCount++;
	if (fRecCount == 1)
	{
		VDocBaseLayout::SetWidth( inWidth);
		if (inWidth)
			fLayout->SetMaxWidth( VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetWidth(true))*fLayout->fDPI/72));
	}
	fRecCount--;
}

/** set height (in twips) - CSS height (0 for auto) */
void VDocParagraphLayout::SetHeight(const uLONG inHeight)
{
	//TODO: to be fixed when VDocLayout is implemented
	fRecCount++;
	if (fRecCount == 1)
	{
		VDocBaseLayout::SetHeight( inHeight);
		if (inHeight)
			fLayout->SetMaxHeight( VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetHeight(true))*fLayout->fDPI/72));
	}
	fRecCount--;
}


/** set node properties from the current properties */
void VDocParagraphLayout::SetPropsTo( VDocNode *outNode)
{
	VDocBaseLayout::SetPropsTo( outNode);

	VDocParagraph *para = dynamic_cast<VDocParagraph *>(outNode);
	if (!testAssert(para))
		return;

	if (fLayout->IsRTL())
		para->SetDirection( kTEXT_DIRECTION_RTL);
	else
		para->SetDirection( kTEXT_DIRECTION_LTR);

	if (fLayout->fLineHeight != -1)
	{
		if (fLayout->fLineHeight < 0)
			para->SetLineHeight( (sLONG)-floor((-fLayout->fLineHeight)*100+0.5)); //percentage
		else
			para->SetLineHeight( ICSSUtil::PointToTWIPS( fLayout->fLineHeight)); //absolute line height
	}

	if (fLayout->fTextIndent != 0)
	{
		if (fLayout->fTextIndent >= 0)
			para->SetTextIndent( ICSSUtil::PointToTWIPS( fLayout->fTextIndent)); 
		else
			para->SetTextIndent( -ICSSUtil::PointToTWIPS( -fLayout->fTextIndent));
	}

	uLONG tabStopOffsetTWIPS = (uLONG)ICSSUtil::PointToTWIPS( fLayout->fTabStopOffset);
	if (tabStopOffsetTWIPS != (uLONG)para->GetDefaultPropValue( kDOC_PROP_TAB_STOP_OFFSET))
		para->SetTabStopOffset( tabStopOffsetTWIPS);
	if (fLayout->fTabStopType != TTST_LEFT)
		para->SetTabStopType( (eDocPropTabStopType)fLayout->fTabStopType);
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
		if (inPara->GetDirection() == kTEXT_DIRECTION_RTL)
			fLayout->SetRTL( true);
		else
			fLayout->SetRTL( false);
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
		fLayout->SetLineHeight( lineHeight);
	}

	if (!inIgnoreIfInherited 
		|| 
		(inPara->IsOverriden( kDOC_PROP_TEXT_INDENT) && !inPara->IsInherited( kDOC_PROP_TEXT_INDENT)))
	{
		if (inPara->GetTextIndent() >= 0)
			fLayout->SetTextIndent( ICSSUtil::TWIPSToPoint(inPara->GetTextIndent()));
		else
			fLayout->SetTextIndent( -ICSSUtil::TWIPSToPoint(-inPara->GetTextIndent()));
	}

	bool updateTabStop = false;
	GReal offset = fLayout->fTabStopOffset;
	eTextTabStopType type = fLayout->fTabStopType;

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
		fLayout->SetTabStop( offset, type);
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

	VString *textPtr = fLine->fParagraph->fParaLayout->fLayout->fTextPtr;
	return ((*textPtr)[fStart] == '\t');
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
		fNodeLayout->UpdateLayout( fLine->fParagraph->fParaLayout->fComputingGC);
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
			fCharOffsets.push_back( fNodeLayout->GetDesignBounds().GetWidth());
			return fCharOffsets;
		}

		bool needReleaseGC = false; 
		VGraphicContext *gc = fLine->fParagraph->fParaLayout->fComputingGC;
		if (!gc)
		{
			//might happen if called later while requesting run or caret metrics 
			gc = fLine->fParagraph->fParaLayout->_RetainComputingGC( fLine->fParagraph->fParaLayout->fLayout->fGC);
			xbox_assert(gc);
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

		metrics->MeasureText( runtext, fCharOffsets, fDirection == kPARA_RTL);	//TODO: check in all impls that MeasureText returned offsets take account pair kerning between adjacent characters, otherwise modify method (with a option)
																				// returned offsets should be also absolute offsets from start position (which must be anchored to the right with RTL, and left with LTR)
		while (fCharOffsets.size() < fEnd-fStart)
			fCharOffsets.push_back(fCharOffsets.back()); 
		
		CharOffsets::iterator itOffset = fCharOffsets.begin();
		for (;itOffset != fCharOffsets.end(); itOffset++)
			*itOffset = VTextLayout::RoundMetrics( gc, *itOffset);
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
	VGraphicContext *gc = fLine->fParagraph->fParaLayout->fComputingGC;
	xbox_assert(gc && fFont);

	if (fNodeLayout)
	{
		//run is a embedded object (image for instance)

		xbox_assert( fEnd-fStart <= 1); //embedded node must always be 1 character width

		fAscent = fDescent = 0.0001; //it is computed later in Line::ComputeMetrics as embedded node vertical alignment is dependent on line text runs metrics (HTML compliant)
									 //(we do not set to 0 as it needs to be not equal to 0 from here for fTypoBounds->IsEmpty() not equal to true - otherwise it would screw up line metrics computing)
		GReal width = fNodeLayout->GetDesignBounds().GetWidth();
		
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
	fAscent = VTextLayout::RoundMetrics(gc, metrics->GetAscent());
	fDescent = VTextLayout::RoundMetrics(gc, metrics->GetAscent()+metrics->GetDescent())-fAscent;
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
		width = VTextLayout::RoundMetrics( gc, metrics->GetTextWidth( runtext, fDirection == kPARA_RTL)); //typo width
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
			*itOffset = VTextLayout::RoundMetrics( gc, *itOffset); //width with kerning so round to nearest integral (GDI) or TWIPS boundary (other sub-pixel positionning capable context)

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
		fTabOffset = VTextLayout::RoundMetrics(NULL,fEndPos.x*72/fLine->fParagraph->fCurDPI);

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
	bool justify = wordwrap && fParagraph->fParaLayout->fLayout->GetTextAlign() == AL_JUST && fIndex+1 < fParagraph->fLines.size(); //justify only lines but the last one
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
	
	VGraphicContext *gc = fParagraph->fParaLayout->fComputingGC;
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

	fLineHeight = fParagraph->fParaLayout->fLayout->GetLineHeight();
	if (fLineHeight >= 0)
		fLineHeight = VTextLayout::RoundMetrics(gc, fLineHeight*fParagraph->fCurDPI/72);
	else
	{
		fLineHeight = (fAscent+fDescent)*(-fLineHeight);
		fLineHeight = VTextLayout::RoundMetrics( gc, fLineHeight);
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

	VGraphicContext *gc = fParaLayout->fComputingGC;
	xbox_assert(gc);

	xbox_assert(pos >= 0);
	sLONG abspos = ICSSUtil::PointToTWIPS(pos);
	sLONG taboffset = ICSSUtil::PointToTWIPS( fParaLayout->fLayout->GetTabStopOffset()*fCurDPI/72);
	if (taboffset)
	{
		//compute absolute tab pos
		sLONG div = abspos/taboffset;
		GReal oldpos = pos;
		pos = VTextLayout::RoundMetrics(gc, ICSSUtil::TWIPSToPoint( (div+1)*taboffset));  
		if (pos == oldpos) //might happen because of pixel snapping -> pick next tab
			pos = VTextLayout::RoundMetrics(gc, ICSSUtil::TWIPSToPoint( (div+2)*taboffset));  
	}
	else
	{
		//if tab offset is 0 -> tab width = whitespace width
		VGraphicContext *gc = fParaLayout->fComputingGC;
		xbox_assert(gc);
		VFontMetrics *metrics = new VFontMetrics( font, gc);
		pos += VTextLayout::RoundMetrics(gc, metrics->GetCharWidth( ' '));
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
		textColor = run->fLine->fParagraph->fParaLayout->fLayout->fDefaultTextColor;
	
	if (style->GetHasBackColor() && style->GetBackGroundColor() != RGBACOLOR_TRANSPARENT)
		backColor.FromRGBAColor( style->GetBackGroundColor());
	else
		backColor = VColor(0,0,0,0);

	_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, textColor, backColor, lineWidth, i, false);

	//bind span ref node layout to this run (if it is a embedded object)
	if (style->IsSpanRef() && testAssert(i < fEnd && style->GetSpanRef()->GetType() == kSpanTextRef_Image))
		run->SetNodeLayout( dynamic_cast<IDocNodeLayout *>( style->GetSpanRef()->GetLayoutDelegate()));
}


void VDocParagraphLayout::VParagraph::_NextLine( Run* &run, Line* &line, sLONG i)
{
	xbox_assert(run == (line->thisRuns().back()));

	VGraphicContext *gc = fParaLayout->fComputingGC;
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

		//take account negative line padding if any (lines but the first are decaled with -fParaLayout->fLayout->GetTextIndent())
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

	VGraphicContext *gc = fParaLayout->fComputingGC;
	xbox_assert(gc);
	
	if (fCurDPI != fParaLayout->fDPI)
		fMaxWidth = 0;
	fCurDPI = fParaLayout->fDPI;

	const UniChar *textPtr = fParaLayout->fLayout->fTextPtr->GetCPointer()+fStart;
	if (!inUpdateBoundsOnly)
	{
		//determine bidi 

		fDirection = fParaLayout->fLayout->IsRTL() ? kPARA_RTL : kPARA_LTR; //it is desired base direction 
		VIntlMgr::GetDefaultMgr()->GetBidiRuns( textPtr, fEnd-fStart, fBidiRuns, fBidiVisualToLogicalMap, (sLONG)fDirection, true);
		
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
							styleSource = fParaLayout->fLayout->GetStyleRefAtRange( rangeStart, rangeEnd);
							const VTextStyle *styleSpanRef = testAssert(styleSource) ? styleSource->GetSpanRef()->GetStyle() : NULL;
							if (styleSpanRef)
								style->MergeWithStyle( styleSpanRef);
							if (styleSource->GetSpanRef()->GetType() == kSpanTextRef_Image)
							{
								//create image layout & store to temporary map
								VDocImageLayout *imageLayout = new VDocImageLayout( styleSource->GetSpanRef()->GetImage());
								imageLayout->SetDPI( fCurDPI);
								imageLayout->UpdateLayout( gc);
								mapImageLayout[rangeStart] = imageLayout;
							}
							else
								style->SetSpanRef(NULL); //discard span ref if not a image (useless now)						
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

	fWordWrap = inMaxWidth && (!(fParaLayout->fLayout->GetLayoutMode() & TLM_DONT_WRAP));
	GReal maxWidth = floor(inMaxWidth);

	if (fLines.empty() || !inUpdateBoundsOnly || (!inUpdateDPIMetricsOnly && fWordWrap && maxWidth != fMaxWidth))
	{
		//compute lines: take account styles, direction, line breaking (if max width != 0), tabs & text indent to build lines & runs

		Lines::const_iterator itLine = fLines.begin();
		for (;itLine != fLines.end(); itLine++)
			delete (*itLine);
		fLines.clear();

		fMaxWidth = maxWidth;
		fTextIndent = VTextLayout::RoundMetrics(gc, fParaLayout->fLayout->GetTextIndent()*fCurDPI/72);

		//backup default font
		GReal fontSizeBackup = fParaLayout->fLayout->fDefaultFont->GetPixelSize();
		VFont *fontBackup = fParaLayout->fLayout->RetainDefaultFont( fCurDPI);
		xbox_assert(fontBackup);
		GReal fontSize = fontSizeBackup;
		VFont *font = RetainRefCountable( fontBackup);

		//parse text 
		const UniChar *c = textPtr;
		VIndex lenText = fEnd-fStart;

		sLONG bidiVisualIndex = fDirection == kPARA_RTL ? 1000000 : -1;

		Line *line = new Line( this, 0, fStart, fStart);
		fLines.push_back( line);
		
		Run *run = new Run(line, 0, bidiVisualIndex, fStart, fStart, fDirection, fontSize, font, &(fParaLayout->fLayout->fDefaultTextColor));
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
			bool justify = fWordWrap && fParaLayout->fLayout->GetTextAlign() == AL_JUST;

			VectorOfStringSlice words;
			if (fWordWrap)
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
							_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, fParaLayout->fLayout->fDefaultTextColor, VColor(0,0,0,0), lineWidth, i);
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
								_NextRun( run, line, dir, bidiVisualIndex, fontSize, font, fParaLayout->fLayout->fDefaultTextColor, VColor(0,0,0,0), lineWidth, i);
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
							c = fParaLayout->fLayout->fTextPtr->GetCPointer()+endLine;
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

			fTextIndent = VTextLayout::RoundMetrics(gc, fParaLayout->fLayout->GetTextIndent()*fCurDPI/72);
			
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
						GReal tabOffset = VTextLayout::RoundMetrics(gc, run->fTabOffset*fCurDPI/72);
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
	if (fLayout->fLayoutIsValid && !fLayout->fNeedUpdateBounds)
		return;

	xbox_assert(fComputingGC == NULL && inGC == fLayout->fGC);
	fComputingGC = _RetainComputingGC(fLayout->fGC);
	xbox_assert(fComputingGC);
	fComputingGC->BeginUsingContext(true);
	if (fComputingGC->IsFontPixelSnappingEnabled())
	{
		//for GDI, the best for now is still to compute GDI metrics at the target DPI:
		//need to find another way to not break wordwrap lines while changing DPI with GDI
		//(no problem with DWrite or CoreText)
		
		//if printing, compute metrics at printer dpi
		if (fComputingGC->HasPrinterScale())
		{
			GReal scaleX, scaleY;
			fComputingGC->GetPrinterScale( scaleX, scaleY);
			GReal dpi = fLayout->fDPI*scaleX;
			
			GReal targetDPI = fLayout->fDPI;
			if (fDPI != dpi)
				SetDPI( dpi);
			fTargetDPI = targetDPI;
		}
		else
			//otherwise compute at desired DPI
			if (fDPI != fLayout->fDPI)
				SetDPI( fLayout->fDPI);
			else
				fTargetDPI = fDPI;
	}
	else
	{
		//metrics are not snapped to pixel -> compute metrics at 72 DPI (with TWIPS precision) & scale context to target DPI (both DWrite & CoreText are scalable graphic contexts)
		if (fDPI != 72)
		{
			GReal targetDPI = fLayout->fDPI;
			bool layoutIsValid = fLayout->fLayoutIsValid;
			SetDPI( 72);
			fLayout->fLayoutIsValid = layoutIsValid;
			fTargetDPI = targetDPI;
		}
		else
			fTargetDPI = fLayout->fDPI;
	}

	GReal maxWidth		= fWidthTWIPS ? VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(fWidthTWIPS)*fDPI/72) : fLayout->fMaxWidthInit; 
	GReal maxHeight		= fHeightTWIPS ? VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(fHeightTWIPS)*fDPI/72) : fLayout->fMaxHeightInit; 

	if (fTargetDPI != fDPI)
	{
		if (!fWidthTWIPS)
			maxWidth = VTextLayout::RoundMetrics(NULL,maxWidth*fDPI/fTargetDPI);
		if (!fHeightTWIPS)
			maxHeight = VTextLayout::RoundMetrics(NULL,maxHeight*fDPI/fTargetDPI);
	}

	fMargin.left		= VTextLayout::RoundMetrics(fComputingGC,fAllMargin.left); 
	fMargin.right		= VTextLayout::RoundMetrics(fComputingGC,fAllMargin.right); 
	fMargin.top			= VTextLayout::RoundMetrics(fComputingGC,fAllMargin.top); 
	fMargin.bottom		= VTextLayout::RoundMetrics(fComputingGC,fAllMargin.bottom); 

	if (!fWidthTWIPS)
	{
		maxWidth -= fMargin.left+fMargin.right;
		if (maxWidth < 0)
			maxWidth = 0;
	}
	if (!fHeightTWIPS)
	{
		maxHeight -= fMargin.top+fMargin.bottom;
		if (maxHeight < 0)
			maxHeight = 0;
	}

	//determine paragraph(s)

	//clear & rebuild

	if (!fLayout->fLayoutIsValid)
	{
		fCurMaxWidthTWIPS = 0;

		fParagraphs.reserve(10);
		fParagraphs.clear();

		const UniChar *c = fLayout->fTextPtr->GetCPointer();
		sLONG start = 0;
		sLONG i = start;

		sLONG index = 0;
		while (*c && i < fLayout->fTextLength)
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
	}

	//compute paragraph(s) metrics

	GReal typoWidth = 0.0f;
	GReal typoHeight = 0.0f;

	//determine if we recalc only run & line metrics because of DPI change
	bool updateDPIMetricsOnly = false;
	sLONG maxWidthTWIPS = ICSSUtil::PointToTWIPS(maxWidth*72/fDPI);
	if (fLayout->fLayoutIsValid && !fComputingGC->IsFontPixelSnappingEnabled())
	{
		if (fLayout->fLayoutMode & TLM_DONT_WRAP)
		{
			updateDPIMetricsOnly = fCurMaxWidthTWIPS == 0;
			fCurMaxWidthTWIPS = 0;
		}
		else
		{
			if (fCurMaxWidthTWIPS == maxWidthTWIPS)
				updateDPIMetricsOnly = true;
			else
				fCurMaxWidthTWIPS = maxWidthTWIPS;
		}
	}
	else
		fCurMaxWidthTWIPS = (fLayout->fLayoutMode & TLM_DONT_WRAP) != 0 ? 0 : maxWidthTWIPS;

	Paragraphs::iterator itPara = fParagraphs.begin();
	for (;itPara != fParagraphs.end(); itPara++)
	{
		VParagraph *para = itPara->Get();
		para->ComputeMetrics(fLayout->fLayoutIsValid, updateDPIMetricsOnly, maxWidth); 
		para->SetStartPos(VPoint(0,typoHeight));
		typoWidth = std::max(typoWidth, para->GetTypoBounds(false).GetWidth());
		typoHeight = typoHeight+para->GetLayoutBounds().GetHeight();
	}

	typoWidth += fMargin.left+fMargin.right;
	typoHeight += fMargin.top+fMargin.bottom;

	GReal layoutWidth = 0.0f;
	GReal layoutHeight = 0.0f;
	if (maxWidth)
		layoutWidth = maxWidth+fMargin.left+fMargin.right;
	else
		layoutWidth = typoWidth;
	if (maxHeight)
		layoutHeight = maxHeight+fMargin.top+fMargin.bottom;
	else
		layoutHeight = typoHeight;

	fTypoBounds.SetCoords( 0, 0, typoWidth, typoHeight);
	fLayoutBounds.SetCoords( 0, 0, layoutWidth, layoutHeight);

	//here we update VTextLayout metrics

	if (fTargetDPI != fDPI)
	{
		typoWidth = typoWidth*fTargetDPI/fDPI;
		typoHeight = typoHeight*fTargetDPI/fDPI;
	}

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

	_SetDesignBounds( fLayoutBounds);

	ReleaseRefCountable(&fComputingGC);
}
					
void VDocParagraphLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw)
{
	//apply vertical align offset
	GReal y = 0;
	switch (fLayout->GetParaAlign())
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
	AlignStyle align = fLayout->GetTextAlign();
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
			dx = fMargin.left+VTextLayout::RoundMetrics(fComputingGC, (layoutWidth-inLine->GetTypoBounds(false).GetLeft())*0.5);
		else
			dx = fMargin.left+VTextLayout::RoundMetrics(fComputingGC, (layoutWidth-inLine->GetTypoBounds(false).GetRight())*0.5);
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

	StUseContext_NoRetain context(inGC);
	fLayout->BeginUsingContext(inGC); 

	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
		return;
	}

	VColor selPaintColor;
	if (fLayout->fShouldPaintSelection)
	{
		if (fLayout->fSelIsActive)
			selPaintColor = fLayout->fSelActiveColor;
		else
			selPaintColor = fLayout->fSelInactiveColor;
		if (inGC->IsGDIImpl())
			//GDI has no transparency support
			selPaintColor.SetAlpha(255);
	}

	if (!fLayout->_BeginDrawLayer( inTopLeft))
	{
		//text has been refreshed from layer: we do not need to redraw text content
		fLayout->_EndDrawLayer();
		fLayout->EndUsingContext();
		return;
	}

	{
	StEnableShapeCrispEdges crispEdges( inGC);
	
	VAffineTransform ctm;
	_BeginLocalTransform( inGC, ctm, inTopLeft, true); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space pixel is mapped to fLayout->fDPI (72 on default so on default 1px = 1pt)
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

		if (drawSelection && fLayout->fShouldPaintSelection && fLayout->fSelStart < fLayout->fSelEnd)
		{
			//draw selection color: we paint sel color after char back color but before the text (Word & Mac-like)
			_EndLocalTransform( inGC, ctm, true);
			fLayout->_DrawSelection( inTopLeft);
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

							//here pos is sticked to x = node design bounds left & y = baseline (IDocNodeLayout::Draw will adjust y according to ascent as while drawing text)
							VPoint pos = linePosText + run->GetStartPos();
							bool isRTL = run->GetDirection() == kPARA_RTL;
							if (isRTL)
								pos.SetPosBy(-nodeLayout->GetDesignBounds().GetWidth(),0);
	
							GReal opacity = 1.0f;
							if (run->fStart >= fLayout->fSelStart && run->fEnd <= fLayout->fSelEnd)
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
							fLayout->fTextPtr->GetSubString( run->GetStart()+1, run->GetEnd()-run->GetStart(), runtext);
							
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

	fLayout->_EndDrawLayer();
	fLayout->EndUsingContext();
}


void VDocParagraphLayout::GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
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

void VDocParagraphLayout::GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
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

	fLayout->EndUsingContext();
}

void VDocParagraphLayout::GetCaretMetricsFromCharIndex(	VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex _inCharIndex, 
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

	fLayout->EndUsingContext();
}

/** get paragraph from local position (relative to text box origin+vertical align decal & res-independent - 72dpi)*/
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
	xbox_assert(inGC);

	if (fLayout->fTextLength == 0)
	{
		outCharIndex = 0;
		if (outRunLength)
			*outRunLength = 0;
		return false;
	}

	fLayout->BeginUsingContext(inGC, true);
	if (!testAssert(fLayout->fLayoutIsValid))
	{
		fLayout->EndUsingContext();
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
bool VDocParagraphLayout::MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
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
bool VDocParagraphLayout::MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
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
void VDocParagraphLayout::ReplaceText( sLONG inStart, sLONG inEnd, const VString& inText)
{
	//TODO (for now rebuild all paragraphs)
	fLayout->fLayoutIsValid = false;
}

/** apply character style (use style range) */
void VDocParagraphLayout::ApplyStyle( const VTextStyle* inStyle)
{
	//TODO (for now rebuild all paragraphs)
	fLayout->fLayoutIsValid = false;
}
