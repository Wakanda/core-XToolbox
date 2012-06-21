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
#include "VKernelPrecompiled.h"
#include "VArchiveStream.h"
#include "VStream.h"
#include "VErrorContext.h"

#if VERSIONMAC
#include <mach-o/loader.h>
#endif

VArchiveCatalog::VArchiveCatalog( VFile *inFile, sLONG8 inDataFileSize, sLONG8 inResFileSize, VString &inStoredPath, VString &inFileExtra, uLONG inKind, uLONG inCreator )
{
	inFile->Retain();
	fFile = inFile;
	fDataFileSize = inDataFileSize;
	fResFileSize = inResFileSize;
	fStoredPath = inStoredPath;
	fExtra = inFileExtra;
	fExtract = true;
	fKind = inKind;
	fCreator = inCreator;
}

VArchiveCatalog::~VArchiveCatalog()
{
	fFile->Release();
}

VFile* VArchiveCatalog::GetFile()
{
	return fFile;
}

sLONG8 VArchiveCatalog::GetFileSize( eFileSizeType inSizeType )
{
	sLONG8 filereSize = 0;
	if ( inSizeType | fst_Data )
		filereSize += fDataFileSize;
	if ( inSizeType | fst_Resource )
		filereSize += fResFileSize;
	return filereSize;
}

VString& VArchiveCatalog::GetStoredPath()
{
	return fStoredPath;
}

VString& VArchiveCatalog::GetExtra()
{
	return fExtra;
}

Boolean VArchiveCatalog::GetExtractFlag()
{
	return fExtract;
}

void VArchiveCatalog::SetExtractFlag( Boolean inExtract )
{
	fExtract = inExtract;
}

uLONG VArchiveCatalog::GetKind()
{
	return fKind;
}

uLONG VArchiveCatalog::GetCreator()
{
	return fCreator;
}

VArchiveStream::VArchiveStream()
{
	fDestinationFile = NULL;
	fStream = NULL;
	fCallBack = NULL;
}

VArchiveStream::~VArchiveStream()
{
	if ( fDestinationFile )
		fDestinationFile->Release();
}

VError VArchiveStream::AddFileDesc( VFileDesc* inFileDesc, const VString &inExtra )
{
	if ( (inFileDesc != NULL) && !ContainsFile( inFileDesc->GetParentVFile()) )
	{
		fFileDescList.push_back( inFileDesc );
		fFileDescExtra.push_back( inExtra );
	}
	return VE_OK;
}

VError VArchiveStream::AddFile( VFile* inFile )
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE ;
	VString empty;
	result = AddFile( inFile, empty );
	return result;
}

VError VArchiveStream::AddFile( VFile* inFile, const VString &inExtra )
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE ;
	if ( inFile->Exists() )
	{
		if (!ContainsFile( inFile))
		{
			if ( fRelativeFolder.IsEmpty() )
			{
				XBOX::VFilePath parentPath;
				inFile->GetPath().GetParent( parentPath );
				fRelativeFolder = parentPath;
			}

			fFileExtra.push_back( inExtra );
			fFileList.push_back( inFile );
		}
		result = VE_OK;
	}
	return result;
}

VError VArchiveStream::AddFile( VFilePath &inFilePath )
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE ;
	VFile *file = new VFile( inFilePath );
	result = AddFile( file );
	file->Release();
	return result;
}

VError VArchiveStream::AddFolder( VFolder* inFolder )
{
	/**
	 * ACI0076147 folder's existence to report error to caller
	 */
	VError result = VE_OK;
	if ( inFolder->Exists() )
	{	
		VFolder *parentFolder = inFolder->RetainParentFolder();
		VString parentFolderPath ( parentFolder->GetPath().GetPath() ); 
		parentFolder->Release();
		for( VFolderIterator folder(inFolder); folder.IsValid() && result == VE_OK; ++folder )
			result = AddFolder( folder.Current(), parentFolderPath );
		for( VFileIterator file(inFolder); file.IsValid() && result == VE_OK; ++file )
			result = AddFile( file.Current(), parentFolderPath );
	}	
	else {
		result = VE_STREAM_CANNOT_FIND_SOURCE;
		
	}

	return result;
}

VError VArchiveStream::AddFolder( VFolder* inFolder, const VString &inExtraInfo )
{
	VError result = VE_OK ;

	if ( fRelativeFolder.IsEmpty() )
	{
		XBOX::VFilePath parentPath;
		inFolder->GetPath().GetParent( parentPath );
		fRelativeFolder = parentPath;
	}

	for( VFolderIterator folder(inFolder); folder.IsValid() && result == VE_OK; ++folder )
		result = AddFolder( folder.Current(), inExtraInfo );
	for( VFileIterator file(inFolder); file.IsValid() && result == VE_OK; ++file )
		result = AddFile( file.Current(), inExtraInfo );
	return result;
}

