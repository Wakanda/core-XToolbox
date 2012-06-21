#include "VJavaScriptPrecompiled.h"

#include "VJSMysqlBuffer.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

VJSMysqlBufferObject::VJSMysqlBufferObject ()
{
	fState = -2;
	fToAdvance = 0;
	fRead = 0;
	fOffsetRead = 0;
	eof = false;
	fpos = 0;
}


VJSMysqlBufferObject::~VJSMysqlBufferObject()
{

	for(uLONG i = 0; i < fBuffer.size(); i++)
		fBuffer[i]->Release();

}


void VJSMysqlBufferObject::AddBufferSimple(VJSBufferObject* targetBuffer)
{
	targetBuffer->Retain();
	fBuffer.push_back(targetBuffer);
}


Boolean VJSMysqlBufferObject::GoNext(uLONG len)
{
	//GoNext purpose is to advance to the current position to "len" next bytes
	//if we reach the end of the buffer and it still remain some bytes in this case
	//we set fToAdvance to the number of bytes to advance and we return false 
	if(fOffset <= fBufferLength - len){
		fOffset += len;
		return true;
	}

	fToAdvance = fOffset - fBufferLength + len;
	return false;

}


Boolean VJSMysqlBufferObject::ReadNext(VJSBufferObject* inBuffer)
{
	//fToRead determines the number of bytes that we shall read
	//we read byte per byte and we incremente the values and decremente fToread
	//	- if we success to read all the desired bytes we set fToRead to 0 and return true
	//	- else return false
	while(fToRead >0){

		if(fOffset <= fBufferLength - 1) {
			fRead |= ((uBYTE*)(inBuffer->GetDataPtr ()))[fOffset];
			fOffset++;
			fToRead --;

		} else 

			break;
	}

	if(fToRead == 0){
		return true;
	}
	return false;

}


