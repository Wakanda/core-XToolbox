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

/** create border 
@remarks
	if inLeft is NULL, left border is same as right border 
	if inBottom is NULL, bottom border is same as top border
	if inRight is NULL, right border is same as top border
*/
VDocBorderLayout::VDocBorderLayout( const Border *inTop, const Border *inRight, const Border *inBottom, const Border *inLeft):VObject()
{
	if (inTop)
		fBorders.fTop = *inTop;
	else
	{
		fBorders.fTop.fStyle = (eDocPropBorderStyle)VDocNode::GetDefaultPropValue( kDOC_PROP_BORDER_STYLE).GetRect().top;
		fBorders.fTop.fWidth = ICSSUtil::TWIPSToPoint( VDocNode::GetDefaultPropValue( kDOC_PROP_BORDER_WIDTH).GetRect().top);
		RGBAColor color = (RGBAColor)VDocNode::GetDefaultPropValue( kDOC_PROP_BORDER_COLOR).GetRect().top;
		fBorders.fTop.fColor.FromRGBAColor( color);
		fBorders.fTop.fRadius = ICSSUtil::TWIPSToPoint( VDocNode::GetDefaultPropValue( kDOC_PROP_BORDER_RADIUS).GetRect().top);

		inTop = &(fBorders.fTop);
	}
	
	if (!inRight)
		inRight = inTop;
	if (!inBottom)
		inBottom = inTop;
	if (!inLeft)
		inLeft = inRight;

	fBorders.fRight = *inRight;
	fBorders.fBottom = *inBottom;
	fBorders.fLeft = *inLeft;

	fClipping.fTop = fClipping.fRight = fClipping.fBottom = fClipping.fLeft = NULL;

	fDPI = 72;

	_CheckRadius();
	_CheckClip();
}

/** create from the passed document node 
@remarks
	here only border properties are imported
*/
VDocBorderLayout::VDocBorderLayout(const VDocNode *inNode):VObject()
{
	sDocPropRect border = inNode->GetBorderStyle();
	fBorders.fTop.fStyle = (eDocPropBorderStyle)border.top;
	fBorders.fRight.fStyle = (eDocPropBorderStyle)border.right;
	fBorders.fBottom.fStyle = (eDocPropBorderStyle)border.bottom;
	fBorders.fLeft.fStyle = (eDocPropBorderStyle)border.left;

	border = inNode->GetBorderWidth();
	fBorders.fTop.fWidth = ICSSUtil::TWIPSToPoint(border.top);
	fBorders.fRight.fWidth = ICSSUtil::TWIPSToPoint(border.right);
	fBorders.fBottom.fWidth = ICSSUtil::TWIPSToPoint(border.bottom);
	fBorders.fLeft.fWidth = ICSSUtil::TWIPSToPoint(border.left);

	border = inNode->GetBorderColor();
	RGBAColor color = (RGBAColor)border.top;
	fBorders.fTop.fColor.FromRGBAColor( color);
	color = (RGBAColor)border.right;
	fBorders.fRight.fColor.FromRGBAColor( color);
	color = (RGBAColor)border.bottom;
	fBorders.fBottom.fColor.FromRGBAColor( color);
	color = (RGBAColor)border.left;
	fBorders.fLeft.fColor.FromRGBAColor( color);

	border = inNode->GetBorderRadius();
	fBorders.fTop.fRadius = ICSSUtil::TWIPSToPoint(border.top);
	fBorders.fRight.fRadius = ICSSUtil::TWIPSToPoint(border.right);
	fBorders.fBottom.fRadius = ICSSUtil::TWIPSToPoint(border.bottom);
	fBorders.fLeft.fRadius = ICSSUtil::TWIPSToPoint(border.left);
	
	fClipping.fTop = fClipping.fRight = fClipping.fBottom = fClipping.fLeft = NULL;

	fDPI = 72;

	_CheckRadius();
	_CheckClip();
}

