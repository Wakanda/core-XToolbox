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
#ifndef __VTextStyle__
#define __VTextStyle__

//forward reference for DB language context reference (defined in 4D)
class VDBLanguageContext;

BEGIN_TOOLBOX_NAMESPACE

class VString;

class XTOOLBOX_API VDocSpanTextRef;
class XTOOLBOX_API IDocSpanTextRef;
class XTOOLBOX_API VDocCache4DExp;


#define UNDEFINED_STYLE -1
typedef uLONG RGBAColor;

#define RGBACOLOR_TRANSPARENT 0

//CSS unit types (alias of XBOX::CSSProperty::eUnit - make sure it is consistent with CSSProperty::eUnit enum)
typedef enum eCSSUnit
{
	kCSSUNIT_PERCENT = 0,			/* percentage (number range is [0.0, 1.0]) */
	kCSSUNIT_PX,					/* pixels */
	kCSSUNIT_PC,					/* picas */
	kCSSUNIT_PT,					/* points */
	kCSSUNIT_MM,					/* millimeters */
	kCSSUNIT_CM,					/* centimeters */
	kCSSUNIT_IN,					/* inchs */
	kCSSUNIT_EM,					/* size of the current font */
	kCSSUNIT_EX,					/* x-height of the current font */
	kCSSUNIT_FONTSIZE_LARGER,		/* larger than the parent node font size (used by FONTSIZE only) */
	kCSSUNIT_FONTSIZE_SMALLER		/* smaller than the parent node font size (used by FONTSIZE only) */
} eCSSUnit;


//CSS helper methods
class XTOOLBOX_API ICSSUtil
{
public:
	/** convert point value to TWIPS 
	@remarks
		round to nearest TWIPS integral value
	*/
	static sLONG PointToTWIPS(Real inValue) { return inValue >= 0 ? floor(inValue*20+0.5f) : -floor((-inValue)*20+0.5f); }
	static Real TWIPSToPoint( sLONG inValue) { return (((Real)inValue)/20.0); }

	/** convert CSS dimension to TWIPS */
	static sLONG CSSDimensionToTWIPS(const Real inDimNumber, const eCSSUnit inDimUnit, const uLONG inDPI = 72, const sLONG inParentValueTWIPS = 0);

	/** convert CSS dimension to point */
	static Real CSSDimensionToPoint(const Real inDimNumber, const eCSSUnit inDimUnit, const uLONG inDPI = 72, const sLONG inParentValueTWIPS = 0)
	{
		//convert first to TWIPS to ensure point will have TWIPS precision
		sLONG twips = CSSDimensionToTWIPS( inDimNumber, inDimUnit, inDPI, inParentValueTWIPS);
		return TWIPSToPoint(twips);
	}

	/** round a number at TWIPS precision using nearest TWIPS integral value (so result is always multiple of 1/20 = 0.05) 
	@remarks 
		it is used to round values in point to TWIPS precision - so that RoundTWIPS(value in point)*20 is always a integer 
	*/
	static Real	RoundTWIPS( const Real inValue) { return TWIPSToPoint(PointToTWIPS( inValue)); } 

	/** round a number at TWIPS precision using ceil of TWIPS value */
	static Real	RoundTWIPSCeil( const Real inValue) { return TWIPSToPoint(ceil(inValue*20)); } 
	/** round a number at TWIPS precision using floor of TWIPS value */
	static Real	RoundTWIPSFloor( const Real inValue) { return TWIPSToPoint(floor(inValue*20)); } 

};

typedef enum eStyleJust
{
	JST_Notset=-1,
	JST_Default=0,
	JST_Left,
	JST_Right,
	JST_Center,
	JST_Mixed,
	JST_Justify,
	JST_Baseline,		//used only for embedded image vertical alignment
	JST_Superscript,	//used only for embedded image vertical alignment
	JST_Subscript,		//used only for embedded image vertical alignment
	JST_Top = JST_Left,
	JST_Bottom = JST_Right
} eStyleJust;


