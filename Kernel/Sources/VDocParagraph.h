
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
#ifndef __VDocParagraph__
#define __VDocParagraph__

BEGIN_TOOLBOX_NAMESPACE

#define kDOC_PROP_LINE_HEIGHT_NORMAL 0xffffffff

/** span text reference type */
typedef enum eDocSpanTextRef
{
	kSpanTextRef_4DExp,		//4D expression
	kSpanTextRef_URL,		//URL link
	kSpanTextRef_User,		//user property 
	kSpanTextRef_UnknownTag	//unknown or user tag
} eDocSpanTextRef;

/** span text reference delegate 
@remarks
	used it to customize span reference behavior:
		- evaluate to plain text a 4D expression, a url link or a custom property or tag
		- customize style
		- define if it is clickable & define behavior on click event

	remarks:  call VDocSpanTextRef::SetDelegate to install your delegate (preferably at application init)
			 
			 you should call IDocSpanTextRef base method for span ref type you do not customize behavior
			 (for instance if you override IDSTR_Eval only for one type (for instance 4D expression), call base method for other types)
*/
class XTOOLBOX_API IDocSpanTextRef
{
public:
	/** evaluate source ref & return computed value 
	@remarks
		on default, if not 4D expression, outValue = *inDefaultValue
					if 4D expression, outValue = "#NOT IMPL#"
	*/
	virtual void IDSTR_Eval(const eDocSpanTextRef inType, const VString& inRef, VString& outValue, const VString& inDefaultValue, VDBLanguageContext *inLC = NULL);

	/** return custom text style which overrides current span reference style 
	@remarks
		on default if inIsHighlighted == true, apply a light gray background
		if 4D exp -> none style overriden
		if url link -> apply char blue color (& underline if mouse over)
		if user link -> apply char blue color (& underline if mouse over)
		if unknown tag -> none style overriden
	*/
	virtual const VTextStyle *IDSTR_GetStyle(const eDocSpanTextRef inType, bool inShowRef, bool inIsHighlighted, bool inIsMouseOver);

	/** return true if reference is clickable 
	@remarks
		on default only urls & user links are clickable
	*/
	virtual bool  IDSTR_IsClickable(const eDocSpanTextRef inType, const VString& inRef) const { return inType == kSpanTextRef_URL || inType == kSpanTextRef_User; }

	/** method called if user has clicked on a clickable reference 

		should return true if does something
	@remarks
		on default does nothing & return false

		if url, inRef contains the url (encoded)
	*/
	virtual bool  IDSTR_OnClick(const eDocSpanTextRef inType, const VString& inRef) { return false; }

	/** method called if mouse enters a clickable reference	
	@remarks
		on default does nothing
	*/
	virtual bool  IDSTR_OnMouseEnter(const eDocSpanTextRef inType, const VString& inRef) { return false; }

	/** method called if mouse leaves a clickable reference	
	@remarks
		on default does nothing
	*/
	virtual bool  IDSTR_OnMouseLeave(const eDocSpanTextRef inType, const VString& inRef) { return false; }

private:
	static VTextStyle fStyleHighlight;
	static VTextStyle fStyleLink;
	static VTextStyle fStyleLinkAndHighlight;
	static VTextStyle fStyleLinkActive;
	static VTextStyle fStyleLinkActiveAndHighlight;
};


class XTOOLBOX_API VDocSpanTextRef: public VObject
{
public:
	VDocSpanTextRef(const eDocSpanTextRef inType, const VString& inRef, const VString& inDefaultValue = CVSTR("\240")):VObject() 
	{
		fType = inType;
		fRef = inRef;
		fDefaultValue = inDefaultValue;
		fValueDirty = fDisplayValueDirty = true;
		fShowRef = false;
		fShowHighlight = false;
		fIsMouseOver = false;
	}
	VDocSpanTextRef():VObject()
	{
		fType = kSpanTextRef_User;
		fValueDirty = fDisplayValueDirty = true;
		fShowRef = false;
		fShowHighlight = false;
		fIsMouseOver = false;
	}
	virtual	~VDocSpanTextRef() {}

