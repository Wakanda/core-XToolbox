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

#include "VServerNetPrecompiled.h"
#include "VHTTPMessage.h"
#include "HTTPTools.h"

BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE
using namespace HTTPTools;


//--------------------------------------------------------------------------------------------------


char *memstr (const char *mem, size_t mem_size, const char *sub, size_t sub_size)
{
	char *ret = NULL;
	char *ptr = const_cast<char *>(mem);
	while (ptr && !ret)
	{
		ptr = reinterpret_cast<char *> (memchr (ptr, *sub, mem_size - (sub_size - 1) - (int)(ptr - mem)));
		if (ptr)
		{
			if (!memcmp (ptr, sub, sub_size))
				ret = ptr;
			++ptr;
		}
	}

	return ret;
}


bool _ExtractHeaderValuePair (const char *inStartLinePtr, const char *inEndLinePtr, XBOX::VString& outHeader, XBOX::VString& outValue, const XBOX::CharSet inDefaultCharSet)
{
	if (NULL == inStartLinePtr)
		return false;

	const char *colonPtr = strchr (inStartLinePtr, ':');

	if (NULL != colonPtr)
	{
		outHeader.FromBlock (inStartLinePtr, colonPtr - inStartLinePtr, XBOX::VTC_DefaultTextExport);
		if (!outHeader.IsEmpty())
		{
			++colonPtr; // skip colon
			while (isspace (*colonPtr))
				++colonPtr;

			if (XBOX::VTC_UNKNOWN != inDefaultCharSet)
				outValue.FromBlock (colonPtr, (inEndLinePtr - colonPtr), inDefaultCharSet);
			else
				outValue.FromBlock (colonPtr, (inEndLinePtr - colonPtr), XBOX::VTC_ISO_8859_1);

			if (((inEndLinePtr - colonPtr) > 0) && outValue.IsEmpty())	// YT 01-Feb-2012 - ACI0075472 - Something was going wrong at conversion... Let's try in UTF-8
				outValue.FromBlock (colonPtr, (inEndLinePtr - colonPtr), XBOX::VTC_UTF_8);
		}

		return !(outHeader.IsEmpty());
	}

	return false;
}


//--------------------------------------------------------------------------------------------------


VHTTPMessage::VHTTPMessage()
: fHeaders()
, fBody (NULL)
, fDisposeBody (true)
{
}


VHTTPMessage::~VHTTPMessage()
{
	Clear();
}


void VHTTPMessage::Clear()
{
	fHeaders.Clear();

	if (NULL != fBody)
	{
		if (fBody->GetDataSize())
		{
			/*
			YT 05-Jul-2010 - http://wiki.4d.fr/bugtrack/show_bug.cgi?id=747
			IMPORTANT: use StealData() first to prevent ~VPtrStream() deleting body's data (which could be owned by Cache Manager)
			*/
			void *buffer = fBody->StealData();

			if (fDisposeBody && buffer)
				XBOX::vFree (buffer);
		}

		delete fBody;
		fBody = NULL;
	}

	fDisposeBody = true;
}


XBOX::VPtrStream& VHTTPMessage::GetBody()
{
	if (NULL == fBody)
		fBody = new XBOX::VPtrStream ();

	return *fBody;
}


const XBOX::VPtrStream& VHTTPMessage::GetBody() const
{
	if (NULL == fBody)
		fBody = new XBOX::VPtrStream ();

	return *fBody;
}


