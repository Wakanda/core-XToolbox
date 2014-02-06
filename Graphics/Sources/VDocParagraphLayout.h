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
#ifndef __VParagraphLayout__
#define __VParagraphLayout__

BEGIN_TOOLBOX_NAMESPACE

/** default selection color (Word-like color) */
#define VTEXTLAYOUT_SELECTION_ACTIVE_COLOR		XBOX::VColor(81,155,227,128)
#define VTEXTLAYOUT_SELECTION_INACTIVE_COLOR	XBOX::VColor(200,200,200,128)

/** placeholder color */
#define VTEXTLAYOUT_COLOR_PLACEHOLDER			XBOX::VColor(177,169,187)

/** common interface shared by classes VDocParagraphLayout, VDocTextLayout & VTextLayout (VTextLayout is obsolete but still supported for v14 compatibility) 

@remarks
	text indexs parameters are always relative to the current layout instance (GetTextStart() returns the text offset relative to parent):
	so to convert a text index from the local layout to the parent layout, add GetTextStart()
	and to convert a text index from the parent layout to the local layout, subtract GetTextStart()

	if the method scope is paragraph layout instance(s) (like SetParaTextAlign, SetParaMargins, SetLineHeight, etc...) and if current instance is a paragraph layout container:
	Set... method is applied to all children VDocParagraphLayout instances which intersect the passed text range and
	Get... method is applied to the first child VDocParagraphLayout instance which intersect the passed text range
	(paragraph only methods are applied to the current layout instance if it is a VDocParagraphLayout instance yet);
	exceptions are explicitly specified in method commentary
*/ 

class XTOOLBOX_API ITextLayout: public IDocBaseLayout
{
public:

virtual	bool				IsImplDocLayout() const { return true; } //false if class is VTextLayout (obsolete native implementation)

/////////////////	IDocNodeLayout interface

		/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&		GetLayoutBounds() const = 0;

		/** get typo local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&		GetTypoBounds() const { return GetLayoutBounds(); }

/////////////////	IDocBaseLayout interface 

		/** query text layout interface (if available for this class instance) 
		@remarks
			for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
			might return NULL
		*/ 
virtual	ITextLayout*		QueryTextLayoutInterface() { return this; }

		/** query text layout interface (if available for this class instance) 
		@remarks
			for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
			might return NULL
		*/ 
virtual	const ITextLayout*	QueryTextLayoutInterfaceConst() const { return this; }

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true) = 0;
        /** get outside margins (in twips) */
virtual	void				GetMargins( sDocPropRect& outMargins) const = 0;

////////////////

virtual	bool				IsDirty() const = 0;
		/** set dirty */
virtual	void				SetDirty(bool inApplyToChildren = false, bool inApplyToParent = true) = 0;

virtual	void				SetPlaceHolder( const VString& inPlaceHolder)	{}
virtual	void				GetPlaceHolder( VString& outPlaceHolder) const { outPlaceHolder.SetEmpty(); }

		/** enable/disable password mode */
virtual	void				EnablePassword(bool inEnable) {}
virtual bool				IsEnabledPassword() const { return false; }

		/** enable/disable cache */
virtual	void				ShouldEnableCache( bool inShouldEnableCache) = 0;
virtual	bool				ShouldEnableCache() const = 0;


		/** allow/disallow offscreen text rendering (default is false)
		@remarks
			text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
			you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
		@see
			ShouldEnableCache
		*/
virtual	void				ShouldDrawOnLayer( bool inShouldDrawOnLayer) = 0;
virtual	bool				ShouldDrawOnLayer() const = 0;
virtual void				SetLayerDirty(bool inApplyToChildren = false) = 0;

		/** paint selection */
virtual	void				ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor = NULL, const VColor* inInactiveColor = NULL) = 0;
virtual	bool				ShouldPaintSelection() const = 0;
		/** change selection (selection is used only for painting) */
virtual	void				ChangeSelection( sLONG inStart, sLONG inEnd = -1, bool inActive = true) = 0;
virtual	bool				IsSelectionActive() const = 0;

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

		/** create and retain paragraph from the current layout plain text, styles & paragraph properties (only first paragraph properties are copied)
		@param inStart
			range start
		@param inEnd
			range end
		*/
virtual	VDocParagraph*		CreateParagraph(sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** apply paragraph properties & styles if any to the current layout (passed paragraph text is ignored) 
		@param inPara
			the paragraph style
		@param inResetStyles
			if true, reset styles to default prior to apply new paragraph style
		@remarks
			such paragraph is just used for styling:
			so note that only paragraph first level VTreeTextStyle styles are applied (to the layout full plain text: paragraph style range is ignored)
		*/
virtual	void				ApplyParagraphStyle( const VDocParagraph *inPara, bool inResetStyles = false, bool inApplyParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** init layout from the passed document 
		@param inDocument
			document source
		@remarks
			the passed document is used only to initialize layout: document is not retained
			//TODO: maybe we should optionally bind layout with the document class instance
		*/
virtual	void				InitFromDocument( const VDocText *inDocument) { xbox_assert(false); }
	
		/** init layout from the passed paragraph 
		@param inPara
			paragraph source
		@remarks
			the passed paragraph is used only to initialize layout: paragraph is not retained
		*/
virtual	void				InitFromParagraph( const VDocParagraph *inPara) = 0;

		/** replace layout range with the specified document fragment */
virtual	void				ReplaceDocument(const VDocText *inDoc, sLONG inStart = 0, sLONG inEnd = -1, 
											bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
											void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL) { xbox_assert(false); }

		/** reset all text properties & character styles to default values */
virtual	void				ResetParagraphStyle(bool inResetParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1) = 0;


		/** merge paragraph(s) on the specified range to a single paragraph 
		@remarks
			if there are more than one paragraph on the range, plain text & styles from the paragraphs but the first one are concatenated with first paragraph plain text & styles
			resulting with a single paragraph on the range (using paragraph styles from the first paragraph) where plain text might contain more than one CR

			remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
			you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
		*/
virtual	bool				MergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1) { return false; }

		/** unmerge paragraph(s) on the specified range 
		@remarks
			any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR, 
			resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)

			remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
			you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
		*/
virtual	bool				UnMergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1) { return false; }


virtual	bool				HasSpanRefs() const = 0;

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
													void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL) = 0;

		/** return span reference at the specified text position if any */
virtual	const VTextStyle*	GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart = NULL) const = 0;

		/** return the first span reference which intersects the passed range */
virtual	const VTextStyle*	GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outBaseStart = NULL) const = 0;

		/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
virtual	bool				UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL) = 0;

		/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
virtual	bool				FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL) = 0;

		/** eval 4D expressions  
		@remarks
			note that it only updates span 4D expression references but does not update plain text:
			call UpdateSpanRefs after to update layout plain text

			return true if any 4D expression has been evaluated
		*/
virtual	bool				EvalExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL) = 0;


		/** convert uniform character index (the index used by 4D commands) to actual plain text position 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				CharPosFromUniform( sLONG& ioPos) const = 0;

		/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				CharPosToUniform( sLONG& ioPos) const = 0;

		/** convert uniform character range (as used by 4D commands) to actual plain text position 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				RangeFromUniform( sLONG& inStart, sLONG& inEnd) const = 0;

		/** convert actual plain text range to uniform range (as used by 4D commands) 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
virtual	void				RangeToUniform( sLONG& inStart, sLONG& inEnd) const = 0;

		/** adjust selection 
		@remarks
			you should call this method to adjust with surrogate pairs or span references
		*/
virtual	void				AdjustRange(sLONG& ioStart, sLONG& ioEnd) const = 0;

		/** adjust char pos
		@remarks
			you should call this method to adjust with surrogate pairs or span references
		*/
virtual	void				AdjustPos(sLONG& ioPos, bool inToEnd = true) const = 0;


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
virtual	void				SetLayerViewRect( const VRect& inRectRelativeToWnd) = 0;
virtual	const VRect&		GetLayerViewRect() const = 0;

		/** insert text at the specified position */
virtual	void				InsertPlainText( sLONG inPos, const VString& inText) = 0;

		/** delete text range */
virtual	void				DeletePlainText( sLONG inStart, sLONG inEnd) = 0;

		/** replace text */
virtual	void				ReplacePlainText( sLONG inStart, sLONG inEnd, const VString& inText) = 0;

		/** apply style (use style range) */
virtual	bool				ApplyStyle( const VTextStyle* inStyle, bool inFast = false, bool inIgnoreIfEmpty = true) = 0;

virtual	bool				ApplyStyles( const VTreeTextStyle * inStyles, bool inFast = false) = 0;

		/** show span references source or value */ 
virtual	void				ShowRefs( SpanRefCombinedTextTypeMask inShowRefs, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL) = 0;
virtual	SpanRefCombinedTextTypeMask ShowRefs() const = 0;

		/** highlight span references in plain text or not */
virtual	void				HighlightRefs(bool inHighlight, bool inUpdateAlways = false) = 0;
virtual	bool				HighlightRefs() const = 0;

		/** call this method to update span reference style while mouse enters span ref 
		@remarks
			return true if style has changed
		*/
virtual	bool				NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0) = 0;

		/** call this method to update span reference style while mouse leaves span ref 
		@remarks
			return true if style has changed
		*/
virtual	bool				NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0) = 0;

		/** set default font 
		@remarks
			it should be not NULL - STDF_TEXT standard font on default (it is applied prior styles set with SetText)
			
			by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
		*/
