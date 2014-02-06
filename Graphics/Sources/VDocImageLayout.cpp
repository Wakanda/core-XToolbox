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

#undef min
#undef max


VDocImageLayout::VDocImageLayout(const VString& inURL):VDocBaseLayout()
{
	_Init();
	fURL = inURL;
}

/** create from the passed image node */
VDocImageLayout::VDocImageLayout(const VDocImage *inNode):VDocBaseLayout()
{
	_Init();
	InitPropsFrom( static_cast<const VDocNode *>(inNode));	
}

VDocImageLayout::~VDocImageLayout()
{
	ReleaseRefCountable(&fPictureData);
}

void VDocImageLayout::_Init()
{
	fAscent = 0;
	fImgWidthTWIPS = 0;
	fImgHeightTWIPS = 0;
	fPictureData = NULL;
	fPictureDataStamp = 0;
	fImgLayerSettings.fDPI = 0;
}

/** set layout DPI */
void VDocImageLayout::SetDPI( const GReal inDPI)
{
	VDocBaseLayout::SetDPI( inDPI);
}

/** get image width in px (relative to layout dpi) */
GReal VDocImageLayout::GetImageWidth() const
{
	return RoundMetrics(NULL, ICSSUtil::TWIPSToPoint( fImgWidthTWIPS)*fDPI/72);
}

/** get image height in px (relative to layout dpi) */
GReal VDocImageLayout::GetImageHeight() const
{
	return RoundMetrics(NULL, ICSSUtil::TWIPSToPoint( fImgHeightTWIPS)*fDPI/72);
}

/** initialize from the passed node */
void VDocImageLayout::InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited)
{
	VDocBaseLayout::InitPropsFrom( inNode, inIgnoreIfInherited);

	const VDocImage *docImage = dynamic_cast<const VDocImage *>(inNode);
	if (!testAssert(docImage))
		return;

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_IMG_SOURCE) && !inNode->IsInherited( kDOC_PROP_IMG_SOURCE)))
		fURL = docImage->GetSource();

	if (!inIgnoreIfInherited 
		|| 
		(inNode->IsOverriden( kDOC_PROP_IMG_ALT_TEXT) && !inNode->IsInherited( kDOC_PROP_IMG_ALT_TEXT)))
		fText = docImage->GetText();
}

/** set node properties from the current properties */
void VDocImageLayout::SetPropsTo( VDocNode *outNode)
{
	VDocBaseLayout::SetPropsTo( outNode);

	VDocImage *docImage = dynamic_cast<VDocImage *>(outNode);
	if (!testAssert(docImage))
		return;

	docImage->SetSource( fURL);
	docImage->SetText( fText);
}

/** update layout metrics */ 
void VDocImageLayout::UpdateLayout(VGraphicContext *inGC)
{
	//update picture data
	const VPictureData *pictureData = VDocImageLayoutPictureDataManager::Get()->Retain( fURL, &fPictureDataStamp);
	CopyRefCountable(&fPictureData, pictureData);
	ReleaseRefCountable(&pictureData);
	if (!fPictureData)
		fImgLayer.Set( (VImageOffScreen *)NULL);

	fAscent = 0; //will be computed later with UpdateAscent() if image is embedded in a paragraph

	if (fPictureData)
	{
		//for compatibility with 4D form (not dpi-aware), we assume on default 72 dpi for the images
		fImgWidthTWIPS = fPictureData->GetWidth()*20;
		fImgHeightTWIPS = fPictureData->GetHeight()*20;
	}
	else
		fImgWidthTWIPS = fImgHeightTWIPS = 32*20;

	GReal width, height;
	
	width = fWidthTWIPS ? fWidthTWIPS : fImgWidthTWIPS;
	height = fHeightTWIPS ? fHeightTWIPS : fImgHeightTWIPS;
	if (fMinWidthTWIPS)
	{
		if (width < fMinWidthTWIPS)
			width = fMinWidthTWIPS;
	}
	if (fMinHeightTWIPS)
	{
		if (height < fMinHeightTWIPS)
			height = fMinHeightTWIPS;
	}
	
	if (fWidthTWIPS && !fHeightTWIPS) //preserve aspect ratio for height
		height = width*fImgHeightTWIPS/fImgWidthTWIPS;
	else if (!fWidthTWIPS && fHeightTWIPS) //preserve aspect ratio for width
		width = height*fImgWidthTWIPS/fImgHeightTWIPS;

	width = fAllMargin.left+fAllMargin.right+RoundMetrics(NULL, ICSSUtil::TWIPSToPoint((sLONG)width)*fDPI/72);
	height = fAllMargin.top+fAllMargin.bottom+RoundMetrics(NULL, ICSSUtil::TWIPSToPoint((sLONG)height)*fDPI/72);

	//here layout bounds are relative to image top left (including all margins)
	_SetLayoutBounds( inGC, VRect(0,0,width,height));
}

