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
#include "VXMLPrecompiled.h"
#include "VCSSParser.h"
#include "VSpanText.h"
#include "XMLSaxParser.h"
#include "XML/VXML.h"

#include <../../xerces_for_4d/Xerces4DDom.hpp>
#include <../../xerces_for_4d/Xerces4DSax.hpp>
#include <../../xerces_for_4d/XML4DUtils.hpp>

BEGIN_TOOLBOX_NAMESPACE

#if VERSION_LINUX

static char* strlwr(char* string)
{
	if(string==NULL)
		return NULL;

	for(char* ptr=string ; *ptr!=0 ; ptr++)
		*ptr=(char)tolower(*ptr);

	return string;
}

#endif

static void JustToString(eStyleJust inValue, VString& outValue);
static void ColorToValue(const RGBAColor inColor, XBOX::VString& outValue);

/** map of default style per element name */
typedef std::map<std::string,VTreeTextStyle *> MapOfStylePerHTMLElement;

/** map of HTML element names with break line ending */
typedef std::set<std::string> SetOfHTMLElementWithBreakLineEnding;


/** return true if character is valid XML 1.1 
	(some control & special chars are not supported like [#x1-#x8], [#xB-#xC], [#xE-#x1F], [#x7F-#x84], [#x86-#x9F] - cf W3C) */
bool VSpanTextParser::IsXMLChar(UniChar inChar) const
{
	return xercesc::XMLChar1_1::isXMLChar( inChar);
}

/** convert plain text to XML 
@param ioText
	text to convert
@param inFixInvalidChars
	if true (default), fix invalid XML chars - see remarks below
@param inEscapeXMLMarkup
	if true (default), escape XML markup (<,>,',",&) 
@param inConvertCRtoBRTag
	if true (default is false), it will convert any end of line to <BR/> XML tag
@param inConvertForCSS
	if true, convert also "\" to "\\" because in CSS '\' is used for escaping characters  
@remarks
	see IsXMLChar 

	note that end of lines are normalized to one CR only (0x0D) - if end of lines are not converted to <BR/>

	if inFixInvalidChars == true, invalid chars are converted to whitespace (but for 0x0B which is replaced with 0x0D) 
	otherwise Xerces would fail parsing invalid XML 1.1 characters - parser would throw exception
*/
void VSpanTextParser::TextToXML(VString& ioText, bool inFixInvalidChars, bool inEscapeXMLMarkup, bool inConvertCRtoBRTag, bool inConvertForCSS, bool inEscapeCR)
{
	if (ioText.IsEmpty())
		return;

	VString text;
	text.EnsureSize(ioText.GetLength());

	VString sCRTag("<br/>");
	if (inEscapeCR)
	{
		inConvertCRtoBRTag = true;
		if (inConvertForCSS)
			sCRTag.FromCString("\\0D"); //CSS escaping
		else
			sCRTag.FromCString("&#xD;"); //XML escaping
	}

	ioText.ConvertCarriageReturns(XBOX::eCRM_CR); //normalize end of lines to CR

	const UniChar *c = ioText.GetCPointer();

	if (inConvertForCSS)
	{
		if (inEscapeXMLMarkup)
		{
			for(;*c;c++)
			{
				if (*c == '\\')
					text.AppendString(CVSTR("\\\\")); //in CSS, we need to escape plain text '\' as it is used in CSS for escaping...
				else if (*c == CHAR_AMPERSAND)
					text.AppendString(CVSTR("&amp;"));
				else if (*c == CHAR_GREATER_THAN_SIGN)
					text.AppendString(CVSTR("&gt;"));
				else if (*c == CHAR_QUOTATION_MARK)
					text.AppendString(CVSTR("&quot;"));
				else if (*c == CHAR_LESS_THAN_SIGN)
					text.AppendString(CVSTR("&lt;"));
				else if (*c == CHAR_APOSTROPHE)
					text.AppendString(CVSTR("&apos;"));
				else if (*c == 0x0B || *c == 0x0D)
				{
					if (inConvertCRtoBRTag)
						text.AppendString(sCRTag);
					else
						text.AppendUniChar(0x0D);
				}
				else if (inFixInvalidChars && !IsXMLChar(*c))
					text.AppendUniChar(' ');
				else
					text.AppendUniChar(*c);
			}
		}
		else
			for(;*c;c++)
			{
				if (*c == '\\')
					text.AppendString(CVSTR("\\\\")); //in CSS, we need to escape plain text '\' as it is used in CSS for escaping...
				else if (*c == 0x0B || *c == 0x0D)
				{
					if (inConvertCRtoBRTag)
						text.AppendString(sCRTag);
					else
						text.AppendUniChar(0x0D);
				}
				else if (inFixInvalidChars && !IsXMLChar(*c))
					text.AppendUniChar(' ');
				else
					text.AppendUniChar(*c);
			}
	}
	else if (inEscapeXMLMarkup)
	{
		for(;*c;c++)
		{
			if (*c == CHAR_AMPERSAND)
				text.AppendString(CVSTR("&amp;"));
			else if (*c == CHAR_GREATER_THAN_SIGN)
				text.AppendString(CVSTR("&gt;"));
			else if (*c == CHAR_QUOTATION_MARK)
				text.AppendString(CVSTR("&quot;"));
			else if (*c == CHAR_LESS_THAN_SIGN)
				text.AppendString(CVSTR("&lt;"));
			else if (*c == CHAR_APOSTROPHE)
				text.AppendString(CVSTR("&apos;"));
			else if (*c == 0x0B || *c == 0x0D)
			{
				if (inConvertCRtoBRTag)
					text.AppendString(sCRTag);
				else
					text.AppendUniChar(0x0D);
			}
			else if (inFixInvalidChars && !IsXMLChar(*c))
				text.AppendUniChar(' ');
			else
				text.AppendUniChar(*c);
		}
	}
	else
		for(;*c;c++)
		{
			if (*c == 0x0B || *c == 0x0D)
			{
				if (inConvertCRtoBRTag)
					text.AppendString(sCRTag);
				else
					text.AppendUniChar(0x0D);
			}
			else if (inFixInvalidChars && !IsXMLChar(*c))
				text.AppendUniChar(' ');
			else
				text.AppendUniChar(*c);
		}

	ioText = text;
}


class VSpanHandler : public VObject, public IXMLHandler
{
public:
	VSpanHandler(	VSpanHandler *inParent, const VString &inElementName, VTreeTextStyle *inStyleParent, 
					const VTextStyle *inStyleInherit, VString *ioText, uLONG inVersion, VDBLanguageContext *inLC = NULL);
	~VSpanHandler();

	virtual IXMLHandler*		StartElement(const VString& inElementName);
	virtual void				EndElement( const VString& inElementName);
	virtual void				SetAttribute(const VString& inName, const VString& inValue);
	virtual void				SetText( const VString& inText);

	void				SetHTMLAttribute(const VString& inName, const VString& inValue);

	/** set document node (node is retained) */
	void				SetDocNode( VDocNode *inDocNode)
	{
		fDocNode = inDocNode;
	}
	/** get and retain document node */
	VDocNode			*RetainDocNode()
	{
		if (fDocNode.Get())
			return RetainRefCountable( fDocNode.Get());
		else
			return NULL;
	}

	/** set parent document node */
	void SetParentDocNode( VDocNode *inDocNode) { fParentDocNode = inDocNode; }
	VDocNode *GetParentDocNode() { return fParentDocNode; }	

protected:
	void				_CreateStyles();
	const VTextStyle	*_GetStyleInheritForChildren();
	const VTextStyle	*_GetStyleInherit() { return (fStyleInherit ? fStyleInherit : (fStylesParent ? fStylesParent->GetData() : NULL)); }

	VString* fText;
	VSpanHandler *fSpanHandlerRoot;
	bool fShowSpanRef;
	bool fNoUpdateRef;
	VTreeTextStyle *fStyles;
	VTreeTextStyle *fStylesParent;
	VTreeTextStyle *fStylesRoot;
	VTextStyle *fStyleInherit;
	bool fStyleInheritForChildrenDone;
	VString fElementName;
	sLONG fStart;

	std::vector<VTreeTextStyle *> *fStylesSpanRefs;
	VIndex fDocNodeLevel;
	VDocNode *fParentDocNode;
	VRefPtr<VDocNode> fDocNode;
	bool fIsUnknownTag;
	bool fIsOpenTagClosed;
	bool fIsUnknownRootTag;
	VString fUnknownRootTagText;
	VString *fUnknownTagText;

	VDBLanguageContext *fDBLanguageContext;
};

class VParserXHTML : public VSpanHandler
{
public:
	/** XHTML or SPAN tagged text parser 
	@param ioText
		in: must be not NULL
		out: parsed plain text
	@param inVersion
		document version 
	@param inDPI
		dpi used to convert CSS measures from resolution independent unit to pixel unit 
		(default is 72 so 1px = 1pt for compatibility with 4D forms - might be overriden with "d4-dpi" property)
	*/
	VParserXHTML(VString* ioText, uLONG inVersion = kDOC_VERSION_SPAN4D_1, const uLONG inDPI = 72, bool inShowSpanRef = false, bool inNoUpdateRef = true, VDBLanguageContext *inLC = NULL)
		:VSpanHandler(NULL, CVSTR("body"), NULL, NULL, ioText, inVersion, inLC)
	{
		fShowSpanRef = inShowSpanRef;
		fNoUpdateRef = inNoUpdateRef;
		fDBLanguageContext = fNoUpdateRef ? NULL : inLC;

		VDocText *docText = new VDocText();
		fDocNode = static_cast<VDocNode *>(docText);
		docText->SetVersion( inVersion);
		docText->SetDPI( inDPI);
	
		//remark: for now, we create only one paragraph as all text shares same paragraph properties 
		VDocParagraph *para = new VDocParagraph();
		docText->AddChild( static_cast<VDocNode *>(para));

		if (!fStyles)
			fStyles = new VTreeTextStyle( new VTextStyle());
		para->SetStyles( fStyles); //paragraph retains/shares current styles
		
		if (!fText)
			fText = para->GetTextPtr();

		para->Release();

		docText->Release();

		xbox_assert( fText);
		fDPI = inDPI;
		if (inVersion == kDOC_VERSION_XHTML11)
		{
			VTaskLock lock(&fMutexStyles);
			if (fMapOfStyle.empty())
			{
				//build map of default styles per HTML element
				//FIXME ?: i guess i have set the mandatory ones but if i forget some mandatory HTML elements, please feel free to update this list...;)

				//italic
				VTreeTextStyle *style = RetainDefaultStyle("i");
				style->GetData()->SetItalic(TRUE);
				style->Release();

				//bold
				style = RetainDefaultStyle("b");
				style->GetData()->SetBold(TRUE);
				style->Release();

				//strikeout
				style = RetainDefaultStyle("s");
				style->GetData()->SetStrikeout(TRUE);
				style->Release();

				style = RetainDefaultStyle("strike");
				style->GetData()->SetStrikeout(TRUE);
				style->Release();

				//underline
				style = RetainDefaultStyle("u");
				style->GetData()->SetUnderline(TRUE);
				style->Release();

				//other (based on default style in browser)...

				style = RetainDefaultStyle("em");
				style->GetData()->SetItalic(TRUE);
				style->Release();

				style = RetainDefaultStyle("strong");
				style->GetData()->SetBold(TRUE);
				style->Release();

				style = RetainDefaultStyle("dfn");
				style->GetData()->SetItalic(TRUE);
				style->Release();

				style = RetainDefaultStyle("var");
				style->GetData()->SetItalic(TRUE);
				style->Release();

				style = RetainDefaultStyle("cite");
				style->GetData()->SetItalic(TRUE);
				style->Release();

				style = RetainDefaultStyle("address");
				style->GetData()->SetItalic(TRUE);
				style->Release();

				style = RetainDefaultStyle("center");
				style->GetData()->SetJustification(JST_Center);
				style->Release();

				//set HTML elements for which to add a extra break line
				fSetOfBreakLineEnding.insert(std::string("textArea"));
				fSetOfBreakLineEnding.insert(std::string("div"));
				fSetOfBreakLineEnding.insert(std::string("p"));
				fSetOfBreakLineEnding.insert(std::string("h1"));
				fSetOfBreakLineEnding.insert(std::string("h2"));
				fSetOfBreakLineEnding.insert(std::string("h3"));
				fSetOfBreakLineEnding.insert(std::string("h4"));
				fSetOfBreakLineEnding.insert(std::string("h5"));
				fSetOfBreakLineEnding.insert(std::string("h6"));
			}
		}
	};
	virtual ~VParserXHTML()
	{
		ReleaseRefCountable(&fStyles);
	}
	

	void CloseParsing(bool inPropagateParaPropsToStyles = false)
	{
#if VERSIONWIN
		fText->ConvertCarriageReturns(eCRM_CR); //we need to convert back to CR (on any platform but Linux here) because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)
#elif VERSIONMAC
		fText->ConvertCarriageReturns(eCRM_CR); //we need to convert back to CR (on any platform but Linux here) because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)
#endif

		if (fStyles)
			fStyles->GetData()->SetRange(fStart, fText->GetLength());

		if (!inPropagateParaPropsToStyles)
		{
			//caller retains VDocText so set text to parsed text & character styles
			//(only one paragraph for now so set full document text)

			_AttachSpanRefs();
			if (fStyles && ((fStyles)->IsUndefined(false)))
				//if none styles are defined remove styles
				ReleaseRefCountable(&fStyles);
			fDocNode->GetDocumentNodeForWrite()->SetStyles( fStyles, false);
			return;
		}

		if (fStyles)
		{
			//caller does not retain VDocText but only plain text + character styles 
			//so we need to pass VDocParagraph background color & text align properties to these styles (for v13 compatibility)

			VDocNode *child = fDocNode->RetainChild(0);
			xbox_assert(child->GetType() == kDOC_NODE_TYPE_PARAGRAPH);
			VDocParagraph *para = dynamic_cast<VDocParagraph *>(child);
			xbox_assert(para);
#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
			if (para->IsOverriden( kDOC_PROP_BACKGROUND_COLOR) && !para->IsInherited( kDOC_PROP_BACKGROUND_COLOR) && !fStyles->GetData()->GetHasBackColor())
			{
				fStyles->GetData()->SetHasBackColor( true);
				fStyles->GetData()->SetBackGroundColor( para->GetBackgroundColor());
			}  
#endif
			if (para->IsOverriden( kDOC_PROP_TEXT_ALIGN) && !para->IsInherited( kDOC_PROP_TEXT_ALIGN))
				fStyles->GetData()->SetJustification( (eStyleJust)para->GetTextAlign());
			ReleaseRefCountable(&child);
		}
		_AttachSpanRefs();

		if (fStyles && ((fStyles)->IsUndefined(false)))
		{
			//if none styles are defined remove styles
			ReleaseRefCountable(&fStyles);
		}
	}

