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
#include "VGraphicContext.h"
#if VERSIONWIN
#include "XWinGDIPlusGraphicContext.h"
#if ENABLE_D2D
#include "XWinD2DGraphicContext.h"
#endif
#endif
#if VERSIONMAC
#include "XMacQuartzGraphicContext.h"
#endif

#include "VDocBaseLayout.h"
#include "VDocBorderLayout.h"
#include "VDocImageLayout.h"
#include "VDocParagraphLayout.h"
#include "VDocTableLayout.h"
#include "VDocTextLayout.h"

#undef min
#undef max

#if ENABLE_VDOCTEXTLAYOUT

/**	VDocContainerLayout constructor
@param inNode
	container node:	passed node is used to initialize container layout
						(it should be a container node)
@param inShouldEnableCache
	enable/disable cache (default is true): see ShouldEnableCache
*/
VDocContainerLayout::VDocContainerLayout(const VDocNode *inNode, bool inShouldEnableCache):VDocBaseLayout()
{
	_Init();
	fShouldEnableCache = inShouldEnableCache;

	_InitFromNode( inNode);
}

void VDocContainerLayout::_InitFromNode( const VDocNode *inNode)
{
	//replace this node & all children with passed node

	fChildren.clear();
	if (inNode)
	{
		if (inNode->GetType() == kDOC_NODE_TYPE_DOCUMENT || inNode->GetType() == kDOC_NODE_TYPE_TABLECELL || inNode->GetType() == kDOC_NODE_TYPE_DIV)
		{
			//init properties
			InitPropsFrom( inNode);

			//add children
			VIndex count = inNode->GetChildCount();
			for (int i = 0; i < count; i++)
			{
				const VDocNode *node = inNode->GetChild( i);
				if (node->GetType() == kDOC_NODE_TYPE_PARAGRAPH) 
				{
					const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(node);
					if (testAssert(para))
					{
						VDocParagraphLayout *paraLayout = NULL;
						paraLayout = new VDocParagraphLayout( para, fShouldEnableCache);

						AddChild( static_cast<IDocBaseLayout *>(static_cast<VDocBaseLayout *>(paraLayout)));
						paraLayout->Release();
					}
				}
				else if (node->GetType() == kDOC_NODE_TYPE_DIV)
				{
					//TODO
				}
			}
			if (!count)
			{
				//at least one empty paragraph
				VDocParagraphLayout *paraLayout = new VDocParagraphLayout( fShouldEnableCache);

				AddChild( static_cast<IDocBaseLayout *>(static_cast<VDocBaseLayout *>(paraLayout)));

				paraLayout->Release();
			}
		}
		else
			xbox_assert(false);
	}
	else
	{
		//at least one empty paragraph
		VDocParagraphLayout *paraLayout = new VDocParagraphLayout( fShouldEnableCache);

		AddChild( static_cast<IDocBaseLayout *>(static_cast<VDocBaseLayout *>(paraLayout)));
		paraLayout->Release();
	}

	if (!fPlaceHolder.IsEmpty())
	{
		//propagate placeholder to children
		VString placeholder = fPlaceHolder;
		fPlaceHolder.SetEmpty();
		SetPlaceHolder(placeholder);
	}
}

void VDocContainerLayout::_ReplaceFromNode( const VDocNode *inNode, sLONG inStart, sLONG inEnd,
											bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, 
											void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	if (inNode->GetChildCount() == 0)
		return;
	if (!testAssert(inNode->GetType() == kDOC_NODE_TYPE_DOCUMENT || inNode->GetType() == kDOC_NODE_TYPE_TABLECELL || inNode->GetType() == kDOC_NODE_TYPE_DIV))
		return; //fragment needs to be a container

	_NormalizeRange( inStart, inEnd);

	if (inAutoAdjustRangeWithSpanRef || !inSkipCheckRange)
		AdjustRange( inStart, inEnd);

	VTreeTextStyle *stylesDefault = NULL;
	if (inNode->GetTextLength() > 0)
	{
		if (inInheritUniformStyle && inStart < inEnd && GetUniChar(inStart) != 13)
		{
			VTreeTextStyle* styles = RetainStyles( inStart, inStart+1);
			VTextStyle *style = styles->CreateUniformStyleOnRange( inStart, inStart+1);
			ReleaseRefCountable(&styles);
			stylesDefault = style ? new VTreeTextStyle( style) : RetainDefaultStyle();
		}
		else
		{
			VTreeTextStyle* styles = RetainStyles( inStart, inStart);
			VTextStyle *style = styles->CreateUniformStyleOnRange( inStart, inStart);
			ReleaseRefCountable(&styles);
			stylesDefault = style ? new VTreeTextStyle( style) : RetainDefaultStyle();
		}
		xbox_assert(stylesDefault);
	}

	//first remove plain text and/or paragraphs fully included on replacement range
	if (inStart < inEnd)
	{
		uLONG length = GetTextLength();
		SetText(CVSTR(""), NULL, inStart, inEnd, false, true, inInheritUniformStyle, NULL, 
				inUserData, inBeforeReplace, inAfterReplace);
		if (length == GetTextLength())
		{
			//replacement has failed -> abandon
			xbox_assert(false);
			ReleaseRefCountable(&stylesDefault);
			return;
		}
	}
	if (inNode->GetTextLength() == 0)
	{
		ReleaseRefCountable(&stylesDefault);
		return;
	}

	//now replacement range is [inStart, inStart]

	VDocLayoutChildrenIterator it( static_cast<const VDocBaseLayout *>(this));
	VDocBaseLayout *layout = it.First(inStart);
	if (!testAssert(layout))
	{
		ReleaseRefCountable(&stylesDefault);
		return;
	}

	if (layout->GetDocType() == kDOC_NODE_TYPE_DIV)
	{
		//defer replacement to child container as replacement is constrained to child container

		VDocContainerLayout *layoutCont = dynamic_cast<VDocContainerLayout *>(layout);
		if (inBeforeReplace && inAfterReplace)
		{
			//install local delegate in order to map/unmap text indexs from/to local
			fReplaceTextUserData = inUserData;
			fReplaceTextBefore = inBeforeReplace;
			fReplaceTextAfter = inAfterReplace;
			fReplaceTextCurLayout = layout;

			layoutCont->_ReplaceFromNode(	inNode, inStart-layout->GetTextStart(), inStart-layout->GetTextStart(), 
											false, true, inInheritUniformStyle, 
											this, _BeforeReplace, _AfterReplace);
		}
		else
			layoutCont->_ReplaceFromNode(	inNode, inStart-layout->GetTextStart(), inStart-layout->GetTextStart(), 
											false, true, inInheritUniformStyle, 
											NULL, NULL, NULL);
		return;
	}
	xbox_assert(layout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH);

	VString textBefore;
	VTreeTextStyle *stylesBefore = NULL;
	VString textAfter;
	VTreeTextStyle *stylesAfter = NULL;

	ITextLayout *textLayout = layout->QueryTextLayoutInterface();
	if (!testAssert(textLayout))
	{
		ReleaseRefCountable(&stylesDefault);
		return;
	}

	VDocParagraph *paraDefault = new VDocParagraph();
	paraDefault->SetStyles( stylesDefault, false);
	ReleaseRefCountable(&stylesDefault);

	sLONG insertAt = layout->GetChildIndex();
	bool isLast = layout->IsLastChild(true); //is it last paragraph of document ?

	if (inStart < layout->GetTextStart()+layout->GetTextLength())
	{
		if (inStart > layout->GetTextStart())
		{
			//backup text & styles before & after insertion position & remove layout
			textLayout->GetText( textBefore, 0, inStart-layout->GetTextStart());
			stylesBefore = textLayout->RetainStyles(0, inStart-layout->GetTextStart());
		}

		textLayout->GetText( textAfter, inStart-layout->GetTextStart());
		stylesAfter = textLayout->RetainStyles(inStart-layout->GetTextStart());

		VDocBaseLayout::RemoveChild( layout->GetChildIndex());
	}
	else 
	{
		xbox_assert(layout->GetChildIndex() == fChildren.size()-1);

		if (testAssert(isLast))
		{
			//last document paragraph has not a paragraph terminating CR (it it has a CR, it is a embedded CR & not a paragraph separator)
			//so concat with it & remove it

			//backup text & styles
			textLayout->GetText( textBefore);
			stylesBefore = textLayout->RetainStyles();

			VDocBaseLayout::RemoveChild( layout->GetChildIndex());
		}
	}
	
	sLONG deltaLength = 0;
	VIndex count = inNode->GetChildCount();
	for (int i = 0; i < count; i++)
	{
		const VDocNode *node = inNode->GetChild( i);
		if (node->GetType() == kDOC_NODE_TYPE_PARAGRAPH) 
		{
			const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(node);
			if (testAssert(para))
			{
				VDocParagraphLayout *paraLayout = new VDocParagraphLayout( fShouldEnableCache);
				paraLayout->InitPropsFrom( static_cast<VDocNode *>(paraDefault));
				paraLayout->InitPropsFrom( static_cast<const VDocNode *>(para), true);
				paraLayout->SetText( para->GetText(), para->GetStyles(), true);

				if (i == 0 && !textBefore.IsEmpty())
					//first node to insert
					paraLayout->SetText(textBefore, stylesBefore, 0, 0, false, true);
				
				if (i == count-1) //last node to insert
				{
					if (insertAt < fChildren.size() || !isLast) //insert before another paragraph
					{
						if (textAfter.IsEmpty() || isLast)
						{
							textAfter.AppendUniChar(13); //append paragraph terminating CR
							deltaLength++;
						}
					}
				
					if (!textAfter.IsEmpty())
						paraLayout->SetText( textAfter, stylesAfter, paraLayout->GetTextLength(), paraLayout->GetTextLength(), false, true);
				}

				InsertChild( static_cast<IDocBaseLayout *>(static_cast<VDocBaseLayout *>(paraLayout)), insertAt++);
				paraLayout->Release();
			}
		}
		else if (node->GetType() == kDOC_NODE_TYPE_DIV)
		{
			//TODO
		}
	}

	if (!fPlaceHolder.IsEmpty())
	{
		//propagate placeholder to children
		VString placeholder = fPlaceHolder;
		fPlaceHolder.SetEmpty();
		SetPlaceHolder(placeholder);
	}

	ReleaseRefCountable(&stylesBefore);
	ReleaseRefCountable(&stylesAfter);
	ReleaseRefCountable(&paraDefault);

	if (inBeforeReplace && inAfterReplace)
	{
		VString text;
		inBeforeReplace( inUserData, inStart, inStart, text);
		inAfterReplace( inUserData, inStart, inStart+inNode->GetTextLength()+deltaLength);
	}
}

VDocContainerLayout::~VDocContainerLayout()
{
	xbox_assert(fGC == NULL && fBackupParentGC == NULL);
}


void VDocContainerLayout::_Init()
{
	fShouldEnableCache				= false;
	fShouldDrawOnLayer				= false;
	fShowRefs						= kSRCTT_Value;
	fHighlightRefs					= false;
	
	fIsPassword						= false;

	fShouldPaintSelection			= false;
	fSelActiveColor					= VTEXTLAYOUT_SELECTION_ACTIVE_COLOR;
	fSelInactiveColor				= VTEXTLAYOUT_SELECTION_INACTIVE_COLOR;
	fSelIsActive					= true;
	fSelStart						= fSelEnd = -1;

	fReplaceTextCurLayout			= NULL;
	fIsDirty						= true;

	fGC								= NULL;
	fGCUseCount						= 0;
	fGCType							= eDefaultGraphicContext; //means same as undefined here
	fBackupParentGC					= NULL;
	fIsPrinting						= false;
	fLayoutKerning					= 0.0f;
	fLayoutTextRenderingMode		= TRM_NORMAL;

	fRenderMaxWidth					= 0.0f;
	fRenderMaxHeight				= 0.0f;
	fRenderMargin.left				= fRenderMargin.right = fRenderMargin.top = fRenderMargin.bottom = 0;
	fMargin.left					= fMargin.right = fMargin.top = fMargin.bottom = 0.0f;
}	


void VDocContainerLayout::_PropagatePropsToLayout( VDocBaseLayout *inLayout, bool inSkipPlaceholder)
{
	ITextLayout *textLayout = inLayout->QueryTextLayoutInterface();
	if (!textLayout)
		return;

	textLayout->SetDPI(fDPI);	//children layout inherit dpi from parent computing dpi (for children layout computing dpi = rendering dpi)
								//(because scaling from computing dpi to rendering dpi must be applied only once by the top-level container)
	textLayout->ShouldEnableCache( fShouldEnableCache);
	textLayout->ShouldDrawOnLayer( fShouldDrawOnLayer);
	textLayout->ShowRefs( fShowRefs);
	textLayout->HighlightRefs( fHighlightRefs); 
	textLayout->ShouldPaintSelection( fShouldPaintSelection, &fSelActiveColor, &fSelInactiveColor);
	if (!inSkipPlaceholder)
		textLayout->SetPlaceHolder( fPlaceHolder);
	textLayout->EnablePassword(fIsPassword);
	textLayout->SetLayerViewRect(fLayerViewRect);
}

