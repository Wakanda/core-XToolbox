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
#include "VFolder.h"
#include "VFile.h"
#include "VLogger.h"
#include "VProcess.h"


const sLONG kSPLITABLE_LOG_FILE_MAX = 10485760; // 10 * 1024L * 1024L

VSplitableLogFile::VSplitableLogFile( const VFolder& inBasePath, const VString& inBaseName, sLONG inStartNumber)
{
	fFile = NULL;
	fLogNumber = inStartNumber;
	fLogName = inBaseName;
	fFolderPath = inBasePath.GetPath();
	fDelegate = NULL;
}


VSplitableLogFile::~VSplitableLogFile()
{
	Close();
}


bool VSplitableLogFile::Open( bool inCreateEmptyFile)
{
	if (fFile == NULL)
	{
		VString vpath;
		_BuildFilePath( vpath);
		if(!vpath.IsEmpty())
		{
			StStringConverter<char> convert( VTC_StdLib_char);
			if (inCreateEmptyFile)
			{
				fFile = ::fopen( convert.ConvertString(vpath), "w");
				if (fDelegate != NULL)
					fDelegate->DoCreateNewLogFile( vpath);
			}
			else
			{
				// Check if file exists
				fFile = ::fopen( convert.ConvertString(vpath), "r");
				if (fFile != NULL)
				{
					fFile = ::freopen( convert.ConvertString(vpath), "a", fFile);
				}
				else
				{
					fFile = ::fopen( convert.ConvertString(vpath), "w");
					if (fDelegate != NULL)
						fDelegate->DoCreateNewLogFile( vpath);
				}
			}
		}
	}
	return fFile!=NULL;
}


bool VSplitableLogFile::Open( sLONG inLogNumber, bool inCreateEmptyFile)
{
	fLogNumber = inLogNumber;
	return Open( inCreateEmptyFile);
}


void VSplitableLogFile::OpenStdOut()
{
	if ( (fFile != NULL) && (fFile != stdout) )
		Close();
	fFile = stdout;
}


void VSplitableLogFile::Close()
{
	if (fFile != NULL)
	{
		if (fFile != stdout)
		{
			::fclose( fFile);
		}
		fFile = NULL;
	}
}


void VSplitableLogFile::Flush()
{
	if(fFile != NULL)
	{
		::fflush( fFile);
	}
}


void VSplitableLogFile::SetLogNumber( sLONG inLogNumber, bool inCreateEmptyFile)
{
	fLogNumber = inLogNumber;
	if(IsOpen())
	{
		Close();
		Open( inCreateEmptyFile);
	}
}


sLONG VSplitableLogFile::CheckLogSize()
{
	sLONG res = 0;
	if (fFile != NULL)
	{
		// check size, segments the log in 10 Mo chunks
		long pos = ::ftell( fFile);
		if (pos > kSPLITABLE_LOG_FILE_MAX)
		{
			Close();

			++fLogNumber;

			Open( true);

			res = fLogNumber;
		}
	}
	return res;
}

sLONG VSplitableLogFile::CheckRemainingBytes() const
{
	if (fFile != NULL)
		return (sLONG) (kSPLITABLE_LOG_FILE_MAX - ::ftell( fFile));

	return 0;
}


void VSplitableLogFile::AppendFormattedString( const char* inFormat, ...)
{
	CheckLogSize();

	if (fFile != NULL)
	{
		va_list argList;

		va_start(argList, inFormat);
		::vfprintf( fFile, inFormat, argList);
		va_end( argList);
	}
}


void VSplitableLogFile::AppendString( const char* inString)
{
	CheckLogSize();

	if (fFile != NULL)
	{
		::fputs( inString, fFile);
	}
}


void VSplitableLogFile::GetCurrentFileName( VString& outName) const
{
	VSplitableLogFile::BuildLogFileName( fLogName, fLogNumber, outName);
}


void VSplitableLogFile::BuildLogFileName( const VString& inBaseName, sLONG inLogNumber, VString& outName)
{
	outName.Clear();
	outName += inBaseName;
	outName += "_";
	outName.AppendLong( inLogNumber);
	outName += ".txt";
}


void VSplitableLogFile::SetDelegate( IDelegate *inDelegate)
{
	fDelegate = inDelegate;
}


