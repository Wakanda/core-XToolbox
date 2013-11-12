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

#include "VWebSocket.h"

USING_TOOLBOX_NAMESPACE

// See section 4.2.2 of spec.

const char	VWebSocket::SERVER_KEY_SUFFIX[]	= "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

void VWebSocketFrame::SetRSVFlags (uBYTE inRSVFlags)
{
	xbox_assert(!(inRSVFlags & ~0x70));

	fRSVFlags = inRSVFlags;
}

void VWebSocketFrame::SetOpcode (uBYTE inOpcode)
{
	xbox_assert(!(inOpcode & ~0xf));

	fOpcode = inOpcode;
}

void VWebSocketFrame::SetMaskingKey (uLONG inMaskingKey)
{	
	fMaskingKeyFlag = true; 
	fMaskingKey = inMaskingKey;			
}

void VWebSocketFrame::GenerateMaskingKey ()
{
	fMaskingKeyFlag = true; 
	fMaskingKey = (uLONG) XBOX::VSystem::Random(false);	// Should use a secure RNG instead!
}

void VWebSocketFrame::ApplyMaskingKey (
	const uBYTE *inMaskedData, 
	uBYTE *outUnmaskedData, 
	uLONG inMaskingKey, 
	VSize inDataLength,
	uLONG inModuloIndex)
{
	xbox_assert(inMaskedData != NULL && outUnmaskedData != NULL && !(inModuloIndex & ~0x3));

	uBYTE	maskingKey[4];

	maskingKey[0] = inMaskingKey >> 24;
	maskingKey[1] = inMaskingKey >> 16;
	maskingKey[2] = inMaskingKey >> 8;
	maskingKey[3] = inMaskingKey;

	for (VSize i = 0; i < inDataLength; i++, inModuloIndex = (inModuloIndex + 1) & 0x3)

		outUnmaskedData[i] = inMaskedData[i] ^ maskingKey[inModuloIndex];
}

XBOX::VError VWebSocketFrame::Decode (const uBYTE *inFrame, VSize *ioFrameLength)
{
	xbox_assert(inFrame != NULL && ioFrameLength != NULL);	

	// A frame header is at least 2 bytes long.

	if (*ioFrameLength < 2)

		return VE_SRVR_WEBSOCKET_HEADER_TOO_SHORT;

	fFIN = (inFrame[0] & 0x80) != 0;
	fRSVFlags = inFrame[0] & (FLAG_RSV1 | FLAG_RSV2 | FLAG_RSV3);
	fOpcode = inFrame[0] & 0xf;
	
	if ((fPayloadDataLength = inFrame[1] & 0x7f) < 126) 

		fPayloadData = &inFrame[2];

	else if (fPayloadDataLength == 126) {
		
		if (*ioFrameLength < 4)

			return VE_SRVR_WEBSOCKET_HEADER_TOO_SHORT;

		fPayloadDataLength = inFrame[2] << 8;
		fPayloadDataLength |= inFrame[3];

#if WEBSOCKET_STRICT_FRAME_DECODER

		// Length must be encoded using minimal method.

		if (fPayloadDataLength < 126)

			return VE_SRVR_WEBSOCKET_HEADER_INCORRECT;

#endif

		fPayloadData = &inFrame[4];

	} else {

		if (*ioFrameLength < 10)

			return VE_SRVR_WEBSOCKET_HEADER_TOO_SHORT;

		// 64-bit length must have highest bit as zero.

		if (inFrame[2] & 0x80)

			return VE_SRVR_WEBSOCKET_HEADER_INCORRECT;	

		fPayloadDataLength = (uLONG8) inFrame[2] << 56;
		fPayloadDataLength |= (uLONG8) inFrame[3] << 48;
		fPayloadDataLength |= (uLONG8) inFrame[4] << 40;
		fPayloadDataLength |= (uLONG8) inFrame[5] << 32;
		fPayloadDataLength |= inFrame[6] << 24;
		fPayloadDataLength |= inFrame[7] << 16;
		fPayloadDataLength |= inFrame[8] << 8;
		fPayloadDataLength |= inFrame[9];

#if WEBSOCKET_STRICT_FRAME_DECODER

		// Length must be encoded using minimal method.

		if (fPayloadDataLength < 0xffff)

			return VE_SRVR_WEBSOCKET_HEADER_INCORRECT;

#endif

		fPayloadData = &inFrame[10];

	}

	if ((fMaskingKeyFlag = (inFrame[1] >> 7) != 0)) {

		if (inFrame + *ioFrameLength < fPayloadData + 4)

			return VE_SRVR_WEBSOCKET_HEADER_TOO_SHORT;

		fMaskingKey = fPayloadData[0] << 24;
		fMaskingKey |= fPayloadData[1] << 16;
		fMaskingKey |= fPayloadData[2] << 8;
		fMaskingKey |= fPayloadData[3];

		fPayloadData += 4;

	}

	uLONG8	frameLength;

	frameLength = fPayloadData - inFrame;	// Header size.
	frameLength += fPayloadDataLength;	
	if (*ioFrameLength < frameLength)

		return VE_SRVR_WEBSOCKET_FRAME_TOO_SHORT;
		
	else {

		*ioFrameLength = (VSize) frameLength;
		return XBOX::VE_OK;

	}
}