virtual	void				SetDefaultFont( const VFont *inFont, GReal inDPI = 72.0f, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** get & retain default font 
		@remarks
			by default, return 4D form-compliant font (with dpi = 72) so not the actual fRenderDPI-scaled font 
			(for consistency with SetDefaultFont & because fRenderDPI internal scaling should be transparent for caller)
		*/
virtual	VFont*				RetainDefaultFont(GReal inDPI = 72.0f, sLONG inStart = 0) const = 0;

		/** set default text color 
		@remarks
			black color on default (it is applied prior styles set with SetText)
		*/
virtual	void				SetDefaultTextColor( const VColor &inTextColor, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** get default text color */
virtual	bool				GetDefaultTextColor(VColor& outColor, sLONG inStart = 0) const = 0;

		/** set back color 
		@param  inColor
			background color
		@param	inPaintBackColor
			unused (preserved for compatibility with VTextLayout)
		@remarks
			OBSOLETE: you should use SetBackgroundColor - it is preserved for compatibility with VTextLayout
		*/
virtual	void				SetBackColor( const VColor& inColor, bool inPaintBackColor = false) = 0;
virtual	const VColor&		GetBackColor() const = 0;
		
		/** replace current text & styles 
		@remarks
			if inCopyStyles == true, styles are copied
			if inCopyStyles == false (default), styles MIGHT be retained: in that case, caller should not modify passed styles

			this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
		*/
virtual	void				SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false) = 0;

		/** replace current text & styles
		@remarks	
			here passed styles are always copied

			this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
		*/
virtual	void				SetText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd = -1, 
									 bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
									 const VDocSpanTextRef * inPreserveSpanTextRef = NULL, 
									 void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL)
		{
			VTreeTextStyle *styles = new VTreeTextStyle( inStyles);
			SetText( inText, styles, false);
			ReleaseRefCountable(&styles);
		}

		/** const accessor on paragraph text 
		@remarks
			if it is a container, return first paragraph text
		*/ 
virtual	const VString&		GetText() const = 0;

virtual	void				GetText(VString& outText, sLONG inStart = 0, sLONG inEnd = -1) const { outText = GetText(); }


		/** return unicode character at the specified position (0-based) */
virtual	UniChar				GetUniChar( sLONG inIndex) const = 0;

		/** return styles on the passed range 
		@remarks
			if inIncludeDefaultStyle == false (default is true) and if layout is a paragraph, it does not include paragraph uniform style (default style) but only styles which are not uniform on paragraph text:
			otherwise it returns all styles applied to the plain text

			caution: if inCallerOwnItAlways == false (default is true), returned styles might be shared with layout instance so you should not modify it
		*/
virtual	VTreeTextStyle*		RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inIncludeDefaultStyle = true, bool inCallerOwnItAlways = true) const = 0;

		/** set extra text styles 
		@remarks
			it can be used to add a temporary style effect to the text layout without modifying the main text styles
			(these styles are not exported to VDocParagraph by CreateParagraph)

			if inCopyStyles == true, styles are copied
			if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 

			TODO: should be obsolete for VDocTextLayout because for VDocTextLayout extended styles are used only to render automatic urls style:
				  it would be better in that case to manage automatic urls as volatile url span references directly in VDocParagraphLayout class
				  (for visible urls only)
		*/
virtual	void				SetExtStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false) = 0;

virtual	VTreeTextStyle*		RetainExtStyles() const = 0;

		/** set max text width (0 for infinite) - in pixel relative to the layout target DPI
		@remarks
			it is also equal to text box width 
			so you should specify one if text is not aligned to the left border
			otherwise text layout box width will be always equal to formatted text width
		*/ 
virtual	void				SetMaxWidth(GReal inMaxWidth = 0.0f) = 0;
virtual	GReal				GetMaxWidth() const = 0;

		/** set max text height (0 for infinite) - in pixel relative to the layout target DPI
		@remarks
			it is also equal to text box height 
			so you should specify one if paragraph is not aligned to the top border
			otherwise text layout box height will be always equal to formatted text height
		*/ 
virtual	void				SetMaxHeight(GReal inMaxHeight = 0.0f) = 0;
virtual	GReal				GetMaxHeight() const = 0;

		/** replace ouside margins	(in point) */
virtual	void				SetMargins(const GReal inMarginLeft, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f)
{
	//to avoid math discrepancies from res-independent point to current DPI, we always store in TWIPS & convert from TWIPS

	sDocPropRect margin;
	margin.left			= ICSSUtil::PointToTWIPS( inMarginLeft);
	margin.right		= ICSSUtil::PointToTWIPS( inMarginRight);
	margin.top			= ICSSUtil::PointToTWIPS( inMarginTop);
	margin.bottom		= ICSSUtil::PointToTWIPS( inMarginBottom);
	SetMargins( margin);
}

		/** get outside margins (in point) */
virtual	void				GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom) const
{
	sDocPropRect margin;
	GetMargins( margin);
	if (outMarginLeft)
		*outMarginLeft = ICSSUtil::TWIPSToPoint( margin.left);
	if (outMarginRight)
		*outMarginRight = ICSSUtil::TWIPSToPoint( margin.right);
	if (outMarginTop)
		*outMarginTop = ICSSUtil::TWIPSToPoint( margin.top);
	if (outMarginBottom)
		*outMarginBottom = ICSSUtil::TWIPSToPoint( margin.bottom);
}

		/** get paragraph line height 

			if positive, it is fixed line height in point
			if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
			if -1, it is normal line height 
		*/
virtual	GReal				GetLineHeight(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

		/** set paragraph line height 

			if positive, it is fixed line height in point
			if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
			if -1, it is normal line height 
		*/
virtual	void				SetLineHeight( const GReal inLineHeight, sLONG inStart = 0, sLONG inEnd = -1) = 0;

virtual	bool				IsRTL(sLONG inStart = 0, sLONG inEnd = -1) const = 0;
virtual	void				SetRTL(bool inRTL, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** get first line padding offset in point
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
virtual	GReal				GetTextIndent(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

		/** set first line padding offset in point 
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
virtual	void				SetTextIndent(const GReal inPadding, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** get tab stop offset in point */
virtual	GReal				GetTabStopOffset(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

		/** set tab stop (offset in point) */
virtual	void				SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** get tab stop type */ 
virtual	eTextTabStopType	GetTabStopType(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

		/** set text layout mode (default is TLM_NORMAL) */
virtual	void				SetLayoutMode( TextLayoutMode inLayoutMode = TLM_NORMAL, sLONG inStart = 0, sLONG inEnd = -1) = 0;
		/** get text layout mode */
virtual	TextLayoutMode		GetLayoutMode(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetParaMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMargins( inMargins, inCalcAllMargin);
		}

        /** get outside margins (in twips) */
virtual	void				GetParaMargins( sDocPropRect& outMargins, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			GetMargins( outMargins);
		}

		/** replace ouside margins	(in point) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaMargins(const GReal inMarginLeft, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f, sLONG inStart = 0, sLONG inEnd = -1)
{
	//to avoid math discrepancies from res-independent point to current DPI, we always store in TWIPS & convert from TWIPS

	sDocPropRect margin;
	margin.left			= ICSSUtil::PointToTWIPS( inMarginLeft);
	margin.right		= ICSSUtil::PointToTWIPS( inMarginRight);
	margin.top			= ICSSUtil::PointToTWIPS( inMarginTop);
	margin.bottom		= ICSSUtil::PointToTWIPS( inMarginBottom);
	SetParaMargins( margin, true, inStart, inEnd);
}

		/** get outside margins (in point) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				GetParaMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom, sLONG inStart = 0, sLONG inEnd = -1) const	
{
	sDocPropRect margin;
	GetParaMargins( margin, inStart, inEnd);
	if (outMarginLeft)
		*outMarginLeft = ICSSUtil::TWIPSToPoint( margin.left);
	if (outMarginRight)
		*outMarginRight = ICSSUtil::TWIPSToPoint( margin.right);
	if (outMarginTop)
		*outMarginTop = ICSSUtil::TWIPSToPoint( margin.top);
	if (outMarginBottom)
		*outMarginBottom = ICSSUtil::TWIPSToPoint( margin.bottom);
}

		/** set inside margins	(in twips) - CSS padding - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaPadding(const sDocPropRect& inPadding, bool inCalcAllMargin = true, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetPadding( inPadding, inCalcAllMargin);
		}

		/** get inside margins	(in twips) - CSS padding - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				GetParaPadding(sDocPropRect& outPadding, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			GetPadding(outPadding);
		}

		/** set text align - CSS text-align (-1 for not defined) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaTextAlign( const AlignStyle inHAlign, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetTextAlign( inHAlign);
		}

virtual AlignStyle			GetParaTextAlign(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetTextAlign();
		}

		/** set vertical align - CSS vertical-align (AL_DEFAULT for default) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaVerticalAlign( const AlignStyle inVAlign, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetVerticalAlign( inVAlign);
		}
virtual AlignStyle			GetParaVerticalAlign(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetVerticalAlign();
		}

		/** set width (in twips) - CSS width (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaWidth(const uLONG inWidth, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetWidth(inWidth);
		}
virtual uLONG				GetParaWidth(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetWidth(inIncludeAllMargins);
		}

		/** set height (in twips) - CSS height (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaHeight(const uLONG inHeight, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetHeight(inHeight);
		}
virtual uLONG				GetParaHeight(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetHeight(inIncludeAllMargins);
		}

		/** set min width (in twips) - CSS min-width (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaMinWidth(const uLONG inWidth, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMinWidth(inWidth);
		}
virtual uLONG				GetParaMinWidth(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetMinWidth(inIncludeAllMargins);
		}

		/** set min height (in twips) - CSS min-height (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaMinHeight(const uLONG inHeight, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMinHeight(inHeight);
		}
virtual uLONG				GetParaMinHeight(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetMinHeight(inIncludeAllMargins);
		}

		/** set background color - CSS background-color (transparent for not defined) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaBackgroundColor( const VColor& inColor, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundColor( inColor);
		}
virtual const VColor&		GetParaBackgroundColor(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetBackgroundColor();
		}

		/** set background clipping - CSS background-clip - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaBackgroundClip( const eDocPropBackgroundBox inBackgroundClip, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundClip( inBackgroundClip);
		}
virtual eDocPropBackgroundBox	GetParaBackgroundClip(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetBackgroundClip();
		}

		/** set background origin - CSS background-origin - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundOrigin( inBackgroundOrigin);
		}
virtual eDocPropBackgroundBox	GetParaBackgroundOrigin(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetBackgroundOrigin();
		}

		/** set borders - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance 
		@remarks
			if inLeft is NULL, left border is same as right border 
			if inBottom is NULL, bottom border is same as top border
			if inRight is NULL, right border is same as top border
			if inTop is NULL, top border is default border (solid, medium width & black color)
		*/
virtual	void				SetParaBorders( const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBorders( inTop, inRight, inBottom, inLeft);
		}

		/** clear borders */
virtual	void				ClearParaBorders(sLONG inStart = 0, sLONG inEnd = -1)
		{
			ClearBorders();
		}

		/** set background image - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance 
		@remarks
			see VDocBackgroundImageLayout constructor for parameters description */
virtual	void				SetParaBackgroundImage(	const VString& inURL,	
												const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
												const eDocPropBackgroundRepeat inRepeat = kDOC_BACKGROUND_REPEAT,
												const GReal inWidth = kDOC_BACKGROUND_SIZE_AUTO, const GReal inHeight = kDOC_BACKGROUND_SIZE_AUTO,
												const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false,
												sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundImage(	inURL,	
								inHAlign, inVAlign, 
								inRepeat,
								inWidth, inHeight,
								inWidthIsPercentage, inHeightIsPercentage);
		}

		/** clear background image */
virtual	void				ClearParaBackgroundImage(sLONG inStart = 0, sLONG inEnd = -1)
		{
			ClearBackgroundImage();
		}

		/** begin using text layout for the specified gc (reentrant)
		@remarks
			for best performance, it is recommended to call this method prior to more than one call to Draw or metrics get methods & terminate with EndUsingContext
			otherwise Draw or metrics get methods will check again if layout needs to be updated at each call to Draw or metric get method
			But you do not need to call BeginUsingContext & EndUsingContext if you call only once Draw or a metrics get method.

			CAUTION: if you call this method, you must then pass the same gc to Draw or metrics get methods before EndUsingContext is called
					 otherwise Draw or metrics get methods will do nothing (& a assert failure is raised in debug)

			If you do not call Draw method, you should pass inNoDraw = true to avoid to create a drawing context on some impls (actually for now only Direct2D impl uses this flag)
		*/
virtual	void				BeginUsingContext( VGraphicContext *inGC, bool inNoDraw = false) = 0;

		/** return text layout bounds including margins (in gc user space)
		@remarks
			text layout origin is set to inTopLeft
			on output, outBounds contains text layout bounds including any glyph overhangs; outBounds left top origin should be equal to inTopLeft

			on input, max width is determined by GetMaxWidth() & max height by GetMaxHeight()
			(if max width or max height is equal to 0.0f, it is assumed to be infinite & so layout bounds will be equal to formatted text bounds)
			output layout width will be equal to max width if maw width != 0 otherwise to formatted text width
			output layout height will be equal to max height if maw height != 0 otherwise to formatted text height
			(see CAUTION remark below)

			if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
			(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
			 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)

			CAUTION: layout bounds can be greater than formatted text bounds (if max width or max height is not equal to 0)
					 because layout bounds are equal to the text box design bounds and not to the formatted text bounds: 
					 you should use GetTypoBounds in order to get formatted text width & height
		*/
virtual	void				GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false) = 0;

		/** return formatted text bounds (typo bounds) including margins (in gc user space)
		@remarks
			text typo bounds origin is set to inTopLeft

			as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
			because layout bounds are equal to the text box design bounds and not to the formatted text bounds
			you should use this method to get formatted text width & height 
		*/
virtual	void				GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false) = 0;

		/** return text layout run bounds from the specified range (in gc user space)
		@remarks
			text layout origin is set to inTopLeft
		*/
virtual	void				GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/** return the caret position & height of text at the specified text position in the text layout (in gc user space)
		@remarks
			text layout origin is set to inTopLeft

			text position is 0-based

			caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
			if text layout is drawed at inTopLeft

			by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
			but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
		*/
virtual	void				GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true) = 0;

		/** return the text position at the specified coordinates
		@remarks
			text layout origin is set to inTopLeft (in gc user space)
			the hit coordinates are defined by inPos param (in gc user space)

			output text position is 0-based

			return true if input position is inside character bounds
			if method returns false, it returns the closest character index to the input position
		*/
virtual	bool				GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength = NULL) = 0;

		/** get character range which intersects the passed bounds (in gc user space) 
		@remarks
			text layout origin is set to inTopLeft (in gc user space)

			char range is rounded to line run limit so the returned range might be slightly greater than the actual char range that intersects the passed bounds
		*/
virtual	void				GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd) = 0;


		/** move character index to the nearest character index on the line before/above the character line 
		@remarks
			return false if character is on the first line (ioCharIndex is not modified)
		*/
