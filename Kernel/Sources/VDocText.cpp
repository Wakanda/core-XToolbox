
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

#include "VKernelPrecompiled.h"

#include "VString.h"
#include "VIntlMgr.h"
#include "VProcess.h"
#include "ILocalizer.h"

//in order to be able to use std::min && std::max
#undef max
#undef min

USING_TOOLBOX_NAMESPACE

bool VDocNode::sPropInitDone = false;


/** map of default inherited per CSS property
@remarks
	default inherited status is based on W3C CSS rules for regular CSS properties;
	4D CSS properties (prefixed with '-d4-') are NOT inherited on default
*/
bool VDocNode::sPropInherited[kDOC_PROP_COUNT] = 
{
	//document node properties

	false,	//kDOC_PROP_ID,
	false,	//kDOC_PROP_CLASS

	false,	//kDOC_PROP_VERSION,
	false,	//kDOC_PROP_WIDOWSANDORPHANS,
	false,	//kDOC_PROP_DPI,
	false,	//kDOC_PROP_ZOOM,
	false,	//kDOC_PROP_LAYOUT_MODE,
	false,	//kDOC_PROP_SHOW_IMAGES,
	false,	//kDOC_PROP_SHOW_REFERENCES,
	false,	//kDOC_PROP_SHOW_HIDDEN_CHARACTERS,
	true,	//kDOC_PROP_LANG,						//regular CSS 'lang(??)'
	false,	//kDOC_PROP_DWRITE,

	false,	//kDOC_PROP_DATETIMECREATION,
	false,	//kDOC_PROP_DATETIMEMODIFIED,

	//false,	//kDOC_PROP_TITLE,
	//false,	//kDOC_PROP_SUBJECT,
	//false,	//kDOC_PROP_AUTHOR,
	//false,	//kDOC_PROP_COMPANY,
	//false		//kDOC_PROP_NOTES,

	//node common properties

	true,		//kDOC_PROP_TEXT_ALIGN,				//regular CSS 'text-align'
	false,		//kDOC_PROP_VERTICAL_ALIGN,			//regular CSS 'vertical-align' (it should be used also for storing superscript & subscript character style)
	false,		//kDOC_PROP_MARGIN,					//regular CSS 'margin'
	false,		//kDOC_PROP_PADDING,				//regular CSS 'padding'
	false,		//kDOC_PROP_BACKGROUND_COLOR,		//regular CSS 'background-color' (for node but span it is element back color; for span node, it is character back color)
													//(in regular CSS, it is not inherited but it is if declared inline with style attribute for span tag)
	false,		//kDOC_PROP_BACKGROUND_IMAGE,		//regular CSS 'background-image'
	false,		//kDOC_PROP_BACKGROUND_POSITION,	//regular CSS 'background-position'
	false,		//kDOC_PROP_BACKGROUND_REPEAT,		//regular CSS 'background-repeat'
	false,		//kDOC_PROP_BACKGROUND_CLIP,		//regular CSS 'background-clip'
	false,		//kDOC_PROP_BACKGROUND_ORIGIN,		//regular CSS 'background-origin'
	false,		//kDOC_PROP_BACKGROUND_SIZE,		//regular CSS 'background-size'
	false,		//kDOC_PROP_WIDTH,					//regular CSS 'width'
	false,		//kDOC_PROP_HEIGHT,					//regular CSS 'height'
	false,		//kDOC_PROP_MIN_WIDTH,				//regular CSS 'min-width'
	false,		//kDOC_PROP_MIN_HEIGHT,				//regular CSS 'min-height'
	false,		//kDOC_PROP_BORDER_STYLE,			//regular CSS 'border-style'
	false,		//kDOC_PROP_BORDER_WIDTH,			//regular CSS 'border-width'
	false,		//kDOC_PROP_BORDER_COLOR,			//regular CSS 'border-color'
	false,		//kDOC_PROP_BORDER_RADIUS			//regular CSS 'border-radius'

	//paragraph node properties

	true,		//kDOC_PROP_DIRECTION,				//regular CSS 'direction'
	true,		//kDOC_PROP_LINE_HEIGHT,			//regular CSS 'line-height'
	true,		//kDOC_PROP_TEXT_INDENT,
	false,		//kDOC_PROP_TAB_STOP_OFFSET,
	false,		//kDOC_PROP_TAB_STOP_TYPE,
	false,		//kDOC_PROP_LIST_STYLE_TYPE,		//regular CSS 'list-style-type' & 4D '-d4-list-style-type'
	false,		//kDOC_PROP_LIST_STRING_FORMAT_LTR,	
	false,		//kDOC_PROP_LIST_STRING_FORMAT_RTL,
	false,		//kDOC_PROP_LIST_FONT_FAMILY,
	false,		//kDOC_PROP_LIST_STYLE_IMAGE,		//regular CSS 'list-style-image'
	false,		//kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT,
	false,		//kDOC_PROP_LIST_START				//regular html 'start' & 4D CSS '-d4-list-start'
	false,		//kDOC_PROP_NEW_PARA_CLASS

	//image node properties

	false,		//kDOC_PROP_IMG_SOURCE,				
	false,		//kDOC_PROP_IMG_ALT_TEXT,

	//span character styles

	true,		//kDOC_PROP_FONT_FAMILY,			
	true,		//kDOC_PROP_FONT_SIZE,				
	true,		//kDOC_PROP_COLOR,				
	true,		//kDOC_PROP_FONT_STYLE,		
	true,		//kDOC_PROP_FONT_WEIGHT,		
	false,		//kDOC_PROP_TEXT_DECORATION,		//in regular CSS, it is not inherited but it is if declared inline with style attribute for span tag
	false,		//kDOC_PROP_4DREF,					// '-d4-ref'
	false,		//kDOC_PROP_4DREF_USER,				// '-d4-ref-user'
	false		//kDOC_PROP_TEXT_BACKGROUND_COLOR   // '-d4-text-background-color' (it defines text background color but element background color)
};

/** map of default values */
VDocProperty *VDocNode::sPropDefault[kDOC_PROP_COUNT] = {0};

/** map of element name per document node type */
VString	VDocNode::sElementName[kDOC_NODE_TYPE_COUNT]; 

VString	VDocNode::sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_LAST+1];
VString	VDocNode::sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_LAST+1];

VString	VDocNode::sListFontFamily[kDOC_LIST_STYLE_TYPE_LAST+1];

VString	VDocNode::sLowerRomanStrings[10];
VString	VDocNode::sUpperRomanStrings[10];

VString	VDocNode::sLowerRomanStrings10[10];
VString	VDocNode::sUpperRomanStrings10[10];

VString	VDocNode::sLowerRomanStrings100[10];
VString	VDocNode::sUpperRomanStrings100[10];

UniChar	VDocNode::sDecimalGreekUniChar[10] = { '0' };
UniChar	VDocNode::sDecimalGreekUniChar10[10] = { '0' };
UniChar	VDocNode::sDecimalGreekUniChar100[10] = { '0' };

UniChar	VDocNode::sLowerGreekUniChar[24] = { '0' };

UniChar	VDocNode::sHebrewUniChar[10] = { '0' };
UniChar	VDocNode::sHebrewUniChar10[10] = { '0' };
UniChar	VDocNode::sHebrewUniChar100[10] = { '0' };

UniChar	VDocNode::sGeorgianUniChar[10] = { '0' };
UniChar	VDocNode::sGeorgianUniChar10[10] = { '0' };
UniChar	VDocNode::sGeorgianUniChar100[10] = { '0' };
UniChar	VDocNode::sGeorgianUniChar1000[10] = { '0' };

UniChar	VDocNode::sArmenianUniChar[10] = { '0' };
UniChar	VDocNode::sArmenianUniChar10[10] = { '0' };
UniChar	VDocNode::sArmenianUniChar100[10] = { '0' };
UniChar	VDocNode::sArmenianUniChar1000[10] = { '0' };

UniChar	VDocNode::sCJK[10] = { '0' };

UniChar	VDocNode::sHiragana[49] = { '0' };

VDocProperty& VDocProperty::operator = (const VDocProperty& inValue) 
{ 
	if (this == &inValue)
		return *this;

	fType = inValue.fType;
	switch (fType)
	{
	case kPROP_TYPE_BOOL:
		fValue.fBool = inValue.fValue.fBool;
		break;
	case kPROP_TYPE_ULONG:
		fValue.fULong = inValue.fValue.fULong;
		break;
	case kPROP_TYPE_SLONG:
		fValue.fSLong = inValue.fValue.fSLong;
		break;
	case kPROP_TYPE_SMALLREAL:
	case kPROP_TYPE_PERCENT:
		fValue.fSmallReal = inValue.fValue.fSmallReal;
		break;
	case kPROP_TYPE_REAL:
		fValue.fReal = inValue.fValue.fReal;
		break;
	case kPROP_TYPE_RECT:
		fValue.fRect = inValue.fValue.fRect;
		break;
	case kPROP_TYPE_VECTOR_SLONG:
		fVectorOfSLong = inValue.fVectorOfSLong;
		break;
	case kPROP_TYPE_VSTRING:
		fString = inValue.fString;
		break;
	case kPROP_TYPE_VTIME:
		fTime = inValue.fTime;
		break;
	case kPROP_TYPE_INHERIT:
		break;
	default:
		xbox_assert(false);
		break;
	}

	return *this;
}


bool VDocProperty::operator == (const VDocProperty& inValue) const
{ 
	if (this == &inValue)
		return true;

	if (fType != inValue.fType)
		return false;

	switch (fType)
	{
	case kPROP_TYPE_BOOL:
		return (fValue.fBool == inValue.fValue.fBool);
		break;
	case kPROP_TYPE_ULONG:
		return (fValue.fULong == inValue.fValue.fULong);
		break;
	case kPROP_TYPE_SLONG:
		return (fValue.fSLong == inValue.fValue.fSLong);
		break;
	case kPROP_TYPE_VSTRING:
		return (fString.EqualToStringRaw( inValue.fString));
		break;
	case kPROP_TYPE_SMALLREAL:
	case kPROP_TYPE_PERCENT:
		return (fValue.fSmallReal == inValue.fValue.fSmallReal);
		break;
	case kPROP_TYPE_REAL:
		return (fValue.fReal == inValue.fValue.fReal);
		break;
	case kPROP_TYPE_RECT:
		return fValue.fRect.left == inValue.fValue.fRect.left
			   &&
			   fValue.fRect.top == inValue.fValue.fRect.top
			   &&	
			   fValue.fRect.right == inValue.fValue.fRect.right
			   &&	
			   fValue.fRect.bottom == inValue.fValue.fRect.bottom
			   ;
		break;
	case kPROP_TYPE_VECTOR_SLONG:
		{
		if (fVectorOfSLong.size() != inValue.fVectorOfSLong.size())
			return false;
		VectorOfSLONG::const_iterator it1 = fVectorOfSLong.begin();
		VectorOfSLONG::const_iterator it2 = inValue.fVectorOfSLong.begin();
		for (;it1 != fVectorOfSLong.end(); it1++, it2++)
		{
			if (*it1 != *it2)
				return false;
		}		
		return true;
		}
		break;
	case kPROP_TYPE_VTIME:
		return (fTime == inValue.fTime);
	case kPROP_TYPE_INHERIT:
		return true;
		break;
	default:
		xbox_assert(false);
		break;
	}
	return false;
}


