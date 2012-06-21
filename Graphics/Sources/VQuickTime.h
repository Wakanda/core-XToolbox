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
#ifndef __VQuickTime__
#define __VQuickTime__



#if _WIN32 || __GNUC__
#pragma pack(push,8)
#else
#pragma options align = power 
#endif

#if USE_QUICKTIME

// PLEASE READ ME:
// On Mac, there's no namespace. Simply add Quicktime framework.
// On Windows, you need to include VQuicktimeSDK.h somewhere in your code.
//	All QT functions and types are set in 'QTIME' namespace.
//	
// For cross-platform code, please use XXXRef types (see bellow).


// Class types
#if VERSIONMAC

#define QTIME
#define USING_QTIME_NAMESPACE
#define BEGIN_QTIME_NAMESPACE
#define END_QTIME_NAMESPACE

#include <QuickTime/QuickTime.h>

typedef ComponentRecord					*QTComponentRef;
typedef Handle							QTHandleRef;
typedef PicHandle						QTPictureRef;
typedef QTAtom							QTAtomRef;
typedef QTAtomContainer					QTAtomContainerRef;
typedef ComponentInstance				QTInstanceRef;
typedef SCSpatialSettings				QTSpatialSettingsRef;
typedef ComponentDescription			QTComponentDescriptionRef;
typedef GraphicsExportComponent			QTExportComponentRef;
typedef GraphicsImportComponent			QTImportComponentRef;
typedef ImageDescriptionHandle			QTImageDescriptionHandle;
typedef PointerDataRefRecord			QTPointerDataRefRecord;
typedef PointerDataRefRecord			**QTPointerDataRef;
typedef OsType							QTCodecType;
typedef QTIME::ComponentRecord			*QTCodecComponentRef;
typedef unsigned long					QTCodecQ;
typedef SCParams						QTSCParamsRef;

#else

#define QTIME qtime
#define USING_QTIME_NAMESPACE using namespace qtime;
#define BEGIN_QTIME_NAMESPACE namespace qtime {
#define END_QTIME_NAMESPACE }

BEGIN_QTIME_NAMESPACE
	struct ComponentRecord;
	struct ComponentDescription;
	struct ComponentInstanceRecord;
	struct Picture;
	struct FSSpec;
	struct SCSpatialSettings;
	struct ImageDescription;
	struct PointerDataRefRecord;
	struct SCParams;
END_QTIME_NAMESPACE

typedef QTIME::ComponentRecord				*QTComponentRef;
typedef char								**QTHandleRef;
typedef QTIME::Picture						**QTPictureRef;
typedef long								QTAtomRef;
typedef char								**QTAtomContainerRef;
typedef QTIME::ComponentInstanceRecord		*QTInstanceRef;
typedef QTIME::SCSpatialSettings			QTSpatialSettingsRef;
typedef QTIME::ComponentDescription			QTComponentDescriptionRef;
typedef QTIME::ComponentInstanceRecord		*QTExportComponentRef;
typedef QTIME::ComponentInstanceRecord		*QTImportComponentRef;
typedef QTIME::ImageDescription				**QTImageDescriptionHandle;
typedef QTIME::PointerDataRefRecord			QTPointerDataRefRecord;
typedef QTIME::PointerDataRefRecord			**QTPointerDataRef;
typedef OsType								QTCodecType;
typedef QTIME::ComponentRecord				*QTCodecComponentRef;
typedef unsigned long						QTCodecQ;
typedef QTIME::SCParams						QTSCParamsRef;
#endif


BEGIN_TOOLBOX_NAMESPACE

// Needed declarations

class V4DPictureData;
class VPictureData;
class VQTSpatialSettings;


// Class constants
typedef enum CompressionQuality
{
	CQ_MinQuality		= 0x00000000,
	CQ_LowQuality		= 0x00000100,
	CQ_NormalQuality	= 0x00000200,
	CQ_HighQuality		= 0x00000300,
	CQ_MaxQuality		= 0x000003FF,
	CQ_LosslessQuality	= 0x00000400
} CompressionQuality;

const sLONG	MAX_MOVIES_COUNT = 10;