class XTOOLBOX_API VTextStyle : public VObject
{
typedef VObject inherited;
public:
	/** style mask enum type definition (alias VFrameTextEdit::ENTE_StyleAttributs) */
	typedef enum eStyle
	{
		eStyle_Font			= 1,
		eStyle_FontSize		= 2, 
		eStyle_Bold			= 4,
		eStyle_Italic		= 8,
		eStyle_Underline	= 16,
		eStyle_Strikeout	= 32,
		eStyle_Color		= 64,
		eStyle_Just			= 128,
		eStyle_BackColor	= 256,
		eStyle_Ref			= 512,

		eStyle_MaskFont		= eStyle_Font|
							  eStyle_FontSize,

		eStyle_MaskFace		= eStyle_Bold|
							  eStyle_Italic|
							  eStyle_Underline|
							  eStyle_Strikeout, 

		eStyle_MaskColors	= eStyle_Color| 
							  eStyle_BackColor,

		eStyle_Mask			= eStyle_Font|
							  eStyle_FontSize|
							  eStyle_Bold|
							  eStyle_Italic|
							  eStyle_Underline|
							  eStyle_Strikeout| 
							  eStyle_Color| 
							  eStyle_Just|
							  eStyle_BackColor|
							  eStyle_Ref
	} eStyle;

	VTextStyle(const VTextStyle* inTextStyle = NULL);
	virtual		~VTextStyle();
	VTextStyle& operator = ( const VTextStyle& inTextStyle);

	void		SetFontStyleToNormal()
	{
				SetBold(0);
				SetItalic(0);
				SetUnderline(0);
				SetStrikeout(0);
	}

	void		SetRange(sLONG inBegin, sLONG inEnd);
	void		GetRange(sLONG& outBegin, sLONG& outEnd) const;
	
	//Bold , italic et underline et strikeout peuvent avoir les valeur TRUE(1), FALSE(0) ou UNDEFINED_STYLE(-1)
	void		SetBold(sLONG inValue);
	sLONG		GetBold() const;
	void		SetItalic(sLONG inValue);
	sLONG		GetItalic() const;
	void		SetUnderline(sLONG inValue);
	sLONG		GetUnderline() const;
	void		SetStrikeout(sLONG inValue);
	sLONG		GetStrikeout() const;
	void		SetColor(const RGBAColor& inValue);
	RGBAColor	GetColor() const;
	void		SetBackGroundColor(const RGBAColor& inValue);
	RGBAColor	GetBackGroundColor() const;
	void		SetHasBackColor(bool inValue);
	bool		GetHasBackColor() const;
	void		SetHasForeColor(bool inValue);
	bool		GetHasForeColor() const;
	/** set font size 
	@remarks
		it is 4D form compliant font size so on Windows it is actually pixel font size (equal to point size in 4D form where dpi=72)
		
		this font size is equal to font size in 4D form
	*/
	void		SetFontSize(const Real& inValue);
	/** get font size 
	@remarks
		it is 4D form compliant font size so on Windows it is actually pixel font size (equal to point size in 4D form where dpi=72)
		
		this font size is equal to font size in 4D form
	*/
	Real		GetFontSize() const;
	void		SetFontName(const VString& inValue);
	const VString&	GetFontName() const;
	void		SetJustification(eStyleJust inValue);
	eStyleJust	GetJustification() const;

	void		Translate(sLONG inValue);

	/** merge current style with the specified style 
	@remarks
		input style properties override current style properties without taking account input style range
		(current style range is preserved)
	*/
	void		MergeWithStyle( const VTextStyle *inStyle, const VTextStyle *inStyleParent = NULL, bool inForMetricsOnly = false, bool inAllowMergeSpanRefs = false);

	/** append styles from the passed style 
	@remarks
		input style properties override current style properties without taking account input style range
		(current style range is preserved)

		if (inOnlyIfNotSet == true), overwrite only styles which are not defined locally
	*/
	void		AppendStyle( const VTextStyle *inStyle, bool inOnlyIfNotSet = false, bool inAllowMergeSpanRefs = false);

	/** return true if none property is defined (such style is useless & should be discarded) */
	bool		IsUndefined(bool inIgnoreSpanRef = false) const;

