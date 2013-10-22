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

const char* PRESERVE_PARENT_OPTION = "preserve_parent";

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
	fUniqueFilesCollection = NULL;

	fUniqueFilesCollection = new SetOfVFilePath();
}

VArchiveStream::~VArchiveStream()
{
	fFolderPathList.clear();
	fFolderExtra.clear();
	fSourceFolderForFolders.clear();
	fSourceFolderForFiles.clear();
	fSourceFolderForFileDescs.clear();

	if (fUniqueFilesCollection)
		fUniqueFilesCollection->clear();
	delete fUniqueFilesCollection;
	fUniqueFilesCollection = NULL;
	if ( fDestinationFile )
		fDestinationFile->Release();
}

VError VArchiveStream::AddFileDesc( VFileDesc* inFileDesc, const VString &inExtra )
{
	if ( (inFileDesc != NULL)  )
	{
		const VFile* file = inFileDesc->GetParentVFile();
		if (!ContainsFile( file))
		{
			fUniqueFilesCollection->insert(file->GetPath());
			fFileDescList.push_back( inFileDesc );
			fFileDescExtra.push_back( inExtra );
			fSourceFolderForFileDescs.push_back(fRelativeFolder);
		}
	}
	return VE_OK;
}



VError VArchiveStream::AddFile( VFile* inFile, const VString &inExtra )
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE ;
	result = _AddFile(*inFile,fRelativeFolder,inExtra);

	return result;
}

/*
  ACI0080117: Jan 24th 2013, O.R., restore the possibility of preserving
  parent folder info for extraction
*/
VError VArchiveStream::AddFile( VFile* inFile,bool inPreserveParentFolder)
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE ;
	VString extraInfo;
	if (inPreserveParentFolder)
	{
		//We use bogus extra info to forward this to _WriteCatalog() later on
		extraInfo = PRESERVE_PARENT_OPTION;
	}
	result = _AddFile(*inFile,fRelativeFolder,extraInfo);
	return result;
}

VError VArchiveStream::AddFile( VFilePath &inFilePath )
{
	VError result = VE_STREAM_CANNOT_FIND_SOURCE ;
	VString empty;
	VFile *file = new VFile( inFilePath );
	result = _AddFile(*file,fRelativeFolder,empty);
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
		///ACI0077162, Jul 11 2012, O.R.: lazily add path of folder. Proceed() will eventually recurse thru the tree
		result = AddFolder( inFolder, CVSTR("") );
	}	
	else {
		result = VE_STREAM_CANNOT_FIND_SOURCE;
		
	}

	return result;
}

VError VArchiveStream::AddFolder( VFolder* inFolder, const VString &inExtraInfo )
{
	VError result = VE_OK ;

	xbox_assert(!fRelativeFolder.IsEmpty());
	if ( fRelativeFolder.IsEmpty())
	{
		XBOX::VFilePath parentPath;
		inFolder->GetPath().GetParent( parentPath );
		fRelativeFolder = parentPath;
	}

	///ACI0077162, Jul 11 2012, O.R.: lazily add path of folder. Proceed() will eventually recurse thru the tree
	fFolderPathList.push_back(inFolder->GetPath());
	fFolderExtra.push_back(inExtraInfo);
	fSourceFolderForFolders.push_back(fRelativeFolder);
	
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
	if (fUniqueFilesCollection == NULL)
		return false;

	bool found = false;
	
	//ACI0077162, Jul 11th 2012, O.R.: use a set instead of vector to check for duplicates, to decrease search time
	 
	SetOfVFilePath::iterator it = fUniqueFilesCollection->find(inFile->GetPath());
	found = (it != fUniqueFilesCollection->end());
	return found;
}


void VArchiveStream::SetRelativeFolderSource( const VFilePath &inFolderPath )
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

