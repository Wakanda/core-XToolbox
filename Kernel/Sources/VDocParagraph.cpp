
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

//in order to be able to use std::min && std::max
#undef max
#undef min

USING_TOOLBOX_NAMESPACE

VString VDocSpanTextRef::fNBSP = CVSTR("\240"); //no-breaking space

#define IDSTR_COLOR_HIGHLIGHT		0xffe6e6e6
#define IDSTR_COLOR_LINK			0xff0000ff

VTextStyle IDocSpanTextRef::fStyleHighlight;
VTextStyle IDocSpanTextRef::fStyleLink;
VTextStyle IDocSpanTextRef::fStyleLinkAndHighlight;
VTextStyle IDocSpanTextRef::fStyleLinkActive;
VTextStyle IDocSpanTextRef::fStyleLinkActiveAndHighlight;

IDocSpanTextRef VDocSpanTextRef::fDefaultDelegate;
IDocSpanTextRef *VDocSpanTextRef::fDelegate = NULL;

/** evaluate source ref & return computed value 
@remarks
	on default, if not 4D expression, outValue = *inDefaultValue
				if 4D expression, outValue = "#NOT IMPL#"
*/
void IDocSpanTextRef::IDSTR_Eval(const eDocSpanTextRef inType, const VString& inRef, VString& outValue, const VString& inDefaultValue, VDBLanguageContext *inLC)
{
	if (inType != kSpanTextRef_4DExp)
		outValue = inDefaultValue;
	else
		outValue = CVSTR("#NOT IMPL#");
}

/** return custom text style which overrides current span reference style 
@remarks
	on default if inIsHighlighted == true, apply a light gray background
	if 4D exp -> none style overriden
	if url -> apply char blue color & underline
	if user -> none style overriden
	if unknown tag -> none style overriden
*/
const VTextStyle *IDocSpanTextRef::IDSTR_GetStyle(const eDocSpanTextRef inType, bool inShowRef, bool inIsHighlighted, bool inIsMouseOver)
{
	static bool fIDSTR_Initialized = false;
	if (!fIDSTR_Initialized)
	{
		fStyleHighlight.SetHasBackColor(true);
		fStyleHighlight.SetBackGroundColor( IDSTR_COLOR_HIGHLIGHT);

		fStyleLinkAndHighlight.SetHasForeColor(true);
		fStyleLinkAndHighlight.SetColor( IDSTR_COLOR_LINK);
		fStyleLinkAndHighlight.SetUnderline( false);
		fStyleLinkAndHighlight.SetHasBackColor(true);
		fStyleLinkAndHighlight.SetBackGroundColor( IDSTR_COLOR_HIGHLIGHT);

		fStyleLink.SetHasForeColor(true);
		fStyleLink.SetColor( IDSTR_COLOR_LINK);
		fStyleLink.SetUnderline( false);

		fStyleLinkActiveAndHighlight.SetHasForeColor(true);
		fStyleLinkActiveAndHighlight.SetColor( IDSTR_COLOR_LINK);
		fStyleLinkActiveAndHighlight.SetUnderline( true);
		fStyleLinkActiveAndHighlight.SetHasBackColor(true);
		fStyleLinkActiveAndHighlight.SetBackGroundColor( IDSTR_COLOR_HIGHLIGHT);

		fStyleLinkActive.SetHasForeColor(true);
		fStyleLinkActive.SetColor( IDSTR_COLOR_LINK);
		fStyleLinkActive.SetUnderline( true);

		fIDSTR_Initialized = true;
	}

	switch ( inType)
	{
	case kSpanTextRef_URL:
	case kSpanTextRef_User:
		if (inIsMouseOver)
		{
			if (inIsHighlighted)
				return &fStyleLinkActiveAndHighlight;
			else
				return &fStyleLinkActive;
		}	 
		else if (inIsHighlighted)
			return &fStyleLinkAndHighlight;
		else
			return &fStyleLink;
		break;
	case kSpanTextRef_4DExp:
	case kSpanTextRef_UnknownTag:
		if (inShowRef || inIsHighlighted)
			//always highlight while displaying 4D tokenized expression or embedded tag
			return &fStyleHighlight;
		break;
	default:
		if (inIsHighlighted)
			return &fStyleHighlight;
	}
	return NULL;
}