	VDocSpanTextRef(const VDocSpanTextRef& inSpanTextRef):VObject()
	{
		fType = inSpanTextRef.fType;
		fRef = inSpanTextRef.fRef;
		fDefaultValue = inSpanTextRef.fDefaultValue;
		fComputedValue = inSpanTextRef.fComputedValue;
		fValueDirty = inSpanTextRef.fValueDirty;
		fDisplayValueDirty = inSpanTextRef.fDisplayValueDirty;
		fShowRef = inSpanTextRef.fShowRef;
		fShowHighlight = inSpanTextRef.fShowHighlight;

		//edit & runtime only flag so do not copy
		fIsMouseOver = false;
	}

	VDocSpanTextRef& operator = ( const VDocSpanTextRef& inSpanTextRef)
	{
		if (this == &inSpanTextRef)
			return *this;

		fType = inSpanTextRef.fType;
		fRef = inSpanTextRef.fRef;
		fDefaultValue = inSpanTextRef.fDefaultValue;
		fComputedValue = inSpanTextRef.fComputedValue;
		fValueDirty = inSpanTextRef.fValueDirty;
		fDisplayValueDirty = inSpanTextRef.fDisplayValueDirty;
		fShowRef = inSpanTextRef.fShowRef;
		fShowHighlight = inSpanTextRef.fShowHighlight;

		//edit & runtime only flags so do not copy
		fIsMouseOver = false;

		return *this;
	}

	bool operator == ( const VDocSpanTextRef& inSpanTextRef)
	{
		if (this == &inSpanTextRef)
			return true;

		return (fType == inSpanTextRef.fType
				&&
				fRef.EqualToStringRaw(inSpanTextRef.fRef)
				&&
				fDefaultValue.EqualToStringRaw(inSpanTextRef.fDefaultValue)
				&&
				fShowRef == inSpanTextRef.fShowRef
				&&
				fShowHighlight == inSpanTextRef.fShowHighlight
				);
	}

	/** set span ref delegate */
	static void SetDelegate( IDocSpanTextRef *inDelegate);
	static IDocSpanTextRef *GetDelegate();

	eDocSpanTextRef GetType() const { return fType; }
	void SetRef( const VString& inValue) { fRef = inValue; fDisplayValueDirty = true; }
	const VString& GetRef() const { return fRef; }
	void SetDefaultValue( const VString& inValue) {	fDefaultValue = inValue; fValueDirty = true; fDisplayValueDirty = true; }
	const VString& GetDefaultValue() const 	{ return fDefaultValue; }
	const VString& GetComputedValue()  
	{ 
		if (fValueDirty)
		{
			if (fType != kSpanTextRef_4DExp)
			{
				//4D expression is only evaluated on call to EvalExpression
				//so here we eval only other references
				if (!fDelegate)
					fDelegate = &fDefaultDelegate;
				fDelegate->IDSTR_Eval( fType, fRef, fComputedValue, fDefaultValue);
			}
			fValueDirty = false;
		}
		return fComputedValue; 
	}

	/** evaluate now 4D expression */
	void EvalExpression(VDBLanguageContext *inLC, VDocCache4DExp *inCache4DExp = NULL);

	/** return span reference style override
	@remarks
		this style is always applied after VTreeTextStyle styles
	*/
	const VTextStyle *GetStyle() const 
	{
		if (!fDelegate)
			fDelegate = &fDefaultDelegate;
		return fDelegate->IDSTR_GetStyle( fType, fShowRef, fShowHighlight, fIsMouseOver);
	}

	bool ShowRef() const { return fShowRef; }
	void ShowRef( bool inShowRef) 
	{
		if (fShowRef == inShowRef)
			return;
		if (fType != kSpanTextRef_4DExp) //we can only show 4D expression but not other references (other references are only accessible in XML text)
		{
			xbox_assert(fShowRef == false);
			return; 
		}
		fShowRef = inShowRef; 
		fDisplayValueDirty = true; //text has changed
	}