/** set dirty */
void VDocContainerLayout::SetDirty(bool inApplyToChildren, bool inApplyToParent) 
{ 
	fIsDirty = true; 
	if (fParent && inApplyToParent) 
	{ 
		ITextLayout *textLayout = fParent->QueryTextLayoutInterface(); 
		if (textLayout) 
			textLayout->SetDirty(); 
	}
	if (!inApplyToChildren)
		return;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetDirty( inApplyToChildren, false);

		layout = it.Next();
	}
}

void VDocContainerLayout::SetPlaceHolder( const VString& inPlaceHolder)	
{ 
	if (fPlaceHolder.EqualToStringRaw( inPlaceHolder))
		return;

	fPlaceHolder = inPlaceHolder; 
	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	if (layout) //placeholder is applied only on the first layout (because it is applied only if text is empty and only the last paragraph can be empty)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetPlaceHolder( inPlaceHolder);
	}
}

/** enable/disable password mode */
void VDocContainerLayout::EnablePassword(bool inEnable) 
{ 
	if (fIsPassword == inEnable)
		return;

	fIsPassword = inEnable; 
	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->EnablePassword( inEnable);

		layout = it.Next();
	}
}

void VDocContainerLayout::SetLayerViewRect( const VRect& inRectRelativeToWnd) 
{
	if (fLayerViewRect == inRectRelativeToWnd)
		return;
	fLayerViewRect = inRectRelativeToWnd;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetLayerViewRect( inRectRelativeToWnd);

		layout = it.Next();
	}
}

void VDocContainerLayout::_ShouldEnableCache(bool inEnable)
{
	fShouldEnableCache = inEnable;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->ShouldEnableCache( inEnable);

		layout = it.Next();
	}
}

void VDocContainerLayout::_ShouldDrawOnLayer(bool inEnable)
{
	fShouldDrawOnLayer = inEnable;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->ShouldDrawOnLayer( inEnable);

		layout = it.Next();
	}
}

void VDocContainerLayout::_BeforeReplace(void *inUserData, sLONG& ioStart, sLONG& ioEnd, VString &ioText)
{
	//we need to map/unmap text indexs according to current child layout if any
	
	VDocContainerLayout *layout = (VDocContainerLayout *)inUserData;
	xbox_assert(layout->fReplaceTextCurLayout);

	//map to local
	ioStart += layout->fReplaceTextCurLayout->GetTextStart();
	ioEnd += layout->fReplaceTextCurLayout->GetTextStart();
	//call parent delegate
	layout->fReplaceTextBefore( layout->fReplaceTextUserData, ioStart, ioEnd, ioText);
	//map to child
	ioStart -= layout->fReplaceTextCurLayout->GetTextStart();
	ioEnd -= layout->fReplaceTextCurLayout->GetTextStart();
}

void VDocContainerLayout::_AfterReplace(void *inUserData, sLONG inStartReplaced, sLONG inEndReplaced)
{
	//we need to map/unmap text indexs according to current child layout if any
	
	VDocContainerLayout *layout = (VDocContainerLayout *)inUserData;
	xbox_assert(layout->fReplaceTextCurLayout);

	//map to local
	inStartReplaced += layout->fReplaceTextCurLayout->GetTextStart();
	inEndReplaced += layout->fReplaceTextCurLayout->GetTextStart();
	//call parent delegate
	layout->fReplaceTextAfter( layout->fReplaceTextUserData, inStartReplaced, inEndReplaced);
}

void VDocContainerLayout::ShowRefs(SpanRefCombinedTextTypeMask inShowRefs,  void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	if (fShowRefs == inShowRefs)
		return;
	fShowRefs = inShowRefs;

	SetDirty();

	VDocLayoutChildrenReverseIterator it(static_cast<VDocBaseLayout*>(this)); //reverse iteration (in order to not screw up text indexs because of replacement)
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && textLayout->GetTextLength())
		{
			if (inBeforeReplace && inAfterReplace)
			{
				//install local delegate in order to map/unmap text indexs from/to local
				fReplaceTextUserData = inUserData;
				fReplaceTextBefore = inBeforeReplace;
				fReplaceTextAfter = inAfterReplace;
				fReplaceTextCurLayout = layout;

				textLayout->ShowRefs( inShowRefs, this, _BeforeReplace, _AfterReplace, inLC);
			}
			else
				textLayout->ShowRefs( inShowRefs, NULL, NULL, NULL, inLC);
		}

		layout = it.Next();
	}
}

void VDocContainerLayout::HighlightRefs(bool inHighlight, bool inUpdateAlways)
{
	if (!inUpdateAlways && fHighlightRefs == inHighlight)
		return;

	fHighlightRefs = inHighlight;
	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->HighlightRefs( inHighlight, inUpdateAlways);

		layout = it.Next();
	}
}

void VDocContainerLayout::ShouldEnableCache( bool inShouldEnableCache)
{
	if (fShouldEnableCache == inShouldEnableCache)
		return;
	_ShouldEnableCache( inShouldEnableCache);

	SetDirty();
}


/** allow/disallow offscreen text rendering (default is false)
@remarks
	text is rendered offscreen only if ShouldEnableCache() == true && ShouldDrawOnLayer() == true
	you should call ShouldDrawOnLayer(true) only if you want to cache text rendering
@see
	ShouldEnableCache
*/
void VDocContainerLayout::ShouldDrawOnLayer( bool inShouldDrawOnLayer)
{
	return; //FIXME: for now disable layers (text does not draw well on transparent layers so we should find a way to use opaque layers)

	if (fShouldDrawOnLayer == inShouldDrawOnLayer)
		return;
	_ShouldDrawOnLayer( inShouldDrawOnLayer);
	SetDirty();
}

void VDocContainerLayout::SetLayerDirty(bool inApplyToChildren) 
{ 
	if (!inApplyToChildren)
		return;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetLayerDirty( inApplyToChildren);

		layout = it.Next();
	}
}

/** paint native selection or custom selection */
void VDocContainerLayout::ShouldPaintSelection(bool inPaintSelection, const VColor* inActiveColor, const VColor* inInactiveColor)
{
	if (fShouldPaintSelection == inPaintSelection 
		&&
		(!inPaintSelection
		||
		((!inActiveColor || fSelActiveColor == *inActiveColor)
		 &&
		 (!inInactiveColor || fSelInactiveColor == *inInactiveColor))))
		return;

	fShouldPaintSelection = inPaintSelection;
	if (inActiveColor)
		fSelActiveColor = *inActiveColor;
	if (inInactiveColor)
		fSelInactiveColor = *inInactiveColor;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			textLayout->ShouldPaintSelection( inPaintSelection, inActiveColor, inInactiveColor);
			if (inPaintSelection)
			{
				if (fSelStart < textLayout->GetTextStart()+textLayout->GetTextLength() && fSelEnd > textLayout->GetTextStart())
					textLayout->ChangeSelection( fSelStart-textLayout->GetTextStart(), fSelEnd-textLayout->GetTextStart(), fSelIsActive);
				else
					textLayout->ChangeSelection( 0,0, fSelIsActive);
			}
			else
				textLayout->ChangeSelection( 0,0, fSelIsActive);
		}
		layout = it.Next();
	}
}


void VDocContainerLayout::ChangeSelection( sLONG inStart, sLONG inEnd, bool inActive)
{
	_NormalizeRange( inStart, inEnd);

	if (fSelIsActive == inActive 
		&&
		(fSelStart == inStart && (fSelEnd == inEnd || (inEnd == -1 && fSelEnd == GetTextLength()))))
		return;

	sLONG start = fSelStart;
	sLONG end = fSelEnd;

	fSelIsActive = inActive;
	fSelStart = inStart;
	fSelEnd = inEnd;
	if (inEnd == -1)
		fSelEnd = GetTextLength();
	if (!fShouldPaintSelection)
		return;

	if (fSelStart < start)
		start = fSelStart;
	if (fSelEnd > end)
		end = fSelEnd;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(start, end); //iterate on union of selection range before and after modification
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			if (fSelStart < textLayout->GetTextStart()+textLayout->GetTextLength() && fSelEnd > textLayout->GetTextStart())
				textLayout->ChangeSelection( fSelStart-textLayout->GetTextStart(), fSelEnd-textLayout->GetTextStart(), fSelIsActive);
			else
				textLayout->ChangeSelection( 0,0, fSelIsActive);
		}
		layout = it.Next();
	}
}


/** set layout DPI (default is 4D form DPI = 72) */
void VDocContainerLayout::SetDPI( const GReal inDPI)
{
	if (fDPI == inDPI && fDPI == fRenderDPI)
		return;
	VDocBaseLayout::SetDPI( inDPI);

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetDPI( inDPI);

		layout = it.Next();
	}

	SetDirty();
}

/** set inside margins	(in twips) - CSS padding */
void VDocContainerLayout::SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin)
{
	if (fMarginInTWIPS == inPadding)
		return;
	VDocBaseLayout::SetPadding( inPadding, inCalcAllMargin);
	SetDirty();
}

/** set outside margins	(in twips) - CSS margin */
void VDocContainerLayout::SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin)
{
	if (fMarginOutTWIPS == inMargins)
		return;
	VDocBaseLayout::SetMargins( inMargins, inCalcAllMargin);
	SetDirty();
}


/** set text align - CSS text-align (-1 for not defined) */
void VDocContainerLayout::SetTextAlign( const AlignStyle inHAlign) 
{ 
	if (fTextAlign == inHAlign)
		return;
	VDocBaseLayout::SetTextAlign( inHAlign);

	//text-align scope is paragraph-only (it is not meaningful for other layout) so always propagate to children
	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetTextAlign( inHAlign);

		layout = it.Next();
	}
	SetDirty();
}

/** set vertical align - CSS vertical-align (-1 for not defined) */
void VDocContainerLayout::SetVerticalAlign( const AlignStyle inVAlign) 
{ 
	if (fVerticalAlign == inVAlign)
		return;
	VDocBaseLayout::SetVerticalAlign( inVAlign);
	SetDirty();
}


/** set width (in twips) - CSS width (0 for auto) */
void VDocContainerLayout::SetWidth(const uLONG inWidth)
{
	if (fWidthTWIPS == inWidth)
		return;
	VDocBaseLayout::SetWidth( inWidth ? (inWidth > GetMinWidth() ? inWidth : GetMinWidth()) : 0);
	fRenderMaxWidth = inWidth ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetWidth(true))*fRenderDPI/72) : 0;
	SetDirty(true); //apply to children as paragraph width might be determined from container content width if paragraph width is automatic
}

/** set min width (in twips) - CSS min-width (0 for auto) */
void VDocContainerLayout::SetMinWidth(const uLONG inWidth)
{
	if (fMinWidthTWIPS == inWidth)
		return;
	VDocBaseLayout::SetMinWidth( inWidth);
	if (fWidthTWIPS && fWidthTWIPS < fMinWidthTWIPS) 
		SetWidth( inWidth);
	SetDirty(true); //apply to children as paragraph width might be determined from container content width if paragraph width is automatic
}


/** set height (in twips) - CSS height (0 for auto) */
void VDocContainerLayout::SetHeight(const uLONG inHeight)
{
	if (fHeightTWIPS == inHeight)
		return;
	VDocBaseLayout::SetHeight( inHeight ? (inHeight > GetMinHeight() ? inHeight : GetMinHeight()) : 0);
	fRenderMaxHeight = inHeight ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetHeight(true))*fRenderDPI/72) : 0;
	SetDirty();
}

/** set min height (in twips) - CSS min-height (0 for auto) */
void VDocContainerLayout::SetMinHeight(const uLONG inHeight)
{
	if (fMinHeightTWIPS == inHeight)
		return;
	VDocBaseLayout::SetMinHeight( inHeight);
	if (fHeightTWIPS && fHeightTWIPS < fMinHeightTWIPS) 
		SetHeight( inHeight);
	SetDirty();
}

/** set background color - CSS background-color (transparent for not defined) */
void VDocContainerLayout::SetBackgroundColor( const VColor& inColor)
{
	if (fBackgroundColor == inColor)
		return;
	VDocBaseLayout::SetBackgroundColor( inColor);
}


/** set background clipping - CSS background-clip */
void VDocContainerLayout::SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip)
{
	if (fBackgroundClip == inBackgroundClip)
		return;
	VDocBaseLayout::SetBackgroundClip( inBackgroundClip);
}