VError VArchiveStream::AddFolder( VFilePath &inFileFolder )
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE;
	VFolder *folder = new VFolder(inFileFolder);
	result = AddFolder( folder );
	folder->Release();
	return result;
}


bool VArchiveStream::ContainsFile( const VFile* inFile) const
{
	if (inFile == NULL)
		return true;	// not add null files

	bool found = false;

	for( VectorOfVFileDesc::const_iterator i = fFileDescList.begin() ; (i != fFileDescList.end()) && !found ; ++i)
		found = inFile->IsSameFile( (*i)->GetParentVFile());

	for( VectorOfVFile::const_iterator i = fFileList.begin() ; (i != fFileList.end()) && !found ; ++i)
		found = inFile->IsSameFile( *i);

	return found;
}


void VArchiveStream::SetRelativeFolderSource( VFilePath &inFolderPath )
{
	fRelativeFolder = inFolderPath;
}

void VArchiveStream::SetDestinationFile( VFile* inFile )
{
	fDestinationFile = inFile;
	fDestinationFile->Retain();
}

void VArchiveStream::SetStreamer( VStream* inStream )
{
	fStream = inStream;
}

void VArchiveStream::SetProgressCallBack( CB_VArchiveStream inProgressCallBack )
{
	fCallBack = inProgressCallBack;
}

VError VArchiveStream::_WriteFile( const VFileDesc* inFileDesc, char* buffToUse, VSize buffSize, uLONG8 &ioPartialByteCount, uLONG8 inTotalByteCount )
{
	bool userAbort = false;
	VSize byteCount = 0;
	sLONG8 offset = 0;
	sLONG8 fileSize = inFileDesc->GetSize();
	VError result = fStream->PutLong8(fileSize);

	while ( offset < fileSize && result == VE_OK )
	{
		uLONG8 wByteCount = fileSize - offset;
		if ( wByteCount > (uLONG8)buffSize )
			byteCount = buffSize;
		else
			byteCount = (VSize)wByteCount;

		result = inFileDesc->GetData(buffToUse,byteCount,offset);
		if ( result == VE_OK )
		{
			result = fStream->PutData(buffToUse,byteCount);
			ioPartialByteCount += byteCount;
			offset += byteCount;
		}

		if ( fCallBack )
		{
			fCallBack(CB_UpdateProgress,ioPartialByteCount,inTotalByteCount,userAbort);
			if ( userAbort )
				result = VE_STREAM_USER_ABORTED;
		}
	}

	return result;
}

VError VArchiveStream::_WriteCatalog( const VFile* inFile, const VString &inExtraInfo, uLONG8 &ioTotalByteCount )
{
	VString fileInfo;
	VStr8 slash("/");
	VStr8 extra("::");
	VStr8 folderSep(XBOX::FOLDER_SEPARATOR);

	if (!inFile->GetPath().GetRelativePath(fRelativeFolder,fileInfo))
	{
		if ( !inExtraInfo.IsEmpty() )
		{
			VString basePathString( inExtraInfo);
			if ( basePathString.GetUniChar( basePathString.GetLength()) != FOLDER_SEPARATOR )
				basePathString += FOLDER_SEPARATOR;
			
			if ( !inFile->GetPath().GetRelativePath( VFilePath( basePathString),fileInfo))
			{
				VFolder *folder = inFile->RetainParentFolder();
				if ( folder )
				{
					VString fileName;
					folder->GetName(fileInfo);
					// folder name can be a drive letter like Z:
					fileInfo.Exchange( (UniChar) ':', (UniChar) '_');

					inFile->GetName(fileName);
					fileInfo += slash;
					fileInfo += fileName;
					fileInfo.Insert( slash, 1 );
					folder->Release();
				}
			}
			else
			{
				fileInfo.Insert( FOLDER_SEPARATOR, 1);
			}
		}
	}
	else
	{
		fileInfo.Insert( FOLDER_SEPARATOR, 1);
	}

	fileInfo.Exchange(folderSep ,slash, 1, 255);
	if ( fileInfo.GetUniChar(1) == '/' )
		fileInfo.Insert( '.', 1 );

	fileInfo += extra;
	fileInfo += inExtraInfo;
	VError result = fStream->PutLong('file');
	if ( result == VE_OK )
		result = fileInfo.WriteToStream(fStream);
	if ( result == VE_OK )
	{
		sLONG8 fileSize = 0;
		inFile->GetSize(&fileSize);
		fStream->PutLong8(fileSize);
		ioTotalByteCount += fileSize;
#if VERSIONWIN || VERSION_LINUX
		/* rez file size */
		fStream->PutLong8(0);
		/* no file kind or creator under Windows */
		fStream->PutLong(0); /* kind */
		fStream->PutLong(0); /* creator */
#elif VERSIONMAC
		inFile->GetResourceForkSize(&fileSize);
		fStream->PutLong8(fileSize);
		ioTotalByteCount += fileSize;

		OsType osType;
		inFile->MAC_GetKind( &osType );
		result = fStream->PutLong( osType );
		inFile->MAC_GetCreator( &osType );
		result = fStream->PutLong( osType );
#endif
	}
	return result;
}

