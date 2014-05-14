
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

#define RGBACOLOR_UNDEFINED 0xbadbad  //used to filter undefine border color - as it is color with alpha = 0 it is never used as a valid border color either

/** CSS rect type definition */
typedef struct sDocPropRect
{
	sLONG left;
	sLONG top;
	sLONG right;
	sLONG bottom;

	bool operator ==( const sDocPropRect& inRect) const { return left == inRect.left && top == inRect.top && right == inRect.right && bottom == inRect.bottom; }
} sDocPropRect;

/** layout page mode */
typedef enum eDocPropLayoutPageMode
{
	kDOC_LAYOUT_MODE_NORMAL = 0,
	kDOC_LAYOUT_MODE_PAGE = 1,
	kDOC_LAYOUT_MODE_WEB = 2
} eDocPropLayoutPageMode;

/** text align */
typedef eStyleJust eDocPropTextAlign;

/** text direction (alias for CSSProperty::eDirection) */
typedef enum eDocPropDirection
{
	kDOC_DIRECTION_LTR = 0,
	kDOC_DIRECTION_RTL
} eDocPropDirection;

/** tab stop type (alias for CSSProperty::eTabStopType) */
typedef enum eDocPropTabStopType
{
	kDOC_TAB_STOP_TYPE_LEFT = 0,
	kDOC_TAB_STOP_TYPE_RIGHT,
	kDOC_TAB_STOP_TYPE_CENTER,
	kDOC_TAB_STOP_TYPE_DECIMAL,
	kDOC_TAB_STOP_TYPE_BAR
} eDocPropTabStopType;


/** border style (alias for CSSProperty::eBorderStyle) */
typedef enum eDocPropBorderStyle
{
	kDOC_BORDER_STYLE_NONE = 0,
	kDOC_BORDER_STYLE_HIDDEN,
	kDOC_BORDER_STYLE_DOTTED,
	kDOC_BORDER_STYLE_DASHED,
	kDOC_BORDER_STYLE_SOLID,
	kDOC_BORDER_STYLE_DOUBLE,
	kDOC_BORDER_STYLE_GROOVE,
	kDOC_BORDER_STYLE_RIDGE,
	kDOC_BORDER_STYLE_INSET,
	kDOC_BORDER_STYLE_OUTSET
} eDocPropBorderStyle;

/** background repeat (alias for CSSProperty::eBackgroundRepeat) */
typedef enum eDocPropBackgroundRepeat 
{
	kDOC_BACKGROUND_REPEAT = 0,
	kDOC_BACKGROUND_REPEAT_X,
	kDOC_BACKGROUND_REPEAT_Y,
	kDOC_BACKGROUND_NO_REPEAT

} eDocPropBackgroundRepeat;


/** background box (alias for CSSProperty::eBackgroundBox) */
typedef enum eDocPropBackgroundBox
{
	kDOC_BACKGROUND_BORDER_BOX = 0,
	kDOC_BACKGROUND_PADDING_BOX,
	kDOC_BACKGROUND_CONTENT_BOX

} eDocPropBackgroundBox;


/** background size (alias for CSSProperty::eBackgroundSize)
@remark
	background size doc property type is VectorOfSLong (size = 2 for width & height) ;
	here is the mapping between CSS values (on the left) & doc property sLONG value (on the right):

	length 				length in TWIPS (if length > 0 - 0 is assumed to be auto)
	percentage			[1%;1000%] is mapped to [-1;-1000] (so 200% is mapped to -200) - overflowed percentage is bound to the [1%;1000%]
	auto				kDOC_BACKGROUND_SIZE_AUTO (no height)
	cover				kDOC_BACKGROUND_SIZE_COVER (no height)
	contain				kDOC_BACKGROUND_SIZE_CONTAIN (no height)
*/
typedef enum eDocPropBackgroundSize
{
	kDOC_BACKGROUND_SIZE_AUTO		=	0,
	kDOC_BACKGROUND_SIZE_COVER		=	-10000,
	kDOC_BACKGROUND_SIZE_CONTAIN	=	-10001,
	kDOC_BACKGROUND_SIZE_UNDEFINED	=	-20000	//used to filter undefined width or height

} eDocPropBackgroundSize;


typedef enum eFontStyle
{
	kFONT_STYLE_NORMAL		= 0,
	kFONT_STYLE_ITALIC		= 1,
	kFONT_STYLE_OBLIQUE		= 2		//implemented as italic
} eFontStyle;

typedef enum eTextDecoration 
{
	kTEXT_DECORATION_NONE			= 0,
	kTEXT_DECORATION_UNDERLINE		= 1,
	kTEXT_DECORATION_OVERLINE		= 2,	//remark: not supported
	kTEXT_DECORATION_LINETHROUGH	= 4,	//alias 'strikeout' style
	kTEXT_DECORATION_BLINK			= 8		//remark: not supported
} eTextDecoration;

typedef enum eFontWeight	//note: only normal & bold are actually supported (if value >= kFONT_WEIGHT_BOLD, it is assumed to be kFONT_WEIGHT_BOLD otherwise kFONT_WEIGHT_NORMAL)
{
	kFONT_WEIGHT_100 		= 100,
	kFONT_WEIGHT_200 		= 200,
	kFONT_WEIGHT_300 		= 300,
	kFONT_WEIGHT_400 		= 400,
	kFONT_WEIGHT_500 		= 500,
	kFONT_WEIGHT_600 		= 600,
	kFONT_WEIGHT_700 		= 700,
	kFONT_WEIGHT_800 		= 800,
	kFONT_WEIGHT_900 		= 900,

	kFONT_WEIGHT_NORMAL 	= 400,
	kFONT_WEIGHT_BOLD		= 700,

	kFONT_WEIGHT_LIGHTER	= -1,
	kFONT_WEIGHT_BOLDER		= -2,

	kFONT_WEIGHT_MIN		= 100,
	kFONT_WEIGHT_MAX		= 900
} eFontWeight;	