	VTreeTextStyle *RetainStyles() 
	{ 
		return RetainRefCountable(fStyles); 
	}

	VDocText *RetainDocument()
	{
		return fDocNode->RetainDocumentNode();
	}

	/** document DPI (default is 72) 
	@remarks
		might be overriden with "d4-dpi" property
	*/
	uLONG GetDPI() { return fDocNode->GetDocumentNode()->GetDPI(); }

	void SetDPI( const uLONG inDPI) { fDocNode->GetDocumentNodeForWrite()->SetDPI( inDPI); }

	/** SPAN format version - on default, it is assumed to be kDOC_VERSION_SPAN_1 if parsing SPAN tagged text (that is v12/v13 compatible) & kDOC_VERSION_XHTML11 if parsing XHTML (but 4D SPAN or 4D XHTML) 
	@remarks
		might be overriden with "d4-version" property
	*/
	uLONG GetVersion() { return fDocNode->GetDocumentNode()->GetVersion(); }

	void SetVersion( const uLONG inVersion) { fDocNode->GetDocumentNodeForWrite()->SetVersion( inVersion); }

	static void ClearShared()
	{
		MapOfStylePerHTMLElement::iterator it = fMapOfStyle.begin();
		for(;it != fMapOfStyle.end();it++)
			it->second->Release();
		fMapOfStyle.clear();
	}


	/** write accessor on default style used for the specified HTML element 
	@remarks
		caller can use this method to customize default style for a HTML element

		always return a valid style
	*/
	static VTreeTextStyle* RetainDefaultStyle(const VString &inElementName)
	{
		VString elemName( inElementName);
		elemName.ToLowerCase();
		char *name = (char *)VMemory::NewPtr( elemName.GetLength()+1, 0);
		elemName.ToCString( name, elemName.GetLength()+1);
		VTreeTextStyle *style = RetainDefaultStyle( name);
		VMemory::DisposePtr(name);
		return style;
	}

	/** write accessor on default style used for the specified HTML element 
	@remarks
		caller can use this method to customize default style for a HTML element

		always return a valid style
	*/
	static VTreeTextStyle* RetainDefaultStyle(const char *inElementName)
	{
		VTaskLock lock(&fMutexStyles);

		xbox_assert(inElementName);
		MapOfStylePerHTMLElement::iterator it = fMapOfStyle.find( std::string(inElementName));
		if (it != fMapOfStyle.end())
		{
			//return current style
			it->second->Retain();
			return it->second;
		}
		else
		{
			//create new style & return it
			VTreeTextStyle *style = new VTreeTextStyle( new VTextStyle());
			fMapOfStyle[std::string(inElementName)] = style;
			style->Retain();
			return style;
		}
	}

	/** get accessor on default style used for the specified HTML element 
	@remarks
		return NULL if there is no default style
	*/
	static VTreeTextStyle* GetAndRetainDefaultStyle(const VString &inElementName)
	{
		VString elemName( inElementName);
		elemName.ToLowerCase();
		char *name = (char *)VMemory::NewPtr( elemName.GetLength()+1, 0);
		elemName.ToCString( name, elemName.GetLength()+1);
		VTreeTextStyle *style = GetAndRetainDefaultStyle( name);
		VMemory::DisposePtr(name);
		return style;
	}

	/** get accessor on default style used for the specified HTML element 
	@remarks
		return NULL if there is no default style
	*/
	static VTreeTextStyle* GetAndRetainDefaultStyle(const char *inElementName)
	{
		VTaskLock lock(&fMutexStyles);

		xbox_assert(inElementName);
		MapOfStylePerHTMLElement::iterator it = fMapOfStyle.find( std::string(inElementName));
		if (it != fMapOfStyle.end())
		{
			it->second->Retain();
			return it->second;
		}
		else
			return NULL;
	}

	/** return true if we need to add a ending break line while parsing the specified element */ 
	static bool NeedBreakLine( const VString &inElementName)
	{
		VTaskLock lock(&fMutexStyles);
		if (fSetOfBreakLineEnding.empty())
			return false;

		VString elemName( inElementName);
		elemName.ToLowerCase();
		char *name = (char *)VMemory::NewPtr( elemName.GetLength()+1, 0);
		elemName.ToCString( name, elemName.GetLength()+1);
		bool needBreakLine = NeedBreakLine( name);
		VMemory::DisposePtr(name);
		return needBreakLine;
	}

	/** return true if we need to add a ending break line while parsing the specified element */ 
	static bool NeedBreakLine( const char *inElementName)
	{
		VTaskLock lock(&fMutexStyles);
		if (fSetOfBreakLineEnding.empty())
			return false;

		SetOfHTMLElementWithBreakLineEnding::const_iterator it = fSetOfBreakLineEnding.find(std::string(inElementName));
		return it != fSetOfBreakLineEnding.end();
	}
private:
	void _AttachSpanRefs()
	{
		if (fStylesSpanRefs->empty())
			return;

		if (!fStyles)
		{
			fStyles = new VTreeTextStyle( new VTextStyle());
			fStyles->GetData()->SetRange(fStart, fText->GetLength());
		}
		if (fStyles->GetChildCount() > 0)
		{
			std::vector<VTreeTextStyle *>::iterator it = fStylesSpanRefs->begin();
			for (;it != fStylesSpanRefs->end(); it++)
			{
				fStyles->ApplyStyle( (*it)->GetData());
				(*it)->Release();
			}
		}
		else
		{
			std::vector<VTreeTextStyle *>::iterator it = fStylesSpanRefs->begin();
			for (;it != fStylesSpanRefs->end(); it++)
			{
				(*it)->GetData()->ResetCommonStyles( fStyles->GetData(), true);
				fStyles->AddChild( *it);
				(*it)->Release();
			}
		}
	}

	/** map of default style per HTML element */
	static MapOfStylePerHTMLElement fMapOfStyle;

	/** set of HTML elements for which to add a ending end of line */
	static SetOfHTMLElementWithBreakLineEnding fSetOfBreakLineEnding;

	static VCriticalSection fMutexStyles;

	/** document DPI (default is 72) 
	@remarks
		might be overriden with "d4-dpi" property
	*/
	uLONG fDPI;
};

/** map of default style per HTML element */
MapOfStylePerHTMLElement VParserXHTML::fMapOfStyle;

SetOfHTMLElementWithBreakLineEnding VParserXHTML::fSetOfBreakLineEnding;

VCriticalSection VParserXHTML::fMutexStyles;

//
// VSpanHandler
//

VSpanHandler::VSpanHandler(VSpanHandler *inParent, const VString& inElementName, VTreeTextStyle *inStylesParent, 
						   const VTextStyle *inStyleInheritParent, VString* ioText, uLONG /*inVersion*/, VDBLanguageContext *inLC)
{	
	fShowSpanRef = inParent ? inParent->fShowSpanRef : false;
	fNoUpdateRef = inParent ? inParent->fNoUpdateRef : true;
	fDBLanguageContext = fNoUpdateRef ? NULL : inLC;
	fSpanHandlerRoot = inParent ? inParent->fSpanHandlerRoot : this;
	fText = ioText; 
	fStart = fText ? fText->GetLength() : 0;
	fStylesRoot = NULL;
	fStylesParent = inStylesParent;
	fStyleInherit = inStyleInheritParent ? new VTextStyle(inStyleInheritParent) : NULL;
	fStyleInheritForChildrenDone = false;
	VTreeTextStyle *stylesDefault = VParserXHTML::GetAndRetainDefaultStyle(inElementName);
	if (stylesDefault)
	{
		fStyles = new VTreeTextStyle( stylesDefault);
		fStyles->GetData()->SetRange( fStart, fStart);
		if (fStylesParent)
			fStylesParent->AddChild( fStyles);
		ReleaseRefCountable(&stylesDefault);
	}
	else
		fStyles = NULL;
	if (fStylesParent)
	{
		VTreeTextStyle *styles = fStylesParent;
		while (styles->GetParent())
			styles = styles->GetParent();
		fStylesRoot = styles;
	}
	else 
		fStylesRoot = fStyles;

	fElementName = inElementName;

	if (fSpanHandlerRoot == this)
		fStylesSpanRefs = new std::vector<VTreeTextStyle *>();
	else
		fStylesSpanRefs = NULL;
	fDocNodeLevel = -1;
	fParentDocNode = NULL;
	fIsUnknownTag = false;
	fIsOpenTagClosed = false;
	fIsUnknownRootTag = false;
	fUnknownTagText = inParent ? inParent->fUnknownTagText : NULL;
}
	
VSpanHandler::~VSpanHandler()
{
	if (fStyles && fStyles->GetParent() && fStyles->IsUndefined(false))
	{
		//range is empty or style not defined (can happen while parsing node without any text or without styles): remove this useless style from parent if any
		VTreeTextStyle *parent = fStyles->GetParent();
		if (parent)
		{
			xbox_assert(parent == fStylesParent);
			for (int i = 1; i <= parent->GetChildCount(); i++)
				if (parent->GetNthChild(i) == fStyles)
				{
					parent->RemoveChildAt(i);
					break;
				}
		}
	}
	ReleaseRefCountable(&fStyles);
	if (fStyleInherit)
		delete fStyleInherit;
}

void  VSpanHandler::_CreateStyles()
{
	if (fStyles == NULL)
	{
		fStyles = new VTreeTextStyle();
		VTextStyle* style = new VTextStyle();
		style->SetRange(fStart, fText->GetLength());
		fStyles->SetData(style);
		if (fStylesParent)
			fStylesParent->AddChild( fStyles);
		if (!fStylesRoot)
			fStylesRoot = fStyles;
	}
}

const VTextStyle *VSpanHandler::_GetStyleInheritForChildren()
{
	if (!fStyleInheritForChildrenDone)
	{
		if (fStyles && fStyleInherit)
			//merge current style with parent inherited style
			fStyleInherit->MergeWithStyle( fStyles->GetData());
		fStyleInheritForChildrenDone = true;
	}
	VTextStyle *styleInherit = fStyleInherit;
	if (!styleInherit)
	{
		//determine inherited styles to pass to children
		if (fStylesParent)
			styleInherit = fStylesParent->GetData();
		else if (fStyles)
			styleInherit = fStyles->GetData();
	}
	return styleInherit;
}

IXMLHandler* VSpanHandler::StartElement(const VString& inElementName)
{
	IXMLHandler* result = NULL; 

	if (fIsUnknownTag)
	{
		if (!fIsOpenTagClosed)
		{
			fIsOpenTagClosed = true;
			fText->AppendUniChar('>');
		}

		//parsing unknown tags -> store as unknown tag span ref 
		//for unknown tag children tags, we serialize full XML tag
		VSpanHandler *spanHandler;
		result = spanHandler = new VSpanHandler(this, inElementName, fStyles ? fStyles : fStylesParent, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
		spanHandler->fDocNodeLevel = fDocNodeLevel;
		spanHandler->fIsUnknownTag = true;
		spanHandler->SetParentDocNode( fDocNode);
		//for now, all span nodes share the same paragraph node
		VDocNode *para = fDocNode->GetDocumentNodeForWrite()->RetainChild(0);
		spanHandler->SetDocNode( para); //set span handler node to paragraph node
		para->Release();
		
		//children of unknown tag use same style & span reference 
		//(unknown tag root & children are serialized as xml text in fText and will be stored in span reference on root unknown tag EndElement)
		xbox_assert(fStyles && fStyles->GetData()->IsSpanRef());
		CopyRefCountable(&(spanHandler->fStyles), fStyles);		
		spanHandler->fStart = fStart;
		fText->AppendString( CVSTR("<")+inElementName);
	}
	else if (fStyles && fStyles->GetData()->IsSpanRef())
		return NULL; //span reference but unknown tag (4D expression, user span ref, url link) have none children (as text style is hard-coded)
	else if(inElementName.EqualToString("span"))
	{
		_CreateStyles();
		VSpanHandler *spanHandler;

		if (fDocNodeLevel < 0 && fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_XHTML11) 
		{
			//root span node & 4D SPAN format = document root node 
			_CreateStyles();
			result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
			spanHandler->fDocNodeLevel = 0;
			VDocNode *document = fDocNode->GetDocumentNodeForWrite();
			spanHandler->SetDocNode( static_cast<VDocNode *>(document)); //set span handler node to document node
		}
		else //otherwise root span child node or not 4D SPAN or 4D XHTML format (XHTML from exterior source)
		{
			result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
			spanHandler->fDocNodeLevel = fDocNodeLevel+1;
			spanHandler->SetParentDocNode( fDocNode);
			//for now, all span nodes share the same paragraph node
			VDocNode *para = fDocNode->GetDocumentNodeForWrite()->RetainChild(0);
			spanHandler->SetDocNode( para); //set span handler node to paragraph node
			para->Release();
		}
	}
	else if(inElementName.EqualToString("a") && fDocNodeLevel >= 0)
	{
		//url
		_CreateStyles();
		VSpanHandler *spanHandler;

		result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
		spanHandler->fDocNodeLevel = fDocNodeLevel+1;
		spanHandler->SetParentDocNode( fDocNode);
		//for now, all span nodes share the same paragraph node
		VDocNode *para = fDocNode->GetDocumentNodeForWrite()->RetainChild(0);
		spanHandler->SetDocNode( para); //set span handler node to paragraph node
		para->Release();
	}
	else if (inElementName.EqualToString("br"))
	{
		*fText+="\r";
		if (fUnknownTagText)
			*fUnknownTagText += "\r";
	}
	else if (fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_XHTML11 && fDocNodeLevel >= 0)
	{
		//parsing unknown tags -> store as unknown tag span ref 

		_CreateStyles();
				
		VSpanHandler *spanHandler;
		result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
		spanHandler->fDocNodeLevel = fDocNodeLevel+1;
		spanHandler->fIsUnknownTag = true;
		spanHandler->SetParentDocNode( fDocNode);
		//for now, all span nodes share the same paragraph node
		VDocNode *para = fDocNode->GetDocumentNodeForWrite()->RetainChild(0);
		spanHandler->SetDocNode( para); //set span handler node to paragraph node
		para->Release();

		spanHandler->fIsUnknownRootTag = true;
		spanHandler->_CreateStyles();
		VDocSpanTextRef *spanRef = new VDocSpanTextRef( kSpanTextRef_UnknownTag, VDocSpanTextRef::fNBSP, VDocSpanTextRef::fNBSP);
		spanHandler->fStyles->GetData()->SetSpanRef( spanRef);
		if (spanHandler->fStyles->GetData()->IsSpanRef())
			spanHandler->fStyles->NotifySetSpanRef();
		fText->AppendString( CVSTR("<")+inElementName);
		spanHandler->fUnknownTagText = &(spanHandler->fUnknownRootTagText);
	}
	else if (fDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11)
	{
		//parsing XHTML generic content: parsing plain text & as a span node

		if (inElementName.EqualToString("q"))
			//start of quote
			*fText += "\"";

		_CreateStyles();
		VSpanHandler *spanHandler;
		result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);
		spanHandler->fDocNodeLevel = fDocNodeLevel+1;
		spanHandler->SetParentDocNode( fDocNode);
		//for now, all span nodes share the same paragraph node
		VDocNode *para = fDocNode->GetDocumentNodeForWrite()->RetainChild(0);
		spanHandler->SetDocNode( para); //set span handler node to paragraph node
		para->Release();
	}
	return result;
}

