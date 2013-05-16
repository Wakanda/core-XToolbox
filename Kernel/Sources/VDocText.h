
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

#include <map>

BEGIN_TOOLBOX_NAMESPACE

/** padding or margin type definition (in TWIPS) */
typedef struct sDocPropPaddingMargin
{
	sLONG left;
	sLONG top;
	sLONG right;
	sLONG bottom;
} sDocPropPaddingMargin;

/** layout mode */
typedef enum
{
	kDOC_LAYOUT_MODE_NORMAL = 0,
	kDOC_LAYOUT_MODE_PAGE = 1
} eDocPropLayoutMode;

/** text align */
typedef eStyleJust eDocPropTextAlign;

/** text direction (alias for CSSProperty::eDirection) */
typedef enum
{
	kTEXT_DIRECTION_LTR,
	kTEXT_DIRECTION_RTL
} eDocPropDirection;

/** tab stop type (alias for CSSProperty::eTabStopType) */
typedef enum
{
	kTEXT_TAB_STOP_TYPE_LEFT,
	kTEXT_TAB_STOP_TYPE_RIGHT,
	kTEXT_TAB_STOP_TYPE_CENTER,
	kTEXT_TAB_STOP_TYPE_DECIMAL,
	kTEXT_TAB_STOP_TYPE_BAR
} eDocPropTabStopType;


/** document node type */
typedef enum
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
typedef enum
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

	//kDOC_PROP_TITLE,		//user properties: stored only in JSON object (not managed internally by the document but only by 4D language)
	//kDOC_PROP_SUBJECT,
	//kDOC_PROP_AUTHOR,
	//kDOC_PROP_COMPANY,
	//kDOC_PROP_NOTES,

	//common properties

	kDOC_PROP_TEXT_ALIGN,
	kDOC_PROP_VERTICAL_ALIGN,
	kDOC_PROP_MARGIN,
	kDOC_PROP_BACKGROUND_COLOR,

	//paragraph properties

	kDOC_PROP_DIRECTION,
	kDOC_PROP_LINE_HEIGHT,
	kDOC_PROP_PADDING_FIRST_LINE,
	kDOC_PROP_TAB_STOP_OFFSET,
	kDOC_PROP_TAB_STOP_TYPE,

	//span properties (character style properties)
	//(managed through VTreeTextStyle)

	kDOC_PROP_FONT_FAMILY,		
	kDOC_PROP_FONT_SIZE,			
	kDOC_PROP_COLOR,				
	kDOC_PROP_FONT_STYLE,		
	kDOC_PROP_FONT_WEIGHT,		
	kDOC_PROP_TEXT_DECORATION,
	kDOC_PROP_4DREF,		// '-d4-ref'
	kDOC_PROP_4DREF_USER,	// '-d4-ref-user'

	kDOC_NUM_PROP,

	kDOC_PROP_ALL
} eDocProperty;


/** property class (polymorph type) 
@remarks
	it is used internally by VDocNode to stored parsed properties

	note that parsed CSS coords & lengths if not percentage are stored in TWIPS as SLONG (coords) or ULONG (lengths) (unparsed value is converted to TWIPS according to document DPI)
	CSS percentage value is stored as a SmallReal with 100% mapped to 1.0 - some relative values might be computed only by GUI classes for final layout (if relative value depends on some layout properties)
*/
class XTOOLBOX_API VDocProperty : public VObject 
{
public:
	typedef enum
	{
		kPROP_TYPE_BOOL,
		kPROP_TYPE_ULONG,			//used also for storing parsed CSS lengths in TWIPS
		kPROP_TYPE_SLONG,			//used also for storing parsed CSS coords in TWIPS
		kPROP_TYPE_SMALLREAL,		
		kPROP_TYPE_REAL,	

		kPROP_TYPE_PERCENT,			//CSS percentage (SmallReal on range [0,1])
		kPROP_TYPE_PADDINGMARGIN,	//sDocPropPaddingMargin 
		kPROP_TYPE_VECTOR_SLONG,	//tab stops offsets in TWIPS or other vector of lengths or coords in TWIPS
		kPROP_TYPE_VSTRING, 
		kPROP_TYPE_VTIME,			//storage for date & time

		kPROP_TYPE_INHERIT			//inherited property (might be used for properties which are not inherited on default - like margin, padding, etc...)
	} eType;

