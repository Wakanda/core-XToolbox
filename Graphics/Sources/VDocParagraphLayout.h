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

/** paragraph layout
@remarks
	this class is dedicated to manage platform-independent (ICU & platform-independent algorithm) paragraph layout
	only layout basic runs (same direction & style) are still computed & rendered with native impl - GDI (legacy layout mode) or DWrite on Windows, CoreText on Mac

	text might be made of a single paragraph or multiple paragraphs (but with common paragraph style - suitable to render for instance a single HTML paragraph)

	this class is embedded (for now) in VTextLayout & stores a reference to the parent VTextLayout instance: VTextLayout internally instanciates it 
	if and only if VTextLayout::ShouldUseNativeLayout() is false (true on default for compatibility with v14)
	TODO: when VDocumentLayout is created, VDocumentLayout should embed this class instance(s) & share common interface with VTextLayout (this latter class being obsolete in v15+ - but preserved for compat)
		  (for now we still embed it in VTextLayout in order to test features with VStyledTextEditView)

	important: all metrics (ascent for instance) here are in res-independent point 
	(onscreen, it is equal to pixel on Windows & Mac platform as 1pt=1px onscreen in 4D form on Windows & Mac:
	 if 4D form becomes later dpi-aware on Windows & Mac, as form should be zoomed to screen dpi/72 as for printing, it will not require modifications 
	 so it is important to use from now res-independent metrics in order to be consistent with high-dpi in v15 - and to ease porting)

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
			- images
	 TODO:  
			- bullets
			- tables
			- etc...
*/
class	VDocParagraphLayout: public VDocBaseLayout 
{
public:
virtual						~VDocParagraphLayout();

/////////////////	IDocNodeLayout interface

		/** update text layout metrics */ 
virtual	void				UpdateLayout(VGraphicContext *inGC);
							
		/** render paragraph in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual	void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f);

/////////////////

/////////////////	VDocBaseLayout override

		/** initialize from the passed node */
virtual	void				InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited = false);

		/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true);

		/** set text align - CSS text-align (-1 for not defined) */
virtual void				SetTextAlign( const eStyleJust inHAlign); 

		/** set vertical align - CSS vertical-align (-1 for not defined) */
virtual void				SetVerticalAlign( const eStyleJust inVAlign);

		/** set width (in twips) - CSS width (0 for auto) */
virtual	void				SetWidth(const uLONG inWidth);

		/** set height (in twips) - CSS height (0 for auto) */
virtual	void				SetHeight(const uLONG inHeight);

/////////////////

		void				GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false);

		void				GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1);

		void				GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);

		bool				GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength = NULL);

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

		//following methods are useful for fast text replacement (only the modified paragraph(s) are fully computed again)

		/** insert text at the specified position */
		void				InsertText( sLONG inPos, const VString& inText) { ReplaceText( inPos, inPos, inText); }

		/** delete text range */
		void				DeleteText( sLONG inStart = 0, sLONG inEnd = -1) { ReplaceText( inStart, inEnd, VString("")); }

		/** replace text range */
		void				ReplaceText( sLONG inStart, sLONG inEnd, const VString& inText);

		/** apply character style (use style range) */
		void				ApplyStyle( const VTextStyle* inStyle);

protected:
		friend class		VTextLayout;

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
					it might not be equal to ascent+descent if VTextLayout line height is not set to 100% 
					(e.g. actual line height is (ascent+descent)*2 if VTextLayout line height is set to 200%
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
				const VIntlMgr::BidiRuns&	GetBidiRuns() const { return fBidiRuns; }

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
				VIntlMgr::BidiRuns	fBidiRuns;
				VIntlMgr::BidiVisualToLogicalRunIndexMap fBidiVisualToLogicalMap;

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

							VDocParagraphLayout(VTextLayout *inLayout); //only VTextLayout can instanciate that class

		void				_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw = false);

		/** get line start position relative to text box left top origin */
		void				_GetLineStartPos( const Line* inLine, VPoint& outPos);

		/** get paragraph from character index (0-based index) */
		VParagraph*			_GetParagraphFromCharIndex(const sLONG inCharIndex);
		Line*				_GetLineFromCharIndex(const sLONG inCharIndex);
		Run*				_GetRunFromCharIndex(const sLONG inCharIndex);

		/** get paragraph from local position (relative to text box origin+vertical align decal & res-independent - 72dpi)*/
		VParagraph*			_GetParagraphFromLocalPos(const VPoint& inPos);
		Line*				_GetLineFromLocalPos(const VPoint& inPos);
		Run*				_GetRunFromLocalPos(const VPoint& inPos, VPoint &outLineLocalPos);

		VGraphicContext*	_RetainComputingGC( VGraphicContext *inGC);

		typedef std::vector< VRefPtr<VParagraph> > Paragraphs;

		//design datas


		/** ref on text layout (not retained as fLayout is owner of this class) */
		VTextLayout*		fLayout;


		//computed datas


		/** computing gc */
		VGraphicContext*	fComputingGC;

		/** computed paragraphs */
		Paragraphs			fParagraphs;

		/** layout bounds (alias for design bounds) in px - relative to fDPI - including margin, padding & border */
		VRect				fLayoutBounds;

		/** typographic bounds in px - relative to fDPI - including margin, padding & border 
		@remarks
			formatted text bounds (minimal bounds to display all text with margin, padding & border)
		*/
		VRect				fTypoBounds;

		/** margins in px - relative to fDPI 
		@remarks
			same as fAllMargin but rounded according to gc pixel snapping (as we align first text run according to pixel snapping rule)
		*/
		VTextLayout::sRectMargin	fMargin;

		/** current max width - without margins - in TWIPS */
		sLONG				fCurMaxWidthTWIPS;

		/** recursivity counter */
		sLONG				fRecCount;

}; //VDocParagraphLayout


END_TOOLBOX_NAMESPACE

#endif