class XTOOLBOX_API VQuicktimeInfo :public VObject IWATCHABLE_DEBUG(VQuicktimeInfo)
{
	public:
	VQuicktimeInfo();
	virtual ~VQuicktimeInfo();
	sLONG IW_CountProperties();
	virtual void IW_GetPropName(sLONG inPropID,VValueSingle& outName);
	virtual void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue);
	virtual void IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild);
};
 
class XTOOLBOX_API VQT_IEComponentManager : public VObject IWATCHABLE_DEBUG(VQT_IEComponentManager)
{
	public:
	VQT_IEComponentManager();
	virtual ~VQT_IEComponentManager();
	
	void Init(sLONG inCOmponentKind);
	class VQT_IEComponentInfo* FindComponementInfoByName(const VString& inName);
	
	sLONG IW_CountProperties();
	virtual void IW_GetPropName(sLONG inPropID,VValueSingle& outName);
	virtual void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue);
	virtual void IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild);
	
	sLONG GetCount(){return (sLONG)fComponentList.size();};
	void GetNthComponentName(sLONG index,VString & outName);
	VQT_IEComponentInfo* GetNthComponent(sLONG index);
	sLONG GetKing(){return fKind;};
	
	class VQT_IEComponentInfo*	FindComponentByMimeType(const VString& inMime);
	class VQT_IEComponentInfo*	FindComponentByExtension(const VString& inExt);
	class VQT_IEComponentInfo*	FindComponentByMacType(const OsType);

	private:
	
	uBOOL fInited;
	sLONG fKind;
	std::vector<class VQT_IEComponentInfo*> fComponentList;
};

class XTOOLBOX_API xOsTypeVectorWatch : public VObject IWATCHABLE_DEBUG(xOsTypeVectorWatch)
{
	public:
	xOsTypeVectorWatch(std::vector<OsType>* inVector,const VString& inDisplayName)
	{
		fList=inVector;
		fName=inDisplayName;
	}
	virtual ~xOsTypeVectorWatch()
	{
	}
	sLONG IW_CountProperties()
	{
		if(fList)
			return (sLONG)fList->size()+1;
		else
			return 0;
	}
	void IW_GetPropName(sLONG inPropID,VValueSingle& outName)
	{
		if(inPropID==0)
			outName.FromString(fName);
		else
		{
			if(fList)
			{
				inPropID--;
				if(inPropID>=0 && inPropID<(sLONG)fList->size())
				{
					IW_SetOSTypeVal((*fList)[inPropID],outName);
				}
			}
		}
			
	}
	void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue)
	{
		if(inPropID==0 && fList)
			outValue.FromLong((sLONG)fList->size());
	}
	private:
	std::vector<OsType>* fList;
	VString fName;
};

class XTOOLBOX_API xVStringVectorWatch : public VObject IWATCHABLE_DEBUG(xVStringVectorWatch)
{
	public:
	xVStringVectorWatch(std::vector<VString>* inVector,const VString& inDisplayName)
	{
		fList=inVector;
		fName=inDisplayName;
	}
	virtual ~xVStringVectorWatch()
	{
	}
	sLONG IW_CountProperties()
	{
		if(fList)
			return (sLONG)fList->size()+1;
		else
			return 0;
	}
	void IW_GetPropName(sLONG inPropID,VValueSingle& outName)
	{
		if(inPropID==0)
			outName.FromString(fName);
		else
		{
			if(fList)
			{
				inPropID--;
				if(inPropID>=0 && inPropID<(sLONG)fList->size())
				{
					outName.FromString((*fList)[inPropID]);
				}
			}
		}
			
	}
	void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue)
	{
		
		if(inPropID==0 && fList)
		{
			outValue.FromLong((sLONG)fList->size());
		}
	}
	private:
	std::vector<VString>* fList;
	VString fName;
};
class XTOOLBOX_API VQT_IEComponentInfo : public VObject IWATCHABLE_DEBUG(VQT_IEComponentInfo)
{
public:
			VQT_IEComponentInfo (QTComponentRef inComponentRef);
			VQT_IEComponentInfo (OsType inType, OsType inSubType, const VString& inName, const VString& inInfo, OsType inDefaultMacType, OsType inDefaultMacCreator, OsType inDefaultFileExtension);
			VQT_IEComponentInfo (const VQT_IEComponentInfo& inRef);
	virtual	~VQT_IEComponentInfo ();
	
