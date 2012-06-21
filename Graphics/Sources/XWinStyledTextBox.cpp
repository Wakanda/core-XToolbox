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
#include "VGraphicsPrecompiled.h"
#define UNICODE
#include "vfont.h"
#include "vcolor.h"
#include <windows.h>
#include <stddef.h>
#include <windowsx.h>
#include <richedit.h>
#include <textserv.h>
#include <Tom.h>
#include <tchar.h>
#include "VRect.h"
#include "XWinStyledTextBox.h"
#include "XWinGDIGraphicContext.h"

//ensure we can use std::min & std::max
#undef min
#undef max
#include <stdio.h>

EXTERN_C const IID IID_ITextServices = { // 8d33f740-cf58-11ce-a89d-00aa006cadc5
    0x8d33f740,
    0xcf58,
    0x11ce,
    {0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5}
  };
EXTERN_C const IID IID_ITextHost = { // {D764B0BB-39E9-4307-9CA7-707FB2C1B689}
    0xd764b0bb,
    0x39e9,
    0x4307,
    { 0x9c, 0xa7, 0x70, 0x7f, 0xb2, 0xc1, 0xb6, 0x89 }
  };
EXTERN_C const IID IID_ITextDocument = {
	0x8CC497C0,
	0xA1DF,
	0x11CE,
	{0x80, 0x98, 0x00, 0xAA, 0x00, 0x47, 0xBE, 0x5D}
};
class xBSTR
{
	public:
	xBSTR()
	{
		fString =NULL;
	}
	xBSTR(const XBOX::VString& inStr)
	{
		if(fString)
			fString = SysAllocString(inStr.GetCPointer());
	}
	~xBSTR()
	{
		SysFreeString(fString);
	}
	void ToVString(XBOX::VString& outStr)
	{
		outStr.Clear();
		if(fString)
			outStr.FromUniCString(fString);
	}
	operator BSTR()
	{
		return fString;
	}
	operator BSTR*()
	{
		return &fString;
	}
	BSTR fString;
};

GReal gOffsetX = 0.0f;

// HIMETRIC units per inch (used for conversion)
#define HIMETRIC_PER_INCH 2540.0

// Convert Himetric along the X axis to X pixels
LONG HimetricXtoDX(LONG xHimetric, LONG xPerInch)
{
	return (LONG) MulDiv(xHimetric, xPerInch, HIMETRIC_PER_INCH);
}

// Convert Himetric along the Y axis to Y pixels
LONG HimetricYtoDY(LONG yHimetric, LONG yPerInch)
{
	return (LONG) MulDiv(yHimetric, yPerInch, HIMETRIC_PER_INCH);
}

// Convert Pixels on the X axis to Himetric
LONG DXtoHimetricX(LONG dx, LONG xPerInch)
{
	return (LONG) MulDiv(dx, HIMETRIC_PER_INCH, xPerInch);
}

// Convert Pixels on the Y axis to Himetric
LONG DYtoHimetricY(LONG dy, LONG yPerInch)
{
	return (LONG) MulDiv(dy, HIMETRIC_PER_INCH, yPerInch);
}

/////////////////////////////////////////////////////////////////////////////
// CallBack functions

DWORD CALLBACK EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	StreamCookie* pCookie = (StreamCookie*) dwCookie;

	if(pCookie->fSize - pCookie->fCount < (DWORD) cb)
		*pcb = pCookie->fSize - pCookie->fCount;
	else
		*pcb = cb;
	CopyMemory(pbBuff, ((char *)(pCookie->fText))+pCookie->fCount, *pcb);
	pCookie->fCount += *pcb;

	return 0;	//	callback succeeded - no errors
}

DWORD CALLBACK EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	StreamCookie* pCookie = (StreamCookie*) dwCookie;

	pCookie->fSize += cb;
	*pcb = cb;

	char * ptr = (char*)(pCookie->fText);
	if(pCookie->fText)
		CopyMemory(ptr+pCookie->fCount, pbBuff, *pcb);
	pCookie->fCount += cb;
	return 0;	//	callback succeeded - no errors
}

USING_TOOLBOX_NAMESPACE

/////////////////////////////////////////////////////////////////////////////
// XWinStyledTextBox
XWinStyledTextBox::XWinStyledTextBox(HDC inHDC, const VString& inText, VTreeTextStyle *inStyles, const VRect& inHwndBounds, const VColor& inTextColor, VFont *inFont, const TextLayoutMode inLayoutMode, const GReal inRefDocDPI)
: fHDC(inHDC), fHDCMetrics(NULL), VStyledTextBox(inText, inStyles, inHwndBounds, inTextColor, inFont)
{
	VRect bounds(inHwndBounds);
	bounds.NormalizeToInt(false); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	RECT rt = bounds;

	SetDrawContext(inHDC);

	//by default set reference DPI to 4D form DPI 
	_SetDocumentDPI(inRefDocDPI);

	fLayoutMode = inLayoutMode;
	Initialize(inText, inStyles, inTextColor, inFont);
	fClientRect = rt;
	_ComputeOffsetX();
}


/** change drawing context 
@remarks
	by default, drawing context is the one used to initialize instance

	you can draw to another context but it assumes new drawing context is compatible with the context used to initialize the text box
	(it is caller responsibility to check new context compatibility: normally it is necessary only on Windows because of GDI resolution dependent DPI)
*/
void XWinStyledTextBox::SetDrawContext( ContextRef inContextRef)
{
	fHDC = inContextRef;
	if (!fHDC)
	{
		if (!fHDCMetrics)
			fHDCMetrics = ::CreateCompatibleDC( NULL); //use screen compatible dc (only valid for metrics: fonts metrics are scaled internally from screen dpi to document dpi - 72 on default for 4D form compat)
		fHDC = fHDCMetrics;
	}
	nPixelsPerInchX = GetDeviceCaps(fHDC, LOGPIXELSX);
	nPixelsPerInchY = GetDeviceCaps(fHDC, LOGPIXELSY);
}

/** compute x offset
@remarks
	see fOffsetX comment
*/
void XWinStyledTextBox::_ComputeOffsetX()
{
	if (fJust == JST_Right || fJust == JST_Center)
	{
		if (gOffsetX)
			fOffsetX = gOffsetX;
		else
			fOffsetX = -4.0f; 
		return;
	}
	if (!gOffsetX)
	{
		VRect bounds(fClientRect.left, fClientRect.top, fClientRect.right-fClientRect.left, fClientRect.bottom-fClientRect.top);
		if (bounds.GetWidth() == 0.0f)
			bounds.SetWidth(100.0f);
		if (bounds.GetHeight() == 0.0f)
			bounds.SetHeight(100.0f);
		VPoint pos;
		GReal height;
		fOffsetX = 0.0f;
		GetCaretMetricsFromCharIndex(bounds, 0, pos, height); 
		gOffsetX = -(pos.GetX()-fClientRect.left);
	}
	fOffsetX = gOffsetX;
}


/** set document reference DPI
@remarks
	by default it is assumed to be 72 so fonts are properly rendered in 4D form on Windows
	but you can override it if needed
*/
void XWinStyledTextBox::_SetDocumentDPI( const GReal inDPI)
{
	fFontSizeToDocumentFontSize = inDPI / nPixelsPerInchY;
}

Boolean XWinStyledTextBox::GetPlainText( VString& outText)
{
	if (!fIsPlainTextDirty || !fTextServices)
	{
		outText.FromString(fText);
		return TRUE;
	}
	
	fIsPlainTextDirty = false;

	BSTR bStr = NULL;
	fTextServices->TxGetText(&bStr);
	LONG len = ::SysStringLen(bStr);
	if (!bStr)
	{
		fText.SetEmpty();
		outText.SetEmpty();
		return FALSE;
	}

	fText.FromUniCString( bStr);
	fText.ExchangeAll(0x0B,0x0D);

	outText.FromString(fText);
	::SysFreeString( bStr);

	return TRUE;
}