Boolean VJSMysqlBufferObject::AddBuffer(VJSBufferObject* inBuffer)
{
	inBuffer->Retain();
	fBuffer.push_back(inBuffer);
	Boolean _break = true;
	fOffset = 0;
	fBufferLength = inBuffer->GetDataSize();

	while(_break){
		switch(fState){

			case -2 :

				fToAdvance = 0;
				fToRead = 3;
				fRead = 0;
				if(ReadNext(inBuffer)){

					fState ++;
					fPackLength = fRead;
					fToAdvance = 1;
					fRead = 0;

				}
				else 
					_break = false;

				break;

			case -1 :

				if(GoNext(fToAdvance)){

					fState ++;
					fRead = 0;

				}
				else 
					_break = false;

				break;

			case 0 :

				fToRead = 1;

				if(ReadNext(inBuffer)){

					fState ++;

					if(fRead == 00){

						fState = 10;
						fOffsetRead = 0;
					}
					else if(fRead == 255){

						fState = 20;
						fToAdvance = fPackLength - 2;

					}
					else{
						fState = 31;
					}
				}

				else 
					_break = false;

				break;
			case 12 :
			case 20 :
			case 38 :
			case 41 :
			case 46 :
			case 60 :
			case 15 :
			case 18 :


				if(GoNext(fToAdvance))
					fState ++;

				else 
					_break = false;

				break;

			case 10 :
			case 13 :
			case 47	:

				fToRead = 1;

				if(ReadNext(inBuffer))
					fState ++;

				else 
					_break = false;

				break;


			case 16 :

				fToRead = 2;
				fState ++;
				break;

			case 17 :

				if(ReadNext(inBuffer)){

					fToAdvance = fPackLength - fOffsetRead - 5;
					fState ++;

					if(!eof)
						eof = ((fRead & 8)!= 0);
				}
				else 
					_break = false;

				break;

			case 19 :

				if(eof)
					return false;

				fState = -2;

				break;

			case 21 :
				return false;
				break;
			case 11 :
			case 14 :
			case 31 :

				if(fRead < 251)
					fToAdvance = 0;

				else if(fRead == 251)
					fToAdvance = 0;

				else if(fRead == 252)
					fToAdvance = 2;

				else if(fRead == 253)
					fToAdvance = 3;


				else if(fRead == 254)
					fToAdvance = 8;

				fOffsetRead += fToAdvance;
				fState ++;

				break;

			case 32:

				if(fOffsetRead == 0) {
					fState += 2;
				}
				else{
					fToRead = fToAdvance;
					fState ++;
					fRead = 0;
				}

				break;

			case 33 :
			case 36 :
			case 44 :
			case 62 :


				if(ReadNext(inBuffer))
					fState ++;

				else 
					_break = false;

				break;

			case 34:

				fOffsetRead = fRead;
				fRead = 0; 
				fState ++;

				break;

			case 35:
				//case 42:

				fToRead = 3;
				fRead = 0;
				fState ++;

				break;

			case 37:

				fToAdvance = fRead + 1;
				fState ++;

				break;

			case 39:

				fOffsetRead --;

				if(fOffsetRead == 0)
					fState ++;

				else
					fState -= 4;
				break;

			case 40:

				fToAdvance = 9;
				fState ++;
				fRead = 0;

				break;

			case 42:

				fRowCount = 0;
				fRowsStart.push_back(std::pair<uLONG, uLONG>(fBuffer.size() - 1, fOffset));
				fState ++;

				break;

			case 43:

				fToRead = 3;
				fRead = 0;
				fState ++;

				break;

			case 45:

				if(fRead == 5){
				//if(fRead == 5 && fOffset == fBufferLength - 6){

					fToAdvance = 1;
					fState ++;
					fRead = 0;

				}
				else {

					fToAdvance = fRead + 1;
					fRowCount ++;
					fState  = 50;

				}
				break;

			case 50 :

				if(GoNext(fToAdvance))
					fState  = 43;

				else 
					_break = false;

				break;

			case 48 :

				if(fRead == 254){

					fToAdvance = 2;
					fState = 60;

				}
				else{

					fToAdvance = 4;
					fRowCount ++;
					fState = 50;

				}	

				break;

			case 61 :

				fToRead = 2;
				fRead = 0;
				fState ++;

				break;

			case 63 :

				fRowsCount.push_back(fRowCount);


				if(fOffset > 9)
					fRowsEnd.push_back(std::pair<uLONG, uLONG>(fBuffer.size() - 1, fOffset - 10));

				else
					fRowsEnd.push_back(std::pair<uLONG, uLONG>(fBuffer.size() - 2, fBuffer.size() -9 + fOffset));

				if((fRead & 8) == 0)

					return false;


				else

					fState = -2;

				if(eof)
					return false;
				break;
		}
	}

	return true;
}


void VJSMysqlBufferObject::InitList()
{
	fOffset = 0;
	fpos = 0;
	fBufferLength = fBuffer[fpos]->GetDataSize();
	fOffsetRead = 0;
	fCoordIter = -1;
}


uLONG VJSMysqlBufferObject::GetRowsCount(){
	return fRowsCount[fCoordIter];
}


VJSArray VJSMysqlBufferObject::Fetch(XBOX::VJSContext &inContext, sLONG inCount, uLONG inFieldCount, VJSArray titles, VJSArray types)
{
	//read VJSMysqlBufferObject::AdvanceFetch(sLONG inCount) first
	VJSArray fetchedRows(inContext);
	fetchedRows.Clear();

	VJSObject row(inContext);
	VString title;

	if(inCount == -1)
		inCount = fRowsCount[fCoordIter];

	if(inCount != 0){
		if(inCount <= (fRowsCount[fCoordIter] - fRowPosition)){

			for(uLONG i = 0; i < inCount ; i ++){

				row.MakeEmpty();
				Advance(4);

				for (uLONG t = 0; t < inFieldCount; t ++){

					titles.GetValueAt(t).GetString(title);
					row.SetProperty((VString) title, ReadLCBString( ReadLCB()));

				}

				fetchedRows.PushValue(row);
			}

			fRowPosition += inCount;

		}
		else{

			for(uLONG i = 0; i < fRowsCount[fCoordIter] - fRowPosition ; i ++){

				row.MakeEmpty();
				Advance(4);

				for (uLONG t = 0; t < inFieldCount; t ++){

					titles.GetValueAt(t).GetString(title);
					row.SetProperty((VString) title, ReadLCBString( ReadLCB()));

				}

				fetchedRows.PushValue(row);

			}

			fRowPosition = fRowsCount[fCoordIter];
			
		}
	}

	return fetchedRows;
}


