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
#ifndef __XWINSTYLEDTEXTBOX_H_
#define __XWINSTYLEDTEXTBOX_H_

#include <richedit.h>
#include <textserv.h>
#include "VStyledTextBox.h"

class ITextServices;
struct ITextDocument;
struct ITextRange;

#ifndef LY_PER_INCH
#define LY_PER_INCH   1440
#define HOST_BORDER 0
#endif

typedef struct StreamCookie
{
	BSTR	fText;
	DWORD	fSize;
	DWORD	fCount;
} StreamCookie;

typedef struct pp_charformatw
{
	UINT		cbSize;
	DWORD		dwMask;
	DWORD		dwEffects;
	LONG		yHeight;
	LONG		yOffset;
	COLORREF	crTextColor;
	BYTE		bCharSet;
	BYTE		bPitchAndFamily;
	WCHAR		szFaceName[LF_FACESIZE];
} ppCHARFORMATW;
struct ppCHARFORMAT2W : pp_charformatw
{
	WORD		wWeight;			// Font weight (LOGFONT value)
	SHORT		sSpacing;			// Amount to space between letters
	COLORREF	crBackColor;		// Background color
	LCID		lcid;				// Locale ID
	DWORD		dwReserved;			// Reserved. Must be 0
	SHORT		sStyle;				// Style handle
	WORD		wKerning;			// Twip size above which to kern char pair
	BYTE		bUnderlineType;		// Underline type
	BYTE		bAnimation;			// Animated text like marching ants
	BYTE		bRevAuthor;			// Revision author index
};

BEGIN_TOOLBOX_NAMESPACE
class VTextStyle;



/////////////////////////////////////////////////////////////////////////////
// CFormattedTextDraw
class XWinStyledTextBox : 
	public ITextHost,
	public VStyledTextBox
{
public:
	XWinStyledTextBox(HDC inHDC, const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont, const TextLayoutMode inLayoutMode = TLM_NORMAL, const GReal inRefDocDPI = 72.0f);
// Minimal COM functionality
    HRESULT STDMETHODCALLTYPE QueryInterface( 
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject);
    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);

	/** return the caret position & height of text at the specified text position
	@remarks
		text position is 0-based

		output caret position is in GDI current user space (so is bounded by inHwndBounds)

		caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

		by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
		but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
	*/
	void	GetCaretMetricsFromCharIndex( const VRect& inBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading = true, const bool inCaretUseCharMetrics = true);

	/** return the text position at the specified coordinates
	@remarks
		output text position is 0-based

		input coordinates are in GDI current user space

		return true if input position is inside character bounds
		if method returns false, it returns the closest character index to the input position
	*/
	bool	GetCharIndexFromCoord( const VRect& inBounds, const VPoint& inPos, VIndex& outCharIndex);

	/** return styled text box run bounds from the specified range	*/
	void	GetRunBoundsFromRange( const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart = 0, sLONG inEnd = -1);

	Boolean GetRTFText(/*[out, retval]*/ VString& outRTFText);

	Boolean	SetRTFText(/*[in]*/const VString& inRTFText);

	/** get plain text 
	@note
		override from VStyledTextBox
	*/
	Boolean GetPlainText( VString& outText);

	/** get uniform style on the specified range 
	@remarks
		return end location of uniform range
	*/
	sLONG GetStyle(VTextStyle* ioStyle, sLONG rangeStart, sLONG rangeEnd)
	{
		return _GetUniformStyle( ioStyle, rangeStart, rangeEnd);
	}

	/** return ranges of hidden text */
	void GetRangesHidden( VectorOfTextRange& outRanges);

	
	/** change drawing context 
	@remarks
		by default, drawing context is the one used to initialize instance

		you can draw to another context but it assumes new drawing context is compatible with the context used to initialize the text box
		(it is caller responsibility to check new context compatibility: normally it is necessary only on Windows because of GDI resolution dependent DPI)
	*/
	virtual void SetDrawContext( ContextRef inContextRef);

	/** insert text at the specified position */
	virtual void InsertText( sLONG inPos, const VString& inText);

	/** delete text range */
	virtual void DeleteText( sLONG rangeStart, sLONG rangeEnd);

	/** apply style (use style range) */
	virtual void ApplyStyle( VTextStyle* inStyle) { DoApplyStyle( inStyle); }

protected:
	/** get uniform style on the specified range 
	 @remarks
		return false if style is not uniform 
	 */
	virtual bool _GetStyle(VTextStyle* ioStyle, sLONG rangeStart, sLONG rangeEnd);

	/** get plain text */
	const VString& _GetPlainText();

	bool _VTextStyleFromCharFormat(VTextStyle* ioStyle, CHARFORMAT2W* inCharFormat);

	/** set document reference DPI
	@remarks
		by default it is assumed to be 72 so fonts are properly rendered in 4D form on Windows
		but you can override it if needed
	*/
	void _SetDocumentDPI( const GReal inDPI);

	/** clear hidden text 
	@remarks
		plain text includes hidden text so call this method to remove hidden text
	*/
	void _ClearHidden();

	/** return true if full range is hidden */
	bool _IsRangeHidden( sLONG inStart, sLONG inEnd, bool &isMixed); 


	// From VStyledTextBox
	Boolean DoInitialize();
	void DoRelease();
	Boolean DoDraw(const VRect& inBounds);
	void DoGetSize(GReal &ioWidth, GReal &outHeight);
	void DoApplyStyle(VTextStyle* inStyle, VTextStyle *inStyleInherit = NULL);
	void _SetFont(VFont *font, const VColor &textColor);
	void _SetText(const VString& inText) { _SetTextEx( inText); }
	void _SetTextEx(const VString& inText, bool inSelection = false);

	/** compute x offset
	@remarks
		see fOffsetX comment
	*/
	void _ComputeOffsetX();

	GReal _GetOffsetX() const;

	void _EnableMultiStyle();

	// IFormattedTextDraw
