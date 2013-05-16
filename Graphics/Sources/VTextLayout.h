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
#ifndef __VTextLayout__
#define __VTextLayout__

BEGIN_TOOLBOX_NAMESPACE

// Forward declarations

class	VAffineTransform;
class	VRect;
class	VRegion;
class 	VTreeTextStyle;
class 	VStyledTextBox;

#if ENABLE_D2D
class XTOOLBOX_API VWinD2DGraphicContext;
#endif
#if VERSIONMAC
class XTOOLBOX_API VMacQuartzGraphicContext;
#endif

/** text layout class
@remarks
	this class manages a text layout & optionally pre-rendering of its content in a offscreen layer (if offscreen layer is enabled)

	for big text, it is recommended to use this class to draw text or get text metrics in order to optimize speed
				  because layout is preserved as long as layout properties or passed gc is compatible with the last used gc

	You should call at least SetText method to define the text & optionally styles; all other layout properties have default values;

	to draw to a VGraphicContext, use method 'Draw( gc, posTopLeft)'
	you can get computed layout metrics for a specified gc with 'GetLayoutBounds( gc, bounds, posTopLeft)' 
	you can get also computed run metrics for a specified gc with 'GetRunBoundsFromRange( gc, vectorOfBounds, posTopLeft, start, end)';
	there are also methods to get caret metrics from pos or get char index from pos.

	If layout cache is enabled, this class preserves text layout & metrics as long as the passed gc is compatible with the last used gc;
	optionally it can also pre-render the text layout in a offscreen layer compatible with the passed gc;
	on default only text layout is preserved: you should enable the offscreen layer only on a per-case basis.

	On Windows, it is safe to draw a VTextLayout to any graphic context but it is recommended whenever possible to draw to the same kind of graphic context 
	(ie a GDI, a GDIPlus, a hardware Direct2D gc or a software Direct2D gc) otherwise as layout or layer need to be computed again if gc kind has changed, 
	caching mecanism would not be useful if gc kind is changed at every frame... 

	This class is NOT thread-safe (as 4D GUI is single-threaded, it should not be a pb as this class should be used only by GUI component & GUI users)

	caution: note here that all passed/returned metrics are in pixel if unit is not explicitly specified :
	it is equal to point if dpi == 72 (default dpi in 4D forms)
*/
class XTOOLBOX_API VTextLayout: public VObject, public IRefCountable
{
public:
			/** VTextLayout constructor 
			
			@param inShouldEnableCache
				enable/disable cache (default is true): see ShouldEnableCache
			@note
				on Windows, if graphic context is GDI graphic context, offscreen layer is disabled because offscreen layers are not implemented in this context
			*/
								VTextLayout(bool inShouldEnableCache = true);
	virtual						~VTextLayout();

			/** return true if layout is valid/not dirty */
			bool				IsValid() const { return fLayoutIsValid; }

			/** set dirty
			@remarks
				use this method if text owner is caller and caller has modified text without using VTextLayout methods
				or if styles owner is caller and caller has modified styles without using VTextLayout methods
			*/
			void				SetDirty() { fLayoutIsValid = false; }

			bool				IsDirty() const { return !fLayoutIsValid; }

			/** enable/disable cache
			@remarks
				true (default): VTextLayout will cache impl text layout & optionally rendered text content on multiple frames
								in that case, impl layout & text content are computed/rendered again only if gc passed to methods is no longer compatible with the last used gc 
								or if text layout is dirty;

				false:			disable layout cache
				
				if VGraphicContext::fPrinterScale == true - ie while printing, cache is automatically disabled so you can enable cache even if VTextLayout instance is used for both screen drawing & printing
				(but it is not recommended: you should use different layout instances for painting onscreen or printing)
			*/
			void				ShouldEnableCache( bool inShouldEnableCache)
			{
				VTaskLock protect(&fMutex);
				if (fShouldEnableCache == inShouldEnableCache)
					return;
				fShouldEnableCache = inShouldEnableCache;
				ReleaseRefCountable(&fLayerOffScreen);
				fLayerIsDirty = true;
			}
			bool				ShouldEnableCache() const { return fShouldEnableCache; }