VError VArchiveStream::_AddFile(VFile& inFile,const VFilePath& inSourceFolder,const VString& inExtraInfo)
{
	XBOX::VError result = VE_FILE_NOT_FOUND;
	if ( inFile.Exists() )
	{
		///ACI0078887, O.R. Nov 29th 2012: mask error if file is already known, does no harm 
		result = VE_OK;
		if (!ContainsFile( &inFile))
		{
			VString relativePath;
			if(testAssert(!inSourceFolder.IsEmpty() && inFile.GetPath().GetRelativePath(inSourceFolder,relativePath)))
			{
				fUniqueFilesCollection->insert(inFile.GetPath());
				fFileExtra.push_back( inExtraInfo );
				fFileList.push_back( &inFile );
				fSourceFolderForFiles.push_back(inSourceFolder);	
			}
			else
			{
				///ACI0078887, O.R. Nov 29th 2012: explicitely fail if item is not relative to source folder, this should never happen
				result = VE_INVALID_PARAMETER;
			}
		}
	}
	return result;
}

VError VArchiveStream::_AddOneFolder(VFolder& inFolder,const VFilePath& inSourceFolder,const VString& inExtraInfo)
{
	VError error = VE_OK;
	bool userAbort = false;
	
	
	///ACI0078887, O.R. Nov 29th 2012: ensure alias/link files are added to the archive, NOT their target
	for( VFileIterator file(&inFolder,FI_WANT_FILES); file.IsValid() && error == VE_OK && !userAbort; ++file )
	{
		error = _AddFile( *(file.Current()),inSourceFolder,inExtraInfo );
		if(fCallBack)
		{
			fCallBack(CB_UpdateProgress,0,0,userAbort);
		}
	}

	///ACI0078887, O.R. Nov 29th 2012: ensure alias/link folders are added to the archive, NOT their target
	for( VFolderIterator folder(&inFolder,FI_WANT_FOLDERS); folder.IsValid() && error == VE_OK && !userAbort; ++folder )
	{
		error = _AddOneFolder( static_cast<VFolder&>(folder),inSourceFolder, inExtraInfo );
		if(fCallBack)
		{
			fCallBack(CB_UpdateProgress,0,0,userAbort);
		}
	}
	if (userAbort)
		error = VE_STREAM_USER_ABORTED;

	return error;
}

//ACI0078887 23rd Nov 2012, O.R., always specify folder used for computing relative path of entry in TOC
VError VArchiveStream::_WriteCatalog( const VFile* inFile, const XBOX::VFilePath& inSourceFolder,const VString &inExtraInfo, uLONG8 &ioTotalByteCount )
{
	VString fileInfo;
	VStr8 slash("/");
	VStr8 extra("::");
	VStr8 folderSep(XBOX::FOLDER_SEPARATOR);
	VError result = VE_INVALID_PARAMETER;
	XBOX::VString savedExtraInfo = inExtraInfo;

	//ACI0078887 23rd Nov 2012, O.R., compute relative path using the specified source folder
	//don't assume that extra info carries relative folder info
		//ACI0078887 23rd Nov 2012, O.R., compute relative path using the specified source folder
	//don't assume that extra info carries relative folder info
	if (savedExtraInfo == PRESERVE_PARENT_OPTION)
	{
		//this is a bogus extra info that we use internally so we need to clear it so that it is not saved into
		//the archive.
		//This options means that the file will have to be restored in with its parent directory preserved.
		savedExtraInfo.Clear();

		VFolder *folder = inFile->RetainParentFolder();
		VString fileName;
		folder->GetName(fileInfo);
		// folder name can be a drive letter like Z:
		fileInfo.Exchange( (UniChar) ':', (UniChar) '_');
		inFile->GetName(fileName);
		fileInfo += slash;
		fileInfo += fileName;
		fileInfo.Insert( slash, 1 );
		folder->Release();
		result  = VE_OK;
	}
	else
	{
		if(testAssert(inFile->GetPath().GetRelativePath(inSourceFolder,fileInfo)))
		{
			result  = VE_OK;
			fileInfo.Insert( FOLDER_SEPARATOR, 1);
		}
	}

	if(result == VE_OK)
	{
		fileInfo.Exchange(folderSep ,slash, 1, 255);
		if ( fileInfo.GetUniChar(1) == '/' )
			fileInfo.Insert( '.', 1 );

		fileInfo += extra;
		fileInfo += savedExtraInfo;
		result = fStream->PutLong('file');
		if ( result == VE_OK )
		{
			result = fileInfo.WriteToStream(fStream);
		}
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
	}
	return result;
}