	void UpdateComponentInfo(VQT_IEComponentInfo& inComp);
	
	OsType	GetComponentType () const { return fComponentType; };
	OsType	GetComponentSubType () const { return fComponentSubType; };
	void	GetComponentName (VString& outName) const { outName = fComponentName; };
	void	GetComponentInfo (VString& outInfo) const { outInfo = fComponentInfo; };
	uBOOL	FindMimeType(const VString& inMime);
	uBOOL	FindExtension(const VString& inExt);
	uBOOL	FindMacType(const OsType);
	
	sLONG CountMimeType(){return (sLONG)fMimeTypeList.size();};
	sLONG CountExtension(){return (sLONG)fExtensionList.size();};
	sLONG CountMacType(){return (sLONG)fMacTypeList.size();};
	
	void GetNthMimeType(sLONG inIndex,VString& outMime){outMime=fMimeTypeList[inIndex];};
	void GetNthExtension(sLONG inIndex,VString& outExtension){outExtension=fExtensionList[inIndex];};
	void GetNthMacType(sLONG inIndex,OsType& outType){outType=fMacTypeList[inIndex];};
	
	sLONG GetComponenFlags(){return fComponentFlags;}
	bool TestComponentFlags(sLONG inFlag){ return (fComponentFlags & inFlag) == inFlag;}
	
	sLONG IW_CountProperties();
	virtual void IW_GetPropName(sLONG inPropID,VValueSingle& outName);
	virtual void IW_GetPropValue(sLONG inPropID,VValueSingle& outValue);
	virtual void IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild);
	
	void xDump();
protected:
	OsType	fComponentType;
	OsType	fComponentSubType;

	VString	fComponentName;
	VString	fComponentInfo;
	
	sLONG	fComponentFlags;
	
	std::vector<QTIME::ComponentDescription> fDescList;
	std::vector<VString> fMimeTypeList;
	std::vector<VString> fExtensionList;
	std::vector<OsType> fMacTypeList;
	
	xOsTypeVectorWatch fMacTypeListWatch;
	xVStringVectorWatch fExtensionListWatch;
	xVStringVectorWatch fMimeTypeListWatch;
private:
	void xAppendMacType(const OsType);
	void xAppendExtensions(const VString& inExtensions);
	void xAppendMimeType(const VString& inMimes);
	void xDumpFlags(uLONG inFlags,uLONG inFlagsMask);
	void xDumpFlag(uLONG inFlags,uLONG inFlagsMask,const VString& FlagName,uLONG FlagValue, uBOOL OnlyIfTrue=true);
	
};


class XTOOLBOX_API VQuicktime {
public:
	// Initialisation
	static Boolean	Init ();
	static void		DeInit ();
	static Boolean	HasQuicktime () { assert(sQuickTimeVersion > -1); return (sQuickTimeVersion > 0); };
	static sLONG	GetVersion () { assert(sQuickTimeVersion > -1); return sQuickTimeVersion; };

	// Progress dialogs
	static void		SetCompressionProgressMessage (const VString& inMessage) { delete sCompressMessage; sCompressMessage = (VString*)inMessage.Clone(); };
	static void		SetDeCompressionProgressMessage (const VString& inMessage) { delete sDeCompressMessage; sDeCompressMessage = (VString*)inMessage.Clone(); };
	static void		SetThumbnailProgressMessage (const VString& inMessage) { delete sThumbnailMessage; sThumbnailMessage = (VString*)inMessage.Clone(); };
	static void		SetConvertToJpegProgressMessage (const VString& inMessage) { delete sConvertToJpegMessage; sConvertToJpegMessage = (VString*)inMessage.Clone(); };
	
	// Image compression
	static VError	RunCompressionDialog (VQTSpatialSettings& ioSettings, const VPictureData* inPreview);
	
