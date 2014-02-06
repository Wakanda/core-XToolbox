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
#ifndef __VBaseLayout__
#define __VBaseLayout__

BEGIN_TOOLBOX_NAMESPACE

class	VDocBorderLayout;
class	VDocParagraphLayout;
struct	sDocBorder;
struct	sDocBorderRect;
class	VDocBackgroundImageLayout;


/** aspect ratio */
typedef enum eKeepAspectRatio
{
	kKAR_XMIN			= 1,
	kKAR_XMID			= 2,
	kKAR_XMAX			= 4,

	kKAR_YMIN			= 8,
	kKAR_YMID			= 16,
	kKAR_YMAX			= 32,

	kKAR_XMINYMIN		= kKAR_XMIN|kKAR_YMIN,
	kKAR_XMINYMID		= kKAR_XMIN|kKAR_YMID,
	kKAR_XMINYMAX		= kKAR_XMIN|kKAR_YMAX,

	kKAR_XMIDYMIN		= kKAR_XMID|kKAR_YMIN,
	kKAR_XMIDYMID		= kKAR_XMID|kKAR_YMID,
	kKAR_XMIDYMAX		= kKAR_XMID|kKAR_YMAX,
		
	kKAR_XMAXYMIN		= kKAR_XMAX|kKAR_YMIN,
	kKAR_XMAXYMID		= kKAR_XMAX|kKAR_YMID,
	kKAR_XMAXYMAX		= kKAR_XMAX|kKAR_YMAX,

	kKAR_XYDECAL_MASK	= 63,

	kKAR_COVER			= 64, /* cover or contain */ 
	kKAR_NONE			= 0
} eKeepAspectRatio;



/** document node layout delegate */
class IDocNodeLayout: public IDocSpanTextRefLayout
{
public:
		/** set layout DPI */
virtual	void				SetDPI( const GReal inDPI = 72) = 0;
virtual GReal				GetDPI() const = 0;

		/** update layout metrics */ 
virtual void				UpdateLayout(VGraphicContext *inGC) = 0;

		/** render layout element in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f) = 0;

		/** update ascent (compute ascent relatively to line & text ascent, descent in px (relative to computing dpi) - not including line embedded node(s) metrics) 
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateLayout
		*/
virtual	void				UpdateAscent(const GReal inLineAscent = 0, const GReal inLineDescent = 0, const GReal inTextAscent = 0, const GReal inTextDescent = 0) {}

		/** get ascent (in px - relative to computing dpi)
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateAscent
		*/
virtual	GReal				GetAscent() const { return 0; }

		/** get descent (in px - relative to computing dpi)
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateAscent
		*/
virtual	GReal				GetDescent() const { return 0; }
	
		/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&		GetLayoutBounds() const = 0;

		/** get typo local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&		GetTypoBounds() const { return GetLayoutBounds(); }

		/** get background bounds (in px - relative to computing dpi) */
virtual	const VRect&		GetBackgroundBounds(eDocPropBackgroundBox inBackgroundBox = kDOC_BACKGROUND_BORDER_BOX) const = 0;
};


class XTOOLBOX_API ITextLayout;

class IDocBaseLayout : public IDocNodeLayout
{
public:

		/** query text layout interface (if available for this class instance) 
		@remarks
			for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
			might return NULL
		*/ 
virtual	ITextLayout*		QueryTextLayoutInterface() { return NULL; }

		/** query text layout interface (if available for this class instance) 
		@remarks
			for instance VDocParagraphLayout & VDocContainerLayout derive from ITextLayout
			might return NULL
		*/ 
virtual	const ITextLayout*	QueryTextLayoutInterfaceConst() const { return NULL; }

		/** return document node type */
virtual	eDocNodeType		GetDocType() const { return kDOC_NODE_TYPE_UNKNOWN; }

		/** get text start index 
		@remarks
			index is relative to parent layout text start index
			(if this layout has not a parent, it should be always 0)
		*/
virtual	sLONG				GetTextStart() const { return 0; }

		/** return local text length 
		@remarks
			it should be at least 1 no matter the kind of layout (any layout object should have at least 1 character size)
		*/
virtual	uLONG				GetTextLength() const = 0;

		/** get parent layout reference 
		@remarks
			for instance for a VDocParagraphLayout instance, it might be a VDocContainerLayout instance (paragraph layout container)
			(but as VDocParagraphLayout instance can be used stand-alone, it might be NULL too)
		*/
virtual IDocBaseLayout*		GetParent() { return NULL; }