/** set node properties from the current properties */
void VDocBorderLayout::SetPropsTo( VDocNode *outNode)
{
	if ((fBorders.fLeft.fStyle > kTEXT_BORDER_STYLE_HIDDEN && fBorders.fLeft.fWidth > 0) 
		||
		(fBorders.fTop.fStyle > kTEXT_BORDER_STYLE_HIDDEN && fBorders.fTop.fWidth > 0) 
		||
		(fBorders.fRight.fStyle > kTEXT_BORDER_STYLE_HIDDEN && fBorders.fRight.fWidth > 0)  
		||
		(fBorders.fBottom.fStyle > kTEXT_BORDER_STYLE_HIDDEN && fBorders.fBottom.fWidth > 0) )
	{
		sDocPropRect style, width, color, radius;

		style.left		= fBorders.fLeft.fStyle;
		style.top		= fBorders.fTop.fStyle;
		style.right		= fBorders.fRight.fStyle;
		style.bottom	= fBorders.fBottom.fStyle;
	
		width.left		= ICSSUtil::PointToTWIPS(fBorders.fLeft.fWidth);
		width.top		= ICSSUtil::PointToTWIPS(fBorders.fTop.fWidth);
		width.right		= ICSSUtil::PointToTWIPS(fBorders.fRight.fWidth);
		width.bottom	= ICSSUtil::PointToTWIPS(fBorders.fBottom.fWidth);
	
		color.left		= (sLONG)fBorders.fLeft.fColor.GetRGBAColor();
		color.top		= (sLONG)fBorders.fTop.fColor.GetRGBAColor();
		color.right		= (sLONG)fBorders.fRight.fColor.GetRGBAColor();
		color.bottom	= (sLONG)fBorders.fBottom.fColor.GetRGBAColor();

		radius.left		= ICSSUtil::PointToTWIPS(fBorders.fLeft.fRadius);
		radius.top		= radius.left;
		radius.right	= radius.left;
		radius.bottom	= radius.left;

		outNode->SetBorderStyle( style);
		outNode->SetBorderWidth( width);
		outNode->SetBorderColor( color);
		if (radius.left)
			outNode->SetBorderRadius( radius);
	}
}

VDocBorderLayout::~VDocBorderLayout()
{
	if (fClipping.fTop)
		delete fClipping.fTop;
	if (fClipping.fRight)
		delete fClipping.fRight;
	if (fClipping.fBottom)
		delete fClipping.fBottom;
	if (fClipping.fLeft)
		delete fClipping.fLeft;
}

bool VDocBorderLayout::Border::operator == (const Border& inBorder)
{
	return (fStyle == inBorder.fStyle && fWidth == inBorder.fWidth && fColor == inBorder.fColor); //we ignore radius as it is assumed all radius are the same
}

void VDocBorderLayout::_CheckRadius()
{
	if (!(fBorders.fLeft == fBorders.fRight && fBorders.fTop == fBorders.fBottom && fBorders.fLeft == fBorders.fTop))
		fBorders.fLeft.fRadius = fBorders.fRight.fRadius = fBorders.fTop.fRadius = fBorders.fBottom.fRadius = 0; 
}


void VDocBorderLayout::_CheckClip()
{
	fNeedClip = false;

	//auto-collapse borders 
	if (fBorders.fLeft.fStyle <= kTEXT_BORDER_STYLE_HIDDEN)
		fBorders.fLeft.fWidth = 0;
	if (fBorders.fRight.fStyle <= kTEXT_BORDER_STYLE_HIDDEN)
		fBorders.fRight.fWidth = 0;
	if (fBorders.fTop.fStyle <= kTEXT_BORDER_STYLE_HIDDEN)
		fBorders.fTop.fWidth = 0;
	if (fBorders.fBottom.fStyle <= kTEXT_BORDER_STYLE_HIDDEN)
		fBorders.fBottom.fWidth = 0;

	//determine if we need to clip
	if (fBorders.fLeft.ShouldClip()
		||
		fBorders.fRight.ShouldClip()
		||
		fBorders.fBottom.ShouldClip()
		||
		fBorders.fTop.ShouldClip()
		||
		fBorders.fLeft != fBorders.fTop
		||
		fBorders.fTop != fBorders.fRight
		||
		fBorders.fBottom != fBorders.fRight
		||
		fBorders.fBottom != fBorders.fLeft
		)
	{
		//reset radius if we clip 
		fBorders.fLeft.fRadius = fBorders.fRight.fRadius = fBorders.fTop.fRadius = fBorders.fBottom.fRadius = 0;
		fNeedClip = true;
	}
}

