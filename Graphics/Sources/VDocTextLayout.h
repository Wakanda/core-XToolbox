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
#ifndef __VDocTextLayout__
#define __VDocTextLayout__

BEGIN_TOOLBOX_NAMESPACE

#if !ENABLE_VDOCTEXTLAYOUT

#define VDocTextLayout VDocParagraphLayout
  
#else

/**
VDocContainerLayout

@remarks
	this class is dedicated to manage container layout (for instance VDocTextLayout & VDocTableCellLayout derive from this class)
	
	- a container layout might embed only paragraph or container layouts
		(because image or table layout are always embedded in a paragraph layout)
	- for now container children are rendered only vertically one below the other (TODO: allow children to be rendered horizontally - as in HTML according to CSS 'float' property)

	- a container layout has not specific properties: it inherits only its properties from VDocBaseLayout
	(important: please do not break that rule -> specific container should have a dedicated class which derive from that class)

	- in order to override some element(s) property(ies), you should use ApplyClassStyle, passing a custom class style with empty class name - filled with only the property(ies) you want to override - and the document element node type:
	it will override all element(s) property(ies) for elements which match the target document node type and intersect the passed range
	- in order to get some element(s) property(ies), you should use RetainLayoutFromCharPos which returns the first layout element which matches the passed document node type at the passed character index
	These two methods make it easy to read/write element properties on a specified range
	IMPORTANT: in order to modify text character styles but default, you should use ApplyStyle(s) but ApplyClassStyle: with ApplyClassStyle, you can only override paragraph default uniform text character styles.
	 
	- constructor is protected so you can create only a specialized class instance - for instance VDocTextLayout
*/
class XTOOLBOX_API VDocContainerLayout: public VDocBaseLayout, public ITextLayout
{
public:

virtual	bool				IsImplDocLayout() const { return true; } 

protected:
							/**	VDocContainerLayout constructor
							@param inNode
								container node:	passed node is used to initialize container layout
								if NULL, initialize default container
							@param inShouldEnableCache
								enable/disable cache (default is true): see ShouldEnableCache
							*/
							VDocContainerLayout(const VDocNode *inNode, bool inShouldEnableCache = true, VDocClassStyleManager *ioCSMgr = NULL);

virtual						~VDocContainerLayout();

public:

/////////////////	IDocSpanTextRefLayout interface

virtual	void				ReleaseRef() { IRefCountable::Release(); }

/////////////////	IDocBaseLayout interface

virtual	void				SetDPI( const GReal inDPI = 72);
virtual GReal				GetDPI() const { return VDocBaseLayout::GetDPI(); }

		/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&		GetLayoutBounds() const { return VDocBaseLayout::GetLayoutBounds(); }

		/** get typo local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&		GetTypoBounds() const { return fTypoBounds; }

		/** get background bounds (in px - relative to fDPI) */
virtual	const VRect&		GetBackgroundBounds(eDocPropBackgroundBox inBackgroundBox = kDOC_BACKGROUND_BORDER_BOX) const { return VDocBaseLayout::GetBackgroundBounds( inBackgroundBox); }

		/** update text layout metrics */ 
virtual	void				UpdateLayout(VGraphicContext *inGC);
							
		/** render paragraph in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual	void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f);

		/** query text layout interface (if available for this class instance) 
		@remarks
			for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
			might return NULL
		*/ 
virtual	ITextLayout*		QueryTextLayoutInterface() { return ITextLayout::QueryTextLayoutInterface(); }

		/** query text layout interface (if available for this class instance) 
		@remarks
			for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
			might return NULL
		*/ 
virtual	const ITextLayout*	QueryTextLayoutInterfaceConst() const { return ITextLayout::QueryTextLayoutInterfaceConst();  }

		/** return document node type */
virtual	eDocNodeType		GetDocType() const { return kDOC_NODE_TYPE_UNKNOWN; } //should be defined in specialized class

virtual	VDocTextLayout*		GetDocTextLayout() const { return fDoc; }

		/** get current ID */
virtual	const VString&		GetID() const { return VDocBaseLayout::GetID(); }

		/** set current ID (custom ID) */
virtual	bool				SetID(const VString& inValue) { return VDocBaseLayout::SetID( inValue); }
		
		/** get class*/
virtual	const VString&		GetClass() const { return VDocBaseLayout::GetClass(); }

		/** set class */
virtual	void				SetClass( const VString& inClass) { VDocBaseLayout::SetClass(inClass); }

		/** retain layout node which matches the passed ID (will return always NULL if node is not attached to a VDocTextLayout) */
virtual	VDocBaseLayout*		RetainLayout(const VString& inID) { return VDocBaseLayout::RetainLayout( inID); }

		/** get text start index 
		@remarks
			index is relative to parent text layout
			(if layout has not a parent, it should be always 0)
		*/
virtual	sLONG				GetTextStart() const { return VDocBaseLayout::GetTextStart(); }

		/** return local text length 
		@remarks
			it should be at least 1 no matter the kind of layout (any layout object should have at least 1 character size)
		*/
virtual	uLONG				GetTextLength() const { return VDocBaseLayout::GetTextLength(); }

		/** get parent layout reference 
		@remarks
			for instance for a VDocParagraphLayout instance, it might be a VDocContainerLayout instance (paragraph layout container)
			(but as VDocParagraphLayout instance can be used stand-alone, it might be NULL too)
		*/
virtual IDocBaseLayout*		GetParent() { return VDocBaseLayout::GetParent(); }

