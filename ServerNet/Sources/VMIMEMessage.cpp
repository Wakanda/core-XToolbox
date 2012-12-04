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
#include "VMIMEMessage.h"
#include "VNameValueCollection.h"
#include "VHTTPHeader.h"
#include "HTTPTools.h"
#include "VMIMEReader.h"


BEGIN_TOOLBOX_NAMESPACE
USING_TOOLBOX_NAMESPACE
using namespace HTTPTools;


//--------------------------------------------------------------------------------------------------


VMIMEMessage::VMIMEMessage()
: fEncoding (CONST_MIME_MESSAGE_ENCODING_URL)
{
}


VMIMEMessage::~VMIMEMessage()
{
	Clear();
}


void VMIMEMessage::Clear()
{
	fMIMEParts.clear();
}


void VMIMEMessage::_AddTextPart (const XBOX::VString& inName, bool inIsInline, const XBOX::VString& inContentType, const XBOX::VString& inContentID, XBOX::VPtrStream& inStream)
{
	VMIMEMessagePart *partSource = new VMIMEMessagePart();
	if (NULL != partSource)
	{
		partSource->SetName (inName);
		partSource->SetIsInline(inIsInline);
		partSource->SetMediaType (inContentType);
		partSource->SetContentID(inContentID);
		if (XBOX::VE_OK == inStream.OpenReading())
			partSource->PutData (inStream.GetDataPtr(), inStream.GetDataSize());
		inStream.CloseReading();

		fMIMEParts.push_back (partSource);
		partSource->Release();
	}
}

void VMIMEMessage::_AddFilePart (const XBOX::VString& inName, const XBOX::VString& inFileName, bool inIsInline, const XBOX::VString& inContentType, const XBOX::VString& inContentID, XBOX::VPtrStream& inStream)
{
	VMIMEMessagePart *partSource = new VMIMEMessagePart();
	if (NULL != partSource)
	{
		partSource->SetName (inName);
		partSource->SetFileName (inFileName);
		partSource->SetIsInline(inIsInline);
		partSource->SetMediaType (inContentType);
		partSource->SetContentID(inContentID);
		if (XBOX::VE_OK == inStream.OpenReading())
			partSource->PutData (inStream.GetDataPtr(), inStream.GetDataSize());
		inStream.CloseReading();

		fMIMEParts.push_back (partSource);
		partSource->Release();
	}
}


void VMIMEMessage::_AddValuePair (const XBOX::VString& inName, const XBOX::VString& inContentType, void *inData, const XBOX::VSize inDataSize)
{
	VMIMEMessagePart *partSource = new VMIMEMessagePart();
	if (NULL != partSource)
	{
		partSource->SetName (inName);
		partSource->SetMediaType (inContentType);
		partSource->PutData (inData, inDataSize);

		fMIMEParts.push_back (partSource);
		partSource->Release();
	}
}


void VMIMEMessage::Load (bool inFromPOST, const XBOX::VString& inContentType, const XBOX::VString& inURLQuery, const XBOX::VStream& inStream)
{
	Clear();

	if (inFromPOST)
	{
		XBOX::VNameValueCollection	params;

		VHTTPHeader::SplitParameters (inContentType, fEncoding, params);

		if (HTTPTools::EqualASCIIVString (fEncoding, CONST_MIME_MESSAGE_ENCODING_MULTIPART))
		{
			fBoundary = params.Get (CONST_MIME_MESSAGE_BOUNDARY);
			_ReadMultipart (inStream);
		}
		else
		{
			_ReadUrl (inStream);
		}
	}
	else
	{
		_ReadUrl (inURLQuery);
	}
}


void VMIMEMessage::LoadMail (const XBOX::VString &inBoundary, const XBOX::VStream &inStream)
{
	fBoundary = inBoundary;
	_ReadMultipartMail(inStream);	
}


