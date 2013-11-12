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

/** document node layout delegate */
class IDocNodeLayout: public IDocSpanTextRefLayout
{
public:
		/** set layout DPI */
virtual	void				SetDPI( const GReal inDPI = 72) = 0;

		/** update layout metrics */ 
virtual void				UpdateLayout(VGraphicContext *inGC) = 0;

		/** render layout element in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f) = 0;

		/** update ascent (compute ascent relatively to line & text ascent, descent in px (relative to dpi set by SetDPI) - not including line embedded node(s) metrics) 
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateLayout
		*/
virtual	void				UpdateAscent(const GReal inLineAscent = 0, const GReal inLineDescent = 0, const GReal inTextAscent = 0, const GReal inTextDescent = 0) {}

		/** get ascent (in px - relative to dpi set by SetDPI)
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateAscent
		*/
virtual	GReal				GetAscent() const { return 0; }

		/** get descent (in px - relative to dpi set by SetDPI)
		@remarks
			mandatory only if node is embedded in a paragraph line (image for instance)
			should be called after UpdateAscent
		*/
virtual	GReal				GetDescent() const { return 0; }
	
		/** get design bounds (in px - relative to dpi set by SetDPI) */
virtual	const VRect&		GetDesignBounds() const = 0;

		/** get background bounds (in px - relative to dpi set by SetDPI) */
virtual	const VRect&		GetBackgroundBounds() const = 0;
};


/** base layout class
@remarks
	this class manages text element common layout (margins, padding, borders, etc...)
	all text element layout classes are derived from that class (but VDocBorderLayout because VDocBorderLayout instance is embedded in VDocBaseLayout)
*/
class VDocBaseLayout: public VObject, public IDocNodeLayout
{
public:
							VDocBaseLayout();
virtual						~VDocBaseLayout();

/////////////////	IDocNodeLayout interface

		/** set layout DPI */
virtual	void				SetDPI( const GReal inDPI = 72);

		/** get design bounds (in px - relative to fDPI) */
virtual	const VRect&		GetDesignBounds() const { return fDesignBounds; }

		/** get background bounds (in px - relative to fDPI) */
virtual	const VRect&		GetBackgroundBounds() const { return fBackgroundBounds; }

		/** render layout element in the passed graphic context at the passed top left origin
		@remarks		
			method does not save & restore gc context
		*/
virtual void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint(), const GReal inOpacity = 1.0f);

///////////////// 

		/** initialize properties from the passed node */
virtual	void				InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited = false);

		/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

		/** set outside margins	(in twips) - CSS margin */
virtual	void				SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin = true);

		/** set inside margins	(in twips) - CSS padding */
virtual	void				SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin = true);

		/** set width (in twips) - CSS width (0 for auto) - without margin, padding & border */
virtual	void				SetWidth(const uLONG inWidth) { fWidthTWIPS = inWidth; }
virtual uLONG				GetWidth(bool inIncludeAllMargins = false) const;

		/** set height (in twips) - CSS height (0 for auto) - without margin, padding & border */
virtual	void				SetHeight(const uLONG inHeight) { fHeightTWIPS = inHeight; }
virtual uLONG				GetHeight(bool inIncludeAllMargins = false) const;

		/** set min width (in twips) - CSS min-width (0 for auto) */
virtual	void				SetMinWidth(const uLONG inWidth) { fMinWidthTWIPS = inWidth; }
virtual uLONG				GetMinWidth(bool inIncludeAllMargins = false) const;

		/** set min height (in twips) - CSS min-height (0 for auto) */
virtual	void				SetMinHeight(const uLONG inHeight) { fMinHeightTWIPS = inHeight; }
virtual uLONG				GetMinHeight(bool inIncludeAllMargins = false) const;

		/** set text align - CSS text-align (0 for default) */
virtual void				SetTextAlign( const eStyleJust inHAlign) { fTextAlign = inHAlign; }
virtual eStyleJust			GetTextAlign() const { return fTextAlign; }

		/** set vertical align - CSS vertical-align (0 for default) */
virtual void				SetVerticalAlign( const eStyleJust inVAlign) { fVerticalAlign = inVAlign; }
virtual eStyleJust			GetVerticalAlign() const { return fVerticalAlign; }

		/** set background color - CSS background-color (transparent for not defined) */
virtual void				SetBackgroundColor( const VColor& inColor) { fBackgroundColor = inColor; }
virtual const VColor&		GetBackgroundColor() const { return fBackgroundColor; }

protected:
friend class				VDocParagraphLayout;
		
		/** create border from passed document node */
		void				_CreateBorder(const VDocNode *inDocNode, bool inCalcAllMargin = true);

		/** derived class should call this method when design bounds are changed - normally in UpdateLayout */
		void				_SetDesignBounds( const VRect& inBounds);

		void				_UpdateBorderWidth(bool inCalcAllMargin = true);
		void				_CalcAllMargin();

virtual void				_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw, const VPoint& inPreDecal);
virtual void				_EndLocalTransform( VGraphicContext *inGC, const VAffineTransform& inCTM, bool inDraw = false);

virtual void				_BeginLayerOpacity(VGraphicContext *inGC, const GReal inOpacity = 1.0f);
virtual void				_EndLayerOpacity(VGraphicContext *inGC, const GReal inOpacity = 1.0f);

		//design datas

		/** layout DPI (DPI used for computing metrics) */
		GReal						fDPI;

		/** drawing DPI (DPI used for rendering) */
		GReal						fTargetDPI;

		/** inside margin - CSS padding (in TWIPS) */
		sDocPropRect				fMarginInTWIPS;

		/** outside margin - CSS margin (in TWIPS) */
		sDocPropRect				fMarginOutTWIPS;
		
		/** width in TWIPS (0 for auto) - without margin, padding & border  */
		uLONG						fWidthTWIPS;

		/** height in TWIPS (0 for auto) - without margin, padding & border  */
		uLONG						fHeightTWIPS;

		/** min-width in TWIPS (0 for auto) */
		uLONG						fMinWidthTWIPS;

		/** min-height in TWIPS (0 for auto) */
		uLONG						fMinHeightTWIPS;

		/** text-align (0 for default) */
		eStyleJust					fTextAlign;

		/** vertical-align (0 for default) */
		eStyleJust					fVerticalAlign;

		/** background-color (alpha = 0 for transparent) */
		VColor						fBackgroundColor;

		/** border layout */
		VDocBorderLayout*				fBorderLayout;


		//computed datas


		/** inside margin - CSS padding (in px) */
		VTextLayout::sRectMargin	fMarginIn;

		/** outside margin - CSS margin (in px - relative to fDPI) */
		VTextLayout::sRectMargin	fMarginOut;

		/** borders width (in px - relative to fDPI) - border width in pt is stored in fBorderLayout */
		VTextLayout::sRectMargin	fBorderWidth;

		/** all margins = total of outside margin+border width+inside margin (in px - relative to fDPI) */
		VTextLayout::sRectMargin	fAllMargin;	

		/** design bounds (in px - relative to fDPI) 
		@remarks
			design bounds include margin, border & padding
		*/
		VRect						fDesignBounds;

		/** background bounds (in px - relative to fDPI) 
		@remarks
			background bounds include border & padding but not margin
		*/
		VRect						fBackgroundBounds;

		GReal						fOpacityBackup;
};

END_TOOLBOX_NAMESPACE

#endif