/** get plain text */
const VString& XWinStyledTextBox::_GetPlainText()
{
	if (!fIsPlainTextDirty || !fTextServices)
		return fText;

	fIsPlainTextDirty = false;

	BSTR bStr = NULL;
	fTextServices->TxGetText(&bStr);
	LONG len = ::SysStringLen(bStr);
	if (!bStr)
	{
		fText.SetEmpty();
		return fText;
	}

	fText.FromUniCString( bStr);
	fText.ExchangeAll(0x0B,0x0D);

	::SysFreeString( bStr);

	return fText;
}

/**
*  Tests whether the given character is a valid space.
*/
static bool isSpace( unsigned char c )
{
  if (c == 32)
	  return true;
  return ( c < 32 ) &&
        ( ( ( ( ( 1L << '\t' ) |
        ( 1L << '\n' ) |
        ( 1L << '\r' ) |
        ( 1L << '\f' ) ) >> c ) & 1L ) != 0 );
}

Boolean XWinStyledTextBox::GetRTFText(VString& outRTFText)
{
	HRESULT hr;
	LRESULT lResult = 0;
	EDITSTREAM editStream;

	if (!fTextServices) 
		return FALSE;

	//get size of RTF text
	fStreamCookie.fText = NULL;
	fStreamCookie.fSize = 0;
	fStreamCookie.fCount = 0;

	editStream.dwCookie = (DWORD_PTR) &fStreamCookie;
	editStream.dwError = 0;
	editStream.pfnCallback = EditStreamOutCallback;
	hr = fTextServices->TxSendMessage(EM_STREAMOUT, (WPARAM)SF_RTF, (LPARAM)&editStream, &lResult );
	if (FAILED(hr))
		return FALSE;

	//stream out RTF text
	fStreamCookie.fText = (BSTR)malloc(fStreamCookie.fSize+2);
	if (!fStreamCookie.fText)
		return FALSE;

	memset(fStreamCookie.fText, 0, fStreamCookie.fSize+2);
	fStreamCookie.fSize = 0;
	fStreamCookie.fCount = 0;

	editStream.dwCookie = (DWORD_PTR) &fStreamCookie;
	editStream.dwError = 0;
	editStream.pfnCallback = EditStreamOutCallback;
	lResult = 0;
	hr = fTextServices->TxSendMessage(EM_STREAMOUT, (WPARAM)SF_RTF, (LPARAM)&editStream, &lResult);
	if (hr == S_OK)
	{
		//JQ: workaround for buggy EM_STREAMOUT: even without setting SFF_WRITEXTRAPAR, 
		//	  calling EM_STREAMOUT with SF_RTF adds a extra \fs?? & \par each time we call EM_STREAMOUT which is quite nasty...
		//
		//    the only workaround i have found is to trim extra \par & \fs at the end 

		//prepare reverse parsing
		unsigned char *cStart = (unsigned char *)((char *)fStreamCookie.fText);
		unsigned char *c = cStart+strlen((char *)fStreamCookie.fText);
		unsigned char *cPar = c;
		unsigned char *cParNext = c;

		int numPar = 0;

		c--;
		//skip ending spaces
		while (c >= cStart && isSpace( *c))
			c--;
		if (c >= cStart && *c == '}') //skip end bracket
		{
			while(true)
			{
				//skip ending spaces
				c--;
				while (c >= cStart && isSpace( *c))
					c--;
				if (c < cStart || *c == '{' || *c == '}')
					break;
				if (c-3 >= cStart
					&&
					*(c-3) == '\\'
					&&
					*(c-2) == 'p'
					&&
					*(c-1) == 'a'
					&&
					*(c) == 'r'
					)
				{
					//trim only last \par
					if (numPar)
						break;
					else
					{
						numPar++;
						cParNext = cPar;
						cPar = c-3;
						c = c-3;
					}
				}
				else if (c-4 >= cStart
						&&
						*(c-4) == '\\'
						&&
						*(c-3) == 'f'
						&&
						*(c-2) == 's'
						&&
						*(c-1) >= '0' && *(c-1) <= '9'
						&&
						*(c-0) >= '0' && *(c-0) <= '9'
						)
					 {
						//trim \fs?? tag
						cParNext = c-4;
						cPar = c-4;
						c = c-4;
					 }
					 else if	(c-5 >= cStart
								&&
								*(c-5) == '\\'
								&&
								*(c-4) == 'f'
								&&
								*(c-3) == 's'
								&&
								*(c-2) >= '0' && *(c-2) <= '9'
								&&
								*(c-1) >= '0' && *(c-1) <= '9'
								&&
								*(c-0) >= '0' && *(c-0) <= '9'
								)
								{
									//trim \fs??? tag
									cParNext = c-5;
									cPar = c-5;
									c = c-5;
								}
								else
									break; //either plain text or rtf tag but \par or \fs?? -> stop parsing
				
			}

			if (*cPar)
			{
				*cPar = '}'; //trim ending \par or \fs?? 
				*(cPar+1) = 0;
			}
		}

		outRTFText.FromCString( (char *)fStreamCookie.fText);
		free( fStreamCookie.fText);

		return TRUE;
	}
	free( fStreamCookie.fText);

	return FALSE;
}

Boolean XWinStyledTextBox::SetRTFText(const VString& inRTFText)
{
	HRESULT hr;
	long    len;
	LRESULT lResult = 0;
	EDITSTREAM editStream;

	len = inRTFText.GetLength()*sizeof(UniChar);

	if (!fTextServices) 
		return FALSE;

	_EnableMultiStyle();

	VSize size = inRTFText.GetLength()+1;
	char *buffer = (char*) malloc(size);
	if (!buffer)
		return FALSE;
	size = inRTFText.ToBlock(buffer, size, VTC_DefaultTextExport, true, false);

	fStreamCookie.fText = (BSTR)buffer;
	fStreamCookie.fSize = size;
	fStreamCookie.fCount = 0;

	editStream.dwCookie = (DWORD_PTR) &fStreamCookie;
	editStream.dwError = 0;
	editStream.pfnCallback = EditStreamInCallback;
	hr = fTextServices->TxSendMessage(EM_STREAMIN, (WPARAM)(SF_RTFNOOBJS), (LPARAM)&editStream, &lResult);
	free(buffer);

	fIsPlainTextDirty = true;

	//ensure hidden text is not included with plain text
	//_ClearHidden(); //FIXME: using ITextServices to clear hidden text seems to be buggy randomly (although hidden text ranges are correct) so for now defer it to caller
	
	return (hr == S_OK);
}

void XWinStyledTextBox::_SetTextEx(const VString& inText, bool inSelection)
{
	LRESULT lResult = 0;
	SETTEXTEX  pST;
	sLONG len;
	pST.flags= inSelection ? ST_SELECTION : ST_DEFAULT;
	pST.codepage = 1200;//Unicode
	if (inSelection)
	{
		HRESULT hr = fTextServices->TxSendMessage(EM_SETTEXTEX, (WPARAM) (&pST),(LPARAM)inText.GetCPointer(), &lResult );
		fIsPlainTextDirty = true;
		return;
	}
	fText.FromString(inText);
	HRESULT hr = fTextServices->TxSendMessage(EM_SETTEXTEX, (WPARAM) (&pST),(LPARAM)fText.GetCPointer(), &lResult );
	fIsPlainTextDirty = false;
}

