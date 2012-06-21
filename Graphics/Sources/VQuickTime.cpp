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
#include "VQuickTime.h"
#include "V4DPictureIncludeBase.h"

#if USE_QUICKTIME

#if VERSIONWIN
BEGIN_QTIME_NAMESPACE
	#include <QTML.h>
	#include <Gestalt.h>
	#include <Resources.h>
	#include <ImageCodec.h>
	#include <ImageCompression.h>
	#include <QuickTimeComponents.h>
END_QTIME_NAMESPACE
#endif

#endif

VError VQuicktime::LoadSCSpatialSettingsFromBag( const VValueBag& inBag, QTSpatialSettingsRef *outSettings)
{
	VError err;
	VString codec_type;
	sLONG depth;
	sLONG spatialQuality;
	
	if ( inBag.GetString( L"codec_type", codec_type)
		&& inBag.GetLong( "depth", depth)
		&& inBag.GetLong( "spatial_quality", spatialQuality) 
		&& (codec_type.GetLength() == 4)
		&& (depth >= -32768 && depth <= 32767)
		)
	{
		outSettings->codecType = codec_type.GetOsType();
		outSettings->depth = (short) depth;
		outSettings->spatialQuality = (QTCodecQ) spatialQuality;
		outSettings->codec = NULL;
		err = VE_OK;
	}
	else
	{
		::memset( outSettings, 0, sizeof( QTSpatialSettingsRef));
		err = VE_INVALID_PARAMETER;
	}
	return err;
}


VError VQuicktime::SaveSCSpatialSettingsToBag( VValueBag& ioBag, const QTSpatialSettingsRef& inSettings, VString& outKind)
{
	VString codec_type;
	codec_type.FromOsType( (OsType) inSettings.codecType);
	ioBag.SetString( L"codec_type", codec_type);
	
	ioBag.SetLong( L"depth", inSettings.depth);
	ioBag.SetLong( L"spatial_quality", inSettings.spatialQuality);
	
	outKind = L"qt_spatial_settings";
	
	return VE_OK;
}

VQTSpatialSettings::VQTSpatialSettings(const QTSpatialSettingsRef* inSet)
{
	fSpatialSettings=new QTSpatialSettingsRef;
	if(inSet)
		memcpy(fSpatialSettings,inSet,sizeof(QTSpatialSettingsRef));
	else
	{
		::memset(fSpatialSettings, 0, sizeof( QTSpatialSettingsRef));
	}
}
void VQTSpatialSettings::FromSCParams(const QTSCParamsRef* inSet)
{
	if(!fSpatialSettings)
		fSpatialSettings=new QTSpatialSettingsRef;
	if(inSet)
	{
		fSpatialSettings->codecType = inSet->theCodecType;
		fSpatialSettings->codec = inSet->theCodec;
		fSpatialSettings->depth = inSet->depth;
		fSpatialSettings->spatialQuality = inSet->spatialQuality;
	}
	else
	{
		::memset(fSpatialSettings, 0, sizeof( QTSpatialSettingsRef));
	}
}

sLONG VQTSpatialSettings::SizeOfSCParams(){return sizeof(QTSCParamsRef);}

VQTSpatialSettings::VQTSpatialSettings(QTCodecType inCodecType,QTCodecComponentRef inCodecComponent,sWORD depth,QTCodecQ inSpatialQuality)
{
	fSpatialSettings=new QTSpatialSettingsRef;
	fSpatialSettings->codecType=inCodecType;
    fSpatialSettings->codec=inCodecComponent;
    fSpatialSettings->depth=depth;
    fSpatialSettings->spatialQuality=inSpatialQuality;
	
}
VError	VQTSpatialSettings::LoadFromBag( const VValueBag& inBag)
{
	VError err;
	VString codec_type;
	sLONG depth;
	sLONG spatialQuality;
	
	if ( inBag.GetString( L"codec_type", codec_type)
		&& inBag.GetLong( "depth", depth)
		&& inBag.GetLong( "spatial_quality", spatialQuality) 
		&& (codec_type.GetLength() == 4)
		&& (depth >= -32768 && depth <= 32767)
		)
	{
		fSpatialSettings->codecType = codec_type.GetOsType();
		fSpatialSettings->depth = (short) depth;
		fSpatialSettings->spatialQuality = (QTCodecQ) spatialQuality;
		fSpatialSettings->codec = NULL;
		err = VE_OK;
	}
	else
	{
		::memset( fSpatialSettings, 0, sizeof( QTSpatialSettingsRef));
		err = VE_INVALID_PARAMETER;
	}
	return err;
}
VError	VQTSpatialSettings::SaveToBag( VValueBag& ioBag, VString& outKind) const
{
	VString codec_type;
	codec_type.FromOsType( (OsType) fSpatialSettings->codecType);
	ioBag.SetString( L"codec_type", codec_type);
	
	ioBag.SetLong( L"depth", fSpatialSettings->depth);
	ioBag.SetLong( L"spatial_quality", fSpatialSettings->spatialQuality);
	
	outKind = L"qt_spatial_settings";
	
	return VE_OK;
}
VQTSpatialSettings::~VQTSpatialSettings()
{
	if(fSpatialSettings)
		delete fSpatialSettings;
}
VQTSpatialSettings::operator QTSpatialSettingsRef*()
{
	return fSpatialSettings;
}

void VQTSpatialSettings::SetCodecType(QTCodecType inCodecType)
{
	fSpatialSettings->codecType = inCodecType;
}
void VQTSpatialSettings::SetCodecComponent(QTCodecComponentRef inCodecComponent)
{
	fSpatialSettings->codec = inCodecComponent;
}
void VQTSpatialSettings::SetDepth(sWORD inDepth)
{
	fSpatialSettings->depth = inDepth;
}
void VQTSpatialSettings::SetSpatialQuality(QTCodecQ inSpatialQuality)
{
	fSpatialSettings->spatialQuality = inSpatialQuality;
}

QTCodecType	VQTSpatialSettings::GetCodecType()
{
	return fSpatialSettings->codecType;
}
QTCodecComponentRef	VQTSpatialSettings::GetCodecComponent()
{
	return fSpatialSettings->codec;
}
sWORD VQTSpatialSettings::GetDepth()
{
	return fSpatialSettings->depth;
}
QTCodecQ VQTSpatialSettings::GetSpatialQuality()
{
	return  fSpatialSettings->spatialQuality;
}

#if USE_QUICKTIME

#if VERSIONMAC
#define QT_CharSet VTC_MacOSAnsi
#elif VERSIONWIN
#define QT_CharSet VTC_Win32Ansi
#endif

// Class constants
enum
{
	kCompressMessage	= 1,
	kDecompressMessage, 
	kThumbnailMessage, 
	kConvertToJpegMessage
};

const sLONG		kPICTURE_HEADER_SIZE = 512;


// Class statics
sLONG					VQuicktime::sQuickTimeVersion		= -1;
OsType					VQuicktime::sCompressor				= 0;
Real					VQuicktime::sProgressPercentage		= 0.0;
VString*				VQuicktime::sCompressMessage		= NULL;
VString*				VQuicktime::sDeCompressMessage		= NULL;
VString*				VQuicktime::sThumbnailMessage		= NULL;
VString*				VQuicktime::sConvertToJpegMessage	= NULL;
VQT_IEComponentManager* VQuicktime::sImportComponents		=NULL;
VQT_IEComponentManager* VQuicktime::sExportComponents		=NULL;
VQuicktimeInfo*			VQuicktime::sQTInfo					=NULL;

#if VERSIONWIN
sLONG		VQuicktime::sMovies[MAX_MOVIES_COUNT];
#endif