/** set span ref delegate */
void VDocSpanTextRef::SetDelegate( IDocSpanTextRef *inDelegate)
{
	if (!inDelegate)
		fDelegate = &fDefaultDelegate;
	else
		fDelegate = inDelegate;
}
IDocSpanTextRef *VDocSpanTextRef::GetDelegate()
{ 
	if (!fDelegate)
		fDelegate = &fDefaultDelegate;

	return fDelegate; 
}


/** return 4D expression value, as evaluated from the passed 4D tokenized expression
@remarks
	returns cached value if 4D exp has been evaluated yet
*/
void VDocCache4DExp::Get( const VString& inExp, VString& outValue, VDBLanguageContext *inLC)
{
	if (inLC && inLC != fDBLanguageContext)
	{
		fDBLanguageContext = inLC;
		fMapOfExp.clear();
	}

	if (!testAssert(fDBLanguageContext) || !testAssert(fSpanRef))
	{
		outValue.SetEmpty();
		return;
	}
	MapOfExp::iterator itExp = fMapOfExp.find( inExp);
	if (itExp != fMapOfExp.end())
	{
		if (fEvalAlways)
		{
			if (fEvalDeferred)
			{
				//defer exp eval
				if (fMapOfDeferredExp.size() < fMaxSize)
					fMapOfDeferredExp[ inExp ] = true;
			}
			else
			{
				//eval & update cached value
				fSpanRef->SetRef( inExp);
				fSpanRef->EvalExpression( fDBLanguageContext);
				itExp->second.second = fSpanRef->GetComputedValue();
			}
		}
		itExp->second.first = (uLONG)VSystem::GetCurrentTime(); //update last read access time
		outValue = itExp->second.second; //return exp value
		return;
	}

	if (fEvalDeferred)
	{
		//defer exp eval
		if (fMapOfDeferredExp.size() < fMaxSize)
			fMapOfDeferredExp[ inExp ] = true;
		outValue.SetEmpty();
		return;
	}

	fSpanRef->SetRef( inExp);
	fSpanRef->EvalExpression( fDBLanguageContext);
	outValue = fSpanRef->GetComputedValue();
	while (fMapOfExp.size() >= fMaxSize)
	{
		//remove the oldest entry
		MapOfExp::iterator itToRemove = fMapOfExp.begin();
		for (itExp = fMapOfExp.begin(); itExp != fMapOfExp.end(); itExp++)
		{
			if (itExp->second.first < itToRemove->second.first)
				itToRemove = itExp;
		}
		fMapOfExp.erase(itToRemove);
	}
	fMapOfExp[ inExp ] = ExpValue((uLONG)VSystem::GetCurrentTime(),outValue);
}

/** manually add a pair <exp,value> to the cache
@remarks
	add or replace the existing value 

	caution: it does not check if value if valid according to the expression so it might be anything
*/
void VDocCache4DExp::Set( const VString& inExp, const VString& inValue)
{
	MapOfExp::iterator itExp = fMapOfExp.find( inExp);
	if (itExp != fMapOfExp.end())
	{
		itExp->second.first = (uLONG)VSystem::GetCurrentTime();
		itExp->second.second = inValue;
		return;
	}

	while (fMapOfExp.size() >= fMaxSize)
	{
		//remove the oldest entry
		MapOfExp::iterator itToRemove = fMapOfExp.begin();
		for (itExp = fMapOfExp.begin(); itExp != fMapOfExp.end(); itExp++)
		{
			if (itExp->second.first < itToRemove->second.first)
				itToRemove = itExp;
		}
		fMapOfExp.erase(itToRemove);
	}
	fMapOfExp[ inExp] = ExpValue((uLONG)VSystem::GetCurrentTime(), inValue);
}

/** append cached expressions from the passed cache */
void VDocCache4DExp::AppendFromCache( const VDocCache4DExp *inCache)
{
	MapOfExp::const_iterator itExp = inCache->fMapOfExp.begin();
	for (;itExp != inCache->fMapOfExp.end(); itExp++)
		Set( itExp->first, itExp->second.second);
}

/** return the cache pair <exp,value> at the specified cache index (1-based) */
bool VDocCache4DExp::GetCacheExpAndValueAt( VIndex inIndex, VString& outExp, VString& outValue)
{
	MapOfExp::const_iterator itExp = fMapOfExp.begin();
	if (inIndex >= 1 && inIndex <= fMapOfExp.size())
	{
		if (inIndex > 1)
			std::advance( itExp, inIndex-1);

		outExp = itExp->first;
		outValue = itExp->second.second;
		return true;
	}
	else
		return false;
}


