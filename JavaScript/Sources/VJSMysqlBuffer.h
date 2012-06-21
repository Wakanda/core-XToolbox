
#ifndef __VJSMYSQLBUFFER__
#define __VJSMYSQLBUFFER__

#include "VJSClass.h"
#include "VJSValue.h"

#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

/*
The aim of the VJSMysqlBufferObject class is to provide an efficient way
of reading and retreiving results from a MySQL database.
*/

class VJSMysqlBufferObject : public XBOX::IRefCountable{
public:

	//Default constructor
	VJSMysqlBufferObject();

	//Destructor
	virtual ~VJSMysqlBufferObject();

	/*
	Adds targetBuffer to the buffersObj List, and retains it.
	@param: targetBuffer as a pointer to VJSBufferObject
	@return: Boolean that determines if we there is still buffers to add or not
	*/
	Boolean	AddBuffer(VJSBufferObject* targetBuffer);

	/*

	Adds targetBuffer to the buffersObj List without analysis
	@param: targetBuffer as a pointer to VJSBufferObject
	@return: void
	*/
	void	AddBufferSimple(VJSBufferObject* targetBuffer);


	/*
	Initializes MysqlBuffer element with:
	- Setting offset to 0.
	- Initializing the iterator with the first element of the buffersObj
	list.
	- Setting the iBufferLength to the length of the current iterator
	buffer.
	@return: void

	*/
	void	InitList();

	/*
	prepares the select for fetch
	@return: void
	*/
	void	PrepareSelectFetch();

	/*
	save rows data start and end position
	@return: void
	*/
	void	SaveRowsPosition();

	/*
	TODO(no docs found about this)
	@return: void
	*/
	void	__SaveRows(XBOX::VJSContext &inContext, sLONG fieldCount,  VJSArray titles, VJSArray types);

	/*
	advance in buffer with len places
	@param: len as uLONG
	@return: void
	*/
	void	Advance(uLONG len);

	/*
	Tests if the next bytes are for an EOF Packet (The end of a series of
	Field Packets),if not increments offset 4 times.
	@return: Boolean
	*/
	Boolean	IsRow();

	/*
	moves to the next result if any
	@return: Boolean
	*/
	Boolean	GoNext(uLONG len);

	/*
	tells if there is next result to retreive
	@return: Boolean
	*/
	Boolean	HasNext();

	//a flag if w have reached the end
	Boolean eof;

	/*
	read fToRead bytes and return reading status
	@param: inBuffer as VJSBufferObject pointer
	@return: Boolean
	*/
	Boolean ReadNext(VJSBufferObject* inBuffer);

	/*
	get the select result count
	@return: uLONG
	*/
	uLONG	GetSelectResultCount();

	/*
	get the rows count
	@return: uLONG
	*/
	uLONG	GetRowsCount();

	/*
	getter for offsetRead value
	@return: uLONG
	*/
	uLONG	GetOffsetRead();


	/*
	Gets the number of received buffers ( buffersObj list length).
	@return: uLONG
	*/
	uWORD	GetSize();

	/*
	reads the next hex
	@return: uBYTE
	*/
	uBYTE	ReadNextHex();

	/*
	Reads the next byte and increments offset.
	@return: uBYTE
	*/
	inline uBYTE	ReadUInt8();

	/*
	Reads the next 2 bytes and increments offset.
	@return: uWORD
	*/
	inline uWORD	ReadUInt16LE();

	/*
	Reads the next 3 bytes and increments offset.
	@return: uLONG
	*/
	inline uLONG	ReadUInt24LE();

	/*
	Reads the next 4 bytes and increments offset.
	@return: uLONG
	*/
	inline uLONG	ReadUInt32LE();

	/*
	Reads the next 8 bytes and increments offset.
	@return: uLONG
	*/
	inline uLONG8	ReadUInt64LE();

	/*
	Decodes and returns a string from buffer data encoded with 'UTF-8' encoding,
	and beginning at specified offset and contains bytesLength bytes.
	@param: bytesLength as uLONG
	@return: VString
	*/
	VString	ReadString(uLONG bytesLength);


