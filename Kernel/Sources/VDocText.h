
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
#ifndef __VDocText__
#define __VDocText__

#include "VString.h"
#include "VTime.h"
#include "VJSONValue.h"
#include "VTextStyle.h"

BEGIN_TOOLBOX_NAMESPACE

//CSS unit types (alias of XBOX::CSSProperty::eUnit - make sure it is consistent with XML enum)
//note: kDOC_CSSUNIT_EM, kDOC_CSSUNIT_EX, kDOC_CSSUNIT_FONTSIZE_LARGER & kDOC_CSSUNIT_FONTSIZE_SMALLER are not supported
//		so we assume lengths & coords are not font-size dependent
//		(such CSS length or coord is ignored on parsing) 

typedef enum eDocPropCSSUnit
{
	kDOC_CSSUNIT_PERCENT = 0,			/* percentage (number range is [0.0, 1.0]) */
	kDOC_CSSUNIT_PX,					/* pixels */
	kDOC_CSSUNIT_PC,					/* picas */
	kDOC_CSSUNIT_PT,					/* points */
	kDOC_CSSUNIT_MM,					/* millimeters */
	kDOC_CSSUNIT_CM,					/* centimeters */
	kDOC_CSSUNIT_IN,					/* inchs */
	kDOC_CSSUNIT_EM,					/* size of the current font */
	kDOC_CSSUNIT_EX,					/* x-height of the current font */
	kDOC_CSSUNIT_FONTSIZE_LARGER,		/* larger than the parent node font size (used by FONTSIZE only) */
	kDOC_CSSUNIT_FONTSIZE_SMALLER		/* smaller than the parent node font size (used by FONTSIZE only) */
} eDocPropCSSUnit;

/** padding or margin type definition (in TWIPS) */
typedef struct sDocPropPaddingMargin
{
	sLONG left;
	sLONG top;
	sLONG right;
	sLONG bottom;
} sDocPropPaddingMargin;

/** color definition */
typedef struct sDocPropColor
{
	RGBAColor color;
	bool transparent;
} sDocPropColor;

/** layout mode */
typedef enum eDocPropLayoutMode
{
	kDOC_LAYOUT_MODE_NORMAL = 0,
	kDOC_LAYOUT_MODE_PAGE = 1
} eDocPropLayoutMode;


/** document node type */
typedef enum eDocNodeType
{
	kDOC_NODE_TYPE_DOCUMENT = 0,
	kDOC_NODE_TYPE_PARAGRAPH,
	kDOC_NODE_TYPE_IMAGE,
	kDOC_NODE_TYPE_TABLE,
	kDOC_NODE_TYPE_TABLEROW,
	kDOC_NODE_TYPE_TABLECELL,

	kDOC_NODE_TYPE_UNKNOWN

} eDocNodeType;


/** document properties */
typedef enum eDocProperty
{
	//document properties
	kDOC_PROP_ID,

	kDOC_PROP_VERSION,
	kDOC_PROP_WIDOWSANDORPHANS,
	kDOC_PROP_USERUNIT,
	kDOC_PROP_DPI,
	kDOC_PROP_ZOOM,
	kDOC_PROP_COMPAT_V13,
	kDOC_PROP_LAYOUT_MODE,
	kDOC_PROP_SHOW_IMAGES,
	kDOC_PROP_SHOW_REFERENCES,
	kDOC_PROP_SHOW_HIDDEN_CHARACTERS,
	kDOC_PROP_LANG,

	kDOC_PROP_DATETIMECREATION,
	kDOC_PROP_DATETIMEMODIFIED,
	kDOC_PROP_TITLE,
	kDOC_PROP_SUBJECT,
	kDOC_PROP_AUTHOR,
	kDOC_PROP_COMPANY,
	kDOC_PROP_NOTES,

	kDOC_NUM_PROP,
	kDOC_PROP_ALL
};

class VDocNode;
class IDocProperty
{
public:
	/** property accessors */
	template <class Type>
	static Type GetProperty( const VDocNode *inNode, const eDocProperty inProperty);
	template <class Type>
	static const Type& GetPropertyRef( const VDocNode *inNode, const eDocProperty inProperty);

	template <class Type>
	static void SetProperty( VDocNode *inNode, const eDocProperty inProperty, const Type inValue);
	template <class Type>
	static void SetPropertyPerRef( VDocNode *inNode, const eDocProperty inProperty, const Type& inValue);

	template <class Type>
	static Type GetPropertyDefault( const eDocProperty inProperty);

	template <class Type>
	static const Type& GetPropertyDefaultRef( const eDocProperty inProperty);
};