virtual	bool				MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex) = 0;

		/** move character index to the nearest character index on the line after/below the character line
		@remarks
			return false if character is on the last line (ioCharIndex is not modified)
		*/
virtual	bool				MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex) = 0;
		
		/** return true if layout uses truetype font only */
virtual	bool				UseFontTrueTypeOnly(VGraphicContext *inGC) const = 0;

		/** end using text layout (reentrant) */
virtual	void				EndUsingContext() = 0;

virtual	VTreeTextStyle*		RetainDefaultStyle(sLONG inStart = 0, sLONG inEnd = -1) const = 0;

		/** consolidate end of lines
		@remarks
			if text is multiline:
				we convert end of lines to a single CR 
			if text is singleline:
				if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
				if inEscaping, we escape line endings and tab 
		*/
virtual	bool				ConsolidateEndOfLines(bool inIsMultiLine = true, bool inEscaping = false, sLONG inStart = 0, sLONG inEnd = -1) = 0;

		/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) range at the specified character index */
virtual	void				GetWordRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces = true, bool inTrimPunctuation = false);
		
		/**	get paragraph range at the specified character index 
		@remarks
			if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
			(for VTextLayout, parameter is ignored)
		*/
virtual	void				GetParagraphRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange = false);

		/////// helper methods


		/** consolidate end of lines
		@remarks
			if text is multiline:
				we convert end of lines to a single CR 
			if text is singleline:
				if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
				if inEscaping, we escape line endings and tab 
		*/
static	bool				ConsolidateEndOfLines(const VString &inText, VString &outText, VTreeTextStyle *ioStyles, bool inIsMultiLine = true, bool inEscaping = false);
	
static	void				AdjustWithSurrogatePairs( sLONG& ioStart, sLONG &ioEnd, const VString& inText);
static	void				AdjustWithSurrogatePairs( sLONG& ioPos, const VString& inText, bool inToEnd = true);

		/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) or paragraph range (if inGetParagraph == true) at the specified character index */
static	void				GetWordOrParagraphRange( VIntlMgr *inIntlMgr, const VString& inText, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inGetParagraph = false, bool inTrimSpaces = true, bool inTrimPunctuation = false);
};


/** paragraph layout
@remarks
	this class is dedicated to manage platform-independent (ICU & platform-independent algorithm) paragraph layout
	only layout basic runs (same direction & style) are still computed & rendered with native impl - GDI (legacy layout mode) or DWrite on Windows, CoreText on Mac

	text might be made of a single paragraph or multiple paragraphs (but with common paragraph style - suitable to render for instance a single HTML paragraph)
	(if you need to render contiguous paragraphs with variable paragraph style, you should use VDocContainerLayout or any class derived from VDocContainerLayout)

	 IMPLEMENTED:
			- paragraph layout computing & rendering
			- word-wrap (ICU-based line-breaking)
			- margins
			- text indent
			- absolute tabs
			- direction (left to right, right to left) - ICU-based bidirectional 
			- text-align: left, right, center & justify
			- vertical-align: top, center & bottom (& baseline, superscript & subscript for embedded images)
			- borders
			- hit testing 
			- embedded image(s)
			- background image (repeated or not)
			- background clipping
	 TODO:  
			- bullets
			- tables
			- etc...
*/
class XTOOLBOX_API VDocParagraphLayout: public VDocBaseLayout, public ITextLayout
{
public:
							VDocParagraphLayout(bool inShouldEnableCache = true); 

							/**	VDocParagraphLayout constructor
							@param inPara
								VDocParagraph source: the passed paragraph is used only for initializing layout plain text, styles & paragraph properties
													  but inPara is not retained 
								(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
							@param inShouldEnableCache
								enable/disable cache (default is true): see ShouldEnableCache
							*/
							VDocParagraphLayout(const VDocParagraph *inPara, bool inShouldEnableCache = true);

virtual						~VDocParagraphLayout();

/////////////////	IDocSpanTextRefLayout interface

virtual	void				ReleaseRef() { IRefCountable::Release(); }

/////////////////	IDocNodeLayout interface

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

/////////////////	IDocBaseLayout override (as both VDocBaseLayout & ITextLayout derive from IDocBaseLayout we need to override all IDocBaseLayout methods here to prevent ambiguity)

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
virtual	eDocNodeType		GetDocType() const { return kDOC_NODE_TYPE_PARAGRAPH; }

		/** get text start index 
		@remarks
			index is relative to parent layout text start index
			(if this layout has not a parent, it should be always 0)
		*/
virtual	sLONG				GetTextStart() const { return VDocBaseLayout::GetTextStart(); }

		/** return local text length 
		@remarks
			it should be at least 1 no matter the kind of layout (any layout object should have at least 1 character size)
		*/
virtual	uLONG				GetTextLength() const { return fText.GetLength(); }

		/** get parent layout reference 
		@remarks
			for instance for a VDocParagraphLayout instance, it might be a VDocContainerLayout instance (paragraph layout container)
			(but as VDocParagraphLayout instance can be used stand-alone, it might be NULL too)
		*/
virtual IDocBaseLayout*		GetParent() { return VDocBaseLayout::GetParent(); }

		/** get count of child layout instances */
virtual	uLONG				GetChildCount() const { return 0; }
	
		/** get layout child index (relative to parent child vector & 0-based) */
virtual sLONG				GetChildIndex() const { return VDocBaseLayout::GetChildIndex(); }

		/** add child layout instance */
virtual void				AddChild( IDocBaseLayout* inLayout) { xbox_assert(false); }

