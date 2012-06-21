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

#include "ServiceDiscovery.h"

#include "Tools.h"
#include "VUDPEndPoint.h"


BEGIN_TOOLBOX_NAMESPACE


using namespace ServerNetTools;


class Bonjour 
{
public:	
	
	// Default mDNS multicast address and port.
	
	static const uLONG	kIPv4MulticastAddress	= 0xe00000fb;	// 224.0.0.251
	static const PortNumber	kPort				= 5353;
	
	// Maximum DNS name length.
	
	static const uLONG	kMaximumNameLength		= 63;

	// Packet sent will never be bigger than kBufferSize (MTU sized). 
	// However, the read buffer is bigger to accomodate with bigger MTU or cut packets.

	static const uLONG	kBufferSize				= 1500;			// Standard MTU for ethernet is 1500 bytes.
	static const uLONG	kReadBufferSize			= 4096;			// Loopback has huge MTU size, also packets can be cut and re-assembled.
																// Too big packets will be rejected by read() function.
	
	// Supported DNS resource record types.
	
	static const uWORD	kTypeA					= 1,	// IPv4 address.
						kTypePTR				= 12,	// "Pointer".
						kTypeTXT				= 16,	// Text (pairs of key = value).
						kTypeAAAA				= 28,	// IPv6 address.
						kTypeSRV				= 33;	// Service.
	
	// Default time to live is 1 minute.
	
	static const uLONG	kDefaultTimeToLive		= 60;
			
	// Encode or parse a DNS name. Encoder doesn't make use of compression.
	
	static VError	EncodeDNSName (uBYTE *outBuffer, uLONG *ioSize, const VString &inString);			
	static VError	ParseDNSName (const uBYTE *inPacket, uLONG *ioPacketSize, uLONG inIndex, VString *outName);
	
	// Encode or parse a DNS resource record. Time to live parameter is in seconds.
	
	static VError	EncodeResourceRecord (uBYTE *outBuffer, uLONG *ioSize, 
											  const VString &inName, uWORD inType, 
											  const uBYTE *inData, uLONG inDataSize, 
											  uLONG inTimeToLive = kDefaultTimeToLive);
	static VError	ParseResourceRecord (const uBYTE *inPacket, uLONG *ioPacketSize, uLONG inIndex, 
											 VString *outName, uWORD *outType, 
											 uLONG *outDataSize, const uBYTE **outData, 
											 uLONG *outTimeToLive = NULL);
	
	// Encode/parse a VValueBag to/from a DNS resource record TXT data field. Encoder use one more byte from ioSize to detect too long string,
	// so maximum encoded TXT data length is inSize - 1.
	
	static VError	ValueBagToTXT (uBYTE *outBuffer, uLONG *ioSize, const VValueBag &inValueBag);
	static VError	TXTToValueBag (const uBYTE *inData, uLONG *ioDataSize, VValueBag *outValueBag);
		
	// Read/write 2 bytes in network order.
	
	static uLONG	Read2Bytes (const uBYTE *inBuffer);
	static void		Write2Bytes (uBYTE *outBuffer, uWORD inValue);
};

VServiceDiscoveryServer	*VServiceDiscoveryServer::sSingletonInstance	= NULL;
VCriticalSection		VServiceDiscoveryServer::sCriticalSection;

VError Bonjour::EncodeDNSName (uBYTE *outBuffer, uLONG *ioSize, const VString &inString)
{
	xbox_assert(outBuffer != NULL && ioSize != NULL);
	
	VError	error;	
	uBYTE			buffer[kMaximumNameLength + 2];
		
	error = VE_OK;	
		
	buffer[kMaximumNameLength + 1] = 0xff;
	inString.ToCString((char * ) buffer, kMaximumNameLength + 2);
	if (buffer[kMaximumNameLength + 1] != 0xff)
		
		return VE_SRVR_BONJOUR_NAME_TOO_LONG;
	
	if (!buffer[0]) {
	
		outBuffer[0] = 0;
		*ioSize = 1;
		return VE_OK;
		
	}
	
	uBYTE	*p, *q, *r;
	uLONG	size;
		
	p = q = buffer;
	r = outBuffer;
	size = 0;
	for ( ; ; ) {
		
		while (*p && *p != '.') 
			
			p++;
		
		size += p - q + 1;
		if (size + 1 > *ioSize) {
			
			error = VE_SRVR_BONJOUR_CANT_FIT_PACKET;
			break;
			
		}
		
		*r++ = p - q;		
		while (*p != *q)
			
			*r++ = *q++;
		
		if (*p) {
			
			p++;
			q++;
			
		} else {
			
			*r++ = 0;
			*ioSize = size + 1;
			error = VE_OK;
			break;
			
		}
		
	}	
	
	return error;
}