void VJSMysqlBufferObject::AdvanceFetch(sLONG inCount)
{
	// Advance Fetch means, skip the next inCount rows
	// if inCount = -1 we should skip all rows
	if(inCount == -1)
		inCount = fRowsCount[fCoordIter];

	if(inCount != 0){
		if(inCount <= (fRowsCount[fCoordIter] - fRowPosition)){
		// if the number of rows to skip is less than rest of no skiped rows
			for(uLONG i = 0; i < inCount ; i ++)
				Advance(ReadUInt24LE() + 1);//next row

			fRowPosition += inCount;

		}
		else{
			// if the number of rows to skip is more than the rest of non skiped rows 
			//-->		then we skip only the rest 

			for(uLONG i = 0; i < fRowsCount[fCoordIter] - fRowPosition ; i ++)
				Advance(ReadUInt24LE() + 1);

			fRowPosition = fRowsCount[fCoordIter];
			
		}
	}

}


void VJSMysqlBufferObject::PrepareSelectFetch()
{
	if(fCoordIter >= 0){
		//if this is not the first time we must iterate to the next statement
		fCoordIter++;
	}
	else{
		//before fetching results fCoordIter is set to -1
		fCoordIter = 0;
	}
	//the buffer position which contain the start of rows data in buffers list
	fpos = fRowsStart[fCoordIter].first;
	//Byte position start in the selected buffer
	fOffset = fRowsStart[fCoordIter].second;
	//row position is set to 0 because we didn't start the fetch
	fRowPosition = 0;
}


void VJSMysqlBufferObject::SaveRowsPosition(){
	uLONG rowsCount = 0;
	//save the rows data start position
	fRowsStart.push_back(std::pair<uLONG, uLONG>(fpos, fOffset));

	//see IsRow function first
	while (IsRow())//if the next packet is not an EOF
		rowsCount++;
	//if the next packet is an EOF
	//we save the position at the end of RowsData
	fRowsEnd.push_back(std::pair<uLONG, uLONG>(fpos, fOffset));
	//we save the number of rows of the current statement	
	fRowsCount.push_back(rowsCount);
}


void VJSMysqlBufferObject::SkipRowsData (){

	if(fCoordIter >= 0){
		//if this is not the first time we must iterate to the next statement
		fCoordIter++;
	}
	else{
		//before fetching results fCoordIter is set to -1
		fCoordIter = 0;
	}

	//the buffer position which contain the start of rows data in buffers list
	fpos = fRowsEnd[fCoordIter].first;
	
	//Byte position start in the selected buffer
	fOffset = fRowsEnd[fCoordIter].second;


	if(fRowsStart.size()- fCoordIter == 1)
		fCoordIter = -1;
	
	//because SkipRowsData just before the conncatenation which use fCoordIter for the fetch.
	//if we reach the last statement we reset fCoordIter to -1
}


uWORD VJSMysqlBufferObject::GetSize()
{
	return fBuffer.size();

}


uLONG VJSMysqlBufferObject::GetSelectResultCount()
{
	//return fRowsList.size();
	return (uLONG) 0;

}


uLONG VJSMysqlBufferObject::GetOffsetRead()
{
	uLONG offsetRead = fOffsetRead;
	fOffsetRead = 0;
	return offsetRead;

}


uBYTE VJSMysqlBufferObject::ReadNextHex()
{
	if(fOffset <= fBufferLength - 1) {
		return ((uBYTE*)(fBuffer[fpos]->GetDataPtr ()))[fOffset];
	}
	else{
		fpos++;
		fOffset = 0;
		fBufferLength = fBuffer[fpos]->GetDataSize();
		return ((uBYTE*)(fBuffer[fpos]->GetDataPtr()))[fOffset];
	}
}