		/** get count of child layout instances */
virtual	uLONG				GetChildCount() const { return 0; }
	
		/** get layout child index (relative to parent child vector & 0-based) */
virtual sLONG				GetChildIndex() const { return 0; }

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

virtual	bool				IsLastChild(bool inLookUp = false) const { return true; }

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true) = 0;
        /** get outside margins (in twips) */
virtual	void				GetMargins( sDocPropRect& outMargins) const = 0;

		/** set inside margins	(in twips) - CSS padding */
virtual	void				SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin = true) = 0;
		/** get inside margins	(in twips) - CSS padding */
virtual	void				GetPadding(sDocPropRect& outPadding) const = 0;

		/** set text align - CSS text-align (-1 for not defined) */
virtual void				SetTextAlign( const AlignStyle inHAlign) = 0; 
virtual AlignStyle			GetTextAlign() const = 0;

		/** set vertical align - CSS vertical-align (AL_DEFAULT for default) */
virtual void				SetVerticalAlign( const AlignStyle inVAlign) = 0;
virtual AlignStyle			GetVerticalAlign() const = 0;

		/** set width (in twips) - CSS width (0 for auto) */
virtual	void				SetWidth(const uLONG inWidth) = 0;
virtual uLONG				GetWidth(bool inIncludeAllMargins = false) const = 0;

		/** set height (in twips) - CSS height (0 for auto) */
virtual	void				SetHeight(const uLONG inHeight) = 0;
virtual uLONG				GetHeight(bool inIncludeAllMargins = false) const = 0;

		/** set min width (in twips) - CSS min-width (0 for auto) */
virtual	void				SetMinWidth(const uLONG inWidth) = 0;
virtual uLONG				GetMinWidth(bool inIncludeAllMargins = false) const = 0;

		/** set min height (in twips) - CSS min-height (0 for auto) */
virtual	void				SetMinHeight(const uLONG inHeight) = 0;
virtual uLONG				GetMinHeight(bool inIncludeAllMargins = false) const = 0;

		/** set background color - CSS background-color (transparent for not defined) */
virtual void				SetBackgroundColor( const VColor& inColor) = 0;
virtual const VColor&		GetBackgroundColor() const = 0;

		/** set background clipping - CSS background-clip */
virtual void				SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip) = 0;
virtual eDocPropBackgroundBox	GetBackgroundClip() const = 0;

		/** set background origin - CSS background-origin */
virtual void				SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin) = 0; 
virtual eDocPropBackgroundBox	GetBackgroundOrigin() const = 0;

		/** set borders 
		@remarks
			if inLeft is NULL, left border is same as right border 
			if inBottom is NULL, bottom border is same as top border
			if inRight is NULL, right border is same as top border
			if inTop is NULL, top border is default border (solid, medium width & black color)
		*/
virtual	void				SetBorders( const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL) = 0; 

		/** clear borders */
virtual	void				ClearBorders() = 0;
					
		/** set background image 
		@remarks
			see VDocBackgroundImageLayout constructor for parameters description */
virtual	void				SetBackgroundImage(	const VString& inURL,	
												const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
												const eDocPropBackgroundRepeat inRepeat = kDOC_BACKGROUND_REPEAT,
												const GReal inWidth = kDOC_BACKGROUND_SIZE_AUTO, const GReal inHeight = kDOC_BACKGROUND_SIZE_AUTO,
												const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false) = 0; 

		/** clear background image */
virtual	void				ClearBackgroundImage() = 0;

};

class XTOOLBOX_API ITextLayout;

class VDocLayoutChildrenIterator;
class VDocLayoutChildrenReverseIterator;

