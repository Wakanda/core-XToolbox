
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

#define kDOC_PROP_LINE_HEIGHT_NORMAL -100 //100%

/** span text reference type */
typedef enum eDocSpanTextRef
{
	kSpanTextRef_4DExp,		//4D expression
	kSpanTextRef_URL,		//URL link
	kSpanTextRef_User,		//user property 
	kSpanTextRef_Image,		//image
	kSpanTextRef_UnknownTag	//unknown or user tag
} eDocSpanTextRef;

/** span reference display text type */
typedef enum eSpanRefTextType
{
	kSRTT_Uniform			= 0,		//space as text
	kSRTT_Value				= 1,		//value as text
	kSRTT_Src				= 2,		//source as text
	kSRTT_4DExp_Normalized	= 4			//tokenized expression is normalized - converted for current language & version without special tokens (valid only for 4D exp and if combined with kSRTT_Src)

} eSpanRefTextType;

/** span reference display text type mask */
typedef uLONG	SpanRefTextTypeMask;

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

/** normalize 4D expression to current language and version (remove also special tokens) */
virtual	void					IDSTR_NormalizeExpToCurrentLanguageAndVersion(VString& ioExp, VDBLanguageContext *inLC = NULL) {}

/** evaluate source ref & return computed value 
@remarks
	on default, if not 4D expression, outValue = *inDefaultValue
				if 4D expression, outValue = "#NOT IMPL#"
*/
virtual	void					IDSTR_Eval(const eDocSpanTextRef inType, const VString& inRef, VString& outValue, const VString& inDefaultValue, VDBLanguageContext *inLC = NULL);

/** return custom text style which overrides current span reference style 
@remarks
	on default if inIsHighlighted == true, apply a light gray background
	if 4D exp -> none style overriden
	if url link -> apply char blue color (& underline if mouse over)
	if user link -> apply char blue color (& underline if mouse over)
	if unknown tag -> none style overriden
*/
virtual const VTextStyle*		IDSTR_GetStyle(const eDocSpanTextRef inType, SpanRefTextTypeMask inShowRef, bool inIsHighlighted, bool inIsMouseOver);

/** return true if reference is clickable 
@remarks
	on default only urls & user links are clickable
*/
virtual bool					IDSTR_IsClickable(const eDocSpanTextRef inType, const VString& inRef) const { return inType == kSpanTextRef_URL || inType == kSpanTextRef_User; }

/** method called if user has clicked on a clickable reference 

	should return true if does something
@remarks
	on default does nothing & return false

	if url, inRef contains the url (encoded)
*/
virtual bool					IDSTR_OnClick(const eDocSpanTextRef inType, const VString& inRef) { return false; }

/** method called if mouse enters a clickable reference	
@remarks
	on default does nothing
*/
virtual bool					IDSTR_OnMouseEnter(const eDocSpanTextRef inType, const VString& inRef) { return false; }

/** method called if mouse leaves a clickable reference	
@remarks
	on default does nothing
*/
virtual bool					IDSTR_OnMouseLeave(const eDocSpanTextRef inType, const VString& inRef) { return false; }

private:
static	VTextStyle 				fStyleHighlight;
static	VTextStyle 				fStyleLink;
static	VTextStyle 				fStyleLinkAndHighlight;
static	VTextStyle 				fStyleLinkActive;
static	VTextStyle 				fStyleLinkActiveAndHighlight;
};


/** span text reference custom layout delegate 
@remarks
	it might be used for instance to customize layout & painting of embedded objects like images (see INodeLayout in GUI - derived from this class)
*/
class XTOOLBOX_API IDocSpanTextRefLayout
{
public:
virtual	void					ReleaseRef() = 0;
protected:
virtual							~IDocSpanTextRefLayout() {}
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
			fShowRef = kSRTT_Value;
			fShowHighlight = false;
			fIsMouseOver = false;
			fLayoutDelegate = NULL;
		}
								VDocSpanTextRef(VDocImage* inImage):VObject() 
		{
			xbox_assert(inImage);
			fLayoutDelegate = NULL;
			SetImage( inImage);
			fShowRef = kSRTT_Value;
			fShowHighlight = false;
			fIsMouseOver = false;
		}
								VDocSpanTextRef():VObject()
		{
			fType = kSpanTextRef_User;
			fValueDirty = fDisplayValueDirty = true;
			fShowRef = kSRTT_Value;
			fShowHighlight = false;
			fIsMouseOver = false;
			fLayoutDelegate = NULL;
		}
