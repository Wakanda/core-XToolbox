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
#include "VDocTextLayout.h"

#undef min
#undef max


VDocBaseLayout::VDocBaseLayout():VObject(), IRefCountable()
{
	fMarginInTWIPS.left = fMarginInTWIPS.right = fMarginInTWIPS.top = fMarginInTWIPS.bottom = 0;
	fMarginOutTWIPS.left = fMarginOutTWIPS.right = fMarginOutTWIPS.top = fMarginOutTWIPS.bottom = 0;
	fMarginOut.left = fMarginOut.right = fMarginOut.top = fMarginOut.bottom = 0;
	fMarginIn.left = fMarginIn.right = fMarginIn.top = fMarginIn.bottom = 0;
	fAllMargin.left = fAllMargin.right = fAllMargin.top = fAllMargin.bottom = 0;
	fBorderWidth.left = fBorderWidth.right = fBorderWidth.top = fBorderWidth.bottom = 0;
	fBorderLayout = NULL;

	fWidthTWIPS = fHeightTWIPS = fMinWidthTWIPS = fMinHeightTWIPS = 0; //automatic on default

	fTextAlign = fVerticalAlign = AL_DEFAULT; //default

	fBackgroundColor = VColor(0,0,0,0); //transparent

	fBackgroundClip		= kDOC_BACKGROUND_BORDER_BOX;
	fBackgroundOrigin	= kDOC_BACKGROUND_PADDING_BOX;
	fBackgroundImageLayout	= NULL;

	fDPI = fRenderDPI = 72;

	fTextStart = 0;
	fTextLength = 1;
	fParent = NULL;
	fChildIndex = 0;

	fNoUpdateTextInfo = false;
}

VDocBaseLayout::~VDocBaseLayout()
{
	if (fBackgroundImageLayout)
		delete fBackgroundImageLayout;
	if (fBorderLayout)
		delete fBorderLayout;
}


GReal VDocBaseLayout::RoundMetrics( VGraphicContext *inGC, GReal inValue)
{
	if (inGC && inGC->IsFontPixelSnappingEnabled())
	{
		//context aligns glyphs to pixel boundaries:
		//stick to nearest integral (as GDI does)
		//(note that as VDocParagraphLayout pixel is dependent on fixed fDPI, metrics need to be computed if fDPI is changed)
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

void VDocBaseLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw, const VPoint& inPreDecal)
{
	//we build transform to the target device coordinate space
	//if painting, we apply the transform to the passed gc & return in outCTM the previous transform which is restored in _EndLocalTransform
	//if not painting (get metrics), we return in outCTM the transform from computing dpi coord space to the ctm space but we do not modify the passed gc transform

	if (inDraw)
	{
		inGC->UseReversedAxis();
		inGC->GetTransform( outCTM);
	}

	VAffineTransform xform;
	VPoint origin( inTopLeft);

	//apply decal in local coordinate space
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

	if (fRenderDPI != fDPI)
	{
		scalex *= fRenderDPI/fDPI;
		scaley *= fRenderDPI/fDPI;
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
		VRect bounds = _GetBackgroundBounds( inGC);
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

/** return true if it is the last child node
@param inLoopUp
	true: return true if and only if it is the last child node at this level and up to the top-level parent
	false: return true if it is the last child node relatively to the node parent
*/
bool VDocBaseLayout::IsLastChild(bool inLookUp) const
{
	if (!fParent)
		return true;
	if (fChildIndex != fParent->fChildren.size()-1)
		return false;
	if (!inLookUp)
		return true;
	return fParent->IsLastChild( true);
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

	//if (fTextAlign != AL_DEFAULT) 
		outNode->SetTextAlign( JustFromAlignStyle(fTextAlign)); //as property is inheritable, always set it
	if (fVerticalAlign != AL_DEFAULT)
		outNode->SetVerticalAlign( JustFromAlignStyle(fVerticalAlign));

	if (fBackgroundColor.GetRGBAColor() != RGBACOLOR_TRANSPARENT)
		outNode->SetBackgroundColor( fBackgroundColor.GetRGBAColor());

	if (!(VDocProperty((uLONG) fBackgroundClip) == outNode->GetDefaultPropValue( kDOC_PROP_BACKGROUND_CLIP)))
		outNode->SetBackgroundClip( fBackgroundClip);

	if (fBorderLayout)
		fBorderLayout->SetPropsTo( outNode);

	_SetBackgroundImagePropsTo( outNode);
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
		SetTextAlign( JustToAlignStyle(just));
	}

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_VERTICAL_ALIGN) && !inNode->IsInherited( kDOC_PROP_VERTICAL_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inNode->GetVerticalAlign();
		SetVerticalAlign( JustToAlignStyle(just));
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
		(inNode->IsOverriden( kDOC_PROP_BACKGROUND_CLIP) && !inNode->IsInherited( kDOC_PROP_BACKGROUND_CLIP)))
		SetBackgroundClip( inNode->GetBackgroundClip());

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_BACKGROUND_IMAGE) && !inNode->IsInherited( kDOC_PROP_BACKGROUND_IMAGE)))
		_CreateBackgroundImage( inNode);
	
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

