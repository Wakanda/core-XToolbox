
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

VString	VDocNode::sElementNameEmpty;
VString	VDocText::sElementName;



/** map of default inherited per CSS property
@remarks
	default inherited status is based on W3C CSS rules for regular CSS properties;
	4D CSS properties (prefixed with '-d4-') are NOT inherited on default
*/
bool VDocNode::sPropInherited[kDOC_NUM_PROP] = 
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
	false		//kDOC_PROP_4DREF_USER,				// '-d4-ref-user'
};

/** map of default values */
VDocProperty *VDocNode::sPropDefault[kDOC_NUM_PROP] = {0};


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
	sPropDefault[kDOC_PROP_TEXT_INDENT]				= new VDocProperty( (uLONG)0); //0cm 
	sPropDefault[kDOC_PROP_TAB_STOP_OFFSET]			= new VDocProperty( (uLONG)floor(((1.25*72*20)/2.54) + 0.5)); //1,25cm = 709 TWIPS
	sPropDefault[kDOC_PROP_TAB_STOP_TYPE]			= new VDocProperty( (uLONG)kDOC_TAB_STOP_TYPE_LEFT); 
	sPropDefault[kDOC_PROP_NEW_PARA_CLASS]			= new VDocProperty( VString("")); 

	//image properties

	sPropDefault[kDOC_PROP_IMG_SOURCE]				= new VDocProperty( VString("")); 
	sPropDefault[kDOC_PROP_IMG_ALT_TEXT]			= new VDocProperty( VString("")); //FIXME: maybe we should set default text for image element ?

	//span character styles
	//(following are not actually used because character styles are managed only through VTreeTextStyle)
	 
	sPropDefault[kDOC_PROP_FONT_FAMILY]				= new VDocProperty( VString("Times New Roman")); 
	sPropDefault[kDOC_PROP_FONT_SIZE]				= new VDocProperty( (uLONG)(12*20)); 
	sPropDefault[kDOC_PROP_COLOR]					= new VDocProperty( (uLONG)0xff000000); 
	sPropDefault[kDOC_PROP_FONT_STYLE]				= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_FONT_WEIGHT]				= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_TEXT_DECORATION]			= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_4DREF]					= new VDocProperty( VDocSpanTextRef::fSpace); 
	sPropDefault[kDOC_PROP_4DREF_USER]				= new VDocProperty( VDocSpanTextRef::fSpace); 

	VDocText::sElementName = CVSTR("body");
	VDocParagraph::sElementName = CVSTR("p");
	VDocImage::sElementName = CVSTR("img");

	sPropInitDone = true;
}

void VDocNode::DeInit()
{
	for (int i = 0; i < kDOC_NUM_PROP; i++)
		if (sPropDefault[i]) 
		{	
			delete sPropDefault[i];
			sPropDefault[i] = NULL;
		}
	sPropInitDone = false;
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



void VDocNode::_UpdateTextInfo()
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

	if (fChildren.size())
	{
		fTextLength = 0;
		VectorOfNode::const_iterator itNode = fChildren.begin();
		for(;itNode != fChildren.end(); itNode++)
			fTextLength += itNode->Get()->GetTextLength();
	}
}

void VDocNode::_UpdateTextInfoEx(bool inUpdateNextSiblings)
{
	_UpdateTextInfo();
	if (fParent)
	{
		//update siblings after this node
		if (inUpdateNextSiblings)
		{
			VectorOfNode::iterator itNode = fParent->fChildren.begin();
			std::advance( itNode, fChildIndex+1);
			for (;itNode != fParent->fChildren.end(); itNode++)
				(*itNode).Get()->_UpdateTextInfo();
		}

		fParent->_UpdateTextInfoEx( true);
	}
}


void VDocNode::_OnAttachToDocument(VDocNode *inDocumentNode)
{
	xbox_assert(inDocumentNode && inDocumentNode->fType == kDOC_NODE_TYPE_DOCUMENT);

	//update document reference
	if (!fDoc)
		fDoc = inDocumentNode;

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

	if (fDoc)
		inNode->_OnAttachToDocument( fDoc);
	else
		inNode->fDirtyStamp++;

	inNode->_UpdateTextInfoEx(true);
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
	if (fDoc)
		inNode->_OnAttachToDocument( fDoc);
	else
		inNode->fDirtyStamp++;

	inNode->_UpdateTextInfoEx();
}