/** list style type (alias CSSProperty::eListStyleType) */
typedef enum eDocPropListStyleType
{
	kDOC_LIST_STYLE_TYPE_FIRST = 0,

	kDOC_LIST_STYLE_TYPE_NONE = 0,

	kDOC_LIST_STYLE_TYPE_UL_FIRST,

	kDOC_LIST_STYLE_TYPE_DISC = kDOC_LIST_STYLE_TYPE_UL_FIRST,
	kDOC_LIST_STYLE_TYPE_CIRCLE,
	kDOC_LIST_STYLE_TYPE_SQUARE,

	kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE,
	kDOC_LIST_STYLE_TYPE_DIAMOND,
	kDOC_LIST_STYLE_TYPE_CLUB,
	kDOC_LIST_STYLE_TYPE_CUSTOM,

	kDOC_LIST_STYLE_TYPE_UL_LAST = kDOC_LIST_STYLE_TYPE_CUSTOM,
	kDOC_LIST_STYLE_TYPE_UL_4D_FIRST = kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE,
	kDOC_LIST_STYLE_TYPE_UL_4D_LAST = kDOC_LIST_STYLE_TYPE_CUSTOM,

	kDOC_LIST_STYLE_TYPE_OL_FIRST,

	kDOC_LIST_STYLE_TYPE_DECIMAL = kDOC_LIST_STYLE_TYPE_OL_FIRST,
	kDOC_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO,
	kDOC_LIST_STYLE_TYPE_LOWER_LATIN,
	kDOC_LIST_STYLE_TYPE_LOWER_ROMAN,
	kDOC_LIST_STYLE_TYPE_UPPER_LATIN,
	kDOC_LIST_STYLE_TYPE_UPPER_ROMAN,
	kDOC_LIST_STYLE_TYPE_LOWER_GREEK,
	kDOC_LIST_STYLE_TYPE_ARMENIAN,
	kDOC_LIST_STYLE_TYPE_GEORGIAN,
	kDOC_LIST_STYLE_TYPE_HEBREW,
	kDOC_LIST_STYLE_TYPE_HIRAGANA,
	kDOC_LIST_STYLE_TYPE_KATAKANA,
	kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC,

	kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK,
	kDOC_LIST_STYLE_TYPE_OL_4D_FIRST = kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK,
	kDOC_LIST_STYLE_TYPE_OL_4D_LAST = kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK,

	kDOC_LIST_STYLE_TYPE_OL_LAST = kDOC_LIST_STYLE_TYPE_OL_4D_LAST,

	kDOC_LIST_STYLE_TYPE_LAST = kDOC_LIST_STYLE_TYPE_OL_LAST

} eDocPropListStyleType;

#define kDOC_LIST_START_AUTO	-1234567


/** document node type */
typedef enum eDocNodeType
{
	kDOC_NODE_TYPE_DOCUMENT = 0,
	kDOC_NODE_TYPE_DIV, //TODO: not implemented yet
	kDOC_NODE_TYPE_PARAGRAPH,
	kDOC_NODE_TYPE_IMAGE,
	kDOC_NODE_TYPE_TABLE,
	kDOC_NODE_TYPE_TABLEROW,
	kDOC_NODE_TYPE_TABLECELL,

	kDOC_NODE_TYPE_CLASS_STYLE,	//it is used to store class style properties only (storage class for parsed CSS declarations with selector 'elemname.classname')
	kDOC_NODE_TYPE_UNKNOWN,

	kDOC_NODE_TYPE_COUNT

} eDocNodeType;


/** document properties */
typedef enum eDocProperty
{
	//document properties
	kDOC_PROP_ID = 0,
	kDOC_PROP_CLASS,

	kDOC_PROP_VERSION,
	kDOC_PROP_WIDOWSANDORPHANS,
	kDOC_PROP_DPI,
	kDOC_PROP_ZOOM,
	kDOC_PROP_LAYOUT_MODE,
	kDOC_PROP_SHOW_IMAGES,
	kDOC_PROP_SHOW_REFERENCES,
	kDOC_PROP_SHOW_HIDDEN_CHARACTERS,
	kDOC_PROP_LANG,
	kDOC_PROP_DWRITE,

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
	kDOC_PROP_PADDING,
	kDOC_PROP_BACKGROUND_COLOR,
	kDOC_PROP_BACKGROUND_IMAGE,
	kDOC_PROP_BACKGROUND_POSITION,
	kDOC_PROP_BACKGROUND_REPEAT,
	kDOC_PROP_BACKGROUND_CLIP,
	kDOC_PROP_BACKGROUND_ORIGIN,
	kDOC_PROP_BACKGROUND_SIZE,
	kDOC_PROP_WIDTH,
	kDOC_PROP_HEIGHT,
	kDOC_PROP_MIN_WIDTH,
	kDOC_PROP_MIN_HEIGHT,
	kDOC_PROP_BORDER_STYLE,
	kDOC_PROP_BORDER_WIDTH,
	kDOC_PROP_BORDER_COLOR,
	kDOC_PROP_BORDER_RADIUS,

	//paragraph properties

	kDOC_PROP_DIRECTION,
	kDOC_PROP_LINE_HEIGHT,
	kDOC_PROP_TEXT_INDENT,
	kDOC_PROP_TAB_STOP_OFFSET,
	kDOC_PROP_TAB_STOP_TYPE,
	kDOC_PROP_LIST_STYLE_TYPE,	
	kDOC_PROP_LIST_STRING_FORMAT_LTR,	
	kDOC_PROP_LIST_STRING_FORMAT_RTL,	
	kDOC_PROP_LIST_FONT_FAMILY,			//default is "Times New Roman"
										//TODO: maybe we should also allow to customize other list font properties ?
	kDOC_PROP_LIST_STYLE_IMAGE,
	kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT,	
	kDOC_PROP_LIST_START,				//list start number (default is automatic -> 1 or previous paragraph - with same class - number+1)	
	kDOC_PROP_NEW_PARA_CLASS,

	//image properties,

	kDOC_PROP_IMG_SOURCE,
	kDOC_PROP_IMG_ALT_TEXT,

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
	kDOC_PROP_TEXT_BACKGROUND_COLOR,	//-d4-text-background-color (it defines text background color but element background color)

	kDOC_PROP_COUNT,

	kDOC_PROP_ALL
} eDocProperty;

typedef enum eListStyleTypeEvent
{
	kDOC_LSTE_BEFORE_REMOVE_FROM_LIST,		//called before node is removed from list (if node is about to be detached from document or if node list style type is about to be changed)
	kDOC_LSTE_AFTER_ADD_TO_LIST,			//called after node is added to list (if node with defined list style type is added to document or after node list style type is changed)
	kDOC_LSTE_AFTER_UPDATE_START_NUMBER		//called after node start number has been modified
} eListStyleTypeEvent;

#define	D4ID_PREFIX_RESERVED	"D4ID_"		//id prefix reserved for 4D automatic id

