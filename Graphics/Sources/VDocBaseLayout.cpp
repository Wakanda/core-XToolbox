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
#include "VDocTextLayout.h"

#undef min
#undef max

VString	IDocBaseLayout::sEmptyString;

VDocBaseLayout::VDocBaseLayout():VObject(), IRefCountable()
{
	fDoc = NULL;
	fID = VString(D4ID_PREFIX_RESERVED)+"1";

	fMarginInTWIPS.left = fMarginInTWIPS.right = fMarginInTWIPS.top = fMarginInTWIPS.bottom = 0;
	fMarginOutTWIPS.left = fMarginOutTWIPS.right = fMarginOutTWIPS.top = fMarginOutTWIPS.bottom = 0;
	fMarginOut.left = fMarginOut.right = fMarginOut.top = fMarginOut.bottom = 0;
	fMarginIn.left = fMarginIn.right = fMarginIn.top = fMarginIn.bottom = 0;
	fAllMargin.left = fAllMargin.right = fAllMargin.top = fAllMargin.bottom = 0;
	fBorderWidth.left = fBorderWidth.right = fBorderWidth.top = fBorderWidth.bottom = 0;
	fBorderLayout = NULL;

	fWidthTWIPS = fHeightTWIPS = fMinWidthTWIPS = fMinHeightTWIPS = 0; //automatic on default

	fTextAlign = fVerticalAlign = AL_DEFAULT; //default

	fBackgroundColor = VColor(0,0,0,0); //transparent

	fBackgroundClip		= kDOC_BACKGROUND_BORDER_BOX;
	fBackgroundOrigin	= kDOC_BACKGROUND_PADDING_BOX;
	fBackgroundImageLayout	= NULL;

	fDPI = fRenderDPI = 72;

	fTextStart = 0;
	fTextLength = 1;
	fParent = NULL;
	fEmbeddingParent = NULL;
	fChildIndex = 0;

	fNoUpdateTextInfo = false;
	fNoDeltaTextLength = false;
}

VDocBaseLayout::~VDocBaseLayout()
{
	if (fBackgroundImageLayout)
		delete fBackgroundImageLayout;
	if (fBorderLayout)
		delete fBorderLayout;
}

void VDocBaseLayout::_GenerateID(VString& outID)
{
	uLONG id = 0;
	if (fDoc) //if detached -> id is irrelevant 
			 //(id searching is done only from a VDocText 
			 // & ids are generated on node attach event to ensure new attached nodes use unique ids)
		id = fDoc->_AllocID();
	outID.FromHexLong((uLONG8)id, false); //not pretty formating to make string smallest possible (1-8 characters for a uLONG)
	outID = VString(D4ID_PREFIX_RESERVED)+outID;
}

/** set current ID (custom ID) */
bool VDocBaseLayout::SetID(const VString& inValue) 
{	
	if (fID.EqualToString( inValue))
		return true;

	if (inValue.IsEmpty())
	{
		//remove custom id -> replace id with automatic id
		if (!fID.BeginsWith( D4ID_PREFIX_RESERVED))
		{
			if (fDoc)
			{
				//remove from document map of node per ID
				if (!fID.IsEmpty())
				{
					VDocTextLayout::MapOfLayoutPerID::iterator itDocNode = fDoc->fMapOfLayoutPerID.find(fID);
					if (itDocNode != fDoc->fMapOfLayoutPerID.end())
						fDoc->fMapOfLayoutPerID.erase(itDocNode);
				}

				//generate automatic ID
				_GenerateID( fID);

				//add to document map per ID
				if (static_cast<VDocBaseLayout *>(fDoc) != this)
					fDoc->fMapOfLayoutPerID[ fID] = this;
			}
			else
				_GenerateID(fID);
		}
		return true;
	}

	VDocBaseLayout *node = RetainLayout(inValue);
	if (node)
	{
		ReleaseRefCountable(&node);
		return false;
	}

	if (fDoc && !fID.IsEmpty())
	{
		VDocTextLayout::MapOfLayoutPerID::iterator itDocNode = fDoc->fMapOfLayoutPerID.find(fID);
		if (itDocNode != fDoc->fMapOfLayoutPerID.end())
			fDoc->fMapOfLayoutPerID.erase(itDocNode);
	}

	fID = inValue; 

	if (fDoc && static_cast<VDocBaseLayout *>(fDoc) != this)
		fDoc->fMapOfLayoutPerID[ fID] = this;
	return true;
}

void VDocBaseLayout::_OnAttachToDocument(VDocTextLayout *inDocLayout)
{
	if (fDoc)
		return;
	
	xbox_assert(inDocLayout && inDocLayout->GetDocType() == kDOC_NODE_TYPE_DOCUMENT);

	//update document reference
	fDoc = inDocLayout;
	xbox_assert(fDoc == inDocLayout);

	//update ID (to ensure this node & children nodes use unique IDs for this document)
	VDocBaseLayout *layout = fID.IsEmpty() ? static_cast<VDocBaseLayout *>(RetainRefCountable(fDoc)) : RetainLayout( fID);
	if (layout) //generate new ID only if ID is used yet
	{
		ReleaseRefCountable(&layout);
		_GenerateID(fID);
	}

	//update document map of node per ID
	fDoc->fMapOfLayoutPerID[fID] = this;

	//update children
	VectorOfChildren::iterator itLayout = fChildren.begin();
	for(;itLayout != fChildren.end(); itLayout++)
		itLayout->Get()->_OnAttachToDocument( inDocLayout);
}

void VDocBaseLayout::_OnDetachFromDocument(VDocTextLayout *inDocLayout)
{
	if (!fDoc)
		return;

	xbox_assert(inDocLayout && inDocLayout == fDoc);

	if (fDoc->fLayoutUserDelegate && fDoc->fLayoutUserDelegate->ShouldHandleLayout(this))
		fDoc->fLayoutUserDelegate->OnBeforeDetachFromDocument( this);

	fDoc = NULL;
	
	//remove from document map of node per ID
	VDocTextLayout::MapOfLayoutPerID::iterator itMapLayout = inDocLayout->fMapOfLayoutPerID.find(fID);
	if (itMapLayout != inDocLayout->fMapOfLayoutPerID.end())
		inDocLayout->fMapOfLayoutPerID.erase(itMapLayout);

	//update children
	VectorOfChildren::iterator itLayout = fChildren.begin();
	for(;itLayout != fChildren.end(); itLayout++)
		itLayout->Get()->_OnDetachFromDocument( inDocLayout);
}

/** accessor on class style manager */
VDocClassStyleManager*	VDocBaseLayout::GetClassStyleManager() const 
{ 
	return fDoc ? fDoc->GetClassStyleManager() : NULL; 
}

bool VDocBaseLayout::ApplyClass( const VString &inClass, bool inResetStyles, sLONG inStart, sLONG inEnd, eDocNodeType inTargetDocType)
{
	if (!testAssert(!inClass.IsEmpty()))
		return false;
	if (!GetClassStyleManager())
		return false;
	VDocClassStyle *cs = GetClassStyleManager()->RetainClassStyle(inTargetDocType, inClass);
	if (cs)
	{
		bool result = ApplyClassStyle( cs, inResetStyles, true, inStart, inEnd, inTargetDocType);
		ReleaseRefCountable(&cs);
		return result;
	}
	return false;
}

VDocClassStyle* VDocBaseLayout::RetainClassStyle( const VString& inClass, eDocNodeType inDocNodeType, bool inAppendDefaultStyles) const
{
	if (!GetClassStyleManager())
		return false;
	return GetClassStyleManager()->RetainClassStyle(inDocNodeType, inClass, inAppendDefaultStyles);
}

bool VDocBaseLayout::AddOrReplaceClassStyle( const VString& inClass, VDocClassStyle *inClassStyle, eDocNodeType inDocNodeType, bool inRemoveDefaultStyles)
{
	if (!fDoc)
		return false;
	if (!GetClassStyleManager())
		return false;

	GetClassStyleManager()->AddOrReplaceClassStyle( inDocNodeType, inClass, inClassStyle, inRemoveDefaultStyles);

	if (!inClass.IsEmpty())
	{
		//update all layout nodes which use this class
		VDocClassStyle *cs = GetClassStyleManager()->RetainClassStyle(inDocNodeType, inClass);
		if (cs)
		{
			fDoc->ApplyClassStyle( cs, true, false, 0, -1, inDocNodeType);
			ReleaseRefCountable(&cs);
		}
	}
	return true;
}


/** remove class style */
bool VDocBaseLayout::RemoveClassStyle( const VString& inClass, eDocNodeType inDocNodeType)
{
	if (fDoc)
		return fDoc->RemoveClassStyle( inClass, inDocNodeType);
	else
		return false;
}

/** clear class styles */
bool VDocBaseLayout::ClearClassStyles( bool inKeepDefaultStyles)
{
	if (fDoc)
		return fDoc->ClearClassStyles( inKeepDefaultStyles);
	else
		return false;
}

/** remove class */
bool VDocBaseLayout::_RemoveClass( const VString& inClass, eDocNodeType inDocNodeType)
{
	if (GetDocType() != inDocNodeType)
		return false;
	if (inClass != fClass)
		return false;
	fClass.SetEmpty();
	return true;
}

/** clear class */
bool VDocBaseLayout::_ClearClass()
{
	if (!fClass.IsEmpty())
	{
		fClass.SetEmpty();
		return true;
	}
	else
		return false;
}


/** retain layout node which matches the passed ID (will return always NULL if node is not attached to a VDocTextLayout) */
VDocBaseLayout* VDocBaseLayout::RetainLayout(const VString& inID)
{
	if (fDoc)
		return fDoc->RetainLayout( inID);
	else
		return NULL;
}