		/** get count of child layout instances */
virtual	uLONG				GetChildCount() const { return VDocBaseLayout::GetChildCount(); }
	
		/** get layout child index (relative to parent child vector & 0-based) */
virtual sLONG				GetChildIndex() const { return VDocBaseLayout::GetChildIndex(); }

		/** add child layout instance */
virtual void				AddChild( IDocBaseLayout* inLayout) { VDocBaseLayout::AddChild( inLayout); _PropagatePropsToLayout( dynamic_cast<VDocBaseLayout *>(inLayout), true); SetDirty(); }

		/** insert child layout instance (index is 0-based) */
virtual void				InsertChild( IDocBaseLayout* inLayout, sLONG inIndex = 0) { VDocBaseLayout::InsertChild( inLayout, inIndex); _PropagatePropsToLayout( dynamic_cast<VDocBaseLayout *>(inLayout), true); SetDirty(); }

		/** remove child layout instance (index is 0-based) */
virtual void				RemoveChild( sLONG inIndex) { if (testAssert(GetChildCount() > 1)) { VDocBaseLayout::RemoveChild( inIndex); SetDirty(); } } //at least one paragraph

		/** get child layout instance from index (0-based) */
virtual	IDocBaseLayout*		GetNthChild(sLONG inIndex) const { return VDocBaseLayout::GetNthChild( inIndex); }

		/** retain first child layout instance which intersects the passed text index */
virtual IDocBaseLayout*		GetFirstChild( sLONG inStart = 0) const { return VDocBaseLayout::GetFirstChild( inStart); }

virtual	bool				IsLastChild(bool inLookUp = false) const { return VDocBaseLayout::IsLastChild( inLookUp); }

		/** initialize from the passed node */
virtual	void				InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInheritedDefault = false, bool inClassStyleOverrideOnlyUniformCharStyles = true, VDocClassStyleManager *ioCSMgr = NULL);

		/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

		//container-scope properties

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true);
        /** get outside margins (in twips) */
virtual	void				GetMargins( sDocPropRect& outMargins) const { VDocBaseLayout::GetMargins( outMargins); }

		/** set inside margins	(in twips) - CSS padding */
virtual	void				SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin = true);
		/** get inside margins	(in twips) - CSS padding */
virtual	void				GetPadding(sDocPropRect& outPadding) const { VDocBaseLayout::GetPadding( outPadding); }

		/** set text align - CSS text-align (-1 for not defined) */
virtual void				SetTextAlign( const AlignStyle inHAlign); 
virtual AlignStyle			GetTextAlign() const;

		/** set vertical align - CSS vertical-align (-1 for not defined) */
virtual void				SetVerticalAlign( const AlignStyle inVAlign);
virtual AlignStyle			GetVerticalAlign() const { return VDocBaseLayout::GetVerticalAlign(); }

		/** set width (in twips) - CSS width (0 for auto) */
virtual	void				SetWidth(const uLONG inWidth);
virtual uLONG				GetWidth(bool inIncludeAllMargins = false) const { return VDocBaseLayout::GetWidth( inIncludeAllMargins); }

		/** set height (in twips) - CSS height (0 for auto) */
virtual	void				SetHeight(const uLONG inHeight);
virtual uLONG				GetHeight(bool inIncludeAllMargins = false) const { return VDocBaseLayout::GetHeight( inIncludeAllMargins); } 

		/** set min width (in twips) - CSS min-width (0 for auto) */
virtual	void				SetMinWidth(const uLONG inWidth);
virtual uLONG				GetMinWidth(bool inIncludeAllMargins = false) const { return VDocBaseLayout::GetMinWidth( inIncludeAllMargins); }

		/** set min height (in twips) - CSS min-height (0 for auto) */
virtual	void				SetMinHeight(const uLONG inHeight);
virtual uLONG				GetMinHeight(bool inIncludeAllMargins = false) const { return VDocBaseLayout::GetMinHeight( inIncludeAllMargins); } 

		/** set background color - CSS background-color (transparent for not defined) */
virtual void				SetBackgroundColor( const VColor& inColor);
virtual const VColor&		GetBackgroundColor() const { return VDocBaseLayout::GetBackgroundColor(); }

		/** set background clipping - CSS background-clip */
virtual void				SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip);
virtual eDocPropBackgroundBox	GetBackgroundClip() const { return VDocBaseLayout::GetBackgroundClip(); }

		/** set background origin - CSS background-origin */
virtual void				SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin);
virtual eDocPropBackgroundBox	GetBackgroundOrigin() const { return VDocBaseLayout::GetBackgroundOrigin(); }

		/** set borders 
		@remarks
			if inLeft is NULL, left border is same as right border 
			if inBottom is NULL, bottom border is same as top border
			if inRight is NULL, right border is same as top border
			if inTop is NULL, top border is default border (solid, medium width & black color)
		*/
		void				SetBorders( const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL);

		/** clear borders */
		void				ClearBorders();
							
		/** set background image 
		@remarks
			see VDocBackgroundImageLayout constructor for parameters description */
		void				SetBackgroundImage(	const VString& inURL,	
												const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
												const eDocPropBackgroundRepeat inRepeat = kDOC_BACKGROUND_REPEAT,
												const GReal inWidth = kDOC_BACKGROUND_SIZE_AUTO, const GReal inHeight = kDOC_BACKGROUND_SIZE_AUTO,
												const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false);

		/** clear background image */
		void				ClearBackgroundImage();


/////////////////	ITextLayout override