/** remove the passed 4D tokenized exp from cache */
void VDocCache4DExp::Remove( const VString& inExp)
{
	MapOfExp::iterator itExp = fMapOfExp.find( inExp);
	if (itExp != fMapOfExp.end())
		fMapOfExp.erase(itExp);
}



/** eval deferred expressions & add to cache */
bool VDocCache4DExp::EvalDeferred()
{
	if (!fDBLanguageContext)
		return false;

	bool result = false;
	bool evalDeferred = fEvalDeferred;
	fEvalDeferred = false;
	MapOfDeferredExp::const_iterator itExp = fMapOfDeferredExp.begin();
	for (;itExp != fMapOfDeferredExp.end(); itExp++)
	{
		VString value;
		Get( itExp->first, value);
		if (!value.IsEmpty())
			result = true;
	}
	fMapOfDeferredExp.clear();
	fEvalDeferred = evalDeferred;
	return result;
}

/** evaluate now 4D expression */
void VDocSpanTextRef::EvalExpression(VDBLanguageContext *inLC, VDocCache4DExp *inCache4DExp)
{
	if (!inLC && !inCache4DExp)
		return;
	if (fType == kSpanTextRef_4DExp)
	{
		if (!fDelegate)
			fDelegate = &fDefaultDelegate;
		
		VString fOldComputedValue = fComputedValue;
		if (inCache4DExp)
		{
			xbox_assert(inCache4DExp->GetDBLanguageContext() == inLC || !inLC);
			inCache4DExp->Get( fRef, fComputedValue);
		}
		else
			fDelegate->IDSTR_Eval( fType, fRef, fComputedValue, fDefaultValue, inLC);
		fDefaultValue = fNBSP; 
		fValueDirty = false; 
		fDisplayValueDirty = fDisplayValueDirty || (!fShowRef && !fComputedValue.EqualToStringRaw( fOldComputedValue));
	}
}



/** append properties from the passed node
@remarks
	if inOnlyIfInheritOrNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties

	for paragraph, we need to append character styles too:
	but we append only character styles from fStyles VTextStyle, not from fStyles children if any
*/
void VDocParagraph::AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfInheritOrNotOverriden, bool inNoAppendSpanStyles)
{
	//apppend paragraph & common properties
	VDocNode::AppendPropsFrom( inNode, inOnlyIfInheritOrNotOverriden);

	//now append character styles
	if (inNoAppendSpanStyles)
		return;

	const VDocParagraph *inNodePara = dynamic_cast<const VDocParagraph *>(inNode);
	if (inNodePara->fStyles)
	{
		const VTextStyle *style = inNodePara->fStyles->GetData();
		if (!fStyles)
		{
			fStyles = new VTreeTextStyle( new VTextStyle( style));
			fStyles->GetData()->SetRange(0, fText.GetLength());
		}
		else
			fStyles->GetData()->AppendStyle( style, inOnlyIfInheritOrNotOverriden);
		fDirtyStamp++;
	}
}


eDocPropDirection VDocParagraph::GetDirection() const 
{ 
	uLONG direction = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_DIRECTION); 
	if (direction >= kTEXT_DIRECTION_LTR && direction <= kTEXT_DIRECTION_RTL)
		return (eDocPropDirection)direction;
	else
		return kTEXT_DIRECTION_LTR;
}
void VDocParagraph::SetDirection(const eDocPropDirection inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_DIRECTION, (uLONG)inValue); }

/** get line height 
@remarks
	if positive, it is fixed line height in TWIPS
	if negative, abs(value) is equal to percentage of current font size (so -250 means 250% of current font size)

	(default is -100 so 100%)
*/
sLONG VDocParagraph::GetLineHeight() const { return IDocProperty::GetProperty<sLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_LINE_HEIGHT); }

/** set line height 
@remarks
	if positive, it is fixed line height in TWIPS
	if negative, abs(value) is equal to percentage of current font size (so -250 sets line height to current font size * 250/100)
*/
void VDocParagraph::SetLineHeight(const sLONG inValue) { IDocProperty::SetProperty<sLONG>( static_cast<VDocNode *>(this), kDOC_PROP_LINE_HEIGHT, inValue); }