GReal VDocBaseLayout::RoundMetrics( VGraphicContext *inGC, GReal inValue)
{
	if (inGC && inGC->IsFontPixelSnappingEnabled())
	{
		//context aligns glyphs to pixel boundaries:
		//stick to nearest integral (as GDI does)
		//(note that as VDocParagraphLayout pixel is dependent on fixed fDPI, metrics need to be computed if fDPI is changed)
		if (inValue >= 0)
			return floor( inValue+0.5);
		else
			return ceil( inValue-0.5);
	}
	else
	{
		//context does not align glyphs on pixel boundaries (or it is not for text metrics - passing NULL as inGC) :
		//round to TWIPS precision (so result is multiple of 0.05) (it is as if metrics are computed at 72*20 = 1440 DPI which should achieve enough precision)
		if (inValue >= 0)
			return ICSSUtil::RoundTWIPS( inValue);
		else
			return -ICSSUtil::RoundTWIPS( -inValue);
	}
}

void VDocBaseLayout::_BeginLocalTransform( VGraphicContext *inGC, VAffineTransform& outCTM, const VPoint& inTopLeft, bool inDraw, const VPoint& inPreDecal)
{
	//we build transform to the target device coordinate space
	//if painting, we apply the transform to the passed gc & return in outCTM the previous transform which is restored in _EndLocalTransform
	//if not painting (get metrics), we return in outCTM the transform from computing dpi coord space to the ctm space but we do not modify the passed gc transform

	if (inDraw)
	{
		inGC->UseReversedAxis();
		inGC->GetTransform( outCTM);
	}

	VAffineTransform xform;
	VPoint origin( inTopLeft);

	//apply decal in local coordinate space
	xform.Translate(inPreDecal.GetX(), inPreDecal.GetY());

	//apply printer scaling if appropriate
	
	GReal scalex = 1, scaley = 1;
	if (inGC->HasPrinterScale() && inGC->ShouldScaleShapesWithPrinterScale()) 
	{
		inGC->GetPrinterScale( scalex, scaley);
		origin.x = origin.x*scalex; //if printing, top left position is scaled also
		origin.y = origin.y*scaley;
	}

	if (fRenderDPI != fDPI)
	{
		scalex *= fRenderDPI/fDPI;
		scaley *= fRenderDPI/fDPI;
	}

	if (scalex != 1 || scaley != 1)
		xform.Scale( scalex, scaley, VAffineTransform::MatrixOrderAppend);
	
	//stick (0,0) to inTopLeft

	xform.Translate( origin.GetX(), origin.GetY(), VAffineTransform::MatrixOrderAppend);
	
	//set new transform
	if (inDraw)
	{
		xform.Multiply( outCTM, VAffineTransform::MatrixOrderAppend);
		inGC->UseReversedAxis();
		inGC->SetTransform( xform);
	}
	else
		outCTM = xform;
}

void VDocBaseLayout::_EndLocalTransform( VGraphicContext *inGC, const VAffineTransform& inCTM, bool inDraw)
{
	if (!inDraw)
		return;
	inGC->UseReversedAxis();
	inGC->SetTransform( inCTM);
}

bool VDocBaseLayout::_ApplyVerticalAlign( VGraphicContext *inGC, VAffineTransform& outCTM, const VRect& inLayoutBounds, const VRect& inTypoBounds)
{
	if (inLayoutBounds.GetHeight() == inTypoBounds.GetHeight())
		return false;

	//apply vertical align offset
	GReal y = 0;
	switch (GetVerticalAlign())
	{
	case AL_BOTTOM:
		y = inLayoutBounds.GetHeight()-inTypoBounds.GetHeight(); 
		break;
	case AL_CENTER:
		y = inLayoutBounds.GetHeight()*0.5-inTypoBounds.GetHeight()*0.5;
		break;
	default:
		return false;
		break;
	}

	inGC->UseReversedAxis();
	inGC->GetTransform( outCTM);
	VAffineTransform ctm;
	ctm.SetTranslation( 0, y);
	inGC->ConcatTransform( ctm);

	return true;
}


void VDocBaseLayout::_BeginLayerOpacity(VGraphicContext *inGC, const GReal inOpacity)
{
	//called after _BeginLocalTransform so origin is aligned to element top left origin in fDPI coordinate space

	if (inOpacity == 1.0f)
		return;
	if (inGC->IsPrintingContext() && !inGC->IsQuartz2DImpl())
		//if printing, do not use layer but apply opacity per shape
		fOpacityBackup = inGC->SetTransparency( inOpacity);
	else
	{
		VRect bounds = _GetBackgroundBounds( inGC);
		inGC->BeginLayerTransparency( bounds, inOpacity);
	}
}

void VDocBaseLayout::_EndLayerOpacity(VGraphicContext *inGC, const GReal inOpacity)
{
	if (inOpacity == 1.0f)
		return;
	if (inGC->IsPrintingContext() && !inGC->IsQuartz2DImpl())
		//if printing, do not use layer but apply opacity per shape
		inGC->SetTransparency( fOpacityBackup);
	else
		inGC->EndLayer();
}

void VDocBaseLayout::_BeforeDraw( VGraphicContext *inGC)
{
	if (inGC->IsPrintingContext())
		return;
	if (fDoc && fDoc->GetLayoutUserDelegate() && fDoc->GetLayoutUserDelegate()->ShouldHandleLayout( this))
		fDoc->GetLayoutUserDelegate()->OnBeforeDraw( this, fDoc->fLayoutUserDelegateData, inGC, fLayoutBounds);
}

void VDocBaseLayout::_AfterDraw( VGraphicContext *inGC)
{
	if (inGC->IsPrintingContext())
		return;
	if (fDoc && fDoc->GetLayoutUserDelegate() && fDoc->GetLayoutUserDelegate()->ShouldHandleLayout( this))
		fDoc->GetLayoutUserDelegate()->OnAfterDraw( this, fDoc->fLayoutUserDelegateData, inGC, fLayoutBounds);
}


/** return true if it is the last child node
@param inLoopUp
	true: return true if and only if it is the last child node at this level and up to the top-level parent
	false: return true if it is the last child node relatively to the node parent
*/
bool VDocBaseLayout::IsLastChild(bool inLookUp) const
{
	if (!fParent)
		return true;
	if (fChildIndex != fParent->fChildren.size()-1)
		return false;
	if (!inLookUp)
		return true;
	return fParent->IsLastChild( true);
}


/** set node properties from the current properties */
void VDocBaseLayout::SetPropsTo( VDocNode *outNode)
{
	if (outNode->GetType() == GetDocType()) //we export ID & class if and only if outNode doc type is equal to the layout doc type
											//(otherwise outNode is probably a class style & we should not override class style ID or class)
	{
		outNode->SetID( fID);
		outNode->SetClass( fClass);
	}

	if (!(VDocProperty(fMarginOutTWIPS) == outNode->GetDefaultPropValue( kDOC_PROP_MARGIN)))
		outNode->SetMargin( fMarginOutTWIPS);

	if (!(VDocProperty(fMarginInTWIPS) == outNode->GetDefaultPropValue( kDOC_PROP_PADDING)))
		outNode->SetPadding( fMarginInTWIPS);

	if (fWidthTWIPS != 0)
		outNode->SetWidth( fWidthTWIPS);
	if (fMinWidthTWIPS != 0)
		outNode->SetMinWidth( fMinWidthTWIPS);

	if (fHeightTWIPS != 0)
		outNode->SetHeight( fHeightTWIPS);
	if (fMinHeightTWIPS != 0)
		outNode->SetMinHeight( fMinHeightTWIPS);

	//if (fTextAlign != AL_DEFAULT) 
		outNode->SetTextAlign( JustFromAlignStyle(fTextAlign)); //as property is inheritable, always set it
	if (fVerticalAlign != AL_DEFAULT)
		outNode->SetVerticalAlign( JustFromAlignStyle(fVerticalAlign));

	if (fBackgroundColor.GetRGBAColor() != RGBACOLOR_TRANSPARENT)
		outNode->SetBackgroundColor( fBackgroundColor.GetRGBAColor());

	if (!(VDocProperty((uLONG) fBackgroundClip) == outNode->GetDefaultPropValue( kDOC_PROP_BACKGROUND_CLIP)))
		outNode->SetBackgroundClip( fBackgroundClip);

	if (fBorderLayout)
		fBorderLayout->SetPropsTo( outNode);

	_SetBackgroundImagePropsTo( outNode);
}