	typedef std::vector<sLONG> VectorOfSLONG;

	eType GetType() const { return fType; }

	VDocProperty() { fType = kPROP_TYPE_INHERIT; fValue.fULong = 0; } //default constructor -> inherited property ("inherit" CSS value)
																	  //(we set default ULONG value to 0 to ensure methods which store enum value in ULONG will not fail if called
																	  // - because 0 is valid for all enum types)

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
	}
	operator SmallReal() const { return fValue.fSmallReal; }
	bool IsSmallReal() const { return fType == kPROP_TYPE_SMALLREAL; }
	bool IsPercent() const { return fType == kPROP_TYPE_PERCENT; }

	VDocProperty( const Real inValue) { fType = kPROP_TYPE_REAL; fValue.fReal = inValue; }
	operator Real() const { return fValue.fReal; }
	bool IsReal() const { return fType == kPROP_TYPE_REAL; }

	VDocProperty( const sDocPropPaddingMargin& inValue) { fType = kPROP_TYPE_PADDINGMARGIN; fValue.fPaddingMargin = inValue; }
	const sDocPropPaddingMargin& GetPaddingMargin() const { return fValue.fPaddingMargin; }
	operator const sDocPropPaddingMargin&() const { return fValue.fPaddingMargin; }
	bool IsPaddingOrMargin() const { return fType == kPROP_TYPE_PADDINGMARGIN; }

	VDocProperty( const VectorOfSLONG& inValue) { fType = kPROP_TYPE_VECTOR_SLONG; fVectorOfSLong = inValue; }
	const VectorOfSLONG& GetVectorOfSLONG() const { return fVectorOfSLong; }
	operator const VectorOfSLONG&() const { return fVectorOfSLong; }
	bool IsVectorOfSLONG() const { return fType == kPROP_TYPE_VECTOR_SLONG; }

	VDocProperty( const VString& inValue) { fType = kPROP_TYPE_VSTRING; fString = inValue; }
	const VString &GetString() const { return fString; } 
	operator const VString&() const { return fString; }
	bool IsString() const { return fType == kPROP_TYPE_VSTRING; }

	VDocProperty( const VTime& inValue) { fType = kPROP_TYPE_VTIME; fTime = inValue; }
	const VTime &GetTime() const { return fTime; } 
	operator const VTime&() const { return fTime; }
	bool IsTime() const { return fType == kPROP_TYPE_VTIME; }

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
		sDocPropPaddingMargin fPaddingMargin;
	} VALUE;

	VString fString;
	VTime fTime;
	VectorOfSLONG fVectorOfSLong;

	eType fType;
	VALUE fValue;
};

class VDocNode;


/** JSON document node property class 
@remarks
	this class is instanciated as needed by VDocNode (normally only while communicating with 4D language)
*/
class XTOOLBOX_API VJSONDocProps: public VJSONObject
{
public:
			VJSONDocProps(VDocNode *inNode);

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
	VRefPtr<VDocNode> fDocNode;
};


/** document node class */
class IDocProperty;
class VDocText;
class XTOOLBOX_API VDocNode : public VObject, public IRefCountable
{
public:
	friend class IDocProperty;

	/** map of properties */
	typedef std::map<uLONG, VDocProperty> MapOfProp;

	/** vector of nodes */
	typedef std::vector< VRefPtr<VDocNode> > VectorOfNode;

	static void Init();
	static void DeInit();