	/** return true if this style & the input style are equal */
	bool		IsEqualTo( const VTextStyle &inStyle, bool inIncludeRange = true) const;

	void		Reset();

	/** reset styles if equal to inStyle styles */
	void		ResetCommonStyles( const VTextStyle *inStyle, bool inIgnoreSpanRef = true);

	/** return style which have properties common to this style & the input style 
		and reset properties which are common in the two styles
		(NULL if none property is commun)
	*/
	VTextStyle *ExtractCommonStyle(VTextStyle *ioStyle2);

	bool		IsSpanRef() const { return fSpanTextRef != NULL; }

	/** set and own span text reference */
	void		SetSpanRef( VDocSpanTextRef *inRef); 

	/** get span text reference  */
	VDocSpanTextRef *GetSpanRef() const { return fSpanTextRef; }

protected:
	sLONG	fBegin;
	sLONG	fEnd;
	sLONG	fIsBold;
	sLONG	fIsUnderline;
	sLONG	fIsStrikeout;
	sLONG	fIsItalic;
	RGBAColor	fColor;
	RGBAColor	fBkColor;
	bool	fHasBackColor:1;
	bool	fHasForeColor:1;
	Real	fFontSize;
    VString*	fFontName;
	eStyleJust fJust;

	/** span text reference (like 4D expression, url link, user prop, etc...) */
	VDocSpanTextRef *fSpanTextRef;
};


class XTOOLBOX_API VTreeTextStyle : public VObject, public IRefCountable
{
typedef VObject inherited;
public:
typedef void (*fnBeforeReplace)(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText);
typedef void (*fnAfterReplace)(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced);


	VTreeTextStyle(VTextStyle* inData = NULL);
	VTreeTextStyle(const VTreeTextStyle& inStyle);
	VTreeTextStyle(const VTreeTextStyle* inStyle, bool inDeepCopy = true);
	virtual		~VTreeTextStyle();

	VTreeTextStyle* GetParent()const;
	void AddChild(VTreeTextStyle* inValue, bool inAutoGrowRange = false);
	void InsertChild(VTreeTextStyle* inValue, sLONG pos, bool inAutoGrowRange = false);
	void RemoveChildAt(sLONG inNth);

	void RemoveChildren();

	void SetData(VTextStyle* inValue);
	VTextStyle* GetData () const;

	sLONG GetChildCount() const	;
	VTreeTextStyle* GetNthChild(sLONG inNth) const;

	bool HasSpanRefs() const 
	{ 
		xbox_assert(fNumChildSpanRef >= 0); 
		return fNumChildSpanRef > 0; 
	}

	void Translate(sLONG inValue);

	/** replace styled text at the specified range with text 
	@remarks
		replaced text will inherit only uniform style on replaced range

		return length or replaced text + 1 or 0 if failed (for instance it would fail if trying to replace some part only of a span text reference - only all span reference text might be replaced at once)
	*/
	static VIndex ReplaceStyledText(	VString& ioText, VTreeTextStyle*& ioStyles, const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inStart = 0, sLONG inEnd = -1, 
									bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inInheritUniformStyle = true, const VDocSpanTextRef * inPreserveSpanTextRef = NULL, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);

	static VIndex InsertStyledText(	VString& ioText, VTreeTextStyle*& ioStyles, const VString& inText, const VTreeTextStyle *inStyles = NULL, sLONG inPos = 0, 
									bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false)
	{
		return ReplaceStyledText( ioText, ioStyles, inText, inStyles, inPos, inPos, inAutoAdjustRangeWithSpanRef, inSkipCheckRange);
	}

	static VIndex RemoveStyledText(	VString& ioText, VTreeTextStyle*& ioStyles, sLONG inStart = 0, sLONG inEnd = -1, 
									bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false)
	{
		return ReplaceStyledText( ioText, ioStyles, CVSTR(""), NULL, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange);
	}

	/** return true if position is over a span reference */
	const bool IsOverSpanRef(const sLONG inPos) { return GetStyleRefAtPos( inPos) != NULL; }

	/** return true if range intersects a span reference */
	const bool IsOverSpanRef(const sLONG inStart, const sLONG inEnd) { return GetStyleRefAtRange( inStart, inEnd) != NULL; }