		/** insert child layout instance (index is 0-based) */
virtual void				InsertChild( IDocBaseLayout* inLayout, sLONG inIndex = 0) { xbox_assert(false); }

		/** remove child layout instance (index is 0-based) */
virtual void				RemoveChild( sLONG inIndex) { xbox_assert(false); }

		/** get child layout instance from index (0-based) */
virtual	IDocBaseLayout*		GetNthChild(sLONG inIndex) const { return NULL; }

		/** get first child layout instance which intersects the passed text index */
virtual IDocBaseLayout*		GetFirstChild( sLONG inStart = 0) const { return NULL; }

virtual	bool				IsLastChild(bool inLookUp = false) const { return VDocBaseLayout::IsLastChild( inLookUp); }

		/** initialize from the passed node */
virtual	void				InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited = false);

		/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

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
virtual AlignStyle			GetTextAlign() const { return VDocBaseLayout::GetTextAlign(); }

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

		//paragraph-scope properties (overriden only to avoid ambiguity as this layout is a paragraph yet)

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetParaMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMargins( inMargins, inCalcAllMargin);
		}

        /** get outside margins (in twips) */
virtual	void				GetParaMargins( sDocPropRect& outMargins, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			GetMargins( outMargins);
		}

		/** replace ouside margins	(in point) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaMargins(const GReal inMarginLeft, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMargins( inMarginLeft, inMarginRight, inMarginTop, inMarginBottom);
		}
		/** get outside margins (in point) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				GetParaMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom, sLONG inStart = 0, sLONG inEnd = -1) const	
		{
			GetMargins( outMarginLeft, outMarginRight, outMarginTop, outMarginBottom);
		}

		/** set inside margins	(in twips) - CSS padding - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaPadding(const sDocPropRect& inPadding, bool inCalcAllMargin = true, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetPadding( inPadding, inCalcAllMargin);
		}

		/** get inside margins	(in twips) - CSS padding - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				GetParaPadding(sDocPropRect& outPadding, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			GetPadding(outPadding);
		}

		/** set text align - CSS text-align (-1 for not defined) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaTextAlign( const AlignStyle inHAlign, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetTextAlign( inHAlign);
		}

virtual AlignStyle			GetParaTextAlign(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetTextAlign();
		}

		/** set vertical align - CSS vertical-align (AL_DEFAULT for default) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaVerticalAlign( const AlignStyle inVAlign, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetVerticalAlign( inVAlign);
		}
virtual AlignStyle			GetParaVerticalAlign(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetVerticalAlign();
		}


		/** set width (in twips) - CSS width (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaWidth(const uLONG inWidth, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetWidth(inWidth);
		}
virtual uLONG				GetParaWidth(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetWidth(inIncludeAllMargins);
		}

		/** set height (in twips) - CSS height (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaHeight(const uLONG inHeight, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetHeight(inHeight);
		}
virtual uLONG				GetParaHeight(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetHeight(inIncludeAllMargins);
		}

		/** set min width (in twips) - CSS min-width (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaMinWidth(const uLONG inWidth, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMinWidth(inWidth);
		}
virtual uLONG				GetParaMinWidth(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetMinWidth(inIncludeAllMargins);
		}

		/** set min height (in twips) - CSS min-height (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual	void				SetParaMinHeight(const uLONG inHeight, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetMinHeight(inHeight);
		}
virtual uLONG				GetParaMinHeight(bool inIncludeAllMargins = false, sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetMinHeight(inIncludeAllMargins);
		}

		/** set background color - CSS background-color (transparent for not defined) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaBackgroundColor( const VColor& inColor, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundColor( inColor);
		}
virtual const VColor&		GetParaBackgroundColor(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetBackgroundColor();
		}

		/** set background clipping - CSS background-clip - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaBackgroundClip( const eDocPropBackgroundBox inBackgroundClip, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundClip( inBackgroundClip);
		}
virtual eDocPropBackgroundBox	GetParaBackgroundClip(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetBackgroundClip();
		}

		/** set background origin - CSS background-origin - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
virtual void				SetParaBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundOrigin( inBackgroundOrigin);
		}
virtual eDocPropBackgroundBox	GetParaBackgroundOrigin(sLONG inStart = 0, sLONG inEnd = -1) const
		{
			return GetBackgroundOrigin();
		}

		/** set borders - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance 
		@remarks
			if inLeft is NULL, left border is same as right border 
			if inBottom is NULL, bottom border is same as top border
			if inRight is NULL, right border is same as top border
			if inTop is NULL, top border is default border (solid, medium width & black color)
		*/
virtual	void				SetParaBorders( const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBorders( inTop, inRight, inBottom, inLeft);
		}

		/** clear borders */
virtual	void				ClearParaBorders(sLONG inStart = 0, sLONG inEnd = -1)
		{
			ClearBorders();
		}

		/** set background image - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance 
		@remarks
			see VDocBackgroundImageLayout constructor for parameters description */
virtual	void				SetParaBackgroundImage(	const VString& inURL,	
												const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
												const eDocPropBackgroundRepeat inRepeat = kDOC_BACKGROUND_REPEAT,
												const GReal inWidth = kDOC_BACKGROUND_SIZE_AUTO, const GReal inHeight = kDOC_BACKGROUND_SIZE_AUTO,
												const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false,
												sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetBackgroundImage(	inURL,	
								inHAlign, inVAlign, 
								inRepeat,
								inWidth, inHeight,
								inWidthIsPercentage, inHeightIsPercentage);
		}

		/** clear background image */
virtual	void				ClearParaBackgroundImage(sLONG inStart = 0, sLONG inEnd = -1)
		{
			ClearBackgroundImage();
		}