sLONG GetPictOpcodeSkipSize(VStream& pStream,uWORD opcode)
{
	VError	verror;
	sLONG	charcount;
	sBYTE	abyte;
	sWORD	aword;
	sLONG	along;
	QTIME::PixMap	pixmap;
	
	switch(opcode)
	{
		case 0x0017: // all this section undocumented (reserved for apple use)
		case 0x0018:
		case 0x0019:
		case 0x003D:
		case 0x003E:
		case 0x003F:
		case 0x004D:
		case 0x004E:
		case 0x004F:
		case 0x005D:
		case 0x005E:
		case 0x005F:
		case 0x00CF:
			return 0;
		case 0x0000: // nop
		case 0x001C:
		case 0x001E:
		case 0x0038:
		case 0x0039:
		case 0x003A:
		case 0x003B:
		case 0x003C:
		case 0x0048:
		case 0x0049:
		case 0x004A:
		case 0x004B:
		case 0x004C:
		case 0x0058:
		case 0x0059:
		case 0x005A:
		case 0x005B:
		case 0x005C:
		case 0x0078:
		case 0x0079:
		case 0x007A:
		case 0x007B:
		case 0x007C:
		case 0x007D:
		case 0x007E:
		case 0x007F:
		case 0x0088:
		case 0x0089:
		case 0x008A:
		case 0x008B:
		case 0x008C:
		case 0x008D:
		case 0x008E:
		case 0x008F:
			return 0;
		case 0x0001:
		case 0x0070:
		case 0x0071:
		case 0x0072:
		case 0x0073:
		case 0x0074:
		case 0x0075:
		case 0x0076:
		case 0x0077:
		case 0x0080:
		case 0x0081:
		case 0x0082:
		case 0x0083:
		case 0x0084:
		case 0x0085:
		case 0x0086:
		case 0x0087:
			verror = pStream.GetWord(aword);
			if(verror!=VE_OK)
				return 0xFFFFFFFF;
			return aword-sizeof(sWORD);
		case 0x0024:
		case 0x0025:
		case 0x0026:
		case 0x0027:
		case 0x002F:
		case 0x0092:
		case 0x0093:
		case 0x0094:
		case 0x0095:
		case 0x0096:
		case 0x0097:
		case 0x009C:
		case 0x009D:
		case 0x009E:
		case 0x009F:
			verror = pStream.GetWord(aword);
			if(verror!=VE_OK)
				return 0xFFFFFFFF;
			return aword;
		case 0x00FF: // pict end
			return 0xFFFFFFFF;
		case 0x0003:
		case 0x0004:
		case 0x0005:
		case 0x0008:
		case 0x000D:
		case 0x0011: // version
		case 0x0015:
		case 0x0016:
		case 0x0023:
		case 0x00A0:
		case 0x0100:
		case 0x01FF:
		case 0x02FF:
			return 2;
		case 0x0006:
		case 0x0007:
		case 0x000B:
		case 0x000C:
		case 0x000E:
		case 0x000F:
		case 0x0021:
		case 0x0068:
		case 0x0069:
		case 0x006A:
		case 0x006B:
		case 0x006C:
		case 0x006D:
		case 0x006E:
		case 0x006F:
		case 0x0200:
			return 4;
		case 0x001A:
		case 0x001B:
		case 0x001D:
		case 0x001F:
		case 0x0022:
			return 6;
		case 0x0002:
		case 0x0009:
		case 0x000A:
		case 0x0010:
		case 0x0020:
		case 0x002E:
		case 0x0030:
		case 0x0031:
		case 0x0032:
		case 0x0033:
		case 0x0034:
		case 0x0035:
		case 0x0036:
		case 0x0037:
		case 0x0040:
		case 0x0041:
		case 0x0042:
		case 0x0043:
		case 0x0044:
		case 0x0045:
		case 0x0046:
		case 0x0047:
		case 0x0050:
		case 0x0051:
		case 0x0052:
		case 0x0053:
		case 0x0054:
		case 0x0055:
		case 0x0056:
		case 0x0057:
			return 8;
		case 0x002D:
			return 10;
		case 0x0060:
		case 0x0061:
		case 0x0062:
		case 0x0063:
		case 0x0064:
		case 0x0067:
			return 12;
		case 0x0BFF:
			return 22;
		case 0x0C01:
			return 24;
		case 0x7F00:
		case 0x7FFF:
			return 254;
		case 0x0012: // Pixmap
		case 0x0013:
		case 0x0014:
			verror = pStream.GetWord(aword);
			if(verror!=VE_OK)
				return 0xFFFFFFFF;
			if(aword==2)
				return sizeof(QTIME::Pattern)+sizeof(QTIME::RGBColor);
			else
			{
				verror = pStream.SetPosByOffset(8); // skip Pattern
				if(verror!=VE_OK)
					return 0xFFFFFFFF;
				// read PixMap
				verror = pStream.GetLong((sLONG&)pixmap.baseAddr);
				charcount = 7;
				verror = pStream.GetWords(&pixmap.rowBytes,&charcount); // rowbytes + rect + pmversion + packtype
				charcount = 3;
				verror = pStream.GetLongs(&pixmap.packSize,&charcount); // packsize + hres + vres
				charcount = 4;
				verror = pStream.GetWords(&pixmap.pixelType,&charcount); // pixeltype +pixelsize + cmpcount + cmpsize
				charcount = 3;
				
			#if VERSIONMAC
				// Under Carbon planeBytes & pmReserved have changed (QT 3.0) - JT 09/02/00
				verror = pStream.GetLongs(&pixmap.pixelFormat, &charcount); 	// pixelFormat + pmTable + pmExt
			#else
				verror = pStream.GetLongs(&pixmap.pixelFormat,&charcount); // planebytes + pmTable + pmreserved
			#endif
			
				if(verror!=VE_OK)
					return 0xFFFFFFFF;
				verror = pStream.SetPosByOffset(sizeof(sLONG)+sizeof(sWORD)); // run to ctab size
				if(verror!=VE_OK)
					return 0xFFFFFFFF;
				verror = pStream.GetWord(aword);
				if(verror!=VE_OK)
					return 0xFFFFFFFF;
				verror = pStream.SetPosByOffset((aword+1)*sizeof(QTIME::ColorSpec)); // skip cspec array
				if(verror!=VE_OK)
					return 0xFFFFFFFF;
				pixmap.rowBytes &= 0x7FFF;
				if(pixmap.rowBytes<8)
					return pixmap.rowBytes*(pixmap.bounds.bottom-pixmap.bounds.top);
				else if(pixmap.rowBytes<=250)
				{
					charcount = 1;
					while(verror==VE_OK && pixmap.bounds.bottom>pixmap.bounds.top++)
					{
						verror = pStream.GetByte(abyte);
						verror = pStream.SetPosByOffset(abyte); // skip scan line
					}
				}
				else
				{
					charcount = 2;
					while(pixmap.bounds.bottom>pixmap.bounds.top++)
					{
						verror = pStream.GetWord(aword);
						verror = pStream.SetPosByOffset(aword); // skip scan line
					}
				}
				if(verror!=VE_OK)
					return 0xFFFFFFFF;
				else
					return 0;
			}
			break;
		case 0x0028:
		case 0x002B:
		case 0x0029:
		case 0x002A:
		case 0x002C:
			switch(opcode)
			{
			case 0x0028:
				verror = pStream.SetPosByOffset(4);
				break;
			case 0x0029:
			case 0x002A:
				verror = pStream.SetPosByOffset(1);// dh or dv
				break;
			case 0x002B:
				verror = pStream.SetPosByOffset(2);// dh + dv
				break;
			case 0x002C:
				verror = pStream.SetPosByOffset(4);
				break;
			}
			verror = pStream.GetByte(abyte);
			aword = abyte+1;
			return aword & 0x01FE; // to be even
		case 0x0090:
		case 0x0091:
			return 0xFFFFFFFF; // TBD
		case 0x0098:
			{
				sLONG offset, i;
				sWORD rowbyte, height, width, bitperpixel, colcount, packsize;
				
				rowbyte = (pStream.GetWord() & 0x7FFF); // get row byte
				offset = sizeof(sWORD)*2;
				pStream.SetPosByOffset(offset);
				height = pStream.GetWord(); // get picture height
				width = pStream.GetWord(); // get picture width
				offset = sizeof(sWORD)*2 + sizeof(sLONG)*2 + sizeof(sWORD)*3;
				pStream.SetPosByOffset(offset);
				bitperpixel = pStream.GetWord(); // get bits number per pixels
				offset = sizeof(sLONG)*5 + sizeof(sWORD);
				pStream.SetPosByOffset(offset);
				colcount = pStream.GetWord() + 1; // get color count
				offset = sizeof(sWORD)*4*colcount; // skip color table
				offset += sizeof(sWORD)*4; // skip source rect
				offset += sizeof(sWORD)*4; // skip dest rect
				offset += sizeof(sWORD); // skip transfert mode
				pStream.SetPosByOffset(offset);
				if(rowbyte < 8)
				{
					pStream.SetPosByOffset(height*rowbyte);
				}
				else
				{
					for( i = 0 ; i < height ; i++ )
					{
						if(rowbyte <= 250)
							packsize = (uBYTE)pStream.GetByte();
						else
							packsize = pStream.GetWord();
							
						pStream.SetPosByOffset(packsize);
					}
				}
				if(pStream.GetLastError() == VE_OK)
				{
					uLONG8 pos = pStream.GetPos();
					offset = (pos & 1) ? 1 : 0;
				}
				else
					offset = 0xFFFFFFFF;

				return (offset);
			}
		case 0x0099:
			return 0xFFFFFFFF; // TBD
		case 0x009A:
			{
				sLONG i, offset;
				sWORD rowbyte, height, width, packtype, packsize;
				
				offset = sizeof(sLONG);
				pStream.SetPosByOffset(offset); // skip base adr
				rowbyte = (pStream.GetWord() & 0x7FFF); // get rowbyte
				offset = sizeof(sWORD)*2;
				pStream.SetPosByOffset(offset);
				height = pStream.GetWord(); // get picture height
				width = pStream.GetWord(); // get picture width
				offset = sizeof(sWORD);
				pStream.SetPosByOffset(offset);
				packtype = pStream.GetWord();	// get packtype
												// packtype = 1 -> 16 bits per pixels (no packing)
												// packtype = 4 -> 24 or 32 bits per pixels
				offset = sizeof(sLONG)*3 + sizeof(sWORD)*4 + sizeof(sLONG)*3 + sizeof(sWORD)*9;
				pStream.SetPosByOffset(offset);
				if(packtype == 1) // no packing
				{
					offset = sizeof(sWORD)*width*height;
					pStream.SetPosByOffset(offset);
					if(pStream.GetLastError() == VE_OK)
					{
						uLONG8 pos = pStream.GetPos();
						offset = (pos & 1) ? 1 : 0;
					}
					else
						offset = 0xFFFFFFFF;
				}
				else if(packtype == 4)
				{
					for( i = 0 ; i < height ; i++ )
					{
						if(rowbyte > 250)
							packsize = pStream.GetWord();
						else
							packsize = (uBYTE)pStream.GetByte();

						pStream.SetPosByOffset(packsize);
					}
					if(pStream.GetLastError() == VE_OK)
					{
						uLONG8 pos = pStream.GetPos();
						offset = (pos & 1) ? 1 : 0;
					}
					else
						offset = 0xFFFFFFFF;
				}
				else
				{
					offset = 0xFFFFFFFF;
				}
				return (offset);
			}
		case 0x009B:
			return 0xFFFFFFFF; // TBD
		case 0x00A1:
			verror = pStream.SetPosByOffset(2); // skip kind
			verror = pStream.GetWord(aword);
			if(verror!=VE_OK)
				return 0xFFFFFFFF;
			return aword;
		case 0x00A2:
		case 0x00AF:
			verror = pStream.GetWord(aword);
			if(verror!=VE_OK)
				return 0xFFFFFFFF;
			return aword;
		case 0x0B00:
		case 0x8000:
		case 0x80FF:
			return 0;
		case 0x00D0:
		case 0x00FE:
		case 0x8100:
		case 0x8201:
		case 0xFFFF:
			verror = pStream.GetLong(along);
			if(verror!=VE_OK)
				return 0xFFFFFFFF;
			return along;
		case 0x0C00: // header
			return 24;
		default:
			return 0xFFFFFFFF;
		}
}


/************* quicktime debug info ************/

VQuicktimeInfo::VQuicktimeInfo()
{
}
VQuicktimeInfo::~VQuicktimeInfo()
{
}
sLONG VQuicktimeInfo::IW_CountProperties()
{
	if(!VQuicktime::HasQuicktime())
		return 1;
	else
		return 3;
}
void VQuicktimeInfo::IW_GetPropName(sLONG inPropID,VValueSingle& outName)
{
	switch (inPropID)
	{
		case 0:
		{
			outName.FromString("version");
			break;
		}
		case 1:
		{
			outName.FromString("importer");
			break;
		}
		case 2:
		{
			outName.FromString("exporter");
			break;
		}
	}
}
void VQuicktimeInfo::IW_GetPropValue(sLONG inPropID,VValueSingle& outValue)
{
	VString tmp;
	switch(inPropID)
	{
		case 0:
		{
			if(VQuicktime::HasQuicktime())
				IW_SetHexVal(VQuicktime::GetVersion(),outValue);
			else
				outValue.FromString("Not installed !!");
			break;
		}
		case 1:
		{
			outValue.FromLong(VQuicktime::CountImporter());
			break;
		}
		case 2:
		{
			outValue.FromLong(VQuicktime::CountExporter());
			break;
		}
	}
}
void VQuicktimeInfo::IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild)
{
	switch(inPropID)
	{
		case 1:
		{
			outEditable=false;
			*outChild=VQuicktime::GetImportComponentManager();
			break;
		}
		case 2:
		{
			outEditable=false;
			*outChild=VQuicktime::GetExportComponentManager();
			break;
		}
	}
}


/***********************************************/
static void _AdjustQuality (CompressionQuality& ioQuality);
static void _AdjustQuality(CompressionQuality& ioQuality)
{
	if (ioQuality >= CQ_LosslessQuality)
		ioQuality = CQ_LosslessQuality;
	else if (ioQuality >= CQ_MaxQuality)
		ioQuality = CQ_MaxQuality;
	else if (ioQuality >= CQ_HighQuality)
		ioQuality = CQ_HighQuality;
	else if (ioQuality >= CQ_NormalQuality)
		ioQuality = CQ_NormalQuality;
	else if (ioQuality >= CQ_LowQuality)
		ioQuality = CQ_LowQuality;
	else 
		ioQuality = CQ_MinQuality;
}