void VDocNode::Init()
{
	sElementName[kDOC_NODE_TYPE_DOCUMENT]			= "body";
	sElementName[kDOC_NODE_TYPE_DIV]				= "div";
	sElementName[kDOC_NODE_TYPE_PARAGRAPH]			= "p";
	sElementName[kDOC_NODE_TYPE_IMAGE]				= "img";
	sElementName[kDOC_NODE_TYPE_TABLE]				= "table";
	sElementName[kDOC_NODE_TYPE_TABLEROW]			= "tr";
	sElementName[kDOC_NODE_TYPE_TABLECELL]			= "td";
	sElementName[kDOC_NODE_TYPE_CLASS_STYLE]		= "";
	sElementName[kDOC_NODE_TYPE_UNKNOWN]			= "";

	//document default properties (for regular CSS styles, default values are compliant with W3C default values)

	sPropDefault[kDOC_PROP_ID]						= new VDocProperty(VString("1"));
	sPropDefault[kDOC_PROP_CLASS]					= new VDocProperty(VString(""));

	sPropDefault[kDOC_PROP_VERSION]					= new VDocProperty((uLONG)kDOC_VERSION_SPAN4D);
	sPropDefault[kDOC_PROP_WIDOWSANDORPHANS]		= new VDocProperty(true);
	sPropDefault[kDOC_PROP_DPI]						= new VDocProperty((uLONG)72);
	sPropDefault[kDOC_PROP_ZOOM]					= new VDocProperty((uLONG)100);
	sPropDefault[kDOC_PROP_LAYOUT_MODE]				= new VDocProperty((uLONG)kDOC_LAYOUT_MODE_NORMAL);
	sPropDefault[kDOC_PROP_SHOW_IMAGES]				= new VDocProperty(true);
	sPropDefault[kDOC_PROP_SHOW_REFERENCES]			= new VDocProperty(false);
	sPropDefault[kDOC_PROP_SHOW_HIDDEN_CHARACTERS]	= new VDocProperty(false);
	sPropDefault[kDOC_PROP_LANG]					= new VDocProperty((uLONG)(VProcess::Get()->GetLocalizer() ? VProcess::Get()->GetLocalizer()->GetLocalizationLanguage() : VIntlMgr::ResolveDialectCode(DC_USER)));
	sPropDefault[kDOC_PROP_DWRITE]					= new VDocProperty(false);

	VTime time;
	time.FromSystemTime();
	sPropDefault[kDOC_PROP_DATETIMECREATION]		= new VDocProperty(time);
	sPropDefault[kDOC_PROP_DATETIMEMODIFIED]		= new VDocProperty(time);

	//sPropDefault[kDOC_PROP_TITLE]					= new VDocProperty(VString("Document1"));
	//sPropDefault[kDOC_PROP_SUBJECT]				= new VDocProperty(VString(""));
	//VString userName;
	//VSystem::GetLoginUserName( userName);
	//sPropDefault[kDOC_PROP_AUTHOR]				= new VDocProperty(userName);
	//sPropDefault[kDOC_PROP_COMPANY]				= new VDocProperty(VString(""));
	//sPropDefault[kDOC_PROP_NOTES]					= new VDocProperty(VString(""));

	//common properties

	sPropDefault[kDOC_PROP_TEXT_ALIGN]				= new VDocProperty( (uLONG)JST_Default);
	sPropDefault[kDOC_PROP_VERTICAL_ALIGN]			= new VDocProperty( (uLONG)JST_Default);
	sDocPropRect margin = { 0, 0, 0, 0 }; //please do not modify it
	sPropDefault[kDOC_PROP_MARGIN]					= new VDocProperty( margin);
	sPropDefault[kDOC_PROP_PADDING]					= new VDocProperty(	margin);

	sPropDefault[kDOC_PROP_BACKGROUND_COLOR]		= new VDocProperty( (RGBAColor)RGBACOLOR_TRANSPARENT); //default is transparent
	sPropDefault[kDOC_PROP_BACKGROUND_IMAGE]		= new VDocProperty( CVSTR("")); //empty url
	VDocProperty::VectorOfSLONG position;
	position.push_back(JST_Left);					//default background image horizontal align = background origin box left
	position.push_back(JST_Top);					//default background image vertical align = background origin box top
	sPropDefault[kDOC_PROP_BACKGROUND_POSITION]		= new VDocProperty( position); //left top 
	sPropDefault[kDOC_PROP_BACKGROUND_REPEAT]		= new VDocProperty( (uLONG)kDOC_BACKGROUND_REPEAT); //repeat 
	sPropDefault[kDOC_PROP_BACKGROUND_CLIP]			= new VDocProperty( (uLONG)kDOC_BACKGROUND_BORDER_BOX);	//border-box 
	sPropDefault[kDOC_PROP_BACKGROUND_ORIGIN]		= new VDocProperty( (uLONG)kDOC_BACKGROUND_PADDING_BOX);	//padding-box 
	VDocProperty::VectorOfSLONG size;
	size.push_back(kDOC_BACKGROUND_SIZE_AUTO);					
	size.push_back(kDOC_BACKGROUND_SIZE_AUTO);					
	sPropDefault[kDOC_PROP_BACKGROUND_SIZE]			= new VDocProperty(	size);		//auto

	sPropDefault[kDOC_PROP_WIDTH]					= new VDocProperty( (uLONG)0);	//0 = auto
	sPropDefault[kDOC_PROP_HEIGHT]					= new VDocProperty( (uLONG)0);	//0 = auto
	sPropDefault[kDOC_PROP_MIN_WIDTH]				= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_MIN_HEIGHT]				= new VDocProperty( (uLONG)0); 
	sDocPropRect borderStyle = { 0, 0, 0, 0 };
	sPropDefault[kDOC_PROP_BORDER_STYLE]			= new VDocProperty( borderStyle); //none border
	sDocPropRect borderWidth = { 2*20, 2*20, 2*20, 2*20 };
	sPropDefault[kDOC_PROP_BORDER_WIDTH]			= new VDocProperty( borderWidth); //medium = 2pt = 2*20 TWIPS (regular CSS = 2px if 72dpi (4D form) or 3px if 96dpi(as in browsers)) 
	sDocPropRect borderColor = { RGBACOLOR_TRANSPARENT, RGBACOLOR_TRANSPARENT, RGBACOLOR_TRANSPARENT, RGBACOLOR_TRANSPARENT};
	sPropDefault[kDOC_PROP_BORDER_COLOR]			= new VDocProperty( borderColor); //transparent
	sDocPropRect borderRadius = { 0, 0, 0, 0 };
	sPropDefault[kDOC_PROP_BORDER_RADIUS]			= new VDocProperty( borderRadius); 

	//paragraph properties

	sPropDefault[kDOC_PROP_DIRECTION]				= new VDocProperty( (uLONG)kDOC_DIRECTION_LTR); 
	sPropDefault[kDOC_PROP_LINE_HEIGHT]				= new VDocProperty( (sLONG)(kDOC_PROP_LINE_HEIGHT_NORMAL)); //normal
	sPropDefault[kDOC_PROP_TEXT_INDENT]				= new VDocProperty( (sLONG)0); //0cm 
	sPropDefault[kDOC_PROP_TAB_STOP_OFFSET]			= new VDocProperty( (uLONG)floor(((1.25*72*20)/2.54) + 0.5)); //1,25cm = 709 TWIPS
	sPropDefault[kDOC_PROP_TAB_STOP_TYPE]			= new VDocProperty( (uLONG)kDOC_TAB_STOP_TYPE_LEFT); 
	sPropDefault[kDOC_PROP_LIST_STYLE_TYPE]			= new VDocProperty( (uLONG)kDOC_LIST_STYLE_TYPE_NONE); 
	sPropDefault[kDOC_PROP_LIST_STRING_FORMAT_LTR]	= new VDocProperty( VString()); 
	sPropDefault[kDOC_PROP_LIST_STRING_FORMAT_RTL]	= new VDocProperty( VString()); 
	sPropDefault[kDOC_PROP_LIST_FONT_FAMILY]		= new VDocProperty( VString("Times New Roman")); //TODO: check on Mac 
	sPropDefault[kDOC_PROP_LIST_STYLE_IMAGE]		= new VDocProperty( VString(""));
	sPropDefault[kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT]	= new VDocProperty( (uLONG)0); //0 = auto
	sPropDefault[kDOC_PROP_LIST_START]				= new VDocProperty( (sLONG)kDOC_LIST_START_AUTO);
	sPropDefault[kDOC_PROP_NEW_PARA_CLASS]			= new VDocProperty( VString("")); 

	//image properties

	sPropDefault[kDOC_PROP_IMG_SOURCE]				= new VDocProperty( VString("")); 
	sPropDefault[kDOC_PROP_IMG_ALT_TEXT]			= new VDocProperty( VString("")); //FIXME: maybe we should set default text for image element ?

	//span character styles
	 
	sPropDefault[kDOC_PROP_FONT_FAMILY]				= new VDocProperty( VString("Times New Roman")); 
	sPropDefault[kDOC_PROP_FONT_SIZE]				= new VDocProperty( (uLONG)(12*20)); 
	sPropDefault[kDOC_PROP_COLOR]					= new VDocProperty( (uLONG)0xff000000); 
	sPropDefault[kDOC_PROP_FONT_STYLE]				= new VDocProperty( (uLONG)kFONT_STYLE_NORMAL); 
	sPropDefault[kDOC_PROP_FONT_WEIGHT]				= new VDocProperty( (uLONG)kFONT_WEIGHT_NORMAL); 
	sPropDefault[kDOC_PROP_TEXT_DECORATION]			= new VDocProperty( (uLONG)kTEXT_DECORATION_NONE); 
	sPropDefault[kDOC_PROP_4DREF]					= new VDocProperty( VDocSpanTextRef::fSpace); 
	sPropDefault[kDOC_PROP_4DREF_USER]				= new VDocProperty( VDocSpanTextRef::fSpace); 
	sPropDefault[kDOC_PROP_TEXT_BACKGROUND_COLOR]	= new VDocProperty( (RGBAColor)RGBACOLOR_TRANSPARENT); //default is transparent

	//default string formats per list style type
	//remark: string format here IS NOT a 4D string format; '#' is a placeholder for the list item value (which might be numeric or alphabetic)

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_NONE]						= "";
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_NONE]						= "";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DISC].FromUniChar( 0x25CF);  
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_DISC] = sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DISC]; 						

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_CIRCLE].FromUniChar( 0x25CB);  				
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_CIRCLE] = sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_CIRCLE];			

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_SQUARE].FromUniChar( 0x25A0);  					
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_SQUARE] = sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_SQUARE];						

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE].FromUniChar( 0x25A1);  			
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE] = sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE];				

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DIAMOND].FromUniChar( 0x2666);  				
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_DIAMOND] = sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DIAMOND];				

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_CLUB].FromUniChar( 0x2663);  
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_CLUB] = sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_CLUB];	

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_CUSTOM]					= "-";		 //custom unordered list character string
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_CUSTOM]					= "-";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DECIMAL]					= "#.";		
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_DECIMAL]					= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO]		= "0#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO]		= "0#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_LOWER_LATIN]				= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_LOWER_LATIN]				= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_LOWER_ROMAN]				= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_LOWER_ROMAN]				= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_UPPER_LATIN]				= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_UPPER_LATIN]				= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_UPPER_ROMAN]				= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_UPPER_ROMAN]				= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_LOWER_GREEK]				= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_LOWER_GREEK]				= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_ARMENIAN]					= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_ARMENIAN]					= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_GEORGIAN]					= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_GEORGIAN]					= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_HEBREW]					= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_HEBREW]					= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_HIRAGANA]					= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_HIRAGANA]					= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_KATAKANA]					= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_KATAKANA]					= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC]			= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC]			= "#.";

	sListStringFormatLTR[kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK]			= "#."; 
	sListStringFormatRTL[kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK]			= "#.";

	for (int i = kDOC_LIST_STYLE_TYPE_FIRST; i <= kDOC_LIST_STYLE_TYPE_LAST; i++)
		sListFontFamily[i]		= "Times New Roman";
	//for some list types, we cannot use Times New Roman
	sListFontFamily[kDOC_LIST_STYLE_TYPE_ARMENIAN]			= "Arial Unicode MS";
	sListFontFamily[kDOC_LIST_STYLE_TYPE_GEORGIAN]			= "Arial Unicode MS";
	sListFontFamily[kDOC_LIST_STYLE_TYPE_HIRAGANA]			= "Arial Unicode MS";
	sListFontFamily[kDOC_LIST_STYLE_TYPE_KATAKANA]			= "Arial Unicode MS";
	sListFontFamily[kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC]	= "Arial Unicode MS";
				 	
	sLowerRomanStrings[0]		= "x";
	sLowerRomanStrings[1]		= "i";
	sLowerRomanStrings[2]		= "ii";
	sLowerRomanStrings[3]		= "iii";
	sLowerRomanStrings[4]		= "iv";
	sLowerRomanStrings[5]		= "v";
	sLowerRomanStrings[6]		= "vi";
	sLowerRomanStrings[7]		= "vii";
	sLowerRomanStrings[8]		= "viii";
	sLowerRomanStrings[9]		= "ix";
	 
	sUpperRomanStrings[0]		= "0";
	sUpperRomanStrings[1]		= "I";
	sUpperRomanStrings[2]		= "II";
	sUpperRomanStrings[3]		= "III";
	sUpperRomanStrings[4]		= "IV";
	sUpperRomanStrings[5]		= "V";
	sUpperRomanStrings[6]		= "VI";
	sUpperRomanStrings[7]		= "VII";
	sUpperRomanStrings[8]		= "VIII";
	sUpperRomanStrings[9]		= "IX";

	sLowerRomanStrings10[0]		= "0";		//00
	sLowerRomanStrings10[1]		= "x";		//10
	sLowerRomanStrings10[2]		= "xx";		//20
	sLowerRomanStrings10[3]		= "xxx";	//30
	sLowerRomanStrings10[4]		= "xl";		//40
	sLowerRomanStrings10[5]		= "l";		//50
	sLowerRomanStrings10[6]		= "lx";		//60
	sLowerRomanStrings10[7]		= "lxx";	//70
	sLowerRomanStrings10[8]		= "lxxx";	//80
	sLowerRomanStrings10[9]		= "xc";		//90
	 
	sUpperRomanStrings10[0]		= "0";		//00
	sUpperRomanStrings10[1]		= "X";		//10
	sUpperRomanStrings10[2]		= "XX";		//20
	sUpperRomanStrings10[3]		= "XXX";	//30
	sUpperRomanStrings10[4]		= "XL";		//40
	sUpperRomanStrings10[5]		= "L";		//50
	sUpperRomanStrings10[6]		= "LX";		//60
	sUpperRomanStrings10[7]		= "LXX";	//70
	sUpperRomanStrings10[8]		= "LXXX";	//80
	sUpperRomanStrings10[9]		= "XC";		//90

	sLowerRomanStrings100[0]	= "0";		//000
	sLowerRomanStrings100[1]	= "c";		//100
	sLowerRomanStrings100[2]	= "cc";		//200
	sLowerRomanStrings100[3]	= "ccc";	//300
	sLowerRomanStrings100[4]	= "cd";		//400
	sLowerRomanStrings100[5]	= "d";		//500
	sLowerRomanStrings100[6]	= "dc";		//600
	sLowerRomanStrings100[7]	= "dcc";	//700
	sLowerRomanStrings100[8]	= "dccc";	//800
	sLowerRomanStrings100[9]	= "cm";		//900

	sUpperRomanStrings100[0]	= "0";		//000
	sUpperRomanStrings100[1]	= "C";		//100
	sUpperRomanStrings100[2]	= "CC";		//200
	sUpperRomanStrings100[3]	= "CCC";	//300
	sUpperRomanStrings100[4]	= "CD";		//400
	sUpperRomanStrings100[5]	= "D";		//500
	sUpperRomanStrings100[6]	= "DC";		//600
	sUpperRomanStrings100[7]	= "DCC";	//700
	sUpperRomanStrings100[8]	= "DCCC";	//800
	sUpperRomanStrings100[9]	= "CM";		//900
	

	sDecimalGreekUniChar[0]			= '0';
	sDecimalGreekUniChar[1]			= 0x03B1;
	sDecimalGreekUniChar[2]			= 0x03B2;
	sDecimalGreekUniChar[3]			= 0x03B3;
	sDecimalGreekUniChar[4]			= 0x03B4;
	sDecimalGreekUniChar[5]			= 0x03B5;
	sDecimalGreekUniChar[6]			= 0x03C2;
	sDecimalGreekUniChar[7]			= 0x03B6;
	sDecimalGreekUniChar[8]			= 0x03B7;
	sDecimalGreekUniChar[9]			= 0x03B8;

	sDecimalGreekUniChar10[0]		= '0';		//00
	sDecimalGreekUniChar10[1]		= 0x03B9;	//10
	sDecimalGreekUniChar10[2]		= 0x03BA;	//20
	sDecimalGreekUniChar10[3]		= 0x03BB;	//30
	sDecimalGreekUniChar10[4]		= 0x03BC;	//40
	sDecimalGreekUniChar10[5]		= 0x03BD;	//50
	sDecimalGreekUniChar10[6]		= 0x03BE;	//60
	sDecimalGreekUniChar10[7]		= 0x03BF;	//70
	sDecimalGreekUniChar10[8]		= 0x03C0;	//80
	sDecimalGreekUniChar10[9]		= 0x03DF;	//90

	sDecimalGreekUniChar100[0]	= '0';		//000
	sDecimalGreekUniChar100[1]	= 0x03C1;	//100
	sDecimalGreekUniChar100[2]	= 0x03C3;	//200
	sDecimalGreekUniChar100[3]	= 0x03C4;	//300
	sDecimalGreekUniChar100[4]	= 0x03C5;	//400
	sDecimalGreekUniChar100[5]	= 0x03C6;	//500
	sDecimalGreekUniChar100[6]	= 0x03C7;	//600
	sDecimalGreekUniChar100[7]	= 0x03C8;	//700
	sDecimalGreekUniChar100[8]	= 0x03C9;	//800
	sDecimalGreekUniChar100[9]	= 0x03E1;	//900


	sLowerGreekUniChar[0]		= 0x03B1;	//alphabetical alpha, beta, gamma, ..., omega (24 letters)
	sLowerGreekUniChar[1]		= 0x03B2;
	sLowerGreekUniChar[2]		= 0x03B3;
	sLowerGreekUniChar[3]		= 0x03B4;
	sLowerGreekUniChar[4]		= 0x03B5;
	sLowerGreekUniChar[5]		= 0x03B6;
	sLowerGreekUniChar[6]		= 0x03B7;
	sLowerGreekUniChar[7]		= 0x03B8;
	sLowerGreekUniChar[8]		= 0x03B9;
	sLowerGreekUniChar[9]		= 0x03BA;
	sLowerGreekUniChar[10]		= 0x03BB;
	sLowerGreekUniChar[11]		= 0x03BC;
	sLowerGreekUniChar[12]		= 0x03BD;
	sLowerGreekUniChar[13]		= 0x03BE;
	sLowerGreekUniChar[14]		= 0x03BF;
	sLowerGreekUniChar[15]		= 0x03C0;
	sLowerGreekUniChar[16]		= 0x03C1;	//we skip 0x3C2 which is not included in lower greek alphabet
	sLowerGreekUniChar[17]		= 0x03C3;
	sLowerGreekUniChar[18]		= 0x03C4;
	sLowerGreekUniChar[19]		= 0x03C5;
	sLowerGreekUniChar[20]		= 0x03C6;
	sLowerGreekUniChar[21]		= 0x03C7;
	sLowerGreekUniChar[22]		= 0x03C8;
	sLowerGreekUniChar[23]		= 0x03C9;

	sHebrewUniChar[0]			= '0';
	sHebrewUniChar[1]			= 0x05D0;
	sHebrewUniChar[2]			= 0x05D1;
	sHebrewUniChar[3]			= 0x05D2;
	sHebrewUniChar[4]			= 0x05D3;
	sHebrewUniChar[5]			= 0x05D4;
	sHebrewUniChar[6]			= 0x05D5;
	sHebrewUniChar[7]			= 0x05D6;
	sHebrewUniChar[8]			= 0x05D7;
	sHebrewUniChar[9]			= 0x05D8;

	sHebrewUniChar10[0]			= '0';		//00
	sHebrewUniChar10[1]			= 0x05D9;	//10
	sHebrewUniChar10[2]			= 0x05DB;	//20
	sHebrewUniChar10[3]			= 0x05DC;	//30
	sHebrewUniChar10[4]			= 0x05DE;	//40
	sHebrewUniChar10[5]			= 0x05E0;	//50
	sHebrewUniChar10[6]			= 0x05E1;	//60
	sHebrewUniChar10[7]			= 0x05E2;	//70
	sHebrewUniChar10[8]			= 0x05E4;	//80
	sHebrewUniChar10[9]			= 0x05E6;	//90

	sHebrewUniChar100[0]		= '0';		//000
	sHebrewUniChar100[1]		= 0x05E7;	//100
	sHebrewUniChar100[2]		= 0x05E8;	//200
	sHebrewUniChar100[3]		= 0x05E9;	//300
	sHebrewUniChar100[4]		= 0x05EA;	//400
	sHebrewUniChar100[5]		= 0x05DA;	//500
	sHebrewUniChar100[6]		= 0x05DD;	//600
	sHebrewUniChar100[7]		= 0x05DF;	//700
	sHebrewUniChar100[8]		= 0x05E3;	//800
	sHebrewUniChar100[9]		= 0x05E5;	//900


	sGeorgianUniChar[0]			= '0';
	sGeorgianUniChar[1]			= 0x10D0;
	sGeorgianUniChar[2]			= 0x10D1;
	sGeorgianUniChar[3]			= 0x10D2;
	sGeorgianUniChar[4]			= 0x10D3;
	sGeorgianUniChar[5]			= 0x10D4;
	sGeorgianUniChar[6]			= 0x10D5;
	sGeorgianUniChar[7]			= 0x10D6;
	sGeorgianUniChar[8]			= 0x10F1;
	sGeorgianUniChar[9]			= 0x10D7;

	sGeorgianUniChar10[0]		= '0';		//00
	sGeorgianUniChar10[1]		= 0x10D8;	//10
	sGeorgianUniChar10[2]		= 0x10D9;	//20
	sGeorgianUniChar10[3]		= 0x10DA;	//30
	sGeorgianUniChar10[4]		= 0x10DB;	//40
	sGeorgianUniChar10[5]		= 0x10DC;	//50
	sGeorgianUniChar10[6]		= 0x10F2;	//60
	sGeorgianUniChar10[7]		= 0x10DD;	//70
	sGeorgianUniChar10[8]		= 0x10DE;	//80
	sGeorgianUniChar10[9]		= 0x10DF;	//90

	sGeorgianUniChar100[0]		= '0';		//000
	sGeorgianUniChar100[1]		= 0x10E0;	//100
	sGeorgianUniChar100[2]		= 0x10E1;	//200
	sGeorgianUniChar100[3]		= 0x10E2;	//300
	sGeorgianUniChar100[4]		= 0x10F3;	//400
	sGeorgianUniChar100[5]		= 0x10E4;	//500
	sGeorgianUniChar100[6]		= 0x10E5;	//600
	sGeorgianUniChar100[7]		= 0x10E6;	//700
	sGeorgianUniChar100[8]		= 0x10E7;	//800
	sGeorgianUniChar100[9]		= 0x10E8;	//900

	sGeorgianUniChar1000[0]		= '0';		//0000
	sGeorgianUniChar1000[1]		= 0x10E9;	//1000
	sGeorgianUniChar1000[2]		= 0x10EA;	//2000
	sGeorgianUniChar1000[3]		= 0x10EB;	//3000
	sGeorgianUniChar1000[4]		= 0x10EC;	//4000
	sGeorgianUniChar1000[5]		= 0x10ED;	//5000
	sGeorgianUniChar1000[6]		= 0x10EE;	//6000
	sGeorgianUniChar1000[7]		= 0x10F4;	//7000
	sGeorgianUniChar1000[8]		= 0x10EF;	//8000
	sGeorgianUniChar1000[9]		= 0x10F0;	//9000


	sArmenianUniChar[0]			= '0';
	sArmenianUniChar[1]			= 0x0531;
	sArmenianUniChar[2]			= 0x0532;
	sArmenianUniChar[3]			= 0x0533;
	sArmenianUniChar[4]			= 0x0534;
	sArmenianUniChar[5]			= 0x0535;
	sArmenianUniChar[6]			= 0x0536;
	sArmenianUniChar[7]			= 0x0537;
	sArmenianUniChar[8]			= 0x0538;
	sArmenianUniChar[9]			= 0x0539;

	sArmenianUniChar10[0]		= '0';		//00
	sArmenianUniChar10[1]		= 0x053A;	//10
	sArmenianUniChar10[2]		= 0x053B;	//20
	sArmenianUniChar10[3]		= 0x053C;	//30
	sArmenianUniChar10[4]		= 0x053D;	//40
	sArmenianUniChar10[5]		= 0x053E;	//50
	sArmenianUniChar10[6]		= 0x053F;	//60
	sArmenianUniChar10[7]		= 0x0540;	//70
	sArmenianUniChar10[8]		= 0x0541;	//80
	sArmenianUniChar10[9]		= 0x0542;	//90

	sArmenianUniChar100[0]		= '0';		//000
	sArmenianUniChar100[1]		= 0x0543;	//100
	sArmenianUniChar100[2]		= 0x0544;	//200
	sArmenianUniChar100[3]		= 0x0545;	//300
	sArmenianUniChar100[4]		= 0x0546;	//400
	sArmenianUniChar100[5]		= 0x0547;	//500
	sArmenianUniChar100[6]		= 0x0548;	//600
	sArmenianUniChar100[7]		= 0x0549;	//700
	sArmenianUniChar100[8]		= 0x054A;	//800
	sArmenianUniChar100[9]		= 0x054B;	//900

	sArmenianUniChar1000[0]		= '0';		//0000
	sArmenianUniChar1000[1]		= 0x054C;	//1000
	sArmenianUniChar1000[2]		= 0x054D;	//2000
	sArmenianUniChar1000[3]		= 0x054E;	//3000
	sArmenianUniChar1000[4]		= 0x054F;	//4000
	sArmenianUniChar1000[5]		= 0x0550;	//5000
	sArmenianUniChar1000[6]		= 0x0551;	//6000
	sArmenianUniChar1000[7]		= 0x0552;	//7000
	sArmenianUniChar1000[8]		= 0x0553;	//8000
	sArmenianUniChar1000[9]		= 0x0554;	//9000

	sCJK[0]						= 0x96F6;
	sCJK[1]						= 0x4E00;
	sCJK[2]						= 0x4E8C;
	sCJK[3]						= 0x4E09;
	sCJK[4]						= 0x56DB; 
	sCJK[5]						= 0x4E94;
	sCJK[6]						= 0x516D;
	sCJK[7]						= 0x4E03;
	sCJK[8]						= 0x516B;
	sCJK[9]						= 0x4E5D;

	sHiragana[0]				= '0';
	sHiragana[1]				= 0x3041;
	sHiragana[2]				= 0x3043;
	sHiragana[3]				= 0x3045;
	sHiragana[4]				= 0x3047; 
	sHiragana[5]				= 0x3049;
	sHiragana[6]				= 0x304B;
	sHiragana[7]				= 0x304D;
	sHiragana[8]				= 0x304F;

	sHiragana[9]				= 0x3051;
	sHiragana[10]				= 0x3053;
	sHiragana[11]				= 0x3055;
	sHiragana[12]				= 0x3057;
	sHiragana[13]				= 0x3059;
	sHiragana[14]				= 0x305B;
	sHiragana[15]				= 0x305D;
	sHiragana[16]				= 0x305F;

	sHiragana[17]				= 0x3061;
	sHiragana[18]				= 0x3063;
	sHiragana[19]				= 0x3066;
	sHiragana[20]				= 0x3068;
	sHiragana[21]				= 0x306A;
	sHiragana[22]				= 0x306B;
	sHiragana[23]				= 0x306C;
	sHiragana[24]				= 0x306D;
	sHiragana[25]				= 0x306E;
	sHiragana[26]				= 0x306F;

	sHiragana[27]				= 0x3072;
	sHiragana[28]				= 0x3075;
	sHiragana[29]				= 0x3078;
	sHiragana[30]				= 0x307B;
	sHiragana[31]				= 0x307E;
	sHiragana[32]				= 0x307F;
	
	sHiragana[33]				= 0x3080;
	sHiragana[34]				= 0x3081;
	sHiragana[35]				= 0x3082;
	sHiragana[36]				= 0x3083;
	sHiragana[37]				= 0x3085;
	sHiragana[38]				= 0x3087;
	sHiragana[39]				= 0x3089;
	sHiragana[40]				= 0x308A;
	sHiragana[41]				= 0x308B;
	sHiragana[42]				= 0x308C;
	sHiragana[43]				= 0x308D;
	sHiragana[44]				= 0x308E;

	sHiragana[45]				= 0x3090;
	sHiragana[46]				= 0x3091;
	sHiragana[47]				= 0x3092;
	sHiragana[48]				= 0x3093;
	
	sPropInitDone = true;
}