void VSpanHandler::EndElement( const VString& inElementName)
{
	//xbox_assert(inElementName == fElementName);

	VTextStyle *style = fStyles ? fStyles->GetData() : NULL;
	if (style)
	{
		sLONG start, end;
		style->GetRange(start, end);
		end = fText->GetLength();
		style->SetRange(start, end);
	}

	if(style && style->IsSpanRef())
	{
		//detach span ref fStyles & store temporary in root span handler (span refs will be attached to fStyles root on CloseParsing)
		if (!(fIsUnknownTag && !fIsUnknownRootTag))
		{
			xbox_assert(fStyles->GetParent());
			fStyles->Retain();
			fStyles->GetData()->MergeWithStyle( _GetStyleInherit());
			fStyles->GetParent()->RemoveChildAt( fStyles->GetParent()->GetChildCount());
			xbox_assert(fStyles->GetParent() == NULL && fStyles->GetRefCount() >= 2);
			fSpanHandlerRoot->fStylesSpanRefs->push_back(fStyles);
		}

		//finalize span ref
		sLONG start, end;
		style->GetRange(start, end);
		style->GetSpanRef()->ShowRef( fShowSpanRef);

		VString reftext;
		if (fIsUnknownTag)
		{
			if (fIsOpenTagClosed)
			{
				fText->AppendString("</");
				fText->AppendString(fElementName);
				fText->AppendUniChar('>');
			}
			else
			{
				fIsOpenTagClosed = true;
				fText->AppendString("/>");
			}
			xbox_assert(style->GetSpanRef()->GetType() == kSpanTextRef_UnknownTag);
			if (fIsUnknownRootTag)
			{
				start = fStart;
				end = fText->GetLength();
				if (start < end)
					fText->GetSubString( start+1, end-start, reftext);
				style->GetSpanRef()->SetRef( reftext);
				style->GetSpanRef()->SetDefaultValue( fUnknownRootTagText);

				fText->Truncate(fStart);
				VString value = fNoUpdateRef ? VDocSpanTextRef::fNBSP : style->GetSpanRef()->GetDisplayValue();
				fText->AppendString(value); 
				style->SetRange(fStart, fText->GetLength());
			}
		}
		else if (start <= end)
		{
			if (start < end)
				fText->GetSubString( start+1, end-start, reftext);			
			else
				reftext.SetEmpty();

			reftext.ConvertCarriageReturns(eCRM_CR);
			style->GetSpanRef()->SetDefaultValue( reftext); //default value is set to initial parsed plain text
			
			fText->Truncate(fStart);
			VString value = fNoUpdateRef ? VDocSpanTextRef::fNBSP : style->GetSpanRef()->GetDisplayValue();
			fText->AppendString(value); 
			style->SetRange(fStart, fText->GetLength());
		}
	}

	if (fDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11)
	{
		//HTML parsing:

		if (inElementName.EqualToString("q"))
			//end of quote
			*fText += "\"";

		//add break lines for elements which need it
		if (VParserXHTML::NeedBreakLine(inElementName))
			*fText += "\r";
	}
}

void VSpanHandler::SetAttribute(const VString& inName, const VString& inValue)
{
	if (fIsUnknownTag)
	{
		fText->AppendUniChar(' ');
		VString name = inName;
		VSpanTextParser::Get()->TextToXML( name, true, true);
		fText->AppendString(name);
		
		fText->AppendString("=\"");

		VString value = inValue;
		VSpanTextParser::Get()->TextToXML( value, true, true);
		fText->AppendString(value);

		fText->AppendUniChar('\"');
	}
	else if( inName.EqualToString("style"))
	{
		_CreateStyles();
		
		if (fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_XHTML11) //parsing 4D tagged text
		{
			//kDOC_VERSION_SPAN_1 format compatible with v13:
			//	- root span node is parsed for document, paragraph & character properties
			//  - span nodes which have a parent span node are parsed for character styles only
			// for instance: <span style="zoom:150%;text-align:right;margin:0 60pt;font-family:'Arial'">mon texte <span style="font-weight:bold">bold</span></span>

			if (fDocNodeLevel >= 0)
			{
				if (fDocNodeLevel == 0) //root span element
				{
					//root element: parse document, paragraph or character properties
					VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, inValue);
				}
				else
					//otherwise parse only span styles
					VSpanTextParser::Get()->_ParseCSSToVTreeTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, inValue, fStyles, _GetStyleInherit());
			}
		}
		else
			//if XHTML from exterior source, parse only span styles
			VSpanTextParser::Get()->_ParseCSSToVTreeTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, inValue, fStyles, _GetStyleInherit());
	} 
	if( fElementName.EqualToString("a") && inName.EqualToString("href"))
	{
		//parse url

		if (!(fStyles && fStyles->GetData()->IsSpanRef()))
		{
			_CreateStyles();
			VSpanTextParser::Get()->ParseAttributeHRef( (VDocNode *)fDocNode, inValue, *((fStyles)->GetData()));
			if (fStyles->GetData()->IsSpanRef())
				fStyles->NotifySetSpanRef();
		}
	}
	else if (fDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11)
	{
		_CreateStyles();

		SetHTMLAttribute( inName, inValue);
	}
}

void VSpanHandler::SetHTMLAttribute(const VString& inName, const VString& inValue)
{
	//convert to CSS - no performance hit here as we should never fall here while parsing private 4D SPAN or 4D XHTML (which uses only CSS styles)
	//but only while parsing XHTML fragment from paste or drag drop from external source like from Web browser
	//(and paste and drag drop action is not done multiple times per frame)

	VString valueCSS;
	if (inName.EqualToString("face"))
	{
		valueCSS.AppendString(VString(kCSS_NAME_FONT_FAMILY)+":"+inValue);
		_CreateStyles();
		VSpanTextParser::Get()->ParseCSSToVTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, valueCSS, *((fStyles)->GetData()), _GetStyleInherit());
	}
	else if (inName.EqualToString("color"))
	{
		valueCSS.AppendString(VString(kCSS_NAME_COLOR)+":"+inValue);
		_CreateStyles();
		VSpanTextParser::Get()->ParseCSSToVTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, valueCSS, *((fStyles)->GetData()), _GetStyleInherit());
	}
	else if (inName.EqualToString("bgcolor"))
	{
		valueCSS.AppendString(VString(kCSS_NAME_BACKGROUND_COLOR)+":"+inValue);
		_CreateStyles();
		VSpanTextParser::Get()->ParseCSSToVTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, valueCSS, *((fStyles)->GetData()), _GetStyleInherit());
	}
	else if (inName.EqualToString("size"))
	{
		//HTML font size is a number from 1 to 7; default is 3 = 12pt
		sLONG size = inValue.GetLong();
		if (size >= 1 && size <= 7)
		{
			size = size*12/3; //convert to point
			VString fontSize;
			fontSize.FromLong(size);

			valueCSS.AppendString(VString(kCSS_NAME_FONT_SIZE)+":"+fontSize+"pt");

			_CreateStyles();
			VSpanTextParser::Get()->ParseCSSToVTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, valueCSS, *((fStyles)->GetData()), _GetStyleInherit());
		}
	}
}

void VSpanHandler::SetText( const VString& inText)
{
	if (fIsUnknownTag)
	{
		if (!fIsOpenTagClosed)
		{
			fIsOpenTagClosed = true;
			fText->AppendUniChar('>');
		}
		VString text = inText;
		VSpanTextParser::Get()->TextToXML( text, true, true, true);
		*fText += text;
		if (fUnknownTagText)
			*fUnknownTagText += inText;
	}
	else
		*fText += inText;
}


//////////////////////////////////////////////////////////////////////////////////////


	
void JustToString(eStyleJust inValue, VString& outValue)
{
	if(inValue == JST_Left)
		outValue = "left";
	else if(inValue == JST_Right)
		outValue = "right";
	else if(inValue == JST_Center)
		outValue = "center";
	else if(inValue == JST_Justify)
		outValue = "justify";
	else
	{
		assert(false);
		outValue = "left";
	}
}


void ColorToValue(const RGBAColor inColor, VString& outValue)
{
	outValue.FromHexLong(inColor);
	sLONG len = outValue.GetLength();
	//ignorer le canal alpha
	if(len==8 && outValue.BeginsWith("FF"))
	{
		outValue.Replace("",1,2);
	}

	for(int i=0; i < 6-len;i++)
	{
		outValue = "0" + outValue;
	}

	outValue = "#" + outValue;
}

//////////////////////////////////////////////////////////////////////////////////////
// class VSpanTextParser
//////////////////////////////////////////////////////////////////////////////////////

VSpanTextParser *VSpanTextParser::sInstance = NULL;

#if VERSIONWIN
float VSpanTextParser::GetSPANFontSizeDPI() const
{
	//screen dpi for compatibility with SPAN format in v12/v13 
	//(SPAN format stores font size in point*72/screen dpi (screen dependent font size) on Windows only (alike font size in point on Mac)
	// - nasty but we have no choice than to maintain compatibility on default)
	static float sGetDPIValue = 0.0f;
	if (!sGetDPIValue)
	{
		HDC hdc = ::CreateCompatibleDC(NULL);
		sGetDPIValue = ::GetDeviceCaps(hdc, LOGPIXELSY);
		::DeleteDC( hdc);
	}
	return sGetDPIValue;
}
#endif


void VSpanTextParser::_BRToCR( VString& ioText)
{
	//for compatibility v12: in v12 we tolerate malformed xml text, that is un-escaped plain text with xml <BR/> tags...
	ioText.ExchangeRawString(CVSTR("<BR/>"), CVSTR("\r"), 1, kMAX_VIndex); 
}


void VSpanTextParser::_CheckAndFixXMLText(const VString& inText, VString& outText, bool inParseSpanOnly)
{
	sLONG posStartSPAN = 1; //pos of first SPAN tag
	sLONG posEndSPAN = inText.GetLength()+1; //pos of character following ending SPAN tag
	bool isV13 = false;
	if (inParseSpanOnly)
	{
		//fixed ACI0076343: for compatibility with v12 which tolerate mixing un-escaped plain text with xml text enclosed with <SPAN> tags
		//					we need to convert to xml text the text which is not bracketed with SPAN 
		//					otherwise parser would fail

		//search for opening SPAN tag
		posStartSPAN = inText.Find("<span", 1, false);
		if (!posStartSPAN)
		{
			//plain text only: convert full text to xml
			outText = inText;
			_BRToCR(outText); //v12 tolerates mixing unescaped plain text with <BR/> xml tags so ensure to convert any <BR/> tag prior to convert to XML
			TextToXML( outText, true, true, true);
			outText = VString("<span>")+outText+"</span>";
		}
		else 
		{
			if (inText.GetUniChar(posStartSPAN+1) == 'S')
				//it is v13 format where span tag is in upper case:
				//we need to embed whole span text in a root span tag (otherwise new parser would fail as it is XML compliant so only one root tag is allowed)
				isV13 = true;

			VString before, after;
			if (posStartSPAN > 1)
			{
				//convert to XML text preceeding first SPAN 
				inText.GetSubString( 1, posStartSPAN-1, before);
				_BRToCR( before); //v12 tolerates mixing unescaped plain text with <BR/> xml tags so ensure to convert any <BR/> tag prior to convert to XML
				TextToXML( before, true, true, true);
			}
			//search for ending SPAN tag
			const UniChar *c = inText.GetCPointer()+inText.GetLength()-1;
			sLONG pos = inText.GetLength()-1;
			VString spanEndTag = "</SPAN>"; //for compat v13
			VString spanEndTag2 = "</span>";
			bool hasEndSpan = false;
			do
			{
				while (pos >= 0 && *c != '>')
				{
					pos--;
					if (pos >= 0)
						c--;
				}
				if (pos >= 0)
				{
					if (memcmp( (void*)(c+1-spanEndTag.GetLength()), spanEndTag.GetCPointer(), spanEndTag.GetLength()*sizeof(UniChar)) == 0
						||
						memcmp( (void*)(c+1-spanEndTag2.GetLength()), spanEndTag2.GetCPointer(), spanEndTag2.GetLength()*sizeof(UniChar)) == 0)
					{
						posEndSPAN = pos+2;
						hasEndSpan = true;
						break;
					}	
					else
					{
						pos--;
						if (pos >= 0)
							c--;
					}
				}
			} while (pos >= 0);

			if (!hasEndSpan)
			{
				//not valid XML text: convert full text to xml
				outText = inText;
				TextToXML( outText, true, true, true); //here we do not call _BRToCR as we want to preserve any <BR/> tags as plain text (invalid span text)
				outText = VString("<span>")+outText+"</span>";
			}
			else
				if (posEndSPAN <= inText.GetLength())
				{
					//convert to XML text following ending SPAN tag
					inText.GetSubString( posEndSPAN, inText.GetLength()-posEndSPAN+1, after);
					_BRToCR( after); //v12 tolerates mixing unescaped plain text with <BR/> xml tags so ensure to convert any <BR/> tag prior to convert to XML
					TextToXML( after, true, true, true);
				}
				if (!before.IsEmpty() || !after.IsEmpty())
				{
					inText.GetSubString( posStartSPAN, posEndSPAN-posStartSPAN, outText);
					TextToXML(outText, true, false, true); //fix XML invalid chars & convert CR if not done yet but do not convert XML markup as text should be XML text yet
					outText = VString("<span>")+before+outText+after+"</span>";
				}
				else
				{
					outText = inText;
					TextToXML(outText, true, false, true); //fix XML invalid chars & convert CR if not done yet but do not convert XML markup as text should be XML text yet
					if (isV13)
						outText = VString("<span>")+outText+"</span>";
				}
		}
	}
	else
	{
		outText = inText;
		TextToXML(outText, true, false, true); //fix XML invalid chars & convert CR if not done yet but do not convert XML markup as text should be XML text yet
	}
}