VArrayList* VQuicktime::GetIEComponentList(uLONG inIEKind)
{	
	if (!HasQuicktime()) return NULL;

	VArrayList*	componentList = 0;	//new VArrayList();
	
	if (GetVersion() >= 0x04000000)
	{
		QTIME::ComponentDescription description;
		description.componentType = inIEKind;
		description.componentSubType = 0;
		description.componentManufacturer = 0;
		description.componentFlags = 0;
		description.componentFlagsMask = QTIME::movieImportSubTypeIsFileExtension;//QTIME::graphicsExporterIsBaseExporter;
		
		QTIME::Component	component = NULL;
		while ((component = VQuicktime::FindNextComponent(component, &description)) != NULL)
			componentList->AddTail(new VQT_IEComponentInfo(component));
	}
	else if (GetVersion() >= 0x03000000)
	{		
		QTIME::ComponentInstance	instance = QTIME::OpenDefaultComponent(QTIME::GraphicsImporterComponentType, QTIME::kQTFileTypePicture);
		
		QTIME::QTAtomContainer	exportInfo = NULL;
		QTIME::OSErr	error = QTIME::GraphicsImportGetExportImageTypeList(instance, &exportInfo);
		
		sWORD	typesCount = CountChildrenOfType(exportInfo, QTIME::kParentAtomIsContainer, 'expo');	//kGraphicsExportGroup
		for (sLONG typeIndex = 1; typeIndex <= typesCount; typeIndex++) 
		{
			QTIME::QTAtom	groupAtom = QTIME::QTFindChildByIndex(exportInfo, QTIME::kParentAtomIsContainer, QTIME::kGraphicsExportGroup, typeIndex, NULL);

			uLONG	fileType;
			QTIME::QTAtom	infoAtom = QTIME::QTFindChildByIndex(exportInfo, groupAtom, QTIME::kGraphicsExportFileType, 1, NULL);
			QTIME::QTCopyAtomDataToPtr(exportInfo, infoAtom, false, sizeof(fileType), &fileType, NULL);

			uLONG	fileExtension;
			infoAtom = QTIME::QTFindChildByIndex(exportInfo, groupAtom, QTIME::kGraphicsExportExtension, 1, NULL);
			QTIME::QTCopyAtomDataToPtr(exportInfo, infoAtom, false, sizeof(fileExtension), &fileExtension, NULL);

			QTIME::Str255	descStr;
			sLONG	descSize;
			infoAtom = QTIME::QTFindChildByIndex(exportInfo, groupAtom, QTIME::kGraphicsExportDescription, 1, NULL);
			QTIME::QTCopyAtomDataToPtr(exportInfo, infoAtom, true, sizeof(QTIME::Str255), &descStr[1], &descSize);
			
			VStr255	info;
			VStr255	name;
			name.FromBlock(&descStr[1], descSize, QT_CharSet);
			
			componentList->AddTail(new VQT_IEComponentInfo('grip', fileType, name, info, fileType, 'ogle', fileExtension));
		}
		
		QTIME::CloseComponent(instance);
	}

	return componentList;
}

// cant put GetIEComponentList inside namespace using because of VArrayList template that uses ambiguous Boolean type
USING_QTIME_NAMESPACE

void VQuicktime::_InitIEComponentList(std::vector<VQT_IEComponentInfo> &inList,uLONG pIEKind)
{
	if (!HasQuicktime()) 
		return;

	if (GetVersion() >= 0x04000000)
	{
		QTIME::ComponentDescription description;
		description.componentType = pIEKind;
		description.componentSubType = 0;
		description.componentManufacturer = 0;
		description.componentFlags = 0;
		description.componentFlagsMask = QTIME::graphicsExporterIsBaseExporter;
		
		QTIME::Component	component = NULL;
		VString tmp;
		tmp.FromOsType(ByteSwapValue(pIEKind));
		
		
		while ((component = VQuicktime::FindNextComponent(component, &description)) != NULL)
			inList.push_back(VQT_IEComponentInfo(component));
	}
	else if (GetVersion() >= 0x03000000)
	{		
		QTIME::ComponentInstance	instance = QTIME::OpenDefaultComponent(QTIME::GraphicsImporterComponentType, QTIME::kQTFileTypePicture);
		
		QTIME::QTAtomContainer	exportInfo = NULL;
		QTIME::OSErr	error = QTIME::GraphicsImportGetExportImageTypeList(instance, &exportInfo);
		
		sWORD	typesCount = CountChildrenOfType(exportInfo, QTIME::kParentAtomIsContainer, 'expo');	//kGraphicsExportGroup
		for (sLONG typeIndex = 1; typeIndex <= typesCount; typeIndex++) 
		{
			QTIME::QTAtom	groupAtom = QTIME::QTFindChildByIndex(exportInfo, QTIME::kParentAtomIsContainer, QTIME::kGraphicsExportGroup, typeIndex, NULL);

			uLONG	fileType;
			QTIME::QTAtom	infoAtom = QTIME::QTFindChildByIndex(exportInfo, groupAtom, QTIME::kGraphicsExportFileType, 1, NULL);
			QTIME::QTCopyAtomDataToPtr(exportInfo, infoAtom, false, sizeof(fileType), &fileType, NULL);

			uLONG	fileExtension;
			infoAtom = QTIME::QTFindChildByIndex(exportInfo, groupAtom, QTIME::kGraphicsExportExtension, 1, NULL);
			QTIME::QTCopyAtomDataToPtr(exportInfo, infoAtom, false, sizeof(fileExtension), &fileExtension, NULL);

			QTIME::Str255	descStr;
			sLONG	descSize;
			infoAtom = QTIME::QTFindChildByIndex(exportInfo, groupAtom, QTIME::kGraphicsExportDescription, 1, NULL);
			QTIME::QTCopyAtomDataToPtr(exportInfo, infoAtom, true, sizeof(QTIME::Str255), &descStr[1], &descSize);
			
			VStr255	info;
			VStr255	name;
			name.FromBlock(&descStr[1], descSize, QT_CharSet);
			
			inList.push_back(VQT_IEComponentInfo('grip', fileType, name, info, fileType, 'ogle', fileExtension));
		}
		
		QTIME::CloseComponent(instance);
	}

}

uBYTE VQuicktime::Init()
{
#if VERSIONWIN && VERSIONDEBUG
	char path[MAX_PATH];
	size_t pathlen;
	HINSTANCE inst=0;
	GetModuleFileName(inst,path,MAX_PATH);
	pathlen=strlen(path);
	for(size_t i=pathlen;path[i]!='\\';path[i]=0,--i);
	strcat(path,"4ddebug.ini");
	if(!GetPrivateProfileInt ("TESTING", "quicktime", 1, path))
	{
		sQuickTimeVersion=-1;
		return false;
	}
#endif
	
	
	//sQTInfo = new VQuicktimeInfo();
	if (sQuickTimeVersion > -1) 
		return true;
	
	OSErr	error = noErr;

#if VERSIONWIN
	for (sLONG index = 0; index < MAX_MOVIES_COUNT; index++)
		sMovies[index] = 0;

	error = ::InitializeQTML(0);
		
	if (error == noErr)
		error = ::EnterMovies();
#endif

	if (error == noErr)
		error = ::Gestalt(gestaltQuickTimeVersion, &sQuickTimeVersion);

	if (error != noErr)
		sQuickTimeVersion = 0;
	if(error==noErr)
	{
		sExportComponents = new VQT_IEComponentManager();
		sImportComponents = new VQT_IEComponentManager();
		sExportComponents->Init('grex');
		sImportComponents->Init('grip');
	}

	return (error == noErr);
}


void VQuicktime::DeInit()
{
	delete sCompressMessage;
	sCompressMessage = NULL;
	
	delete sDeCompressMessage;
	sDeCompressMessage = NULL;
	
	delete sThumbnailMessage;
	sThumbnailMessage = NULL;
	
	delete sConvertToJpegMessage;
	sConvertToJpegMessage = NULL;
	
	delete sExportComponents;
	sExportComponents = NULL;
	delete sImportComponents;
	sImportComponents = NULL;
	
	//delete sQTInfo; 

#if USE_OFFSCREEN_DRAWPICTURE
	if (sOffscreenGWorld != NULL)
	{
		::DisposeGWorld(gOffscreenGWorld);
		sOffscreenGWorld = NULL;
	}
	
	if (sOffscreenOldPALETTE != NULL)
	{
		::SelectPalette(sOffscreenHDC, sOffscreenOldPALETTE, false);
		sOffscreenOldPALETTE = NULL;
	}
	
	if (sOffscreenOldHBITMAP != NULL)
	{
		::SelectObject(sOffscreenHDC, sOffscreenOldHBITMAP);
		::DeleteObject(sOffscreenHBITMAP);
		sOffscreenOldHBITMAP = NULL;
	}
	
	::DeleteDC(sOffscreenHDC);
	sOffscreenHDC = NULL;
#endif
	
#if VERSIONWIN
	if (sQuickTimeVersion > -1)
	{
		::ExitMovies();
		::TerminateQTML();
	}
#endif

	sQuickTimeVersion = -1;
}


VError VQuicktime::RunCompressionDialog (VQTSpatialSettings& ioSettings, const VPictureData* inPreview)
{
	if (!HasQuicktime())
		return VE_CANNOT_INITIALIZE_APPLICATION;
	
	ComponentResult	error = noErr;

	if (GetVersion() == 0)
		return false;

	ComponentInstance	instance = ::OpenDefaultComponent(StandardCompressionType, StandardCompressionSubType);
	if (instance != NULL)
	{
		PicHandle pict = NULL;
		if (inPreview != NULL)
		{
			VHandle handle = inPreview->CreatePicVHandle();
			if (handle != NULL)
			{
				pict = (PicHandle) VHandleToPicHandle( handle);
				VMemory::DisposeHandle( handle);
			}
		}
		if (pict != NULL)
		{
			Rect bounds = { 0, 0, 100, 100};
			::SCSetTestImagePictHandle( instance, pict, &bounds, scPreferCropping);
		}
		::SCSetInfo(instance, scSpatialSettingsType, (void*) ((QTSpatialSettingsRef*)ioSettings));
		
		error = ::SCRequestImageSettings(instance);
		if (error == noErr)
			error = ::SCGetInfo(instance, scSpatialSettingsType, (void*) ((QTSpatialSettingsRef*)ioSettings));
			
		if (pict != NULL)
			DisposeQTHandle( (QTHandleRef) pict);

		::CloseComponent(instance);
	}

	return error;
}

VError VQuicktime::CompressPicture(const V4DPictureData& inSource, V4DPictureData& ioDest, const QTSpatialSettingsRef& inSettings, uBYTE inShowProgress)
{
	OSErr	error = noErr;
	/*
	VOldImageData*	srcImage = inSource.GetImage();
	VOldImageData*	dstImage = ioDest.GetImage();
	
	if (testAssert(srcImage->GetImageKind() == IK_QTIME && dstImage->GetImageKind() == IK_QTIME))
		error = CompressQTimeData(*((VOldQTimeData*) srcImage), *((VOldQTimeData*) dstImage), inSettings, inShowProgress);	
	*/
	assert(false);
	return error;
}


VError VQuicktime::CompressToQTimeData(const VPictureData& inSource, VPictureData** ioDest, VQTSpatialSettings& inSettings, bool inShowProgress)
{	
	VError	error=VE_INVALID_PARAMETER;
	*ioDest=0;
	if (!HasQuicktime()) 
		return VE_UNIMPLEMENTED;
	
	OSErr err;

	VHandle	srchandle = inSource.CreatePicVHandle(0);
	PicHandle	dstPict = (PicHandle) NewQTHandle(0);
	PicHandle	srcPict;
	assert(srchandle != NULL && dstPict != NULL); 
	
	// TBD use FCompressPicture
	if(srchandle && dstPict)
	{
		srcPict=(PicHandle)VHandleToPicHandle(srchandle);
		VMemory::DisposeHandle(srchandle);
		if(srcPict)
		{
			
			err = ::CompressPicture(srcPict, dstPict, inSettings.GetSpatialQuality(), inSettings.GetCodecType());
			if(err==noErr)
			{
				char* pictptr=LockQTHandle ((QTHandleRef)dstPict);
				VSize datasize=GetQTHandleSize((QTHandleRef)dstPict);
				if(pictptr && datasize)
				{
					VPictureDataProvider *ds=VPictureDataProvider::Create((VPtr)pictptr,false,datasize,0);
					*ioDest=new VPictureData_MacPicture(ds);
					ds->Release();
					error=VE_OK;
				}
			}
		}
		DisposeQTHandle((QTHandleRef)dstPict);
		
	}

	return error;
}


VError VQuicktime::CompressFile(const VFile& inSource, VFile& ioDest, const QTSpatialSettingsRef& inSettings, uBYTE inShowProgress)
{
	if(HasQuicktime())
		return _CompressFile(inSource, ioDest, inSettings, inShowProgress, false);
	else
		return VE_UNIMPLEMENTED ;
}


