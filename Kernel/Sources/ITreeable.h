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
#ifndef __ITreeable__
#define __ITreeable__

#include "Kernel/Sources/VKernelTypes.h"

BEGIN_TOOLBOX_NAMESPACE

/*!
@class	ITreeable
@abstract Interface that adds GetLeft/GetRight functionnality.
@discussion
An object that inherits this interface can be used by VBinTreeOf.
*/

template<class Type>
class ITreeable
{
public:
	ITreeable():fLeft( NULL),fRight( NULL), fFather(NULL), fBalance(0)	{;};

	Type	*GetLeft() const {return fLeft;};
	void	SetLeft( Type *inLeft) {fLeft = inLeft;};

	Type 	*GetRight() const {return fRight;};
	void	SetRight( Type *inRight) {fRight = inRight;};

	Type 	*GetFather() const {return fFather;};
	void	SetFather( Type *inFather) {fFather = inFather;};

	sWORD  GetBalance() const { return fBalance; };
	void   SetBalance(sWORD inBalance) { fBalance = inBalance; };

	VIndex GetCount() const;
	Type   *Add(Type* x, Boolean *UnBalanced);
	Type*  Find(Type* x) const;
	Type* Remove(Boolean *Smaller, Type* key, Type* &outTheOne);

	Type* Next();

	Type* LeftMost();

	Boolean IsBalanced(sLONG &height);

	void   DeleteAll();
	void   ReleaseAll();

protected:
	Type* left_rotation(Boolean *UnBalanced);
	Type* right_rotation(Boolean *UnBalanced);
	Type* RemoveSearchTree(Type* &treenode, Boolean *Smaller);
	Type* rebalanceRightSubtreeSmaller(Boolean *Smaller);
	Type* rebalanceLeftSubtreeSmaller(Boolean *Smaller);

	Type* NextFather();

	Type	*fLeft;
	Type	*fRight;
	Type  *fFather;
	sWORD fBalance;
};


template<class Type>
Boolean ITreeable<Type>::IsBalanced(sLONG &height)
{
	sLONG leftheight = 0, rightheight = 0;
	Boolean res = true;

	height++;

	if (GetLeft() != NULL)
		res = res && GetLeft()->IsBalanced(leftheight);

	if (GetRight() != NULL)
		res = res && GetRight()->IsBalanced(rightheight);

	if (leftheight > rightheight)
		height = height + leftheight;
	else
		height = height + rightheight;

	if ( (leftheight > (rightheight + 1)) || (rightheight > (leftheight + 1)) )
		res = false;

	return res;
}

template<class Type>
void ITreeable<Type>::DeleteAll()
{
	if (GetLeft() != NULL)
	{
		GetLeft()->DeleteAll();
		delete GetLeft();
	}
	if (GetRight() != NULL)
	{
		GetRight()->DeleteAll();
		delete GetRight();
	}
}


template<class Type>
void ITreeable<Type>::ReleaseAll()
{
	if (GetLeft() != NULL)
	{
		GetLeft()->ReleaseAll();
		GetLeft()->Release();
	}
	if (GetRight() != NULL)
	{
		GetRight()->ReleaseAll();
		GetRight()->Release();
	}
}


template<class Type>
VIndex ITreeable<Type>::GetCount() const
{
	VIndex count = 1;
	if (GetLeft() != NULL)
		count = count + GetLeft()->GetCount();
	if (GetRight() != NULL)
		count = count + GetRight()->GetCount();
	return count;
}


template<class Type>
Type* ITreeable<Type>::Find(Type* x) const
{
	Type* res = NULL;

	if (((Type*)this)->IsEqualTo((Type*)x))
	{
		res = (Type*)this;
	}
	else
	{
		if (((Type*)this)->IsLessThan(x))
		{
			if (GetRight() != NULL)
				res = GetRight()->Find(x);
		}
		else
		{
			if (GetLeft() != NULL)
				res = GetLeft()->Find(x);
		}
	}

	return(res);
}


template<class Type>
Type* ITreeable<Type>::LeftMost()
{
	Type* res = (Type*)this;
	if (GetLeft() != NULL)
		res = GetLeft()->LeftMost();

	return res;
}


template<class Type>
Type* ITreeable<Type>::NextFather()
{
	Type* res = NULL;

	if (GetFather() != NULL)
	{
		if (GetFather()->GetRight() == (Type*)this)
		{
			res = GetFather()->NextFather();
		}
		else
			res = GetFather();
	}
	return res;
}


template<class Type>
Type* ITreeable<Type>::Next()
{
	Type* res;

	if (GetRight() != NULL)
		res = GetRight()->LeftMost();
	else
		res = NextFather();

	return res;
}