/** set background origin - CSS background-origin */
void VDocContainerLayout::SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin)
{
	if (fBackgroundOrigin == inBackgroundOrigin)
		return;
	VDocBaseLayout::SetBackgroundOrigin( inBackgroundOrigin);
	SetDirty();
}

/** set borders 
@remarks
	if inLeft is NULL, left border is same as right border 
	if inBottom is NULL, bottom border is same as top border
	if inRight is NULL, right border is same as top border
	if inTop is NULL, top border is default border (solid, medium width & black color)
*/
void VDocContainerLayout::SetBorders( const sDocBorder *inTop, const sDocBorder *inRight, const sDocBorder *inBottom, const sDocBorder *inLeft)
{
	VDocBaseLayout::SetBorders( inTop, inRight, inBottom, inLeft);
	SetDirty();
}


/** clear borders */
void VDocContainerLayout::ClearBorders()
{
	if (!fBorderLayout)
		return;
	VDocBaseLayout::ClearBorders();
	SetDirty();
}

/** set background image 
@remarks
	see VDocBackgroundImageLayout constructor for parameters description */
void VDocContainerLayout::SetBackgroundImage(	const VString& inURL,	
												const eStyleJust inHAlign, const eStyleJust inVAlign, 
												const eDocPropBackgroundRepeat inRepeat,
												const GReal inWidth, const GReal inHeight,
												const bool inWidthIsPercentage,  const bool inHeightIsPercentage)
{
	if (inURL.IsEmpty())
	{
		ClearBackgroundImage();
		return;
	}
	VDocBaseLayout::SetBackgroundImage( inURL, inHAlign, inVAlign, inRepeat, inWidth, inHeight, inWidthIsPercentage, inHeightIsPercentage);
	SetDirty();
}

/** clear background image */
void VDocContainerLayout::ClearBackgroundImage()
{
	if (!fBackgroundImageLayout)
		return;
	VDocBaseLayout::ClearBackgroundImage();
	SetDirty();
}


/** set node properties from the current properties */
void VDocContainerLayout::SetPropsTo( VDocNode *outNode)
{
	VDocBaseLayout::SetPropsTo( outNode);
}

void VDocContainerLayout::InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInherited)
{
	VDocBaseLayout::InitPropsFrom(inNode, inIgnoreIfInherited);
	SetDirty();
}



/** update text layout metrics */ 
void VDocContainerLayout::UpdateLayout(VGraphicContext *inGC)
{
	xbox_assert(inGC);

	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}

	BeginUsingContext( inGC, true); //will call _UpdateLayout & _UpdateLayoutInternal
	EndUsingContext();
}


/** update text layout metrics */ 
void VDocContainerLayout::_UpdateLayout()
{
	if (!fIsDirty)
		return;

	xbox_assert(fGC);

	//backup cur kerning
	fLayoutKerning = fGC->GetCharActualKerning();

	//backup cur text rendering mode
	fLayoutTextRenderingMode = fGC->GetTextRenderingMode();

	_AdjustComputingDPI(fGC);
	
	fIsDirty = false;

	sLONG widthTWIPS	= _GetWidthTWIPS();
	sLONG heightTWIPS	= _GetHeightTWIPS();

	GReal maxWidth		= widthTWIPS ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(widthTWIPS)*fDPI/72) : 0; 
	GReal maxHeight		= heightTWIPS ? RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(heightTWIPS)*fDPI/72) : 0; 

	fMargin.left		= RoundMetrics(fGC,fAllMargin.left); 
	fMargin.right		= RoundMetrics(fGC,fAllMargin.right); 
	fMargin.top			= RoundMetrics(fGC,fAllMargin.top); 
	fMargin.bottom		= RoundMetrics(fGC,fAllMargin.bottom); 

	//update children layout

	fChildOrigin.reserve( fChildren.size());
	fChildOrigin.clear();

	GReal typoWidth = 0.0f;
	GReal typoHeight = 0.0f;
	GReal layoutWidth = 0.0f;
	GReal layoutHeight = 0.0f;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			if (textLayout->IsDirty()) 
				textLayout->UpdateLayout( fGC);
			
			fChildOrigin.push_back(VPoint(fMargin.left,fMargin.top+typoHeight)); //it is used to optimize hit testing

			typoWidth = std::max(typoWidth, textLayout->GetTypoBounds().GetWidth()); //formatted text width
			layoutWidth = std::max(layoutWidth, textLayout->GetLayoutBounds().GetWidth()); //layout width
			typoHeight += textLayout->GetLayoutBounds().GetHeight();
		}
		layout = it.Next();
	}
	layoutHeight = typoHeight; //layout height is equal to typographic height on default

	if (maxWidth) 
		layoutWidth = maxWidth;
	if (maxHeight)
		layoutHeight = maxHeight;

	//determine local metrics

	_FinalizeTextLocalMetrics(	fGC, typoWidth, typoHeight, layoutWidth, layoutHeight, 
								fMargin, fRenderMaxWidth, fRenderMaxHeight, 
								fTypoBounds, fRenderTypoBounds, fRenderLayoutBounds, fRenderMargin);

	_SetLayoutBounds( fGC, fLayoutBounds); //here we pass bounds relative to fDPI
}

					
void VDocContainerLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw)
{
	//apply vertical align offset
	GReal y = 0;
	if (fLayoutBounds.GetHeight() != fTypoBounds.GetHeight())
		switch (GetVerticalAlign())
		{
		case AL_BOTTOM:
			y = fLayoutBounds.GetHeight()-fTypoBounds.GetHeight(); 
			break;
		case AL_CENTER:
			y = fLayoutBounds.GetHeight()*0.5-fTypoBounds.GetHeight()*0.5;
			break;
		default:
			break;
		}
	VDocBaseLayout::_BeginLocalTransform( inGC, outCTM, inTopLeft, inDraw, VPoint(0,y)); 
}



VDocParagraphLayout* VDocContainerLayout::_GetParagraphFromCharIndex(const sLONG inCharIndex, sLONG *outCharStartIndex) const
{
	IDocBaseLayout *layout = GetFirstChild( inCharIndex);
	if (layout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		VDocParagraphLayout *paraLayout = dynamic_cast<VDocParagraphLayout *>(layout);
		xbox_assert(paraLayout);
		if (outCharStartIndex)
			*outCharStartIndex += paraLayout->GetTextStart();
		return paraLayout;
	}
	//search deeply
	VDocContainerLayout *contLayout = dynamic_cast<VDocContainerLayout *>(layout);
	if (testAssert(contLayout))
	{
		if (outCharStartIndex)
			*outCharStartIndex += layout->GetTextStart();
		return contLayout->_GetParagraphFromCharIndex( inCharIndex-layout->GetTextStart(), outCharStartIndex);
	}
	return NULL;
}
  

void VDocContainerLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft, const GReal inOpacity)
{
	xbox_assert(inGC);
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}

	BeginUsingContext(inGC); 

	if (!testAssert(!fIsDirty))
	{
		EndUsingContext();
		return;
	}
	
	if (!testAssert(fChildOrigin.size() == fChildren.size()))
		return;

	{
	StEnableShapeCrispEdges crispEdges( inGC);
	
	VAffineTransform ctm;
	_BeginLocalTransform( inGC, ctm, inTopLeft, true); //here (0,0) is sticked to text box origin+vertical align offset & coordinate space pixel is mapped to fDPI (72 on default so on default 1px = 1pt)
	//disable shape printer scaling as for printing, we draw at printer dpi yet - design metrics are computed at printer dpi (mandatory only for GDI)
	bool shouldScaleShapesWithPrinterScale = inGC->ShouldScaleShapesWithPrinterScale(false);

	//apply transparency if appropriate
	_BeginLayerOpacity( inGC, inOpacity);

	//draw common layout decoration: borders, etc...
	//(as layout origin is sticked yet to 0,0 because of current transform, we do not need to translate)
	VDocBaseLayout::Draw( inGC, VPoint());

	VRect clipBounds;
	inGC->GetClipBoundingBox( clipBounds);
	clipBounds.NormalizeToInt();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	VectorOfPoint::const_iterator itOrigin = fChildOrigin.begin();
	while (layout)
	{
		//draw only children which are visible
		if (itOrigin->GetY() >= clipBounds.GetBottom())
			break;
		if (itOrigin->GetY()+layout->GetLayoutBounds().GetHeight() > clipBounds.GetTop())
		{
			ITextLayout *textLayout = layout->QueryTextLayoutInterface();
			if (testAssert(textLayout))
				//only child elements with text layout interface (paragraph or container) are paintable here
				//(other elements are embedded in paragraphs)
				textLayout->Draw( inGC, *itOrigin);
		}

		layout = it.Next();
		itOrigin++;
	}

	_EndLayerOpacity( inGC, inOpacity);

	inGC->ShouldScaleShapesWithPrinterScale(shouldScaleShapesWithPrinterScale);

	_EndLocalTransform( inGC, ctm, true);
	}

	EndUsingContext();
}



void VDocContainerLayout::GetTypoBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	//call GetLayoutBounds in order to update both layout & typo metrics
	GetLayoutBounds( inGC, outBounds, inTopLeft, inReturnCharBoundsForEmptyText);
	if (!fIsDirty)
	{
		outBounds.SetWidth( fRenderTypoBounds.GetWidth());
		outBounds.SetHeight( fRenderTypoBounds.GetHeight());
	}
}

void VDocContainerLayout::GetLayoutBounds( VGraphicContext *inGC, VRect& outBounds, const VPoint& inTopLeft, bool inReturnCharBoundsForEmptyText)
{
	if (inGC)
	{
		if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
			return;
		if (inGC == fBackupParentGC)
		{
			xbox_assert(fGC);
			inGC = fGC;
		}
		xbox_assert(inGC);

		BeginUsingContext(inGC, true);

		if (testAssert(!fIsDirty))
		{
			outBounds = fRenderLayoutBounds;
			outBounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
		}
		else
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);

		EndUsingContext();
	}
	else
	{
		//return stored metrics

		if (!fIsDirty)
		{
			outBounds = fRenderLayoutBounds;
			outBounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());
		}
		else
			outBounds.SetCoordsTo( inTopLeft.GetX(), inTopLeft.GetY(), 0.0f, 0.0f);
	}
}

void VDocContainerLayout::GetRunBoundsFromRange( VGraphicContext *inGC, std::vector<VRect>& outRunBounds, const VPoint& inTopLeft, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (inEnd < 0)
		inEnd = GetTextLength();
	if (GetTextLength() == 0 || inStart >= inEnd)
	{
		outRunBounds.clear();
		return;
	}

	outRunBounds.clear();

	BeginUsingContext(inGC, true);
	if (!testAssert(!fIsDirty))
	{
		EndUsingContext();
		return;
	}

	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to layout origin+vertical align offset & coordinate space is mapped to computing dpi point

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First( inStart, inEnd);
	VectorOfPoint::const_iterator itOrigin = fChildOrigin.begin();
	if (layout && layout->GetChildIndex() > 0)
		std::advance( itOrigin, layout->GetChildIndex());
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			std::vector<VRect> runBounds;
			textLayout->GetRunBoundsFromRange( inGC, runBounds, *itOrigin, inStart-layout->GetTextStart(), inEnd-layout->GetTextStart()); 
			if (!runBounds.empty())
			{
				std::vector<VRect>::const_iterator itBounds = runBounds.begin();
				for(;itBounds != runBounds.end(); itBounds++)
				{
					VRect bounds = xform.TransformRect( *itBounds);
					outRunBounds.push_back( bounds);
				}
			}
		}

		layout = it.Next();
		itOrigin++;
	}

		
	_EndLocalTransform( inGC, xform, false);

	EndUsingContext();
}

void VDocContainerLayout::GetCaretMetricsFromCharIndex(	VGraphicContext *inGC, const VPoint& inTopLeft, const VIndex _inCharIndex, 
														VPoint& outCaretPos, GReal& outTextHeight, 
														const bool inCaretLeading, const bool inCaretUseCharMetrics)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	sLONG inCharIndex = _inCharIndex;
	if (inCharIndex < 0)
		inCharIndex = 0;

	BeginUsingContext(inGC, true);
	if (!testAssert(!fIsDirty))
	{
		EndUsingContext();
		return;
	}


	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); //here (0,0) is sticked to layout box origin+vertical align offset & coordinate space is mapped to computing dpi point

	VDocBaseLayout *layout = dynamic_cast<VDocBaseLayout *>(GetFirstChild(inCharIndex));
	if (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			textLayout->GetCaretMetricsFromCharIndex( inGC, fChildOrigin[textLayout->GetChildIndex()], inCharIndex-textLayout->GetTextStart(), outCaretPos, outTextHeight, inCaretLeading, inCaretUseCharMetrics);
			outCaretPos = xform.TransformPoint( outCaretPos);
			outTextHeight = xform.GetScaleY()*outTextHeight; //remark: we assume here that there is no shearing or rotation (which should be always the case while editing text document)
		}
	}

	_EndLocalTransform( inGC, xform, false);

	EndUsingContext();
}