#define	CJK_NUMBER_10			0x5341
#define CJK_NUMBER_100			0x767E
#define CJK_NUMBER_1000			0x5343

void VDocNode::DeInit()
{
	for (int i = 0; i < kDOC_PROP_COUNT; i++)
		if (sPropDefault[i]) 
		{	
			delete sPropDefault[i];
			sPropDefault[i] = NULL;
		}
	sPropInitDone = false;
}

/** return default ltr string format for the passed list style type 
@remarks
	method should be called to get default string format if property is not overriden 
*/
const VString& VDocNode::GetDefaultListStringFormatLTR(  eDocPropListStyleType inListStyleType)
{
	return sListStringFormatLTR[ inListStyleType];
}
		
/** return default rtl string format for the passed list style type 
@remarks
	method should be called to get default string format if property is not overriden 
*/
const VString& VDocNode::GetDefaultListStringFormatRTL(  eDocPropListStyleType inListStyleType)
{
	return sListStringFormatRTL[ inListStyleType];
}

/** return default list font family for the passed list style type */
const VString& VDocNode::GetDefaultListFontFamily( eDocPropListStyleType inListStyleType)
{
	return sListFontFamily[ inListStyleType];
}


/** get list item string value from list style type and list item number */
void VDocNode::GetListItemValue(	eDocPropListStyleType inListStyleType, VString& outValue, sLONG inNumber, bool inRTL, 
									const VString* inCustomStringFormatLTR, 
									const VString* inCustomStringFormatRTL)
{ 
	if (inListStyleType >= kDOC_LIST_STYLE_TYPE_OL_FIRST && inListStyleType <= kDOC_LIST_STYLE_TYPE_OL_LAST && inNumber <= 0)
		inListStyleType = kDOC_LIST_STYLE_TYPE_DECIMAL;

	if (inRTL)
		outValue = inCustomStringFormatRTL ? *inCustomStringFormatRTL : GetDefaultListStringFormatRTL(inListStyleType);
	else
		outValue = inCustomStringFormatLTR ? *inCustomStringFormatLTR : GetDefaultListStringFormatLTR(inListStyleType);
	if (inListStyleType < kDOC_LIST_STYLE_TYPE_OL_FIRST)
		return;

	switch (inListStyleType)
	{
	case kDOC_LIST_STYLE_TYPE_DECIMAL:
	case kDOC_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
		{
		VString value;
		value.FromLong( inNumber);
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_LOWER_LATIN:
		{
		VString value;
		value.EnsureSize(10);

		sLONG numLetter = (inNumber-1)%26;
		sLONG div = (inNumber-1)/26;
		value.AppendUniChar((UniChar)'a'+numLetter); 
		while (div)
		{
			numLetter = (div-1)%26;
			div = (div-1)/26;
			value.Insert((UniChar)'a'+numLetter, 1);
		}

		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_UPPER_LATIN:
		{
		VString value;
		value.EnsureSize(10);

		sLONG numLetter = (inNumber-1)%26;
		sLONG div = (inNumber-1)/26;
		value.AppendUniChar((UniChar)'A'+numLetter); 
		while (div)
		{
			numLetter = (div-1)%26;
			div = (div-1)/26;
			value.Insert((UniChar)'A'+numLetter, 1);
		}

		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_LOWER_ROMAN:
		{
		//see http://fr.wikipedia.org/wiki/Num%C3%A9ration_romaine

		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
			for (int i = 1; i <= mille; i++)
				value.AppendUniChar('m');
		inNumber = inNumber%1000;
		if (inNumber)
		{
			sLONG cent = inNumber/100;
			if (cent)
				value.AppendString( sLowerRomanStrings100[cent]); 
			inNumber = inNumber%100;
			sLONG ten = inNumber/10;
			if (ten)
				value.AppendString( sLowerRomanStrings10[ten]);	
			inNumber = inNumber%10;
			if (inNumber)
				value.AppendString( sLowerRomanStrings[inNumber]);
		}
		else if (!mille)
			value = sLowerRomanStrings[0];
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_UPPER_ROMAN:
		{
		//see http://fr.wikipedia.org/wiki/Num%C3%A9ration_romaine

		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
			for (int i = 1; i <= mille; i++)
				value.AppendUniChar('M');
		inNumber = inNumber%1000;
		if (inNumber)
		{
			sLONG cent = inNumber/100;
			if (cent)
				value.AppendString( sUpperRomanStrings100[cent]); 
			inNumber = inNumber%100;
			sLONG ten = inNumber/10;
			if (ten)
				value.AppendString( sUpperRomanStrings10[ten]);	
			inNumber = inNumber%10;
			if (inNumber)
				value.AppendString( sUpperRomanStrings[inNumber]);
		}
		else if (!mille)
			value = sUpperRomanStrings[0];
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_LOWER_GREEK:
		{
		//alphabetical greek (based on 24 greek characters)

		VString value;
		value.EnsureSize(10);
		
		sLONG numLetter = (inNumber-1)%24;
		sLONG div = (inNumber-1)/24;
		value.AppendUniChar( sLowerGreekUniChar[numLetter]); 
		while (div)
		{
			numLetter = (div-1)%24;
			div = (div-1)/24;
			value.Insert( sLowerGreekUniChar[numLetter], 1); 
		}

		outValue.Exchange( CVSTR("#"), value);
		}
		break;

	case kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK:
		{
		//decimal greek
		//see http://fr.wikipedia.org/wiki/Num%C3%A9ration_grecque

		if (inNumber >= 10000)
		{
			xbox_assert(false); //we should never have list so long...
			VString value;
			value.FromLong( inNumber);
			outValue.Exchange( CVSTR("#"), value);
			break;
		}

		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
		{
			value.AppendUniChar( 0x02B9); //modifier prime for x1000
			value.AppendUniChar( sDecimalGreekUniChar[mille]); //alpha, beta, gamma, etc...
		}
		inNumber = inNumber%1000;
		if (inNumber)
		{
			sLONG cent = inNumber/100;
			if (cent)
				value.AppendUniChar( sDecimalGreekUniChar100[cent]); 
			inNumber = inNumber%100;
			sLONG ten = inNumber/10;
			if (ten)
				value.AppendUniChar( sDecimalGreekUniChar10[ten]);	
			inNumber = inNumber%10;
			if (inNumber)
				value.AppendUniChar( sDecimalGreekUniChar[inNumber]);
		}
		else if (!mille)
			value = sDecimalGreekUniChar[0];
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_ARMENIAN:
		{
		//numerical armenian
		//see http://en.wikipedia.org/wiki/Armenian_numerals

		if (inNumber >= 10000)
		{
			xbox_assert(false); //we should never have list so long...
			VString value;
			value.FromLong( inNumber);
			outValue.Exchange( CVSTR("#"), value);
			break;
		}

		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
			value.AppendUniChar( sArmenianUniChar1000[mille]); 
		inNumber = inNumber%1000;
		if (inNumber)
		{
			sLONG cent = inNumber/100;
			if (cent)
				value.AppendUniChar( sArmenianUniChar100[cent]); 
			inNumber = inNumber%100;
			sLONG ten = inNumber/10;
			if (ten)
				value.AppendUniChar( sArmenianUniChar10[ten]);	
			inNumber = inNumber%10;
			if (inNumber)
				value.AppendUniChar( sArmenianUniChar[inNumber]);
		}
		else if (!mille)
			value = sArmenianUniChar[0];
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_GEORGIAN:
		{
		//numerical georgian
		//see http://en.wikipedia.org/wiki/Georgian_numerals

		if (inNumber >= 10000)
		{
			xbox_assert(false); //we should never have list so long...
			VString value;
			value.FromLong( inNumber);
			outValue.Exchange( CVSTR("#"), value);
			break;
		}

		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
			value.AppendUniChar( sGeorgianUniChar1000[mille]); 
		inNumber = inNumber%1000;
		if (inNumber)
		{
			sLONG cent = inNumber/100;
			if (cent)
				value.AppendUniChar( sGeorgianUniChar100[cent]); 
			inNumber = inNumber%100;
			sLONG ten = inNumber/10;
			if (ten)
				value.AppendUniChar( sGeorgianUniChar10[ten]);	
			inNumber = inNumber%10;
			if (inNumber)
				value.AppendUniChar( sGeorgianUniChar[inNumber]);
		}
		else if (!mille)
			value = sGeorgianUniChar[0];
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_HEBREW:
		{
		//numerical hebrew
		//see http://fr.wikipedia.org/wiki/Num%C3%A9ration_h%C3%A9bra%C3%AFque
		//FIXME: i am not sure about the 500, 600, 700, 800 & 900: do i need to use the two or three character form if is not a final letter or not ?
		//		 also i use geresh punctuation to avoid ambiguity for thousands but is it right ?

		if (inNumber >= 10000)
		{
			xbox_assert(false); //we should never have list so long...
			VString value;
			value.FromLong( inNumber);
			outValue.Exchange( CVSTR("#"), value);
			break;
		}

		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
			value.AppendUniChar( sHebrewUniChar[mille]);

		inNumber = inNumber%1000;
		if (inNumber)
		{
			if (mille)
			{
				value.AppendUniChar(0x05F3); //geresh punctuation (in order to distinguish thousands)
				value.AppendUniChar(' ');
			}

			sLONG cent = inNumber/100;
			if (cent)
				value.AppendUniChar( sHebrewUniChar100[cent]); 
			inNumber = inNumber%100;
			
			if (inNumber == 15) //special case for 15 (it is 9+6)
			{
				value.AppendUniChar( sHebrewUniChar[9]);
				value.AppendUniChar( sHebrewUniChar[6]);
			}	
			else if (inNumber == 16) //special case for 16 (it is 9+7)
			{
				value.AppendUniChar( sHebrewUniChar[9]);
				value.AppendUniChar( sHebrewUniChar[7]);
			}	
			else
			{
				sLONG ten = inNumber/10;
				if (ten)
					value.AppendUniChar( sHebrewUniChar10[ten]);	
				inNumber = inNumber%10;
				if (inNumber)
					value.AppendUniChar( sHebrewUniChar[inNumber]);
			}
		}
		else if (mille)
			value.AppendUniChar(0x05F3); //geresh punctuation (in order to distinguish thousands)
		else 
			value.AppendUniChar(sHebrewUniChar[0]);

		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_HIRAGANA:
		{
		//alphabetical hiragana (based on 48 hiragana characters)
		//see http://en.wikipedia.org/wiki/Hiragana

		VString value;
		value.EnsureSize(10);
		
		sLONG numLetter = (inNumber-1)%48;
		sLONG div = (inNumber-1)/48;
		value.AppendUniChar( sHiragana[numLetter+1]); 
		while (div)
		{
			numLetter = (div-1)%48;
			div = (div-1)/48;
			value.Insert( sHiragana[numLetter+1], 1); 
		}

		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_KATAKANA:
		{
		//alphabetical katakana (based on 48 katakana characters)
		//see http://en.wikipedia.org/wiki/Katakana
		//note: katakana unicode table offset from hiragana is equal to 0x30A0-0x3040

		VString value;
		value.EnsureSize(10);
		
		sLONG hiraganaToKatakanaOffset = 0x30A0 - 0x3040;

		sLONG numLetter = (inNumber-1)%48;
		sLONG div = (inNumber-1)/48;
		value.AppendUniChar( (UniChar)(sHiragana[numLetter+1]+hiraganaToKatakanaOffset)); 
		while (div)
		{
			numLetter = (div-1)%48;
			div = (div-1)/48;
			value.Insert( (UniChar)(sHiragana[numLetter+1]+hiraganaToKatakanaOffset), 1); 
		}

		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	case kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC:
		{
		//numerical CJK (unified ideographic)
		//see http://fr.wikipedia.org/wiki/Num%C3%A9ration_japonaise & http://fr.wikipedia.org/wiki/Num%C3%A9ration_chinoise

		if (inNumber >= 10000)
		{
			xbox_assert(false); //we should never have list so long...
			VString value;
			value.FromLong( inNumber);
			outValue.Exchange( CVSTR("#"), value);
			break;
		}
		
		VString value;
		value.EnsureSize(10);

		sLONG mille = inNumber/1000;
		if (mille)
		{
			value.AppendUniChar( sCJK[mille]); //n*
			value.AppendUniChar( CJK_NUMBER_1000); //1000	
		}
		inNumber = inNumber%1000;
		if (inNumber)
		{
			sLONG cent = inNumber/100;
			if (cent)
			{
				value.AppendUniChar( sCJK[cent]); //n*
				value.AppendUniChar( CJK_NUMBER_100); //100	
			}
			inNumber = inNumber%100;
			sLONG ten = inNumber/10;
			if (ten)
			{
				if (ten > 1 || cent || mille)
					value.AppendUniChar( sCJK[ten]); //n*
				value.AppendUniChar( CJK_NUMBER_10); //10	
			}
			inNumber = inNumber%10;
			if (inNumber)
			{
				if (!ten && (cent || mille))
					value.AppendUniChar( sCJK[0]); //insert zero 
				value.AppendUniChar( sCJK[inNumber]);
			}	 
		}
		else if (!mille)
			value.AppendUniChar( sCJK[0]);
		outValue.Exchange( CVSTR("#"), value);
		}
		break;
	default:
		xbox_assert(false);
		break;
	}
}


VDocNode::VDocNode( const VDocNode* inNode)
{
	xbox_assert(inNode);
	fType = inNode->fType;
	fParent = NULL; 
	fDoc = NULL; 
	fID = inNode->fID;
	fClass = inNode->fClass;
	fProps = inNode->fProps;

	fDirtyStamp = 1;
	fTextStart = 0;
	fChildIndex = 0;
	fTextLength = inNode->fTextLength;
	fParserHandler = NULL;

	fNoDeltaTextLength = false;

	if (fType == kDOC_NODE_TYPE_DOCUMENT)
		return;

	VectorOfNode::const_iterator it = inNode->fChildren.begin();
	for (;it != inNode->fChildren.end(); it++)
	{
		VDocNode *node = it->Get()->Clone();
		if (node)
		{
			AddChild( node);
			node->Release();
		}
	}
}

/** get document node */
VDocText *VDocNode::GetDocumentNode() const
{ 
	return fDoc ? dynamic_cast<VDocText *>(fDoc) : NULL; 
}


VDocText *VDocNode::RetainDocumentNode() 
{
	VDocText *doc = fDoc ? dynamic_cast<VDocText *>(fDoc) : NULL; 
	if (doc)
		return RetainRefCountable( doc);
	else
		return NULL;
}


void VDocNode::_GenerateID(VString& outID)
{
	uLONG id = 0;
	VDocText *doc = dynamic_cast<VDocText *>(fDoc);
	if (doc) //if detached -> id is irrelevant 
			 //(id searching is done only from a VDocText 
			 // & ids are generated on node attach event to ensure new attached nodes use unique ids)
		id = doc->_AllocID();
	outID.FromHexLong((uLONG8)id, false); //not pretty formating to make string smallest possible (1-8 characters for a uLONG)
	outID = VString(D4ID_PREFIX_RESERVED)+outID;
}

bool VDocNode::SetText( const VString& inText, VTreeTextStyle *inStyles, bool inCopyStyles)
{
	if (fChildren.size() == 0)
		return false;
	
	return ReplaceStyledText( inText, inStyles, 0, -1, false, true) > 0;
}

/** return next sibling */
VDocNode* VDocNode::_GetNextSibling() const
{
	if (!fParent)
		return NULL;
	if (fChildIndex+1 < fParent->fChildren.size())
		return fParent->fChildren[fChildIndex+1].Get();
	return NULL;
}

/** return child node at the passed text position (text position is 0-based and relative to node start offset) */
VDocNode* VDocNode::_GetChildNodeFromPos(const sLONG inTextPos) const
{
	if (fChildren.size() == 0)
		return NULL;

	VDocNode *node = NULL;
	if (inTextPos <= 0)
		node = fChildren[0].Get();
	else if (inTextPos >= fTextLength)
		node = fChildren.back().Get();
	else //search dichotomically
	{
		sLONG start = 0;
		sLONG end = fChildren.size()-1;
		while (start < end)
		{
			sLONG mid = (start+end)/2;
			if (start < mid)
			{
				if (inTextPos < fChildren[mid].Get()->GetTextStart())
					end = mid-1;
				else
					start = mid;
			}
			else
			{ 
				xbox_assert(start == mid && end == start+1);
				if (inTextPos < fChildren[end].Get()->GetTextStart())
					node = fChildren[start].Get();
				else
					node = fChildren[end].Get();
				break;
			}
		}
		if (!node)
			node = fChildren[start].Get();
	}
	return node;
}

void VDocNode::GetText(VString& outPlainText, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	outPlainText.SetEmpty();
	
	if (fChildren.size() == 0)
		return;

	if (inEnd <= inStart)
		return;

	VDocNodeChildrenIterator it(this);
	VDocNode *node = it.First(inStart, inEnd);
	VString text;
	while (node)
	{
		node->GetText( text, inStart-node->GetTextStart(), inEnd-node->GetTextStart());
		outPlainText.AppendString( text);

		node = it.Next();
	}
}

void VDocNode::SetStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	if (fChildren.size() == 0)
		return;
	VDocNode *node = _GetChildNodeFromPos( 0);
	if (node)
		node->SetStyles( inStyles, inCopyStyles);
}

VTreeTextStyle* VDocNode::RetainStyles(sLONG inStart, sLONG inEnd, bool inRelativeToStart, bool inNULLIfRangeEmpty, bool inNoMerge) const
{
	if (fChildren.size() == 0)
		return NULL;

	_NormalizeRange(inStart, inEnd);

	if (inEnd < inStart)
		return NULL;
	if (inNULLIfRangeEmpty && inStart == inEnd)
		return NULL;

	std::vector<VTreeTextStyle *> styles;

	VDocNodeChildrenIterator it(this);
	VDocNode *node = it.First(inStart, inEnd);
	while (node)
	{
		VTreeTextStyle *style = node->RetainStyles( inStart-node->GetTextStart(), inEnd-node->GetTextStart(), false, inNULLIfRangeEmpty, inNoMerge);
		if (style)
		{
			if (inRelativeToStart)
				style->Translate(node->GetTextStart()-inStart);
			else
				style->Translate(node->GetTextStart());
			styles.push_back( style);
		}

		node = it.Next();
	}

	if (styles.size() > 0)
	{
		//build final styles tree from children styles
		VTreeTextStyle *result = new VTreeTextStyle( new VTextStyle());
		if (inRelativeToStart)
			result->GetData()->SetRange(0, inEnd-inStart);
		else
			result->GetData()->SetRange( inStart, inEnd);
		std::vector<VTreeTextStyle *>::const_iterator itStyle = styles.begin();
		for (;itStyle != styles.end(); itStyle++)
		{
			result->AddChild( *itStyle);
			(*itStyle)->Release();
		}
		return result;
	}
	return NULL;
}

VDocParagraph* VDocNode::RetainFirstParagraph(sLONG inStart)
{
	if (fChildren.size() == 0)
		return NULL;

	VDocNode *node = _GetChildNodeFromPos( inStart);
	if (node)
		return node->RetainFirstParagraph( inStart-node->GetTextStart());
	return NULL;
}

const VDocParagraph* VDocNode::GetFirstParagraph(sLONG inStart) const
{
	if (fChildren.size() == 0)
		return NULL;

	VDocNode *node = _GetChildNodeFromPos( inStart);
	if (node)
		return node->GetFirstParagraph( inStart-node->GetTextStart());
	return NULL;
}


VDocNode* VDocNode::RetainFirstChild(sLONG inStart) const
{
	VDocNode *node = _GetChildNodeFromPos( inStart);
	return RetainRefCountable(node);
}

VDocNode* VDocNode::GetFirstChild(sLONG inStart) const
{
	return _GetChildNodeFromPos( inStart);
}


/** replace styled text at the specified range (relative to node start offset) with new text & styles
@remarks
	replaced text will inherit only uniform style on replaced range - if inInheritUniformStyle == true (default)

	return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
*/
VIndex VDocNode::ReplaceStyledText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd, 
									bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, const VDocSpanTextRef * inPreserveSpanTextRef)
{
	if (fChildren.size() == 0)
		return 0;

	sLONG start = inStart;
	sLONG end = inEnd;
	_NormalizeRange(start, end);

	if (end < start)
	{
		//node range does not include range to replace
		return 0;
	}

	VDocNode *node = _GetChildNodeFromPos( start);
	eDocNodeType type = node->GetType();
	bool isParagraph = type == kDOC_NODE_TYPE_PARAGRAPH;
	if (node)
	{
		VIndex length = 0; //length of replacement text+1
   
		sLONG startBackup = start;

		//delete or truncate all sibling up to inEnd but first node
		
		sLONG index = node->fChildIndex;
		VString textEnd;
		VTreeTextStyle *stylesEnd = NULL;
		VectorOfNode::const_iterator itNode;

		if (isParagraph || inText.IsEmpty()) //skip if it is a container and if we replace with not empty text 
											 //i.e. we replace with not empty text only the first container plain text (word-like behavior)
		{
			std::vector<sLONG> toremove;
			itNode = fChildren.begin();
			std::advance( itNode, index);
			xbox_assert( itNode->Get() == node);
			start = node->GetTextStart()+node->GetTextLength();
			std::advance( itNode, 1); //move to next
			for (;start <= end && itNode != fChildren.end(); itNode++) //iterate on siblings up to inEnd
			{
				node = itNode->Get();
				xbox_assert(node->GetType() == type); //for now all children should be the same kind 
				if (end >= node->GetTextStart()+node->GetTextLength())
					toremove.push_back(node->fChildIndex); //mark paragraph for delete (as it is not the first one & it is fully included in replaced range)
				else if (end >= node->GetTextStart())
				{
					if (end > node->GetTextStart())
					{
						//partial truncating -> truncate start of paragraph node
						VIndex nodeLength = node->ReplaceStyledText( CVSTR(""), NULL, 0, end-node->GetTextStart()); 
						if (nodeLength)
							length = nodeLength;
					}

					if (isParagraph && (inText.IsEmpty() || inText.GetUniChar(inText.GetLength()) != 13))
					{
						//merge with next paragraph

						//backup text & styles
						node->GetText( textEnd, 0, node->GetTextLength());
						stylesEnd = node->RetainStyles();
						
						//remove it
						toremove.push_back(node->fChildIndex); //mark paragraph for delete 
					}
					break;
				}
				else 
					break;
				start = node->GetTextStart()+node->GetTextLength(); 
			}
			if (isParagraph)
			{
				//paragraph nodes -> remove nodes if any (reversely)
				std::vector<sLONG>::const_reverse_iterator itRemove = toremove.rbegin();
				for(; itRemove != toremove.rend(); itRemove++)
				{
					RemoveNthChild( *itRemove);
					length = 1; //should be > 0 if text has been removed
				}
			}
			else if (toremove.size() > 0)
			{
				//container nodes -> remove text from marked nodes
				sLONG startRemove = toremove[0];
				sLONG endRemove = toremove.back();
				VectorOfNode::const_iterator itNodeRemove = fChildren.begin();
				std::advance( itNodeRemove, startRemove);
				VString sCR;
				sCR.AppendUniChar(13); //at least one CR
				for(; startRemove <= endRemove && itNodeRemove != fChildren.end(); itNodeRemove++, startRemove++)
				{
					node = itNodeRemove->Get();
					VIndex nodeLength = node->ReplaceStyledText( sCR, NULL, 0, node->GetTextLength()); //it will preserve uniform style which is correct 
					if (nodeLength)
						length = nodeLength;
				}
			}
		}

		//finally replace text in first node

		itNode = fChildren.begin();
		std::advance( itNode, index);
		node	= itNode->Get();
		start	= startBackup;

		bool isLast = !fParent || fChildIndex == fParent->fChildren.size()-1;
		VDocNode *parent = fParent;
		while (isLast && parent)
		{
			if (parent->fParent)
			{
				isLast = parent->fChildIndex == parent->fParent->fChildren.size()-1;
				parent = parent->fParent;
			}
		}

		if (!isLast && end >= node->GetTextStart()+node->GetTextLength())
		{
			//ensure paragraph (but the last one) is always terminated by CR
			if (textEnd.GetLength() > 0)
			{
				if (textEnd[textEnd.GetLength()-1] != 13)
				{
					textEnd.AppendUniChar(13);
					if (stylesEnd)
						stylesEnd->ExpandAtPosBy(textEnd.GetLength()-1,1);
				}
			}
			else if (inText.GetLength() <= 0 || inText[inText.GetLength()-1] != 13)
				textEnd.AppendUniChar(13);
		}

		if (textEnd.GetLength())
		{
			//concat whith plain text & styles 
			VString text(inText);
			text.AppendString(textEnd);

			VTreeTextStyle *styles = new VTreeTextStyle( new VTextStyle());
			styles->GetData()->SetRange( 0, text.GetLength());
			
			VTreeTextStyle *stylesCopy = inStyles ? new VTreeTextStyle( inStyles) : NULL;
			if (stylesCopy)
			{
				styles->AddChild( stylesCopy);
				ReleaseRefCountable(&stylesCopy);
			}
			if (stylesEnd)
			{
				if (inText.GetLength() > 0)
					stylesEnd->Translate(inText.GetLength());
				styles->AddChild( stylesEnd);
				ReleaseRefCountable(&stylesEnd);
			}

			VIndex nodeLength = node->ReplaceStyledText(	text, styles, start-node->GetTextStart(), end-node->GetTextStart(), 
															inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef);
			if (nodeLength)
				length = nodeLength;

			ReleaseRefCountable(&styles);
		}
		else
		{
			VIndex nodeLength = node->ReplaceStyledText(	inText, inStyles, start-node->GetTextStart(), end-node->GetTextStart(), 
															inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef);
			if (nodeLength)
				length = nodeLength;
		}
		return length;
	}
	return 0;
}


/** replace plain text with computed or source span references */
void VDocNode::UpdateSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs, VDBLanguageContext *inLC)
{
	VDocNodeChildrenReverseIterator it(this);
	VDocNode *node = it.First();
	while (node)
	{
		node->UpdateSpanRefs( inShowSpanRefs, inLC);
		node = it.Next();
	}
}


/** replace text with span text reference on the specified range (relative to node start offset)
@remarks
	span ref plain text is set here to uniform non-breaking space (on default): 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VDocNode::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inNoUpdateRef, VDBLanguageContext *inLC)
{
	if (fChildren.size() == 0)
		return false;
	
	sLONG start = inStart;
	sLONG end = inEnd;
	_NormalizeRange(start, end);

	if (end < start)
	{
		//node range does not include range to replace
		delete inSpanRef; //ensure we delete span ref as method takes ownership even if replacement failed
		return false;
	}

	xbox_assert(!inSpanRef->GetImage() || (inSpanRef->GetImage()->GetParent() == NULL && inSpanRef->GetImage()->GetDocumentNode() == NULL)); //if image, it should not be bound to a document yet

	VTextStyle *style = new VTextStyle();
	style->SetSpanRef( inSpanRef);
	if (inNoUpdateRef)
		style->GetSpanRef()->ShowRef( kSRTT_Uniform | (style->GetSpanRef()->ShowRef() & kSRTT_4DExp_Normalized));
	if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp && style->GetSpanRef()->ShowRefNormalized() && (style->GetSpanRef()->ShowRef() & kSRTT_Src))
		style->GetSpanRef()->UpdateRefNormalized( inLC);

	VString textSpanRef = style->GetSpanRef()->GetDisplayValue(); 
	style->SetRange(0, textSpanRef.GetLength()); 
	VTreeTextStyle *styles = new VTreeTextStyle( style);

	VIndex lengthPlusOne = ReplaceStyledText( textSpanRef, styles, inStart, inEnd, inAutoAdjustRangeWithSpanRef);
	ReleaseRefCountable(&styles);
	return lengthPlusOne > 0;
}

/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	(relative to node start offset)

	return true if any 4D expression has been replaced with plain text
*/
bool VDocNode::FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd)
{
	if (fChildren.size() == 0)
		return false;

	sLONG start = inStart;
	sLONG end = inEnd;
	_NormalizeRange(start, end);
	if (end < start)
		return false;

	bool result = false;
	VDocNodeChildrenReverseIterator it(this);
	VDocNode *node = it.First(start, end);
	while (node)
	{
		result = node->FreezeExpressions( inLC, start-node->GetTextStart(), end-node->GetTextStart()) || result;
		node = it.Next();
	}
	return result;
}

/** return the first span reference which intersects the passed range (relative to node start offset) */
const VTextStyle* VDocNode::GetStyleRefAtRange(sLONG inStart, sLONG inEnd, sLONG *outStartBase)
{
	if (fChildren.size() == 0)
		return NULL;

	sLONG start = inStart;
	sLONG end = inEnd;
	_NormalizeRange(start, end);
	if (end < start)
		return NULL;

	VDocNode *node = _GetChildNodeFromPos( start);
	if (node)
	{
		sLONG startBase = 0;
		const VTextStyle* style = node->GetStyleRefAtRange( start-node->GetTextStart(), end-node->GetTextStart(), &startBase);
		if (outStartBase)
			*outStartBase += startBase;
		return style;
	}
	return NULL;
}



void VDocNode::_UpdateTextInfo(bool inUpdateTextLength, bool inApplyOnlyDeltaTextLength, sLONG inDeltaTextLength)
{
	if (fParent)
	{
		xbox_assert(fParent->fChildren.size() > 0 && fParent->fChildren[fChildIndex].Get() == this);

		if (fChildIndex > 0)
		{
			VDocNode *prevNode = fParent->fChildren[fChildIndex-1].Get();
			fTextStart = prevNode->GetTextStart()+prevNode->GetTextLength();
		}
		else
			fTextStart = 0;
	}
	else
		fTextStart = 0;

	if (!inUpdateTextLength)
		return;

	if (fChildren.size())
	{
		if (inApplyOnlyDeltaTextLength && !fNoDeltaTextLength)
			fTextLength += inDeltaTextLength;
		else
		{
			fTextLength = 0;
			VectorOfNode::const_iterator itNode = fChildren.begin();
			for(;itNode != fChildren.end(); itNode++)
				fTextLength += itNode->Get()->GetTextLength();
		}
	}
}

void VDocNode::_UpdateTextInfoEx(bool inUpdateNextSiblings, bool inUpdateTextLength, bool inApplyOnlyDeltaTextLength, sLONG inDeltaTextLength)
{
	sLONG deltaTextLength = (sLONG)fTextLength;
	_UpdateTextInfo(inUpdateTextLength, inApplyOnlyDeltaTextLength, inDeltaTextLength);
	deltaTextLength = ((sLONG)fTextLength)-deltaTextLength;

	if (fParent)
	{
		//update siblings after this node
		if (inUpdateNextSiblings)
		{
			VectorOfNode::iterator itNode = fParent->fChildren.begin();
			std::advance( itNode, fChildIndex+1);
			for (;itNode != fParent->fChildren.end(); itNode++)
				(*itNode).Get()->_UpdateTextInfo(false);
		}

		fParent->_UpdateTextInfoEx( true, true, true, deltaTextLength);
	}
}


void VDocNode::_OnAttachToDocument(VDocNode *inDocumentNode)
{
	xbox_assert(inDocumentNode && inDocumentNode->fType == kDOC_NODE_TYPE_DOCUMENT);

	//update document reference
	if (!fDoc)
		fDoc = inDocumentNode;
	xbox_assert(fDoc == inDocumentNode);

	//update ID (to ensure this node & children nodes use unique IDs for this document)
	VDocNode *node = fID.IsEmpty() ? RetainRefCountable(fDoc) : RetainNode( fID);
	if (node) //generate new ID only if ID is used yet
	{
		ReleaseRefCountable(&node);
		_GenerateID(fID);
	}

	//update document map of node per ID
	(dynamic_cast<VDocText *>(fDoc))->fMapOfNodePerID[fID] = this;

	//update children
	VectorOfNode::iterator itNode = fChildren.begin();
	for(;itNode != fChildren.end(); itNode++)
		itNode->Get()->_OnAttachToDocument( inDocumentNode);

	fDirtyStamp++;
}

void VDocNode::_OnDetachFromDocument(VDocNode *inDocumentNode)
{
	VDocText *docText = dynamic_cast<VDocText *>(inDocumentNode);
	xbox_assert(docText && inDocumentNode == fDoc);

	fDoc = NULL;

	//remove from document map of node per ID
	VDocText::MapOfNodePerID::iterator itDocNode = docText->fMapOfNodePerID.find(fID);
	if (itDocNode != docText->fMapOfNodePerID.end())
		docText->fMapOfNodePerID.erase(itDocNode);

	//update children
	VectorOfNode::iterator itNode = fChildren.begin();
	for(;itNode != fChildren.end(); itNode++)
		itNode->Get()->_OnDetachFromDocument( inDocumentNode);
	
	fDirtyStamp++;
}

/** insert child node at passed position (0-based) */
void VDocNode::InsertChild( VDocNode *inNode, VIndex inIndex)
{
	if (!testAssert(inIndex >= 0 && inIndex <= fChildren.size()))
		return;

	if (inIndex >= fChildren.size())
	{
		AddChild( inNode);
		return;
	}

	if (!testAssert(inNode))
		return;

	VRefPtr<VDocNode> protect(inNode); //might be detached before attach (if moved) & only one ref

	if (inNode->fParent)
	{
		if (inNode->fParent == this)
			return;
		inNode->Detach();
	}

	if (inNode->fType == kDOC_NODE_TYPE_DOCUMENT)
	{
		//we add as a document fragment so we add only child nodes
		//(child nodes are detached from inNode prior to be attached to this node)

		sLONG index = inIndex;
		VIndex childCount = inNode->fChildren.size();
		for (int i = 0; i < childCount; i++)
		{
			xbox_assert( inNode->fChildren.size() >= 1);
			InsertChild( inNode->fChildren[0].Get(), index++); //first child is detached from inNode & attached to this node
		}
		xbox_assert( inNode->fChildren.size() == 0); //all inNode child nodes have been attached to this node
		return;
	}

	VectorOfNode::iterator itNode = fChildren.begin();
	std::advance(itNode, inIndex);
	fChildren.insert( itNode, VRefPtr<VDocNode>(inNode));

	inNode->fChildIndex = inIndex;
	itNode = fChildren.begin();
	std::advance( itNode, inIndex+1);
	for (;itNode != fChildren.end(); itNode++)
		(*itNode).Get()->fChildIndex++;

	inNode->fParent = this;
	inNode->fDoc = fDoc;
	fDirtyStamp++;

	fNoDeltaTextLength = true;
	inNode->_UpdateTextInfoEx(true);
	fNoDeltaTextLength = false;

	if (fDoc)
		inNode->_OnAttachToDocument( fDoc);
	else
		inNode->fDirtyStamp++;

	inNode->_UpdateListNumber( kDOC_LSTE_AFTER_ADD_TO_LIST);
}


/** add child node */
void VDocNode::AddChild( VDocNode *inNode)
{
	if (!testAssert(inNode))
		return;

	VRefPtr<VDocNode> protect(inNode); //might be detached before attach (if moved) & only one ref

	if (inNode->fParent)
	{
		if (inNode->fParent == this)
			return;
		inNode->Detach();
	}

	if (inNode->fType == kDOC_NODE_TYPE_DOCUMENT)
	{
		//we add as a document fragment so we add only child nodes
		//(child nodes are detached from inNode prior to be attached to this node)

		VIndex childCount = inNode->fChildren.size();
		for (int i = 0; i < childCount; i++)
		{
			xbox_assert( inNode->fChildren.size() >= 1);
			AddChild( inNode->fChildren[0].Get()); //first child is detached from inNode & attached to this node
		}
		xbox_assert( inNode->fChildren.size() == 0); //all inNode child nodes have been attached to this node
		return;
	}

	fChildren.push_back( VRefPtr<VDocNode>(inNode));
	inNode->fParent = this;
	inNode->fDoc = fDoc;
	inNode->fChildIndex = fChildren.size()-1;
	fDirtyStamp++;
	fNoDeltaTextLength = true;
	inNode->_UpdateTextInfoEx();
	fNoDeltaTextLength = false;
	
	if (fDoc)
		inNode->_OnAttachToDocument( fDoc);
	else
		inNode->fDirtyStamp++;

	inNode->_UpdateListNumber( kDOC_LSTE_AFTER_ADD_TO_LIST);
}

/** detach child node from parent */
void VDocNode::Detach()
{   
	if (!fParent)
		return;

	VRefPtr<VDocNode> protect(this); //might be last ref

	_UpdateListNumber( kDOC_LSTE_BEFORE_REMOVE_FROM_LIST);

	xbox_assert(fType != kDOC_NODE_TYPE_DOCUMENT);

	VectorOfNode::iterator itChild =  fParent->fChildren.begin();
	std::advance( itChild, fChildIndex);
	xbox_assert(itChild->Get() == this);
	fParent->fChildren.erase(itChild);
	itChild = fParent->fChildren.begin();
	std::advance(itChild, fChildIndex);
	for (;itChild != fParent->fChildren.end(); itChild++)
		itChild->Get()->fChildIndex--;

	fParent->fNoDeltaTextLength = true;
	itChild = fParent->fChildren.begin();
	std::advance(itChild, fChildIndex);
	if (itChild != fParent->fChildren.end())
		itChild->Get()->_UpdateTextInfoEx(true);
	else
		fParent->_UpdateTextInfoEx(true);
	fParent->fNoDeltaTextLength = false;

	fParent->fDirtyStamp++;
	fParent = NULL;
	if (fDoc)
		_OnDetachFromDocument( fDoc);
	else
		fDirtyStamp++;

	fChildIndex = 0;
}

/** set current ID (custom ID) */
bool VDocNode::SetID(const VString& inValue) 
{	
	if (fID.EqualToString( inValue))
		return true;
	
	if (inValue.IsEmpty())
	{
		//remove custom id -> replace id with automatic id
		if (!fID.BeginsWith( D4ID_PREFIX_RESERVED))
		{
			VDocText *doc = GetDocumentNode();
			if (doc)
			{
				//remove from document map of node per ID
				if (!fID.IsEmpty())
				{
					VDocText::MapOfNodePerID::iterator itDocNode = doc->fMapOfNodePerID.find(fID);
					if (itDocNode != doc->fMapOfNodePerID.end())
						doc->fMapOfNodePerID.erase(itDocNode);
				}

				//generate automatic ID
				_GenerateID( fID);

				//add to document map per ID
				if (fDoc != this)
					doc->fMapOfNodePerID[ fID] = this;
			}
			else
				_GenerateID(fID);
			fDirtyStamp++;
		}
		return true;
	}

	VDocNode *node = RetainNode(inValue);
	if (node)
	{
		xbox_assert(false); //ID is used yet (!)
		ReleaseRefCountable(&node);
		return false;
	}

	VDocText *doc = GetDocumentNode();
	if (doc && !fID.IsEmpty())
	{
		VDocText::MapOfNodePerID::iterator itDocNode = doc->fMapOfNodePerID.find(fID);
		if (itDocNode != doc->fMapOfNodePerID.end())
			doc->fMapOfNodePerID.erase(itDocNode);
	}

	fID = inValue; 

	if (doc && fDoc != this)
		doc->fMapOfNodePerID[ fID] = this;

	fDirtyStamp++; 
	return true;
}

/** return true if it is the last child node
@param inLoopUp
	true: return true if and only if it is the last child node at this level and up to the top-level parent
	false: return true if it is the last child node relatively to the node parent
*/
bool VDocNode::IsLastChild(bool inLookUp) const
{
	if (!fParent)
		return true;
	if (fChildIndex != fParent->fChildren.size()-1)
		return false;
	if (!inLookUp)
		return true;
	return fParent->IsLastChild( true);
}


/** force a property to be inherited */
void VDocNode::SetInherit( const eDocProperty inProp)
{
	bool isDirty = false;

	//remove local definition if any
	MapOfProp::iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		if (it->second.IsInherited())
			return;
		fProps.erase(it);
		isDirty = true;
	}

	//if property is not inherited on default, force inherit
	if (!sPropInherited[(uLONG)inProp])
	{
		fProps[ inProp] = VDocProperty();
		isDirty = true;
	}

	if (isDirty)
		fDirtyStamp++;
}

/** return true if property value is inherited */ 
bool VDocNode::IsInherited( const eDocProperty inProp) const
{
	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		if (it->second.IsInherited())
			return true;
		return false;
	}

	return (sPropInherited[(uLONG)inProp]);
}

/** return true if property value is inherited from default value */ 
bool VDocNode::IsInheritedDefault( const eDocProperty inProp) const
{
	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		if (it->second.IsInherited())
		{
			if (fParent)
				return fParent->IsInheritedDefault( inProp);
			else
				return true;
		}
		return false;
	}
	if (fParent && sPropInherited[(uLONG)inProp])
		return fParent->IsInheritedDefault( inProp);
	else
		return true;
}

/** return true if property value is overriden locally */ 
bool VDocNode::IsOverriden( const eDocProperty inProp, bool inRecurseUp) const
{
	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
		return true;
	else if (inRecurseUp && fParent)
		return fParent->IsOverriden( inProp);
	else
		return false;
}

/** remove local property */
void VDocNode::RemoveProp( const eDocProperty inProp)
{
	if (inProp == kDOC_PROP_LIST_STYLE_TYPE && IsOverriden(kDOC_PROP_LIST_STYLE_TYPE))
		_UpdateListNumber( kDOC_LSTE_BEFORE_REMOVE_FROM_LIST);

	//remove local definition if any
	MapOfProp::iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		fProps.erase(it);
		
		if (inProp == kDOC_PROP_LIST_START)
			_UpdateListNumber( kDOC_LSTE_AFTER_UPDATE_START_NUMBER); 

		fDirtyStamp++;
	}
}


const VDocProperty& VDocNode::GetProperty(const eDocProperty inProp) const
{
	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
		return it->second;
	return (*(sPropDefault[inProp]));
}

const VDocProperty&	VDocNode::GetPropertyInherited(const eDocProperty inProp) const
{
	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		if (it->second.IsInherited())
		{
			if (fParent)
				return fParent->GetPropertyInherited(inProp);
			else
				return (*(sPropDefault[inProp]));
		}
		else
			return it->second;
	}
	if (fParent && sPropInherited[(uLONG)inProp])
		return fParent->GetPropertyInherited(inProp);
	else
		return (*(sPropDefault[inProp]));
}

/** remove all properties */
void VDocNode::ClearAllProps()
{
	if (IsOverriden(kDOC_PROP_LIST_STYLE_TYPE))
		_UpdateListNumber( kDOC_LSTE_BEFORE_REMOVE_FROM_LIST);

	fProps.clear();
	fLang.SetEmpty();
	fDirtyStamp++;
}


/** return true if property needs to be serialized 
@remarks
	it is used by serializer to serialize only properties which are not equal to the inherited value
	or to the default value for not inherited properties
*/
bool VDocNode::ShouldSerializeProp( const eDocProperty inProp, const VDocNode *inDefault) const
{
	if (inDefault)
	{
		if (inDefault->IsOverriden( inProp))
			return GetProperty( inProp) != inDefault->GetProperty( inProp);
	}

	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		if (it->second.IsInherited())
			return !sPropInherited[(uLONG)inProp];
		else if (fParent && sPropInherited[(uLONG)inProp])	
			return fParent->GetPropertyInherited(inProp) != it->second;
		else
			return it->second != *(sPropDefault[inProp]);
	}
	return false;
}


/** get default property value */
const VDocProperty& VDocNode::GetDefaultPropValue( const eDocProperty inProp) 
{
	if (testAssert(inProp >= 0 && inProp <= kDOC_PROP_COUNT))
		return *(sPropDefault[ (uLONG)inProp]);
	else
		return *(sPropDefault[ (uLONG)0]);
}


eDocPropTextAlign VDocNode::GetTextAlign() const 
{ 
	uLONG textAlign = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_TEXT_ALIGN);
	if (testAssert(textAlign >= JST_Default && textAlign <= JST_Justify))
		return (eDocPropTextAlign)textAlign;
	else
		return JST_Default;
}
void VDocNode::SetTextAlign(const eDocPropTextAlign inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_TEXT_ALIGN, (uLONG)inValue); }