virtual	bool				IsDirty() const { return fIsDirty; }
		/** set dirty */
virtual	void				SetDirty(bool inApplyToChildren = false, bool inApplyToParent = true);

		/** enable/disable cache */
virtual	void				ShouldEnableCache( bool inShouldEnableCache);
virtual	bool				ShouldEnableCache() const { return fShouldEnableCache; }

virtual	void				SetPlaceHolder( const VString& inPlaceHolder);
virtual	void				GetPlaceHolder( VString& outPlaceHolder) const { outPlaceHolder = fPlaceHolder; }

		/** enable/disable password mode */
virtual	void				EnablePassword(bool inEnable);
virtual	bool				IsEnabledPassword() const { return fIsPassword; }

		/** allow/disallow offscreen text rendering (default is false)
		@remarks
			text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
			you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
		@see
			ShouldEnableCache
		*/
virtual	void				ShouldDrawOnLayer( bool inShouldDrawOnLayer);
virtual	bool				ShouldDrawOnLayer() const { return fShouldDrawOnLayer; }
virtual	void				SetLayerDirty(bool inApplyToChildren = false);

		/** paint selection */
virtual	void				ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor = NULL, const VColor* inInactiveColor = NULL);
virtual	bool				ShouldPaintSelection() const { return fShouldPaintSelection; }
		/** change selection (selection is used only for painting) */
virtual	void				ChangeSelection( sLONG inStart, sLONG inEnd = -1, bool inActive = true);
virtual	bool				IsSelectionActive() const { return fSelIsActive; }

		/** create and retain document from the current layout  
		@param inStart
			range start
		@param inEnd
			range end

		@remarks
			if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
			with a single child node initialized from the current layout
		*/
virtual	VDocText*			CreateDocument(sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** create and retain paragraph from the current layout plain text, styles & paragraph properties 
		@param inStart
			range start
		@param inEnd
			range end
		*/
virtual	VDocParagraph*		CreateParagraph(sLONG inStart = 0, sLONG inEnd = -1);

		/** apply class style to the current layout and/or children depending on inTargetDocType
		@param inClassStyle (mandatory only for VDocTextLayout impl)
			the class style - if inClassStyle class name is not empty, apply only to current or children nodes with same class name
		@param inResetStyles
			if true, reset styles to default prior to apply new styles
		@param inReplaceClass (mandatory only for VDocTextLayout impl)
			if inClassStyle class is not empty:
				if true, apply class style to current layout and/or children which match inTargetDocType and change layout class to inClassStyle class
				if false, apply class style to current layout and/or children which match both inTargetDocType and inClassStyle class
			if inClassStyle class is empty, ignore inReplaceClass and apply inClassStyle to current layout and/or children which match inTargetDocType - and do not modify layout class
		@param inStart (mandatory only if current layout is a container)
			text range start 
		@param inEnd (mandatory only if current layout is a container)
			text range end 
		@param inTargetDocType (mandatory only for VDocTextLayout impl)
			apply to current layout and/or children if and only if layout document type is equal to inTargetDocType
			(it works also with embedded images or tables)
		*/
virtual	bool				ApplyClassStyle( const VDocClassStyle *inClassStyle, bool inResetStyles = false, bool inReplaceClass = false, sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH);

		/** apply class to the current layout and/or children depending on inTargetDocType
			(mandatory only for VDocTextLayout impl)
		@param inClass
			the class name 
		@param inResetStyles
			if true, reset styles to default prior to apply class styles
		@param inStart (mandatory only if current layout is a container)
			text range start 
		@param inEnd (mandatory only if current layout is a container)
			text range end 
		@param inTargetDocType 
			apply to current layout and/or children if and only if layout document type is equal to inTargetDocType
			(it works also with embedded images or tables)

		@remarks
			if might fail if there is none class style associated with the class name (you should use RetainClassStyle and/or AddOrReplaceClassStyle to edit class style prior to call this method)
		*/
virtual	bool				ApplyClass( const VString &inClass, bool inResetStyles = false, sLONG inStart = 0, sLONG inEnd = -1, eDocNodeType inTargetDocType = kDOC_NODE_TYPE_PARAGRAPH) { return VDocBaseLayout::ApplyClass( inClass, inResetStyles, inStart, inEnd, inTargetDocType); }

		/** retain class style */
virtual	VDocClassStyle*		RetainClassStyle( const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH, bool inAppendDefaultStyles = false) const { return VDocBaseLayout::RetainClassStyle( inClass, inDocNodeType, inAppendDefaultStyles); }

		/** add or replace class style
		@remarks
			class style might be modified if inDefaultStyles is not equal to NULL (as only styles not equal to default are stored)
		*/
virtual	bool				AddOrReplaceClassStyle( const VString& inClass, VDocClassStyle *inClassStyle, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH, bool inRemoveDefaultStyles = false) { return VDocBaseLayout::AddOrReplaceClassStyle( inClass, inClassStyle, inDocNodeType, inRemoveDefaultStyles); }

		/** remove class style */
virtual bool				RemoveClassStyle( const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH) { return VDocBaseLayout::RemoveClassStyle(inClass, inDocNodeType); }

		/** clear class styles */
virtual	bool				ClearClassStyles( bool inKeepDefaultStyles) { return VDocBaseLayout::ClearClassStyles( inKeepDefaultStyles); }


		/** accessor on class style manager */
virtual	VDocClassStyleManager*	GetClassStyleManager() const { return VDocBaseLayout::GetClassStyleManager(); }

		/** init layout from the passed paragraph 
		@param inPara
			paragraph source
		@remarks
			the passed paragraph is used only for initializing layout plain text, styles & paragraph properties but inPara is not retained 
			note here that current styles are reset (see ResetParagraphStyle) & replaced by paragraph styles
			also current plain text is replaced by the passed paragraph plain text 
		*/
virtual	void				InitFromParagraph( const VDocParagraph *inPara);

		/** reset all text properties & character styles to default values */
virtual	void				ResetParagraphStyle(bool inResetParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1);

		/** merge paragraph(s) on the specified range to a single paragraph 
		@remarks
			if there are more than one paragraph on the range, plain text & styles from the paragraphs but the first one are concatenated with first paragraph plain text & styles
			resulting with a single paragraph on the range (using paragraph styles from the first paragraph) where plain text might contain more than one CR

			remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
			you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
		*/
virtual	bool				MergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1);

		/** retain layout at the specified character index which matches the specified document node type */ 