Boolean XWinStyledTextBox::DoDraw(const XBOX::VRect &inBounds)
{
	VRect bounds(inBounds);
	bounds.NormalizeToInt(false); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	HRESULT hr = S_OK;

	//optimization: do not draw outside clip rect (!)
	RECT updateRect;
	int type;

	POINT viewportOffset;
	::GetViewportOrgEx( fHDC, &viewportOffset);
	int gm = ::GetGraphicsMode( fHDC);
	if (gm == GM_ADVANCED && (viewportOffset.x || viewportOffset.y))
	{
		//get clip box in device space
		{
		StGDIResetTransform resetCTM(fHDC);
		type = ::GetClipBox( fHDC, &updateRect);
		}
		::OffsetRect( &updateRect, -viewportOffset.x, -viewportOffset.y); //transform to logical coord space

		//transform to user space
		VAffineTransform ctm;
		XFORM xform;
		::GetWorldTransform( fHDC, &xform);
		ctm.FromNativeTransform( xform);
		if (!ctm.IsIdentity())
		{
			VRect bounds(updateRect);
			bounds = ctm.Inverse().TransformRect( bounds);
			updateRect = bounds;
		}
	}
	else
	{
		type = ::GetClipBox( fHDC, &updateRect);
		::OffsetRect( &updateRect, -viewportOffset.x, -viewportOffset.y); //transform to logical coord space
	}
	if (type == NULLREGION)
		return TRUE;
	else if (type == -1 || type == ERROR)
		updateRect = bounds;

	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);;
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);;
	hr = fTextServices->TxDraw(
	    DVASPECT_CONTENT,  		// Draw Aspect
		0,						// Lindex
		NULL,					// Info for drawing optimization
		NULL,					// target device information
		fHDC,				// Draw device HDC
		NULL,			 	   	// Target device HDC
		(RECTL *) &rt,	// Bounding client rectangle
		NULL,					// Clipping rectangle for metafiles
		(RECT *) &updateRect,   // Update rectangle
		NULL, 	   				// Call back function
		NULL,					// Call back parameter
		TXTVIEW_ACTIVE);		// What view of the object could be TXTVIEW_ACTIVE
	return (hr == S_OK);
}


/** return the caret position & height of text at the specified text position
@remarks
	text position is 0-based

	output caret position is in GDI current user space (so is bounded by inHwndBounds)

	caret should be drawed from outCaretPos to VPoint(outCaretPos.x,OutCaretPos.y+outTextHeight)

	by default inCaretUseCharMetrics = true, so output caret metrics are based on the run metrics of the input character (suitable to draw caret)
	but if inCaretUseCharMetrics = false, output caret metrics are based on the full line metrics (use this one to get line height)
*/
void XWinStyledTextBox::GetCaretMetricsFromCharIndex( const VRect& inBounds, const VIndex inCharIndex, VPoint& outCaretPos, GReal& outTextHeight, const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	outCaretPos.SetPos( floor(inBounds.GetLeft()), floor(inBounds.GetTop()));
	outTextHeight = 0.0f;

	if (!fTextDocument)
		return;

	VRect bounds(inBounds);
	bounds.NormalizeToInt(false); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	HRESULT hr = fTextServices->OnTxInPlaceActivate( &fClientRect); //we must activate control in order GetPoint to run successfully
																	//(this activation is windowless)
	if (FAILED(hr))
		return;

	xbox_assert(fTextRange == NULL);
	hr = fTextDocument->Range(inCharIndex, inCharIndex+1, &fTextRange);
	if (FAILED(hr))
		return;

	long type = tomStart | (inCaretLeading ? TA_LEFT : TA_RIGHT) | TA_TOP;
	long x = 0, y = 0;
	hr = fTextRange->GetPoint( type, &x, &y);
	if (SUCCEEDED(hr) && hr != S_FALSE)
	{
		outCaretPos.SetPos( (GReal)x, (GReal)y);

		long type = tomStart | (inCaretLeading ? TA_LEFT : TA_RIGHT) | TA_BOTTOM;
		long yy;
		hr = fTextRange->GetPoint( type, &x, &yy);
		if (SUCCEEDED(hr) && hr != S_FALSE && yy >= y)
			outTextHeight = yy-y;
	}

	hr = fTextServices->OnTxInPlaceDeactivate();

	fTextRange->Release();
	fTextRange = NULL;
}

/** return the text position at the specified coordinates
@remarks
	output text position is 0-based

	input coordinates are in GDI current user space

	return true if input position is inside character bounds
	if method returns false, it returns the closest character index to the input position
*/
bool XWinStyledTextBox::GetCharIndexFromCoord( const VRect& inBounds, const VPoint& inPos, VIndex& outCharIndex)
{
	outCharIndex = 0;

	if (!fTextDocument)
		return false;

	_GetPlainText(); //to ensure fText is updated from RTF text 

	VRect bounds(inBounds);
	bounds.NormalizeToInt(false); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	xbox_assert(fTextRange == NULL);
	long x = floor(inPos.GetX()+0.5f);
	long y = floor(inPos.GetY()+0.5f);

	HRESULT hr = fTextServices->OnTxInPlaceActivate( &fClientRect); //we must activate control in order RangeFromPoint to run successfully
																	//(this activation is windowless)
	if (FAILED(hr))
		return false;

	if (x >= fClientRect.right)
		x = fClientRect.right-1;
	if (x < fClientRect.left)
		x = fClientRect.left;
	if (y >= fClientRect.bottom)
		y = fClientRect.bottom-1;
	if (y < fClientRect.top)
		y = fClientRect.top;
	hr = fTextDocument->RangeFromPoint(x, y, &fTextRange);
	if (FAILED(hr))
		return false;

	long start = 0;
	hr = fTextRange->GetStart( &start);
	if (FAILED(hr))
		return false;
	outCharIndex = start;
	if (outCharIndex < 0)
		outCharIndex = 0;

	if (outCharIndex > 0 && (outCharIndex-1 < fText.GetLength()) && (fText[outCharIndex-1] == 0x0D || fText[outCharIndex-1] == '\f'))
	{
		long type = tomStart | TA_LEFT | TA_TOP;
		long xx = 0, yy = 0;
		hr = fTextRange->GetPoint( type, &xx, &yy);
		if (SUCCEEDED(hr) && yy > y)
			outCharIndex--;
	}

	hr = fTextServices->OnTxInPlaceDeactivate();

	fTextRange->Release();
	fTextRange = NULL;
	return true;
}