eDocPropTextAlign VDocNode::GetVerticalAlign() const 
{ 
	uLONG textAlign = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_VERTICAL_ALIGN);
	if (testAssert(textAlign >= JST_Default && textAlign <= JST_Subscript))
		return (eDocPropTextAlign)textAlign;
	else
		return JST_Default;
}
void VDocNode::SetVerticalAlign(const eDocPropTextAlign inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_VERTICAL_ALIGN, (uLONG)inValue); }

const sDocPropRect& VDocNode::GetMargin() const { return IDocProperty::GetPropertyRef<sDocPropRect>( this, kDOC_PROP_MARGIN); }
void VDocNode::SetMargin(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( this, kDOC_PROP_MARGIN, inValue); }

const sDocPropRect& VDocNode::GetPadding() const { return IDocProperty::GetPropertyRef<sDocPropRect>( this, kDOC_PROP_PADDING); }
void VDocNode::SetPadding(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( this, kDOC_PROP_PADDING, inValue); }

RGBAColor VDocNode::GetBackgroundColor() const { return IDocProperty::GetProperty<RGBAColor>( this, kDOC_PROP_BACKGROUND_COLOR); }
void VDocNode::SetBackgroundColor(const RGBAColor inValue) { IDocProperty::SetProperty<RGBAColor>( this, kDOC_PROP_BACKGROUND_COLOR, inValue); }

