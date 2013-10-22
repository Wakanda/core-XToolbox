
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

VCriticalSection VDocNode::sMutex;

/** map of default inherited per CSS property
@remarks
	default inherited status is based on W3C CSS rules for regular CSS properties;
	4D CSS properties (prefixed with '-d4-') are NOT inherited on default
*/
bool VDocNode::sPropInherited[kDOC_NUM_PROP] = 
{
	//document node properties

	false,	//kDOC_PROP_VERSION,
	false,	//kDOC_PROP_WIDOWSANDORPHANS,
	false,	//kDOC_PROP_DPI,
	false,	//kDOC_PROP_ZOOM,
	false,	//kDOC_PROP_LAYOUT_MODE,
	false,	//kDOC_PROP_SHOW_IMAGES,
	false,	//kDOC_PROP_SHOW_REFERENCES,
	false,	//kDOC_PROP_SHOW_HIDDEN_CHARACTERS,
	true,	//kDOC_PROP_LANG,						//regular CSS 'lang(??)'

	false,	//kDOC_PROP_DATETIMECREATION,
	false,	//kDOC_PROP_DATETIMEMODIFIED,

	//false,	//kDOC_PROP_TITLE,
	//false,	//kDOC_PROP_SUBJECT,
	//false,	//kDOC_PROP_AUTHOR,
	//false,	//kDOC_PROP_COMPANY,
	//false		//kDOC_PROP_NOTES,

	//node common properties

	true,		//kDOC_PROP_TEXT_ALIGN,				//regular CSS 'text-align'
	false,		//kDOC_PROP_VERTICAL_ALIGN,			//regular CSS 'vertical-align'
	false,		//kDOC_PROP_MARGIN,					//regular CSS 'margin'
	false,		//kDOC_PROP_BACKGROUND_COLOR,		//regular CSS 'background-color' (for node but span it is element back color; for span node, it is character back color)
													//(in regular CSS, it is not inherited but it is in 4D span node for compat with older 4D span format)
	//paragraph node properties

	true,		//kDOC_PROP_DIRECTION,				//regular CSS 'direction'
	true,		//kDOC_PROP_LINE_HEIGHT,			//regular CSS 'line-height'
	false,		//kDOC_PROP_PADDING_FIRST_LINE,
	false,		//kDOC_PROP_TAB_STOP_OFFSET,
	false,		//kDOC_PROP_TAB_STOP_TYPE,

	//span character styles

	true,		//kDOC_PROP_FONT_FAMILY,			
	true,		//kDOC_PROP_FONT_SIZE,				
	true,		//kDOC_PROP_COLOR,				
	true,		//kDOC_PROP_FONT_STYLE,		
	true,		//kDOC_PROP_FONT_WEIGHT,		
	false,		//kDOC_PROP_TEXT_DECORATION,		//in regular CSS, it is not inherited but it is in 4D span node for compat with older 4D span format
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
	case kPROP_TYPE_PADDINGMARGIN:
		fValue.fPaddingMargin = inValue.fValue.fPaddingMargin;
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


bool VDocProperty::operator == (const VDocProperty& inValue) 
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
	case kPROP_TYPE_PADDINGMARGIN:
		return fValue.fPaddingMargin.left == inValue.fValue.fPaddingMargin.left
			   &&
			   fValue.fPaddingMargin.top == inValue.fValue.fPaddingMargin.top
			   &&	
			   fValue.fPaddingMargin.right == inValue.fValue.fPaddingMargin.right
			   &&	
			   fValue.fPaddingMargin.bottom == inValue.fValue.fPaddingMargin.bottom
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
	//document properties

	sPropDefault[kDOC_PROP_VERSION]					= new VDocProperty((uLONG)kDOC_VERSION_SPAN4D);
	sPropDefault[kDOC_PROP_WIDOWSANDORPHANS]		= new VDocProperty(true);
	sPropDefault[kDOC_PROP_DPI]						= new VDocProperty((uLONG)72);
	sPropDefault[kDOC_PROP_ZOOM]					= new VDocProperty((uLONG)100);
	sPropDefault[kDOC_PROP_LAYOUT_MODE]				= new VDocProperty((uLONG)kDOC_LAYOUT_MODE_NORMAL);
	sPropDefault[kDOC_PROP_SHOW_IMAGES]				= new VDocProperty(true);
	sPropDefault[kDOC_PROP_SHOW_REFERENCES]			= new VDocProperty(false);
	sPropDefault[kDOC_PROP_SHOW_HIDDEN_CHARACTERS]	= new VDocProperty(false);
	sPropDefault[kDOC_PROP_LANG]					= new VDocProperty((uLONG)(VProcess::Get()->GetLocalizer() ? VProcess::Get()->GetLocalizer()->GetLocalizationLanguage() : VIntlMgr::ResolveDialectCode(DC_USER)));
	VTime time;
	time.FromSystemTime();
	sPropDefault[kDOC_PROP_DATETIMECREATION]		= new VDocProperty(time);
	sPropDefault[kDOC_PROP_DATETIMEMODIFIED]		= new VDocProperty(time);

	//sPropDefault[kDOC_PROP_TITLE]					= new VDocProperty(VString("Document1"));
	//sPropDefault[kDOC_PROP_SUBJECT]					= new VDocProperty(VString(""));
	//VString userName;
	//VSystem::GetLoginUserName( userName);
	//sPropDefault[kDOC_PROP_AUTHOR]					= new VDocProperty(userName);
	//sPropDefault[kDOC_PROP_COMPANY]					= new VDocProperty(VString(""));
	//sPropDefault[kDOC_PROP_NOTES]					= new VDocProperty(VString(""));

	//common properties

	sPropDefault[kDOC_PROP_TEXT_ALIGN]				= new VDocProperty( (uLONG)JST_Default);
	sPropDefault[kDOC_PROP_VERTICAL_ALIGN]			= new VDocProperty( (uLONG)JST_Default);
	sDocPropPaddingMargin margin = { 0, 0, 0, 0 }; //please do not modify it
	sPropDefault[kDOC_PROP_MARGIN]					= new VDocProperty( margin);
	sPropDefault[kDOC_PROP_BACKGROUND_COLOR]		= new VDocProperty( (RGBAColor)RGBACOLOR_TRANSPARENT); //default is transparent

	//paragraph properties

	sPropDefault[kDOC_PROP_DIRECTION]				= new VDocProperty( (uLONG)kTEXT_DIRECTION_LTR); 
	sPropDefault[kDOC_PROP_LINE_HEIGHT]				= new VDocProperty( (sLONG)(kDOC_PROP_LINE_HEIGHT_NORMAL)); //normal
	sPropDefault[kDOC_PROP_PADDING_FIRST_LINE]		= new VDocProperty( (uLONG)0); //0cm 
	sPropDefault[kDOC_PROP_TAB_STOP_OFFSET]			= new VDocProperty( (uLONG)floor(((1.25*72*20)/2.54) + 0.5)); //1,25cm = 709 TWIPS
	sPropDefault[kDOC_PROP_TAB_STOP_TYPE]			= new VDocProperty( (uLONG)kTEXT_TAB_STOP_TYPE_LEFT); 

	//span character styles
	//(following are not actually used because character styles are managed only through VTreeTextStyle)
	 
	sPropDefault[kDOC_PROP_FONT_FAMILY]				= new VDocProperty( CVSTR("Times New Roman")); 
	sPropDefault[kDOC_PROP_FONT_SIZE]				= new VDocProperty( (uLONG)(12*20)); 
	sPropDefault[kDOC_PROP_COLOR]					= new VDocProperty( (uLONG)0xff000000); 
	sPropDefault[kDOC_PROP_FONT_STYLE]				= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_FONT_WEIGHT]				= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_TEXT_DECORATION]			= new VDocProperty( (uLONG)0); 
	sPropDefault[kDOC_PROP_4DREF]					= new VDocProperty( VDocSpanTextRef::fNBSP); 
	sPropDefault[kDOC_PROP_4DREF_USER]				= new VDocProperty( VDocSpanTextRef::fNBSP); 

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
	if (fType == kDOC_NODE_TYPE_DOCUMENT)
	{
		fDoc = this;
		VDocText *docText = dynamic_cast<VDocText *>(fDoc);
		xbox_assert(docText);
		docText->fNextIDCount = dynamic_cast<const VDocText *>(inNode)->fNextIDCount;
	}
	fDirtyStamp = 1;

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
const VDocText *VDocNode::GetDocumentNode() const
{ 
	return fDoc ? dynamic_cast<const VDocText *>(fDoc) : NULL; 
}


