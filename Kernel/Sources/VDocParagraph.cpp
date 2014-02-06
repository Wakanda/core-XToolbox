
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

VString	VDocParagraph::sElementName;

VString VDocSpanTextRef::fSpace = CVSTR(" ");	//white-space (default value for text span references - not images or tables)
VString VDocSpanTextRef::fCharBreakable;		//dummy character for images or tables span references
												//(never displayed but used for images or tables ICU line-breaking 
												// so a chinese character because we need it to be always breakable even if contiguous with the same character)

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
const VTextStyle *IDocSpanTextRef::IDSTR_GetStyle(const eDocSpanTextRef inType, SpanRefTextTypeMask inShowRef, bool inIsHighlighted, bool inIsMouseOver)
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

	if ((inShowRef & (kSRTT_Src|kSRTT_Value)) == 0)
	{
		if (inIsHighlighted)
			return &fStyleHighlight;
	}
	else
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
		case kSpanTextRef_Image:
			if ((inShowRef & kSRTT_Src) || inIsHighlighted)
				//always highlight while displaying 4D tokenized expression or embedded tag xml text or image source
				return &fStyleHighlight;
			break;
		default:
			if (inIsHighlighted)
				return &fStyleHighlight;
		}
	return NULL;
}

const VString& VDocSpanTextRef::GetDisplayValue(bool inResetDisplayValueDirty)  
{
	//we ensure span reference is always at least one character size (if reference or value is empty, we replace with one no-breaking space string)

	if (inResetDisplayValueDirty)
		fDisplayValueDirty = false;
	if (fType == kSpanTextRef_Image)
	{
		if (fCharBreakable.IsEmpty())
			fCharBreakable = (UniChar)0x4E10;	//(never displayed but used for images or tables ICU line-breaking 
												//so a chinese character because we need it to be always breakable even if contiguous with the same character)

		if ((fShowRef & kSRTT_Src) != 0)
			return fImage.Get() ? fImage->GetSource() : (fRef.IsEmpty() ? fSpace : fRef);
		else if ((fShowRef & kSRTT_Value) != 0)
			return fCharBreakable; //return breakable character
		else 
			return fSpace; //uniform -> return whitespace
	}
	else
		if ((fShowRef & kSRTT_Src) != 0)
		{
			if (fType == kSpanTextRef_4DExp && ((fShowRef & kSRTT_4DExp_Normalized) != 0) && !fRefNormalized.IsEmpty())
				return fRefNormalized;
			else
				return fRef.IsEmpty() ? fSpace : fRef;
		}
		else if ((fShowRef & kSRTT_Value) != 0)
		{
			GetComputedValue();
			return fComputedValue.IsEmpty() ? fSpace : fComputedValue;	
		}
		else
			return fSpace;
}


/** for 4D expression, return expression for current language and version without special tokens;
	for other reference, return reference unmodified */
void VDocSpanTextRef::UpdateRefNormalized(VDBLanguageContext *inLC)
{
	if (fType != kSpanTextRef_4DExp || !inLC)
		return;
	
	if (!fDelegate)
		fDelegate = &fDefaultDelegate;
	fRefNormalized = fRef;
	fDelegate->IDSTR_NormalizeExpToCurrentLanguageAndVersion( fRefNormalized, inLC);
	if (((fShowRef & kSRTT_Src) != 0) && ((fShowRef & kSRTT_4DExp_Normalized) != 0))
		fDisplayValueDirty = true;
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

SpanRefTextTypeMask	VDocSpanTextRef::CombinedTextTypeMaskToTextTypeMask( SpanRefCombinedTextTypeMask inCTT)
{
	SpanRefTextTypeMask value = kSRTT_Uniform;
	if (inCTT)
		switch (fType)
		{
			case kSpanTextRef_4DExp:
				{
				if (inCTT & kSRCTT_4DExp_Value)
					value = kSRTT_Value;
				else if (inCTT & kSRCTT_4DExp_Src)
					value = kSRTT_Src;
				if (inCTT & kSRCTT_4DExp_Normalized)
					value = value | kSRTT_4DExp_Normalized;
				}
				break;
			case kSpanTextRef_URL:
				if (inCTT & kSRCTT_URL_Value)
					value = kSRTT_Value;
				else if (inCTT & kSRCTT_URL_Src)
					value = kSRTT_Src;
				break;
			case kSpanTextRef_User:
				if (inCTT & kSRCTT_User_Value)
					value = kSRTT_Value;
				else if (inCTT & kSRCTT_User_Src)
					value = kSRTT_Src;
				break;
			case kSpanTextRef_UnknownTag:
				if (inCTT & kSRCTT_UnknownTag_Value)
					value = kSRTT_Value;
				else if (inCTT & kSRCTT_UnknownTag_Src)
					value = kSRTT_Src;
				break;
			case kSpanTextRef_Image:
				if (inCTT & kSRCTT_Image_Value)
					value = kSRTT_Value;
				else if (inCTT & kSRCTT_Image_Src)
					value = kSRTT_Src;
				break;
			default: //for other embedded objects text is always equal to a space
				break;
		}
	return value;
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
		fDefaultValue = fSpace; 
		fValueDirty = false; 
		fDisplayValueDirty = fDisplayValueDirty || ((fShowRef & kSRTT_Value) && !fComputedValue.EqualToStringRaw( fOldComputedValue));
	}
}