void VDocBorderLayout::_BuildPath(VGraphicPath *&path, const VectorOfPoint& inPoints)
{
	if (path)
		path->Clear();
	else
		path = new VGraphicPath();
	path->Begin();
	path->BeginSubPathAt( inPoints[0]);
	for (int i = 1; i < inPoints.size(); i++)
		path->AddLineTo( inPoints[i]);
	path->CloseSubPath();
	path->End();
}

/** set border drawing rect (in pixel)
@param inRect
	border drawing rect
@param inDPI
	desired dpi (width & radius are scaled to inDPI/72 as they are in point)
@remarks
	rect is aligned to border center (so inflate -0.5*border width for exterior bounds & +0.5*border width for interior bounds)
	path & clipping paths if needed are precomputed
*/
void VDocBorderLayout::SetDrawRect( const VRect& inRect, const GReal inDPI)
{
	if (fDrawRect == inRect && fDPI == inDPI)
		return;

	fDrawRect = inRect;
	fDPI = inDPI;
	GReal scale = fDPI/72;

	if (fNeedClip)
	{
		//prepare clipping paths:
		//clipping paths are used to properly clip each border along corner lines - which are oriented from corner points to the draw rect center -
		//so border painting do not overlap at corners
		//(useful only if borders do not share the same style)

		GReal widthLeft = ceil(fBorders.fLeft.fWidth*scale);
		GReal widthRight = ceil(fBorders.fRight.fWidth*scale);
		GReal widthTop = ceil(fBorders.fTop.fWidth*scale);
		GReal widthBottom = ceil(fBorders.fBottom.fWidth*scale);
		fMaxWidth = widthLeft;
		if (widthRight > fMaxWidth)
			fMaxWidth = widthRight;
		if (widthTop > fMaxWidth)
			fMaxWidth = widthTop;
		if (widthBottom > fMaxWidth)
			fMaxWidth = widthBottom;

		VectorOfPoint points;
		if (widthTop)
		{
			points.push_back( VPoint(fDrawRect.GetX()-widthLeft, fDrawRect.GetY()-widthTop));
			points.push_back( VPoint(fDrawRect.GetRight()+widthRight, fDrawRect.GetY()-widthTop));
			points.push_back( VPoint(fDrawRect.GetRight()-widthRight, fDrawRect.GetY()+widthTop));
			points.push_back( VPoint(fDrawRect.GetX()+widthLeft, fDrawRect.GetY()+widthTop));
			_BuildPath( fClipping.fTop, points);
		}
		else if (fClipping.fTop)
		{
			delete fClipping.fTop;
			fClipping.fTop = NULL;
		}

		if (widthBottom)
		{
			points.clear();
			points.push_back( VPoint(fDrawRect.GetX()+widthLeft, fDrawRect.GetBottom()-widthBottom));
			points.push_back( VPoint(fDrawRect.GetRight()-widthRight, fDrawRect.GetBottom()-widthBottom));
			points.push_back( VPoint(fDrawRect.GetRight()+widthRight, fDrawRect.GetBottom()+widthBottom));
			points.push_back( VPoint(fDrawRect.GetX()-widthLeft, fDrawRect.GetBottom()+widthBottom));
			_BuildPath( fClipping.fBottom, points);
		}
		else if (fClipping.fBottom)
		{
			delete fClipping.fBottom;
			fClipping.fBottom = NULL;
		}

		if (widthLeft)
		{
			points.clear();
			points.push_back( VPoint(fDrawRect.GetX()-widthLeft, fDrawRect.GetY()-widthTop));
			points.push_back( VPoint(fDrawRect.GetX()+widthLeft, fDrawRect.GetY()+widthTop));
			points.push_back( VPoint(fDrawRect.GetX()+widthLeft, fDrawRect.GetBottom()-widthBottom));
			points.push_back( VPoint(fDrawRect.GetX()-widthLeft, fDrawRect.GetBottom()+widthBottom));
			_BuildPath( fClipping.fLeft, points);
		}
		else if (fClipping.fLeft)
		{
			delete fClipping.fLeft;
			fClipping.fLeft = NULL;
		}

		if (widthRight)
		{
			points.clear();
			points.push_back( VPoint(fDrawRect.GetRight()-widthRight, fDrawRect.GetY()+widthTop));
			points.push_back( VPoint(fDrawRect.GetRight()+widthRight, fDrawRect.GetY()-widthTop));
			points.push_back( VPoint(fDrawRect.GetRight()+widthRight, fDrawRect.GetBottom()+widthBottom));
			points.push_back( VPoint(fDrawRect.GetRight()-widthRight, fDrawRect.GetBottom()-widthBottom));
			_BuildPath( fClipping.fRight, points);
		}
		else if (fClipping.fRight)
		{
			delete fClipping.fRight;
			fClipping.fRight = NULL;
		}
	}
}


