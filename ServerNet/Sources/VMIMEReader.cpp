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
#include "VMIMEReader.h"
#include "HTTPTools.h"


BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE
using namespace HTTPTools;


VMIMEReader::VMIMEReader (const XBOX::VString& inBoundary, XBOX::VStream& inStream)
: fStream (inStream)
, fBoundary (inBoundary)
, fFirstBoundaryDelimiter()
, fLastBoundaryDelimiter()
{
	fStream.OpenReading ();
}


VMIMEReader::~VMIMEReader()
{
	fStream.CloseReading ();
}


void VMIMEReader::GetNextPart (VHTTPMessage& ioMessage)
{
	if (fBoundary.IsEmpty())
		_GuessBoundary();
	else
		_FindFirstBoundary();

	_ParseMessage (ioMessage);
}


bool VMIMEReader::HasNextPart()
{
	return (!_LastPart());
}


bool VMIMEReader::_LastPart()
{
	XBOX::VString	lineString;
	XBOX::VError	error = XBOX::VE_OK;

	if (fLastBoundaryDelimiter.IsEmpty())
	{
		fLastBoundaryDelimiter.FromString (CONST_MIME_MESSAGE_BOUNDARY_DELIMITER);
		fLastBoundaryDelimiter.AppendString (fBoundary);
		fLastBoundaryDelimiter.AppendString (CONST_MIME_MESSAGE_BOUNDARY_DELIMITER);
	}

	sLONG8 lastPos = fStream.GetPos();
	error = fStream.GetTextLine (lineString, false, NULL);
	fStream.SetPos (lastPos);
	return ((XBOX::VE_OK != error) || (HTTPTools::EqualASCIIVString (lineString, fLastBoundaryDelimiter)));
}


const XBOX::VString& VMIMEReader::GetBoundary() const
{
	return fBoundary;
}

XBOX::VError VMIMEReader::DecodeQuotedPrintable (const void *inData, VSize inDataSize, VMemoryBuffer<> *outResult)
{
	xbox_assert(inData != NULL && outResult != NULL);

	outResult->Clear();	
	if (!inDataSize)

		return XBOX::VE_OK;	

	// Decoded size can't be greater than encoded size, will do a reallocate later.

	if (!outResult->SetSize(inDataSize))

		return XBOX::VE_MEMORY_FULL;

	uBYTE	*p, *q;
	VSize	bytesLeft;

	p = (uBYTE *) inData;
	q = (uBYTE *) outResult->GetDataPtr();
	bytesLeft = inDataSize;
	for ( ; ; ) {

		uBYTE	c, d;

		if ((c = *p++) != '=') {

			*q++ = c;
			if (!--bytesLeft)
				
				break;

			else

				continue;

		} 

		// Check if it is last line of quoted printable, tolerate a single '=' at end of line without CRLF.
		
		if (!--bytesLeft)

			break;	

		if ((c = *p++) == '\r' || c == ' ' || c == '\t' || c == '\n') {

			// Skip trailing or transport padding white space(s).
			// Ignore until '\n' character, then add CRLF sequence to output.
			// This error more tolerant.

			if (c != '\n') 

				for ( ; ; )
	
					if (!--bytesLeft)

						break;

					else if (*p++ == '\n')

						break;

			if (!--bytesLeft)

				break;

		} else if (::isdigit(c) || (c >= 'A' && c <= 'F')) {

			if (!--bytesLeft) {

				// Reaching end of quoted printable data without a complete hex-octet,
				// pass the bytes unchanged and stop decoding.

				*q++ = '=';
				*q++ = c;
				break;

			} else {
				
				d = *p++;
				if (::isdigit(d) || (d >= 'A' && d <= 'F')) {

					// Valid hex-octet.

					c -= ::isdigit(c) ? '0' : 'A' - 10;
					d -= ::isdigit(d) ? '0' : 'A' - 10;
					*q++ = (c << 4) | d;

				} else {

					// Not a valid hex-octet, pass characters unchanged and continue;

					*q++ = '=';
					*q++ = c;
					*q++ = d;

				}
				if (!--bytesLeft) 

					break;
				
			}

		} else {

			// Not a valid hex-octet, pass the bytes unchanged and continue.

			*q++ = '=';
			*q++ = c;
			if (!--bytesLeft)

				break;

		}

	}

	// Do a resize to keep only decoded data.

	outResult->SetSize(q - (uBYTE *) outResult->GetDataPtr());

	return XBOX::VE_OK;
}