XBOX::VError VWebSocketFrame::Encode (XBOX::VMemoryBuffer<> *outFrame, VSize inDataLength) const
{
	xbox_assert(outFrame != NULL && inDataLength <= fPayloadDataLength);

	xbox_assert(!(!fFIN && (fOpcode & 0x8)));	// It is forbidden to fragment control message.
	
	VSize	frameSize;
	
	if (fPayloadDataLength < 126) 

		frameSize = fMaskingKeyFlag ? 6 : 2; 

	else if (fPayloadDataLength < 0xffff) 

		frameSize = fMaskingKeyFlag ? 6 : 4; 

	else

		frameSize = fMaskingKeyFlag ? 6 : 10; 

	frameSize += inDataLength;

	if (!outFrame->SetSize(frameSize))

		return XBOX::VE_MEMORY_FULL;

	uBYTE	*frame;

	frame = (uBYTE *) outFrame->GetDataPtr();
	xbox_assert(frame != NULL);

	frame[0] = fFIN ? 0x80 : 0;

	xbox_assert(!(fRSVFlags & ~0x70));
	frame[0] |= fRSVFlags;

	xbox_assert(!(fOpcode & ~0xf));
	frame[0] |= fOpcode;

	xbox_assert(fPayloadDataLength <= kMAX_VSize);

	VIndex	index;
	
	if (fPayloadDataLength < 126) {

		frame[1] = fPayloadDataLength;
		index = 2;
		
	} else if (fPayloadDataLength < 0xffff) {

		frame[1] = 126;
		frame[2] = fPayloadDataLength >> 8;
		frame[3] = fPayloadDataLength;
		index = 4;

	} else {

		xbox_assert(!(fPayloadDataLength & 0x8000000000000000ull));

		frame[1] = 127;
		frame[2] = fPayloadDataLength >> 56;
		frame[3] = fPayloadDataLength >> 48;
		frame[4] = fPayloadDataLength >> 40;
		frame[5] = fPayloadDataLength >> 32;
		frame[6] = fPayloadDataLength >> 24;
		frame[7] = fPayloadDataLength >> 16;
		frame[8] = fPayloadDataLength >> 8;
		frame[9] = fPayloadDataLength;
		index = 10;

	}

	if (fMaskingKeyFlag) {

		frame[1] |= 0x80;

		uBYTE	*maskingKey, *maskedPayloadData;

		maskingKey = &frame[index];
		maskingKey[0] = fMaskingKey >> 24;
		maskingKey[1] = fMaskingKey >> 16;
		maskingKey[2] = fMaskingKey >> 8;
		maskingKey[3] = fMaskingKey;

		maskedPayloadData = maskingKey + 4;
		for (VSize i = 0; i < inDataLength; i++)

			maskedPayloadData[i] = fPayloadData[i] ^ maskingKey[i & 0x3];		

	} else 

		::memcpy(&frame[index], fPayloadData, inDataLength);

	return XBOX::VE_OK;
}