/** detach child node from parent */
void VDocNode::Detach()
{   
	if (!fParent)
		return;

	VRefPtr<VDocNode> protect(this); //might be last ref

	xbox_assert(fType != kDOC_NODE_TYPE_DOCUMENT);

	VectorOfNode::iterator itChild =  fParent->fChildren.begin();
	std::advance( itChild, fChildIndex);
	xbox_assert(itChild->Get() == this);
	fParent->fChildren.erase(itChild);
	itChild = fParent->fChildren.begin();
	std::advance(itChild, fChildIndex);
	for (;itChild != fParent->fChildren.end(); itChild++)
		itChild->Get()->fChildIndex--;

	itChild = fParent->fChildren.begin();
	std::advance(itChild, fChildIndex);
	if (itChild != fParent->fChildren.end())
		itChild->Get()->_UpdateTextInfoEx(true);
	else
		fParent->_UpdateTextInfoEx(true);

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
	if (inValue.BeginsWith(D4ID_PREFIX_RESERVED))
	{
		xbox_assert(false); //forbiden: prefix is reserved for automatic ids
		return false;
	}

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

	if (doc)
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

/** return true if property value is overriden locally */ 
bool VDocNode::IsOverriden( const eDocProperty inProp) const
{
	MapOfProp::const_iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
		return true;
	else
		return false;
}

/** remove local property */
void VDocNode::RemoveProp( const eDocProperty inProp)
{
	//remove local definition if any
	MapOfProp::iterator it = fProps.find( (uLONG)inProp);
	if (it != fProps.end())
	{
		fProps.erase(it);
		fDirtyStamp++;
	}
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

/** return true if property needs to be serialized 
@remarks
	it is used by serializer to serialize only properties which are not equal to the inherited value
	or to the default value for not inherited properties
*/
bool VDocNode::ShouldSerializeProp( const eDocProperty inProp) const
{
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
	if (testAssert(inProp >= 0 && inProp <= kDOC_NUM_PROP))
		return *(sPropDefault[ (uLONG)inProp]);
	else
		return *(sPropDefault[ (uLONG)0]);
}


eDocPropTextAlign VDocNode::GetTextAlign() const 
{ 
	uLONG textAlign = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_TEXT_ALIGN);
	if (testAssert(textAlign >= JST_Default && textAlign <= JST_Justify))
		return (eDocPropTextAlign)textAlign;
	else
		return JST_Default;
}
void VDocNode::SetTextAlign(const eDocPropTextAlign inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_TEXT_ALIGN, (uLONG)inValue); }

eDocPropTextAlign VDocNode::GetVerticalAlign() const 
{ 
	uLONG textAlign = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_VERTICAL_ALIGN);
	if (testAssert(textAlign >= JST_Default && textAlign <= JST_Subscript))
		return (eDocPropTextAlign)textAlign;
	else
		return JST_Default;
}
void VDocNode::SetVerticalAlign(const eDocPropTextAlign inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_VERTICAL_ALIGN, (uLONG)inValue); }

const sDocPropRect& VDocNode::GetMargin() const { return IDocProperty::GetPropertyRef<sDocPropRect>( static_cast<const VDocNode *>(this), kDOC_PROP_MARGIN); }
void VDocNode::SetMargin(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( static_cast<VDocNode *>(this), kDOC_PROP_MARGIN, inValue); }

const sDocPropRect& VDocNode::GetPadding() const { return IDocProperty::GetPropertyRef<sDocPropRect>( static_cast<const VDocNode *>(this), kDOC_PROP_PADDING); }
void VDocNode::SetPadding(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( static_cast<VDocNode *>(this), kDOC_PROP_PADDING, inValue); }

