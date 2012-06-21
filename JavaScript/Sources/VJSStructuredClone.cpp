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
#include "VJavaScriptPrecompiled.h"

#include "VJSStructuredClone.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

USING_TOOLBOX_NAMESPACE

VJSStructuredClone *VJSStructuredClone::RetainClone (XBOX::VJSValue inValue)
{	
	std::map<JS4D::ValueRef, SNode *>	alreadyCreated;
	std::list<SEntry>					toDoList;
	SNode								*root;

	if ((root = _ValueToNode(inValue, &alreadyCreated, &toDoList)) == NULL)

		return NULL;

	while (!toDoList.empty()) {

		// Property iterator will also iterate Array object indexes (they are converted into string).

		XBOX::VJSValue				value(inValue.GetContext(), toDoList.front().fValueRef);
		XBOX::VJSPropertyIterator	i(value.GetObject());
		SNode						*p, *q;

		p = toDoList.front().fNode;
		xbox_assert(p->fType == eNODE_OBJECT || p->fType == eNODE_ARRAY);
		
		toDoList.pop_front();

		// Object or Array with no attributes?

		if (!i.IsValid()) 

			continue;

		// Set first child.

		value = i.GetProperty();
		if ((p->fValue.fFirstChild = _ValueToNode(value, &alreadyCreated, &toDoList)) == NULL) {

			_FreeNode(root);
			root = NULL;			
			break;

		} 		
		q = p->fValue.fFirstChild;
		i.GetPropertyName(q->fName);		
		++i;

		// Iterate sibling(s).

		for ( ; i.IsValid(); ++i) {

			value = i.GetProperty();
			if ((p = _ValueToNode(value, &alreadyCreated, &toDoList)) == NULL) 

				break;

			else {

				q = q->fNextSibling = p;
				i.GetPropertyName(p->fName);

			}

		}

		q->fNextSibling = NULL;
		if (i.IsValid()) {

			_FreeNode(root);
			root = NULL;
			break;

		} 	

	}

	if (root != NULL) {

		VJSStructuredClone	*structuredClone;

		if ((structuredClone = new VJSStructuredClone()) != NULL)

			structuredClone->fRoot = root;

		else

			_FreeNode(root);

		return structuredClone;

	} else

		return NULL;
}

VJSStructuredClone* VJSStructuredClone::RetainCloneForVValueSingle( const XBOX::VValueSingle& inValue)
{
	VJSStructuredClone *structuredClone = NULL;

	SNode *valueNode = _VValueSingleToNode( inValue);
	if (valueNode != NULL)
	{	
		valueNode->fNextSibling = NULL;
		if ((structuredClone = new VJSStructuredClone()) != NULL)
			structuredClone->fRoot = valueNode;
		else
			_FreeNode( valueNode);
	}

	return structuredClone;
}

VJSStructuredClone* VJSStructuredClone::RetainCloneForVBagArray( const XBOX::VBagArray& inBagArray, bool inUniqueElementsAreNotArrays)
{
	VJSStructuredClone *structuredClone = NULL;

	SNode *arrayNode = _VBagArrayToNode( inBagArray, inUniqueElementsAreNotArrays);
	if (arrayNode != NULL)
	{	
		arrayNode->fNextSibling = NULL;
		if ((structuredClone = new VJSStructuredClone()) != NULL)
			structuredClone->fRoot = arrayNode;
		else
			_FreeNode( arrayNode);
	}

	return structuredClone;
}
	
VJSStructuredClone* VJSStructuredClone::RetainCloneForVValueBag( const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays)
{
	VJSStructuredClone *structuredClone = NULL;

	SNode *bagNode = _VValueBagToNode( inBag, inUniqueElementsAreNotArrays);
	if (bagNode != NULL)
	{
		bagNode->fNextSibling = NULL;
		if ((structuredClone = new VJSStructuredClone()) != NULL)
			structuredClone->fRoot = bagNode;
		else
			_FreeNode( bagNode);
	}

	return structuredClone;

}

XBOX::VJSValue VJSStructuredClone::MakeValue (XBOX::VJSContext inContext)
{
	XBOX::VJSValue	root(inContext);

	if (fRoot == NULL)

		root.SetUndefined();

	else {
		
		std::map<SNode *, JS4D::ValueRef>	alreadyCreated;
		std::list<SEntry>					toDoList;

		xbox_assert(fRoot->fType != eNODE_REFERENCE);
		root = _NodeToValue(inContext, fRoot, &alreadyCreated, &toDoList);
		while (!toDoList.empty()) {

			XBOX::VJSObject	object(inContext);
			SNode			*p;

			object.SetObjectRef((JS4D::ObjectRef) toDoList.front().fValueRef);
			p = toDoList.front().fNode;
			xbox_assert(p->fType == eNODE_OBJECT || p->fType == eNODE_ARRAY);

			p = p->fValue.fFirstChild;
			while (p != NULL) {

				object.SetProperty(p->fName, _NodeToValue(inContext, p, &alreadyCreated, &toDoList));
				p = p->fNextSibling;

			}
			
			toDoList.pop_front();

		}

	} 

	return root;
}