	/** replace text with span text reference on the specified range
	@remarks
		span ref plain text is set here to uniform non-breaking space: 
		user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

		you should no more use or destroy passed inSpanRef even if method returns false
	*/
	static bool ReplaceAndOwnSpanRef( VString& ioText, VTreeTextStyle*& ioStyles, VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = true, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);


	/** insert span text reference at the specified text position 
	@remarks
		span ref plain text is set here to uniform non-breaking space: 
		user should call UpdateSpanRefs to replace span ref plain text by computed value or reference value depending on show ref flag

		you should no more use or destroy passed inSpanRef even if method returns false
	*/
	static bool InsertAndOwnSpanRef( VString& ioText, VTreeTextStyle*& ioStyles, VDocSpanTextRef* inSpanRef, sLONG inPos = 0, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = true, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);

	/** convert uniform character index (the index used by 4D commands) to actual plain text position 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void CharPosFromUniform( sLONG& ioPos) const;

	/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void CharPosToUniform( sLONG& ioPos) const;

	/** convert uniform character range (as used by 4D commands) to actual plain text position 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void RangeFromUniform( sLONG& inStart, sLONG& inEnd) const;

	/** convert actual plain text range to uniform range (as used by 4D commands) 
	@remarks
		in order 4D commands to use uniform character position while indexing text containing span references 
		(span ref content might vary depending on computed value or showing source or value)
		we need to map actual plain text position with uniform char position:
		uniform character position assumes a span ref has a text length of 1 character only
	*/
	void RangeToUniform( sLONG& inStart, sLONG& inEnd) const;

	/** adjust selection 
	@remarks
		you should call this method to adjust selection if styles contain span references 
	*/
	void AdjustRange(sLONG& ioStart, sLONG& ioEnd) const;

	/** adjust char pos
	@remarks
		you should call this method to adjust char pos if styles contain span references 
	*/
	void AdjustPos(sLONG& ioPos, bool inToEnd = true) const;

	/** return span reference at the specified text position if any */
	const VTextStyle *GetStyleRefAtPos(const sLONG inPos) const;

	/** return the first span reference which intersects the passed range */
	const VTextStyle *GetStyleRefAtRange(const sLONG inStart = 0, const sLONG inEnd = -1) const;

	/** show or hide span ref value or source
	@remarks
		propagate flag to all VDocSpanTextRef & mark all as dirty

		false: show computed value (4D expression result, url link, user prop value)
		true: show reference (4D tokenized expression, url texte, user prop name) 
	*/ 
	void ShowSpanRefs( bool inShowSpanRefs = true);

	/** highlight or not span references 
	@remarks
		propagate flag to all VDocSpanTextRef 

		on default (if VDocSpanTextRef delegate is not overriden) it changes only highlight status for value
		because if reference is displayed, it is always highlighted

		caller should redraw text after calling this method to ensure new style is propertly applied
	*/ 
	void ShowHighlight( bool inShowHighlight = true);