/** parse span text element or XHTML fragment 
@param inTaggedText
	tagged text to parse (span element text only if inParseSpanOnly == true, XHTML fragment otherwise)
@param outStyles
	parsed styles
@param outPlainText
	parsed plain text
@param inParseSpanOnly
	true (default): input text is 4D SPAN tagged text (version depending on "d4-version" CSS property) - 4D CSS rules apply (all span character styles are inherited)
	false: input text is XHTML 1.1 fragment (generally from external source than 4D) - W3C CSS rules apply (some CSS styles like "text-decoration" & "background-color" are not inherited)
*/
void VSpanTextParser::ParseSpanText(	const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly, const uLONG inDPI, 
										bool inShowSpanRef, bool inNoUpdateRefs, VDBLanguageContext *inLC)
{
	VTaskLock protect(&fMutexParser); //protect parsing

	outPlainText = "";
	if(outStyles)
		outStyles->Release();    
	outStyles = NULL;
	if (inTaggedText.IsEmpty())
		return;

	VString vtext;
	if (inTaggedText.BeginsWith("<xhtml") || inTaggedText.BeginsWith("<html")) //for compat with future 4D XHTML format
		inParseSpanOnly = false;
	_CheckAndFixXMLText( inTaggedText, vtext, inParseSpanOnly);

	VParserXHTML vsaxhandler(&outPlainText, inParseSpanOnly ? kDOC_VERSION_SPAN4D_1 : kDOC_VERSION_XHTML11, inDPI, inShowSpanRef, inNoUpdateRefs, inLC);
	VXMLParser vSAXParser;

	vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever);

	vsaxhandler.CloseParsing(true); //finalize text & styles if appropriate
	outStyles = vsaxhandler.RetainStyles();
}

void VSpanTextParser::GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle)
{
	VDocText *doc = new VDocText();
	VDocParagraph *para = new VDocParagraph();
	doc->AddChild(para);
	VTreeTextStyle *styles = new VTreeTextStyle( inStyles);
	para->SetText( inPlainText, styles, false);

	//set paragraph text align from styles if styles overrides it (text align is now a paragraph property)
	if (styles->GetData()->GetJustification() != JST_Notset)
	{
		para->SetTextAlign( styles->GetData()->GetJustification());
		styles->GetData()->SetJustification( JST_Notset);
	}

	ReleaseRefCountable(&styles);

	VDocParagraph *paraDefault = NULL;
	if (inDefaultStyle)
	{
		paraDefault = new VDocParagraph();
		VTreeTextStyle *stylesDefault = new VTreeTextStyle( new VTextStyle( inDefaultStyle));
		paraDefault->SetStyles( stylesDefault, false);
		ReleaseRefCountable(&stylesDefault);
	}
	SerializeDocument( doc, outTaggedText, paraDefault);

	ReleaseRefCountable(&para);
	ReleaseRefCountable(&paraDefault);
	ReleaseRefCountable(&doc);
}


void VSpanTextParser::GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly, 
												bool inShowSpanRef, bool inNoUpdateRefs, VDBLanguageContext *inLC)
{
	VTreeTextStyle*	vUniformStyles = NULL;
	ParseSpanText(inTaggedText, vUniformStyles, outPlainText, inParseSpanOnly, 72, inShowSpanRef, inNoUpdateRefs, inLC);
	if(vUniformStyles)
		vUniformStyles->Release();
}




/** get styled text on the specified range */
bool VSpanTextParser::GetStyledText( const VString& inSpanText, VString& outSpanText, sLONG inStart, sLONG inEnd, bool inSilentlyTrapParsingErrors)
{
	if (inStart <= 0 && inEnd == -1)
	{
		outSpanText = inSpanText;
		return true;
	}

	VDocText *doc = VSpanTextParser::Get()->ParseDocument( inSpanText, kDOC_VERSION_SPAN4D_1, 72, false, true, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
	if (!doc)
		return false;

	sLONG docTextLength = doc->GetText().GetLength();
	
	if (inStart <= 0 && inEnd >= docTextLength)
	{
		outSpanText = inSpanText;
		ReleaseRefCountable(&doc);
		return true;
	}

	if (inEnd > docTextLength)
		inEnd = docTextLength;
	if (inStart > docTextLength)
		inStart = docTextLength;

	if (inEnd == -1)
		inEnd = docTextLength;
	if (inStart > inEnd)
	{
		ReleaseRefCountable(&doc);
		return false;
	}

	VDocParagraph *para = doc->RetainFirstParagraph();
	xbox_assert(para);

	VTreeTextStyle *styles = para->RetainStyles();
	if (styles)
	{
		VString text;
		if (inEnd > inStart)
			para->GetText().GetSubString(inStart+1, inEnd-inStart, text);
		VTreeTextStyle *newstyles = styles->CreateTreeTextStyleOnRange( inStart, inEnd, true, false);
		ReleaseRefCountable(&styles);
		
		para->SetText( text, newstyles, false);
		ReleaseRefCountable(&newstyles);
	}
	else
	{
		VString text;
		if (inEnd > inStart)
			para->GetText().GetSubString(inStart+1, inEnd-inStart, text);
		
		para->SetText( text);
	}

	//serialize
	SerializeDocument( doc, outSpanText);

	ReleaseRefCountable(&para);
	ReleaseRefCountable(&doc);

	return true;
}


/** replace styled text at the specified range with text 
@remarks
	replaced text will inherit only uniform style on replaced range
*/
bool VSpanTextParser::ReplaceStyledText( VString& ioSpanText, const VString& inSpanTextOrPlainText, sLONG inStart, sLONG inEnd, bool inTextPlainText, bool inInheritUniformStyle, bool inSilentlyTrapParsingErrors)
{
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D_1, 72, false, true, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
	if (!doc)
		return false;

	VIndex docTextLength = doc->GetText().GetLength();
	if (inEnd < 0)
		inEnd = docTextLength;
	if (inStart > inEnd)
	{
		ReleaseRefCountable(&doc);
		return false;
	}

	VDocParagraph *para = doc->RetainFirstParagraph();
	xbox_assert(para);

	VDocText *inDoc = NULL;
	VDocParagraph *inPara = NULL;

	const VString *inText = NULL;
	const VTreeTextStyle *inStyles = NULL;

	if (inTextPlainText)
	{
		inText = &inSpanTextOrPlainText;
		inStyles = NULL;
	}
	else
	{
		inDoc = VSpanTextParser::Get()->ParseDocument( inSpanTextOrPlainText, kDOC_VERSION_SPAN4D_1, 72, false, true, inSilentlyTrapParsingErrors);
		if (!inDoc)
		{
			ReleaseRefCountable(&para);
			ReleaseRefCountable(&doc);
			return false;
		}
		inPara = inDoc->RetainFirstParagraph();

		if (inStart == 0 && inEnd == docTextLength)
			//overwrite paragraph props if we replace all span text - and only if props are not defined yet (otherwise we ignore input span text paragraph props)
			para->AppendPropsFrom( inPara, false, true);
		
		inText = inPara->GetTextPtr();
		inStyles = inPara->RetainStyles();
	}
	
	VTreeTextStyle *styles = para->RetainStyles();
	bool ok = VTreeTextStyle::ReplaceStyledText( *(para->GetTextPtr()), styles, *inText, inStyles, inStart, inEnd, false, true, inInheritUniformStyle) != 0;
	if (ok)
	{
		if (styles != para->GetStyles())
			para->SetStyles( styles, false);
	}
	ReleaseRefCountable(&styles);

	ReleaseRefCountable(&inStyles);
	ReleaseRefCountable(&inPara);
	ReleaseRefCountable(&inDoc);

	//serialize
	SerializeDocument( doc, ioSpanText);

	ReleaseRefCountable(&para);
	ReleaseRefCountable(&doc);

	return true;
}

/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
bool VSpanTextParser::FreezeExpressions( VDBLanguageContext *inLC, VString& ioSpanText, sLONG inStart, sLONG inEnd, bool inSilentlyTrapParsingErrors)
{
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D_1, 72, false, true, inSilentlyTrapParsingErrors); 
	if (!doc)
		return false;

	VIndex docTextLength = doc->GetText().GetLength();
	if (inEnd < 0)
		inEnd = docTextLength;
	if (inStart > inEnd)
	{
		ReleaseRefCountable(&doc);
		return false;
	}

	bool modified = doc->FreezeExpressions( inLC, inStart, inEnd);

	//serialize
	if (modified)
		SerializeDocument( doc, ioSpanText);

	ReleaseRefCountable(&doc);

	return modified;
}


/** return the first span reference which intersects the passed range */
sLONG VSpanTextParser::GetReferenceAtRange(
				VDBLanguageContext *inLC,
				const VString& inSpanText, 
				const sLONG inStart, const sLONG inEnd, 
				sLONG *outFirstRefStart, sLONG *outFirstRefEnd, 
				VString *outFirstRefSource, VString *outFirstRefValue
				)
{
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( inSpanText, kDOC_VERSION_SPAN4D_1, 72, false, true, true); 
	if (!doc)
		return -1;

	sLONG type = -1;
	const VTextStyle *style = doc->GetStyleRefAtRange( inStart, inEnd);
	if (style)
	{
		type = (sLONG)style->GetSpanRef()->GetType();
		if (outFirstRefStart || outFirstRefEnd)
		{
			sLONG start, end;
			style->GetRange( start, end);
			if (outFirstRefStart)
				*outFirstRefStart = start;
			if (outFirstRefEnd)
				*outFirstRefEnd = end;
		}
		if (outFirstRefSource)
			*outFirstRefSource = style->GetSpanRef()->GetRef();
		if (outFirstRefValue)
		{
			if (inLC)
				style->GetSpanRef()->EvalExpression( inLC);
			*outFirstRefValue = style->GetSpanRef()->GetComputedValue();
		}
	}

	ReleaseRefCountable(&doc);

	return type;
}


/** replace styled text with span text reference on the specified range
	
	you should no more use or destroy passed inSpanRef even if method returns false
*/
bool VSpanTextParser::ReplaceAndOwnSpanRef( VString& ioSpanText, VDocSpanTextRef* inSpanRef, sLONG inStart, sLONG inEnd, bool inSilentlyTrapParsingErrors)
{
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D_1, 72, false, true, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
	if (!doc)
		return false;

	VIndex docTextLength = doc->GetText().GetLength();
	if (inEnd < 0)
		inEnd = docTextLength;
	if (inStart > inEnd)
	{
		ReleaseRefCountable(&doc);
		return false;
	}

	doc->ReplaceAndOwnSpanRef( inSpanRef, inStart, inEnd, false, true);

	//serialize
	SerializeDocument( doc, ioSpanText);

	ReleaseRefCountable(&doc);

	return true;
}


/** parse text document */
VDocText *VSpanTextParser::ParseDocument(	const VString& inDocText, const uLONG inVersion, const uLONG inDPI, 
											bool inShowSpanRef, bool inNoUpdateRefs, bool inSilentlyTrapParsingErrors, VDBLanguageContext *inLC)
{
	VTaskLock protect(&fMutexParser); //protect parsing

	//if (inDocText.IsEmpty())
	//	return NULL;

	uLONG version = inVersion;
	if (inDocText.BeginsWith("<xhtml") || inDocText.BeginsWith("<html")) //for compat with future 4D XHTML format
		version = kDOC_VERSION_XHTML11;

	VString vtext;
	_CheckAndFixXMLText( inDocText, vtext, inVersion !=  kDOC_VERSION_XHTML11);

	VParserXHTML vsaxhandler(NULL, inVersion, inDPI, inShowSpanRef, inNoUpdateRefs, inLC);
	VXMLParser vSAXParser;

	if (inSilentlyTrapParsingErrors)
	{
		StErrorContextInstaller errorContext(false);

		vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever);

		VErrorContext* error = errorContext.GetContext();

		if (error && !error->IsEmpty())
			return NULL;

		vsaxhandler.CloseParsing(false); //finalize if appropriate

		return vsaxhandler.RetainDocument();
	}
	else
	{
		vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever);

		vsaxhandler.CloseParsing(false); //finalize if appropriate
		
		return vsaxhandler.RetainDocument();
	}
}