VJSStructuredClone::VJSStructuredClone ()
{
	fRoot = NULL;
}

VJSStructuredClone::~VJSStructuredClone ()
{
	if (fRoot != NULL)

		_FreeNode(fRoot);
}

// _ValueToNode() allocate memory for the node, then set fType member, and if needed, set fValue. fName and fNextSibling are untouched.

VJSStructuredClone::SNode *VJSStructuredClone::_ValueToNode (XBOX::VJSValue inValue, std::map<JS4D::ValueRef, SNode *> *ioAlreadyCloned, std::list<SEntry> *ioToDoList)
{
	xbox_assert(ioAlreadyCloned != NULL && ioToDoList != NULL);

	SNode	*p;

	if ((p = new SNode()) == NULL)

		return NULL;

	switch (inValue.GetType()) {

		case VJSValue::eTYPE_UNDEFINED: 

			p->fType = eNODE_UNDEFINED;
			break;

		case VJSValue::eTYPE_NULL:

			p->fType = eNODE_NULL;
			break;

		case VJSValue::eTYPE_BOOLEAN:

			p->fType = eNODE_BOOLEAN;
			if (!inValue.GetBool(&p->fValue.fBoolean)) {

				delete p;
				p = NULL;

			}
			break;
    	
		case VJSValue::eTYPE_NUMBER:

			p->fType = eNODE_NUMBER;
			if (!inValue.GetReal(&p->fValue.fNumber)) {

				delete p;
				p = NULL;

			}
			break;

		case VJSValue::eTYPE_STRING: {

			XBOX::VString	string;

			p->fType = eNODE_STRING;
			if (!inValue.GetString(string) || (p->fValue.fString = new XBOX::VString(string)) == NULL) {

				delete p;
				p = NULL;

			}

			break;

		}

		case VJSValue::eTYPE_OBJECT: {

			std::vector<VJSValue>	emptyArgument;	
			XBOX::VString			string;

			if (inValue.IsInstanceOf("Boolean")) {

				p->fType = eNODE_BOOLEAN_OBJECT;
				if (!inValue.GetObject().CallMemberFunction("valueOf", &emptyArgument, &inValue, NULL)
				|| !inValue.GetBool(&p->fValue.fBoolean)) {

					delete p;
					p = NULL;

				}

			} else if (inValue.IsInstanceOf("Number")) {

				p->fType = eNODE_NUMBER_OBJECT;
				if (!inValue.GetObject().CallMemberFunction("valueOf", &emptyArgument, &inValue, NULL)
				|| !inValue.GetReal(&p->fValue.fNumber)) {

					delete p;
					p = NULL;

				}

			} else if (inValue.IsInstanceOf("String")) {

				p->fType = eNODE_STRING_OBJECT;
				if (!inValue.GetObject().CallMemberFunction("valueOf", &emptyArgument, &inValue, NULL)
				|| !inValue.GetString(string)
				|| (p->fValue.fString = new XBOX::VString(string)) == NULL) {
			
					delete p;
					p = NULL;

				}

			} else if (inValue.IsInstanceOf("Date")) {

				// getTime() will return the date as milliseconds since 1-01-1970 (UNIX time).

				p->fType = eNODE_DATE_OBJECT;
				if (!inValue.GetObject().CallMemberFunction("getTime", &emptyArgument, &inValue, NULL)
				|| !inValue.GetReal(&p->fValue.fNumber)) {
			
					delete p;
					p = NULL;

				}

			} else if (inValue.IsInstanceOf("RegExp")) {

				// toString() will return the "complete" (along with modifier flag(s)) regular expression. 
			
				p->fType = eNODE_REG_EXP_OBJECT;
				if (!inValue.GetObject().CallMemberFunction("toString", &emptyArgument, &inValue, NULL)
				|| !inValue.GetString(string)
				|| (p->fValue.fString = new XBOX::VString(string)) == NULL) {
			
					delete p;
					p = NULL;

				}
				
			} else if (_IsSerializable(inValue)) {
				
				// Serialize object if possible.
				
				p->fType = eNODE_SERIALIZABLE;
				if ((p->fValue.fSerializationData.fConstructorName = new XBOX::VString()) == NULL) {
					
					delete p;
					p = NULL;
				
				} else if ((p->fValue.fSerializationData.fJSON = new XBOX::VString()) == NULL) {
					
					delete p->fValue.fSerializationData.fConstructorName;
					delete p;
					p = NULL;
				
				} else {
					
					if (!inValue.GetObject().GetPropertyAsString("constructorName", NULL, *p->fValue.fSerializationData.fConstructorName)
					|| !inValue.GetObject().CallMemberFunction("serialize", &emptyArgument, &inValue, NULL)
					|| !inValue.GetString(*p->fValue.fSerializationData.fJSON)) {
					
						_FreeNode(p);
						p = NULL;
						
					}					
					
				}				

			} else if (inValue.IsFunction()) {

				delete p;
				p = NULL;

			} else {

				// Object or Array.

				std::map<JS4D::ValueRef, SNode *>::iterator	j;	

				if ((j = ioAlreadyCloned->find(inValue.GetValueRef())) != ioAlreadyCloned->end()) {

					// Already cloned.

					xbox_assert(inValue.IsObject());

					p->fType = eNODE_REFERENCE;
					p->fValue.fReference = j->second;

				} else {

					SEntry	entry;

					p->fType = inValue.IsArray() ? eNODE_ARRAY : eNODE_OBJECT;
					p->fValue.fFirstChild = NULL;
			
					// Mark as already cloned and add to "todo" list.
			
					(*ioAlreadyCloned)[inValue.GetValueRef()] = p;

					entry.fValueRef = inValue.GetValueRef();
					entry.fNode = p;
					ioToDoList->push_back(entry);

				}
	
			}
			break;

		}

		default:

			xbox_assert(false);
			break;

	}

	return p;
}