	static VError	CompressPicture (const V4DPictureData& inSource, V4DPictureData& outDest, const QTSpatialSettingsRef& inSettings, Boolean inShowProgress);
	static VError	CompressToQTimeData (const VPictureData& inSource, VPictureData** outDest, VQTSpatialSettings& inSettings, bool inShowProgress);
	static VError	CompressFile (const VFile& inSource, VFile& outDest, const QTSpatialSettingsRef& inSettings, Boolean inShowProgress);
	static VError	CompressFileToJpeg (const VFile& inSource, VFile& outDest, CompressionQuality inQuality, Boolean inShowProgress);

	// Thumbails
	static V4DPictureData*	GetPictureThumbnail (const V4DPictureData& inSource, Boolean inShowProgress);
	static VError	GetPictureThumbnail (const V4DPictureData& inSource, V4DPictureData& outData, Boolean inShowProgress);
	static VError	GetFileThumbnail (const VFile& inSource, V4DPictureData& outData, Boolean inShowProgress);
	static VError	SetFilePreview (const VFile& inSource, const V4DPictureData& inData);
	
	// Memory convertion
	static QTHandleRef	NewQTHandle (sLONG inSize);
	static void			DisposeQTHandle (QTHandleRef ioHandle);
	static VError		SetQTHandleSize (QTHandleRef ioHandle, sLONG inSize);
	static sLONG		GetQTHandleSize (const QTHandleRef inHandle);
	static char*		LockQTHandle (QTHandleRef ioHandle);
	static void			UnlockQTHandle (QTHandleRef ioHandle);
	static VError		PtrToQTHand(const void *srcPtr,QTHandleRef *dstHndl,sLONG size);                    

	static QTHandleRef	QTHandleToQTHandle (const QTHandleRef inHandle);
	static VHandle		QTHandleToVHandle (const QTHandleRef inHandle, VHandle outDest = NULL);
	static QTHandleRef	VHandleToQTHandle (const VHandle inHandle, QTHandleRef outDest = NULL);
	static QTHandleRef	VHandleToPicHandle( const VHandle inHandle);

#if USE_MAC_HANDLES
	static Handle		QTHandleToMacHandle (const QTHandleRef inHandle, Handle outDest = NULL);
	static QTHandleRef	MacHandleToQTHandle (const Handle inHandle, QTHandleRef outDest = NULL);
#endif
	
	// Picture convertion
	static QTPictureRef	NewEmptyQTPicture ();
	static void			KillQTPicture (QTPictureRef ioPicture);

#if USE_MAC_PICTS
	static PicHandle	QTPictureToMacPicture (const QTPictureRef inQTPicture, PicHandle outMacData = NULL);
	static QTPictureRef	MacPictureToQTPicture (const PicHandle inMacPicture);

	static PicHandle	MacPictureFromFile (const VFile& inFile);
	static VError		MacPictureToFile (VFile*& ioFile, const PicHandle inPicture, OsType inFormat, Boolean inShowSettings, Boolean inShowPutFile);
#endif

	static VError GraphicsImportGetImageDescription(QTImportComponentRef inComp, QTImageDescriptionHandle *desc );

	// Component management
	static QTInstanceRef	OpenComponent (QTComponentRef inComponentRef);
	static VError	CloseComponent (QTInstanceRef inInstance);
	
	static VError	OpenDefaultComponent (OsType inType, OsType inSubType, QTInstanceRef* outInstance);
	static VError	GetComponentInfo (QTComponentRef inComponent, QTComponentDescriptionRef* outDescription, QTHandleRef ioName, QTHandleRef ioInfo, QTHandleRef ioIcon);
	static QTComponentRef	FindNextComponent (QTComponentRef inComponent, QTComponentDescriptionRef* outDescription);

	static VError	GraphicsExportGetDefaultFileTypeAndCreator (QTExportComponentRef inComponent, OsType& outFileType, OsType& outFileCreator);
	static VError	GraphicsExportGetDefaultFileNameExtension (QTExportComponentRef  inComponent, OsType& outFileNameExtension);
	static VError	GraphicsImportGetExportImageTypeList (QTImportComponentRef inComponent, void* inAtomContainerPtr);
	