			/** allow/disallow offscreen text rendering (default is false)
			@remarks
				text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
				you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
			@see
				ShouldEnableCache
			*/
			void				ShouldDrawOnLayer( bool inShouldDrawOnLayer)
			{
				VTaskLock protect(&fMutex);
				if (fShouldDrawOnLayer == inShouldDrawOnLayer)
					return;
				fShouldDrawOnLayer = inShouldDrawOnLayer;
				fLayerIsDirty = true;
				if (fGC && !fShouldDrawOnLayer && !fIsPrinting)
					_ResetLayer();
			}
			bool				ShouldDrawOnLayer() const { return fShouldDrawOnLayer; }

			/**	VTextLayout constructor
			@param inPara
				VDocParagraph source: the passed paragraph is used only for initializing layout plain text, styles & paragraph properties
									  but inPara is not retained 
				(inPara plain text might be actually contiguous text paragraphs sharing same paragraph style)
			@param inShouldEnableCache
				enable/disable cache (default is true): see ShouldEnableCache
			*/
								VTextLayout(const VDocParagraph *inPara, bool inShouldEnableCache = true);

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
			void				ApplyParagraphStyle( const VDocParagraph *inPara, bool inResetStyles = false, bool inApplyParagraphPropsOnly = false);

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
			void				ResetParagraphStyle(bool inResetParagraphPropsOnly = false);

			/** replace text with span text reference on the specified range
			@remarks
				it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on show ref or not flag

				you should no more use or destroy passed inSpanRef even if method returns false
			*/
			bool				ReplaceAndOwnSpanRef(	VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inAutoAdjustRangeWithSpanRef = true, bool inSkipCheckRange = false, bool inNoUpdateRef = false, 
														void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

			/** return span reference at the specified text position if any */
			const VTextStyle*	GetStyleRefAtPos(const sLONG inPos)
			{
				VTaskLock protect(&fMutex);
				return fStyles ? fStyles->GetStyleRefAtPos( inPos) : NULL;
			}

			/** return the first span reference which intersects the passed range */
			const VTextStyle*	GetStyleRefAtRange(const sLONG inStart = 0, const sLONG inEnd = -1) const
			{
				VTaskLock protect(&fMutex);
				return fStyles ? fStyles->GetStyleRefAtRange( inStart, inEnd) : NULL;
			}

			/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
			bool				UpdateSpanRefs(sLONG inStart = 0, sLONG inEnd = -1, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL);

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

				for instance if VTextLayout container is a VView-derived class & fTextLayout is the text layout attached or used in the VView-derived class,
				in the VView-derived class DoDraw method & prior to call VTextLayout::Draw, you should insert this code to set the layer view rect:

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
				VTaskLock protect(&fMutex);
				if (fLayerViewRect == inRectRelativeToWnd)
					return;
				fLayerViewRect = inRectRelativeToWnd;
				fLayerIsDirty = true;
			}
			const VRect&		GetLayerViewRect() const { return fLayerViewRect; }

			/** insert text at the specified position */
			void				InsertText( sLONG inPos, const VString& inText);

			/** delete text range */
			void				DeleteText( sLONG inStart, sLONG inEnd);

			/** replace text */
			void				ReplaceText( sLONG inStart, sLONG inEnd, const VString& inText);

			/** apply style (use style range) */
			bool				ApplyStyle( const VTextStyle* inStyle, bool inFast = false);

			bool				ApplyStyles( const VTreeTextStyle * inStyles, bool inFast = false);

			/** show span references source (true) or value (false) */ 
			void				ShowRefs( bool inShowRefs, void *inUserData = NULL, VTreeTextStyle::fnBeforeReplace inBeforeReplace = NULL, VTreeTextStyle::fnAfterReplace inAfterReplace = NULL)
			{
				if (fShowRefs == inShowRefs)
					return;
				fShowRefs = inShowRefs;
				if (fStyles)
				{
					fStyles->ShowSpanRefs( inShowRefs);
					UpdateSpanRefs(0, -1, inUserData, inBeforeReplace, inAfterReplace); 
				}
			}

			bool				ShowRefs() const { return fShowRefs; }