VJSStructuredClone::SNode* VJSStructuredClone::_VValueSingleToNode( const XBOX::VValueSingle& inValue)
{
	SNode *valueNode;

	if ((valueNode = new SNode()) == NULL)
		return NULL;

	switch (inValue.GetValueKind())
	{
		case VK_STRING:
		case VK_UUID:
		{
			XBOX::VString val;

			valueNode->fType = eNODE_STRING;
			inValue.GetString( val);
			if ((valueNode->fValue.fString = new XBOX::VString( val)) == NULL)
			{
				delete valueNode;
				valueNode = NULL;
			}
			break;
		}

		case VK_BOOLEAN:
			valueNode->fType = eNODE_BOOLEAN;
			valueNode->fValue.fBoolean = (inValue.GetBoolean()) ? true : false;
			break;

		case VK_BYTE:
		case VK_WORD:
		case VK_LONG:
		case VK_LONG8:
		case VK_REAL:
		case VK_FLOAT:
		case VK_TIME:
		case VK_DURATION:
			valueNode->fType = eNODE_NUMBER;
			valueNode->fValue.fNumber = inValue.GetReal();
			break;

		default:
			xbox_assert( false);
			break;
	}

	return valueNode;
}

VJSStructuredClone::SNode* VJSStructuredClone::_VBagArrayToNode( const XBOX::VBagArray& inBagArray, bool inUniqueElementsAreNotArrays)
{
	SNode *arrayNode;
	
	if ((arrayNode = new SNode()) == NULL)
		return NULL;
	
	arrayNode->fType = eNODE_ARRAY;
	arrayNode->fValue.fFirstChild = NULL;

	SNode *curNode = NULL;

	VIndex elementsCount = inBagArray.GetCount();
	VIndex jsArrayIndex = 0;
	VString propertyName;
	for (VIndex elementIter = 1 ; elementIter <= elementsCount ; ++elementIter)
	{
		const VValueBag *elementBag = inBagArray.GetNth( elementIter);
		if (elementBag != NULL)
		{
			SNode *elemNode = _VValueBagToNode( *elementBag, inUniqueElementsAreNotArrays);
			if (elemNode != NULL)
			{
				elemNode->fName.FromLong( jsArrayIndex++);
				elemNode->fNextSibling = NULL;

				if (curNode == NULL)
					curNode = arrayNode->fValue.fFirstChild = elemNode;
				else
					curNode = curNode->fNextSibling = elemNode;

				if (elementBag->GetAttribute( L"____property_name_in_jsarray", propertyName))
				{
					// Append a property which reference the array element
					elemNode = new SNode();
					if (elemNode != NULL)
					{
						elemNode->fType = eNODE_REFERENCE;
						elemNode->fName.FromString( propertyName);
						elemNode->fValue.fReference = curNode;
						elemNode->fNextSibling = NULL;

						curNode = curNode->fNextSibling = elemNode;
					}
				}
			}
		}
	}
		
	return arrayNode;
}

