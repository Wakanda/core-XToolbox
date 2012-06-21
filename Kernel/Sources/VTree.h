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
#ifndef __VTree__
#define __VTree__

#include "Kernel/Sources/VObject.h"
#include "Kernel/Sources/ITreeable.h"

BEGIN_TOOLBOX_NAMESPACE

/*!
@class VBinTreeOf
@abstract template class that manages a balanced binarry tree of objects.
@discussion
Type must provide GetLeft/SetLeft & GetRight/SetRight methods.
The ITreeable interface eases the implementation of these methods but is not mandatory.
Type must also provide IsEqualTo and IsLessThan methods
*/

template<class Type>
class VBinTreeOf : public VObject
{
public:
			VBinTreeOf (Boolean DeleteOnRemove = true);
	virtual	~VBinTreeOf ();

	virtual void	Add (Type *inNewElement);
	virtual void	Remove (Type *inElementToDelete);
	Type*	Find (Type *inElementToFind) const;

	Boolean	IsEmpty () const {return fRoot == NULL;}
	VIndex	GetCount() const;

	void	DeleteAll();

	Boolean	IsBalanced();

	Type*	First();

protected:
	Type*	fRoot;
	Boolean	fDeleteOnRemove;
};



template<class Type>
VBinTreeOf<Type>::~VBinTreeOf()
{
	if (fDeleteOnRemove)
	{
		DeleteAll();
	}
}



template<class Type>
VBinTreeOf<Type>::VBinTreeOf(Boolean DeleteOnRemove)
{
	fRoot = NULL;
	fDeleteOnRemove = DeleteOnRemove;
}


template<class Type>
Type* VBinTreeOf<Type>::First()
{
	Type* res = NULL;

	if (fRoot != NULL)
		res = fRoot->LeftMost();

	return res;		
}


template<class Type>
Boolean VBinTreeOf<Type>::IsBalanced()
{
	sLONG height = 0;
	Boolean res = true;

	if (fRoot != NULL)
		res = fRoot->IsBalanced(height);

	return res;
}


template<class Type>
VIndex VBinTreeOf<Type>::GetCount() const
{
	VIndex count = 0;
	if (fRoot != NULL)
	{
		count = fRoot->GetCount();
	}
	return count;
}


template<class Type>
void VBinTreeOf<Type>::Add(Type* inNewElement)
{
	Boolean UnBalanced = false;

	if (fRoot == NULL)
	{
		fRoot = inNewElement;
		fRoot->SetLeft(NULL);
		fRoot->SetRight(NULL);
		fRoot->SetFather(NULL);
		fRoot->SetBalance(0);
	}
	else
	{
		fRoot = fRoot->Add(inNewElement, &UnBalanced);
		fRoot->SetFather(NULL);
	}
}


template<class Type>
Type* VBinTreeOf<Type>::Find(Type* inElementToFind) const
{
	if (fRoot == NULL)
		return NULL;
	else
		return fRoot->Find(inElementToFind);
}


template<class Type>
void VBinTreeOf<Type>::DeleteAll()
{
	if (fRoot != NULL)
	{
		fRoot->DeleteAll();
		delete fRoot;
		fRoot = NULL;
	}
}


template<class Type>
void VBinTreeOf<Type>::Remove(Type* inElementToDelete)
{
	if (fRoot != NULL)
	{
		Type* removedOne = NULL;
		Boolean smaller = false;
		fRoot = fRoot->Remove(&smaller, inElementToDelete, removedOne);
		if (removedOne != NULL)
		{
			if (fDeleteOnRemove)
			{
				delete removedOne;
			}
		}
	}
}



//==================================================================================

template<class Type>
class VRetainBinTreeOf : public VBinTreeOf<Type>
{
public:
	VRetainBinTreeOf();
	virtual ~VRetainBinTreeOf();

	virtual void Add(Type *inNewElement);
	virtual void Remove(Type *inElementToDelete);

	void ReleaseAll();

};


template<class Type>
VRetainBinTreeOf<Type>::~VRetainBinTreeOf()
{
	ReleaseAll();
}


template<class Type>
VRetainBinTreeOf<Type>::VRetainBinTreeOf()
{
	VBinTreeOf<Type>::fRoot = NULL;
	VBinTreeOf<Type>::fDeleteOnRemove = false;
}


template<class Type>
void VRetainBinTreeOf<Type>::ReleaseAll()
{
	if (VBinTreeOf<Type>::fRoot != NULL)
	{
		VBinTreeOf<Type>::fRoot->ReleaseAll();
		VBinTreeOf<Type>::fRoot->Release();
		VBinTreeOf<Type>::fRoot = NULL;
	}
}


template<class Type>
void VRetainBinTreeOf<Type>::Remove(Type* inElementToDelete)
{
	if (VBinTreeOf<Type>::fRoot != NULL)
	{
		Type* removedOne = NULL;
		Boolean smaller = false;
		VBinTreeOf<Type>::fRoot = VBinTreeOf<Type>::fRoot->Remove(&smaller, inElementToDelete, removedOne);
		removedOne->Release();
	}
}


template<class Type>
void VRetainBinTreeOf<Type>::Add(Type* inNewElement)
{
	Boolean UnBalanced = false;

	inNewElement->Retain();

	if (VBinTreeOf<Type>::fRoot == NULL)
	{
		VBinTreeOf<Type>::fRoot = inNewElement;
		VBinTreeOf<Type>::fRoot->SetLeft(NULL);
		VBinTreeOf<Type>::fRoot->SetRight(NULL);
		VBinTreeOf<Type>::fRoot->SetFather(NULL);
		VBinTreeOf<Type>::fRoot->SetBalance(0);
	}
	else
	{
		VBinTreeOf<Type>::fRoot = VBinTreeOf<Type>::fRoot->Add(inNewElement, &UnBalanced);
		VBinTreeOf<Type>::fRoot->SetFather(NULL);
	}
}


END_TOOLBOX_NAMESPACE

#endif