/** get first line padding offset in TWIPS 
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
sLONG VDocParagraph::GetTextIndent() const { return IDocProperty::GetProperty<sLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_TEXT_INDENT); }

/** set first line padding offset in TWIPS 
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
void VDocParagraph::SetTextIndent(const sLONG inValue) { IDocProperty::SetProperty<sLONG>( static_cast<VDocNode *>(this), kDOC_PROP_TEXT_INDENT, inValue); }

/** get tab stop offset in TWIPS */
uLONG VDocParagraph::GetTabStopOffset() const { return IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_TAB_STOP_OFFSET); }

/** set tab stop offset in TWIPS */
void VDocParagraph::SetTabStopOffset(const uLONG inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_TAB_STOP_OFFSET, inValue); }

eDocPropTabStopType VDocParagraph::GetTabStopType() const 
{ 
	uLONG type = IDocProperty::GetProperty<uLONG>( static_cast<const VDocNode *>(this), kDOC_PROP_TAB_STOP_TYPE); 
	if (testAssert(type >= kTEXT_TAB_STOP_TYPE_LEFT && type <= kTEXT_TAB_STOP_TYPE_BAR))
		return (eDocPropTabStopType)type;
	else
		return kTEXT_TAB_STOP_TYPE_LEFT;
}
void VDocParagraph::SetTabStopType(const eDocPropTabStopType inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_TAB_STOP_TYPE, (uLONG)inValue); }


/** replace plain text with computed or source span references 
@param inShowSpanRefs
	false (default): plain text contains span ref computed values if any (4D expression result, url link user text, etc...)
	true: plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
*/
void VDocParagraph::UpdateSpanRefs( bool inShowSpanRefs)
{
	if (fStyles)
	{
		fStyles->ShowSpanRefs( inShowSpanRefs);
		if (fStyles->UpdateSpanRefs(fText, *fStyles))
			fDirtyStamp++;
	}
}

/** replace text with span text reference on the specified range
@remarks
	span ref plain text is set here to uniform non-breaking space: 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VDocParagraph::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inNoUpdateRef)
{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isImage = false;
	VDocImage *inImage = NULL;
	if (inSpanRef->GetType() == kSpanTextRef_Image)
	{
		isImage = true;
		if (inSpanRef->GetImage()->GetDocumentNode() == GetDocumentNode())
			inImage = inSpanRef->GetImage(); //we need to rebind after replacement the image to the document as replacement might clone it
		else if (inSpanRef->GetImage()->GetDocumentNode())
		{
			//image belongs to another document: unbind image from the other document
			VDocText *doc = inSpanRef->GetImage()->GetDocumentNode();
			doc->RemoveImage( inSpanRef->GetImage());
		}	
	}
#endif
	if (VTreeTextStyle::ReplaceAndOwnSpanRef( fText, fStyles, inSpanRef, inStart, inEnd, inAutoAdjustRangeWithSpanRef, false, inNoUpdateRef))
	{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (isImage)
		{
			//we bind or rebind image to the document
			for (int i = 1; i <= fStyles->GetChildCount(); i++)
			{
				VTextStyle *style = fStyles->GetNthChild( i)->GetData();
				if (style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_Image && style->GetSpanRef()->GetImage()->GetDocumentNode() == NULL)
				{
					if (inImage)
					{
						//rebind image
						xbox_assert(style->GetSpanRef()->GetImage()->GetID() == inImage->GetID()); //ID should be the same
						style->GetSpanRef()->SetImage( inImage); //rebind it
					}
					else
						//bind new image to the document 
						GetDocumentNode()->AddImage( style->GetSpanRef()->GetImage());
				}
			}
		}
#endif
		fDirtyStamp++;
		return true;
	}
	return false;
}


/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	 

	return true if any 4D expression has been replaced with plain text
*/
bool VDocParagraph::FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd)
{
	bool updated = false;
	if (fStyles)
	{
		if ((updated = fStyles->FreezeExpressions( inLC, fText, *fStyles, inStart, inEnd)) == true)
			fDirtyStamp++;
		xbox_assert(fStyles);
	}
	return updated;
}

/** return the first span reference which intersects the passed range */
const VTextStyle *VDocParagraph::GetStyleRefAtRange(const sLONG inStart, const sLONG inEnd)
{
	if (fStyles)
		return fStyles->GetStyleRefAtRange( inStart, inEnd);
	else
		return NULL;
}
