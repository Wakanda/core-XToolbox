
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
#include "VDocText.h"
#include "VString.h"
#include "VIntlMgr.h"

//in order to be able to use std::min && std::max
#undef max
#undef min

USING_TOOLBOX_NAMESPACE

bool VDocNode::sPropInitDone = false;

VCriticalSection VDocNode::sMutex;

/** map of inherited properties */
bool VDocNode::sPropInherited[kDOC_NUM_PROP] = 
{
	false,	//kDOC_PROP_VERSION,
	false,	//kDOC_PROP_WIDOWSANDORPHANS,
	false,	//kDOC_PROP_USERUNIT,
	false,	//kDOC_PROP_DPI,
	false,	//kDOC_PROP_ZOOM,
	false,	//kDOC_PROP_COMPAT_V13,
	false,	//kDOC_PROP_LAYOUT_MODE,
	false,	//kDOC_PROP_SHOW_IMAGES,
	false,	//kDOC_PROP_SHOW_REFERENCES,
	false,	//kDOC_PROP_SHOW_HIDDEN_CHARACTERS,
	true,	//kDOC_PROP_LANG,

	false,	//kDOC_PROP_DATETIMECREATION,
	false,	//kDOC_PROP_DATETIMEMODIFIED,
	false,	//kDOC_PROP_TITLE,
	false,	//kDOC_PROP_SUBJECT,
	false,	//kDOC_PROP_AUTHOR,
	false,	//kDOC_PROP_COMPANY,
	false	//kDOC_PROP_NOTES,
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
	case kPROP_TYPE_LAYOUTMODE:
		fValue.fLayoutMode = inValue.fValue.fLayoutMode;
		break;
	case kPROP_TYPE_CSSUNIT:
		fValue.fCSSUnit = inValue.fValue.fCSSUnit;
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
	case kPROP_TYPE_COLOR:
		fValue.fColor = inValue.fValue.fColor;
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
	case kPROP_TYPE_SMALLREAL:
	case kPROP_TYPE_PERCENT:
		return (fValue.fSmallReal == inValue.fValue.fSmallReal);
		break;
	case kPROP_TYPE_REAL:
		return (fValue.fReal == inValue.fValue.fReal);
		break;
	case kPROP_TYPE_LAYOUTMODE:
		return (fValue.fLayoutMode == inValue.fValue.fLayoutMode);
		break;
	case kPROP_TYPE_CSSUNIT:
		return (fValue.fCSSUnit == inValue.fValue.fCSSUnit);
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
	case kPROP_TYPE_VSTRING:
		return (fString.EqualToStringRaw( inValue.fString));
		break;
	case kPROP_TYPE_VTIME:
		return (fTime == inValue.fTime);
	case kPROP_TYPE_COLOR:
		return (fValue.fColor.color == inValue.fValue.fColor.color
				&&
				fValue.fColor.transparent == inValue.fValue.fColor.transparent);
		break;
	case kPROP_TYPE_INHERIT:
		break;
	default:
		xbox_assert(false);
		break;
	}
	return false;
}


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
	if (it != inNode->fProps.end())
		return (Type)it->second;

	//then check inherited properties
	if (inNode->fParent && inNode->sPropInherited[(uLONG)inProperty])
		return GetProperty<Type>( inNode->fParent, inProperty);

	//finally return global default value 
	return GetPropertyDefault<Type>( inProperty);
}

template <class Type>
const Type& IDocProperty::GetPropertyRef( const VDocNode *inNode, const eDocProperty inProperty) 
{
	//search first local property
	VDocNode::MapOfProp::const_iterator it = inNode->fProps.find( (uLONG)inProperty);
	if (it != inNode->fProps.end())
		return it->second;

	//then check inherited properties
	if (inNode->fParent && inNode->sPropInherited[inProperty])
		return GetPropertyRef<Type>( inNode->fParent, inProperty);

	//finally return global default value 
	return GetPropertyDefaultRef<Type>( inProperty);
}

template <class Type>
void IDocProperty::SetProperty( VDocNode *inNode, const eDocProperty inProperty, const Type inValue)
{
	inNode->fProps[ (uLONG)inProperty] = VDocProperty( inValue);
	inNode->_SyncToJSON( inProperty);
}

template <class Type>
void IDocProperty::SetPropertyPerRef( VDocNode *inNode, const eDocProperty inProperty, const Type& inValue)
{
	inNode->fProps[ (uLONG)inProperty] = VDocProperty( inValue);
	inNode->_SyncToJSON( inProperty);
}