XBOX::VError VWebSocketFrame::Encode (
	std::vector<XBOX::VMemoryBuffer<> > *outFrames, 
	XBOX::VSize inMaximumFrameSize, bool inGenerateMaskingKey)
{
	xbox_assert(outFrames != NULL);

	xbox_assert(fOpcode);	// Must not be a continuation frame, use low-level Encode() instead.

	XBOX::VError	error;

	outFrames->clear();
	if (inMaximumFrameSize || fPayloadDataLength + 14 <= inMaximumFrameSize) {

		fFIN = true;
		if (inGenerateMaskingKey)

			GenerateMaskingKey();

		xbox_assert(fPayloadDataLength <= kMAX_VSize);

		outFrames->resize(1);
		error = Encode(&outFrames->at(0), fPayloadDataLength);

	} else {

		uBYTE		opcode;
		const uBYTE	*payloadData;
		uLONG8		payloadDataLength;
		VSize		maximumDataLength;

		opcode = fOpcode;
		payloadData = fPayloadData;
		payloadDataLength = fPayloadDataLength;
		maximumDataLength = GetMaximumDataLength(inGenerateMaskingKey || fMaskingKeyFlag, inMaximumFrameSize);

		uLONG8	bytesLeft;
		uLONG	numberFrames;
		
		bytesLeft = payloadDataLength;
		numberFrames = 0;
		for ( ; ; ) {

			VSize	dataLength;

			dataLength = bytesLeft < maximumDataLength ? bytesLeft : maximumDataLength;
			
			fFIN = !(bytesLeft - dataLength); 
			if (inGenerateMaskingKey)

				GenerateMaskingKey();
			
			fPayloadDataLength = dataLength;

			// Encode() will always make smallest possible header.

			outFrames->resize(numberFrames + 1);
			if ((error = Encode(&outFrames->at(numberFrames), dataLength)) != XBOX::VE_OK)

				break;

			numberFrames++;

			if (!fFIN) {

				bytesLeft -= dataLength;

				fOpcode = 0;	// Continuation frame.
				fPayloadData += dataLength;

			} else

				break;

		}

		fOpcode = opcode;
		fPayloadData = payloadData;
		fPayloadDataLength = payloadDataLength;

	}

	return error;
}

VSize VWebSocketFrame::GetMaximumDataLength (bool inUseMaskingKey, VSize inMaximumFrameSize)
{
	VSize	smallHeaderSize, mediumHeaderSize, bigHeaderSize; 

	if (inUseMaskingKey) {

		smallHeaderSize = 2 + 4;
		mediumHeaderSize = 4 + 4;
		bigHeaderSize = 10 + 4;
			
	} else {

		smallHeaderSize = 2;
		mediumHeaderSize = 4;
		bigHeaderSize = 10;
			
	}

	xbox_assert(inMaximumFrameSize >= 15);		// Makes no sense to make frame too small.

	if (inMaximumFrameSize - bigHeaderSize > 0xffff)

		return inMaximumFrameSize - bigHeaderSize;

	else if (inMaximumFrameSize - mediumHeaderSize > 125)

		return inMaximumFrameSize - mediumHeaderSize;

	else

		return inMaximumFrameSize - smallHeaderSize;
}