void VMIMEReader::_FindFirstBoundary()
{
	XBOX::VString	lineString;
	XBOX::VError	error = XBOX::VE_OK;

	if (fFirstBoundaryDelimiter.IsEmpty())
	{
		fFirstBoundaryDelimiter.FromString (CONST_MIME_MESSAGE_BOUNDARY_DELIMITER);
		fFirstBoundaryDelimiter.AppendString (fBoundary);
	}

	do
	{
		error = fStream.GetTextLine (lineString, false, NULL);
	}
	while ((XBOX::VE_OK == error) && !HTTPTools::EqualASCIIVString (lineString, fFirstBoundaryDelimiter));
}


void VMIMEReader::_GuessBoundary()
{
	XBOX::VString lineString;

	sLONG8 lastPos = fStream.GetPos();
	fStream.GetTextLine (lineString, false, NULL);

	// Does the line start with "--" ?
	if (!lineString.IsEmpty() && (lineString.GetUniChar (1) == CHAR_HYPHEN_MINUS) && (lineString.GetUniChar (2) == CHAR_HYPHEN_MINUS))
	{
		fBoundary.FromString (lineString);
	}
	else
	{
		fStream.SetPos (lastPos);
	}
}


void VMIMEReader::_ParseMessage (VHTTPMessage& ioMessage)
{
	ioMessage.Clear();
	ioMessage.ReadFromStream (fStream, fBoundary);
}

VMemoryBufferStream::VMemoryBufferStream (const XBOX::VMemoryBuffer<> *inMemoryBuffer)
{
	xbox_assert(inMemoryBuffer != NULL);

	_initialize(inMemoryBuffer, 1);	
}

VMemoryBufferStream::VMemoryBufferStream (const XBOX::VMemoryBuffer<> *inMemoryBuffers, sLONG inNumberMemoryBuffers)
{
	xbox_assert(inMemoryBuffers != NULL);
	xbox_assert(inNumberMemoryBuffers >= 1);

	_initialize(inMemoryBuffers, inNumberMemoryBuffers);	
}

VMemoryBufferStream::~VMemoryBufferStream ()
{
	if (fStartIndexes != NULL) {

		XBOX::vFree(fStartIndexes);
		fStartIndexes = NULL;

	}
}

XBOX::VError VMemoryBufferStream::DoSetPos (sLONG8 inNewPos)
{
	XBOX::VError	error;
	sLONG			index;

	if (inNewPos >= fStartIndexes[fCurrentMemoryBufferIndex]) {

		// Most common case is a DoSetPos() inside one memory buffer, without "crossing".

		if (inNewPos < fStartIndexes[fCurrentMemoryBufferIndex + 1]) 
			
			error = XBOX::VE_OK;

		else {

			index = fCurrentMemoryBufferIndex;
			for ( ; ; ) 

				if (++index == fNumberMemoryBuffers) {

					if (inNewPos == fTotalSize) {

						fCurrentMemoryBufferIndex = fNumberMemoryBuffers - 1;
						fPosition = inNewPos;
						error = XBOX::VE_STREAM_EOF;
					
					} else 

						error = XBOX::VE_STREAM_CANNOT_SET_POS;

					break;

				} else if (inNewPos < fStartIndexes[index]) {

					fCurrentMemoryBufferIndex = index;					
					error = XBOX::VE_OK;
					break;

				}

		}

	} else {

		index = fCurrentMemoryBufferIndex;
		for ( ; ; )

			if (!index) {

				error = XBOX::VE_STREAM_CANNOT_SET_POS;
				break;

			} else {

				index--;
				if (inNewPos >= fStartIndexes[index]) {

					fCurrentMemoryBufferIndex = index;					
					error = XBOX::VE_OK;
					break;

				}

			}

	}

	return error;
}

sLONG8 VMemoryBufferStream::DoGetSize ()
{
	return fTotalSize;
}

XBOX::VError VMemoryBufferStream::DoSetSize (sLONG8 inNewSize)
{
	xbox_assert(false);

	return XBOX::VE_UNIMPLEMENTED;	// This is not possible.
}

XBOX::VError VMemoryBufferStream::DoUngetData (const void *inBuffer, VSize inNbBytes)
{
	if (!inNbBytes)

		return XBOX::VE_OK;

	xbox_assert(inBuffer != NULL);

	if (DoSetPos(fPosition - inNbBytes) != XBOX::VE_OK)

		return XBOX::VE_STREAM_TOO_MANY_UNGET_DATA;

	else

		return XBOX::VE_OK;
}