/** update ascent (compute ascent relatively to line & text ascent, descent - not including line image(s) metrics) */
void VDocImageLayout::UpdateAscent(const GReal inLineAscent, const GReal inLineDescent, const GReal inTextAscent, const GReal inTextDescent)
{
	switch (fVerticalAlign)
	{
	case AL_TOP:
		fAscent = inLineAscent;
		break;
	case AL_CENTER:
		fAscent = inLineAscent-(inLineAscent+inLineDescent)*0.5+fLayoutBounds.GetHeight()*0.5;
		break;
	case AL_BOTTOM:
		fAscent = -inLineDescent+fLayoutBounds.GetHeight();
		break;
	case AL_SUPER:
		fAscent = inTextAscent*0.5+fLayoutBounds.GetHeight(); //superscript offset is relative to text ascent & not line ascent
		break;
	case AL_SUB:
		fAscent = -inTextAscent*0.5+fLayoutBounds.GetHeight(); //subscript offset is relative to text ascent & not line ascent
		break;
	default:
		//baseline
		fAscent = fLayoutBounds.GetHeight();
		break;
	}
}

/** create or retain image layer 

@remarks
	it is used to store a snapshot of the transformed image 

	depending on differences between current & last draw settings 
	it will create a new layer - snapshot of the transformed image (created from passed picture data & current draw settings) 
	or re-use and return the one from last draw 
*/
VImageOffScreenRef	VDocImageLayout::CreateOrRetainImageLayer(	VGraphicContext *inGC, 
																const VPictureData* inPictureData,
																const VImageOffScreenRef inLayer,
																const sDocImageLayerSettings& inDrawSettings, 
																const sDocImageLayerSettings* inLastDrawSettings,
																bool inRepeat)
{
	if (inGC->IsPrintingContext())
		return NULL;

	if (inLayer && inLastDrawSettings 
		&& 
		inDrawSettings.fDPI == inLastDrawSettings->fDPI
		&&
		inGC->IsCompatible(inLastDrawSettings->fGCType) //current gc should be compatible with gc used to create layer previously
		&& 
		inDrawSettings.fStamp == inLastDrawSettings->fStamp)
	{
		VAffineTransform ctm;
		inGC->UseReversedAxis();
		inGC->GetTransform(ctm);
		if (ctm.GetShearX() == 0  //TODO: should cache also if shearing or rotation ?
			&& 
			ctm.GetShearY() == 0
			&&
			ctm.GetScaleX() == inLastDrawSettings->fDrawTransform.GetScaleX()
			&&
			ctm.GetScaleY() == inLastDrawSettings->fDrawTransform.GetScaleY())
		{
			if (inDrawSettings.fDrawUserBounds.GetWidth() == inLastDrawSettings->fDrawUserBounds.GetWidth()
				&&
				inDrawSettings.fDrawUserBounds.GetHeight() == inLastDrawSettings->fDrawUserBounds.GetHeight())
				return RetainRefCountable(inLayer);
		}
	}
	

	inGC->SaveClip();
	inGC->ClipRect(VRect(0,0,0,0)); //ensure we do not draw now to gc (as we want only to render on layer)

	VAffineTransform ctm;
	inGC->UseReversedAxis();
	inGC->GetTransform( ctm);

	VRect bounds = inDrawSettings.fDrawUserBounds;
	bounds.SetPosTo(0,0);
	bounds = ctm.TransformRect(bounds);
#if VERSIONMAC
	inGC->Quartz2DToQD( bounds); //map to QD bounds
#endif
	bounds.SetPosTo(0,0);

	inGC->ResetTransform();
	inGC->UseReversedAxis();

	inGC->BeginLayerOffScreen( bounds, NULL, false);
	if (inRepeat)
		//to reduce discrepancies between tiles, we normalize dest bounds to pixel grid before drawing picture
		//(visual difference is unnoticeable for patterns & it reduces artifacts between contiguous tiles)
		bounds.NormalizeToInt(true);
	inGC->DrawPictureData( inPictureData, bounds); //here we always scale to fit as any other scaling is managed before & input bounds are already adjusted if needed
	VImageOffScreenRef layer = inGC->EndLayerOffScreen();

	inGC->UseReversedAxis();
	inGC->SetTransform(ctm);

	inGC->RestoreClip();

	return layer;
}