RGBAColor VDocNode::GetBackgroundColor() const { return IDocProperty::GetProperty<RGBAColor>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_COLOR); }
void VDocNode::SetBackgroundColor(const RGBAColor inValue) { IDocProperty::SetProperty<RGBAColor>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_COLOR, inValue); }

eDocPropBackgroundBox VDocNode::GetBackgroundClip() const 
{ 
	uLONG clip = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_CLIP);
	if (testAssert(clip >= kDOC_BACKGROUND_BORDER_BOX && clip <= kDOC_BACKGROUND_CONTENT_BOX))
		return (eDocPropBackgroundBox)clip;
	else 
		return kDOC_BACKGROUND_BORDER_BOX;
}
void VDocNode::SetBackgroundClip(const eDocPropBackgroundBox inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_CLIP, (uLONG)inValue); }

const VString& VDocNode::GetBackgroundImage() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_IMAGE); }
void VDocNode::SetBackgroundImage(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_IMAGE, inValue); }

eDocPropTextAlign VDocNode::GetBackgroundImageHAlign() const 
{ 
	sLONG halign = (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_POSITION))[0]; 
	if (testAssert(halign >= JST_Default && halign <= JST_Subscript))
		return (eDocPropTextAlign)halign;
	else
		return JST_Left;
}
void VDocNode::SetBackgroundImageHAlign(const eDocPropTextAlign inValue) 
{
	VDocProperty::VectorOfSLONG position = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_POSITION);
	position[0] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_POSITION, position); 
}


eDocPropTextAlign VDocNode::GetBackgroundImageVAlign() const 
{ 
	sLONG valign = (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_POSITION))[1]; 
	if (testAssert(valign >= JST_Default && valign <= JST_Subscript))
		return (eDocPropTextAlign)valign;
	else
		return JST_Top;
}
void VDocNode::SetBackgroundImageVAlign(const eDocPropTextAlign inValue) 
{
	VDocProperty::VectorOfSLONG position = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_POSITION);
	position[1] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_POSITION, position); 
}

eDocPropBackgroundRepeat VDocNode::GetBackgroundImageRepeat() const 
{ 
	uLONG repeat = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_REPEAT);
	if (testAssert(repeat >= kDOC_BACKGROUND_REPEAT && repeat <= kDOC_BACKGROUND_NO_REPEAT))
		return (eDocPropBackgroundRepeat)repeat;
	else 
		return kDOC_BACKGROUND_REPEAT;
}
void VDocNode::SetBackgroundImageRepeat(const eDocPropBackgroundRepeat inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_REPEAT, (uLONG)inValue); }

eDocPropBackgroundBox VDocNode::GetBackgroundImageOrigin() const 
{ 
	uLONG origin = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_ORIGIN);
	if (testAssert(origin >= kDOC_BACKGROUND_BORDER_BOX && origin <= kDOC_BACKGROUND_CONTENT_BOX))
		return (eDocPropBackgroundBox)origin;
	else 
		return kDOC_BACKGROUND_PADDING_BOX;
}
void VDocNode::SetBackgroundImageOrigin(const eDocPropBackgroundBox inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_ORIGIN, (uLONG)inValue); }

sLONG VDocNode::GetBackgroundImageWidth() const 
{ 
	return (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_SIZE))[0]; 
}
void VDocNode::SetBackgroundImageWidth(const sLONG inValue) 
{
	VDocProperty::VectorOfSLONG size = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_SIZE);
	size[0] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_SIZE, size); 
}

sLONG VDocNode::GetBackgroundImageHeight() const 
{ 
	return (IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_SIZE))[1]; 
}
void VDocNode::SetBackgroundImageHeight(const sLONG inValue) 
{
	VDocProperty::VectorOfSLONG size = IDocProperty::GetPropertyRef<VDocProperty::VectorOfSLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_SIZE);
	size[1] = inValue;
	IDocProperty::SetPropertyPerRef<VDocProperty::VectorOfSLONG>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_SIZE, size); 
}

uLONG VDocNode::GetWidth() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_WIDTH); }
void VDocNode::SetWidth(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_WIDTH, inValue); }

uLONG VDocNode::GetMinWidth() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_MIN_WIDTH); }
void VDocNode::SetMinWidth(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_MIN_WIDTH, inValue); }