VWebSocket::VWebSocket (XBOX::VTCPEndPoint *inEndPoint, bool inUseMaskingKey, VSize inMaximumFrameSize)
{
	xbox_assert(inEndPoint != NULL);

	fEndPoint = XBOX::RetainRefCountable<XBOX::VTCPEndPoint>(inEndPoint);
	fUseMaskingKey = inUseMaskingKey;	
	SetMaximumFrameSize(inMaximumFrameSize);

	fState = STATE_OPEN;
	fBytesLeft = 0;
}

VWebSocket::~VWebSocket ()
{
	if (fEndPoint != NULL) 

		ForceClose();
}

void VWebSocket::SetMaximumFrameSize (VSize inMaximumFrameSize)
{
	xbox_assert(!inMaximumFrameSize || inMaximumFrameSize >= 15);

	fMaximumFrameSize = inMaximumFrameSize;	
}

XBOX::VError VWebSocket::ForceClose ()
{
	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	XBOX::VError	error;

	fState = STATE_CLOSED;
	if (fEndPoint != NULL) {

		error = fEndPoint->ForceClose();
		XBOX::ReleaseRefCountable<XBOX::VTCPEndPoint>(&fEndPoint);

	} else

		error = XBOX::VE_OK;
	
	return error;
}

XBOX::VError VWebSocket::HandleFrame (const uBYTE *inFrame, VSize *ioFrameLength)
{
	xbox_assert(inFrame != NULL && ioFrameLength != NULL);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (GetState() != STATE_OPEN && GetState() != STATE_CLOSING)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_READ;
	
	XBOX::VError	error;

	if ((error = fDecodingWebSocketFrame.Decode(inFrame, ioFrameLength)) == XBOX::VE_OK) {

		// A complete frame has been successfully received and parsed. Control frames cannot be fragmented.

		uBYTE	opcode;

		if ((opcode = fDecodingWebSocketFrame.GetOpcode()) == VWebSocketFrame::OPCODE_PING) {

			// Answer ping.

			if (fState != STATE_CLOSING) {

				// Be tolerant, if in closing state, just ignore ping frames instead of treating them as errors.

				if (fDecodingWebSocketFrame.HasMaskingKey() && fDecodingWebSocketFrame.GetPayloadLength()) {

					XBOX::VMemoryBuffer<>	buffer;
				
					if ((error = _UnmaskPayloadData(&buffer)) == XBOX::VE_OK)

						error = SendPing(
									fDecodingWebSocketFrame.GetRSVFlags(), 
									(const uBYTE *) buffer.GetDataPtr(), 
									buffer.GetDataSize());

				} else 
					
					error = SendPing(
								fDecodingWebSocketFrame.GetRSVFlags(), 
								fDecodingWebSocketFrame.GetPayloadData(), 
								fDecodingWebSocketFrame.GetPayloadLength());

			}

		} else if (opcode == VWebSocketFrame::OPCODE_CONNECTION_CLOSE) {

			// If we're in "open" state, then closing handshake has been initiated by peer. Send answer and go to "closed" state.
			// Otherwise, we've received the answer from the close control frame we've previously sent, so we can also go to the
			// "closed" state. Still, it is up to the user to check that data (if any) has been mirrored correctly.

			if (fState == STATE_OPEN) {

				if (fDecodingWebSocketFrame.HasMaskingKey() && fDecodingWebSocketFrame.GetPayloadLength()) {

					XBOX::VMemoryBuffer<>	buffer;
				
					if ((error = _UnmaskPayloadData(&buffer)) == XBOX::VE_OK)

						error = SendClose(
									fDecodingWebSocketFrame.GetRSVFlags(), 
									(const uBYTE *) buffer.GetDataPtr(), 
									buffer.GetDataSize());

				} else 
					
					error = SendClose(
								fDecodingWebSocketFrame.GetRSVFlags(), 
								fDecodingWebSocketFrame.GetPayloadData(), 
								fDecodingWebSocketFrame.GetPayloadLength());


			} 

			fState = STATE_CLOSED;
			error = ForceClose();

		} 

	}
	
	return error;
}