void VMIMEMessage::_ReadUrl (const XBOX::VStream& inStream)
{
	XBOX::VStream& stream = const_cast<XBOX::VStream&>(inStream);

	if (XBOX::VE_OK == stream.OpenReading())
	{
		XBOX::VString urlString;
		stream.GetText (urlString);
		_ReadUrl (urlString);
		stream.CloseReading();
	}
}


void VMIMEMessage::_ReadUrl (const XBOX::VString& inString)
{
	if (!inString.IsEmpty())
	{
		const UniChar *stringPtr = inString.GetCPointer();
		UniChar ch = *stringPtr;

		while (ch != '\0')
		{
			XBOX::VString	name;
			XBOX::VString	value;

			while (ch != '\0' && ch != CHAR_EQUALS_SIGN && ch != CHAR_AMPERSAND)
			{
				if (ch == CHAR_PLUS_SIGN) ch = CHAR_SPACE;
				name.AppendUniChar (ch);
				ch = *(++stringPtr);
			}

			if (ch == CHAR_EQUALS_SIGN)
			{
				ch = *(++stringPtr);
				while (ch != '\0' && ch != CHAR_AMPERSAND)
				{
					if (ch == CHAR_PLUS_SIGN) ch = CHAR_SPACE;
					value.AppendUniChar (ch);
					ch = *(++stringPtr);
				}
			}

			XBOX::VString decodedName (name);
			XBOX::VString decodedValue (value);
			XBOX::VURL::Decode (decodedName);
			XBOX::VURL::Decode (decodedValue);

			XBOX::StStringConverter<char> buffer (decodedValue, XBOX::VTC_UTF_8);
			_AddValuePair (decodedName, CONST_TEXT_PLAIN_UTF_8, (void *)buffer.GetCPointer(), buffer.GetLength());

			if (ch == CHAR_AMPERSAND) ch = *(++stringPtr);
		}
	}
}


void VMIMEMessage::_ReadMultipart (const XBOX::VStream& inStream)
{
	XBOX::VStream& stream = const_cast<XBOX::VStream&>(inStream);

	VMIMEReader reader (fBoundary, stream);
	while (reader.HasNextPart())
	{
		VHTTPMessage message;
		reader.GetNextPart (message);

		XBOX::VString dispositionValue;
		XBOX::VNameValueCollection params;
		if (message.GetHeaders().IsHeaderSet (STRING_HEADER_CONTENT_DISPOSITION))
		{
			XBOX::VString contentDispositionHeaderValue;
			message.GetHeaders().GetHeaderValue (STRING_HEADER_CONTENT_DISPOSITION, contentDispositionHeaderValue);
			VHTTPHeader::SplitParameters (contentDispositionHeaderValue, dispositionValue, params, true); // YT 25-Jan-2012 - ACI0075142
		}

		if (params.Has (CONST_MIME_PART_FILENAME))
		{
			XBOX::VString fileName;
			XBOX::VString name;
			XBOX::VString contentType;
			XBOX::VString contentID;

			message.GetHeaders().GetHeaderValue (STRING_HEADER_CONTENT_TYPE, contentType);
			fileName = params.Get (CONST_MIME_PART_FILENAME);
			name = params.Get (CONST_MIME_PART_NAME);

			// Default as "attachment", not "inline".

			_AddFilePart (name, fileName, false, contentType, contentID, message.GetBody());
			message.SetDisposeBody (false); // Data ownership is transfered to VMIMEMessagePart object
		}
		else
		{
			XBOX::VString		nameString = params.Get (CONST_MIME_PART_NAME);
			XBOX::VURL::Decode (nameString);
			_AddValuePair (nameString, CONST_TEXT_PLAIN, message.GetBody().GetDataPtr(), message.GetBody().GetDataSize());
		}
	}
}