	bool ShowHighlight() const { return fShowHighlight; }
	void ShowHighlight( bool inShowHighlight)
	{
		fShowHighlight = inShowHighlight; 
	}

	bool IsMouseOver() const { return fIsMouseOver; }
	
	/** return true if mouse over status has changed, false otherwise */
	bool OnMouseEnter()
	{
		if (fIsMouseOver)
			return false;
		fIsMouseOver = true;
		fDelegate->IDSTR_OnMouseEnter( fType, fRef);
		return true;
	}

	/** return true if mouse over status has changed, false otherwise */
	bool OnMouseLeave()
	{
		if (!fIsMouseOver)
			return false;
		fIsMouseOver = false;
		fDelegate->IDSTR_OnMouseLeave( fType, fRef);
		return true;
	}

	const VString& GetDisplayValue(bool inResetDisplayValueDirty = true)  
	{
		//we ensure span reference is always at least one character size (if reference or value is empty, we replace with one no-breaking space string)
		if (fShowRef)
			return fRef.IsEmpty() ? fNBSP : fRef;
		else
		{
			GetComputedValue();
			return fComputedValue.IsEmpty() ? fNBSP : fComputedValue;	
		}
		if (inResetDisplayValueDirty)
			fDisplayValueDirty = false;
	}

	bool IsDisplayValueDirty() const { return fDisplayValueDirty; }

	bool IsClickable() const
	{
		if (!fDelegate)
			fDelegate = &fDefaultDelegate;
		return fDelegate->IDSTR_IsClickable( fType, fRef);
	}

	bool OnClick() const
	{
		if (!fDelegate)
			fDelegate = &fDefaultDelegate;
		return fDelegate->IDSTR_OnClick( fType, fRef);
	}

static VString fNBSP;

private: 
	bool fValueDirty;			//true if reference needs to be computed again
	bool fDisplayValueDirty;	//true if plain text needs to be updated
	bool fShowRef;				//true: show reference otherwise show eval value
	bool fShowHighlight;		//true: highlight value
	bool fIsMouseOver;			//true: mouse is over the reference

	eDocSpanTextRef fType;
	VString fRef;
	VString fDefaultValue;

	VString fComputedValue;

	static IDocSpanTextRef *fDelegate;
	static IDocSpanTextRef fDefaultDelegate;
};



#define kDOC_CACHE_4DEXP_SIZE 1000

class XTOOLBOX_API VDocCache4DExp: public VObject, public IRefCountable
{
public:
	VDocCache4DExp( VDBLanguageContext *inLC, const sLONG inMaxSize = kDOC_CACHE_4DEXP_SIZE):VObject(),IRefCountable() 	
	{ 
		xbox_assert(inLC);
		fDBLanguageContext = inLC;
		fMaxSize = inMaxSize; 
		fSpanRef = new VDocSpanTextRef(kSpanTextRef_4DExp, CVSTR(""));
		fEvalAlways = fEvalDeferred = false;
	}
	virtual ~VDocCache4DExp() { if (fSpanRef) delete fSpanRef; }

	/** return 4D expression value, as evaluated from the passed 4D tokenized expression
	@remarks
		returns cached value if 4D exp has been evaluated yet, otherwise eval 4D exp & append pair<exp,value> to the cache

		if fEvalAways == true, it always eval 4D exp & update cache
		if fEvalDeferred == true, if 4D exp is not in cache yet, it adds exp to the internal deferred list but does not eval now 4D exp (returns empty string as value):
								  caller might then call EvalDeferred() to eval deferred expressions and clear deferred list
	*/
	void Get( const VString& inExp, VString& outValue, VDBLanguageContext *inLC = NULL);

	/** manually add a pair <exp,value> to the cache
	@remarks
		add or replace the existing value 

		caution: it does not check if value if valid according to the expression so it might be anything
	*/
	void Set( const VString& inExp, const VString& inValue);

	/** return cache actual size */
	sLONG GetCacheCount() const { return fMapOfExp.size(); }