virtual	VDocBaseLayout*		RetainLayoutFromCharPos( sLONG inPos, const eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH);

		/** unmerge paragraph(s) on the specified range 
		@remarks
			any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR, 
			resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)

			remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
			you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
		*/
virtual	bool				UnMergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1);

virtual	bool				HasSpanRefs() const;

		/** convert uniform character index (the index used by 4D commands) to actual plain text position 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				CharPosFromUniform( sLONG& ioPos) const;

		/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				CharPosToUniform( sLONG& ioPos) const;

		/** convert uniform character range (as used by 4D commands) to actual plain text position 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				RangeFromUniform( sLONG& inStart, sLONG& inEnd) const;

		/** convert actual plain text range to uniform range (as used by 4D commands) 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				RangeToUniform( sLONG& inStart, sLONG& inEnd) const;

		/** adjust selection 
		@remarks
			you should call this method to adjust with surrogate pairs or span references
		*/
virtual	void				AdjustRange(sLONG& ioStart, sLONG& ioEnd) const;

		/** adjust char pos
		@remarks
			you should call this method to adjust with surrogate pairs or span references
		*/
virtual	void				AdjustPos(sLONG& ioPos, bool inToEnd = true) const;

		/** replace text with span text reference on the specified range
		@remarks
			it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

			you should no more use or destroy passed inSpanRef even if method returns false

			IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
					   so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
					   (passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens 
					    - if display type is set to normalized source)
		*/
virtual	bool				ReplaceAndOwnSpanRef(	VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = false, 
													void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

		/** return span reference at the specified text position if any */
virtual	const VTextStyle*	GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart = NULL) const;

		/** return the first span reference which intersects the passed range */
virtual	const VTextStyle*	GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outBaseStart = NULL) const;

		/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
virtual	bool				UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

		/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
virtual	bool				FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

		/** eval 4D expressions  
		@remarks
			note that it only updates span 4D expression references but does not update plain text:
			call UpdateSpanRefs after to update layout plain text

			return true if any 4D expression has been evaluated
		*/
virtual	bool				EvalExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL);

		/** set layer view rectangle (user space is VView window user space - hwnd user space)
		@remarks
			useful only if offscreen layer is enabled

			by default, layer surface size is equal to text layout size in pixels
			but it can be useful to constraint layer surface size to not more than the visible area size of the text layout container
			in order to avoid to draw offscreen all the text layout (especially if the text size is great...)

			for instance if VDocParagraphLayout container is a VView-derived class & fTextLayout is the text layout attached or used in the VView-derived class,
			in the VView-derived class DoDraw method & prior to call VDocParagraphLayout::Draw, you should insert this code to set the layer view rect:

			[code/]

			VRect boundsHwnd;
			GetHwndBounds(boundsHwnd);
			fTextLayout->SetLayerViewRect( boundsHwnd);

			[/code]
			
			with that code, layer size will be not greater than min of layout size & VView visible area
			(see class VStyledTextEdtiView for a sample)

			You can still reset to VRect() to use layout size as layer size if you intend to pre-render all text layout 
			(only if you have called before SetLayerViewRect with a not empty VRect)
		*/
virtual	void				SetLayerViewRect( const VRect& inRectRelativeToWnd);
virtual	const VRect&		GetLayerViewRect() const { return fLayerViewRect; }

		/** insert text at the specified position */
virtual	void				InsertPlainText( sLONG inPos, const VString& inText);

		/** delete text range */
virtual	void				DeletePlainText( sLONG inStart, sLONG inEnd);

		/** replace text */
virtual	void				ReplacePlainText( sLONG inStart, sLONG inEnd, const VString& inText);

		/** apply style (use style range) */
virtual	bool				ApplyStyle( const VTextStyle* inStyle, bool inFast = false, bool inIgnoreIfEmpty = true);

virtual	bool				ApplyStyles( const VTreeTextStyle * inStyles, bool inFast = false);

		/** show span references source or value */ 
virtual	void				ShowRefs( SpanRefCombinedTextTypeMask inShowRefs, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);
virtual	SpanRefCombinedTextTypeMask ShowRefs() const { return fShowRefs; }

		/** highlight span references in plain text or not */
virtual	void				HighlightRefs(bool inHighlight, bool inUpdateAlways = false);
virtual	bool				HighlightRefs() const { return fHighlightRefs; }

		/** call this method to update span reference style while mouse enters span ref 
		@remarks
			return true if style has changed

			you should pass here a reference returned by GetStyleRefAtPos or GetStyleRefAtRange 
			(method is safe even if reference has been destroyed - i.e. if inStyleSpanRef is no more valid)
		*/