VError VQuicktime::CompressFileToJpeg(const VFile& inSource, VFile& ioDest, CompressionQuality inQuality, uBYTE inShowProgress)
{
	SCSpatialSettings settings;
	
	_AdjustQuality(inQuality);
	settings.codecType = 'jpeg';
	settings.codec = 0;
	settings.depth = 0;
	settings.spatialQuality = inQuality;
	if(HasQuicktime())
		return _CompressFile(inSource, ioDest, settings, inShowProgress, true);
	else
		return VE_UNIMPLEMENTED ;
	
}


VError VQuicktime::_CompressFile(const VFile& inSource, VFile& ioDest, const QTSpatialSettingsRef& inSettings, uBYTE inShowProgress, uBYTE inToJpeg)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	VError	error = VE_OK;

	FSSpec	dstSpec;
#if VERSIONMAC
	ioDest.MAC_GetFSSpec( &dstSpec);
#else
	WIN_VFileToFSSpec(ioDest, dstSpec);
#endif

	bool	sameSrcAndDestFile = false;

	sWORD	dstFileID;
	OSErr	macErr = ::FSpOpenDF(&dstSpec, fsRdWrPerm, &dstFileID);
	if (macErr == noErr)
	{
		sWORD	srcFileID = -1;
		if (&inSource != &ioDest)	// JT : To be fixed please !!!
		{
			FSSpec	srcSpec;
		#if VERSIONMAC
			inSource.MAC_GetFSSpec(&srcSpec);
		#else
			WIN_VFileToFSSpec(inSource, srcSpec);
		#endif

			macErr = ::FSpOpenDF(&srcSpec, fsRdWrPerm, &srcFileID);
		}
		else
		{
			srcFileID = dstFileID;
			sameSrcAndDestFile = true;
		}

		if (macErr == noErr)
		{
			ICMProgressProcRecord	progressRec;
			ICMProgressProcRecord*	procPtr = NULL;

			if (inShowProgress)
			{
			#if VERSIONMAC
				progressRec.progressProc = ::NewICMProgressUPP(_ProgressProc);
			#else
				progressRec.progressProc = NewICMProgressProc(_ProgressProc);
			#endif
				progressRec.progressRefCon = kCompressMessage;
				procPtr = &progressRec;
			}
			
			CompressionQuality	quality = (CompressionQuality) inSettings.spatialQuality;
			_AdjustQuality(quality);

			sCompressor = inSettings.codecType;
			_ProgressProc(codecProgressOpen, 0x00000000, kCompressMessage);
			_ProgressProc(codecProgressUpdatePercent, 0x00000000, kCompressMessage);
			macErr = ::FCompressPictureFile(srcFileID, dstFileID, inSettings.depth, 0, quality, defaultDither, false, procPtr, inSettings.codecType, inSettings.codec);
			_ProgressProc(codecProgressClose, 0x0000FFFF, kCompressMessage);

			if (inShowProgress)
			{
			#if VERSIONMAC
				::DisposeICMProgressUPP(progressRec.progressProc);
			#endif
			}

			if (!sameSrcAndDestFile)
				::FSClose(srcFileID);
		}

		::FSClose(dstFileID);
	}

	if (inToJpeg && macErr == noErr)
	{
		VFileStream*	strread;
		uWORD	opcode = 0;
		uBYTE setkind = false, jpegwrote = false;
		sLONG	sLONGcode, pictsize, qtoffset, jpegoffset, jpegsize, skipsize, charcount, writeoffset;
		
		VFileDesc *ioDestDesc = NULL;
		error = ioDest.Open(FA_SHARED,&ioDestDesc);
		if (error == VE_OK)
		{
			strread = new VFileStream(&ioDest);
			if (strread->OpenReading() == VE_OK)
			{
				pictsize = strread->GetSize();
				//SetCpuKind(CPU_PPC, CPU_SAME);
				strread->SetBigEndian();
				strread->SetPos(kPICTURE_HEADER_SIZE + 10); // pict file header + sizeof(Picture)
				error = VE_OK;
				skipsize = 0;
				while(error == VE_OK && skipsize != 0xFFFFFFFF)
				{
					while(error == VE_OK)
					{
						error = strread->GetWord(opcode);
						if (error == VE_OK)
						{
							if (opcode == 0x8200) // QuickTimeCompressedPicture
								break;
							else if (opcode == 0x00FF) // end of picture
								break;
							else
							{
								skipsize = ::GetPictOpcodeSkipSize(*strread, opcode);
								if (skipsize == 0xFFFFFFFF) // opcode nothandled
									break;
								error = strread->SetPosByOffset(skipsize);
							}
						}
					}
					if (opcode == 0x8200 && error == VE_OK) // QuickTimeCompressedPicture
					{
						qtoffset = strread->GetPos();
						error = strread->GetLong(jpegsize);
						// now run to start of jpeg data
						if (error == VE_OK) 
							error = strread->SetPosByOffset(sizeof(ImageDescription)-sizeof(sLONG));
						while(error == VE_OK)
						{
							error = strread->GetLong(sLONGcode);
							if (error == VE_OK && sLONGcode == 0xFFD8FFE0)
								break;
							error = strread->SetPosByOffset(-(sLONG) sizeof(sWORD));
						}
						if (sLONGcode == 0xFFD8FFE0 && error == VE_OK) // start of jpegdata
					{
						sLONG	buffersize;
						char*	buffer;
						
							strread->SetPosByOffset(-(sLONG) sizeof(sLONG));
							jpegoffset = strread->GetPos();
						jpegsize -= jpegoffset-qtoffset-4;
							strread->SetBufferSize(0);
						// allocate buffer
							buffersize = 0x00010000;
							if (buffersize>jpegsize)
								buffersize = jpegsize;
						buffer = 0;
						while(buffersize)
						{
								buffer = VMemory::NewPtr(buffersize, 'quic');
							if (buffer)
								break;
							buffersize >>= 1;
						}
						if (!buffer)
								error = strread->SetPosByOffset(jpegsize);
						else
						{
							sLONG	progresssize;
							
							writeoffset = 0;
							if (inShowProgress && jpegsize>buffersize)
							{
								progresssize = jpegsize;
								_ProgressProc(codecProgressOpen, 0x00000000, kConvertToJpegMessage);
							}
							else
								progresssize = 0;
							while(jpegsize && error == VE_OK)
							{
								if (buffersize>jpegsize)
									buffersize = jpegsize;
								error = strread->SetPos(jpegoffset);
								if (error == VE_OK)
								{
									charcount = buffersize;
									error = strread->GetBytes(buffer, &charcount);
									if (error == VE_OK || error == VE_STREAM_EOF)
									{
										jpegsize -= charcount;
										jpegoffset += charcount;
										error = ioDestDesc->SetPos(writeoffset);
										if (error == VE_OK)
										{
											error = ioDestDesc->PutDataAtPos(buffer, charcount);
											writeoffset += charcount;
											jpegwrote = true;
											if (progresssize)
											{
												Real	realpercent;
												sLONG	sLONGpercent;
												
												realpercent = (1.0 - ((Real)jpegsize/(Real)progresssize))*65536;
												sLONGpercent = (sLONG) realpercent;
												if (sLONGpercent>0x0000FFFF)
													sLONGpercent = 0x0000FFFF;
											#if VERSIONMAC
												_ProgressProc(codecProgressUpdatePercent, (Fixed) sLONGpercent, kConvertToJpegMessage);
											#else
												_ProgressProc(codecProgressUpdatePercent, (qtime::Fixed)sLONGpercent, kConvertToJpegMessage);
											#endif
											}
										}
									}
								}
							}
							if (progresssize)
								_ProgressProc(codecProgressClose, 0x0000FFFF, kConvertToJpegMessage);
							VMemory::DisposePtr(buffer);
						}
					}
				}
			}
			if (jpegwrote)
			{
					error = ioDestDesc->SetSize(writeoffset);
					if (error == VE_OK || error == VE_STREAM_EOF)
					{
						jpegwrote = false;
						setkind = true;
					}
					else
						error = ioDestDesc->SetSize(0); // to avoid displaying crashing garbage
				}
				strread->CloseReading();
			}
			delete ioDestDesc;
			ioDestDesc = NULL;

			if (setkind && (error == VE_OK || error == VE_STREAM_EOF))
			{
			#if VERSIONMAC
				FInfo	fileinfo;
				OSErr	temperr;
			
				temperr = ::FSpGetFInfo(&dstSpec, &fileinfo);
				fileinfo.fdType = 'JPEG';
				temperr = ::FSpSetFInfo(&dstSpec, &fileinfo);
			#elif VERSIONWIN
				char	srcPath[MAX_PATH];
				char	dstPath[MAX_PATH];

				size_t	length;
				if (!sameSrcAndDestFile)
				{
					ioDest.GetPath().GetPath().WIN_ToWinCString(dstPath, sizeof(dstPath));

					length = ::strlen(dstPath);
					::memcpy(srcPath, dstPath, length + 1);
				}
				else
				{
					inSource.GetPath().GetPath().WIN_ToWinCString(srcPath, sizeof(srcPath));

					length = ::strlen(srcPath);
					::memcpy(dstPath, srcPath, length + 1);
				}

				while (--length)
				{
					if (dstPath[length] == '.') break;
				}

				if (length > 0 && (length + 3) < MAX_PATH)
				{
					dstPath[length + 1] = 'j';
					dstPath[length + 2] = 'p';
					dstPath[length + 3] = 'g';
					dstPath[length + 4] = 0;
				}

				if (!::MoveFile(srcPath, dstPath))
					DWORD error = ::GetLastError();
			#endif
			}
		}
	}

	return error;
}


V4DPictureData* VQuicktime::GetPictureThumbnail(const V4DPictureData& inSource, uBYTE /*inShowProgress*/)
{	
	/*
	if (!HasQuicktime()) return NULL;
	
	VOldImageData*	srcImageData = inSource.GetImage();
	if (srcImageData == NULL) return false;
	
	PicHandle	srcPict = (PicHandle) srcImageData->GetData();
	
	Rect	bound = { 0, 0, 0, 0 };
	PicHandle	destPict = ::OpenPicture(&bound);
	::ClosePicture();
	
	VOldQTimeData*	previewData = NULL;
	
	OSErr	error = ::MakeThumbnailFromPicture(srcPict, 0, destPict, (ICMProgressProcRecord*)-1);
	if (error == noErr)
	{
		::HLock((Handle)destPict);
		previewData = new VOldQTimeData((VPtr)*destPict, ::GetHandleSize((Handle)destPict));
		::HUnlock((Handle)destPict);
		::KillPicture(destPict);
	}
	
	return previewData;
	*/
	assert(false);
	return 0;
}


VError VQuicktime::GetFileThumbnail(const VFile& inSource, V4DPictureData& outData, uBYTE inShowProgress)
{	
/*
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	ICMProgressProcRecord	progressRec;
	ICMProgressProcRecord*	procPtr = NULL;
	
	
	FSSpec	fsSpec;
	
#if VERSIONWIN
	WIN_VFileToFSSpec(inSource, fsSpec);
#else
	inSource.MAC_GetFSSpec(&fsSpec);
#endif
	
	sWORD	fileID;
	OSErr	error = ::FSpOpenDF(&fsSpec, fsRdPerm, &fileID);
	if (error == noErr)
	{
		if (inShowProgress)
		{
		#if VERSIONMAC
			progressRec.progressProc = NewICMProgressUPP(_ProgressProc);
		#else
			progressRec.progressProc = NewICMProgressProc(_ProgressProc);
		#endif
			progressRec.progressRefCon = kThumbnailMessage;
			procPtr = &progressRec;
		}
		
		PicHandle	pict = (PicHandle) outData.GetData();
		error = ::MakeThumbnailFromPictureFile(fileID, 0, pict, procPtr);
		::FSClose(fileID);
	}
	
	return error;
	*/
	assert(false);
	return 0;
}