	VDocNode():VObject(),IRefCountable() 
	{ 
		fType = kDOC_NODE_TYPE_UNKNOWN; 
		fParent = NULL; 
		fDoc = NULL; 
		if (!sPropInitDone) 
			Init();
		fID = "1";
		fDirtyStamp = 0;
	}

	virtual ~VDocNode() {}

	eDocNodeType GetType() const { return fType; }

	/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
	virtual VDocNode *RetainNode(const VString& inID)
	{
		if (fDoc)
			return fDoc->RetainNode( inID);
		else
			return NULL;
	}

	/** retain document node */
	VDocText *RetainDocumentNode();

	/** get document node */
	const VDocText *GetDocumentNode() const;

	/** get document node for write */
	VDocText *GetDocumentNodeForWrite();

	/** create JSONDocProps object
	@remarks
		should be called only if needed (for instance if caller need com with 4D language through C_OBJECT)

		caller might also get/set properties using the JSON object returned by RetainJSONProps
	*/
	VJSONDocProps *CreateAndRetainJSONProps();

	/** add child node */
	void AddChild( VDocNode *inNode);

	/** detach child node from parent */
	void Detach();

	/** remove Nth child node (0-based index) */
	void RemoveNthChild( VIndex inIndex)
	{
		xbox_assert(inIndex >= 0 && inIndex < fChildren.size());

		VRefPtr<VDocNode> nodeToRemove = fChildren[inIndex];
		nodeToRemove->Detach(); //is detached from this node (removed from fChildren) so maybe destroyed on nodeToRemove release
	}

	/** get children count */
	VIndex GetChildCount() const { return (VIndex)fChildren.size(); }

	/** get Nth child node (0-based index) */
	const VDocNode *GetChild(VIndex inIndex) const
	{
		if (testAssert(inIndex >= 0 && inIndex < fChildren.size())) 
			return fChildren[inIndex].Get();  
		else
			return NULL;
	}

	/** retain Nth child node (0-based index) */
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
			xbox_assert(false); //ID is used yet (!)
			ReleaseRefCountable(&node);
			return false;
		}
		fID = inValue; 
		fDirtyStamp++; 
		return true;
	}

	/** dirty stamp counter accessor */
	uLONG GetDirtyStamp() const { return fDirtyStamp; }

	/** append properties from the passed node
	@remarks
		if inOnlyIfInheritOrNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties
	*/
	virtual void AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfInheritOrNotOverriden = false, bool inNoAppendSpanStyles = false); 

	/** common element properties */

	/** return true if at least one property is overriden locally */
	bool HasProps() const { return fProps.size() > 0; }	

	/** force a property to be inherited */
	void SetInherit( const eDocProperty inProp);

	/** return true if property value is inherited */ 
	bool IsInherited( const eDocProperty inProp) const;

	/** return true if property value is overriden locally 
		(if IsInherited() == true, it is a force inherit (that is CSS value = "inherit"), otherwise it is a value override) */ 
	bool IsOverriden( const eDocProperty inProp) const;

	/** remove local property */
	void RemoveProp( const eDocProperty inProp);

	/** get default property value */
	const VDocProperty& GetDefaultPropValue( const eDocProperty inProp) const;

	eDocPropTextAlign GetTextAlign() const; 
	void SetTextAlign(const eDocPropTextAlign inValue);

	eDocPropTextAlign GetVerticalAlign() const; 
	void SetVerticalAlign(const eDocPropTextAlign inValue);

	const sDocPropPaddingMargin& GetMargin() const;
	void SetMargin(const sDocPropPaddingMargin& inValue);

	RGBAColor GetBackgroundColor() const;
	void SetBackgroundColor(const RGBAColor inValue);

protected:
	virtual VDocNode *Clone() const {	return NULL; }
	VDocNode( const VDocNode* inNode);

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
	 on attach, ids are updated in order to ensure ids are unique in one document)