VError Bonjour::ParseDNSName (const uBYTE *inPacket, uLONG *ioPacketSize, uLONG inIndex, VString *outName)
{
	xbox_assert(inPacket != NULL && ioPacketSize != NULL && outName != NULL);
	
	const uBYTE		*p;
	uLONG			total, i, n;
	bool			hasJumped;
	VError	error;

	outName->Clear();
	if (inIndex >= *ioPacketSize) 
	
		return VE_SRVR_BONJOUR_MALFORMED_PACKET;
	
	if (!inPacket[inIndex]) {
	
		*ioPacketSize = 1;
		return VE_OK;
		
	}
	
	total = 0;
	hasJumped = false;
	p = &inPacket[inIndex];
	for ( ; ; ) 
		
		if ((*p & 0xc0) == 0xc0) {
			
			i = (*p++ & 0x3f) << 8;
			i |= *p++ & 0xff; 
			if (i > *ioPacketSize) {
			
				error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
				break;
				
			}
				
			p = &inPacket[i];
			
			if (!hasJumped) {
				
				total += 2;
				hasJumped = true;
				
			}
			
		} else {
			
			n = *p++;
			if (p + n >= inPacket + *ioPacketSize) {

				error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
				break;
				
			}
			
			for (i = 0; i < n; i++) 
				
				outName->AppendChar(*p++);
			
			if (*p) {
				
				if (!hasJumped)
					
					total += n + 1;
				
				outName->AppendChar('.');
				
			} else {
				
				if (!hasJumped)
					
					total += n + 2;

				*ioPacketSize = total;
				error = VE_OK;
				break;
				
			}
			
		}
	
	return error;
}

VError Bonjour::EncodeResourceRecord (uBYTE *outBuffer, uLONG *ioSize, 
											const VString &inName, uWORD inType, 
											const uBYTE *inData, uLONG inDataSize, 
											uLONG inTimeToLive)
{
	xbox_assert(outBuffer != NULL && ioSize != NULL);
	
	VError	error;
	uBYTE			*p;
	uLONG			size;
	
	p = outBuffer;
	size = *ioSize;	
	if ((error = EncodeDNSName(p, &size, inName)) != VE_OK)
		
		return error;
	
	if (*ioSize < size + 10 + inDataSize || inDataSize > 0xffff)
	
		return VE_SRVR_BONJOUR_CANT_FIT_PACKET;
	
	p += size;
	
	Write2Bytes(p, inType);
	p += 2;	
	
	Write2Bytes(p, 1);
	p += 2;
	
	*p++ = inTimeToLive >> 24;
	*p++ = inTimeToLive >> 16;
	*p++ = inTimeToLive >> 8;
	*p++ = inTimeToLive;
		
	Write2Bytes(p, inDataSize);
	p += 2;
	
	::memcpy(p, inData, inDataSize);
	
	*ioSize = size + 10 + inDataSize;	
	return VE_OK;
}

VError Bonjour::ParseResourceRecord (const uBYTE *inPacket, uLONG *ioPacketSize, uLONG inIndex, 
										   VString *outName, uWORD *outType, 
										   uLONG *outDataSize, const uBYTE **outData, 
										   uLONG *outTimeToLive)
{
	xbox_assert(inPacket != NULL && ioPacketSize != NULL && outName != NULL && outType != NULL && outDataSize != NULL && outData != NULL);
	
	VError	error;		
	const uBYTE		*p;
	uLONG			size;
			
	p = &inPacket[inIndex];
	size = *ioPacketSize;
	if ((error = ParseDNSName(inPacket, &size, inIndex, outName)) != VE_OK)
		
		return error;
	
	p += size;
	if ((inIndex += size + 10) > *ioPacketSize) 
		
		return  VE_SRVR_BONJOUR_MALFORMED_PACKET;
	
	*outType = Read2Bytes(p);
	p += 2;
	
	if (Read2Bytes(p) != 1)

		return VE_SRVR_BONJOUR_MALFORMED_PACKET;
	
	p += 2;
	
	if (outTimeToLive != NULL) {
	
		*outTimeToLive = *p++ << 24;
		*outTimeToLive |= *p++ << 16;	
		*outTimeToLive |= *p++ << 8;
		*outTimeToLive |= *p++;	
		
	} else 
		
		p += 4;
	
	*outDataSize = Read2Bytes(p);
	p += 2;
	
	if (inIndex + *outDataSize > *ioPacketSize) 
		
		return VE_SRVR_BONJOUR_MALFORMED_PACKET;
	
	else {
		
		*outData = p;
		*ioPacketSize = size + 10 + *outDataSize;
		return VE_OK;
		
	}
}

VError Bonjour::ValueBagToTXT (uBYTE *outBuffer, uLONG *ioSize, const VValueBag &inValueBag)
{
	xbox_assert(outBuffer != NULL && ioSize != NULL);
	
	uLONG			numberAttributes, bytesLeft, size;
	uBYTE			*p;
	VError	error;

	if (!(numberAttributes = inValueBag.GetAttributesCount())) {
			
		if (!*ioSize)
			
			return VE_SRVR_BONJOUR_CANT_FIT_PACKET;
			
		else {

			*ioSize = 1;
			return VE_OK;
			
		}
		
	}
	
	bytesLeft = *ioSize;
	size = 0;
	p = outBuffer;
	error = VE_OK;
	for (uLONG i = 0; i < numberAttributes; i++) {
		
		const VValueSingle	*pVValueSingle;
		VString				key, value;
		
		pVValueSingle = inValueBag.GetNthAttribute(1 + i, &key);
		if (pVValueSingle->GetValueInfo()->GetKind() != VK_STRING) {
			
			error = VE_SRVR_BONJOUR_INVALID_ATTRIBUTE;
			break;
			
		}

		value.FromValue(*pVValueSingle);			
		
		if ((size = key.GetLength() + 1 + value.GetLength()) > 255 || bytesLeft < size + 1) {
			
			error = VE_SRVR_BONJOUR_CANT_FIT_PACKET;
			break;
			
		}
		
		// TXT strings are supposed to be ASCII (test is not perfect).
		
		*p++ = size;
		if ((size = (uLONG) key.ToBlock(p, key.GetLength() + 1, VTC_StdLib_char, false, false)) == key.GetLength() + 1) {
			
			error = VE_SRVR_BONJOUR_TXT_NOT_ASCII;
			break;
			
		}
			
		bytesLeft -= 1 + size;
		p += size;
		
		*p++ = '=';
		if ((size = (uLONG) value.ToBlock(p, value.GetLength() + 1, VTC_StdLib_char, false, false)) == value.GetLength() + 1) {
			
			error = VE_SRVR_BONJOUR_TXT_NOT_ASCII;
			break;
			
		}

		bytesLeft -= 1 + size;
		p += size;		

	}
	
	if (error == VE_OK) 
	
		*ioSize -= bytesLeft;	

	return error;
}