virtual							~VDocSpanTextRef() 
		{
			if (fLayoutDelegate)
				fLayoutDelegate->ReleaseRef();
		}

								VDocSpanTextRef(const VDocSpanTextRef& inSpanTextRef):VObject()
		{
			fType = inSpanTextRef.fType;
			fRef = inSpanTextRef.fRef;
			fRefNormalized = inSpanTextRef.fRefNormalized;
			fDefaultValue = inSpanTextRef.fDefaultValue;
			fComputedValue = inSpanTextRef.fComputedValue;
			fValueDirty = inSpanTextRef.fValueDirty;
			fDisplayValueDirty = inSpanTextRef.fDisplayValueDirty;
			fShowRef = inSpanTextRef.fShowRef;
			fShowHighlight = inSpanTextRef.fShowHighlight;
			fImage = inSpanTextRef.fImage;
			if (inSpanTextRef.fImage.Get())
				//while copying, we need to clone image as only the original VDocSpanTextRef might retain a reference on the original VDocImage stored in VDocText
				fImage = VRefPtr<VDocImage>(dynamic_cast<VDocImage *>(inSpanTextRef.fImage.Get()->Clone()), false);
			else
				fImage = inSpanTextRef.fImage;

			//edit & runtime only flag so do not copy
			fIsMouseOver = false;
			fLayoutDelegate = NULL;
		}

		VDocSpanTextRef&		operator = ( const VDocSpanTextRef& inSpanTextRef)
		{
			if (this == &inSpanTextRef)
				return *this;

			fType = inSpanTextRef.fType;
			fRef = inSpanTextRef.fRef;
			fRefNormalized = inSpanTextRef.fRefNormalized;
			fDefaultValue = inSpanTextRef.fDefaultValue;
			fComputedValue = inSpanTextRef.fComputedValue;
			fValueDirty = inSpanTextRef.fValueDirty;
			fDisplayValueDirty = inSpanTextRef.fDisplayValueDirty;
			fShowRef = inSpanTextRef.fShowRef;
			fShowHighlight = inSpanTextRef.fShowHighlight;
			if (inSpanTextRef.fImage.Get())
				//while copying, we need to clone image as only the original VDocSpanTextRef might retain a reference on the original VDocImage stored in VDocText
				fImage = VRefPtr<VDocImage>(dynamic_cast<VDocImage *>(inSpanTextRef.fImage.Get()->Clone()), false);
			else
				fImage = inSpanTextRef.fImage;

			//edit & runtime only flags so do not copy
			fIsMouseOver = false;
			if (fLayoutDelegate)
				fLayoutDelegate->ReleaseRef();
			fLayoutDelegate = NULL;
			return *this;
		}

		bool					operator == ( const VDocSpanTextRef& inSpanTextRef)
		{
			if (this == &inSpanTextRef)
				return true;

			return (fType == inSpanTextRef.fType
					&&
					fRef.EqualToStringRaw(inSpanTextRef.fRef)
					&&
					fRefNormalized.EqualToStringRaw(inSpanTextRef.fRefNormalized)
					&&
					fDefaultValue.EqualToStringRaw(inSpanTextRef.fDefaultValue)
					&&
					fShowRef == inSpanTextRef.fShowRef
					&&
					fShowHighlight == inSpanTextRef.fShowHighlight
					&&
					fImage.Get() == inSpanTextRef.fImage.Get()
					);
		}

		/** set span ref delegate */