Boolean VJSMysqlBufferObject::HasNext(){

	return (Boolean)(fRowPosition < fRowsCount[fCoordIter]);
	//(Boolean)!(fpos == fRowsEnd[fCoordIter].first && fOffset >= fRowsEnd[fCoordIter].second);
}


Boolean VJSMysqlBufferObject::IsRow()
{
	if(
	 //if it is the last 9 bytes of a buffer
	fOffset == fBufferLength - 9 &&
	//and if if the 5th field is 254 ie it is EOF
	((uBYTE*)(fBuffer[fpos]->GetDataPtr()))[fBufferLength - 5] == 0xfe
	//and we have exactly 5 bytes (because an EOF contains 5 bytes)	
	&& ((uBYTE*)(fBuffer[fpos]->GetDataPtr()))[fBufferLength - 9] == 5
	){
			return false;
	}

	Advance(ReadUInt24LE() + 1);

	return true;
}



void VJSMysqlBufferObject::Advance(uLONG len)
{
	//to position the fOffset to the next len bytes, 
	//if the the number of bytes aren't enough we go to the next buffer and continue advancing
	if(fOffset <= fBufferLength - len){
		fOffset += len;
		return;
	}

	fpos++;
	fOffset += len - fBufferLength;
	fBufferLength = fBuffer[fpos]->GetDataSize();

}


uBYTE VJSMysqlBufferObject::ReadUInt8()
{
	//read one byte
	//although we are at the buffer 's end we pointe to the next buffer and and at the start of this buffer fOffset = 0
	if(fOffset <= fBufferLength - 1) {
		return ((uBYTE*)(fBuffer[fpos]->GetDataPtr ()))[fOffset++];

	}
	else{
		fpos++;
		fOffset = 0;
		fBufferLength = fBuffer[fpos]->GetDataSize();
		return ((uBYTE*)(fBuffer[fpos]->GetDataPtr()))[fOffset++];
	}

}


uWORD VJSMysqlBufferObject::ReadUInt16LE()
{
	uWORD result = 0;

	if(fOffset <= fBufferLength - 2) {
		uBYTE* buffer = (uBYTE*)(fBuffer[fpos]->GetDataPtr ());
		result = *((uWORD *) &buffer[fOffset]);
		fOffset += 2;
	}
	else if(fOffset == fBufferLength - 1) {
		result = ((uBYTE*)(fBuffer[fpos]->GetDataPtr()))[fOffset];
		fpos++;
		fOffset = 0;
		result |= ((uBYTE*)(fBuffer[fpos]->GetDataPtr()))[fOffset] << 8;
		fOffset += 1;
		fBufferLength = fBuffer[fpos]->GetDataSize();
	}
	else if(fOffset == fBufferLength){
		uBYTE* buffer = (uBYTE*)(fBuffer[fpos]->GetDataPtr ());
		fpos++;
		fOffset = 0;
		result = (*((uWORD *) &buffer[fOffset]));
		result += 2;
		fBufferLength = fBuffer[fpos]->GetDataSize();
	}

	return result;
}


uLONG VJSMysqlBufferObject::ReadUInt24LE()
{
	uLONG result;
	if(fOffset <= fBufferLength - 3) {
		uBYTE* buffer = (uBYTE*)(fBuffer[fpos]->GetDataPtr ());
		result = *((uWORD *) &buffer[fOffset]);
		result |= buffer[fOffset + 2] << 16;
		fOffset += 3;
	}
	else{
		result = ReadUInt8() | (ReadUInt16LE())<< 8;
		fBufferLength = fBuffer[fpos]->GetDataSize();
	}
	return result;
}


uLONG VJSMysqlBufferObject::ReadUInt32LE()
{
	uLONG result;

	if(fOffset <= fBufferLength - 4) {
		uBYTE* buffer = (uBYTE*)(fBuffer[fpos]->GetDataPtr ());
		result = *((uLONG *) &buffer[fOffset]);
		fOffset += 4;
		return result;
	}
	else{
		result = ReadUInt16LE() | (ReadUInt16LE())<< 16;
		fBufferLength = fBuffer[fpos]->GetDataSize();
		return result;
	}
}