/////////////////

		bool				IsDirty() const { return !fLayoutIsValid || fLayoutBoundsDirty; }

		/** set dirty */
		void				SetDirty(bool inApplyToChildren = false, bool inApplyToParent = true);
		
		void				SetPlaceHolder( const VString& inPlaceHolder)	{ fPlaceHolder = inPlaceHolder; SetDirty(); }
		void				GetPlaceHolder( VString& outPlaceHolder) const { outPlaceHolder = fPlaceHolder; }

		/** enable/disable password mode */
		void				EnablePassword(bool inEnable) { fIsPassword = inEnable; SetDirty(); }
		bool				IsEnabledPassword() const { return fIsPassword; }


		/** enable/disable cache
		@remarks
			true (default): VDocParagraphLayout will cache impl text layout & optionally rendered text content on multiple frames
							in that case, impl layout & text content are computed/rendered again only if gc passed to methods is no longer compatible with the last used gc 
							or if text layout is dirty;

			false:			disable layout cache
			
			if VGraphicContext::fPrinterScale == true - ie while printing, cache is automatically disabled so you can enable cache even if VDocParagraphLayout instance is used for both screen drawing & printing
			(but it is not recommended: you should use different layout instances for painting onscreen or printing)
		*/
		void				ShouldEnableCache( bool inShouldEnableCache);
		bool				ShouldEnableCache() const { return fShouldEnableCache; }


		/** allow/disallow offscreen text rendering (default is false)
		@remarks
			text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
			you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
		@see
			ShouldEnableCache
		*/
		void				ShouldDrawOnLayer( bool inShouldDrawOnLayer);
		bool				ShouldDrawOnLayer() const { return fShouldDrawOnLayer; }
		void				SetLayerDirty(bool inApplyToChildren = false) { fLayerIsDirty = true; }


		/** paint selection */
		void				ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor = NULL, const VColor* inInactiveColor = NULL);
		bool				ShouldPaintSelection() const { return fShouldPaintSelection; }
		/** change selection (selection is used only for painting) */
		void				ChangeSelection( sLONG inStart, sLONG inEnd = -1, bool inActive = true);
		bool				IsSelectionActive() const { return fSelIsActive; }

		/** create and retain document from the current layout  
		@param inStart
			range start
		@param inEnd
			range end

		@remarks
			if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
			with a single child node initialized from the current layout
		*/
		VDocText*			CreateDocument(sLONG inStart = 0, sLONG inEnd = -1);

		/** create and retain paragraph from the current layout plain text, styles & paragraph properties 
		@param inStart
			range start
		@param inEnd
			range end
		*/
		VDocParagraph*		CreateParagraph(sLONG inStart = 0, sLONG inEnd = -1);

		/** apply paragraph properties & styles if any to the current layout (passed paragraph text is ignored) 
		@param inPara
			the paragraph style
		@param inResetStyles
			if true, reset styles to default prior to apply new paragraph style
		@remarks
			such paragraph is just used for styling:
			so note that only paragraph first level VTreeTextStyle styles are applied (to the layout full plain text: paragraph style range is ignored)
		*/
		void				ApplyParagraphStyle( const VDocParagraph *inPara, bool inResetStyles = false, bool inApplyParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1);

		/** init layout from the passed paragraph 
		@param inPara
			paragraph source
		@remarks
			the passed paragraph is used only for initializing layout plain text, styles & paragraph properties but inPara is not retained 
			note here that current styles are reset (see ResetParagraphStyle) & replaced by paragraph styles
			also current plain text is replaced by the passed paragraph plain text 
		*/
		void				InitFromParagraph( const VDocParagraph *inPara);

		/** reset all text properties & character styles to default values */
		void				ResetParagraphStyle(bool inResetParagraphPropsOnly = false, sLONG inStart = 0, sLONG inEnd = -1);

		/** merge paragraph(s) on the specified range to a single paragraph 
		@remarks
			if there are more than one paragraph on the range, plain text & styles from the paragraphs but the first one are concatenated with first paragraph plain text & styles
			resulting with a single paragraph on the range (using paragraph styles from the first paragraph) where plain text might contain more than one CR

			remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
			you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
		*/
		bool				MergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1) { return false; /** here do nothing as it is only meaningful for container layout class */}

		/** unmerge paragraph(s) on the specified range 
		@remarks
			any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR, 
			resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)

			remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
			you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
		*/
		bool				UnMergeParagraphs( sLONG inStart = 0, sLONG inEnd = -1);

		bool				HasSpanRefs() const { return fStyles && fStyles->HasSpanRefs(); }

		/** convert uniform character index (the index used by 4D commands) to actual plain text position 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
		void				CharPosFromUniform( sLONG& ioPos) const { if (fStyles) fStyles->CharPosFromUniform( ioPos); }

		/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
		void				CharPosToUniform( sLONG& ioPos) const { if (fStyles) fStyles->CharPosToUniform( ioPos); }

		/** convert uniform character range (as used by 4D commands) to actual plain text position 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
		void				RangeFromUniform( sLONG& inStart, sLONG& inEnd) const { if (fStyles) fStyles->RangeFromUniform( inStart, inEnd); }

		/** convert actual plain text range to uniform range (as used by 4D commands) 
		@remarks
			in order 4D commands to use uniform character position while indexing text containing span references 
			(span ref content might vary depending on computed value or showing source or value)
			we need to map actual plain text position with uniform char position:
			uniform character position assumes a span ref has a text length of 1 character only
		*/
		void				RangeToUniform( sLONG& inStart, sLONG& inEnd) const { if (fStyles) fStyles->RangeToUniform( inStart, inEnd); }

		/** adjust selection 
		@remarks
			you should call this method to adjust with surrogate pairs or span references
		*/
		void				AdjustRange(sLONG& ioStart, sLONG& ioEnd) const;

		/** adjust char pos
		@remarks
			you should call this method to adjust with surrogate pairs or span references
		*/
		void				AdjustPos(sLONG& ioPos, bool inToEnd = true) const;

		/** replace text with span text reference on the specified range
		@remarks
			it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

			you should no more use or destroy passed inSpanRef even if method returns false

			IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
					   so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
					   (passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens 
					    - if display type is set to normalized source)
		*/
		bool				ReplaceAndOwnSpanRef(	VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = false, 
													void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

		/** return span reference at the specified text position if any */
		const VTextStyle*	GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart = NULL) const
		{
			if (outBaseStart)
				*outBaseStart = 0;
			return fStyles ? fStyles->GetStyleRefAtPos( inPos) : NULL;
		}

		/** return the first span reference which intersects the passed range */
		const VTextStyle*	GetStyleRefAtRange(sLONG inStart = 0, sLONG inEnd = -1, sLONG *outBaseStart = NULL) const
		{
			if (outBaseStart)
				*outBaseStart = 0;
			if (fStyles)
			{
				sLONG start = inStart, end = inEnd;
				_NormalizeRange( start, end);
				return fStyles->GetStyleRefAtRange( start, end);
			}
			else
				return NULL;
		}

		/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
		bool				UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL);

		/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
		bool				FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

		/** eval 4D expressions  
		@remarks
			note that it only updates span 4D expression references but does not update plain text:
			call UpdateSpanRefs after to update layout plain text

			return true if any 4D expression has been evaluated
		*/
		bool				EvalExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL);

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
		void				SetLayerViewRect( const VRect& inRectRelativeToWnd) 
		{
			if (fLayerViewRect == inRectRelativeToWnd)
				return;
			fLayerViewRect = inRectRelativeToWnd;
			fLayerIsDirty = true;
		}
		const VRect&		GetLayerViewRect() const { return fLayerViewRect; }

		/** insert text at the specified position */
		void				InsertPlainText( sLONG inPos, const VString& inText);

		/** delete text range */
		void				DeletePlainText( sLONG inStart, sLONG inEnd);

		/** replace text */
		void				ReplacePlainText( sLONG inStart, sLONG inEnd, const VString& inText);

		/** apply style (use style range) */
		bool				ApplyStyle( const VTextStyle* inStyle, bool inFast = false, bool inIgnoreIfEmpty = true);

		bool				ApplyStyles( const VTreeTextStyle * inStyles, bool inFast = false);

		/** show span references source or value */ 
		void				ShowRefs( SpanRefCombinedTextTypeMask inShowRefs, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL, VDBLanguageContext *inLC = NULL)
		{
			if (fShowRefs == inShowRefs)
				return;
			fShowRefs = inShowRefs;
			if (fStyles)
			{
				fStyles->ShowSpanRefs( inShowRefs);
				UpdateSpanRefs(0, -1, inUserData, inBeforeReplace, inAfterReplace, inLC); 
			}
		}

		SpanRefCombinedTextTypeMask	ShowRefs() const { return fShowRefs; }

		/** highlight span references in plain text or not */
		void				HighlightRefs(bool inHighlight, bool inUpdateAlways = false)
		{
			if (!inUpdateAlways && fHighlightRefs == inHighlight)
				return;
			fHighlightRefs = inHighlight;
			if (fStyles)
			{
				fStyles->ShowHighlight( fHighlightRefs);
				SetDirty();
			}
		}

		/** call this method to update span reference style while mouse enters span ref 
		@remarks
			return true if style has changed
		*/
		bool				NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0)
		{
			if (!fStyles)
				return false;
			if (fStyles->NotifyMouseEnterStyleSpanRef( inStyleSpanRef))
			{
				SetDirty();
				return true;
			}
			return false;
		}

		/** call this method to update span reference style while mouse leaves span ref 
		@remarks
			return true if style has changed
		*/
		bool				NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart = 0)
		{
			if (!fStyles)
				return false;
			if (fStyles->NotifyMouseLeaveStyleSpanRef( inStyleSpanRef))
			{
				SetDirty();
				return true;
			}
			return false;
		}


		bool				HighlightRefs() const { return fHighlightRefs; }

		/** set default font 
		@remarks
			it should be not NULL - STDF_TEXT standard font on default (it is applied prior styles set with SetText)
			
			by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
		*/
		void				SetDefaultFont( const VFont *inFont, GReal inDPI = 72.0f, sLONG inStart = 0, sLONG inEnd = -1);

		/** get & retain default font 
		@remarks
			by default, return 4D form-compliant font (with dpi = 72) so not the actual fRenderDPI-scaled font 
			(for consistency with SetDefaultFont & because fDPI internal scaling should be transparent for caller)
		*/
		VFont*				RetainDefaultFont(GReal inDPI = 72.0f, sLONG inStart = 0) const;

		/** set default text color 
		@remarks
			black color on default (it is applied prior styles set with SetText)
		*/
		void				SetDefaultTextColor( const VColor &inTextColor, sLONG inStart = 0, sLONG inEnd = -1);

		/** get default text color */
		bool				GetDefaultTextColor(VColor& outColor, sLONG inStart = 0) const;

		/** set back color 
		@param  inColor
			background color
		@param	inPaintBackColor
			unused (preserved for compatibility with VTextLayout)
		@remarks
			OBSOLETE: you should use SetBackgroundColor - it is preserved for compatibility with VTextLayout
		*/
		void				SetBackColor( const VColor& inColor, bool inPaintBackColor = false) { SetBackgroundColor( inColor); }
		const VColor&		GetBackColor() const { return GetBackgroundColor(); }
		
		/** replace current text & styles 
		@remarks
			if inCopyStyles == true, styles are copied
			if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 

			this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
		*/
		void				SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

		/** replace current text & styles
		@remarks	
			here passed styles are always copied

			this method preserves current uniform style on range (first character styles) if not overriden by the passed styles
		*/
		void				SetText( const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd = -1, 
									 bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, 
									 const VDocSpanTextRef * inPreserveSpanTextRef = NULL, 
									 void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

		const VString&		GetText() const;

		void				GetText(VString& outText, sLONG inStart = 0, sLONG inEnd = -1) const;

		/** return unicode character at passed position (0-based) - 0 if out of range */
		UniChar				GetUniChar( sLONG inIndex) const { if (inIndex >= 0 && inIndex < fText.GetLength()) return fText[inIndex]; else return 0; }

		/** return styles on the passed range 
		@remarks
			if inIncludeDefaultStyle == false (default is true) and if layout is a paragraph, it does not include paragraph uniform style (default style) but only styles which are not uniform on paragraph text:
			otherwise it returns all styles applied to the plain text

			caution: if inCallerOwnItAlways == false (default is true), returned styles might be shared with layout instance so you should not modify it
		*/
		VTreeTextStyle*		RetainStyles(sLONG inStart = 0, sLONG inEnd = -1, bool inRelativeToStart = true, bool inIncludeDefaultStyle = true, bool inCallerOwnItAlways = true) const;

		/** set extra text styles 
		@remarks
			it can be used to add a temporary style effect to the text layout without modifying the main text styles
			(these styles are not exported to VDocParagraph by CreateParagraph)

			if inCopyStyles == true, styles are copied
			if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
		*/
		void				SetExtStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

		VTreeTextStyle*		RetainExtStyles() const
		{
			return fExtStyles ? new VTreeTextStyle(fExtStyles) : NULL;
		}

		/** replace ouside margins	(in point) */
		void				SetMargins(const GReal inMarginLeft, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f)
		{
			ITextLayout::SetMargins(inMarginLeft, inMarginRight, inMarginTop, inMarginBottom);
		}

		/** get outside margins (in point) */
		void				GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom) const
		{
			ITextLayout::GetMargins(outMarginLeft, outMarginRight, outMarginTop, outMarginBottom);
		}

		/** set max text width (0 for infinite) - in pixel relative to the layout target DPI
		@remarks
			it is also equal to text box width 
			so you should specify one if text is not aligned to the left border
			otherwise text layout box width will be always equal to formatted text width
		*/ 
		void				SetMaxWidth(GReal inMaxWidth = 0.0f);
		GReal				GetMaxWidth() const { return fRenderMaxWidth; }

		/** set max text height (0 for infinite) - in pixel relative to the layout target DPI
		@remarks
			it is also equal to text box height 
			so you should specify one if paragraph is not aligned to the top border
			otherwise text layout box height will be always equal to formatted text height
		*/ 
		void				SetMaxHeight(GReal inMaxHeight = 0.0f); 
		GReal				GetMaxHeight() const { return fRenderMaxHeight; }

		/** get paragraph line height 

			if positive, it is fixed line height in point
			if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
			if -1, it is normal line height 
		*/
		GReal				GetLineHeight(sLONG inStart = 0, sLONG inEnd = -1) const { return fLineHeight; }

		/** set paragraph line height 

			if positive, it is fixed line height in point
			if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
			if -1, it is normal line height 
		*/
		void				SetLineHeight( const GReal inLineHeight, sLONG inStart = 0, sLONG inEnd = -1);

		bool				IsRTL(sLONG inStart = 0, sLONG inEnd = -1) const { return (GetLayoutMode() & TLM_RIGHT_TO_LEFT) != 0; }
		void				SetRTL(bool inRTL, sLONG inStart = 0, sLONG inEnd = -1)
		{
			SetLayoutMode( inRTL ? (GetLayoutMode() | TLM_RIGHT_TO_LEFT) : (GetLayoutMode() & (~TLM_RIGHT_TO_LEFT)));
		}

		/** get first line padding offset in point
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
		GReal				GetTextIndent(sLONG inStart = 0, sLONG inEnd = -1) const { return fTextIndent; }

		/** set first line padding offset in point 
		@remarks
			might be negative for negative padding (that is second line up to the last line is padded but the first line)
		*/
		void				SetTextIndent(const GReal inPadding, sLONG inStart = 0, sLONG inEnd = -1);

		/** get tab stop offset in point */
		GReal				GetTabStopOffset(sLONG inStart = 0, sLONG inEnd = -1) const { return fTabStopOffset; }

		/** set tab stop (offset in point) */
		void				SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG inStart = 0, sLONG inEnd = -1);

		/** get tab stop type */ 
		eTextTabStopType	GetTabStopType(sLONG inStart = 0, sLONG inEnd = -1) const { return fTabStopType; }

		/** set text layout mode (default is TLM_NORMAL) */
		void				SetLayoutMode( TextLayoutMode inLayoutMode = TLM_NORMAL, sLONG inStart = 0, sLONG inEnd = -1);
		/** get text layout mode */
		TextLayoutMode		GetLayoutMode(sLONG inStart = 0, sLONG inEnd = -1) const { return fLayoutMode; }

		/** begin using text layout for the specified gc (reentrant)
		@remarks
			for best performance, it is recommended to call this method prior to more than one call to Draw or metrics get methods & terminate with EndUsingContext
			otherwise Draw or metrics get methods will check again if layout needs to be updated at each call to Draw or metric get method
			But you do not need to call BeginUsingContext & EndUsingContext if you call only once Draw or a metrics get method.

			CAUTION: if you call this method, you must then pass the same gc to Draw or metrics get methods before EndUsingContext is called
					 otherwise Draw or metrics get methods will do nothing (& a assert failure is raised in debug)

			If you do not call Draw method, you should pass inNoDraw = true to avoid to create a drawing context on some impls (actually for now only Direct2D impl uses this flag)
		*/
		void				BeginUsingContext( VGraphicContext *inGC, bool inNoDraw = false);

		/** return text layout bounds including margins (in gc user space)
		@remarks
			text layout origin is set to inTopLeft
			on output, outBounds contains text layout bounds including any glyph overhangs; outBounds left top origin should be equal to inTopLeft

			on input, max width is determined by inTextLayout->GetMaxWidth() & max height by inTextLayout->GetMaxHeight()
			(if max width or max height is equal to 0.0f, it is assumed to be infinite & so layout bounds will be equal to formatted text bounds)
			output layout width will be equal to max width if maw width != 0 otherwise to formatted text width
			output layout height will be equal to max height if maw height != 0 otherwise to formatted text height
			(see CAUTION remark below)

			if text is empty and if inReturnCharBoundsForEmptyText == true, returned bounds will be set to a single char bounds 
			(useful while editing in order to compute initial text bounds which should not be empty in order to draw caret;
			 if you use text layout only for read-only text, you should let inReturnCharBoundsForEmptyText to default which is false)

			CAUTION: layout bounds can be greater than formatted text bounds (if max width or max height is not equal to 0)
					 because layout bounds are equal to the text box design bounds and not to the formatted text bounds: 
					 you should use GetTypoBounds in order to get formatted text width & height
		*/
		void				GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

		/** return formatted text bounds (typo bounds) including margins (in gc user space)
		@remarks
			text typo bounds origin is set to inTopLeft

			as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
			because layout bounds are equal to the text box design bounds and not to the formatted text bounds
			you should use this method to get formatted text width & height 
		*/
		void				GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

		/** return text layout run bounds from the specified range (in gc user space)
		@remarks
			text layout origin is set to inTopLeft
		*/
		void				GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);

		/** return the caret position & height of text at the specified text position in the text layout (in gc user space)
		@remarks
			text layout origin is set to inTopLeft

			text position is 0-based

			caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
			if text layout is drawed at inTopLeft

			by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
			but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
		*/
		void				GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);

		/** return the text position at the specified coordinates
		@remarks
			text layout origin is set to inTopLeft (in gc user space)
			the hit coordinates are defined by inPos param (in gc user space)

			output text position is 0-based

			return true if input position is inside character bounds
			if method returns false, it returns the closest character index to the input position
		*/
		bool				GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength = NULL);

		/** get character range which intersects the passed bounds (in gc user space) 
		@remarks
			text layout origin is set to inTopLeft (in gc user space)

			char range is rounded to line run limit so the returned range might be slightly greater than the actual char range that intersects the passed bounds
		*/
		void				GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd);


		/** move character index to the nearest character index on the line before/above the character line 
		@remarks
			return false if character is on the first line (ioCharIndex is not modified)
		*/
		bool				MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex);

		/** move character index to the nearest character index on the line after/below the character line
		@remarks
			return false if character is on the last line (ioCharIndex is not modified)
		*/
		bool				MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex);
		
		/** return true if layout uses truetype font only */
		bool				UseFontTrueTypeOnly(VGraphicContext *inGC) const;

		/** end using text layout (reentrant) */
		void				EndUsingContext();

		VTreeTextStyle*		RetainDefaultStyle(sLONG inStart = 0, sLONG inEnd = -1) const;

		/** consolidate end of lines
		@remarks
			if text is multiline:
				we convert end of lines to a single CR 
			if text is singleline:
				if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
				if inEscaping, we escape line endings and tab 
		*/
		bool				ConsolidateEndOfLines(bool inIsMultiLine = true, bool inEscaping = false, sLONG inStart = 0, sLONG inEnd = -1);

		/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) range at the specified character index */
		void				GetWordRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces = true, bool inTrimPunctuation = false)
		{
			ITextLayout::GetWordRange( inIntlMgr, inOffset, outStart, outEnd, inTrimSpaces, inTrimPunctuation);
		}
		
		/**	get paragraph range at the specified character index 
		@remarks
			if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
			(for VTextLayout, parameter is ignored)
		*/
		void				GetParagraphRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange = false)
		{
			if (inUseDocParagraphRange)
			{
				outStart = 0;
				outEnd = GetTextLength();
				return;
			}
			ITextLayout::GetParagraphRange( inIntlMgr, inOffset, outStart, outEnd, inUseDocParagraphRange);
		}