	static short	CountChildrenOfType (QTAtomContainerRef inContainer, QTAtomRef inParentAtom, OsType inType);
	static QTAtomRef	FindChildByIndex (QTAtomContainerRef inContainer, QTAtomRef inParentAtom, OsType inType, sWORD inIndex, sLONG* outAtomID = NULL);
	static VError	CopyAtomDataToPtr (QTAtomContainerRef inContainer, QTAtomRef inAtom, Boolean inSizeOrLessOK, sLONG inMaxSize, void* outData, sLONG* outActualSize = NULL);
	
	
	static VError GetGraphicsImporterForDataRef(QTHandleRef dataRef,OsType dataRefType,QTInstanceRef *gi);
	static VError GetGraphicsImporterForFile(VFile& inFile,QTImportComponentRef *gi);
	
	static VError GraphicsImportGetAsPicture(QTImportComponentRef  ci,QTPictureRef *picture);
	
	static VError GraphicsImportValidate(QTImportComponentRef ci,unsigned char* valid);
	static VError GraphicsImportSetDataFile(VFile& inFile,QTImportComponentRef gi);
	static VError GraphicsImportSetDataHandle(QTImportComponentRef  ci,QTHandleRef h);
	
	static VError GraphicsImportSetDataReference(QTImportComponentRef gi,QTHandleRef dataRef,OsType dataRefType);

	// Utilities
	static VFile*	StandardGetFilePreview ();
	static VArrayList*	GetIEComponentList (uLONG inIEKind);

#if USE_MAC_PICTS && USE_MAC_HANDLES
	static Handle	ExportMacPictureToMacHandle (PicHandle inSource, OsType inExporter);
#endif

	static void*	_SetCompressionDialogPreview (QTInstanceRef inInstance);
	static void		_ReleaseCompressionDialogPreview (void* inQTPicture);

#if VERSIONWIN
	static void		WIN_VFileToFSSpec (const VFile& inFile, qtime::FSSpec& outSpec);
	static VFile*	WIN_FSSpecToVFile (const qtime::FSSpec& inSpec);
#endif

	static	VError	LoadSCSpatialSettingsFromBag( const VValueBag& inBag, QTSpatialSettingsRef *outSettings);
	static	VError	SaveSCSpatialSettingsToBag( VValueBag& ioBag, const QTSpatialSettingsRef& inSettings, VString& outKind);


	static sLONG CountImporter()
	{
		if(!HasQuicktime())
			return 0;
		else
			return sImportComponents->GetCount();
	}
	static sLONG CountExporter()
	{
		if(!HasQuicktime())
			return 0;
		else
			return sExportComponents->GetCount();
	}
	static VQT_IEComponentManager* GetImportComponentManager(){return sImportComponents;};
	static VQT_IEComponentManager* GetExportComponentManager(){return sExportComponents;};

protected:
	static sLONG	sQuickTimeVersion;
	static OsType	sCompressor;
	static Real		sProgressPercentage;
	static VString*	sCompressMessage;
	static VString*	sDeCompressMessage;
	static VString*	sThumbnailMessage;
	static VString*	sConvertToJpegMessage;
	
#if USE_OFFSCREEN_DRAWPICTURE
	static GWorldPtr	sOffscreenGWorld;
	static HDC			sOffscreenHDC;
	static HBITMAP		sOffscreenHBITMAP;
	static PALETTE		sOffscreenOldPALETTE;
	static HBITMAP		sOffscreenOldHBITMAP;
#endif

#if VERSIONWIN
	static sLONG	sMovies[MAX_MOVIES_COUNT];
#endif

	static VError	_CompressFile (const VFile& inSource, VFile& ioDest, const QTSpatialSettingsRef& inSettings, Boolean inShowProgress, Boolean inToJpeg);

#if VERSIONMAC
	static pascal OSErr	_ProgressProc (sWORD inAction, sLONG inCompleteness, sLONG inRefcon);
	static pascal void	_DrawProgressBar (DialogPtr inDialog, sWORD inItem);
#elif VERSIONWIN
	static sWORD __cdecl	_ProgressProc (sWORD inAction, sLONG inCompleteness, sLONG inRefcon);
#endif
	
	
	