/** property class (polymorph type) 
@remarks
	it is used internally by VDocNode to stored parsed properties

	note that parsed CSS coords & lengths if not percentage are stored in TWIPS as SLONG (coords) or ULONG (lengths) (unparsed value is converted to TWIPS according to document DPI)
	CSS percentage value is stored as a SmallReal with 100% mapped to 1.0 - some relative values might be computed only by GUI classes for final layout (if relative value depends on some layout properties)
*/
class XTOOLBOX_API VDocProperty : public VObject 
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
			kPROP_TYPE_RECT,			//sDocPropRect 
			kPROP_TYPE_VECTOR_SLONG,	//tab stops offsets in TWIPS or other vector of lengths or coords in TWIPS
			kPROP_TYPE_VSTRING, 
			kPROP_TYPE_VTIME,			//storage for date & time

			kPROP_TYPE_INHERIT			//inherited property (might be used for properties which are not inherited on default - like margin, padding, etc...)
		} eType;

		typedef std::vector<sLONG> VectorOfSLONG;

		eType					GetType() const { return fType; }

								VDocProperty() { fType = kPROP_TYPE_INHERIT; fValue.fULong = 0; } //default constructor -> inherited property ("inherit" CSS value)
																								  //(we set default ULONG value to 0 to ensure methods which store enum value in ULONG will not fail if called
																								  // - because 0 is valid for all enum types)

		bool					IsInherited() const { return fType == kPROP_TYPE_INHERIT; }

								VDocProperty( const bool inValue) { fType = kPROP_TYPE_BOOL; fValue.fBool = inValue; }
								operator bool() const { return fValue.fBool; }

								VDocProperty( const uLONG inValue) { fType = kPROP_TYPE_ULONG; fValue.fULong = inValue; }
								operator uLONG() const { return fValue.fULong; }
		bool					IsULONG() const { return fType == kPROP_TYPE_ULONG; }

								VDocProperty( const sLONG inValue) { fType = kPROP_TYPE_SLONG; fValue.fSLong = inValue; }
								operator sLONG() const { return fValue.fSLong; }
		bool					IsSLONG() const { return fType == kPROP_TYPE_SLONG; }

								VDocProperty( const SmallReal inValue, bool isPercent = false) 
								{ 
									fType = isPercent ? kPROP_TYPE_PERCENT : kPROP_TYPE_SMALLREAL; 
									fValue.fSmallReal = inValue; 
								}
								operator SmallReal() const { return fValue.fSmallReal; }
		bool					IsSmallReal() const { return fType == kPROP_TYPE_SMALLREAL; }
		bool					IsPercent() const { return fType == kPROP_TYPE_PERCENT; }

								VDocProperty( const Real inValue) { fType = kPROP_TYPE_REAL; fValue.fReal = inValue; }
								operator Real() const { return fValue.fReal; }
		bool					IsReal() const { return fType == kPROP_TYPE_REAL; }

								VDocProperty( const sDocPropRect& inValue) { fType = kPROP_TYPE_RECT; fValue.fRect = inValue; }
		const sDocPropRect&		GetRect() const { return fValue.fRect; }
								operator const sDocPropRect&() const { return fValue.fRect; }
		bool					IsRect() const { return fType == kPROP_TYPE_RECT; }

								VDocProperty( const VectorOfSLONG& inValue) { fType = kPROP_TYPE_VECTOR_SLONG; fVectorOfSLong = inValue; }
		const VectorOfSLONG&	GetVectorOfSLONG() const { return fVectorOfSLong; }
								operator const VectorOfSLONG&() const { return fVectorOfSLong; }
		bool					IsVectorOfSLONG() const { return fType == kPROP_TYPE_VECTOR_SLONG; }

								VDocProperty( const VString& inValue) { fType = kPROP_TYPE_VSTRING; fString = inValue; }
		const VString&			GetString() const { return fString; } 
								operator const VString&() const { return fString; }
		bool					IsString() const { return fType == kPROP_TYPE_VSTRING; }

								VDocProperty( const VTime& inValue) { fType = kPROP_TYPE_VTIME; fTime = inValue; }
		const VTime&			GetTime() const { return fTime; } 
								operator const VTime&() const { return fTime; }
		bool					IsTime() const { return fType == kPROP_TYPE_VTIME; }

								VDocProperty( const VDocProperty& inValue)
								{
									if (&inValue == this)
										return;
									*this = inValue;
								}
		VDocProperty&			operator =(const VDocProperty& inValue);

		bool					operator ==(const VDocProperty& inValue) const;
		bool					operator !=(const VDocProperty& inValue) const { return (!(*this == inValue)); }

virtual							~VDocProperty() {} 

private:
	typedef union 
	{
		bool					fBool;
		uLONG					fULong;
		sLONG					fSLong;
		SmallReal				fSmallReal;
		Real					fReal;
		sDocPropRect			fRect;
	} VALUE;

		VString					fString;
		VTime					fTime;
		VectorOfSLONG			fVectorOfSLong;

		eType					fType;
		VALUE					fValue;
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
		VRefPtr<VDocNode>		fDocNode;
};


/** document node class */
class 	IDocProperty;
class 	VDocText;
class 	VDocImage;
class 	VDocParagraph;
class	VDocNodeChildrenIterator;
class	VDocNodeChildrenReverseIterator;

/** parser handler 
@remarks
	it is used to apply CSS style sheet on nodes while parsing
*/ 
class XTOOLBOX_API IDocNodeParserHandler
{	
public:
	virtual	void IDNPH_SetAttribute(const VString& inName, const VString& inValue) = 0;
};


class VDocClassStyleManager;

class XTOOLBOX_API VDocNode : public VObject, public IRefCountable
{
public:
friend	class IDocProperty;
friend	class VDocImage;
friend	class VDocParagraph;
friend	class VDocText;
friend	class VDocNodeChildrenIterator;
friend	class VDocNodeChildrenReverseIterator;
friend	class VDocClassStyleManager;

	/** map of properties */
	typedef std::map<uLONG, VDocProperty>		MapOfProp;

	/** vector of nodes */
	typedef std::vector< VRefPtr<VDocNode> >	VectorOfNode;

	typedef std::vector< VRefPtr<VDocImage> >	VectorOfImage;

static	void					Init();
static	void					DeInit();

								VDocNode():VObject(),IRefCountable() 
								{ 
									fType = kDOC_NODE_TYPE_UNKNOWN; 
									fParent = NULL; 
									fDoc = NULL; 
									if (!sPropInitDone) 
										Init();
									fID = VString(D4ID_PREFIX_RESERVED)+"1";
									fDirtyStamp = 0;
									fTextStart = 0;
									fTextLength = 1;
									fChildIndex = 0;
									fParserHandler = NULL;
									fNoDeltaTextLength = false;
								}

virtual							~VDocNode() {}

		eDocNodeType			GetType() const { return fType; }

		/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
virtual	VDocNode*				RetainNode(const VString& inID)
		{
			if (fDoc)
				return fDoc->RetainNode( inID);
			else
				return NULL;
		}

		/** retain document node */
		VDocText*				RetainDocumentNode();

		/** get document node */
		VDocText*				GetDocumentNode() const;

		/** create JSONDocProps object
		@remarks
			should be called only if needed (for instance if caller need com with 4D language through C_OBJECT)

			caller might also get/set properties using the JSON object returned by RetainJSONProps
		*/
		VJSONDocProps*			CreateAndRetainJSONProps();

		/** return parent node */
		VDocNode*				GetParent() const { return fParent; }

		/** insert child node at passed position (0-based) */
virtual	void					InsertChild( VDocNode *inNode, VIndex inIndex);

		/** add child node */
virtual	void					AddChild( VDocNode *inNode);

		/** detach child node from parent */
		void					Detach();

		/** remove Nth child node (0-based index) */
		void					RemoveNthChild( VIndex inIndex)
		{
			xbox_assert(inIndex >= 0 && inIndex < fChildren.size());

			VRefPtr<VDocNode> nodeToRemove = fChildren[inIndex];
			nodeToRemove->Detach(); //is detached from this node (removed from fChildren) so maybe destroyed on nodeToRemove release
		}

		VIndex					GetChildIndex() const { return fChildIndex; }

		/** get children count */
		VIndex					GetChildCount() const { return (VIndex)fChildren.size(); }

		/** get Nth child node (0-based index) */
		VDocNode*				GetChild(VIndex inIndex) const
		{
			if (testAssert(inIndex >= 0 && inIndex < fChildren.size())) 
				return fChildren[inIndex].Get();  
			else
				return NULL;
		}