/** initialize from the passed node */
void VDocBaseLayout::InitPropsFrom( const VDocNode *inNode, bool inIgnoreIfInheritedDefault, bool /*inClassStyleOverrideOnlyUniformCharStyles*/, VDocClassStyleManager *ioCSMgr)
{
	//special case for ID & class

	bool shouldFilterUndefined = inNode->GetType() == kDOC_NODE_TYPE_CLASS_STYLE && inNode->GetClass().IsEmpty(); //filter undefined values for border, margin, padding, background position or size (only if passed a class style with empty class name)

	if (inNode->GetType() == GetDocType())
		//we import id if and only if inNode document type is equal to layout document type
		//(otherwise inNode is probably a class style node and we should not import ID)
		SetID( inNode->GetID());

	if (!ioCSMgr && fDoc)
		ioCSMgr = fDoc->fClassStyleManager.Get();

	//for class, we do not import class name if inNode class is not valid
	VString sclass = inNode->GetClass();
	if (ioCSMgr && !sclass.IsEmpty())
	{
		VDocClassStyle* style = ioCSMgr->RetainClassStyle( GetDocType(), sclass);
		if (!style)
		{
			//we add new class style if and only if there is none yet with the same class name
			if (inNode->GetType() == kDOC_NODE_TYPE_CLASS_STYLE)
			{
				//inNode is a class style sheet yet -> copy it
				SetClass( sclass);
				const VDocClassStyle *styleConst = dynamic_cast<const VDocClassStyle *>(inNode);
				xbox_assert(styleConst);
				style = dynamic_cast<VDocClassStyle*>(styleConst->Clone());
				xbox_assert(style);
				ioCSMgr->AddOrReplaceClassStyle( GetDocType(), sclass, style);
			}
			else
			{
				style = inNode->GetDocumentNode() ? inNode->GetDocumentNode()->GetClassStyleManager()->RetainClassStyle( GetDocType(), sclass) : NULL;
				if (style)
				{
					//copy class stylesheet from inNode document
					SetClass( sclass);
					VDocClassStyle *styleClone = dynamic_cast<VDocClassStyle*>(style->Clone());
					xbox_assert(styleClone);
					ioCSMgr->AddOrReplaceClassStyle( GetDocType(), sclass, styleClone);
					ReleaseRefCountable(&styleClone);
				}
			}
		}
		else
		{
			//class style exists: we need to check if it is equal to the input document class style otherwise we should append a new class style
			VDocClassStyle *inStyle = inNode->GetDocumentNode() && inNode->GetDocumentNode()->GetClassStyleManager() != ioCSMgr ? inNode->GetDocumentNode()->GetClassStyleManager()->RetainClassStyle( GetDocType(), sclass) : NULL;
			if (inStyle)
			{
				if (*inStyle == *style)
					//classes are equal -> update class name
					SetClass(sclass);
				else
				{
					//same class name as input document class but classes are different -> generate new class name & append new class from input class
					uLONG id = 0;
					VString sNewClass;
					while (true)
					{
						id++;
						sNewClass.FromLong(id);
						sNewClass = sclass+sNewClass;
						VDocClassStyle *newstyle = ioCSMgr->RetainClassStyle( GetDocType(), sNewClass);
						if (!newstyle)
							break;
						ReleaseRefCountable(&newstyle);
					}
					SetClass( sNewClass);
					VDocClassStyle *styleClone = dynamic_cast<VDocClassStyle*>(inStyle->Clone());
					xbox_assert(styleClone);
					ioCSMgr->AddOrReplaceClassStyle( GetDocType(), sNewClass, styleClone);
					ReleaseRefCountable(&styleClone);
				}
				ReleaseRefCountable(&inStyle);
			}
			else if (inNode->GetDocumentNode() && inNode->GetDocumentNode()->GetClassStyleManager() == ioCSMgr)
				SetClass(sclass);
		}
		ReleaseRefCountable(&style);
	}

	//import other properties

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_MARGIN)))
	{
		if (shouldFilterUndefined)
		{
			sDocPropRect margin = fMarginOutTWIPS;
			if (inNode->GetMargin().left >= 0)
				margin.left = inNode->GetMargin().left;
			if (inNode->GetMargin().top >= 0)
				margin.top = inNode->GetMargin().top;
			if (inNode->GetMargin().right >= 0)
				margin.right = inNode->GetMargin().right;
			if (inNode->GetMargin().bottom >= 0)
				margin.bottom = inNode->GetMargin().bottom;
			SetMargins( margin);
		}
		else
			SetMargins( inNode->GetMargin());
	}

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_PADDING)))
	{
		if (shouldFilterUndefined)
		{
			sDocPropRect margin = fMarginInTWIPS;
			if (inNode->GetPadding().left >= 0)
				margin.left = inNode->GetPadding().left;
			if (inNode->GetPadding().top >= 0)
				margin.top = inNode->GetPadding().top;
			if (inNode->GetPadding().right >= 0)
				margin.right = inNode->GetPadding().right;
			if (inNode->GetPadding().bottom >= 0)
				margin.bottom = inNode->GetPadding().bottom;
			SetPadding( margin);
		}
		else
			SetPadding( inNode->GetPadding());
	}
	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_TEXT_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inNode->GetTextAlign();
		SetTextAlign( JustToAlignStyle(just));
	}

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_VERTICAL_ALIGN)))
	{
		eStyleJust just = (eStyleJust)inNode->GetVerticalAlign();
		SetVerticalAlign( JustToAlignStyle(just));
	}

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_BACKGROUND_COLOR)))
	{
		RGBAColor color = inNode->GetBackgroundColor();
		VColor xcolor;
		xcolor.FromRGBAColor( color);
		SetBackgroundColor( xcolor);
	}
	
	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_BACKGROUND_CLIP)))
		SetBackgroundClip( inNode->GetBackgroundClip());

	if (inIgnoreIfInheritedDefault && fBackgroundImageLayout)
	{
		//we need to ensure to keep background image properties which are not overriden if there is a back image yet
		//(because _CreateBackgroundImage uses only properties from passed node to create background image)
		if (inNode->IsOverriden( kDOC_PROP_BACKGROUND_IMAGE)
			||
			inNode->IsOverriden( kDOC_PROP_BACKGROUND_POSITION)
			||
			inNode->IsOverriden( kDOC_PROP_BACKGROUND_ORIGIN)
			||
			inNode->IsOverriden( kDOC_PROP_BACKGROUND_REPEAT)
			||
			inNode->IsOverriden( kDOC_PROP_BACKGROUND_SIZE)
			)
			{
				XBOX::VDocClassStyle *cs = new XBOX::VDocClassStyle();
				_SetBackgroundImagePropsTo( cs);
				if (shouldFilterUndefined)
				{
					if (inNode->IsOverriden( kDOC_PROP_BACKGROUND_IMAGE))
						cs->SetBackgroundImage( inNode->GetBackgroundImage());
					if (inNode->IsOverriden( kDOC_PROP_BACKGROUND_POSITION))
					{
						if (inNode->GetBackgroundImageHAlign() >= 0)
							cs->SetBackgroundImageHAlign( inNode->GetBackgroundImageHAlign());
						if (inNode->GetBackgroundImageVAlign() >= 0)
							cs->SetBackgroundImageVAlign( inNode->GetBackgroundImageVAlign());
					}
					if (inNode->IsOverriden( kDOC_PROP_BACKGROUND_ORIGIN))
						cs->SetBackgroundImageOrigin( inNode->GetBackgroundImageOrigin());
					if (inNode->IsOverriden( kDOC_PROP_BACKGROUND_REPEAT))
						cs->SetBackgroundImageRepeat( inNode->GetBackgroundImageRepeat());
					if (inNode->IsOverriden( kDOC_PROP_BACKGROUND_SIZE))
					{
						sLONG width = inNode->GetBackgroundImageWidth();
						sLONG height = inNode->GetBackgroundImageHeight();
						if (height == kDOC_BACKGROUND_SIZE_CONTAIN || height == kDOC_BACKGROUND_SIZE_COVER)
							width = height;

						if (width != kDOC_BACKGROUND_SIZE_UNDEFINED)
						{
							cs->SetBackgroundImageWidth( width);
							if (width == kDOC_BACKGROUND_SIZE_CONTAIN || width == kDOC_BACKGROUND_SIZE_COVER)
								height = kDOC_BACKGROUND_SIZE_AUTO;
						}
						if (height != kDOC_BACKGROUND_SIZE_UNDEFINED)
						{
							if (width == kDOC_BACKGROUND_SIZE_UNDEFINED && (cs->GetBackgroundImageWidth() == kDOC_BACKGROUND_SIZE_CONTAIN || cs->GetBackgroundImageWidth() == kDOC_BACKGROUND_SIZE_COVER))
								cs->SetBackgroundImageWidth( kDOC_BACKGROUND_SIZE_AUTO);
							cs->SetBackgroundImageHeight( height);
						}
					}
				}
				else
					cs->AppendPropsFrom( inNode, false, true);
				_CreateBackgroundImage( cs);
				XBOX::ReleaseRefCountable(&cs);
			}
	}
	else
		if (!inIgnoreIfInheritedDefault 
			|| 
			(!inNode->IsInheritedDefault( kDOC_PROP_BACKGROUND_IMAGE)))
			_CreateBackgroundImage( inNode);
	
	if (inIgnoreIfInheritedDefault && fBorderLayout)
	{
		//we need to ensure to keep border properties which are not overriden if there is a border layout yet
		//(because _CreateBorder uses only properties from passed node to create border)
		if (inNode->IsOverriden( kDOC_PROP_BORDER_STYLE)
			||
			inNode->IsOverriden( kDOC_PROP_BORDER_WIDTH)
			||
			inNode->IsOverriden( kDOC_PROP_BORDER_COLOR)
			||
			inNode->IsOverriden( kDOC_PROP_BORDER_RADIUS)
			)
			{
				XBOX::VDocClassStyle *cs = new XBOX::VDocClassStyle();
				fBorderLayout->SetPropsTo( cs);
				if (shouldFilterUndefined)
				{
					if (inNode->IsOverriden( kDOC_PROP_BORDER_STYLE))
					{
						sDocPropRect rect = cs->GetBorderStyle();
						sDocPropRect inRect = inNode->GetBorderStyle();
						if (inRect.left >= 0)
							rect.left = inRect.left;
						if (inRect.top >= 0)
							rect.top = inRect.top;
						if (inRect.right >= 0)
							rect.right = inRect.right;
						if (inRect.bottom >= 0)
							rect.bottom = inRect.bottom;
						cs->SetBorderStyle( rect);
					}
					if (inNode->IsOverriden( kDOC_PROP_BORDER_WIDTH))
					{
						sDocPropRect rect = cs->GetBorderWidth();
						sDocPropRect inRect = inNode->GetBorderWidth();
						if (inRect.left >= 0)
							rect.left = inRect.left;
						if (inRect.top >= 0)
							rect.top = inRect.top;
						if (inRect.right >= 0)
							rect.right = inRect.right;
						if (inRect.bottom >= 0)
							rect.bottom = inRect.bottom;
						cs->SetBorderWidth( rect);
					}
					if (inNode->IsOverriden( kDOC_PROP_BORDER_COLOR))
					{
						sDocPropRect rect = cs->GetBorderColor();
						sDocPropRect inRect = inNode->GetBorderColor();
						if (inRect.left != RGBACOLOR_UNDEFINED)
							rect.left = inRect.left;
						if (inRect.top != RGBACOLOR_UNDEFINED)
							rect.top = inRect.top;
						if (inRect.right != RGBACOLOR_UNDEFINED)
							rect.right = inRect.right;
						if (inRect.bottom != RGBACOLOR_UNDEFINED)
							rect.bottom = inRect.bottom;
						cs->SetBorderColor( rect);
					}
					if (inNode->IsOverriden( kDOC_PROP_BORDER_RADIUS))
						cs->SetBorderRadius( inNode->GetBorderRadius());
				}
				else
					cs->AppendPropsFrom( inNode, false, true);
				_CreateBorder( cs, true, inIgnoreIfInheritedDefault);
				XBOX::ReleaseRefCountable(&cs);
			}
	}
	else
		if (!inIgnoreIfInheritedDefault 
			|| 
			(!inNode->IsInheritedDefault( kDOC_PROP_BORDER_STYLE)))
			_CreateBorder( inNode, true, inIgnoreIfInheritedDefault);

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_WIDTH)))
		SetWidth( inNode->GetWidth());

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_MIN_WIDTH)))
		SetMinWidth( inNode->GetMinWidth());

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_HEIGHT)))
		SetHeight( inNode->GetHeight());

	if (!inIgnoreIfInheritedDefault 
		|| 
		(!inNode->IsInheritedDefault( kDOC_PROP_MIN_HEIGHT)))
		SetMinHeight( inNode->GetMinHeight());
}