/** get document node for write */
VDocText *VDocNode::GetDocumentNodeForWrite()
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
	fDirtyStamp++;
	if (fDoc)
		inNode->_OnAttachToDocument( fDoc);
	else
		inNode->fDirtyStamp++;
}

/** detach child node from parent */
void VDocNode::Detach()
{   
	if (!fParent)
		return;

	VRefPtr<VDocNode> protect(this); //might be last ref

	xbox_assert(fType != kDOC_NODE_TYPE_DOCUMENT);

	VectorOfNode::iterator itParent =  fParent->fChildren.begin();
	for (;itParent != fParent->fChildren.end(); itParent++)
		if (itParent->Get() == this)
		{
			fParent->fChildren.erase(itParent);
			break;
		}
	fParent->fDirtyStamp++;
	fParent = NULL;
	if (fDoc)
		_OnDetachFromDocument( fDoc);
	else
		fDirtyStamp++;
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

/** get default property value */
const VDocProperty& VDocNode::GetDefaultPropValue( const eDocProperty inProp) const
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
	if (testAssert(textAlign >= JST_Default && textAlign <= JST_Justify))
		return (eDocPropTextAlign)textAlign;
	else
		return JST_Default;
}
void VDocNode::SetVerticalAlign(const eDocPropTextAlign inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_VERTICAL_ALIGN, (uLONG)inValue); }