	/** return the cache pair <exp,value> at the specified cache index (1-based) */
	bool GetCacheExpAndValueAt( VIndex inIndex, VString& outExp, VString& outValue);

	/** append cached expressions from the passed cache */
	void AppendFromCache( const VDocCache4DExp *inCache);

	/** remove the passed 4D tokenized exp from cache */
	void Remove( const VString& inExp);

	/** clear cache */
	void Clear() { fMapOfExp.clear(); }

	void SetDBLanguageContext( VDBLanguageContext *inLC)
	{
		fDBLanguageContext = inLC;
		fMapOfExp.clear();
	}
	VDBLanguageContext *GetDBLanguageContext() const { return fDBLanguageContext; }

	/** always eval 4D expression
	@remarks
		if true, Get() will always eval & update cached 4D expression, even if it is in the cache yet
		if false, Get() will only eval & cache 4D expression if it is not in the cache yet, otherwise it returns value from cache
	*/
	void SetEvalAlways(bool inEvalAlways = true) { fEvalAlways = inEvalAlways; }
	bool GetEvalAlways() const { return fEvalAlways; }

	/** defer 4D expression eval 
	@remarks
		see fEvalDeferred
	*/
	void SetEvalDeferred(bool inEvalDeferred = true) { fEvalDeferred = inEvalDeferred; if (!fEvalDeferred) fMapOfDeferredExp.clear(); }
	bool GetEvalDeferred() const { return fEvalDeferred; }

	/** clear deferred expression */
	void ClearDeferred() { fMapOfDeferredExp.clear(); }

	/** eval deferred expressions & add to cache 
	@remarks
		return true if any expression has been evaluated

		on exit, clear also map of deferred expressions
	*/
	bool EvalDeferred();

private:
	/** time stamp + exp value */
	typedef std::pair<uLONG, VString> ExpValue;

	/** map of exp value per exp */
	typedef unordered_map_VString< ExpValue > MapOfExp;
	
	/** map of deferred expressions (here boolean value is dummy & unused) */
	typedef unordered_map_VString< bool > MapOfDeferredExp;

	/** DB language context */
	VDBLanguageContext *fDBLanguageContext;

	/** map of 4D expressions per 4D tokenized expression  */
	MapOfExp fMapOfExp;
	
	/** map of deferred 4D expressions 
	@see
		fEvalDeferred
	*/
	MapOfDeferredExp fMapOfDeferredExp;

	/** span ref (used locally to eval any 4D exp) */
	VDocSpanTextRef *fSpanRef;

	/** if true, Get() will always eval 4D exp & update cached value 
		otherwise it returns cached value if any 
	*/
	bool fEvalAlways;

	/** if true, Get() will return exp value from cache if exp has been evaluated yet
		otherwise it adds expression to evaluate to a internal deferred list:
		expressions in deferred list can later be evaluated by calling EvalDeferred	()
	@remarks
		if true, fEvalAlways is ignored
	*/
	bool fEvalDeferred;

	sLONG fMaxSize;
};

/** document paragraph class 
@remarks
	this class is used both for storing a paragraph node or a paragraph style (which includes paragraph props, common props & character styles)
	(if it it used only for styling, class should have none parent & text is ignored - see VTextLayout::ApplyParagraphStyle)
*/
class XTOOLBOX_API VDocParagraph : public VDocNode
{
public:
	friend class VDocNode;

	VDocParagraph():VDocNode() 
	{
		fType = kDOC_NODE_TYPE_PARAGRAPH;
		fStyles = NULL;
	}
	virtual ~VDocParagraph() 
	{
		ReleaseRefCountable(&fStyles);
	}

	virtual VDocNode *Clone() const {	return static_cast<VDocNode *>(new VDocParagraph(this)); }