VError VArchiveStream::Proceed()
{
	VError result = VE_STREAM_NOT_OPENED;
	if ( fStream )
	{
		bool userAbort = false;
		result = fStream->OpenWriting();
		if ( result == VE_OK )
		{
			sLONG8 offset;
			sLONG8 fileSize;
			sLONG8 byteCount;
			uLONG8 totalByteCount = 0;
			uLONG8 partialByteCount = 0;

			/* put the backup file signature */
			fStream->PutLong('FPBK');
			/* put the current version of the file */
			fStream->PutByte(4);
			/* put the number of file stored in this archive */
			fStream->PutLong8((sLONG8)(fFileList.size()+fFileDescList.size()));
			
			/* put the header of the file listing */
			fStream->PutLong('LIST');
			for ( uLONG i = 0; i < fFileDescList.size() && result == VE_OK; i++ )
			{
				result = _WriteCatalog(fFileDescList[i]->GetParentVFile(),fFileDescExtra[i],totalByteCount);
			}
			for ( uLONG i = 0; i < fFileList.size() && result == VE_OK; i++ )
			{
				result = _WriteCatalog(fFileList[i],fFileExtra[i],totalByteCount);
			}

			if ( fCallBack )
				fCallBack(CB_OpenProgress,partialByteCount,totalByteCount,userAbort);

			VSize bufferSize = 1024*1024;
			char *buffer = new char[bufferSize];

			/* writing file descriptor that we have to the archive files */
			/* nota : filedesc passed to the archivestream is considered as data fork */
			for( VectorOfVFileDesc::iterator i = fFileDescList.begin() ; result == VE_OK && i != fFileDescList.end() ; ++i)
			{
				StErrorContextInstaller errors( false);
				result = fStream->PutLong('Fdat');
				if ( result == VE_OK )
				{
					result = (*i)->SetPos(0);
					if ( result == VE_OK )
						result = _WriteFile( (*i), buffer, bufferSize, partialByteCount, totalByteCount );
				}

				if ( result == VE_OK )
					result = fStream->PutLong('Frez');

				if ( result == VE_OK )
					result = fStream->PutLong8(0);

			}

			for( VectorOfVFile::iterator i = fFileList.begin() ; result == VE_OK && i != fFileList.end() ; ++i )
			{
				StErrorContextInstaller errors( false);
				VFileDesc *fileDesc = NULL;

				result = fStream->PutLong('Fdat');
				if ( result == VE_OK )
				{
					result = (*i)->Open(FA_READ,&fileDesc);
					if ( fileDesc )
					{
						result = _WriteFile( fileDesc, buffer, bufferSize, partialByteCount, totalByteCount );
						delete fileDesc;
						fileDesc = NULL;
					}
					else
					{
						result = fStream->PutLong8(0);
					}
				}

				if ( result == VE_OK )
					result = fStream->PutLong('Frez');

				if ( result == VE_OK )
				{
#if VERSIONMAC
					result = (*i)->Open(FA_READ,&fileDesc,FO_OpenResourceFork);
					if ( fileDesc )
					{
						result = _WriteFile( fileDesc, buffer, bufferSize, partialByteCount, totalByteCount );
						delete fileDesc;
						fileDesc = NULL;
					}
					else
#endif
					{
						result = fStream->PutLong8(0);
					}
				}
			}
			delete[] buffer;

			if ( fCallBack )
				fCallBack(CB_CloseProgress,partialByteCount,totalByteCount,userAbort);

			fStream->CloseWriting();
		}
	}
	return result;
}