/** get child layout from local position (relative to layout box origin+vertical align decal & computing dpi)*/
VDocBaseLayout* VDocContainerLayout::_GetChildFromLocalPos(const VPoint& inPos)
{
	if (fChildren.size() == 0)
		return NULL;

	GReal y = inPos.GetY();
	sLONG index = 0;
	if (y > 0)
	{
		if (y >= fChildOrigin.back().GetY())
			index = fChildOrigin.size()-1;
		else //search dichotomically
		{
			sLONG start = 0;
			sLONG end = fChildOrigin.size()-1;
			while (start < end)
			{
				sLONG mid = (start+end)/2;
				if (start < mid)
				{
					if (y < fChildOrigin[mid].GetY())
						end = mid-1;
					else
						start = mid;
				}
				else
				{ 
					xbox_assert(start == mid && end == start+1);
					if (y < fChildOrigin[end].GetY())
						index = start;
					else
						index = end;
					break;
				}
			}
			if (start >= end)
				index = start;
		}
	}

	xbox_assert(index < fChildren.size());
	return fChildren[index].Get();
}

bool VDocContainerLayout::GetCharIndexFromPos( VGraphicContext *inGC, const VPoint& inTopLeft, const VPoint& inPos, VIndex& outCharIndex, sLONG *outRunLength)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return false;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (GetTextLength() == 0)
	{
		outCharIndex = 0;
		if (outRunLength)
			*outRunLength = 0;
		return false;
	}

	BeginUsingContext(inGC, true);
	if (!testAssert(!fIsDirty))
	{
		EndUsingContext();
		outCharIndex = 0;
		if (outRunLength)
			*outRunLength = 0;
		return false;
	}


	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); 

	VPoint pos( inPos);
	pos = xform.Inverse().TransformPoint( pos); //transform from ctm space to local space - where (0,0) is sticked to layout box origin+vertical align offset & according to computing dpi

	VDocBaseLayout *layout = _GetChildFromLocalPos( pos);
	if (testAssert(layout))
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			VPoint origin = fChildOrigin[layout->GetChildIndex()];
			textLayout->GetCharIndexFromPos( inGC, origin, pos, outCharIndex, outRunLength);
			outCharIndex += textLayout->GetTextStart();
		}
	}
	else
	{
		outCharIndex = 0;
		if (outRunLength)
			*outRunLength = 0;
	}

	_EndLocalTransform( inGC, xform, false);

	EndUsingContext();

	return true;
}


/** get character range which intersects the passed bounds (in gc user space) 
@remarks
	layout origin is set to inTopLeft (in gc user space)
*/
void VDocContainerLayout::GetCharRangeFromRect( VGraphicContext *inGC, const VPoint& inTopLeft, const VRect& inBounds, sLONG& outRangeStart, sLONG& outRangeEnd)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	BeginUsingContext( inGC, true);

	VAffineTransform xform;
	_BeginLocalTransform( inGC, xform, inTopLeft, false); 

	VPoint pos( inBounds.GetTopLeft());
	pos = xform.Inverse().TransformPoint( pos); //transform from ctm space to local space - where (0,0) is sticked to layout box origin+vertical align offset & according to computing dpi

	VDocBaseLayout *layout = _GetChildFromLocalPos( pos);
	if (testAssert(layout))
	{
		outRangeStart = GetTextLength()+1;
		outRangeEnd = 0;

		VectorOfChildren::const_iterator itChild = fChildren.begin();
		if (layout->GetChildIndex() > 0)
			std::advance(itChild, layout->GetChildIndex());
		do
		{
			ITextLayout *textLayout = layout->QueryTextLayoutInterface();
			if (testAssert(textLayout))
			{
				VPoint origin = fChildOrigin[layout->GetChildIndex()];
				VRect bounds;
				bounds = xform.Inverse().TransformRect( inBounds);
				sLONG start = 0, end = 0;
				textLayout->GetCharRangeFromRect( inGC, origin, bounds, start, end);
				if (start <= end)
				{
					start += textLayout->GetTextStart();
					end += textLayout->GetTextStart();
					if (start < outRangeStart)
						outRangeStart = start;
					if (end > outRangeEnd)
						outRangeEnd = end;
				}
			}
			
			itChild++;
			if (itChild != fChildren.end())
				layout = (*itChild).Get();
			else
				layout = NULL;
		} while (layout);
		if (!fParent && outRangeStart > outRangeEnd)
			outRangeStart = outRangeEnd = 0;
	}
	else
		outRangeStart = outRangeEnd = 0;

	_EndLocalTransform( inGC, xform, false);

	EndUsingContext();
}


/** move character index to the nearest character index on the line before/above the character line 
@remarks
	return false if character is on the first line (ioCharIndex is not modified)
*/
bool VDocContainerLayout::MoveCharIndexUp( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return false;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (GetTextLength() == 0 || ioCharIndex <= 0)
		return false;

	BeginUsingContext(inGC, true);
	if (!testAssert(!fIsDirty))
	{
		EndUsingContext();
		return false;
	}

	bool result = true;

	IDocBaseLayout *layout = GetFirstChild( ioCharIndex);
	if (testAssert(layout))
	{
		VDocBaseLayout *baseLayout = dynamic_cast<VDocBaseLayout *>(layout);
		xbox_assert(baseLayout);
		ITextLayout *textLayout = baseLayout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			sLONG charIndex = ioCharIndex-textLayout->GetTextStart();
			if (textLayout->MoveCharIndexUp( inGC, inTopLeft, charIndex))
				ioCharIndex = charIndex+textLayout->GetTextStart();
			else if (textLayout->GetChildIndex() > 0)
			{
				//get this layout char metrics (remark: only child origin is mandatory here so we can ignore inTopLeft & local transform)

				VPoint charPos;
				GReal textHeight;
				VPoint origin = fChildOrigin[textLayout->GetChildIndex()];
				textLayout->GetCaretMetricsFromCharIndex( inGC, origin, charIndex, charPos, textHeight, true, false);
				
				//now get previous sibling layout (which is up)
				baseLayout = fChildren[textLayout->GetChildIndex()-1];
				xbox_assert(baseLayout);
				textLayout = baseLayout->QueryTextLayoutInterface();
				if (testAssert(textLayout))
				{
					//now use previous char position to determine layout char index

					origin = fChildOrigin[textLayout->GetChildIndex()];
					textLayout->GetCharIndexFromPos( inGC, origin, charPos, charIndex);
					xbox_assert(charIndex >= 0 && charIndex <= textLayout->GetTextLength());
					result = (charIndex + textLayout->GetTextStart()) != ioCharIndex;
					ioCharIndex = charIndex + textLayout->GetTextStart();
				}
				else
					result = false;
			}
			else
				result = false;
		}
		else 
			result = false;
	}
	else
		result = false;

	EndUsingContext();

	return result;
}

/** move character index to the nearest character index on the line after/below the character line
@remarks
	return false if character is on the last line (ioCharIndex is not modified)
*/
bool VDocContainerLayout::MoveCharIndexDown( VGraphicContext *inGC, const VPoint& inTopLeft, sLONG& ioCharIndex)
{
	if (!testAssert(fGC == NULL || fGC == inGC || inGC == fBackupParentGC))
		return false;
	xbox_assert(inGC);
	if (inGC == fBackupParentGC)
	{
		xbox_assert(fGC);
		inGC = fGC;
	}
	xbox_assert(inGC);

	if (GetTextLength() == 0 || ioCharIndex >= GetTextLength()-1)
		return false;

	BeginUsingContext(inGC, true);
	if (!testAssert(!fIsDirty))
	{
		EndUsingContext();
		return false;
	}

	bool result = true;

	IDocBaseLayout *layout = GetFirstChild( ioCharIndex);
	if (testAssert(layout))
	{
		VDocBaseLayout *baseLayout = dynamic_cast<VDocBaseLayout *>(layout);
		xbox_assert(baseLayout);
		ITextLayout *textLayout = baseLayout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			sLONG charIndex = ioCharIndex-textLayout->GetTextStart();
			if (textLayout->MoveCharIndexDown( inGC, inTopLeft, charIndex))
				ioCharIndex = charIndex+textLayout->GetTextStart();
			else if (textLayout->GetChildIndex()+1 < fChildren.size())
			{
				//get this layout char metrics (remark: only child origin is mandatory here so we can ignore inTopLeft & local transform)

				VPoint charPos;
				GReal textHeight;
				VPoint origin = fChildOrigin[textLayout->GetChildIndex()];
				textLayout->GetCaretMetricsFromCharIndex( inGC, origin, charIndex, charPos, textHeight, true, false);
				
				//now get next sibling layout (which is down)
				baseLayout = fChildren[textLayout->GetChildIndex()+1];
				xbox_assert(baseLayout);
				textLayout = baseLayout->QueryTextLayoutInterface();
				if (testAssert(textLayout))
				{
					//now use previous char position to determine layout char index

					origin = fChildOrigin[textLayout->GetChildIndex()];
					textLayout->GetCharIndexFromPos( inGC, origin, charPos, charIndex);
					xbox_assert(charIndex >= 0 && charIndex <= textLayout->GetTextLength());
					result = (charIndex + textLayout->GetTextStart()) != ioCharIndex;
					ioCharIndex = charIndex + textLayout->GetTextStart();
				}
				else
					result = false;
			}
			else
				result = false;
		}
		else 
			result = false;
	}
	else
		result = false;

	EndUsingContext();

	return result;
}



/** create and retain paragraph from the current layout plain text, styles & paragraph properties */
VDocParagraph *VDocContainerLayout::CreateParagraph(sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraph *para = new VDocParagraph();

	if (inEnd == -1)
		inEnd = GetTextLength();

	//copy plain text & styles from the passed range to the paragraph node
	VString text;
	GetText( text, inStart, inEnd);
	VTreeTextStyle *styles = RetainStyles( inStart, inEnd, true);
	para->SetText( text, styles, false);
	ReleaseRefCountable(&styles);

	//then copy paragraph properties (only from the first paragraph layout)
	VDocBaseLayout *layout = dynamic_cast<VDocBaseLayout *>(GetFirstChild( inStart));
	while (layout && layout->GetDocType() != kDOC_NODE_TYPE_PARAGRAPH)
	{
		//search deeply for first paragraph
		inStart -= layout->GetTextStart();
		layout =  dynamic_cast<VDocBaseLayout *>(layout->GetFirstChild( inStart));
	}
	if (layout)
		layout->SetPropsTo( static_cast<VDocNode *>(para));
	return para;
}