	/*
	Decodes and returns the Length Coded String from buffer data encoded
	with 'UTF-8', beginning at specified offset.
	@param: lcb as uLONG
	@return: VString
	*/
	VString	ReadLCBString(uLONG lcb);


	/*
	Decodes and returns the Length Coded String from buffer data encoded
	with 'UTF-8', beginning at specified offset.
	@return: uLONG
	*/
	sLONG	ReadLCB();

	/*
	Read Length Coded Binary Number and add it to fOffsetRead ,ReadLCBPlus
	is used only with OK packet to know how many bytes have been read, to
	verify if the message field(optional) exists or not
	@return: uLONG
	*/
	uLONG	ReadLCBPlus();

	/*
	fetch 'count' of rows
	@param: inContext as XBOX::VJSContext reference
	@param: count as sLONG
	@param: inFieldCount as uLONG
	@param: titles as VJSArray
	@param: types as VJSArray
	@return: VJSArray
	*/
	VJSArray Fetch(XBOX::VJSContext &inContext, sLONG count, uLONG inFieldCount, VJSArray titles, VJSArray types);

	/*
	advance fetch with inCount
	@param: inCount as sLONG
	@return: void
	*/
	void	 AdvanceFetch(sLONG inCount);

	/*
	skip rows data from fetch
	@return: void
	*/
	void	 SkipRowsData();

private:

	inline void SetToRead(uLONG inValue){fToRead = inValue;fRead=0;fReaded=0;}

	//The position in current buffer at which to begin copying bytes.
	uLONG fOffset;
	//The byte length of the current (from the buffers std::list) buffer data
	sLONG fBufferLength;

	//Analysis state, used only in the addBuffer function
	sBYTE fState;
	//number of bytes to advance
	uLONG fToAdvance;
	//number of bytes to read
	uLONG fToRead;
	//number of bytes already read
	uLONG fReaded;
	//packet length
	uLONG fPackLength;
	//this variable is used in the OK packet to know current data size
	uLONG fOffsetRead;
	//the readen value
	uLONG fRead;
	//the row's position between rowsData used with fetch to ensure that we are not at the end packet.
	uLONG fRowPosition;
	//total number of rows
	uLONG fRowCount;
	//vector of buffers' pointers
	std::vector<VJSBufferObject*> fBuffer;
	//current buffer position
	uLONG  fpos ;

	std::vector <std::pair<uLONG, uLONG> > fRowsStart,fRowsEnd;
	//vector of the number of rows data of a ResultSet
	std::vector <uLONG> fRowsCount;
	//ResultSet statement position for iterating between multiple ResultSets
	sLONG fCoordIter;
};


class XTOOLBOX_API VJSMysqlBufferClass : public XBOX::VJSClass<VJSMysqlBufferClass, VJSMysqlBufferObject>{
public:

	typedef XBOX::VJSClass<VJSMysqlBufferClass, VJSMysqlBufferObject> inherited;

	static void GetDefinition           (ClassDefinition& outDefinition);
	static void Initialize              (const XBOX::VJSParms_initialize& inParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void Finalize                (const XBOX::VJSParms_finalize& inParms,  VJSMysqlBufferObject *inMysqlBuffer);
	static void Construct               (XBOX::VJSParms_construct& inParms);

private:

	static void		_addBuffer( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_initList(XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_initFetch(XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_isRow(XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_hasNext(XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_prepareSelectFetch( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_getRowsCount( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_getSelectResultCount( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_getSize( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_getOffsetRead( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readNextHex( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);


	static void		_readUInt8 (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readUInt16LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readUInt24LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readUInt32LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readUInt64LE (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);


	static void		_readString (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_saveRowsPosition (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_readLCBString (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readLCBStringPlus (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_readLCB (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_readLCBPlus (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_advance (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);
	static void		_fetch (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_advanceFetch( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);

	static void		_skipRowsData( XBOX::VJSParms_callStaticFunction& ioParms, VJSMysqlBufferObject *inMysqlBuffer);

};
END_TOOLBOX_NAMESPACE
#endif