VJSStructuredClone::SNode* VJSStructuredClone::_VValueBagToNode( const XBOX::VValueBag& inBag, bool inUniqueElementsAreNotArrays)
{
	// inspired from VValueBag::GetJSONString
	SNode *bagNode = NULL;
	
	if ((bagNode = new SNode()) == NULL)
		return NULL;
	
	bagNode->fType = eNODE_OBJECT;
	bagNode->fValue.fFirstChild = NULL;
	
	SNode *curNode = NULL;

	// Iterate the attributes
	VString attName;
	VIndex attCount = inBag.GetAttributesCount();
	for (VIndex attIndex = 1 ; attIndex <= attCount ; ++attIndex)
	{
		const VValueSingle *attValue = inBag.GetNthAttribute( attIndex, &attName);
		if ((attName != L"____objectunic") && (attName != L"____property_name_in_jsarray"))
		{
			SNode *attNode = NULL;

			if (attValue != NULL)
			{
				attNode = _VValueSingleToNode( *attValue);
			}
			else
			{
				VString emptyString;
				attNode = _VValueSingleToNode( emptyString);
			}

			if (attNode != NULL)
			{
				VValueBag::StKey CDataBagKey( attName);
				if (CDataBagKey.Equal( VValueBag::CDataAttributeName()))
					attName = "__cdata";

				attNode->fName.FromString( attName);
				attNode->fNextSibling = NULL;
				if (curNode == NULL)
					curNode = bagNode->fValue.fFirstChild = attNode;
				else
					curNode = curNode->fNextSibling = attNode;
			}
		}
	}

	// Iterate the elements
	VString elementName;
	VIndex elementNamesCount = inBag.GetElementNamesCount();
	for (VIndex elementNamesIndex = 1 ; elementNamesIndex <= elementNamesCount ; ++elementNamesIndex)
	{
		const VBagArray* bagArray = inBag.GetNthElementName( elementNamesIndex, &elementName);
		if (bagArray != NULL)
		{
			SNode *elemNode = NULL;

			if ((bagArray->GetCount() == 1) && inUniqueElementsAreNotArrays)
			{
				elemNode = _VValueBagToNode( *bagArray->GetNth(1), inUniqueElementsAreNotArrays);
			}
			else if (bagArray->GetNth(1)->GetAttribute("____objectunic") != NULL)
			{
				elemNode = _VValueBagToNode( *bagArray->GetNth(1), inUniqueElementsAreNotArrays);
			}
			else
			{
				elemNode = _VBagArrayToNode( *bagArray, inUniqueElementsAreNotArrays);
			}

			if (elemNode != NULL)
			{
				elemNode->fName.FromString( elementName);
				elemNode->fNextSibling = NULL;

				if (curNode == NULL)
					curNode = bagNode->fValue.fFirstChild = elemNode;
				else
					curNode = curNode->fNextSibling = elemNode;
			}
		}
	}

	return bagNode;
}