/** serialize document (formatting depending on version) 
@remarks
	//kDOC_VERSION_SPAN_1 format compatible with v13:
	//	- root span node is parsed for document, paragraph & character properties
	//  - span nodes which have a parent span node are parsed for character styles only
	//
	// for instance: <span style="zoom:150%;text-align:right;margin:0 60pt;font-family:'Arial'">mon texte <span style="font-weight:bold">bold</span></span>

	if (inDocParaDefault != NULL), only properties and character styles different from the passed paragraph properties & character styles will be serialized
*/
void VSpanTextParser::SerializeDocument( const VDocText *inDoc, VString& outDocText, const VDocParagraph *inDocParaDefault)
{
	xbox_assert(inDoc);

	outDocText.SetEmpty();

	fIsOnlyPlainText = true;

	_OpenTagDoc( outDocText); //kDOC_VERSION_SPAN4D_1 root node is a <span> 

	//serialize document & paragraph properties (only one paragraph for now)

	const VDocNode *child = inDoc->GetChild(0);
	const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(child);
	xbox_assert(para);

	if (para->GetText().IsEmpty())
	{
		outDocText.FromString( para->GetText());
		return;
	}

	VTextStyle *styleInherit = new VTextStyle();
	styleInherit->SetBold( 0);
	styleInherit->SetItalic( 0);
	styleInherit->SetStrikeout( 0);
	styleInherit->SetUnderline( 0);

	if (inDocParaDefault && inDocParaDefault->GetStyles())
		styleInherit->MergeWithStyle( inDocParaDefault->GetStyles()->GetData());

	//ensure we do not serialize text align or char background color if equal to paragraph text align or back color

	if (styleInherit->GetJustification() == JST_Default)
	{
		if (para->IsOverriden(kDOC_PROP_TEXT_ALIGN) && !para->IsInherited(kDOC_PROP_TEXT_ALIGN))
			styleInherit->SetJustification( (eStyleJust)para->GetTextAlign());
	}

	if (!styleInherit->GetHasBackColor() && para->IsOverriden(kDOC_PROP_BACKGROUND_COLOR) && !para->IsInherited(kDOC_PROP_BACKGROUND_COLOR))
	{
		RGBAColor color = para->GetBackgroundColor();
		if (color != RGBACOLOR_TRANSPARENT)
		{
			styleInherit->SetHasBackColor(true);
			styleInherit->SetBackGroundColor( color);
		}
	}

	if (para->HasProps() || para->GetStyles()) 
	{
		//kDOC_VERSION_SPAN4D_1 -> we serialize in root span node paragraph props + paragraph character styles

		VIndex lengthBeforeStyle = outDocText.GetLength();
		outDocText.AppendCString(" style=\"");
		VIndex start = outDocText.GetLength();

#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
		if (inDoc->HasProps()) //in 4D SPAN format, version = 1, dpi = 72 & zoom = 100%
			_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDoc), outDocText);
#endif

		if (para->HasProps())
			_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(para), outDocText, static_cast<const VDocNode *>(inDocParaDefault), outDocText.GetLength() == start);
		
		VTextStyle *style = para->GetStyles() ? para->GetStyles()->GetData() : NULL;
		if (style)
			_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(para), *style, outDocText, styleInherit, false, outDocText.GetLength() == start);
		
		if (outDocText.GetLength() > start)
		{
			outDocText.AppendUniChar('\"');
			fIsOnlyPlainText = false;
		}
		else
			outDocText.Truncate( lengthBeforeStyle);
	}

	_CloseTagDoc( outDocText, false); //close root opening tag

	//serialize paragraph plain text & character styles

	_SerializeParagraphTextAndStyles( child, para->GetStyles(), para->GetText(), outDocText, styleInherit, true);

	_CloseTagDoc( outDocText); //close document tag

	if (fIsOnlyPlainText)
	{
		//optimization: if text is only plain text, we output plain text & not span text
		if ((!inDoc->GetText().Find(CVSTR("<span")) && (!inDoc->GetText().Find(CVSTR("<br/>")))))	//if plain text contains <span or <br/>, we serialize it as xml otherwise it might be parsed as xml after on parsing
																								//(that is text might be either fully plain text or either fully xml but we cannot mix both) 
			outDocText.FromString( inDoc->GetText());
	}
}


void VSpanTextParser::_OpenTagSpan(VString& ioText)
{
	ioText.AppendCString( "<span");
}

void VSpanTextParser::_CloseTagSpan(VString& ioText, bool inAfterPlainText)
{
	if (inAfterPlainText)
		ioText.AppendCString( "</span>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagDoc(VString& ioText)
{
	ioText.AppendCString( "<span"); //in kDOC_VERSION_SPAN4D_1 root tag is a <span>
}

void VSpanTextParser::_CloseTagDoc(VString& ioText, bool inAfterPlainText)
{
	if (inAfterPlainText)
		ioText.AppendCString( "</span>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagParagraph(VString& ioText)
{
	ioText.AppendCString( "<span"); //in kDOC_VERSION_SPAN4D_2 paragraph tag is a <span>
}

void VSpanTextParser::_CloseTagParagraph(VString& ioText, bool inAfterPlainText)
{
	if (inAfterPlainText)
		ioText.AppendCString( "</span>");
	else
		ioText.AppendCString( ">");
}


void VSpanTextParser::_SerializeParagraphTextAndStyles( const VDocNode *inDocNode, const VTreeTextStyle *inStyles, const VString& inPlainText, VString &ioText, 
														const VTextStyle *_inStyleInherit, bool inSkipCharStyles)
{
	if (!inStyles)
	{
		if (!inPlainText.IsEmpty())
		{
			VString text( inPlainText);
			TextToXML(text, true, true, true);
			ioText.AppendString(text);
		}
		return;
	}
	sLONG start, end;
	const VTextStyle *style = inStyles->GetData();
	style->GetRange( start, end);
	if (start >= end) 
		return;

	sLONG childCount = inStyles->GetChildCount();

	bool hasTag = false;
	const VTextStyle *styleInherit = NULL;
	VTextStyle *styleInheritForChildren = NULL;
	bool doDeleteStyleInherit = false;
	bool needToEmbedSpanRef = false;
	bool isUnknownTag = style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_UnknownTag;
	bool isURL = style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_URL;

	if (!style->IsUndefined())
	{
		styleInherit = _inStyleInherit;
		if (!styleInherit)
		{
			doDeleteStyleInherit = true;
			VTextStyle *newstyle = new VTextStyle();
			newstyle->SetBold( 0);
			newstyle->SetItalic( 0);
			newstyle->SetStrikeout( 0);
			newstyle->SetUnderline( 0);
			newstyle->SetJustification( JST_Default);
			styleInherit = newstyle;
		}
		styleInheritForChildren = new VTextStyle( styleInherit);
		styleInheritForChildren->MergeWithStyle( style);
		if ((style->IsSpanRef() && !isUnknownTag) || (!inSkipCharStyles && !styleInheritForChildren->IsEqualTo(*styleInherit, false))) //always serialize span refs, otherwise serialize only if some style overrides some inherited style
		{
			//open span tag & serialize styles

			fIsOnlyPlainText = false;

			hasTag = true;
			bool hasStyle = !inSkipCharStyles && !style->IsUndefined(true);

			if ((isUnknownTag || isURL) && hasStyle)
			{
				//we embed span ref tag inside a span tag as character style is only appliable to a span tag

				needToEmbedSpanRef = true;
				_OpenTagSpan( ioText); //open span tag container
			}
			else 
			{
				if (isUnknownTag)
					hasTag = false;
				else if (isURL)
				{
					ioText.AppendCString( "<a");
					SerializeAttributeHRef( inDocNode, style->GetSpanRef(), ioText);
				}
				else
				{
					if (!hasStyle)
						hasStyle = style->IsSpanRef(); //for 4D exp or user link, we need to serialize -d4-ref or -d4-ref-user properties
					_OpenTagSpan( ioText);
				}
			}

			if (hasStyle)
			{
				ioText.AppendCString(" style=\"");

				//serialize CSS character styles

				_SerializeVTextStyleToCSS( inDocNode, *style, ioText, styleInherit);

				//close style 

				ioText.AppendUniChar('\"');
			}


			if (needToEmbedSpanRef)
			{
				_CloseTagSpan(ioText, false); //close span container opening tag

				if (!isUnknownTag)
				{
					//open span ref tag
					if (style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_URL)
					{
						isURL = true;
						ioText.AppendCString( "<a");
						SerializeAttributeHRef( inDocNode, style->GetSpanRef(), ioText);
					}
					else 
						_OpenTagSpan( ioText);
				}
			}

			if (!isUnknownTag)
				_CloseTagSpan(ioText, false); //only close span opening tag
		}
		if (doDeleteStyleInherit)
			delete styleInherit;
	}
	
	if (childCount > 0 && !style->IsSpanRef())
	{
		for (int i = 0; i < childCount; i++)
		{
			VTreeTextStyle *childStyles = inStyles->GetNthChild( i+1);
			sLONG childStart, childEnd;
			childStyles->GetData()->GetRange( childStart, childEnd);
			if (start < childStart)
			{
				//serialize text between children
				VString text;
				inPlainText.GetSubString( start+1, childStart-start, text);
				TextToXML(text, true, true, true);
				ioText.AppendString(text);
			}
			_SerializeParagraphTextAndStyles( inDocNode, childStyles, inPlainText, ioText, styleInheritForChildren ? styleInheritForChildren : _inStyleInherit);
			start = childEnd;
		}
	}

	if (style->IsSpanRef())
	{
		switch (style->GetSpanRef()->GetType())
		{
		case kSpanTextRef_URL:	 //default value is tag plain text = url label
		case kSpanTextRef_User:  //default value is tag plain text = user label
		case kSpanTextRef_4DExp: //default value is NBSP
			{
			fIsOnlyPlainText = false;

			VString text;
			text = style->GetSpanRef()->GetDefaultValue(); 
			if (text.IsEmpty() || style->GetSpanRef()->GetType() == kSpanTextRef_4DExp)
				text = VDocSpanTextRef::fNBSP;
			TextToXML(text, true, true, true);
			ioText.AppendString( text);
			}
			break;
		case kSpanTextRef_UnknownTag:
			{
			fIsOnlyPlainText = false;

			ioText.AppendString( style->GetSpanRef()->GetRef()); //serialize unknown tag and children tags (already serialized in span reference)
			}
			break;
		default:
			break;
		}
	}
	else if (start < end)
	{
		//serialize remaining text
		VString text;
		inPlainText.GetSubString( start+1, end-start, text);
		TextToXML(text, true, true, true);
		ioText.AppendString(text);
	}

	if (styleInheritForChildren)
		delete styleInheritForChildren;

	if (hasTag)
	{
		fIsOnlyPlainText = false;

		//close tag span
		if (!isUnknownTag)
		{
			if (style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_URL)
				ioText.AppendCString( "</a>");
			else
				_CloseTagSpan(ioText, true);
		}
		
		if (needToEmbedSpanRef)
			_CloseTagSpan(ioText, true); //close span container
	}
}


/** initialize VDocNode props from CSS declaration string */ 
void VSpanTextParser::ParseCSSToVDocNode( VDocNode *ioDocNode, const VString& inCSSDeclarations)
{
	if (!testAssert(ioDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D_1))
		return; //not implemented yet

	xbox_assert(ioDocNode && ioDocNode->GetDocumentNode());
	xbox_assert(ioDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT); //we parse document & paragraph properties only for root node

	//prepare inline CSS parsing

	VCSSLexParser *lexParser = new VCSSLexParser();

	VCSSParserInlineDeclarations cssParser(true);
	cssParser.Start(inCSSDeclarations);
	VString attribut, value;

	//prepare paragraph parsing

	VDocParagraph *para = NULL;
	VTreeTextStyle *styles = NULL;
	VTextStyle *style = NULL;

	para = ioDocNode->GetDocumentNodeForWrite()->RetainFirstParagraph();
	xbox_assert(para);

	styles = para->RetainStyles();
	if (!styles)
	{
		styles = new VTreeTextStyle( new VTextStyle());
		para->SetStyles( styles, false);
	}
	xbox_assert(styles);
	style = styles->GetData();

	while (cssParser.GetNextAttribute(attribut, value))
	{
#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
		if (attribut == kCSS_NAME_D4_DPI)
			//CSS d4-dpi
			_ParsePropDoc(lexParser, ioDocNode->GetDocumentNodeForWrite(), kDOC_PROP_DPI, value);
		else if (attribut == kCSS_NAME_D4_VERSION)
			//CSS d4-version
			_ParsePropDoc(lexParser, ioDocNode->GetDocumentNodeForWrite(), kDOC_PROP_VERSION, value);
		else if (attribut == kCSS_NAME_D4_ZOOM)
			//CSS d4-zoom
			_ParsePropDoc(lexParser, ioDocNode->GetDocumentNodeForWrite(), kDOC_PROP_ZOOM, value);
#endif
		//span character properties  
		if (attribut == kCSS_NAME_FONT_FAMILY)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_FAMILY, value, *style, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_FONT_SIZE)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_SIZE, value, *style, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_COLOR)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_COLOR, value, *style, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_FONT_STYLE)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_STYLE, value, *style, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_TEXT_DECORATION)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_TEXT_DECORATION, value, *style, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_FONT_WEIGHT)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_WEIGHT, value, *style, NULL, NULL, NULL);
#if kDOC_VERSION_MAX_SUPPORTED <= kDOC_VERSION_SPAN4D_1
		else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
			_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_BACKGROUND_COLOR, value, *style, NULL, NULL, NULL);
#endif

		//paragraph node common properties
		else if (attribut == kCSS_NAME_TEXT_ALIGN)
			_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_TEXT_ALIGN, value);
#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
		else if (attribut == kCSS_NAME_VERTICAL_ALIGN)
			_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_VERTICAL_ALIGN, value);
		else if (attribut == kCSS_NAME_MARGIN)
			_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_MARGIN, value);
		else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
			_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_BACKGROUND_COLOR, value); //here it is element back color
#endif

		//paragraph node private properties
#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
		else if (attribut == kCSS_NAME_DIRECTION)
			_ParsePropParagraph(lexParser, para, kDOC_PROP_DIRECTION, value);
		else if (attribut == kCSS_NAME_LINE_HEIGHT)
			_ParsePropParagraph(lexParser, para, kDOC_PROP_LINE_HEIGHT, value);
		else if (attribut == kCSS_NAME_D4_PADDING_FIRST_LINE)
			_ParsePropParagraph(lexParser, para, kDOC_PROP_PADDING_FIRST_LINE, value);
		else if (attribut == kCSS_NAME_D4_TAB_STOP_OFFSET)
			_ParsePropParagraph(lexParser, para, kDOC_PROP_TAB_STOP_OFFSET, value);
		else if (attribut == kCSS_NAME_D4_TAB_STOP_TYPE)
			_ParsePropParagraph(lexParser, para, kDOC_PROP_TAB_STOP_TYPE, value);
#endif
	}

	ReleaseRefCountable(&styles);

	delete lexParser;
}