uLONG VDocNode::GetHeight() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_HEIGHT); }
void VDocNode::SetHeight(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_HEIGHT, inValue); }

uLONG VDocNode::GetMinHeight() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_MIN_HEIGHT); }
void VDocNode::SetMinHeight(const RGBAColor inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_MIN_HEIGHT, inValue); }

const sDocPropRect& VDocNode::GetBorderStyle() const { return IDocProperty::GetPropertyRef<sDocPropRect>( static_cast<const VDocNode *>(this), kDOC_PROP_BORDER_STYLE); }
void VDocNode::SetBorderStyle(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( static_cast<VDocNode *>(this), kDOC_PROP_BORDER_STYLE, inValue); }

const sDocPropRect& VDocNode::GetBorderWidth() const { return IDocProperty::GetPropertyRef<sDocPropRect>( static_cast<const VDocNode *>(this), kDOC_PROP_BORDER_WIDTH); }
void VDocNode::SetBorderWidth(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( static_cast<VDocNode *>(this), kDOC_PROP_BORDER_WIDTH, inValue); }

const sDocPropRect& VDocNode::GetBorderColor() const { return IDocProperty::GetPropertyRef<sDocPropRect>( static_cast<const VDocNode *>(this), kDOC_PROP_BORDER_COLOR); }
void VDocNode::SetBorderColor(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( static_cast<VDocNode *>(this), kDOC_PROP_BORDER_COLOR, inValue); }

const sDocPropRect& VDocNode::GetBorderRadius() const { return IDocProperty::GetPropertyRef<sDocPropRect>( static_cast<const VDocNode *>(this), kDOC_PROP_BORDER_RADIUS); }
void VDocNode::SetBorderRadius(const sDocPropRect& inValue) { IDocProperty::SetPropertyPerRef<sDocPropRect>( static_cast<VDocNode *>(this), kDOC_PROP_BORDER_RADIUS, inValue); }


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

/** append properties from the passed node
@remarks
	if inNode type is not equal to the current node type, recurse down and apply inNode properties to any child node with the same type as inNode:
	in that case, only children nodes which intersect the passed range are modified
	if inNode type is equal to the current node type, apply inNode properties to the current node only (passed range is ignored)

	if inOnlyIfInheritOrNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties
*/
void VDocNode::AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfInheritOrNotOverriden, bool inNoAppendSpanStyles, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	if (GetType() != inNode->GetType())
	{
		//recurse down on the passed range

		VDocNodeChildrenIterator it(this);
		VDocNode *node = it.First(inStart, inEnd);
		while (node)
		{
			node->AppendPropsFrom( inNode, inOnlyIfInheritOrNotOverriden, inNoAppendSpanStyles, inStart-node->GetTextStart(), inEnd-node->GetTextStart());
			node = it.Next();
		}
		return;
	}

	bool isDirty = false;
	MapOfProp::const_iterator it = inNode->fProps.begin();
	for (;it != inNode->fProps.end(); it++)
	{
		if (inOnlyIfInheritOrNotOverriden)
		{
			eDocProperty prop = (eDocProperty)it->first;
			if (!IsOverriden( prop) || IsInherited( prop))
			{
				fProps[prop] = it->second;
				isDirty = true;
			}
		}
		else
		{
			fProps[(eDocProperty)(it->first)] = it->second;
			isDirty = true;
		}
	}
	if (isDirty)
		fDirtyStamp++;
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
	if (inNodeText->fParaStyles.size())
	{
		MapOfParaStylePerClass::const_iterator itMap = inNodeText->fParaStyles.begin();
		for(;itMap != inNodeText->fParaStyles.end(); itMap++)
		{
			VDocParagraph *para = dynamic_cast<VDocParagraph *>(itMap->second.Get()->Clone());
			xbox_assert(para);
			fParaStyles[itMap->first] = VRefPtr<VDocParagraph>(para, false);
		}
	}
}