/** create background image from passed document node */
void VDocBaseLayout::_CreateBackgroundImage(const VDocNode *inNode)
{
	if (fBackgroundImageLayout)
		delete fBackgroundImageLayout;
	fBackgroundImageLayout = NULL;

	VString url = inNode->GetBackgroundImage();
	if (!url.IsEmpty())
	{
		SetBackgroundOrigin( inNode->GetBackgroundImageOrigin());

		eStyleJust halign = inNode->GetBackgroundImageHAlign();
		eStyleJust valign = inNode->GetBackgroundImageVAlign();
				
		eDocPropBackgroundRepeat repeat = inNode->GetBackgroundImageRepeat();
	
		sLONG width = inNode->GetBackgroundImageWidth();
		GReal widthPoint;
		bool isWidthPercentage = false;
		if (width == kDOC_BACKGROUND_SIZE_COVER || width == kDOC_BACKGROUND_SIZE_CONTAIN || width == kDOC_BACKGROUND_SIZE_AUTO)
			widthPoint = width;
		else if (width < 0) //percentage
		{
			isWidthPercentage = true;
			widthPoint = (-width)/100.0f; //remap 100% -> 1
		}
		else
			widthPoint = ICSSUtil::TWIPSToPoint(width);

		GReal heightPoint;
		bool isHeightPercentage = false;
		if (width == kDOC_BACKGROUND_SIZE_COVER || width == kDOC_BACKGROUND_SIZE_CONTAIN)
			heightPoint = kDOC_BACKGROUND_SIZE_AUTO;
		else
		{
			sLONG height = inNode->GetBackgroundImageHeight();
			if (height == kDOC_BACKGROUND_SIZE_COVER || height == kDOC_BACKGROUND_SIZE_CONTAIN || height == kDOC_BACKGROUND_SIZE_AUTO)
				heightPoint = kDOC_BACKGROUND_SIZE_AUTO;
			else if (height < 0) //percentage
			{
				isHeightPercentage = true;
				heightPoint = (-height)/100.0f; //remap 100% -> 1
			}
			else
				heightPoint = ICSSUtil::TWIPSToPoint(height);
		}

		fBackgroundImageLayout = new VDocBackgroundImageLayout( url, halign, valign, repeat, widthPoint, heightPoint, isWidthPercentage, isHeightPercentage);
	}
}

/** set document node background image properties */
void VDocBaseLayout::_SetBackgroundImagePropsTo( VDocNode* outDocNode)
{
	if (fBackgroundImageLayout && !fBackgroundImageLayout->GetURL().IsEmpty())
	{
		outDocNode->SetBackgroundImage( fBackgroundImageLayout->GetURL());

		if (!(VDocProperty((uLONG) GetBackgroundOrigin()) == outDocNode->GetDefaultPropValue( kDOC_PROP_BACKGROUND_ORIGIN)))
			outDocNode->SetBackgroundImageOrigin( GetBackgroundOrigin());

		if (!(VDocProperty((uLONG)fBackgroundImageLayout->GetRepeat()) == outDocNode->GetDefaultPropValue( kDOC_PROP_BACKGROUND_REPEAT)))
			outDocNode->SetBackgroundImageRepeat( fBackgroundImageLayout->GetRepeat());

		if (fBackgroundImageLayout->GetHAlign() != JST_Left)
			outDocNode->SetBackgroundImageHAlign( fBackgroundImageLayout->GetHAlign());
		if (fBackgroundImageLayout->GetVAlign() != JST_Top)
			outDocNode->SetBackgroundImageVAlign( fBackgroundImageLayout->GetVAlign());
		
		if (fBackgroundImageLayout->GetWidth() != kDOC_BACKGROUND_SIZE_AUTO || fBackgroundImageLayout->GetHeight() != kDOC_BACKGROUND_SIZE_AUTO)
		{
			GReal widthPoint = fBackgroundImageLayout->GetWidth();
			sLONG width;
			if (widthPoint <= 0)
				width = widthPoint;
			else if (fBackgroundImageLayout->IsWidthPercentage())
			{
				width = floor((widthPoint*100)+0.5);
				if (width < 1)
					width = 1;
				else if (width > 1000)
					width = 1000;
				width = -width;
			}
			else
				width = ICSSUtil::PointToTWIPS(widthPoint);

			outDocNode->SetBackgroundImageWidth( width);

			GReal heightPoint = fBackgroundImageLayout->GetHeight();
			sLONG height;
			if (heightPoint <= 0)
				height = kDOC_BACKGROUND_SIZE_AUTO;
			else if (fBackgroundImageLayout->IsHeightPercentage())
			{
				height = floor((heightPoint*100)+0.5);
				if (height < 1)
					height = 1;
				else if (height > 1000)
					height = 1000;
				height = -height;
			}
			else
				height = ICSSUtil::PointToTWIPS(heightPoint);
			
			outDocNode->SetBackgroundImageHeight( height);
		}
	}
}

/** set background image 
@remarks
	see VDocBackgroundImageLayout constructor for parameters description */
void VDocBaseLayout::SetBackgroundImage(	const VString& inURL,	
											const eStyleJust inHAlign, const eStyleJust inVAlign, 
											const eDocPropBackgroundRepeat inRepeat,
											const GReal inWidth, const GReal inHeight,
											const bool inWidthIsPercentage,  const bool inHeightIsPercentage)
{
	if (inURL.IsEmpty() && !fBackgroundImageLayout)
		return;

	if (fBackgroundImageLayout)
		delete fBackgroundImageLayout;
	fBackgroundImageLayout = NULL;

	if (!inURL.IsEmpty())
		fBackgroundImageLayout = new VDocBackgroundImageLayout( inURL, inHAlign, inVAlign, inRepeat, inWidth, inHeight, inWidthIsPercentage, inHeightIsPercentage);
}

/** clear background image */
void VDocBaseLayout::ClearBackgroundImage()
{
	if (!fBackgroundImageLayout)
		return;

	delete fBackgroundImageLayout;
	fBackgroundImageLayout = NULL;
}