*/
class XTOOLBOX_API VDocText : public VDocNode
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
	}
	virtual ~VDocText() 
	{
	}

	virtual VDocNode *Clone() const {	return static_cast<VDocNode *>(new VDocText(this)); }

	/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
	VDocNode *RetainNode(const VString& inID);

	//document properties

	uLONG GetVersion() const;
	void SetVersion(const uLONG inValue);

	bool GetWidowsAndOrphans() const;
	void SetWidowsAndOrphans(const bool inValue);

	eCSSUnit GetUserUnit() const;
	void SetUserUnit(const eCSSUnit inValue);

	uLONG GetDPI() const;
	void SetDPI(const uLONG inValue);

	/** get zoom percentage 
	@remarks
		100 -> 100%
		250 -> 250%
	*/
	uLONG GetZoom() const;

	void SetZoom(const uLONG inValue);

	bool GetCompatV13() const;
	void SetCompatV13(const bool inValue);

	eDocPropLayoutMode GetLayoutMode() const;
	void SetLayoutMode(const eDocPropLayoutMode inValue);

	bool ShouldShowImages() const;
	void ShouldShowImages(const bool inValue);

	bool ShouldShowReferences() const;
	void ShouldShowReferences(const bool inValue);

	bool ShouldShowHiddenCharacters() const;
	void ShouldShowHiddenCharacters(const bool inValue);

	DialectCode GetLang() const;
	void SetLang(const DialectCode inValue);

	const VTime& GetDateTimeCreation() const;
	void SetDateTimeCreation(const VTime& inValue);

	const VTime& GetDateTimeModified() const;
	void SetDateTimeModified(const VTime& inValue);

	//const VString& GetTitle() const;
	//void SetTitle(const VString& inValue);

	//const VString& GetSubject() const;
	//void SetSubject(const VString& inValue);

	//const VString& GetAuthor() const;
	//void SetAuthor(const VString& inValue);

	//const VString& GetCompany() const;
	//void SetCompany(const VString& inValue);

	//const VString& GetNotes() const;
	//void SetNotes(const VString& inValue);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//
	// following methods are helper methods to set/get document text & character styles (for setting/getting document or paragraph properties, use property accessors)
	// remark: for now VDocText contains only one VDocParagraph child node & so methods are for now redirected to the VDocParagraph node
	//
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/** replace current text & character styles 
	@remarks
		if inCopyStyles == true, styles are copied
		if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
	*/
	void SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);
	const VString& GetText() const;

	void SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);
	const VTreeTextStyle *GetStyles() const;
	VTreeTextStyle *RetainStyles() const;

	VDocParagraph *RetainFirstParagraph(); 

	/** replace plain text with computed or source span references 
	@param inShowSpanRefs
		false (default): plain text contains span ref computed values if any (4D expression result, url link user text, etc...)
		true: plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
	*/
	void UpdateSpanRefs( bool inShowSpanRefs = false);

	/** replace text with span text reference on the specified range
	@remarks
		span ref plain text is set here to uniform non-breaking space: 
		user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

		you should no more use or destroy passed inSpanRef even if method returns false
	*/
	bool ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inNoUpdateRef = true);

	/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	 

		return true if any 4D expression has been replaced with plain text
	*/
	bool FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1);

	/** return the first span reference which intersects the passed range */
	const VTextStyle *GetStyleRefAtRange(const sLONG inStart = 0, const sLONG inEnd = -1);

protected:
	VDocText( const VDocText* inNodeText):VDocNode(static_cast<const VDocNode *>(inNodeText))	{}

	uLONG _AllocID() { return fNextIDCount++; }

	/** document next ID number 
	@remarks
		it is impossible for one document to have more than kMAX_uLONG nodes
		so incrementing ID is enough to ensure ID is unique
		(if a node is added to a parent node, ID is checked & generated again if node ID is used yet)
	*/ 
	uLONG fNextIDCount;

	mutable MapOfNodePerID fMapOfNodePerID;
};

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