/** apply paragraph properties & styles if any to the current layout (passed paragraph text is ignored) 
@param inPara
	the paragraph style
@param inResetStyles
	if true, reset styles to default prior to apply new paragraph style
@remarks
	such paragraph is just used for styling:
	so note that only paragraph first level VTreeTextStyle styles are applied (to the layout text range: paragraph style range is ignored)

	also only paragraph properties or character styles which are overriden are applied
	(so to ensure to replace all, caller needs to define explicitly all paragraph & character styles
	 or call ResetParagraphStyle() before)
*/
void VDocContainerLayout::ApplyParagraphStyle( const VDocParagraph *inPara, bool inResetStyles, bool inApplyParagraphPropsOnly, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->ApplyParagraphStyle( inPara, inResetStyles, inApplyParagraphPropsOnly, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** init layout from the passed paragraph 
@param inPara
	paragraph source
@remarks
	the passed paragraph is used only for initializing layout plain text, styles & paragraph properties but inPara is not retained 
	note here that current styles are reset (see ResetParagraphStyle) & replaced by new paragraph styles
	also current plain text is replaced by the passed paragraph plain text 
*/
void VDocContainerLayout::InitFromParagraph( const VDocParagraph *inPara)
{
	xbox_assert(inPara);

	SetDirty();

	fChildren.clear();

	VDocParagraphLayout *paraLayout = new VDocParagraphLayout( inPara, fShouldEnableCache);
	AddChild( static_cast<IDocBaseLayout *>(static_cast<VDocBaseLayout *>(paraLayout)));
	paraLayout->SetPlaceHolder( fPlaceHolder);
	paraLayout->Release();
}

/** reset all paragraph properties & character styles to default values */
void VDocContainerLayout::ResetParagraphStyle(bool inResetParagraphPropsOnly, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->ResetParagraphStyle( inResetParagraphPropsOnly, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}


/** merge paragraph(s) on the specified range to a single paragraph 
@remarks
	if there are more than one paragraph on the range, plain text & styles from the paragraphs but the first one are concatenated with first paragraph plain text & styles
	resulting with a single paragraph on the range (using paragraph styles from the first paragraph) where plain text might contain more than one CR

	remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
	you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
*/
bool VDocContainerLayout::MergeParagraphs( sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);
	if (inStart >= inEnd)
		return false;

	VDocParagraphLayout *paraStart  = _GetParagraphFromCharIndex(inStart);
	VDocParagraphLayout *paraEnd  = _GetParagraphFromCharIndex(inEnd-1);
	if (paraStart == paraEnd)
		return false;

	VString text;
	GetText(text, inStart, inEnd);
	xbox_assert(text.GetLength() == inEnd-inStart);
	VTreeTextStyle *styles = RetainStyles( inStart, inEnd);
	SetText( text, styles, inStart, inEnd);
	ReleaseRefCountable(&styles);
	return true;
}

/** unmerge paragraph(s) on the specified range 
@remarks
	any paragraph on the specified range is split to two or more paragraphs if it contains more than one terminating CR, 
	resulting in paragraph(s) which contain only one terminating CR (sharing the same paragraph style)

	remember that VDocParagraphLayout (& VDocParagraph) represent a html paragraph (which might contain more than one CR) and not a paragraph with a single terminating CR:
	you should use UnMergeParagraphs to convert from html paragraphs to classical paragraphs
*/
bool VDocContainerLayout::UnMergeParagraphs( sLONG inStart, sLONG inEnd)
{
	std::vector<sLONG> indexs;

	_NormalizeRange( inStart, inEnd);

	//as calling UnMergeParagraphs on child node(s) might change fChildren,
	//we cannot simply iterate on children

	VDocLayoutChildrenReverseIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		indexs.push_back( layout->GetChildIndex());
		layout = it.Next();
	}

	bool result = false;
	VectorOfChildren::iterator itLayout;
	std::vector<sLONG>::const_iterator itIndex = indexs.begin();
	for(; itIndex != indexs.end(); itIndex++)
	{
		itLayout = fChildren.begin();
		std::advance(itLayout, *itIndex);
		
		ITextLayout *textLayout = (*itLayout).Get()->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			result = textLayout->UnMergeParagraphs( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart()) || result;
	}
	if (result)
		SetDirty();
	return result;
}


/** set default text color 
@remarks
	black color on default (it is applied prior styles set with SetText)
*/
void VDocContainerLayout::SetDefaultTextColor( const VColor &inTextColor, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetDefaultTextColor( inTextColor, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get default text color */
bool VDocContainerLayout::GetDefaultTextColor(VColor& outColor, sLONG inStart) const 
{ 
	//return first paragraph default text color
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetDefaultTextColor( outColor);
	else
		return NULL;
}


/** set default font 
@remarks
	it should be not NULL - STDF_TEXT standard font on default (it is applied prior styles set with SetText)

	by default, input font is assumed to be 4D form-compliant font (created with dpi = 72)
*/
void VDocContainerLayout::SetDefaultFont( const VFont *inFont, GReal inDPI, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetDefaultFont( inFont, inDPI, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}


/** get & retain default font 
@remarks
	by default, return 4D form-compliant font (with dpi = 72) so not the actual fDPI-scaled font 
	(for consistency with SetDefaultFont & because fDPI internal scaling should be transparent for caller)
*/
VFont *VDocContainerLayout::RetainDefaultFont(GReal inDPI, sLONG inStart) const
{
	//return first paragraph default font
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->RetainDefaultFont( inDPI);
	else
		return NULL;
}


VTreeTextStyle* VDocContainerLayout::RetainDefaultStyle(sLONG inStart, sLONG inEnd) const
{
	//return first paragraph default style
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->RetainDefaultStyle( inStart);
	else
		return NULL;
}


const VString& VDocContainerLayout::GetText() const
{
	//return first paragraph plain text
	static VString sEmpty;
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex(0);
	if (paraLayout)
		return paraLayout->GetText();
	else
		return sEmpty;
}



/** set max text width (0 for infinite) - in pixel (pixel relative to the rendering DPI)
@remarks
	it is also equal to text box render width 
	so you should specify one if text is not aligned to the left border
	otherwise text layout box width will be always equal to formatted text width
*/ 
void VDocContainerLayout::SetMaxWidth(GReal inMaxWidth) 
{
	if (inMaxWidth)
	{
		sLONG width = ICSSUtil::PointToTWIPS(inMaxWidth*72/fRenderDPI);
		width -= _GetAllMarginHorizontalTWIPS(); //subtract margins
		if (width <= 0)
			width = 1;
		VDocContainerLayout::SetWidth( width);
	}
	else
		VDocContainerLayout::SetWidth( 0);
}

/** set max text height (0 for infinite) - in pixel (pixel relative to the rendering DPI)
@remarks
	it is also equal to text box height 
	so you should specify one if paragraph is not aligned to the top border
	otherwise text layout box height will be always equal to formatted text height
*/ 
void VDocContainerLayout::SetMaxHeight(GReal inMaxHeight) 
{
	if (inMaxHeight)
	{
		sLONG height = ICSSUtil::PointToTWIPS(inMaxHeight*72/fRenderDPI);
		height -= _GetAllMarginVerticalTWIPS(); //subtract margins
		if (height <= 0)
			height = 1;
		VDocContainerLayout::SetHeight( height);
	}
	else
		VDocContainerLayout::SetHeight( 0);
}



/** set paragraph line height 

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height 
*/
void VDocContainerLayout::SetLineHeight( const GReal inLineHeight, sLONG inStart, sLONG inEnd) 
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetLineHeight( inLineHeight, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get paragraph line height 

	if positive, it is fixed line height in point
	if negative, line height is (-value)*normal line height (so -2 is 2*normal line height)
	if -1, it is normal line height 
*/
GReal VDocContainerLayout::GetLineHeight(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetLineHeight();
	else
		return -1;
}

void VDocContainerLayout::SetRTL(bool inRTL, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetRTL( inRTL, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}


/** set first line padding offset in point 
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
void VDocContainerLayout::SetTextIndent(const GReal inPadding, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetTextIndent( inPadding, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get first line padding offset in point
@remarks
	might be negative for negative padding (that is second line up to the last line is padded but the first line)
*/
GReal VDocContainerLayout::GetTextIndent(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetTextIndent();
	else
		return 0;
}


/** set tab stop (offset in point) */
void VDocContainerLayout::SetTabStop(const GReal inOffset, const eTextTabStopType& inType, sLONG inStart, sLONG inEnd)
{
	if (!testAssert(inOffset > 0.0f))
		return;

	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetTabStop( inOffset, inType, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get tab stop offset in point */
GReal VDocContainerLayout::GetTabStopOffset(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetTabStopOffset();
	else
		return 0;
}

/** get tab stop type */ 
eTextTabStopType VDocContainerLayout::GetTabStopType(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetTabStopType();
	else
		return TTST_LEFT;
}

/** set text layout mode (default is TLM_NORMAL) */
void VDocContainerLayout::SetLayoutMode( TextLayoutMode inLayoutMode, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetLayoutMode( inLayoutMode, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get text layout mode */
TextLayoutMode VDocContainerLayout::GetLayoutMode(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetLayoutMode();
	else
		return TLM_NORMAL;
}


/** set outside margins	(in twips) - CSS margin - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaMargins(const sDocPropRect& inMargins, bool inCalcAllMargin, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaMargins( inMargins, inCalcAllMargin, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get outside margins (in twips) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::GetParaMargins( sDocPropRect& outMargins, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		paraLayout->GetMargins( outMargins);
	else
		outMargins.left = outMargins.top = outMargins.right = outMargins.bottom = 0;
}

/** set inside margins	(in twips) - CSS padding - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaPadding(const sDocPropRect& inPadding, bool inCalcAllMargin, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaPadding( inPadding, inCalcAllMargin, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** get inside margins	(in twips) - CSS padding - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::GetParaPadding(sDocPropRect& outPadding, sLONG inStart, sLONG inEnd) const 
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		paraLayout->GetPadding( outPadding);
	else
		outPadding.left = outPadding.top = outPadding.right = outPadding.bottom = 0;
}

/** set text align - CSS text-align (-1 for not defined) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaTextAlign( const AlignStyle inHAlign, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaTextAlign( inHAlign, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

AlignStyle VDocContainerLayout::GetParaTextAlign(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetTextAlign();
	else
		return GetTextAlign();
}

/** set vertical align - CSS vertical-align (AL_DEFAULT for default) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaVerticalAlign( const AlignStyle inVAlign, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaVerticalAlign( inVAlign, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

AlignStyle VDocContainerLayout::GetParaVerticalAlign(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetVerticalAlign();
	else
		return AL_TOP;
}

/** set width (in twips) - CSS width (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaWidth(const uLONG inWidth, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaWidth( inWidth, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
uLONG VDocContainerLayout::GetParaWidth(bool inIncludeAllMargins, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetWidth();
	else
		return 0;
}

/** set height (in twips) - CSS height (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaHeight(const uLONG inHeight, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaHeight( inHeight, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
uLONG VDocContainerLayout::GetParaHeight(bool inIncludeAllMargins, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetHeight();
	else
		return 0;
}

/** set min width (in twips) - CSS min-width (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaMinWidth(const uLONG inWidth, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaMinWidth( inWidth, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
uLONG VDocContainerLayout::GetParaMinWidth(bool inIncludeAllMargins, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetMinWidth();
	else
		return 0;
}

/** set min height (in twips) - CSS min-height (0 for auto) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaMinHeight(const uLONG inHeight, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaMinHeight( inHeight, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
uLONG VDocContainerLayout::GetParaMinHeight(bool inIncludeAllMargins, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetMinHeight();
	else
		return 0;
}

/** set background color - CSS background-color (transparent for not defined) - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaBackgroundColor( const VColor& inColor, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaBackgroundColor( inColor, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
const VColor& VDocContainerLayout::GetParaBackgroundColor(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetBackgroundColor();
	else
		return GetBackgroundColor();
}

/** set background clipping - CSS background-clip - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaBackgroundClip( const eDocPropBackgroundBox inBackgroundClip, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaBackgroundClip( inBackgroundClip, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
eDocPropBackgroundBox VDocContainerLayout::GetParaBackgroundClip(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetBackgroundClip();
	else
		return kDOC_BACKGROUND_BORDER_BOX;
}

/** set background origin - CSS background-origin - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance */
void VDocContainerLayout::SetParaBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaBackgroundOrigin( inBackgroundOrigin, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}
eDocPropBackgroundBox VDocContainerLayout::GetParaBackgroundOrigin(sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inStart);
	if (paraLayout)
		return paraLayout->GetBackgroundOrigin();
	else
		return kDOC_BACKGROUND_PADDING_BOX;
}

/** set borders - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance 
		@remarks
			if inLeft is NULL, left border is same as right border 
			if inBottom is NULL, bottom border is same as top border
			if inRight is NULL, right border is same as top border
			if inTop is NULL, top border is default border (solid, medium width & black color)
		*/
void VDocContainerLayout::SetParaBorders( const sDocBorder *inTop, const sDocBorder *inRight, const sDocBorder *inBottom, const sDocBorder *inLeft, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaBorders( inTop, inRight, inBottom, inLeft, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** clear borders */
void VDocContainerLayout::ClearParaBorders(sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->ClearParaBorders( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** set background image - on the passed range for embedded paragraph layout instance(s) or for this layout instance if it is a paragraph layout instance 
@remarks
	see VDocBackgroundImageLayout constructor for parameters description */
void VDocContainerLayout::SetParaBackgroundImage(	const VString& inURL,	
										const eStyleJust inHAlign, const eStyleJust inVAlign, 
										const eDocPropBackgroundRepeat inRepeat,
										const GReal inWidth, const GReal inHeight,
										const bool inWidthIsPercentage,  const bool inHeightIsPercentage,
										sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->SetParaBackgroundImage( inURL, inHAlign, inVAlign, 
												inRepeat,
												inWidth, inHeight,
												inWidthIsPercentage, inHeightIsPercentage,
												inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}

/** clear background image */
void VDocContainerLayout::ClearParaBackgroundImage(sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			textLayout->ClearParaBackgroundImage( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());

		layout = it.Next();
	}
}


/** begin using text layout for the specified gc */
void VDocContainerLayout::BeginUsingContext( VGraphicContext *inGC, bool inNoDraw)
{
	xbox_assert(inGC);
	
	if (fGCUseCount > 0)
	{
		xbox_assert(fGC == inGC || inGC == fBackupParentGC);
		if (!fParent)
			fGC->BeginUsingContext( inNoDraw);
		//if caller has updated some layout settings, we need to update again the layout
		if (fIsDirty)
			_UpdateLayout();
		fGCUseCount++;
		return;
	}
	
	fGCUseCount++;

	if (fParent)
	{
		fIsPrinting = inGC->IsPrintingContext();
		if (fIsPrinting)
			fIsDirty = true;
		xbox_assert(fGC == inGC);
		fNeedRestoreTRM = false;
		
		_SetGC(inGC);
		_UpdateLayout();
		return;
	}

	inGC->BeginUsingContext( inNoDraw);
	inGC->UseReversedAxis();

	fIsPrinting = inGC->IsPrintingContext();
	if (fIsPrinting)
		fIsDirty = true;

#if VERSIONWIN
	//if GDI and not printing, derive to GDIPlus for enhanced graphic features onscreen
	xbox_assert(fBackupParentGC == NULL);
	if (!fIsPrinting && inGC->IsGDIImpl())
	{
		CopyRefCountable(&fBackupParentGC, inGC);
		inGC = inGC->BeginUsingGdiplus();
	}
#endif

	_SetGC( inGC);
	xbox_assert(fGC == inGC);

#if VERSIONWIN
	//backup & restore trm only for top level layout
	fBackupTRM = fGC->GetDesiredTextRenderingMode();
	fNeedRestoreTRM = true;

	TextRenderingMode trm = fBackupTRM;

	if (fGC->IsGdiPlusImpl() && (!(fGC->GetTextRenderingMode() & TRM_LEGACY_ON)) && (!(fGC->GetTextRenderingMode() & TRM_LEGACY_OFF)))//force always legacy for GDIPlus -  as native text layout is allowed only for D2D - with DWrite (and actually only with v15+)
		trm = (trm & (~TRM_LEGACY_OFF)) | TRM_LEGACY_ON;
	
	if (trm != fBackupTRM)
		fGC->SetTextRenderingMode( trm);
#endif

	if (!fIsDirty)
	{
		//check if text rendering mode has changed
		if (fLayoutTextRenderingMode != fGC->GetTextRenderingMode())
			SetDirty(true);
		//check if kerning has changed (only on Mac OS: kerning is not used for text box layout on other impls)
		else if (fLayoutKerning != fGC->GetCharActualKerning())
			SetDirty(true);
	}

	//update text layout (only if it is no longer valid)
	_UpdateLayout();

}

/** end using text layout */
void VDocContainerLayout::EndUsingContext()
{
	fGCUseCount--;
	xbox_assert(fGCUseCount >= 0);
	if (fGCUseCount > 0)
	{
		if (!fParent)
			fGC->EndUsingContext();
		return;
	}

	if (!fParent)
	{
#if VERSIONWIN
		if (fNeedRestoreTRM)
			fGC->SetTextRenderingMode( fBackupTRM);
		if (fBackupParentGC)
		{
			fBackupParentGC->EndUsingGdiplus();
			fBackupParentGC->EndUsingContext();
			ReleaseRefCountable(&fBackupParentGC);
		}
		else
#endif
			fGC->EndUsingContext();
	}
	_SetGC( NULL);
}





/** set text layout graphic context 
@remarks
	it is the graphic context to which is bound the text layout
	if offscreen layer is disabled text layout needs to be redrawed again at every frame
	so it is recommended to enable the internal offscreen layer if you intend to preserve the same rendered layout on multiple frames
	because offscreen layer & text content are preserved as long as offscreen layer is compatible with the attached gc & text content is not dirty
*/
void VDocContainerLayout::_SetGC( VGraphicContext *inGC)
{
	if (fGC == inGC)
		return;

	if (inGC)
	{
		fIsDirty = fIsDirty || !fShouldEnableCache || fIsPrinting;
		
		//if new gc is not compatible with last used gc, we need to update layout

		if (!fParent && !inGC->IsCompatible( fGCType, !fShouldDrawOnLayer)) //check if current layout is compatible with new gc
																			 //we take account hardware status if and only if layers are enabled
																			 //(if hardware status has changed, we need to update layout so children layout can update layer)
			SetDirty(true);//gc kind has changed: inval layout & children layouts

		fGCType = inGC->GetGraphicContextType(); //backup current gc type
	}

	//attach new gc or dispose actual gc (if inGC == NULL)
	CopyRefCountable(&fGC, inGC);
}




/** replace current text & styles
@remarks	
	here passed styles are always copied
*/
void VDocContainerLayout::SetText(	const VString& inText, const VTreeTextStyle *inStyles, sLONG inStart, sLONG inEnd, 
									bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, 
									const VDocSpanTextRef * inPreserveSpanTextRef, 
									void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	_NormalizeRange( inStart, inEnd);

	SetDirty();

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	sLONG layoutFirstIndex = layout ? layout->GetChildIndex() : -1;
	eDocNodeType docType = layout ? layout->GetDocType() : kDOC_NODE_TYPE_UNKNOWN;
	bool isParagraph = docType == kDOC_NODE_TYPE_PARAGRAPH;
	if (testAssert(layout))
	{
		//first iterate on children which intersect range but the first one

		sLONG indexStart = -1;
		sLONG indexEnd = -1;
		layout = isParagraph || inText.IsEmpty() ? it.Next() : NULL; //we do not modify other layout children if it is a container and if we replace with not empty text (word-like behavior)
																	 //-> i.e. we replace only first container plain text if we do not remove only text
		VString textEnd;
		VTreeTextStyle *stylesEnd = NULL;
		while (layout)
		{
			xbox_assert(docType == layout->GetDocType()); //for now, container children should have the same type

			bool doContinue = true;
			if (layout->GetTextStart()+layout->GetTextLength() > inEnd)
			{
				//last layout is partially truncated - maybe

				ITextLayout *textLayout = layout->QueryTextLayoutInterface();
				if (testAssert(textLayout))
				{
					//we adjust end range to be sure (!)
					inEnd -= layout->GetTextStart();
					textLayout->AdjustPos( inEnd, true);
					inEnd += layout->GetTextStart();
					if (layout->GetTextStart()+layout->GetTextLength() > inEnd) //it is still partially truncated
					{
						//truncate it
						xbox_assert(inEnd >= layout->GetTextStart());
						textLayout->DeletePlainText(0, inEnd-layout->GetTextStart());
						
						if (isParagraph)
						{
							//backup text & styles (to concat with first paragraph text & styles)
							textLayout->GetText( textEnd);
							stylesEnd = textLayout->RetainStyles();

							//mark for remove
							if (indexStart < 0)
								indexStart = indexEnd = layout->GetChildIndex();
							else
								indexEnd = layout->GetChildIndex();
						}
						doContinue = false;
					}
				}
				if (!doContinue)
					break;
			}
			{
				//plain text is fully truncated
				//mark paragraph node for remove

				if (indexStart < 0)
					indexStart = indexEnd = layout->GetChildIndex();
				else
					indexEnd = layout->GetChildIndex();

				layout = it.Next();
			}
		}
		if (layout == NULL && isParagraph)
		{
			//maybe we need to merge text & styles with next sibling if any (if it is a paragraph)
			if (indexEnd < 0)
			{
				//we replace only first paragraph -> check if replaced range includes terminating CR -> if so we need to merge plain text with next paragraph
				indexEnd = layoutFirstIndex;
				if (inStart < inEnd && indexEnd + 1 < fChildren.size())
				{
					layout = fChildren[layoutFirstIndex].Get();
					if (layout->GetTextStart()+layout->GetTextLength() == inEnd && (inText.IsEmpty() || inText.GetUniChar(inText.GetLength()) != 13))
						//merge with next layout
						layout = fChildren[indexEnd+1].Get();
					else
						layout = NULL;
				}
			}
			else if (inText.IsEmpty() || inText.GetUniChar(inText.GetLength()) != 13)
				//we are about to delete some paragraphs which are fully included in range -> we need to merge first paragraph plain text with next paragraph plain text which is not removed & remove this last paragraph too
				layout = (indexEnd + 1 < fChildren.size()) ? fChildren[indexEnd+1].Get() : NULL;
			if (layout && layout->GetTextLength())
			{
				xbox_assert(docType == layout->GetDocType()); //for now, container children should have the same type

				ITextLayout *textLayout = layout->QueryTextLayoutInterface();
				if (testAssert(textLayout))
				{
					textLayout->GetText( textEnd);
					stylesEnd = textLayout->RetainStyles(0, -1, false, true, true);
				}
			}
			else
				layout = NULL; 
			if (layout)
			{
				//mark for remove
				indexEnd++;
				if (indexStart < 0)
					indexStart = indexEnd;
			}
		}
		if (indexStart >= 0)
		{
			if (isParagraph)
			{
				//remove paragraphs which are marked for remove
				for (int i = indexEnd; i >= indexStart; i--)
					VDocBaseLayout::RemoveChild( i);
			}
			else
			{
				//not paragraph(s) -> remove only plain text
				for (int i = indexEnd; i >= indexStart; i--)
				{
					ITextLayout *textLayout = fChildren[i]->QueryTextLayoutInterface();
					xbox_assert(textLayout);
					textLayout->DeletePlainText(0,-1);
				}
			}
		}

		//then replace first layout text
		VDocBaseLayout *layoutFirst = fChildren[layoutFirstIndex].Get();
		ITextLayout *textLayoutFirst = layoutFirst->QueryTextLayoutInterface();
		xbox_assert(textLayoutFirst);

		bool isLast = layoutFirst->IsLastChild(true);

		if (!isLast 
			&& 
			inEnd >= layoutFirst->GetTextStart()+layoutFirst->GetTextLength() //we are about to crop terminating CR
			)
		{
			//ensure paragraph (but the last one) is always terminated by CR 
			if (textEnd.GetLength() > 0)
			{
				if (textEnd[textEnd.GetLength()-1] != 13)
				{
					textEnd.AppendUniChar(13);
					if (stylesEnd)
						stylesEnd->ExpandAtPosBy(textEnd.GetLength()-1,1);
				}
			}
			else if (inText.GetLength() <= 0 || inText[inText.GetLength()-1] != 13)
				textEnd.AppendUniChar(13);
		}

		if (!textEnd.IsEmpty())
		{
			//concat whith plain text & styles 
			VString text(inText);
			text.AppendString(textEnd);

			VTreeTextStyle *styles = new VTreeTextStyle( new VTextStyle());
			styles->GetData()->SetRange( 0, text.GetLength());
			
			VTreeTextStyle *stylesCopy = inStyles ? new VTreeTextStyle( inStyles) : NULL;
			if (stylesCopy)
			{
				styles->AddChild( stylesCopy);
				ReleaseRefCountable(&stylesCopy);
			}
			if (stylesEnd)
			{
				if (inText.GetLength() > 0)
					stylesEnd->Translate(inText.GetLength());
				styles->AddChild( stylesEnd);
				ReleaseRefCountable(&stylesEnd);
			}
			
			if (inBeforeReplace && inAfterReplace)
			{
				//install local delegate in order to map/unmap text indexs from/to local
				fReplaceTextUserData = inUserData;
				fReplaceTextBefore = inBeforeReplace;
				fReplaceTextAfter = inAfterReplace;
				fReplaceTextCurLayout = layoutFirst;

				textLayoutFirst->SetText(	text, styles, inStart-layoutFirst->GetTextStart(), inEnd-layoutFirst->GetTextStart(), 
										inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef, 
										this, _BeforeReplace, _AfterReplace);
			}
			else
				textLayoutFirst->SetText(	text, styles, inStart-layoutFirst->GetTextStart(), inEnd-layoutFirst->GetTextStart(), 
										inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef);
				
			ReleaseRefCountable(&styles);
		}
		else
		{
			if (inBeforeReplace && inAfterReplace)
			{
				//install local delegate in order to map/unmap text indexs from/to local
				fReplaceTextUserData = inUserData;
				fReplaceTextBefore = inBeforeReplace;
				fReplaceTextAfter = inAfterReplace;
				fReplaceTextCurLayout = layoutFirst;

				textLayoutFirst->SetText(	inText, inStyles, inStart-layoutFirst->GetTextStart(), inEnd-layoutFirst->GetTextStart(), 
										inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef, 
										this, _BeforeReplace, _AfterReplace);
			}
			else
				textLayoutFirst->SetText(	inText, inStyles, inStart-layoutFirst->GetTextStart(), inEnd-layoutFirst->GetTextStart(), 
										inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, inPreserveSpanTextRef);
		}
	}
}


void VDocContainerLayout::GetText(VString& outText, sLONG inStart, sLONG inEnd) const
{
	_NormalizeRange( inStart, inEnd);

	outText.Clear();
	if (GetTextLength() == 0)
		return;
	outText.EnsureSize(inEnd-inStart);

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && textLayout->GetTextLength() > 0)
		{
			VString text;
			textLayout->GetText( text, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());
			outText.AppendString( text);
		}
		layout = it.Next();
	}
}

/** return unicode character at passed position (0-based) - 0 if out of range */
UniChar	 VDocContainerLayout::GetUniChar( sLONG inIndex) const
{
	if (inIndex < 0 || inIndex >= GetTextLength())
		return 0;

	sLONG inCharStartIndex = 0;
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inIndex, &inCharStartIndex);
	if (paraLayout)
		return paraLayout->GetUniChar( inIndex-inCharStartIndex);
	else
		return 0;
}


VTreeTextStyle*	VDocContainerLayout::RetainStyles(sLONG inStart, sLONG inEnd, bool inRelativeToStart, bool /*inIncludeDefaultStyle*/, bool /*inCallerOwnItAlways*/) const
{
	_NormalizeRange( inStart, inEnd);

	VTreeTextStyle *stylesLocal = NULL;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && (textLayout->GetTextLength() || inStart == inEnd))
		{
			VTreeTextStyle *styles = textLayout->RetainStyles( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart(), false, true, true);
			if (styles)
			{
				styles->Translate( inRelativeToStart ? textLayout->GetTextStart()-inStart : textLayout->GetTextStart());
				if (!stylesLocal)
				{
					stylesLocal = new VTreeTextStyle( new VTextStyle());
					stylesLocal->GetData()->SetRange( inRelativeToStart ? 0 : inStart, inRelativeToStart ? inEnd-inStart : inEnd);
				}
				if (styles->HasSpanRefs())
					stylesLocal->ApplyStyles( styles, false); //in order span references to be attached to stylesLocal
				else
					stylesLocal->AddChild( styles); //can add styles directly as children layout styles do not overlap
				styles->Release();
			}
		}
		layout = it.Next();
	}
	return stylesLocal;
}


/** convert uniform character index (the index used by 4D commands) to actual plain text position 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VDocContainerLayout::CharPosFromUniform( sLONG& ioPos) const
{
	if (ioPos < 0)
		ioPos = 0;

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		if (ioPos <= layout->GetTextStart())
			break;

		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			ioPos = ioPos-layout->GetTextStart();
			textLayout->CharPosFromUniform( ioPos);
			ioPos += layout->GetTextStart();
		}
		layout = it.Next();
	}
}

/** convert actual plain text char index to uniform char index (the index used by 4D commands) 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VDocContainerLayout::CharPosToUniform( sLONG& ioPos) const
{
	if (ioPos < 0)
		ioPos = 0;

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First();
	sLONG delta = 0;
	while (layout)
	{
		if (ioPos <= layout->GetTextStart())
			break;

		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			sLONG pos = ioPos-layout->GetTextStart();
			sLONG posBackup = pos;
			textLayout->CharPosToUniform( pos);
			delta += pos-posBackup;
		}
		layout = it.Next();
	}
	ioPos += delta;
}

/** convert uniform character range (as used by 4D commands) to actual plain text position 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VDocContainerLayout::RangeFromUniform( sLONG& inStart, sLONG& inEnd) const
{
	if (inEnd < 0)
		inEnd = GetTextLength();

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		if (inEnd <= layout->GetTextStart())
			break;

		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			inStart -= layout->GetTextStart();
			inEnd -= layout->GetTextStart();
			textLayout->RangeFromUniform( inStart, inEnd);
			inStart += layout->GetTextStart();
			inEnd += layout->GetTextStart();
		}
		layout = it.Next();
	}
}


/** convert actual plain text range to uniform range (as used by 4D commands) 
@remarks
	in order 4D commands to use uniform character position while indexing text containing span references 
	(span ref content might vary depending on computed value or showing source or value)
	we need to map actual plain text position with uniform char position:
	uniform character position assumes a span ref has a text length of 1 character only
*/
void VDocContainerLayout::RangeToUniform( sLONG& inStart, sLONG& inEnd) const
{
	if (inEnd < 0)
		inEnd = GetTextLength();

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First();
	sLONG deltaStart = 0, deltaEnd = 0;
	while (layout)
	{
		if (inEnd <= layout->GetTextStart())
			break;

		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			sLONG start = inStart-layout->GetTextStart();
			sLONG startBackup = start;
			sLONG end = inEnd-layout->GetTextStart();
			sLONG endBackup = end;

			textLayout->RangeToUniform( start, end);

			deltaStart += start-startBackup;
			deltaEnd += end-endBackup;
		}
		layout = it.Next();
	}
	inStart += deltaStart;
	inEnd += deltaEnd;
}

/** adjust selection 
@remarks
	you should call this method to adjust with surrogate pairs or span references
*/
void VDocContainerLayout::AdjustRange(sLONG& ioStart, sLONG& ioEnd) const
{
	if (ioEnd < 0)
		ioEnd = GetTextLength();

	if (ioStart == ioEnd)
	{
		//ensure to keep range empty 
		AdjustPos(ioStart,false);
		ioEnd = ioStart;
		return;
	}
	xbox_assert(ioStart < ioEnd);

	AdjustPos( ioStart, false);
	AdjustPos( ioEnd, true);
}

/** adjust char pos
@remarks
	you should call this method to adjust with surrogate pairs or span references
*/
void VDocContainerLayout::AdjustPos(sLONG& ioPos, bool inToEnd) const
{
	if (ioPos <= 0)
		ioPos = 0;
	if (ioPos > GetTextLength())
		ioPos = GetTextLength();

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First(ioPos);
	if (layout && ioPos > layout->GetTextStart() && ioPos < layout->GetTextStart()+layout->GetTextLength())
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			ioPos -= layout->GetTextStart();
			textLayout->AdjustPos( ioPos, inToEnd);
			ioPos += layout->GetTextStart();
		}
	}
}



bool VDocContainerLayout::HasSpanRefs() const
{
	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			if (textLayout->HasSpanRefs())
				return true;
		}

		layout = it.Next();
	}
	return false;
}


/** return span reference at the specified text position if any */
const VTextStyle*	VDocContainerLayout::GetStyleRefAtPos(sLONG inPos, sLONG *outBaseStart) const
{
	if (inPos < 0)
		inPos = 0;
	if (inPos > GetTextLength())
		inPos = GetTextLength();

	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First(inPos);
	if (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			sLONG baseStart = 0;
			const VTextStyle *style = textLayout->GetStyleRefAtPos(inPos-layout->GetTextStart(), &baseStart);
			if (style)
			{
				if (outBaseStart)
					*outBaseStart = baseStart+layout->GetTextStart();
				return style;
			}
		}
	}
	return NULL;
}

/** return the first span reference which intersects the passed range */
const VTextStyle*	VDocContainerLayout::GetStyleRefAtRange(sLONG inStart, sLONG inEnd, sLONG *outBaseStart) const
{
	_NormalizeRange( inStart, inEnd);

	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this)); 
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			sLONG baseStart = 0;
			const VTextStyle *style = textLayout->GetStyleRefAtRange(inStart-layout->GetTextStart(), inEnd-layout->GetTextStart(), &baseStart);
			if (style)
			{
				if (outBaseStart)
					*outBaseStart = baseStart+layout->GetTextStart();
				return style;
			}
		}

		layout = it.Next();
	}
	return NULL;
}