static	void					SetDelegate( IDocSpanTextRef *inDelegate);
static	IDocSpanTextRef*		GetDelegate();
		SpanRefTextTypeMask		CombinedTextTypeMaskToTextTypeMask( SpanRefCombinedTextTypeMask inCTT);

		eDocSpanTextRef			GetType() const { return fType; }

		void					SetRef( const VString& inValue) { fRef = inValue; fRefNormalized.SetEmpty(); fDisplayValueDirty = true; }
		const VString&			GetRef() const { return fRef; }

		const VString&			GetRefNormalized() const { return fRefNormalized; }

								/** for 4D expression, compute and store expression for current language and version without special tokens;
									for other reference, do nothing */
		void					UpdateRefNormalized(VDBLanguageContext *inLC);

		void					SetDefaultValue( const VString& inValue) {	fDefaultValue = inValue; fValueDirty = true; fDisplayValueDirty = true; }
		const VString&			GetDefaultValue() const 	{ return fDefaultValue; }

		const VString&			GetComputedValue()  
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

		/** set image ref */
		void					SetImage( VDocImage *inImage) 
		{ 
			xbox_assert(inImage);
			fType = kSpanTextRef_Image; 
			fImage = VRefPtr<VDocImage>(inImage); 
			fRef = inImage->GetSource();
			fRefNormalized.SetEmpty();
			fDefaultValue = fComputedValue = inImage->GetAltText();
			fValueDirty = false;
			fDisplayValueDirty = true;
			if (fLayoutDelegate)
				fLayoutDelegate->ReleaseRef();
			fLayoutDelegate = NULL;
		}

		/** get image ref */
		VDocImage*				GetImage() const { return fImage.Get(); }

		/** set layout delegate 
		@remarks
			it might be used to paint span embedded objects like images
		*/
		void					SetLayoutDelegate( IDocSpanTextRefLayout* inDelegate)
		{
			fLayoutDelegate = inDelegate;
		}
		IDocSpanTextRefLayout*	GetLayoutDelegate() const { return fLayoutDelegate; }

		/** evaluate now 4D expression */
		void					EvalExpression(VDBLanguageContext *inLC, VDocCache4DExp *inCache4DExp = NULL);

		/** return span reference style override
		@remarks
			this style is always applied after VTreeTextStyle styles
		*/
		const VTextStyle*		GetStyle() const 
		{
			if (!fDelegate)
				fDelegate = &fDefaultDelegate;
			return fDelegate->IDSTR_GetStyle( fType, fShowRef, fShowHighlight, fIsMouseOver);
		}

		SpanRefTextTypeMask		ShowRef() const { return fShowRef; }
		void					ShowRef( SpanRefTextTypeMask inShowRef) 
		{
			if (fShowRef == inShowRef)
				return;
			fShowRef = inShowRef; 
			fDisplayValueDirty = true; //text has changed
		}

								/** true: display normalized reference if ShowRef() == kSRTT_Src (for 4D exp, it is expression for current language and version without special tokens)
									false: display reference as set initially
								@remarks
									you should call at least once UpdateRefNormalized before calling then GetDisplayValue
								*/ 
		bool					ShowRefNormalized() const { return (fShowRef & kSRTT_4DExp_Normalized) != 0; }
		void					ShowRefNormalized( bool inShowRefNormalized) 
		{
			if (fType != kSpanTextRef_4DExp)
				return;
			if ((fShowRef & kSRTT_4DExp_Normalized) == (inShowRefNormalized ? kSRTT_4DExp_Normalized : 0))
				return;
			if (inShowRefNormalized)
				fShowRef = fShowRef | kSRTT_4DExp_Normalized; 
			else
				fShowRef = fShowRef & (~kSRTT_4DExp_Normalized);
			if ((fShowRef & kSRTT_Src) != 0)
				fDisplayValueDirty = true; //text has changed
		}

		bool					ShowHighlight() const { return fShowHighlight; }
		void					ShowHighlight( bool inShowHighlight)
		{
			fShowHighlight = inShowHighlight; 
		}

		bool					IsMouseOver() const { return fIsMouseOver; }
		
		/** return true if mouse over status has changed, false otherwise */
		bool					OnMouseEnter()
		{
			if (fIsMouseOver)
				return false;
			fIsMouseOver = true;
			fDelegate->IDSTR_OnMouseEnter( fType, fRef);
			return true;
		}

		/** return true if mouse over status has changed, false otherwise */
		bool					OnMouseLeave()
		{
			if (!fIsMouseOver)
				return false;
			fIsMouseOver = false;
			fDelegate->IDSTR_OnMouseLeave( fType, fRef);
			return true;
		}

		const VString&			GetDisplayValue(bool inResetDisplayValueDirty = true);  

		bool					IsDisplayValueDirty() const { return fDisplayValueDirty; }

		bool					IsClickable() const
		{
			if (!fDelegate)
				fDelegate = &fDefaultDelegate;
			return fDelegate->IDSTR_IsClickable( fType, fRef);
		}

		bool					OnClick() const
		{
			if (!fDelegate)
				fDelegate = &fDefaultDelegate;
			return fDelegate->IDSTR_OnClick( fType, fRef);
		}