template<class Type>
Type* ITreeable<Type>::Add(Type* x, Boolean *UnBalanced)
{
	Type* res = (Type*)this;

	if (x->IsLessThan((Type*)this))
	{
		if (GetLeft() == NULL)
		{
			SetLeft(x);
			*UnBalanced = true;
		}
		else
		{
			SetLeft(GetLeft()->Add(x, UnBalanced));
		}

		GetLeft()->SetFather((Type*)this);

		if (*UnBalanced)
		{
			switch(GetBalance())
			{
			case -1 : 
				SetBalance(0);
				*UnBalanced = false;
				break;

			case  0 : 
				SetBalance(1);
				break;

			case  1 : 
				res = left_rotation(UnBalanced);
				break;

			}
		}

	}
	else
	{
		if (GetRight() == NULL)
		{
			SetRight(x);
			*UnBalanced = true;
		}
		else
		{
			SetRight(GetRight()->Add(x, UnBalanced));
		}

		GetRight()->SetFather((Type*)this);

		if (*UnBalanced)
		{
			switch(GetBalance())
			{
			case 1 : 
				SetBalance(0);
				*UnBalanced = false;
				break;

			case  0 : 
				SetBalance(-1);
				break;

			case  -1 : 
				res = right_rotation(UnBalanced);
				break;

			}

		}
	}

	return(res);
}


template<class Type>
Type* ITreeable<Type>::left_rotation(Boolean *UnBalanced)
{
	Type *petitfils, *fils;
	Type *res = (Type*)this;

	fils = GetLeft();
	if (fils->GetBalance() == 1)
	{
		SetLeft(fils->GetRight());
		if (fils->GetRight() != NULL)
			fils->GetRight()->SetFather((Type*)this);

		fils->SetRight((Type*)this);
		SetFather(fils);

		SetBalance(0);
		res = fils;
	}
	else
	{
		petitfils = fils->GetRight();

		fils->SetRight(petitfils->GetLeft());
		if (petitfils->GetLeft() != NULL)
			petitfils->GetLeft()->SetFather(fils);

		petitfils->SetLeft(fils);
		fils->SetFather(petitfils);

		SetLeft(petitfils->GetRight());
		if (petitfils->GetRight() != NULL)
			petitfils->GetRight()->SetFather((Type*)this);

		petitfils->SetRight((Type*)this);
		SetFather(petitfils);

		switch(petitfils->GetBalance())
		{
		case  1 : 
			SetBalance( -1);
			fils->SetBalance(0);
			break;
		case  0 :
			SetBalance(0);
			fils->SetBalance(0);
			break;
		case -1 : 
			SetBalance (0);
			fils->SetBalance(1);
			break;
		}
		res = petitfils;
	}

	res->SetBalance(0);
	*UnBalanced = false;

	return(res);
}


template<class Type>
Type* ITreeable<Type>::right_rotation(Boolean *UnBalanced)
{

	Type *petitfils, *fils;
	Type *res = (Type*)this;

	fils = GetRight();
	if (fils->GetBalance() == -1)
	{
		SetRight(fils->GetLeft());
		if (fils->GetLeft() != NULL)
			fils->GetLeft()->SetFather((Type*)this);

		fils->SetLeft((Type*)this);
		SetFather(fils);

		SetBalance(0);
		res = fils;
	}
	else
	{
		petitfils = fils->GetLeft();

		fils->SetLeft(petitfils->GetRight());
		if (petitfils->GetRight() != NULL)
			petitfils->GetRight()->SetFather(fils);

		petitfils->SetRight(fils);
		fils->SetFather(petitfils);

		SetRight(petitfils->GetLeft());
		if (petitfils->GetLeft() != NULL)
			petitfils->GetLeft()->SetFather((Type*)this);

		petitfils->SetLeft((Type*)this);
		SetFather(petitfils);

		switch(petitfils->GetBalance())
		{
		case -1 :
			SetBalance(1);
			fils->SetBalance(0);
			break;
		case  0 :
			SetBalance(0);
			fils->SetBalance(0);
			break;
		case  1 :
			SetBalance(0);
			fils->SetBalance(-1);
			break;
		}
		res = petitfils;
	}

	res->SetBalance(0);
	*UnBalanced = false;

	return(res);
}



template<class Type>
Type* ITreeable<Type>::Remove(Boolean *Smaller, Type* key, Type* &outTheOne)
{
	Type *res = (Type*)this;

	if (((Type*)this)->IsEqualTo(key))
	{
		outTheOne = (Type*)this;
		if (GetLeft() == NULL)
		{
			res = GetRight();
			*Smaller = true;
		}
		else
		{
			if (GetRight() == NULL)
			{
				res = GetLeft();
				*Smaller = true;
			}
			else
			{
				Type* child = GetLeft();
				res = RemoveSearchTree(child, Smaller);
				res->SetLeft(child);
				res->SetRight(GetRight());
				if (GetRight() != NULL)
					GetRight()->SetFather(res);
				if (child != NULL)
					child->SetFather(res);
			}
		}
	}
	else
	{
		if (((Type*)this)->IsLessThan(key))
		{
			if (GetRight() != NULL)
			{
				SetRight(GetRight()->Remove(Smaller, key, outTheOne));
				if (GetRight() != NULL)
					GetRight()->SetFather((Type*)this);
				if (*Smaller)
					res = rebalanceRightSubtreeSmaller(Smaller);
			}
		}
		else
		{
			if (GetLeft() != NULL)
			{
				SetLeft(GetLeft()->Remove(Smaller, key, outTheOne));
				if (GetLeft() != NULL)
					GetLeft()->SetFather((Type*)this);
				if (*Smaller)
					res = rebalanceLeftSubtreeSmaller(Smaller);
			}
		}
	}

	if (res != NULL)
		res->SetFather(GetFather());
	return res;
}