	/** replace current text & styles 
	@remarks
		if inCopyStyles == true, styles are copied
		if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
	*/
	void SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false)
	{
		fText = inText;
		SetStyles( inStyles, inCopyStyles);
	}

	const VString& GetText() const
	{
		return fText;
	}

	VString *GetTextPtr()
	{
		return &fText;
	}

	/** set text styles 
	@remarks
		if inCopyStyles == true, styles are copied
		if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
	*/
	void SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false)
	{
		if (fStyles == inStyles)
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
			if (end > fText.GetLength())
				fStyles->Truncate(fText.GetLength());

			if (fStyles->GetData()->GetJustification() != JST_Notset)
				fStyles->GetData()->SetJustification( JST_Notset); //it is a paragraph style only
		}
	}

	const VTreeTextStyle *GetStyles() const
	{
		return fStyles;
	}
	VTreeTextStyle *RetainStyles() const
	{
		return RetainRefCountable(fStyles);
	}

	/** append properties from the passed node
	@remarks
		if inOnlyIfInheritOrNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties

		for paragraph, we need to append character styles too:
		but we append only character styles from fStyles VTextStyle, not from fStyles children if any
	*/
	void AppendPropsFrom( const VDocNode *inNode, bool inOnlyIfInheritOrNotOverriden = false, bool inNoAppendSpanStyles = false); 

	//paragraph properties

	eDocPropDirection GetDirection() const; 
	void SetDirection(const eDocPropDirection inValue);

	/** get line height 
	@remarks
		if positive, it is fixed line height in TWIPS
		if negative, abs(value) is equal to percentage of current font size (so -250 means 250% of current font size)
		if kDOC_PROP_LINE_HEIGHT_NORMAL, it is normal line height (that is font ascent+descent+external leading)

		(default is kDOC_PROP_LINE_HEIGHT_NORMAL that is normal)
	*/
	sLONG GetLineHeight() const;
	
	/** set line height 
	@remarks
		if positive, it is fixed line height in TWIPS
		if negative, abs(value) is equal to percentage of current font size (so -250 means 250% of current font size)
		if kDOC_PROP_LINE_HEIGHT_NORMAL, it is normal line height (that is font ascent+descent+external leading)

		(default is kDOC_PROP_LINE_HEIGHT_NORMAL that is normal)
	*/
	void SetLineHeight(const sLONG inValue);

	/** get first line padding offset in TWIPS 
	@remarks
		might be negative for negative padding (that is second line up to the last line is padded but the first line)
	*/
	sLONG GetPaddingFirstLine() const;

	/** set first line padding offset in TWIPS 
	@remarks
		might be negative for negative padding (that is second line up to the last line is padded but the first line)
	*/
	void SetPaddingFirstLine(const sLONG inValue);

	/** get tab stop offset in TWIPS */
	uLONG GetTabStopOffset() const;

	/** set tab stop offset in TWIPS */
	void SetTabStopOffset(const uLONG inValue);

	eDocPropTabStopType GetTabStopType() const; 
	void SetTabStopType(const eDocPropTabStopType inValue);

	/** replace plain text with computed or source span references 
	@param inShowSpanRefs
		false (default): plain text contains span ref computed values if any (4D expression result, url link user text, etc...)
		true: plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
	*/
	void UpdateSpanRefs( bool inShowSpanRefs = false);

	/** replace text with span text reference on the specified range
	@remarks
		span ref plain text is set here to uniform non-breaking space: 
		user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

		you should no more use or destroy passed inSpanRef even if method returns false
	*/
	bool ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inNoUpdateRef = true);

	/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	 

		return true if any 4D expression has been replaced with plain text
	*/
	bool FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1);

	/** return the first span reference which intersects the passed range */
	const VTextStyle *GetStyleRefAtRange(const sLONG inStart = 0, const sLONG inEnd = -1);

protected:
	VDocParagraph( const VDocParagraph* inNodePara):VDocNode(static_cast<const VDocNode *>(inNodePara))	
	{
		fText = inNodePara->fText;
		fStyles = inNodePara->fStyles ? new VTreeTextStyle( inNodePara->fStyles) : NULL;
	}

	VString fText;
	VTreeTextStyle *fStyles; 
};


END_TOOLBOX_NAMESPACE

#endif