VError Bonjour::TXTToValueBag (const uBYTE *inData, uLONG *inDataSize, VValueBag *outValueBag)
{
	xbox_assert(inData != NULL && inDataSize != NULL && outValueBag != NULL);
		
	outValueBag->Destroy();
	if (!inDataSize) 
		
		return VE_SRVR_BONJOUR_MALFORMED_PACKET;	// Zero sized is forbidden.
	
	if (*inDataSize == 1) {
		
		if (inData[0])
			
			return VE_SRVR_BONJOUR_MALFORMED_PACKET;

		else {
		
			*inDataSize = 1;
			return VE_OK;
			
		}
		
	}

	const uBYTE		*p, *q;
	VError			error;	
	
	p = inData;
	q = inData + *inDataSize;
	error = VE_OK;
	do {
		
		uLONG	size;
		uBYTE	c;
		VString	key, value;
		
		if (!(size = *p++))
			
			continue;					// Empty string, ignore. 
				
		if (p + size > q) {

			error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
			break;
			
		}

		for ( ; ; ) {
			
			size--;			
			if ((c = *p++) == '=')
				
				break;

			else {

				key.AppendChar(c);
				if (!size)
					
					break;

			}
			
		}
				
		// If no key or (key, value) pair is already defined, ignore.
		
		if (!key.GetLength() || outValueBag->AttributeExists(key)) {		
			
			p += size;
			continue;
			
		}
				
		while (size--)
	
			value.AppendChar(*p++);
		
		// Note that a missing value (this is possible, see dns-sd specification section 6) results
		// in an empty string.
		
		outValueBag->SetString(key, value);
		
	} while (p < q);
	
	if (error == VE_OK)
		
		*inDataSize = p - inData;
	
	return error;
}

uLONG Bonjour::Read2Bytes (const uBYTE *inBuffer)
{
	xbox_assert(inBuffer != NULL);
	
	return (inBuffer[0] << 8) | inBuffer[1];
}

void Bonjour::Write2Bytes (uBYTE *outBuffer, uWORD inValue)
{
	xbox_assert(outBuffer != NULL);
	
	outBuffer[0] = inValue >> 8;
	outBuffer[1] = (uBYTE) inValue;
}

void VServiceRecord::SetHostName ()
{
	VSystem::GetHostName(fHostName);

	fHostName.ToLowerCase();			// This will change 'ç' to 'c', 'é' to 'e', etc.
	fHostName.ExchangeAll(' ', '-');	
	if (fHostName.GetLength() && !fHostName.EndsWith(".local", true))
		
		fHostName.AppendString(".local");
}

VServiceDiscoveryServer	*VServiceDiscoveryServer::GetInstance ()
{	
	StLocker<VCriticalSection>	lock(&sCriticalSection);
	
	if (sSingletonInstance == NULL) 
		
		sSingletonInstance = new VServiceDiscoveryServer();

	else 
		
		sSingletonInstance->Retain();
		
	return sSingletonInstance;
}

VError VServiceDiscoveryServer::AddServiceRecord (const VServiceRecord &inRecord)
{	
	StLocker<VCriticalSection>					lock(&sCriticalSection);
	std::list<VServiceRecord>::const_iterator	i;
	
	for (i = fServiceRecords.begin(); i != fServiceRecords.end(); i++) 
		
		if (i->fServiceName.EqualToString(inRecord.fServiceName, true)
		&& i->fProviderName.EqualToString(inRecord.fProviderName, true)) 
			
			return VE_SRVR_BONJOUR_RECORD_ALREADY_EXISTS;
			
	fServiceRecords.push_front(inRecord);
	
	return VE_OK;
}

VError VServiceDiscoveryServer::RemoveServiceRecord (const VString &inServiceName, const VString &inProviderName)
{
	StLocker<VCriticalSection>			lock(&sCriticalSection);	
	std::list<VServiceRecord>::iterator	i;
	
	for (i = fServiceRecords.begin(); i != fServiceRecords.end(); i++) 
		
		if (i->fServiceName.EqualToString(inServiceName, true)
		&& i->fProviderName.EqualToString(inProviderName, true)) {
			
			fServiceRecords.erase(i);	
			return VE_OK;
			
		}
	
	return VE_SRVR_BONJOUR_RECORD_NOT_FOUND;	
}