XBOX::VError VMemoryBufferStream::DoPutData (const void *inBuffer, VSize inNbBytes)
{
	if (!inNbBytes)

		return XBOX::VE_OK;

	xbox_assert(inBuffer != NULL);

	const uBYTE		*p;
	XBOX::VError	error;
	VSize			position;

	p = (uBYTE *) inBuffer;
	error = XBOX::VE_OK;
	position = (VSize) fPosition;
	for ( ; ; ) {

		VSize	count;
		
		if ((count = fStartIndexes[fCurrentMemoryBufferIndex + 1] - position)) {

			uBYTE	*q;

			if (count > inNbBytes)

				count = inNbBytes;

			q = (uBYTE *) fMemoryBuffers[fCurrentMemoryBufferIndex].GetDataPtr();
			q += position - fStartIndexes[fCurrentMemoryBufferIndex];

			::memcpy(q, p, count);

			inNbBytes -= count;
			fTotalBytes += count;
			
			if ((position += count) == fStartIndexes[fCurrentMemoryBufferIndex + 1]) {

				if (position != fTotalSize) {

					// Keep fCurrentMemoryBufferIndex up to date.
					// See comment at end of function for fPosition.

					fCurrentMemoryBufferIndex++;

				} else {

					error = XBOX::VE_STREAM_EOF;
					break;

				}

			}

			if (inNbBytes)

				p += count;

			else

				break;
			
		} else {

			error = XBOX::VE_STREAM_EOF;
			break;

		}

	}

	// If error is equal to XBOX::VE_OK, then VStream::PutData() will update fPosition.
	// Otherwise, do it here.

	if (error != XBOX::VE_OK)

		fPosition = position;

	return error;
}

XBOX::VError VMemoryBufferStream::DoGetData (void *outBuffer, VSize *ioCount)
{
	xbox_assert(ioCount != NULL);

	VSize	bytesToRead, bytesRead;

	if (!(bytesToRead = *ioCount)) 

		return XBOX::VE_OK;

	else

		bytesRead = 0;		

	xbox_assert(outBuffer != NULL);

	uBYTE			*p;
	XBOX::VError	error;
	VSize			position;

	p = (uBYTE *) outBuffer;
	error = XBOX::VE_OK;
	position = (VSize) fPosition;
	for ( ; ; ) {

		VSize	count;
		
		if ((count = fStartIndexes[fCurrentMemoryBufferIndex + 1] - position)) {

			const uBYTE	*q;

			if (count > bytesToRead)

				count = bytesToRead;

			q = (uBYTE *) fMemoryBuffers[fCurrentMemoryBufferIndex].GetDataPtr();
			q += position - fStartIndexes[fCurrentMemoryBufferIndex];

			::memcpy(p, q, count);

			bytesToRead -= count;
			fTotalBytes += count;
			bytesRead += count;
			
			if ((position += count) == fStartIndexes[fCurrentMemoryBufferIndex + 1]) {

				if (position != fTotalSize) {

					// Keep fCurrentMemoryBufferIndex up to date.
					// VStream::GetData() will update fPosition.

					fCurrentMemoryBufferIndex++;

				} else {

					error = XBOX::VE_STREAM_EOF;
					break;

				}

			}

			if (bytesToRead)

				p += count;				

			else

				break;
			
		} else {

			error = XBOX::VE_STREAM_EOF;
			break;

		}

	}

	*ioCount = bytesRead;
	return error;
}

void VMemoryBufferStream::_initialize (const XBOX::VMemoryBuffer<> *inMemoryBuffers, sLONG inNumberMemoryBuffers)
{
	xbox_assert(inMemoryBuffers != NULL);
	xbox_assert(inNumberMemoryBuffers >= 1);

	fMemoryBuffers = inMemoryBuffers;
	fNumberMemoryBuffers = inNumberMemoryBuffers;

	if ((fStartIndexes = (VSize *) XBOX::vMalloc(sizeof(VSize) * (fNumberMemoryBuffers + 1), 0)) == NULL)

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);

	else {

		VSize	i, j;

		j = 0;
		for (i = 0; i < fNumberMemoryBuffers; i++) {

			fStartIndexes[i] = j;
			j += fMemoryBuffers[i].GetDataSize();

		}
		fStartIndexes[i] = fTotalSize = j;		

		fCurrentMemoryBufferIndex = 0;
		fTotalBytes = 0;

	}
}

END_TOOLBOX_NAMESPACE