VArchiveUnStream::VArchiveUnStream()
{
	fCallBack = NULL;
	fSourceFile = NULL;
}

VArchiveUnStream::~VArchiveUnStream()
{
	if ( fSourceFile )
		fSourceFile->Release();

	for ( ArchiveCatalog::iterator i = fFileCatalog.begin() ; i != fFileCatalog.end() ; ++i )
		delete *i;
}

void VArchiveUnStream::SetSourceFile( VFile* inFileSource )
{
	if ( fSourceFile )
		fSourceFile->Release();
	fSourceFile = inFileSource;
	fSourceFile->Retain();
}

void VArchiveUnStream::SetDestinationFolder( VFolder* inFolder )
{
	fDestinationFolder = inFolder->GetPath();
}

void VArchiveUnStream::SetDestinationFolder( VFilePath &inFolder )
{
	fDestinationFolder = inFolder;
}

void VArchiveUnStream::SetStreamer( VStream* inStream )
{
	fStream = inStream;
}

void VArchiveUnStream::SetProgressCallBack( CB_VArchiveStream inProgressCallBack )
{
	fCallBack = inProgressCallBack;
}

VError VArchiveUnStream::ProceedCatalog()
{
	VError result = VE_OK;
	VStr8 dotSlash("./");
	VStr8 slash("/");
	VStr8 extra("::");
	VStr8 folderSep(XBOX::FOLDER_SEPARATOR);

	if ( fStream->GetLong() == 'FPBK' )
	{
		VString filePath;
		VString fileExtra;
		VString storedPath;
		uLONG kind = 0;
		uLONG creator = 0;
		sLONG8 dataFileSize = 0;
		sLONG8 resFileSize = 0;
		uBYTE version = fStream->GetByte();
		uLONG8 fileCount = fStream->GetLong8();
		fTotalByteCount = 0;


		if ( fStream->GetLong() == 'LIST' )
		{
			for ( uLONG i = 0; i < fileCount && result == VE_OK; i++ )
			{
				if ( fStream->GetLong() == 'file' )
				{
					result = filePath.ReadFromStream(fStream);
					if ( result == VE_OK )
					{
						VIndex index = filePath.Find(extra);
						fileExtra = filePath;
						fileExtra.Remove(1,index+1);
						filePath.Remove(index,filePath.GetLength()-index+1);
						storedPath = filePath;
						if ( filePath.Find( dotSlash ) > 0 )
						{
							// some archives have bogous file paths such as ./Z:/folder
							filePath.Exchange( (UniChar) ':', (UniChar) '_');
							filePath.Replace(fDestinationFolder.GetPath(),1,2);
						}
						filePath.Exchange(slash ,folderSep, 1, 255);
					
						dataFileSize = fStream->GetLong8();
						fTotalByteCount += dataFileSize;
						resFileSize = fStream->GetLong8();
						fTotalByteCount += resFileSize;

						kind = fStream->GetLong();
						creator = fStream->GetLong();

						VFile *file = new VFile(filePath);
						fFileCatalog.push_back(new VArchiveCatalog(file,dataFileSize,resFileSize,storedPath,fileExtra,kind,creator));
						ReleaseRefCountable( &file);
					}
				}
				else
					result = VE_STREAM_BAD_SIGNATURE;
			}
		}
		else
			result = VE_STREAM_BAD_SIGNATURE;
	}
	return result;
}

#if VERSIONMAC
bool VArchiveUnStream::_IsExecutable(char* buffToUse, VSize buffSize)
{
	bool result = false;

	if(buffSize >= sizeof(mach_header))
	{
		mach_header* machoptr = (mach_header*)buffToUse;
		if(machoptr->magic == MH_MAGIC)
		{
			switch (machoptr->cputype) {
				case CPU_TYPE_ANY:
				case CPU_TYPE_VAX:
				case CPU_TYPE_MC680x0:
				case CPU_TYPE_X86:
				case CPU_TYPE_X86_64:
				case CPU_TYPE_MC98000:
				case CPU_TYPE_HPPA:
				case CPU_TYPE_ARM:
				case CPU_TYPE_MC88000:
				case CPU_TYPE_SPARC:
				case CPU_TYPE_I860:
				case CPU_TYPE_POWERPC:
				case CPU_TYPE_POWERPC64:
					result = true;
					break;
				default:
					//Assert, deux cause possible. Nouveau type de cpu ou le fichier n est pas un executable mais il a le meme magic number
					result = true;
					break;
			}
		}
	}
	
	return result;
}
#endif	

