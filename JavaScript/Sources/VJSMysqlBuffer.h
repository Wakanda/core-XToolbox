
#ifndef __VJSMYSQLBUFFER__
#define __VJSMYSQLBUFFER__

#include "VJSClass.h"
#include "VJSValue.h"

#include "VJSBuffer.h"

BEGIN_TOOLBOX_NAMESPACE

class VJSMysqlBufferObject : public XBOX::IRefCountable
{
public:

	VJSMysqlBufferObject();
	virtual ~VJSMysqlBufferObject();

	Boolean	AddBuffer(VJSBufferObject* targetBuffer);
	void	AddBufferSimple(VJSBufferObject* targetBuffer);
	void	InitList();

	void	PrepareSelectFetch();

	void	SaveRows();
	void	__SaveRows(XBOX::VJSContext &inContext, sLONG fieldCount,  VJSArray titles, VJSArray types);
	void	Advance(uLONG len);

	Boolean	IsRow();
	Boolean	GoNext(uLONG len);
	Boolean	HasNext();
	Boolean eof;

	Boolean ReadNext(VJSBufferObject* inBuffer);
	uLONG	GetSelectResultCount();
	uLONG	GetRowsCount();
	uLONG	GetOffsetRead();
	uWORD	GetSize();

	uBYTE	ReadNextHex();
	uBYTE	ReadUInt8();
	uWORD	ReadUInt16LE();
	uLONG	ReadUInt24LE();
	uLONG	ReadUInt32LE();
	uLONG8	ReadUInt64LE();

	VString	ReadString(uLONG bytesLength);
	VString	ReadLCBString(uLONG lcb);

	uLONG	ReadLCB();
	uLONG	ReadLCBPlus();

	VJSArray Fetch(XBOX::VJSContext &inContext, sLONG count, uLONG inFieldCount, VJSArray titles, VJSArray types);
	void	 AdvanceFetch(sLONG inCount);

	void	 SkipRowsData();
private:

	uLONG fOffset;
	sLONG fBufferLength;

	sBYTE fState;
	uLONG fToAdvance;
	uLONG fToRead;
	uLONG fPackLength;
	uLONG fOffsetRead;
	uLONG fRead;
	uLONG fRowPosition;
	uLONG fRowCount;

	std::vector<VJSBufferObject*> fBuffer;
	uLONG  fpos ;

	std::vector <std::pair<uLONG, uLONG> > fRowsStart,fRowsEnd;
	std::vector <uLONG> fRowsCount;
	sLONG fCoordIter;
};

class XTOOLBOX_API VJSMysqlBufferClass : public XBOX::VJSClass<VJSMysqlBufferClass, VJSMysqlBufferObject>
{
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
	static void		_saveRows (XBOX::VJSParms_callStaticFunction &ioParms, VJSMysqlBufferObject *inMysqlBuffer);

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