/** base layout class
@remarks
	this class manages text element common layout (margins, padding, borders, background color, background image, etc...)
	all text element layout classes are derived from that class (but VDocBorderLayout because VDocBorderLayout instance is embedded in VDocBaseLayout)

	text index parameters are always relative to current layout text: use GetTextStart() to convert local text index to parent layout text index;
	Also GetTextLength() returns local text length (including all children text lengths)
	Note that text length is always 1 for a image layout.

	important: all design metrics here are stored internally in twips or point (with twips precision) 
	(onscreen, point is equal to pixel on Windows & Mac platform as 1pt=1px onscreen in 4D form on Windows & Mac:
	 when 4D form will become dpi-aware on Windows & Mac, as form should be zoomed to screen dpi/72 as for printing, it will not require modifications 
	 so it is important to store from now res-independent metrics in order to be consistent with high-dpi in v15 - and to ease porting)
	
	about layout computed metrics, class manages internally a dpi for computing metrics & a dpi for rendering: 
	rendering dpi (fRenderDPI) might be different from computing dpi (fDPI) but user can only change the rendering dpi with SetDPI()
	(for instance for perfectly scalable contexts like Direct2D or CoreText, metrics are computed with dpi = 72 (in point so with TWIPS precision, as for design metrics) 
	 & while rendering, context is scaled to rendering dpi/72 which ensures consistent rendering no matter the rendering dpi; 
	 with GDI, metrics are computed at rendering dpi - because GDI does not scale text & shapes properly as GDI metrics are rounded to pixel)

	 note that if layout has a parent layout, rendering dpi is actually always equal to computing dpi:
	 it is because only the highest layout container class instance may have a rendering dpi not equal to computing dpi (because we can apply gc scaling from computing dpi to rendering dpi only once)
	 for instance consider a VDocContainerLayout class instance which embeds one child VDocParagraphLayout instance:
	 if gc impl is D2D/DWrite or Quartz2D, while rendering onscreen, VDocContainerLayout class computing dpi = 72 and rendering dpi = dpi as set by SetDPI();
	 but for child VDocParagraphLayout instance, rendering dpi = computing dpi = 72;
	 therefore gc scaling from computing dpi to rendering dpi is applied only once by VDocContainerLayout class instance while drawing onscreen.
*/
class XTOOLBOX_API VDocBaseLayout: public VObject, public IDocBaseLayout, public IRefCountable
{
public:
								VDocBaseLayout();
virtual							~VDocBaseLayout();


/////////////////	IDocSpanTextRefLayout interface

virtual	void					ReleaseRef() { IRefCountable::Release(); }

/////////////////	IDocNodeLayout interface

		/** set rendering DPI */
virtual	void					SetDPI( const GReal inDPI = 72);
virtual GReal					GetDPI() const { return fRenderDPI; }

		/** get layout local bounds (in px - relative to computing dpi - and with left top origin at (0,0)) as computed with last call to UpdateLayout */
virtual	const VRect&			GetLayoutBounds() const { return fLayoutBounds; }

		/** get background bounds (in px - relative to computing dpi) */
virtual	const VRect&			GetBackgroundBounds(eDocPropBackgroundBox inBackgroundBox = kDOC_BACKGROUND_BORDER_BOX) const;

		/** render layout element in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual void					Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f);

/////////////////	IDocBaseLayout interface

		/** get text start index 
		@remarks
			index is relative to parent text layout
			(if layout has not a parent, it should be always 0)
		*/
virtual	sLONG					GetTextStart() const { xbox_assert(fParent || fTextStart == 0); return fTextStart; }

		/** return local text length 
		@remarks
			it should be at least 1 no matter the kind of layout (any layout object should have at least 1 character size)
		*/
virtual	uLONG					GetTextLength() const { return fTextLength; }

		/** get parent layout reference 
		@remarks
			for instance for a VDocParagraphLayout instance, it might be a VDocContainerLayout instance (paragraph layout container)
			(but as VDocParagraphLayout instance can be used stand-alone, it might be NULL too)
		*/
virtual IDocBaseLayout*			GetParent() { return static_cast<IDocBaseLayout*>(fParent); }

		/** get count of child layout instances */
virtual	uLONG					GetChildCount() const { return fChildren.size(); }
	
		/** get layout child index (relative to parent child vector & 0-based) */
virtual sLONG					GetChildIndex() const { return fChildIndex; }

		/** add child layout instance */
virtual void					AddChild( IDocBaseLayout* inLayout);

		/** insert child layout instance (index is 0-based) */
virtual void					InsertChild( IDocBaseLayout* inLayout, sLONG inIndex = 0);

		/** remove child layout instance (index is 0-based) */
virtual void					RemoveChild( sLONG inIndex);

		/** get child layout instance from index (0-based) */
virtual	IDocBaseLayout*			GetNthChild(sLONG inIndex) const { xbox_assert(inIndex >= 0 && inIndex < fChildren.size()); return static_cast<IDocBaseLayout*>(fChildren[inIndex].Get()); }

		/** get first child layout instance which intersects the passed text index */
virtual IDocBaseLayout*			GetFirstChild( sLONG inStart = 0) const;

virtual	bool					IsLastChild(bool inLookUp = false) const;