template<class Type>
Type* ITreeable<Type>::RemoveSearchTree(Type* &TreeNode, Boolean *Smaller)
{
	Type *res = (Type*)this;
	if (TreeNode->GetRight() == NULL)
	{
		res = TreeNode;
		TreeNode = TreeNode->GetLeft();
		*Smaller = true;
	}
	else
	{
		Type* child = TreeNode->GetRight();
		res = RemoveSearchTree(child, Smaller);
		TreeNode->SetRight(child);
		if (child != NULL)
			child->SetFather(TreeNode);
		if (*Smaller)
			TreeNode = TreeNode->rebalanceRightSubtreeSmaller(Smaller);
	}

	return res;
}


template<class Type>
Type* ITreeable<Type>::rebalanceLeftSubtreeSmaller(Boolean *Smaller)
{
	Type *res = (Type*)this;
	Type *child, *grandchild;

	switch(GetBalance())
	{
		case 0:
			SetBalance(-1);
			*Smaller = false;
			break;

		case 1:
			SetBalance(0);
			break;

		case -1:
			child = GetRight();
			switch(child->GetBalance())
			{
				case 0:
				case -1:
					SetRight(child->GetLeft());
					if (child->GetLeft() != NULL)
						child->GetLeft()->SetFather((Type*)this);
					child->SetLeft((Type*)this);
					SetFather(child);
					if (child->GetBalance() == 0)
					{
						*Smaller = false;
						child->SetBalance(1);
						SetBalance(-1);
					}
					else
					{
						child->SetBalance(0);
						SetBalance(0);
					}
					res = child;
					break;

				case 1:
					grandchild = child->GetLeft();
					child->SetLeft(grandchild->GetRight());
					if (grandchild->GetRight() != NULL)
						grandchild->GetRight()->SetFather(child);
					grandchild->SetRight(child);
					child->SetFather(grandchild);
					SetRight(grandchild->GetLeft());
					if (grandchild->GetLeft() != NULL)
						grandchild->GetLeft()->SetFather((Type*)this);
					grandchild->SetLeft((Type*)this);
					SetFather(grandchild);
					if (grandchild->GetBalance() == 1)
					{
						child->SetBalance(-1);
						SetBalance(-1);
					}
					else
					{
						child->SetBalance(0);
						SetBalance(0);
					}
					grandchild->SetBalance(0);
					res = grandchild;
					break;
			}
			break;

	
	}

	return res;
}



template<class Type>
Type* ITreeable<Type>::rebalanceRightSubtreeSmaller(Boolean *Smaller)
{
	Type *res = (Type*)this;
	Type *child, *grandchild;

	switch(GetBalance())
	{
		case 0:
			SetBalance(1);
			*Smaller = false;
			break;

		case -1:
			SetBalance(0);
			break;

		case 1:
			child = GetLeft();
			switch(child->GetBalance())
			{
			case 0:
			case 1:
				SetLeft(child->GetRight());
				if (child->GetRight() != NULL)
					child->GetRight()->SetFather((Type*)this);
				child->SetRight((Type*)this);
				SetFather(child);
				if (child->GetBalance() == 0)
				{
					*Smaller = false;
					child->SetBalance(-1);
					SetBalance(1);
				}
				else
				{
					child->SetBalance(0);
					SetBalance(0);
				}
				res = child;
				break;

			case -1:
				grandchild = child->GetRight();
				child->SetRight(grandchild->GetLeft());
				if (grandchild->GetLeft() != NULL)
					grandchild->GetLeft()->SetFather(child);
				grandchild->SetLeft(child);
				child->SetFather(child);
				SetLeft(grandchild->GetRight());
				if (grandchild->GetRight() != NULL)
					grandchild->GetRight()->SetFather((Type*)this);
				grandchild->SetRight((Type*)this);
				SetFather(grandchild);
				if (grandchild->GetBalance() == -1)
				{
					child->SetBalance(1);
					SetBalance(1);
				}
				else
				{
					child->SetBalance(0);
					SetBalance(0);
				}
				grandchild->SetBalance(0);
				res = grandchild;
				break;
			}
			break;


	}

	return res;
}

END_TOOLBOX_NAMESPACE

#endif