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

#undef min
#undef max


VDocBaseLayout::VDocBaseLayout():VObject()
{
	fMarginInTWIPS.left = fMarginInTWIPS.right = fMarginInTWIPS.top = fMarginInTWIPS.bottom = 0;
	fMarginOutTWIPS.left = fMarginOutTWIPS.right = fMarginOutTWIPS.top = fMarginOutTWIPS.bottom = 0;
	fMarginOut.left = fMarginOut.right = fMarginOut.top = fMarginOut.bottom = 0;
	fMarginIn.left = fMarginIn.right = fMarginIn.top = fMarginIn.bottom = 0;
	fAllMargin.left = fAllMargin.right = fAllMargin.top = fAllMargin.bottom = 0;
	fBorderWidth.left = fBorderWidth.right = fBorderWidth.top = fBorderWidth.bottom = 0;
	fBorderLayout = NULL;

	fWidthTWIPS = fHeightTWIPS = fMinWidthTWIPS = fMinHeightTWIPS = 0; //automatic on default

	fTextAlign = fVerticalAlign = JST_Default; //default

	fBackgroundColor = VColor(0,0,0,0); //transparent

	fDPI = fTargetDPI = 72;
}

VDocBaseLayout::~VDocBaseLayout()
{
	if (fBorderLayout)
		delete fBorderLayout;
}

void VDocBaseLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw, const VPoint& inPreDecal)
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

	//apply decal in res-independent coordinate space
	xform.Translate(inPreDecal.GetX(), inPreDecal.GetY());

	//apply printer scaling if appropriate
	
	GReal scalex = 1, scaley = 1;
	if (inGC->HasPrinterScale() && inGC->ShouldScaleShapesWithPrinterScale()) 
	{
		xbox_assert( inGC->IsGDIImpl()); 
		inGC->GetPrinterScale( scalex, scaley);
		origin.x = origin.x*scalex; //if printing, top left position is scaled also
		origin.y = origin.y*scaley;
	}

	if (fTargetDPI != fDPI)
	{
		scalex *= fTargetDPI/fDPI;
		scaley *= fTargetDPI/fDPI;
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

void VDocBaseLayout::_EndLocalTransform( VGraphicContext *inGC, const VAffineTransform& inCTM, bool inDraw)
{
	if (!inDraw)
		return;
	inGC->UseReversedAxis();
	inGC->SetTransform( inCTM);
}

void VDocBaseLayout::_BeginLayerOpacity(VGraphicContext *inGC, const GReal inOpacity)
{
	//called after _BeginLocalTransform so origin is aligned to element top left origin in fDPI coordinate space

	if (inOpacity == 1.0f)
		return;
	if (inGC->IsPrintingContext() && !inGC->IsQuartz2DImpl())
		//if printing, do not use layer but apply opacity per shape
		fOpacityBackup = inGC->SetTransparency( inOpacity);
	else
	{
		VRect bounds;
		if (fBorderLayout)
			//we use border layout clip rectangle in order background painting to be aligned according to crisp edges status
			//(which is dependent on both drawing rect & line width)
			fBorderLayout->GetClipRect( inGC, bounds);
		else
			//if none borders, we can paint directly using clip bounds without margins
			bounds = fBackgroundBounds;
		inGC->BeginLayerTransparency( bounds, inOpacity);
	}
}

void VDocBaseLayout::_EndLayerOpacity(VGraphicContext *inGC, const GReal inOpacity)
{
	if (inOpacity == 1.0f)
		return;
	if (inGC->IsPrintingContext() && !inGC->IsQuartz2DImpl())
		//if printing, do not use layer but apply opacity per shape
		inGC->SetTransparency( fOpacityBackup);
	else
		inGC->EndLayer();
}


/** set node properties from the current properties */
void VDocBaseLayout::SetPropsTo( VDocNode *outNode)
{
	if (!(VDocProperty(fMarginOutTWIPS) == outNode->GetDefaultPropValue( kDOC_PROP_MARGIN)))
		outNode->SetMargin( fMarginOutTWIPS);

	if (!(VDocProperty(fMarginInTWIPS) == outNode->GetDefaultPropValue( kDOC_PROP_PADDING)))
		outNode->SetPadding( fMarginInTWIPS);

	if (fWidthTWIPS != 0)
		outNode->SetWidth( fWidthTWIPS);
	if (fMinWidthTWIPS != 0)
		outNode->SetMinWidth( fMinWidthTWIPS);

	if (fHeightTWIPS != 0)
		outNode->SetHeight( fHeightTWIPS);
	if (fMinHeightTWIPS != 0)
		outNode->SetMinHeight( fMinHeightTWIPS);

	if (fTextAlign != JST_Default)
		outNode->SetTextAlign( (eDocPropTextAlign)fTextAlign);
	if (fVerticalAlign != JST_Default)
		outNode->SetVerticalAlign( (eDocPropTextAlign)fVerticalAlign);

	if (fBackgroundColor.GetRGBAColor() != RGBACOLOR_TRANSPARENT)
		outNode->SetBackgroundColor( fBackgroundColor.GetRGBAColor());

	if (fBorderLayout)
		fBorderLayout->SetPropsTo( outNode);
}


/** initialize from the passed node */
void VDocBaseLayout::InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited)
{
	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_MARGIN) && !inNode->IsInherited( kDOC_PROP_MARGIN)))
		SetMargins( inNode->GetMargin());

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_PADDING) && !inNode->IsInherited( kDOC_PROP_PADDING)))
		SetPadding( inNode->GetPadding());

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_TEXT_ALIGN) && !inNode->IsInherited( kDOC_PROP_TEXT_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inNode->GetTextAlign();
		SetTextAlign( just);
	}

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_VERTICAL_ALIGN) && !inNode->IsInherited( kDOC_PROP_VERTICAL_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inNode->GetVerticalAlign();
		SetVerticalAlign( just);
	}

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_BACKGROUND_COLOR) && !inNode->IsInherited( kDOC_PROP_BACKGROUND_COLOR)))
	{
		RGBAColor color = inNode->GetBackgroundColor();
		VColor xcolor;
		xcolor.FromRGBAColor( color);
		SetBackgroundColor( xcolor);
	}
	
	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_BORDER_STYLE) && !inNode->IsInherited( kDOC_PROP_BORDER_STYLE)))
		_CreateBorder( inNode);

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_WIDTH) && !inNode->IsInherited( kDOC_PROP_WIDTH)))
		SetWidth( inNode->GetWidth());

		if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_MIN_WIDTH) && !inNode->IsInherited( kDOC_PROP_MIN_WIDTH)))
		SetMinWidth( inNode->GetMinWidth());

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_HEIGHT) && !inNode->IsInherited( kDOC_PROP_HEIGHT)))
		SetHeight( inNode->GetHeight());

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_MIN_HEIGHT) && !inNode->IsInherited( kDOC_PROP_MIN_HEIGHT)))
		SetMinHeight( inNode->GetMinHeight());
}