/** render layout element in the passed graphic context at the passed top left origin
@remarks		
	method does not save & restore gc context
*/
void VDocImageLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft, const GReal inOpacity)
{
	VAffineTransform ctm;
	_BeginLocalTransform( inGC, ctm, inTopLeft, true); //here (0,0) is sticked to image top left & coordinate space pixel is mapped to fDPI (72 on default so on default 1px = 1pt)
	//disable shape printer scaling as for printing, we draw at printer dpi yet - design metrics are computed at printer dpi (mandatory only for GDI)
	bool shouldScaleShapesWithPrinterScale = inGC->ShouldScaleShapesWithPrinterScale(false);

	//apply transparency if appropriate
	_BeginLayerOpacity( inGC, inOpacity);

	//draw background & border if any
	VDocBaseLayout::Draw( inGC);

	//draw image
	VRect bounds(	fLayoutBounds.GetX()+fAllMargin.left, fLayoutBounds.GetY()+fAllMargin.top, 
					fLayoutBounds.GetWidth()-fAllMargin.left-fAllMargin.right, fLayoutBounds.GetHeight()-fAllMargin.top-fAllMargin.bottom);
	
	VPictureDrawSettings *drawSettings = NULL;
	VPictureDrawSettings drawSettingsLocal;

	if (fPictureData)
	{
		if (inGC->IsPrintingContext())
		{
			//printing
			
			drawSettingsLocal.SetDevicePrinter(true); //important: svg images for instance might use different css styles while drawing onscreen or printing
			drawSettings = &drawSettingsLocal;

			if (inGC->IsGDIImpl())
			{
				//we draw at printer DPI: but VPictureData instances are drawed while printing with GDI at 72 dpi (using hard-coded GDIPlus context page scale = printerDPI/72)
				//so ensure to reverse to 72 dpi to ensure picture is drawed well (otherwise it would be scaled to printer DPI/72 twice)
				inGC->ShouldScaleShapesWithPrinterScale( shouldScaleShapesWithPrinterScale);
				_EndLocalTransform( inGC, ctm, true);

				if (!shouldScaleShapesWithPrinterScale)
				{
					//we draw at printer DPI

					GReal scaleX, scaleY;
					inGC->GetPrinterScale( scaleX, scaleY);
					xbox_assert(scaleX && scaleY);

					//scale transform & bounds back to 72
					VAffineTransform ctmDrawing = ctm;
					ctmDrawing.SetTranslation(ctm.GetTranslateX()/scaleX, ctm.GetTranslateY()/scaleY);
					ctmDrawing.SetShearing( ctm.GetShearX()/scaleX, ctm.GetShearY()/scaleY);
					ctmDrawing.SetScaling( ctm.GetScaleX()/scaleX, ctm.GetScaleY()/scaleY);
					ctmDrawing.Translate( inTopLeft.x, -fAscent+inTopLeft.y, VAffineTransform::MatrixOrderPrepend);

					//it is better to let GDIPlus to perform transform later
					drawSettingsLocal.SetDrawingMatrix( ctmDrawing);
					inGC->SetTransform( VAffineTransform());
				}
				else
				{
					//we draw at 72 dpi (might happen only if VDocImageLayout is drawed standalone & not as part of VDocParagraphLayout or VDocTextLayout flow)

					VAffineTransform ctmDrawing = ctm;
					ctmDrawing.Translate( inTopLeft.x, -fAscent+inTopLeft.y, VAffineTransform::MatrixOrderPrepend);

					//it is better to let GDIPlus to perform transform later
					drawSettingsLocal.SetDrawingMatrix( ctmDrawing);
					inGC->SetTransform( VAffineTransform());
				}
			}

			inGC->DrawPictureData( fPictureData, bounds, drawSettings);
		}
		else
		{
			//not printing

			//we use layer to store a snapshot of the transformed image

#if VERSIONWIN
			//GDI impl has support only for one offscreen layer at a time (and on Windows, gc uses one yet - because of composition)
			//so we derive to GDIPlus here in order to use offscreen layer for pattern
			VGraphicContext *gcBackup = NULL;
			if (inGC->IsGDIImpl())
			{
				gcBackup = inGC;
				inGC = inGC->BeginUsingGdiplus();
			}
#endif

			//prepare image layer settings
			sDocImageLayerSettings imgLayerSettings;
			imgLayerSettings.fDPI = fDPI;
			imgLayerSettings.fGCType = inGC->GetGraphicContextType();
			imgLayerSettings.fDrawUserBounds = bounds;
			imgLayerSettings.fStamp = fPictureDataStamp;
			inGC->UseReversedAxis();
			inGC->GetTransform( imgLayerSettings.fDrawTransform);

			//re-use layer or create new one
			VImageOffScreenRef layer = CreateOrRetainImageLayer( inGC, fPictureData, (VImageOffScreen *)fImgLayer, imgLayerSettings, &fImgLayerSettings);
			fImgLayer.Set(layer);

			//now draw layer
			if (layer)
			{
				//backup current layer settings
				fImgLayerSettings = imgLayerSettings;
				if (!testAssert(inGC->DrawLayerOffScreen( bounds, layer, true))) //draw always 
					inGC->DrawPictureData( fPictureData, bounds, NULL);
			}
			else
				inGC->DrawPictureData( fPictureData, bounds, NULL);
			ReleaseRefCountable(&layer);

#if VERSIONWIN
			if (gcBackup)
			{
				inGC = gcBackup;
				inGC->EndUsingGdiplus();
			}
#endif
		}
	}
	else
	{
		//draw default dummy image (TODO: we should use a default svg image for instance ?)
		StDisableShapeCrispEdges noCrispEdges(inGC);
		inGC->ClearLineDashPattern();
		inGC->SetLineColor( VColor(255,0,0));
		VAffineTransform ctm;
		inGC->GetTransformToScreen(ctm);
		GReal lineWidth = ctm.GetScaleX() ? 1/std::abs(ctm.GetScaleX()) : 1;
		inGC->SetLineWidth(lineWidth);
		inGC->DrawLine(bounds.GetTopLeft(), bounds.GetBotRight());
		inGC->DrawLine(bounds.GetTopRight(), bounds.GetBotLeft());
		inGC->FrameRect(bounds);
		inGC->SetLineColor( VColor(0,0,0));
	}

	_EndLayerOpacity( inGC, inOpacity);

	inGC->ShouldScaleShapesWithPrinterScale( shouldScaleShapesWithPrinterScale);
	_EndLocalTransform( inGC, ctm, true);
}


void VDocImageLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw)
{
	VDocBaseLayout::_BeginLocalTransform( inGC, outCTM, inTopLeft, inDraw, VPoint(0, -fAscent));
}

//
// VDocBackgroundImageLayout
//


/** background image layout 

@param inURL
	image url
@param inHAlign
	horizontal alignment (relative to origin rect - for starting tile if image is repeated)
@param inVAlign
	vertical alignment (relative to origin rect - for starting tile if image is repeated)
@param inRepeat
	repeat mode (painting is bound by inClipRect)
@param inWidth 
	if equal to kDOC_BACKGROUND_SIZE_COVER:	background image is scaled (preserving aspect ratio) in order to cover all origin rect (some image part might not be visible) - cf CSS W3C 'background-size'
												and inHeight is ignored
	if equal to kDOC_BACKGROUND_SIZE_CONTAIN:	background image is scaled (preserving aspect ratio) in order to be fully contained in origin rect (all image is visible) - cf CSS W3C 'background-size'
												and inHeight is ignored
	if equal to kDOC_BACKGROUND_SIZE_AUTO:		width is automatic (proportional to height according to image aspect ratio)

	otherwise it is either width in point if inWidthIsPercentage == false, or percentage of origin rect width if inWidthIsPercentage == true
@param inHeight (mandatory only if inWidth >= kDOC_BACKGROUND_SIZE_AUTO)
	if equal to kDOC_BACKGROUND_SIZE_AUTO:		height is automatic (proportional to width according to image aspect ratio)

	otherwise it is either height in point if inWidthIsPercentage == false, or percentage of origin rect height if inWidthIsPercentage == true
@param inWidthIsPercentage
	if true, inWidth is percentage of origin rect width (1 is equal to 100%)
@param inHeightIsPercentage
	if true, inHeight is percentage of origin rect height (1 is equal to 100%)
*/ 
VDocBackgroundImageLayout::VDocBackgroundImageLayout(const VString& inURL,	
			 const eStyleJust inHAlign, const eStyleJust inVAlign, 
			 const eDocPropBackgroundRepeat inRepeat,
			 const GReal inWidth, const GReal inHeight,
			 const bool inWidthIsPercentage,  const bool inHeightIsPercentage)
{
	_Init();
	fDPI				= 72;
	fURL				= inURL;
	fHAlign				= inHAlign;
	fVAlign				= inVAlign;
	fRepeat				= inRepeat;
	fWidth				= inWidth;
	fHeight				= inHeight;
	fWidthIsPercentage	= inWidthIsPercentage;
	fHeightIsPercentage	= inHeightIsPercentage;
	if (fWidth == kDOC_BACKGROUND_SIZE_AUTO)
		fWidthIsPercentage = false;
	if (fWidth < 0)
		fHeight = kDOC_BACKGROUND_SIZE_AUTO;
	if (fHeight == kDOC_BACKGROUND_SIZE_AUTO)
		fHeightIsPercentage = false;
}

VDocBackgroundImageLayout::~VDocBackgroundImageLayout()
{
	ReleaseRefCountable(&fPictureData);
}

void VDocBackgroundImageLayout::_Init()
{
	fImgWidthTWIPS		= 0;
	fImgHeightTWIPS		= 0;
	fPictureData		= NULL;

	fHAlign				= JST_Left;
	fVAlign				= JST_Top;
	fRepeat				= kDOC_BACKGROUND_REPEAT;

	fWidth				= kDOC_BACKGROUND_SIZE_AUTO;
	fHeight				= kDOC_BACKGROUND_SIZE_AUTO;
	fWidthIsPercentage	= false;
	fHeightIsPercentage	= false;

	fPictureDataStamp	= 0;
	fImgLayerSettings.fDPI = 0;
}


/** get image width in px (relative to layout dpi) */
GReal VDocBackgroundImageLayout::GetImageWidth() const
{
	return VDocBaseLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint( fImgWidthTWIPS)*fDPI/72);
}

/** get image height in px (relative to layout dpi) */
GReal VDocBackgroundImageLayout::GetImageHeight() const
{
	return VDocBaseLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint( fImgHeightTWIPS)*fDPI/72);
}


void VDocBackgroundImageLayout::SetClipBounds( const VRect& inClipRect)
{
	fClipBounds			= inClipRect;
}

void VDocBackgroundImageLayout::SetOriginBounds( const VRect& inOriginRect)
{
	fOriginBounds		= inOriginRect;
}