static VString					fSpace;
static VString					fCharBreakable;

private: 
		bool 					fValueDirty;			//true if reference needs to be computed again
		bool 					fDisplayValueDirty;		//true if plain text needs to be updated
		SpanRefTextTypeMask 	fShowRef;
		bool 					fShowHighlight;			//true: highlight value
		bool 					fIsMouseOver;			//true: mouse is over the reference

		eDocSpanTextRef			fType;
		VString					fRef;
		VString					fRefNormalized;			//normalized reference (if 4D expression, it is expression normalized for current language and version and without special tokens)	
		VString					fDefaultValue;

		VString					fComputedValue;

		VRefPtr<VDocImage>		fImage;	//embedded image reference (if image type)
										//might be attached to a VDocText or not 

		IDocSpanTextRefLayout*	fLayoutDelegate; //layout delegate which might be used to paint embedded objects (like images)

static	IDocSpanTextRef*		fDelegate;
static	IDocSpanTextRef			fDefaultDelegate;
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
virtual							~VDocCache4DExp() { if (fSpanRef) delete fSpanRef; }

		/** return 4D expression value, as evaluated from the passed 4D tokenized expression
		@remarks
			returns cached value if 4D exp has been evaluated yet, otherwise eval 4D exp & append pair<exp,value> to the cache

			if fEvalAways == true, it always eval 4D exp & update cache
			if fEvalDeferred == true, if 4D exp is not in cache yet, it adds exp to the internal deferred list but does not eval now 4D exp (returns empty string as value):
									  caller might then call EvalDeferred() to eval deferred expressions and clear deferred list
		*/
		void					Get( const VString& inExp, VString& outValue, VDBLanguageContext *inLC = NULL);

		/** manually add a pair <exp,value> to the cache
		@remarks
			add or replace the existing value 

			caution: it does not check if value if valid according to the expression so it might be anything
		*/
		void					Set( const VString& inExp, const VString& inValue);

		/** return cache actual size */
		sLONG					GetCacheCount() const { return fMapOfExp.size(); }

		/** return the cache pair <exp,value> at the specified cache index (1-based) */
		bool					GetCacheExpAndValueAt( VIndex inIndex, VString& outExp, VString& outValue);

		/** append cached expressions from the passed cache */
		void					AppendFromCache( const VDocCache4DExp *inCache);

		/** remove the passed 4D tokenized exp from cache */
		void					Remove( const VString& inExp);

		/** clear cache */
		void					Clear() { fMapOfExp.clear(); }

		void					SetDBLanguageContext( VDBLanguageContext *inLC)
		{
			fDBLanguageContext = inLC;
			fMapOfExp.clear();
		}
		VDBLanguageContext*		GetDBLanguageContext() const { return fDBLanguageContext; }

		/** always eval 4D expression
		@remarks
			if true, Get() will always eval & update cached 4D expression, even if it is in the cache yet
			if false, Get() will only eval & cache 4D expression if it is not in the cache yet, otherwise it returns value from cache
		*/
		void					SetEvalAlways(bool inEvalAlways = true) { fEvalAlways = inEvalAlways; }
		bool					GetEvalAlways() const { return fEvalAlways; }

		/** defer 4D expression eval 
		@remarks
			see fEvalDeferred
		*/
		void					SetEvalDeferred(bool inEvalDeferred = true) { fEvalDeferred = inEvalDeferred; if (!fEvalDeferred) fMapOfDeferredExp.clear(); }
		bool					GetEvalDeferred() const { return fEvalDeferred; }

		/** clear deferred expression */
		void					ClearDeferred() { fMapOfDeferredExp.clear(); }

		/** eval deferred expressions & add to cache 
		@remarks
			return true if any expression has been evaluated

			on exit, clear also map of deferred expressions
		*/
		bool					EvalDeferred();