/** create background image from passed document node */
void VDocBaseLayout::_CreateBackgroundImage(const VDocNode *inNode)
{
	if (fBackgroundImageLayout)
		delete fBackgroundImageLayout;
	fBackgroundImageLayout = NULL;

	VString url = inNode->GetBackgroundImage();
	if (!url.IsEmpty())
	{
		SetBackgroundOrigin( inNode->GetBackgroundImageOrigin());

		eStyleJust halign = inNode->GetBackgroundImageHAlign();
		if (halign < 0)
			halign = JST_Left;
		eStyleJust valign = inNode->GetBackgroundImageVAlign();
		if (valign < 0)
			valign = JST_Top;
				
		eDocPropBackgroundRepeat repeat = inNode->GetBackgroundImageRepeat();
	
		sLONG width = inNode->GetBackgroundImageWidth();
		if (width == kDOC_BACKGROUND_SIZE_UNDEFINED)
			width = kDOC_BACKGROUND_SIZE_AUTO;
		sLONG height = inNode->GetBackgroundImageHeight();
		if (height == kDOC_BACKGROUND_SIZE_UNDEFINED)
			height = kDOC_BACKGROUND_SIZE_AUTO;

		GReal widthPoint;
		bool isWidthPercentage = false;
		if (width == kDOC_BACKGROUND_SIZE_COVER || width == kDOC_BACKGROUND_SIZE_CONTAIN || width == kDOC_BACKGROUND_SIZE_AUTO)
			widthPoint = width;
		else if (height == kDOC_BACKGROUND_SIZE_COVER || height == kDOC_BACKGROUND_SIZE_CONTAIN)
			widthPoint = height;
		else if (width < 0) //percentage
		{
			isWidthPercentage = true;
			widthPoint = (-width)/100.0f; //remap 100% -> 1
		}
		else
			widthPoint = ICSSUtil::TWIPSToPoint(width);

		GReal heightPoint;
		bool isHeightPercentage = false;
		if (widthPoint == kDOC_BACKGROUND_SIZE_COVER || widthPoint == kDOC_BACKGROUND_SIZE_CONTAIN)
			heightPoint = kDOC_BACKGROUND_SIZE_AUTO;
		else
		{
			if (height == kDOC_BACKGROUND_SIZE_COVER || height == kDOC_BACKGROUND_SIZE_CONTAIN || height == kDOC_BACKGROUND_SIZE_AUTO)
				heightPoint = kDOC_BACKGROUND_SIZE_AUTO;
			else if (height < 0) //percentage
			{
				isHeightPercentage = true;
				heightPoint = (-height)/100.0f; //remap 100% -> 1
			}
			else
				heightPoint = ICSSUtil::TWIPSToPoint(height);
		}

		fBackgroundImageLayout = new VDocBackgroundImageLayout( url, halign, valign, repeat, widthPoint, heightPoint, isWidthPercentage, isHeightPercentage);
	}
}

/** set document node background image properties */
void VDocBaseLayout::_SetBackgroundImagePropsTo( VDocNode* outDocNode)
{
	if (fBackgroundImageLayout && !fBackgroundImageLayout->GetURL().IsEmpty())
	{
		outDocNode->SetBackgroundImage( fBackgroundImageLayout->GetURL());

		if (!(VDocProperty((uLONG) GetBackgroundOrigin()) == outDocNode->GetDefaultPropValue( kDOC_PROP_BACKGROUND_ORIGIN)))
			outDocNode->SetBackgroundImageOrigin( GetBackgroundOrigin());

		if (!(VDocProperty((uLONG)fBackgroundImageLayout->GetRepeat()) == outDocNode->GetDefaultPropValue( kDOC_PROP_BACKGROUND_REPEAT)))
			outDocNode->SetBackgroundImageRepeat( fBackgroundImageLayout->GetRepeat());

		if (fBackgroundImageLayout->GetHAlign() != JST_Left)
			outDocNode->SetBackgroundImageHAlign( fBackgroundImageLayout->GetHAlign());
		if (fBackgroundImageLayout->GetVAlign() != JST_Top)
			outDocNode->SetBackgroundImageVAlign( fBackgroundImageLayout->GetVAlign());
		
		if (fBackgroundImageLayout->GetWidth() != kDOC_BACKGROUND_SIZE_AUTO || fBackgroundImageLayout->GetHeight() != kDOC_BACKGROUND_SIZE_AUTO)
		{
			GReal widthPoint = fBackgroundImageLayout->GetWidth();
			sLONG width;
			if (widthPoint <= 0)
				width = widthPoint;
			else if (fBackgroundImageLayout->IsWidthPercentage())
			{
				width = floor((widthPoint*100)+0.5);
				if (width < 1)
					width = 1;
				else if (width > 1000)
					width = 1000;
				width = -width;
			}
			else
				width = ICSSUtil::PointToTWIPS(widthPoint);

			outDocNode->SetBackgroundImageWidth( width);

			GReal heightPoint = fBackgroundImageLayout->GetHeight();
			sLONG height;
			if (heightPoint <= 0)
				height = kDOC_BACKGROUND_SIZE_AUTO;
			else if (fBackgroundImageLayout->IsHeightPercentage())
			{
				height = floor((heightPoint*100)+0.5);
				if (height < 1)
					height = 1;
				else if (height > 1000)
					height = 1000;
				height = -height;
			}
			else
				height = ICSSUtil::PointToTWIPS(heightPoint);
			
			outDocNode->SetBackgroundImageHeight( height);
		}
	}
}

/** set background image 
@remarks
	see VDocBackgroundImageLayout constructor for parameters description */
void VDocBaseLayout::SetBackgroundImage(	const VString& inURL,	
											const eStyleJust inHAlign, const eStyleJust inVAlign, 
											const eDocPropBackgroundRepeat inRepeat,
											const GReal inWidth, const GReal inHeight,
											const bool inWidthIsPercentage,  const bool inHeightIsPercentage)
{
	if (inURL.IsEmpty() && !fBackgroundImageLayout)
		return;

	if (fBackgroundImageLayout)
		delete fBackgroundImageLayout;
	fBackgroundImageLayout = NULL;

	if (!inURL.IsEmpty())
		fBackgroundImageLayout = new VDocBackgroundImageLayout( inURL, inHAlign, inVAlign, inRepeat, inWidth, inHeight, inWidthIsPercentage, inHeightIsPercentage);
}

/** clear background image */
void VDocBaseLayout::ClearBackgroundImage()
{
	if (!fBackgroundImageLayout)
		return;

	delete fBackgroundImageLayout;
	fBackgroundImageLayout = NULL;
}


/** set layout DPI */
void VDocBaseLayout::SetDPI( const GReal inDPI)
{
	fRenderDPI = inDPI;

	if (fDPI == inDPI)
		return;
	fDPI = inDPI;

	sDocPropRect margin = fMarginOutTWIPS;
	fMarginOutTWIPS.left = fMarginOutTWIPS.right = fMarginOutTWIPS.top = fMarginOutTWIPS.bottom = 0;
	SetMargins( margin, false); //force recalc margin in px according to dpi

	margin = fMarginInTWIPS;
	fMarginInTWIPS.left = fMarginInTWIPS.right = fMarginInTWIPS.top = fMarginInTWIPS.bottom = 0;
	SetPadding( margin, false); //force recalc padding in px according to dpi

	_UpdateBorderWidth(false);
	_CalcAllMargin();
}

/** set outside margins	(in twips) - CSS margin */
void VDocBaseLayout::SetMargins(const sDocPropRect& inMargins, bool inCalcAllMargin)
{
	fMarginOutTWIPS = inMargins;
    
	fMarginOut.left		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.left) * fDPI/72);
	fMarginOut.right	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.right) * fDPI/72);
	fMarginOut.top		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.top) * fDPI/72);
	fMarginOut.bottom	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginOutTWIPS.bottom) * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

/** set inside margins	(in twips) - CSS padding */
void VDocBaseLayout::SetPadding(const sDocPropRect& inPadding, bool inCalcAllMargin)
{
	fMarginInTWIPS = inPadding;
    
	fMarginIn.left		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.left) * fDPI/72);
	fMarginIn.right		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.right) * fDPI/72);
	fMarginIn.top		= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.top) * fDPI/72);
	fMarginIn.bottom	= RoundMetrics(NULL,ICSSUtil::TWIPSToPoint( fMarginInTWIPS.bottom) * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

uLONG VDocBaseLayout::GetWidth(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins || !fWidthTWIPS)
		return	fWidthTWIPS;
	else
	{
		return	fWidthTWIPS+GetAllMarginHorizontalTWIPS();
	}
}