/** set layout DPI */
void VDocBaseLayout::SetDPI( const GReal inDPI)
{
	fTargetDPI = inDPI;

	if (fDPI == inDPI)
		return;
	fDPI = inDPI;

	SetMargins( fMarginOutTWIPS, false);
	SetPadding( fMarginInTWIPS, false);
	_UpdateBorderWidth(false);
	_CalcAllMargin();
}

/** set outside margins	(in twips) - CSS margin */
void VDocBaseLayout::SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin)
{
	fMarginOutTWIPS = inMargins;
    
	fMarginOut.left		= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.left) * fDPI/72);
	fMarginOut.right	= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.right) * fDPI/72);
	fMarginOut.top		= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.top) * fDPI/72);
	fMarginOut.bottom	= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.bottom) * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

/** set inside margins	(in twips) - CSS padding */
void VDocBaseLayout::SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin)
{
	fMarginInTWIPS = inPadding;
    
	fMarginIn.left		= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.left) * fDPI/72);
	fMarginIn.right		= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.right) * fDPI/72);
	fMarginIn.top		= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.top) * fDPI/72);
	fMarginIn.bottom	= VTextLayout::RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.bottom) * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

uLONG VDocBaseLayout::GetWidth(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins || !fWidthTWIPS)
		return	fWidthTWIPS;
	else
	{
		return	fWidthTWIPS+
				fMarginInTWIPS.left+fMarginInTWIPS.right+
				fMarginOutTWIPS.left+fMarginOutTWIPS.right+
				(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetLeftBorder().fWidth+fBorderLayout->GetRightBorder().fWidth) : 0);
	}
}

uLONG VDocBaseLayout::GetHeight(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins || !fHeightTWIPS)
		return	fHeightTWIPS;
	else
	{
		return	fHeightTWIPS+
				fMarginInTWIPS.top+fMarginInTWIPS.bottom+
				fMarginOutTWIPS.top+fMarginOutTWIPS.bottom+
				(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetTopBorder().fWidth+fBorderLayout->GetBottomBorder().fWidth) : 0);
	}
}

uLONG VDocBaseLayout::GetMinWidth(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins)
		return	fMinWidthTWIPS;
	else
	{
		return	fMinWidthTWIPS+
				fMarginInTWIPS.left+fMarginInTWIPS.right+
				fMarginOutTWIPS.left+fMarginOutTWIPS.right+
				(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetLeftBorder().fWidth+fBorderLayout->GetRightBorder().fWidth) : 0);
	}
}

uLONG VDocBaseLayout::GetMinHeight(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins)
		return	fMinHeightTWIPS;
	else
	{
		return	fMinHeightTWIPS+
				fMarginInTWIPS.top+fMarginInTWIPS.bottom+
				fMarginOutTWIPS.top+fMarginOutTWIPS.bottom+
				(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetTopBorder().fWidth+fBorderLayout->GetBottomBorder().fWidth) : 0);
	}
}