		/** initialize properties from the passed node */
virtual	void					InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited = false);

		/** set node properties from the current properties */
virtual	void					SetPropsTo( VDocNode *outNode);

		/** set outside margins	(in twips) - CSS margin */
virtual	void					SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true);
        /** get outside margins (in twips) */
virtual	void					GetMargins( sDocPropRect& outMargins) const { outMargins = fMarginOutTWIPS; }

		/** set inside margins	(in twips) - CSS padding */
virtual	void					SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin = true);
		/** get inside margins	(in twips) - CSS padding */
virtual	void					GetPadding(sDocPropRect& outPadding) const { outPadding = fMarginInTWIPS; }

		/** set width (in twips) - CSS width (0 for auto) - without margin, padding & border */
virtual	void					SetWidth(const uLONG inWidth) { fWidthTWIPS = inWidth; }
virtual uLONG					GetWidth(bool inIncludeAllMargins = false) const;

		/** set height (in twips) - CSS height (0 for auto) - without margin, padding & border */
virtual	void					SetHeight(const uLONG inHeight) { fHeightTWIPS = inHeight; }
virtual uLONG					GetHeight(bool inIncludeAllMargins = false) const;

		/** set min width (in twips) - CSS min-width (0 for auto) */
virtual	void					SetMinWidth(const uLONG inWidth) { fMinWidthTWIPS = inWidth; }
virtual uLONG					GetMinWidth(bool inIncludeAllMargins = false) const;

		/** set min height (in twips) - CSS min-height (0 for auto) */
virtual	void					SetMinHeight(const uLONG inHeight) { fMinHeightTWIPS = inHeight; }
virtual uLONG					GetMinHeight(bool inIncludeAllMargins = false) const;

		/** set text align - CSS text-align (AL_DEFAULT for default) */
virtual void					SetTextAlign( const AlignStyle inHAlign) { fTextAlign = inHAlign; }
virtual AlignStyle				GetTextAlign() const { return fTextAlign; }

		/** set vertical align - CSS vertical-align (AL_DEFAULT for default) */
virtual void					SetVerticalAlign( const AlignStyle inVAlign) { fVerticalAlign = inVAlign; }
virtual AlignStyle				GetVerticalAlign() const { return fVerticalAlign; }

		/** set background color - CSS background-color (transparent for not defined) */
virtual void					SetBackgroundColor( const VColor& inColor) { fBackgroundColor = inColor; }
virtual const VColor&			GetBackgroundColor() const { return fBackgroundColor; }

		/** set background clipping - CSS background-clip */
virtual void					SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip);
virtual eDocPropBackgroundBox	GetBackgroundClip() const { return fBackgroundClip; }

		/** set background origin - CSS background-origin */
virtual void					SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin);
virtual eDocPropBackgroundBox	GetBackgroundOrigin() const { return fBackgroundOrigin; }

		/** set borders 
		@remarks
			if inLeft is NULL, left border is same as right border 
			if inBottom is NULL, bottom border is same as top border
			if inRight is NULL, right border is same as top border
			if inTop is NULL, top border is default border (solid, medium width & black color)
		*/
		void					SetBorders( const sDocBorder *inTop = NULL, const sDocBorder *inRight = NULL, const sDocBorder *inBottom = NULL, const sDocBorder *inLeft = NULL);

		/** clear borders */
		void					ClearBorders();
							
		const sDocBorderRect*	GetBorders() const;

		/** set background image 
		@remarks
			see VDocBackgroundImageLayout constructor for parameters description */
		void					SetBackgroundImage(	const VString& inURL,	
													const eStyleJust inHAlign = JST_Left, const eStyleJust inVAlign = JST_Top, 
													const eDocPropBackgroundRepeat inRepeat = kDOC_BACKGROUND_REPEAT,
													const GReal inWidth = kDOC_BACKGROUND_SIZE_AUTO, const GReal inHeight = kDOC_BACKGROUND_SIZE_AUTO,
													const bool inWidthIsPercentage = false,  const bool inHeightIsPercentage = false);

		/** clear background image */
		void					ClearBackgroundImage();

/////////////////

		/** make transform from source viewport to destination viewport
			(with uniform or non uniform scaling) */
static	void					ComputeViewportTransform( VAffineTransform& outTransform, const VRect& inRectSrc, const VRect& inRectDst, uLONG inKeepAspectRatio);