/** set layout DPI */
void VDocBaseLayout::SetDPI( const GReal inDPI)
{
	fRenderDPI = inDPI;

	if (fDPI == inDPI)
		return;
	fDPI = inDPI;

	sDocPropRect margin = fMarginOutTWIPS;
	fMarginOutTWIPS.left = fMarginOutTWIPS.right = fMarginOutTWIPS.top = fMarginOutTWIPS.bottom = 0;
	SetMargins( margin, false); //force recalc margin in px according to dpi

	margin = fMarginInTWIPS;
	fMarginInTWIPS.left = fMarginInTWIPS.right = fMarginInTWIPS.top = fMarginInTWIPS.bottom = 0;
	SetPadding( margin, false); //force recalc padding in px according to dpi

	_UpdateBorderWidth(false);
	_CalcAllMargin();
}

/** set outside margins	(in twips) - CSS margin */
void VDocBaseLayout::SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin)
{
	fMarginOutTWIPS = inMargins;
    
	fMarginOut.left		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.left) * fDPI/72);
	fMarginOut.right	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.right) * fDPI/72);
	fMarginOut.top		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.top) * fDPI/72);
	fMarginOut.bottom	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.bottom) * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

/** set inside margins	(in twips) - CSS padding */
void VDocBaseLayout::SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin)
{
	fMarginInTWIPS = inPadding;
    
	fMarginIn.left		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.left) * fDPI/72);
	fMarginIn.right		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.right) * fDPI/72);
	fMarginIn.top		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.top) * fDPI/72);
	fMarginIn.bottom	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.bottom) * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

uLONG VDocBaseLayout::GetWidth(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins || !fWidthTWIPS)
		return	fWidthTWIPS;
	else
	{
		return	fWidthTWIPS+_GetAllMarginHorizontalTWIPS();
	}
}

uLONG VDocBaseLayout::GetHeight(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins || !fHeightTWIPS)
		return	fHeightTWIPS;
	else
	{
		return	fHeightTWIPS+_GetAllMarginVerticalTWIPS();
	}
}

uLONG VDocBaseLayout::GetMinWidth(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins)
		return	fMinWidthTWIPS;
	else
	{
		return	fMinWidthTWIPS+_GetAllMarginHorizontalTWIPS();
	}
}

uLONG VDocBaseLayout::GetMinHeight(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins)
		return	fMinHeightTWIPS;
	else
	{
		return	fMinHeightTWIPS+_GetAllMarginVerticalTWIPS();
	}
}

/** set background clipping - CSS background-clip */
void VDocBaseLayout::SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip) 
{ 
	fBackgroundClip = inBackgroundClip; 
	if (fBackgroundImageLayout)
		fBackgroundImageLayout->SetClipBounds( GetBackgroundBounds( fBackgroundClip));
}

/** set background origin - CSS background-origin */
void VDocBaseLayout::SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin)
{ 
	fBackgroundOrigin = inBackgroundOrigin; 
	if (fBackgroundImageLayout)
		fBackgroundImageLayout->SetOriginBounds( GetBackgroundBounds( fBackgroundOrigin));
}


/** get background bounds (in px - relative to fDPI) */
const VRect& VDocBaseLayout::GetBackgroundBounds(eDocPropBackgroundBox inBackgroundBox) const
{
	switch (inBackgroundBox)
	{
	case kDOC_BACKGROUND_BORDER_BOX:
		return fBorderBounds;
		break;
	case kDOC_BACKGROUND_PADDING_BOX:
		return fPaddingBounds;
		break;
	default:
		//kDOC_BACKGROUND_CONTENT_BOX
		return fContentBounds;
		break;
	}
	return fBorderBounds;
}

/** set borders 
@remarks
	if inLeft is NULL, left border is same as right border 
	if inBottom is NULL, bottom border is same as top border
	if inRight is NULL, right border is same as top border
	if inTop is NULL, top border is default border (solid, medium width & black color)
*/
void VDocBaseLayout::SetBorders( const sDocBorder *inTop, const sDocBorder *inRight, const sDocBorder *inBottom, const sDocBorder *inLeft)
{
	if (fBorderLayout)
		delete fBorderLayout;
	fBorderLayout = new VDocBorderLayout( inTop, inRight, inBottom, inLeft);
	_UpdateBorderWidth();
}

/** clear borders */
void VDocBaseLayout::ClearBorders()
{
	if (fBorderLayout)
		delete fBorderLayout;
	fBorderLayout = NULL;
	_UpdateBorderWidth();
}
					
const sDocBorderRect* VDocBaseLayout::GetBorders() const
{
	if (fBorderLayout)
		return &(fBorderLayout->GetBorders());
	else
		return NULL;
}