static	VTextStyle*			CreateTextStyle( const VFont *inFont, const GReal inDPI = 72.0f);

		/**	create text style from the specified font name, font size & font face
		@remarks
			if font name is empty, returned style inherits font name from parent
			if font face is equal to UNDEFINED_STYLE, returned style inherits style from parent
			if font size is equal to UNDEFINED_STYLE, returned style inherits font size from parent
		*/
static	VTextStyle*			CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI = 0);		

protected:

		typedef std::vector<GReal>	CharOffsets;
		typedef enum eDirection
		{
			kPARA_LTR	= 0,	//left to right writing direction
			kPARA_RTL	= 1		//right to left writing direction
		} eDirection;

		/** run  
		@remarks
			a run uses a single direction & style
		*/
		class Line;
		class Run
		{
		public: 
									Run(Line *inLine, const sLONG inIndex, const sLONG inBidiVisualIndex, const sLONG inStart, const sLONG inEnd, 
										const eDirection inDirection = kPARA_LTR, const GReal inFontSize = 12, VFont *inFont = NULL, const VColor *inTextColor = NULL, const VColor *inBackColor = NULL);
									Run(const Run& inRun);
		Run&						operator=(const Run& inRun);
		virtual						~Run() { ReleaseRefCountable(&fFont); }

				sLONG				GetStart() const { return fStart; }
				sLONG				GetEnd() const { return fEnd; }
				void				SetStart( const sLONG inStart) { fStart = inStart; }
				void				SetEnd( const sLONG inEnd) { fEnd = inEnd; }

				sLONG				GetIndex() const { return fIndex; }

				void				SetDirection( const eDirection inDir) { fDirection = inDir; }
				eDirection			GetDirection() const { return fDirection; }

				void				SetBidiVisualIndex( const sLONG inBidiVisualIndex) { fBidiVisualIndex = inBidiVisualIndex; }
				sLONG				GetBidiVisualIndex() const { return fBidiVisualIndex; }

				void				SetFont( VFont *inFont) { CopyRefCountable(&fFont, inFont); }
				VFont*				GetFont() const { return fFont; }

				void				SetFontSize( const GReal inSize) { fFontSize = inSize; }
				GReal				GetFontSize() const { return fFontSize; }

				/** update internal font & embedded objects layout according to new DPI */
				void				NotifyChangeDPI();

				void				SetTextColor( const VColor& inColor) { fTextColor = inColor; }
				const VColor&		GetTextColor() const { return fTextColor; }

				void				SetBackColor( const VColor& inColor) { fBackColor = inColor; }
				const VColor&		GetBackColor() const { return fBackColor; }

				void				SetNodeLayout( IDocNodeLayout *inNodeLayout)  { fNodeLayout = inNodeLayout; }
				IDocNodeLayout*		GetNodeLayout() const { return fNodeLayout; }
				
				void				SetStartPos( const VPoint& inPos) { fStartPos = inPos; }
				const VPoint&		GetStartPos() const { return fStartPos; }

				void				SetEndPos( const VPoint& inPos) { fEndPos = inPos; }
				const VPoint&		GetEndPos() const { return fEndPos; }

				const GReal			GetAscent() const { return fAscent; }
				const GReal			GetDescent() const { return fDescent; }
				const VRect&		GetTypoBounds(bool inWithTrailingSpaces = true) const { if (inWithTrailingSpaces) return fTypoBoundsWithTrailingSpaces; else return fTypoBounds; }
				const VRect&		GetTypoBoundsWithKerning() const { return fTypoBoundsWithKerning; }
				const CharOffsets&	GetCharOffsets();

				bool				IsTab() const;

				void				SetIsLeadingSpace( bool inLeadingSpace) { fIsLeadingSpace = inLeadingSpace; }
				bool				IsLeadingSpace() const { return fIsLeadingSpace; }

				/** first pass: compute run metrics (metrics will be adjusted later in ComputeMetrics - to take account bidi for instance)
								here only run width is used before ComputeMetrics by VParagraph to check for word-wrap for instance
				@param inIgnoreTrailingSpaces
					true: ignore trailing spaces (according to bidi direction)
				*/
				void				PreComputeMetrics(bool inIgnoreTrailingSpaces = false);

				/** compute final metrics according to bidi */
				void				ComputeMetrics(GReal& inBidiCurDirStartOffset);

		protected:
				friend class VDocParagraphLayout;

				Line*				fLine;

				//input info

				/** line run index */
				sLONG				fIndex;

				/** bidi visual index */
				sLONG				fBidiVisualIndex;
				
				/** run start & end index */
				sLONG				fStart, fEnd;

				/** run direction */
				eDirection			fDirection;

				/** run start position (y is aligned on base line) - relative to line start pos & anchored to left border for LTR and right border for RTL */
				VPoint				fStartPos;

				/** design font size (in point)
				@remarks
					it is unscaled font size (fFont is font scaled by layout DPI)
				*/
				GReal				fFontSize;

				/** run font (& so style) */
				VFont*				fFont;

				/** text color (use it if alpha > 0) */
				VColor				fTextColor;

				/** background color (use it if alpha > 0) */
				VColor				fBackColor;

				/** node layout reference (used for embedded image for instance) 
				@remarks
					Run instance is not IDocNodeLayout instance owner: IDocNodeLayout owner is a VDocSpanTextRef instance, itself owned by a VParagraph::fStyles style
				*/
				IDocNodeLayout*		fNodeLayout;	

				//computed info

				/** run end position (y is aligned on base line) 
				@remarks
					it is run next char position (after run last char): 
					it takes account kerning between run last char & next char if next char has the same direction as the current run
					otherwise end pos is set to run left or right border depending on run direction
					(that is adjacent runs with different writing direction do not overlap while adjacent runs with the same direction might overlap because of kerning)
				*/
				VPoint				fEndPos;

				/** run ascent & descent (equal to font ascent & descent) */
				GReal				fAscent, fDescent;

				/** run typo bounds (without trailing spaces)
				@remarks
					should be less than typo bounds with trailing spaces 
					suitable for clipping & bounding box 
				*/
				VRect				fTypoBounds;

				/** run typo bounds with trailing spaces 
				@remarks
					should be greater than typo bounds without trailing spaces  
					suitable for selection highlighting  
				*/
				VRect				fTypoBoundsWithTrailingSpaces;

				/** run hit bounds 
				@remarks
					might be less than typo bounds as it takes account kerning to avoid overlapping with adjacent run bounds: 
					suitable for hit testing & not overlapping run bounds (includes trailing spaces) */
				VRect				fTypoBoundsWithKerning;

				/** character offsets from run start position (trailing offsets) */
				CharOffsets			fCharOffsets; 

				/** true if it is a leading run with only whitespaces (according to paragraph direction) */
				bool				fIsLeadingSpace;

				/** computed tab offset in point (from line start pos) */
				GReal				fTabOffset;
		};
		typedef std::vector<Run *> Runs;

		/*	line 
		@remarks 
			a line is not always equal to a paragraph:
			for instance if word-wrapping is enabled, a paragraph might be composed of multiple lines
		*/
		class VParagraph;
		class Line
		{
		public:
									Line(VParagraph *inParagraph, const sLONG inIndex, const sLONG inStart, const sLONG inEnd);
									Line(const Line& inLine);
				Line&				operator=(const Line& inLine);

		virtual						~Line();

				sLONG				GetIndex() const { return fIndex; }

				void				SetStart( const sLONG inStart) { fStart = inStart; }
				void				SetEnd( const sLONG inEnd) { fEnd = inEnd; }
				sLONG				GetStart() const { return fStart; }
				sLONG				GetEnd() const { return fEnd; }

				void				SetStartPos( const VPoint& inPos) { fStartPos = inPos; }
				const VPoint&		GetStartPos() const { return fStartPos; }

				/** const accessor on runs in logical order */	
				const Runs&			GetRuns() const { return fRuns; }

				/** const accessor on runs in visual order (from left to right visual order) */
				const Runs&			GetVisualRuns() const { return fVisualRuns; }

				/** read/write accessor on runs (logical order) */
				Runs&				thisRuns() { return fRuns;}

				GReal				GetAscent() const { return fAscent; }
				GReal				GetDescent() const { return fDescent; }
				/** get line height: 
					on default equal to line ascent+descent: 
					it might not be equal to ascent+descent if VDocParagraphLayout line height is not set to 100% 
					(e.g. actual line height is (ascent+descent)*2 if VDocParagraphLayout line height is set to 200%
					 - note that interline spacing is always below descent)					*/
				GReal				GetLineHeight() const { return fLineHeight; }

				/** get typo bounds
				@param inWithTrailingSpaces
					true: include line trailing spaces bounds (suitable for selection painting and hit testing)
					false: does not include line trailing spaces bounds (suitable to determine bounding box & formatted text max width)
				*/
				const VRect&		GetTypoBounds(bool inWithTrailingSpaces = true) const { if (inWithTrailingSpaces) return fTypoBoundsWithTrailingSpaces; else return fTypoBounds; }
				
				/** compute metrics according to start pos, start & end indexs & run info */
				void				ComputeMetrics();

				/** for run visual order sorting */
		static	bool CompareVisual(Run* inRun1, Run* inRun2);
		protected:
				friend class		Run;
				friend class		VDocParagraphLayout;

				/* font ascent & descent */
				typedef std::pair<GReal,GReal> FontMetrics;
				typedef std::vector< FontMetrics > VectorOfFontMetrics;

				VParagraph*			fParagraph;

				//input info

				/** paragraph line index */
				sLONG				fIndex;

				/** line start & end index */
				sLONG				fStart, fEnd;

				/** line start position (y is aligned on base line) - relative to paragraph start pos */
				VPoint				fStartPos;
					
				/** line runs (logical order) */
				Runs				fRuns;
		
				/** line runs (visual order) */
				Runs				fVisualRuns;

				//computed info

				/** line ascent & descent (max of runs ascent & descent) */
				GReal				fAscent, fDescent;

				/** line typo bounds - without trailing spaces (union of run typo bounds)*/
				VRect				fTypoBounds;

				/** line typo bounds with trailing spaces (union of run typo bounds)*/
				VRect				fTypoBoundsWithTrailingSpaces;

				/** actual line height (see GetLineHeight()) */
				GReal				fLineHeight;
		};
		/** typedef for paragraph line(s) (more than one only if paragraph is word-wrapped) */
		typedef std::vector<Line *>					Lines;

		/**	paragraph info */
		class VParagraph: public VObject, public IRefCountable
		{
		public:
				/** typedef for paragraph direction range: pair<start of range, range direction>*/
				typedef	std::pair<sLONG, eDirection>		DirectionRange;
				typedef	std::vector<DirectionRange>			Directions;


									VParagraph(VDocParagraphLayout *inParaLayout, const sLONG inIndex, const sLONG inStart, const sLONG inEnd);
		virtual						~VParagraph();

				void				SetDirty( const bool inDirty) { fDirty = inDirty; }
				bool				IsDirty() const { return fDirty; }

				void				SetRange( const sLONG inStart, const sLONG inEnd);
				sLONG				GetStart() const { return fStart; }
				sLONG				GetEnd() const { return fEnd; }

				sLONG				GetDirection() const { return fDirection; }
				const VectorOfBidiRuns&	GetBidiRuns() const { return fBidiRuns; }

				const Lines&		GetLines() const { return fLines;}

				void				SetStartPos( const VPoint& inPos) { fStartPos = inPos; }
				const VPoint&		GetStartPos() const { return fStartPos; }

				/** get typo bounds
				@param inWithTrailingSpaces
					true: include line trailing spaces bounds 
					false: does not include line trailing spaces bounds 
				*/
				const VRect&		GetTypoBounds(bool inWithTrailingSpaces = true) const { if (inWithTrailingSpaces) return fTypoBoundsWithTrailingSpaces; else return fTypoBounds; }
				
				const VRect&		GetLayoutBounds() const { return fLayoutBounds; }
				
				bool				GetWordWrapOverflow() const { return fWordWrapOverflow; }
				
				void				ComputeMetrics(bool inUpdateBoundsOnly, bool inUpdateDPIMetricsOnly, const GReal inMaxWidth);

		protected:
				friend class VDocParagraphLayout;

				void				_NextRun( Run* &run, Line *line, GReal& lineWidth, sLONG i, bool isTab = false);
				void				_NextRun( Run* &run, Line *line, eDirection dir, sLONG bidiVisualIndex, GReal& lineWidth, sLONG i, bool isTab = false);
				void				_NextRun( Run* &run, Line *line, eDirection dir, sLONG bidiVisualIndex, const GReal fontSize, VFont* font, VColor textColor, VColor backColor, GReal& lineWidth, sLONG i, bool isTab = false);
				void				_NextRun( Run* &run, Line *line, eDirection dir, sLONG bidiVisualIndex, GReal& fontSize, VFont* &font, VTextStyle* style, GReal& lineWidth, sLONG i);
				void				_NextLine( Run* &run, Line* &line, sLONG i);
				void				_ApplyTab( GReal& pos, VFont *font);

				/** extract font style from VTextStyle */
				VFontFace			_TextStyleToFontFace( const VTextStyle *inStyle);
				
				bool				_IsOnlyLTR() const { return fDirection == kPARA_LTR && ((fBidiRuns.size() == 0) || (fBidiRuns.size() == 1 && ((fBidiRuns[0].fBidiLevel & 1) == 0))); }
				bool				_IsOnlyRTL() const { return fDirection == kPARA_RTL && (fBidiRuns.size() == 1 && ((fBidiRuns[0].fBidiLevel & 1) == 1)); }

				typedef std::vector<VTextStyle *> Styles;

				friend class Line;
				friend class Run;

				//input info

				bool				fDirty;

				/** paragraph index */
				VIndex				fIndex;

				/** ref on paragraph layout */
				VDocParagraphLayout*	fParaLayout;

				/** paragraph start & end indexs */
				sLONG				fStart, fEnd;

				/** paragraph start offset (relative to text box left top origin)*/
				VPoint				fStartPos;

				//computed info

				/** main paragraph direction
				@remarks
					if mixed, fBidiRuns stores direction for sub-ranges

					(determined with ICU)
				*/
				eDirection			fDirection;
				VectorOfBidiRuns	fBidiRuns;
				VectorOfBidiVisualToLogical fBidiVisualToLogicalMap;

				/** text indent */
				GReal				fTextIndent;

				/** paragraph line(s) (more than one only if paragraph is word-wrapped) */
				Lines				fLines;

				/** paragraph character uniform styles (sequential) */
				Styles				fStyles;

				/** typographic bounds */
				VRect				fTypoBounds;

				/** typographic bounds with trailing spaces */
				VRect				fTypoBoundsWithTrailingSpaces;

				/** current max width (minus left & right margins) */
				GReal				fMaxWidth;

				/** current word-wrap status */
				bool 				fWordWrap;

				/** layout bounds */
				VRect				fLayoutBounds;

				/** true if formatted width is greater than max width (if word-wrap enabled) 
				@remarks
					if max width is too small it might be not possible to word-break
					and so formatted width might overflow max width
				*/
				bool				fWordWrapOverflow;

				GReal				fCurDPI;
		};