/** replace current text & styles 
@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
bool VDocParagraph::SetText( const VString& inText, VTreeTextStyle *inStyles, bool inCopyStyles)
{
	fText = inText;
	SetStyles( inStyles, inCopyStyles);

	_UpdateTextInfoEx(true);

	fDirtyStamp++;
	return true;
}

void VDocParagraph::GetText(VString& outPlainText, sLONG inStart, sLONG inEnd) const
{
	if (inStart <= 0 && inEnd == -1)
		outPlainText = fText;
	else
	{
		_NormalizeRange( inStart, inEnd);

		if (inEnd > inStart)
			fText.GetSubString(inStart+1, inEnd-inStart, outPlainText);
		else
			outPlainText.SetEmpty();
	}
}

/** set text styles 
@remarks
	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
void VDocParagraph::SetStyles( VTreeTextStyle *inStyles, bool inCopyStyles)
{
	if (fStyles == inStyles && !inCopyStyles)
		return;

	ReleaseRefCountable(&fStyles);
	if (inStyles)
	{
		if (inCopyStyles)
			fStyles = new VTreeTextStyle( inStyles);
		else
			fStyles = RetainRefCountable( inStyles);
		sLONG start, end;
		fStyles->GetData()->GetRange( start, end);
		xbox_assert( start >= 0);
		if (start < 0)
		{
			//invalid
			ReleaseRefCountable(&fStyles);
			return;
		}
		if (end > fText.GetLength())
			fStyles->Truncate(fText.GetLength());

		if (fStyles->GetData()->GetJustification() != JST_Notset)
			fStyles->GetData()->SetJustification( JST_Notset); //it is a paragraph style only
	}

	_AfterReplace(this, 0, fText.GetLength());

	fDirtyStamp++;
}

VTreeTextStyle* VDocParagraph::RetainStyles(sLONG inStart, sLONG inEnd, bool inRelativeToStart, bool inNULLIfRangeEmpty, bool inNoMerge) const
{
	if (!fStyles)
		return NULL;

	if (inStart <= 0 && inEnd == -1)
	{
		if (inNULLIfRangeEmpty)
		{
			sLONG start, end;
			fStyles->GetData()->GetRange( start, end);
			if (start >= end)
				return NULL;
		}
		return (new VTreeTextStyle(fStyles));
	}
	else
	{
		_NormalizeRange( inStart, inEnd);

		if (inEnd >= inStart)
			return fStyles->CreateTreeTextStyleOnRange( inStart, inEnd, inRelativeToStart, inNULLIfRangeEmpty, inNoMerge);
		else
			return NULL;
	}
}


void VDocParagraph::_UpdateTextInfo()
{
	VDocNode::_UpdateTextInfo(); //for fTextStart

	fTextLength = fText.GetLength();
}

/** append properties from the passed node
@remarks
	if inOnlyIfInheritOrNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties

	for paragraph, we need to append character styles too:
	but we append only character styles from fStyles VTextStyle, not from fStyles children if any
*/
void VDocParagraph::AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfInheritOrNotOverriden, bool inNoAppendSpanStyles, sLONG /*inStart*/, sLONG /*inEnd*/)
{
	if (inNode->GetType() != kDOC_NODE_TYPE_PARAGRAPH)
		return;

	//apppend paragraph & common properties
	VDocNode::AppendPropsFrom( inNode, inOnlyIfInheritOrNotOverriden);

	//now append character styles
	if (inNoAppendSpanStyles)
		return;

	const VDocParagraph *inNodePara = dynamic_cast<const VDocParagraph *>(inNode);
	xbox_assert(inNodePara);
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
	if (direction >= kDOC_DIRECTION_LTR && direction <= kDOC_DIRECTION_RTL)
		return (eDocPropDirection)direction;
	else
		return kDOC_DIRECTION_LTR;
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
	if (testAssert(type >= kDOC_TAB_STOP_TYPE_LEFT && type <= kDOC_TAB_STOP_TYPE_BAR))
		return (eDocPropTabStopType)type;
	else
		return kDOC_TAB_STOP_TYPE_LEFT;
}
void VDocParagraph::SetTabStopType(const eDocPropTabStopType inValue) { IDocProperty::SetProperty<uLONG>( static_cast<VDocNode *>(this), kDOC_PROP_TAB_STOP_TYPE, (uLONG)inValue); }