VError VServiceDiscoveryServer::Start ()
{
	if ((fUDPEndPoint = VUDPEndPointFactory::CreateMulticastEndPoint(Bonjour::kIPv4MulticastAddress, Bonjour::kPort)) == NULL) 
		return VE_SRVR_BONJOUR_NETWORK_FAILURE;
	else
	{	
		Run();

		return VE_OK;
	}
}

VError VServiceDiscoveryServer::PublishServiceRecord (const VString &inServiceName, const VString &inProviderName)
{
	
	StLocker<VCriticalSection>				lock(&sCriticalSection);		
	std::list<VServiceRecord>::iterator		i;
	uBYTE									buffer[Bonjour::kBufferSize];	
	
	if (GetState() != TS_RUNNING || fUDPEndPoint == NULL) 

		return VE_SRVR_BONJOUR_SERVER_NOT_RUNNING;
		
	for (i = fServiceRecords.begin(); i != fServiceRecords.end(); i++) 
		
		if (i->fServiceName.EqualToString(inServiceName, true)
		&& i->fProviderName.EqualToString(inProviderName, true))
			
			break;		
		
	if (i == fServiceRecords.end())
			
		return VE_SRVR_BONJOUR_RECORD_NOT_FOUND;

	memset(buffer, 0, 12);

	return _SendPTRAnswer(buffer, Bonjour::kBufferSize, 12, *i);
}

void VServiceDiscoveryServer::KillAndWaitTermination ()
{
	Kill();
	
	for (uLONG w = 0; GetState() != TS_DEAD && w < kMaximumWait; w += kTimeOut + 1)
	
		VTask::Sleep(kTimeOut + 1);
}

VServiceDiscoveryServer::VServiceDiscoveryServer ()
: VTask(NULL, 0, eTaskStylePreemptive, NULL)
{
	fUDPEndPoint = NULL;
}

VServiceDiscoveryServer::~VServiceDiscoveryServer ()
{
}

void VServiceDiscoveryServer::DoOnRefCountZero ()
{
	StLocker<VCriticalSection>	lock(&sCriticalSection);
	
	sSingletonInstance = NULL;
	
	delete this;
}

Boolean VServiceDiscoveryServer::DoRun ()
{
	while (GetState() == TS_RUNNING) {
		
		StDropErrorContext errCtx;
			
		uBYTE					readBuffer[Bonjour::kReadBufferSize];
		uLONG					size;
		VError					result;
		std::vector<VString>	queriedServices;
		
		size = Bonjour::kReadBufferSize;
		
		const XNetAddr bonjourInfo(Bonjour::kIPv4MulticastAddress, Bonjour::kPort);
				
		XNetAddr senderInfo;

		result = fUDPEndPoint->Read(readBuffer, &size, &senderInfo);

		// fUDPEndPoint->Read() should only return XBOX::VE_SRVR_READ_FAILED error.

		if (result != XBOX::VE_OK) {

			// UDP is connectionless, error may be transient or permanent, 
			// no way to find out. Just wait and retry.

			XBOX::VTask::Sleep(100);
			continue;

		} else {
						
			// If packet is received from mDNS multicast port, send to multicast address, 
			// otherwise use unicast to target actual sender.
			
			sCriticalSection.Lock();

			if (senderInfo.GetPort() != Bonjour::kPort) 

				fUDPEndPoint->SetDestination(senderInfo);

			else
				
				fUDPEndPoint->SetDestination(bonjourInfo);

			// Currently drop packets bigger than Bonjour::kBufferSize (MTU).
			// If answering those packets, the resulting packet would be bigger.
			
			if (size < Bonjour::kBufferSize)

				_HandlePacket(readBuffer, size, Bonjour::kBufferSize);

			sCriticalSection.Unlock();	

		}
		
	}
	
	sCriticalSection.Lock();
	
	if (fUDPEndPoint != NULL) {
		
		delete fUDPEndPoint;
		fUDPEndPoint = NULL;
		
	}
	
	sCriticalSection.Unlock();	
	
	return false;
}