uLONG8 VJSMysqlBufferObject::ReadUInt64LE()
{
	uLONG8 result;

	if(fOffset <= fBufferLength - 4) {
		uBYTE* buffer = (uBYTE*)(fBuffer[fpos]->GetDataPtr ());
		result = *((uLONG8 *) &buffer[fOffset]);
		fOffset += 4;
	}
	else{
		uLONG8 left = ReadUInt32LE();
		uLONG8 right = ReadUInt32LE();
		result = left | (right << 32);
		fBufferLength = fBuffer[fpos]->GetDataSize();
	}
 
	return result;
}


uLONG VJSMysqlBufferObject::ReadLCB ()
{
	//Read Length Coded Binary Number
	uBYTE firstCoded = ReadUInt8();

	if(firstCoded < 251){
		return (uLONG)firstCoded;
	}
	else if(firstCoded == 251){
		return 0;
	}
	else if(firstCoded == 252){
		return (uLONG)ReadUInt16LE();
	}
	else if(firstCoded == 253){
		return ReadUInt24LE();
	}
	else if(firstCoded == 254){
		return ReadUInt64LE();
	}
	else{
		return 0;
	}
}


uLONG VJSMysqlBufferObject::ReadLCBPlus()
{
	//Read Length Coded Binary Number and add it to fOffsetRead ,ReadLCBPlus is used only with OK 
	//packet to know how many bytes have been read, to verify if the message field(optional) exists or not
	uLONG lcb;
	uBYTE firstCoded = ReadUInt8();

	if(firstCoded < 251){
		lcb = (uLONG)firstCoded;
		fOffsetRead += lcb + 1;
	}
	else if(firstCoded == 251){
		lcb = 0;
		fOffsetRead += 1;
	}
	else if(firstCoded == 252){
		lcb = (uLONG)ReadUInt16LE();
		fOffsetRead += lcb + 2;
	}
	else if(firstCoded == 253){
		lcb = ReadUInt24LE();
		fOffsetRead += lcb + 3;
	}
	else if(firstCoded == 254){
		lcb = ReadUInt64LE();
		fOffsetRead += lcb + 8;
	}

	return lcb;
}


VString VJSMysqlBufferObject::ReadString (uLONG bytesLength)
{
	//read String of bytesLength bytes (the argument)
	CharSet	encoding = VTC_UTF_8;
	XBOX::VString	decodedString;

	if(fOffset + bytesLength <= fBufferLength){
		//if the string bytes are in one buffer
		fBuffer[fpos]->ToString(encoding, fOffset, fOffset + bytesLength , &decodedString);
	}
	else{
		//if the string bytes are shared between many buffers
		XBOX::VString	subDecodedString;
		uLONG stillToRead;
		//how much bytes we still need to complete reading all string bytes
		stillToRead= bytesLength + fOffset - fBufferLength;
		decodedString = "";

		fBuffer[fpos]->ToString(encoding, fOffset, fBufferLength, &subDecodedString);
		decodedString.AppendString(subDecodedString);

		while(stillToRead != 0){
			fpos++;
			fOffset = 0;
			fBufferLength = fBuffer[fpos]->GetDataSize();

			if(stillToRead <= fBufferLength) {
				//here, because one "string bytes" may will be shared in not only 2 buffers but even more
				fBuffer[fpos]->ToString(encoding, fOffset, stillToRead, &subDecodedString);
				fOffset += stillToRead;
				decodedString.AppendString(subDecodedString);
				break;

			}
			else {
				//if we read the full buffer and we haven't achieved yet reading the whole string
				fBuffer[fpos]->ToString(encoding, fOffset, fBufferLength, &subDecodedString);
				stillToRead -= fBufferLength;
				decodedString.AppendString(subDecodedString);
			}
		}
	}
	
	return decodedString;
}