/** replace text with span text reference on the specified range
@remarks
	it will insert plain text as returned by VDocSpanTextRef::GetDisplayValue() - depending so on local display type and on inNoUpdateRef (only NBSP plain text if inNoUpdateRef == true - inNoUpdateRef overrides local display type)

	you should no more use or destroy passed inSpanRef even if method returns false

	IMPORTANT: it does not eval 4D expressions but use last evaluated value for span reference plain text (if display type is set to value):
			   so you must call after EvalExpressions to eval 4D expressions and then UpdateSpanRefs to update 4D exp span references plain text - if you want to show 4D exp values of course
			   (passed inLC is used here only to convert 4D exp tokenized expression to current language and version without the special tokens 
			    - if display type is set to normalized source)
*/
bool VDocContainerLayout::ReplaceAndOwnSpanRef( VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inNoUpdateRef,
									    void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	xbox_assert(inSpanRef);

	_NormalizeRange( inStart, inEnd);

	if (inStart > inEnd)
	{
		delete inSpanRef;
		return false;
	}

	inSpanRef->ShowHighlight(fHighlightRefs);
	inSpanRef->ShowRef(inSpanRef->CombinedTextTypeMaskToTextTypeMask( fShowRefs));

	VTextStyle *style = new VTextStyle();
	style->SetSpanRef( inSpanRef);
	if (inNoUpdateRef)
		style->GetSpanRef()->ShowRef( kSRTT_Uniform | (style->GetSpanRef()->ShowRef() & kSRTT_4DExp_Normalized));
	if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp && style->GetSpanRef()->ShowRefNormalized() && (style->GetSpanRef()->ShowRef() & kSRTT_Src))
		style->GetSpanRef()->UpdateRefNormalized( inLC);

	VString textSpanRef = style->GetSpanRef()->GetDisplayValue(); 
	style->SetRange(0, textSpanRef.GetLength()); 
	VTreeTextStyle *styles = new VTreeTextStyle( style);

	SetText( textSpanRef, styles, inStart, inEnd, inAutoAdjustRangeWithSpanRef, inSkipCheckRange, true, NULL, inUserData, inBeforeReplace, inAfterReplace);
	return true;
}


