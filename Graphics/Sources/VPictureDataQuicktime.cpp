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
#include "Kernel/VKernel.h"
#include "VQuickTimeSDK.h"

#include "V4DPictureIncludeBase.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif

#if USE_QUICKTIME

VPictureData_Quicktime::VPictureData_Quicktime()
:inherited()
{
	_Init();
}
VPictureData_Quicktime::VPictureData_Quicktime(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder)
:inherited(inDataProvider,inRecorder)
{
	_Init();
	
}
VPictureData_Quicktime::VPictureData_Quicktime(const VPictureData_Quicktime& inData)
:inherited(inData)
{
	_Init();
}

VPictureData_Quicktime::~VPictureData_Quicktime()
{
	_DoReset();
}

VError VPictureData_Quicktime::Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder)const
{
	VError result=VE_UNKNOWN_ERROR;
	if(fDataProvider)
	{
		result=inherited::Save(inData,inOffset,outSize);
	}
	return result;
}
void VPictureData_Quicktime::_Init()
{
	fComponentInstance=0;
	fQTDataRef=0;
	#if VERSIONMAC
	fCGImage=0;
	#else
	fDIB=0;
	fGDIBm=0;
	#endif
}
#if VERSIONWIN
Gdiplus::Bitmap*	VPictureData_Quicktime::GetGDIPlus_Bitmap()const
{
	_Load();
	return fGDIBm;
}
#else
CGImageRef VPictureData_Quicktime::GetCGImage()const
{
	_Load();
	return fCGImage;
}
#endif
VPictureData* VPictureData_Quicktime::Clone()const
{
	return new VPictureData_Quicktime(*this); 
}

void VPictureData_Quicktime::DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	#if VERSIONWIN
	if(fGDIBm)
		xDraw(fGDIBm,inPortRef,r,inSet);
	#else
	if(fCGImage)
		xDraw(fCGImage,inPortRef,r,inSet);
	#endif
		
}
#if VERSIONWIN
#if ENABLE_D2D
void VPictureData_Quicktime::DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fGDIBm)
		xDraw(fGDIBm,inDC,r,inSet);
}
#endif
void VPictureData_Quicktime::DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fGDIBm)
		xDraw(fGDIBm,inDC,r,inSet);
}
#else
void VPictureData_Quicktime::DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet)const
{
	_Load();
	if(fCGImage)
		xDraw(fCGImage,inDC,r,inSet);
}
#endif
void VPictureData_Quicktime::_DoReset()const
{
	if(fComponentInstance)
		VQuicktime::CloseComponent(fComponentInstance);
	fComponentInstance=0;
	if(fQTDataRef)
		delete fQTDataRef;
	fQTDataRef=0;
	#if VERSIONWIN
	#if ENABLE_D2D
	VWinD2DGraphicContext::ReleaseBitmap(fGDIBm);
	#endif
	if(fGDIBm)
		delete fGDIBm;
	if(fDIB)
		GlobalFree(fDIB);
	fDIB=0	;
	fGDIBm=0;
	#else
	if(fCGImage)
		CFRelease(fCGImage);
	fCGImage=0;
	#endif
}
void VPictureData_Quicktime::_DoLoad()const
{

	if(!GetDataProvider())
		return;
#if VERSIONMAC
	if(fCGImage)
		return;
#else
	if(fGDIBm)
		return;
#endif

	QTIME::OSErr err;
	fQTDataRef=new xQTPointerDataRef(GetDataProvider());
	
	
	err = VQuicktime::GetGraphicsImporterForDataRef( *fQTDataRef,QTIME::PointerDataHandlerSubType,&fComponentInstance);
	if(err==VE_OK)
	{
		err = QTIME::GraphicsImportSetDataReference(fComponentInstance, *fQTDataRef,fQTDataRef->GetKind());
		if(!err)
		{
			#if VERSIONMAC
			
			
			err=GraphicsImportCreateCGImage(fComponentInstance,&fCGImage,kGraphicsImportCreateCGImageUsingCurrentSettings );
			if(fCGImage)
			{
				fBounds.SetCoords(0,0,CGImageGetWidth(fCGImage),CGImageGetHeight(fCGImage));
			}
			#else
			QTIME::ImageDescriptionHandle     desc=0;
			err=QTIME::GraphicsImportGetImageDescription (fComponentInstance,&desc);
			if(err==0 && desc)
			{
				qtime::PicHandle macpict;
				fBounds.SetCoords(0,0,(**desc).width,(**desc).height);
				
				qtime::DisposeHandle((qtime::Handle)desc);
				
				err = qtime::GraphicsImportGetAsPicture(fComponentInstance, &macpict);
				if(err==0)
				{
					fDIB = qtime::GetDIBFromPICT(macpict);
					if(fDIB)
					{
						BITMAPINFO* bm=(BITMAPINFO*)GlobalLock(fDIB);
						if(bm)
						{
							char* pixels;
							long rgbquadsize;
							long oldmode;

							if(bm->bmiHeader.biClrUsed)
								rgbquadsize = bm->bmiHeader.biClrUsed*sizeof(RGBQUAD);
							else 
							{
								switch(bm->bmiHeader.biBitCount)
								{
									case 1:
										rgbquadsize = 2*sizeof(RGBQUAD);
										break;
									case 4:
										rgbquadsize = 16*sizeof(RGBQUAD);
										break;
									case 8:
										rgbquadsize = 256*sizeof(RGBQUAD);
										break;
									case 16:
									case 32:
										if(bm->bmiHeader.biCompression==BI_RGB)
											rgbquadsize = 0;
										else
											rgbquadsize = 3*sizeof(DWORD);
										break;
									case 24:
										rgbquadsize = 0;
										break;
								}
							}
							if(bm->bmiHeader.biSizeImage==0)
							bm->bmiHeader.biSizeImage = ((((bm->bmiHeader.biWidth  * bm->bmiHeader.biBitCount) + 31) & ~31) >> 3) * bm->bmiHeader.biHeight ; 

							pixels = (char*)bm+bm->bmiHeader.biSize+rgbquadsize;
							
							fGDIBm=new Gdiplus::Bitmap(bm,(void*)pixels);
						}
					}
					qtime::KillPicture(macpict);
				}
			}
			#endif
		err = VQuicktime::CloseComponent(fComponentInstance);
		fComponentInstance=0;
		}
	}
}

void VPictureData_Quicktime::FromVFile(VFile& inFile)
{
	VPictureCodecFactoryRef fact;
	SetDataProvider(0,true);
	VFileDesc* outFileDesc;
	
	const VPictureCodec* decoder=fact->RetainDecoder(inFile,true);
	if(decoder)
	{
		VPictureDataProvider* datasrc=VPictureDataProvider::Create(inFile);
		if(datasrc)
		{
			datasrc->SetDecoder(decoder);
			SetDataProvider(datasrc);
			datasrc->Release();
		}
		decoder->Release();
	}
}

#endif