static	GReal					RoundMetrics( VGraphicContext *inGC, GReal inValue);

static	AlignStyle				JustToAlignStyle( const eStyleJust inJust);

static	eStyleJust				JustFromAlignStyle( const AlignStyle inJust);

protected:
friend class					VDocParagraphLayout;
friend class					VDocLayoutChildrenIterator;
friend class					VDocLayoutChildrenReverseIterator;

typedef struct sRectMargin
		{
								GReal top;
								GReal right;
								GReal bottom;
								GReal left;
		} sRectMargin;

typedef std::vector< VRefPtr<VDocBaseLayout> > VectorOfChildren;

		/** create border from passed document node */
		void					_CreateBorder(const VDocNode *inDocNode, bool inCalcAllMargin = true);

		/** create background image from passed document node */
		void					_CreateBackgroundImage(const VDocNode *inDocNode);

		/** set document node background image properties */
		void					_SetBackgroundImagePropsTo( VDocNode* outDocNode);

		/** derived class should call this method when layout bounds are changed - normally in UpdateLayout */
		void					_SetLayoutBounds( VGraphicContext *inGC, const VRect& inBounds);

		/** finalize text layout (VDocParagraphLayout & VDocContainerLayout) local metrics */
		void					_FinalizeTextLocalMetrics(	VGraphicContext *inGC, GReal typoWidth, GReal typoHeight, GReal layoutWidth, GReal layoutHeight, 
															const sRectMargin& inMargin, GReal inRenderMaxWidth, GReal inRenderMaxHeight, 
															VRect& outTypoBounds, VRect& outRenderTypoBounds, VRect &outRenderLayoutBounds, sRectMargin& outRenderMargin);

		void					_UpdateBorderWidth(bool inCalcAllMargin = true);
		void					_CalcAllMargin();

		/** get background bounds (in px - relative to dpi set by SetDPI) 
		@remarks
			unlike GetBackgroundBounds, it takes account gc crispEdges status 
			(useful for proper clipping according to crispEdges status)
		*/
		const VRect&			_GetBackgroundBounds(VGraphicContext *inGC, eDocPropBackgroundBox inBackgroundBox = kDOC_BACKGROUND_BORDER_BOX) const;

		/** if inDraw == true, set ctm to local user space (where 0,0 is equal to layout left top origin - according to computing dpi & vertical alignement) 
							   and returns in outCTM previous ctm
			if inDraw == false, ctm is preserved but returns in outCTM transform from local user space to current ctm user space
								(so methods which return metrics apply outCTM transform to computed local metrics before returning metrics)
		*/
virtual void					_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw, const VPoint& inPreDecal);
		/** restore ctm is it was modified by _BeginLocalTransform */
virtual void					_EndLocalTransform( VGraphicContext *inGC, const VAffineTransform& inCTM, bool inDraw = false);

virtual void					_BeginLayerOpacity(VGraphicContext *inGC, const GReal inOpacity = 1.0f);
virtual void					_EndLayerOpacity(VGraphicContext *inGC, const GReal inOpacity = 1.0f);