/** serialize VDocNode props to CSS declaration string */ 
void VSpanTextParser::_SerializeVDocNodeToCSS(  const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault, bool inIsFirst)
{
	xbox_assert(inDocNode && inDocNode->GetDocumentNode());
	
	VIndex count = inIsFirst ? 0 : 1;

	if (inDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT)
	{
		//serialize document properties (this document node contains only document properties)

		//CSS d4-version
		if (inDocNode->GetDocumentNode()->GetVersion() > kDOC_VERSION_SPAN4D_1) 
		{
			bool isOverriden = inDocNode->IsOverriden( kDOC_PROP_VERSION) && !inDocNode->IsInherited( kDOC_PROP_VERSION);
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_VERSION);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)(inDocNode->GetDocumentNode()->GetVersion() < kDOC_VERSION_SPAN4D_1 ? kDOC_VERSION_SPAN4D_1 : inDocNode->GetDocumentNode()->GetVersion()));
				count++;
			}
			//CSS d4-dpi
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_DPI) && !inDocNode->IsInherited( kDOC_PROP_DPI);
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_DPI);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)inDocNode->GetDocumentNode()->GetDPI());
				count++;
			}

			//CSS d4-zoom
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_ZOOM) && !inDocNode->IsInherited( kDOC_PROP_ZOOM);
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_ZOOM);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)inDocNode->GetDocumentNode()->GetZoom());
				outCSSDeclarations.AppendUniChar('%');
				count++;
			}
		}
	}
	else if (inDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		//serialize paragraph properties (here only node common properties + paragraph-only properties) 

		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(inDocNode);
		xbox_assert(para);
		const VDocParagraph *paraDefault = inDocNodeDefault ? dynamic_cast<const VDocParagraph *>(inDocNodeDefault) : NULL;


		//serialize common properties

		VIndex length = outCSSDeclarations.GetLength();
		_SerializePropsCommon( inDocNode, outCSSDeclarations, inDocNodeDefault);
		if (length != outCSSDeclarations.GetLength())
			count++;

		//serialize paragraph-only properties

#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
		//CSS direction
		bool isOverriden = inDocNode->IsOverriden( kDOC_PROP_DIRECTION);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_DIRECTION))
			{
				//inherited on default
			}
			else
			{
				eDocPropDirection direction = para->GetDirection();

				if (!(paraDefault && direction == paraDefault->GetDirection()))
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_DIRECTION);
					outCSSDeclarations.AppendUniChar( ':');

					switch (direction)
					{
						case kTEXT_DIRECTION_LTR:
							outCSSDeclarations.AppendString("ltr");
							break;
						default:
							outCSSDeclarations.AppendString("rtl");
							break;
					}
					count++;
				}
			}
		}

		//CSS line-height
		isOverriden = inDocNode->IsOverriden( kDOC_PROP_LINE_HEIGHT);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LINE_HEIGHT))
			{
				//inherited on default
			}
			else
			{
				sLONG lineHeight = para->GetLineHeight();

				if (!(paraDefault && lineHeight == paraDefault->GetLineHeight()))
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_LINE_HEIGHT);
					outCSSDeclarations.AppendUniChar( ':');

					if (lineHeight == kDOC_PROP_LINE_HEIGHT_NORMAL)
						outCSSDeclarations.AppendCString( "normal");
					else if (lineHeight >= 0)
					{
						//fixed line height in TWIPS
						VString sValue;
						_TWIPSToCSSDimPoint( lineHeight, sValue);
						outCSSDeclarations.AppendString( sValue);
					}
					else
					{
						//percentage
						outCSSDeclarations.AppendLong( -lineHeight);
						outCSSDeclarations.AppendUniChar( '%');
					}
					count++;
				}
			}
		}

		//CSS d4-padding-first-line
		isOverriden = inDocNode->IsOverriden( kDOC_PROP_PADDING_FIRST_LINE);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_PADDING_FIRST_LINE))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_D4_PADDING_FIRST_LINE);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				sLONG padding = para->GetPaddingFirstLine();

				if (!(paraDefault && padding == paraDefault->GetPaddingFirstLine()))
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_PADDING_FIRST_LINE);
					outCSSDeclarations.AppendUniChar( ':');

					VString sValue;
					_TWIPSToCSSDimPoint( padding, sValue);
					outCSSDeclarations.AppendString( sValue);
					count++;
				}
			}
		}


		//CSS d4-tab-stop-offset
		isOverriden = inDocNode->IsOverriden( kDOC_PROP_TAB_STOP_OFFSET);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_TAB_STOP_OFFSET))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_D4_TAB_STOP_OFFSET);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				uLONG offset = para->GetTabStopOffset();

				if (!(paraDefault && offset == paraDefault->GetTabStopOffset()))
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_TAB_STOP_OFFSET);
					outCSSDeclarations.AppendUniChar( ':');

					VString sValue;
					_TWIPSToCSSDimPoint( (sLONG)offset, sValue);
					outCSSDeclarations.AppendString( sValue);
					count++;
				}
			}
		}

		//CSS d4-tab-stop-type
		isOverriden = inDocNode->IsOverriden( kDOC_PROP_TAB_STOP_TYPE);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_TAB_STOP_TYPE))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_D4_TAB_STOP_TYPE);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				eDocPropTabStopType type = para->GetTabStopType();

				if (!(paraDefault && type == paraDefault->GetTabStopType()))
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_TAB_STOP_TYPE);
					outCSSDeclarations.AppendUniChar( ':');

					switch (type)
					{
						case kTEXT_TAB_STOP_TYPE_LEFT:
							outCSSDeclarations.AppendCString("left");
							break;
						case kTEXT_TAB_STOP_TYPE_RIGHT:
							outCSSDeclarations.AppendCString("right");
							break;
						case kTEXT_TAB_STOP_TYPE_CENTER:
							outCSSDeclarations.AppendCString("center");
							break;
						case kTEXT_TAB_STOP_TYPE_DECIMAL:
							outCSSDeclarations.AppendCString("decimal");
							break;
						default:
							outCSSDeclarations.AppendCString("bar");
							break;
					}
					count++;
				}
			}
		}
#endif
	}
}	


/** convert TWIPS value to CSS dimension value (in point) */
void VSpanTextParser::_TWIPSToCSSDimPoint( const sLONG inValueTWIPS, VString& outValueCSS)
{
	Real number = ICSSUtil::TWIPSToPoint( inValueTWIPS);
	outValueCSS.FromReal( number);
	outValueCSS.Exchange(',','.');
	outValueCSS.AppendCString( "pt");
}

/** create paragraph default character styles */
VTextStyle *VSpanTextParser::_CreateParagraphDefaultCharStyles( const VDocParagraph *inDocPara)
{
	VTextStyle *styleDefault = new VTextStyle();
	styleDefault->SetBold( 0);
	styleDefault->SetItalic( 0);
	styleDefault->SetStrikeout( 0);
	styleDefault->SetUnderline( 0);
	styleDefault->SetJustification( JST_Default);
	styleDefault->SetHasBackColor(true);
	styleDefault->SetBackGroundColor( inDocPara->GetBackgroundColor()); //to avoid character background color to be set to paragraph background color (useless)
	return styleDefault;
}


/** parse document CSS property */	
bool VSpanTextParser::_ParsePropDoc( VCSSLexParser *inLexParser, VDocText *ioDocText, const eDocProperty inProperty, const VString& inValue)
{
	bool result = false;

	bool doDeleteLexParser = false;
	if (inLexParser == NULL)
	{
		inLexParser = new VCSSLexParser();
		doDeleteLexParser = true;
	}

	if (inProperty == kDOC_PROP_DPI)
	{
		//CSS d4-dpi

		uLONG dpi = inValue.GetLong();
		if (dpi >= 72 && dpi <= 1200)
		{
			ioDocText->SetDPI(dpi);
			result = true;
		}
	}
	else if (inProperty == kDOC_PROP_VERSION)
	{
		//CSS d4-version

		uLONG version = (uLONG)inValue.GetLong();
		ioDocText->SetVersion(version >= kDOC_VERSION_SPAN4D_1 && version <= kDOC_VERSION_MAX ? version : kDOC_VERSION_XHTML11);
		result = true;
	}
	else if (inProperty == kDOC_PROP_ZOOM)
	{
		//CSS d4-zoom

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		uLONG zoom = 0;
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				xbox_assert(false);
			else
			{
				if (valueCSS->v.css.fLength.fAuto)
					xbox_assert(false);
				else
				{
					zoom = (uLONG)floor(valueCSS->v.css.fLength.fNumber * 100 + 0.5f);
					result = true;
				}
			}
		}
		ReleaseRefCountable(&valueCSS);

		if (result)
		{
			if (zoom < 25)
				zoom = 25; //avoid too small zoom
			if (zoom > 400)
				zoom  = 400; //avoid too big zoom
			ioDocText->SetZoom(zoom);
		}
	}

	if (doDeleteLexParser)
		delete inLexParser;
	return result;
}

/** get node font size (in TWIPS) */
sLONG VSpanTextParser::_GetFontSize( const VDocNode *inDocNode)
{
	if (inDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
	{
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(inDocNode);
		xbox_assert(para);
		return _GetParagraphFontSize( para);
	}
	else
		return 12*20;
}

/** get paragraph font size (in TWIPS) */
sLONG VSpanTextParser::_GetParagraphFontSize( const VDocParagraph *inDocPara)
{
	sLONG fontsize = 12*20;
	if (inDocPara->GetStyles())
	{
		Real rfontsize = inDocPara->GetStyles()->GetData()->GetFontSize();
		if (rfontsize > 0)
			fontsize = ICSSUtil::PointToTWIPS(rfontsize);
	}
	return fontsize;
}


/** parse paragraph CSS property */	
bool VSpanTextParser::_ParsePropParagraph( VCSSLexParser *inLexParser, VDocParagraph *ioDocPara, const eDocProperty inProperty, const VString& inValue)
{
	bool result = false;

	bool doDeleteLexParser = false;
	if (inLexParser == NULL)
	{
		inLexParser = new VCSSLexParser();
		doDeleteLexParser = true;
	}

	if (inProperty == kDOC_PROP_DIRECTION)
	{
		//CSS direction

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::DIRECTION, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocPara->SetInherit( kDOC_PROP_DIRECTION);
			else
				ioDocPara->SetDirection((eDocPropDirection)valueCSS->v.css.fDirection);
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LINE_HEIGHT)
	{
		//CSS line-height

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LINEHEIGHT, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocPara->SetInherit( kDOC_PROP_LINE_HEIGHT);
			else
			{
				if (valueCSS->v.css.fLength.fAuto)
					ioDocPara->SetLineHeight( kDOC_PROP_LINE_HEIGHT_NORMAL);
				else if (valueCSS->v.css.fLength.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
					ioDocPara->SetLineHeight( -floor(valueCSS->v.css.fLength.fNumber * 100 + 0.5f));
				else
					ioDocPara->SetLineHeight( ICSSUtil::CSSDimensionToTWIPS( valueCSS->v.css.fLength.fNumber, 
																			(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																			ioDocPara->GetDocumentNode()->GetDPI(),
																			_GetParagraphFontSize(ioDocPara)));
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_PADDING_FIRST_LINE)
	{
		//CSS d4-padding-first-line

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::COORD, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocPara->SetInherit( kDOC_PROP_PADDING_FIRST_LINE);
				result = true;
			}
			else
			{
				if (valueCSS->v.css.fLength.fAuto || valueCSS->v.css.fLength.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
				{
					xbox_assert(false); //forbidden
				}
				else
				{
					ioDocPara->SetPaddingFirstLine( ICSSUtil::CSSDimensionToTWIPS( valueCSS->v.css.fLength.fNumber, 
																			(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																			ioDocPara->GetDocumentNode()->GetDPI(),
																			_GetParagraphFontSize(ioDocPara)));
					result = true;
				}
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_TAB_STOP_OFFSET)
	{
		//CSS d4-tab-stop-offset

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocPara->SetInherit( kDOC_PROP_TAB_STOP_OFFSET);
				result = true;
			}
			else
			{
				if (valueCSS->v.css.fLength.fAuto || valueCSS->v.css.fLength.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
				{
					xbox_assert(false); //forbidden
				}
				else
				{
					ioDocPara->SetTabStopOffset( (uLONG)ICSSUtil::CSSDimensionToTWIPS(	valueCSS->v.css.fLength.fNumber, 
																						(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																						ioDocPara->GetDocumentNode()->GetDPI(),
																						_GetParagraphFontSize(ioDocPara)));
					result = true;
				}
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_TAB_STOP_TYPE)
	{
		//CSS d4-tab-stop-type

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::TABSTOPTYPE, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocPara->SetInherit( kDOC_PROP_TAB_STOP_TYPE);
			else
				ioDocPara->SetTabStopType((eDocPropTabStopType)valueCSS->v.css.fTabStopType);
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}

	if (doDeleteLexParser)
		delete inLexParser;
	return result;
}



/** parse common CSS property */

const VString kCSS_IN					= CVSTR("in");
const VString kCSS_CM					= CVSTR("cm");
const VString kCSS_MM					= CVSTR("mm");
const VString kCSS_PT					= CVSTR("pt");
const VString kCSS_PC					= CVSTR("pc");
const VString kCSS_PX					= CVSTR("px");
const VString kCSS_EM					= CVSTR("em");
const VString kCSS_EX					= CVSTR("ex");
const VString kCSS_PERCENT				= CVSTR("%");

const VString kCSS_INHERIT				= CVSTR("inherit");	

const VString kCSS_AUTO					= CVSTR("auto");

bool VSpanTextParser::_ParsePropCommon( VCSSLexParser *inLexParser, VDocNode *ioDocNode, const eDocProperty inProperty, const VString& inValue)
{
	bool result = false;

	bool doDeleteLexParser = false;
	if (inLexParser == NULL)
	{
		inLexParser = new VCSSLexParser();
		doDeleteLexParser = true;
	}

	else if (inProperty == kDOC_PROP_TEXT_ALIGN)
	{
		//CSS text-align

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::TEXTALIGN, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_TEXT_ALIGN);
			else
				switch(valueCSS->v.css.fTextAlign)
				{
				case CSSProperty::kTEXT_ALIGN_LEFT:
					ioDocNode->SetTextAlign( (eDocPropTextAlign)JST_Left);
					break;
				case CSSProperty::kTEXT_ALIGN_RIGHT:
					ioDocNode->SetTextAlign( (eDocPropTextAlign)JST_Right);
					break;
				case CSSProperty::kTEXT_ALIGN_CENTER:
					ioDocNode->SetTextAlign( (eDocPropTextAlign)JST_Center);
					break;
				case CSSProperty::kTEXT_ALIGN_JUSTIFY:
					ioDocNode->SetTextAlign( (eDocPropTextAlign)JST_Justify);
					break;
				default:
					xbox_assert(false);
					break;
				}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_VERTICAL_ALIGN)
	{
		//CSS vertical-align

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::VERTICALALIGN, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_VERTICAL_ALIGN);
			else
				switch(valueCSS->v.css.fVerticalAlign)
				{
				case CSSProperty::kTEXT_ALIGN_BOTTOM:
				case CSSProperty::kTEXT_ALIGN_TEXT_BOTTOM:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Bottom);
					break;
				case CSSProperty::kTEXT_ALIGN_MIDDLE:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Center);
					break;
				default:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Top);
					break;
				}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_MARGIN)
	{
		//CSS margin (remark: percentage values are forbidden)

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);

		std::vector<sLONG> margin;
		bool done = false;
		bool inherit = false;
		bool automatic = false;
		bool error = false;
		while (!done && inLexParser->GetCurToken() != CSSToken::END)
		{
			switch( inLexParser->GetCurToken())
			{
			case CSSToken::NUMBER:
				if (inLexParser->GetCurTokenValueNumber() < 0)
					error = done = true;
				else
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS(inLexParser->GetCurTokenValueNumber(), kCSSUNIT_PX, ioDocNode->GetDocumentNode()->GetDPI()));
				break;
			case CSSToken::PERCENTAGE:
				{
				xbox_assert(false); //forbidden
				error = done = true;
				}
				break;
			case CSSToken::DIMENSION:
				if (inLexParser->GetCurTokenValueNumber() < 0)
					error = done = true;
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PX, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_PX, ioDocNode->GetDocumentNode()->GetDPI()));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PT, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_PT));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_IN, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_IN));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_CM, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_CM));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_MM, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_MM));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PC, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_PC));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EM, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_EM, 72, _GetFontSize( ioDocNode)));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_EX, false))
					margin.push_back( ICSSUtil::CSSDimensionToTWIPS( inLexParser->GetCurTokenValueNumber(), kCSSUNIT_EX, 72, _GetFontSize( ioDocNode)));
				else if (inLexParser->GetCurTokenValueIdent().EqualToString( kCSS_PERCENT, false))
				{
					xbox_assert(false); //forbidden
					error = done = true;
				}
				else
					error = done = true;
				break;
			case CSSToken::IDENT:
				{
				if (inLexParser->GetCurTokenValue().EqualToString( kCSS_INHERIT, false))
					inherit = done = true;
				else if (inLexParser->GetCurTokenValue().EqualToString( kCSS_AUTO, false))
					automatic = done = true;
				else
					error = done = true;
				}
				break;
			case CSSToken::SEMI_COLON:
			case CSSToken::RIGHT_CURLY_BRACE:
			case CSSToken::IMPORTANT_SYMBOL:
				done = true;
				break;
			default:
				error = done = true;
				break;
			}
			if (done)
				break;
			inLexParser->Next( true); 
		} 
		if (!error)
		{
			if (inherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_MARGIN);
				result = true;
			}
			else if (automatic)
			{
				ioDocNode->RemoveProp( kDOC_PROP_MARGIN); //reset to default margin
				result = true;
			}
			else if (margin.size() >= 1 && margin.size() <= 4)
			{
				//parse top, right, bottom & left margins 

				sDocPropPaddingMargin smargin;
				if (margin.size() == 4)
				{
					smargin.top = margin[0];
					smargin.right = margin[1];
					smargin.bottom = margin[2];
					smargin.left = margin[3];
				}
				else if (margin.size() == 3)
				{
					smargin.top = margin[0];
					smargin.right = margin[1];
					smargin.bottom = margin[2];
					smargin.left = smargin.right;
				}
				else if (margin.size() == 2)
				{
					smargin.top = margin[0];
					smargin.right = margin[1];
					smargin.bottom = smargin.top;
					smargin.left = smargin.right;
				}
				else
				{
					smargin.top = margin[0];
					smargin.right = margin[0];
					smargin.bottom = margin[0];
					smargin.left = margin[0];
				}
				ioDocNode->SetMargin( smargin);
				result = true;
			}
		}
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_COLOR)
	{
		//CSS background-color (here it is element background color - not character back color)

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::COLOR, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_COLOR);
				result = true;
			}
			else
			{
				const CSSProperty::Color *color = &(valueCSS->v.css.fColor);
				if (color->fTransparent)
				{
					ioDocNode->SetBackgroundColor( RGBACOLOR_TRANSPARENT);
					result = true;
				}
				else if (testAssert(!color->fInvert))
				{
					ioDocNode->SetBackgroundColor( color->fColor);
					result = true;
				}
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	if (doDeleteLexParser)
		delete inLexParser;
	return result;
}