/** update span references: if a span ref is dirty, plain text is evaluated again & visible value is refreshed */ 
bool VDocContainerLayout::UpdateSpanRefs(sLONG inStart, sLONG inEnd, void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace, VDBLanguageContext *inLC)
{
	_NormalizeRange( inStart, inEnd);

	bool result = false;
	VDocLayoutChildrenReverseIterator it(static_cast<VDocBaseLayout*>(this)); //reverse iteration (in order to not screw up text indexs because of replacement)
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && textLayout->GetTextLength())
		{
			if (inBeforeReplace && inAfterReplace)
			{
				//install local delegate in order to map/unmap text indexs from/to local
				fReplaceTextUserData = inUserData;
				fReplaceTextBefore = inBeforeReplace;
				fReplaceTextAfter = inAfterReplace;
				fReplaceTextCurLayout = layout;

				result = textLayout->UpdateSpanRefs( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart(), this, _BeforeReplace, _AfterReplace, inLC) || result;
			}
			else
				result = textLayout->UpdateSpanRefs( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart(), NULL, NULL, NULL, inLC) || result;
		}

		layout = it.Next();
	}
	if (result)
		SetDirty();
	return result;
}

/** eval 4D expressions  
@remarks
	note that it only updates span 4D expression references but does not update plain text:
	call UpdateSpanRefs after to update layout plain text

	return true if any 4D expression has been evaluated
*/
bool VDocContainerLayout::EvalExpressions( VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, VDocCache4DExp *inCache4DExp)
{
	_NormalizeRange( inStart, inEnd);

	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && layout->GetTextLength())
			result = textLayout->EvalExpressions( inLC, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart(), inCache4DExp) || result;

		layout = it.Next();
	}
	return result;
}


/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
bool VDocContainerLayout::FreezeExpressions(VDBLanguageContext *inLC, sLONG inStart, sLONG inEnd, void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{
	_NormalizeRange( inStart, inEnd);

	bool result = false;
	VDocLayoutChildrenReverseIterator it(static_cast<VDocBaseLayout*>(this)); //reverse iteration (in order to not screw up text indexs because of replacement)
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && layout->GetTextLength())
		{
			if (inBeforeReplace && inAfterReplace)
			{
				//install local delegate in order to map/unmap text indexs from/to local
				fReplaceTextUserData = inUserData;
				fReplaceTextBefore = inBeforeReplace;
				fReplaceTextAfter = inAfterReplace;
				fReplaceTextCurLayout = layout;

				result = textLayout->FreezeExpressions( inLC, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart(), this, _BeforeReplace, _AfterReplace) || result;
			}
			else
				result = textLayout->FreezeExpressions( inLC, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart(), NULL, NULL, NULL) || result;
		}

		layout = it.Next();
	}
	if (result)
		SetDirty();
	return result;
}


/** set extra text styles 
@remarks
	it can be used to add a temporary style effect to the text layout without modifying the main text styles
	(for instance to add a text selection effect: see VStyledTextEditView)

	if inCopyStyles == true, styles are copied
	if inCopyStyles == false (default), styles are retained: in that case, if you modify passed styles you should call this method again 
*/
void VDocContainerLayout::SetExtStyles( VTreeTextStyle *inStyles, bool /*inCopyStyles*/)
{
	//TODO: should be obsolete -> for document layout classes, it is used only to show automatic urls -> it would be better to render automatic urls as volatile url span references 
	//		directly in VDocParagraphLayout

	if (!testAssert(!inStyles || !inStyles->HasSpanRefs())) //span references are forbidden in extended styles
		return;

	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && layout->GetTextLength())
		{
			VTreeTextStyle *styles = inStyles ? inStyles->CreateTreeTextStyleOnRange( layout->GetTextStart(), layout->GetTextStart()+layout->GetTextLength()) : NULL;
			
			if (styles && !styles->IsUndefined(true))
				textLayout->SetExtStyles( styles);
			else
				textLayout->SetExtStyles( NULL);

			ReleaseRefCountable(&styles);
		}
		layout = it.Next();
	}
}