void VDocBorderLayout::_GetAltColor( const VColor& inColor, VColor& outColor)
{
	//darken normal color to get alternate color for groove, ridge, inset & outset styles 
	//(FIXME: check if it is actually the same as rendered by browsers)

	outColor.SetRed( inColor.GetRed()*0.5);
	outColor.SetGreen( inColor.GetGreen()*0.5);
	outColor.SetBlue( inColor.GetBlue()*0.5);
}


/** get clip rectangle if border is drawed in the passed gc at the passed top left position 
@remarks
	clip rectangle takes account crisp edges status
*/
void VDocBorderLayout::GetClipRect(VGraphicContext *inGC, VRect& outClipRect, const VPoint& inTopLeft) const
{
	GReal widthLeft = VTextLayout::RoundMetrics(NULL,fBorders.fLeft.fWidth*fDPI/72); 
	GReal widthRight = VTextLayout::RoundMetrics(NULL,fBorders.fRight.fWidth*fDPI/72); 
	GReal widthTop = VTextLayout::RoundMetrics(NULL,fBorders.fTop.fWidth*fDPI/72); 
	GReal widthBottom = VTextLayout::RoundMetrics(NULL,fBorders.fBottom.fWidth*fDPI/72); 

	outClipRect = fDrawRect;
	outClipRect.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());

	if (inGC->IsShapeCrispEdgesEnabled())
	{
		//calc clip rect according to crisp edges & border width 
		GReal left,top,right,bottom;
		VPoint pos;

		inGC->SetLineWidth(widthLeft);
		pos.SetPosTo( fDrawRect.GetX(), fDrawRect.GetY());
		inGC->CEAdjustPointInTransformedSpace( pos);
		left = pos.GetX();

		inGC->SetLineWidth(widthRight);
		pos.SetPosTo( fDrawRect.GetRight(), fDrawRect.GetY());
		inGC->CEAdjustPointInTransformedSpace( pos);
		right = pos.GetX();

		inGC->SetLineWidth(widthTop);
		pos.SetPosTo( fDrawRect.GetX(), fDrawRect.GetY());
		inGC->CEAdjustPointInTransformedSpace( pos);
		top = pos.GetY();

		inGC->SetLineWidth(widthBottom);
		pos.SetPosTo( fDrawRect.GetX(), fDrawRect.GetBottom());
		inGC->CEAdjustPointInTransformedSpace( pos);
		bottom = pos.GetY();

		inGC->SetLineWidth(1);

		outClipRect.SetCoordsTo(left, top, right-left, bottom-top);

		outClipRect.SetPosBy(-widthLeft*0.5,-widthTop*0.5);
		outClipRect.SetSizeBy( (widthLeft+widthRight)*0.5, (widthTop+widthBottom)*0.5);
	}
	else
	{
		outClipRect.SetPosBy(-widthLeft*0.5,-widthTop*0.5);
		outClipRect.SetSizeBy( (widthLeft+widthRight)*0.5, (widthTop+widthBottom)*0.5);
	}
}