uLONG VDocBaseLayout::GetHeight(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins || !fHeightTWIPS)
		return	fHeightTWIPS;
	else
	{
		return	fHeightTWIPS+GetAllMarginVerticalTWIPS();
	}
}

uLONG VDocBaseLayout::GetMinWidth(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins)
		return	fMinWidthTWIPS;
	else
	{
		return	fMinWidthTWIPS+GetAllMarginHorizontalTWIPS();
	}
}

uLONG VDocBaseLayout::GetMinHeight(bool inIncludeAllMargins) const 
{ 
	if (!inIncludeAllMargins)
		return	fMinHeightTWIPS;
	else
	{
		return	fMinHeightTWIPS+GetAllMarginVerticalTWIPS();
	}
}

/** set background clipping - CSS background-clip */
void VDocBaseLayout::SetBackgroundClip( const eDocPropBackgroundBox inBackgroundClip) 
{ 
	fBackgroundClip = inBackgroundClip; 
	if (fBackgroundImageLayout)
		fBackgroundImageLayout->SetClipBounds( GetBackgroundBounds( fBackgroundClip));
}

/** set background origin - CSS background-origin */
void VDocBaseLayout::SetBackgroundOrigin( const eDocPropBackgroundBox inBackgroundOrigin)
{ 
	fBackgroundOrigin = inBackgroundOrigin; 
	if (fBackgroundImageLayout)
		fBackgroundImageLayout->SetOriginBounds( GetBackgroundBounds( fBackgroundOrigin));
}


/** get background bounds (in px - relative to fDPI) */
const VRect& VDocBaseLayout::GetBackgroundBounds(eDocPropBackgroundBox inBackgroundBox) const
{
	switch (inBackgroundBox)
	{
	case kDOC_BACKGROUND_BORDER_BOX:
		return fBorderBounds;
		break;
	case kDOC_BACKGROUND_PADDING_BOX:
		return fPaddingBounds;
		break;
	default:
		//kDOC_BACKGROUND_CONTENT_BOX
		return fContentBounds;
		break;
	}
	return fBorderBounds;
}

/** set borders 
@remarks
	if inLeft is NULL, left border is same as right border 
	if inBottom is NULL, bottom border is same as top border
	if inRight is NULL, right border is same as top border
	if inTop is NULL, top border is default border (solid, medium width & black color)
*/
void VDocBaseLayout::SetBorders( const sDocBorder *inTop, const sDocBorder *inRight, const sDocBorder *inBottom, const sDocBorder *inLeft)
{
	if (fBorderLayout)
		delete fBorderLayout;
	fBorderLayout = new VDocBorderLayout( inTop, inRight, inBottom, inLeft);
	_UpdateBorderWidth();
}

/** clear borders */
void VDocBaseLayout::ClearBorders()
{
	if (fBorderLayout)
		delete fBorderLayout;
	fBorderLayout = NULL;
	_UpdateBorderWidth();
}
					
const sDocBorderRect* VDocBaseLayout::GetBorders() const
{
	if (fBorderLayout)
		return &(fBorderLayout->GetBorders());
	else
		return NULL;
}

/** get background bounds (in px - relative to dpi set by SetDPI) 
@remarks
	unlike GetBackgroundBounds, if gc crispEdges is enabled & has a border, it will return pixel grid aligned bounds 
	(useful for proper clipping according to crispEdges status)
*/
const VRect& VDocBaseLayout::_GetBackgroundBounds(VGraphicContext *inGC, eDocPropBackgroundBox inBackgroundBox) const
{
	switch (inBackgroundBox)
	{
	case kDOC_BACKGROUND_BORDER_BOX:
		if (fBorderLayout && inGC->IsShapeCrispEdgesEnabled())
		{
			//we use border layout clip rectangle in order background painting or clipping to be aligned according to crisp edges status
			//(which is dependent on both drawing rect & line width)
			fBorderLayout->GetClipRect( inGC, fTempBorderBounds, VPoint(), true);
			return fTempBorderBounds;
		}
		else
			//if none borders or no crispEdges, we can paint or clip directly using border bounds
			return fBorderBounds;
		break;
	case kDOC_BACKGROUND_PADDING_BOX:
		if (fBorderLayout && inGC->IsShapeCrispEdgesEnabled())
		{
			//we use border layout clip rectangle in order background painting or clipping to be aligned according to crisp edges status
			//(which is dependent on both drawing rect & line width)
			fBorderLayout->GetClipRect( inGC, fTempPaddingBounds, VPoint(), false);
			return fTempPaddingBounds;
		}
		else
			//if none borders or no crispEdges, we can paint or clip directly using padding bounds
			return fPaddingBounds;
		break;
	default:
		//kDOC_BACKGROUND_CONTENT_BOX
		return fContentBounds;
		break;
	}
	return fBorderBounds;
}


void VDocBaseLayout::_UpdateBorderWidth(bool inCalcAllMargin)
{
	if (!fBorderLayout)
	{
		fBorderWidth.left = fBorderWidth.right = fBorderWidth.top = fBorderWidth.bottom = 0;
		if (inCalcAllMargin)
			_CalcAllMargin();
		return;
	}
	fBorderWidth.left	= RoundMetrics(NULL,fBorderLayout->GetLeftBorder().fWidth * fDPI/72);
	fBorderWidth.right	= RoundMetrics(NULL,fBorderLayout->GetRightBorder().fWidth * fDPI/72);
	fBorderWidth.top	= RoundMetrics(NULL,fBorderLayout->GetTopBorder().fWidth * fDPI/72);
	fBorderWidth.bottom	= RoundMetrics(NULL,fBorderLayout->GetBottomBorder().fWidth * fDPI/72);

	if (inCalcAllMargin)
		_CalcAllMargin();
}

void VDocBaseLayout::_CalcAllMargin()
{
	fAllMargin.left		= fMarginIn.left+fBorderWidth.left+fMarginOut.left;
	fAllMargin.right	= fMarginIn.right+fBorderWidth.right+fMarginOut.right;
	fAllMargin.top		= fMarginIn.top+fBorderWidth.top+fMarginOut.top;
	fAllMargin.bottom	= fMarginIn.bottom+fBorderWidth.bottom+fMarginOut.bottom;
}

void VDocBaseLayout::_SetLayoutBounds( VGraphicContext *inGC, const VRect& inBounds)
{
	fLayoutBounds = inBounds;
	fBorderBounds = inBounds;
	
	GReal width = fLayoutBounds.GetWidth()-fMarginOut.left-fMarginOut.right;
	if (width < 0) //might happen with GDI due to pixel snapping
		width = 0;
	GReal height = fLayoutBounds.GetHeight()-fMarginOut.top-fMarginOut.bottom;
	if (height < 0) //might happen with GDI due to pixel snapping
		height = 0;
	fBorderBounds.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left, fLayoutBounds.GetY()+fMarginOut.top, width, height);

	if (fBorderLayout)
	{
		//prepare border layout (border drawing rect is aligned on border center)

		width = width-fBorderWidth.left*0.5f-fBorderWidth.right*0.5;
		if (width < 0) //might happen with GDI due to pixel snapping
			width = 0;
		height = height-fBorderWidth.top*0.5f-fBorderWidth.bottom*0.5;
		if (height < 0) //might happen with GDI due to pixel snapping
			height = 0;
		VRect drawRect;
		drawRect.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left+fBorderWidth.left*0.5f, fLayoutBounds.GetY()+fMarginOut.top+fBorderWidth.top*0.5, width, height);

		fBorderLayout->SetDrawRect( drawRect, fDPI);

		width = fLayoutBounds.GetWidth()-fMarginOut.left-fMarginOut.right-fBorderWidth.left-fBorderWidth.right;
		if (width < 0) //might happen with GDI due to pixel snapping
			width = 0;
		height = fLayoutBounds.GetHeight()-fMarginOut.top-fMarginOut.bottom-fBorderWidth.top-fBorderWidth.bottom;
		if (height < 0) //might happen with GDI due to pixel snapping
			height = 0;
		fPaddingBounds.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left+fBorderWidth.left, fLayoutBounds.GetY()+fMarginOut.top+fBorderWidth.top, width, height);
	}
	else
		fPaddingBounds = fBorderBounds;

	width = width-fMarginIn.left-fMarginIn.right;
	if (width < 0) //might happen with GDI due to pixel snapping
		width = 0;
	height = height-fMarginIn.top-fMarginIn.bottom;
	if (height < 0) //might happen with GDI due to pixel snapping
		height = 0;
	fContentBounds.SetCoordsTo( fLayoutBounds.GetX()+fMarginOut.left+fBorderWidth.left+fMarginIn.left, fLayoutBounds.GetY()+fMarginOut.top+fBorderWidth.top+fMarginIn.top, width, height);

	if (fBackgroundImageLayout)
	{
		//set background image clip & origin bounds
	
		fBackgroundImageLayout->SetDPI( fDPI);
		fBackgroundImageLayout->SetClipBounds( _GetBackgroundBounds( inGC, fBackgroundClip));
		if (fBackgroundClip != fBackgroundOrigin)
			fBackgroundImageLayout->SetOriginBounds( _GetBackgroundBounds( inGC, fBackgroundOrigin));
		else
			fBackgroundImageLayout->SetOriginBounds( fBackgroundImageLayout->GetClipBounds());
		fBackgroundImageLayout->UpdateLayout( inGC);
	}
}