virtual	bool				NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0);

		/** call this method to update span reference style while mouse leaves span ref 
		@remarks
			return true if style has changed

			you should pass here a reference returned by GetStyleRefAtPos or GetStyleRefAtRange
			(method is safe even if reference has been destroyed - i.e. if inStyleSpanRef is no more valid)
		*/
virtual	bool				NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0);

		/** set default font 
		@remarks
			it should be not NULL - STDF_TEXT standard font on default (it is applied prior styles set with SetText)
			
			by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
		*/
virtual	void				SetDefaultFont( const VFont *inFont, GReal inDPI = 72.0f, sLONG inStart = 0, sLONG inEnd = -1);

		/** get & retain default font 
		@remarks
			by default, return 4D form-compliant font (with dpi = 72) so not the actual fRenderDPI-scaled font 
			(for consistency with SetDefaultFont & because fRenderDPI internal scaling should be transparent for caller)
		*/
virtual	VFont*				RetainDefaultFont(GReal inDPI = 72.0f, sLONG inStart = 0) const;

		/** set default text color 
		@remarks
			black color on default (it is applied prior styles set with SetText)
		*/
virtual	void				SetDefaultTextColor( const VColor &inTextColor, sLONG inStart = 0, sLONG inEnd = -1);

		/** get default text color */
virtual	bool				GetDefaultTextColor(VColor& outColor, sLONG inStart = 0) const;

		/** set back color 
		@param  inColor
			background color
		@param	inPaintBackColor
			unused (preserved for compatibility with VTextLayout)
		@remarks
			OBSOLETE: you should use SetBackgroundColor - it is preserved for compatibility with VTextLayout
		*/
virtual void				SetBackColor( const VColor& inColor, bool inPaintBackColor = false) { SetBackgroundColor( inColor); }
virtual	const VColor&		GetBackColor() const { return GetBackgroundColor(); }
		
		/** replace current text & styles 
		@remarks
			if inCopyStyles == true, styles are copied
			if inCopyStyles == false (default), styles might be retained: in that case, if you modify passed styles you should call this method again 

			this method preserves current uniform style on range (first character styles) if not overriden by the passed styles 
		*/
virtual	void				SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false)
		{
			SetText( inText, inStyles, 0, -1);
		}

		/** replace current text & styles
		@remarks	
			here passed styles are always copied

			this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
		*/
virtual	void				SetText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd = -1, 
									 bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
									 const VDocSpanTextRef * inPreserveSpanTextRef = NULL, 
									 void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

		/** const accessor on layout text 
		@remarks
			if it is a container, return first paragraph text
		*/ 
virtual	const VString&		GetText() const;

virtual	void				GetText(VString& outText, sLONG inStart = 0, sLONG inEnd = -1) const;

		/** return unicode character at passed position (0-based) - 0 if out of range */
virtual	UniChar				GetUniChar( sLONG inIndex) const;

		/** return styles on the passed range 
		@remarks
			if inIncludeDefaultStyle == false (default is true) and if layout is a paragraph, it does not include paragraph uniform style (default style) but only styles which are not uniform on paragraph text:
			otherwise it returns all styles applied to the plain text

			caution: if inCallerOwnItAlways == false (default is true), returned styles might be shared with layout instance so you should not modify it
		*/
virtual	VTreeTextStyle*		RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inIncludeDefaultStyle = true, bool inCallerOwnItAlways = true) const;

		/** set extra text styles 
		@remarks
			it can be used to add a temporary style effect to the text layout without modifying the main text styles
			(these styles are not exported to VDocParagraph by CreateParagraph)

			if inCopyStyles == true, styles are copied
			if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
		*/
virtual	void				SetExtStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

virtual	VTreeTextStyle*		RetainExtStyles() const;

		/** replace ouside margins	(in point) */
virtual	void				SetMargins(const GReal inMarginLeft, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f)
		{
			ITextLayout::SetMargins(inMarginLeft, inMarginRight, inMarginTop, inMarginBottom);
		}

		/** get outside margins (in point) */
virtual	void				GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom) const
		{
			ITextLayout::GetMargins(outMarginLeft, outMarginRight, outMarginTop, outMarginBottom);
		}

		/** set max text width (0 for infinite) - in pixel relative to the layout target DPI
		@remarks
			it is also equal to text box width 
			so you should specify one if text is not aligned to the left border
			otherwise text layout box width will be always equal to formatted text width
		*/ 
virtual	void				SetMaxWidth(GReal inMaxWidth = 0.0f);
		GReal				GetMaxWidth() const { return fRenderMaxWidth; }

		/** set max text height (0 for infinite) - in pixel relative to the layout target DPI
		@remarks
			it is also equal to text box height 
			so you should specify one if paragraph is not aligned to the top border
			otherwise text layout box height will be always equal to formatted text height
		*/ 
virtual	void				SetMaxHeight(GReal inMaxHeight = 0.0f);
virtual	GReal				GetMaxHeight() const { return fRenderMaxHeight; }

		/** get paragraph line height 

			if positive, it is fixed line height in point
			if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
			if -1, it is normal line height 
		*/