XBOX::VError VWebSocket::SendMessage (const uBYTE *inMessage, VSize inLength, bool inIsText)
{
	xbox_assert(!(inMessage == NULL && inLength) && !(inMessage != NULL && !inLength));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_NOT_OPEN_STATE;

	XBOX::VError						error;
	std::vector<XBOX::VMemoryBuffer<> > frames;

	fEncodingWebSocketFrame.SetRSVFlags(0);
	fEncodingWebSocketFrame.SetOpcode(inIsText ? VWebSocketFrame::OPCODE_TEXT_FRAME : VWebSocketFrame::OPCODE_BINARY_FRAME);
	fEncodingWebSocketFrame.SetPayloadLength(inLength);
	if (inLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inMessage);

	// FIN flag and masking keys are handled by Encode() function.

	if ((error = fEncodingWebSocketFrame.Encode(&frames, fMaximumFrameSize, fUseMaskingKey)) == XBOX::VE_OK)
	
		for (uLONG i = 0; i < frames.size(); i++) 
	
			if ((error = fEndPoint->WriteExactly(frames[i].GetDataPtr(), (uLONG) frames[i].GetDataSize(), DEFAULT_WRITE_TIMEOUT)) != XBOX::VE_OK)

				break;

	return error;
}

XBOX::VError VWebSocket::StartMessage (VSize inTotalLength, bool inIsText, uBYTE inRSVFlags, const uBYTE *inMessageStart, VSize inStartLength)
{
	xbox_assert(inTotalLength > 0 && inMessageStart != NULL && inStartLength <= inTotalLength);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_NOT_OPEN_STATE;

	if (fBytesLeft)

		return VE_SRVR_WEBSOCKET_MESSAGE_ALREADY_STARTED;

	fBytesLeft = inTotalLength - inStartLength;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(!fBytesLeft);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(inIsText ? VWebSocketFrame::OPCODE_TEXT_FRAME : VWebSocketFrame::OPCODE_BINARY_FRAME);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inStartLength);
	if (inStartLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inMessageStart);

	if ((error = fEncodingWebSocketFrame.Encode(&fBuffer, inStartLength)) == XBOX::VE_OK)

		error = fEndPoint->WriteExactly(fBuffer.GetDataPtr(), (uLONG) fBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT);

	return error;
}

XBOX::VError VWebSocket::ContinueMessage (uBYTE inRSVFlags, const uBYTE *inData, VSize inLength)
{
	xbox_assert(inData != NULL && inLength > 0);

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_NOT_OPEN_STATE;

	if (!fBytesLeft)

		return VE_SRVR_WEBSOCKET_MESSAGE_NOT_STARTED;

	if (fBytesLeft < inLength)

		return VE_SRVR_WEBSOCKET_TOO_MUCH_DATA;

	fBytesLeft -= inLength;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(!fBytesLeft);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(VWebSocketFrame::OPCODE_CONTINUATION_FRAME);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inLength);
	if (inLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fBuffer, inLength)) == XBOX::VE_OK)

		error = fEndPoint->WriteExactly(fBuffer.GetDataPtr(), (uLONG) fBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT);

	return error;
}
	
XBOX::VError VWebSocket::SendPing (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength)
{
	return _SendPingOrPong(inRSVFlags, inData, inDataLength, true);
}

XBOX::VError VWebSocket::SendPong (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength)
{
	return _SendPingOrPong(inRSVFlags, inData, inDataLength, false);
}