	/** call this method to update span reference style while mouse enters span ref 
	@remarks
		return true if style has changed
	*/
	bool NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef);

	/** call this method to update span reference style while mouse leaves span ref 
	@remarks
		return true if style has changed
	*/
	bool NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef);

	/** update span references plain text  
	@remarks
		parse & update span references plain text  

		return true if plain text or styles have been updated

		this method should be called on control event DoIdle in order to update dirty span text references plain text
		(range might be constrained to text visible range only)
	*/
	static bool UpdateSpanRefs( VString& ioText, VTreeTextStyle& ioStyles, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);

	/** freeze span references  
	@remarks
		reset styles span references

		this method preserves VTextStyle span reference container: it only resets VTextStyle span reference, transforming it to a normal VTextStyle
	*/
	void FreezeSpanRefs( sLONG inStart = 0, sLONG inEnd = -1);

	/** freeze expressions  
	@remarks
		parse & replace any 4D expression reference with evaluated expression plain text & discard 4D expression reference on the passed range 

		return true if any 4D expression has been replaced with plain text
	*/
	static bool FreezeExpressions( VDBLanguageContext *inLC, VString& ioText, VTreeTextStyle& ioStyles, sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, fnBeforeReplace inBeforeReplace = NULL, fnAfterReplace inAfterReplace = NULL);

	/** reset cached 4D expressions on the specified range */
	void ResetCacheExpressions( VDocCache4DExp *inCache4DExp, sLONG inStart = 0, sLONG inEnd = -1);

	/** eval 4D expressions  
	@remarks
		return true if any 4D expression has been evaluated
	*/
	bool EvalExpressions( VDBLanguageContext *inLC, sLONG inStart = 0, sLONG inEnd = -1, VDocCache4DExp *inCache4DExp = NULL);

	bool HasExpressions() const;

	/** expand styles ranges from the specified range position by the specified offset 
	@remarks
		should be called just after text has been inserted in a multistyled text
		(because inserted text should inherit style at current inserted text position)
		for instance:
			text.Insert(textToInsert, 10); //caution: here 1-based index
			styles->ExpandRangesAtPosBy( 9, textToInsert.GetLength()); //caution: here 0-based index so 10-1 and not 10
	*/
	void ExpandAtPosBy(	sLONG inStart, sLONG inInflate = 1, const VDocSpanTextRef *inInflateSpanRef = NULL, VTextStyle **outStyleToPostApply = NULL, 
						bool inOnlyTranslate = false,
						bool inIsStartOfParagraph = false);

	/** truncate the specified style range
	@param inStart
		start of truncated range
	@param inLength
		length of truncated range (-1 to truncate from inStart to the end of this VTreeTextStyle range)
	@remarks
		should be called just after text has been deleted in a multistyled text
		for instance:
			text.Remove(10, 5); //caution: here 1-based index
			styles->Truncate(9, 5); //caution: here 0-based index so 10-1 and not 10
	*/
	void Truncate(sLONG inStart, sLONG inLength = -1, bool inRemoveStyleIfEmpty = true, bool inPreserveSpanRefIfEmpty = false);

	/** return true if plain text range can be deleted or replaced 
	@remarks
		return false if range overlaps span text references plain text & if range does not fully includes span text reference(s) plain text
		(we cannot delete or replace some sub-part of span text reference plain text but only full span text reference plain text at once)
	*/
	bool CanDeleteOrReplace( sLONG inStart, sLONG inEnd);

	/** return true if style can be applied 
	@remarks
		return false if it is conflicting with span references (span references cannot override)	
	*/
	bool CanApplyStyle( const VTextStyle *inStyle, sLONG inStartBase = 0) const;

	/** build a vector of sequential styles from the current cascading styles 
	@remarks
		output styles ranges are disjoint & n+1 style range follows immediately n style range

		this method does not modify current VTreeTextStyle
	*/
	void BuildSequentialStylesFromCascadingStyles( std::vector<VTextStyle *>& outStyles, VTextStyle *inStyleParent = NULL, bool inForMetrics = false, bool inFirstUniformStyleOnly = false) const;

	/** convert cascading styles to sequential styles 
	@remarks
		cascading styles are merged in order to build a tree which has only one level deeper;
		on output, child VTreeTextStyle ranges will be disjoint & n+1 style range follows immediately n style range
	*/
	void BuildSequentialStylesFromCascadingStyles( VTextStyle *inStyleParent = NULL, bool inFirstUniformStyleOnly = false);

	/** build cascading styles from the input sequential styles 
	@remarks
		input styles ranges should not overlap  

		this methods overrides current styles with the input styles
		this class owns input VTextStyle instances so caller should not delete it
	*/
	void BuildCascadingStylesFromSequentialStyles( std::vector<VTextStyle *>& inStyles);

	/** create new tree text style from the current but constrained to the specified range & eventually relative to the start of the specified range 
	@param inStart
		start of new range 
	@param inEnd
		end of new range 
	@param inRelativeToStart
		if true, new range is translated by -inStart
	@param inNULLIfRangeEmpty
		if true, return NULL if new range is empty 
	*/ 
	VTreeTextStyle *CreateTreeTextStyleOnRange( const sLONG inStart, const sLONG inEnd, bool inRelativeToStart = true, bool inNULLIfRangeEmpty = true, bool inNoMerge = false) const;

	/** apply style 
	@param inStyle
		style to apply
	@remarks
		caller is owner of inStyle
	*/
	void ApplyStyle( const VTextStyle *inStyle, bool inIgnoreIfEmpty = true, sLONG inStartBase = 0);

	/** apply styles 
	@param inStyle
		styles to apply
	@remarks
		caller is owner of inStyle
	*/
	void ApplyStyles( const VTreeTextStyle *inStyles, bool inIgnoreIfEmpty = true, sLONG inStartBase = 0);

	/** create & return uniform style on the specified range 
	@remarks
		undefined value is set for either mixed or undefined style 

		mixed status is returned optionally in outMaskMixed using VTextStyle::Style mask values
		
		return value is always not NULL
	*/
	VTextStyle *CreateUniformStyleOnRange( sLONG inStart, sLONG inEnd, bool inRelativeToStart = true, uLONG *outMaskMixed = NULL) const;

	/** get font name on the specified range 
	@return
		true: font name is used on all range
		false: font name is mixed: return font name of the font used by range start character
	*/
	bool		GetFontName(sLONG inStart, sLONG inEnd, VString& ioFontName) const;

	/** get font size on the specified range 
	@return
		true: font size is used on all range
		false: font size is mixed: return font size of the font used by range start character

		caution: for compatibility, returned font size is in 4D form compliant font size (72dpi)
				 so it is actually font size in pixel on Windows: see VTextStyle
	*/
	bool		GetFontSize(sLONG inStart, sLONG inEnd, Real& ioFontSize) const;

	/** get text color on the specified range 
	@return
		true: text color is used on all range
		false: text color is mixed: return text color of the font used by range start character
	*/
	bool		GetTextForeColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const;

	/** get back text color on the specified range 
	@return
		true: back text color is used on all range
		false: back text color is mixed: return back text color of the font used by range start character
	*/
	bool		GetTextBackColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const;

	/** get font weight on the specified range 
	@return
		true: font weight is used on all range
		false: font weight is mixed: return font weight of the font used by range start character
	*/
	bool		GetBold( sLONG inStart, sLONG inEnd, sLONG& ioBold) const;

	/** get font italic on the specified range 
	@return
		true: font italic is used on all range
		false: font italic is mixed: return font italic of the font used by range start character
	*/
	bool		GetItalic(sLONG inStart, sLONG inEnd, sLONG& ioItalic) const;

	/** get font underline on the specified range 
	@return
		true: font underline is used on all range
		false: font underline is mixed: return font underline of the font used by range start character
	*/
	bool		GetUnderline(sLONG inStart, sLONG inEnd, sLONG& ioUnderline) const;

	/** get font strikeout on the specified range 
	@return
		true: font strikeout is used on all range
		false: font strikeout is mixed: return font strikeout of the font used by range start character
	*/
	bool		GetStrikeout(sLONG inStart, sLONG inEnd, sLONG& ioStrikeOut) const;

	/** get justification on the specified range 
	@return
		true: justification is used on all range
		false: justification is mixed: return justification used by range start character
	*/
	bool		GetJustification(sLONG inStart, sLONG inEnd, eStyleJust& ioJust) const;

	/** return true if it is undefined */
	bool		IsUndefined(bool inTrueIfRangeEmpty = false) const;

	/** return true if font family is defined */
	bool		HasFontFamilyOverride() const;

	void		NotifySetSpanRef() { if (fParent) fParent->fNumChildSpanRef++; }
	void		NotifyClearSpanRef() { if (fParent) fParent->fNumChildSpanRef--; }