const VString& VDocParagraph::GetNewParaClass() const
{
	return IDocProperty::GetPropertyRef<VString>( static_cast<const VDocNode *>(this), kDOC_PROP_NEW_PARA_CLASS);
}

void VDocParagraph::SetNewParaClass( const VString& inClass)
{
	IDocProperty::SetPropertyPerRef<VString>( static_cast<VDocNode *>(this), kDOC_PROP_NEW_PARA_CLASS, inClass);
}


/** replace plain text with computed or source span references */
void VDocParagraph::UpdateSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs, VDBLanguageContext *inLC)
{
	if (fStyles)
	{
		fStyles->ShowSpanRefs( inShowSpanRefs);
		if (fStyles->UpdateSpanRefs(fText, *fStyles, 0, -1, NULL, NULL, NULL, inLC))
		{
			_UpdateTextInfoEx(true);
			fDirtyStamp++;
		}
	}
}

/** replace styled text at the specified range (relative to node start offset) with new text & styles
@remarks
	replaced text will inherit only uniform style on replaced range - if inInheritUniformStyle == true (default)

	return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
*/
VIndex VDocParagraph::ReplaceStyledText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd, 
										 bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, const VDocSpanTextRef * inPreserveSpanTextRef)
{
	_NormalizeRange( inStart, inEnd);

	VIndex result = VTreeTextStyle::ReplaceStyledText( fText, fStyles, inText, inStyles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef, this, _BeforeReplace, _AfterReplace);
	if (result)
	{
		_UpdateTextInfoEx(true);
		fDirtyStamp++;
	}
	return result;
}

void VDocParagraph::_AfterReplace(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced)
{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	VDocParagraph* para = (VDocParagraph *)inUserData;
	if (para->fStyles)
	{
		while (inStartReplaced < inEndReplaced)
		{
			const VTextStyle *style = para->fStyles->GetStyleRefAtRange( inStartReplaced, inEndReplaced);
			if (!style)
				break;

			if (style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_Image && style->GetSpanRef()->GetImage()->GetDocumentNode() != para->GetDocumentNode())
			{
				//bind new image to the document (as image is not child of a document element but is embedded in a span reference, we need to bind it to the document explicitly)
				
				xbox_assert( style->GetSpanRef()->GetImage()->GetDocumentNode() == NULL); //it should not belong to another document...

				//bind to this document 
				para->GetDocumentNode()->AddImage( style->GetSpanRef()->GetImage());
			}
			sLONG start, end;
			style->GetRange( start, end);
			if (!testAssert(start < end))
				break;
			inStartReplaced = end;
		}
	}
#endif
}

/** replace text with span text reference on the specified range
@remarks
	span ref plain text is set here to uniform non-breaking space: 
	user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VDocParagraph::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inNoUpdateRef, VDBLanguageContext *inLC)
{
	_NormalizeRange( inStart, inEnd);

	if (VTreeTextStyle::ReplaceAndOwnSpanRef( fText, fStyles, inSpanRef, inStart, inEnd, inAutoAdjustRangeWithSpanRef, false, inNoUpdateRef, this, _BeforeReplace, _AfterReplace, inLC))
	{
		_UpdateTextInfoEx(true);

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
		_NormalizeRange( inStart, inEnd);

		if ((updated = fStyles->FreezeExpressions( inLC, fText, *fStyles, inStart, inEnd)) == true)
		{
			_UpdateTextInfoEx(true);
			fDirtyStamp++;
		}
		xbox_assert(fStyles);
	}
	return updated;
}

/** return the first span reference which intersects the passed range */
const VTextStyle *VDocParagraph::GetStyleRefAtRange(sLONG inStart, sLONG inEnd, sLONG* /*outStartBase*/)
{
	if (fStyles)
	{
		_NormalizeRange( inStart, inEnd);
		return fStyles->GetStyleRefAtRange( inStart, inEnd);
	}
	else
		return NULL;
}