VError VServiceDiscoveryServer::_HandlePacket (uBYTE *inPacket, uLONG inPacketSize, uLONG inBufferSize)
{
	xbox_assert(inPacket != NULL);
		
	uLONG									numberQuestions, size, i, offset;
	VError									error;
	const uBYTE								*p;
	VString									vString;
	std::list<VServiceRecord>::iterator		j;
	std::map<VString, bool>					queriedServices, queriedAddresses;
	std::map<VString, bool>::iterator		k;		
		
	if (inPacketSize < 12) 
		
		return VE_SRVR_BONJOUR_MALFORMED_PACKET;
	
	if ((inPacket[2] & 0x80)
	|| (inPacket[3] & 0x0f)
	|| !(numberQuestions = Bonjour::Read2Bytes(&inPacket[4])))
		
		return VE_OK;
	
	p = &inPacket[12];
	for (i = 0; i < numberQuestions; i++) {
		
		uWORD	questionType, questionClass;
		
		size = inPacketSize;
		if ((error = Bonjour::ParseDNSName(inPacket, &size, p - inPacket, &vString)) != VE_OK) 

			return error;

		p += size;	
		questionType = Bonjour::Read2Bytes(p);
		questionClass = Bonjour::Read2Bytes(p + 2);		
		p += 4;
		
		if (questionClass != 1)
			
			continue;
		
		if (questionType == Bonjour::kTypePTR) {

			if (vString.EndsWith(".local", true)) {
			
				vString.Truncate(vString.GetLength() - 6);
				for (j = fServiceRecords.begin(); j != fServiceRecords.end(); j++)

					if (j->fServiceName.EqualToString(vString, true)) {

						queriedServices[vString] = true;
						break;

					} else if (vString.EqualToString("_services._dns-sd._udp", true)) {
						
						queriedServices[vString] = true;
						break;
				
					}
				
			}
			
		} else if (questionType == Bonjour::kTypeA) 
			
			queriedAddresses[vString] = true;

		// Ignore all other types of queries.

	}	
	
	error = VE_OK;
	offset = p - inPacket;
	for (k = queriedServices.begin(); k != queriedServices.end(); k++) 

		if (k->first.EqualToString("_services._dns-sd._udp", true))

			error = _SendServiceList(inPacket, inBufferSize, offset);

		else
		
			error = _AnswerQuery(inPacket, inBufferSize, offset, k->first);
	
	for (k = queriedAddresses.begin(); k != queriedAddresses.end(); k++) 
		
		for (j = fServiceRecords.begin(); j != fServiceRecords.end(); j++)
		
			if (j->fHostName.EqualToString(k->first, false)) {
				
				error = _SendAAnswer(inPacket, inBufferSize, offset, k->first, j->fAddress);
				break;
				
			}

	return error;
}

VError VServiceDiscoveryServer::_AnswerQuery (uBYTE *ioPacket, uLONG inBufferSize, uLONG inOffset, const VString &inQueriedService)
{
	xbox_assert(ioPacket != NULL);
	
	VError						error;
	std::list<VServiceRecord>::iterator	i;

	error = VE_OK;
	for (i = fServiceRecords.begin(); i != fServiceRecords.end(); i++)

		if (i->fServiceName.EqualToString(inQueriedService, true))
		
			error = _SendPTRAnswer(ioPacket, inBufferSize, inOffset, *i);
	
	return error;
}

VError VServiceDiscoveryServer::_SendServiceList (uBYTE *ioPacket, uLONG inBufferSize, uLONG inOffset)
{
	xbox_assert(ioPacket != NULL);
	
	VError									error;
	std::list<VServiceRecord>::iterator		i;
	std::map<VString, bool>					allServices;
	std::map<VString, bool>::iterator		j;
	
	error = VE_OK;

	for (i = fServiceRecords.begin(); i != fServiceRecords.end(); i++)

		allServices[i->fServiceName] = true;
	
	for (j = allServices.begin(); j != allServices.end(); ) {
		
		uBYTE	*p, *q, buffer[Bonjour::kBufferSize];
		uLONG	size, dataSize, numberPTRs;
		VString	serviceName;

		p = &ioPacket[inOffset];
		q = ioPacket + inBufferSize;

		// Try to pack as many PTR resource records as possible in each packet.

		numberPTRs = 0;
		for ( ; j != allServices.end(); j++) {
			
			serviceName = j->first;
			serviceName.AppendString(".");
			
			dataSize = Bonjour::kMaximumNameLength + 1;
			size = q - p;	
			if ((error = Bonjour::EncodeDNSName(buffer, &dataSize, serviceName)) != VE_OK	
			|| (error = Bonjour::EncodeResourceRecord(p, &size, 
													  "_services._dns-sd._udp.local", Bonjour::kTypePTR, 
													  buffer, dataSize, 
													  Bonjour::kDefaultTimeToLive)) != VE_OK) {

				if (error == VE_SRVR_BONJOUR_CANT_FIT_PACKET) 

					break;		// Packet is full, send it.

				else 
										
					continue;	// Silently ignore any other form of error.

			} else {

				p += size;
				numberPTRs++;

			}

		}

		if (numberPTRs) {

			ioPacket[2] |= 0x80;
			Bonjour::Write2Bytes(&ioPacket[6], numberPTRs);
			Bonjour::Write2Bytes(&ioPacket[8], 0);
			Bonjour::Write2Bytes(&ioPacket[10], 0);

			xbox_assert(p - ioPacket <= inBufferSize);
			fUDPEndPoint->WriteExactly(ioPacket, p - ioPacket);

		}
	}
	
	return error;
}