protected:
	/** merge current style recursively with the specified style 
	@remarks
		input style properties which are defined override current style properties without taking account input style range
		(current style range is preserved)
	*/
	void _MergeWithStyle( const VTextStyle *inStyle, VTextStyle *inStyleInherit = NULL);

	/** apply style 
	@param inStyle
		style to apply
	@remarks
		caller is owner of inStyle
	*/
	void _ApplyStyleRec( const VTextStyle *inStyle, sLONG inRecLevel = 0, VTextStyle *inStyleInherit = NULL, bool inIgnoreIfEmpty = true, sLONG inStartBase = 0);

	void SetParent(VTreeTextStyle* inValue);

	typedef std::vector<VTreeTextStyle*>	VectorOfStyle;
	VectorOfStyle		fChildren;
	VTreeTextStyle*		fParent;
	VTextStyle*			fdata;

	VIndex				fNumChildSpanRef;
};

class VDocNode;
class VDocText;
class VDocParagraph;

class ISpanTextParser
{
public:
#if VERSIONWIN
virtual float GetSPANFontSizeDPI() const = 0;
#endif

/** return true if character is valid XML 1.1 
	(some control & special chars are not valid like [#x1-#x8], [#xB-#xC], [#xE-#x1F], [#x7F-#x84], [#x86-#x9F] - cf W3C) */
virtual bool IsXMLChar(UniChar inChar) const = 0;

/** convert plain text to XML 
@param ioText
	text to convert
@param inFixInvalidChars
	if true (default), fix invalid XML chars - see remarks below
@param inEscapeXMLMarkup
	if true (default), escape XML markup (<,>,',",&) 
@param inConvertCRtoBRTag
	if true (default is false), it will convert any end of line to <BR/> XML tag
@param inConvertForCSS
	if true, convert "\" to "\\" because in CSS '\' is used for escaping characters (!)  
@param inEscapeCR
	if true, escapes CR to "\0D" in CSS or "&#xD;" in XML (exclusive with inConvertCRtoBRTag) - should be used only while serializing XML or CSS strings
@remarks
	see IsXMLChar 

	note that end of lines are normalized to one CR only (0x0D) - if end of lines are not converted to <BR/>

	if inFixInvalidChars == true, invalid chars are converted to whitespace (but for 0x0B which is replaced with 0x0D) 
	otherwise Xerces would fail parsing invalid XML 1.1 characters - parser would throw exception
*/
virtual void TextToXML(VString& ioText, bool inFixInvalidChars = true, bool inEscapeXMLMarkup = true, bool inConvertCRtoBRTag = false, bool inConvertForCSS = false, bool inEscapeCR = false) = 0;

/** parse span text element or XHTML fragment 
@param inTaggedText
	tagged text to parse (span element text only if inParseSpanOnly == true, XHTML fragment otherwise)
@param outStyles
	parsed styles
@param outPlainText
	parsed plain text
@param inParseSpanOnly
	true (default): input text is 4D SPAN tagged text (version depending on "d4-version" CSS property) - 4D CSS rules apply (all span character styles are inherited)
	false: input text is XHTML 1.1 fragment (generally from external source than 4D) - W3C CSS rules apply (some CSS styles like "text-decoration" & "background-color" are not inherited)
@param inDPI
	dpi used to convert from pixel to point
@param inShowSpanRefs
	false (default): after call to VDocText::UpdateSpanRefs, plain text would contain span ref computed values if any (4D expression result, url link user text, etc...)
	true: after call to VDocText::UpdateSpanRefs, plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
@param inNoUpdateRefs
	false: replace plain text by source or computed span ref values after parsing (call VDocText::UpdateSpanRefs)
	true (default): span refs plain text is set to only one unbreakable space

CAUTION: this method returns only parsed character styles: you should use now ParseDocument method which returns a VDocText * if you need to preserve paragraph or document properties
*/
virtual void ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly = true, const uLONG inDPI = 72, 
							bool inShowSpanRefs = false, bool inNoUpdateRefs = true, VDBLanguageContext *inLC = NULL) = 0;

