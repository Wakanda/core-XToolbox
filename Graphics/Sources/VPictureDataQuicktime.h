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
#ifndef __VPictureDataQuicktime__
#define __VPictureDataQuicktime__

#if USE_QUICKTIME

BEGIN_TOOLBOX_NAMESPACE

class XTOOLBOX_API VPictureData_Quicktime :public VPictureData_Bitmap
{
typedef VPictureData_Bitmap inherited;
protected:
	VPictureData_Quicktime();
	VPictureData_Quicktime(const VPictureData_Quicktime& inData);
	VPictureData_Quicktime& operator=(const VPictureData_Quicktime&){assert(false);return *this;};
public:
	
	VPictureData_Quicktime(VPictureDataProvider* inDataProvider,_VPictureAccumulator* inRecorder=0);
	virtual ~VPictureData_Quicktime();

	virtual void DrawInPortRef(PortRef inPortRef,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#if VERSIONWIN
	virtual void DrawInGDIPlusGraphics(Gdiplus::Graphics* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#if ENABLE_D2D
	virtual void DrawInD2DGraphicContext(VWinD2DGraphicContext* inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
#endif
	#else
	virtual void DrawInCGContext(CGContextRef inDC,const VRect& r,VPictureDrawSettings* inSet=NULL)const;
	#endif
	virtual VError Save(VBlob* inData,VIndex inOffset,VSize& outSize,_VPictureAccumulator* inRecorder=0)const;

	virtual void FromVFile(VFile& inFile);
	
	virtual VPictureData* Clone()const;
	
	#if VERSIONWIN
	virtual Gdiplus::Bitmap* GetGDIPlus_Bitmap()const;
	#else
	virtual CGImageRef	GetCGImage()const;
	#endif
	
protected:
	virtual void _DoReset()const;
	virtual void _DoLoad()const;
private:
	void _Init();
	#if VERSIONMAC
	mutable CGImageRef fCGImage;
	#else
	mutable Gdiplus::Bitmap*	fGDIBm;
	mutable void*				fDIB;
	#endif
	mutable QTInstanceRef		fComponentInstance;
	mutable class xQTPointerDataRef*	fQTDataRef;
};

END_TOOLBOX_NAMESPACE

#endif

#endif