/** get background bounds (in px - relative to dpi set by SetDPI) 
@remarks
	unlike GetBackgroundBounds, if gc crispEdges is enabled & has a border, it will return pixel grid aligned bounds 
	(useful for proper clipping according to crispEdges status)
*/
const VRect& VDocBaseLayout::_GetBackgroundBounds(VGraphicContext *inGC, eDocPropBackgroundBox inBackgroundBox) const
{
	switch (inBackgroundBox)
	{
	case kDOC_BACKGROUND_BORDER_BOX:
		if (fBorderLayout && inGC->IsShapeCrispEdgesEnabled())
		{
			//we use border layout clip rectangle in order background painting or clipping to be aligned according to crisp edges status
			//(which is dependent on both drawing rect & line width)
			fBorderLayout->GetClipRect( inGC, fTempBorderBounds, VPoint(), true);
			return fTempBorderBounds;
		}
		else
			//if none borders or no crispEdges, we can paint or clip directly using border bounds
			return fBorderBounds;
		break;
	case kDOC_BACKGROUND_PADDING_BOX:
		if (fBorderLayout && inGC->IsShapeCrispEdgesEnabled())
		{
			//we use border layout clip rectangle in order background painting or clipping to be aligned according to crisp edges status
			//(which is dependent on both drawing rect & line width)
			fBorderLayout->GetClipRect( inGC, fTempPaddingBounds, VPoint(), false);
			return fTempPaddingBounds;
		}
		else
			//if none borders or no crispEdges, we can paint or clip directly using padding bounds
			return fPaddingBounds;
		break;
	default:
		//kDOC_BACKGROUND_CONTENT_BOX
		return fContentBounds;
		break;
	}
	return fBorderBounds;
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
	fBorderWidth.left	= RoundMetrics(NULL,fBorderLayout->GetLeftBorder().fWidth * fDPI/72);
	fBorderWidth.right	= RoundMetrics(NULL,fBorderLayout->GetRightBorder().fWidth * fDPI/72);
	fBorderWidth.top	= RoundMetrics(NULL,fBorderLayout->GetTopBorder().fWidth * fDPI/72);
	fBorderWidth.bottom	= RoundMetrics(NULL,fBorderLayout->GetBottomBorder().fWidth * fDPI/72);

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

void VDocBaseLayout::_SetLayoutBounds( VGraphicContext *inGC, const VRect& inBounds)
{
	fLayoutBounds = inBounds;
	fBorderBounds = inBounds;
	
	GReal width = fLayoutBounds.GetWidth()-fMarginOut.left-fMarginOut.right;
	if (width < 0) //might happen with GDI due to pixel snapping
		width = 0;
	GReal height = fLayoutBounds.GetHeight()-fMarginOut.top-fMarginOut.bottom;
	if (height < 0) //might happen with GDI due to pixel snapping
		height = 0;
	fBorderBounds.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left, fLayoutBounds.GetY()+fMarginOut.top, width, height);

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
		drawRect.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left+fBorderWidth.left*0.5f, fLayoutBounds.GetY()+fMarginOut.top+fBorderWidth.top*0.5, width, height);

		fBorderLayout->SetDrawRect( drawRect, fDPI);

		width = fLayoutBounds.GetWidth()-fMarginOut.left-fMarginOut.right-fBorderWidth.left-fBorderWidth.right;
		if (width < 0) //might happen with GDI due to pixel snapping
			width = 0;
		height = fLayoutBounds.GetHeight()-fMarginOut.top-fMarginOut.bottom-fBorderWidth.top-fBorderWidth.bottom;
		if (height < 0) //might happen with GDI due to pixel snapping
			height = 0;
		fPaddingBounds.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left+fBorderWidth.left, fLayoutBounds.GetY()+fMarginOut.top+fBorderWidth.top, width, height);
	}
	else
		fPaddingBounds = fBorderBounds;

	width = width-fMarginIn.left-fMarginIn.right;
	if (width < 0) //might happen with GDI due to pixel snapping
		width = 0;
	height = height-fMarginIn.top-fMarginIn.bottom;
	if (height < 0) //might happen with GDI due to pixel snapping
		height = 0;
	fContentBounds.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left+fBorderWidth.left+fMarginIn.left, fLayoutBounds.GetY()+fMarginOut.top+fBorderWidth.top+fMarginIn.top, width, height);

	if (fBackgroundImageLayout)
	{
		//set background image clip & origin bounds
	
		fBackgroundImageLayout->SetDPI( fDPI);
		fBackgroundImageLayout->SetClipBounds( _GetBackgroundBounds( inGC, fBackgroundClip));
		if (fBackgroundClip != fBackgroundOrigin)
			fBackgroundImageLayout->SetOriginBounds( _GetBackgroundBounds( inGC, fBackgroundOrigin));
		else
			fBackgroundImageLayout->SetOriginBounds( fBackgroundImageLayout->GetClipBounds());
		fBackgroundImageLayout->UpdateLayout( inGC);
	}
}