void VDocNode::Init()
{
	sPropDefault[kDOC_PROP_VERSION]					= new VDocProperty((uLONG)1);
	sPropDefault[kDOC_PROP_WIDOWSANDORPHANS]		= new VDocProperty(true);
	sPropDefault[kDOC_PROP_USERUNIT]				= new VDocProperty( kDOC_CSSUNIT_CM);
	sPropDefault[kDOC_PROP_DPI]						= new VDocProperty((uLONG)72);
	sPropDefault[kDOC_PROP_ZOOM]					= new VDocProperty((SmallReal)1.0f);
	sPropDefault[kDOC_PROP_COMPAT_V13]				= new VDocProperty(true);
	sPropDefault[kDOC_PROP_LAYOUT_MODE]				= new VDocProperty(kDOC_LAYOUT_MODE_NORMAL);
	sPropDefault[kDOC_PROP_SHOW_IMAGES]				= new VDocProperty(true);
	sPropDefault[kDOC_PROP_SHOW_REFERENCES]			= new VDocProperty(false);
	sPropDefault[kDOC_PROP_SHOW_HIDDEN_CHARACTERS]	= new VDocProperty(false);
	sPropDefault[kDOC_PROP_LANG]					= new VDocProperty((uLONG)VIntlMgr::ResolveDialectCode(DC_USER));
	VTime time;
	time.FromSystemTime();
	sPropDefault[kDOC_PROP_DATETIMECREATION]		= new VDocProperty(time);
	sPropDefault[kDOC_PROP_DATETIMEMODIFIED]		= new VDocProperty(time);
	sPropDefault[kDOC_PROP_TITLE]					= new VDocProperty(VString("Document1"));
	sPropDefault[kDOC_PROP_SUBJECT]					= new VDocProperty(VString(""));
	VString userName;
	VSystem::GetLoginUserName( userName);
	sPropDefault[kDOC_PROP_AUTHOR]					= new VDocProperty(userName);
	sPropDefault[kDOC_PROP_COMPANY]					= new VDocProperty(VString(""));
	sPropDefault[kDOC_PROP_NOTES]					= new VDocProperty(VString(""));

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

void VDocNode::_GenerateID(VString& outID)
{
	uLONG id = 0;
	VDocText *doc = static_cast<VDocText *>(fDoc);
	if (doc) //if detached -> id is irrelevant 
			 //(id searching is done only from a VDocText 
			 // & ids are generated on node attach event to ensure new attached nodes use unique ids)
		id = doc->_AllocID();
	outID.FromHexLong((uLONG8)id, false); //not pretty formating to make string smallest possible
}

void VDocNode::_OnAttachToDocument(VDocNode *inDocumentNode)
{
	xbox_assert(inDocumentNode && inDocumentNode->fType == kDOC_NODE_TYPE_DOCUMENT);

	//update document reference
	if (!fDoc)
		fDoc = inDocumentNode;

	//update ID (to ensure this node & children nodes use unique IDs for this document)
	_GenerateID(fID);

	//update document map of node per ID
	(static_cast<VDocText *>(fDoc))->fMapOfNodePerID[fID] = this;

	//update children
	VectorOfNode::iterator itNode = fChildren.begin();
	for(;itNode != fChildren.end(); itNode++)
		itNode->Get()->_OnAttachToDocument( inDocumentNode);

	fDirtyStamp++;
}

void VDocNode::_OnDetachFromDocument(VDocNode *inDocumentNode)
{
	VDocText *docText = static_cast<VDocText *>(inDocumentNode);
	xbox_assert(docText);

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

	VRefPtr<VDocNode> protect(this); //might be detached before attach (if moved) & only one ref

	if (inNode->fParent)
	{
		if (inNode->fParent == this)
			return;
		inNode->Detach();
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
	fDoc = NULL;
}


/** create JSONDocProps & sync it with this node
@remarks
	should be called only if needed (for instance if caller need com with 4D language)
*/
void VDocNode::InitJSONProps()
{
	if (!fJSONProps.IsNull())
		return;

	fJSONProps = new VJSONDocProps(this);
}


/** synchronize the passed property or all properties to JSON (only if JSON object is created) */
void VDocNode::_SyncToJSON(const eDocProperty inPropID)
{
	if (fJSONProps.IsNull())
		return;
	
	//TODO

	fJSONPropsDirtyStamp = fJSONProps->GetDirtyStamp();
}

/** synchronize the passed property (using JSON prop name) or all properties from JSON object to this node */
void VDocNode::_SyncFromJSON( const VString& inPropJSONName)
{
	if (fJSONProps.IsNull())
		return;
	
	//TODO

	fJSONPropsDirtyStamp = fJSONProps->GetDirtyStamp();
}


VJSONDocProps::VJSONDocProps(VDocNode *inNode):VJSONObject() 
{ 
	xbox_assert(inNode); 
	fDocNode = inNode; 
	fDirtyStamp = 1; 
	fDocNode->_SyncToJSON();
}


// if inValue is not JSON_undefined, SetProperty sets the property named inName with specified value, replacing previous one if it exists.
// else the property is removed from the collection.
bool VJSONDocProps::SetProperty( const VString& inName, const VJSONValue& inValue) 
{
	bool result = VJSONObject::SetProperty( inName, inValue);
	if (result)
	{
		fDirtyStamp++;
		fDocNode->_SyncFromJSON( inName);
	}
	return result;
}

// remove the property named inName.
// equivalent to SetProperty( inName, VJSONValue( JSON_undefined));
void VJSONDocProps::RemoveProperty( const VString& inName)
{
	VJSONObject::RemoveProperty( inName);
	fDirtyStamp++;
	fDocNode->_SyncFromJSON( inName);
}

// Remove all properties.
void VJSONDocProps::Clear()
{
	VJSONObject::Clear();
	fDirtyStamp++;
	fDocNode->_SyncFromJSON();
}


/** retain node which matches the passed ID (will return always NULL if node first parent is not a VDocText) */
VDocNode *VDocText::RetainNode(const VString& inID)
{
	MapOfNodePerID::iterator itNode = fMapOfNodePerID.find( inID);
	if (itNode != fMapOfNodePerID.end())
		return itNode->second.Retain();
	return NULL;
}