virtual	GReal				GetLineHeight(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set paragraph line height 

			if positive, it is fixed line height in point
			if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
			if -1, it is normal line height 
		*/
virtual	void				SetLineHeight( const GReal inLineHeight, sLONG inStart = 0, sLONG inEnd = -1);

virtual	bool				IsRTL(sLONG inStart = 0, sLONG inEnd = -1) const { return (GetLayoutMode() & TLM_RIGHT_TO_LEFT) != 0; }
virtual	void				SetRTL(bool inRTL, sLONG inStart = 0, sLONG inEnd = -1);

		/** get first line padding offset in point
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
virtual	GReal				GetTextIndent(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set first line padding offset in point 
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
virtual	void				SetTextIndent(const GReal inPadding, sLONG inStart = 0, sLONG inEnd = -1);

		/** get tab stop offset in point */
virtual	GReal				GetTabStopOffset(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set tab stop (offset in point) */
virtual	void				SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG inStart = 0, sLONG inEnd = -1);

		/** get tab stop type */ 
virtual	eTextTabStopType	GetTabStopType(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set text layout mode (default is TLM_NORMAL) */
virtual	void				SetLayoutMode( TextLayoutMode inLayoutMode = TLM_NORMAL, sLONG inStart = 0, sLONG inEnd = -1);
		/** get text layout mode */
virtual	TextLayoutMode		GetLayoutMode(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set class for new paragraph on RETURN */
virtual	void				SetNewParaClass(const VString& inClass, sLONG inStart = 0, sLONG inEnd = -1);

		/** get class for new paragraph on RETURN */
virtual	const VString&		GetNewParaClass(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set list style type 
		@remarks
			reset all list item properties if inType == kDOC_LIST_STYLE_TYPE_NONE
			(because other list item properties are meaningless if paragraph is not a list item)
		*/
virtual	void				SetListStyleType( eDocPropListStyleType inType, sLONG inStart = 0, sLONG inEnd = -1);
		/** get list style type */
virtual	eDocPropListStyleType GetListStyleType( sLONG inStart = 0, sLONG inEnd = -1) const;
	
		/** set list string format LTR */
virtual	void				SetListStringFormatLTR( const VString& inStringFormat, sLONG inStart = 0, sLONG inEnd = -1);
		/** get list string format LTR */
virtual	const VString&		GetListStringFormatLTR( sLONG inStart = 0, sLONG inEnd = -1) const;
		
		/** set list string format RTL */
virtual	void				SetListStringFormatRTL( const VString& inStringFormat, sLONG inStart = 0, sLONG inEnd = -1);
		/** get list string format RTL */
virtual	const VString&		GetListStringFormatRTL( sLONG inStart = 0, sLONG inEnd = -1) const;
	
		/** set list font family */
virtual	void				SetListFontFamily( const VString& inFontFamily, sLONG inStart = 0, sLONG inEnd = -1);
		/** get list font family */
virtual	const VString&		GetListFontFamily( sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set list image url */
virtual	void				SetListStyleImage( const VString& inURL, sLONG inStart = 0, sLONG inEnd = -1);
		/** get list image url */
virtual	const VString&		GetListStyleImage( sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set list image height in point (0 for auto) */
virtual	void				SetListStyleImageHeight( GReal inHeight, sLONG inStart = 0, sLONG inEnd = -1);
		/** get list image height in point (0 for auto) */
virtual	GReal				GetListStyleImageHeight( sLONG inStart = 0, sLONG inEnd = -1) const;

		/** set or clear list start number */
virtual	void				SetListStart( sLONG inListStartNumber = 1, sLONG inStart = 0, sLONG inEnd = -1, bool inClear = false);
		/** get list start number 
		@remarks
			return true and start number if start number is set otherwise return false
		*/
virtual	bool				GetListStart( sLONG* outListStartNumber = NULL, sLONG inStart = 0, sLONG inEnd = -1) const;


		/** begin using text layout for the specified gc (reentrant)
		@remarks
			for best performance, it is recommended to call this method prior to more than one call to Draw or metrics get methods & terminate with EndUsingContext
			otherwise Draw or metrics get methods will check again if layout needs to be updated at each call to Draw or metric get method
			But you do not need to call BeginUsingContext & EndUsingContext if you call only once Draw or a metrics get method.

			CAUTION: if you call this method, you must then pass the same gc to Draw or metrics get methods before EndUsingContext is called
					 otherwise Draw or metrics get methods will do nothing (& a assert failure is raised in debug)

			If you do not call Draw method, you should pass inNoDraw = true to avoid to create a drawing context on some impls (actually for now only Direct2D impl uses this flag)
		*/
virtual	void				BeginUsingContext( VGraphicContext *inGC, bool inNoDraw = false);

		/** return layout bounds including margins (in gc user space)
		@remarks
			layout origin is set to inTopLeft
			on output, outBounds contains text layout bounds including any glyph overhangs; outBounds left top origin should be equal to inTopLeft

			on input, max width is determined by GetMaxWidth() & max height by GetMaxHeight()
			output layout width will be equal to max width if max width != 0 otherwise to formatted text width if no parent or to parent content max width (recursively determined if equal to 0)
			output layout height will be equal to max height if max height != 0 otherwise to formatted text height
			(see CAUTION remark below)

			if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
			(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
			 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)

			CAUTION: layout bounds can be greater than formatted text bounds (if max width or max height is not equal to 0)
					 because layout bounds are equal to the text box design bounds and not to the formatted text bounds: 
					 you should use GetTypoBounds in order to get formatted text width & height
		*/
virtual	void				GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

		/** return formatted text bounds (typo bounds) including margins (in gc user space)
		@remarks
			layout origin is set to inTopLeft

			as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
			because layout bounds are equal to the design bounds and not to the formatted text bounds
			you should use this method to get formatted text width & height 
		*/
virtual	void				GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

		/** return bounding rectangle for child layout node(s) matching the passed document node type intersecting the passed text range 
		    or current layout bounds if current layout document type matches the passed document node type
		@remarks
			text layout origin is set to inTopLeft (in gc user space)
			output bounds are in gc user space
		*/
virtual void				GetLayoutBoundsFromRange( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1, const eDocNodeType inChildDocNodeType = kDOC_NODE_TYPE_PARAGRAPH);

		/** return text layout run bounds from the specified range (in gc user space)
		@remarks
			layout origin is set to inTopLeft
		*/
virtual	void				GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);

		/** return the caret position & height of text at the specified text position in the text layout (in gc user space)
		@remarks
			layout origin is set to inTopLeft

			text position is 0-based

			caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
			if text layout is drawed at inTopLeft

			by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
			but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
		*/
virtual	void				GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);

		/** return the text position at the specified coordinates
		@remarks
			layout origin is set to inTopLeft (in gc user space)
			the hit coordinates are defined by inPos param (in gc user space)

			output text position is 0-based

			return true if input position is inside character bounds
			if method returns false, it returns the closest character index to the input position
		*/
virtual	bool				GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength = NULL);

		/** get character range which intersects the passed bounds (in gc user space) 
		@remarks
			layout origin is set to inTopLeft (in gc user space)

			char range is rounded to line run limit so the returned range might be slightly greater than the actual char range that intersects the passed bounds
		*/
virtual	void				GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd);


		/** move character index to the nearest character index on the line before/above the character line 
		@remarks
			return false if character is on the first line (ioCharIndex is not modified)
		*/
virtual	bool				MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex);

		/** move character index to the nearest character index on the line after/below the character line
		@remarks
			return false if character is on the last line (ioCharIndex is not modified)
		*/
virtual	bool				MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex);
		
		/** consolidate end of lines
		@remarks
			if text is multiline:
				we convert end of lines to a single CR 
			if text is singleline:
				if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
				if inEscaping, we escape line endings and tab 
		*/