void VDocBaseLayout::_FinalizeTextLocalMetrics(	VGraphicContext *inGC, GReal typoWidth, GReal typoHeight, GReal layoutWidth, GReal layoutHeight, 
												const sRectMargin& inMargin, GReal inRenderMaxWidth, GReal inRenderMaxHeight, 
												VRect& outTypoBounds, VRect& outRenderTypoBounds, VRect &outRenderLayoutBounds, sRectMargin& outRenderMargin)
{
	GReal minWidth = fMinWidthTWIPS ? RoundMetrics(inGC,ICSSUtil::TWIPSToPoint(fMinWidthTWIPS)*fDPI/72) : 0;
	if (minWidth && layoutWidth < minWidth)
		layoutWidth = minWidth;
	typoWidth += inMargin.left+inMargin.right;
	layoutWidth += inMargin.left+inMargin.right;

	GReal minHeight = fMinHeightTWIPS ? RoundMetrics(inGC,ICSSUtil::TWIPSToPoint(fMinHeightTWIPS)*fDPI/72) : 0;
	if (minHeight && layoutHeight < minHeight)
		layoutHeight = minHeight;
	typoHeight += inMargin.top+inMargin.bottom;
	layoutHeight += inMargin.top+inMargin.bottom;

	//store final computing dpi metrics

	outTypoBounds.SetCoords( 0, 0, typoWidth, typoHeight);
	fLayoutBounds.SetCoords( 0, 0, layoutWidth, layoutHeight);

	//now update target dpi metrics

	if (fRenderDPI != fDPI)
	{
		typoWidth = typoWidth*fRenderDPI/fDPI;
		typoHeight = typoHeight*fRenderDPI/fDPI;

		layoutWidth = layoutWidth*fRenderDPI/fDPI;
		layoutHeight = layoutHeight*fRenderDPI/fDPI;

		outRenderMargin.left		= RoundMetrics(inGC,fAllMargin.left*fRenderDPI/fDPI); 
		outRenderMargin.right		= RoundMetrics(inGC,fAllMargin.right*fRenderDPI/fDPI); 
		outRenderMargin.top			= RoundMetrics(inGC,fAllMargin.top*fRenderDPI/fDPI); 
		outRenderMargin.bottom		= RoundMetrics(inGC,fAllMargin.bottom*fRenderDPI/fDPI); 
	}
	else
		outRenderMargin				= inMargin;

	outRenderTypoBounds.SetCoords( 0, 0, ceil(typoWidth), ceil(typoHeight));
	outRenderLayoutBounds.SetCoords( 0, 0, ceil(layoutWidth), ceil(layoutHeight));

	if (inRenderMaxWidth != 0.0f && outRenderTypoBounds.GetWidth() > inRenderMaxWidth)
		outRenderTypoBounds.SetWidth( std::floor(inRenderMaxWidth)); 
	if (inRenderMaxHeight != 0.0f && outRenderTypoBounds.GetHeight() > inRenderMaxHeight)
		outRenderTypoBounds.SetHeight( std::floor(inRenderMaxHeight)); 
	
	if (inRenderMaxWidth != 0.0f)
		outRenderLayoutBounds.SetWidth( std::floor(inRenderMaxWidth)); 
	
	if (inRenderMaxHeight != 0.0f)
		outRenderLayoutBounds.SetHeight( std::floor(inRenderMaxHeight)); 
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
		if (borderStyle.left > kDOC_BORDER_STYLE_HIDDEN
			||
			borderStyle.right > kDOC_BORDER_STYLE_HIDDEN
			||
			borderStyle.top > kDOC_BORDER_STYLE_HIDDEN
			||
			borderStyle.bottom > kDOC_BORDER_STYLE_HIDDEN)
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
		//background is clipped according to background clip box

		inGC->SetFillColor( fBackgroundColor);

		VRect bounds = _GetBackgroundBounds( inGC, fBackgroundClip);
		bounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());

		inGC->FillRect( bounds);
	}   

	//draw background image
	if (fBackgroundImageLayout)
		fBackgroundImageLayout->Draw( inGC, inTopLeft);

	//draw borders
	if (fBorderLayout && !fLayoutBounds.IsEmpty())
		fBorderLayout->Draw( inGC, inTopLeft);
}


/** make transform from source viewport to destination viewport
	(with uniform or non uniform scaling) */