/** serialize common CSS properties to CSS styles */
void VSpanTextParser::_SerializePropsCommon( const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault, bool inIsFirst)
{
	VIndex count = inIsFirst ? 0 : 1;
	
	//CSS text-align
	bool isOverriden = inDocNode->IsOverriden( kDOC_PROP_TEXT_ALIGN);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_TEXT_ALIGN))
		{
			//text-align is inherited on default so it is useless to serialize "inherit"
		}
		else
		{
			eDocPropTextAlign textAlign = inDocNode->GetTextAlign();
			if ( 
				textAlign != JST_Default 
				&& 
				textAlign != JST_Mixed
				&&
				(!inDocNodeDefault || (!(textAlign == inDocNodeDefault->GetTextAlign()))))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_TEXT_ALIGN);
				outCSSDeclarations.AppendUniChar( ':');

				switch (textAlign)
				{
					case JST_Right:
						outCSSDeclarations.AppendString("right");
						break;
					case JST_Center:
						outCSSDeclarations.AppendString("center");
						break;
					case JST_Justify:
						outCSSDeclarations.AppendString("justify");
						break;
					default:
						outCSSDeclarations.AppendString("left");
						break;
				}
				count++;
			}
		}
	}

#if kDOC_VERSION_MAX_SUPPORTED > kDOC_VERSION_SPAN4D_1
	//CSS vertical-align
	isOverriden = inDocNode->IsOverriden( kDOC_PROP_VERTICAL_ALIGN);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_VERTICAL_ALIGN))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_VERTICAL_ALIGN);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			eDocPropTextAlign textAlign = inDocNode->GetVerticalAlign();
			if (textAlign != JST_Default 
				&& 
				textAlign != JST_Mixed
				&&
				(!inDocNodeDefault || (!(textAlign == inDocNodeDefault->GetVerticalAlign()))))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_VERTICAL_ALIGN);
				outCSSDeclarations.AppendUniChar( ':');

				switch (textAlign)
				{
					case JST_Bottom:
						outCSSDeclarations.AppendString("bottom");
						break;
					case JST_Center:
						outCSSDeclarations.AppendString("middle");
						break;
					default:
						outCSSDeclarations.AppendString("top");
						break;
				}
				count++;
			}
		}
	}
	//CSS margin
	isOverriden = inDocNode->IsOverriden( kDOC_PROP_MARGIN);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_MARGIN))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_MARGIN);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			//CSS order: top, right, bottom & left 
			bool doIt = true;
			sDocPropPaddingMargin margin = inDocNode->GetMargin();
			if (inDocNodeDefault)
			{
				sDocPropPaddingMargin marginDefault = inDocNodeDefault->GetMargin();
				if (margin.left == marginDefault.left
					&&
					margin.top == marginDefault.top
					&&
					margin.right == marginDefault.right
					&&
					margin.bottom == marginDefault.bottom)
					doIt = false;
			}
			if (doIt)
			{
				Real top, right, bottom, left;
				Real number[4];
				number[0] = ICSSUtil::TWIPSToPoint( margin.top);
				number[1] = ICSSUtil::TWIPSToPoint( margin.right);
				number[2] = ICSSUtil::TWIPSToPoint( margin.bottom);
				number[3] = ICSSUtil::TWIPSToPoint( margin.left);
				
				VIndex numDim = 4;
				if (margin.left == margin.right)
				{
					numDim = 3;
					if (margin.bottom == margin.top)
					{
						numDim = 2;
						if (margin.right == margin.top)
							numDim = 1;
					}
				}

				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_MARGIN);
				outCSSDeclarations.AppendUniChar( ':');

				for (int i = 0; i < numDim; i++)
				{
					if (i > 0)
						outCSSDeclarations.AppendUniChar(' ');
					VString sValue;
					_TWIPSToCSSDimPoint( number[i], sValue);
					outCSSDeclarations.AppendString( sValue);
				}
				count++;
			}
		}
	}
	//CSS background-color (here element background color, not character back color)
	isOverriden = inDocNode->IsOverriden( kDOC_PROP_BACKGROUND_COLOR);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_COLOR))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_COLOR);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			RGBAColor color = inDocNode->GetBackgroundColor();
			if (!(inDocNodeDefault && color == inDocNodeDefault->GetBackgroundColor()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_COLOR);
				outCSSDeclarations.AppendUniChar( ':');
				
				if (color == RGBACOLOR_TRANSPARENT)
					outCSSDeclarations.AppendString( "transparent");
				else
				{
					VString scolor;
					ColorToValue( color, scolor);

					outCSSDeclarations.AppendString( scolor);
				}
				count++;
			}
		}
	}
#endif
}

bool VSpanTextParser::SerializeAttributeHRef( const VDocNode *ioDocNode, const VDocSpanTextRef *inSpanRef, VString& outText)
{
	if (testAssert(inSpanRef))
	{
		if (inSpanRef->GetType() == kSpanTextRef_URL)
		{
			outText.AppendCString(" href=\"");
			VString url = inSpanRef->GetRef();
			TextToXML( url, true, true, true, false, true); //end of line should be escaped if any
			outText.AppendString( url);
			outText.AppendUniChar('\"');
			return true;
		}
	}
	return false;
}

bool VSpanTextParser::ParseAttributeHRef( VDocNode *ioDocNode, const VString& inValue, VTextStyle& outStyle)
{
	VString value = inValue;
	value.TrimeSpaces();
	if (!value.IsEmpty())
	{
		value.ConvertCarriageReturns(eCRM_CR);
		VDocSpanTextRef *spanRef = new VDocSpanTextRef( kSpanTextRef_URL,
														value,
														VDocSpanTextRef::fNBSP);
		outStyle.SetSpanRef( spanRef);
		return true;
	}
	return false;
}