virtual	void					_UpdateTextInfo();

		void					_UpdateTextInfoEx(bool inUpdateNextSiblings = false);

		GReal					_GetAllMarginHorizontalTWIPS() const;
		GReal					_GetAllMarginVerticalTWIPS() const;

		/** return width (inherited from parent content if has a parent & local width is 0) */
		sLONG					_GetWidthTWIPS() const;
		/** return height */
		sLONG					_GetHeightTWIPS() const;

		void					_AdjustComputingDPI(VGraphicContext *inGC);

		VDocBaseLayout*			_GetTopLevelParentLayout() const;

		/** normalize passed range so it is bound to [0,GetTextLength()] */ 
		void					_NormalizeRange(sLONG& ioStart, sLONG& ioEnd) const;

		//design datas

		/** layout DPI (DPI used for computing metrics) */
		GReal						fDPI;

		/** drawing DPI (DPI used for rendering) */
		GReal						fRenderDPI;

		/** inside margin - CSS padding (in TWIPS) */
		sDocPropRect				fMarginInTWIPS;

		/** outside margin - CSS margin (in TWIPS) */
		sDocPropRect				fMarginOutTWIPS;
		
		/** width in TWIPS (0 for auto) - without margin, padding & border  */
		uLONG						fWidthTWIPS;

		/** height in TWIPS (0 for auto) - without margin, padding & border  */
		uLONG						fHeightTWIPS;

		/** min width in TWIPS (0 for auto) */
		uLONG						fMinWidthTWIPS;

		/** min height in TWIPS (0 for auto) */
		uLONG						fMinHeightTWIPS;

		/** text align (AL_DEFAULT for default) */
		AlignStyle					fTextAlign;

		/** vertical align (AL_DEFAULT for default) */
		AlignStyle					fVerticalAlign;

		/** background color (alpha = 0 for transparent) */
		VColor						fBackgroundColor;

		/** background clipping mode */
		eDocPropBackgroundBox		fBackgroundClip;

		/** background origin */
		eDocPropBackgroundBox		fBackgroundOrigin;

		/** background image */
		VDocBackgroundImageLayout*  fBackgroundImageLayout;

		/** border layout */
		VDocBorderLayout*			fBorderLayout;
		
		/** text start index (relative to parent layout start index) */
		sLONG						fTextStart;

		/** local text layout length */
		uLONG						fTextLength;

		/** parent layout */
		VDocBaseLayout*				fParent;

		/** layout children */
		VectorOfChildren			fChildren;

		/** layout child index (relative to parent layout) */
		sLONG						fChildIndex;

		//computed datas


		/** inside margin - CSS padding (in px - relative to computing dpi) */
		sRectMargin					fMarginIn;

		/** outside margin - CSS margin (in px - relative to computing dpi) */
		sRectMargin					fMarginOut;

		/** borders width (in px - relative to computing dpi) - border width in pt is stored in fBorderLayout */
		sRectMargin					fBorderWidth;

		/** all margins = total of outside margin+border width+inside margin (in px - relative to computing dpi) */
		sRectMargin					fAllMargin;	

		/** local layout bounds (in px - relative to computing dpi) 
		@remarks
			layout bounds include margin, border & padding
		*/
		VRect						fLayoutBounds;

		/** local border bounds (in px - relative to computing dpi) 
		@remarks
			border bounds include border & padding but not margin
		*/
		VRect						fBorderBounds;

		/** local padding bounds (in px - relative to computing dpi) 
		@remarks
			padding bounds include padding but not margin & border
		*/
		VRect						fPaddingBounds;

		/** local content bounds (in px - relative to computing dpi) 
		@remarks
			content bounds do not include margin & border & padding
			(for instance it is text bounds only for VDocParagraphLayout)
		*/
		VRect						fContentBounds;


		//temporary datas

		GReal						fOpacityBackup;

mutable	bool						fNoUpdateTextInfo;

mutable	VRect						fTempBorderBounds;
mutable	VRect						fTempPaddingBounds;
};

class XTOOLBOX_API VDocLayoutChildrenIterator : public VObject
{
public:
	VDocLayoutChildrenIterator( const VDocBaseLayout *inLayout):VObject() { xbox_assert(inLayout); fParent = inLayout; fIterator = fParent->fChildren.end(); fEnd = -1; }
virtual	~VDocLayoutChildrenIterator() {}

		VDocBaseLayout*				First(sLONG inTextStart = 0, sLONG inTextEnd = -1); //start iteration on the passed text range (full text range on default)
		VDocBaseLayout*				Next();
		VDocBaseLayout*				Current() {  if (fIterator != fParent->fChildren.end()) return (*fIterator).Get(); else return NULL; }
	
private:
		VDocBaseLayout::VectorOfChildren::const_iterator fIterator;
		const VDocBaseLayout*		fParent;
		sLONG						fEnd;
};

class XTOOLBOX_API VDocLayoutChildrenReverseIterator : public VObject
{
public:
	VDocLayoutChildrenReverseIterator( const VDocBaseLayout *inLayout):VObject() { xbox_assert(inLayout); fParent = inLayout; fIterator = fParent->fChildren.rend(); fStart = 0; }
virtual	~VDocLayoutChildrenReverseIterator() {}

		VDocBaseLayout*				First(sLONG inTextStart = 0, sLONG inTextEnd = -1);
		VDocBaseLayout*				Next();
		VDocBaseLayout*				Current() {  if (fIterator != fParent->fChildren.rend()) return (*fIterator).Get(); else return NULL; }

private:
		VDocBaseLayout::VectorOfChildren::const_reverse_iterator fIterator;
		const VDocBaseLayout*		fParent;
		sLONG						fStart;
};


END_TOOLBOX_NAMESPACE

#endif