void VDocBaseLayout::ComputeViewportTransform( VAffineTransform& outTransform, const VRect& _inRectSrc, const VRect& _inRectDst, uLONG inKeepAspectRatio) 
{
	VRect inRectSrc = _inRectSrc;
	VRect inRectDst = _inRectDst;

	//first translate coordinate space in order to set origin to (0,0)
	outTransform.MakeIdentity();
	outTransform.Translate( -inRectSrc.GetX(),
					  -inRectSrc.GetY(),
					  VAffineTransform::MatrixOrderAppend);
	
	//treat uniform scaling if appropriate
	if (inKeepAspectRatio != kKAR_NONE)
	{
		//compute new rectangle size without losing aspect ratio

		if (inKeepAspectRatio & kKAR_COVER)
		{
			//first case: cover

			if (inRectSrc.GetWidth() > inRectSrc.GetHeight())
			{
				//scale to meet viewport height
				Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
				inRectSrc.SetHeight( inRectDst.GetHeight());
				inRectSrc.SetWidth( scale*inRectSrc.GetHeight());

				if (inRectSrc.GetWidth() < inRectDst.GetWidth())
				{
					//scale to meet viewport width
					Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
					inRectSrc.SetWidth( inRectDst.GetWidth());
					inRectSrc.SetHeight( scale*inRectSrc.GetWidth());
				}
			}	
			else
			{
				//scale to meet viewport width
				Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
				inRectSrc.SetWidth( inRectDst.GetWidth());
				inRectSrc.SetHeight( scale*inRectSrc.GetWidth());

				if (inRectSrc.GetHeight() < inRectDst.GetHeight())
				{
					//scale to meet viewport height
					Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
					inRectSrc.SetHeight( inRectDst.GetHeight());
					inRectSrc.SetWidth( scale*inRectSrc.GetHeight());
				}
			}
		}
		else
			//second case: contain (default)
			if (inRectDst.GetWidth() > inRectDst.GetHeight())
			{
				if (inRectSrc.GetWidth() > inRectSrc.GetHeight())
				{
					Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
					inRectSrc.SetWidth( inRectDst.GetWidth());
					inRectSrc.SetHeight( scale*inRectSrc.GetWidth());

					if (inRectSrc.GetHeight() > inRectDst.GetHeight())
					{
						Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
						inRectSrc.SetHeight( inRectDst.GetHeight());
						inRectSrc.SetWidth( scale*inRectSrc.GetHeight());
					}
				}	
				else
				{
					Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
					inRectSrc.SetHeight( inRectDst.GetHeight());
					inRectSrc.SetWidth( scale*inRectSrc.GetHeight());
				}
			}	
			else
			{
				if (inRectSrc.GetWidth() > inRectSrc.GetHeight())
				{
					Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
					inRectSrc.SetWidth( inRectDst.GetWidth());
					inRectSrc.SetHeight( scale*inRectSrc.GetWidth());
				}	
				else
				{
					Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
					inRectSrc.SetHeight( inRectDst.GetHeight());
					inRectSrc.SetWidth( scale*inRectSrc.GetHeight());

					if (inRectSrc.GetWidth() > inRectDst.GetWidth())
					{
						Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
						inRectSrc.SetWidth( inRectDst.GetWidth());
						inRectSrc.SetHeight( scale*inRectSrc.GetWidth());
					}
				}
			}
		
		//scale coordinate space from initial source one to the modified one
		Real scaleX = inRectSrc.GetWidth()/_inRectSrc.GetWidth();
		Real scaleY = inRectSrc.GetHeight()/_inRectSrc.GetHeight();

		outTransform.Scale( scaleX, scaleY,
					  VAffineTransform::MatrixOrderAppend);
	
		//compute rectangle destination offset (alias the keep ratio decal)
		Real x = inRectDst.GetX();
		Real y = inRectDst.GetY();
		switch (inKeepAspectRatio & kKAR_XYDECAL_MASK)
		{
		case kKAR_XMINYMIN:
			break;
		case kKAR_XMINYMID:
			y += (inRectDst.GetHeight()-inRectSrc.GetHeight())/2;
			break;
		case kKAR_XMINYMAX:
			y += inRectDst.GetHeight()-inRectSrc.GetHeight();
			break;

		case kKAR_XMIDYMIN:	
			x += (inRectDst.GetWidth()-inRectSrc.GetWidth())/2;
			break;
		case kKAR_XMIDYMID:
			x += (inRectDst.GetWidth()-inRectSrc.GetWidth())/2;
			y += (inRectDst.GetHeight()-inRectSrc.GetHeight())/2;
		break;
		case kKAR_XMIDYMAX:
			x += (inRectDst.GetWidth()-inRectSrc.GetWidth())/2;
			y += inRectDst.GetHeight()-inRectSrc.GetHeight();
			break;
			
		case kKAR_XMAXYMIN:	
			x += inRectDst.GetWidth()-inRectSrc.GetWidth();
			break;
		case kKAR_XMAXYMID:	
			x += inRectDst.GetWidth()-inRectSrc.GetWidth();
			y += (inRectDst.GetHeight()-inRectSrc.GetHeight())/2;
			break;
		case kKAR_XMAXYMAX:
			x += inRectDst.GetWidth()-inRectSrc.GetWidth();
			y += inRectDst.GetHeight()-inRectSrc.GetHeight();
			break;
		}
		inRectDst.SetX( x);
		inRectDst.SetY( y);
	}	else
	{
		//non uniform scaling

		Real scaleX = inRectDst.GetWidth()/inRectSrc.GetWidth();
		Real scaleY = inRectDst.GetHeight()/inRectSrc.GetHeight();
		
		outTransform.Scale( scaleX, scaleY,
					  VAffineTransform::MatrixOrderAppend);
		
	}
	
	//finally translate in destination coordinate space
	outTransform.Translate( inRectDst.GetX(),
					  inRectDst.GetY(),
					  VAffineTransform::MatrixOrderAppend);
	
}


AlignStyle VDocBaseLayout::JustToAlignStyle( const eStyleJust inJust) 
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
	case JST_Baseline:
		return AL_BASELINE;
		break;
	case JST_Superscript:
		return AL_SUPER;
		break;
	case JST_Subscript:
		return AL_SUB;
		break;
	default:
		return AL_DEFAULT;
		break;
	}
}

eStyleJust VDocBaseLayout::JustFromAlignStyle( const AlignStyle inJust) 
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
	case AL_BASELINE:
		return JST_Baseline;
		break;
	case AL_SUPER:
		return JST_Superscript;
		break;
	case AL_SUB:
		return JST_Subscript;
		break;
	default:
		return JST_Default;
		break;
	}
}

void VDocBaseLayout::_UpdateTextInfo()
{
	if (fParent)
	{
		xbox_assert(fParent->fChildren.size() > 0 && fParent->fChildren[fChildIndex].Get() == this);

		if (fChildIndex > 0)
		{
			VDocBaseLayout *prevNode = fParent->fChildren[fChildIndex-1].Get();
			fTextStart = prevNode->GetTextStart()+prevNode->GetTextLength();
		}
		else
			fTextStart = 0;
	}
	else
		fTextStart = 0;

	if (fChildren.size())
	{
		fTextLength = 0;
		VectorOfChildren::const_iterator itNode = fChildren.begin();
		for(;itNode != fChildren.end(); itNode++)
			fTextLength += itNode->Get()->GetTextLength();
	}
	else
		fTextLength = 0; //_UpdateTextInfo should be overriden by classes for which text length should not be 0
}

void VDocBaseLayout::_UpdateTextInfoEx(bool inUpdateNextSiblings)
{
	if (fNoUpdateTextInfo)
		return;
	if (fParent && fParent->fNoUpdateTextInfo)
		return;

	_UpdateTextInfo();
	if (fParent)
	{
		//update siblings after this node
		if (inUpdateNextSiblings)
		{
			VectorOfChildren::iterator itNode = fParent->fChildren.begin();
			std::advance( itNode, fChildIndex+1);
			for (;itNode != fParent->fChildren.end(); itNode++)
				(*itNode).Get()->_UpdateTextInfo();
		}

		fParent->_UpdateTextInfoEx( true);
	}
}

GReal VDocBaseLayout::_GetAllMarginHorizontalTWIPS() const
{
	return	fMarginOutTWIPS.left+fMarginOutTWIPS.right+fMarginInTWIPS.left+fMarginInTWIPS.right+
			(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetLeftBorder().fWidth+fBorderLayout->GetRightBorder().fWidth) : 0);
}

