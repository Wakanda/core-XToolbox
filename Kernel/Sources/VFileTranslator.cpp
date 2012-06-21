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
#include "VFileTranslator.h"
#include "VFile.h"
#include "VString.h"


VFileTranslator::VFileTranslator()
{
	fSrcFile = NULL;
	fDestFile = NULL;
}


VFileTranslator::VFileTranslator(VFile* inSrcFile, VFile* inDestFile)
{
	fSrcFile = inSrcFile;
	fDestFile = inDestFile;
}


VFileTranslator::~VFileTranslator()
{
}


void VFileTranslator::GetTranslatorName(VString& /*inString*/) const
{
}


void VFileTranslator::SetSourceFile(VFile* inSrcFile)
{
	fSrcFile = inSrcFile;
}


void VFileTranslator::SetDestinationFile(VFile* inDestFile)
{
	fDestFile = inDestFile;
}


Boolean VFileTranslator::DoTranslation(void)
{
	return false;// return true if successfull
}


#if VERSIONMAC && 0

#pragma mark-

VNavTranslator::VNavTranslator(const NavReplyRecord& inReply, VTranslationMode inMode)
{
	fTranslationMode = inMode;
	::BlockMoveData(&inReply, &fReply, sizeof(NavReplyRecord));
}


VNavTranslator::~VNavTranslator()
{
	::NavDisposeReply(&fReply);
}


void VNavTranslator::GetTranslatorName(VString& /*inString*/) const
{
}


void VNavTranslator::SetTranslationMode(VTranslationMode inMode)
{
	fTranslationMode = inMode;
}


Boolean VNavTranslator::DoTranslation(void)
{
	OSErr	error = noErr;
	
	if (fTranslationMode == TRM_TRANSLATEINPLACE)
		error = ::NavCompleteSave(&fReply, kNavTranslateInPlace);
	else
		error = ::NavCompleteSave(&fReply, kNavTranslateCopy);
		
	return (error == noErr);
}

#endif

#if OBSOLETE_CODE
#pragma mark-

VEasyOpenTranslator::VEasyOpenTranslator()
{
}


VEasyOpenTranslator::VEasyOpenTranslator(VFile* inSrcFile, VFile* inDestFile) : VFileTranslator(inSrcFile, inDestFile)
{
}


VEasyOpenTranslator::~VEasyOpenTranslator()
{
	if (fDestFile != NULL) delete fDestFile;
}


void VEasyOpenTranslator::GetTranslatorName(VString& inString) const
{
	assert(fSrcFile != NULL);
	assert(fDestFile != NULL);
	
	if (fSrcFile != NULL && fDestFile != NULL)
	{
		FSSpec	srcSpec;
		Str31	nameStr;
		fSrcFile->MAC_GetFSSpec(srcSpec);
		
		OSType fileKind;
		fDestFile->MAC_GetImpl()->GetKind(fileKind);
		FileTranslationSpec	path;
		sWORD	numPath = ::GetFileTranslationPaths(&srcSpec, fileKind, 1, &path);
		
		if (numPath > 0 && ::GetTranslationExtensionName(&path, nameStr) == noErr)
		{
			inString.MAC_FromMacPString(nameStr);
		}
	}
}


Boolean VEasyOpenTranslator::DoTranslation()
{
	FSSpec	srcSpec, dstSpec;
	OSErr	error = noErr;
	
	assert(fSrcFile != NULL);
	if (fSrcFile == NULL) return false;
	
	assert(fDestFile != NULL);
	if (fDestFile == NULL) return false;
	
	fSrcFile->MAC_GetFSSpec(srcSpec);
	fDestFile->MAC_GetFSSpec(dstSpec);
	fDestFile->Delete();
	
	OSType fileKind;
	fDestFile->MAC_GetKind(fileKind);
	FileTranslationSpec	path;
	error = ::GetFileTranslationPaths(&srcSpec, fileKind, 1, &path);
	
	if (error > 0)		// one path found at least
		error = ::TranslateFile(&srcSpec, &dstSpec, &path);
	
	return (error == noErr);
}

#endif // OBSOLETE_CODE