		/** retain Nth child node (0-based index) */
		VDocNode*				RetainChild(VIndex inIndex)
		{
			if (testAssert(inIndex >= 0 && inIndex < fChildren.size())) 
				return fChildren[inIndex].Retain();  
			else
				return NULL;
		}

		/** return true if it is the last child node
		@param inLoopUp
			true: return true if and only if it is the last child node at this level and up to the top-level parent
			false: return true if it is the last child node relatively to the node parent
		*/
		bool					IsLastChild(bool inLookUp = false) const;

		/** get current ID */
		const VString&			GetID() const { return fID; }

		/** set current ID (custom ID) */
		bool					SetID(const VString& inValue);
		
		/** get class*/
		const VString&			GetClass() const { return fClass; }

		/** set class */
virtual	void					SetClass( const VString& inClass) { fClass = inClass; fDirtyStamp++; }
		
		/** get associated html element name 
		@remarks
			it is used only to apply CSS rules
		*/
virtual	const VString&			GetElementName() const { return GetElementName(fType); }

static	const VString&			GetElementName(eDocNodeType inType)  { return sElementName[inType]; }

		/** get text start offset 
		@remarks
			it is text offset relative to parent node text offset (0-based)
		*/
		uLONG					GetTextStart() const { return fTextStart; }

		/** get text length 
		@remarks
			text length is 
			paragraph text length for paragraph,
			1 for image,
			otherwise sum of child nodes text length 
		*/
virtual	uLONG					GetTextLength() const { return fTextLength; }

		/** get plain text on the specified range (relative to node start offset) */
virtual	void					GetText(VString& outPlainText, sLONG inStart = 0, sLONG inEnd = -1) const;
								
		/** retain styles on the specified range (relative to node start offset) 
		@remarks
			return styles with range relative to node start offset 
		*/
virtual	VTreeTextStyle*			RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inNULLIfRangeEmpty = true, bool inNoMerge = false) const;

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocParagraph*			RetainFirstParagraph(sLONG inStart = 0); 

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	const VDocParagraph*	GetFirstParagraph(sLONG inStart = 0) const; 

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocNode*				RetainFirstChild(sLONG inStart = 0) const; 

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocNode*				GetFirstChild(sLONG inStart = 0) const; 

		/** replace styled text at the specified range (relative to node start offset) with new text & styles
		@remarks
			replaced text will inherit only uniform style on replaced range - if inInheritUniformStyle == true (default)

			return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
		*/
virtual	VIndex					ReplaceStyledText(	const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inStart = 0, sLONG inEnd = -1, 
													bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, const VDocSpanTextRef * inPreserveSpanTextRef = NULL);

		/** replace plain text with computed or source span references 
		@param inShowSpanRefs
			span references text mask (default is computed values)
		*/
virtual	void					UpdateSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs = kSRCTT_Value, VDBLanguageContext *inLC = NULL);

		/** replace text with span text reference on the specified range (relative to node start offset)
		@remarks
			span ref plain text is set here to uniform non-breaking space: 
			user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

			you should no more use or destroy passed inSpanRef even if method returns false
		*/
virtual	bool					ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inNoUpdateRef = true, VDBLanguageContext *inLC = NULL);

		/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	(relative to node start offset)

			return true if any 4D expression has been replaced with plain text
		*/
virtual	bool					FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1);

		/** return the first span reference which intersects the passed range (relative to node start offset) */
virtual	const VTextStyle*		GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outStartBase = NULL);

		/** set plain text and styles 
		@remarks
			replace node text & styles 

			 styles range is relative to node start offset

			 OBSOLETE: you should use now ReplaceStyledText (method is preserved for v14 compatibility)
		*/
virtual	bool					SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

		/** set character styles
		@remarks
			replace character styles if node is a text node
			otherwise replace first child text node character styles 
			(it is a full replacement so previous uniform style is not preseved as with ReplaceStyledText 
			 - use it only to initialize)

			 styles range is relative to node start offset
			 styles range must not exceed text node range otherwise it will fail
			 
			 OBSOLETE: you should use now ReplaceStyledText (method is preserved for v14 compatibility)
		*/
virtual	void					SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);


		/** dirty stamp counter accessor */
		uLONG					GetDirtyStamp() const { return fDirtyStamp; }

		/** append properties from the passed node
		@remarks
			if inNode type is not equal to the current node type, recurse down and append inNode properties to any child node with the same type as inNode:
			in that case, only children nodes which intersect the passed range are modified; 

			if current node type is class style (VDocClassStyle instance), append inNode properties no matter inNode type
			if curent node type is not class style but inNode type is class style (VDocClassStyle instance), update current node or children nodes with type = inClassStyleTarget;

			if inNode type is equal to the current node type, apply inNode properties to the current node only (passed range is ignored)

			if inOnlyIfNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties
		*/
virtual	void					AppendPropsFrom(	const VDocNode *inNode, bool inOnlyIfNotOverriden = false, bool inNoAppendSpanStyles = false, 
													sLONG inStart = 0, sLONG inEnd = -1, 
													uLONG inClassStyleTarget = kDOC_NODE_TYPE_PARAGRAPH, bool inClassStyleOverrideOnlyUniformCharStyles = true, 
													bool inOnlyIfNotOverridenRecurseUp = false); 

		/** return true if at least one property is overriden locally */
		bool					HasProps() const { return fProps.size() > 0; }	

		/** force a property to be inherited */
		void					SetInherit( const eDocProperty inProp);

		/** return true if property value is inherited */ 
		bool					IsInherited( const eDocProperty inProp) const;

		/** return true if property value is inherited from default 
		@remarks
			return true if value is inherited from the default value (false if this node or parent node value is overriden but not inherited) 
		*/ 
		bool					IsInheritedDefault( const eDocProperty inProp) const;

		/** return true if property value is overriden locally 
			(if also IsInherited() == true, it is a force inherit (that is CSS value = "inherit"), otherwise it is a value override) */ 
		bool					IsOverriden( const eDocProperty inProp, bool inRecurseUp = false) const;

		const VDocProperty&		GetProperty(const eDocProperty inProp) const;

		const VDocProperty&		GetPropertyInherited(const eDocProperty inProp) const;

		/** remove local property */
		void					RemoveProp( const eDocProperty inProp);

		/** remove all properties (but id and class) */
		void					ClearAllProps();

		/** return true if property needs to be serialized 
		@remarks
			it is used by serializer to serialize only properties which are not equal to the inherited value
			or to the default value for not inherited properties
		*/
		bool					ShouldSerializeProp( const eDocProperty inProp, const VDocNode *inDefault  = NULL) const;

		/** get default property value */