/** property class (polymorph type) 
@remarks
	it is used internally by VDocNode to stored parsed properties

	note that parsed CSS coords & lengths if not percentage are stored in TWIPS as SLONG (coords) or ULONG (lengths) (unparsed value is converted to TWIPS according to document DPI)
	CSS percentage value is stored as a SmallReal with range constrained to [0,1] - relative values are computed only by GUI classes for layout
*/
class VDocProperty : public VObject 
{
public:
	typedef enum eType
	{
		kPROP_TYPE_BOOL,
		kPROP_TYPE_ULONG,			//used also for storing parsed CSS lengths in TWIPS
		kPROP_TYPE_SLONG,			//used also for storing parsed CSS coords in TWIPS
		kPROP_TYPE_SMALLREAL,		
		kPROP_TYPE_REAL,	

		kPROP_TYPE_PERCENT,			//CSS percentage (SmallReal on range [0,1])
		kPROP_TYPE_LAYOUTMODE,		//VDocument layout mode
		kPROP_TYPE_CSSUNIT,			//eDocPropCSSUnit
		kPROP_TYPE_PADDINGMARGIN,	//sDocPropPaddingMargin 
		kPROP_TYPE_VECTOR_SLONG,	//tab stops offsets in TWIPS or other vector of lengths or coords in TWIPS
		kPROP_TYPE_VSTRING, 
		kPROP_TYPE_VTIME,			//storage for date & time
		kPROP_TYPE_COLOR,			//sDocPropColor (color+transparent flag)

		kPROP_TYPE_INHERIT			//inherited property (might be used for properties which are not inherited on default - like margin, padding, etc...)
	} eType;

	typedef std::vector<sLONG> VectorOfSLONG;

	eType GetType() const { return fType; }

	VDocProperty() { fType = kPROP_TYPE_INHERIT; fValue.fBool = true; } //default constructor -> inherited property ("inherit" CSS value)

	bool IsInherited() const { return fType == kPROP_TYPE_INHERIT; }

	VDocProperty( const bool inValue) { fType = kPROP_TYPE_BOOL; fValue.fBool = inValue; }
	operator bool() const { return fValue.fBool; }

	VDocProperty( const uLONG inValue) { fType = kPROP_TYPE_ULONG; fValue.fULong = inValue; }
	operator uLONG() const { return fValue.fULong; }
	bool IsULONG() const { return fType == kPROP_TYPE_ULONG; }

	VDocProperty( const sLONG inValue) { fType = kPROP_TYPE_SLONG; fValue.fSLong = inValue; }
	operator sLONG() const { return fValue.fSLong; }
	bool IsSLONG() const { return fType == kPROP_TYPE_SLONG; }

	VDocProperty( const SmallReal inValue, bool isPercent = false) 
	{ 
		fType = isPercent ? kPROP_TYPE_PERCENT : kPROP_TYPE_SMALLREAL; 
		fValue.fSmallReal = inValue; 
		if (isPercent)
		{
			if (fValue.fSmallReal < 0.0f)
				fValue.fSmallReal = 0.0f;
			else if (fValue.fSmallReal > 1.0f)
				fValue.fSmallReal = 1.0f;
		}
	}
	operator SmallReal() const { return fValue.fSmallReal; }
	bool IsSmallReal() const { return fType == kPROP_TYPE_SMALLREAL; }
	bool IsPercent() const { return fType == kPROP_TYPE_PERCENT; }

	VDocProperty( const Real inValue) { fType = kPROP_TYPE_REAL; fValue.fReal = inValue; }
	operator Real() const { return fValue.fReal; }
	bool IsReal() const { return fType == kPROP_TYPE_REAL; }

	VDocProperty( const eDocPropLayoutMode inValue) { fType = kPROP_TYPE_LAYOUTMODE; fValue.fLayoutMode = inValue; }
	operator eDocPropLayoutMode() const { return fValue.fLayoutMode; }
	bool IsLayoutMode() const { return fType == kPROP_TYPE_LAYOUTMODE; }

	VDocProperty( const eDocPropCSSUnit inValue) { fType = kPROP_TYPE_CSSUNIT; fValue.fCSSUnit = inValue; }
	operator eDocPropCSSUnit() const { return fValue.fCSSUnit; }
	bool IsCSSUnit() const { return fType == kPROP_TYPE_CSSUNIT; }