void VDocBaseLayout::_UpdateBorderWidth(bool inCalcAllMargin)
{
	if (!fBorderLayout)
	{
		fBorderWidth.left = fBorderWidth.right = fBorderWidth.top = fBorderWidth.bottom = 0;
		if (inCalcAllMargin)
			_CalcAllMargin();
		return;
	}
	fBorderWidth.left	= VTextLayout::RoundMetrics(NULL,fBorderLayout->GetLeftBorder().fWidth * fDPI/72);
	fBorderWidth.right	= VTextLayout::RoundMetrics(NULL,fBorderLayout->GetRightBorder().fWidth * fDPI/72);
	fBorderWidth.top	= VTextLayout::RoundMetrics(NULL,fBorderLayout->GetTopBorder().fWidth * fDPI/72);
	fBorderWidth.bottom	= VTextLayout::RoundMetrics(NULL,fBorderLayout->GetBottomBorder().fWidth * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

void VDocBaseLayout::_CalcAllMargin()
{
	fAllMargin.left		= fMarginIn.left+fBorderWidth.left+fMarginOut.left;
	fAllMargin.right	= fMarginIn.right+fBorderWidth.right+fMarginOut.right;
	fAllMargin.top		= fMarginIn.top+fBorderWidth.top+fMarginOut.top;
	fAllMargin.bottom	= fMarginIn.bottom+fBorderWidth.bottom+fMarginOut.bottom;
}

void VDocBaseLayout::_SetDesignBounds( const VRect& inBounds)
{
	fDesignBounds = inBounds;
	fBackgroundBounds = inBounds;
	
	GReal width = fDesignBounds.GetWidth()-fMarginOut.right-fMarginOut.left;
	if (width < 0) //might happen with GDI due to pixel snapping
		width = 0;
	GReal height = fDesignBounds.GetHeight()-fMarginOut.top-fMarginOut.bottom;
	if (height < 0) //might happen with GDI due to pixel snapping
		height = 0;
	fBackgroundBounds.SetCoordsTo( fDesignBounds.GetX()+fMarginOut.left, fDesignBounds.GetY()+fMarginOut.top, width, height);

	if (fBorderLayout)
	{
		//prepare border layout (border drawing rect is aligned on border center)

		width = width-fBorderWidth.left*0.5f-fBorderWidth.right*0.5;
		if (width < 0) //might happen with GDI due to pixel snapping
			width = 0;
		height = height-fBorderWidth.top*0.5f-fBorderWidth.bottom*0.5;
		if (height < 0) //might happen with GDI due to pixel snapping
			height = 0;
		VRect drawRect;
		drawRect.SetCoordsTo( fDesignBounds.GetX()+fMarginOut.left+fBorderWidth.left*0.5f, fDesignBounds.GetY()+fMarginOut.top+fBorderWidth.top*0.5, width, height);
		fBorderLayout->SetDrawRect( drawRect, fDPI);
	}
}

/** create border from passed document node */
void VDocBaseLayout::_CreateBorder(const VDocNode *inDocNode, bool inCalcAllMargin)
{
	if (fBorderLayout)
		delete fBorderLayout;
	fBorderLayout = NULL;
	if (inDocNode->IsOverriden(kDOC_PROP_BORDER_STYLE))
	{
		sDocPropRect borderStyle = inDocNode->GetBorderStyle();
		if (borderStyle.left > kTEXT_BORDER_STYLE_HIDDEN
			||
			borderStyle.right > kTEXT_BORDER_STYLE_HIDDEN
			||
			borderStyle.top > kTEXT_BORDER_STYLE_HIDDEN
			||
			borderStyle.bottom > kTEXT_BORDER_STYLE_HIDDEN)
			fBorderLayout = new VDocBorderLayout(inDocNode);
	}
	_UpdateBorderWidth();
}


/** render layout element in the passed graphic context at the passed top left origin
@remarks		
	method does not save & restore gc context
*/
void VDocBaseLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft, const GReal /*inOpacity*/) //opacity should be applied in derived class only
{
	if (fBackgroundColor.GetAlpha())
	{
		//background includes element content, padding & borders but not margin

		inGC->SetFillColor( fBackgroundColor);

		VRect bounds;
		if (fBorderLayout)
			//we use border layout clip rectangle in order  background painting to be aligned according to crisp edges status
			//(which is dependent on both drawing rect & line width)
			fBorderLayout->GetClipRect( inGC, bounds);
		else
		{
			//if none borders, we can paint directly using clip bounds without margins
			bounds = fBackgroundBounds;
		}
		bounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());

		inGC->FillRect( bounds);
	}   

	if (fBorderLayout && !fDesignBounds.IsEmpty())
		fBorderLayout->Draw( inGC, inTopLeft);
}