GReal VDocBaseLayout::_GetAllMarginVerticalTWIPS() const
{
	return	fMarginOutTWIPS.top+fMarginOutTWIPS.bottom+fMarginInTWIPS.top+fMarginInTWIPS.bottom+
			(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetTopBorder().fWidth+fBorderLayout->GetBottomBorder().fWidth) : 0);
}

sLONG VDocBaseLayout::_GetWidthTWIPS() const
{
	sLONG widthTWIPS = fWidthTWIPS;
	sLONG margin = 0;
	if (!widthTWIPS && fParent)
	{
		//if parent has a content width not NULL, determine width from parent content width minus local margin+padding+border
		//TODO: ignore if current layout is a a table cell layout

		VDocBaseLayout *parent = fParent;
		widthTWIPS = parent->fWidthTWIPS;
		margin = _GetAllMarginHorizontalTWIPS();
		while (parent && !widthTWIPS)
		{
			if (parent->fParent)
				margin += parent->_GetAllMarginHorizontalTWIPS();
			parent = parent->fParent;
			if (parent)
				widthTWIPS = parent->fWidthTWIPS;
		}
	}
	if (widthTWIPS)
	{
		widthTWIPS -= margin;
		if (widthTWIPS < fMinWidthTWIPS)
			widthTWIPS = fMinWidthTWIPS;
		if (widthTWIPS <= 0)
			widthTWIPS = 1;
	}
	return widthTWIPS;
}

sLONG VDocBaseLayout::_GetHeightTWIPS() const
{
	sLONG heightTWIPS = fHeightTWIPS;
	sLONG margin = 0;

	/* remark: automatic height should be always determined by formatted content and not by parent content height
	if (!heightTWIPS && fParent)
	{
		//if parent has a content height not NULL, determine height from parent content height minus local margin+padding+border

		VDocBaseLayout *parent = fParent;
		heightTWIPS = parent->fHeightTWIPS;
		margin = _GetAllMarginVerticalTWIPS();
		while (parent && !heightTWIPS)
		{
			if (parent->fParent)
				margin += parent->_GetAllMarginVerticalTWIPS();
			parent = parent->fParent;
			if (parent)
				heightTWIPS = parent->fHeightTWIPS;
		}
	}
	*/
	if (heightTWIPS)
	{
		heightTWIPS -=	margin;
		if (heightTWIPS < fMinHeightTWIPS)
			heightTWIPS = fMinHeightTWIPS;
		if (heightTWIPS <= 0)
			heightTWIPS = 1;
	}

	return heightTWIPS;
}


/** add child layout instance */
void VDocBaseLayout::AddChild( IDocBaseLayout* inLayout)
{
	xbox_assert(inLayout && (inLayout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH || inLayout->GetDocType() == kDOC_NODE_TYPE_DIV));

	VDocBaseLayout *layout = dynamic_cast<VDocBaseLayout*>(inLayout);
	if (testAssert(layout))
	{
		fChildren.push_back(VRefPtr<VDocBaseLayout>(layout));
		layout->fChildIndex = fChildren.size()-1;
		layout->fParent = this;
		layout->_UpdateTextInfoEx(false);
	}
}

/** insert child layout instance */
void VDocBaseLayout::InsertChild( IDocBaseLayout* inLayout, sLONG inIndex)
{
	xbox_assert(inLayout && (inLayout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH || inLayout->GetDocType() == kDOC_NODE_TYPE_DIV));

	if (!(testAssert(inIndex >= 0 && inIndex <= fChildren.size())))
		return;
	if (inIndex >= fChildren.size())
	{
		AddChild( inLayout);
		return;
	}

	VDocBaseLayout *layout = dynamic_cast<VDocBaseLayout*>(inLayout);
	if (!testAssert(layout))
		return;

	VectorOfChildren::iterator itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	fChildren.insert(itChild, VRefPtr<VDocBaseLayout>(layout)); 
	layout->fParent = this;

	itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	for (; itChild != fChildren.end(); itChild++)
		itChild->Get()->fChildIndex = inIndex++;
	
	layout->_UpdateTextInfoEx(true);
}

/** remove child layout instance */
void VDocBaseLayout::RemoveChild( sLONG inIndex)
{
	if (!(testAssert(inIndex >= 0 && inIndex < fChildren.size())))
		return;
	VectorOfChildren::iterator itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	fChildren.erase(itChild);

	itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	for (; itChild != fChildren.end(); itChild++)
		itChild->Get()->fChildIndex = inIndex++;
	
	itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	if (itChild != fChildren.end())
		itChild->Get()->_UpdateTextInfoEx(true);
	else
		_UpdateTextInfoEx(true);
}


/** retain first child layout instance which intersects the passed text index */
IDocBaseLayout*	VDocBaseLayout::GetFirstChild( sLONG inStart) const
{
	if (fChildren.size() == 0)
		return NULL;

	VDocBaseLayout *node = NULL;
	if (inStart <= 0)
		node = fChildren[0].Get();
	else if (inStart >= fTextLength)
		node = fChildren.back().Get();
	else //search dichotomically
	{
		sLONG start = 0;
		sLONG end = fChildren.size()-1;
		while (start < end)
		{
			sLONG mid = (start+end)/2;
			if (start < mid)
			{
				if (inStart < fChildren[mid].Get()->GetTextStart())
					end = mid-1;
				else
					start = mid;
			}
			else
			{ 
				xbox_assert(start == mid && end == start+1);
				if (inStart < fChildren[end].Get()->GetTextStart())
					node = fChildren[start].Get();
				else
					node = fChildren[end].Get();
				break;
			}
		}
		if (!node)
			node = fChildren[start].Get();
	}
	return static_cast<IDocBaseLayout*>(node);
}