VTreeTextStyle* VDocContainerLayout::RetainExtStyles() const
{
	VTreeTextStyle *stylesLocal = NULL;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && (textLayout->GetTextLength() || GetTextLength() == 0))
		{
			VTreeTextStyle *styles = textLayout->RetainExtStyles();
			if (styles)
			{
				styles->Translate( textLayout->GetTextStart());
				if (!stylesLocal)
				{
					stylesLocal = new VTreeTextStyle( new VTextStyle());
					stylesLocal->GetData()->SetRange( 0, GetTextLength());
				}
				if (styles->HasSpanRefs())
					stylesLocal->ApplyStyles( styles, false); //in order span references to be attached to stylesLocal
				else
					stylesLocal->AddChild( styles); //can add styles directly as children layout styles do not overlap
				styles->Release();
			}
		}
		layout = it.Next();
	}
	return stylesLocal;
}


/** insert text at the specified position */
void VDocContainerLayout::InsertPlainText( sLONG inPos, const VString& inText)
{
	if (!inText.GetLength())
		return;

	SetText( inText, NULL, inPos, inPos);
}


/** replace text */
void VDocContainerLayout::ReplacePlainText( sLONG inStart, sLONG inEnd, const VString& inText)
{
	SetText( inText, NULL, inStart, inEnd);
}


/** delete text range */
void VDocContainerLayout::DeletePlainText( sLONG inStart, sLONG inEnd)
{
	SetText( CVSTR(""), NULL, inStart, inEnd);
}



bool VDocContainerLayout::ApplyStyles( const VTreeTextStyle *inStyles, bool /*inFast*/)
{
	if (!inStyles)
		return false;

	sLONG start, end;
	inStyles->GetData()->GetRange(start, end);
	_NormalizeRange(start, end);
	if (start > end)
		return false;
	if (GetTextLength() > 0)
		if (start == end)
			return false;

	bool result = false;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(start, end);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			VTreeTextStyle *styles = inStyles->CreateTreeTextStyleOnRange( layout->GetTextStart(), layout->GetTextStart()+layout->GetTextLength(), true, layout->GetTextLength() > 0);
			if (styles)
				result = textLayout->ApplyStyles( styles) || result;
			ReleaseRefCountable(&styles);
		}
		layout = it.Next();
	}
	if (result)
		SetDirty();
	return result;
}



/** apply style (use style range) */
bool VDocContainerLayout::ApplyStyle( const VTextStyle* inStyle, bool inFast, bool inIgnoreIfEmpty)
{
	if (!inStyle)
		return false;

	sLONG start, end;
	inStyle->GetRange(start, end);
	_NormalizeRange(start, end);
	if (start > end || inStyle->IsUndefined())  
		return false;
	if (inIgnoreIfEmpty && GetTextLength() > 0)
		if (start == end)
			return false;

	bool result = false;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(start, end);
	VTextStyle *style = new VTextStyle(inStyle);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			if (textLayout->GetTextLength() == 0) //special case if text is empty
			{
				xbox_assert(!fParent || fChildIndex == fParent->GetChildCount()-1); //can be only last node
				if (start == end && start == 0 && (!fParent || fChildIndex == 0))
					result = textLayout->ApplyStyle( inStyle, inFast, inIgnoreIfEmpty) || result; 
			}
			else
			{
				style->SetRange( std::max((sLONG)0,start-textLayout->GetTextStart()), std::min((sLONG)textLayout->GetTextLength(),end-textLayout->GetTextStart()));
				result = textLayout->ApplyStyle( style, inFast, inIgnoreIfEmpty) || result;
			}
		}
		layout = it.Next();
	}
	delete style;
	if (result)
		SetDirty();
	return result;
}



/** consolidate end of lines
@remarks
	if text is multiline:
		we convert end of lines to a single CR 
	if text is singleline:
		if inEscaping, we escape line endings otherwise we consolidate line endings with whitespaces (default)
		if inEscaping, we escape line endings and tab 
*/
bool VDocContainerLayout::ConsolidateEndOfLines(bool inIsMultiLine, bool inEscaping, sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	if (!inIsMultiLine)
	{
		//not multiline -> concat all text & styles on range with the first paragraph on range
		VString text;
		GetText(text, inStart, inEnd);
		VTreeTextStyle *styles = RetainStyles(inStart, inEnd);

		VString outText;
		if (ITextLayout::ConsolidateEndOfLines( text, outText, styles, inIsMultiLine, inEscaping))
		{
			SetText( outText, styles, inStart, inEnd);
			ReleaseRefCountable(&styles);
			return true;
		}
		ReleaseRefCountable(&styles);
		return false;
	}
	//multiline

	bool result = false;

	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout) && textLayout->GetTextLength())
			result = textLayout->ConsolidateEndOfLines( inIsMultiLine, inEscaping, inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart()) || result;
		layout = it.Next();
	}
	if (result)
		SetDirty();
	return result;
}


/**	get word (or space - if not trimming spaces - or punctuation - if not trimming punctuation) at the specified character index */
void VDocContainerLayout::GetWordRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inTrimSpaces, bool inTrimPunctuation)
{
	if (inOffset < 0)
		inOffset = 0;
	if (inOffset > GetTextLength())
		inOffset = GetTextLength();

	sLONG inCharStartIndex = 0;
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inOffset, &inCharStartIndex);
	if (paraLayout)
	{
		paraLayout->GetWordRange( inIntlMgr, inOffset-inCharStartIndex, outStart, outEnd, inTrimSpaces, inTrimPunctuation);
		outStart += inCharStartIndex;
		outEnd += inCharStartIndex;
	}
	else
		outStart = outEnd = 0;
}

/**	get paragraph range at the specified character index 
@remarks
	if inUseDocParagraphRange == true (default is false), return document paragraph range (which might contain one or more CRs) otherwise return actual paragraph range (actual line with one terminating CR only)
	(for VTextLayout, parameter is ignored)
*/
void VDocContainerLayout::GetParagraphRange( VIntlMgr *inIntlMgr, sLONG inOffset, sLONG& outStart, sLONG& outEnd, bool inUseDocParagraphRange)
{
	if (inOffset < 0)
		inOffset = 0;
	if (inOffset > GetTextLength())
		inOffset = GetTextLength();

	sLONG inCharStartIndex = 0;
	VDocParagraphLayout *paraLayout = _GetParagraphFromCharIndex( inOffset, &inCharStartIndex);
	if (paraLayout)
	{
		paraLayout->GetParagraphRange( inIntlMgr, inOffset-inCharStartIndex, outStart, outEnd, inUseDocParagraphRange);
		outStart += inCharStartIndex;
		outEnd += inCharStartIndex;
	}
	else
		outStart = outEnd = 0;
}

/** return true if layout uses truetype font only */
bool VDocContainerLayout::UseFontTrueTypeOnly(VGraphicContext *inGC) const
{
	if (inGC->IsQuartz2DImpl())
		return true;

	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First();
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
		{
			if (!textLayout->UseFontTrueTypeOnly(inGC))
				return false;
		}
		layout = it.Next();
	}
	return true;
}

/** call this method to update span reference style while mouse enters span ref 
@remarks
	return true if style has changed

	you should pass here a reference returned by GetStyleRefAtPos or GetStyleRefAtRange 
	(method is safe even if reference has been destroyed - i.e. if inStyleSpanRef is no more valid)
*/
bool VDocContainerLayout::NotifyMouseEnterStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart)
{
	if (!inStyleSpanRef)
		return false;
	if (inBaseStart < 0)
		inBaseStart = 0;
	if (inBaseStart > GetTextLength())
		inBaseStart = GetTextLength();

	sLONG start = 0, end = -1;
	try
	{
		inStyleSpanRef->GetRange( start, end);
		start += inBaseStart;
		end += inBaseStart;
	}
	catch(...)
	{
		xbox_assert(false);
		return NotifyMouseLeaveStyleSpanRef(NULL); //force to reset mouse over status
	}

	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(start, end);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			result = textLayout->NotifyMouseEnterStyleSpanRef( inStyleSpanRef, inBaseStart-textLayout->GetTextStart()) || result;
		layout = it.Next();
	}
	return result;
}

/** call this method to update span reference style while mouse leaves span ref 
@remarks
	return true if style has changed

	you should pass here a reference returned by GetStyleRefAtPos or GetStyleRefAtRange
	(method is safe even if reference has been destroyed - i.e. if inStyleSpanRef is no more valid)
*/
bool VDocContainerLayout::NotifyMouseLeaveStyleSpanRef( const VTextStyle *inStyleSpanRef, sLONG inBaseStart)
{
	if (inBaseStart < 0)
		inBaseStart = 0;
	if (inBaseStart > GetTextLength())
		inBaseStart = GetTextLength();

	sLONG start = 0, end = -1;
	if (inStyleSpanRef)
	{
		try
		{
			inStyleSpanRef->GetRange( start, end);
			start += inBaseStart;
			end += inBaseStart;
		}
		catch(...)
		{
			xbox_assert(false);
			inStyleSpanRef = NULL;
		}
	}

	bool result = false;
	VDocLayoutChildrenIterator it(static_cast<const VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(start, end);
	while (layout)
	{
		ITextLayout *textLayout = layout->QueryTextLayoutInterface();
		if (testAssert(textLayout))
			result = textLayout->NotifyMouseLeaveStyleSpanRef( inStyleSpanRef, inBaseStart-textLayout->GetTextStart()) || result;
		layout = it.Next();
	}
	return result;
}

/////////// VDocTextLayout


/** create and retain document from the current layout  
@param inStart
	range start
@param inEnd
	range end

@remarks
	if current layout is not a document layout, initialize a document from the top-level layout class instance if it exists (otherwise a default document)
	with a single child node initialized from the current layout
*/
VDocText* VDocTextLayout::CreateDocument(sLONG inStart, sLONG inEnd)
{
	_NormalizeRange( inStart, inEnd);

	VDocText *doc = new VDocText();
	if (!doc)
		return NULL;
	SetPropsTo( static_cast<VDocNode *>(doc));
	
	VDocLayoutChildrenIterator it(static_cast<VDocBaseLayout*>(this));
	VDocBaseLayout *layout = it.First(inStart, inEnd);
	while (layout)
	{
		if (layout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH) 
		{
			ITextLayout *textLayout = layout->QueryTextLayoutInterface();
			if (testAssert(textLayout))
			{
				VDocParagraph *para = textLayout->CreateParagraph( inStart-textLayout->GetTextStart(), inEnd-textLayout->GetTextStart());
				if (para)
				{
					doc->AddChild( para);
					para->Release();
				}
			}
		}
		else if (layout->GetDocType() == kDOC_NODE_TYPE_DIV)
		{
			//TODO
		}
		else
			xbox_assert(false);
		layout = it.Next();
	}
	return doc;
}

/** init layout from the passed document 
@param inDocument
	document source
@remarks
	the passed document is used only to initialize layout: document is not retained
	//TODO: maybe we should optionally bind layout with the document class instance
*/
void VDocTextLayout::InitFromDocument( const VDocText *inDocument)
{
	_InitFromNode(static_cast<const VDocNode *>(inDocument));
	SetDirty();
}

/** replace layout range with the specified document fragment */
void VDocTextLayout::ReplaceDocument(	const VDocText *inDoc, sLONG inStart, sLONG inEnd,
										bool inAutoAdjustRangeWithSpanRef, bool inSkipCheckRange, bool inInheritUniformStyle, 
										void *inUserData, VTreeTextStyle::fnBeforeReplace inBeforeReplace, VTreeTextStyle::fnAfterReplace inAfterReplace)
{ 
	_ReplaceFromNode(	inDoc, inStart, inEnd, 
						inAutoAdjustRangeWithSpanRef, inSkipCheckRange, inInheritUniformStyle, 
						inUserData, inBeforeReplace, inAfterReplace);
	SetDirty();
}

/** set node properties from the current properties */
void VDocTextLayout::SetPropsTo( VDocNode *outNode)
{
	VDocText *docText = dynamic_cast<VDocText *>(outNode);
	xbox_assert(docText);
	docText->SetVersion( kDOC_VERSION_XHTML4D);

	VDocContainerLayout::SetPropsTo( outNode);
}

#endif