	VDocProperty( const sDocPropPaddingMargin& inValue) { fType = kPROP_TYPE_PADDINGMARGIN; fValue.fPaddingMargin = inValue; }
	const sDocPropPaddingMargin& GetPaddingMargin() const { return fValue.fPaddingMargin; }
	operator const sDocPropPaddingMargin&() const { return fValue.fPaddingMargin; }
	bool IsPaddingOrMargin() const { return fType == kPROP_TYPE_PADDINGMARGIN; }

	VDocProperty( const VectorOfSLONG& inValue) { fType = kPROP_TYPE_VECTOR_SLONG; fVectorOfSLong = inValue; }
	const VectorOfSLONG& GetVectorOfSLONG() const { return fVectorOfSLong; }
	operator const VectorOfSLONG&() const { return fVectorOfSLong; }
	bool IsVectorOfSLONG() const { return fType == kPROP_TYPE_VECTOR_SLONG; }

	VDocProperty( const VString& inValue) { fType = kPROP_TYPE_VSTRING; fString = inValue; }
	const VString &GetString() const { fString; } 
	operator const VString&() const { return fString; }
	bool IsString() const { return fType == kPROP_TYPE_VSTRING; }

	VDocProperty( const VTime& inValue) { fType = kPROP_TYPE_VTIME; fTime = inValue; }
	const VTime &GetTime() const { return fTime; } 
	operator const VTime&() const { return fTime; }
	bool IsTime() const { return fType == kPROP_TYPE_VTIME; }

	VDocProperty( const sDocPropColor& inValue) { fType = kPROP_TYPE_COLOR; fValue.fColor = inValue; }
	operator const sDocPropColor&() const { return fValue.fColor; }
	bool IsColor() const { return fType == kPROP_TYPE_COLOR; }
	RGBAColor GetRGBAColor() const { return fValue.fColor.color; } 
	bool IsTransparent() const { return fValue.fColor.transparent; }

	VDocProperty( const VDocProperty& inValue)
	{
		if (&inValue == this)
			return;
		*this = inValue;
	}

	VDocProperty& operator =(const VDocProperty& inValue);

	bool operator ==(const VDocProperty& inValue);

	virtual ~VDocProperty() {} 

private:
	typedef union 
	{
		bool fBool;
		uLONG fULong;
		sLONG fSLong;
		SmallReal fSmallReal;
		Real fReal;
		eDocPropLayoutMode fLayoutMode;
		eDocPropCSSUnit fCSSUnit;
		sDocPropPaddingMargin fPaddingMargin;
		sDocPropColor fColor;
	} VALUE;

	VString fString;
	VTime fTime;
	VectorOfSLONG fVectorOfSLong;

	eType fType;
	VALUE fValue;
};

/** JSON document node property class 
@remarks
	this class is instanciated as needed by VDocNode (normally only while communicating with 4D language)
	and maintained synchronous with VDocNode parsed properties
*/
class VDocNode;
class VJSONDocProps: public VJSONObject
{
public:
			VJSONDocProps(VDocNode *inNode);

			/** return current dirty stamp 
			@remarks
				used to sync with VDocNode if JSONObject has been modified by 4D language
			*/
			uLONG					GetDirtyStamp() const { return fDirtyStamp; }

			///////////////////////////////////////////////////////////////////////////////////////////////////////
			//
			//	derived from VJSONObject
			//
			///////////////////////////////////////////////////////////////////////////////////////////////////////

			// if inValue is not JSON_undefined, SetProperty sets the property named inName with specified value, replacing previous one if it exists.
			// else the property is removed from the collection.
			bool					SetProperty( const VString& inName, const VJSONValue& inValue); 

			// remove the property named inName.
			// equivalent to SetProperty( inName, VJSONValue( JSON_undefined));
			void					RemoveProperty( const VString& inName);
			
			// Remove all properties.
			void					Clear();
protected:
	uLONG fDirtyStamp;
	VRefPtr<VDocNode> fDocNode;
};


/** document node class */
class VDocNode : public VObject, public IRefCountable, public IDocProperty
{
public:
	friend class IDocProperty;

	/** vector of nodes */
	typedef std::vector< VRefPtr<VDocNode> > VectorOfNode;

	/** map of properties */
	typedef std::map<uLONG, VDocProperty> MapOfProp;

	static void Init();
	static void DeInit();

	VDocNode():VObject(),IRefCountable() 
	{ 
		fType = kDOC_NODE_TYPE_UNKNOWN; 
		fParent = NULL; 
		fDoc = NULL; 
		if (!sPropInitDone) 
			Init();
		fID = "0";
		fDirtyStamp = 0;
	}
	
	virtual ~VDocNode() {}