VString VJSMysqlBufferObject::ReadLCBString (uLONG lcb)
{
	//the same analyse as the ReadLCBString above, expect that the string that
	//we gonna read is written in the value of the Length Coded Binary bytes
	//read String writen in "Length Coded Binary" bytes
	XBOX::VString	decodedString;
	CharSet	encoding = VTC_UTF_8;

	if(fOffset + lcb <= fBufferLength){
		fBuffer[fpos]->ToString(encoding, fOffset, fOffset + lcb , &decodedString);
		fOffset += lcb;
	}
	else{
		XBOX::VString	subDecodedString;
		uLONG stillToRead;
		stillToRead= lcb + fOffset - fBufferLength;
		//decodedString = "";
		fBuffer[fpos]->ToString(encoding, fOffset, fBufferLength, &subDecodedString);
		decodedString.AppendString(subDecodedString);
		while(stillToRead != 0){
			fpos++;
			fOffset = 0;
			fBufferLength = fBuffer[fpos]->GetDataSize();
			if(stillToRead <= fBufferLength) {
				fBuffer[fpos]->ToString(encoding, fOffset, stillToRead, &subDecodedString);
				fOffset += stillToRead;
				decodedString.AppendString(subDecodedString);
				break;
			}
			else {
				fBuffer[fpos]->ToString(encoding, fOffset, fBufferLength, &subDecodedString);
				stillToRead -= fBufferLength;
				decodedString.AppendString(subDecodedString);

			}
		}
	}
	return decodedString;	
}


void VJSMysqlBufferClass::Finalize(const XBOX::VJSParms_finalize& inParms, VJSMysqlBufferObject* inMysqlBuffer)
{
	if(inMysqlBuffer)
		delete inMysqlBuffer;
}


void VJSMysqlBufferClass::Initialize(const XBOX::VJSParms_initialize& inParms, VJSMysqlBufferObject* inMysqlBuffer){
}


void VJSMysqlBufferClass::Construct(XBOX::VJSParms_construct& ioParms)
{
	VJSMysqlBufferObject* inMysqlBuffer=new VJSMysqlBufferObject();

	if(inMysqlBuffer==NULL)
	{
		XBOX::vThrowError(VE_UNKNOWN_ERROR);
		return;
	}

	ioParms.ReturnConstructedObject(VJSMysqlBufferClass::CreateInstance(ioParms.GetContextRef(), inMysqlBuffer));
}


void VJSMysqlBufferClass:: _addBuffer( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	xbox_assert(inMysqlBuffer != NULL);

	XBOX::VJSObject	targetObject(ioParms.GetContext());
	if (!ioParms.CountParams() || !ioParms.GetParamObject(1, targetObject) || !targetObject.IsInstanceOf("Buffer")) {
		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;
	}

	VJSBufferObject	*targetBuffer;
	targetBuffer = targetObject.GetPrivateData<VJSBufferClass>();
	xbox_assert(targetBuffer != NULL);

	//if the second argument doesn't exist or it is equal to 0 we shall analyze
	//the buffer as a general case
	
	//queryT = 2nd argument
	//		queryT = 0 -> we shall analyze all buffers bytes by byte because end is not known
	//		queryT = 1 -> we shall stop buffers concatenation after the first statement analysis
	//		queryT = 2 -> we shall just analyze the 9 bytes for every buffer received until EOF packet
	//		queryT = 3 -> we shall concatenate just the first buffer

	if(ioParms.CountParams() == 2){
		sLONG queryType;
		if (!ioParms.GetLongParam(2, &queryType)) {
			XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
			return;
		}
		if(queryType == 1)
			inMysqlBuffer->eof = true;
		else if( queryType == 2){
			inMysqlBuffer->AddBufferSimple(targetBuffer);
			uBYTE* vBuffer = (uBYTE*)(targetBuffer->GetDataPtr());
			if(
				//this field signals that this packet is EOF packet
				vBuffer[targetBuffer->GetDataSize() - 5] == 0xfe
				//number of usefull data is 5(then an EOF packet)
				&& vBuffer[targetBuffer->GetDataSize() - 9] == 5
				//no more info from the server
				&& (((vBuffer[targetBuffer->GetDataSize() - 2] + (vBuffer[targetBuffer->GetDataSize() - 1] << 8)) & 8) == 0)
				){
					ioParms.ReturnBool(false);
					return;
			}
			ioParms.ReturnBool(true);
			return;
		}
		else if( queryType == 3){
			inMysqlBuffer->AddBufferSimple(targetBuffer);
			ioParms.ReturnBool(false);
			return;
		}
	}
	ioParms.ReturnBool(inMysqlBuffer->AddBuffer(targetBuffer) == TRUE);
}