/** query text layout interface (if available for this class instance) */
/*
ITextLayout* VDocBaseLayout::QueryTextLayoutInterface()
{
	VDocParagraphLayout *paraLayout = dynamic_cast<VDocParagraphLayout *>(this);
	if (paraLayout)
		return static_cast<ITextLayout *>(paraLayout);
#if ENABLE_VDOCTEXTLAYOUT
	else
	{
		VDocContainerLayout *contLayout = dynamic_cast<VDocContainerLayout *>(this);
		if (contLayout)
			return static_cast<ITextLayout *>(contLayout);
	}
#endif
	return NULL;
}
*/

void VDocBaseLayout::_AdjustComputingDPI(VGraphicContext *inGC)
{
	if (!fParent)
	{
		//we adjust computing dpi only for top-level layout class (children class instances inherit computing dpi from top-level computing dpi)

		if (inGC->IsFontPixelSnappingEnabled())
		{
			//for GDI, the best for now is still to compute GDI metrics at the target DPI:
			//need to find another way to not break wordwrap lines while changing DPI with GDI
			//(no problem with DWrite or CoreText)
			
			//if printing, compute metrics at printer dpi
			if (inGC->HasPrinterScale())
			{
				GReal scaleX, scaleY;
				inGC->GetPrinterScale( scaleX, scaleY);
				GReal dpi = fRenderDPI*scaleX;
				
				GReal targetDPI = fRenderDPI;
				if (fDPI != dpi)
					SetDPI( dpi);
				fRenderDPI = targetDPI;
			}
			else
				//otherwise compute at desired DPI
				if (fDPI != fRenderDPI)
					SetDPI( fRenderDPI);
		}
		else
		{
			//metrics are not snapped to pixel -> compute metrics at 72 DPI (with TWIPS precision) & scale context to target DPI (both DWrite & CoreText are scalable graphic contexts)
			if (fDPI != 72)
			{
				GReal targetDPI = fRenderDPI;
				SetDPI( 72);
				fRenderDPI = targetDPI;
			}
		}
	}
}

VDocBaseLayout* VDocBaseLayout::_GetTopLevelParentLayout() const
{
	VDocBaseLayout *parent = fParent;
	if (!parent)
		return NULL;

	while (parent->fParent)
		parent = parent->fParent;
	return parent;
}


void VDocBaseLayout::_NormalizeRange(sLONG& ioStart, sLONG& ioEnd) const
{
	if (ioStart < 0)
		ioStart = 0;
	else if (ioStart > GetTextLength())
		ioStart = GetTextLength();

	if (ioEnd < 0)
		ioEnd = GetTextLength();
	else if (ioEnd > GetTextLength())
		ioEnd = GetTextLength();
	
	xbox_assert(ioStart <= ioEnd);
}


VDocBaseLayout*	VDocLayoutChildrenIterator::First(sLONG inTextStart, sLONG inTextEnd) 
{
	fEnd = inTextEnd;
	if (inTextStart > 0)
	{
		IDocBaseLayout *layout = fParent->GetFirstChild( inTextStart);
		if (layout)
		{
			fIterator = fParent->fChildren.begin();
			if (layout->GetChildIndex() > 0)
				std::advance(fIterator, layout->GetChildIndex());
		}
		else 
			fIterator = fParent->fChildren.end(); 
	}
	else
		fIterator = fParent->fChildren.begin(); 
	return Current(); 
}


VDocBaseLayout*	VDocLayoutChildrenIterator::Next() 
{  
	fIterator++; 
	if (fEnd >= 0 && fIterator != fParent->fChildren.end() && fEnd <= (*fIterator).Get()->GetTextStart()) //if child range start >= fEnd
	{
		if (fEnd < (*fIterator).Get()->GetTextStart() || (*fIterator).Get()->GetTextLength() != 0) //if child range is 0 and child range end == fEnd, we keep it otherwise we discard it and stop iteration
		{
			fIterator = fParent->fChildren.end();
			return NULL;
		}
	}
	return Current(); 
}


VDocBaseLayout*	VDocLayoutChildrenReverseIterator::First(sLONG inTextStart, sLONG inTextEnd) 
{
	fStart = inTextStart;
	if (inTextEnd < 0)
		inTextEnd = fParent->GetTextLength();

	if (inTextEnd < fParent->GetTextLength())
	{
		IDocBaseLayout *layout = fParent->GetFirstChild( inTextEnd);
		if (layout)
		{
			if (inTextEnd <= layout->GetTextStart() && layout->GetTextLength() > 0)
				layout = layout->GetChildIndex() > 0 ? fParent->fChildren[layout->GetChildIndex()-1].Get() : NULL;
			if (layout)
			{
				fIterator = fParent->fChildren.rbegin(); //point on last child
				if (layout->GetChildIndex() < fParent->fChildren.size()-1)
					std::advance(fIterator, fParent->fChildren.size()-1-layout->GetChildIndex());
			}
			else
				fIterator = fParent->fChildren.rend(); 
		}
		else 
			fIterator = fParent->fChildren.rend(); 
	}
	else
		fIterator = fParent->fChildren.rbegin(); 
	return Current(); 
}


VDocBaseLayout*	VDocLayoutChildrenReverseIterator::Next() 
{  
	fIterator++; 
	if (fStart > 0 && fIterator != fParent->fChildren.rend() && fStart >= (*fIterator).Get()->GetTextStart()+(*fIterator).Get()->GetTextLength()) 
	{
		fIterator = fParent->fChildren.rend();
		return NULL;
	}
	return Current(); 
}