/** update layout metrics */ 
void VDocBackgroundImageLayout::UpdateLayout(VGraphicContext *inGC)
{
	if (fClipBounds.IsEmpty())
		return;

	VAffineTransform ctm;
	inGC->GetTransform( ctm);

	//update picture data
	const VPictureData *pictureData = VDocImageLayoutPictureDataManager::Get()->Retain( fURL, &fPictureDataStamp);
	CopyRefCountable(&fPictureData, pictureData);
	ReleaseRefCountable(&pictureData);
	if (!fPictureData)
		fImgLayer.Set( (VImageOffScreen *)NULL);

	if (fPictureData)
	{
		//for compatibility with 4D form (not dpi-aware), we assume on default 72 dpi for the images
		fImgWidthTWIPS = fPictureData->GetWidth()*20;
		fImgHeightTWIPS = fPictureData->GetHeight()*20;
	}
	else
		fImgWidthTWIPS = fImgHeightTWIPS = 32*20;

	//determine image drawing bounds

	if (fWidth < 0) //cover or contain
	{
		VRect boundsSrc(0,0,fImgWidthTWIPS,fImgHeightTWIPS);
		VAffineTransform xform;
		
		uLONG keepAspectRatio = fWidth == kDOC_BACKGROUND_SIZE_COVER ? kKAR_COVER : 0;

		switch (fHAlign)
		{
		case JST_Right:
			keepAspectRatio = keepAspectRatio | kKAR_XMAX;
			break;
		case JST_Center:
			keepAspectRatio = keepAspectRatio | kKAR_XMID;
			break;
		default:
			keepAspectRatio = keepAspectRatio | kKAR_XMIN;
			break;
		}

		switch (fVAlign)
		{
		case JST_Bottom:
			keepAspectRatio = keepAspectRatio | kKAR_YMAX;
			break;
		case JST_Center:
			keepAspectRatio = keepAspectRatio | kKAR_YMID;
			break;
		default:
			keepAspectRatio = keepAspectRatio | kKAR_YMIN;
			break;
		}

		VDocBaseLayout::ComputeViewportTransform( xform, boundsSrc, fOriginBounds, keepAspectRatio);
		fImgDrawBounds = xform.TransformRect( boundsSrc);
	}
	else
	{
		bool widthAuto = !fWidthIsPercentage && fWidth == kDOC_BACKGROUND_SIZE_AUTO;
		bool heightAuto = !fHeightIsPercentage && fHeight == kDOC_BACKGROUND_SIZE_AUTO;

		GReal width, height;

		width	= !widthAuto ?	(fWidthIsPercentage ? fOriginBounds.GetWidth()*fWidth : VDocBaseLayout::RoundMetrics(NULL, fWidth*fDPI/72)) : VDocBaseLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint((sLONG)fImgWidthTWIPS)*fDPI/72);
		height	= !heightAuto ? (fHeightIsPercentage ? fOriginBounds.GetHeight()*fHeight : VDocBaseLayout::RoundMetrics(NULL, fHeight*fDPI/72)) : VDocBaseLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint((sLONG)fImgHeightTWIPS)*fDPI/72);
		
		if (fRepeat != kDOC_BACKGROUND_NO_REPEAT && ctm.GetScaleX() && ctm.GetScaleY())
		{
			//for performance, we ensure minimal size for patterns 
			if (width*ctm.GetScaleX() < 8)
				width = 8/ctm.GetScaleX();
			if (height*ctm.GetScaleY() < 8)
				height = 8/ctm.GetScaleY();
		}

		if (!widthAuto && heightAuto) //preserve aspect ratio for height
			height = width*fImgHeightTWIPS/fImgWidthTWIPS;
		else if (widthAuto && !heightAuto) //preserve aspect ratio for width
			width = height*fImgWidthTWIPS/fImgHeightTWIPS;

		GReal x = fOriginBounds.GetX(), y = fOriginBounds.GetY();
		switch (fHAlign)
		{
		case JST_Right:
			x = x + fOriginBounds.GetWidth() - width;
			break;
		case JST_Center:
			x = x + (fOriginBounds.GetWidth()-width)*0.5;
			break;
		default:
			break;
		}

		switch (fVAlign)
		{
		case JST_Bottom:
			y = y + fOriginBounds.GetHeight() - height;
			break;
		case JST_Center:
			y = y + (fOriginBounds.GetHeight()-height)*0.5;
			break;
		default:
			break;
		}

		fImgDrawBounds.SetCoordsTo( x, y, width, height);
	}

	fRepeatXCount = 1;
	fRepeatYCount = 1;

	if (fRepeat != kDOC_BACKGROUND_NO_REPEAT)
	{
		//decal start image bounds & determine image count on x and y according to repeat mode
		
		sLONG countX = 0, countY = 0;
		VRect imgBounds = fImgDrawBounds;
		GReal startX = fImgDrawBounds.GetX(), startY = fImgDrawBounds.GetY();

		if (fRepeat == kDOC_BACKGROUND_REPEAT || fRepeat == kDOC_BACKGROUND_REPEAT_X)
		{
			if (imgBounds.GetX() > fClipBounds.GetX())
				countX = (imgBounds.GetX()-fClipBounds.GetX()+imgBounds.GetWidth()-1)/imgBounds.GetWidth();
			if (countX)
			{
				fRepeatXCount += countX;
				startX = imgBounds.GetX()-countX*imgBounds.GetWidth();
			}

			countX = 0;
			if (imgBounds.GetRight() < fClipBounds.GetRight())
				countX = (fClipBounds.GetRight()-imgBounds.GetRight()+imgBounds.GetWidth()-1)/imgBounds.GetWidth();
			fRepeatXCount += countX;
		}

		if (fRepeat == kDOC_BACKGROUND_REPEAT || fRepeat == kDOC_BACKGROUND_REPEAT_Y)
		{
			if (imgBounds.GetY() > fClipBounds.GetY())
				countY = (imgBounds.GetY()-fClipBounds.GetY()+imgBounds.GetHeight()-1)/imgBounds.GetHeight();
			if (countY)
			{
				fRepeatYCount += countY;
				startY = imgBounds.GetY()-countY*imgBounds.GetHeight();
			}

			countY = 0;
			if (imgBounds.GetBottom() < fClipBounds.GetBottom())
				countY = (fClipBounds.GetBottom()-imgBounds.GetBottom()+imgBounds.GetHeight()-1)/imgBounds.GetHeight();
			fRepeatYCount += countY;
		}

		fImgDrawBounds.SetPosTo( startX, startY);
	}
}

    
/** render layout element in the passed graphic context at the passed top left origin
@remarks		
	method does not save & restore gc context
*/
void VDocBackgroundImageLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft)
{
	if (fClipBounds.IsEmpty())
		return;

	//draw image
	
	VPictureDrawSettings *drawSettings = NULL;
	VPictureDrawSettings drawSettingsLocal;

	VAffineTransform ctm;
	inGC->UseReversedAxis();
	inGC->GetTransform( ctm);

	if (ctm.GetScaleX() == 0.0f || ctm.GetScaleY() == 0.0f)
		return;

	//clip background
	inGC->SaveClip();
	VRect clipBounds( fClipBounds);
	clipBounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
	inGC->ClipRect( clipBounds);

	VRect imgBounds = fImgDrawBounds;
	imgBounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());

	if (inGC->IsPrintingContext() && fPictureData)
	{
		drawSettingsLocal.SetDevicePrinter(true); //important: svg images for instance might use different css styles while drawing onscreen or printing
		drawSettings = &drawSettingsLocal;

		if (inGC->IsGDIImpl())
		{
			//we draw at printer DPI: but VPictureData instances are drawed while printing with GDI at 72 dpi (using hard-coded GDIPlus context page scale = printerDPI/72)
			//so ensure to reverse to 72 dpi to ensure picture is drawed well (otherwise it would be scaled to printer DPI/72 twice)
			if (!inGC->ShouldScaleShapesWithPrinterScale())
			{
				//we draw at printer DPI

				GReal scaleX, scaleY;
				inGC->GetPrinterScale( scaleX, scaleY);
				xbox_assert(scaleX && scaleY);

				//scale transform & bounds back to 72
				VAffineTransform ctmDrawing = ctm;
				ctmDrawing.SetTranslation(ctmDrawing.GetTranslateX()/scaleX, ctmDrawing.GetTranslateY()/scaleY);
				ctmDrawing.SetShearing( ctmDrawing.GetShearX()/scaleX, ctmDrawing.GetShearY()/scaleY);
				ctmDrawing.SetScaling( ctmDrawing.GetScaleX()/scaleX, ctmDrawing.GetScaleY()/scaleY);

				imgBounds.SetSizeBy( 1/scaleX, 1/scaleY);
				imgBounds.SetPosBy( 1/scaleX, 1/scaleY);

				//it is better to let GDIPlus to perform transform later
				drawSettingsLocal.SetDrawingMatrix( ctmDrawing);
				inGC->SetTransform( VAffineTransform());
			}
			else
			{
				//we draw at 72 dpi (might happen only if VDocImageLayout is drawed standalone & not as part of VDocParagraphLayout or VDocTextLayout flow)

				//it is better to let GDIPlus to perform transform later
				drawSettingsLocal.SetDrawingMatrix( ctm);
				inGC->SetTransform( VAffineTransform());
			}
		}
	}

	if (fPictureData)
	{
		if (!inGC->IsPrintingContext())
		{
			//not printing

			//we use layer to store a snapshot of the transformed image 

			bool repeat = (fRepeatXCount > 1 || fRepeatYCount > 1) && testAssert(ctm.GetShearX() == 0.0f && ctm.GetShearY() == 0.0f);

#if VERSIONWIN
			//GDI impl has support only for one offscreen layer at a time (and on Windows, gc uses one yet - because of composition)
			//so we derive to GDIPlus here in order to use offscreen layer for pattern
			VGraphicContext *gcBackup = NULL;
			if (inGC->IsGDIImpl())
			{
				gcBackup = inGC;
				inGC = inGC->BeginUsingGdiplus();
			}
#endif

			//prepare image layer settings
			sDocImageLayerSettings imgLayerSettings;
			imgLayerSettings.fDPI = fDPI;
			imgLayerSettings.fGCType = inGC->GetGraphicContextType();
			imgLayerSettings.fDrawUserBounds = imgBounds;
			imgLayerSettings.fStamp = fPictureDataStamp;
			inGC->UseReversedAxis();
			inGC->GetTransform( imgLayerSettings.fDrawTransform);

			//re-use layer or create new one
			VImageOffScreenRef layer = VDocImageLayout::CreateOrRetainImageLayer( inGC, fPictureData, (VImageOffScreen *)fImgLayer, imgLayerSettings, &fImgLayerSettings, repeat);
			fImgLayer.Set(layer);

			//now draw first tile
			if (layer) 
			{
				//backup current layer settings
				fImgLayerSettings = imgLayerSettings;
				if (!testAssert(inGC->DrawLayerOffScreen( imgBounds, layer, true)))
					inGC->DrawPictureData( fPictureData, imgBounds, NULL);
			}
			else
				inGC->DrawPictureData( fPictureData, imgBounds, NULL);

			if (repeat)
			{
				//repeat 
      
				if (layer)    
				{
					//now blit layer fRepeatXCount x fRepeatYCount - 1 times

					VRect bounds = imgBounds;
					for(int j = 0; j < fRepeatYCount; j++)
					{
						for (int i = 0; i < fRepeatXCount; i++)
						{
							if (i+j > 0)
							{
								bool ok = inGC->DrawLayerOffScreen( bounds, layer, true); //always draw layer 
								if (!testAssert(ok)) //should never occur as we force draw layer
									inGC->DrawPictureData( fPictureData, bounds, NULL);
							}
							bounds.SetPosBy( bounds.GetWidth(),0);
						}
						bounds.SetPosTo(imgBounds.GetX(), bounds.GetBottom());
					}
				}
			}
			ReleaseRefCountable(&layer);

#if VERSIONWIN
			if (gcBackup)
			{
				inGC = gcBackup;
				inGC->EndUsingGdiplus();
			}
#endif
		}
		else
		{	
			//printing (cannot use layer)

			if (fRepeatXCount == 1 && fRepeatYCount == 1)
				//no repeat
				inGC->DrawPictureData( fPictureData, imgBounds, drawSettings);
			else
			{
				//repeat 

				VRect bounds = imgBounds;
				for(int j = 0; j < fRepeatYCount; j++)
				{
					for (int i = 0; i < fRepeatXCount; i++)
					{
						inGC->DrawPictureData( fPictureData, bounds, drawSettings);
						bounds.SetPosBy( bounds.GetWidth(),0);
					}
					bounds.SetPosTo(imgBounds.GetX(), bounds.GetBottom());
				}
			}
		}
	}
	else
	{
		//draw default dummy image (TODO: we should use a default svg image for instance)

		inGC->SetFillColor( VColor(255,0,0));

		if (fRepeatXCount == 1 && fRepeatYCount == 1)
			//no repeat
			inGC->FillRect(imgBounds);
		else
		{
			//repeat 

			VRect bounds = imgBounds;
			for(int j = 0; j < fRepeatYCount; j++)
			{
				for (int i = 0; i < fRepeatXCount; i++)
				{
					inGC->FillRect(bounds);
					bounds.SetPosBy( bounds.GetWidth(),0);
				}
				bounds.SetPosTo(imgBounds.GetX(), bounds.GetBottom());
			}
		}
	
		inGC->SetFillColor( VColor(255,255,255));
	}


	inGC->RestoreClip();

	inGC->UseReversedAxis();
	inGC->SetTransform( ctm);
}