eDocPropBackgroundBox VDocNode::GetBackgroundClip() const 
{ 
	uLONG clip = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_BACKGROUND_CLIP);
	if (testAssert(clip >= kDOC_BACKGROUND_BORDER_BOX && clip <= kDOC_BACKGROUND_CONTENT_BOX))
		return (eDocPropBackgroundBox)clip;
	else 
		return kDOC_BACKGROUND_BORDER_BOX;
}
void VDocNode::SetBackgroundClip(const eDocPropBackgroundBox inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_BACKGROUND_CLIP, (uLONG)inValue); }

const VString& VDocNode::GetBackgroundImage() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_BACKGROUND_IMAGE); }
void VDocNode::SetBackgroundImage(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_BACKGROUND_IMAGE, inValue); }

eDocPropTextAlign VDocNode::GetBackgroundImageHAlign() const 
{ 
	sLONG halign = (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_POSITION))[0]; 
	if (testAssert(halign >= JST_Notset && halign <= JST_Subscript))
		return (eDocPropTextAlign)halign;
	else
		return JST_Left;
}
void VDocNode::SetBackgroundImageHAlign(const eDocPropTextAlign inValue) 
{
	VDocProperty::VectorOfSLONG position = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_POSITION);
	position[0] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_POSITION, position); 
}