void VMIMEMessage::_ReadMultipartMail (const XBOX::VStream& inStream)
{
	XBOX::VStream	&stream	= const_cast<XBOX::VStream&>(inStream);
	VMIMEReader		reader(fBoundary, stream);

	while (reader.HasNextPart()) {
		
		XBOX::VHTTPMessage	message;

		reader.GetNextPart(message);

		// Parse header of part.

		XBOX::VHTTPHeader							header		= message.GetHeaders();
		const XBOX::VNameValueCollection			&headerList	= header.GetHeaderList();
		XBOX::VNameValueCollection::ConstIterator	it;

		XBOX::VString	name, fileName, contentType, contentID;
		bool			isInline, isBase64, isQuotedPrintable;

		isInline = isBase64 = isQuotedPrintable = false;
		for (it = headerList.begin(); it != headerList.end(); it++) {

			XBOX::VString				value;
			XBOX::VNameValueCollection	params;			

			if (HTTPTools::EqualASCIIVString(it->first, "Content-Type", false)) {

				header.GetHeaderValue(it->first, contentType);
				VHTTPHeader::SplitParameters(contentType, value, params);

				if (params.Has("name")) {

					name = params.Get("name");
					_UnQuote(&name, '"', '"');
				
				}

			} else if (HTTPTools::EqualASCIIVString(it->first, "Content-Disposition", false)) {

				XBOX::VString	disposition;

				header.GetHeaderValue(it->first, value);
				VHTTPHeader::SplitParameters(value, disposition, params);

				isInline = HTTPTools::EqualASCIIVString(disposition, "inline");

				if (params.Has("filename")) {

					fileName = params.Get("filename");
					_UnQuote(&fileName, '"', '"');
				
				}

			} else if (HTTPTools::EqualASCIIVString(it->first, "Content-Transfer-Encoding", false)) {

				XBOX::VString	encoding;

				header.GetHeaderValue(it->first, value);
				VHTTPHeader::SplitParameters(value, encoding, params);

				isBase64 = HTTPTools::EqualASCIIVString(encoding, "base64");
				isQuotedPrintable = HTTPTools::EqualASCIIVString(encoding, "quoted-printable");

			} else if (HTTPTools::EqualASCIIVString(it->first, "Content-ID", false)) {

				header.GetHeaderValue(it->first, value);
				VHTTPHeader::SplitParameters(value, contentID, params);

				_UnQuote(&contentID, '<', '>');

			} else {

				// Ignore unknown fields.

			}

		}

		//  Get body of part, decode it if need.

		XBOX::VMemoryBuffer<>	decodedData;
		XBOX::VPtrStream		decodedBody;
		
		XBOX::VPtrStream		*body	= message.GetBodyPtr();

		if (isBase64) {

			XBOX::Base64Coder::Decode(body->GetDataPtr(), body->GetDataSize(), decodedData);

			decodedBody.SetDataPtr(decodedData.GetDataPtr(), decodedData.GetDataSize());
			decodedData.ForgetData();

			body = &decodedBody;

		} else if (isQuotedPrintable) {

			VMIMEReader::DecodeQuotedPrintable (body->GetDataPtr(), body->GetDataSize(), &decodedData);

			decodedBody.SetDataPtr(decodedData.GetDataPtr(), decodedData.GetDataSize());
			decodedData.ForgetData();

			body = &decodedBody;

		}

		if (fileName.IsEmpty()) 

			_AddTextPart(name, isInline, contentType, contentID, *body);

		else

			_AddFilePart(name, fileName, isInline, contentType, contentID, *body);

		decodedBody.Clear();

	}
}