virtual	bool				ConsolidateEndOfLines(bool inIsMultiLine = true, bool inEscaping = false, sLONG inStart = 0, sLONG inEnd = -1);

		/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) range at the specified character index */
virtual	void				GetWordRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces = true, bool inTrimPunctuation = false);
		
		/**	get paragraph range at the specified character index 
		@remarks
			if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
			(for VTextLayout, parameter is ignored)
		*/
virtual	void				GetParagraphRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange = false);

		/** return true if layout uses truetype font only */
virtual	bool				UseFontTrueTypeOnly(VGraphicContext *inGC) const;

		/** end using text layout (reentrant) */
virtual	void				EndUsingContext();

virtual	VTreeTextStyle*		RetainDefaultStyle(sLONG inStart = 0, sLONG inEnd = -1) const;

		void				_PropagatePropsToLayout( VDocBaseLayout *inLayout, bool inSkipPlaceholder = true);

protected:
typedef	std::vector<VPoint>	VectorOfPoint;

		void				_Init();

		void				_InitFromNode( const VDocNode *inNode, VDocClassStyleManager *ioCSMgr = NULL);
		void				_ReplaceFromNode(	const VDocNode *inNode, sLONG inStart = 0, sLONG inEnd = -1,
												bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
												void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

		/** remove class */
virtual	bool				_RemoveClass( const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH);

		/** clear class */
virtual	bool				_ClearClass();

		void				_ShouldEnableCache(bool inEnable);
		void				_ShouldDrawOnLayer(bool inEnable);

static void					_BeforeReplace(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText);
static void					_AfterReplace(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced);

		void				_UpdateLayout();

virtual void				_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw, const VPoint& inPreDecal = VPoint());

		VDocParagraphLayout*	_GetParagraphFromCharIndex(const sLONG inCharIndex, sLONG *outCharStartIndex = NULL) const;

		/** get child layout from local position (relative to layout box origin+vertical align decal & computing dpi)*/
		VDocBaseLayout*		_GetChildFromLocalPos(const VPoint& inPos);

		void				_SetGC( VGraphicContext *inGC);

		// design datas
 
		/** cache usage status */
		bool				fShouldEnableCache;

		/** layer usage status */
		bool				fShouldDrawOnLayer;

		/** show references status */
		SpanRefCombinedTextTypeMask fShowRefs;

		/** highlight references status */
		bool 				fHighlightRefs;

		//// selection management (we store only paint selection properties as management is actually done in child instances)

		/** should paint selection ? */
		bool				fShouldPaintSelection;

		/** selection range (used only for painting) */
		sLONG				fSelStart, fSelEnd;
		bool				fSelIsActive;

		/** selection color */
		VColor				fSelActiveColor, fSelInactiveColor;

		//// placeholder management

		VString				fPlaceHolder;

		//// password management

		bool				fIsPassword;
		VString				fTextPassword;

		//// layer management

		VRect				fLayerViewRect;


		//computed datas

		//// computed metrics

		//// layout metrics (in computing dpi)

		/** typographic bounds in px - relative to computing DPI - including margin, padding & border 
		@remarks
			formatted text bounds (minimal bounds to display all text content with margin, padding & border)
		*/
		VRect				fTypoBounds;

		/** margins in px - relative to computing dpi 
		@remarks
			same as fAllMargin but rounded according to gc pixel snapping (as we align first text run according to pixel snapping rule)
		*/
		sRectMargin			fMargin;

		/** children positions - relative to container layout top left origin and to computing dpi */
		VectorOfPoint		fChildOrigin;

		//// layout metrics (in rendering dpi)

		/** max width in px (relative to layout target dpi) including margins */
		GReal				fRenderMaxWidth;		

		/** max height in px (relative to layout target dpi) including margins */
		GReal				fRenderMaxHeight;		

		/** layout bounds (alias for design bounds) in px - relative to fRenderDPI - including margin, padding & border */
		VRect				fRenderLayoutBounds;

		/** typographic bounds in px - relative to rendering dpi - including margin, padding & border 
		@remarks
			formatted text bounds (minimal bounds to display all text with margin, padding & border)
		*/
		VRect				fRenderTypoBounds;

		/** margins in px - relative to rendering dpi 
		@remarks
			same as fAllMargin but rounded according to gc pixel snapping (as we align first text run according to pixel snapping rule)
		*/
		sRectMargin			fRenderMargin;

		//// graphic context management

		GraphicContextType	fGCType;
		VGraphicContext*	fGC;
		sLONG				fGCUseCount;

		TextRenderingMode   fBackupTRM;
		VGraphicContext*	fBackupParentGC;
		bool				fNeedRestoreTRM;
		bool				fIsPrinting;
		GReal				fLayoutKerning; //kerning used to compute current layout if layout is not dirty
		TextRenderingMode	fLayoutTextRenderingMode; //text rendering mode used to compute current layout if layout is not dirty

		//// text replacement management

		/** current child layout used by text replacement through replacement delegates 
		@remarks
			we need to map/unmap text indexs according to child local range & current local range

			if NULL, text indexs are relative to local
		*/  
		VDocBaseLayout*						fReplaceTextCurLayout;
		void*								fReplaceTextUserData;
		VTreeTextStyle::fnBeforeReplace		fReplaceTextBefore;
		VTreeTextStyle::fnAfterReplace		fReplaceTextAfter;

		/** dirty status */
		bool				fIsDirty;
};