typedef	std::vector<VRect>	VectorOfRect;

		void				_Init();

		bool				_ApplyStyle(const VTextStyle *inStyle, bool inIgnoreIfEmpty = true);

		bool				_ApplyStylesRec( const VTreeTextStyle * inStyles, bool inFast = false);

		/** retain default uniform style 
		@remarks
			if fStyles == NULL, theses styles are text content uniform styles (monostyle)
		*/
		VTreeTextStyle*		_RetainDefaultTreeTextStyle() const;

		/** retain full styles
		@remarks
			use this method to get the full styles applied to the text content
			(but not including the graphic context default font: this font is already applied internally by gc methods before applying styles)

			caution: please do not modify fStyles between _RetainFullTreeTextStyle & _ReleaseFullTreeTextStyle

			(in monostyle, it is generally equal to _RetainDefaultTreeTextStyle)
		*/
		VTreeTextStyle*		_RetainFullTreeTextStyle() const;

		void				_ReleaseFullTreeTextStyle( VTreeTextStyle *inStyles) const;

		void				_CheckStylesConsistency();

		bool				_CheckLayerBounds(const VPoint& inTopLeft, VRect& outLayerBounds);

		bool				_BeginDrawLayer(const VPoint& inTopLeft = VPoint());
		void				_EndDrawLayer();

		/** set text layout graphic context 
		@remarks
			it is the graphic context to which is bound the text layout
			if offscreen layer is disabled text layout needs to be redrawed again at every frame
			so it is recommended to enable the internal offscreen layer if you intend to preserve rendered layout on multiple frames
			because offscreen layer & text content are preserved as long as offscreen layer is compatible with the attached gc & text content is not dirty
		*/
		void				_SetGC( VGraphicContext *inGC);
		
		/** return the graphic context bound to the text layout 
		@remarks
			gc is not retained
		*/
		VGraphicContext*	_GetGC() { return fGC; }

		/** update text layout if it is not valid */
		void				_UpdateLayout();
		void				_UpdateLayoutInternal();

		void				_BeginUsingStyles();
		void				_EndUsingStyles();

		void				_ResetLayer();

		/** return true if layout styles uses truetype font only */
		bool				_StylesUseFontTrueTypeOnly() const;

		void				_UpdateSelectionRenderInfo( const VPoint& inTopLeft, const VRect& inClipBounds);
		void				_DrawSelection(const VPoint& inTopLeft);

		void				_ReplaceText( const VDocParagraph *inPara);