XBOX::VError VWebSocket::SendClose (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength)
{
	xbox_assert(!(inData == NULL && inDataLength) && !(inData != NULL && !inDataLength));
	xbox_assert(!(inDataLength && inDataLength < 2));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN && fState != STATE_CLOSING)

		return VE_SRVR_WEBSOCKET_INVALID_STATE_FOR_CLOSE;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(true);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(VWebSocketFrame::OPCODE_CONNECTION_CLOSE);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inDataLength);
	if (inDataLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fBuffer, inDataLength)) == XBOX::VE_OK
	&& (error = fEndPoint->WriteExactly(fBuffer.GetDataPtr(), (uLONG) fBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT)) == XBOX::VE_OK) {

		if (fState == STATE_OPEN)

			fState = STATE_CLOSING;	// Closing handshake initiated.

		else {

			fState = STATE_CLOSED;
			error = ForceClose();	// Close frame of peer has been answered, can drop the TCP socket.

		}

	}

	return error;
}

XBOX::VError VWebSocket::_SendPingOrPong (uBYTE inRSVFlags, const uBYTE *inData, VSize inDataLength, bool inIsPing)
{
	xbox_assert(!(inData == NULL && inDataLength) && !(inData != NULL && !inDataLength));

	XBOX::StLocker<XBOX::VCriticalSection>	lock(&fMutex);

	if (fState != STATE_OPEN)

		return VE_SRVR_WEBSOCKET_NOT_OPEN_STATE;

	XBOX::VError	error;

	fEncodingWebSocketFrame.SetFIN(true);
	fEncodingWebSocketFrame.SetRSVFlags(inRSVFlags);
	fEncodingWebSocketFrame.SetOpcode(inIsPing ? VWebSocketFrame::OPCODE_PING : VWebSocketFrame::OPCODE_PONG);
	if (fUseMaskingKey)

		fEncodingWebSocketFrame.GenerateMaskingKey();

	else

		fEncodingWebSocketFrame.ClearMaskingKey();

	fEncodingWebSocketFrame.SetPayloadLength(inDataLength);
	if (inDataLength)
	
		fEncodingWebSocketFrame.SetPayloadData(inData);

	if ((error = fEncodingWebSocketFrame.Encode(&fBuffer, inDataLength)) == XBOX::VE_OK) 

		error = fEndPoint->WriteExactly(fBuffer.GetDataPtr(), (uLONG) fBuffer.GetDataSize(), DEFAULT_WRITE_TIMEOUT);

	return error;
}

XBOX::VError VWebSocket::_UnmaskPayloadData (XBOX::VMemoryBuffer<> *outBuffer)
{
	xbox_assert(outBuffer != NULL);
	xbox_assert(fDecodingWebSocketFrame.HasMaskingKey() && fDecodingWebSocketFrame.GetPayloadLength());				
	xbox_assert(fDecodingWebSocketFrame.GetPayloadData() != NULL);
	
	if (!outBuffer->SetSize(fDecodingWebSocketFrame.GetPayloadLength()))

		return XBOX::VE_MEMORY_FULL;

	else {

		VWebSocketFrame::ApplyMaskingKey(
			fDecodingWebSocketFrame.GetPayloadData(), 
			(uBYTE *) outBuffer->GetDataPtr(),
			fDecodingWebSocketFrame.GetMaskingKey(),
			fDecodingWebSocketFrame.GetPayloadLength(),
			0);

		return XBOX::VE_OK;

	}
}

XBOX::VError VWebSocket::GenerateWebSocketKey (XBOX::VString *outWebSocketKey)
{
	xbox_assert(outWebSocketKey != NULL);

	uBYTE					random[16];
	XBOX::VMemoryBuffer<>	buffer;
	
	for (uLONG i = 0; i < 16; i++) 

		random[i] = XBOX::VSystem::Random() & 0xff;

	if (!Base64Coder::Encode(random, 16, buffer))

		return XBOX::VE_UNKNOWN_ERROR;	// Base64 encoder doesn't have good error reporting.

	else {

		outWebSocketKey->FromBlock(buffer.GetDataPtr(), buffer.GetDataSize(), VTC_US_ASCII);
		return XBOX::VE_OK;

	}
}