void VJSMysqlBufferClass:: _readNextHex( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uBYTE>(inMysqlBuffer->ReadNextHex());
}


void VJSMysqlBufferClass:: _isRow( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnBool(inMysqlBuffer->IsRow() == TRUE);
}


void VJSMysqlBufferClass:: _hasNext( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnBool(inMysqlBuffer->HasNext() == TRUE);
}


void VJSMysqlBufferClass:: _getSize( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uWORD>(inMysqlBuffer->GetSize());
}


void VJSMysqlBufferClass:: _fetch( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	sLONG inCount;
	sLONG fieldCount;
	XBOX::VJSContext inContext(ioParms.GetContextRef());
	XBOX::VJSArray titles(inContext);
	XBOX::VJSArray types(inContext);

	if (	ioParms.CountParams() != 4 
		||	!ioParms.IsNumberParam(1) 
		||	!ioParms.IsNumberParam(2) 
		||	!ioParms.GetLongParam(1, &inCount)// the number of fetched rows(gonna to fetch)
		||	!ioParms.GetLongParam(2, &fieldCount) // the number of columns
		||	!ioParms.GetParamArray(3, titles)// the array of columns name
		||	!ioParms.GetParamArray(4, types)) {// array of the  types of each column

			XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
			return;

	}

	ioParms.ReturnValue(inMysqlBuffer->Fetch(inContext, inCount, (uLONG) fieldCount, titles, types));
}


void VJSMysqlBufferClass:: _advanceFetch( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	sLONG inCount;

	if (ioParms.CountParams() != 1 ||	!ioParms.IsNumberParam(1) ||	!ioParms.GetLongParam(1, &inCount) ) 
	{

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}
	inMysqlBuffer->AdvanceFetch(inCount);
}


void VJSMysqlBufferClass:: _getSelectResultCount( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->GetSelectResultCount());
}


void VJSMysqlBufferClass:: _getRowsCount( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->GetRowsCount());
}


void VJSMysqlBufferClass:: _getOffsetRead( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber(inMysqlBuffer->GetOffsetRead());
}


void VJSMysqlBufferClass:: _initList (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	inMysqlBuffer->InitList();
}


void VJSMysqlBufferClass:: _readUInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uBYTE>(inMysqlBuffer->ReadUInt8());
}


void VJSMysqlBufferClass:: _readUInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uWORD>(inMysqlBuffer->ReadUInt16LE());
}


void VJSMysqlBufferClass:: _readUInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->ReadUInt24LE());
}


void VJSMysqlBufferClass:: _readUInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->ReadUInt32LE());
}


void VJSMysqlBufferClass:: _readUInt64LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->ReadUInt64LE());
}


void VJSMysqlBufferClass:: _readLCB (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->ReadLCB());
}