/** generate span text from text & character styles 

CAUTION: this method will serialize text align for compat with v13 but you should use now SerializeDocument (especially if text has been parsed before with ParseDocument)
*/
virtual void GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle = NULL) =0;

/** get plain text from span text 
@remarks
	span references are evaluated on default
*/
virtual void GetPlainTextFromSpanText(	const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly = true, 
										bool inShowSpanRefs = false, bool inNoUpdateRefs = false, VDBLanguageContext *inLC = NULL) =0;

/** replace styled text at the specified range with text 
@remarks
	if inInheritUniformStyle == true, replaced text will inherit uniform style on replaced range (true on default)
*/
virtual bool ReplaceStyledText(	VString& ioSpanText, const VString& inSpanTextOrPlainText, sLONG inStart = 0, sLONG inEnd = -1, bool inTextPlainText = false, bool inInheritUniformStyle = true, bool inSilentlyTrapParsingErrors = true) = 0;

/** replace styled text with span text reference on the specified range
	
	you should no more use or destroy passed inSpanRef even if method returns false
*/
virtual bool ReplaceAndOwnSpanRef( VString& ioSpanText, VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true) = 0;

/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range 

	return true if any 4D expression has been replaced with plain text
*/
virtual bool FreezeExpressions( VDBLanguageContext *inLC, VString& ioSpanText, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true) = 0;