/** return styled text box run bounds from the specified range	*/
void XWinStyledTextBox::GetRunBoundsFromRange( const VRect& inBounds, std::vector<VRect>& outRunBounds, sLONG inStart, sLONG inEnd)
{
	if (!fTextDocument)
		return;

	VRect bounds(inBounds);
	bounds.NormalizeToInt(false); //normalize from floating point coord space to integer coord space (to ensure we do not down-crop the layout box)
	bounds.SetPosBy( fOffsetX, 0.0f);
	bounds.SetSizeBy( -fOffsetX, 0.0f); //inflate also width by abs(fOffsetX) because right limit should not be modified (especially for wordwrap)

	RECT rt = bounds;
	fClientRect = rt;
	fExtent.cx = DXtoHimetricX(bounds.GetWidth(), nPixelsPerInchX);
	fExtent.cy = DYtoHimetricY(bounds.GetHeight(), nPixelsPerInchY);

	xbox_assert(fTextRange == NULL);

	//get plain text
	_GetPlainText(); //to ensure fText is updated from RTF text 
	if (fText.GetLength())
	{
		//parse lines
		if (inEnd < 0)
			inEnd = fText.GetLength();
		else if (inEnd > fText.GetLength())
			inEnd = fText.GetLength();
		if (inEnd <= inStart)
			return;

		HRESULT hr = fTextServices->OnTxInPlaceActivate( &fClientRect); //we must activate control in order GetPoint to run successfully
																		//(this activation is windowless)
		if (FAILED(hr))
			return;

		sLONG lineStart = inStart, lineEnd = inStart;
		const UniChar *c = fText.GetCPointer()+inStart;
		for (;lineStart < inEnd;)
		{
			//parse one line
			bool isCR = (*c == 13) || (*c == 10) || (*c == '\f');
			while (lineEnd < inEnd 
				   &&
				   (!isCR))
			{
				lineEnd++;
				c++;
				isCR = (*c == 13) || (*c == 10) || (*c == '\f');
			}
			if (isCR)
			{
				lineEnd++;
				c++;
				isCR = false;
			}
			bool ok = true;
			if (lineEnd > lineStart)
			{
				//get line bounds
				hr = fTextDocument->Range(lineStart, lineEnd, &fTextRange);
				if (FAILED(hr))
					break;

				//get left top origin
				long type = tomStart | TA_LEFT | TA_TOP;
				long x = 0, y = 0;
				hr = fTextRange->GetPoint( type, &x, &y);
				long xx = x, yy = y;

				//in case of line is wrapped, we need to parse line by dichotomy to extract this line real end
				if (SUCCEEDED(hr) && hr != S_FALSE)
				{
					//get left bottom 
					long type = tomStart | TA_LEFT | TA_BOTTOM;
					hr = fTextRange->GetPoint( type, &xx, &yy);
					if (SUCCEEDED(hr) && hr != S_FALSE)
					{
						//scan line by dichotomy until we find the last character with same bottom
						long yyStart = yy;

						sLONG start = lineStart;
						sLONG end = lineEnd;
						sLONG endMax = lineEnd;
						bool found = false;
						while (true)
						{
							long type = tomEnd | TA_LEFT | TA_BOTTOM;
							hr = fTextRange->GetPoint( type, &xx, &yy);
							if (SUCCEEDED(hr) && hr != S_FALSE)
							{
								if (yy == yyStart)
								{
									//this range is not wrapped
									if (end == endMax)
									{
										//we have reached actual end of line
										found = true;
										break;
									}
									else
									{
										//dichotomically scan remaining sub-range
										start = end-1; //because end pos character is not include in this range
										if (start < lineStart)
											start = lineStart;
										end = (start+1+endMax)/2;
										if (end <= start+1)
										{
											found = true;
											break;
										}
										fTextRange->Release();
										fTextRange = NULL;
										hr = fTextDocument->Range(start, end, &fTextRange);
										if (FAILED(hr))
										{
											ok = false;
											break;
										}
									}
								}
								else 
								{
									//this range is wrapped: dichotomically scan it
									endMax = end-1; //end pos if on another line so max valid end should be less than current end
									end = (start+end)/2;
									if (end <= start)
									{
										found = true;
										break; 
									}
									fTextRange->Release();
									fTextRange = NULL;
									hr = fTextDocument->Range(start, end, &fTextRange);
									if (FAILED(hr))
									{
										ok = false;
										break;
									}
								}
							}
							else
							{
								ok = false;
								break;
							}
						}
						if (found)
						{
							c = fText.GetCPointer()+end;
							isCR = (*c == 13) || (*c == 10) || (*c == '\f');
							lineEnd = end;
							ok = true;
						}
						else
							xx = x;
					}
					else 
						ok = false;
				}
				else
					ok = false;

				if (ok)
				{
					if (xx > x && yy > y)
					{
						VRect bounds;
						bounds.SetCoords(x, y, xx-x, yy-y);
						outRunBounds.push_back( bounds);
					}
				}

				if (fTextRange)
				{
					fTextRange->Release();
					fTextRange = NULL;
				}
			}
			if (!ok)
				break;

			/*
			if (lineEnd < inEnd)
			{
				if (isCR)
				{
					c++;
					lineEnd++;
				}
			}
			*/
			lineStart = lineEnd;
			lineEnd = lineStart;
		}

		hr = fTextServices->OnTxInPlaceDeactivate();
	}

}

/** insert text at the specified position */
void XWinStyledTextBox::InsertText( sLONG inPos, const VString& inText)
{
	if (!fTextDocument)
		return;

	if (inText.GetLength() == 0)
		return;

	LONG len = _GetPlainText().GetLength();
	if (inPos < 0)
		inPos = 0;
	if (inPos > len)
		inPos = len;


	LRESULT lResult = 0;
	HRESULT hr = fTextServices->TxSendMessage(EM_SETSEL, inPos, inPos, &lResult); //select
	_SetTextEx( inText, true);
	fIsPlainTextDirty = true;
	hr = fTextServices->TxSendMessage(EM_SETSEL, -1, -1, &lResult); //unselect

	if (inPos > 0)
	{
		//new text inherits style from character previous inserted position

		//get prev char style
		xbox_assert(fTextRange == NULL);
		fTextDocument->Range(inPos-1, inPos, &fTextRange);
		fTextRange->Select();
		LRESULT lResult = 0;

		LONG wParam = SCF_SELECTION;
		CHARFORMAT2W	charFormat;
		memset(&charFormat, 0, sizeof(CHARFORMAT2W));
		charFormat.cbSize = sizeof(CHARFORMAT2W);
		fTextServices->TxSendMessage(EM_GETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lResult );

		//set style for inserted text
		fTextRange->SetStart( inPos);
		fTextRange->SetEnd( inPos+inText.GetLength());
		fTextRange->Select();
		lResult = 0;
		fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lResult );

		if (fTextRange)
		{
			fTextRange->Release();
			fTextRange = NULL;
		}
	}

	/*
	xbox_assert(fTextRange == NULL);

	hr = fTextDocument->Range(inPos, inPos, &fTextRange);
	if (SUCCEEDED(hr) && fTextRange)
	{
		if (SUCCEEDED(fTextRange->CanEdit(NULL)))
		{
			BSTR oleText = ::SysAllocString( inText.GetCPointer());
			if (oleText)
				hr = fTextRange->SetText( oleText); 
			::SysFreeString( oleText);

			if (SUCCEEDED(hr))
				fIsPlainTextDirty = true;
		}
		fTextRange->Release();
		fTextRange = NULL;
	}
	*/
}

/** delete text range */
void XWinStyledTextBox::DeleteText( sLONG rangeStart, sLONG rangeEnd)
{
	if (!fTextDocument)
		return;

	if (rangeEnd <= rangeStart)
		return;
	LONG len = _GetPlainText().GetLength();
	if (rangeStart < 0)
		rangeStart = 0;
	if (rangeStart > len)
		rangeStart = len;
	if (rangeEnd > len)
		rangeEnd = len;

	LRESULT lResult = 0;
	HRESULT hr = fTextServices->TxSendMessage(EM_SETSEL, rangeStart, rangeEnd, &lResult); //select
	_SetTextEx( VString(), true);
	fIsPlainTextDirty = true;
	hr = fTextServices->TxSendMessage(EM_SETSEL, -1, -1, &lResult); //unselect

	/*
	xbox_assert(fTextRange == NULL);

	hr = fTextDocument->Range(rangeStart, rangeEnd, &fTextRange);
	if (SUCCEEDED(hr) && fTextRange)
	{
		if (fTextRange->CanEdit())
		{		
			hr = fTextRange->Delete( tomCharacter, 0, NULL); //delete text range
			if (SUCCEEDED(hr))
				fIsPlainTextDirty = true;
		}
		fTextRange->Release();
		fTextRange = NULL;
	}
	*/
}