//
// VDocImageLayoutPictureDataManager
//


VDocImageLayoutPictureDataManager* VDocImageLayoutPictureDataManager::sSingleton = NULL;

VDocImageLayoutPictureDataManager* VDocImageLayoutPictureDataManager::Get()
{
	if (!sSingleton)
		Init();
	return sSingleton;
}

bool VDocImageLayoutPictureDataManager::Init()
{
	sSingleton = new VDocImageLayoutPictureDataManager();
	return true;
}
void VDocImageLayoutPictureDataManager::Deinit()
{
	if (sSingleton)
		delete sSingleton;
}


VDocImageLayoutPictureDataManager::VDocImageLayoutPictureDataManager()
{
	fAppIntf = 0;
	fResourceFolderCacheCount = 0;
}

VDocImageLayoutPictureDataManager::~VDocImageLayoutPictureDataManager()
{
	VectorOfFolderCache::const_iterator it = fFolderCache.begin();
	for (;it != fFolderCache.end(); it++)
		(*it)->Release();
}


/** return picture data from encoded url */
const VPictureData* VDocImageLayoutPictureDataManager::Retain( const VString& inUrl, uLONG8 *outStamp)
{
	VTaskLock protect(&fMutex);

	StErrorContextInstaller errorContext(false); //silent errors

	bool doInit = fFolderCache.empty();
	if (doInit)
		fResourceFolderCacheCount = 0;

	if (fAppIntf)
	{
		//add, replace or clear database resources folder
		VFilePath filePath;
		VError error = fAppIntf->GetDBResourcePath( filePath);
		if (error == VE_OK)
		{
			if (doInit || !fCurDBPath.IsFolder() || fCurDBPath != filePath)
			{
				VFolder *folder = new VFolder( filePath);
				if (folder->HasValidPath())
				{
					VPictureProxyCache_Folder *folderCache = new VPictureProxyCache_Folder( *folder);
					if (fCurDBPath.IsFolder())
					{
						xbox_assert(fFolderCache.size() >= 1);
						VPictureProxyCache_Folder *old = fFolderCache[0];
						ReleaseRefCountable(&old);
						fFolderCache[0] = folderCache;
					}
					else if (doInit)
					{
						fFolderCache.push_back( folderCache);
						fResourceFolderCacheCount++;
					}
					else
					{
						fFolderCache.insert(fFolderCache.begin(), folderCache);
						fResourceFolderCacheCount++;
					}
				}
				else if (fCurDBPath.IsFolder())
				{
					//remove previous database folder (as new database folder is invalid)
					xbox_assert(fFolderCache.size() >= 1);
					VPictureProxyCache_Folder *old = fFolderCache[0];
					ReleaseRefCountable(&old);
					fFolderCache.erase(fFolderCache.begin());
					filePath.Clear();
					fResourceFolderCacheCount--;
				}
				ReleaseRefCountable(&folder);
				fCurDBPath = filePath;
			}
		}
		else if (fCurDBPath.IsFolder())
		{
			//remove previous database folder (as new database folder is invalid)
			xbox_assert(fFolderCache.size() >= 1);
			VPictureProxyCache_Folder *old = fFolderCache[0];
			ReleaseRefCountable(&old);
			fFolderCache.erase(fFolderCache.begin());
			fCurDBPath.Clear();
			fResourceFolderCacheCount--;
		}
	}
	if (doInit)
	{
		//add application resources folder
		VFolder *folder = VProcess::Get()->RetainFolder( VProcess::eFS_Resources);
		if (folder->HasValidPath())
		{
			VPictureProxyCache_Folder *folderCache = new VPictureProxyCache_Folder( *folder);
			fFolderCache.push_back( folderCache);
			fResourceFolderCacheCount++;
		}
		ReleaseRefCountable(&folder);
	}
	
	const VPictureData *pictureData = NULL;
	uLONG8 stamp = 0;
	if (inUrl.BeginsWith( CVSTR("file:")))
	{
		//absolute url local file 

		//build VURL from URL string
		VURL *url	= new VURL( inUrl, true);	
		xbox_assert(url);

		//build VFile from VURL
		VFile *file = new VFile( *url);
		delete url;

		if (file->HasValidPath())
		{
			//iterate on folder caches to get the picture
			VString pathRelative;
			VectorOfFolderCache::const_iterator it = fFolderCache.begin();
			for (;it != fFolderCache.end(); it++)
			{
				if ((*it)->IsPictureFileURL(*file, pathRelative))
				{
					VPicture picture;
					if ((*it)->GetPicture( pathRelative, picture, stamp) == VE_OK)
						pictureData = picture.RetainPictDataForDisplay();
					break;
				}
			}

			if (!pictureData && it == fFolderCache.end())
			{
				//file is not mapped with any folder cache yet -> add new folder cache using file parent folder
				VFolder *folderParent = file->RetainParentFolder();
				if (folderParent)
				{
					VPictureProxyCache_Folder *folderCache = new VPictureProxyCache_Folder( *folderParent);
					fFolderCache.push_back( folderCache);

					if (testAssert(folderCache->IsPictureFileURL(*file, pathRelative)))
					{
						VPicture picture;
						if (folderCache->GetPicture( pathRelative, picture, stamp) == VE_OK)
							pictureData = picture.RetainPictDataForDisplay();
					}
				}
				ReleaseRefCountable(&folderParent);
			}
		}
		ReleaseRefCountable(&file);
	}
	else if (inUrl.FindUniChar( ':') <= 0 && !inUrl.IsEmpty())
	{
		//relative url -> iterate only on database & app resources folder caches

		//decode url (VPictureProxyCache_Folder::GetPicture accepts only decoded url i.e. posix path)
		VString urlDecoded;
		VURL *url = new VURL( inUrl, true);	
		url->GetAbsoluteURL( urlDecoded, false);
		delete url;

		sLONG resourceFolderCount = fResourceFolderCacheCount;
		VectorOfFolderCache::const_iterator it = fFolderCache.begin();
		for (;resourceFolderCount && it != fFolderCache.end(); it++, resourceFolderCount--)
		{
			VPicture picture;
			if ((*it)->GetPicture( urlDecoded, picture, stamp) == VE_OK)
			{
				pictureData = picture.RetainPictDataForDisplay();
				break;
			}
		}
	}
	if (outStamp)
		*outStamp = stamp;
	return pictureData;
}

/** autowash cached pictures */ 
void VDocImageLayoutPictureDataManager::AutoWash()
{
	VTaskLock protect(&fMutex);

	VectorOfFolderCache::const_iterator it = fFolderCache.begin();
	for (;it != fFolderCache.end(); it++)
		(*it)->AutoWash();
}


/** clear cached pictures */ 
void VDocImageLayoutPictureDataManager::Clear()
{
	VTaskLock protect(&fMutex);

	VectorOfFolderCache::const_iterator it = fFolderCache.begin();
	for (;it != fFolderCache.end(); it++)
		(*it)->Clear();
}