VError VServiceDiscoveryServer::_SendPTRAnswer (uBYTE *ioPacket, uLONG inBufferSize, uLONG inOffset, const VServiceRecord &inServiceRecord)
{
	xbox_assert(ioPacket != NULL);

	VError	error;
	uBYTE			*p, *q, buffer[Bonjour::kBufferSize];
	uLONG			size, dataSize;
	VString	serviceName, providerName, hostName;

	ioPacket[2] |= 0x84;
	Bonjour::Write2Bytes(&ioPacket[6], 1);
	Bonjour::Write2Bytes(&ioPacket[8], 0);
	Bonjour::Write2Bytes(&ioPacket[10], 3);
			
	serviceName = inServiceRecord.fServiceName;
	serviceName.AppendString(".local");

	providerName = inServiceRecord.fProviderName;
	providerName.AppendString(".");
	providerName.AppendString(serviceName);
		
	hostName = inServiceRecord.fHostName;

	p = &ioPacket[inOffset];
	q = ioPacket + inBufferSize;

	// Encode PTR resource record.
	
	dataSize = Bonjour::kMaximumNameLength + 1;
	if ((error = Bonjour::EncodeDNSName(buffer, &dataSize, providerName)) != VE_OK)  
				
		return error;

	size = q - p;
	if ((error = Bonjour::EncodeResourceRecord(p, &size, 
											   serviceName, Bonjour::kTypePTR, 
											   buffer, dataSize, 
											   Bonjour::kDefaultTimeToLive)) != VE_OK)
				
		return error;
			
	p += size;
			
	// Encode SRV resource record.
			
	Bonjour::Write2Bytes(buffer, 0);
	Bonjour::Write2Bytes(buffer + 2, 0);
	Bonjour::Write2Bytes(buffer + 4, inServiceRecord.fPort);
	dataSize = Bonjour::kMaximumNameLength + 1;
	if ((error = Bonjour::EncodeDNSName(buffer + 6, &dataSize, hostName)) != VE_OK) 
				
		return error;
			
	size = q - p;
	if ((error = Bonjour::EncodeResourceRecord(p, &size, 
											   providerName, Bonjour::kTypeSRV, 
											   buffer, 6 + dataSize, 
											   Bonjour::kDefaultTimeToLive)) != VE_OK)
				
		return error;
		
	p += size;

	// Encode A and TXT resource records. (Should AAAA records for IPv6 support).
		
	buffer[0] = inServiceRecord.fAddress >> 24;
	buffer[1] = inServiceRecord.fAddress >> 16;
	buffer[2] = inServiceRecord.fAddress >> 8;
	buffer[3] = inServiceRecord.fAddress;

	size = q - p;
	if ((error = Bonjour::EncodeResourceRecord(p, &size, 
											   hostName, Bonjour::kTypeA, 
											   buffer, 4, 
											   Bonjour::kDefaultTimeToLive)) != VE_OK)	
		return error;
			
	p += size;
			
	dataSize = Bonjour::kBufferSize;
	if ((error = Bonjour::ValueBagToTXT(buffer, &dataSize, inServiceRecord.fValueBag)) != VE_OK)
				
		return error;

	size = q - p;
	if ((error = Bonjour::EncodeResourceRecord(p, &size, 
											   providerName, Bonjour::kTypeTXT, 
											   buffer, dataSize, 
											   Bonjour::kDefaultTimeToLive)) != VE_OK)
				
		return error; 

	p += size;
	
	xbox_assert(p - ioPacket <= inBufferSize);
	return fUDPEndPoint->WriteExactly(ioPacket, p - ioPacket);
}

VError VServiceDiscoveryServer::_SendAAnswer (uBYTE *ioPacket, uLONG inBufferSize, uLONG inOffset, const VString &inHostName, uLONG inIPv4)
{
	xbox_assert(ioPacket != NULL);
	
	uBYTE			*p, *q, buffer[4];
	uLONG			size;
	VError	error;	

	ioPacket[2] |= 0x84;
	Bonjour::Write2Bytes(&ioPacket[6], 1);
	Bonjour::Write2Bytes(&ioPacket[8], 0);
	Bonjour::Write2Bytes(&ioPacket[10], 0);

	p = &ioPacket[inOffset];
	q = ioPacket + inBufferSize;

	buffer[0] = inIPv4 >> 24;
	buffer[1] = inIPv4 >> 16;
	buffer[2] = inIPv4 >> 8;
	buffer[3] = inIPv4;
	size = q - p;
	if ((error = Bonjour::EncodeResourceRecord(p, &size, 
											   inHostName, Bonjour::kTypeA, 
											   buffer, 4, 
											   Bonjour::kDefaultTimeToLive)) != VE_OK)
				
		return error;

	p[size - 12] |= 0x80;
	p += size;
	
	xbox_assert(p - ioPacket <= inBufferSize);
	return fUDPEndPoint->WriteExactly(ioPacket, p - ioPacket);	
}

VServiceDiscoveryClient::VServiceDiscoveryClient ()
{
	fUDPEndPoint = NULL;
	Reset();
}

VServiceDiscoveryClient::~VServiceDiscoveryClient ()
{
	if (fUDPEndPoint != NULL) {
		
		delete fUDPEndPoint;
		fUDPEndPoint = NULL;
		
	}
}

VError VServiceDiscoveryClient::Reset ()
{
	fUDPEndPoint = VUDPEndPointFactory::CreateMulticastEndPoint(Bonjour::kIPv4MulticastAddress, Bonjour::kPort);

	if ( fUDPEndPoint != NULL)
	{
		fUDPEndPoint->SetIsBlocking(false);
		
		fIdentifier = VSystem::Random(false);
		return VE_SRVR_BONJOUR_NETWORK_FAILURE;
	
	} else 
		
		return VE_OK;
}

VError VServiceDiscoveryClient::SendQuery (const VString &inServiceName)
{
	std::vector<VString>	serviceNames;
	
	serviceNames.push_back(inServiceName);
	
	return SendQuery(serviceNames);
}

