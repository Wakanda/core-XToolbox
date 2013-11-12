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
}

/** set layout DPI */
void VDocImageLayout::SetDPI( const GReal inDPI)
{
	VDocBaseLayout::SetDPI( inDPI);
}

/** get image width in px (relative to layout dpi) */
GReal VDocImageLayout::GetImageWidth() const
{
	return VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint( fImgWidthTWIPS)*fDPI/72);
}

/** get image height in px (relative to layout dpi) */
GReal VDocImageLayout::GetImageHeight() const
{
	return VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint( fImgHeightTWIPS)*fDPI/72);
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
	const VPictureData *pictureData = VDocImageLayoutPictureDataManager::Get()->Retain( fURL);
	CopyRefCountable(&fPictureData, pictureData);
	ReleaseRefCountable(&pictureData);

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

	width = fAllMargin.left+fAllMargin.right+VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint((sLONG)width)*fDPI/72);
	height = fAllMargin.top+fAllMargin.bottom+VTextLayout::RoundMetrics(NULL, ICSSUtil::TWIPSToPoint((sLONG)height)*fDPI/72);

	//here design bounds are relative to image top left (including all margins)
	_SetDesignBounds( VRect(0,0,width,height));
}

/** update ascent (compute ascent relatively to line & text ascent, descent - not including line image(s) metrics) */
void VDocImageLayout::UpdateAscent(const GReal inLineAscent, const GReal inLineDescent, const GReal inTextAscent, const GReal inTextDescent)
{
	switch (fVerticalAlign)
	{
	case JST_Top:
		fAscent = inLineAscent;
		break;
	case JST_Center:
		fAscent = inLineAscent-(inLineAscent+inLineDescent)*0.5+fDesignBounds.GetHeight()*0.5;
		break;
	case JST_Bottom:
		fAscent = -inLineDescent+fDesignBounds.GetHeight();
		break;
	case JST_Superscript:
		fAscent = inTextAscent*0.5+fDesignBounds.GetHeight(); //superscript offset is relative to text ascent & not line ascent
		break;
	case JST_Subscript:
		fAscent = -inTextAscent*0.5+fDesignBounds.GetHeight(); //subscript offset is relative to text ascent & not line ascent
		break;
	default:
		//baseline
		fAscent = fDesignBounds.GetHeight();
		break;
	}
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
	VRect bounds(	fDesignBounds.GetX()+fAllMargin.left, fDesignBounds.GetY()+fAllMargin.top, 
					fDesignBounds.GetWidth()-fAllMargin.left-fAllMargin.right, fDesignBounds.GetHeight()-fAllMargin.top-fAllMargin.bottom);
	
	VPictureDrawSettings *drawSettings = NULL;
	VPictureDrawSettings drawSettingsLocal;

	if (fPictureData)
	{
		if (inGC->IsPrintingContext())
		{
			drawSettingsLocal.SetDevicePrinter(true); //important: svg images for instance might use different css styles while drawing onscreen or printing
			drawSettings = &drawSettingsLocal;
		}

		if (inGC->IsGDIImpl() && inGC->HasPrinterScale())
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
				VAffineTransform ctm2 = ctm;
				ctm2.SetTranslation(ctm2.GetTranslateX()/scaleX, ctm2.GetTranslateY()/scaleY);
				ctm2.SetScaling( ctm2.GetScaleX()/scaleX, ctm2.GetScaleY()/scaleY);
				ctm2.Translate( inTopLeft.x, -fAscent+inTopLeft.y, VAffineTransform::MatrixOrderPrepend);

				//it is better to let GDIPlus to perform transform later
				drawSettingsLocal.SetDrawingMatrix( ctm2);
				drawSettings = &drawSettingsLocal;
				inGC->SetTransform( VAffineTransform());
			}
			else
			{
				//we draw at 72 dpi (might happen only if VDocImageLayout is drawed standalone & not as part of VDocParagraphLayout or VDocTextLayout flow)

				VAffineTransform ctm2 = ctm;
				ctm2.Translate( inTopLeft.x, -fAscent+inTopLeft.y, VAffineTransform::MatrixOrderPrepend);

				//it is better to let GDIPlus to perform transform later
				drawSettingsLocal.SetDrawingMatrix( ctm2);
				drawSettings = &drawSettingsLocal;
				inGC->SetTransform( VAffineTransform());
			}
		}
		
		inGC->DrawPictureData( fPictureData, bounds, drawSettings);
	}
	else
	{
		//draw default dummy image (TODO: we should use a default svg image for instance)
		inGC->SetFillColor( VColor(255,0,0));
		inGC->FillRect(bounds);
		inGC->SetFillColor( VColor(255,255,255));
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
const VPictureData* VDocImageLayoutPictureDataManager::Retain( const VString& inUrl)
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