eDocPropTextAlign VDocNode::GetBackgroundImageVAlign() const 
{ 
	sLONG valign = (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_POSITION))[1]; 
	if (testAssert(valign >= JST_Notset && valign <= JST_Subscript))
		return (eDocPropTextAlign)valign;
	else
		return JST_Top;
}
void VDocNode::SetBackgroundImageVAlign(const eDocPropTextAlign inValue) 
{
	VDocProperty::VectorOfSLONG position = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_POSITION);
	position[1] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_POSITION, position); 
}

eDocPropBackgroundRepeat VDocNode::GetBackgroundImageRepeat() const 
{ 
	uLONG repeat = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_BACKGROUND_REPEAT);
	if (testAssert(repeat >= kDOC_BACKGROUND_REPEAT && repeat <= kDOC_BACKGROUND_NO_REPEAT))
		return (eDocPropBackgroundRepeat)repeat;
	else 
		return kDOC_BACKGROUND_REPEAT;
}
void VDocNode::SetBackgroundImageRepeat(const eDocPropBackgroundRepeat inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_BACKGROUND_REPEAT, (uLONG)inValue); }

eDocPropBackgroundBox VDocNode::GetBackgroundImageOrigin() const 
{ 
	uLONG origin = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_BACKGROUND_ORIGIN);
	if (testAssert(origin >= kDOC_BACKGROUND_BORDER_BOX && origin <= kDOC_BACKGROUND_CONTENT_BOX))
		return (eDocPropBackgroundBox)origin;
	else 
		return kDOC_BACKGROUND_PADDING_BOX;
}
void VDocNode::SetBackgroundImageOrigin(const eDocPropBackgroundBox inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_BACKGROUND_ORIGIN, (uLONG)inValue); }

sLONG VDocNode::GetBackgroundImageWidth() const 
{ 
	return (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_SIZE))[0]; 
}
void VDocNode::SetBackgroundImageWidth(const sLONG inValue) 
{
	VDocProperty::VectorOfSLONG size = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_SIZE);
	size[0] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_SIZE, size); 
}

sLONG VDocNode::GetBackgroundImageHeight() const 
{ 
	return (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_SIZE))[1]; 
}
void VDocNode::SetBackgroundImageHeight(const sLONG inValue) 
{
	VDocProperty::VectorOfSLONG size = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_SIZE);
	size[1] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( this, kDOC_PROP_BACKGROUND_SIZE, size); 
}

uLONG VDocNode::GetWidth() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_WIDTH); }
void VDocNode::SetWidth(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_WIDTH, inValue); }

uLONG VDocNode::GetMinWidth() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_MIN_WIDTH); }
void VDocNode::SetMinWidth(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_MIN_WIDTH, inValue); }

uLONG VDocNode::GetHeight() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_HEIGHT); }
void VDocNode::SetHeight(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_HEIGHT, inValue); }

uLONG VDocNode::GetMinHeight() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_MIN_HEIGHT); }
void VDocNode::SetMinHeight(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_MIN_HEIGHT, inValue); }

const sDocPropRect& VDocNode::GetBorderStyle() const { return IDocProperty::GetPropertyRef<sDocPropRect>( this, kDOC_PROP_BORDER_STYLE); }
void VDocNode::SetBorderStyle(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( this, kDOC_PROP_BORDER_STYLE, inValue); }

const sDocPropRect& VDocNode::GetBorderWidth() const { return IDocProperty::GetPropertyRef<sDocPropRect>( this, kDOC_PROP_BORDER_WIDTH); }
void VDocNode::SetBorderWidth(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( this, kDOC_PROP_BORDER_WIDTH, inValue); }

const sDocPropRect& VDocNode::GetBorderColor() const { return IDocProperty::GetPropertyRef<sDocPropRect>( this, kDOC_PROP_BORDER_COLOR); }
void VDocNode::SetBorderColor(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( this, kDOC_PROP_BORDER_COLOR, inValue); }

const sDocPropRect& VDocNode::GetBorderRadius() const { return IDocProperty::GetPropertyRef<sDocPropRect>( this, kDOC_PROP_BORDER_RADIUS); }
void VDocNode::SetBorderRadius(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( this, kDOC_PROP_BORDER_RADIUS, inValue); }

eDocPropDirection VDocNode::GetDirection() const 
{ 
	uLONG direction = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_DIRECTION); 
	if (direction >= kDOC_DIRECTION_LTR && direction <= kDOC_DIRECTION_RTL)
		return (eDocPropDirection)direction;
	else
		return kDOC_DIRECTION_LTR;
}
void VDocNode::SetDirection(const eDocPropDirection inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_DIRECTION, (uLONG)inValue); }

sLONG VDocNode::GetLineHeight() const { return IDocProperty::GetProperty<sLONG>( this, kDOC_PROP_LINE_HEIGHT); }

void VDocNode::SetLineHeight(const sLONG inValue) { IDocProperty::SetProperty<sLONG>( this, kDOC_PROP_LINE_HEIGHT, inValue); }

sLONG VDocNode::GetTextIndent() const { return IDocProperty::GetProperty<sLONG>( this, kDOC_PROP_TEXT_INDENT); }

void VDocNode::SetTextIndent(const sLONG inValue) { IDocProperty::SetProperty<sLONG>( this, kDOC_PROP_TEXT_INDENT, inValue); }

uLONG VDocNode::GetTabStopOffset() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_TAB_STOP_OFFSET); }

void VDocNode::SetTabStopOffset(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_TAB_STOP_OFFSET, inValue); }

eDocPropTabStopType VDocNode::GetTabStopType() const 
{ 
	uLONG type = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_TAB_STOP_TYPE); 
	if (testAssert(type >= kDOC_TAB_STOP_TYPE_LEFT && type <= kDOC_TAB_STOP_TYPE_BAR))
		return (eDocPropTabStopType)type;
	else
		return kDOC_TAB_STOP_TYPE_LEFT;
}
void VDocNode::SetTabStopType(const eDocPropTabStopType inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_TAB_STOP_TYPE, (uLONG)inValue); }


eDocPropListStyleType	VDocNode::GetListStyleType() const
{
	uLONG type = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_LIST_STYLE_TYPE); 
	if (testAssert(type >= kDOC_LIST_STYLE_TYPE_FIRST && type <= kDOC_LIST_STYLE_TYPE_LAST))
		return (eDocPropListStyleType)type;
	else
		return kDOC_LIST_STYLE_TYPE_FIRST;
}

void VDocNode::SetListStyleType(const eDocPropListStyleType inValue) 
{
	eDocPropListStyleType typeBefore = GetListStyleType();
	if (inValue == typeBefore)
		return;

	_UpdateListNumber( kDOC_LSTE_BEFORE_REMOVE_FROM_LIST);
 
	IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_LIST_STYLE_TYPE, (uLONG)inValue); 

	_UpdateListNumber( kDOC_LSTE_AFTER_ADD_TO_LIST);
}

const VString&	VDocNode::GetListStringFormatLTR() const
{
	return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_LIST_STRING_FORMAT_LTR);
}
void VDocNode::SetListStringFormatLTR(const VString& inValue)
{
	IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_LIST_STRING_FORMAT_LTR, inValue);
}

const VString&	VDocNode::GetListStringFormatRTL() const
{
	return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_LIST_STRING_FORMAT_RTL);
}
void VDocNode::SetListStringFormatRTL(const VString& inValue)
{
	IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_LIST_STRING_FORMAT_RTL, inValue);
}


const VString&	VDocNode::GetListFontFamily() const
{
	return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_LIST_FONT_FAMILY);
}


void VDocNode::SetListFontFamily(const VString& inValue)
{
	IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_LIST_FONT_FAMILY, inValue);
}

const VString&	VDocNode::GetListStyleImage() const
{
	return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_LIST_STYLE_IMAGE);
}

void VDocNode::SetListStyleImage(const VString& inValue)
{
	_UpdateListNumber( kDOC_LSTE_BEFORE_REMOVE_FROM_LIST);
 
	IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_LIST_STYLE_IMAGE, inValue);

	_UpdateListNumber( kDOC_LSTE_AFTER_ADD_TO_LIST);

	if (!inValue.IsEmpty() && GetListStyleType() == kDOC_LIST_STYLE_TYPE_NONE)
		SetListStyleType(kDOC_LIST_STYLE_TYPE_DISC); //use disc if image is not found
}

uLONG VDocNode::GetListStyleImageHeight() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT); }

void VDocNode::SetListStyleImageHeight(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT, inValue); }

sLONG VDocNode::GetListStart() const { return IDocProperty::GetProperty<sLONG>( this, kDOC_PROP_LIST_START); }

void VDocNode::SetListStart(const sLONG inValue) 
{ 
	IDocProperty::SetProperty<sLONG>( this, kDOC_PROP_LIST_START, inValue);
	
	_UpdateListNumber( kDOC_LSTE_AFTER_UPDATE_START_NUMBER); 
}


const VString& VDocNode::GetNewParaClass() const
{
	return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_NEW_PARA_CLASS);
}

void VDocNode::SetNewParaClass( const VString& inClass)
{
	IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_NEW_PARA_CLASS, inClass);
}


//span (character only) properties

const VString& VDocNode::GetFontFamily() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_FONT_FAMILY); }
void VDocNode::SetFontFamily(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_FONT_FAMILY, inValue); }

Real VDocNode::GetFontSize() const { return IDocProperty::GetProperty<Real>( this, kDOC_PROP_FONT_SIZE); }
void VDocNode::SetFontSize(const Real inValue) { if (testAssert(inValue > 0)) IDocProperty::SetProperty<Real>( this, kDOC_PROP_FONT_SIZE, inValue); }

RGBAColor VDocNode::GetColor() const { return IDocProperty::GetProperty<RGBAColor>( this, kDOC_PROP_COLOR); }
void VDocNode::SetColor(const RGBAColor inValue) { IDocProperty::SetProperty<RGBAColor>( this, kDOC_PROP_COLOR, inValue); }

eFontStyle VDocNode::GetFontStyle() const 
{
	uLONG type = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_FONT_STYLE); 
	if (testAssert(type >= kFONT_STYLE_NORMAL && type <= kFONT_STYLE_OBLIQUE))
		return (eFontStyle)type;
	else
		return kFONT_STYLE_NORMAL;
}
void VDocNode::SetFontStyle(const eFontStyle inValue) {IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_FONT_STYLE, inValue); }

eFontWeight VDocNode::GetFontWeight() const 
{
	uLONG type = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_FONT_WEIGHT); 
	if (type >= kFONT_WEIGHT_MIN && type <= kFONT_WEIGHT_MAX) //here 0 is valid & converted to kFONT_WEIGHT_NORMAL
		return (eFontWeight)type;
	else
		return kFONT_WEIGHT_NORMAL;
}
void VDocNode::SetFontWeight(const eFontWeight inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_FONT_WEIGHT, inValue); }

uLONG VDocNode::GetTextDecoration() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_TEXT_DECORATION); }
void VDocNode::SetTextDecoration(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_TEXT_DECORATION, inValue); }

RGBAColor VDocNode::GetTextBackgroundColor() const { return IDocProperty::GetProperty<RGBAColor>( this, kDOC_PROP_TEXT_BACKGROUND_COLOR); }
void VDocNode::SetTextBackgroundColor(const RGBAColor inValue) { IDocProperty::SetProperty<RGBAColor>( this, kDOC_PROP_TEXT_BACKGROUND_COLOR, inValue); }

void VDocNode::_NormalizeRange(sLONG& ioStart, sLONG& ioEnd) const
{
	if (ioStart < 0)
		ioStart = 0;
	else if (ioStart > GetTextLength())
		ioStart = GetTextLength();

	if (ioEnd < 0)
		ioEnd = GetTextLength();
	else if (ioEnd > GetTextLength())
		ioEnd = GetTextLength();
	
	xbox_assert(ioStart <= ioEnd);
}



void VDocNode::GetAllMargins(uLONG& outMarginWidth, uLONG& outMarginHeight) const
{
	XBOX::sDocPropRect margin = GetMargin();
	XBOX::sDocPropRect padding = GetPadding();
	XBOX::sDocPropRect borderwidth = GetBorderWidth();

	outMarginWidth = margin.left+margin.right+padding.left+padding.right+borderwidth.left+borderwidth.right;
	outMarginHeight = margin.top+margin.bottom+padding.top+padding.bottom+borderwidth.top+borderwidth.bottom;
}