XBOX::VError VHTTPMessage::ReadFromStream (XBOX::VStream& inStream, const XBOX::VString& inBoundary)
{
#define	MAX_BUFFER_LENGTH	32768

	const char			HTTP_CR = '\r';
	const char			HTTP_LF = '\n';
	const char			HTTP_CRLF [] = { HTTP_CR, HTTP_LF, 0 };
	const char			HTTP_CRLFCRLF [] = { HTTP_CR, HTTP_LF, HTTP_CR, HTTP_LF, 0 };

	XBOX::VError		streamError = XBOX::VE_OK;
	HTTPParsingState	parsingState;
	XBOX::VError		parsingError = XBOX::VE_OK;
	XBOX::VSize			bufferSize = MAX_BUFFER_LENGTH;
	char *				buffer = (char *)XBOX::vMalloc (bufferSize, 0);
	XBOX::VSize			bufferOffset = 0;
	XBOX::VSize			unreadBytes = 0;
	sLONG				lineLen = 0;
	XBOX::VString		header;
	XBOX::VString		value;
	char *				startLinePtr = NULL;
	char *				endLinePtr = NULL;
	char *				endHeaderPtr = NULL;
	sLONG				endLineSize = sizeof (HTTP_CRLF) - 1;
	sLONG				contentLength = 0;
	void *				bodyContentBuffer = NULL;
	XBOX::VSize			bodyContentSize = 0;
	const sLONG			MAX_REQUEST_ENTITY_SIZE = XBOX::MaxLongInt;
	XBOX::VString		boundaryEnd;
	char *				boundary = NULL;
	bool				stopReadingStream = false;
	XBOX::CharSet		bodyCharSet = (XBOX::VTC_UNKNOWN != inStream.GetCharSet()) ? inStream.GetCharSet() : XBOX::VTC_UTF_8;

	if (!inBoundary.IsEmpty())
	{
		boundaryEnd.AppendString ("--");
		boundaryEnd.AppendString (inBoundary);
		boundary = new char[boundaryEnd.GetLength() + 1];
		if (NULL != boundary)
			boundaryEnd.ToCString (boundary, boundaryEnd.GetLength() + 1);
	}

	if (NULL == buffer)
		return XBOX::VE_MEMORY_FULL;

	parsingState = PS_ReadingHeaders;

	XBOX::StErrorContextInstaller errorContext (XBOX::VE_STREAM_EOF, XBOX::VE_OK);

	bool isAlreadyReading = inStream.IsReading();

	if (!isAlreadyReading)
		streamError = inStream.OpenReading();

	while ((XBOX::VE_OK == streamError) && !stopReadingStream)
	{
		if (0 == unreadBytes)
			bufferOffset = 0;

		bufferSize = MAX_BUFFER_LENGTH - bufferOffset;

		streamError = inStream.GetData (buffer + bufferOffset, &bufferSize);

		unreadBytes = (bufferSize + bufferOffset);
		bufferOffset = 0;

		while ((unreadBytes > 0) && (XBOX::VE_OK == parsingError))
		{
			if (parsingState <= PS_ReadingHeaders)
			{
				startLinePtr = buffer + bufferOffset;
				endLinePtr = strstr (startLinePtr, HTTP_CRLF);

				if ((NULL != endLinePtr) && (NULL == endHeaderPtr))
					endHeaderPtr = strstr (startLinePtr, HTTP_CRLFCRLF);
			}

			/* Start to parse the Status-Line */
			switch (parsingState)
			{
			case PS_ReadingHeaders:
				{
					if (NULL != endLinePtr)
					{
						if (startLinePtr != (endHeaderPtr + endLineSize))
						{
							if (_ExtractHeaderValuePair (startLinePtr, endLinePtr, header, value, bodyCharSet))
							{
								GetHeaders().SetHeaderValue (header, value, false);
							}
						}

						else /*if (startLinePtr == endHeaderPtr)*/
						{
							parsingState = PS_ReadingBody;
							XBOX::VString contentLengthString;
							if (GetHeaders().GetHeaderValue (STRING_HEADER_CONTENT_LENGTH, contentLengthString))
								contentLength = HTTPTools::GetLongFromString (contentLengthString);
						}
					}
					break;
				}

			case PS_ReadingBody:
				{
					if (!boundaryEnd.IsEmpty())
					{
						if (NULL != boundary)
						{
							char *endBoundaryPtr = memstr (buffer + bufferOffset, unreadBytes, boundary, strlen (boundary));
							if (NULL != endBoundaryPtr)
							{
								XBOX::VSize nbBytesToCopy  = (endBoundaryPtr - (buffer + bufferOffset));
								inStream.UngetData (endBoundaryPtr, unreadBytes - nbBytesToCopy);
								unreadBytes = nbBytesToCopy;
								if (NULL != memstr (endBoundaryPtr - 2, 2, HTTP_CRLF, 2))
									unreadBytes -= 2;	// Skip CRLF after boundary part
								stopReadingStream = true;
							}
						}
					}

					if (NULL == bodyContentBuffer)
					{
						// There's no Content-Length field in header
						if (0 == contentLength)
						{
							bodyContentBuffer = XBOX::vMalloc (bufferSize, 0);
							bodyContentSize = 0;
						}
						// There's one Content-Length, just check it match limits
						else if ((contentLength > 0) && (contentLength < MAX_REQUEST_ENTITY_SIZE))
						{
							bodyContentBuffer = XBOX::vMalloc (contentLength, 0);
							bodyContentSize = 0;
						}
					}

					if ((NULL != bodyContentBuffer) && (bodyContentSize + unreadBytes < MAX_REQUEST_ENTITY_SIZE))
					{
						XBOX::VSize nbBytesToCopy = unreadBytes;
						if (bodyContentSize + nbBytesToCopy > contentLength)
							bodyContentBuffer = XBOX::vRealloc (bodyContentBuffer, bodyContentSize + nbBytesToCopy);

						memcpy ((char *)(bodyContentBuffer) + bodyContentSize, buffer + bufferOffset, unreadBytes);
						bodyContentSize += unreadBytes;
						bufferOffset = unreadBytes = 0;
					}
					else
					{
						parsingError = XBOX::VE_MEMORY_FULL;

						if (NULL != bodyContentBuffer)
						{
							XBOX::vFree (bodyContentBuffer);
							bodyContentBuffer = NULL;
						}
					}
					break;
				}
			}

			if (XBOX::VE_OK != parsingError)
				break;

			if (NULL != endLinePtr)
			{
				lineLen = (endLinePtr - startLinePtr) + endLineSize; // to skip CRLF;
				bufferOffset += lineLen;
				unreadBytes -= lineLen;
				endLinePtr = NULL;
			}
			else
			{
				if (bufferOffset > 0)
				{
					memmove (buffer, buffer + bufferOffset, unreadBytes);
					buffer[unreadBytes] = 0;
				}
				bufferOffset = unreadBytes;
				break;
			}
		}

		if (XBOX::VE_OK != parsingError)
			break;
	}

	if (!isAlreadyReading)
		inStream.CloseReading();

	if (XBOX::VE_STREAM_EOF == streamError)
		streamError = XBOX::VE_OK;

	if (!parsingError && !streamError)
	{
		if (NULL != bodyContentBuffer)
		{
#if VERSIONDEBUG
			if ((contentLength > 0) && (bodyContentSize != contentLength))
				assert (false);
#endif
			GetBody().SetDataPtr (bodyContentBuffer, bodyContentSize);

			XBOX::VString	contentType;
			XBOX::CharSet	charSet = XBOX::VTC_UNKNOWN;

			/* Set VStream charset according to content-type header other else set default charset to UTF-8 */
			if ((!GetHeaders().GetContentType (contentType, &charSet)) || (XBOX::VTC_UNKNOWN == charSet))
				charSet = XBOX::VTC_UTF_8;
			GetBody().SetCharSet (charSet);
		}

		parsingState = PS_ParsingFinished;
	}
	else
	{
		if (NULL != bodyContentBuffer)
			XBOX::vFree (bodyContentBuffer);
	}

	delete [] boundary;
	boundary = NULL;

	XBOX::vFree (buffer);
	buffer = NULL;

	return streamError;

#undef MAX_BUFFER_LENGTH
}


END_TOOLBOX_NAMESPACE