void VDocBaseLayout::_FinalizeTextLocalMetrics(	VGraphicContext *inGC, GReal typoWidth, GReal typoHeight, GReal layoutWidth, GReal layoutHeight, 
												const sRectMargin& inMargin, GReal inRenderMaxWidth, GReal inRenderMaxHeight, 
												VRect& outTypoBounds, VRect& outRenderTypoBounds, VRect &outRenderLayoutBounds, sRectMargin& outRenderMargin)
{
	GReal minWidth = fMinWidthTWIPS ? RoundMetrics(inGC,ICSSUtil::TWIPSToPoint(fMinWidthTWIPS)*fDPI/72) : 0;
	if (minWidth && layoutWidth < minWidth)
		layoutWidth = minWidth;
	typoWidth += inMargin.left+inMargin.right;
	layoutWidth += inMargin.left+inMargin.right;

	GReal minHeight = fMinHeightTWIPS ? RoundMetrics(inGC,ICSSUtil::TWIPSToPoint(fMinHeightTWIPS)*fDPI/72) : 0;
	if (minHeight && layoutHeight < minHeight)
		layoutHeight = minHeight;
	typoHeight += inMargin.top+inMargin.bottom;
	layoutHeight += inMargin.top+inMargin.bottom;

	//store final computing dpi metrics

	outTypoBounds.SetCoords( 0, 0, typoWidth, typoHeight);
	fLayoutBounds.SetCoords( 0, 0, layoutWidth, layoutHeight);

	//now update target dpi metrics

	if (fRenderDPI != fDPI)
	{
		typoWidth = typoWidth*fRenderDPI/fDPI;
		typoHeight = typoHeight*fRenderDPI/fDPI;

		layoutWidth = layoutWidth*fRenderDPI/fDPI;
		layoutHeight = layoutHeight*fRenderDPI/fDPI;

		outRenderMargin.left		= RoundMetrics(inGC,fAllMargin.left*fRenderDPI/fDPI); 
		outRenderMargin.right		= RoundMetrics(inGC,fAllMargin.right*fRenderDPI/fDPI); 
		outRenderMargin.top			= RoundMetrics(inGC,fAllMargin.top*fRenderDPI/fDPI); 
		outRenderMargin.bottom		= RoundMetrics(inGC,fAllMargin.bottom*fRenderDPI/fDPI); 
	}
	else
		outRenderMargin				= inMargin;

	outRenderTypoBounds.SetCoords( 0, 0, ceil(typoWidth), ceil(typoHeight));
	outRenderLayoutBounds.SetCoords( 0, 0, ceil(layoutWidth), ceil(layoutHeight));

	if (inRenderMaxWidth != 0.0f && outRenderTypoBounds.GetWidth() > inRenderMaxWidth)
		outRenderTypoBounds.SetWidth( std::floor(inRenderMaxWidth)); 
	if (inRenderMaxHeight != 0.0f && outRenderTypoBounds.GetHeight() > inRenderMaxHeight)
		outRenderTypoBounds.SetHeight( std::floor(inRenderMaxHeight)); 
	
	if (inRenderMaxWidth != 0.0f)
		outRenderLayoutBounds.SetWidth( std::floor(inRenderMaxWidth)); 
	
	if (inRenderMaxHeight != 0.0f)
		outRenderLayoutBounds.SetHeight( std::floor(inRenderMaxHeight)); 
}


/** create border from passed document node */
void VDocBaseLayout::_CreateBorder(const VDocNode *inDocNode, bool inCalcAllMargin, bool inNewBorderShouldUseBlackColor)
{
	if (fBorderLayout)
		delete fBorderLayout;
	fBorderLayout = NULL;
	if (inDocNode->IsOverriden(kDOC_PROP_BORDER_STYLE))
	{
		sDocPropRect borderStyle = inDocNode->GetBorderStyle();
		if (borderStyle.left > kDOC_BORDER_STYLE_HIDDEN
			||
			borderStyle.right > kDOC_BORDER_STYLE_HIDDEN
			||
			borderStyle.top > kDOC_BORDER_STYLE_HIDDEN
			||
			borderStyle.bottom > kDOC_BORDER_STYLE_HIDDEN)
		{
			if (inNewBorderShouldUseBlackColor)
			{
				sDocPropRect filter = { 0,0,0,0};
				if ((!fBorderLayout || fBorderLayout->GetLeftBorder().fStyle <= kDOC_BORDER_STYLE_HIDDEN) && borderStyle.left > kDOC_BORDER_STYLE_HIDDEN)
					filter.left = 1;
				if ((!fBorderLayout || fBorderLayout->GetRightBorder().fStyle <= kDOC_BORDER_STYLE_HIDDEN) && borderStyle.right > kDOC_BORDER_STYLE_HIDDEN)
					filter.right = 1;
				if ((!fBorderLayout || fBorderLayout->GetTopBorder().fStyle <= kDOC_BORDER_STYLE_HIDDEN) && borderStyle.top > kDOC_BORDER_STYLE_HIDDEN)
					filter.top = 1;
				if ((!fBorderLayout || fBorderLayout->GetBottomBorder().fStyle <= kDOC_BORDER_STYLE_HIDDEN) && borderStyle.bottom > kDOC_BORDER_STYLE_HIDDEN)
					filter.bottom = 1;

				fBorderLayout = new VDocBorderLayout(inDocNode);

				if (filter.left && fBorderLayout->fBorders.fLeft.fColor.GetAlpha() == 0)
					fBorderLayout->fBorders.fLeft.fColor = VColor(0,0,0);
				if (filter.right && fBorderLayout->fBorders.fRight.fColor.GetAlpha() == 0)
					fBorderLayout->fBorders.fRight.fColor = VColor(0,0,0);
				if (filter.top && fBorderLayout->fBorders.fTop.fColor.GetAlpha() == 0)
					fBorderLayout->fBorders.fTop.fColor = VColor(0,0,0);
				if (filter.bottom && fBorderLayout->fBorders.fBottom.fColor.GetAlpha() == 0)
					fBorderLayout->fBorders.fBottom.fColor = VColor(0,0,0);
			}
			else
				fBorderLayout = new VDocBorderLayout(inDocNode);
		}
	}
	_UpdateBorderWidth();
}


/** render layout element in the passed graphic context at the passed top left origin
@remarks		
	method does not save & restore gc context
*/
void VDocBaseLayout::Draw( VGraphicContext *inGC, const VPoint& inTopLeft, const GReal /*inOpacity*/) //opacity should be applied in derived class only
{
	if (fBackgroundColor.GetAlpha())
	{
		//background is clipped according to background clip box

		inGC->SetFillColor( fBackgroundColor);

		VRect bounds = _GetBackgroundBounds( inGC, fBackgroundClip);
		bounds.SetPosBy( inTopLeft.GetX(), inTopLeft.GetY());

		inGC->FillRect( bounds);
	}   

	//draw background image
	if (fBackgroundImageLayout)
		fBackgroundImageLayout->Draw( inGC, inTopLeft);

	//draw borders
	if (fBorderLayout && !fLayoutBounds.IsEmpty())
		fBorderLayout->Draw( inGC, inTopLeft);
}


/** make transform from source viewport to destination viewport
	(with uniform or non uniform scaling) */