	static void _InitIEComponentList(std::vector<VQT_IEComponentInfo> &inList,uLONG pIEKind);
	static VQT_IEComponentManager* sImportComponents;
	static VQT_IEComponentManager* sExportComponents;
	static VQuicktimeInfo* sQTInfo;
};



END_TOOLBOX_NAMESPACE

#else // USE_QUICKTIME

#if VERSIONMAC

#define QTIME
#define USING_QTIME_NAMESPACE
#define BEGIN_QTIME_NAMESPACE
#define END_QTIME_NAMESPACE

#else

#define QTIME qtime
#define USING_QTIME_NAMESPACE using namespace qtime;
#define BEGIN_QTIME_NAMESPACE namespace qtime {
#define END_QTIME_NAMESPACE }

#endif

typedef OsType								QTCodecType;
typedef unsigned long						QTCodecQ;

BEGIN_QTIME_NAMESPACE
	typedef struct QTCodecComponent {
		long                data[1];
	}QTCodecComponent;		
	typedef struct QTSpatialSettings
	{
			QTCodecType					codecType;
			QTCodecComponent			*codec;
			short						depth;
			QTCodecQ					spatialQuality;
	}QTSpatialSettings;
	
	typedef struct QTSCParams
	{
		long				flags;
		QTCodecType			theCodecType;
		QTCodecComponent	*theCodec;
		QTCodecQ			spatialQuality;
		QTCodecQ			temporalQuality;
		short				depth;
		long				frameRate;
		long				keyFrameRate;
		long				reserved1;
		long				reserved2;
} QTSCParams;
	
END_QTIME_NAMESPACE	

typedef QTIME::QTSCParams					QTSCParamsRef;
typedef QTIME::QTSpatialSettings			QTSpatialSettingsRef;
typedef QTIME::QTCodecComponent				*QTCodecComponentRef;	

	
BEGIN_TOOLBOX_NAMESPACE
	class XTOOLBOX_API VQuicktime 
	{
	public:
		// Initialisation
		static Boolean	Init (){return false;};
		static void		DeInit (){};
		static Boolean	HasQuicktime () { return false; }
		static sLONG	GetVersion () { return 0; }
		
		static	VError	LoadSCSpatialSettingsFromBag( const VValueBag& inBag, QTSpatialSettingsRef *outSettings);
		static	VError	SaveSCSpatialSettingsToBag( VValueBag& ioBag, const QTSpatialSettingsRef& inSettings, VString& outKind);
		
	};
END_TOOLBOX_NAMESPACE

#endif

BEGIN_TOOLBOX_NAMESPACE		
class XTOOLBOX_API VQTSpatialSettings :public VObject,public IBaggable
{
	public:
			
	// IBaggable interface
	virtual VError	LoadFromBag( const VValueBag& inBag);
	virtual VError	SaveToBag( VValueBag& ioBag, VString& outKind) const;
	/** ********* ********* **/
		
	VQTSpatialSettings(const QTSpatialSettingsRef* inSet=0);
	VQTSpatialSettings(QTCodecType inCodecType,QTCodecComponentRef inCodecComponent,sWORD depth,QTCodecQ inSpatialQuality);
	virtual ~VQTSpatialSettings();
	operator QTSpatialSettingsRef*();
			
	void SetCodecType(QTCodecType inCodecType);
	void SetCodecComponent(QTCodecComponentRef inCodecComponent);
	void SetDepth(sWORD inDepth);
	void SetSpatialQuality(QTCodecQ inCodecQ);
			
	QTCodecType				GetCodecType();
	QTCodecComponentRef		GetCodecComponent();
	sWORD					GetDepth();
	QTCodecQ				GetSpatialQuality();
	
	void FromSCParams(const QTSCParamsRef* inSet=0);
	static sLONG SizeOfSCParams();
	
	const QTSpatialSettingsRef& GetSpatialSettingsRef(){return *fSpatialSettings;}
			
private:
			
		QTSpatialSettingsRef* fSpatialSettings;
		
	};		

END_TOOLBOX_NAMESPACE		
		
#if _WIN32 || __GNUC__
#pragma pack( pop )
#else
#pragma options align = reset
#endif

#endif