	/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
	virtual VDocNode *RetainNode(const VString& inID)
	{
		if (fDoc)
			return fDoc->RetainNode( inID);
		else
			return NULL;
	}

	/** create JSONDocProps & sync it with this node
	@remarks
		should be called only if needed (for instance if caller need com with 4D language through C_OBJECT)
		(will do nothing if internal JSON object is created yet)

		after this command is called (& until ClearJSONProps is called)
		the internal JSON props are synchronized with this node properties:
		caller might also get/set properties using the JSON object returned by RetainJSONProps
	*/
	void InitJSONProps();

	/** release internal JSONDocProps object */
	void ClearJSONProps() { fJSONProps = NULL; }

	/** return retained JSON property object 
	@remarks
		might be NULL if InitJSONDocProps() has not been called first

		caller can use it to set/get this node properties with JSON
	*/
	VJSONDocProps *RetainJSONProps() { return fJSONProps.Retain(); } 

	/** add child node */
	void AddChild( VDocNode *inNode);

	/** detach child node from parent */
	void Detach();

	/** get children count */
	VIndex GetChildCount() const { return (VIndex)fChildren.size(); }

	/** get Nth child (0-based) */
	VDocNode *operator [](VIndex inIndex) 
	{ 
		if (testAssert(inIndex >= 0 && inIndex < fChildren.size())) 
			return fChildren[inIndex].Get();  
		else
			return NULL;
	}

	/** retain Nth child node (0-based) */
	VDocNode *RetainChild(VIndex inIndex)
	{
		if (testAssert(inIndex >= 0 && inIndex < fChildren.size())) 
			return fChildren[inIndex].Retain();  
		else
			return NULL;
	}

	/** get current ID */
	const VString& GetID() const { return fID; }

	/** set current ID (custom ID) */
	bool SetID(const VString& inValue) 
	{	
		VDocNode *node = RetainNode(inValue);
		if (node)
		{
			ReleaseRefCountable(&node);
			return false;
		}
		fID = inValue; 
		fDirtyStamp++; 
	}

	/** dirty stamp counter accessor */
	uLONG GetDirtyStamp() const { return fDirtyStamp; }

	/** synchronize the passed property or all properties to JSON (only if JSON object is created) */
	void _SyncToJSON(const eDocProperty inPropID = kDOC_PROP_ALL);

	/** synchronize the passed property (using JSON prop name) or all properties from JSON object to this node */
	void _SyncFromJSON( const VString& inPropJSONName = "");
protected:
	void _GenerateID(VString& outID);
	void _OnAttachToDocument(VDocNode *inDocumentNode);
	void _OnDetachFromDocument(VDocNode *inDocumentNode);
	
	/** node type */
	eDocNodeType fType;

	/** node ID (not in fProps for faster access) */
	VString fID;

	/** document node */
	VDocNode *fDoc;

	/** parent node */
	VDocNode *fParent;

	/** locally defined properties */
	MapOfProp fProps;
	
	/** children nodes */
	VectorOfNode fChildren;

	/** dirty stamp counter */
	uLONG fDirtyStamp;

	/** ref on JSON object 
	@remarks
		it is instanciated only if needed
	*/
	VRefPtr<VJSONDocProps> fJSONProps;
	uLONG fJSONPropsDirtyStamp;

	static bool sPropInitDone;

	/** map of inherited properties */
	static bool sPropInherited[kDOC_NUM_PROP];

	/** map of default values */
	static VDocProperty *sPropDefault[kDOC_NUM_PROP];

	static VCriticalSection sMutex;
};

/** main document class 
@remarks
	it is used for both full document or document fragment
	(if AddChild adds/inserts a VDocText, it is assumed to be a document fragment & so only VDocText child nodes are added:
	 on attach or detach, ids are updated in order to ensure ids are unique in one document)
*/
class VDocText : public VDocNode
{
public:
	typedef unordered_map_VString< VRefPtr<VDocNode> > MapOfNodePerID;

	friend class VDocNode;

	VDocText():VDocNode() 
	{
		fType = kDOC_NODE_TYPE_DOCUMENT;
		fDoc = static_cast<VDocNode *>(this);
		fParent = NULL;
		fNextIDCount = 1;
		_GenerateID(fID);
		fMapOfNodePerID[fID] = static_cast<VDocNode *>(this);
	}
	virtual ~VDocText() 
	{
	}

	/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
	VDocNode *RetainNode(const VString& inID);

	//document properties