static	const VDocProperty&		GetDefaultPropValue( const eDocProperty inProp);


		//document properties

		uLONG					GetVersion() const;
		void					SetVersion(const uLONG inValue);

		bool					GetWidowsAndOrphans() const;
		void					SetWidowsAndOrphans(const bool inValue);

		uLONG					GetDPI() const;
		void					SetDPI(const uLONG inValue);

		
		/** get zoom percentage 
		@remarks
			100 -> 100%
			250 -> 250%
		*/
		uLONG					GetZoom() const;
		void					SetZoom(const uLONG inValue);

		eDocPropLayoutPageMode	GetLayoutMode() const;
		void					SetLayoutMode(const eDocPropLayoutPageMode inValue);

		bool					ShouldShowImages() const;
		void					ShouldShowImages(const bool inValue);

		bool					ShouldShowReferences() const;
		void					ShouldShowReferences(const bool inValue);

		bool					ShouldShowHiddenCharacters() const;
		void					ShouldShowHiddenCharacters(const bool inValue);

		bool					GetDWrite() const;
		void					SetDWrite(const bool inValue);

		const VTime&			GetDateTimeCreation() const;
		void					SetDateTimeCreation(const VTime& inValue);

		const VTime&			GetDateTimeModified() const;
		void					SetDateTimeModified(const VTime& inValue);

		//const VString&		GetTitle() const;
		//void					SetTitle(const VString& inValue);

		//const VString&		GetSubject() const;
		//void					SetSubject(const VString& inValue);

		//const VString&		GetAuthor() const;
		//void					SetAuthor(const VString& inValue);

		//const VString&		GetCompany() const;
		//void					SetCompany(const VString& inValue);

		//const VString&		GetNotes() const;
		//void					SetNotes(const VString& inValue);


		//common element properties


		DialectCode				GetLang() const;
		const VString&			GetLangBCP47() const;

		void					SetLang(const DialectCode inValue);

		/** get vertical align */ 
		eDocPropTextAlign		GetVerticalAlign() const; 
		/** set vertical align */ 
		void					SetVerticalAlign(const eDocPropTextAlign inValue);

		/** get margins (in TWIPS) */
		const sDocPropRect&		GetMargin() const;
		/** set margins (in TWIPS) */
		void					SetMargin(const sDocPropRect& inValue);

		/** get padding (in TWIPS) */
		const sDocPropRect&		GetPadding() const;
		/** set padding (in TWIPS) */
		void					SetPadding(const sDocPropRect& inValue);

		/** get background color */
		RGBAColor				GetBackgroundColor() const;
		/** set background color */
		void					SetBackgroundColor(const RGBAColor inValue);

		/** get background clip box (used by both background image & color) */
		eDocPropBackgroundBox	GetBackgroundClip() const;
		/** set background clip box (used by both background image & color) */
		void					SetBackgroundClip(const eDocPropBackgroundBox inValue);

		/** get background image url */
		const VString&			GetBackgroundImage() const;
		/** set background image url */
		void					SetBackgroundImage(const VString& inValue);

		/** get background image horizontal position (alignment) */
		eDocPropTextAlign 		GetBackgroundImageHAlign() const;
		/** set background image horizontal position (alignment) */
		void					SetBackgroundImageHAlign(const eDocPropTextAlign inValue);

		/** get background image vertical position (alignment) */
		eDocPropTextAlign 		GetBackgroundImageVAlign() const;
		/** get background image vertical position (alignment) */
		void					SetBackgroundImageVAlign(const eDocPropTextAlign inValue);

		/** get background image repeat mode */
		eDocPropBackgroundRepeat	GetBackgroundImageRepeat() const;
		/** set background image repeat mode */
		void						SetBackgroundImageRepeat(const eDocPropBackgroundRepeat inValue);

		/** get background image origin box */
		eDocPropBackgroundBox	GetBackgroundImageOrigin() const;
		/** set background image origin box */
		void					SetBackgroundImageOrigin(const eDocPropBackgroundBox inValue);

		/** get background image width
		@remark
			background width is sLONG;
			here is the mapping between CSS values (on the left) & doc property value (on the right):

			length 				length in TWIPS (if length > 0 - 0 is assumed to be auto)
			percentage			[1%;1000%] is mapped to [-1;-1000] (so 200% is mapped to -200) - overflowed percentage is bound to the [1%;1000%]
			auto				kDOC_BACKGROUND_SIZE_AUTO
			cover				kDOC_BACKGROUND_SIZE_COVER
			contain				kDOC_BACKGROUND_SIZE_CONTAIN
		*/
		sLONG					GetBackgroundImageWidth() const;
		/** set background image width
		@remark
			see GetBackgroundImageWidth
		*/
		void					SetBackgroundImageWidth(const sLONG inValue);

		/** get background image height
		@remark
			background height is sLONG;
			here is the mapping between CSS values (on the left) & doc property value (on the right):

			length 				length in TWIPS (if length > 0 - 0 is assumed to be auto)
			percentage			[1%;1000%] is mapped to [-1;-1000] (so 200% is mapped to -200) - overflowed percentage is bound to the [1%;1000%]
			auto				kDOC_BACKGROUND_SIZE_AUTO
			cover				kDOC_BACKGROUND_SIZE_COVER
			contain				kDOC_BACKGROUND_SIZE_CONTAIN
		*/
		sLONG					GetBackgroundImageHeight() const;

		/** set background image height
		@remark
			see GetBackgroundImageHeight
		*/
		void					SetBackgroundImageHeight(const sLONG inValue);

		/** get width (in TWIPS) - 0 for auto */
		uLONG					GetWidth() const;
		/** set width (in TWIPS) - 0 for auto */
		void					SetWidth(const uLONG inValue);

		/** get height (in TWIPS) - 0 for auto */
		uLONG					GetHeight() const;
		/** set height (in TWIPS) - 0 for auto */
		void					SetHeight(const uLONG inValue);

		/** get min width (in TWIPS) */
		uLONG					GetMinWidth() const;
		/** set min width (in TWIPS) */
		void					SetMinWidth(const uLONG inValue);

		/** get min height (in TWIPS) */
		uLONG					GetMinHeight() const;
		/** set min height (in TWIPS) */
		void					SetMinHeight(const uLONG inValue);

		/** get border styles (values from eDocPropBorderStyle) */
		const sDocPropRect&		GetBorderStyle() const;
		/** set border styles (values from eDocPropBorderStyle) */
		void					SetBorderStyle(const sDocPropRect& inValue);

		/** get border widths (values in TWIPS) */
		const sDocPropRect&		GetBorderWidth() const;
		/** set border widths (values in TWIPS) */
		void					SetBorderWidth(const sDocPropRect& inValue);

		/** get border colors (RGBAColor values) */
		const sDocPropRect&		GetBorderColor() const;
		/** set border colors (RGBAColor values) */
		void					SetBorderColor(const sDocPropRect& inValue);

		/** get border radius (values in TWIPS) */
		const sDocPropRect&		GetBorderRadius() const;
		/** set border radius (values in TWIPS) */
		void					SetBorderRadius(const sDocPropRect& inValue);


		//paragraph properties 


		eDocPropDirection		GetDirection() const; 
		void					SetDirection(const eDocPropDirection inValue);

		/** get line height 
		@remarks
			if positive, it is fixed line height in TWIPS
			if negative, abs(value) is equal to percentage of current font size (so -250 means 250% of current font size)
			if kDOC_PROP_LINE_HEIGHT_NORMAL, it is normal line height (normally same as -100)

			(default is kDOC_PROP_LINE_HEIGHT_NORMAL that is normal)
		*/
		sLONG					GetLineHeight() const;
		
		/** set line height 
		@remarks
			if positive, it is fixed line height in TWIPS
			if negative, abs(value) is equal to percentage of current font size (so -250 means 250% of current font size)
			if kDOC_PROP_LINE_HEIGHT_NORMAL, it is normal line height (normally same as -100)

			(default is kDOC_PROP_LINE_HEIGHT_NORMAL that is normal)
		*/
		void					SetLineHeight(const sLONG inValue);

		/** get text align */ 
		eDocPropTextAlign		GetTextAlign() const; 
		/** set text align */ 
		void					SetTextAlign(const eDocPropTextAlign inValue);

		/** get first line padding offset in TWIPS 
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
		sLONG					GetTextIndent() const;

		/** set first line padding offset in TWIPS 
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
		void					SetTextIndent(const sLONG inValue);

		/** get tab stop offset in TWIPS */
		uLONG					GetTabStopOffset() const;

		/** set tab stop offset in TWIPS */
		void					SetTabStopOffset(const uLONG inValue);

		eDocPropTabStopType		GetTabStopType() const; 
		void					SetTabStopType(const eDocPropTabStopType inValue);

		const VString&			GetNewParaClass() const;
		void					SetNewParaClass( const VString& inClass);

		/** get list style type */
		eDocPropListStyleType	GetListStyleType() const;

		/** set list style type */
		void					SetListStyleType(const eDocPropListStyleType inValue);

		/** get list string format 
		@remarks
			ordered lists: left to right string format (on default "#." for decimal so "1.", "2." etc...; # is a placeholder for the ordered string value)
			unordered lists: it is equal on default to a single character per list style type - for instance the single disc character for disc list style type
		*/
		const VString&			GetListStringFormatLTR() const;
		
		/** set list string format 
		@remarks
			ordered lists: left to right string format (on default "#." for decimal so "1.", "2." etc...; # is a placeholder for the ordered string value)
			unordered lists: it is equal on default to a single character per list style type - for instance the single disc character for disc list style type
		*/
		void					SetListStringFormatLTR(const VString& inValue);
		
		/** get list string format 
		@remarks
			ordered lists: right to left string format (on default ".#" for decimal so ".1", ".2" etc...; # is a placeholder for the ordered string value) 
			unordered lists: it is equal on default to a single character per list style type - for instance the single disc character for disc list style type
			
			if empty, it is same as ltr
		*/
		const VString&			GetListStringFormatRTL() const;

		/** set list string format 
		@remarks
			ordered lists: right to left string format (on default ".#" for decimal so ".1", ".2" etc...; # is a placeholder for the ordered string value) 
			unordered lists: it is equal on default to a single character per list style type - for instance the single disc character for disc list style type
			
			if empty, it is same as ltr
		*/
		void					SetListStringFormatRTL(const VString& inValue);

		/** get list font family 
		@remarks
			default is "Symbol"
		*/
		const VString&			GetListFontFamily() const;

		/** set list font family 
		@remarks
			default is "Symbol"
		*/
		void					SetListFontFamily(const VString& inValue);

		/** get list image url 
		@remarks
			it overrides list style type
		*/
		const VString&			GetListStyleImage() const;

		/** set list image url 
		@remarks
			it overrides list style type
		*/
		void					SetListStyleImage(const VString& inValue);

		/** get list style image height in TWIPS 
		@remarks
			width is assumed to be automatic (proportional to height according to image aspect ratio)

			on default it is automatic (equal to image height in TWIPS)
		*/
		uLONG					GetListStyleImageHeight() const;

		/** set list style image height in TWIPS 
		@remarks
			width is assumed to be automatic (proportional to height according to image aspect ratio)

			on default it is automatic (equal to image height in TWIPS)
		*/
		void					SetListStyleImageHeight(const uLONG inValue);

		/** get list start number 
		@remarks
			default is 1 or previous paragraph - with same class & list style type - number+1
		*/
		sLONG					GetListStart() const;
		void					SetListStart(const sLONG inValue);

		/** return default ltr string format for the passed list style type 
		@remarks
			method should be called to get default string format if property is not overriden 
		*/
