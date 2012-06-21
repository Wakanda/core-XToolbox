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
#ifndef __VPicture__
#define __VPicture__

#include <vector>
#include <map>
#include <string>

BEGIN_TOOLBOX_NAMESPACE

class VPictureQDBridgeBase;
class VMacPictureAllocatorBase;
class VPictureData;
class xV4DPictureBlobRef;
class VPictureDataProvider;
enum
{
	kPict_DBG_Delete=0,
	kPict_DBG_Copy,
	kPictData_DBG_Delete,
	kPictData_DBG_Retain,
	kPictData_DBG_Release
};

class XTOOLBOX_API I4DPictureDebug
{
	public:
	I4DPictureDebug();
	virtual ~I4DPictureDebug();
	void	AddBreak(long id);
	bool	IsBreakEnable(long inID)const;
	void	RemoveBreak(long inID);
	void	ToogleBreak(long inID);
	void	InvokeBreak(long inID) const;
private:
	typedef std::map<long,long> _BreakMap;
	mutable _BreakMap fBreakMap;
};


#if _WIN32 || __GNUC__
#pragma pack(push,4)
#else
#pragma options align = power 
#endif

typedef struct xVPictureBlock
{
	sLONG fKind;
	uLONG fFlags;
	uLONG fHRes;
	uLONG fVRes;
	uLONG fWidth;
	uLONG fHeight;
	sLONG fDataOffset;
	uLONG fDataSize;
}xVPictureBlock, *xVPictureBlockPtr;

typedef struct xVPictureBlockHeaderV1
{
	sLONG fSign;
	sLONG fVersion;
	sLONG fNbPict;
	sLONG fDataSize;
}xVPictureBlockHeaderV1, *xVPictureBlockHeaderV1Ptr;

typedef struct xVPictureBlockHeaderV2
{
	xVPictureBlockHeaderV1 fBase;
	sWORD fPicEnd_PosX;
	sWORD fPicEnd_PosY;
	sWORD fPicEnd_TransfertMode;
}xVPictureBlockHeaderV2, *xVPictureBlockHeaderV2Ptr;

typedef struct xVPictureBlockHeaderV3
{
	xVPictureBlockHeaderV2 fHeaderV2;
	Real fMatrixData[9];
}xVPictureBlockHeaderV3, *xVPictureBlockHeaderV3Ptr;

typedef struct xVPictureBlockHeaderV4
{
	xVPictureBlockHeaderV3 fHeaderV3;
	sWORD fSplitH;
	sWORD fSplitV;
}xVPictureBlockHeaderV4, *xVPictureBlockHeaderV4Ptr;

typedef struct xVPictureBlockHeaderV5
{
	xVPictureBlockHeaderV1 fBase;
	sLONG					fBagSize;
}xVPictureBlockHeaderV5, *xVPictureBlockHeaderV5Ptr;

typedef struct xVPictureBlockHeaderV6
{
	xVPictureBlockHeaderV1 fBase;
	sLONG					fExtensionsBufferSize;
	sLONG					fBagSize;
}xVPictureBlockHeaderV6, *xVPictureBlockHeaderV6Ptr;

typedef struct xVPictureBlockHeaderV7
{
	xVPictureBlockHeaderV1	fBase;
	sLONG					fExtensionsBufferSize;
	sLONG					fBagSize;
	sLONG					fExtraDataSize;
}xVPictureBlockHeaderV7, *xVPictureBlockHeaderV7Ptr;


#if _WIN32 || __GNUC__
#pragma pack(pop)
#else
#pragma options align = reset
#endif



class XTOOLBOX_API VPicture_info : public VValueInfo
{
public:
	VPicture_info( ValueKind inTrueKind = VK_IMAGE):VValueInfo( VK_IMAGE, inTrueKind){;}
	virtual	VValue*					Generate() const;
	virtual	VValue*					Generate(VBlob* inBlob) const;
	virtual	VValue*					LoadFromPtr( const void *inBackStore, bool inRefOnly) const;
	virtual void*					ByteSwapPtr( void *inBackStore, bool inFromNative) const;