/** return the first span reference which intersects the passed range 
@return
	eDocSpanTextRef enum value or -1 if none ref found
*/
virtual sLONG GetReferenceAtRange(	VDBLanguageContext *inLC,
								    const VString& inSpanText, 
									const sLONG inStart = 0, const sLONG inEnd = -1, 
									sLONG *outFirstRefStart = NULL, sLONG *outFirstRefEnd = NULL, 
									VString *outFirstRefSource  = NULL, VString *outFirstRefValue = NULL
									) = 0;

//document parsing

/** parse text document 
@param inDocText
	span or xhtml text
@param inVersion
	span or xhtml version (might be overriden by -d4-version)
@param inDPI
	dpi used to convert from pixel to point
@param inShowSpanRefs
	false (default): after call to VDocText::UpdateSpanRefs, plain text would contain span ref computed values if any (4D expression result, url link user text, etc...)
	true: after call to VDocText::UpdateSpanRefs, plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
@param inNoUpdateRefs
	false: replace plain text by source or computed span ref values after parsing (call VDocText::UpdateSpanRefs)
	true (default): span refs plain text is set to only one unbreakable space
@remarks
	on default, document format is assumed to be V13 compatible (but version might be modified if "d4-version" CSS prop is present)
*/
virtual VDocText *ParseDocument(	const VString& inDocText, const uLONG inVersion = kDOC_VERSION_SPAN4D, const uLONG inDPI = 72, 
									bool inShowSpanRefs = false, bool inNoUpdateRefs = true, bool inSilentlyTrapParsingErrors = true, VDBLanguageContext *inLC = NULL) = 0;

/** initialize VTextStyle from CSS declaration string */ 
virtual void ParseCSSToVTextStyle( VDBLanguageContext *inLC, VDocNode *ioDocNode, const VString& inCSSDeclarations, VTextStyle &outStyle, const VTextStyle *inStyleInherit = NULL) = 0;

/** initialize VDocNode props from CSS declaration string */ 
virtual void ParseCSSToVDocNode( VDocNode *ioDocNode, const VString& inCSSDeclarations) = 0;

/** parse document CSS property */	
virtual bool ParsePropDoc( VDocText *ioDocText, const uLONG inProperty, const VString& inValue) = 0;

/** parse paragraph CSS property */	
virtual bool ParsePropParagraph( VDocParagraph *ioDocPara, const uLONG inProperty, const VString& inValue) = 0;

/** parse common CSS property */	
virtual bool ParsePropCommon( VDocNode *ioDocNode, const uLONG inProperty, const VString& inValue) = 0;

/** parse span CSS style */	
virtual bool ParsePropSpan( VDocNode *ioDocNode, const uLONG inProperty, const VString& inValue, VTextStyle &outStyle, VDBLanguageContext *inLC = NULL) = 0;


//document serializing

/** serialize document (formatting depending on version) 
@remarks
	//kDOC_VERSION_SPAN_1 format compatible with v13:
	//  - span nodes are parsed for character styles only
	//
	if (inDocParaDefault != NULL): only properties and character styles different from the passed paragraph properties & character styles will be serialized
*/
virtual void SerializeDocument( const VDocText *inDoc, VString& outDocText, const VDocParagraph *inDocParaDefault = NULL) = 0;
};

END_TOOLBOX_NAMESPACE

#endif