void VDocBorderLayout::_SetLinePatternFromBorderStyle( VGraphicContext *inGC, eDocPropBorderStyle inStyle)
{
	//remark: we use dash pattern & not impl hard-coded dash & dotted in order pattern to be the same on any platform
	switch (inStyle)
	{
	case kTEXT_BORDER_STYLE_DOTTED:
		{
			VLineDashPattern pattern;
			pattern.push_back(1);
			pattern.push_back(1);
			inGC->SetLineDashPattern( 0, pattern);
		}
		break;
	case kTEXT_BORDER_STYLE_DASHED:
		{
			VLineDashPattern pattern;
			pattern.push_back(3);
			pattern.push_back(3);
			inGC->SetLineDashPattern( 0, pattern);
		}
		break;
	default:
		inGC->SetLineStdPattern( PAT_LINE_PLAIN);
		break;
	}
}


/** render border 
@param inGC
	graphic context

@remarks
	method does not save & restore gc context
*/
void VDocBorderLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft)
{
	xbox_assert(inGC);
	if (fDrawRect.IsEmpty())
		return;

	if (!fNeedClip)
	{
		if (!fBorders.fLeft.fColor.GetAlpha() || fBorders.fLeft.fStyle <= kTEXT_BORDER_STYLE_HIDDEN)
			return;
	}

	VRect drawRect = fDrawRect;
	drawRect.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());

#if VERSIONWIN
	//GDI impl does not support clipping paths or width-scaled dash or dotted patterns so derive to Gdiplus if context is GDI
	//(should occur only while printing)
	VGraphicContext *gcBackup = NULL;
	if (inGC->IsGDIImpl())
	{
		gcBackup = inGC;
		inGC = inGC->BeginUsingGdiplus();
	}
