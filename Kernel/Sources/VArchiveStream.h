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
#ifndef __VArchiveStream__
#define __VArchiveStream__

#include "VFilePath.h"
#include "VFile.h"
#include "VFolder.h"

BEGIN_TOOLBOX_NAMESPACE

typedef enum eCallBackAction
{
	CB_OpenProgress,
	CB_OpenProgressUndetermined,
	CB_UpdateProgress,
	CB_CloseProgress
} eCallBackAction;

typedef enum eFileSizeType
{
	fst_Data = 1,
	fst_Resource = 2,
	fst_Both = fst_Data | fst_Resource
} eFileSizeType;

class XTOOLBOX_API VArchiveCatalog : public VObject
{
public:
	VArchiveCatalog( VFile *inFile, sLONG8 inDataFileSize, sLONG8 inResFileSize, VString &inStoredPath, VString &inFileExtra, uLONG inKind, uLONG inCreator );
	~VArchiveCatalog();

	VFile*		GetFile();
	sLONG8		GetFileSize( eFileSizeType inSizeType = fst_Data );
	VString&	GetStoredPath();
	VString&	GetExtra();
	Boolean		GetExtractFlag();
	void		SetExtractFlag( Boolean inExtract );
	uLONG		GetKind();
	uLONG		GetCreator();

protected:
	VFile *fFile;
	sLONG8 fResFileSize;
	sLONG8 fDataFileSize;
	VString	fStoredPath;
	VString fExtra;
	Boolean fExtract;
	uLONG fKind;
	uLONG fCreator;
};

typedef std::vector<VArchiveCatalog*> ArchiveCatalog;
typedef std::vector<VString> VectorOfVString;
typedef std::vector<VFileDesc*> VectorOfFileDesc;
typedef std::set<VFilePath> SetOfVFilePath;
typedef std::vector<VFilePath >	VectorOfPaths;

typedef void (*CB_VArchiveStream)( eCallBackAction inCBA, uLONG8 inBytesCount, uLONG8 inTotalBytesCount, bool &outAbort );

class XTOOLBOX_API VArchiveStream : public VObject
{
public:
	VArchiveStream();
	~VArchiveStream();

			VError AddFileDesc( VFileDesc* inFileDesc, const VString &inExtra );
				
			/**
			 * \brief adds a file to the archive
			 * \param inFile the file to be added
			 * \param inPreserveParentFolder if true, then @c inFile will be restored
			 * underneath its current parent folder
			 * e.g. if @c InFile is "/users/images/TerryTate.png" and @c inPreserveParentFolder is set
			 * then when the archive is later restored to "/var/nekochimpo/", the image will be found
			 * under "/var/nekochimpo/images/TerryTate.png".
			 * If @c inPreserveParentFolder is false, the image wound be found under 
			 * "/var/nekochimpo/TerryTate.png".
			 */
			VError	AddFile( VFile* inFile,bool inPreserveParentFolder = false);

			VError	AddFile( VFile* inFile, const VString &inExtra );
			VError	AddFile( VFilePath &inFilePath );

			VError	AddFolder( VFolder* inFolder, const VString &inExtraInfo );
			VError	AddFolder( VFolder* inFolder );
			VError	AddFolder( VFilePath &inFileFolder );

			bool	ContainsFile( const VFile* inFile) const;

			void	SetRelativeFolderSource(const VFilePath &inFolderPath );
			void	SetDestinationFile( VFile* inFile );
			void	SetStreamer( VStream* inStream );
			void	SetProgressCallBack( CB_VArchiveStream inProgressCallBack );

	virtual	VError	Proceed();

protected:

	VError			_AddFile(VFile& inFile,const VFilePath& inSourceFolder,const VString& inExtraInfo);
	VError			_AddOneFolder(VFolder& inFolder,const VFilePath& inSourceFolder,const VString& inExtraInfo);
	
	virtual VError	_WriteCatalog( const VFile* inFile,const VFilePath& inStorageFolder, const VString &inExtraInfo, uLONG8 &ioTotalByteCount );
	virtual VError	_WriteFile( const VFileDesc* inFileDesc, char* buffToUse, VSize buffSize, uLONG8 &ioPartialByteCount, uLONG8 inTotalByteCount );

	VFile				*fDestinationFile;	/* file archive */
	VFilePath			fRelativeFolder;	/* file path stored in the archive is relative to this folder */

	VectorOfVFile		fFileList;			/* list of file to be stored */
	VectorOfVString		fFileExtra;			/* extra information for each file */
	VectorOfPaths		fSourceFolderForFiles;	/* abs path of folder to compute relative storage path of item in archive */

	VectorOfFileDesc	fFileDescList;		/* list of filedesc to be saved */
	VectorOfVString		fFileDescExtra;		/* extra information about this filedesc */
	VectorOfPaths		fSourceFolderForFileDescs;	/* abs path of folder to compute relative storage path of item in archive */

	VectorOfPaths		fFolderPathList;	/* List of folder paths to be stored (with all files and sub-folders) */
	VectorOfVString		fFolderExtra;		/* extra information about entries in the folder list */
	VectorOfPaths		fSourceFolderForFolders;	/* abs path of folder to compute relative storage path of item in archive */


	SetOfVFilePath		*fUniqueFilesCollection; /* collection of unique file paths that will be processed */

	VStream				*fStream;			/* streaming used for pushing data */
	CB_VArchiveStream	fCallBack;			/* compression progress call back */
};

class XTOOLBOX_API VArchiveUnStream : public VObject
{
public:
							VArchiveUnStream();
							~VArchiveUnStream();

	virtual void			SetSourceFile( VFile* inFileSource );
	virtual	void			SetDestinationFolder( VFolder* inFolder );
	virtual	void			SetDestinationFolder( VFilePath &inFolder );
	virtual	void			SetStreamer( VStream* inStream );
	virtual	void			SetProgressCallBack( CB_VArchiveStream inProgressCallBack );

	virtual	VError			Proceed();

	virtual	VError			ProceedCatalog();
	virtual	VError			ProceedFile();

	virtual	ArchiveCatalog*	GetCatalog();

protected:

	virtual	VError			_ExtractFile( VArchiveCatalog *inCatalog, const VFileDesc* inFileDesc, char* buffToUse, VSize buffSize, uLONG8 &ioPartialByteCount );
	#if VERSIONMAC
	static bool				_IsExecutable(char* buffToUse, VSize buffSize);
	#endif
	
	VFile				*fSourceFile;
	uLONG8				fTotalByteCount;

	ArchiveCatalog		fFileCatalog;
	VFilePath			fDestinationFolder;

	VStream				*fStream;
	CB_VArchiveStream	fCallBack;			/* compression progress call back */


};
END_TOOLBOX_NAMESPACE

#endif