void VDocBaseLayout::ComputeViewportTransform( VAffineTransform& outTransform, const VRect& _inRectSrc, const VRect& _inRectDst, uLONG inKeepAspectRatio) 
{
	VRect inRectSrc = _inRectSrc;
	VRect inRectDst = _inRectDst;

	//first translate coordinate space in order to set origin to (0,0)
	outTransform.MakeIdentity();
	outTransform.Translate( -inRectSrc.GetX(),
					  -inRectSrc.GetY(),
					  VAffineTransform::MatrixOrderAppend);
	
	//treat uniform scaling if appropriate
	if (inKeepAspectRatio != kKAR_NONE)
	{
		//compute new rectangle size without losing aspect ratio

		if (inKeepAspectRatio & kKAR_COVER)
		{
			//first case: cover

			if (inRectSrc.GetWidth() > inRectSrc.GetHeight())
			{
				//scale to meet viewport height
				Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
				inRectSrc.SetHeight( inRectDst.GetHeight());
				inRectSrc.SetWidth( scale*inRectSrc.GetHeight());

				if (inRectSrc.GetWidth() < inRectDst.GetWidth())
				{
					//scale to meet viewport width
					Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
					inRectSrc.SetWidth( inRectDst.GetWidth());
					inRectSrc.SetHeight( scale*inRectSrc.GetWidth());
				}
			}	
			else
			{
				//scale to meet viewport width
				Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
				inRectSrc.SetWidth( inRectDst.GetWidth());
				inRectSrc.SetHeight( scale*inRectSrc.GetWidth());

				if (inRectSrc.GetHeight() < inRectDst.GetHeight())
				{
					//scale to meet viewport height
					Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
					inRectSrc.SetHeight( inRectDst.GetHeight());
					inRectSrc.SetWidth( scale*inRectSrc.GetHeight());
				}
			}
		}
		else
			//second case: contain (default)
			if (inRectDst.GetWidth() > inRectDst.GetHeight())
			{
				if (inRectSrc.GetWidth() > inRectSrc.GetHeight())
				{
					Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
					inRectSrc.SetWidth( inRectDst.GetWidth());
					inRectSrc.SetHeight( scale*inRectSrc.GetWidth());

					if (inRectSrc.GetHeight() > inRectDst.GetHeight())
					{
						Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
						inRectSrc.SetHeight( inRectDst.GetHeight());
						inRectSrc.SetWidth( scale*inRectSrc.GetHeight());
					}
				}	
				else
				{
					Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
					inRectSrc.SetHeight( inRectDst.GetHeight());
					inRectSrc.SetWidth( scale*inRectSrc.GetHeight());
				}
			}	
			else
			{
				if (inRectSrc.GetWidth() > inRectSrc.GetHeight())
				{
					Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
					inRectSrc.SetWidth( inRectDst.GetWidth());
					inRectSrc.SetHeight( scale*inRectSrc.GetWidth());
				}	
				else
				{
					Real scale = inRectSrc.GetWidth()/inRectSrc.GetHeight();
					inRectSrc.SetHeight( inRectDst.GetHeight());
					inRectSrc.SetWidth( scale*inRectSrc.GetHeight());

					if (inRectSrc.GetWidth() > inRectDst.GetWidth())
					{
						Real scale = inRectSrc.GetHeight()/inRectSrc.GetWidth();
						inRectSrc.SetWidth( inRectDst.GetWidth());
						inRectSrc.SetHeight( scale*inRectSrc.GetWidth());
					}
				}
			}
		
		//scale coordinate space from initial source one to the modified one
		Real scaleX = inRectSrc.GetWidth()/_inRectSrc.GetWidth();
		Real scaleY = inRectSrc.GetHeight()/_inRectSrc.GetHeight();

		outTransform.Scale( scaleX, scaleY,
					  VAffineTransform::MatrixOrderAppend);
	
		//compute rectangle destination offset (alias the keep ratio decal)
		Real x = inRectDst.GetX();
		Real y = inRectDst.GetY();
		switch (inKeepAspectRatio & kKAR_XYDECAL_MASK)
		{
		case kKAR_XMINYMIN:
			break;
		case kKAR_XMINYMID:
			y += (inRectDst.GetHeight()-inRectSrc.GetHeight())/2;
			break;
		case kKAR_XMINYMAX:
			y += inRectDst.GetHeight()-inRectSrc.GetHeight();
			break;

		case kKAR_XMIDYMIN:	
			x += (inRectDst.GetWidth()-inRectSrc.GetWidth())/2;
			break;
		case kKAR_XMIDYMID:
			x += (inRectDst.GetWidth()-inRectSrc.GetWidth())/2;
			y += (inRectDst.GetHeight()-inRectSrc.GetHeight())/2;
		break;
		case kKAR_XMIDYMAX:
			x += (inRectDst.GetWidth()-inRectSrc.GetWidth())/2;
			y += inRectDst.GetHeight()-inRectSrc.GetHeight();
			break;
			
		case kKAR_XMAXYMIN:	
			x += inRectDst.GetWidth()-inRectSrc.GetWidth();
			break;
		case kKAR_XMAXYMID:	
			x += inRectDst.GetWidth()-inRectSrc.GetWidth();
			y += (inRectDst.GetHeight()-inRectSrc.GetHeight())/2;
			break;
		case kKAR_XMAXYMAX:
			x += inRectDst.GetWidth()-inRectSrc.GetWidth();
			y += inRectDst.GetHeight()-inRectSrc.GetHeight();
			break;
		}
		inRectDst.SetX( x);
		inRectDst.SetY( y);
	}	else
	{
		//non uniform scaling

		Real scaleX = inRectDst.GetWidth()/inRectSrc.GetWidth();
		Real scaleY = inRectDst.GetHeight()/inRectSrc.GetHeight();
		
		outTransform.Scale( scaleX, scaleY,
					  VAffineTransform::MatrixOrderAppend);
		
	}
	
	//finally translate in destination coordinate space
	outTransform.Translate( inRectDst.GetX(),
					  inRectDst.GetY(),
					  VAffineTransform::MatrixOrderAppend);
	
}


AlignStyle VDocBaseLayout::JustToAlignStyle( const eStyleJust inJust) 
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
	case JST_Baseline:
		return AL_BASELINE;
		break;
	case JST_Superscript:
		return AL_SUPER;
		break;
	case JST_Subscript:
		return AL_SUB;
		break;
	default:
		return AL_DEFAULT;
		break;
	}
}

eStyleJust VDocBaseLayout::JustFromAlignStyle( const AlignStyle inJust) 
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
	case AL_BASELINE:
		return JST_Baseline;
		break;
	case AL_SUPER:
		return JST_Superscript;
		break;
	case AL_SUB:
		return JST_Subscript;
		break;
	default:
		return JST_Default;
		break;
	}
}

void VDocBaseLayout::_UpdateTextInfo(bool inUpdateTextLength, bool inApplyOnlyDeltaTextLength, sLONG inDeltaTextLength)
{
	if (fParent)
	{
		xbox_assert(fParent->fChildren.size() > 0 && fParent->fChildren[fChildIndex].Get() == this);

		if (fChildIndex > 0)
		{
			VDocBaseLayout *prevNode = fParent->fChildren[fChildIndex-1].Get();
			fTextStart = prevNode->GetTextStart()+prevNode->GetTextLength();
		}
		else
			fTextStart = 0;
	}
	else
		fTextStart = 0;

	if (!inUpdateTextLength)
		return;

	if (fChildren.size())
	{
		if (inApplyOnlyDeltaTextLength && !fNoDeltaTextLength)
			fTextLength += inDeltaTextLength;
		else
		{
			fTextLength = 0;
			VectorOfChildren::const_iterator itNode = fChildren.begin();
			for(;itNode != fChildren.end(); itNode++)
				fTextLength += itNode->Get()->GetTextLength();
		}
	}
	else
		fTextLength = 0; //_UpdateTextInfo should be overriden by classes for which text length should not be 0
}

void VDocBaseLayout::_UpdateTextInfoEx(bool inUpdateNextSiblings, bool inUpdateTextLength, bool inApplyOnlyDeltaTextLength, sLONG inDeltaTextLength)
{
	if (fNoUpdateTextInfo)
		return;
	if (fParent && fParent->fNoUpdateTextInfo)
		return;

	sLONG deltaTextLength = (sLONG)fTextLength;
	_UpdateTextInfo(inUpdateTextLength, inApplyOnlyDeltaTextLength, inDeltaTextLength);
	deltaTextLength = ((sLONG)fTextLength)-deltaTextLength;

	if (fParent)
	{
		//update siblings after this node
		if (inUpdateNextSiblings)
		{
			VectorOfChildren::iterator itNode = fParent->fChildren.begin();
			std::advance( itNode, fChildIndex+1);
			for (;itNode != fParent->fChildren.end(); itNode++)
				(*itNode).Get()->_UpdateTextInfo(false);
		}

		fParent->_UpdateTextInfoEx( true, true, true, deltaTextLength);
	}
}

sLONG VDocBaseLayout::GetAllMarginHorizontalTWIPS() const
{
	return	fMarginOutTWIPS.left+fMarginOutTWIPS.right+fMarginInTWIPS.left+fMarginInTWIPS.right+
			(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetLeftBorder().fWidth+fBorderLayout->GetRightBorder().fWidth) : 0);
}

sLONG VDocBaseLayout::GetAllMarginVerticalTWIPS() const
{
	return	fMarginOutTWIPS.top+fMarginOutTWIPS.bottom+fMarginInTWIPS.top+fMarginInTWIPS.bottom+
			(fBorderLayout ? ICSSUtil::PointToTWIPS(fBorderLayout->GetTopBorder().fWidth+fBorderLayout->GetBottomBorder().fWidth) : 0);
}

sLONG VDocBaseLayout::_GetWidthTWIPS() const
{
	sLONG widthTWIPS = fWidthTWIPS;
	sLONG margin = 0;
	if (!widthTWIPS && fParent)
	{
		//if parent has a content width not NULL, determine width from parent content width minus local margin+padding+border
		//TODO: ignore if current layout is a a table cell layout

		VDocBaseLayout *parent = fParent;
		widthTWIPS = parent->fWidthTWIPS;
		margin = GetAllMarginHorizontalTWIPS();
		while (parent && !widthTWIPS)
		{
			if (parent->fParent)
				margin += parent->GetAllMarginHorizontalTWIPS();
			parent = parent->fParent;
			if (parent)
				widthTWIPS = parent->fWidthTWIPS;
		}
	}
	if (widthTWIPS)
	{
		widthTWIPS -= margin;
		if (widthTWIPS < fMinWidthTWIPS)
			widthTWIPS = fMinWidthTWIPS;
		if (widthTWIPS <= 0)
			widthTWIPS = 1;
	}
	return widthTWIPS;
}

sLONG VDocBaseLayout::_GetHeightTWIPS() const
{
	sLONG heightTWIPS = fHeightTWIPS;
	sLONG margin = 0;

	/* remark: automatic height should be always determined by formatted content and not by parent content height
	if (!heightTWIPS && fParent)
	{
		//if parent has a content height not NULL, determine height from parent content height minus local margin+padding+border

		VDocBaseLayout *parent = fParent;
		heightTWIPS = parent->fHeightTWIPS;
		margin = GetAllMarginVerticalTWIPS();
		while (parent && !heightTWIPS)
		{
			if (parent->fParent)
				margin += parent->GetAllMarginVerticalTWIPS();
			parent = parent->fParent;
			if (parent)
				heightTWIPS = parent->fHeightTWIPS;
		}
	}
	*/
	if (heightTWIPS)
	{
		heightTWIPS -=	margin;
		if (heightTWIPS < fMinHeightTWIPS)
			heightTWIPS = fMinHeightTWIPS;
		if (heightTWIPS <= 0)
			heightTWIPS = 1;
	}

	return heightTWIPS;
}

/** convert local char index to absolute character index (i.e. relative to start of document full text) */
void VDocBaseLayout::LocalToAbsoluteCharIndex( sLONG& ioCharIndex)
{
	VDocBaseLayout *layout = this;
	while(layout)
	{
		ioCharIndex += layout->GetTextStart();
		if (layout->fParent)
			layout = layout->fParent;
		else
			layout = layout->fEmbeddingParent;
	}
}

/** add child layout instance */
void VDocBaseLayout::AddChild( IDocBaseLayout* inLayout)
{
	xbox_assert(inLayout && (inLayout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH || inLayout->GetDocType() == kDOC_NODE_TYPE_DIV));

	VDocBaseLayout *layout = dynamic_cast<VDocBaseLayout*>(inLayout);
	if (testAssert(layout))
	{
		fChildren.push_back(VRefPtr<VDocBaseLayout>(layout));
		layout->fChildIndex = fChildren.size()-1;
		layout->fParent = this;
		if (fDoc)
			layout->_OnAttachToDocument( fDoc);
		fNoDeltaTextLength = true;
		layout->_UpdateTextInfoEx(false);
		fNoDeltaTextLength = false;

		layout->_UpdateListNumber( kDOC_LSTE_AFTER_ADD_TO_LIST);
	}
}