private:
	virtual ~XWinStyledTextBox() {};

	//Boolean Create();
	//Boolean Draw(VRect &inBounds);
	//void	SetText(/*[in]*/const VString& inText);
	//void SetFont(VFont *font, VColor &textColor);
	//void ApplyStyle(XBOX::VTextStyle* inStyle);

// ITextHost
	HDC TxGetDC();
	INT TxReleaseDC(HDC hdc);
	BOOL TxShowScrollBar(INT fnBar, BOOL fShow);
	BOOL TxEnableScrollBar(INT fuSBFlags, INT fuArrowflags);
	BOOL TxSetScrollRange(INT fnBar, LONG nMinPos, INT nMaxPos, BOOL fRedraw);
	BOOL TxSetScrollPos(INT fnBar, INT nPos, BOOL fRedraw);
	void TxInvalidateRect(LPCRECT prc, BOOL fMode);
	void TxViewChange(BOOL fUpdate);
	BOOL TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight);
	BOOL TxShowCaret(BOOL fShow);
	BOOL TxSetCaretPos(INT x, INT y);
	BOOL TxSetTimer(UINT idTimer, UINT uTimeout);
	void TxKillTimer(UINT idTimer);
	void TxScrollWindowEx(INT dx, INT dy, LPCRECT lprcScroll, LPCRECT lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate, UINT fuScroll);
	void TxSetCapture(BOOL fCapture);
	void TxSetFocus();
	void TxSetCursor(HCURSOR hcur, BOOL fText);
	BOOL TxScreenToClient(LPPOINT lppt);
	BOOL TxClientToScreen(LPPOINT lppt);
	HRESULT	TxActivate(LONG * plOldState);
	HRESULT	TxDeactivate(LONG lNewState);
	HRESULT	TxGetClientRect(LPRECT prc);
	HRESULT	TxGetViewInset(LPRECT prc);
	HRESULT TxGetCharFormat(const CHARFORMATW **ppCF);
	HRESULT	TxGetParaFormat(const PARAFORMAT **ppPF);
	COLORREF TxGetSysColor(int nIndex);
	HRESULT	TxGetBackStyle(TXTBACKSTYLE *pstyle);
	HRESULT	TxGetMaxLength(DWORD *plength);
	HRESULT	TxGetScrollBars(DWORD *pdwScrollBar);
	HRESULT	TxGetPasswordChar(TCHAR *pch);
	HRESULT	TxGetAcceleratorPos(LONG *pcp);
	HRESULT	TxGetExtent(LPSIZEL lpExtent);
	HRESULT OnTxCharFormatChange(const CHARFORMATW * pcf);
	HRESULT	OnTxParaFormatChange(const PARAFORMAT * ppf);
	HRESULT	TxGetPropertyBits(DWORD dwMask, DWORD *pdwBits);
	HRESULT	TxNotify(DWORD iNotify, void *pv);
	HIMC TxImmGetContext();
	void TxImmReleaseContext(HIMC himc);
	HRESULT	TxGetSelectionBarWidth(LONG *lSelBarWidth);

// Custom functions
	bool CharFormatFromVTextStyle(CHARFORMAT2W* ioCharFormat, VTextStyle* inStyle, HDC hdc);
	HRESULT CharFormatFromHFONT(CHARFORMAT2W* ioCharFormat, HFONT hFont, const VColor &textColor, HDC hdc);
	HRESULT InitDefaultCharFormat(HDC hdc);
	HRESULT InitDefaultParaFormat();
	HRESULT CreateTextServicesObject();

// Variables
	RECT			fClientRect;			// Client Rect
	RECT			fViewInset;		// view rect inset
	SIZEL			fExtent;		// Extent array

	int				nPixelsPerInchX;    // Pixels per logical inch along width
	int				nPixelsPerInchY;    // Pixels per logical inch along height

	CHARFORMAT2W	fCharFormat;
	PARAFORMAT2		fParaFormat;
	DWORD			m_dwPropertyBits;	// Property bits
	DWORD			m_dwMaxLength;
	StreamCookie	fStreamCookie;

	ITextServices	*fTextServices;
	ITextDocument	*fTextDocument;
	ITextRange		*fTextRange;
	VString			fText;
	LONG			fRefCount;					// Reference Count
	HDC				fHDC;
	HDC				fHDCMetrics;
	bool			fIsPlainTextDirty;

	/** ratio to apply to fontsize in order to take account 4D form 72 dpi for instance on Windows 
	@remarks
		by default it is set automatically to 72.0/VWinGDIGraphicContext::GetLogPixelsY() to ensure consistency with 4D form
	*/
	GReal			fFontSizeToDocumentFontSize;

	TextLayoutMode		fLayoutMode;
	justificationStyle	fJust;

	/** RTE adds a extra left margin even if we do not request it (...)
		so we need to decal textbox x origin with offset=-left margin
		otherwise text is always decaled to the right by left margin pixels;
		for consistency, this offset is used to determine text box size too (ie text box actual width = RTE text box width-fabs(fOffsetX))
		(we determine the left margin using the left x of the first character 
		 so if this bug is fixed in a future RTE version it should be fine)
	*/
	GReal			fOffsetX;
};
END_TOOLBOX_NAMESPACE

#endif //__XWINSTYLEDTEXTBOX_H_