void XWinStyledTextBox::DoApplyStyle(VTextStyle* inStyle, VTextStyle * /*inStyleInherit*/)
{
	sLONG rangeStart, rangeEnd;

	inStyle->GetRange(rangeStart, rangeEnd);
	LONG len = _GetPlainText().GetLength();
	LONG wParam;

	if (rangeStart > 0 || rangeEnd < len)
		//switch to RichText mode if style is not applied to all text
		_EnableMultiStyle();

	xbox_assert(fTextRange == NULL);

	//JQ 09/07/2010: fixed range for whole string (rangeEnd=rangeStart+len)
	//JQ 18/04/2011: using SCF_ALL overrides current settings too than merging with current one so always use SCF_SELECTION here
	//if (rangeStart == 0 && rangeEnd == len)
	//{
	//	// For the whole string
	//	wParam = SCF_ALL;
	//}
	//else
	{
		// The selection only
		fTextDocument->Range(rangeStart, rangeEnd, &fTextRange);
		fTextRange->Select();
		wParam = SCF_SELECTION;
	}
	LRESULT lResult = 0;

	if (m_dwPropertyBits & TXTBIT_RICHTEXT)
	{
		//multistyle: merge new style with current style on selection range -> only styles overriden with charFormat.dwMask will be changed

		CHARFORMAT2W	charFormat;
		memset(&charFormat, 0, sizeof(CHARFORMAT2W));
		charFormat.cbSize = sizeof(CHARFORMAT2W);
		
		if (CharFormatFromVTextStyle(&charFormat, inStyle, fHDC))
			fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lResult );
	}
	else
	{
		//monostyle: we update uniform fCharFormat & inform control with TXTBIT_CHARFORMATCHANGE because EM_SETCHARFORMAT works only with RichText edit control

		if (CharFormatFromVTextStyle(&fCharFormat, inStyle, fHDC))
		{
			fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)wParam, (LPARAM)&fCharFormat, &lResult );
			fTextServices->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, 
				TXTBIT_CHARFORMATCHANGE);
		}
	}

	//update eventually paragraph format: here only justification

	//PARAFORMAT2	paraFormat;
	//memset(&paraFormat, 0, sizeof(PARAFORMAT2));
	//paraFormat.cbSize = sizeof(PARAFORMAT2);
	//fTextServices->TxSendMessage(EM_GETPARAFORMAT , 0, (LPARAM)&paraFormat, &lResult );

	//set alignment (can only be applied on all text)
	justificationStyle oldJust = fJust;
	if (inStyle && rangeStart == 0 && rangeEnd >= len)
	{
		bool needUpdate = false;
		if (fJust == JST_Notset)
			fJust = inStyle->GetJustification();
		else if (fJust != inStyle->GetJustification() && inStyle->GetJustification() != JST_Notset)
			fJust = inStyle->GetJustification();

		switch (inStyle->GetJustification())
		{
			case JST_Center:
				{
				fParaFormat.wAlignment = PFA_CENTER;
				needUpdate = true;
				}
				break;
				
			case JST_Right:
				{
				fParaFormat.wAlignment = PFA_RIGHT;
				needUpdate = true;
				}
				break;
				
			case JST_Left:
				{
				fParaFormat.wAlignment = PFA_LEFT;
				needUpdate = true;
				}
				break;

			case JST_Default:
				{
				fParaFormat.wAlignment = PFA_LEFT;
				needUpdate = true;
				}
				break;

			case JST_Justify:
				{
				fParaFormat.wAlignment = PFA_JUSTIFY;
				needUpdate = true;
				}
				break;

			default:
				break;
		}
		if (needUpdate)
		{
			fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lResult );
			fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);
		}
	}

	if (fTextRange)
	{
		fTextRange->Release();
		fTextRange = NULL;
	}

	if (fJust != oldJust)
		_ComputeOffsetX();
}


Boolean XWinStyledTextBox::DoInitialize()
{
	fIsPlainTextDirty = false;

	SetRectEmpty(&fClientRect);
	SetRectEmpty(&fViewInset);

	//FIXME: dynamic switching from standard edit to rich edit is buggy because text size is not always equal if text contains japanese characters for instance
	//		 so disable it for now until a better fix...
	//on default, we use standard edit control: it will automatically switch to RichText edit control if RTF text is set or if a style is applied on a sub-part of the text 
	//m_dwPropertyBits = TXTBIT_READONLY | TXTBIT_HIDESELECTION | TXTBIT_MULTILINE | ((fLayoutMode & TLM_DONT_WRAP) ? 0 : TXTBIT_WORDWRAP)/* | TXTBIT_USECURRENTBKG | TXTBIT_WORDWRAP*/;
	m_dwPropertyBits = TXTBIT_READONLY | TXTBIT_RICHTEXT | TXTBIT_HIDESELECTION | TXTBIT_MULTILINE | ((fLayoutMode & TLM_DONT_WRAP) ? 0 : TXTBIT_WORDWRAP)/* | TXTBIT_USECURRENTBKG | TXTBIT_WORDWRAP*/;

	InitDefaultCharFormat(fHDC);
	InitDefaultParaFormat();
	fTextServices = NULL;
	fTextDocument = NULL;
	fTextRange = NULL;
	fRefCount = 1;

	m_dwMaxLength = INFINITE;
	HRESULT hr = S_OK;
	hr = CreateTextServicesObject();
	if (SUCCEEDED(hr))
	{
		LRESULT lr = 0;
		fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lr);
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);
	}
	return (hr == S_OK);
}

void XWinStyledTextBox::_EnableMultiStyle()
{
	if (!fTextServices)
		return;

	if (m_dwPropertyBits & TXTBIT_RICHTEXT)
		return;

	//in order to switch to RichText control, we need to re-create control from scratch because we cannot transform a standard edit control to a RichText edit control
	//(we could use first RichText edit in plain text mode but we do not want to use RichText control in monostyle only)

	//backup first plain text (uniform style is available in fCharFormat)
	VString text = _GetPlainText();

	//release & create RichText control
	if (fTextDocument != NULL)
		fTextDocument->Release();
	fTextDocument = NULL;
	if (fTextServices != NULL)
		fTextServices->Release();
	fTextServices = NULL;

	m_dwPropertyBits = TXTBIT_RICHTEXT | TXTBIT_READONLY | TXTBIT_HIDESELECTION | TXTBIT_MULTILINE | ((fLayoutMode & TLM_DONT_WRAP) ? 0 : TXTBIT_WORDWRAP)/* | TXTBIT_USECURRENTBKG | TXTBIT_WORDWRAP*/;
	m_dwMaxLength = INFINITE;
	HRESULT hr = S_OK;
	hr = CreateTextServicesObject();
	if (SUCCEEDED(hr))
	{
		LRESULT lr = 0;
		fTextServices->TxSendMessage(EM_SETPARAFORMAT , (WPARAM)0, (LPARAM)&fParaFormat, &lr);
		fTextServices->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE);
	}
	
	//set text
	_SetText( text);

	//set uniform style 
	xbox_assert(fTextRange == NULL);
	fTextDocument->Range(0, text.GetLength(), &fTextRange);
	fTextRange->Select();

	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_SELECTION, (LPARAM)&fCharFormat, &lResult );

	fTextRange->Release();
	fTextRange = NULL;
}

void XWinStyledTextBox::DoRelease()
{
	if (fTextDocument != NULL)
		fTextDocument->Release();
	if (fTextServices != NULL)
		fTextServices->Release();
	fHDC = NULL;
	if (fHDCMetrics)
		::DeleteDC( fHDCMetrics);
}

void XWinStyledTextBox::DoGetSize(GReal &ioWidth, GReal &outHeight)
{
	if (fTextServices)
	{
		SIZEL szExtent;
		sLONG width = ioWidth+fabs(fOffsetX);//(ioWidth * HIMETRIC_PER_INCH) / (GReal)nPixelsPerInchX;
		sLONG height = 1;

		szExtent.cx = 1;
		szExtent.cy = 1;
		fTextServices->TxGetNaturalSize(DVASPECT_CONTENT, 
			fHDC, 
			NULL,
			NULL,
			TXTNS_FITTOCONTENT,
			&szExtent,
			&width,
			&height);

		ioWidth = width-fabs(fOffsetX);
		if (ioWidth < 0.0f)
			ioWidth = 0.0f;
		outHeight = height;
	}
}

