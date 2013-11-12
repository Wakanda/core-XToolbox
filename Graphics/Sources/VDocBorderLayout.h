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
#ifndef __VBorderLayout__
#define __VBorderLayout__

BEGIN_TOOLBOX_NAMESPACE

class VDocImageLayout;
class VDocParagraphLayout;

/** border layout class 
@remarks
	class dedicated to manage & render a text element border 
@note
	class does not derive from VDocBaseLayout because it is embedded in VDocBaseLayout (as borders are common to almost all text elements)

	corner radius is applied if and only if all borders use same style & only for styles from kTEXT_BORDER_STYLE_DOTTED to kTEXT_BORDER_STYLE_DOUBLE
	TODO: manage also border background image (cf CSS style background-image)
*/ 
class VDocBorderLayout: public VObject
{
public:
typedef	struct Border
{
		eDocPropBorderStyle	fStyle;		//border style
		GReal				fWidth;		//border width (in point)
		VColor				fColor;		//border color
		GReal				fRadius;	//border corner radius (in point)

		bool operator == (const Border& inBorder); 
		bool operator != (const Border& inBorder) { return (!(*this == inBorder)); }

		bool ShouldClip() const { return fStyle >= kTEXT_BORDER_STYLE_GROOVE && fStyle <= kTEXT_BORDER_STYLE_OUTSET; }
} Border;


							/** create border 
							@remarks
								if inLeft is NULL, left border is same as right border 
								if inBottom is NULL, bottom border is same as top border
								if inRight is NULL, right border is same as top border
							*/
							VDocBorderLayout( const Border *inTop = NULL, const Border *inRight = NULL, const Border *inBottom = NULL, const Border *inLeft = NULL);

							/** create from the passed document node 
							@remarks
								only border properties are imported
							*/
							VDocBorderLayout(const VDocNode *inNode);

							/** set node properties from the current properties */
virtual	void				SetPropsTo( VDocNode *outNode);

		const Border&		GetLeftBorder() const { return fBorders.fLeft; }
		const Border&		GetRightBorder() const { return fBorders.fRight; }
		const Border&		GetTopBorder() const { return fBorders.fTop; }
		const Border&		GetBottomBorder() const { return fBorders.fBottom; }

							/** set border drawing rect (in px relative to inDPI)
							@param inRect
								border drawing rect (in px relative to inDPI)
							@param inDPI
								desired dpi (width & radius - design unit is point - are scaled to inDPI/72)
								(if parent class is derived from VDocBaseLayout, it should be equal to VDocBaseLayout::fDPI - not fTargetDPI)
							@remarks
								rect is aligned to border center (so inflate -0.5*border width for exterior bounds & +0.5*border width for interior bounds)
								path & clipping paths if needed are precomputed here
							*/
		void				SetDrawRect( const VRect& inRect, const GReal inDPI = 72);
		const VRect&		GetDrawRect() const { return fDrawRect; }

							/** get clip rectangle if border is drawed in the passed gc at the passed top left position 
							@remarks
								clip rectangle takes account crisp edges status
							*/
		void				GetClipRect(VGraphicContext *inGC, VRect& outClipRect, const VPoint& inTopLeft = VPoint()) const;

							/** render border 
							@param inGC
								graphic context

							@remarks
								method does not save & restore gc context

								it is assumed 1pt=1px so you should pass dpi to SetRect in order width & radius are scaled according to another dpi than 72
								(only for onscreen drawing)

								caller should enable crisp edges in order borders are properly aligned on pixel grid
							*/
		void				Draw( VGraphicContext *inGC, const VPoint& inTopLeft = VPoint());


virtual						~VDocBorderLayout();

protected:
		typedef std::vector<VPoint> VectorOfPoint;

friend	class				VDocParagraphLayout;
friend	class				VDocImageLayout;

		void				_CheckRadius();
		void				_CheckClip();

		void				_GetAltColor(const VColor& inColor, VColor& outColor);

		void				_BuildPath(VGraphicPath *&path, const VectorOfPoint& inPoints);

		void				_SetLinePatternFromBorderStyle( VGraphicContext *inGC, eDocPropBorderStyle inStyle);

typedef	struct BorderRect
{
		Border				fTop;		//top-left for radius
		Border				fRight;		//top-right for radius
		Border				fBottom;	//bottom-right for radius
		Border				fLeft;		//bottom-left for radius

} BorderRect;

typedef struct ClipRect
{
		VGraphicPath		*fTop;
		VGraphicPath		*fRight;
		VGraphicPath		*fBottom;
		VGraphicPath		*fLeft;
} ClipRect;

		//design datas

		BorderRect			fBorders;
		
		GReal				fDPI;

		//computed datas

		VRect				fDrawRect;

		bool				fNeedClip;
		ClipRect			fClipping;

		GReal				fMaxWidth;
};



END_TOOLBOX_NAMESPACE

#endif