VError VServiceDiscoveryClient::SendQuery (const std::vector<VString> &inServiceNames, uLONG *outNumberRequestSent)
{
	if (outNumberRequestSent != NULL)
		
		*outNumberRequestSent = 0;
	
	if (fUDPEndPoint == NULL)

		return VE_SRVR_BONJOUR_NETWORK_FAILURE;
	
	// Services are queried individually.
	
	uBYTE	buffer[Bonjour::kBufferSize];
	uLONG	size;
	VError	error;	
	
	error = VE_OK;
	for (uLONG i = 0; i < inServiceNames.size(); i++) {

		// Will never send a packet bigger than Bonjour::kBufferSize (MTU == 1500 usually).

		size = Bonjour::kBufferSize;
		if ((error = _EncodePTRQuery(buffer, &size, inServiceNames[i], fIdentifier)) != VE_OK) {
		
			error = VE_OK;
			continue;	// Silently ignore encoding error (should warn instead).
			
		} else if ((error = fUDPEndPoint->WriteExactly(buffer, size)) != VE_OK)
			
			break;

		else if (outNumberRequestSent != NULL)
				
			*outNumberRequestSent += 1;
		
	}
					
	return error;
}

VError VServiceDiscoveryClient::ReceiveServiceRecords (std::vector<VServiceRecord> *outServiceRecords, 
															 const std::vector<VString> &inServiceNames, 
															 bool inAcceptBroadcast)
{
	xbox_assert(outServiceRecords != NULL);
	
	sLONG	identifier;
	VError	error;	
		
	if (fUDPEndPoint == NULL)
		
		return VE_SRVR_BONJOUR_NETWORK_FAILURE;
	
	if (!inServiceNames.size())
		
		return VE_OK;

	identifier = inAcceptBroadcast ? -1 : fIdentifier;	
	outServiceRecords->clear();	

	// Catch read errors.
	StSilentErrorContext errCtx;

	error = VE_OK;
	for (uLONG i = 0; i < kMaximumNumberReceived; i++) {

		uBYTE	readBuffer[Bonjour::kReadBufferSize];
		uLONG	size;
		
		size = Bonjour::kReadBufferSize;
		if ((error = fUDPEndPoint->Read(readBuffer, &size, NULL /*senderInfo*/ )) != VE_OK
		|| (error = _ParsePacket(readBuffer, size, outServiceRecords, inServiceNames, identifier)) != VE_OK)
			
			break;

	}

	return error;
}

VError VServiceDiscoveryClient::_EncodePTRQuery (uBYTE *outBuffer, uLONG *ioSize, const VString &inServiceName, uWORD inIdentifier)
{
	xbox_assert(outBuffer != NULL && ioSize != NULL);
	
	uBYTE	*p;
	VString	vString;
	uLONG	size;
	VError	error;
	
	if (*ioSize < 12) 
	
		return VE_SRVR_BONJOUR_CANT_FIT_PACKET;
	
	p = outBuffer;
	
	::memset(p, 0, 12);
	Bonjour::Write2Bytes(p, inIdentifier);
	Bonjour::Write2Bytes(p + 4, 1);
	p += 12;
	
	vString = inServiceName;
	vString.AppendString(".local");
	size = *ioSize - 12;
	if ((error = Bonjour::EncodeDNSName(p, &size, vString)) != VE_OK)
		
		return error;
	
	p += size;
	
	if (*ioSize < 12 + size + 4) 
			
		return VE_SRVR_BONJOUR_CANT_FIT_PACKET;
	
	Bonjour::Write2Bytes(p, Bonjour::kTypePTR);
	Bonjour::Write2Bytes(p + 2, 1);

	xbox_assert(*ioSize >=  12 + size + 4);		// Check encoded packet size.
	*ioSize = 12 + size + 4;	

	return VE_OK;
}