			/** highlight span references in plain text or not */
			void				HighlightRefs(bool inHighlight, bool inUpdateAlways = false)
			{
				if (!inUpdateAlways && fHighlightRefs == inHighlight)
					return;
				fHighlightRefs = inHighlight;
				if (fStyles)
				{
					fStyles->ShowHighlight( fHighlightRefs);
					fLayoutIsValid = false;
				}
			}

			/** call this method to update span reference style while mouse enters span ref 
			@remarks
				return true if style has changed
			*/
			bool				NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef)
			{
				if (!fStyles)
					return false;
				if (fStyles->NotifyMouseEnterStyleSpanRef( inStyleSpanRef))
				{
					fLayoutIsValid = false;
					return true;
				}
				return false;
			}

			/** call this method to update span reference style while mouse leaves span ref 
			@remarks
				return true if style has changed
			*/
			bool				NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef)
			{
				if (!fStyles)
					return false;
				if (fStyles->NotifyMouseLeaveStyleSpanRef( inStyleSpanRef))
				{
					fLayoutIsValid = false;
					return true;
				}
				return false;
			}


			bool				HighlightRefs() const { return fHighlightRefs; }

			/** set default font 
			@remarks
				by default, base font is current gc font BUT: 
				it is highly recommended to call this method to avoid to inherit default font from gc
				& bind a default font to the text layout
				otherwise if gc font has changed, text layout needs to be computed again if there is no default font defined

				by default, passed font is assumed to be 4D form-compliant font (created with dpi = 72)
			*/
			void				SetDefaultFont( const VFont *inFont, GReal inDPI = 72.0f);

			/** get & retain default font 
			@remarks
				by default, return 4D form-compliant font (with dpi = 72) so not the actual fDPI-scaled font 
				(for consistency with SetDefaultFont & because fDPI internal scaling should be transparent for caller)
			*/
			VFont*				RetainDefaultFont(GReal inDPI = 72.0f) const;

			/** set default text color 
			@remarks
				by default, base text color is current gc text color BUT: 
				it is highly recommended to call this method to avoid to inherit text color from gc
				& bind a default text color to the text layout
				otherwise if gc text color has changed, text layout needs to be computed again & if there is no default text color defined
			*/
			void				SetDefaultTextColor( const VColor &inTextColor, bool inEnable = true)
			{
				VTaskLock protect(&fMutex);
				if (fHasDefaultTextColor == inEnable)
				{
					if (fHasDefaultTextColor && fDefaultTextColor == inTextColor)
						return;
					if (!fHasDefaultTextColor)
						return;
				}
				fLayoutIsValid = false;
				fHasDefaultTextColor = inEnable;
				if (inEnable)
					fDefaultTextColor = inTextColor;
				ReleaseRefCountable(&fDefaultStyle);
			}

			/** get default text color */
			bool				GetDefaultTextColor(VColor& outColor) const 
			{ 
				VTaskLock protect(&fMutex);
				if (!fHasDefaultTextColor)
					return false;
				outColor = fDefaultTextColor;
				return true; 
			}
	
			/** set back color 
			@param  inColor
				layout back color
			@param	inPaintBackColor
				if false (default), layout frame is not filled with back color (for instance if parent frame paints back color yet)
					but it is still used to ignore character background color if character back color is equal to the frame back color
					(unless back color is transparent color)
			@remarks
				default is white and back color is not painted
			*/
			void				SetBackColor( const VColor& inColor, bool inPaintBackColor = false) 
			{
				VTaskLock protect(&fMutex);
				if (fBackColor != inColor || fPaintBackColor != inPaintBackColor)
				{
					fPaintBackColor = inPaintBackColor;
					fBackColor = inColor;
					fLayoutIsValid = false;
					ReleaseRefCountable(&fDefaultStyle);
					fCharBackgroundColorDirty = true;
				}
			}
			const VColor&		GetBackColor() const { return fBackColor; }

			/** replace current text & styles 
			@remarks
				if inCopyStyles == true, styles are copied
				if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
			*/
			void				SetText( const VString& inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

			/** replace current text & styles 
			@remarks
				here VTextLayout does not copy the input text but only keeps a reference on it if inCopyText == false (default): 
					caller still owns the layout text so caller should not destroy the referenced text before VTextLayout is destroyed
					if caller modifies inText, it should call this method again
					Also, InsertText & DeleteText will update inText (so caller does not need to update it if it uses these methods)

				if inCopyStyles == true, styles are copied
				if inCopyStyles == false, styles are retained: in that case, if you modify passed styles you should call this method again 
										  Also, ApplyStyle will update inStyles (so caller does not need to update it if it uses ApplyStyle)
			*/
			void				SetText( VString* inText, VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false, bool inCopyText = false);

			const VString&		GetText() const
			{
#if VERSIONDEBUG
				xbox_assert(fTextPtr);
				if (fTextPtr != &fText)
				{
					//check fTextPtr validity: prevent access exception in case external VString ref has been released prior to VTextLayout
					try
					{
						sTextLayoutCurLength = fTextPtr->GetLength();
					}
					catch(...)
					{
						xbox_assert(false);
						static VString sTextDummy = "READ ACCESS ERROR: VTextLayout::GetText text pointer is invalid";
						return sTextDummy;
					}
				}
#endif
				return *fTextPtr;
			}

			VString*			GetExternalTextPtr() const { return fTextPtrExternal; }

			/** set text styles 
			@remarks
				if inCopyStyles == true, styles are copied
				if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
			*/
			void				SetStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

			VTreeTextStyle*		GetStyles() const
			{
				return fStyles;
			}
			VTreeTextStyle*		RetainStyles() const
			{
				return RetainRefCountable(fStyles);
			}

			/** set extra text styles 
			@remarks
				it can be used to add a temporary style effect to the text layout without modifying the main text styles
				(for instance to add a text selection effect: see VStyledTextEditView)

				if inCopyStyles == true, styles are copied
				if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
			*/
			void				SetExtStyles( VTreeTextStyle *inStyles = NULL, bool inCopyStyles = false);

			VTreeTextStyle*		GetExtStyles() const
			{
				VTaskLock protect(&fMutex); //as styles can be retained, we protect access 
				return fExtStyles;
			}
			VTreeTextStyle*		RetainExtStyles() const
			{
				VTaskLock protect(&fMutex); //as styles can be retained, we protect access 
				return RetainRefCountable(fExtStyles);
			}

			/** replace margins	(in point) */
			void				SetMargins(const GReal inMarginLeft = 0.0f, const GReal inMarginRight = 0.0f, const GReal inMarginTop = 0.0f, const GReal inMarginBottom = 0.0f);	
	
			/** get margins (in point) */
			void				GetMargins(GReal *outMarginLeft, GReal *outMarginRight, GReal *outMarginTop, GReal *outMarginBottom);	

            /** get margins (in TWIPS) */
            void                GetMargins( sDocPropPaddingMargin& outMargins);
    
            /** get margins (in TWIPS) */
            void                SetMargins( const sDocPropPaddingMargin& inMargins);

			/** set max text width (0 for infinite) - in pixel
			@remarks
				it is also equal to text box width 
				so you should specify one if text is not aligned to the left border
				otherwise text layout box width will be always equal to formatted text width
			*/ 
			void				SetMaxWidth(GReal inMaxWidth = 0.0f) 
			{
				VTaskLock protect(&fMutex);
				inMaxWidth = std::ceil(inMaxWidth);

				if (inMaxWidth != 0.0f && inMaxWidth < fMarginLeft+fMarginRight)
					inMaxWidth = fMarginLeft+fMarginRight;

				if (fMaxWidthInit == inMaxWidth)
					return;

				fLayoutIsValid = false;
				fMaxWidth = fMaxWidthInit = inMaxWidth;

				if (fMaxWidth != 0.0f)
				{
					fMaxWidth = fMaxWidthInit-(fMarginLeft+fMarginRight);
					if (fMaxWidth <= 0)
						fMaxWidth = 0.00001f;
				}
			}
			GReal				GetMaxWidth() const { return fMaxWidthInit; }

			/** set max text height (0 for infinite) - in pixel
			@remarks
				it is also equal to text box height 
				so you should specify one if paragraph is not aligned to the top border
				otherwise text layout box height will be always equal to formatted text height
			*/ 
			void				SetMaxHeight(GReal inMaxHeight = 0.0f) 
			{
				VTaskLock protect(&fMutex);
				inMaxHeight = std::ceil(inMaxHeight);

				if (inMaxHeight != 0.0f && inMaxHeight < fMarginTop+fMarginBottom)
					inMaxHeight = fMarginTop+fMarginBottom;

				if (fMaxHeightInit == inMaxHeight)
					return;

				fLayoutIsValid = false;
				fMaxHeight = fMaxHeightInit = inMaxHeight;

				if (fMaxHeight != 0.0f)
				{
					fMaxHeight = fMaxHeightInit-(fMarginTop+fMarginBottom);
					if (fMaxHeight <= 0)
						fMaxHeight = 0.00001f;
				}
			}
			GReal				GetMaxHeight() const { return fMaxHeightInit; }

			/** set text horizontal alignment (default is AL_DEFAULT) */
			void				SetTextAlign( AlignStyle inHAlign = AL_DEFAULT)
			{
				VTaskLock protect(&fMutex);
				if (fHAlign == inHAlign)
					return;
				fLayoutIsValid = false;
				fHAlign = inHAlign;
			}
			/** get text horizontal aligment */
			AlignStyle			GetTextAlign() const { return fHAlign; }

			/** set text paragraph alignment (default is AL_DEFAULT) */
			void				SetParaAlign( AlignStyle inVAlign = AL_DEFAULT)
			{
				VTaskLock protect(&fMutex);
				if (fVAlign == inVAlign)
					return;
				fLayoutIsValid = false;
				fVAlign = inVAlign;
			}
			/** get text paragraph aligment */
			AlignStyle			GetParaAlign() const { return fVAlign; }

			/** get paragraph line height 

				if positive, it is fixed line height in point
				if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
				if -1, it is normal line height 
			*/
			GReal				GetLineHeight() const { return fLineHeight; }

			/** set paragraph line height 

				if positive, it is fixed line height in point
				if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
				if -1, it is normal line height 
			*/
			void				SetLineHeight( const GReal inLineHeight) 
			{
				VTaskLock protect(&fMutex);
				if (fLineHeight == inLineHeight)
					return;
				fLayoutIsValid = false;
				fLineHeight = inLineHeight;
			}

			bool				IsRTL() const { return (GetLayoutMode() & TLM_RIGHT_TO_LEFT) != 0; }
			void				SetRTL(bool inRTL)
			{
				SetLayoutMode( inRTL ? (GetLayoutMode() | TLM_RIGHT_TO_LEFT) : (GetLayoutMode() & (~TLM_RIGHT_TO_LEFT)));
			}

			/** get first line padding offset in point
			@remarks
				might be negative for negative padding (that is second line up to the last line is padded but the first line)
			*/
			GReal				GetPaddingFirstLine() const { return fPaddingFirstLine; }

			/** set first line padding offset in point 
			@remarks
				might be negative for negative padding (that is second line up to the last line is padded but the first line)
			*/
			void				SetPaddingFirstLine(const GReal inPadding)
			{
				VTaskLock protect(&fMutex);
				if (fPaddingFirstLine == inPadding)
					return;
				fPaddingFirstLine = inPadding;
				fLayoutIsValid = false;
			}

			/** get tab stop offset in point */
			GReal				GetTabStopOffset() const { return fTabStopOffset; }

			/** set tab stop (offset in point) */
			void				SetTabStop(const GReal inOffset, const eTextTabStopType& inType)
			{
				VTaskLock protect(&fMutex);
				if (!testAssert(inOffset > 0.0f))
					return;
				if (fTabStopOffset == inOffset && fTabStopType == inType)
					return;
				fTabStopOffset = inOffset;
				fTabStopType = inType;
				fLayoutIsValid = false;
			}

			/** get tab stop type */ 
			eTextTabStopType	GetTabStopType() const { return fTabStopType; }

			/** set text layout mode (default is TLM_NORMAL) */
			void				SetLayoutMode( TextLayoutMode inLayoutMode = TLM_NORMAL)
			{
				VTaskLock protect(&fMutex);
				if (fLayoutMode == inLayoutMode)
					return;
				fLayoutIsValid = false;
				fLayoutMode = inLayoutMode;
			}
			/** get text layout mode */
			TextLayoutMode		GetLayoutMode() const { return fLayoutMode; }

			/** set layout DPI (default is 4D form DPI = 72) */
			void				SetDPI( GReal inDPI = 72.0f);

			/** get layout DPI */
			GReal				GetDPI() const { return fDPI; }

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

			/** draw text layout in the specified graphic context 
			@remarks
				text layout left top origin is set to inTopLeft
				text layout width is determined automatically if inTextLayout->GetMaxWidth() == 0.0f, otherwise it is equal to inTextLayout->GetMaxWidth()
				text layout height is determined automatically if inTextLayout->GetMaxHeight() == 0.0f, otherwise it is equal to inTextLayout->GetMaxHeight()
			*/
			void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint())
			{
				if (!testAssert(fGC == NULL || fGC == inGC))
					return;
				xbox_assert(inGC);
				inGC->_DrawTextLayout( this, inTopLeft);
			}
	
			/** return formatted text bounds (typo bounds) including margins
			@remarks
				text typo bounds origin is set to inTopLeft

				as layout bounds (see GetLayoutBounds) can be greater than formatted text bounds (if max width or max height is not equal to 0)
				because layout bounds are equal to the text box design bounds and not to the formatted text bounds
				you should use this method to get formatted text width & height 
			*/
			void				GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false)
			{
				//call GetLayoutBounds in order to update both layout & typo metrics
				VTaskLock protect(&fMutex);
				GetLayoutBounds( inGC, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
				outBounds.SetWidth( fCurWidth);
				outBounds.SetHeight( fCurHeight);
			}

			/** return text layout bounds including margins
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
			void				GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft = VPoint(), bool inReturnCharBoundsForEmptyText = false)
			{
				VTaskLock protect(&fMutex);
				if (inGC)
				{
					if (!testAssert(fGC == NULL || fGC == inGC))
						return;
					inGC->_GetTextLayoutBounds( this, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
				}
				else
				{
					//return stored metrics

					if (fTextLength == 0 && !inReturnCharBoundsForEmptyText)
					{
						outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
						return;
					}
					outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), fCurLayoutWidth, fCurLayoutHeight);
				}
			}

			/** return text layout run bounds from the specified range 
			@remarks
				text layout origin is set to inTopLeft
			*/
			void				GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft = VPoint(), sLONG inStart = 0, sLONG inEnd = -1)
			{
				if (!testAssert(fGC == NULL || fGC == inGC))
					return;
				xbox_assert(inGC);
				inGC->_GetTextLayoutRunBoundsFromRange( this, outRunBounds, inTopLeft, inStart, inEnd);
			}

			/** return the caret position & height of text at the specified text position in the text layout
			@remarks
				text layout origin is set to inTopLeft

				text position is 0-based

				caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)
				if text layout is drawed at inTopLeft

				by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
				but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
			*/
			void				GetCaretMetricsFromCharIndex( VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true)
			{
				if (!testAssert(fGC == NULL || fGC == inGC))
					return;
				xbox_assert(inGC);
				inGC->_GetTextLayoutCaretMetricsFromCharIndex( this, inTopLeft, inCharIndex, outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics);
			}

	
			/** return the text position at the specified coordinates
			@remarks
				text layout origin is set to inTopLeft (in gc user space)
				the hit coordinates are defined by inPos param (in gc user space)

				output text position is 0-based

				return true if input position is inside character bounds
				if method returns false, it returns the closest character index to the input position
			*/
			bool				GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex)
			{
				if (!testAssert(fGC == NULL || fGC == inGC))
					return false;
				xbox_assert(inGC);
				bool inside = inGC->_GetTextLayoutCharIndexFromPos( this, inTopLeft, inPos, outCharIndex);
				return inside; 
			}

			/** return true if layout uses truetype font only */
			bool				UseFontTrueTypeOnly(VGraphicContext *inGC) const;

			/** end using text layout (reentrant) */
			void				EndUsingContext();

			const VTreeTextStyle* RetainDefaultStyle() const { return _RetainDefaultTreeTextStyle(); }

			/** retain full styles
			@remarks
				use this method to get the full styles applied to the text content

				(in monostyle, it is generally equal to RetainDefaultStyle)

				caller should call ReleaseFullTreeTextStyle to release returned styles but ReleaseRefCountable (because internal state is thread-locked between calls)

				RetainFullTreeTextStyle & ReleaseFullTreeTextStyle are not reentrant
			*/
			const VTreeTextStyle* RetainFullTreeTextStyle() const { fMutex.Lock(); return _RetainFullTreeTextStyle(); }

			void				ReleaseFullTreeTextStyle(const VTreeTextStyle *inStyles) const
			{
				xbox_assert(inStyles == fDefaultStyle);

				if (fDefaultStyle->GetChildCount() > 0)
				{
					xbox_assert(fDefaultStyle->GetChildCount() == 1 && fStyles);
					fDefaultStyle->RemoveChildAt(1);
				}
				fDefaultStyle->Release();

				fMutex.Unlock();
			}

	static	VTextStyle*			CreateTextStyle( const VFont *inFont, const GReal inDPI = 72.0f);

			/**	create text style from the specified font name, font size & font face
			@remarks
				if font name is empty, returned style inherits font name from parent
				if font face is equal to UNDEFINED_STYLE, returned style inherits style from parent
				if font size is equal to UNDEFINED_STYLE, returned style inherits font size from parent
			*/
	static	VTextStyle*			CreateTextStyle(const VString& inFontFamilyName, const VFontFace& inFace, GReal inFontSize, const GReal inDPI = 0);		

	static	AlignStyle			JustToAlignStyle( const eStyleJust inJust) 
			{
				switch (inJust)
				{
				case JST_Left:
					return AL_LEFT;
					break;
				case JST_Right:
					return AL_RIGHT;
					break;
				case JST_Center:
					return AL_CENTER;
					break;
				case JST_Justify:
					return AL_JUST;
					break;
				default:
					return AL_DEFAULT;
					break;
				}
			}

	static	eStyleJust			JustFromAlignStyle( const AlignStyle inJust) 
			{
				switch (inJust)
				{
				case AL_LEFT:
					return JST_Left;
					break;
				case AL_RIGHT:
					return JST_Right;
					break;
				case AL_CENTER:
					return JST_Center;
					break;
				case AL_JUST:
					return JST_Justify;
					break;
				default:
					return JST_Default;
					break;
				}
			}