/** parse span CSS style */	
bool VSpanTextParser::_ParsePropSpan( VCSSLexParser *inLexParser, VDocNode *inDocNode, const eDocProperty inProperty, 
									  const VString& inValue, VTextStyle &outStyle, 
									  bool *outBackColorInherit, bool *outTextDecorationInherit, const VTextStyle *inStyleInherit, VDBLanguageContext *inLC)
{
	bool result = false;

	bool doDeleteLexParser = false;
	if (inLexParser == NULL)
	{
		inLexParser = new VCSSLexParser();
		doDeleteLexParser = true;
	}

	if (inProperty == kDOC_PROP_FONT_FAMILY)
	{
		//CSS font-family

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::FONTFAMILY, NULL, true);
		if (valueCSS && !valueCSS->fInherit && valueCSS->v.css.fFontFamily)
		{
			//TODO: VTextStyle should store a vector of font-family names & while rendering the first existing font should be used (as in SVG component) ?
			if (valueCSS->v.css.fFontFamily)
			{
				outStyle.SetFontName( valueCSS->v.css.fFontFamily->front());
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_FONT_SIZE)
	{
		//CSS font-size

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::FONTSIZE, NULL, true);
		const CSSProperty::Length* length = valueCSS ? &(valueCSS->v.css.fLength) : NULL;
		if (valueCSS && !valueCSS->fInherit && !length->fAuto)
		{
			if (length->fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
			{
				//relative to parent or default font size
				if (inStyleInherit && inStyleInherit->GetFontSize() >= 0)
					outStyle.SetFontSize( floor(inStyleInherit->GetFontSize()*length->fNumber+0.5f));
				else
					outStyle.SetFontSize( floor(12.0f*length->fNumber+0.5f));
			}
			else
			{
				Real fontSize = ICSSUtil::CSSDimensionToPoint(	length->fNumber, 
																(eCSSUnit)length->fUnit, 
																inDocNode->GetDocumentNode()->GetDPI(), 
																inStyleInherit && inStyleInherit->GetFontSize() >= 0 ?
																ICSSUtil::PointToTWIPS(inStyleInherit->GetFontSize()) : 12*20
																);
#if VERSIONWIN
				uLONG version = inDocNode->GetDocumentNode()->GetVersion();
				if (version == kDOC_VERSION_SPAN4D_1)
					fontSize = floor(fontSize*VSpanTextParser::Get()->GetSPANFontSizeDPI()/72.0f+0.5f);
#else
				fontSize = floor(fontSize+0.5f);
#endif
				outStyle.SetFontSize( fontSize);
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_COLOR)
	{
		//CSS color

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::COLOR, NULL, true);
		if (valueCSS && !valueCSS->fInherit)
		{
			const CSSProperty::Color *color = &(valueCSS->v.css.fColor);
			if (testAssert(!color->fInvert && !color->fTransparent))
			{
				outStyle.SetHasForeColor(true);
				outStyle.SetColor( color->fColor);
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_COLOR)
	{
		//CSS background-color (here it is character background color)

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::COLOR, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				if (outBackColorInherit)
					*outBackColorInherit = true;
			}
			else
			{
				const CSSProperty::Color *color = &(valueCSS->v.css.fColor);
				if (color->fTransparent)
				{
					outStyle.SetHasBackColor(true);
					outStyle.SetBackGroundColor( RGBACOLOR_TRANSPARENT);
					result = true;
				}
				else if (testAssert(!color->fInvert))
				{
					outStyle.SetHasBackColor(true);
					outStyle.SetBackGroundColor( color->fColor);
					result = true;
				}
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_FONT_STYLE)
	{
		//CSS font-style

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::FONTSTYLE, NULL, true);
		if (valueCSS && !valueCSS->fInherit)
		{
			switch( valueCSS->v.css.fFontStyle)
			{
			case CSSProperty::kFONT_STYLE_ITALIC:
			case CSSProperty::kFONT_STYLE_OBLIQUE:
				outStyle.SetItalic(TRUE);
				break;
			default:
				outStyle.SetItalic(FALSE);
				break;
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_TEXT_DECORATION)
	{
		//CSS text-decoration

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::TEXTDECORATION, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				if (outTextDecorationInherit)
					*outTextDecorationInherit = true;
			}
			else
			{
				//as CSS property is defined we need to define both underline & line-through VTextStyle properties to ensure they are not inherited
				outStyle.SetUnderline( (valueCSS->v.css.fTextDecoration & CSSProperty::kFONT_STYLE_UNDERLINE) != 0 ? TRUE : FALSE);
				outStyle.SetStrikeout( (valueCSS->v.css.fTextDecoration & CSSProperty::kFONT_STYLE_LINETHROUGH) != 0 ? TRUE : FALSE);
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_FONT_WEIGHT)
	{
		//CSS font-weight

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::FONTWEIGHT, NULL, true);
		if (valueCSS && !valueCSS->fInherit)
		{
			outStyle.SetBold( valueCSS->v.css.fFontWeight >= CSSProperty::kFONT_WEIGHT_BOLD ? TRUE : FALSE);
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_TEXT_ALIGN)
	{
		//CSS text-align 
		//remark: now it is a element common property (because can only be applied to a paragraph for instance and not to a span range) 
		//so redirect to the right parser in order prop to be stored as a node prop & not a VTreeTextStyle prop

		VDocNode *para = NULL;
		if (inDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
			para = inDocNode;
		else
		{
			VDocNode *child = inDocNode->GetDocumentNodeForWrite()->RetainChild(0);
			xbox_assert(child && child->GetType() == kDOC_NODE_TYPE_PARAGRAPH);
			para = child;
		}
		_ParsePropCommon( inLexParser, para, kDOC_PROP_TEXT_ALIGN, inValue);
	}
	else if (inProperty == kDOC_PROP_4DREF || inProperty == kDOC_PROP_4DREF_USER)
	{
		//CSS -d4-ref -d4-ref-user

		VString value = inValue.GetCPointer();
		TextToXML( value, false, false, false, true, false); //Xerces parses escaped CSS sequences too
															 //so we need to escape again parsed '\' to ensure our CSS parser will properly parse it & not parse it as the escape char

		inLexParser->Start( value.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::STRING, NULL, true);
		if (valueCSS && !valueCSS->fInherit)
		{
			VString value = *(valueCSS->v.css.fString);
			value.TrimeSpaces();
			if (!value.IsEmpty())
			{
				value.ConvertCarriageReturns(eCRM_CR);
				VDocSpanTextRef *spanRef = new VDocSpanTextRef( inProperty == kDOC_PROP_4DREF ? kSpanTextRef_4DExp : kSpanTextRef_User,
																value,
																VDocSpanTextRef::fNBSP);
				if (inLC)
					spanRef->EvalExpression( inLC);
				outStyle.SetSpanRef( spanRef);
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}

	if (doDeleteLexParser)
		delete inLexParser;
	return result;
}


/** initialize VTextStyle from CSS declaration string */ 
void VSpanTextParser::ParseCSSToVTextStyle(  VDBLanguageContext *inLC, VDocNode *inDocNode, const VString& inCSSDeclarations, VTextStyle &outStyle, const VTextStyle *inStyleInherit)
{
	xbox_assert(inDocNode && inDocNode->GetDocumentNode());

	VCSSLexParser *lexParser = new VCSSLexParser();

	VCSSParserInlineDeclarations cssParser(true);
	cssParser.Start(inCSSDeclarations);
	VString attribut, value;

	//in W3C XHMTL 1.1, background-color & text-decoration are not inherited on default (cf W3C CSS)
	//but they are in 4D SPAN or XHTML private formats
	bool backgroundColorInherited = inDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11 ? false : true;
	bool textDecorationInherited = inDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11 ? false : true;

	while(cssParser.GetNextAttribute(attribut, value))
	{
		if (attribut == kCSS_NAME_FONT_FAMILY)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_FAMILY, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_FONT_SIZE)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_SIZE, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_COLOR)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_COLOR, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_BACKGROUND_COLOR, value, outStyle, &backgroundColorInherited, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_FONT_STYLE)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_STYLE, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_TEXT_DECORATION)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_TEXT_DECORATION, value, outStyle, NULL, &textDecorationInherited, inStyleInherit);
		else if (attribut == kCSS_NAME_FONT_WEIGHT)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_WEIGHT, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_TEXT_ALIGN)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_TEXT_ALIGN, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_D4_REF)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_4DREF, value, outStyle, NULL, NULL, NULL, inLC);
		else if (attribut == kCSS_NAME_D4_REF_USER)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_4DREF_USER, value, outStyle, NULL, NULL, NULL);
	}

	//apply W3C CSS pure rules if text is pure XHTML 1.1 (and not 4D SPAN or 4D XHTML)
	if (!backgroundColorInherited && !outStyle.GetHasBackColor())
	{
		//default is transparent
		outStyle.SetHasBackColor(true);
		outStyle.SetBackGroundColor( RGBACOLOR_TRANSPARENT);
	}
	if (!textDecorationInherited)
	{
		//default is none decoration
		if (outStyle.GetUnderline() == UNDEFINED_STYLE)
			outStyle.SetUnderline(0);
		if (outStyle.GetStrikeout() == UNDEFINED_STYLE)
			outStyle.SetStrikeout(0);
	}

	delete lexParser;
}

/** serialize VTextStyle to CSS declaration string */ 
void VSpanTextParser::_SerializeVTextStyleToCSS( const VDocNode *inDocNode, const VTextStyle &inStyle, VString& outCSSDeclarations, const VTextStyle *_inStyleInherit, bool inIgnoreColors, bool inIsFirst)
{
	outCSSDeclarations.EnsureSize( outCSSDeclarations.GetLength()+256); //for perf: avoid multiple memory alloc

	const VTextStyle *styleInherit = _inStyleInherit;
	if (!styleInherit)
	{
		VTextStyle *style = new VTextStyle();
		style->SetBold( 0);
		style->SetItalic( 0);
		style->SetStrikeout( 0);
		style->SetUnderline( 0);
		style->SetJustification( JST_Default);
		styleInherit = style;
	}

	VIndex count = inIsFirst ? 0 : 1;
	if (!inStyle.GetFontName().IsEmpty() && !inStyle.GetFontName().EqualToString( styleInherit->GetFontName(), true))
	{
		//font-family

		if (count)
			outCSSDeclarations.AppendUniChar(';');
		outCSSDeclarations.AppendCString( kCSS_NAME_FONT_FAMILY);
		outCSSDeclarations.AppendString( ":\'");
		outCSSDeclarations.AppendString(inStyle.GetFontName());
		outCSSDeclarations.AppendUniChar('\'');
		count++;
	}
	if (inStyle.GetFontSize() != -1 && inStyle.GetFontSize() != styleInherit->GetFontSize())
	{
		//font-size

		if (count)
			outCSSDeclarations.AppendUniChar(';');
		outCSSDeclarations.AppendCString( kCSS_NAME_FONT_SIZE);
		outCSSDeclarations.AppendUniChar( ':');

		Real rfontSize = inStyle.GetFontSize();
		VString fontsize;
		if (fmod((double)rfontSize, (double)1.0) == 0.0)
		{
#if VERSIONWIN
			uLONG version = inDocNode->GetDocumentNode()->GetVersion();
			if (version == kDOC_VERSION_SPAN4D_1)
			{
				//for compat with v13, we still need to store this nasty font size (screen resolution dependent)
				rfontSize = floor(rfontSize*72/VSpanTextParser::Get()->GetSPANFontSizeDPI()+0.5f);
			}
#endif
			fontsize.FromLong( (sLONG)rfontSize);
			outCSSDeclarations.AppendString( fontsize);
			outCSSDeclarations.AppendCString( "pt");
		}
		else
		{
#if VERSIONWIN
			uLONG version = inDocNode->GetDocumentNode()->GetVersion();
			if (version == kDOC_VERSION_SPAN4D_1)
			{
				//for compat with v13, we still need to store this nasty font size (screen resolution dependent)
				rfontSize = floor(rfontSize*72/VSpanTextParser::Get()->GetSPANFontSizeDPI()+0.5f);
			}
#endif
			//convert to TWIPS and back to point to achieve TWIPS precision (using TWIPS precision helps to avoid math discrepancies: conversion is lossless between TWIPS & point)
			sLONG twips = ICSSUtil::PointToTWIPS( rfontSize);
			_TWIPSToCSSDimPoint( twips, fontsize);

			outCSSDeclarations.AppendString( fontsize);
		}

		count++;
	}
	if (!inIgnoreColors)
	{
		if (inStyle.GetHasForeColor() && inStyle.GetColor() != styleInherit->GetColor())
		{
			//color

			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_COLOR);
			outCSSDeclarations.AppendUniChar( ':');

			VString color;
			ColorToValue( inStyle.GetColor(), color);

			outCSSDeclarations.AppendString( color);
			count++;
		}
		if (inStyle.GetHasBackColor() && inStyle.GetBackGroundColor() != styleInherit->GetBackGroundColor())
		{
			//background-color

			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_COLOR);
			outCSSDeclarations.AppendUniChar( ':');

			if (inStyle.GetBackGroundColor() == RGBACOLOR_TRANSPARENT)
				outCSSDeclarations.AppendString( "transparent");
			else
			{
				VString color;
				ColorToValue( inStyle.GetBackGroundColor(), color);

				outCSSDeclarations.AppendString( color);
			}
			count++;
		}
	}
	if (inStyle.GetItalic() != UNDEFINED_STYLE && inStyle.GetItalic() != styleInherit->GetItalic())
	{
		//font-style

		if (count)
			outCSSDeclarations.AppendUniChar(';');
		outCSSDeclarations.AppendCString( kCSS_NAME_FONT_STYLE);
		outCSSDeclarations.AppendString( inStyle.GetItalic() == TRUE ? ":italic" : ":normal");
		count++;
	}
	if (inStyle.GetBold() != UNDEFINED_STYLE && inStyle.GetBold() != styleInherit->GetBold())
	{
		//font-weight

		if (count)
			outCSSDeclarations.AppendUniChar(';');
		outCSSDeclarations.AppendCString( kCSS_NAME_FONT_WEIGHT);
		outCSSDeclarations.AppendString( inStyle.GetBold() == TRUE ? ":bold" : ":normal");
		count++;
	}
	if ((inStyle.GetUnderline() != UNDEFINED_STYLE && inStyle.GetUnderline() != styleInherit->GetUnderline())
		|| 
		(inStyle.GetStrikeout() != UNDEFINED_STYLE && inStyle.GetStrikeout() != styleInherit->GetStrikeout()))
	{
		//text-decoration

		if (count)
			outCSSDeclarations.AppendUniChar(';');
		outCSSDeclarations.AppendCString( kCSS_NAME_TEXT_DECORATION);
		outCSSDeclarations.AppendUniChar( ':');

		VString value;
		if (inStyle.GetUnderline() == TRUE || styleInherit->GetUnderline() == TRUE) //as one CSS prop is used for both VTextStyle properties, we need to take account inherited value
			value = "underline";
		if (inStyle.GetStrikeout() == TRUE || styleInherit->GetStrikeout() == TRUE) 
		{
			if (value.IsEmpty())
				value = "line-through";
			else
				value = value + " line-through";
		}
		if (value.IsEmpty())
			value = "none";

		outCSSDeclarations.AppendString( value);
		count++;
	}
	/* //now it is a paragraph style only
	if (inStyle.GetJustification() != JST_Notset && inStyle.GetJustification() != styleInherit->GetJustification())
	{
		//text-align

		if (count)
			outCSSDeclarations.AppendUniChar(';');
		outCSSDeclarations.AppendCString( kCSS_NAME_TEXT_ALIGN);
		switch(inStyle.GetJustification())
		{
		case JST_Default:
		case JST_Left:
		case JST_Mixed:
			outCSSDeclarations.AppendString("left");
			break;
		case JST_Right:
			outCSSDeclarations.AppendString("right");
			break;
		case JST_Center:
			outCSSDeclarations.AppendString("center");
			break;
		case JST_Justify:
			outCSSDeclarations.AppendString("justify");
			break;
		default:
			outCSSDeclarations.AppendString("inherit");
			xbox_assert(false);
			break;
		}
		count++;
	}
	*/
	if (inStyle.GetSpanRef() != NULL)
	{
		if (inStyle.GetSpanRef()->GetType() == kSpanTextRef_4DExp)
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_D4_REF);
			outCSSDeclarations.AppendString( ":\'"); //start CSS string
			VString ref = inStyle.GetSpanRef()->GetRef();
			TextToXML( ref, true, true, true, true, true); //convert for CSS & escape end of line
			outCSSDeclarations.AppendString( ref);
			outCSSDeclarations.AppendUniChar( '\''); //end CSS string
			count++;
		}
		else if (inStyle.GetSpanRef()->GetType() == kSpanTextRef_User)
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_D4_REF_USER);
			outCSSDeclarations.AppendString( ":\'"); //start CSS string
			VString ref = inStyle.GetSpanRef()->GetRef();
			TextToXML( ref, true, true, true, true, true); //convert for CSS & escape end of line
			outCSSDeclarations.AppendString( ref);
			outCSSDeclarations.AppendUniChar( '\''); //end CSS string
			count++;
		}
	}

	if (styleInherit && !_inStyleInherit)
		delete styleInherit;
}


VSpanTextParser* VSpanTextParser::Get()
{
	if (sInstance == NULL)
		sInstance =  new VSpanTextParser();
	assert(sInstance != NULL);
	return sInstance;
}

void	VSpanTextParser::Init()
{
	sInstance = NULL;
}

void	VSpanTextParser::DeInit()
{
	if (sInstance != NULL)
	{
		delete sInstance;
		sInstance = NULL;
	}

	VParserXHTML::ClearShared();
}

END_TOOLBOX_NAMESPACE