#endif

	inGC->SetLineJoin( JS_DEFAULT);

	VColor curColor;
	VColor curFillColor = VColor(0,0,0,0);
	GReal curStrokeWidth = 1.0f;
	inGC->SetLineCap( CS_DEFAULT); 

	//HTML compliant: dash pattern unit = stroke width
	bool shouldLineDashPatternUnitEqualToLineWidth = inGC->ShouldLineDashPatternUnitEqualToLineWidth();
	inGC->ShouldLineDashPatternUnitEqualToLineWidth( true);

	if (!fNeedClip)
	{
		//all borders have the same style 

		GReal radius = fBorders.fLeft.fRadius*fDPI/72;
		GReal width = VTextLayout::RoundMetrics(NULL,fBorders.fLeft.fWidth*fDPI/72); 

		_SetLinePatternFromBorderStyle( inGC, fBorders.fLeft.fStyle);

		inGC->SetLineColor( curColor = fBorders.fLeft.fColor);

		if (fBorders.fLeft.fStyle == kTEXT_BORDER_STYLE_DOUBLE)
		{
			GReal strokeWidthDouble = width/3;

			VRect rectOut, rectIn;
			rectOut = drawRect;
			rectOut.Inset(-width*0.5+strokeWidthDouble*0.5, -width*0.5+strokeWidthDouble*0.5);

			rectIn = drawRect;
			rectIn.Inset( width*0.5-strokeWidthDouble*0.5, width*0.5-strokeWidthDouble*0.5);

			inGC->SetLineWidth( curStrokeWidth = strokeWidthDouble);
			
			if (radius != 0)
				inGC->FrameRoundRect( rectOut, radius*2, radius*2);
			else
				inGC->FrameRect( rectOut);

			if (radius != 0)
				inGC->FrameRoundRect( rectIn, radius*2, radius*2);
			else
				inGC->FrameRect( rectIn);
		}
		else
		{
			inGC->SetLineWidth( curStrokeWidth = width);
			if (radius)
				inGC->FrameRoundRect( drawRect, radius*2, radius*2);
			else
				inGC->FrameRect( drawRect);
		}
	}
	else
	{
		//borders do not have the same style or have bicolor styles

        inGC->SaveClip();
		VRect clipRect;
		GetClipRect( inGC, clipRect, inTopLeft);
        inGC->ClipRect(clipRect);

		//draw left border

		if (fBorders.fLeft.fColor.GetAlpha() && fBorders.fLeft.fStyle > kTEXT_BORDER_STYLE_HIDDEN)
		{
			inGC->SaveClip();
			inGC->ClipPath( *fClipping.fLeft);

			GReal width = VTextLayout::RoundMetrics(NULL,fBorders.fLeft.fWidth*fDPI/72);

			_SetLinePatternFromBorderStyle( inGC, fBorders.fLeft.fStyle);

			if (fBorders.fLeft.fStyle == kTEXT_BORDER_STYLE_DOUBLE)
			{
				GReal strokeWidthDouble = width/3;
				
				GReal dxOut = -width*0.5+strokeWidthDouble*0.5;
				GReal dxIn = width*0.5-strokeWidthDouble*0.5;
				
				inGC->SetLineColor( curColor = fBorders.fLeft.fColor);

				inGC->SetLineWidth( curStrokeWidth = strokeWidthDouble);
				
				inGC->DrawLine( drawRect.GetTopLeft()+VPoint(dxOut,-fMaxWidth), drawRect.GetBotLeft()+VPoint(dxOut,fMaxWidth));

				inGC->DrawLine( drawRect.GetTopLeft()+VPoint(dxIn,-fMaxWidth), drawRect.GetBotLeft()+VPoint(dxIn,fMaxWidth));
			}
			else
			{
				switch (fBorders.fLeft.fStyle)
				{
				case kTEXT_BORDER_STYLE_GROOVE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fLeft.fColor, colorAlt);
					inGC->SetFillColor( colorAlt);
					inGC->FillRect( VRect( drawRect.GetX()-width*0.5, drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					inGC->SetFillColor( curFillColor = fBorders.fLeft.fColor);
					inGC->FillRect( VRect( drawRect.GetX(), drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					}
					break;
				case kTEXT_BORDER_STYLE_RIDGE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fLeft.fColor, colorAlt);
					inGC->SetFillColor( fBorders.fLeft.fColor);
					inGC->FillRect( VRect( drawRect.GetX()-width*0.5, drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					inGC->SetFillColor( curFillColor = colorAlt);
					inGC->FillRect( VRect( drawRect.GetX(), drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					}
					break;
				case kTEXT_BORDER_STYLE_INSET:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fLeft.fColor, colorAlt);
					inGC->SetLineColor( curColor = colorAlt);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetTopLeft()+VPoint(0,-fMaxWidth), drawRect.GetBotLeft()+VPoint(0,fMaxWidth));
					}
					break;
				case kTEXT_BORDER_STYLE_OUTSET:
				default:
					{
					inGC->SetLineColor( curColor = fBorders.fLeft.fColor);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetTopLeft()+VPoint(0,-fMaxWidth), drawRect.GetBotLeft()+VPoint(0,fMaxWidth));
					}
					break;
				}
			}
			inGC->RestoreClip();
		}

		//draw right border

		if (fBorders.fRight.fColor.GetAlpha() && fBorders.fRight.fStyle > kTEXT_BORDER_STYLE_HIDDEN)
		{
			inGC->SaveClip();
			inGC->ClipPath( *fClipping.fRight);

			GReal width = VTextLayout::RoundMetrics(NULL,fBorders.fRight.fWidth*fDPI/72);

			_SetLinePatternFromBorderStyle( inGC, fBorders.fRight.fStyle);

			if (fBorders.fRight.fStyle == kTEXT_BORDER_STYLE_DOUBLE)
			{
				GReal strokeWidthDouble = width/3;
				
				GReal dxOut = -width*0.5+strokeWidthDouble*0.5;
				GReal dxIn = width*0.5-strokeWidthDouble*0.5;
				
				inGC->SetLineColor( curColor = fBorders.fRight.fColor);

				inGC->SetLineWidth( curStrokeWidth = strokeWidthDouble);
				
				inGC->DrawLine( drawRect.GetTopRight()+VPoint(dxOut,-fMaxWidth), drawRect.GetBotRight()+VPoint(dxOut,fMaxWidth));

				inGC->DrawLine( drawRect.GetTopRight()+VPoint(dxIn,-fMaxWidth), drawRect.GetBotRight()+VPoint(dxIn,fMaxWidth));
			}
			else
			{
				switch (fBorders.fRight.fStyle)
				{
				case kTEXT_BORDER_STYLE_GROOVE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fRight.fColor, colorAlt);
					inGC->SetFillColor( colorAlt);
					inGC->FillRect( VRect( drawRect.GetRight()-width*0.5, drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					inGC->SetFillColor( curFillColor = fBorders.fRight.fColor);
					inGC->FillRect( VRect( drawRect.GetRight(), drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					}
					break;
				case kTEXT_BORDER_STYLE_RIDGE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fRight.fColor, colorAlt);
					inGC->SetFillColor( fBorders.fRight.fColor);
					inGC->FillRect( VRect( drawRect.GetRight()-width*0.5, drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					inGC->SetFillColor( curFillColor = colorAlt);
					inGC->FillRect( VRect( drawRect.GetRight(), drawRect.GetY()-fMaxWidth, width*0.5, drawRect.GetHeight()+fMaxWidth*2));
					}
					break;
				case kTEXT_BORDER_STYLE_OUTSET:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fRight.fColor, colorAlt);
					inGC->SetLineColor( curColor = colorAlt);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetTopRight()+VPoint(0,-fMaxWidth), drawRect.GetBotRight()+VPoint(0,fMaxWidth));
					}
					break;
				case kTEXT_BORDER_STYLE_INSET:
				default:
					{
					inGC->SetLineColor( curColor = fBorders.fRight.fColor);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetTopRight()+VPoint(0,-fMaxWidth), drawRect.GetBotRight()+VPoint(0,fMaxWidth));
					}
					break;
				}
			}
			inGC->RestoreClip();
		}

		//draw top border

		if (fBorders.fTop.fColor.GetAlpha() && fBorders.fTop.fStyle > kTEXT_BORDER_STYLE_HIDDEN)
		{
			inGC->SaveClip();
			inGC->ClipPath( *fClipping.fTop);

			GReal width = VTextLayout::RoundMetrics(NULL,fBorders.fTop.fWidth*fDPI/72);

			_SetLinePatternFromBorderStyle( inGC, fBorders.fTop.fStyle);

			if (fBorders.fTop.fStyle == kTEXT_BORDER_STYLE_DOUBLE)
			{
				GReal strokeWidthDouble = width/3;

				GReal dyOut = -width*0.5+strokeWidthDouble*0.5;
				GReal dyIn = width*0.5-strokeWidthDouble*0.5;
				
				inGC->SetLineColor( curColor = fBorders.fTop.fColor);

				inGC->SetLineWidth( curStrokeWidth = strokeWidthDouble);
				
				inGC->DrawLine( drawRect.GetTopLeft()+VPoint(-fMaxWidth,dyOut), drawRect.GetTopRight()+VPoint(fMaxWidth,dyOut));

				inGC->DrawLine( drawRect.GetTopLeft()+VPoint(-fMaxWidth,dyIn), drawRect.GetTopRight()+VPoint(fMaxWidth,dyIn));
			}
			else
			{
				switch (fBorders.fTop.fStyle)
				{
				case kTEXT_BORDER_STYLE_GROOVE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fTop.fColor, colorAlt);
					inGC->SetFillColor( colorAlt);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetY()-width*0.5, drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					inGC->SetFillColor( curFillColor = fBorders.fTop.fColor);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetY(), drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					}
					break;
				case kTEXT_BORDER_STYLE_RIDGE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fTop.fColor, colorAlt);
					inGC->SetFillColor( fBorders.fTop.fColor);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetY()-width*0.5, drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					inGC->SetFillColor( curFillColor = colorAlt);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetY(), drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					}
					break;
				case kTEXT_BORDER_STYLE_INSET:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fTop.fColor, colorAlt);
					inGC->SetLineColor( curColor = colorAlt);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetTopLeft()+VPoint(-fMaxWidth,0), drawRect.GetTopRight()+VPoint(fMaxWidth,0));
					}
					break;
				case kTEXT_BORDER_STYLE_OUTSET:
				default:
					{
					inGC->SetLineColor( curColor = fBorders.fTop.fColor);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetTopLeft()+VPoint(-fMaxWidth,0), drawRect.GetTopRight()+VPoint(fMaxWidth,0));
					}
					break;
				}
			}
			inGC->RestoreClip();
		}

		//draw bottom border

		if (fBorders.fBottom.fColor.GetAlpha() && fBorders.fBottom.fStyle > kTEXT_BORDER_STYLE_HIDDEN)
		{
			inGC->SaveClip();
			inGC->ClipPath( *fClipping.fBottom);

			GReal width = VTextLayout::RoundMetrics(NULL,fBorders.fBottom.fWidth*fDPI/72);

			_SetLinePatternFromBorderStyle( inGC, fBorders.fBottom.fStyle);

			if (fBorders.fBottom.fStyle == kTEXT_BORDER_STYLE_DOUBLE)
			{
				GReal strokeWidthDouble = width/3;
				
				GReal dyOut = -width*0.5+strokeWidthDouble*0.5;
				GReal dyIn = width*0.5-strokeWidthDouble*0.5;
				
				inGC->SetLineColor( curColor = fBorders.fBottom.fColor);

				inGC->SetLineWidth( curStrokeWidth = strokeWidthDouble);
				
				inGC->DrawLine( drawRect.GetBotLeft()+VPoint(-fMaxWidth,dyOut), drawRect.GetBotRight()+VPoint(fMaxWidth,dyOut));

				inGC->DrawLine( drawRect.GetBotLeft()+VPoint(-fMaxWidth,dyIn), drawRect.GetBotRight()+VPoint(fMaxWidth,dyIn));
			}
			else
			{
				switch (fBorders.fBottom.fStyle)
				{
				case kTEXT_BORDER_STYLE_GROOVE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fBottom.fColor, colorAlt);
					inGC->SetFillColor( colorAlt);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetBottom()-width*0.5, drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					inGC->SetFillColor( curFillColor = fBorders.fBottom.fColor);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetBottom(), drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					}
					break;
				case kTEXT_BORDER_STYLE_RIDGE:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fBottom.fColor, colorAlt);
					inGC->SetFillColor( fBorders.fBottom.fColor);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetBottom()-width*0.5, drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					inGC->SetFillColor( curFillColor = colorAlt);
					inGC->FillRect( VRect( drawRect.GetLeft()-fMaxWidth, drawRect.GetBottom(), drawRect.GetWidth()+fMaxWidth*2, width*0.5));
					}
					break;
				case kTEXT_BORDER_STYLE_OUTSET:
					{
					VColor colorAlt;
					_GetAltColor( fBorders.fBottom.fColor, colorAlt);
					inGC->SetLineColor( curColor = colorAlt);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetBotLeft()+VPoint(-fMaxWidth,0), drawRect.GetBotRight()+VPoint(fMaxWidth,0));
					}
					break;
				case kTEXT_BORDER_STYLE_INSET:
				default:
					{
					inGC->SetLineColor( curColor = fBorders.fBottom.fColor);
					inGC->SetLineWidth( curStrokeWidth = width);
					inGC->DrawLine( drawRect.GetBotLeft()+VPoint(-fMaxWidth,0), drawRect.GetBotRight()+VPoint(fMaxWidth,0));
					}
					break;
				}
			}
			inGC->RestoreClip();
		}

        inGC->RestoreClip();
	}

	//restore line styles
	inGC->SetLineStdPattern( PAT_LINE_PLAIN);
	inGC->ShouldLineDashPatternUnitEqualToLineWidth( shouldLineDashPatternUnitEqualToLineWidth);
	if (curStrokeWidth != 1.0f)
		inGC->SetLineWidth( 1.0f);

#if VERSIONWIN
	if (gcBackup)
	{
		inGC = gcBackup;
		inGC->EndUsingGdiplus();
	}
#endif
}