static	const VString&			GetDefaultListStringFormatLTR(  eDocPropListStyleType inListStyleType);
		
		/** return default rtl string format for the passed list style type 
		@remarks
			method should be called to get default string format if property is not overriden 
		*/
static	const VString&			GetDefaultListStringFormatRTL(  eDocPropListStyleType inListStyleType);

		/** get list item string value from list style type and list item number */
static	void					GetListItemValue(	eDocPropListStyleType inListStyleType, VString& outValue, sLONG inNumber = 1, bool inRTL = false, 
													const VString* inCustomStringFormatLTR = NULL, 
													const VString* inCustomStringFormatRTL = NULL);

		/** return default list font family for the passed list style type */
static	const VString&			GetDefaultListFontFamily( eDocPropListStyleType inListStyleType);

		//span (character only) properties


		const VString&			GetFontFamily() const;
		void					SetFontFamily( const VString& inName);

		Real					GetFontSize() const;
		void					SetFontSize( const Real inSize);

		RGBAColor				GetColor() const;
		void					SetColor( const RGBAColor inColor);

		eFontStyle				GetFontStyle() const;
		void					SetFontStyle( const eFontStyle inStyle);
		
		eFontWeight				GetFontWeight() const;
		void					SetFontWeight( const eFontWeight inWeight);

		uLONG					GetTextDecoration() const;
		void					SetTextDecoration( const uLONG inDeco);

		RGBAColor				GetTextBackgroundColor() const;
		void					SetTextBackgroundColor( const RGBAColor inColor);
		


		void					SetParserHandler( IDocNodeParserHandler* inHandler) { fParserHandler = inHandler; }
		IDocNodeParserHandler*	GetParserHandler() const { return fParserHandler; }

		/** set VTextStyle styles from text character properties */
		void					CharPropsCopyToVTextStyle( VTextStyle& outStyle, bool inOverrideStyles = true, bool inInheritedDefault = false) const;

		/** set VTextStyle styles from text character properties & remove text character properties */
		void					CharPropsMoveToVTextStyle( VTextStyle& outStyle, bool inOverrideStyles = true, bool inInheritedDefault = false);

		/** set text character properties from VTextStyle */
		void					VTextStyleCopyToCharProps( const VTextStyle& inStyle, bool inOverrideProps = true);

		/** remove text character properties */
		void					RemoveTextCharacterProperties();

		void					GetAllMargins(uLONG& outMarginWidth, uLONG& outMarginHeight) const;

		void					AttachToDocument( VDocNode *inDocumentNode) { _OnAttachToDocument(inDocumentNode); }
		void					DetachFromDocument( VDocNode *inDocumentNode) { _OnDetachFromDocument(inDocumentNode); }
protected:

virtual	VDocNode*				Clone() const {	return (new VDocNode( this)); }
								VDocNode( const VDocNode* inNode);

		void					_GenerateID(VString& outID);