void VSplitableLogFile::_BuildFilePath( VString& outPath) const
{
	if(!fFolderPath.IsEmpty())
	{
		VString fname;
	#if VERSIONMAC
		fFolderPath.GetPosixPath( outPath);
		if (outPath[outPath.GetLength() - 1] != '/')
			outPath += '/';
	#else
		fFolderPath.GetPath( outPath);
	#endif
		GetCurrentFileName( fname);
		outPath += fname;
	}
	else
	{
		outPath.Clear();
	}
}


static const char *GetMessageLevelName( EMessageLevel inLevel)
{
	switch( inLevel)
	{
		case EML_Dump:			return "DUMP";
		case EML_Trace:			return "TRACE";
		case EML_Debug:			return "DEBUG";
		case EML_Warning:		return "WARN";
		case EML_Error:			return "ERROR";
		case EML_Fatal:			return "FATAL";
		case EML_Assert:		return "ASSERT";	// non standard
		case EML_Information:
		default:				return "INFO";
	}
}


VLog4jMsgFileLogger::VLog4jMsgFileLogger( const VFolder& inLogFolder, const VString& inLogName)
: fLogName(inLogName)
, fOutput(NULL)
, fIsStarted( false)
, fFilter((1<<EML_Information) | (1<<EML_Warning) | (1<<EML_Error) | (1<<EML_Fatal) /*| (1<<EML_Trace) | (1<<EML_Dump)*/)
{
	inLogFolder.GetPath( fFolderPath);
}


VLog4jMsgFileLogger::~VLog4jMsgFileLogger()
{
	Stop();
}


void VLog4jMsgFileLogger::Start( sLONG inLogNumber)
{
	fLock.Lock();
	if (fOutput == NULL)
	{
		if (!fFolderPath.IsEmpty() && fFolderPath.IsValid() && fFolderPath.IsFolder() && !fLogName.IsEmpty())
		{
			xbox_assert(inLogNumber > 0);

			VFolder folder( fFolderPath);

			fOutput = new VSplitableLogFile( folder, fLogName, inLogNumber);
			if (fOutput != NULL)
			{
				if (fOutput->Open( false))
				{
					VTime::Now( fStartTime);
					fIsStarted = true;

					VString path;
					fOutput->GetCurrentFilePath( path);
					
					fReadersLock.Lock();

					for (std::vector<IReader*>::iterator iter = fReaders.begin() ; iter != fReaders.end() ; ++iter)
						(*iter)->DoStart( path);

					fReadersLock.Unlock();
				}
				else
				{
					fOutput->Close();
					delete fOutput;
					fOutput = NULL;
				}
			}
		}
	}
	fLock.Unlock();
}


void VLog4jMsgFileLogger::Start()
{
	if (!fFolderPath.IsEmpty() && fFolderPath.IsValid() && fFolderPath.IsFolder() && !fLogName.IsEmpty())
	{
		// Find the highest log file number
		VString fileName;
		VFolder folder(fFolderPath);
		sLONG highestNumber = -1, curNumber = 1;

		while (highestNumber == -1)
		{
			BuildLogFileName( fLogName, curNumber, fileName);
			
			VFile file( folder, fileName);
			if (file.Exists())
				++curNumber;
			else
				highestNumber = (curNumber > 1) ? curNumber - 1 : 1;
		}

		Start( highestNumber);
	}
}


void VLog4jMsgFileLogger::Stop()
{
	fLock.Lock();
	if (fOutput != NULL)
	{
		fOutput->Close();
		delete fOutput;
		fOutput = NULL;
		fIsStarted = false;
	}
	fStartTime.Clear();
	fLock.Unlock();
}


bool VLog4jMsgFileLogger::IsStarted() const
{
	return fIsStarted;
}


void VLog4jMsgFileLogger::Flush()
{
	fLock.Lock();
	if(fOutput != NULL)
	{
		fOutput->Flush();
	}
	fLock.Unlock();
}