/** VDocTextLayout

@remarks
	this class is dedicated to manage text document layout
*/
class XTOOLBOX_API VDocTextLayout: public VDocContainerLayout
{
public:
							/**	VDocTextLayout constructor
							@param inDocText
								document text node
								if NULL, initialize default text layout
							@param inShouldEnableCache
								enable/disable cache (default is true): see ShouldEnableCache
							*/
							VDocTextLayout(const VDocText *inDocText = NULL, bool inShouldEnableCache = true);

virtual						~VDocTextLayout() {}

/////////////////	IDocBaseLayout override

		/** return document node type */
virtual	eDocNodeType		GetDocType() const { return kDOC_NODE_TYPE_DOCUMENT; }

		/** retain layout node which matches the passed ID (will return always NULL if node is not attached to a VDocTextLayout) */
virtual	VDocBaseLayout*		RetainLayout(const VString& inID);

		/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

		/** remove class style */
virtual bool				RemoveClassStyle( const VString& inClass, eDocNodeType inDocNodeType = kDOC_NODE_TYPE_PARAGRAPH);

		/** clear class styles */
virtual	bool				ClearClassStyles( bool inKeepDefaultStyles);

		/** accessor on class style manager */
virtual	VDocClassStyleManager*	GetClassStyleManager() const { return fClassStyleManager.Get(); }


/////////////////	ITextLayout override

		/** create and retain document from the current layout  
		@param inStart
			range start
		@param inEnd
			range end

		@remarks
			if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
			with a single child node initialized from the current layout
		*/
virtual	VDocText*			CreateDocument(sLONG inStart = 0, sLONG inEnd = -1);

		/** init layout from the passed document 
		@param inDocument
			document source
		@remarks
			the passed document is used only to initialize layout: document is not retained
			//TODO: maybe we should optionally bind layout with the document class instance
		*/
virtual	void				InitFromDocument( const VDocText *inDocument);

		/** replace layout range with the specified document fragment */
virtual	void				ReplaceDocument(const VDocText *inDoc, sLONG inStart = 0, sLONG inEnd = -1,
											bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
											void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

		/** attach or detach a layout user delegate */
virtual	void				SetLayoutUserDelegate( IDocNodeLayoutUserDelegate *inDelegate, void *inUserData);
virtual	IDocNodeLayoutUserDelegate* GetLayoutUserDelegate() const { return fLayoutUserDelegate; }

protected:
friend class VDocBaseLayout;
friend class VDocParagraphLayout;
friend class VDocContainerLayout;

typedef	std::map<VString, VRefPtr<VDocBaseLayout> > MapOfLayoutPerID;

		uLONG				_AllocID() { return fNextIDCount++; }

		void				_ClearUnreferenced();

		/** document next ID number 
		@remarks
			it is impossible for one document to have more than kMAX_uLONG nodes
			so incrementing ID is enough to ensure ID is unique
			(if a node is added to a parent node, ID is checked & generated again if node ID is used yet)
		*/ 
		uLONG				fNextIDCount;


mutable	MapOfLayoutPerID	fMapOfLayoutPerID;


		VRefPtr<VDocClassStyleManager>	fClassStyleManager;

		IDocNodeLayoutUserDelegate*		fLayoutUserDelegate;
		void*							fLayoutUserDelegateData;
};

#endif

END_TOOLBOX_NAMESPACE

#endif