static void					_BeforeReplace(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText);
static void					_AfterReplace(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced);

		void				_ApplyParagraphProps( const VDocParagraph *inPara, bool inIgnoreIfInherited = true);
		void				_ResetStyles( bool inResetParagraphProps = true, bool inResetCharStyles = true);

		void				_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw = false);

		/** get line start position relative to text box left top origin */
		void				_GetLineStartPos( const Line* inLine, VPoint& outPos);

		/** get paragraph from character index (0-based index) */
		VParagraph*			_GetParagraphFromCharIndex(const sLONG inCharIndex);
		Line*				_GetLineFromCharIndex(const sLONG inCharIndex);
		Run*				_GetRunFromCharIndex(const sLONG inCharIndex);

		/** get paragraph from local position (relative to text box origin+vertical align decal & computing dpi)*/
		VParagraph*			_GetParagraphFromLocalPos(const VPoint& inPos);
		Line*				_GetLineFromLocalPos(const VPoint& inPos);
		Run*				_GetRunFromLocalPos(const VPoint& inPos, VPoint &outLineLocalPos);

		VGraphicContext*	_RetainComputingGC( VGraphicContext *inGC);

		typedef std::vector< VRefPtr<VParagraph> > Paragraphs;

		/** overriden from VDocBaseLayout */
virtual	void				_UpdateTextInfo();

		// design datas

		/** cache usage status */
		bool				fShouldEnableCache;

		/** layer usage status */
		bool				fShouldDrawOnLayer;

		/** show references status */
		SpanRefCombinedTextTypeMask fShowRefs;

		/** highlight references status */
		bool 				fHighlightRefs;


		//// default style management

		/** default font (always not NULL)*/
mutable VFont*				fDefaultFont;

		/** default text color */
		VColor				fDefaultTextColor;

		/** default style (includes default font & color) */
mutable VTreeTextStyle*		fDefaultStyle;


		//// plain text & styles management

		VString				fText;
		VTreeTextStyle*		fStyles;
		
		/** extra styles (these styles are not stored/parsed from VDocParagraph) */
		VTreeTextStyle*		fExtStyles;
		

		//// paragraph properties management

		/** line height in point */
		GReal 				fLineHeight;

		/** text indent in point */
		GReal 				fTextIndent;

		/** tab stop default offset in point */ 
		GReal 				fTabStopOffset;

		/** tab stop default type */
		eTextTabStopType	fTabStopType;

		/** layout mode (only wrapping & text direction is used here) */
		TextLayoutMode		fLayoutMode;

		//// placeholder management

		VString				fPlaceHolder;
		
		//// password management

		bool				fIsPassword;
		VString				fTextPassword;

		//// selection management

		/** should paint selection ? */
		bool				fShouldPaintSelection;

		/** selection range (used only for painting) */
		sLONG				fSelStart, fSelEnd;
		bool				fSelIsActive;

		/** selection color */
		VColor				fSelActiveColor, fSelInactiveColor;


		//computed datas

		//// layer management

		bool				fLayerIsDirty;
		VImageOffScreen*	fLayerOffScreen;
		VRect				fLayerViewRect;
		VPoint				fLayerOffsetViewRect;
		VAffineTransform	fLayerCTM;
		VPoint				fLayerTopLeft;

		//// graphic context management

		GraphicContextType	fGCType;
		VGraphicContext*	fGC;
		sLONG				fGCUseCount;

		//// layout management

		bool				fLayoutIsValid;
		bool 				fLayoutBoundsDirty;
		GReal				fLayoutKerning; //kerning used to compute current layout if layout is valid
		TextRenderingMode	fLayoutTextRenderingMode; //text rendering mode used to compute current layout if layout is valid
		bool 				fUseFontTrueTypeOnly; //valid only if layout is valid
		bool 				fStylesUseFontTrueTypeOnly; //valid only if layout is valid

		/* current max width - without margins - in TWIPS */
		sLONG				fMaxWidthTWIPS;

		/** computed paragraphs */
		Paragraphs			fParagraphs;


		//// layout metrics (in computing dpi = fDPI)

		/** typographic bounds in px - relative to computing DPI - including margin, padding & border 
		@remarks
			formatted text bounds (minimal bounds to display all text with margin, padding & border)
		*/
		VRect				fTypoBounds;

		/** margins in px - relative to computing dpi 
		@remarks
			same as fAllMargin but rounded according to gc pixel snapping (as we align first text run according to pixel snapping rule)
		*/
		sRectMargin			fMargin;


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

		//// selection management

		bool				fSelIsDirty;
		VectorOfRect		fSelectionRenderInfo;

		//// gc using temporary datas 

		VFont*				fBackupFont;
		VColor				fBackupTextColor;
		TextRenderingMode   fBackupTRM;
		VGraphicContext*	fBackupParentGC;
		bool				fNeedRestoreTRM;
		bool				fSkipDrawLayer;
		bool				fIsPrinting;

		VString				fTextBackup; //used to backup text if placeholder of password mode are active
		bool				fShouldRestoreText;
		bool				fShouldReleaseExtStyles;

		//replacement handler backup - we defer call to caller handler after layout is updated
		//(otherwise text layout would not be consistent with text when caller handler is called)
		void*								fReplaceTextUserData;
		VTreeTextStyle::fnBeforeReplace		fReplaceTextBefore;
		VTreeTextStyle::fnAfterReplace		fReplaceTextAfter;

}; //VDocParagraphLayout


END_TOOLBOX_NAMESPACE

#endif