VError VQuicktime::SetFilePreview(const VFile& inSource, const V4DPictureData& inData)
{	
	/*
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	FSSpec	fsSpec;
	
#if VERSIONWIN
	WIN_VFileToFSSpec(inSource, fsSpec);
	OSType	creator = 0;
	OSType	kind = 0;	//666-666
#else
	inSource.MAC_GetFSSpec( &fsSpec);
	OSType creator,kind;
	inSource.MAC_GetCreator(&creator);
	inSource.MAC_GetKind(&kind);
#endif
	
	sWORD	fileID = ::FSpOpenResFile(&fsSpec, fsWrPerm);
	if (fileID < 0)
	{
		::FSpCreateResFile(&fsSpec, creator, kind, fsWrPerm);
		fileID = ::FSpOpenResFile(&fsSpec, fsWrPerm);
	}
	
	if (fileID < 0)
		return false;
		
	OSErr	error = ::AddFilePreview(fileID, 'PICT', (Handle) inData.GetData());
	::CloseResFile(fileID);
	return error;
	*/
	assert(false);
	return 0;
}


VError VQuicktime::GetPictureThumbnail(const V4DPictureData& inSource, V4DPictureData& outData, uBYTE /*inShowProgress*/)
{	
	/*
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	VOldImageData*	srcImageData = inSource.GetImage();
	if (srcImageData == NULL) return false;
		
	PicHandle	srcPict = (PicHandle) srcImageData->GetData();
	PicHandle	destPict = (PicHandle) outData.GetData();
	
	return ::MakeThumbnailFromPicture(srcPict, 0, destPict, (ICMProgressProcRecord*) -1);
	*/
	assert(false);
	return 0;
}


#if VERSIONMAC


pascal void VQuicktime::_DrawProgressBar(DialogPtr inDialog, sWORD inItem)
{
	sWORD	type;
	Rect	rect;
	Handle	handle;
	::GetDialogItem(inDialog, inItem, &type, &handle, &rect);
	
	::BackColor(whiteColor);
	::EraseRect(&rect);
	::ForeColor(blackColor);
	::PenSize(1, 1);
	::MacFrameRect(&rect);
	
	sWORD	width = (sWORD) (((Real) (rect.right - rect.left) * sProgressPercentage));
	
	rect.right = rect.left + width;
	::PaintRect(&rect);
}


pascal OSErr VQuicktime::_ProgressProc(sWORD inAction, Fixed inCompleteness, sLONG inRefcon)
{
	static DialogPtr	sProgressDialog = NULL;
	static UserItemUPP	sProgressProc = NULL;
	
	Str255	macStr;
	sWORD	item;
	
	VResourceFile*	resFile = VProcess::Get()->RetainResourceFile();
	
	OSErr	error = noErr;
	switch (inAction)
	{
		case codecProgressOpen:
			if (sProgressDialog != NULL)
			{
				VStr4	compressor;
				VString	message;
				
				switch (inRefcon)
				{
					case kCompressMessage:
						if (sCompressMessage != NULL)
						{
							message = *sCompressMessage;
						}
						else
						{
							resFile->GetString(message, 9004, 1);
							compressor.FromOsType(sCompressor);
							message += compressor;
						}
						break;
						
					case kDecompressMessage:
						if (sDeCompressMessage != NULL)
						{
							message = *sDeCompressMessage;
						}
						else
						{
							resFile->GetString(message, 9004, 2);
							compressor.FromOsType(sCompressor);
							message += compressor;
						}
						break;
						
					case kThumbnailMessage:
						if (sThumbnailMessage != NULL)
							message = *sThumbnailMessage;
						else
							resFile->GetString(message, 9004, 3);
						break;
						
					case kConvertToJpegMessage:
						if (sConvertToJpegMessage != NULL)
							message = *sConvertToJpegMessage;
						else
							resFile->GetString(message, 9004, 4);
						break;
				}
				
				message.MAC_ToMacPString(macStr, sizeof(macStr));
				
				sProgressPercentage = 0;
				sProgressProc = NewUserItemUPP(_DrawProgressBar);
				
				::ParamText(macStr, "\p", "\p", "\p");
				
				Rect	bounds;
				sWORD	type;
				Handle	handle;
				sProgressDialog = ::GetNewDialog(9002, 0, (WindowPtr) -1);
				::GetDialogItem(sProgressDialog, 3, &type, &handle, &bounds);
				::SetDialogItem(sProgressDialog, 3, type, (Handle) sProgressProc, &bounds);
				
				WindowRef	dialogWindow = ::GetDialogWindow(sProgressDialog);
				::RepositionWindow(dialogWindow, NULL, kWindowCenterOnMainScreen);
				::ShowWindow(dialogWindow);
				
				// Don't break in order to get the items drawn through this call
			}
		
	case codecProgressUpdatePercent:
		if (sProgressDialog != NULL)
		{
			GrafPtr	savedPort;
			::GetPort(&savedPort);
			::SetPortWindowPort(::GetDialogWindow(sProgressDialog));
			_DrawProgressBar(sProgressDialog, 3);
			
			sProgressPercentage = ((sLONG)inCompleteness) & 0x0000FFFF;
			sProgressPercentage /= 0x0000FFFF;
			sProgressPercentage = Min(sProgressPercentage, 1.0);
				
			uBYTE	done = false;
			while (!done)
			{
				DialogPtr	dialog;
				EventRecord	event;
				if (::WaitNextEvent(everyEvent, &event, 2, 0))
				{
					if (::IsDialogEvent(&event))
					{
						if (::DialogSelect(&event, &dialog, &item))
						{
							if (dialog == sProgressDialog && item == 1)
								error = codecAbortErr;
						}
					}
					
					done = (event.what == 6 && event.message == (sLONG) sProgressDialog);
				}
				
				if (inAction != codecProgressOpen)
					break;
			}
			
			::MacSetPort(savedPort);
		}
		
		if (error != noErr)
		{
			if (sProgressDialog != NULL)
			{
				::DisposeDialog(sProgressDialog);
				sProgressDialog = 0;
			}
			
			if (sProgressProc != NULL)
			{
				::DisposeUserItemUPP(sProgressProc);
				sProgressProc = 0;
			}
		}
		break;
			
	case codecProgressClose:
		if (sProgressDialog != NULL)
		{
			::DisposeDialog(sProgressDialog);
			sProgressDialog = 0;
		}
		
		if (sProgressProc != NULL)
		{
			::DisposeUserItemUPP(sProgressProc);
			sProgressProc = 0;
		}
		break;
	}
	
	resFile->Release();
	
	return error;
}



#endif	// VERSIONMAC


#if VERSIONWIN
OSErr __cdecl VQuicktime::_ProgressProc(sWORD inAction, qtime::Fixed inCompleteness, sLONG inRefcon)
{
	return -8967; // codecAbortErr
}

#endif	// VERSIONWIN


#pragma mark-

QTHandleRef VQuicktime::NewQTHandle(sLONG inSize)
{	
	if (!HasQuicktime()) return NULL;

	return ::NewHandle(inSize);

}

VError VQuicktime::PtrToQTHand(const void *srcPtr,QTHandleRef *dstHndl,sLONG size)
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::PtrToHand(srcPtr,dstHndl,size);

}   

VError VQuicktime::SetQTHandleSize(QTHandleRef ioHandle, sLONG inSize)
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	::SetHandleSize(ioHandle, inSize);
	return ::MemError();

}


sLONG VQuicktime::GetQTHandleSize(const QTHandleRef ioHandle)
{
	if (!HasQuicktime()) return 0;
	
	return ::GetHandleSize(ioHandle);

}


char* VQuicktime::LockQTHandle(QTHandleRef ioHandle)
{	
	if (!HasQuicktime()) return NULL;
	
	::HLock(ioHandle);
	return *ioHandle;

}


void VQuicktime::UnlockQTHandle(QTHandleRef ioHandle)
{	
	if (!HasQuicktime()) return;

	::HUnlock(ioHandle);

}


void VQuicktime::DisposeQTHandle(QTHandleRef ioHandle)
{	
	if (!HasQuicktime()) return;

	::DisposeHandle(ioHandle);

}


QTHandleRef VQuicktime::QTHandleToQTHandle(const QTHandleRef inHandle)
{	
	if (!HasQuicktime()) return NULL;

	assert(inHandle != NULL);
	
	::HandToHand((Handle*)&inHandle);
	return (::MemError() == noErr ? inHandle : NULL);

}

VHandle VQuicktime::QTHandleToVHandle(const QTHandleRef inHandle, VHandle /*outDest*/)
{
	VHandle result = NULL;
	
	if (inHandle != NULL)
	{
		sLONG size = GetQTHandleSize( inHandle);
		result = VMemory::NewHandle( size);
		if (result != NULL)
		{
			VMemory::CopyBlock( LockQTHandle( inHandle), VMemory::LockHandle( result), size);
			UnlockQTHandle( inHandle);
			VMemory::UnlockHandle( result);
		}
	}
	return result;
}


QTHandleRef VQuicktime::VHandleToQTHandle(const VHandle inHandle, QTHandleRef /*outDest*/)
{
	QTHandleRef result = NULL;
	
	if (inHandle != NULL)
	{
		VSize size = VMemory::GetHandleSize( inHandle);
		result = NewQTHandle( (sLONG) size);
		if (result != NULL)
		{
			VMemory::CopyBlock( VMemory::LockHandle( inHandle), LockQTHandle( result), size);
			VMemory::UnlockHandle( inHandle);
			UnlockQTHandle( result);
		}
	}
	return result;
}


QTHandleRef VQuicktime::VHandleToPicHandle( const VHandle inHandle)
{
#if VERSIONWIN
	QTHandleRef result = NULL;
	
	if (inHandle != NULL)
	{
		VSize size = VMemory::GetHandleSize( inHandle);
		result = NewQTHandle( (sLONG) size);
		if (result != NULL)
		{
			void *destPtr = LockQTHandle( result);

			VMemory::CopyBlock( VMemory::LockHandle( inHandle),destPtr, size);
			::ByteSwapWordArray( (uWORD*)destPtr, 5);

			VMemory::UnlockHandle( inHandle);
			UnlockQTHandle( result);
		}
	}
	return result;
#else // VERSIONMAC
	return VHandleToQTHandle( inHandle);
#endif
}

#if USE_MAC_HANDLES

Handle VQuicktime::QTHandleToMacHandle(const QTHandleRef inHandle, Handle ioDest)
{
	sLONG	size = GetQTHandleSize(inHandle);
	if (ioDest != NULL)
	{
		altura::SetHandleSize(ioDest, size);
		if (altura::MemError() != noErr)
			size = 0;
	}
	else
	{
		ioDest = altura::NewHandle(size);
	}
	
	if (size > 0)
		::BlockMove(*(Handle)inHandle, *ioDest, size);
		
	DisposeQTHandle(inHandle);
	return ioDest;
}


QTHandleRef VQuicktime::MacHandleToQTHandle(const Handle inHandle, QTHandleRef ioDest)
{
	sLONG	size = altura::GetHandleSize(inHandle);
	if (ioDest && SetQTHandleSize(ioDest, size) != noErr)
		size = 0;
	else
		ioDest = NewQTHandle(size);
		
	if (size > 0)
		::BlockMove(*inHandle, *(Handle)ioDest, size);
		
	altura::DisposeHandle(inHandle);
	return ioDest;
}

#endif	// USE_MAC_HANDLES


QTPictureRef VQuicktime::NewEmptyQTPicture()
{	
	if (!HasQuicktime()) return NULL;
	PicHandle	picture = NULL;

	GWorldPtr		savedPort;
	GDHandle	savedGD;
	::GetGWorld(&savedPort, &savedGD);

#if VERSIONMAC
	assert(!::IsPortPictureBeingDefined(savedPort));
#endif

	Rect	rect = { 0, 0, 0, 0 };
	picture = ::OpenPicture(&rect);
	::ClosePicture();

	::SetGWorld(savedPort, savedGD);

	return picture;
}