	uLONG GetVersion() const { return GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_VERSION); }
	void SetVersion(const uLONG inValue) { SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_VERSION, inValue); }

	bool GetWidowsAndOrphans() const { return GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_WIDOWSANDORPHANS); }
	void SetWidowsAndOrphans(const bool inValue) { SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_WIDOWSANDORPHANS, inValue); }

	eDocPropCSSUnit GetUserUnit() const { return GetProperty<eDocPropCSSUnit>( static_cast<const VDocNode *>(this), kDOC_PROP_USERUNIT); }
	void SetUserUnit(const eDocPropCSSUnit inValue) { SetProperty<eDocPropCSSUnit>( static_cast<VDocNode *>(this), kDOC_PROP_USERUNIT, inValue); }

	uLONG GetDPI() const { return GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_DPI); }
	void SetDPI(const uLONG inValue) { SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_DPI, inValue); }

	SmallReal GetZoom() const { return GetProperty<SmallReal>( static_cast<const VDocNode *>(this), kDOC_PROP_ZOOM); }
	void SetZoom(const SmallReal inValue) { SetProperty<SmallReal>( static_cast<VDocNode *>(this), kDOC_PROP_ZOOM, inValue); }

	bool GetCompatV13() const { return GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_COMPAT_V13); }
	void SetCompatV13(const bool inValue) { SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_COMPAT_V13, inValue); }
	
	eDocPropLayoutMode GetLayoutMode() const { return GetProperty<eDocPropLayoutMode>( static_cast<const VDocNode *>(this), kDOC_PROP_LAYOUT_MODE); }
	void SetLayoutMode(const eDocPropLayoutMode inValue) { SetProperty<eDocPropLayoutMode>( static_cast<VDocNode *>(this), kDOC_PROP_LAYOUT_MODE, inValue); }

	bool ShouldShowImages() const { return GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_SHOW_IMAGES); }
	void ShouldShowImages(const bool inValue) { SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_SHOW_IMAGES, inValue); }

	bool ShouldShowReferences() const { return GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_SHOW_REFERENCES); }
	void ShouldShowReferences(const bool inValue) { SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_SHOW_REFERENCES, inValue); }

	bool ShouldShowHiddenCharacters() const { return GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_SHOW_HIDDEN_CHARACTERS); }
	void ShouldShowHiddenCharacters(const bool inValue) { SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_SHOW_HIDDEN_CHARACTERS, inValue); }

	DialectCode GetLang() const { return GetProperty<DialectCode>( static_cast<const VDocNode *>(this), kDOC_PROP_LANG); }
	void SetLang(const DialectCode inValue) { SetProperty<DialectCode>( static_cast<VDocNode *>(this), kDOC_PROP_LANG, inValue); }

	const VTime& GetDateTimeCreation() const { return GetPropertyRef<VTime>( static_cast<const VDocNode *>(this), kDOC_PROP_DATETIMECREATION); }
	void SetDateTimeCreation(const VTime& inValue) { SetPropertyPerRef<VTime>( static_cast<VDocNode *>(this), kDOC_PROP_DATETIMECREATION, inValue); }

	const VTime& GetDateTimeModified() const { return GetPropertyRef<VTime>( static_cast<const VDocNode *>(this), kDOC_PROP_DATETIMEMODIFIED); }
	void SetDateTimeModified(const VTime& inValue) { SetPropertyPerRef<VTime>( static_cast<VDocNode *>(this), kDOC_PROP_DATETIMEMODIFIED, inValue); }

	const VString& GetTitle() const { return GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_TITLE); }
	void SetTitle(const VString& inValue) { SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_TITLE, inValue); }

	const VString& GetSubject() const { return GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_SUBJECT); }
	void SetSubject(const VString& inValue) { SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_SUBJECT, inValue); }

	const VString& GetAuthor() const { return GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_AUTHOR); }
	void SetAuthor(const VString& inValue) { SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_AUTHOR, inValue); }

	const VString& GetCompany() const { return GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_COMPANY); }
	void SetCompany(const VString& inValue) { SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_COMPANY, inValue); }

	const VString& GetNotes() const { return GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_NOTES); }
	void SetNotes(const VString& inValue) { SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_NOTES, inValue); }

protected:
	uLONG _AllocID() { return fNextIDCount++; }

	/** document next ID number 
	@remarks
		it is impossible for one document to have more than kMAX_uLONG nodes
		so incrementing ID is enough to ensure ID is unique
		(if a node is added to a element, new node ID (& new node children nodes IDs) are generated again to ensure new nodes IDs are unique for this document)
	*/ 
	uLONG fNextIDCount;

	MapOfNodePerID fMapOfNodePerID;
};


END_TOOLBOX_NAMESPACE


#endif