/** insert child layout instance */
void VDocBaseLayout::InsertChild( IDocBaseLayout* inLayout, sLONG inIndex)
{
	xbox_assert(inLayout && (inLayout->GetDocType() == kDOC_NODE_TYPE_PARAGRAPH || inLayout->GetDocType() == kDOC_NODE_TYPE_DIV));

	if (!(testAssert(inIndex >= 0 && inIndex <= fChildren.size())))
		return;
	if (inIndex >= fChildren.size())
	{
		AddChild( inLayout);
		return;
	}

	VDocBaseLayout *layout = dynamic_cast<VDocBaseLayout*>(inLayout);
	if (!testAssert(layout))
		return;

	VectorOfChildren::iterator itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	fChildren.insert(itChild, VRefPtr<VDocBaseLayout>(layout)); 
	layout->fParent = this;

	itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	for (; itChild != fChildren.end(); itChild++)
		itChild->Get()->fChildIndex = inIndex++;
	
	if (fDoc)
		layout->_OnAttachToDocument( fDoc);

	fNoDeltaTextLength = true;
	layout->_UpdateTextInfoEx(true);
	fNoDeltaTextLength = false;

	layout->_UpdateListNumber( kDOC_LSTE_AFTER_ADD_TO_LIST);
}

/** remove child layout instance */
void VDocBaseLayout::RemoveChild( sLONG inIndex)
{
	if (!(testAssert(inIndex >= 0 && inIndex < fChildren.size())))
		return;

	VectorOfChildren::iterator itChild = fChildren.begin();
	std::advance(itChild, inIndex);

	itChild->Get()->_UpdateListNumber( kDOC_LSTE_BEFORE_REMOVE_FROM_LIST);

	if (fDoc)
		itChild->Get()->_OnDetachFromDocument( fDoc);
	fChildren.erase(itChild);

	itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	for (; itChild != fChildren.end(); itChild++)
		itChild->Get()->fChildIndex = inIndex++;
	
	fNoDeltaTextLength = true;
	itChild = fChildren.begin();
	std::advance(itChild, inIndex);
	if (itChild != fChildren.end())
		itChild->Get()->_UpdateTextInfoEx(true);
	else
		_UpdateTextInfoEx(true);
	fNoDeltaTextLength = false;
}


/** retain first child layout instance which intersects the passed text index */
IDocBaseLayout*	VDocBaseLayout::GetFirstChild( sLONG inStart) const
{
	if (fChildren.size() == 0)
		return NULL;

	VDocBaseLayout *node = NULL;
	if (inStart <= 0)
		node = fChildren[0].Get();
	else if (inStart >= fTextLength)
		node = fChildren.back().Get();
	else //search dichotomically
	{
		sLONG start = 0;
		sLONG end = fChildren.size()-1;
		while (start < end)
		{
			sLONG mid = (start+end)/2;
			if (start < mid)
			{
				if (inStart < fChildren[mid].Get()->GetTextStart())
					end = mid-1;
				else
					start = mid;
			}
			else
			{ 
				xbox_assert(start == mid && end == start+1);
				if (inStart < fChildren[end].Get()->GetTextStart())
					node = fChildren[start].Get();
				else
					node = fChildren[end].Get();
				break;
			}
		}
		if (!node)
			node = fChildren[start].Get();
	}
	return static_cast<IDocBaseLayout*>(node);
}

/** return next sibling */
VDocBaseLayout* VDocBaseLayout::_GetNextSibling() const
{
	if (!fParent)
		return NULL;
	if (fChildIndex+1 < fParent->fChildren.size())
		return fParent->fChildren[fChildIndex+1].Get();
	return NULL; 	
}

/** query text layout interface (if available for this class instance) */
/*
ITextLayout* VDocBaseLayout::QueryTextLayoutInterface()
{
	VDocParagraphLayout *paraLayout = dynamic_cast<VDocParagraphLayout *>(this);
	if (paraLayout)
		return static_cast<ITextLayout *>(paraLayout);
#if ENABLE_VDOCTEXTLAYOUT
	else
	{
		VDocContainerLayout *contLayout = dynamic_cast<VDocContainerLayout *>(this);
		if (contLayout)
			return static_cast<ITextLayout *>(contLayout);
	}
#endif
	return NULL;
}
*/

void VDocBaseLayout::_AdjustComputingDPI(VGraphicContext *inGC, GReal& ioRenderMaxWidth, GReal& ioRenderMaxHeight)
{
	if (!fParent)
	{
		//we adjust computing dpi only for top-level layout class (children class instances inherit computing dpi from top-level computing dpi)

		if (inGC->IsFontPixelSnappingEnabled())
		{
			//for GDI, the best for now is still to compute GDI metrics at the target DPI:
			//need to find another way to not break wordwrap lines while changing DPI with GDI
			//(no problem with DWrite or CoreText)
			
			//if printing, compute metrics at printer dpi
			if (inGC->HasPrinterScale())
			{
				GReal scaleX, scaleY;
				inGC->GetPrinterScale( scaleX, scaleY);
				GReal dpi = fRenderDPI*scaleX;
				
				GReal targetDPI = fRenderDPI;
				if (fDPI != dpi)
					SetDPI( dpi);
				fRenderDPI = targetDPI;
			}
			else
				//otherwise compute at desired DPI
				if (fDPI != fRenderDPI)
					SetDPI( fRenderDPI);
		}
		else
		{
			//metrics are not snapped to pixel -> compute metrics at 72 DPI (with TWIPS precision) & scale context to target DPI (both DWrite & CoreText are scalable graphic contexts)
			if (fDPI != 72)
			{
				GReal targetDPI = fRenderDPI;
				SetDPI( 72);
				fRenderDPI = targetDPI;
			}
		}
	}
	if (fWidthTWIPS)
		ioRenderMaxWidth = RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetWidth(true))*fRenderDPI/72);
	if (fHeightTWIPS)
		ioRenderMaxHeight = RoundMetrics(NULL,ICSSUtil::TWIPSToPoint(GetHeight(true))*fRenderDPI/72);
}

VDocBaseLayout* VDocBaseLayout::_GetTopLevelParentLayout() const
{
	VDocBaseLayout *parent = fParent;
	if (!parent)
		return NULL;

	while (parent->fParent)
		parent = parent->fParent;
	return parent;
}


void VDocBaseLayout::_NormalizeRange(sLONG& ioStart, sLONG& ioEnd) const
{
	if (ioStart < 0)
		ioStart = 0;
	else if (ioStart > GetTextLength())
		ioStart = GetTextLength();

	if (ioEnd < 0)
		ioEnd = GetTextLength();
	else if (ioEnd > GetTextLength())
		ioEnd = GetTextLength();
	
	xbox_assert(ioStart <= ioEnd);
}


VDocBaseLayout*	VDocLayoutChildrenIterator::First(sLONG inTextStart, sLONG inTextEnd) 
{
	fEnd = inTextEnd;
	if (inTextStart > 0)
	{
		IDocBaseLayout *layout = fParent->GetFirstChild( inTextStart);
		if (layout)
		{
			fIterator = fParent->fChildren.begin();
			if (layout->GetChildIndex() > 0)
				std::advance(fIterator, layout->GetChildIndex());
		}
		else 
			fIterator = fParent->fChildren.end(); 
	}
	else
		fIterator = fParent->fChildren.begin(); 
	return Current(); 
}


VDocBaseLayout*	VDocLayoutChildrenIterator::Next() 
{  
	fIterator++; 
	if (fEnd >= 0 && fIterator != fParent->fChildren.end() && fEnd <= (*fIterator).Get()->GetTextStart()) //if child range start >= fEnd
	{
		if (fEnd < (*fIterator).Get()->GetTextStart() || (*fIterator).Get()->GetTextLength() != 0) //if child range is 0 and child range end == fEnd, we keep it otherwise we discard it and stop iteration
		{
			fIterator = fParent->fChildren.end();
			return NULL;
		}
	}
	return Current(); 
}


VDocBaseLayout*	VDocLayoutChildrenReverseIterator::First(sLONG inTextStart, sLONG inTextEnd) 
{
	fStart = inTextStart;
	if (inTextEnd < 0)
		inTextEnd = fParent->GetTextLength();

	if (inTextEnd < fParent->GetTextLength())
	{
		IDocBaseLayout *layout = fParent->GetFirstChild( inTextEnd);
		if (layout)
		{
			if (inTextEnd <= layout->GetTextStart() && layout->GetTextLength() > 0)
				layout = layout->GetChildIndex() > 0 ? fParent->fChildren[layout->GetChildIndex()-1].Get() : NULL;
			if (layout)
			{
				fIterator = fParent->fChildren.rbegin(); //point on last child
				if (layout->GetChildIndex() < fParent->fChildren.size()-1)
					std::advance(fIterator, fParent->fChildren.size()-1-layout->GetChildIndex());
			}
			else
				fIterator = fParent->fChildren.rend(); 
		}
		else 
			fIterator = fParent->fChildren.rend(); 
	}
	else
		fIterator = fParent->fChildren.rbegin(); 
	return Current(); 
}


VDocBaseLayout*	VDocLayoutChildrenReverseIterator::Next() 
{  
	fIterator++; 
	if (fStart > 0 && fIterator != fParent->fChildren.rend() && fStart >= (*fIterator).Get()->GetTextStart()+(*fIterator).Get()->GetTextLength()) 
	{
		fIterator = fParent->fChildren.rend();
		return NULL;
	}
	return Current(); 
}