void VLog4jMsgFileLogger::LogBag( const VValueBag *inMessage)
{
	VString message;
	ILoggerBagKeys::message.Get( inMessage, message);

	VError errorCode = VE_OK;
	ILoggerBagKeys::error_code.Get( inMessage, errorCode);

	EMessageLevel level = ILoggerBagKeys::level.Get( inMessage);
	VString loggerID;
	if (!ILoggerBagKeys::source.Get( inMessage, loggerID))
	{
		VProcess::Get()->GetProductName( loggerID);
		OsType componentSignature = 0;
		if (!ILoggerBagKeys::component_signature.Get( inMessage, componentSignature))
			componentSignature = COMPONENT_FROM_VERROR( errorCode);

		if (componentSignature != 0)
		{
			loggerID.AppendUniChar( '.').AppendOsType( componentSignature);
		}
	}

	if (errorCode != VE_OK)
	{
		VString s;
		s.Printf( "[%d] ", ERRCODE_FROM_VERROR( errorCode));
		message.Insert( s, 1);
	}
	
	sLONG taskId=-1;
	if (ILoggerBagKeys::task_id.Get(inMessage, taskId))
	{
		message.AppendString(CVSTR(", task #"));
		message.AppendLong(taskId);
	}
	
	VString taskName;
	if (!ILoggerBagKeys::task_name.Get( inMessage, taskName))
		VTask::GetCurrent()->GetName( taskName);
	if (!taskName.IsEmpty())
	{
		message += ", ";
		message += taskName;
	}
	
	sLONG socketDescriptor=-1;
	if (ILoggerBagKeys::socket.Get(inMessage, socketDescriptor))
	{
		message.AppendString(CVSTR(", socket "));
		message.AppendLong(socketDescriptor);
	}
	
	VString localAddr;
	if (ILoggerBagKeys::local_addr.Get(inMessage, localAddr))
	{
		message.AppendString(CVSTR(", local addr is "));
		message.AppendString(localAddr);
	}
	
	VString peerAddr;
	if (ILoggerBagKeys::peer_addr.Get(inMessage, peerAddr))
	{
		message.AppendString(CVSTR(", peer addr is "));
		message.AppendString(peerAddr);
	}
	
	bool exchangeEndPointID=false;
	if (ILoggerBagKeys::exchange_id.Get(inMessage, exchangeEndPointID))
	{
		if(exchangeEndPointID)
			message.AppendString(CVSTR(", exchange endpoint id "));
		else
			message.AppendString(CVSTR(", do not exchange endpoint id "));
	}
	
	bool isBlocking=false;
	if (ILoggerBagKeys::is_blocking.Get(inMessage, isBlocking))
	{
		if(isBlocking)
			message.AppendString(CVSTR(", is blocking "));
		else
			message.AppendString(CVSTR(", is not blocking "));
	}
	
	bool isSSL=false;
	if (ILoggerBagKeys::is_ssl.Get(inMessage, isSSL))
	{
		if(isSSL)
			message.AppendString(CVSTR(", with SSL "));
		else
			message.AppendString(CVSTR(", without SSL"));
	}
	
	bool isSelectIO=false;
	if (ILoggerBagKeys::is_select_io.Get(inMessage, isSelectIO))
	{
		if(isSelectIO)
			message.AppendString(CVSTR(", with SelectIO "));
		else
			message.AppendString(CVSTR(", without SelectIO"));
	}
	
	sLONG ioTimeout=-1;
	if (ILoggerBagKeys::ms_timeout.Get(inMessage, ioTimeout))
	{
		message.AppendString(CVSTR(", with "));
		message.AppendLong(ioTimeout);
		message.AppendString(CVSTR("ms timeout"));
	}
	
	sLONG askedCount=-1;
	if (ILoggerBagKeys::count_bytes_asked.Get(inMessage, askedCount))
	{
		message.AppendString(CVSTR(", asked for "));
		message.AppendLong(askedCount);
		message.AppendString(CVSTR(" byte(s)"));
	}
	
	sLONG sentCount=-1;
	if (ILoggerBagKeys::count_bytes_sent.Get(inMessage, sentCount))
	{
		message.AppendString(CVSTR(", sent "));
		message.AppendLong(sentCount);
		message.AppendString(CVSTR(" byte(s)"));
	}
	
	sLONG receivedCount=-1;
	if (ILoggerBagKeys::count_bytes_received.Get(inMessage, receivedCount))
	{
		message.AppendString(CVSTR(", received "));
		message.AppendLong(receivedCount);
		message.AppendString(CVSTR(" byte(s)"));
	}
	
	sLONG ioSpent=-1;
	if (ILoggerBagKeys::ms_spent.Get(inMessage, ioSpent))
	{
		message.AppendString(CVSTR(", done in "));
		message.AppendLong(ioSpent);
		message.AppendString(CVSTR("ms"));
	}
	
	sLONG dumpOffset=-1;
	if (ILoggerBagKeys::dump_offset.Get(inMessage, dumpOffset))
	{
		message.AppendString(CVSTR(", offset "));
		message.AppendLong(dumpOffset);
	}
	
	VString dumpBuffer;
	if (ILoggerBagKeys::dump_buffer.Get(inMessage, dumpBuffer))
	{
		message.AppendString(CVSTR(", data : "));
		message.AppendString(dumpBuffer);
	}
	
	VString fileName;
	if (ILoggerBagKeys::file_name.Get( inMessage, fileName))
	{
		message += ", ";
		message += fileName;
	}

	sLONG lineNumber;
	if (ILoggerBagKeys::line_number.Get( inMessage, lineNumber))
	{
		message += ", ";
		message.AppendLong( lineNumber);
	}
	
	VString stackCrawl;
	if (ILoggerBagKeys::stack_crawl.Get( inMessage, stackCrawl))
	{
		message += ", {";
		stackCrawl.ExchangeAll( CVSTR( "\n"), CVSTR( " ; "));
		message += stackCrawl;
		message += "}";
	}

	Log( loggerID, level, message, NULL);
}