XBOX::VError VMIMEMessage::ToStream (XBOX::VStream& outStream, sLONG inEncoding)
{
	XBOX::VError error = XBOX::VE_OK;

	if (XBOX::VE_OK == outStream.OpenWriting())
	{
		if (fMIMEParts.size() > 0)
		{
			XBOX::VString	string;
			XBOX::VString	charsetName;
			bool			bEncodeBody;	// Encode using base64.

			for (XBOX::VectorOfMIMEPart::const_iterator it = fMIMEParts.begin(); it != fMIMEParts.end(); ++it)
			{
				outStream.PutPrintf ("\r\n--%S\r\n", &fBoundary);

				string.FromCString ("Content-Type: ");
				string.AppendString ((*it)->GetMediaType());
 
				if (!(*it)->GetFileName().IsEmpty())
				{
					string.AppendCString ("; name=\"");
					if (!(*it)->GetName().IsEmpty())
						string.AppendString ((*it)->GetName());
					else
						string.AppendString ((*it)->GetFileName());

					string.AppendCString("\"\r\nContent-Disposition: ");
					string.AppendCString((*it)->IsInline() ? "inline; " : "attachment; ");					
					string.AppendCString("filename=\"");

					string.AppendString ((*it)->GetFileName());
					string.AppendCString ("\"\r\n");

					if (inEncoding == ENCODING_BINARY) {

						string.AppendCString ("Content-Transfer-Encoding: 8bit\r\n");
						bEncodeBody = false;

					} else {

						if ((inEncoding == ENCODING_BINARY_ONLY) && ((*it)->GetMediaTypeKind() == MIMETYPE_TEXT))
						{
							bEncodeBody = false;
						}
						else
						{
							string.AppendCString ("Content-Transfer-Encoding: base64\r\n");
							bEncodeBody = true;
						}
					}
				}
				else
				{
					if ((*it)->GetMediaTypeCharSet() != XBOX::VTC_UNKNOWN)
					{
						string.AppendCString ("; charset=\"");
						XBOX::VTextConverters::Get()->GetNameFromCharSet ((*it)->GetMediaTypeCharSet(), charsetName);
						string.AppendString (charsetName);
						string.AppendCString ("\"");
					}

					if (!(*it)->GetName().IsEmpty()) {

						string.AppendCString("; name=\"");
						string.AppendString((*it)->GetName());
						string.AppendCString("\"");

					}

					string.AppendCString ("\r\n");

					if ((*it)->IsInline())

						string.AppendCString("Content-Disposition: inline\r\n");
						
					if (inEncoding == ENCODING_7BIT || ((inEncoding == ENCODING_BINARY_ONLY) && ((*it)->GetMediaTypeKind() != MIMETYPE_TEXT))) {

						string.AppendCString ("Content-Transfer-Encoding: base64\r\n");
						bEncodeBody = true;

					} else {

						string.AppendCString("Content-Transfer-Encoding: 8bit\r\n");
						bEncodeBody = false;

					}
				}

				if ((*it)->GetContentID().GetLength()) {

					string.AppendCString("Content-ID: <");
					string.AppendString((*it)->GetContentID());
					string.AppendCString(">\r\n");

				}

				string.AppendCString ("\r\n");

				outStream.PutText (string);

				if (bEncodeBody)
				{
					XBOX::VMemoryBuffer<> buffer;
					if (XBOX::Base64Coder::Encode ((*it)->GetData().GetDataPtr(), (*it)->GetData().GetDataSize(), buffer, kBASE64_QUADS_PER_LINE))
					{
						outStream.PutData (buffer.GetDataPtr(), buffer.GetDataSize());
					}
				}
				else
				{
					outStream.PutData ((*it)->GetData().GetDataPtr(), (*it)->GetData().GetDataSize());
				}
			}

			outStream.PutPrintf ("\r\n--%S--\r\n", &fBoundary);
		}

		outStream.CloseWriting();
	}

	return error;
}


void VMIMEMessage::_UnQuote (XBOX::VString *ioString, UniChar inStartQuote, UniChar inEndQuote)
{
	xbox_assert(ioString != NULL);

	if (ioString->GetLength() >= 2
	&& ioString->GetUniChar(1) == inStartQuote 
	&& ioString->GetUniChar(ioString->GetLength()) == inEndQuote) 

		ioString->SubString(2, ioString->GetLength() - 2);
}


END_TOOLBOX_NAMESPACE