	virtual CompareResult			CompareTwoPtrToData( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual CompareResult			CompareTwoPtrToDataBegining( const void* inPtrToValueData1, const void* inPtrToValueData2, Boolean inDiacritical = false) const;
	virtual VSize					GetSizeOfValueDataPtr( const void* inPtrToValueData) const;
};

 
typedef std::map < VString , const class VPictureData* > _VPictDataMap ;
typedef std::pair < VString , const class VPictureData* > _VPictDataPair ;
typedef _VPictDataMap::iterator _VPictDataMap_Iterrator ;
typedef _VPictDataMap::const_iterator _VPictDataMap_Const_Iterrator ;

class XTOOLBOX_API xVPictureMapWatch :public VObject IWATCHABLE_NOREG(xVPictureMapWatch)
{
	public:
	xVPictureMapWatch(_VPictDataMap* inMap)
	{
		fMap=inMap;
	}
	virtual ~xVPictureMapWatch()
	{
	}
	virtual sLONG IW_CountProperties()
	{
		return (sLONG)fMap->size()+1;
	}
	virtual void IW_GetPropName(sLONG /*inPropID*/,VValueSingle& outName)
	{
		outName.FromString("PictData");
	}
	virtual void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue)
	{
		if(inPropID==0)
			return;
		sLONG i;
		_VPictDataMap_Iterrator it;
		for(i=1,it=fMap->begin();it!=fMap->end();it++,i++)
		{
			if(i==inPropID)
			{
				if((*it).second)
					IW_SetHexVal((sLONG_PTR)(*it).second,outValue);
				else
					IW_SetHexVal((sLONG_PTR)NULL,outValue);
			}
		}
	}
	virtual void IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild);
	

	private:
	_VPictDataMap* fMap;
};

class _VPictureAccumulator :public VObject, public IRefCountable
{
	public:
	_VPictureAccumulator();
	virtual ~_VPictureAccumulator();
	bool FindPictData(const VPictureData* inData,VIndex& outIndex);
	const VPictureData* GetPictureData(VIndex inIndex);
	bool AddPictureData(const VPictureData* inData,VIndex& outIndex);
	VSize GetDataSize();
	private:
	VArrayOf<const VPictureData*> fList;
};

class XTOOLBOX_API VPicture :public VValueSingle IWATCHABLE_NOREG(VPicture)
{
	public:
	static	const VPicture_info	sInfo;	
	static void Deinit();
	static void Init(VPictureQDBridgeBase* inQDBridge,VMacPictureAllocatorBase* inMacAllocator);

	VPicture(VFile& inFile,bool inAllowUnknownData);
	VPicture(const VPictureData* inData);
	VPicture(const VPicture& inData);
	VPicture();
	VPicture(VBlob* inBlob);
	VPicture(VStream* inStream,_VPictureAccumulator* inRecorder=0);
	VPicture(PortRef inPortRef,VRect inBounds);
	VPicture& operator=(const VPicture& inPict);
	
	virtual ~VPicture();
	
/*************************** inherited from VValue *******************************/
	virtual VError				ReadRawFromStream( VStream* ioStream, sLONG inParam = 0); //used by db4d server to read data without interpretation (for example : pictures)
	virtual Boolean				CanBeEvaluated () const { return false; }
	virtual	const VValueInfo*	GetValueInfo() const {return &sInfo;}
	virtual VPicture*			Clone() const ;
	virtual VValue*				FullyClone(bool ForAPush = false) const; /* no intelligent cloning (for example cloning by refcounting for Blobs)*/
	virtual void				RestoreFromPop(void* context);
	virtual Boolean				FromValueSameKind(const VValue* inValue);
	virtual VValueSingle*		GetDataBlob() const;

	virtual void				Detach(Boolean inMayEmpty = false); /* used by blobs in DB4D */
	virtual CompareResult		CompareTo (const VValueSingle& inValue, Boolean inDiacritic = false) const ;

	virtual	VError				Flush( void* inDataPtr, void* inContext) const;
	virtual void				SetReservedBlobNumber(sLONG inBlobNum);
	virtual void*				WriteToPtr(void* pData,Boolean pRefOnly, VSize inMax = 0 ) const;
	
	virtual VSize				GetFullSpaceOnDisk() const;
	virtual VSize				GetSpace (VSize inMax) const;
	virtual void				FromValue( const VValueSingle& inValue);
	virtual void				GetValue( VValueSingle& outValue) const;