virtual	void					_OnAttachToDocument(VDocNode *inDocumentNode);
virtual	void					_OnDetachFromDocument(VDocNode *inDocumentNode);
	
		void					_NormalizeRange(sLONG& ioStart, sLONG& ioEnd) const;

virtual	void					_UpdateTextInfo(bool inUpdateTextLength = true, bool inApplyOnlyDeltaTextLength = false, sLONG inDeltaTextLength = 0);

		void					_UpdateTextInfoEx(bool inUpdateNextSiblings = false, bool inUpdateTextLength = true, bool inApplyOnlyDeltaTextLength = false, sLONG inDeltaTextLength = 0);

virtual	void					_UpdateListNumber(eListStyleTypeEvent inListEvent = kDOC_LSTE_AFTER_ADD_TO_LIST) {}

		/** return child node at the passed text position (text position is 0-based and relative to node start offset) */
		VDocNode*				_GetChildNodeFromPos(const sLONG inTextPos) const;
		/** return next sibling */
		VDocNode*				_GetNextSibling() const;

		/** node type */
		eDocNodeType			fType;

		/** node ID (not in fProps for faster access) */
		VString					fID;

		/** node class (not in fProps for faster access) */
		VString					fClass;

		/** BCP-47 language string 
		@remarks
			it is associated with kDOC_PROP_LANG
		*/ 
mutable	VString					fLang;

		/** document node */
		VDocNode*				fDoc;

		/** parent node */
		VDocNode*				fParent;

		/** locally defined properties */
		MapOfProp				fProps;
		
		/** children nodes */
		VectorOfNode			fChildren;

		/** dirty stamp counter */
		uLONG					fDirtyStamp;

		/** child index (0-based) */
		uLONG					fChildIndex;

		/** text start offset (0-based) - relative to parent text start offset */
		uLONG					fTextStart;
		
		/** text length */
		uLONG					fTextLength;

		IDocNodeParserHandler*	fParserHandler;

static	bool					sPropInitDone;

		/** map of inherited properties */
static	bool					sPropInherited[kDOC_PROP_COUNT];

		/** map of default values */
static	VDocProperty*			sPropDefault[kDOC_PROP_COUNT];

		/** map of element name per document node type */
static	VString					sElementName[kDOC_NODE_TYPE_COUNT];

static	VString					sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_LAST+1];
static	VString					sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_LAST+1];

static	VString					sListFontFamily[kDOC_LIST_STYLE_TYPE_LAST+1];

static	VString					sLowerRomanStrings[10];
static	VString					sLowerRomanStrings10[10];
static	VString					sLowerRomanStrings100[10];

static	VString					sUpperRomanStrings[10];
static	VString					sUpperRomanStrings10[10];
static	VString					sUpperRomanStrings100[10];

static	UniChar					sLowerGreekUniChar[24];

static	UniChar					sDecimalGreekUniChar[10];
static	UniChar					sDecimalGreekUniChar10[10];
static	UniChar					sDecimalGreekUniChar100[10];

static	UniChar					sHebrewUniChar[10];
static	UniChar					sHebrewUniChar10[10];
static	UniChar					sHebrewUniChar100[10];

static	UniChar					sGeorgianUniChar[10];
static	UniChar					sGeorgianUniChar10[10];
static	UniChar					sGeorgianUniChar100[10];
static	UniChar					sGeorgianUniChar1000[10];

static	UniChar					sArmenianUniChar[10];
static	UniChar					sArmenianUniChar10[10];
static	UniChar					sArmenianUniChar100[10];
static	UniChar					sArmenianUniChar1000[10];

static	UniChar					sCJK[10];

static	UniChar					sHiragana[49];

mutable bool					fNoDeltaTextLength;
};


/** class style 
@remarks
	storage class for parsed CSS rules 'elemname.classname' (only property bag is mandatory here)
*/
class XTOOLBOX_API VDocClassStyle : public VDocNode
{
public:
								VDocClassStyle():VDocNode() { fType = kDOC_NODE_TYPE_CLASS_STYLE; }
								VDocClassStyle(const VDocClassStyle *inNode):VDocNode(static_cast<const VDocNode *>(inNode)) { fType = kDOC_NODE_TYPE_CLASS_STYLE; fElementName = inNode->fElementName; }

virtual							~VDocClassStyle() {}

virtual VDocNode*				Clone() const {	return static_cast<VDocNode *>(new VDocClassStyle(this)); }

		/** set element name
		@remarks
			it is used only for CSS rules matching (in order a CSS rule element selector to match a VDocClassStyle instance)
		*/
		void					SetElementName( const VString& inElementName) { fElementName = inElementName; }
		void					SetElementName( eDocNodeType inDocType)	{ fElementName = VDocNode::GetElementName( inDocType); }

		/** get associated html element name 
		@remarks
			it is used only to apply CSS rules
		*/
virtual	const VString&			GetElementName() const { return fElementName; }

		bool					operator == (const VDocClassStyle& inStyle);
		bool					operator != (const VDocClassStyle& inStyle) { return !(*this == inStyle); }
protected:
		VString					fElementName;
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
								VDocText(const VDocClassStyleManager *inCSMgr = NULL); 
virtual							~VDocText() 
		{
		}

virtual VDocNode*				Clone() const {	return static_cast<VDocNode *>(new VDocText(this)); }

		/** retain node which matches the passed ID 
		@remarks
			node might not have a parent if it is not a VDocText child node 
			but it can still be attached to the document: 
			for instance embedded images or paragraph styles do not have a parent
			but are attached to the document and so can be searched by ID
		*/
		VDocNode*				RetainNode(const VString& inID);

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//
		//	following methods are helper methods to set/get document text & character styles & span references 
		//	(for setting/getting document or paragraph properties, use property accessors)
		//
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/** set plain text and styles 
		@remarks
			replace text & styles on the specified range

			styles range is relative to node start offset

			OBSOLETE: you should use now ReplaceStyledText (method is preserved for v14 compatibility)
		*/
virtual	bool					SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);
		/** get plain text on the specified range (relative to node start offset) */
virtual	void					GetText(VString& outPlainText, sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set character styles
		@remarks
			replace character styles if node is a text node
			otherwise replace first child text node character styles 
			(it is a full replacement so previous uniform style is not preserved as with ReplaceStyledText 
			 - use it only to initialize)

			 styles range is relative to node start offset
			 styles range must not exceed text node range otherwise it will fail
			 
			 OBSOLETE: you should use now ReplaceStyledText (method is preserved for v14 compatibility)
		*/
virtual	void					SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);
		
		/** retain styles on the specified range (relative to node start offset) 
		@remarks
			return styles with range relative to node start offset 
		*/
virtual	VTreeTextStyle*			RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inNULLIfRangeEmpty = true, bool inNoMerge = false) const;

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocParagraph*			RetainFirstParagraph(sLONG inStart = 0); 

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	const VDocParagraph*	GetFirstParagraph(sLONG inStart = 0) const; 

		/** replace styled text at the specified range (relative to node start offset) with new text & styles
		@remarks
			replaced text will inherit only uniform style on replaced range - if inInheritUniformStyle == true (default)

			return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
		*/