template <class Type>
Type IDocProperty::GetPropertyDefault( const eDocProperty inProperty) 
{
	xbox_assert(VDocNode::sPropInitDone);

	switch (inProperty)
	{
	case kDOC_PROP_DATETIMECREATION:
	case kDOC_PROP_DATETIMEMODIFIED:
		{
			VTime time;
			time.FromSystemTime();
			*(VDocNode::sPropDefault[kDOC_PROP_DATETIMECREATION])	= VDocProperty(time);
			*(VDocNode::sPropDefault[kDOC_PROP_DATETIMEMODIFIED])	= VDocProperty(time);
		}
		break;
	default:
		break;
	}
	return *(VDocNode::sPropDefault[ (uLONG)inProperty]);
}


template <class Type>
const Type& IDocProperty::GetPropertyDefaultRef( const eDocProperty inProperty) 
{
	xbox_assert(VDocNode::sPropInitDone);

	switch (inProperty)
	{
	case kDOC_PROP_DATETIMECREATION:
	case kDOC_PROP_DATETIMEMODIFIED:
		{
			VTime time;
			time.FromSystemTime();
			*(VDocNode::sPropDefault[kDOC_PROP_DATETIMECREATION])	= VDocProperty(time);
			*(VDocNode::sPropDefault[kDOC_PROP_DATETIMEMODIFIED])	= VDocProperty(time);
		}
		break;
	default:
		break;
	}
	return *(VDocNode::sPropDefault[ (uLONG)inProperty]);
}



/** property accessors */
template <class Type>
Type IDocProperty::GetProperty( const VDocNode *inNode, const eDocProperty inProperty) 
{
	//search first in local node properties
	VDocNode::MapOfProp::const_iterator it = inNode->fProps.find( (uLONG)inProperty);
	bool isInherited = false;
	if (it != inNode->fProps.end())
	{	
		isInherited = it->second.IsInherited(); //explicitly inherited (CSS 'inherit' value) - override default CSS inherited status
		if (!isInherited)
			return (Type)(it->second);
	}

	//then check inherited properties
	if (inNode->fParent && (isInherited || inNode->sPropInherited[(uLONG)inProperty]))
		return GetProperty<Type>( inNode->fParent, inProperty);

	//finally return global default value 
	return GetPropertyDefault<Type>( inProperty);
}

template <class Type>
const Type& IDocProperty::GetPropertyRef( const VDocNode *inNode, const eDocProperty inProperty) 
{
	//search first local property
	VDocNode::MapOfProp::const_iterator it = inNode->fProps.find( (uLONG)inProperty);
	bool isInherited = false;
	if (it != inNode->fProps.end())
	{	
		isInherited = it->second.IsInherited(); //explicitly inherited (CSS 'inherit' value) - override default CSS inherited status
		if (!isInherited)
			return it->second;
	}

	//then check inherited properties
	if (inNode->fParent && (isInherited || inNode->sPropInherited[(uLONG)inProperty]))
		return GetPropertyRef<Type>( inNode->fParent, inProperty);

	//finally return global default value 
	return GetPropertyDefaultRef<Type>( inProperty);
}

template <class Type>
void IDocProperty::SetProperty( VDocNode *inNode, const eDocProperty inProperty, const Type inValue)
{
	VDocProperty value( inValue);
	if (inNode->fProps[ (uLONG)inProperty] == value)
		return;
	inNode->fProps[ (uLONG)inProperty] = value;
	inNode->fDirtyStamp++;
}

template <class Type>
void IDocProperty::SetPropertyPerRef( VDocNode *inNode, const eDocProperty inProperty, const Type& inValue)
{
	VDocProperty value( inValue);
	if (inNode->fProps[ (uLONG)inProperty] == value)
		return;
	inNode->fProps[ (uLONG)inProperty] = value;
	inNode->fDirtyStamp++;
}


END_TOOLBOX_NAMESPACE


#endif