/** add or replace a paragraph style 
@param inClass
	class name 
@param inNode
	paragraph model (should contain default properties for the class) - must be a paragraph node
*/
void VDocText::AddOrReplaceParagraphStyle(const VString inClass, VDocParagraph *inPara)
{
	xbox_assert( inPara && !inPara->GetParent());

	if (inPara->GetDocumentNode() == this)
	{
		//paragraph belongs to the document yet
		xbox_assert(fParaStyles.find(inClass) != fParaStyles.end());
		return;
	}

	inPara->_OnAttachToDocument( static_cast<VDocNode *>(this));

	fParaStyles[inClass] = VRefPtr<VDocParagraph>(inPara);
}

/** retain a paragraph style */
VDocParagraph* VDocText::RetainParagraphStyle(const VString inClass) const
{
	MapOfParaStylePerClass::const_iterator itMap = fParaStyles.find( inClass);
	if (itMap != fParaStyles.end())
		return itMap->second.Get();
	else
		return NULL;
}

/** retain paragraph style */
VDocParagraph* VDocText::RetainParagraphStyle(VIndex inIndex, VString& outClassNoD4ClassPrefix) const 
{ 
	if (testAssert(inIndex >= 0 && inIndex < fParaStyles.size()))
	{
		MapOfParaStylePerClass::const_iterator itMap = fParaStyles.begin();
		std::advance( itMap, inIndex);
		outClassNoD4ClassPrefix = itMap->first;
		return RetainRefCountable(itMap->second.Get());
	}
	return NULL;
}



/** remove a paragraph style  */
void VDocText::RemoveParagraphStyle(const VString inClass)
{
	MapOfParaStylePerClass::iterator itMap = fParaStyles.find( inClass);
	if (itMap != fParaStyles.end())
	{
		itMap->second.Get()->_OnDetachFromDocument( static_cast<VDocNode *>(this));
		fParaStyles.erase(itMap);
	}
}

/** append properties from the passed paragraph node to all paragraph styles
@remarks
	if inOnlyIfInheritOrNotOverriden == true, paragraph styles properties which are defined and not inherited are not overriden by inPara properties
*/
void VDocText::AppendPropsToParagraphStylesFrom( const VDocParagraph *inPara, bool inOnlyIfInheritOrNotOverriden, bool inNoAppendSpanStyles)
{
	//we need to apply to document node paragraph style sheets too

	MapOfParaStylePerClass::const_iterator itMap = fParaStyles.begin();
	for(;itMap != fParaStyles.end(); itMap++)
	{
		VDocParagraph *para = itMap->second.Get();
		if (para)
			para->AppendPropsFrom( static_cast<const VDocNode *>(inPara), inOnlyIfInheritOrNotOverriden, inNoAppendSpanStyles);
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


uLONG VDocText::GetVersion() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_VERSION); }
void VDocText::SetVersion(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_VERSION, inValue); }

bool VDocText::GetWidowsAndOrphans() const { return IDocProperty::GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_WIDOWSANDORPHANS); }
void VDocText::SetWidowsAndOrphans(const bool inValue) { IDocProperty::SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_WIDOWSANDORPHANS, inValue); }

uLONG VDocText::GetDPI() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_DPI); }
void VDocText::SetDPI(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_DPI, inValue); }

uLONG VDocText::GetZoom() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_ZOOM); }
void VDocText::SetZoom(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_ZOOM, inValue); }

bool VDocText::GetDWrite() const { return IDocProperty::GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_DWRITE); }
void VDocText::SetDWrite(const bool inValue) { IDocProperty::SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_DWRITE, inValue); }

eDocPropLayoutMode VDocText::GetLayoutMode() const 
{ 
	uLONG mode = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_LAYOUT_MODE); 
	if (testAssert(mode >= kDOC_LAYOUT_MODE_NORMAL && mode <= kDOC_LAYOUT_MODE_PAGE))
		return (eDocPropLayoutMode)mode;
	else
		return kDOC_LAYOUT_MODE_NORMAL;
}
void VDocText::SetLayoutMode(const eDocPropLayoutMode inValue) 
{ 
	IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_LAYOUT_MODE, (uLONG)inValue); 
}