void VDocNode::AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfNotOverriden, bool inNoAppendSpanStyles, sLONG inStart, sLONG inEnd, uLONG inClassStyleTarget, bool inClassStyleOverrideOnlyUniformCharStyles, bool inOnlyIfNotOverridenRecurseUp)
{
	_NormalizeRange( inStart, inEnd);

	if (GetType() != inNode->GetType() && GetType() != kDOC_NODE_TYPE_CLASS_STYLE)
	{
		//recurse down on the passed range

		if (inNode->GetType() != kDOC_NODE_TYPE_CLASS_STYLE || GetType() != inClassStyleTarget)
		{
			VDocNodeChildrenIterator it(this);
			VDocNode *node = it.First(inStart, inEnd);
			while (node)
			{
				node->AppendPropsFrom( inNode, inOnlyIfNotOverriden, inNoAppendSpanStyles, inStart-node->GetTextStart(), inEnd-node->GetTextStart(), inClassStyleTarget, inClassStyleOverrideOnlyUniformCharStyles, inOnlyIfNotOverridenRecurseUp);
				node = it.Next();
			}
			return;
		}
	}

	bool isDirty = false;
	MapOfProp::const_iterator it = inNode->fProps.begin();
	for (;it != inNode->fProps.end(); it++)  
	{
		eDocProperty prop = (eDocProperty)it->first;
		if (inNoAppendSpanStyles && prop >= kDOC_PROP_FONT_FAMILY)
			continue;

		if (fType == kDOC_NODE_TYPE_CLASS_STYLE && prop == kDOC_PROP_LIST_START)
			continue; //we should never store in class style list start number

		if (inOnlyIfNotOverriden)
		{
			if (!IsOverriden( prop, inOnlyIfNotOverridenRecurseUp))
			{
				fProps[prop] = it->second;
				isDirty = true;
			}
		}
		else
		{
			fProps[prop] = it->second;
			isDirty = true;
		}
	}

	if (!inNoAppendSpanStyles && GetType() == kDOC_NODE_TYPE_CLASS_STYLE && inNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		//this node is a class style -> import paragraph uniform character styles as character properties
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(inNode);
		xbox_assert(para);
		if (para->GetStyles())
			VTextStyleCopyToCharProps(*(para->GetStyles()->GetData()),true); 
	}

	if (isDirty)
		fDirtyStamp++;
}


void VDocNode::CharPropsCopyToVTextStyle( VTextStyle& outStyle, bool inOverrideStyles, bool inInheritedDefault) const
{
	if (inOverrideStyles || outStyle.GetFontName().IsEmpty())
	{
		if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_FONT_FAMILY))
		{
			VString fontname = GetFontFamily();
			if (!fontname.IsEmpty())
				outStyle.SetFontName( fontname);
		}
	}
	if (inOverrideStyles || outStyle.GetFontSize() < 0)
	{
		if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_FONT_SIZE))
		{
			Real fontSize = GetFontSize();
			if (fontSize > 0)	
				outStyle.SetFontSize( fontSize);
		}
	}
	if (inOverrideStyles || outStyle.GetItalic() < 0)
	{
		if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_FONT_STYLE))
		{
			eFontStyle fontStyle = GetFontStyle();
			outStyle.SetItalic( fontStyle != kFONT_STYLE_NORMAL ? 1 : 0);
		}
	}
	if (inOverrideStyles || outStyle.GetBold() < 0)
	{
		if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_FONT_WEIGHT))
		{
			eFontWeight fontWeight = GetFontWeight();
			outStyle.SetBold( fontWeight >= kFONT_WEIGHT_BOLD ? 1 : 0);
		}
	}
	if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_TEXT_DECORATION))
	{
		uLONG textDeco = GetTextDecoration();
		if (inOverrideStyles || outStyle.GetUnderline() < 0)
			outStyle.SetUnderline( (textDeco & kTEXT_DECORATION_UNDERLINE) ? 1 : 0);
		if (inOverrideStyles || outStyle.GetStrikeout() < 0)
			outStyle.SetStrikeout( (textDeco & kTEXT_DECORATION_LINETHROUGH) ? 1 : 0);
	}
	if (inOverrideStyles || !outStyle.GetHasForeColor())
	{
		if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_COLOR))
		{
			RGBAColor color = GetColor();
			outStyle.SetHasForeColor(true);
			outStyle.SetColor( color);
		}
	}
	if (inOverrideStyles || !outStyle.GetHasBackColor())
	{
		if (inInheritedDefault || !IsInheritedDefault(kDOC_PROP_TEXT_BACKGROUND_COLOR))
		{
			RGBAColor color = GetTextBackgroundColor();
			outStyle.SetHasBackColor(true);
			outStyle.SetBackGroundColor( color);
		}
	}
}

void VDocNode::CharPropsMoveToVTextStyle( VTextStyle& outStyle, bool inOverrideStyles, bool inInheritedDefault)
{
	CharPropsCopyToVTextStyle( outStyle, inOverrideStyles, inInheritedDefault);
	RemoveTextCharacterProperties();
}


/** remove text character properties */
void VDocNode::RemoveTextCharacterProperties()
{
	RemoveProp( kDOC_PROP_FONT_FAMILY);
	RemoveProp( kDOC_PROP_FONT_SIZE);
	RemoveProp( kDOC_PROP_FONT_STYLE);
	RemoveProp( kDOC_PROP_FONT_WEIGHT);
	RemoveProp( kDOC_PROP_TEXT_DECORATION);
	RemoveProp( kDOC_PROP_COLOR);
	RemoveProp( kDOC_PROP_TEXT_BACKGROUND_COLOR);
}


/** convert VTextStyle to text character properties */
void VDocNode::VTextStyleCopyToCharProps( const VTextStyle& inStyle, bool inOverrideProps) 
{
	if (!inStyle.GetFontName().IsEmpty())
	{
		if (inOverrideProps || !IsOverriden(kDOC_PROP_FONT_FAMILY))
			SetFontFamily( inStyle.GetFontName());
	}
	if (inStyle.GetFontSize() >= 0)
	{
		if (inOverrideProps || !IsOverriden(kDOC_PROP_FONT_SIZE))
			SetFontSize( inStyle.GetFontSize() > 0 ? inStyle.GetFontSize() : 12);
	}
	if (inStyle.GetItalic() >= 0)
	{
		if (inOverrideProps || !IsOverriden(kDOC_PROP_FONT_STYLE))
			SetFontStyle( inStyle.GetItalic() > 0 ? kFONT_STYLE_ITALIC : kFONT_STYLE_NORMAL);
	}
	if (inStyle.GetBold() >= 0)
	{
		if (inOverrideProps || (!IsOverriden(kDOC_PROP_FONT_WEIGHT)))
			SetFontWeight( inStyle.GetBold() > 0 ? kFONT_WEIGHT_BOLD : kFONT_WEIGHT_NORMAL);
	}
	sLONG decoration = -1;
	if (inStyle.GetUnderline() >= 0)
	{
		if (inOverrideProps || (!IsOverriden(kDOC_PROP_TEXT_DECORATION)))
			decoration = inStyle.GetUnderline() > 0 ? kTEXT_DECORATION_UNDERLINE : kTEXT_DECORATION_NONE;
	}
	if (inStyle.GetStrikeout() >= 0)
	{
		if (inOverrideProps || (!IsOverriden(kDOC_PROP_TEXT_DECORATION)))
		{
			if (decoration < 0)
				decoration = inStyle.GetStrikeout() > 0 ? kTEXT_DECORATION_LINETHROUGH : kTEXT_DECORATION_NONE;
			else
				decoration = decoration | (inStyle.GetStrikeout() > 0 ? kTEXT_DECORATION_LINETHROUGH : 0);
		}
	}
	if (decoration >= 0)
		SetTextDecoration( (uLONG)decoration);

	if (inStyle.GetHasForeColor())
	{
		if (inOverrideProps || (!IsOverriden(kDOC_PROP_COLOR)))
			SetColor( inStyle.GetColor());
	}
	if (inStyle.GetHasBackColor())
	{
		if (inOverrideProps || (!IsOverriden(kDOC_PROP_TEXT_BACKGROUND_COLOR)))
			SetTextBackgroundColor( inStyle.GetBackGroundColor());
	}
}


/** create JSONDocProps object
@remarks
	should be called only if needed (for instance if caller need com with 4D language through C_OBJECT)
	caller might also get/set properties using the JSON object returned by RetainJSONProps
*/
VJSONDocProps *VDocNode::CreateAndRetainJSONProps()
{
	return new VJSONDocProps( this);
}


VJSONDocProps::VJSONDocProps(VDocNode *inNode):VJSONObject() 
{ 
	xbox_assert(inNode); 
	fDocNode = inNode; 
}


// if inValue is not JSON_undefined, SetProperty sets the property named inName with specified value, replacing previous one if it exists.
// else the property is removed from the collection.
bool VJSONDocProps::SetProperty( const VString& inName, const VJSONValue& inValue) 
{
	//TODO
	return false;
}

// remove the property named inName.
// equivalent to SetProperty( inName, VJSONValue( JSON_undefined));
void VJSONDocProps::RemoveProperty( const VString& inName)
{
	//TODO
	return;
}

// Remove all properties.
void VJSONDocProps::Clear()
{
	//TODO
}


VDocText::VDocText(const VDocClassStyleManager *inCSMgr):VDocNode() 
{
	fType = kDOC_NODE_TYPE_DOCUMENT;
	fDoc = static_cast<VDocNode *>(this);
	fParent = NULL;
	fNextIDCount = 1;
	_GenerateID(fID);
	if (inCSMgr)
		fClassStyleManager = VRefPtr<VDocClassStyleManager>(new VDocClassStyleManager( *inCSMgr, this), false);
	else
		fClassStyleManager = VRefPtr<VDocClassStyleManager>(new VDocClassStyleManager(this), false);
}

VDocText::VDocText( const VDocText* inNodeText):VDocNode(static_cast<const VDocNode *>(inNodeText))	
{
	fDoc = static_cast<VDocNode *>(this);
	fNextIDCount = inNodeText->fNextIDCount;
	
	VectorOfNode::const_iterator it = inNodeText->fChildren.begin();
	for (;it != inNodeText->fChildren.end(); it++)
	{
		VDocNode *node = it->Get()->Clone();
		if (node)
		{
			AddChild( node);
			node->Release();
		}
	}

	fLang = inNodeText->fLang;

	if (inNodeText->fImages.size())
	{
		VectorOfImage::const_iterator itNode = inNodeText->fImages.begin();
		for(;itNode != inNodeText->fImages.end(); itNode++)
		{
			fImages.push_back( VRefPtr<VDocImage>(dynamic_cast<VDocImage *>(itNode->Get()->Clone()), false));
			xbox_assert(fImages.back().Get() != NULL);
		}
	}
	
	if (inNodeText->fClassStyleManager.Get())
		fClassStyleManager = VRefPtr<VDocClassStyleManager>(new VDocClassStyleManager( *(inNodeText->fClassStyleManager.Get()), this), false);
	else
		fClassStyleManager = VRefPtr<VDocClassStyleManager>(new VDocClassStyleManager(this), false);
}


bool VDocClassStyle::operator == (const VDocClassStyle& inStyle)
{
	if (this == &inStyle)
		return true;
	if (fProps.size() != inStyle.fProps.size())
		return false;
	MapOfProp::const_iterator it = fProps.begin();
	MapOfProp::const_iterator it2 = inStyle.fProps.begin();
	for (;it != fProps.end(); it++, it2++)
	{
		if (it->first != it2->first)
			return false;
		if (it->second != it2->second)
			return false;
	}
	return true;
}

VDocClassStyleManager::VDocClassStyleManager(const VDocClassStyleManager& inManager, VDocText *inDocument):VObject(), IRefCountable()
{
	fDoc = inDocument;
	if (inManager.fClassStyles.size())
	{
		MapOfClassStylePerType::const_iterator itMap = inManager.fClassStyles.begin();
		for(;itMap != inManager.fClassStyles.end(); itMap++)
		{
			fClassStyles[itMap->first] = MapOfStylePerClass();
			MapOfStylePerClass& map = fClassStyles[itMap->first];
			const MapOfStylePerClass &inMap = itMap->second;
			
			MapOfStylePerClass::const_iterator itMap2 = inMap.begin();
			for(;itMap2 != inMap.end(); itMap2++)
			{
				VDocNode *node = itMap2->second.Get()->Clone();
				xbox_assert(node);
				VDocClassStyle *nodeStyle = dynamic_cast<VDocClassStyle *>(node);
				xbox_assert(nodeStyle);
				map[itMap2->first] = VRefPtr<VDocClassStyle>(nodeStyle, false);
				if (fDoc)
					nodeStyle->_OnAttachToDocument( static_cast<VDocNode *>(fDoc));
			}
		}
	}
}

void VDocClassStyleManager::AddOrReplaceClassStyle(eDocNodeType inType, const VString& inClass, VDocClassStyle *inNode, bool inRemoveDefaultStyles)
{
	if (!testAssert(inType >= kDOC_NODE_TYPE_DOCUMENT && inType < kDOC_NODE_TYPE_CLASS_STYLE))
		//we can only style per class nodes but class style node
		return;

	xbox_assert( inNode && !inNode->GetParent());
	
	if (inRemoveDefaultStyles && !inClass.IsEmpty())
	{
		const VDocClassStyle *defaultStyle = RetainClassStyle( inType, CVSTR(""));
		if (defaultStyle)
		{
			//remove default style props
			for (int i = 0; i < kDOC_PROP_COUNT; i++)
			{
				eDocProperty prop = (eDocProperty)i;
				if (inNode->IsOverriden( prop) && defaultStyle->IsOverriden( prop) && defaultStyle->GetProperty(prop) == inNode->GetProperty(prop))
					inNode->RemoveProp( prop);
			}
			ReleaseRefCountable(&defaultStyle);
		}
	}

	if (fDoc)
	{
		VDocClassStyle *style = RetainClassStyle( inType, inClass);
		if (style)
		{
			if (style == inNode)
			{
				ReleaseRefCountable(&style);
				return;
			}
			style->_OnDetachFromDocument( static_cast<VDocNode *>(fDoc));
			ReleaseRefCountable(&style);
		}
	}

	inNode->SetElementName( inType);
	inNode->SetClass( inClass);
	fClassStyles[inType][inClass] = VRefPtr<VDocClassStyle>(inNode);
	if (fDoc)
		inNode->_OnAttachToDocument( static_cast<VDocNode *>(fDoc));
}

VDocClassStyle*	VDocClassStyleManager::_RetainWithDefaultStyles( eDocNodeType inType, VDocClassStyle *inNode) const
{
	if (!inNode->GetClass().IsEmpty())
	{
		const VDocClassStyle *defaultStyle = RetainClassStyle( inType, CVSTR(""));
		if (defaultStyle)
		{
			//append default style props
			VDocClassStyle *csClone = dynamic_cast<VDocClassStyle *>(inNode->Clone());
			xbox_assert(csClone);
			csClone->AppendPropsFrom( defaultStyle, true, false);
			ReleaseRefCountable(&defaultStyle);
			return csClone;
		}
	}
	return RetainRefCountable( inNode);
}


/** retain a class style */
VDocClassStyle* VDocClassStyleManager::RetainClassStyle(eDocNodeType inType, const VString& inClass, bool inAppendDefaultStyles) const
{
	MapOfClassStylePerType::const_iterator itMap = fClassStyles.find(inType);
	if (itMap == fClassStyles.end())
		return NULL;
	const MapOfStylePerClass& map = itMap->second;
	MapOfStylePerClass::const_iterator itMap2 = map.find(inClass);
	if (itMap2 != map.end())
	{
		if (inAppendDefaultStyles)
			return _RetainWithDefaultStyles( inType, itMap2->second.Get());
		else
			return RetainRefCountable(itMap2->second.Get());
	}
	else
		return NULL;
}

/** retain class style */
VDocClassStyle* VDocClassStyleManager::RetainClassStyle(eDocNodeType inType, VIndex inIndex, VString* outClass, bool inAppendDefaultStyles) const 
{ 
	MapOfClassStylePerType::const_iterator itMap = fClassStyles.find(inType);
	if (itMap == fClassStyles.end())
		return NULL;
	const MapOfStylePerClass& map = itMap->second;

	if (testAssert(inIndex >= 0 && inIndex < map.size()))
	{
		MapOfStylePerClass::const_iterator itMap2 = map.begin();
		std::advance( itMap2, inIndex);
		if (outClass)
			*outClass = itMap2->first;
		if (inAppendDefaultStyles)
			return _RetainWithDefaultStyles( inType, itMap2->second.Get());
		else
			return RetainRefCountable(itMap2->second.Get());
	}
	return NULL;
}



/** remove a class style  */
void VDocClassStyleManager::RemoveClassStyle(eDocNodeType inType, const VString& inClass)
{
	MapOfClassStylePerType::iterator itMap = fClassStyles.find(inType);
	if (itMap == fClassStyles.end())
		return;
	MapOfStylePerClass& map = itMap->second;

	MapOfStylePerClass::iterator itMap2 = map.find( inClass);
	if (itMap2 != map.end())
	{
		if (fDoc)
			itMap2->second->_OnDetachFromDocument( static_cast<VDocNode *>(fDoc));
		map.erase(itMap2);
	}
}