void VJSMysqlBufferClass:: _readLCBPlus (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{
	ioParms.ReturnNumber<uLONG>(inMysqlBuffer->ReadLCBPlus());
}


void VJSMysqlBufferClass:: _readLCBString (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{	
	ioParms.ReturnString(inMysqlBuffer->ReadLCBString(inMysqlBuffer->ReadLCB()));
}


void VJSMysqlBufferClass:: _readLCBStringPlus (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{	
	ioParms.ReturnString(inMysqlBuffer->ReadLCBString(inMysqlBuffer->ReadLCBPlus()));
}


void VJSMysqlBufferClass:: _readString (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{	
	sLONG bytesLength;

	if (ioParms.CountParams() != 1 && (!ioParms.IsNumberParam(1)) || !ioParms.GetLongParam(1, &bytesLength)) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	ioParms.ReturnString(inMysqlBuffer->ReadString(bytesLength));
}


void VJSMysqlBufferClass:: _advance (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer)
{	
	sLONG bytesLength;

	if (ioParms.CountParams() != 1 && (!ioParms.IsNumberParam(1)) || !ioParms.GetLongParam(1, &bytesLength)) {

		XBOX::vThrowError(XBOX::VE_INVALID_PARAMETER);
		return;

	}

	inMysqlBuffer->Advance(bytesLength);
}


void VJSMysqlBufferClass:: _prepareSelectFetch (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer){
	inMysqlBuffer->PrepareSelectFetch();
}


void VJSMysqlBufferClass:: _saveRowsPosition (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer){

	inMysqlBuffer->SaveRowsPosition();
}


void VJSMysqlBufferClass:: _skipRowsData (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer){

	inMysqlBuffer->SkipRowsData ();
}


void VJSMysqlBufferClass::GetDefinition ( ClassDefinition& outDefinition)
{

	static inherited::StaticFunction functions[] = 
	{
		{ "addBuffer", js_callStaticFunction<_addBuffer>, JS4D::PropertyAttributeDontDelete },

		{ "init", js_callStaticFunction<_initList>, JS4D::PropertyAttributeDontDelete },

		{ "isRow", js_callStaticFunction<_isRow>, JS4D::PropertyAttributeDontDelete },
		{ "hasNext", js_callStaticFunction<_hasNext>, JS4D::PropertyAttributeDontDelete },

		{ "readNextHex", js_callStaticFunction<_readNextHex>, JS4D::PropertyAttributeDontDelete },
		{ "fetch", js_callStaticFunction<_fetch>, JS4D::PropertyAttributeDontDelete },
		{ "getSize", js_callStaticFunction<_getSize>, JS4D::PropertyAttributeDontDelete },
		{ "getSelectResultCount", js_callStaticFunction<_getSelectResultCount>, JS4D::PropertyAttributeDontDelete },
		{ "getRowsCount", js_callStaticFunction<_getRowsCount>, JS4D::PropertyAttributeDontDelete },

		{ "getOffsetRead", js_callStaticFunction<_getOffsetRead>, JS4D::PropertyAttributeDontDelete },
		{ "prepareSelectFetch", js_callStaticFunction<_prepareSelectFetch>, JS4D::PropertyAttributeDontDelete },

		{ "readUInt8", js_callStaticFunction<_readUInt8>, JS4D::PropertyAttributeDontDelete },
		{ "readUInt16LE", js_callStaticFunction<_readUInt16LE>, JS4D::PropertyAttributeDontDelete },
		{ "readUInt24LE", js_callStaticFunction<_readUInt24LE>, JS4D::PropertyAttributeDontDelete },
		{ "readUInt32LE", js_callStaticFunction<_readUInt32LE>, JS4D::PropertyAttributeDontDelete },
		{ "readUInt64LE", js_callStaticFunction<_readUInt64LE>, JS4D::PropertyAttributeDontDelete },

		{ "readString", js_callStaticFunction<_readString>, JS4D::PropertyAttributeDontDelete },
		{ "readLCBString", js_callStaticFunction<_readLCBString>, JS4D::PropertyAttributeDontDelete },
		{ "readLCBStringPlus", js_callStaticFunction<_readLCBStringPlus>, JS4D::PropertyAttributeDontDelete },

		{ "readLCB", js_callStaticFunction<_readLCB>, JS4D::PropertyAttributeDontDelete },
		{ "readLCBPlus", js_callStaticFunction<_readLCBPlus>, JS4D::PropertyAttributeDontDelete },

		{ "saveRowsPosition", js_callStaticFunction<_saveRowsPosition>, JS4D::PropertyAttributeDontDelete },
		{ "advance", js_callStaticFunction<_advance>, JS4D::PropertyAttributeDontDelete },
		{ "advanceFetch", js_callStaticFunction<_advanceFetch>, JS4D::PropertyAttributeDontDelete },

		{ "skipRowsData", js_callStaticFunction<_skipRowsData>, JS4D::PropertyAttributeDontDelete },

		{ 0, 0, 0}
	};

	outDefinition.className = "MysqlBuffer";
	outDefinition.initialize        = js_initialize<Initialize>;
	outDefinition.finalize          = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
}