virtual	VIndex					ReplaceStyledText(	const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inStart = 0, sLONG inEnd = -1, 
													bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, const VDocSpanTextRef * inPreserveSpanTextRef = NULL);

		/** replace plain text with computed or source span references 
		@param inShowSpanRefs
			span references text mask (default is computed values)
		*/
virtual	void					UpdateSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs = kSRCTT_Value, VDBLanguageContext *inLC = NULL);

		/** replace text with span text reference on the specified range
		@remarks
			span ref plain text is set here to uniform non-breaking space: 
			user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

			you should no more use or destroy passed inSpanRef even if method returns false
		*/
virtual	bool					ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inNoUpdateRef = true, VDBLanguageContext *inLC = NULL);

		/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	 

			return true if any 4D expression has been replaced with plain text
		*/
virtual	bool					FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1);

		/** return the first span reference which intersects the passed range */
virtual	const VTextStyle*		GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outStartBase = NULL);

		/** add image */
		void					AddImage( VDocImage *image);

		/** remove image */
		void					RemoveImage( VDocImage *image);

		/** accessor on class style manager */
		VDocClassStyleManager*	GetClassStyleManager() const { return fClassStyleManager.Get(); }

protected:
typedef	std::map<VString, VRefPtr<VDocNode> >			MapOfNodePerID;

friend	class					VDocNode;

								VDocText( const VDocText* inNodeText);

		uLONG					_AllocID() { return fNextIDCount++; }

		/** document next ID number 
		@remarks
			it is impossible for one document to have more than kMAX_uLONG nodes
			so incrementing ID is enough to ensure ID is unique
			(if a node is added to a parent node, ID is checked & generated again if node ID is used yet)
		*/ 
		uLONG					fNextIDCount;

mutable	MapOfNodePerID			fMapOfNodePerID;


		/** vector of images (embedded or background images) */
		VectorOfImage			fImages;

		VRefPtr<VDocClassStyleManager>	fClassStyleManager;
};

class XTOOLBOX_API VDocClassStyleManager : public VObject, public IRefCountable
{
public:
typedef std::map<VString, VRefPtr<VDocClassStyle> >		MapOfStylePerClass;
typedef std::map<eDocNodeType, MapOfStylePerClass>		MapOfClassStylePerType;

		VDocClassStyleManager(VDocText *inDocument = NULL):VObject(),IRefCountable() { fDoc = inDocument; }
		VDocClassStyleManager(const VDocClassStyleManager& inManager, VDocText *inDocument = NULL);

		virtual	~VDocClassStyleManager() {}

		/** add or replace a class style */
		void					AddOrReplaceClassStyle(eDocNodeType inType, const VString& inClass, VDocClassStyle *inNode, bool inRemoveDefaultStyles = false);

		/** retain class style */
		VDocClassStyle*			RetainClassStyle(eDocNodeType inType, const VString& inClass, bool inAppendDefaultStyles = false) const;

		/** retain first class style for the passed document type */
		VDocClassStyle*			RetainFirstClassStyle(eDocNodeType inType, bool inAppendDefaultStyles = false) const
		{
			if (GetClassStyleCount( inType) > 0)
				return RetainClassStyle( inType, 0);
			else
				return NULL;
		}

		/** retain class style */
		VDocClassStyle*			RetainClassStyle(eDocNodeType inType, VIndex inIndex, VString* outClass = NULL, bool inAppendDefaultStyles = false) const;

		/** remove class style  */
		void					RemoveClassStyle(eDocNodeType inType, const VString& inClass);
		
		VIndex					GetClassStyleCount(eDocNodeType inType) const;

		/** append properties from the passed node to class styles for the passed document type
		@remarks
			if inOnlyIfNotOverriden == true, class styles properties which are defined and not inherited are not overriden by inNode properties
		*/
		void					AppendPropsToClassStylesFrom( eDocNodeType inType, const VDocNode *inNode, bool inOnlyIfNotOverriden = false, bool inNoAppendSpanStyles = false); 

		/** detach from owner document & clear owner document reference */
		void					DetachFromOwnerDocument();

		/** remove all class styles 
		@param inKeepDefaultStyles
			if true (default is false), remove all class styles but default styles (i.e. class styles with empty class name)	
		*/
		void					Clear(bool inKeepDefaultStyles = false);
protected:
		VDocClassStyle*			_RetainWithDefaultStyles( eDocNodeType inType, VDocClassStyle *inNode) const;

		/* owner document (might be NULL only if instance is not embedded in VDocText) 
		@remarks
			fDoc is not retained as fDoc retains VDocClassStyleManager instance
		*/
		VDocText*				fDoc;

		/** style sheet container 
		@remarks
			a style sheet is a VDocNode instance indexed by class & doc type
		*/
		MapOfClassStylePerType	fClassStyles; 
}; 

class IDocProperty
{
public:
/** property accessors */
template <class Type>
static	Type					GetProperty( const VDocNode *inNode, const eDocProperty inProperty);

template <class Type>
static	const Type&				GetPropertyRef( const VDocNode *inNode, const eDocProperty inProperty);

template <class Type>
static	void					SetProperty( VDocNode *inNode, const eDocProperty inProperty, const Type inValue);

template <class Type>
static	void					SetPropertyPerRef( VDocNode *inNode, const eDocProperty inProperty, const Type& inValue);

template <class Type>
static	Type					GetPropertyDefault( const eDocProperty inProperty);

template <class Type>
static	const Type&				GetPropertyDefaultRef( const eDocProperty inProperty);
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

class XTOOLBOX_API VDocNodeChildrenIterator : public VObject
{
public:
		VDocNodeChildrenIterator( const VDocNode *inNode):VObject() { xbox_assert(inNode); fParent = inNode; fIterator = fParent->fChildren.end(); fEnd = -1; }
virtual	~VDocNodeChildrenIterator() {}

		VDocNode*				First(sLONG inTextStart = 0, sLONG inTextEnd = -1); //start iteration on the passed text range (full text range on default)
		VDocNode*				Next();
		VDocNode*				Current() {  if (fIterator != fParent->fChildren.end()) return (*fIterator).Get(); else return NULL; }
	
private:
		VDocNode::VectorOfNode::const_iterator fIterator;
		const VDocNode*			fParent;
		sLONG					fEnd;
};

class XTOOLBOX_API VDocNodeChildrenReverseIterator : public VObject
{
public:
		VDocNodeChildrenReverseIterator( const VDocNode *inNode):VObject() { xbox_assert(inNode); fParent = inNode; fIterator = fParent->fChildren.rend(); fStart = 0; }
virtual	~VDocNodeChildrenReverseIterator() {}

		VDocNode*				First(sLONG inTextStart = 0, sLONG inTextEnd = -1);
		VDocNode*				Next();
		VDocNode*				Current() {  if (fIterator != fParent->fChildren.rend()) return (*fIterator).Get(); else return NULL; }

private:
		VDocNode::VectorOfNode::const_reverse_iterator fIterator;
		const VDocNode*			fParent;
		sLONG					fStart;
};

END_TOOLBOX_NAMESPACE


#endif