XBOX::VJSValue VJSStructuredClone::_NodeToValue (XBOX::VJSContext inContext, SNode *inNode, std::map<SNode *, JS4D::ValueRef> *ioAlreadyCreated, std::list<SEntry> *ioToDoList)
{
	xbox_assert(ioAlreadyCreated != NULL && ioToDoList != NULL);

	XBOX::VJSValue	value(inContext);

	switch (inNode->fType) {

		case eNODE_UNDEFINED:
	
			value.SetUndefined();
			break;

		case eNODE_NULL:

			value.SetNull();
			break;

		case eNODE_BOOLEAN:
				
			value.SetBool(inNode->fValue.fBoolean);
			break;

		case eNODE_NUMBER:

			value.SetNumber<Real>(inNode->fValue.fNumber);
			break;

		case eNODE_STRING:

			value.SetString(*inNode->fValue.fString);
			break;

		case eNODE_BOOLEAN_OBJECT: 

			value.SetBool(inNode->fValue.fBoolean);
			value = _ConstructObject(inContext, "Boolean", value);
			break;

		case eNODE_NUMBER_OBJECT:

			value.SetNumber(inNode->fValue.fNumber);
			value = _ConstructObject(inContext, "Number", value);
			break;

		case eNODE_STRING_OBJECT:
		case eNODE_REG_EXP_OBJECT: 

			value.SetString(*inNode->fValue.fString);
			value = _ConstructObject(inContext, inNode->fType == eNODE_STRING_OBJECT ? "String" : "RegExp", value);
			break;

		case eNODE_DATE_OBJECT: 

			value.SetNumber(inNode->fValue.fNumber);
			value = _ConstructObject(inContext, "Date", value);
			break;
			
		case eNODE_SERIALIZABLE:
			
			value.SetString(*inNode->fValue.fSerializationData.fJSON);
			value = _ConstructObject(inContext, *inNode->fValue.fSerializationData.fConstructorName, value);
			break;

		case eNODE_OBJECT: {

			XBOX::VJSObject	emptyObject(inContext);
			SEntry			entry;

			emptyObject.MakeEmpty();
			value.SetValueRef((JS4D::ValueRef) emptyObject.GetObjectRef());

			xbox_assert(ioAlreadyCreated->find(inNode->fValue.fReference) == ioAlreadyCreated->end());

			(*ioAlreadyCreated)[inNode] = value.GetValueRef();

			entry.fValueRef = value.GetValueRef();
			entry.fNode = inNode;
			ioToDoList->push_back(entry);

			break;

		}

		case eNODE_ARRAY: {

			XBOX::VJSArray	emptyArray(inContext);
			SEntry			entry;

			value.SetValueRef((JS4D::ValueRef) emptyArray.GetObjectRef());

			xbox_assert(ioAlreadyCreated->find(inNode->fValue.fReference) == ioAlreadyCreated->end());

			(*ioAlreadyCreated)[inNode] = value.GetValueRef();

			entry.fValueRef = value.GetValueRef();
			entry.fNode = inNode;
			ioToDoList->push_back(entry);
			
			break;

		}

		case eNODE_REFERENCE: {

			std::map<SNode *, JS4D::ValueRef>::iterator it;

			it = ioAlreadyCreated->find(inNode->fValue.fReference);
			xbox_assert(it != ioAlreadyCreated->end());

			value.SetValueRef(it->second);

			break;

		}

		default:

			xbox_assert(false);
			value.SetNull();
			break;
		
	}
	
	return value;
}

bool VJSStructuredClone::_IsSerializable (XBOX::VJSValue inValue)
{
	xbox_assert(inValue.IsObject());
	
	XBOX::VJSObject	object	= inValue.GetObject();
	
	if (object.HasProperty("constructorName") && object.GetProperty("constructorName").IsString()
	&& object.HasProperty("serialize") && object.GetProperty("serialize").IsFunction())
		
		return true;
	
	else
		
		return false;
}

XBOX::VJSValue VJSStructuredClone::_ConstructObject (XBOX::VJSContext inContext, const XBOX::VString &inConstructorName, XBOX::VJSValue inArgument)
{
	XBOX::VJSObject				object = inContext.GetGlobalObject().GetPropertyAsObject(inConstructorName);
	std::vector<XBOX::VJSValue>	arguments;
	XBOX::VJSValue				value(inContext);

	arguments.push_back(inArgument);
	if (object.CallAsConstructor(&arguments, &object, NULL)) 

		value.SetValueRef((JS4D::ObjectRef) object.GetObjectRef());

	else

		value.SetUndefined();

	return value;
}

// Tree "under" the root SNode inNode, must be fully correct (all pointers properly set in particular).

void VJSStructuredClone::_FreeNode (SNode *inNode)
{
	xbox_assert(inNode != NULL);

	std::list<SNode *>	toFree;
	SNode				*p, *q;

	p = inNode;
	for ( ; ; ) {
		
		switch (p->fType) {

			case eNODE_UNDEFINED:
			case eNODE_NULL:
			case eNODE_BOOLEAN:
			case eNODE_NUMBER:
			case eNODE_BOOLEAN_OBJECT:
			case eNODE_NUMBER_OBJECT:
			case eNODE_DATE_OBJECT:
			case eNODE_REFERENCE:

				break;

			case eNODE_STRING:
			case eNODE_STRING_OBJECT:
			case eNODE_REG_EXP_OBJECT:
				
				delete p->fValue.fString;
				break;

			case eNODE_SERIALIZABLE:		

				delete p->fValue.fSerializationData.fConstructorName;
				delete p->fValue.fSerializationData.fJSON;
				break;
				
			case eNODE_OBJECT:
			case eNODE_ARRAY:

				q = p->fValue.fFirstChild;
				while (q != NULL) {			
			
					toFree.push_front(q);
					q = q->fNextSibling;

				}
				break;

			default:	

				xbox_assert(false);
				break;
		
		}				

		delete p;
		if (!toFree.empty()) {

			p = toFree.front();
			toFree.pop_front();

		} else
			
			break;

	}
}