void VDocClassStyleManager::AppendPropsToClassStylesFrom(eDocNodeType inType, const VDocNode *inNode, bool inOnlyIfNotOverriden, bool inNoAppendSpanStyles)
{
	MapOfClassStylePerType::const_iterator itMap = fClassStyles.find(inType);
	if (itMap == fClassStyles.end())
		return;
	const MapOfStylePerClass& map = itMap->second;

	MapOfStylePerClass::const_iterator itMap2 = map.begin();
	for(;itMap2 != map.end(); itMap2++)
	{
		VDocClassStyle *node = itMap2->second.Get();
		if (node)
			node->AppendPropsFrom( inNode, inOnlyIfNotOverriden, inNoAppendSpanStyles);
	}
}

		
VIndex VDocClassStyleManager::GetClassStyleCount(eDocNodeType inType) const
{
	MapOfClassStylePerType::const_iterator itMap = fClassStyles.find(inType);
	if (itMap == fClassStyles.end())
		return 0;
	const MapOfStylePerClass& map = itMap->second;
	return map.size();
}


/** detach from owner document & clear owner document reference */
void VDocClassStyleManager::DetachFromOwnerDocument()
{
	if (!fDoc)
		return;

	MapOfClassStylePerType::const_iterator itMap = fClassStyles.begin();
	for(;itMap != fClassStyles.end(); itMap++)
	{
		const MapOfStylePerClass& map = itMap->second;

		MapOfStylePerClass::const_iterator itMap2 = map.begin();
		for(;itMap2 != map.end(); itMap2++)
			itMap2->second->_OnDetachFromDocument( static_cast<VDocNode *>(fDoc));
	}
	fDoc = NULL;
}


/** remove all class styles 
@param inKeepDefaultStyles
	if true (default is false), remove all class styles but default styles (i.e. class styles with empty class name)	
*/
void VDocClassStyleManager::Clear(bool inKeepDefaultStyles)
{
	if (!inKeepDefaultStyles)
	{
		DetachFromOwnerDocument();
		fClassStyles.clear();
		return;
	}

	MapOfClassStylePerType::iterator itMap = fClassStyles.begin();
	for(;itMap != fClassStyles.end(); itMap++)
	{
		MapOfStylePerClass& map = itMap->second;

		VDocClassStyle *csDefault = NULL;
		MapOfStylePerClass::iterator itMap2 = map.find(CVSTR(""));
		if (itMap2 != map.end())
		{
			csDefault = itMap2->second.Retain();
			map.erase( itMap2);
		}

		if (fDoc)
			for(itMap2 = map.begin();itMap2 != map.end(); itMap2++)
				itMap2->second->_OnDetachFromDocument( static_cast<VDocNode *>(fDoc));
		map.clear();

		if (csDefault)
			map[CVSTR("")] = VRefPtr<VDocClassStyle>( csDefault, false);
	}

}


/** retain node which matches the passed ID 
@remarks
	node might not have a parent if it is not a VDocText child node 
	but it can still be attached to the document: 
	for instance embedded images or paragraph styles do not have a parent
	but are attached to the document and so can be searched by ID
*/
VDocNode *VDocText::RetainNode(const VString& inID)
{
	if (inID.EqualToStringRaw(fID))
	{
		Retain();
		return static_cast<VDocNode *>(this);
	}
	MapOfNodePerID::iterator itNode = fMapOfNodePerID.find( inID);
	if (itNode != fMapOfNodePerID.end())
		return itNode->second.Retain();
	return NULL;
}

/** add image */
void VDocText::AddImage( VDocImage *image)
{
	//images are not part of the node tree but are either embedded in paragraph (through VDocSpanTextRef) or either background images so they are stored in separate table
	//(but they are attached to the document so they can be searched by ID)

	xbox_assert( image);

	if (image->GetDocumentNode() == this)
	{
		//image belongs to the document yet
		return;
	}

	image->_OnAttachToDocument( static_cast<VDocNode *>(this));
	fImages.push_back( VRefPtr<VDocImage>(image));
}

/** remove image */
void VDocText::RemoveImage( VDocImage *image)
{
	if (image->GetDocumentNode() != this)
		//only if image actually belongs to that document
		return;

	VectorOfImage::iterator itNode = fImages.begin();
	for(;itNode != fImages.end(); itNode++)
	{
		if (itNode->Get() == image)
		{
			fImages.erase( itNode);
			break;
		}
	}
	
	if (image->fDoc)
	{
		xbox_assert( image->fDoc == static_cast<VDocNode *>(this));

		image->_OnDetachFromDocument( static_cast<VDocNode *>(this));
	}
}


uLONG VDocNode::GetVersion() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_VERSION); }
void VDocNode::SetVersion(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_VERSION, inValue); }

bool VDocNode::GetWidowsAndOrphans() const { return IDocProperty::GetProperty<bool>( this, kDOC_PROP_WIDOWSANDORPHANS); }
void VDocNode::SetWidowsAndOrphans(const bool inValue) { IDocProperty::SetProperty<bool>( this, kDOC_PROP_WIDOWSANDORPHANS, inValue); }

uLONG VDocNode::GetDPI() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_DPI); }
void VDocNode::SetDPI(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_DPI, inValue); }

uLONG VDocNode::GetZoom() const { return IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_ZOOM); }
void VDocNode::SetZoom(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_ZOOM, inValue); }

bool VDocNode::GetDWrite() const { return IDocProperty::GetProperty<bool>( this, kDOC_PROP_DWRITE); }
void VDocNode::SetDWrite(const bool inValue) { IDocProperty::SetProperty<bool>( this, kDOC_PROP_DWRITE, inValue); }

eDocPropLayoutPageMode VDocNode::GetLayoutMode() const 
{ 
	uLONG mode = IDocProperty::GetProperty<uLONG>( this, kDOC_PROP_LAYOUT_MODE); 
	if (testAssert(mode >= kDOC_LAYOUT_MODE_NORMAL && mode <= kDOC_LAYOUT_MODE_WEB))
		return (eDocPropLayoutPageMode)mode;
	else
		return kDOC_LAYOUT_MODE_NORMAL;
}
void VDocNode::SetLayoutMode(const eDocPropLayoutPageMode inValue) 
{ 
	IDocProperty::SetProperty<uLONG>( this, kDOC_PROP_LAYOUT_MODE, (uLONG)inValue); 
}

bool VDocNode::ShouldShowImages() const { return IDocProperty::GetProperty<bool>( this, kDOC_PROP_SHOW_IMAGES); }
void VDocNode::ShouldShowImages(const bool inValue) { IDocProperty::SetProperty<bool>( this, kDOC_PROP_SHOW_IMAGES, inValue); }

bool VDocNode::ShouldShowReferences() const { return IDocProperty::GetProperty<bool>( this, kDOC_PROP_SHOW_REFERENCES); }
void VDocNode::ShouldShowReferences(const bool inValue) { IDocProperty::SetProperty<bool>( this, kDOC_PROP_SHOW_REFERENCES, inValue); }

bool VDocNode::ShouldShowHiddenCharacters() const { return IDocProperty::GetProperty<bool>( this, kDOC_PROP_SHOW_HIDDEN_CHARACTERS); }
void VDocNode::ShouldShowHiddenCharacters(const bool inValue) { IDocProperty::SetProperty<bool>( this, kDOC_PROP_SHOW_HIDDEN_CHARACTERS, inValue); }


DialectCode VDocNode::GetLang() const { return IDocProperty::GetProperty<DialectCode>( this, kDOC_PROP_LANG); }


void VDocNode::SetLang(const DialectCode inValue) 
{ 
	IDocProperty::SetProperty<DialectCode>( this, kDOC_PROP_LANG, inValue); 
	VIntlMgr *intlMgr = VIntlMgr::GetDefaultMgr();
	if (testAssert(intlMgr))
		intlMgr->GetBCP47LanguageCode( inValue, fLang);
	else
		fLang.SetEmpty();
}

const VString& VDocNode::GetLangBCP47() const
{
	if (fLang.IsEmpty())
	{
		VIntlMgr *intlMgr = VIntlMgr::GetDefaultMgr();
		if (testAssert(intlMgr))
			intlMgr->GetBCP47LanguageCode( GetLang(), fLang);
	}
	return fLang;
}


const VTime& VDocNode::GetDateTimeCreation() const { return IDocProperty::GetPropertyRef<VTime>( this, kDOC_PROP_DATETIMECREATION); }
void VDocNode::SetDateTimeCreation(const VTime& inValue) { IDocProperty::SetPropertyPerRef<VTime>( this, kDOC_PROP_DATETIMECREATION, inValue); }

const VTime& VDocNode::GetDateTimeModified() const { return IDocProperty::GetPropertyRef<VTime>( this, kDOC_PROP_DATETIMEMODIFIED); }
void VDocNode::SetDateTimeModified(const VTime& inValue) { IDocProperty::SetPropertyPerRef<VTime>( this, kDOC_PROP_DATETIMEMODIFIED, inValue); }

//const VString& VDocText::GetTitle() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_TITLE); }
//void VDocText::SetTitle(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_TITLE, inValue); }

//const VString& VDocText::GetSubject() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_SUBJECT); }
//void VDocText::SetSubject(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_SUBJECT, inValue); }

//const VString& VDocText::GetAuthor() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_AUTHOR); }
//void VDocText::SetAuthor(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_AUTHOR, inValue); }

//const VString& VDocText::GetCompany() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_COMPANY); }
//void VDocText::SetCompany(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_COMPANY, inValue); }

//const VString& VDocText::GetNotes() const { return IDocProperty::GetPropertyRef<VString>( this, kDOC_PROP_NOTES); }
//void VDocText::SetNotes(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( this, kDOC_PROP_NOTES, inValue); }


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// following methods are helper methods to set/get document text & character styles (for setting/getting document or paragraph properties, use property accessors)
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/** replace current text & character styles 
@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
bool VDocText::SetText( const VString& inText, VTreeTextStyle *inStyles, bool inCopyStyles)
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = dynamic_cast<VDocParagraph *>(fChildren[0].Get());
		return para->SetText( inText, inStyles, inCopyStyles);
	}
	else
		return VDocNode::SetText( inText, inStyles, inCopyStyles);
}

void VDocText::GetText(VString& outPlainText, sLONG inStart, sLONG inEnd) const
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(fChildren[0].Get());
		xbox_assert(GetTextStart() == 0 && para->GetTextStart() == 0);
		para->GetText(outPlainText, inStart, inEnd);
	}
	else
		VDocNode::GetText( outPlainText, inStart, inEnd);
}

void VDocText::SetStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = dynamic_cast<VDocParagraph *>(fChildren[0].Get());
		xbox_assert(GetTextStart() == 0 && para->GetTextStart() == 0);
		para->SetStyles( inStyles, inCopyStyles);
	}
	else
		VDocNode::SetStyles( inStyles, inCopyStyles);
}

VTreeTextStyle *VDocText::RetainStyles(sLONG inStart, sLONG inEnd, bool inRelativeToStart, bool inNULLIfRangeEmpty, bool inNoMerge) const
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(fChildren[0].Get());
		xbox_assert(GetTextStart() == 0 && para->GetTextStart() == 0);
		return para->RetainStyles(inStart, inEnd, inRelativeToStart, inNULLIfRangeEmpty, inNoMerge);
	}
	else
		return VDocNode::RetainStyles( inStart, inEnd, inRelativeToStart, inNULLIfRangeEmpty, inNoMerge);
}


VDocParagraph *VDocText::RetainFirstParagraph(sLONG inStart)
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
		return dynamic_cast<VDocParagraph *>(fChildren[0].Retain());
	else
		return VDocNode::RetainFirstParagraph( inStart);
}

const VDocParagraph* VDocText::GetFirstParagraph(sLONG inStart) const
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
		return dynamic_cast<VDocParagraph *>(fChildren[0].Get());
	else
		return VDocNode::GetFirstParagraph( inStart);
}


/** replace styled text at the specified range (relative to node start offset) with new text & styles
@remarks
	replaced text will inherit only uniform style on replaced range - if inInheritUniformStyle == true (default)

	return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
*/
VIndex VDocText::ReplaceStyledText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd, 
										 bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, const VDocSpanTextRef * inPreserveSpanTextRef)
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = dynamic_cast<VDocParagraph *>(fChildren[0].Get());
		xbox_assert(GetTextStart() == 0 && para->GetTextStart() == 0);
		return para->ReplaceStyledText( inText, inStyles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef);
	}
	else
		return VDocNode::ReplaceStyledText( inText, inStyles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef);
}



/** replace plain text with computed or source span references 
@param inShowSpanRefs
	span references text mask (default is computed values)
*/
void VDocText::UpdateSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs, VDBLanguageContext *inLC)
{
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = RetainFirstParagraph();
		if (para)
			para->UpdateSpanRefs( inShowSpanRefs, inLC);
		ReleaseRefCountable(&para);
	}
	else
		VDocNode::UpdateSpanRefs( inShowSpanRefs, inLC);
}

/** replace text with span text reference on the specified range
@remarks
	span ref plain text is set here to uniform non-breaking space: 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VDocText::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inNoUpdateRef, VDBLanguageContext *inLC)
{
	bool updated = false;
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = RetainFirstParagraph();
		if (para)
			updated = para->ReplaceAndOwnSpanRef( inSpanRef, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inNoUpdateRef, inLC);
		ReleaseRefCountable(&para);
	}
	else
		updated = VDocNode::ReplaceAndOwnSpanRef( inSpanRef, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inNoUpdateRef, inLC);
	return updated;
}


/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range  

	return true if any 4D expression has been replaced with plain text
*/
bool VDocText::FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd)
{
	bool updated = false;
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = RetainFirstParagraph();
		if (para)
			updated = para->FreezeExpressions( inLC, inStart, inEnd);
		ReleaseRefCountable(&para);
	}
	else
		updated = VDocNode::FreezeExpressions( inLC, inStart, inEnd);
	return updated;
}


/** return the first span reference which intersects the passed range */
const VTextStyle *VDocText::GetStyleRefAtRange(sLONG inStart, sLONG inEnd, sLONG *outStartBase)
{
	const VTextStyle *style = NULL;
	if (fChildren.size() == 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraph *para = RetainFirstParagraph();
		if (para)
			style = para->GetStyleRefAtRange( inStart, inEnd, outStartBase);
		ReleaseRefCountable(&para);
	}
	else
		style = VDocNode::GetStyleRefAtRange( inStart, inEnd, outStartBase);
	return style;
}


VDocNode*	VDocNodeChildrenIterator::First(sLONG inTextStart, sLONG inTextEnd) 
{
	fEnd = inTextEnd;
	if (inTextStart > 0)
	{
		VDocNode *node = fParent->GetFirstChild( inTextStart);
		if (node)
		{
			fIterator = fParent->fChildren.begin();
			if (node->GetChildIndex() > 0)
				std::advance(fIterator, node->GetChildIndex());
		}
		else 
			fIterator = fParent->fChildren.end(); 
	}
	else
		fIterator = fParent->fChildren.begin(); 
	return Current(); 
}


VDocNode*	VDocNodeChildrenIterator::Next() 
{  
	fIterator++; 
	if (fEnd >= 0 && fIterator != fParent->fChildren.end() && fEnd <= (*fIterator).Get()->GetTextStart()) //if child range start >= fEnd
	{
		if (fEnd < (*fIterator).Get()->GetTextStart() || (*fIterator).Get()->GetTextLength() != 0) //if child range is 0 and child range end == fEnd, we keep it otherwise we discard it and stop iteration
		{
			fIterator = fParent->fChildren.end();
			return NULL;
		}
	}
	return Current(); 
}


VDocNode*	VDocNodeChildrenReverseIterator::First(sLONG inTextStart, sLONG inTextEnd) 
{
	fStart = inTextStart;
	if (inTextEnd < 0)
		inTextEnd = fParent->GetTextLength();

	if (inTextEnd < fParent->GetTextLength())
	{
		VDocNode *node = fParent->GetFirstChild( inTextEnd);
		if (node)
		{
			if (inTextEnd <= node->GetTextStart() && node->GetTextLength() > 0)
				node = node->GetChildIndex() > 0 ? fParent->fChildren[node->GetChildIndex()-1].Get() : NULL;
			if (node)
			{
				fIterator = fParent->fChildren.rbegin(); //point on last child
				if (node->GetChildIndex() < fParent->fChildren.size()-1)
					std::advance(fIterator, fParent->fChildren.size()-1-node->GetChildIndex());
			}
			else
				fIterator = fParent->fChildren.rend(); 
		}
		else 
			fIterator = fParent->fChildren.rend(); 
	}
	else
		fIterator = fParent->fChildren.rbegin(); 
	return Current(); 
}


VDocNode*	VDocNodeChildrenReverseIterator::Next() 
{  
	fIterator++; 
	if (fStart > 0 && fIterator != fParent->fChildren.rend() && fStart >= (*fIterator).Get()->GetTextStart()+(*fIterator).Get()->GetTextLength()) 
	{
		fIterator = fParent->fChildren.rend();
		return NULL;
	}
	return Current(); 
}