XBOX::VError VWebSocket::ComputeAcceptanceKey (const XBOX::VString &inWebSocketKey, XBOX::VString *outAcceptanceKey)
{
	xbox_assert(outAcceptanceKey != NULL);

	XBOX::VString			key(inWebSocketKey);
	SHA1					checksum;
	XBOX::VMemoryBuffer<>	buffer;

	key.AppendCString(SERVER_KEY_SUFFIX);
	VChecksumSHA1::GetChecksumFromStringUTF8(key, checksum);	// Key is all ASCII characters actually (base64 + ascii suffix).
	if (!Base64Coder::Encode(&checksum, SHA1_SIZE, buffer))

		return XBOX::VE_UNKNOWN_ERROR;	// Base64 encoder doesn't have good error reporting.

	else {

		outAcceptanceKey->FromBlock(buffer.GetDataPtr(), buffer.GetDataSize(), VTC_US_ASCII);
		return XBOX::VE_OK;

	}

	return XBOX::VE_OK;
}

bool VWebSocketConnector::IsValidURL (const XBOX::VString &inURL)
{
	XBOX::VURL	url(inURL, false);

	return IsValidURL(url);
}

bool VWebSocketConnector::IsValidURL (const XBOX::VURL &inURL)
{
	XBOX::VString	string;

	// Scheme must be "ws" (plain) or "wss" (secure).

	inURL.GetScheme(string);
	if (string.CompareToString("ws") != CR_EQUAL && string.CompareToString("wss") != CR_EQUAL)

		return false;

	// Must be an absolute URL (with a hostname).

	inURL.GetNetworkLocation(string, false);
	if (string.IsEmpty())

		return false;

	// URL must not have a fragment component.

	inURL.GetFragment(string, false);
	if (!string.IsEmpty())

		return false;

	// URL must not have a parameter component.

	inURL.GetParameters(string, false);
	if (!string.IsEmpty())

		return false;

	// Path can be empty and query component is optional.

	return true;
}

VWebSocketConnector::VWebSocketConnector (const XBOX::VString &inURL)
{
	xbox_assert(IsValidURL(inURL));

	XBOX::VURL	url(inURL, false);

	fHTTPClient.Init(url, "", false, false, true, false);
}

VWebSocketConnector::VWebSocketConnector (const XBOX::VURL &inURL)
{
	xbox_assert(IsValidURL(inURL));

	fHTTPClient.Init(inURL, "", false, false, true, false);
}

VWebSocketConnector::~VWebSocketConnector ()
{
}

XBOX::VError VWebSocketConnector::Connect (const XBOX::VString &inUserAgent)
{
	XBOX::VError	error;
	XBOX::VString	clientKey, expectedAcceptanceKey, actualAcceptanceKey;

	if ((error = VWebSocket::GenerateWebSocketKey(&clientKey)) != XBOX::VE_OK
	|| (error = VWebSocket::ComputeAcceptanceKey(clientKey, &expectedAcceptanceKey)) != XBOX::VE_OK)

		return error;

	//fHTTPClient.SetUseSSL(true);
	fHTTPClient.AddHeader("Upgrade", "websocket");	
	fHTTPClient.AddHeader("Sec-WebSocket-Version", "13");
	fHTTPClient.AddHeader("Sec-WebSocket-Key", clientKey);

	fHTTPClient.SetAsUpgradeRequest();

	if (!inUserAgent.IsEmpty())

		fHTTPClient.SetUserAgent(inUserAgent);
	
	fHTTPClient.SetUseProxy("", 0);
	fHTTPClient.OpenConnection(NULL);
	fHTTPClient.SendRequest(HTTP_GET);
	fHTTPClient.ReadResponseHeader();
/*
	if (fHTTPClient.GetHTTPStatusCode() != 101) {

	}

	if (fHTTPClient.GetResponseHeaderValue("Sec-WebSocket-Accept", actualAcceptanceKey)) {

		if (expectedAcceptanceKey.EqualToString(actualAcceptanceKey)) {

			int	x = 1;

		}


	}
*/
	return XBOX::VE_OK;
}