private:
		/** time stamp + exp value */
		typedef std::pair<uLONG, VString> ExpValue;

		/** map of exp value per exp */
		typedef unordered_map_VString< ExpValue > MapOfExp;
		
		/** map of deferred expressions (here boolean value is dummy & unused) */
		typedef unordered_map_VString< bool > MapOfDeferredExp;


		/** DB language context */
		VDBLanguageContext*		fDBLanguageContext;

		/** map of 4D expressions per 4D tokenized expression  */
		MapOfExp				fMapOfExp;
		
		/** map of deferred 4D expressions 
		@see
			fEvalDeferred
		*/
		MapOfDeferredExp		fMapOfDeferredExp;

		/** span ref (used locally to eval any 4D exp) */
		VDocSpanTextRef*		fSpanRef;

		/** if true, Get() will always eval 4D exp & update cached value 
			otherwise it returns cached value if any 
		*/
		bool					fEvalAlways;

		/** if true, Get() will return exp value from cache if exp has been evaluated yet
			otherwise it adds expression to evaluate to a internal deferred list:
			expressions in deferred list can later be evaluated by calling EvalDeferred	()
		@remarks
			if true, fEvalAlways is ignored
		*/
		bool					fEvalDeferred;

		sLONG					fMaxSize;
};

/** document paragraph class 
@remarks
	this class is used both for storing a paragraph node or a paragraph style (which includes paragraph props, common props & character styles)
	(if it it used only for styling, class should have none parent & text is ignored - see VTextLayout::ApplyParagraphStyle)
*/
class VDocText;
class XTOOLBOX_API VDocParagraph : public VDocNode
{
public:
friend	class					VDocNode;
friend	class					VDocText;

								VDocParagraph():VDocNode() 
		{
			fType = kDOC_NODE_TYPE_PARAGRAPH;
			fStyles = NULL;
			fListNumber = 1;
		}
virtual							~VDocParagraph() 
		{
			ReleaseRefCountable(&fStyles);
		}

virtual	VDocNode*				Clone() const {	return static_cast<VDocNode *>(new VDocParagraph(this)); }


		/** insert child node at passed position (0-based) */
virtual	void					InsertChild( VDocNode *inNode, VIndex inIndex) { xbox_assert(false); } //this node cannot have children (embedded elements are embedded as span references in VTreeTextStyle)

		/** add child node */
virtual	void					AddChild( VDocNode *inNode) { xbox_assert(false); } //this node cannot have children (embedded elements are embedded as span references in VTreeTextStyle)

		/** set class */
virtual	void					SetClass( const VString& inClass);

		const VString&			GetText() const
		{
			return fText;
		}
		VString*				GetTextPtr()
		{
			return &fText;
		}

		/** get text length 
		@remarks
			text length is 
			paragraph text length for paragraph,
			1 for image,
			otherwise sum of child nodes text length 
		*/
		uLONG					GetTextLength() const { return fText.GetLength(); }

		VTreeTextStyle*			GetStyles() const
		{
			return fStyles;
		}

		/** you should call this method if you have modified text externally - using GetTextPtr() */
		void					SetTextDirty() { _UpdateTextInfoEx(true); fDirtyStamp++; }

		/** set plain text and styles 
		@remarks
			replace text & styles 

			 styles range is relative to node start offset
		*/
virtual	bool					SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

		/** get plain text on the specified range (relative to node start offset) */
virtual	void					GetText(VString& outPlainText, sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set character styles
		@remarks
			replace character styles 
			(it is a full replacement so previous uniform style is not preserved as with ReplaceStyledText 
			 - use it only to initialize)

			 styles range is relative to node start offset
		*/
virtual	void					SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

		/** retain styles on the specified range (relative to node start offset) 
		@remarks
			return styles with range relative to node start offset 
		*/
virtual	VTreeTextStyle*			RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inNULLIfRangeEmpty = true, bool inNoMerge = false) const;

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocParagraph*			RetainFirstParagraph(sLONG inStart = 0) { return RetainRefCountable(this); }

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	const VDocParagraph*	GetFirstParagraph(sLONG inStart = 0) const { return this; }

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocNode*				RetainFirstChild(sLONG inStart = 0) const { return NULL; }

