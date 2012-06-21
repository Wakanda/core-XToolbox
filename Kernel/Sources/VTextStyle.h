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

BEGIN_TOOLBOX_NAMESPACE

class VString;

#define UNDEFINED_STYLE -1
typedef uLONG RGBAColor;

enum justificationStyle
{
	JST_Notset=-1,
	JST_Default=0,
	JST_Left,
	JST_Right,
	JST_Center,
	JST_Mixed,
	JST_Justify
};

class XTOOLBOX_API VTextStyle : public VObject
{
typedef VObject inherited;
public:
	VTextStyle(VTextStyle* inTextStyle = NULL);
	virtual		~VTextStyle();
	VTextStyle& operator = ( const VTextStyle& inTextStyle);

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
	void		SetTransparent(bool inValue);
	bool		GetTransparent() const;
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
	void		SetJustification(justificationStyle inValue);
	justificationStyle	GetJustification() const;

	void		Translate(sLONG inValue);

	/** merge current style with the specified style 
	@remarks
		input style properties override current style properties without taking account input style range
		(current style range is preserved)
	*/
	void		MergeWithStyle( const VTextStyle *inStyle, const VTextStyle *inStyleParent = NULL);

	/** return true if none property is defined (such style is useless & should be discarded) */
	bool		IsUndefined() const;

	/** return true if this style & the input style are equal */
	bool		IsEqualTo( const VTextStyle &inStyle, bool inIncludeRange = true) const;

	void		Reset();

	/** return style which have properties common to this style & the input style 
		and reset properties which are common in the two styles
		(NULL if none property is commun)
	*/
	VTextStyle *ExtractCommonStyle(VTextStyle *ioStyle2);
protected:


	sLONG	fBegin;
	sLONG	fEnd;
	sLONG	fIsBold;
	sLONG	fIsUnderline;
	sLONG	fIsStrikeout;
	sLONG	fIsItalic;
	RGBAColor	fColor;
	RGBAColor	fBkColor;
	bool	fIsTransparent:1;
	bool	fHasForeColor:1;
	Real	fFontSize;
    VString*	fFontName;
	justificationStyle fJust;
};


class XTOOLBOX_API VTreeTextStyle : public VObject, public IRefCountable
{
typedef VObject inherited;
public:
	VTreeTextStyle(VTextStyle* inData = NULL);
	VTreeTextStyle(VTreeTextStyle* inStyle, bool inDeepCopy = true);
	virtual		~VTreeTextStyle();

	VTreeTextStyle* GetParent()const;
	void AddChild(VTreeTextStyle* inValue, bool inAutoGrowRange = false);
	void InsertChild(VTreeTextStyle* inValue, sLONG pos, bool inAutoGrowRange = false);
	void RemoveChildAt(sLONG inNth);

	void SetData(VTextStyle* inValue);
	VTextStyle* GetData () const;

	sLONG GetChildCount() const	;
	VTreeTextStyle* GetNthChild(sLONG inNth) const;

	void Translate(sLONG inValue);

	/** expand styles ranges from the specified range position by the specified offset 
	@remarks
		should be called just after text has been inserted in a multistyled text
		(because inserted text should inherit style at current inserted text position)
		for instance:
			text.Insert(textToInsert, 10); //caution: here 1-based index
			styles->ExpandRangesAtPosBy( 9, textToInsert.GetLength()); //caution: here 0-based index so 10-1 and not 10
	*/
	void ExpandAtPosBy(sLONG inStart, sLONG inInflate = 1);

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
	void Truncate(sLONG inStart, sLONG inLength = -1);

	/** build a vector of sequential styles from the current cascading styles 
	@remarks
		output styles ranges are disjoint & n+1 style range follows immediately n style range

		this method does not modify current VTreeTextStyle
	*/
	void BuildSequentialStylesFromCascadingStyles( std::vector<VTextStyle *>& outStyles, VTextStyle *inStyleParent = NULL, bool inForMetrics = false);

	/** convert cascading styles to sequential styles 
	@remarks
		cascading styles are merged in order to build a tree which has only one level deeper;
		on output, child VTreeTextStyle ranges will be disjoint & n+1 style range follows immediately n style range
	*/
	void BuildSequentialStylesFromCascadingStyles( VTextStyle *inStyleParent = NULL);

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
	VTreeTextStyle *CreateTreeTextStyleOnRange( const sLONG inStart, const sLONG inEnd, bool inRelativeToStart = true, bool inNULLIfRangeEmpty = true);

	/** apply style 
	@param inStyle
		style to apply
	@remarks
		caller is owner of inStyle
	*/
	void ApplyStyle( VTextStyle *inStyle);

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
	bool		GetJustification(sLONG inStart, sLONG inEnd, justificationStyle& ioJust) const;

	/** return true if it is undefined */
	bool		IsUndefined(bool inTrueIfRangeEmpty = false) const;

	/** return true if font family is defined */
	bool		HasFontFamilyOverride() const;

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
	void _ApplyStyleRec( VTextStyle *inStyle, sLONG inRecLevel = 0, VTextStyle *inStyleInherit = NULL);

	void SetParent(VTreeTextStyle* inValue);

	typedef std::vector<VTreeTextStyle*>	VectorOfStyle;
	VectorOfStyle		fChildren;
	VTreeTextStyle*		fParent;
	VTextStyle*			fdata;
};


class ISpanTextParser
{
public:

#if VERSIONWIN
virtual float GetDPI() const = 0;
#endif

/** parse span text element or XHTML fragment 
@param inTaggedText
	tagged text to parse (span element text only if inParseSpanOnly == true, XHTML fragment otherwise)
@param outStyles
	parsed styles
@param outPlainText
	parsed plain text
@param inParseSpanOnly
	true (default): only <span> element(s) with CSS styles are parsed
	false: all XHTML text fragment is parsed (parse also mandatory HTML text styles)
*/
virtual void ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly = true) =0;
virtual void GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle = NULL) =0;
virtual void GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly = true) =0;
virtual void ConverUniformStyleToList(const VTreeTextStyle* inUniformStyles, VTreeTextStyle* ioListUniformStyles) =0;
};

END_TOOLBOX_NAMESPACE

#endif
