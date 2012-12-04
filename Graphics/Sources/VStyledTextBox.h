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
//class VStyledTextBox
//{
//public:
//	virtual ~VStyledTextBox() {};
//	virtual Boolean GetNaturalHeight(long Width, long *pVal) = 0;
//	virtual Boolean GetNaturalWidth(long Height, long *pVal) = 0;
//	virtual Boolean Create() = 0;
//	virtual Boolean Draw(VRect& inBounds) = 0;
//	virtual Boolean GetRTFText(VString& outRTFText) = 0;
//	virtual Boolean SetRTFText(const VString& inRTFText) = 0;
//	virtual void	SetText(const VString& inText) = 0;
//	virtual void SetFont(VFont *font, VColor &textColor) = 0;
//	virtual void ApplyStyle(VTextStyle* inStyle) = 0;
//};

#ifndef __VSTYLEDTEXTBOX_H_
#define __VSTYLEDTEXTBOX_H_


BEGIN_TOOLBOX_NAMESPACE
class VTextStyle;

class XTOOLBOX_API VStyledTextBox : public VObject, public IRefCountable
{
protected:
	VStyledTextBox(const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont);
	void Initialize(const VString& inText, VTreeTextStyle *inStyles, const VColor& inTextColor, VFont *inFont);
public:
#if VERSIONWIN
	typedef std::pair<sLONG,sLONG> TextRange;
	typedef std::vector<TextRange> VectorOfTextRange;
#endif

	static VStyledTextBox *CreateImpl(ContextRef inContextRef, const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont, const GReal inRefDocDPI = 72.0f, bool inUseNSAttributes = false);

	/** change drawing context 
	@remarks
		by default, drawing context is the one used to initialize instance

		you can draw to another context but it assumes new drawing context is compatible with the context used to initialize the text box
		(it is caller responsibility to check new context compatibility: normally it is necessary only on Windows because of GDI resolution dependent DPI)
	*/
	virtual void SetDrawContext( ContextRef inContextRef) { xbox_assert(false); }

	/** insert text at the specified position */
	virtual void InsertText( sLONG inPos, const VString& inText) {}

	/** delete text range */
	virtual void DeleteText( sLONG rangeStart, sLONG rangeEnd) {}

	/** replace text range */
	virtual void ReplaceText( sLONG rangeStart, sLONG rangeEnd, const VString& inText) 
	{
		DeleteText( rangeStart, rangeEnd);
		InsertText( rangeStart, inText);
	}
	
	/** apply style (use style range) */
	virtual void ApplyStyle( VTextStyle* inStyle) {}

	Boolean Draw(const VRect& inBounds);
	void GetSize(GReal &ioWidth, GReal &outHeight);

	/** get plain text */
	virtual Boolean GetPlainText( VString& outText) { return false; }

	/** get uniform style on the specified range 
	@remarks
		return end location of uniform range
	*/
	virtual sLONG GetStyle(VTextStyle* ioStyle, sLONG rangeStart, sLONG rangeEnd) { return rangeStart; }
	
	/** get vector of uniform styles from the specified range */
	sLONG GetAllStyles(std::vector<VTextStyle*>& outStyles, sLONG inStart, sLONG inEnd);

	virtual Boolean GetRTFText(/*[out, retval]*/ VString& outRTFText) { return false; }
	virtual Boolean	SetRTFText(/*[in]*/const VString& inRTFText) { return false; }

#if VERSIONMAC	
	/** set min line height for the specified range */
	virtual void	SetMinLineHeight( const GReal inMinHeight, sLONG inStart, sLONG inEnd) {}
	
	virtual Boolean	SetHTMLText(VHandle inHandleHTML) { return false; }
	
	virtual VHandle GetRTFText() { return NULL; }
	virtual Boolean	SetRTFText(VHandle inHandleRTF) { return false; }
#endif

#if VERSIONWIN
	/** return ranges of hidden text */
	virtual void GetRangesHidden( VectorOfTextRange& outRanges) {}
#endif

protected:
#if !VERSIONMAC	
	/** get uniform style on the specified range 
	 @remarks
		return false if style is not uniform 
	 */
	virtual bool _GetStyle(VTextStyle* ioStyle, sLONG rangeStart, sLONG rangeEnd) { return false; }

	/** get uniform style on the specified range
	@remarks
		return end limit of the uniform style
	*/
	virtual sLONG _GetUniformStyle(VTextStyle *styleUniform, sLONG inStart, sLONG inEnd);
#endif
	
	/** set document reference DPI
	@remarks
		by default it is assumed to be 72 so fonts are properly rendered in 4D form on Windows
		but you can override it if needed

		this is actual DPI used by internal fonts

		on Windows, because 4D forms are not dpi-aware, font point size is scaled to inDPI/screen dpi
		(so by default as inDPI = 72, a font with point size = 12 in 4D form will have 12*72/screen dpi actual point size & so font pixel size = 12)

		on Mac, font point size is scaled to inDPI/72
		(so by default as inDPI = 72, font point size = pixel point size - because Quartz2D uses dpi = 72)
	*/
	virtual void _SetDocumentDPI( const GReal inDPI) {}

	virtual Boolean DoInitialize() = 0;
	virtual void DoRelease() = 0;
	virtual Boolean DoDraw(const VRect& inBounds) = 0;
	virtual void DoGetSize(GReal &ioWidth, GReal &outHeight) = 0;
	virtual void DoApplyStyle(VTextStyle* inStyle, VTextStyle *inStyleInherit = NULL) = 0;
	virtual void _SetFont(VFont *font, const VColor &textColor) = 0;
	virtual void _SetText(const VString& inText) = 0;
	virtual	void DoOnRefCountZero ();
	~VStyledTextBox() {};

	void _ApplyAllStyles(XBOX::VTreeTextStyle* inStyles, VTextStyle *inStyleInherit = NULL);
	
	bool fInitialized;
};

END_TOOLBOX_NAMESPACE

#endif //__VSTYLEDTEXTBOX_H_