protected:
	friend	class				VGraphicContext;
#if ENABLE_D2D
	friend	class				VWinD2DGraphicContext;
#endif
#if VERSIONMAC
	friend	class				VMacQuartzGraphicContext;
#endif

	typedef	std::vector<VRect>					VectorOfRect;
	typedef std::pair<VectorOfRect, VColor>		BoundsAndColor;
	typedef	std::vector<BoundsAndColor>			VectorOfBoundsAndColor;



			void				_Init();

			GReal				_GetLayoutWidthMinusMargin() const
			{
				return fCurLayoutWidth-fMarginLeft-fMarginRight;
			}
			GReal				_GetLayoutHeightMinusMargin() const 
			{
				return fCurLayoutHeight-fMarginTop-fMarginBottom;
			}

			bool				_ApplyStyle(const VTextStyle *inStyle);

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
			void				_UpdateTextLayout();

			/** internally called while replacing text in order to update impl-specific stuff */
			void				_PostReplaceText();

			void				_BeginUsingStyles();
			void				_EndUsingStyles();

			void				_ResetLayer();

			/** return true if layout styles uses truetype font only */
			bool				_StylesUseFontTrueTypeOnly() const;

			void				_UpdateBackgroundColorRenderInfo( const VPoint& inTopLeft, const VRect& inClipBounds);
			void				_UpdateBackgroundColorRenderInfoRec( const VPoint& inTopLeft, VTreeTextStyle *inStyles, const sLONG inStart, const sLONG inEnd, bool inHasParentBackgroundColor = false);
			void				_DrawBackgroundColor(const VPoint& inTopLeft, const VRect& inBoundsLayout);

			void				_ReplaceText( const VDocParagraph *inPara);
			void				_ApplyParagraphProps( const VDocParagraph *inPara, bool inIgnoreIfInherited = true);
			void				_ResetStyles( bool inResetParagraphProps = true, bool inResetCharStyles = true);

			bool				fLayoutIsValid;
			
			bool				fShouldEnableCache;
			bool				fShouldDrawOnLayer;
			VImageOffScreen*	fLayerOffScreen;
			VRect				fLayerViewRect;
			VPoint				fCurLayerOffsetViewRect;
			VAffineTransform	fCurLayerCTM;
			bool				fLayerIsDirty;


			GraphicContextType	fCurGCType;
			VGraphicContext*	fGC;
			sLONG				fGCUseCount;

	mutable VFont*				fDefaultFont;
			VColor				fDefaultTextColor;
			bool				fHasDefaultTextColor;
	mutable VTreeTextStyle*		fDefaultStyle;

			VFont*				fCurFont;
			VColor				fCurTextColor;
			GReal				fCurKerning;
			TextRenderingMode	fCurTextRenderingMode;

			/** margin in TWIPS */
			uLONG				fMarginLeftTWIPS, fMarginRightTWIPS, fMarginTopTWIPS, fMarginBottomTWIPS;
	
			/** margin in point */
			GReal				fMarginLeft, fMarginRight, fMarginTop, fMarginBottom;

			/** width of formatted text (excluding margins) */
			GReal				fCurWidth; 
			/** height of formatted text (excluding margins) */
			GReal				fCurHeight;

			/** width of text layout box (including margins)
			@remarks
				it is equal to fMaxWidth if fMaxWidth != 0.0f
				otherwise it is equal to fCurWidth
			*/
			GReal				fCurLayoutWidth;

			/** height of text layout box (including margins)
			@remarks
				it is equal to fMaxHeight if fMaxHeight != 0.0f
				otherwise it is equal to fCurHeight
			*/
			GReal				fCurLayoutHeight;

			GReal 				fCurOverhangLeft;
			GReal 				fCurOverhangRight;
			GReal 				fCurOverhangTop;
			GReal 				fCurOverhangBottom;

			sLONG				fTextLength;
			VString				fText;
			VString*			fTextPtrExternal;
			VString*			fTextPtr;
			VTreeTextStyle*		fStyles;
			VTreeTextStyle*		fExtStyles;
			
			GReal				fMaxWidthInit;
			GReal				fMaxHeightInit;
			GReal				fMaxWidth;
			GReal				fMaxHeight;

			AlignStyle			fHAlign;
			AlignStyle			fVAlign;
			GReal 				fLineHeight;
			GReal 				fPaddingFirstLine;
			GReal 				fTabStopOffset;
			eTextTabStopType	fTabStopType;

			TextLayoutMode		fLayoutMode;
			GReal				fDPI;
	
			bool 				fShowRefs;
			bool 				fHighlightRefs;

			bool 				fUseFontTrueTypeOnly;
			bool 				fStylesUseFontTrueTypeOnly;

			bool 				fNeedUpdateBounds;

			/** text box layout used by Quartz2D or GDIPlus/GDI (Windows legacy impl) */
			VStyledTextBox*		fTextBox;
#if ENABLE_D2D
			/** DWrite text layout used by Direct2D impl only (in not legacy mode)*/
			IDWriteTextLayout*	fLayoutD2D;
#endif

			VFont*				fBackupFont;
			VColor				fBackupTextColor;
			bool				fSkipDrawLayer;

			bool				fIsPrinting;

			/** true if VTextLayout should draw character background color but native impl */
			bool				fShouldDrawCharBackColor;

			/** true if character background color render info needs to be computed again */
			bool				fCharBackgroundColorDirty;

			/** character background color render info 
			@remarks
				it is used to draw text background color if it is not supported natively by impl
				(as for CoreText & Direct2D impl)
			*/
			VectorOfBoundsAndColor	fCharBackgroundColorRenderInfo;

			VColor				fBackColor;
			bool				fPaintBackColor;

#if VERSIONDEBUG
	mutable sLONG				sTextLayoutCurLength;
#endif

	mutable VCriticalSection	fMutex;
};

END_TOOLBOX_NAMESPACE

#endif