void XWinStyledTextBox::_SetFont(VFont *font, const VColor &textColor)
{
	HFONT hfont,oldfont;
	hfont=font ? font->GetFontRef() : NULL;
	CharFormatFromHFONT(&fCharFormat, hfont, textColor, fHDC);
	LRESULT lResult = 0;
	fTextServices->TxSendMessage(EM_SETCHARFORMAT , (WPARAM)SCF_ALL, (LPARAM)&fCharFormat, &lResult );
	fTextServices->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, 
		TXTBIT_CHARFORMATCHANGE);
}

bool XWinStyledTextBox::_VTextStyleFromCharFormat(VTextStyle* ioStyle, CHARFORMAT2W* inCharFormat)
{
	if (!ioStyle)
		return false;

	ioStyle->SetJustification(fJust);

	bool isUniform = true;
	if (inCharFormat->dwMask & CFM_BOLD)
	{
		ioStyle->SetBold(inCharFormat->dwEffects & CFE_BOLD ? 1 : 0);
	}
	else
	{
		ioStyle->SetBold(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_ITALIC)
	{
		ioStyle->SetItalic(inCharFormat->dwEffects & CFE_ITALIC ? 1 : 0);
	}
	else
	{
		ioStyle->SetItalic(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_UNDERLINE)
	{
		ioStyle->SetUnderline(inCharFormat->dwEffects & CFE_UNDERLINE ? 1 : 0);
	}
	else
	{
		ioStyle->SetUnderline(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_STRIKEOUT)
	{
		ioStyle->SetStrikeout(inCharFormat->dwEffects & CFE_STRIKEOUT ? 1 : 0);
	}
	else
	{
		ioStyle->SetStrikeout(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_FACE)
	{
		VString fontname(inCharFormat->szFaceName);
		ioStyle->SetFontName(fontname);
	}
	else
	{
		ioStyle->SetFontName("");
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_SIZE)
	{
		xbox_assert(fFontSizeToDocumentFontSize > 0.0f);
		GReal fontSize = inCharFormat->yHeight*72/LY_PER_INCH; 
		fontSize = fontSize / fFontSizeToDocumentFontSize; //for consistency with  XWinStyledTextBox::CharFormatFromVTextStyle
		ioStyle->SetFontSize( fontSize);
	}
	else
	{
		ioStyle->SetFontSize(-1);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_BACKCOLOR)
	{
		if (inCharFormat->dwEffects & CFE_AUTOBACKCOLOR)
		{
			ioStyle->SetTransparent(true);
		}
		else
		{
			ioStyle->SetTransparent(false);
			XBOX::VColor backcolor;
			backcolor.WIN_FromCOLORREF(inCharFormat->crBackColor);
			ioStyle->SetBackGroundColor(backcolor.GetRGBAColor());
		}
	}
	else
	{
		ioStyle->SetTransparent(true);
		isUniform = false;
	}

	if (inCharFormat->dwMask & CFM_COLOR)
	{
		ioStyle->SetHasForeColor(true);
		XBOX::VColor forecolor;
		forecolor.WIN_FromCOLORREF(inCharFormat->crTextColor);
		ioStyle->SetColor(forecolor.GetRGBAColor());
	}
	else
	{
		ioStyle->SetHasForeColor(false);
		isUniform = false;
	}
	
	return 	isUniform;
}

bool XWinStyledTextBox::_GetStyle(VTextStyle* ioStyle, sLONG inStart, sLONG inEnd)
{
	if (!testAssert(ioStyle))
		return false;

	if (!fTextServices)
		return false;

	if (inStart >= inEnd)
	{
		ioStyle->Reset();
		return false;
	}

	ioStyle->SetRange(inStart, inEnd);
	LONG wParam;

	ITextRange *textRange;
	HRESULT hr = fTextDocument->Range(inStart, inEnd, &textRange);
	if (FAILED(hr))
		return false;

	textRange->Select();
	wParam = SCF_SELECTION;

	CHARFORMAT2W	charFormat;
	memset(&charFormat, 0, sizeof(CHARFORMAT2W));
	charFormat.cbSize = sizeof(CHARFORMAT2W);
	LRESULT lr = 0;
	hr = fTextServices->TxSendMessage(EM_GETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lr);
	if (FAILED(hr))
	{
		textRange->Release();
		return false;
	}
	textRange->Release();

	return _VTextStyleFromCharFormat(ioStyle, &charFormat);
}




/////////////////////////////////////////////////////////////////////////////
// ITextHost functions
HDC XWinStyledTextBox::TxGetDC()
{
	xbox_assert(fHDC);
	return fHDC;
}

INT XWinStyledTextBox::TxReleaseDC(HDC hdc)
{
#if VERSIONDEBUG
	xbox_assert(fHDC == hdc);
#endif
	return 1;
}

BOOL XWinStyledTextBox::TxShowScrollBar(INT fnBar, BOOL fShow)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxEnableScrollBar(INT fuSBFlags, INT fuArrowflags)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetScrollRange(INT fnBar, LONG nMinPos, INT nMaxPos, BOOL fRedraw)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetScrollPos(INT fnBar, INT nPos, BOOL fRedraw)
{
	return FALSE;
}

void XWinStyledTextBox::TxInvalidateRect(LPCRECT prc, BOOL fMode)
{
}

void XWinStyledTextBox::TxViewChange(BOOL fUpdate)
{
}

BOOL XWinStyledTextBox::TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxShowCaret(BOOL fShow)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetCaretPos(INT x, INT y)
{
	return FALSE;
}

BOOL XWinStyledTextBox::TxSetTimer(UINT idTimer, UINT uTimeout)
{
	return FALSE;
}

void XWinStyledTextBox::TxKillTimer(UINT idTimer)
{
}

void XWinStyledTextBox::TxScrollWindowEx(INT dx, INT dy, LPCRECT lprcScroll, LPCRECT lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate, UINT fuScroll)
{
}

void XWinStyledTextBox::TxSetCapture(BOOL fCapture)
{
}

void XWinStyledTextBox::TxSetFocus()
{
}

void XWinStyledTextBox::TxSetCursor(HCURSOR hcur, BOOL fText)
{
}

BOOL XWinStyledTextBox::TxScreenToClient(LPPOINT lppt)
{
	return TRUE; //for windowless, we use only coordinates in current DC user space
}

BOOL XWinStyledTextBox::TxClientToScreen(LPPOINT lppt)
{
	return TRUE; //for windowless, we use only coordinates in current DC user space
}

HRESULT	XWinStyledTextBox::TxActivate(LONG * plOldState)
{
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxDeactivate(LONG lNewState)
{
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetClientRect(LPRECT outClientRect)
{
	*outClientRect = fClientRect;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetViewInset(LPRECT outViewInset)
{
	*outViewInset = fViewInset;
	return S_OK;
}

HRESULT XWinStyledTextBox::TxGetCharFormat(const CHARFORMATW **outCharFormat)
{
	*outCharFormat = (CHARFORMATW*)(&fCharFormat);
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetParaFormat(const PARAFORMAT **outParaFormat)
{
	*outParaFormat = &fParaFormat;
	return S_OK;
}

COLORREF XWinStyledTextBox::TxGetSysColor(int nIndex)
{
	return GetSysColor(nIndex);
}

HRESULT	XWinStyledTextBox::TxGetBackStyle(TXTBACKSTYLE *pstyle)
{
	*pstyle = TXTBACK_TRANSPARENT;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetMaxLength(DWORD *plength)
{
	*plength = m_dwMaxLength;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetScrollBars(DWORD *pdwScrollBar)
{
	*pdwScrollBar = 0;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetPasswordChar(TCHAR *pch)
{
	return S_FALSE;
}

HRESULT	XWinStyledTextBox::TxGetAcceleratorPos(LONG *pcp)
{
	*pcp = -1;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetExtent(LPSIZEL lpExtent)
{
	// Calculate the length & convert to himetric
	*lpExtent = fExtent;

	return S_OK;
}

HRESULT XWinStyledTextBox::OnTxCharFormatChange(const CHARFORMATW * pcf)
{
	memcpy(&fCharFormat, pcf, pcf->cbSize);
	return S_OK;
}

HRESULT	XWinStyledTextBox::OnTxParaFormatChange(const PARAFORMAT * ppf)
{
	memcpy(&fParaFormat, ppf, ppf->cbSize);
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxGetPropertyBits(DWORD dwMask, DWORD *pdwBits)
{
	DWORD dwProperties = 0;

	dwProperties = m_dwPropertyBits | TXTBIT_PARAFORMATCHANGE;

	//if (m_dwPropertyBits & ES_MULTILINE)
	//{
	//	dwProperties |= TXTBIT_MULTILINE;
	//}

	//if (m_dwPropertyBits & ES_READONLY)
	//{
	//	dwProperties |= TXTBIT_READONLY;
	//}


	//if (m_dwPropertyBits & ES_PASSWORD)
	//{
	//	dwProperties |= TXTBIT_USEPASSWORD;
	//}

	//if (!(m_dwPropertyBits & ES_NOHIDESEL))
	//{
	//	dwProperties |= TXTBIT_HIDESELECTION;
	//}
	//dwProperties |= TXTBIT_WORDWRAP;

	*pdwBits = dwProperties & dwMask;
	return S_OK;
}

HRESULT	XWinStyledTextBox::TxNotify(DWORD iNotify, void *pv)
{
	return S_OK;
}

HIMC XWinStyledTextBox::TxImmGetContext()
{
	return NULL;
}

void XWinStyledTextBox::TxImmReleaseContext(HIMC himc)
{
}

HRESULT	XWinStyledTextBox::TxGetSelectionBarWidth(LONG *lSelBarWidth)
{
	*lSelBarWidth = 100;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// custom functions
bool XWinStyledTextBox::CharFormatFromVTextStyle(CHARFORMAT2W* ioCharFormat, VTextStyle* inStyle, HDC inHDC)
{
	//remark: here we must update dwMask only for styles we override (!)

	if (!inStyle)
		return (false);

	bool needUpdate = false;
	sLONG isbold = inStyle->GetBold();
	if(isbold != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_BOLD;
		if (isbold != 0)
		{
			ioCharFormat->dwEffects |= CFE_BOLD;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_BOLD;
		}
	}

	sLONG isitalic = inStyle->GetItalic();
	if(isitalic != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_ITALIC;
		if (isitalic != 0)
		{
			ioCharFormat->dwEffects |= CFE_ITALIC;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_ITALIC;
		}
	}

	sLONG isunderline = inStyle->GetUnderline();
	if(isunderline != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_UNDERLINE;
		if (isunderline != 0)
		{
			ioCharFormat->dwEffects |= CFE_UNDERLINE;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_UNDERLINE;
		}
	}

	sLONG isstrikeout = inStyle->GetStrikeout();
	if(isstrikeout != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_STRIKEOUT;
		if (isstrikeout != 0)
		{
			ioCharFormat->dwEffects |= CFE_STRIKEOUT;
		}
		else
		{
			ioCharFormat->dwEffects &= ~CFE_STRIKEOUT;
		}
	}

	const VString& fontname = inStyle->GetFontName();
	if(fontname.GetLength() > 0)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_FACE;
		//JQ 09/07/2010: fixed font name copy
		//memcpy(ioCharFormat->szFaceName, fontname.GetCPointer(), sizeof(ioCharFormat->szFaceName));
		wcscpy( ioCharFormat->szFaceName, fontname.GetCPointer());
		ioCharFormat->bPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
	}

	GReal fontsize = inStyle->GetFontSize();
	if(fontsize != -1)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_SIZE;
		//JQ 09/07/2010: ??? une taille de fonte est TOUJOURS exprimée en POINTS et pas en PIXELS...
		//				 donc fontsize en inch = fontsize en points / 72 (car 1 pt = 1/72 inch)
		//				 et fontsize en himetrics = fonsize en inch * LY_PER_INCH
		//LONG yPixPerInch = GetDeviceCaps(inHDC, LOGPIXELSY);
		//ioCharFormat->yHeight = fontsize * LY_PER_INCH / yPixPerInch;
		ioCharFormat->yHeight = floor(fontsize * fFontSizeToDocumentFontSize * LY_PER_INCH / 72 + 0.5f);
	}

	bool istransparent = inStyle->GetTransparent();
	if(!istransparent)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_BACKCOLOR;
		ioCharFormat->dwEffects &= ~CFE_AUTOBACKCOLOR;
		XBOX::VColor backcolor;
		backcolor.FromRGBAColor(inStyle->GetBackGroundColor());
		ioCharFormat->crBackColor = backcolor.WIN_ToCOLORREF();
	}

	bool hasForeColor = inStyle->GetHasForeColor();
	if(hasForeColor)
	{
		needUpdate = true;
		ioCharFormat->dwMask |= CFM_COLOR;
		ioCharFormat->dwEffects &= ~CFE_AUTOCOLOR;
		XBOX::VColor forecolor;
		forecolor.FromRGBAColor(inStyle->GetColor());
		ioCharFormat->crTextColor = forecolor.WIN_ToCOLORREF();
	}

	return 	needUpdate;
}


HRESULT XWinStyledTextBox::CharFormatFromHFONT(CHARFORMAT2W* ioCharFormat, HFONT hFont, const VColor &textColor, HDC inHDC)
// Takes an HFONT and fills in a CHARFORMAT2W structure with the corresponding info
{

	// Get LOGFONT for default font
	LOGFONTW lf;
	LONG yPixPerInch;
	if (!hFont)
	{
		//JQ 09/07/2010: try to get current font from hdc first
		//				 otherwise we cannot inherit styles from current dc font !
		hFont = (HFONT) ::GetCurrentObject(inHDC, OBJ_FONT);
		if (!hFont)
			hFont = (HFONT) GetStockObject(SYSTEM_FONT);
	}

	// Get LOGFONT for passed hfont
	if (!GetObjectW(hFont, sizeof(LOGFONTW), &lf))
		return E_FAIL;

	// Set CHARFORMAT structure
	memset(ioCharFormat, 0, sizeof(CHARFORMAT2W));
	ioCharFormat->cbSize = sizeof(CHARFORMAT2W);

	//ioCharFormat->yHeight = -lf.lfHeight * LY_PER_INCH / nPixelsPerInchY;
	ioCharFormat->yHeight = floor(-lf.lfHeight * fFontSizeToDocumentFontSize * LY_PER_INCH / 72 + 0.5f);
	ioCharFormat->yOffset = 0;
	ioCharFormat->crTextColor = textColor.WIN_ToCOLORREF();


	ioCharFormat->dwEffects = CFM_EFFECTS | CFE_AUTOBACKCOLOR;
	if (!(m_dwPropertyBits & TXTBIT_RICHTEXT)) //CFE_AUTOBACKCOLOR is buggy with standard edit control (black color for background...) 
											   //so force COLOR_WINDOW syscolor here to simulate RichText edit control
		ioCharFormat->crBackColor = (COLORREF)::GetSysColor(COLOR_WINDOW);
	ioCharFormat->dwEffects &= ~(/*CFE_PROTECTED |*/ CFE_LINK | CFE_AUTOCOLOR);

	if(lf.lfWeight < FW_BOLD) {
	  ioCharFormat->dwEffects &= ~CFE_BOLD;
	}

	if(!lf.lfItalic) {
	  ioCharFormat->dwEffects &= ~CFE_ITALIC;
	}

	if(!lf.lfUnderline) 
	{
	  ioCharFormat->dwEffects &= ~CFE_UNDERLINE;
	}

	if(!lf.lfStrikeOut) {
	  ioCharFormat->dwEffects &= ~CFE_STRIKEOUT;
	}

	ioCharFormat->dwMask = CFM_ALL | CFM_BACKCOLOR | CFM_STYLE | CFM_COLOR;

	wcscpy(ioCharFormat->szFaceName, lf.lfFaceName);
	ioCharFormat->bCharSet = lf.lfCharSet;
	ioCharFormat->bPitchAndFamily = lf.lfPitchAndFamily;
	return S_OK;
}

HRESULT XWinStyledTextBox::InitDefaultCharFormat(HDC hdc)
{
	VColor col(VColor::sBlackColor);
	return CharFormatFromHFONT(&fCharFormat, NULL, col, hdc);
}

HRESULT XWinStyledTextBox::InitDefaultParaFormat()
{
	memset(&fParaFormat, 0, sizeof(PARAFORMAT2));
	fParaFormat.cbSize = sizeof(PARAFORMAT2);
	fParaFormat.dwMask = PFM_ALL2;
	fParaFormat.wAlignment = PFA_LEFT;
	fJust = JST_Notset;
	//fParaFormat.dxStartIndent = -2*;
	fParaFormat.cTabCount = 0;
	fParaFormat.rgxTabs[0] = lDefaultTab;
	return S_OK;
}

static bool sIsDllLoaded = false;
static PCreateTextServices theCreateTextServices = NULL;

HRESULT XWinStyledTextBox::CreateTextServicesObject()
{
	HRESULT hr;
	IUnknown *spUnk;

	// Create Text Services component
	if(!sIsDllLoaded)
	{
		HMODULE hMod = LoadLibraryW(L"msftedit.dll");
		if(!hMod)
			hMod = LoadLibraryW(L"RichEd20.dll");
		if(hMod)
		{
			theCreateTextServices = ( PCreateTextServices) GetProcAddress(hMod, "CreateTextServices");
			sIsDllLoaded = true;
		}
	}
	hr = (theCreateTextServices(NULL, static_cast<ITextHost*>(this), &spUnk));
	if (hr == S_OK) {
		hr = spUnk->QueryInterface(IID_ITextServices, (void**)&fTextServices);
		hr = spUnk->QueryInterface(IID_ITextDocument, (void**)&fTextDocument);
		spUnk->Release();
	}
	return hr;
}

HRESULT XWinStyledTextBox::QueryInterface( 
	REFIID riid,
	void __RPC_FAR *__RPC_FAR *ppvObject)
{
	HRESULT hr = E_NOINTERFACE;
	*ppvObject = NULL;

	//if (IsEqualIID(riid, IID_IUnknown) 
	//	|| IsEqualIID(riid, IID_ITextHost)) 
	//{
	//	AddRef();
	//	*ppvObject = (ITextHost *) this;
	//	hr = S_OK;
	//}

	return hr;
}

ULONG XWinStyledTextBox::AddRef(void)
{
	return (ULONG)IRefCountable::Retain();
}

ULONG XWinStyledTextBox::Release(void)
{
	return (ULONG)IRefCountable::Release();
}


/** return true if full range is hidden */
bool XWinStyledTextBox::_IsRangeHidden( sLONG inStart, sLONG inEnd, bool &isMixed)
{
	isMixed = false;
	if (!fTextServices)
		return false;

	if (inStart >= inEnd)
		return false;

	ITextRange *textRange;
	HRESULT hr = fTextDocument->Range(inStart, inEnd, &textRange);
	if (FAILED(hr))
		return false;

	textRange->Select();
	LONG wParam = SCF_SELECTION;

	CHARFORMAT2W	charFormat;
	memset(&charFormat, 0, sizeof(CHARFORMAT2W));
	charFormat.cbSize = sizeof(CHARFORMAT2W);
	LRESULT lr = 0;
	hr = fTextServices->TxSendMessage(EM_GETCHARFORMAT , (WPARAM)wParam, (LPARAM)&charFormat, &lr);
	if (FAILED(hr))
	{
		textRange->Release();
		return false;
	}
	textRange->Release();

	bool result = false;
	if (charFormat.dwMask & CFM_HIDDEN)
	{
		result = ((charFormat.dwEffects & CFE_HIDDEN) != 0);
		return result;
	}
	isMixed = true;
	return false;
}

/** return ranges of hidden text */
void XWinStyledTextBox::GetRangesHidden( VectorOfTextRange& outRanges)
{
	sLONG start = 0;
	sLONG end = _GetPlainText().GetLength();

	bool isMixed;
	bool isHidden = _IsRangeHidden( start, end, isMixed);
	if(!isMixed)
	{
		if (isHidden)
			outRanges.push_back( TextRange(start, end));
		return;
	}
	else 
	{
		//context stack (we implement recursivity locally to prevent stack overflow)
		std::vector<TextRange> stackRange;
		std::vector<bool> stackScanNext;

		bool scanNext = false;
		bool doContinue = false;
		do
		{
			doContinue = false;
			sLONG middle = (start+end)/2;
			isHidden = false;
			if (middle > start && !scanNext)
			{
				//scan first half
				isHidden = _IsRangeHidden( start, middle, isMixed);
				if (!isMixed)
				{
					if (isHidden)
						outRanges.push_back(TextRange(start,middle));
				}
				else
				{
					//push context
					stackRange.push_back(TextRange(start,end));
					stackScanNext.push_back(middle < end);

					//iterate recursively on first half
					end = middle;
					doContinue = true;
				}
			}
			scanNext = false;
			if (!doContinue)
			{
				//scan second half
				if (middle < end)
				{
					isHidden = _IsRangeHidden( middle, end, isMixed);
					if (!isMixed)
					{
						if (isHidden)
							outRanges.push_back(TextRange(middle,end));
					}
					else
					{
						//push context
						stackRange.push_back(TextRange(start,end));
						stackScanNext.push_back(false);

						//iterate recursively on second half
						start = middle;
						doContinue = true;
					}
				}

				if (!doContinue)
				{
					while (!stackRange.empty() && !scanNext)
					{
						//restore context
						start = stackRange.back().first;
						end = stackRange.back().second;
						stackRange.pop_back();
						scanNext = stackScanNext.back();
						stackScanNext.pop_back();
					}
				}
			}
		}
		while (!stackRange.empty() || scanNext);
	}
}

/** clear hidden text 
@remarks
	plain text includes hidden text so call this method to remove hidden text
*/
void XWinStyledTextBox::_ClearHidden()
{
	if (!fTextServices)
		return;

	VectorOfTextRange ranges;
	GetRangesHidden( ranges);

	VectorOfTextRange::const_reverse_iterator it = ranges.rbegin();
	for(;it != ranges.rend(); it++)
	{
		ITextRange *textRange = NULL;
		sLONG start = it->first;
		sLONG end = it->second;
		HRESULT hr = fTextDocument->Range((long)(start), (long)(end), &textRange);
		if (FAILED(hr))
			continue;

		textRange->SetText( NULL);
		textRange->Release();
	}
	if (ranges.size() > 0)
		fIsPlainTextDirty = true;
}