VError VServiceDiscoveryClient::_ParsePacket (const uBYTE *inPacket, uLONG inPacketSize, 
													std::vector<VServiceRecord> *outServiceRecord, 
													const std::vector<VString> &inServiceNames,
													sLONG inIdentifier)
{
	xbox_assert(inPacket != NULL && outServiceRecord != NULL);
	
	VError			error;
	uLONG			numberQuestions, numberResourceRecords, size;
	const uBYTE		*p, *q;
	VString			vString;
	
	if (inPacketSize < 12)
	
		return VE_SRVR_BONJOUR_MALFORMED_PACKET;
			
	if (inPacket[3] & 0x0f)
		
		return VE_OK;		// Ignore.
	
	// If identifier isn't used, allow service detection from any "captured" packets.
		
	if ((inIdentifier >= 0 && (Bonjour::Read2Bytes(inPacket) != inIdentifier)) || !(inPacket[2] & 0x80)) 
		
		return VE_OK;		// Ignore.

	numberQuestions = Bonjour::Read2Bytes(&inPacket[4]);
	numberResourceRecords = Bonjour::Read2Bytes(&inPacket[6]);
	numberResourceRecords += Bonjour::Read2Bytes(&inPacket[8]);
	numberResourceRecords += Bonjour::Read2Bytes(&inPacket[10]); 

	// Skip questions.
	
	p = &inPacket[12];
	q = inPacket + inPacketSize;
	for (uLONG i = 0; i < numberQuestions; i++) {

		size = inPacketSize;
		if ((error = Bonjour::ParseDNSName(inPacket, &size, p - inPacket, &vString)) != VE_OK)
			
			return error;
		
		if ((p += size + 4) > q) 
		
			return VE_SRVR_BONJOUR_MALFORMED_PACKET;
		
	}
		
	std::vector<PendingRecord>	pendingRecords;
	uLONG						i, j;
	
	// Parse all resource records.
	
	error = VE_OK;
	for (i = 0; i < numberResourceRecords && error == VE_OK; i++) {
		
		VString	name;
		uWORD			type;
		uLONG			dataSize, timeToLive;
		const uBYTE		*data;
		
		size = inPacketSize;
		if ((error = Bonjour::ParseResourceRecord(inPacket, &size, p - inPacket, 
												  &name, &type, &dataSize, &data, &timeToLive)) != VE_OK)
			
			break;
		
		p += size;	
		
		switch (type) {

			case Bonjour::kTypeA: {

				uLONG	address;
			
				if (dataSize != 4) {
				
					error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
					break;
					
				}
			
				address = data[0] << 24;
				address |= data[1] << 16;
				address |= data[2] << 8;
				address |= data[3];
			
				for (j = 0; j < pendingRecords.size(); j++)
				
					if (pendingRecords[j].fHasSRV
					&& pendingRecords[j].fTarget.EqualToString(name, true)) {
					
						pendingRecords[j].fServiceRecord.fAddress = address;
						pendingRecords[j].fHasA = true;
						
					}
				
				break;
				
			}
				
			case Bonjour::kTypePTR: {

				VString	notSuffixedName;

				notSuffixedName = name;
				if (notSuffixedName.EndsWith(".local", true))

					notSuffixedName.Truncate(notSuffixedName .GetLength() - 6);

				for (j = 0; j < inServiceNames.size(); j++)
					
					if (inServiceNames[j].EqualToString(notSuffixedName, true)) 
						
						break;
				
				if (j == inServiceNames.size())
					
					break;

				size = inPacketSize;
				if ((error = Bonjour::ParseDNSName(inPacket, &size , data - inPacket, &vString)) != VE_OK)
					
					break;
					
				if (size != dataSize) { 
				
					error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
					break;
					
				}
						
				for (j = 0; j < pendingRecords.size(); j++)
					
					if (pendingRecords[j].fServiceRecord.fServiceName.EqualToString(name, true) 
					&& pendingRecords[j].fServiceRecord.fProviderName.EqualToString(vString, true))
						
						break;
						
				if (j == pendingRecords.size()) {

					PendingRecord	record;
						
					record.fServiceRecord.fServiceName = name;
					record.fServiceRecord.fProviderName = vString;
					record.fServiceRecord.fValueBag.Clear();						
					record.fHasSRV = record.fHasA = false;
					record.fTarget.Clear();
					
					pendingRecords.push_back(record);
						
				}
				
				break;
				
			}
				
			case Bonjour::kTypeTXT: {
			
				for (j = 0; j < pendingRecords.size(); j++)
					
					if (pendingRecords[j].fServiceRecord.fProviderName.EqualToString(name, true)) {
						
						size = dataSize;
						if ((error = Bonjour::TXTToValueBag(data, &size, &pendingRecords[j].fServiceRecord.fValueBag)) != VE_OK)
							
							break;
							
						if (size != dataSize) 
								
							error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
						
						break;
						
					}
				
				break;
				
			}
				
			case Bonjour::kTypeAAAA: {
			
				if (dataSize != 16) 
					
					return VE_SRVR_BONJOUR_MALFORMED_PACKET;
				
				// TODO: Retrieve IPv6 address, ignore for now.
				
				break;	
				
			}				
		
			case Bonjour::kTypeSRV: {
				
				for (j = 0; j < pendingRecords.size(); j++)
					
					if (pendingRecords[j].fServiceRecord.fProviderName.EqualToString(name, true)) {

						size = inPacketSize;
						if ((error = Bonjour::ParseDNSName(inPacket, &size, data - inPacket + 6, &vString)) != VE_OK)
							
							break;
						
						if (size != dataSize - 6) {
							
							error = VE_SRVR_BONJOUR_MALFORMED_PACKET;
							break;
							
						}
						
						pendingRecords[j].fServiceRecord.fPort = Bonjour::Read2Bytes(data + 4);	// Ignore priority and weight.
						pendingRecords[j].fTarget = vString;
						pendingRecords[j].fHasSRV = true;
											
						break;					
						
					}
											
				break;	
		
			}
								
			default:	break;	// Ignore.
				
		}
				
	}	
	
	if (error != VE_OK)
		
		return error;
	
	// Find out detected records.
	
	for (i = 0; i < pendingRecords.size(); i++)
		
		if (pendingRecords[i].fHasSRV && pendingRecords[i].fHasA) {
			
			if (pendingRecords[i].fServiceRecord.fProviderName.EndsWith(pendingRecords[i].fServiceRecord.fServiceName))

				pendingRecords[i].fServiceRecord.fProviderName.Truncate(
					pendingRecords[i].fServiceRecord.fProviderName.GetLength() 
					- pendingRecords[i].fServiceRecord.fServiceName.GetLength() 
					- 1);															// Remove "." dot.

			if (pendingRecords[i].fServiceRecord.fServiceName.EndsWith(".local"))

				pendingRecords[i].fServiceRecord.fServiceName.Truncate(
					pendingRecords[i].fServiceRecord.fServiceName.GetLength() 
					- 6);

			pendingRecords[i].fServiceRecord.fHostName = pendingRecords[i].fTarget;
			outServiceRecord->push_back(pendingRecords[i].fServiceRecord);

		}

	return VE_OK;
}

END_TOOLBOX_NAMESPACE