void VQuicktime::KillQTPicture(QTPictureRef ioPicture)
{	
	if (!HasQuicktime()) return;

	::KillPicture(ioPicture);

}


VError VQuicktime::GraphicsImportSetDataFile(VFile& inFile,QTImportComponentRef gi)
{
	if (!HasQuicktime()) return VE_UNIMPLEMENTED;
	VError	error = VE_UNIMPLEMENTED;

	FSSpec	fsSpec;
#if VERSIONMAC
	inFile.MAC_GetFSSpec(&fsSpec);
#else
	WIN_VFileToFSSpec(inFile, fsSpec);
#endif

	error = ::GraphicsImportSetDataFile(gi,&fsSpec);

	return error;
}

VError VQuicktime::GraphicsImportValidate(QTImportComponentRef gi,unsigned char* valid)
{
	if (!HasQuicktime()) return VE_UNIMPLEMENTED;
	VError	error = VE_UNIMPLEMENTED;
	error = ::GraphicsImportValidate(gi, valid);
	
	return error;
}

VError VQuicktime::GetGraphicsImporterForFile(VFile& inFile,QTImportComponentRef *gi)
{
	*gi=NULL;
	if (!HasQuicktime()) return VE_UNIMPLEMENTED;
	VError	error = VE_UNIMPLEMENTED;

	FSSpec	fsSpec;
#if VERSIONMAC
	inFile.MAC_GetFSSpec(&fsSpec);
#else
	WIN_VFileToFSSpec(inFile, fsSpec);
#endif

	error = ::GetGraphicsImporterForFile(&fsSpec, gi);

	return error;
}



#if USE_MAC_PICTS

PicHandle VQuicktime::QTPictureToMacPicture(const QTPictureRef inQTPicture, PicHandle /*outMacData*/)
{	
#if VERSIONMAC
	return MacPictureToQTPicture(inQTPicture);
#else
	if (!HasQuicktime()) return NULL;
	
	if (inQTPicture == outMacData) return inQTPicture;
	
	sLONG	size = GetQTHandleSize(inQTPicture);
	if (outMacData != NULL && SetQTHandleSize((Handle)outMacData, size) != noErr)
		size = 0;
	else
		outMacData = (PicHandle) ::NewHandle(size);
		
	if (outMacData != NULL && size > 0)
	{
		::BlockMove(*(Handle)inQTPicture, *(Handle)outMacData, size);
		::ByteSwapWordArray(*(sWORD**)outMacData, 5);
	}
	
	return outMacData;
#endif
}


QTPictureRef VQuicktime::MacPictureToQTPicture(const PicHandle inMacPicture)
{	
	if (!HasQuicktime()) return NULL;
	
	sLONG	length = ::GetHandleSize((Handle)inMacPicture);
	Handle	newHandle = ::NewHandle(length);
	
	if (newHandle != NULL)
		::BlockMove(*inMacPicture, *newHandle, length);
		
	return (QTPictureRef) newHandle;
}


PicHandle VQuicktime::MacPictureFromFile(const VFile& inFile)
{	
	if (!HasQuicktime()) return NULL;

	PicHandle	picture = NULL;
	
	FSSpec	fsSpec;
#if VERSIONMAC
	inFile.MAC_GetFSSpec(&fsSpec);
#else
	WIN_VFileToFSSpec(inFile, fsSpec);
#endif
	
	QTImportComponentRef	importer = NULL;
	OSErr	error = ::GetGraphicsImporterForFile(&fsSpec, &importer);
	if (error == noErr)
	{
		error = ::GraphicsImportGetAsPicture(importer, &picture);
		::CloseComponent(importer);
		
	#if VERSIONWIN
		PicHandle	macPicture = QTPictureToMacPicture(picture);
		KillQTPicture(picture);
		picture = macPicture;
	#endif
	}

	return picture;
}


VError VQuicktime::MacPictureToFile(VFile*& ioFile, const PicHandle inPicture, OsType inFormat, uBYTE /*inShowSettings*/, uBYTE inShowPutFile)
{	
	if (!HasQuicktime()) return VE_UNIMPLEMENTED;
	
	OSErr error = paramErr;

	GraphicsExportComponent	exporter = NULL;
	
	if (inPicture != NULL)
	{
	#if VERSIONWIN
		inPicture = MacPictureToQTPicture(inPicture);
	#endif
		
		Boolean	headerAppened	= false;
		error = ::OpenADefaultComponent(GraphicsImporterComponentType, kQTFileTypePicture, &exporter);
		
		if (error == noErr && exporter != NULL)
		{
			uLONG	size = ::GetHandleSize((Handle)inPicture);
			::SetHandleSize((Handle)inPicture, size + kPICTURE_HEADER_SIZE);
			
			if ((error = ::MemError()) == noErr)
			{
				::BlockMove(*inPicture, (*(Handle)inPicture) + kPICTURE_HEADER_SIZE, size);
				
				for (sLONG index = 0; index < (kPICTURE_HEADER_SIZE / sizeof(sLONG)); index++)
					*((sLONG*) *inPicture) = 0;
				
				headerAppened = true;
				error = ::GraphicsImportSetDataHandle(exporter, (Handle)inPicture);
			}
				
			if (error == noErr)
			{
				FSSpec	fsSpec;
				
				if (inShowPutFile)
				{
					ScriptCode	scriptCode;
					uLONG	converter;
					
				#if VERSIONMAC
					ioFile->MAC_GetFSSpec(&fsSpec);
				#else
					WIN_VFileToFSSpec(*ioFile, fsSpec);
				#endif
					
					Str31	prompt = "\p";
					error = ::GraphicsImportDoExportImageFileDialog(exporter, &fsSpec, prompt, NULL, &converter, &fsSpec, &scriptCode);
					
				#if VERSIONMAC
					if (error == noErr)
					{
						assert(false);
						// on ne peu pas changer de fichier dans un vfile
						//ioFile.MAC_ChangeFSSpec(fsSpec); 666-666
					}
				#else
					ioFile->Release();
					ioFile = WIN_FSSpecToVFile(fsSpec);
				#endif
				}
				else
				{
					ioFile->MAC_GetFSSpec(&fsSpec);
					error = ::GraphicsImportExportImageFile(exporter, inFormat, 0, &fsSpec, smSystemScript);
				}
			}
			
			::CloseComponent(exporter);
			
			if (headerAppened)
			{
				::BlockMove((*(Handle)inPicture) + kPICTURE_HEADER_SIZE, *inPicture, size);
				::SetHandleSize((Handle)inPicture, size);
			}
		}
		
		#if VERSIONWIN
			KillQTPicture(inPicture);
		#endif
	}

	return error;
}

#endif	// USE_MAC_PICTS


#pragma mark-

QTInstanceRef VQuicktime::OpenComponent(QTComponentRef inComponentRef)
{	
	if (!HasQuicktime()) return NULL;

	return ::OpenComponent(inComponentRef);

}


VError VQuicktime::CloseComponent(QTInstanceRef inInstance)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	return ::CloseComponent(inInstance);

}


VError VQuicktime::OpenDefaultComponent(OsType inType, OsType inSubType, QTInstanceRef* outInstance)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::OpenADefaultComponent(inType, inSubType, outInstance);

}


VError VQuicktime::GetComponentInfo(QTComponentRef inComponent, QTComponentDescriptionRef* outDescription, Handle ioName, Handle ioInfo, Handle ioIcon)
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::GetComponentInfo(inComponent, outDescription, ioName, ioInfo, ioIcon);

}


QTComponentRef VQuicktime::FindNextComponent(QTComponentRef inComponent, QTComponentDescriptionRef* outDescription)
{	
	if (!HasQuicktime()) return NULL;
	return ::FindNextComponent(inComponent, outDescription);

}


VError VQuicktime::GraphicsExportGetDefaultFileTypeAndCreator(QTExportComponentRef inComponent, OsType& outFileType, OsType& outFileCreator)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::GraphicsExportGetDefaultFileTypeAndCreator(inComponent, &outFileType, &outFileCreator);

}


VError VQuicktime::GraphicsExportGetDefaultFileNameExtension(QTExportComponentRef inComponent, OsType& outFileNameExtension)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::GraphicsExportGetDefaultFileNameExtension(inComponent, &outFileNameExtension);

}


VError VQuicktime::GraphicsImportGetExportImageTypeList(QTImportComponentRef inComponent, void* inAtomContainerPtr)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::GraphicsImportGetExportImageTypeList(inComponent, inAtomContainerPtr);

}


sWORD VQuicktime::CountChildrenOfType(QTAtomContainerRef inContainer, QTAtomRef inParentAtom, OsType inType)
{
	if (!HasQuicktime()) return 0;

	return ::QTCountChildrenOfType(inContainer, inParentAtom, inType);

}


QTAtomRef VQuicktime::FindChildByIndex(QTAtomContainerRef inContainer, QTAtomRef inParentAtom, OsType inType, sWORD inIndex, sLONG* outAtomID)
{	
	if (!HasQuicktime()) return 0;
	
	return ::QTFindChildByIndex(inContainer, inParentAtom, inType, inIndex, outAtomID);

}


VError VQuicktime::CopyAtomDataToPtr(QTAtomContainerRef inContainer, QTAtomRef inAtom, uBYTE inSizeOrLessOK, sLONG inMaxSize, void* outData, sLONG* outActualSize)
{	
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	return ::QTCopyAtomDataToPtr(inContainer, inAtom, inSizeOrLessOK, inMaxSize, outData, outActualSize);

}


VError VQuicktime::GetGraphicsImporterForDataRef(QTHandleRef dataRef,OsType dataRefType,QTInstanceRef *gi) 
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	return ::GetGraphicsImporterForDataRef(dataRef,dataRefType,gi) ;

}

VError VQuicktime::GraphicsImportSetDataReference(QTImportComponentRef gi,QTHandleRef dataRef,OsType dataRefType) 
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;

	return ::GraphicsImportSetDataReference(gi,dataRef,dataRefType) ;

}

VError VQuicktime::GraphicsImportSetDataHandle(QTImportComponentRef  ci,QTHandleRef h)
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	return ::GraphicsImportSetDataHandle(ci,h) ;

}

VError VQuicktime::GraphicsImportGetAsPicture(QTImportComponentRef  ci,QTPictureRef *picture)
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	return ::GraphicsImportGetAsPicture(ci,picture) ;

}

#pragma mark-

VFile* VQuicktime::StandardGetFilePreview()
{
	VFile*	result = NULL;
	
#if VERSIONMAC
	VResourceFile*	resFile = VProcess::Get()->RetainResourceFile();
	assert(false);
#if 0
	VGetFileDialog	dialog(resFile, 9000);
	result = dialog.GetFile();
#endif
	resFile->Release();
#endif	// VERSIONMAC

#if VERSIONWIN
	if (!HasQuicktime()) return NULL;
	
	char	pathBuffer[256] = "";
	StandardFileReply	reply;
	SFTypeList	typeList = { kQTFileTypeQuickTimeImage, 0, 0, 0 };

	::StandardGetFilePreview(NULL, 1, typeList, &reply);
	if (reply.sfGood)
		::FSSpecToNativePathName(&reply.sfFile, pathBuffer, sizeof(pathBuffer), kFullNativePath);

	if (pathBuffer[0] != 0)
	{
		VString	path;
		path.WIN_FromWinCString(pathBuffer);
		result = new VFile(path);
	}
#endif	// VERSIONWIN

	return result;
}

#if USE_MAC_PICTS && USE_MAC_HANDLES