const sDocPropPaddingMargin& VDocNode::GetMargin() const { return IDocProperty::GetPropertyRef<sDocPropPaddingMargin>( static_cast<const VDocNode *>(this), kDOC_PROP_MARGIN); }
void VDocNode::SetMargin(const sDocPropPaddingMargin& inValue) { IDocProperty::SetPropertyPerRef<sDocPropPaddingMargin>( static_cast<VDocNode *>(this), kDOC_PROP_MARGIN, inValue); }

RGBAColor VDocNode::GetBackgroundColor() const { return IDocProperty::GetProperty<RGBAColor>( static_cast<const VDocNode *>(this), kDOC_PROP_BACKGROUND_COLOR); }
void VDocNode::SetBackgroundColor(const RGBAColor inValue) { IDocProperty::SetProperty<RGBAColor>( static_cast<VDocNode *>(this), kDOC_PROP_BACKGROUND_COLOR, inValue); }


/** append properties from the passed node
@remarks
	if inOnlyIfInheritOrNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties
*/
void VDocNode::AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfInheritOrNotOverriden, bool /*inNoAppendSpanStyles*/)
{
	if (!testAssert(GetType() == inNode->GetType()))
		return;

	MapOfProp::const_iterator it = inNode->fProps.begin();
	for (;it != inNode->fProps.end(); it++)
	{
		if (inOnlyIfInheritOrNotOverriden)
		{
			eDocProperty prop = (eDocProperty)it->first;
			if (!IsOverriden( prop) || IsInherited( prop))
				fProps[prop] = it->second;
		}
		else
			fProps[(eDocProperty)(it->first)] = it->second;
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


/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
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


uLONG VDocText::GetVersion() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_VERSION); }
void VDocText::SetVersion(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_VERSION, inValue); }

bool VDocText::GetWidowsAndOrphans() const { return IDocProperty::GetProperty<bool>( static_cast<const VDocNode *>(this), kDOC_PROP_WIDOWSANDORPHANS); }
void VDocText::SetWidowsAndOrphans(const bool inValue) { IDocProperty::SetProperty<bool>( static_cast<VDocNode *>(this), kDOC_PROP_WIDOWSANDORPHANS, inValue); }