bool VDocText::ShouldShowImages() const { return IDocProperty::GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_SHOW_IMAGES); }
void VDocText::ShouldShowImages(const bool inValue) { IDocProperty::SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_SHOW_IMAGES, inValue); }

bool VDocText::ShouldShowReferences() const { return IDocProperty::GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_SHOW_REFERENCES); }
void VDocText::ShouldShowReferences(const bool inValue) { IDocProperty::SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_SHOW_REFERENCES, inValue); }

bool VDocText::ShouldShowHiddenCharacters() const { return IDocProperty::GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_SHOW_HIDDEN_CHARACTERS); }
void VDocText::ShouldShowHiddenCharacters(const bool inValue) { IDocProperty::SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_SHOW_HIDDEN_CHARACTERS, inValue); }

DialectCode		VDocNode::GetLang() const
{
	if (fDoc)
		return fDoc->GetLang();
	else
		return IDocProperty::GetPropertyDefault<DialectCode>(kDOC_PROP_LANG);
}

const VString&	VDocNode::GetLangBCP47() const
{
	if (fDoc)
		return fDoc->GetLangBCP47();
	else
	{
		static VString sDefaultLang;
		if (sDefaultLang.IsEmpty())
		{
			DialectCode code = IDocProperty::GetPropertyDefault<DialectCode>(kDOC_PROP_LANG);
			VIntlMgr *intlMgr = VIntlMgr::GetDefaultMgr();
			if (testAssert(intlMgr))
				intlMgr->GetBCP47LanguageCode( code, sDefaultLang);
		}
		return sDefaultLang;
	}
}


DialectCode VDocText::GetLang() const { return IDocProperty::GetProperty<DialectCode>( static_cast<const VDocNode *>(this), kDOC_PROP_LANG); }


void VDocText::SetLang(const DialectCode inValue) 
{ 
	IDocProperty::SetProperty<DialectCode>( static_cast<VDocNode *>(this), kDOC_PROP_LANG, inValue); 
	VIntlMgr *intlMgr = VIntlMgr::GetDefaultMgr();
	if (testAssert(intlMgr))
		intlMgr->GetBCP47LanguageCode( inValue, fLang);
	else
		fLang.SetEmpty();
}

const VString& VDocText::GetLangBCP47() const
{
	if (fLang.IsEmpty())
	{
		VIntlMgr *intlMgr = VIntlMgr::GetDefaultMgr();
		if (testAssert(intlMgr))
			intlMgr->GetBCP47LanguageCode( GetLang(), fLang);
	}
	return fLang;
}


const VTime& VDocText::GetDateTimeCreation() const { return IDocProperty::GetPropertyRef<VTime>( static_cast<const VDocNode *>(this), kDOC_PROP_DATETIMECREATION); }
void VDocText::SetDateTimeCreation(const VTime& inValue) { IDocProperty::SetPropertyPerRef<VTime>( static_cast<VDocNode *>(this), kDOC_PROP_DATETIMECREATION, inValue); }

const VTime& VDocText::GetDateTimeModified() const { return IDocProperty::GetPropertyRef<VTime>( static_cast<const VDocNode *>(this), kDOC_PROP_DATETIMEMODIFIED); }
void VDocText::SetDateTimeModified(const VTime& inValue) { IDocProperty::SetPropertyPerRef<VTime>( static_cast<VDocNode *>(this), kDOC_PROP_DATETIMEMODIFIED, inValue); }

//const VString& VDocText::GetTitle() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_TITLE); }
//void VDocText::SetTitle(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_TITLE, inValue); }

//const VString& VDocText::GetSubject() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_SUBJECT); }
//void VDocText::SetSubject(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_SUBJECT, inValue); }

//const VString& VDocText::GetAuthor() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_AUTHOR); }
//void VDocText::SetAuthor(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_AUTHOR, inValue); }

//const VString& VDocText::GetCompany() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_COMPANY); }
//void VDocText::SetCompany(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_COMPANY, inValue); }

//const VString& VDocText::GetNotes() const { return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_NOTES); }
//void VDocText::SetNotes(const VString& inValue) { IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_NOTES, inValue); }


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