VError VArchiveStream::Proceed()
{
	bool userAbort = false;
	VError result = VE_OK;

	//Iterate over our various list and check for duplicates

	/* ACI0077162, Jul 11th 2012, O.R.: add some calls to the progress callback here in order to
	allow user feedback, in case of lengthy process (e.g. many files/folders to add)*/
	
	if (fCallBack)
	{
		fCallBack(CB_OpenProgressUndetermined,0,0,userAbort);
	}
	
	VectorOfPaths::iterator folderIterator =  fFolderPathList.begin();
	VectorOfVString::iterator folderInfoIterator =  fFolderExtra.begin();
	xbox_assert(fFolderPathList.size() == fFolderExtra.size());

	for ( uLONG i = 0; i < fFolderPathList.size() && result == VE_OK && !userAbort; i++ )
	{
		const VFilePath& folderPath = fFolderPathList[i];//(*folderIterator);
		const VFilePath& sourceFolderPath = fSourceFolderForFolders[i];//(*folderIterator);
		const VString& folderExtraInfo = folderInfoIterator[i];//(*folderInfoIterator);
		VFolder currentFolder(folderPath);
		result = _AddOneFolder(currentFolder,sourceFolderPath,folderExtraInfo);
	}

	if ( fStream && result == VE_OK && !userAbort)
	{
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
			sLONG8 storedObjCount = (sLONG8)fFileList.size();
			storedObjCount += (sLONG8)fFileDescList.size();

			fStream->PutLong8(storedObjCount /*(sLONG8)(fFileList.size()+fFileDescList.size())*/);
			
			/* put the header of the file listing */
			fStream->PutLong('LIST');
			
			for ( uLONG i = 0; i < fFileDescList.size() && result == VE_OK && !userAbort; i++ )
			{
				result = _WriteCatalog(fFileDescList[i]->GetParentVFile(),fSourceFolderForFileDescs[i],fFileDescExtra[i],totalByteCount);
				if ( fCallBack )
				{
					fCallBack(CB_UpdateProgress,0,0,userAbort);
				}
			}

			for ( uLONG i = 0; i < fFileList.size() && result == VE_OK && !userAbort; i++ )
			{
				result = _WriteCatalog(fFileList[i],fSourceFolderForFiles[i],fFileExtra[i],totalByteCount);
				if ( fCallBack )
				{
					fCallBack(CB_UpdateProgress,0,0,userAbort);
				}
			}

			//ACI0077162, Jul 11th 2012, O.R.: Close unbound session and open a bounded one for file processing
			if ( fCallBack )
			{
				fCallBack(CB_CloseProgress,0,0,userAbort);
				fCallBack(CB_OpenProgress,partialByteCount,totalByteCount,userAbort);
			}

			VSize bufferSize = 1024*1024;
			char *buffer = new char[bufferSize];

			/* writing file descriptor that we have to the archive files */
			/* nota : filedesc passed to the archivestream is considered as data fork */
			//ACI0077162, Jul 11th 2012, O.R.: _WriteFile() in charge of calling progress CB to give reactivity to upper layers
			for( VectorOfVFileDesc::iterator i = fFileDescList.begin() ; result == VE_OK && i != fFileDescList.end() && !userAbort ; ++i)
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

			for( VectorOfVFile::iterator i = fFileList.begin() ; result == VE_OK && i != fFileList.end() && !userAbort ; ++i )
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

			

			fStream->CloseWriting();
		}
	}
	//ACI0077162, Jul 11th 2012, O.R.: close remaining session (either unbounded one for check files or bounded one for processing files)
	if ( fCallBack )
		fCallBack(CB_CloseProgress,0,0,userAbort);
	if (userAbort)
	{
		result = XBOX::VE_STREAM_USER_ABORTED;
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