VError VArchiveUnStream::_ExtractFile( VArchiveCatalog *inCatalog, const VFileDesc* inFileDesc, char* buffToUse, VSize buffSize, uLONG8 &ioPartialByteCount )
{
	VError result = VE_OK;
	bool userAbort = false;
	sLONG8 offset = 0;
	sLONG8 byteCount = 0;
	bool firstTime = true;
	sLONG8 fileSize = fStream->GetLong8();

	while ( offset < fileSize && result == VE_OK )
	{
		byteCount = fileSize - offset;
		//Cast pour eviter le warning C4018
		if ( byteCount > ((sLONG8)buffSize ))
			byteCount = buffSize;
		result = fStream->GetData(buffToUse,(VSize) byteCount);
		if ( result == VE_OK )
		{
			if ( inCatalog->GetExtractFlag() )
			{
#if VERSIONMAC
				if(firstTime)
				{
					firstTime = false;
					if(_IsExecutable(buffToUse, buffSize))
					{
						uWORD mode;
						inCatalog->GetFile()->MAC_GetPermissions( &mode );
						mode |= 0111;
						inCatalog->GetFile()->MAC_SetPermissions(mode);
					}
					
				}
#endif			
				result = inFileDesc->PutData(buffToUse,(VSize)byteCount,offset);
			}
			ioPartialByteCount += byteCount;
			offset += byteCount;
		}

		if ( fCallBack )
		{
			fCallBack(CB_UpdateProgress,ioPartialByteCount,fTotalByteCount,userAbort);
			if ( userAbort )
				result = VE_STREAM_USER_ABORTED;
		}
	}
	return result;
}

VError VArchiveUnStream::ProceedFile()
{
	VError result = VE_OK;
	bool userAbort = false;
	uLONG8 partialByteCount = 0;

	if ( fCallBack )
		fCallBack(CB_OpenProgress,partialByteCount,fTotalByteCount,userAbort);

	VFileDesc *fileDesc = NULL;
	VSize bufferSize = 1024*1024;
	char *buffer = new char[bufferSize];
	for ( uLONG i = 0; i < fFileCatalog.size() && result == VE_OK; i++ )
	{
		fileDesc = NULL;
		if ( fStream->GetLong() == 'Fdat' )
		{
			if ( fFileCatalog[i]->GetExtractFlag() )
			{
				VFolder *parentFolder = fFileCatalog[i]->GetFile()->RetainParentFolder();
				if ( parentFolder )
				{
					result = parentFolder->CreateRecursive();
					parentFolder->Release();
				}
				if ( result == VE_OK )
				{
					result = fFileCatalog[i]->GetFile()->Open( FA_SHARED, &fileDesc, FO_CreateIfNotFound);
#if VERSIONMAC
					fFileCatalog[i]->GetFile()->MAC_SetKind(fFileCatalog[i]->GetKind());
					fFileCatalog[i]->GetFile()->MAC_SetCreator(fFileCatalog[i]->GetCreator());
#endif
				}
			}
			if ( result == VE_OK )
			{
				result = _ExtractFile( fFileCatalog[i], fileDesc, buffer, bufferSize, partialByteCount );
				delete fileDesc;
			}
			if ( result == VE_OK )
			{
				fileDesc = NULL;
				if ( fStream->GetLong() == 'Frez' )
				{
					if ( fFileCatalog[i]->GetExtractFlag() )
					{
						#if VERSIONMAC
						result = fFileCatalog[i]->GetFile()->Open( FA_SHARED, &fileDesc, FO_OpenResourceFork);
						#endif
					}
					if ( result == VE_OK )
					{
						result = _ExtractFile( fFileCatalog[i], fileDesc, buffer, bufferSize, partialByteCount );
						delete fileDesc;
					}
				}
				else
				{
					result = VE_STREAM_BAD_SIGNATURE;
				}
			}
		}
		else
		{
			result = VE_STREAM_BAD_SIGNATURE;
		}
	}
	delete[] buffer;

	if ( fCallBack )
		fCallBack(CB_CloseProgress,partialByteCount,fTotalByteCount,userAbort);

	return result;
}

VError VArchiveUnStream::Proceed()
{
	VError result = VE_STREAM_NOT_OPENED;
	if ( fStream )
	{
		result = fStream->OpenReading();
		if ( result == VE_OK )
			result = ProceedCatalog();
		if ( result == VE_OK )
			result = ProceedFile();
		fStream->CloseReading();
	}
	return result;
}

ArchiveCatalog* VArchiveUnStream::GetCatalog()
{
	return &fFileCatalog;
}