bool VLog4jMsgFileLogger::ShouldLog( EMessageLevel inMessageLevel) const
{
	// no lock. Must be as fast as possible.
	// no filtering for now. Accept any kind of message.
	return fIsStarted && ((fFilter & (1 << inMessageLevel)) != 0);
}

bool VLog4jMsgFileLogger::WithTag(uLONG inTag, bool inFlag)
{
	uLONG filter=GetLevelFilter();

	inTag=(1<<inTag);

	bool rv=(filter&inTag) ? true : false;
	
	if(inFlag)
		filter|=inTag;
	else
		filter&=~inTag;
	
	SetLevelFilter(filter);
	
	return rv;
}

bool VLog4jMsgFileLogger::WithTrace(bool inWithTag)		{ return WithTag(EML_Trace,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithDump(bool inWithTag)		{ return WithTag(EML_Dump,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithDebug(bool inWithTag)		{ return WithTag(EML_Debug,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithWarning(bool inWithTag)	{ return WithTag(EML_Warning,	 	inWithTag); }
bool VLog4jMsgFileLogger::WithError(bool inWithTag)		{ return WithTag(EML_Error,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithFatal(bool inWithTag)		{ return WithTag(EML_Fatal,		 	inWithTag); }
bool VLog4jMsgFileLogger::WithAssert(bool inWithTag)	{ return WithTag(EML_Assert,	 	inWithTag); }
bool VLog4jMsgFileLogger::WithInfo(bool inWithTag)		{ return WithTag(EML_Information, 	inWithTag); }


void VLog4jMsgFileLogger::Log( const VString& inLoggerID, EMessageLevel inLevel, const VString& inMessage, VString* outFormattedMessage)
{
	StStringConverter<char> loggerConverter( VTC_StdLib_char);
	StStringConverter<char> messageConverter( VTC_StdLib_char);
	Log( loggerConverter.ConvertString( inLoggerID), inLevel, messageConverter.ConvertString( inMessage), outFormattedMessage);
}


void VLog4jMsgFileLogger::Log( const char* inLoggerID, EMessageLevel inLevel, const char* inMessage, VString* outFormattedMessage)
{
	fLock.Lock();
	if (ShouldLog(inLevel))
	{
		bool done = false;
		char szTime[512];
		time_t now = ::time( NULL);
		::strftime( szTime, sizeof( szTime),"%Y-%m-%d %X", ::localtime( &now));

		if (outFormattedMessage != NULL)
		{
			outFormattedMessage->Clear();

			void *buffer = ::malloc( ::strlen( inMessage) + 256);
			if (buffer != NULL)
			{
				::sprintf( (char*)buffer, "%s [%s] %s - %s\n", szTime, inLoggerID, GetMessageLevelName(inLevel), inMessage);

				outFormattedMessage->FromCString( (char*)buffer);
				fOutput->AppendString( (char*)buffer);
				done = true;

				::free( buffer);
			}
		}

		if (!done)
		{
			fOutput->AppendFormattedString( "%s [%s] %s - %s\n", szTime, inLoggerID, GetMessageLevelName(inLevel), inMessage);
		}
	}
	fLock.Unlock();
}


void VLog4jMsgFileLogger::GetLogFolderPath( VFilePath& outPath) const
{
	outPath.FromFilePath( fFolderPath);
}


void VLog4jMsgFileLogger::GetLogName( VString& outName) const
{
	outName = fLogName;
}


void VLog4jMsgFileLogger::BuildLogFileName( const VString& inLogName, sLONG inLogNumber, VString& outName)
{
	VSplitableLogFile::BuildLogFileName( inLogName, inLogNumber, outName);
}


void VLog4jMsgFileLogger::AttachReader( IReader *inReader)
{
	if (inReader != NULL)
	{
		fReadersLock.Lock();

		if (std::find( fReaders.begin(), fReaders.end(), inReader) == fReaders.end())
		{
			fReaders.push_back( inReader);

			fLock.Lock();
			if (fOutput != NULL && _IsStarted())
			{
				VString path;
				fOutput->GetCurrentFilePath( path);
				inReader->DoStart( path);
			}
			fLock.Unlock();
		}

		fReadersLock.Unlock();
	}
}


void VLog4jMsgFileLogger::DetachReader( IReader *inReader)
{
	if (inReader != NULL)
	{
		fReadersLock.Lock();

		std::vector<IReader*>::iterator found = std::find( fReaders.begin(), fReaders.end(), inReader);
		if (found != fReaders.end())
			fReaders.erase( found);

		fReadersLock.Unlock();
	}
}


void VLog4jMsgFileLogger::DoCreateNewLogFile( const VString& inFilePath)
{
	fLock.Lock();
	VString path;
	fOutput->GetCurrentFilePath( path);
	fLock.Unlock();

	fReadersLock.Lock();

	for (std::vector<IReader*>::iterator iter = fReaders.begin() ; iter != fReaders.end() ; ++iter)
		(*iter)->DoStart( path);

	fReadersLock.Unlock();
}


bool VLog4jMsgFileLogger::_IsStarted() const
{
	return (fOutput != NULL) && fOutput->IsOpen();
}


//============================================================


VLog4jMsgFileReader::VLog4jMsgFileReader()
{
	fFile = NULL;
}


VLog4jMsgFileReader::~VLog4jMsgFileReader()
{
	if (fFile != NULL)
	{
		::fclose( fFile);
		fFile = NULL;
	}
}


void VLog4jMsgFileReader::PeekMessages( VectorOfVString& outMessages) const
{
	outMessages.clear();
	fLock.Lock();
	outMessages.insert( outMessages.begin(), fMessages.begin(), fMessages.end());
	fMessages.clear();
	_PeekMessages( outMessages);
	fLock.Unlock();
}


void VLog4jMsgFileReader::DoStart( const VString& inFilePath)
{
	fLock.Lock();

	if (fFile != NULL)
	{
		// keep the last messages which was recorded in the previous file
		_PeekMessages( fMessages);
		::fclose( fFile);
		fFile = NULL;
	}

	if (!inFilePath.IsEmpty())
	{
		StStringConverter<char> convert( VTC_StdLib_char);
		fFile = ::fopen( convert.ConvertString( inFilePath), "r");
		::fseek( fFile, 0, SEEK_END);
	}

	fLock.Unlock();
}


void VLog4jMsgFileReader::_PeekMessages( VectorOfVString& outMessages) const
{
	if (fFile != NULL)
	{
		char buffer[1024];	// could be increased if need

		while (::fgets( buffer, 1024, fFile) != NULL)
		{
			VString string( buffer);
			outMessages.push_back( string);
		}
#if VERSIONMAC
		// Clean EOF stream flag to allow next fgets() not to fail
		if ( ::feof(fFile) )
			::clearerr(fFile);
#endif
	}
}