		/** return first paragraph which intersects the passed character index (relative to node start offset) */
virtual	VDocNode*				GetFirstChild(sLONG inStart = 0) const { return NULL; }

		/** replace styled text at the specified range (relative to node start offset) with new text & styles
		@remarks
			replaced text will inherit only uniform style on replaced range - if inInheritUniformStyle == true (default)

			return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
		*/
virtual	VIndex					ReplaceStyledText(	const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inStart = 0, sLONG inEnd = -1, 
													bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, const VDocSpanTextRef * inPreserveSpanTextRef = NULL);

		/** append properties from the passed node
		@remarks
			if inOnlyIfNotOverriden == true, local properties which are defined locally and not inherited are not overriden by inNode properties

			if inNode doc type is class style (VDocClassStyle instance), update node if and only if inClassStyleTarget = kDOC_NODE_TYPE_PARAGRAPH

			for paragraph, we need to append character styles too:
			but we append only character styles from fStyles VTextStyle, not from fStyles children if any
		*/
		void					AppendPropsFrom(	const VDocNode *inNode, bool inOnlyIfNotOverriden = false, bool inNoAppendSpanStyles = false, 
													sLONG inStart = 0, sLONG inEnd = -1, 
													uLONG inClassStyleTarget = kDOC_NODE_TYPE_PARAGRAPH, bool inClassStyleOverrideOnlyUniformCharStyles = true, 
													bool inOnlyIfNotOverridenRecurseUp = false); 


		/** replace plain text with computed or source span references */
virtual	void					UpdateSpanRefs( SpanRefCombinedTextTypeMask inShowSpanRefs = kSRCTT_Value, VDBLanguageContext *inLC = NULL);

		/** replace text with span text reference on the specified range
		@remarks
			span ref plain text is set here to uniform non-breaking space: 
			user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

			you should no more use or destroy passed inSpanRef even if method returns false
		*/
virtual	bool					ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inNoUpdateRef = true, VDBLanguageContext *inLC = NULL);

		/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range	 

			return true if any 4D expression has been replaced with plain text
		*/
virtual	bool					FreezeExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1);

		/** return the first span reference which intersects the passed range */
virtual	const VTextStyle*		GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outStartBase = NULL);

		/** normalize paragraph styles 
		@remarks
			it moves any span character property to fStyles 
			in order all character styles are stored only in fStyles
			
			normally this method should be called only by xhtml4D parser
		*/
		void					NormalizeStyles(bool inOverrideStyles = true, bool inApplyOnlyToUniformCharStyles = true);

		sLONG					GetCurListNumber() const { return fListNumber; }

protected:
								VDocParagraph( const VDocParagraph* inNodePara):VDocNode(static_cast<const VDocNode *>(inNodePara))	
		{
			fText = inNodePara->fText;
			fStyles = inNodePara->fStyles ? new VTreeTextStyle( inNodePara->fStyles) : NULL;
			fListNumber = 1;
			_UpdateTextInfo();
		}

static void						_BeforeReplace(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText) {}
static void						_AfterReplace(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced);

virtual	void					_OnAttachToDocument(VDocNode *inDocumentNode);
virtual	void					_OnDetachFromDocument(VDocNode *inDocumentNode);

virtual	void					_UpdateTextInfo(bool inUpdateTextLength = true, bool inApplyOnlyDeltaTextLength = false, sLONG inDeltaTextLength = 0);

virtual	void					_UpdateListNumber(eListStyleTypeEvent inListEvent = kDOC_LSTE_AFTER_ADD_TO_LIST);
virtual bool					_EqualListStringFormat( const VDocNode *inNode);

		VString					fText;
		VTreeTextStyle*			fStyles; 

		sLONG					fListNumber;
};


END_TOOLBOX_NAMESPACE

#endif