Handle VQuicktime::ExportMacPictureToMacHandle(const PicHandle inSource, OsType inExporter)
{	
	if (!HasQuicktime()) return NULL;
	if (inSource == NULL) return NULL;
	
	Handle	result = NULL;

	if (VQuicktime::GetVersion() >= 0x04000000)
	{
	#if VERSIONWIN
		inSource = MacPictureToQTPicture(inSource);
	#endif
	
		GraphicsExportComponent	exporter = NULL;
		OSErr	error = ::OpenADefaultComponent(GraphicsExporterComponentType, inExporter, &exporter);
		if (error == noErr && exporter != NULL)
		{
			error = ::GraphicsExportSetInputPicture(exporter, inSource);
			if (error == noErr)
			{
				result = ::NewHandle(0);
				error = ::GraphicsExportSetOutputHandle(exporter, result);
			}
			
			if (error == noErr)
				error = ::GraphicsExportDoExport(exporter, 0);
				
			if (error != noErr)
				::DisposeHandle(result);
			
			::CloseComponent(exporter);
		}
	
	#if VERSIONWIN
		DisposeQTHandle(inSource);
		
		PicHandle	macPicture = QTPictureToMacPicture(result);
		KillQTPicture(result);
		result = macPicture;
	#endif
	}

	return result;
}

#endif



void* VQuicktime::_SetCompressionDialogPreview(QTInstanceRef inInstance)
{
	PicHandle	picture = NULL;
	
#if VERSIONMAC
	picture = ::GetPicture(9000);
	::HNoPurge((Handle)picture);
#else
	VResourceFile*	resFile = VProcess::Get()->RetainResourceFile();
	if (resFile != NULL)
	{
		VHandle	handle = resFile->GetResource(VResTypeString('PICT'), 9000);
		if (handle != NULL)
		{
			sLONG	size = (sLONG) VMemory::GetHandleSize(handle);
			picture = (PicHandle) NewQTHandle(size);
			
			if (picture != NULL)
				::BlockMove(VMemory::LockHandle(handle), *picture, size);
				
			VMemory::UnlockHandle(handle);
			VMemory::DisposeHandle(handle);
		}
		
		resFile->Release();
	}
#endif

	Rect	bounds = { 0, 0, 100, 100 };
	OSErr	error = ::SCSetTestImagePictHandle(inInstance, picture, &bounds, scPreferCropping);

	return (void*) picture;
}


void VQuicktime::_ReleaseCompressionDialogPreview(void* inQTPicture)
{
	// Picture is a resource (see _SetCompressionDialogPreview)
	if (inQTPicture != NULL)
	{
	#if VERSIONMAC
		::ReleaseResource((Handle) inQTPicture);
	#else
		DisposeQTHandle((QTHandleRef) inQTPicture);
	#endif
	}
}


#if VERSIONWIN

void VQuicktime::WIN_VFileToFSSpec(const VFile& inFile, FSSpec& outSpec)
{
	if (!HasQuicktime()) return;
	
	char	pathBuffer[MAX_PATH] = "";
	inFile.GetPath().GetPath().WIN_ToWinCString(pathBuffer, MAX_PATH);
	::NativePathNameToFSSpec(pathBuffer, &outSpec, 0);

}


VFile* VQuicktime::WIN_FSSpecToVFile(const FSSpec& inSpec)
{
	if (!HasQuicktime()) return NULL;

	uLONG	size = 0;
	char	pathBuffer[MAX_PATH] = "";
	::FSSpecToNativePathName(&inSpec, pathBuffer, size, kFullNativePath);
	
	//ioFile.GetPath().GetPath().WIN_FromCString(pathBuffer, size);
	return new VFile(VString(pathBuffer, size, VTC_Win32Ansi));

}

#endif	// VERSIONWIN

#pragma mark-

VQT_IEComponentInfo::VQT_IEComponentInfo(const VQT_IEComponentInfo& inRef)
:fMacTypeListWatch(&fMacTypeList,VString("Mac type")),
fExtensionListWatch(&fExtensionList,VString("Extensions")),
fMimeTypeListWatch(&fMimeTypeList,VString("Mime type"))
{
	fComponentType=inRef.fComponentType;
	fComponentSubType=inRef.fComponentSubType;
	fComponentName=inRef.fComponentName;
	fComponentInfo=inRef.fComponentInfo;
	fMimeTypeList=inRef.fMimeTypeList;
	fExtensionList=inRef.fExtensionList;
	fDescList=inRef.fDescList; 
	fComponentFlags=inRef.fComponentFlags;
}

VQT_IEComponentInfo::VQT_IEComponentInfo(OsType inType, OsType inSubType, const VString& inName, const VString& inInfo, OsType inDefaultMacType, OsType inDefaultMacCreator, OsType inDefaultFileExtension)
:fMacTypeListWatch(&fMacTypeList,VString("Mac type")),
fExtensionListWatch(&fExtensionList,VString("Extensions")),
fMimeTypeListWatch(&fMimeTypeList,VString("Mime type"))
{
	fComponentType = inType;
	fComponentSubType = inSubType;
	fComponentName = inName;
	fComponentInfo = inInfo;
	fComponentFlags = 0;
	
}
void VQT_IEComponentInfo::xDumpFlag(uLONG inFlags,uLONG inFlagsMask,const VString& FlagName,uLONG FlagValue, uBOOL OnlyIfTrue)
{
	if((inFlags & FlagValue)==FlagValue)
	{
		uBOOL ok= ((inFlags & FlagValue));
		DebugMsg("flag : %S = %i\r\n",&FlagName,inFlags & FlagValue);
	}
}
void VQT_IEComponentInfo::xDumpFlags(uLONG inFLags,uLONG inFlagsMask)
{
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportHandles",canMovieImportHandles);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportFiles",canMovieImportFiles);
	xDumpFlag(inFLags,inFlagsMask,"hasMovieImportUserInterface",hasMovieImportUserInterface);
	xDumpFlag(inFLags,inFlagsMask,"canMovieExportHandles",canMovieExportHandles);
	xDumpFlag(inFLags,inFlagsMask,"canMovieExportFiles",canMovieExportFiles);
	xDumpFlag(inFLags,inFlagsMask,"hasMovieExportUserInterface",hasMovieExportUserInterface);
	xDumpFlag(inFLags,inFlagsMask,"movieImporterIsXMLBased",movieImporterIsXMLBased);
	xDumpFlag(inFLags,inFlagsMask,"dontAutoFileMovieImport",dontAutoFileMovieImport);
	xDumpFlag(inFLags,inFlagsMask,"canMovieExportAuxDataHandle",canMovieExportAuxDataHandle);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportValidateFile",canMovieImportValidateFile);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportValidateHandles",canMovieImportValidateHandles);
	xDumpFlag(inFLags,inFlagsMask,"dontRegisterWithEasyOpen",dontRegisterWithEasyOpen);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportInPlace",canMovieImportInPlace);
	xDumpFlag(inFLags,inFlagsMask,"movieImportSubTypeIsFileExtension",movieImportSubTypeIsFileExtension);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportPartial",canMovieImportPartial);
	xDumpFlag(inFLags,inFlagsMask,"hasMovieImportMIMEList",hasMovieImportMIMEList);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportAvoidBlocking",canMovieImportAvoidBlocking);
	xDumpFlag(inFLags,inFlagsMask,"canMovieExportFromProcedures",canMovieExportFromProcedures);
	xDumpFlag(inFLags,inFlagsMask,"canMovieExportValidateMovie",canMovieExportValidateMovie);
	xDumpFlag(inFLags,inFlagsMask,"movieImportMustGetDestinationMediaType",movieImportMustGetDestinationMediaType);
	xDumpFlag(inFLags,inFlagsMask,"movieExportNeedsResourceFork",movieExportNeedsResourceFork);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportDataReferences",canMovieImportDataReferences);
	xDumpFlag(inFLags,inFlagsMask,"movieExportMustGetSourceMediaType",movieExportMustGetSourceMediaType);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportWithIdle",canMovieImportWithIdle);
	xDumpFlag(inFLags,inFlagsMask,"canMovieImportValidateDataReferences",canMovieImportValidateDataReferences);
	xDumpFlag(inFLags,inFlagsMask,"reservedForUseByGraphicsImporters",reservedForUseByGraphicsImporters);
}

VQT_IEComponentInfo::VQT_IEComponentInfo(QTComponentRef inComponentRef)
:fMacTypeListWatch(&fMacTypeList,VString("Mac type")),
fExtensionListWatch(&fExtensionList,VString("Extensions")),
fMimeTypeListWatch(&fMimeTypeList,VString("Mime type"))
{

	short err=-1;
	if (VQuicktime::GetVersion() >= 0x04000000)
	{		
		Handle	name = (Handle) VQuicktime::NewQTHandle(1); 
		Handle	info = (Handle) VQuicktime::NewQTHandle(1);
		
		*name[0] = 0;
		*info[0] = 0;
 
		ComponentDescription	description;
		description.componentFlags=0;
		description.componentFlagsMask=0xfffffff;
		OSErr	error = VQuicktime::GetComponentInfo(inComponentRef, &description, name, info, 0);
		if (error == noErr)
		{
			
			fComponentFlags=description.componentFlags;
			
			char tt[5],ts[5];
			tt[4]=0;
			ts[4]=0;
			
			memcpy(&tt,&description.componentType,4);
			memcpy(&ts,&description.componentSubType,4);
			ByteSwap((long*)&tt);
			ByteSwap((long*)&ts);
			
			fComponentType = description.componentType;
			fComponentSubType = description.componentSubType;
			
			
			
			fDescList.push_back(description);
			
			if(*name)
				fComponentName.FromBlock(&(*name)[1], (*name)[0], QT_CharSet);
			if(*info)
				fComponentInfo.FromBlock(&(*info)[1], (*info)[0], QT_CharSet);
			/*
			DebugMsg("component : %s %s name %S\r\n",tt,ts,&fComponentName);
			xDumpFlags(description.componentFlags,description.componentFlagsMask);
			DebugMsg("\r\n");
			*/
			ComponentInstance	instance = VQuicktime::OpenComponent(inComponentRef);
			if (instance != NULL )
			{
				if((description.componentFlags & movieImportSubTypeIsFileExtension) != movieImportSubTypeIsFileExtension)
				{
					xAppendMacType(description.componentSubType);
				}
				else
				{
					VString tmp;
					tmp.FromOsType(ByteSwapValue(description.componentSubType));
					tmp.RemoveWhiteSpaces(false,true);
					xAppendExtensions(tmp);
				}
				
				QTIME::QTAtomContainer	mimeInfo = NULL;
				
				if(fComponentType==GraphicsImporterComponentType)
					err=GraphicsImportGetMIMETypeList(instance,&mimeInfo);
				else if(fComponentType==GraphicsExporterComponentType)
				{
					QTIME::OSType ext,mactype,maccrea;
					err=QTIME::GraphicsExportGetDefaultFileNameExtension(instance,&ext);
					if(err==0)
					{
						VString tmp;
						tmp.FromOsType(ByteSwapValue(ext));
						tmp.RemoveWhiteSpaces(false,true);
						xAppendExtensions(tmp);
					}

					err=QTIME::GraphicsExportGetDefaultFileTypeAndCreator(instance,&mactype,&maccrea);
					if(err==0)
					{
						xAppendMacType(mactype);
					}
					err=0;
					err=GraphicsExportGetMIMETypeList(instance,&mimeInfo);
				}
				if(err==0)
				{
					sWORD	typesCount = VQuicktime::CountChildrenOfType(mimeInfo, QTIME::kParentAtomIsContainer, QTIME::kGraphicsExportMIMEType);	//kGraphicsExportGroup
					for (sLONG typeIndex = 1; typeIndex <= typesCount; typeIndex++) 
					{
						
						QTIME::QTAtom	infoAtom = QTIME::QTFindChildByIndex(mimeInfo, QTIME::kParentAtomIsContainer, QTIME::kGraphicsExportMIMEType, typeIndex, NULL);
						char myCString[256]; 
						long actualSize;
   
						err=QTCopyAtomDataToPtr( mimeInfo, infoAtom, true,sizeof(myCString)-1, myCString, &actualSize );
						if(err==noErr)
						{
							myCString[actualSize] = '\0';
							xAppendMimeType(VString(myCString));
						}
						infoAtom = QTIME::QTFindChildByIndex(mimeInfo, QTIME::kParentAtomIsContainer, QTIME::kGraphicsExportExtension, typeIndex, NULL);
						err = QTCopyAtomDataToPtr( mimeInfo, infoAtom, true,sizeof(myCString)-1, myCString, &actualSize );
						if(err==noErr)
						{
							myCString[actualSize] = '\0';
							xAppendExtensions(VString(myCString));
						}
					}
				}
				
				VQuicktime::CloseComponent(instance);
			}	
		}
		
		VQuicktime::DisposeQTHandle(name);
		VQuicktime::DisposeQTHandle(info);
	}

}