uLONG VDocText::GetDPI() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_DPI); }
void VDocText::SetDPI(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_DPI, inValue); }

uLONG VDocText::GetZoom() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_ZOOM); }
void VDocText::SetZoom(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_ZOOM, inValue); }

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

DialectCode VDocText::GetLang() const { return IDocProperty::GetProperty<DialectCode>( static_cast<const VDocNode *>(this), kDOC_PROP_LANG); }
void VDocText::SetLang(const DialectCode inValue) { IDocProperty::SetProperty<DialectCode>( static_cast<VDocNode *>(this), kDOC_PROP_LANG, inValue); }

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
// remark: for now VDocText contains only one VDocParagraph child node & so methods are for now redirected to the VDocParagraph node
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/** replace current text & character styles 
@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
void VDocText::SetText( const VString& inText, VTreeTextStyle *inStyles, bool inCopyStyles)
{
	xbox_assert(GetVersion() <= kDOC_VERSION_SPAN4D && fChildren.size() >= 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH);

	VDocParagraph *para = dynamic_cast<VDocParagraph *>(fChildren[0].Get());
	para->SetText( inText, inStyles, inCopyStyles);
}

const VString& VDocText::GetText() const
{
	xbox_assert(GetVersion() <= kDOC_VERSION_SPAN4D && fChildren.size() >= 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH);

	const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(fChildren[0].Get());
	return (para->GetText());
}

void VDocText::SetStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	xbox_assert(GetVersion() <= kDOC_VERSION_SPAN4D && fChildren.size() >= 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH);

	VDocParagraph *para = dynamic_cast<VDocParagraph *>(fChildren[0].Get());
	para->SetStyles( inStyles, inCopyStyles);
}

const VTreeTextStyle *VDocText::GetStyles() const
{
	xbox_assert(GetVersion() <= kDOC_VERSION_SPAN4D && fChildren.size() >= 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH);

	const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(fChildren[0].Get());
	return (para->GetStyles());
}

VTreeTextStyle *VDocText::RetainStyles() const
{
	xbox_assert(GetVersion() <= kDOC_VERSION_SPAN4D && fChildren.size() >= 1 && fChildren[0]->GetType() == kDOC_NODE_TYPE_PARAGRAPH);

	const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(fChildren[0].Get());
	return (para->RetainStyles());
}


VDocParagraph *VDocText::RetainFirstParagraph() 
{
	//only one paragraph for now
	if (fChildren.size() > 0)
		return dynamic_cast<VDocParagraph *>(fChildren[0].Retain());
	else
		return NULL;
}

/** replace plain text with computed or source span references 
@param inShowSpanRefs
	false (default): plain text contains span ref computed values if any (4D expression result, url link user text, etc...)
	true: plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
*/
void VDocText::UpdateSpanRefs( bool inShowSpanRefs)
{
	VDocParagraph *para = RetainFirstParagraph();
	if (para)
		para->UpdateSpanRefs( inShowSpanRefs);
	ReleaseRefCountable(&para);
}

/** replace text with span text reference on the specified range
@remarks
	span ref plain text is set here to uniform non-breaking space: 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VDocText::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inNoUpdateRef)
{
	bool updated = false;
	VDocParagraph *para = RetainFirstParagraph();
	if (para)
		updated = para->ReplaceAndOwnSpanRef( inSpanRef, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inNoUpdateRef);
	ReleaseRefCountable(&para);
	return updated;
}


/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range  

	return true if any 4D expression has been replaced with plain text
*/
bool VDocText::FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd)
{
	bool updated = false;
	VDocParagraph *para = RetainFirstParagraph();
	if (para)
		updated = para->FreezeExpressions( inLC, inStart, inEnd);
	ReleaseRefCountable(&para);
	return updated;
}


/** return the first span reference which intersects the passed range */
const VTextStyle *VDocText::GetStyleRefAtRange(const sLONG inStart, const sLONG inEnd)
{
	const VTextStyle *style = NULL;
	VDocParagraph *para = RetainFirstParagraph();
	if (para)
		style = para->GetStyleRefAtRange( inStart, inEnd);
	ReleaseRefCountable(&para);
	return style;
}