	virtual void				SetOutsidePath(const VString& inPosixPath, bool inIsRelative = false);
	virtual void				GetOutsidePath(VString& outPosixPath, bool* outIsRelative = NULL);
	virtual void				SetOutsideSuffixe(const VString& inExtention);
	virtual bool				IsOutsidePath();
	virtual VError				ReloadFromOutsidePath();
	virtual void				SetHintRecNum(sLONG TableNum, sLONG FieldNum, sLONG inHint);

	virtual VError				ReadFromStream (VStream* ioStream, sLONG inParam = 0) ;
	virtual VError				WriteToStream (VStream* ioStream, sLONG inParam = 0) const;
	
	virtual Boolean				EqualToSameKind( const VValue* inValue, Boolean inDiacritical = false) const;
	
/*************************************************************************************************************/
	
	void SetExtraData(const VBlob& inBlob);
	VSize GetExtraData(VBlob& outBlob)const;
	VSize GetExtraDataSize()const;
	void ClearExtraData();
	
	void FromVFile(VFile& inFile,bool inAllowUnknownData);
	void Draw(PortRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
	void Draw(VGraphicContext* inGraphicContext,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
	void Print(PortRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
	void Print(VGraphicContext* inGraphicContext,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
	#if VERSIONWIN
	void Draw(Gdiplus::Graphics* inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
	#else
	void Draw(CGContextRef inPortRef,const VRect& r,class VPictureDrawSettings* inSet=NULL)const;
	#endif
	void RemovePastablePicData();
	
	void* GetPicHandle(bool inAppendPicEnd=false)const;
	VSize GetDataSize(_VPictureAccumulator* inRecorder=0) const;	
	
	void DumpPictureInfo(VString& outDump,sLONG level) const;
	
	void FromVPictureData(const VPictureData* inData);
	
	void FromVPicture_Copy(const VPicture& inData,bool inKeepSettings=false);
	void FromVPicture_Retain(const VPicture& inData,bool inKeepSettings=false);

	void FromVPicture_Copy(const VPicture* inData,bool inKeepSettings=false);
	void FromVPicture_Retain(const VPicture* inData,bool inKeepSettings=false);
	
	void FromMacHandle(void* inMacHandle,bool trydecode=false,bool inWithPicEnd=false);
	void FromPtr(void* inPtr,VSize inSize);
	
	void FromMacPicHandle(void* inMacPict,bool inWithPicEnd=false);
	void FromMacPictPtr(void* inMacPtr,VSize inSize,bool inWithPicEnd=false);
	
	void CalcBestPictData(){_SelectDefault();}
	
	bool AppendPictData(const VPictureData* inData,bool inReplaceIfExist=false,bool inRecalcDefaultPictData=false);
	const VPictureData* RetainNthPictData(VIndex inIndex) const;
	const VPictureData* RetainPictDataByMimeType(const VString& inMime) const;
	const VPictureData* RetainPictDataByExtension(const VString& inExt) const;
	const VPictureData* RetainPictDataByIdentifier(const VString& inExt) const;
	VIndex CountPictureData()const {return (VIndex)fPictDataMap.size();}
	void SelectPictureDataByExtension(const VString& inExt,bool inForDisplay=true,bool inForPrinting=false)const;
	void SelectPictureDataByMimeType(const VString& inExt,bool inForDisplay=true,bool inForPrinting=false)const;
	
	const VPictureData* RetainPictDataForDisplay() const;
	const VPictureData* RetainPictDataForPrinting() const;
	
	void GetMimeList(VString& outMimes) const;
	
	const VPictureData* _GetBestPictDataForDisplay()const
	{
		return fBestForDisplay;
	}
	const VPictureData* _GetBestPictDataForPrinting()const
	{
		return fBestForPrinting;
	}
	VPicture* CreateGrayPicture()const;
	
	virtual	void					Clear();
	
	sLONG GetWidth(bool inIgnoreTransform=false) const;
	sLONG GetHeight(bool inIgnoreTransform=false)const;
	VPoint GetWidthHeight(bool inIgnoreTransform=false)const;
	VRect GetCoords(bool inIgnoreTransform=false)const;
	
	bool IsSplited(sWORD* ioLine=0,sWORD* ioCol=0)const;
	void SetSplitInfo(bool inSplit,sWORD inLine,sWORD inCol);
	
	bool IsSamePictData(const VPicture* inPicture)const;
	
	sLONG _GetMaxPictDataRefcount();

	bool IsPictEmpty() const;

	const VValueBag* RetainMetaDatas(const VString* inPictureIdentifier=NULL);
	
	/*********************** IWAtchable *************************/
	// for debug
	void dbg_SetCVCache(bool b){fInCVCache=b;}
	bool dbg_GetCVCache(){return fInCVCache;}

	virtual sLONG IW_CountProperties();
	virtual void IW_GetPropName(sLONG inPropID,VValueSingle& outName);
	virtual void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue);
	virtual void IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild);
	virtual short IW_GetIconID(sLONG inPropID);
	//virtual bool IW_UseCustomMenu(sLONG inPropID,IWatchableMenuInfoArray &outmenuinfo);
	//virtual void IW_HandleCustomMenu(sLONG inID);
	virtual void IW_SetPropValue(sLONG inPropID,VValueSingle& inValue);
	/*********************** ******** *************************/

	sWORD GetPicEnd_PosX()const{return fDrawingSettings.GetPicEnd_PosX();}
	sWORD GetPicEnd_PosY()const{return fDrawingSettings.GetPicEnd_PosY();}
	void SetPicEndPos(sWORD inX,sWORD inY){fDrawingSettings.SetPicEndPos(inX,inY);}
	void  OffsetPicEndPos(sWORD inOffX,sWORD inOffY){fDrawingSettings.OffsetPicEndPos(inOffX,inOffY);}
	
	sWORD GetPicEnd_TransfertMode()const{return fDrawingSettings.GetPicEnd_TransfertMode();}
	void SetPicEnd_TransfertMode(sWORD inMode){return fDrawingSettings.SetPicEnd_TransfertMode(inMode);}
	
	static bool IsVPictureData(uCHAR* inBuff,VSize inBuffSize);
	VError SaveToBlob(VBlob* inBlob,bool inEvenIfEmpty=false,_VPictureAccumulator* inRecorder=0)const;
	VError ReadFromBlob(VBlob& inBlob);

	VError	GetPictureForRTF(VString& outRTFKind,VBlob& outBlob);

	VPicture* BuildThumbnail(sLONG inWidth,sLONG inHeight,PictureMosaic inMode,bool inNoAlpha=false,const VColor& inColor=VColor(255,255,255,255))const;

	void SetEmpty(bool inKeepSettings=false);
	
	static VFile* SelectPictureFile();
	
	VPictureDrawSettings_Base& GetSettings(){return fDrawingSettings;}
	const VPictureDrawSettings_Base& GetSettings()const{return fDrawingSettings;}
	
	void SetDrawingSettings(const VPictureDrawSettings_Base& inSet)
	{
		fDrawingSettings.FromVPictureDrawSettings(&inSet);
	}

	static VPicture*	EncapsulateBlob(const VBlob* inBlob,bool inOwnData);
	VError				ExtractBlob(VBlob& outBlob);
	
	// extract IPTC keywords
	void				GetKeywords( VString& outKeywords);
	
/************/
// transform accessor	
	
	//VAffineTransform& GetPictureMatrix(){return fDrawingSettings.GetPictureMatrix();}
	const VAffineTransform& GetPictureMatrix()const{return fDrawingSettings.GetPictureMatrix();}
	void SetPictureMatrix(const VAffineTransform& inMat)
	{
		fDrawingSettings.SetPictureMatrix(inMat);
		_SetValueSourceDirty();
	}
	void ResetMatrix()
	{
		fDrawingSettings.GetPictureMatrix().MakeIdentity();
		_SetValueSourceDirty();
	}
	void MultiplyMatrix(const VAffineTransform& inMat,VAffineTransform::MatrixOrder inOrder=VAffineTransform::MatrixOrderPrepend)
	{
		fDrawingSettings.GetPictureMatrix().Multiply(inMat,inOrder);
		_SetValueSourceDirty();
	}
	void Translate(GReal sX,GReal sY,VAffineTransform::MatrixOrder inOrder=VAffineTransform::MatrixOrderPrepend)
	{
		fDrawingSettings.GetPictureMatrix().Translate(sX,sY,inOrder);
		_SetValueSourceDirty();
	}
	void Scale(GReal sX,GReal sY,VAffineTransform::MatrixOrder inOrder=VAffineTransform::MatrixOrderPrepend)
	{
		fDrawingSettings.GetPictureMatrix().Scale(sX,sY,inOrder);
		_SetValueSourceDirty();
	}
	bool IsKeyWordInMetaData(const VString& keyword, VIntlMgr* inIntlMgr);
	
	void			SetSourceFileName(const VString& inFileName);
	const VString&	GetSourceFileName() const;
	
protected:

	virtual void					DoNullChanged();

	       
private:
	
	void _Init(bool inSetNULL=false);

	void	_AttachVValueSource(VBlob* inBlob);
	void	_DetachVValueSource(bool inSetNull=true);
	VError	_LoadFromVValueSource();
	VError	_UpdateVValueSource()const;
	void	_SetValueSourceDirty(bool inSet=true)const	{fVValueSourceDirty=inSet;};
	bool	_IsValueSourceDirty()const					{return fVValueSourceDirty;};
	

	void _UpdateNullState();
	
	
	void _ReadHeaderV1(xVPictureBlockHeaderV1& inHeader,bool inSwap);
	//void _ReadHeaderV2(xVPictureBlockHeaderV2& inHeader,bool inSwap);
	bool _Is4DMetaSign(const VString& instr);

	void _ReadExtraInfoBag(VValueBag& inBag);
	VError _SaveExtraInfoBag(VPtrStream& outStream) const;
	VError _SaveExtensions(VPtrStream& outStream) const;
	bool _FindPictData(const VPictureData* inData) const;
	void _ResetExtraInfo();
	void _CopyExtraInfo(const VPicture& inPict);
	void _ResetTransferMode();
	void _ReleaseExtraData();
	
	void _CopyExtraData_Copy(const VPicture& inPict);
	void _CopyExtraData_Retain(const VPicture& inPict);
	
	VSize _CountPictData()const {return fPictDataMap.size();}
	VSize _CountSaveable()const;
	VError _FromStream(VStream* inStream,_VPictureAccumulator* inRecorder=0);
	
	VError _BuildData(xVPictureBlock* inData,sLONG version,std::vector<VString>& inExtList, sLONG nbPict,VStream& inStream,VIndex inStartpos,_VPictureAccumulator* inRecorder=0);
	
	VError _Save(VBlob* inBlob,bool inEvenIfEmpty=false,_VPictureAccumulator* inRecorder=0) const;
	
	void _FullyClone(const VPicture& inData,bool ForAPush);
	
	VSize _GetDataSize(_VPictureAccumulator* inRecorder=0) const;
	void _ClearMap();
	void _CopyMap_Retain(const _VPictDataMap& inSrcMap);
	void _CopyMap_Copy(const _VPictDataMap& inSrcMap);
	bool _CheckBlobRefCount() const;
	bool _FindMetaPicture() const;
	
	void _SelectDefault();
	const VPictureData* _GetDefault()const;
	
	const VPictureData* _GetBestForDisplay();
	const VPictureData* _GetBestForPrinting();
	
	const VPictureData* _GetPictDataByIdentifier(const VString& inID) const;
	const VPictureData* _GetPictDataByMimeType(const VString& inMime)const;
	const VPictureData* _GetPictDataByExtension(const VString& inExt)const;
	const VPictureData* _GetNthPictData(VIndex inIndex) const;
	
	bool								fIs4DBlob;
	bool								fInCVCache;
	
	mutable XBOX::VBlob*				fVValueSource;
	bool								fVValueSourceLoaded;
	mutable bool						fVValueSourceDirty;								
	
	mutable VPictureDataProvider*		fExtraData;
	
	const mutable VPictureData*			fBestForDisplay;
	const mutable VPictureData*			fBestForPrinting;
	
	XBOX::VString						fSourceFileName;
	XBOX::VString						fOutsidePath;
	
	VPictureDrawSettings_Base			fDrawingSettings;
	
	_VPictDataMap						fPictDataMap;
	xVPictureMapWatch					fMapWatch; // for debug

};

END_TOOLBOX_NAMESPACE

#endif