void VQT_IEComponentInfo::xAppendMimeType(const VString& inMimes)
{
	if(inMimes.GetLength())
	{
		VString tmp=inMimes;
		VArrayString outStrings;
		if(tmp.GetSubStrings(L',',outStrings))
		{
			for(long i=1;i<=outStrings.GetCount();i++)
			{
				outStrings.GetString(tmp,i);
				if(!FindMimeType(tmp))
				{
					fMimeTypeList.push_back(tmp);
				}
			}
		}
		else
		{
			if(!FindMimeType(inMimes))
			{
				fMimeTypeList.push_back(inMimes);
			}
		}
	}
}
void VQT_IEComponentInfo::xAppendExtensions(const VString& inExt)
{
	if(inExt.GetLength())
	{
		VString tmp=inExt;
		VArrayString outStrings;
		if(tmp.GetSubStrings(L',',outStrings))
		{
			for(long i=1;i<=outStrings.GetCount();i++)
			{
				outStrings.GetString(tmp,i);
				if(!FindExtension(tmp))
				{
					fExtensionList.push_back(tmp);
				}
			}
		}
		else
		{
			if(!FindExtension(inExt))
			{
				fExtensionList.push_back(inExt);
			}
		}
	}
}
void VQT_IEComponentInfo::xAppendMacType(const OsType inType)
{
	if(!FindMacType(inType))
		fMacTypeList.push_back(inType);
}
uBOOL VQT_IEComponentInfo::FindExtension(const VString& inExt)
{
	for( std::vector<VString>::iterator i = fExtensionList.begin() ; i != fExtensionList.end() ; ++i)
	{
		VString ext=*i;
		if(ext==inExt)
			return true;
	}
	return false;
}
uBOOL VQT_IEComponentInfo::FindMacType(const OsType inType)
{
	for( std::vector<OsType>::iterator i = fMacTypeList.begin() ; i != fMacTypeList.end() ; ++i)
	{
		OsType type=(*i);
		if(type==inType)
			return true;
	}
	return false;
}
uBOOL VQT_IEComponentInfo::FindMimeType(const VString& inMime)
{
	for( std::vector<VString>::iterator i = fMimeTypeList.begin() ; i != fMimeTypeList.end() ; ++i)
	{
		VString s=*i;
		if(s==inMime)
			return true;
	}
	return false;
}

void VQT_IEComponentInfo::xDump()
{
	VString tmp;
	
	tmp.FromOsType(ByteSwapValue(fComponentType));
	DebugMsg("component type : \t%S\r\n",&tmp);
	DebugMsg("component name : \t%S\r\n",&fComponentName);
	DebugMsg("component info : \t%S\r\n",&fComponentInfo);
	
	DebugMsg("component desc list : \t%i\r\n",fDescList.size());
	
	DebugMsg("mime type :\r\n");
	
	for( std::vector<VString>::iterator i = fMimeTypeList.begin() ; i != fMimeTypeList.end() ; ++i)
	{
		tmp=*i;
		DebugMsg("\t %S\r\n",&tmp);
	}
	
	DebugMsg("extensions \r\n");
	
	for( std::vector<VString>::iterator i = fExtensionList.begin() ; i != fExtensionList.end() ; ++i)
	{
		tmp=*i;
		DebugMsg("\t %S\r\n",&tmp);
	}
	
	DebugMsg("mac type :\r\n");
	
	for( std::vector<OsType>::iterator i = fMacTypeList.begin() ; i != fMacTypeList.end() ; ++i)
	{
		tmp.FromOsType(ByteSwapValue(*i));
		DebugMsg("\t '%S'\r\n",&tmp);
	}
	
}

sLONG VQT_IEComponentInfo::IW_CountProperties()
{
	return 5;
}
void VQT_IEComponentInfo::IW_GetPropName(sLONG inPropID,VValueSingle& outName)
{
	switch(inPropID)
	{
		case 0:
		{
			VString tmp;
			GetComponentName(tmp);
			outName.FromString(tmp);
			break;
		}
		case 1:
		{
			VString tmp("SubType");
			outName.FromString(tmp);
			break;
		}
	}
}
void VQT_IEComponentInfo::IW_GetPropValue(sLONG inPropID,VValueSingle& outValue)
{
	VString tmp;
	switch(inPropID)
	{
		case 0:
		{
			
			GetComponentInfo(tmp);
			outValue.FromString(tmp);
			break;
		}
		case 1:
		{
			uLONG l=fComponentSubType;
			#if SMALLENDIAN
			ByteSwapLong(&l);
			#endif

			tmp.FromOsType(l);
			outValue.FromString(tmp);
		}
	}
}
void VQT_IEComponentInfo::IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild)
{
	switch(inPropID)
	{
		case 2: // mime
			*outChild=&fMimeTypeListWatch;
			break;
		case 3: // extension
			*outChild=&fExtensionListWatch;
			break;
		case 4: // mac type
			*outChild=&fMacTypeListWatch;
			break;
	}
}

VQT_IEComponentInfo::~VQT_IEComponentInfo()
{
}

void VQT_IEComponentInfo::UpdateComponentInfo(VQT_IEComponentInfo& inComp)
{
	VString tmp;
	inComp.GetComponentName(tmp);
	assert(tmp==fComponentName);
	assert(inComp.fComponentType==fComponentType);
	
	for( std::vector<VString>::iterator i = inComp.fMimeTypeList.begin() ; i != inComp.fMimeTypeList.end() ; ++i)
	{
		xAppendMimeType((*i));
	}
	for( std::vector<VString>::iterator i = inComp.fExtensionList.begin() ; i != inComp.fExtensionList.end() ; ++i)
	{
		xAppendExtensions((*i));
	}
	for( std::vector<OsType>::iterator i = inComp.fMacTypeList.begin() ; i != inComp.fMacTypeList.end() ; ++i)
	{
		xAppendMacType((*i));
	}
	assert(inComp.fDescList.size()==1);
	for( std::vector<QTIME::ComponentDescription>::iterator i = inComp.fDescList.begin() ; i != inComp.fDescList.end() ; ++i)
	{
		fDescList.push_back(*i);
	}
}

VError VQuicktime::GraphicsImportGetImageDescription(QTImportComponentRef inComp,QTImageDescriptionHandle *desc )
{
	if (!HasQuicktime()) return VE_CANNOT_INITIALIZE_APPLICATION;
	
	::GraphicsImportGetImageDescription(inComp,desc );

	return VE_OK;
}


VQT_IEComponentManager::VQT_IEComponentManager()
{
	fInited=false;
}
VQT_IEComponentManager::~VQT_IEComponentManager()
{
	for( std::vector<VQT_IEComponentInfo*>::iterator i = fComponentList.begin() ; i != fComponentList.end() ; ++i)
	{
		delete (*i);
	}
}
void VQT_IEComponentManager::Init(sLONG inComponentKind)
{

	fKind = inComponentKind;
	
	QTIME::ComponentDescription description;
	description.componentType = fKind;
	description.componentSubType = 0;
	description.componentManufacturer = 0;
	description.componentFlags = 0;
	description.componentFlagsMask = 1 <<12;//QTIME::graphicsExporterIsBaseExporter;
		
		
	sLONG nbcomp=QTIME::CountComponents(&description);
		
	QTIME::Component	component = NULL;
	VString tmp;
	tmp.FromOsType(ByteSwapValue(fKind));
		
	while ((component = VQuicktime::FindNextComponent(component, &description)) != NULL)
	{
		VString tmpcompname;
		VQT_IEComponentInfo* comp;
		VQT_IEComponentInfo tmpcomp(component);
		tmpcomp.GetComponentName(tmpcompname);
		comp=FindComponementInfoByName(tmpcompname);
		if(comp)
		{
			comp->UpdateComponentInfo(tmpcomp);
		}
		else
		{
			fComponentList.push_back(new VQT_IEComponentInfo(tmpcomp));
		}
	}
	/*
	for( std::vector<VQT_IEComponentInfo*>::iterator i = fComponentList.begin() ; i != fComponentList.end() ; ++i)
	{
		(*i)->xDump();
		DebugMsg("\r\n");
	}
	*/

}

VQT_IEComponentInfo* VQT_IEComponentManager::FindComponementInfoByName(const VString& inName)
{
	for( std::vector<VQT_IEComponentInfo*>::iterator i = fComponentList.begin() ; i != fComponentList.end() ; ++i)
	{
		VString compname;
		(*i)->GetComponentName(compname);
		if(compname==inName)
			return *i;
	}
	return 0;
}

class VQT_IEComponentInfo*	VQT_IEComponentManager::FindComponentByMimeType(const VString& inMime)
{
	for( std::vector<VQT_IEComponentInfo*>::iterator i = fComponentList.begin() ; i != fComponentList.end() ; ++i)
	{
		VString compname;
		if((*i)->FindMimeType(inMime))
		{
			return *i;
		}
	}
	return 0;
}
class VQT_IEComponentInfo*	VQT_IEComponentManager::FindComponentByExtension(const VString& inExt)
{
	for( std::vector<VQT_IEComponentInfo*>::iterator i = fComponentList.begin() ; i != fComponentList.end() ; ++i)
	{
		VString compname;
		if((*i)->FindExtension(inExt))
		{
			return *i;
		}
	}
	return 0;
}
class VQT_IEComponentInfo*	VQT_IEComponentManager::FindComponentByMacType(const OsType inType)
{
	for( std::vector<VQT_IEComponentInfo*>::iterator i = fComponentList.begin() ; i != fComponentList.end() ; ++i)
	{
		VString compname;
		if((*i)->FindMacType(inType))
		{
			return *i;
		}
	}
	return 0;
}
void VQT_IEComponentManager::GetNthComponentName(sLONG index,VString & outName)
{
	if(index >= 0 && index <(sLONG)fComponentList.size())
	{
		fComponentList[index]->GetComponentName(outName);
	}
	else
	{
		outName=L"";
	}
}
VQT_IEComponentInfo* VQT_IEComponentManager::GetNthComponent(sLONG index)
{
	VQT_IEComponentInfo* result=NULL;
	if(index >= 0 && index <(sLONG)fComponentList.size())
	{
		result = fComponentList[index];
	}
	return result;
}
sLONG VQT_IEComponentManager::IW_CountProperties()
{
	return GetCount()+1;
}
void VQT_IEComponentManager::IW_GetPropName(sLONG inPropID,VValueSingle& outName)
{
	switch(inPropID)
	{
		case 0:
		{
			if(fKind=='grip')
				outName.FromString("Importer");
			else if(fKind=='grex')
				outName.FromString("Exporter");
			else
				outName.FromString("???????");
		}
	}
}
void VQT_IEComponentManager::IW_GetPropValue(sLONG inPropID,VValueSingle& outValue)
{
	switch(inPropID)
	{
		case 0:
		{
			outValue.FromLong(GetCount());
			break;
		}
	}
}
void VQT_IEComponentManager::IW_GetPropInfo(sLONG inPropID,bool& outEditable,IWatchable_Base** outChild)
{
	if(inPropID>0)
	{
		outEditable=false;
		*outChild=GetNthComponent(inPropID-1);
	}
}

#endif