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

			void				CloseParagraph();

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
			VDocNode*			RetainDocNode()
			{
				if (fDocNode.Get())
					return RetainRefCountable( fDocNode.Get());
				else
					return NULL;
			}

			/** set parent document node */
			void				SetParentDocNode( VDocNode *inDocNode) { fParentDocNode = inDocNode; }
			VDocNode*			GetParentDocNode() { return fParentDocNode; }	

			bool				IsNodeDoc() const { return fDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT; }
			bool				IsNodeParagraph() const { return fIsNodeParagraph; }
			bool				IsNodeSpan() const { return fIsNodeSpan; } 

	static	bool				IsHTMLElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("html"),true); }
	static	bool				IsBodyElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("body"),true); }
	static	bool				IsDivElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("div"),true); }
	static	bool				IsParagraphElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("p"),true) || inElementName.EqualToString(CVSTR("li"),true); }
	static	bool				IsSpanElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("span"),false); }
	static	bool				IsUrlElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("a"),true); }
	static	bool				IsImageElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("img"),true); }

protected:
			void				_CreateStyles();
			const VTextStyle*	_GetStyleInheritForChildren();
			const VTextStyle*	_GetStyleInherit() { return (fStyleInherit ? fStyleInherit : (fStylesParent ? fStylesParent->GetData() : NULL)); }

			void				_AttachSpanRefs()
			{
				xbox_assert(fSpanHandlerParagraph && fStylesSpanRefs);

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

			VSpanHandler*		fSpanHandlerParent;
			VSpanHandler*		fSpanHandlerParagraph;
			VString*			fText;
			bool				fShowSpanRef;
			bool				fNoUpdateRef;
			VTreeTextStyle*		fStyles;
			VTreeTextStyle*		fStylesParent;
			VTreeTextStyle*		fStylesRoot;
			VTextStyle*			fStyleInherit;
			bool				fStyleInheritForChildrenDone;
			VString				fElementName;
			sLONG				fStart;
			bool				fIsInBody;
			bool				fIsNodeParagraph, fIsNodeSpan;

			std::vector<VTreeTextStyle *> *fStylesSpanRefs;
			VIndex				fDocNodeLevel;
			VDocNode*			fParentDocNode;
			VRefPtr<VDocNode>	fDocNode;
			bool				fIsUnknownTag;
			bool				fIsOpenTagClosed;
			bool				fIsUnknownRootTag;
			VString				fUnknownRootTagText;
			VString*			fUnknownTagText;

			VDBLanguageContext*	fDBLanguageContext;
};

class VParserXHTML : public VSpanHandler
{
public:
			/** XHTML or SPAN tagged text parser 
			@param inVersion
				document version 
			@param inDPI
				dpi used to convert CSS measures from resolution independent unit to pixel unit 
				(default is 72 so 1px = 1pt for compatibility with 4D forms - might be overriden with "-d4-dpi" property)
			*/
								VParserXHTML(uLONG inVersion = kDOC_VERSION_SPAN4D, const uLONG inDPI = 72, bool inShowSpanRef = false, bool inNoUpdateRef = true, VDBLanguageContext *inLC = NULL)
								:VSpanHandler(NULL, CVSTR("html"), NULL, NULL, NULL, inVersion, inLC)
			{
				fShowSpanRef = inShowSpanRef;
				fNoUpdateRef = inNoUpdateRef;
				fDBLanguageContext = fNoUpdateRef ? NULL : inLC;

				VDocText *docText = new VDocText();
				fDocNode = static_cast<VDocNode *>(docText);
				docText->SetVersion( inVersion);
				docText->SetDPI( inDPI);
				docText->Release();
			
				fDPI = inDPI;
				if (inVersion != kDOC_VERSION_SPAN4D)
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

						//style = RetainDefaultStyle("center");
						//style->GetData()->SetJustification(JST_Center);
						//style->Release();

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
			}
	virtual						~VParserXHTML()
			{
				ReleaseRefCountable(&fStyles);
			}
	

			void				CloseParsing()
			{
				if (fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D)
				{
					//we need to truncate leading end of line of last paragraph (as it has been added while parsing end of paragraph tag)

					VDocText *docText = fDocNode->RetainDocumentNode();
					if (docText->GetChildCount())
					{
						VDocNode *node = docText->RetainChild( docText->GetChildCount()-1);
						VDocParagraph *para = dynamic_cast<VDocParagraph *>(node);
						xbox_assert(para);
						VString *text = para->GetTextPtr();
						if (!text->IsEmpty() && (*text)[text->GetLength()-1] == 13)
						{
							text->Truncate( text->GetLength()-1);
							VTreeTextStyle *styles = para->RetainStyles();
							if (styles)
							{
								styles->Truncate( text->GetLength(), 1);
								para->SetStyles( styles, false);
								ReleaseRefCountable(&styles);
							}
						}
						ReleaseRefCountable(&node);
					}
					else
					{
						//at least one empty paragraph (empty text)
						VDocParagraph *para = new VDocParagraph();
						docText->AddChild( para);
						ReleaseRefCountable(&para);
					}
					ReleaseRefCountable(&docText);
				}
			}


			VDocText*			RetainDocument()
			{
				return fDocNode->RetainDocumentNode();
			}

			/** document DPI (default is 72) 
			@remarks
				might be overriden with "d4-dpi" property
			*/
			uLONG				GetDPI() { return fDocNode->GetDocumentNode()->GetDPI(); }

			void				SetDPI( const uLONG inDPI) { fDocNode->GetDocumentNodeForWrite()->SetDPI( inDPI); }

			/** SPAN format version - on default, it is assumed to be kDOC_VERSION_SPAN_1 if parsing SPAN tagged text (that is v12/v13 compatible) & kDOC_VERSION_XHTML11 if parsing XHTML (but 4D SPAN or 4D XHTML) 
			@remarks
				might be overriden with "d4-version" property
			*/
			uLONG				GetVersion() { return fDocNode->GetDocumentNode()->GetVersion(); }

			void				SetVersion( const uLONG inVersion) { fDocNode->GetDocumentNodeForWrite()->SetVersion( inVersion); }

	static	void				ClearShared()
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
	static	VTreeTextStyle*		RetainDefaultStyle(const VString &inElementName)
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
	static	VTreeTextStyle*		RetainDefaultStyle(const char *inElementName)
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
	static	VTreeTextStyle*		GetAndRetainDefaultStyle(const VString &inElementName)
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
	static	VTreeTextStyle*		GetAndRetainDefaultStyle(const char *inElementName)
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
	static	bool				NeedBreakLine( const VString &inElementName)
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
	static	bool				NeedBreakLine( const char *inElementName)
			{
				VTaskLock lock(&fMutexStyles);
				if (fSetOfBreakLineEnding.empty())
					return false;

				SetOfHTMLElementWithBreakLineEnding::const_iterator it = fSetOfBreakLineEnding.find(std::string(inElementName));
				return it != fSetOfBreakLineEnding.end();
			}
private:
			/** map of default style per HTML element */
	static	MapOfStylePerHTMLElement			fMapOfStyle;

			/** set of HTML elements for which to add a ending end of line */
	static	SetOfHTMLElementWithBreakLineEnding	fSetOfBreakLineEnding;

	static	VCriticalSection					fMutexStyles;

			/** document DPI (default is 72) 
			@remarks
				might be overriden with "d4-dpi" property
			*/
			uLONG								fDPI;
};

/** map of default style per HTML element */
MapOfStylePerHTMLElement			VParserXHTML::fMapOfStyle;

SetOfHTMLElementWithBreakLineEnding VParserXHTML::fSetOfBreakLineEnding;

VCriticalSection					VParserXHTML::fMutexStyles;

//
// VSpanHandler
//

VSpanHandler::VSpanHandler(VSpanHandler *inParent, const VString& inElementName, VTreeTextStyle *inStylesParent, 
						   const VTextStyle *inStyleInheritParent, VString* ioText, uLONG /*inVersion*/, VDBLanguageContext *inLC)
{	
	fSpanHandlerParent = inParent;
	fSpanHandlerParagraph = inParent ? inParent->fSpanHandlerParagraph : NULL;
	fElementName = inElementName;
	fShowSpanRef = inParent ? inParent->fShowSpanRef : false;
	fNoUpdateRef = inParent ? inParent->fNoUpdateRef : true;
	fDBLanguageContext = fNoUpdateRef ? NULL : inLC;
	fStylesSpanRefs = NULL;
	fText = ioText; 
	fStart = fText ? fText->GetLength() : 0;
	fStylesRoot = NULL;
	fStyles = NULL;
	fStylesParent = NULL;
	fStyleInherit = NULL;
	fStyleInheritForChildrenDone = false;
	fIsInBody = inParent ? inParent->fIsInBody : false;
	fIsNodeParagraph = false;
	fIsNodeSpan = false;

	fDocNodeLevel = -1;
	fParentDocNode = NULL;
	fIsUnknownTag = false;
	fIsOpenTagClosed = false;
	fIsUnknownRootTag = false;
	fUnknownTagText = inParent ? inParent->fUnknownTagText : NULL;

	if (inParent != NULL)
	{
		fParentDocNode = inParent->fDocNode.Get();
		fDocNode = inParent->fDocNode;
		fDocNodeLevel = inParent->fDocNodeLevel+1;

		uLONG version = fDocNode->GetDocumentNode()->GetVersion();
		if (!fIsInBody)
			if (version == kDOC_VERSION_SPAN4D || IsBodyElementName( fElementName))
				fIsInBody = true;
		if (fIsInBody)
		{
			if (inParent->fIsUnknownTag)
			{
				//unknown tag child node
				fDocNodeLevel = inParent->fDocNodeLevel;
				fIsUnknownTag = true;
				
				//children of unknown tag use same style & span reference 
				//(unknown tag root & children are serialized as xml text in fText and will be stored in span reference on root unknown tag EndElement)
				xbox_assert(inParent->fStyles && inParent->fStyles->GetData()->IsSpanRef());
				CopyRefCountable(&fStyles, inParent->fStyles);		
				fStart = inParent->fStart;

				fStylesRoot = fSpanHandlerParagraph ? fSpanHandlerParagraph->fStylesRoot : NULL;
				fStylesSpanRefs = NULL;
			}
			else
			{
				if (!fSpanHandlerParagraph)
				{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
					if (version >= kDOC_VERSION_XHTML4D)
					{
						//TODO: should store container elements in document tree (add another node type)
						if (!IsParagraphElementName( fElementName))
							return;
					}
#endif
					//paragraph node

					fIsNodeParagraph = true;
					fIsNodeSpan = IsSpanElementName( fElementName);

					VDocText *doc = inParent->fDocNode->GetDocumentNodeForWrite();
					
					VDocParagraph *para = new VDocParagraph();
					doc->AddChild( static_cast<VDocNode *>(para));
					fDocNode = static_cast<VDocNode *>(para);

					fStylesParent = NULL;
					fStyles = new VTreeTextStyle( new VTextStyle());
					fStylesRoot = fStyles;
					para->SetStyles( fStyles, false); //paragraph retains/shares current styles
					
					xbox_assert(ioText == NULL);
					fText = para->GetTextPtr();

					fStart = 0;

					para->Release();

					fSpanHandlerParagraph = this;
					fStylesSpanRefs = new std::vector<VTreeTextStyle *>();
				}
				else 
				{
					//paragraph or span child node (might be span, image, url, table, etc... but we parse all as span nodes for now)
					//TODO: we should parse images as VDocImage elements, tables as VDocTable

					xbox_assert(fText);

					fIsNodeSpan = true;

					fStylesParent = inStylesParent;
					fStyleInherit = inStyleInheritParent ? new VTextStyle(inStyleInheritParent) : NULL;

					VTreeTextStyle *stylesDefault = VParserXHTML::GetAndRetainDefaultStyle(inElementName);
					if (stylesDefault)
					{
						fStyles = new VTreeTextStyle( stylesDefault);
						fStyles->GetData()->SetRange( fStart, fStart);
						if (fStylesParent)
							fStylesParent->AddChild( fStyles);
						ReleaseRefCountable(&stylesDefault);
					}
					fStylesRoot = fSpanHandlerParagraph->fStylesRoot;
					fStylesSpanRefs = NULL;

					if (version != kDOC_VERSION_XHTML11 && !IsSpanElementName( fElementName) && !IsUrlElementName( fElementName))
					{
						//SPAN4D or XHTML4D only -> unknown tag (for legacy XHTML11, we parse unknown tags as span text)

						fIsUnknownTag = true;
						fIsUnknownRootTag = true;
						_CreateStyles();
						VDocSpanTextRef *spanRef = new VDocSpanTextRef( kSpanTextRef_UnknownTag, VDocSpanTextRef::fNBSP, VDocSpanTextRef::fNBSP);
						fStyles->GetData()->SetSpanRef( spanRef);
						if (fStyles->GetData()->IsSpanRef())
							fStyles->NotifySetSpanRef();
						fUnknownTagText = &fUnknownRootTagText;
						fText->AppendString( CVSTR("<")+fElementName);
					}
				}
			}
		}
	}
}
	
void VSpanHandler::CloseParagraph()
{
	//finalize text & styles

	xbox_assert( fSpanHandlerParagraph == this && fStyles == fStylesRoot && fText);

#if VERSIONWIN
	fText->ConvertCarriageReturns(eCRM_CR); //we need to convert back to CR (on any platform but Linux here) because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)
#elif VERSIONMAC
	fText->ConvertCarriageReturns(eCRM_CR); //we need to convert back to CR (on any platform but Linux here) because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)
#endif

	if (fStyles)
		fStyles->GetData()->SetRange(fStart, fText->GetLength());

	_AttachSpanRefs();

	if (fStyles && ((fStyles)->IsUndefined(false)))
	{
		//style not defined
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
		ReleaseRefCountable(&fStyles);
	}
	VDocParagraph *para = dynamic_cast<VDocParagraph *>(fDocNode.Get());
	xbox_assert(para);
	para->SetStyles( fStyles, false);
}

VSpanHandler::~VSpanHandler()
{
	//if (fStyles && fStyles->GetParent() && fStyles->IsUndefined(false))
	//{
	//	//range is empty or style not defined (can happen while parsing node without any text or without styles): remove this useless style from parent if any
	//	VTreeTextStyle *parent = fStyles->GetParent();
	//	if (parent)
	//	{
	//		xbox_assert(parent == fStylesParent);
	//		for (int i = 1; i <= parent->GetChildCount(); i++)
	//			if (parent->GetNthChild(i) == fStyles)
	//			{
	//				parent->RemoveChildAt(i);
	//				break;
	//			}
	//	}
	//}
	ReleaseRefCountable(&fStyles);
	if (fStyleInherit)
		delete fStyleInherit;
}

void  VSpanHandler::_CreateStyles()
{
	if (fStyles == NULL && fSpanHandlerParagraph)
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
		xbox_assert( fSpanHandlerParagraph && fText);

		if (!fIsOpenTagClosed)
		{
			fIsOpenTagClosed = true;
			fText->AppendUniChar('>');
		}

		//parsing unknown tags -> store as unknown tag span ref 
		//for unknown tag children tags, we serialize full XML tag
		VSpanHandler *spanHandler;
		result = spanHandler = new VSpanHandler(this, inElementName, fStyles ? fStyles : fStylesParent, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
		
		fText->AppendString( CVSTR("<")+inElementName);
	}
	else if (fText && inElementName.EqualToString("br"))
	{
		*fText+="\r";
		if (fUnknownTagText)
			*fUnknownTagText += "\r";
	}
	else
	{
		uLONG version = fDocNode->GetDocumentNode()->GetVersion();
		bool createHandler = fIsInBody || version == kDOC_VERSION_SPAN4D || IsHTMLElementName( inElementName) || IsBodyElementName( inElementName);
		if (createHandler)
		{
			_CreateStyles();
			VSpanHandler *spanHandler;
			result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
			if (!fIsUnknownTag && fText && fDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11 && inElementName.EqualToString("q"))
			{
				//start of quote
				*fText += "\"";
			}
		}
	}
	return result;
}

void VSpanHandler::EndElement( const VString& inElementName)
{
	//xbox_assert(inElementName == fElementName);
	if (!fSpanHandlerParagraph)
		return;
	xbox_assert(fText);

	if (!fIsUnknownTag && fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D)
	{
		//XHTML parsing:

		if (fDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_XHTML11 && inElementName.EqualToString("q"))
			//end of quote
			*fText += "\"";

		//add break lines for elements which need it
		if (VParserXHTML::NeedBreakLine(inElementName))
			*fText += "\r";
	}

	VTextStyle *style = fStyles ? fStyles->GetData() : NULL;
	sLONG start, end;
	if (style)
	{
		style->GetRange(start, end);
		end = fText->GetLength();
		style->SetRange(start, end);
	}

	if(style && style->IsSpanRef())
	{
		xbox_assert(fSpanHandlerParagraph != this);

		//detach span ref fStyles & store temporary in root span handler (span refs will be attached to fStyles root on CloseParagraph)
		if (!(fIsUnknownTag && !fIsUnknownRootTag))
		{
			xbox_assert(fStyles->GetParent());
			fStyles->Retain();
			fStyles->GetParent()->RemoveChildAt( fStyles->GetParent()->GetChildCount());
			xbox_assert(fStyles->GetParent() == NULL && fStyles->GetRefCount() >= 2);

			if (fStyles->GetChildCount() > 0)
			{
				//if it has children, we keep only span reference uniform styles
				VTextStyle *styleUniform = fStyles->CreateUniformStyleOnRange(start, end > start ? start+1 : end, false);
				styleUniform->SetSpanRef(NULL);
				fStyles->GetData()->AppendStyle( styleUniform, false); //override with children uniform styles
				delete styleUniform;
				fStyles->RemoveChildren();
			}
			fStyles->GetData()->AppendStyle( _GetStyleInherit(), true); //merge with inherited styles
			fSpanHandlerParagraph->fStylesSpanRefs->push_back(fStyles);
		}

		//finalize span ref
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

	if (fSpanHandlerParagraph == this)
		CloseParagraph();
}

void VSpanHandler::SetAttribute(const VString& inName, const VString& inValue)
{
	if (fIsUnknownTag)
	{
		xbox_assert(fSpanHandlerParagraph && fText);

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
		uLONG version = fDocNode->GetDocumentNode()->GetVersion();

		if (version != kDOC_VERSION_SPAN4D && !fSpanHandlerParagraph)
		{
			//XHTML11 or XHTML4D
			//node is html or body node: parse document properties
			//(if -d4-version is found, document version is modified)
			xbox_assert( fDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT);
			VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, inValue);
		}
		else if (fSpanHandlerParagraph)
		{
			//node is a paragraph or a span 

			_CreateStyles();
			
			if (this == fSpanHandlerParagraph) //paragraph element (XHTML4D) or span root element (SPAN4D)
				//parse paragraph or character properties
				VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, inValue);
			else
				//otherwise parse only character styles
				VSpanTextParser::Get()->_ParseCSSToVTreeTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, inValue, fStyles, _GetStyleInherit());
		}
	} 
	else if (fSpanHandlerParagraph && fElementName.EqualToString("a") && inName.EqualToString("href"))
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
		//we tolerate some HTML attributes with legacy XHTML (but not with XHTML4D)

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
	if (!fSpanHandlerParagraph)
		return;
	xbox_assert( fText);

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

	outText = inText;
	outText.ConvertCarriageReturns( eCRM_CR);

	TextToXML(outText, true, false, true); //fix XML invalid chars & convert CR if not done yet but do not convert XML markup as text should be XML text yet
	if (inParseSpanOnly)
		outText = VString("<span>")+outText+"</span>"; //we need to encapsulate in <span> tag as XML parser tolerates only one root tag
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
	{
		outPlainText.SetEmpty();
		return;
	}

	VString vtext;
	if (inTaggedText.BeginsWith("<html")) //for compat with future 4D XHTML format
		inParseSpanOnly = false;
	_CheckAndFixXMLText( inTaggedText, vtext, inParseSpanOnly);

	VParserXHTML vsaxhandler(inParseSpanOnly ? kDOC_VERSION_SPAN4D : kDOC_VERSION_XHTML11, inDPI, inShowSpanRef, inNoUpdateRefs, inLC);
	VXMLParser vSAXParser;

	if (vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever))
	{
		vsaxhandler.CloseParsing(); //finalize text & styles if appropriate
		
		VDocText *doc = vsaxhandler.RetainDocument();
		outPlainText = doc->GetText();
		outStyles = doc->RetainStyles();
		ReleaseRefCountable(&doc);
	}
	else
		outPlainText = inTaggedText; //parser failed: assume it is plain text
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

	VDocText *doc = VSpanTextParser::Get()->ParseDocument( inSpanText, kDOC_VERSION_SPAN4D, 72, false, true, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D, 72, false, true, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
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
		inDoc = VSpanTextParser::Get()->ParseDocument( inSpanTextOrPlainText, kDOC_VERSION_SPAN4D, 72, false, true, inSilentlyTrapParsingErrors);
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D, 72, false, true, inSilentlyTrapParsingErrors); 
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( inSpanText, kDOC_VERSION_SPAN4D, 72, false, true, true); 
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D, 72, false, true, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
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
	if (inDocText.BeginsWith("<html")) //for compat with future 4D XHTML format
		version = kDOC_VERSION_XHTML11;

	VString vtext;
	_CheckAndFixXMLText( inDocText, vtext, version ==  kDOC_VERSION_SPAN4D);

	VParserXHTML vsaxhandler(version, inDPI, inShowSpanRef, inNoUpdateRefs, inLC);
	VXMLParser vSAXParser;

	if (inSilentlyTrapParsingErrors)
	{
		StErrorContextInstaller errorContext(false);

		vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever);

		VErrorContext* error = errorContext.GetContext();

		if (error && !error->IsEmpty())
			return NULL;

		vsaxhandler.CloseParsing(); //finalize if appropriate

		return vsaxhandler.RetainDocument();
	}
	else
	{
		if (!vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever))
			return NULL;

		vsaxhandler.CloseParsing(); //finalize if appropriate
		
		return vsaxhandler.RetainDocument();
	}
}


/** serialize document (formatting depending on version) 
@remarks
	if (inDocParaDefault != NULL), only properties and character styles different from the passed paragraph properties & character styles will be serialized
*/
void VSpanTextParser::SerializeDocument( const VDocText *inDoc, VString& outDocText, const VDocParagraph *inDocParaDefault)
{
	xbox_assert(inDoc);

	outDocText.SetEmpty();

	fIsOnlyPlainText = true;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
	{
		_OpenTagDoc(outDocText); 
		if (inDoc->HasProps()) 
		{
			//serialize document properties

			VIndex lengthBeforeStyle = outDocText.GetLength();
			outDocText.AppendCString(" style=\"");
			VIndex start = outDocText.GetLength();

			_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDoc), outDocText);

			if (outDocText.GetLength() > start)
			{
				outDocText.AppendUniChar('\"');
				fIsOnlyPlainText = false;
			}
			else
				outDocText.Truncate( lengthBeforeStyle);
		}
		_CloseTagDoc(outDocText, false);
		_OpenTagBody(outDocText);
		_CloseTagBody(outDocText, false);
	}
#endif

	//serialize paragraphs
	for (VIndex iChild = 0; iChild < inDoc->GetChildCount(); iChild++)
	{
		const VDocNode *child = inDoc->GetChild(iChild);
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(child);
		xbox_assert(para);

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
			_OpenTagParagraph(outDocText); 
		else
#endif
			_OpenTagSpan(outDocText);

		VTextStyle *styleInherit = new VTextStyle();
		styleInherit->SetBold( 0);
		styleInherit->SetItalic( 0);
		styleInherit->SetStrikeout( 0);
		styleInherit->SetUnderline( 0);

		if (inDocParaDefault && inDocParaDefault->GetStyles())
			styleInherit->MergeWithStyle( inDocParaDefault->GetStyles()->GetData());

		//ensure we do not serialize text align or char background color if equal to paragraph text align or back color

		if (styleInherit->GetJustification() == JST_Notset)
		{
			if (para->IsOverriden(kDOC_PROP_TEXT_ALIGN) && !para->IsInherited(kDOC_PROP_TEXT_ALIGN))
				styleInherit->SetJustification( (eStyleJust)para->GetTextAlign());
			if (styleInherit->GetJustification() == JST_Notset)
				styleInherit->SetJustification( JST_Default);
		}

		if (para->HasProps() || para->GetStyles()) 
		{
			//serialize paragraph props + paragraph character styles

			VIndex lengthBeforeStyle = outDocText.GetLength();
			outDocText.AppendCString(" style=\"");
			VIndex start = outDocText.GetLength();

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

		//close root opening tag
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
			_CloseTagParagraph( outDocText, false); 
		else
#endif
			_CloseTagSpan( outDocText, false);

		//serialize paragraph plain text & character styles

		_SerializeParagraphTextAndStyles( child, para->GetStyles(), para->GetText(), outDocText, styleInherit, true);

		//close paragraph tag
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
			_CloseTagParagraph( outDocText); 
		else
#endif
			_CloseTagSpan( outDocText);
	}

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	//close document tag
	if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
	{
		_CloseTagBody( outDocText);
		_CloseTagDoc( outDocText);
	}
#endif

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	if (fIsOnlyPlainText && inDoc->GetVersion() < kDOC_VERSION_XHTML4D)
#else
	if (fIsOnlyPlainText)
#endif
	{
		//optimization: if text is only plain text, we output plain text & not span text

		//we need to check if plain text is valid XML text & contains XML tags or escaped characters: if so, 
		//we return span text otherwise XML tags or escaped characters would be parsed while parsing tagged text

		VDocText *doc = ParseDocument( inDoc->GetText()); 
		if (!doc)
			//invalid XML -> we can return plain text
			outDocText.FromString( inDoc->GetText());
		else
		{
			if (doc->GetStyles() == NULL && doc->GetText().EqualToStringRaw( inDoc->GetText()))
				//plain text does not contain any XML tag or escaped characters -> we can return plain text
				outDocText.FromString( inDoc->GetText());
		}
	}
}


void VSpanTextParser::_OpenTagSpan(VString& ioText)
{
	ioText.AppendCString( "<span");
}

void VSpanTextParser::_CloseTagSpan(VString& ioText, bool inAfterChildren)
{
	if (inAfterChildren)
		ioText.AppendCString( "</span>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagDoc(VString& ioText)
{
	ioText.AppendCString("<html xmlns=\"http://www.w3.org/1999/xhtml\"");
}

void VSpanTextParser::_CloseTagDoc(VString& ioText, bool inAfterChildren)
{
	if (inAfterChildren)
		ioText.AppendCString( "</html>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagBody(VString& ioText)
{
	ioText.AppendCString("<body");
}

void VSpanTextParser::_CloseTagBody(VString& ioText, bool inAfterChildren)
{
	if (inAfterChildren)
		ioText.AppendCString( "</body>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagParagraph(VString& ioText)
{
	ioText.AppendCString( "<p"); 
}

void VSpanTextParser::_CloseTagParagraph(VString& ioText, bool inAfterChildren)
{
	if (inAfterChildren)
		ioText.AppendCString( "</p>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_TruncateLeadingEndOfLine(VString& ioText)
{
	if (!ioText.IsEmpty())
	{
		sLONG numChar = 0;
		if (ioText[ioText.GetLength()-1] == 13)
			numChar = 1;
		else if (ioText[ioText.GetLength()-1] == 10)
		{
			if (ioText.GetLength() >= 2 && ioText[ioText.GetLength()-2] == 13)
				numChar = 2;
			else
				numChar = 1;
		}
		if (numChar)
			ioText.Truncate( ioText.GetLength()-numChar);
	}
}

void VSpanTextParser::_SerializeParagraphTextAndStyles( const VDocNode *inDocNode, const VTreeTextStyle *inStyles, const VString& inPlainText, VString &ioText, 
														const VTextStyle *_inStyleInherit, bool inSkipCharStyles)
{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isXHTML4D = inDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D;
#endif

	if (!inStyles)
	{
		if (!inPlainText.IsEmpty())
		{
			VString text( inPlainText);
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
			if (isXHTML4D)
				//truncate leading CR if any (the paragraph closing tag itself defines the end of line)
				_TruncateLeadingEndOfLine( text);
#endif
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
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
				if (isXHTML4D && childStart >= inPlainText.GetLength())
					//truncate leading CR if any (the paragraph closing tag itself defines the end of line)
					_TruncateLeadingEndOfLine( text);
#endif
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
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (isXHTML4D && end >= inPlainText.GetLength())
			//truncate leading CR if any (the paragraph closing tag itself defines the end of line)
			_TruncateLeadingEndOfLine( text);
#endif
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
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isXHTML4D = ioDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D;
#endif
	xbox_assert(ioDocNode && ioDocNode->GetDocumentNode());

	//prepare inline CSS parsing

	VCSSLexParser *lexParser = new VCSSLexParser();

	VCSSParserInlineDeclarations cssParser(true);
	cssParser.Start(inCSSDeclarations);
	VString attribut, value;

	switch (ioDocNode->GetType())
	{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		case kDOC_NODE_TYPE_DOCUMENT:	//root node (<body> node) -> parse document properties 
			{
				VDocText *doc = dynamic_cast<VDocText *>(ioDocNode);
				xbox_assert(doc);
				while (cssParser.GetNextAttribute(attribut, value))
				{
					if (attribut == kCSS_NAME_D4_VERSION)
						//CSS -d4-version
						_ParsePropDoc(lexParser, doc, kDOC_PROP_VERSION, value);
					else if (attribut == kCSS_NAME_D4_DPI)
						//CSS -d4-dpi
						_ParsePropDoc(lexParser, doc, kDOC_PROP_DPI, value);
					else if (attribut == kCSS_NAME_D4_ZOOM)
						//CSS -d4-zoom
						_ParsePropDoc(lexParser, doc, kDOC_PROP_ZOOM, value);
				}
			}
			break;
#endif
		case kDOC_NODE_TYPE_PARAGRAPH:
			{
				//parse paragraph properties

				VDocParagraph *para = NULL;
				VTreeTextStyle *styles = NULL;
				VTextStyle *style = NULL;

				para = dynamic_cast<VDocParagraph *>(ioDocNode);
				xbox_assert(para);

				styles = para->RetainStyles();
				if (!styles)
				{
					styles = new VTreeTextStyle( new VTextStyle());
					para->SetStyles( styles, false);
				}
				xbox_assert(styles);
				style = styles->GetData();
				
				bool isSpan4D = ioDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D;

				while (cssParser.GetNextAttribute(attribut, value))
				{
					//span character properties  
					if (attribut == kCSS_NAME_FONT_FAMILY)
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_FAMILY, value, *style);
					else if (attribut == kCSS_NAME_FONT_SIZE)
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_SIZE, value, *style);
					else if (attribut == kCSS_NAME_COLOR)
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_COLOR, value, *style);
					else if (attribut == kCSS_NAME_FONT_STYLE)
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_STYLE, value, *style);
					else if (attribut == kCSS_NAME_TEXT_DECORATION)
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_TEXT_DECORATION, value, *style);
					else if (attribut == kCSS_NAME_FONT_WEIGHT)
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_FONT_WEIGHT, value, *style);
					else if (isSpan4D && attribut == kCSS_NAME_BACKGROUND_COLOR) //if SPAN4D format, it is a span node 
						_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_BACKGROUND_COLOR, value, *style);
					else if (isSpan4D && attribut == kCSS_NAME_TEXT_ALIGN) //if SPAN4D format, it is a span node 
						_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_TEXT_ALIGN, value);

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
					else if (isXHTML4D) //only XHTML4D format is capable to manage all paragraph properties
					{
						//node common properties
						if (attribut == kCSS_NAME_TEXT_ALIGN)
							_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_TEXT_ALIGN, value);
						else if (attribut == kCSS_NAME_VERTICAL_ALIGN)
							_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_VERTICAL_ALIGN, value);
						else if (attribut == kCSS_NAME_MARGIN)
							_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_MARGIN, value);
						else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
							_ParsePropCommon(lexParser, static_cast<VDocNode *>(para), kDOC_PROP_BACKGROUND_COLOR, value); //here it is element back color

						//paragraph node private properties
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
					}
#endif
				}
				ReleaseRefCountable(&styles);
			}
			break;
		default:
			break;
	}
	delete lexParser;
}


/** serialize VDocNode props to CSS declaration string */ 
void VSpanTextParser::_SerializeVDocNodeToCSS(  const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault, bool inIsFirst)
{
	xbox_assert(inDocNode && inDocNode->GetDocumentNode());
	
	VIndex count = inIsFirst ? 0 : 1;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	if (inDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT)
	{
		//serialize document properties (this document node contains only document properties)

		if (inDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D) 
		{
			//CSS d4-version (d4-version value = inDocNode->GetDocumentNode()->GetVersion()-1)
			bool isOverriden = inDocNode->IsOverriden( kDOC_PROP_VERSION) && !inDocNode->IsInherited( kDOC_PROP_VERSION);
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_VERSION);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)(inDocNode->GetDocumentNode()->GetVersion() < kDOC_VERSION_SPAN4D ? kDOC_VERSION_SPAN4D-1 : inDocNode->GetDocumentNode()->GetVersion()-1));
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
	else 
#endif
	if (inDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
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

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		bool isXHTML4D = inDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D;
		if (!isXHTML4D)
			return;

		//serialize paragraph-only properties

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
	styleDefault->SetBackGroundColor( inDocPara->GetBackgroundColor()); //to avoid character background color to be set to paragraph background color 
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

	if (inProperty == kDOC_PROP_VERSION)
	{
		//CSS d4-version

		uLONG version = (uLONG)inValue.GetLong()+1;
		ioDocText->SetVersion(version >= kDOC_VERSION_SPAN4D && version <= kDOC_VERSION_MAX_SUPPORTED ? version : kDOC_VERSION_XHTML11);
		result = true;
	}
	else if (inProperty == kDOC_PROP_DPI)
	{
		//CSS d4-dpi

		uLONG dpi = inValue.GetLong();
		if (dpi >= 72)
		{
			ioDocText->SetDPI(dpi);
			result = true;
		}
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
			if (zoom < 10)
				zoom = 10; //avoid too small zoom
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

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isXHTML4D = inDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D;
	if (!isXHTML4D)
		return;

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
				number[0] = margin.top;
				number[1] = margin.right;
				number[2] = margin.bottom;
				number[3] = margin.left;
				
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
				if (version == kDOC_VERSION_SPAN4D)
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
	else if (inProperty == kDOC_PROP_TEXT_ALIGN)
	{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		xbox_assert(inDocNode->GetDocumentNode()->GetVersion() < kDOC_VERSION_XHTML4D);
#endif

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
	//but they are in SPAN4D format 
	bool backgroundColorInherited = inDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D ? true : false;
	bool textDecorationInherited = inDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D ? true : false;

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
		else if (attribut == kCSS_NAME_D4_REF)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_4DREF, value, outStyle, NULL, NULL, NULL, inLC);
		else if (attribut == kCSS_NAME_D4_REF_USER)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_4DREF_USER, value, outStyle, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_TEXT_ALIGN && inDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D) 
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_TEXT_ALIGN, value, outStyle, NULL, NULL, NULL);
	}

	//apply W3C CSS pure rules if text is pure XHTML 1.1 as XHTML4D (and not SPAN4D)
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
	RGBAColor paraBackgroundColor = (dynamic_cast<const VDocParagraph *>(inDocNode))->GetBackgroundColor();

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
			if (version == kDOC_VERSION_SPAN4D)
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
			if (version == kDOC_VERSION_SPAN4D)
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
		if (inStyle.GetHasForeColor() && (!styleInherit->GetHasForeColor() || inStyle.GetColor() != styleInherit->GetColor()))
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
		if (inStyle.GetHasBackColor() || styleInherit->GetHasBackColor())
		{
			//background-color

			if (!(paraBackgroundColor != RGBACOLOR_TRANSPARENT 
				&& 
				((inStyle.GetHasBackColor() && inStyle.GetBackGroundColor() == paraBackgroundColor) 
				 || 
				 (!inStyle.GetHasBackColor() && styleInherit->GetHasBackColor() && styleInherit->GetBackGroundColor() == paraBackgroundColor))))
			{
				//we serialize background color character style if and only if it is different from the paragraph background color

				bool doIt = true;
				uLONG version = inDocNode->GetDocumentNode()->GetVersion();

				VString value;
				if (!inStyle.GetHasBackColor())
				{
					if (version != kDOC_VERSION_SPAN4D && styleInherit->GetHasBackColor())
						//CSS compliancy: ensure background-color is inherited (as property is not inherited on default in CSS but it is in SPAN4D)
						value = kCSS_INHERIT;
					else 
						doIt = false;
				}
				else if (styleInherit->GetHasBackColor() && inStyle.GetBackGroundColor() == styleInherit->GetBackGroundColor())
				{
					//CSS compliancy: ensure background-color is inherited (as property is not inherited on default in CSS but it is in SPAN4D)
					if (version != kDOC_VERSION_SPAN4D && inStyle.GetBackGroundColor() != RGBACOLOR_TRANSPARENT)
						value = kCSS_INHERIT;
					else 
						doIt = false;
				}
				else if (inStyle.GetBackGroundColor() == RGBACOLOR_TRANSPARENT)
				{
					if (version == kDOC_VERSION_SPAN4D && styleInherit->GetHasBackColor() && styleInherit->GetBackGroundColor() != RGBACOLOR_TRANSPARENT)
						value = "transparent";
					else 
						doIt = false;
				}
				else
					ColorToValue( inStyle.GetBackGroundColor(), value);

				if (doIt)
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_COLOR);
					outCSSDeclarations.AppendUniChar( ':');
					outCSSDeclarations.AppendString( value);
					count++;
				}
			}
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
	if (inStyle.GetUnderline() != UNDEFINED_STYLE || styleInherit->GetUnderline() != UNDEFINED_STYLE
		|| 
		inStyle.GetStrikeout() != UNDEFINED_STYLE || styleInherit->GetStrikeout() != UNDEFINED_STYLE)
	{
		//text-decoration

		bool doIt = true;
		uLONG version = inDocNode->GetDocumentNode()->GetVersion();

		VString value;
		if ((inStyle.GetUnderline() == styleInherit->GetUnderline()
			 && 
			 inStyle.GetStrikeout() == styleInherit->GetStrikeout())
			 ||
			 (inStyle.GetUnderline() == UNDEFINED_STYLE 
			 && 
			 inStyle.GetStrikeout() == UNDEFINED_STYLE))
		{
			//CSS compliancy: ensure text-decoration is inherited (as property is not inherited on default in CSS but it is in SPAN4D)
			if (version != kDOC_VERSION_SPAN4D)
				value = kCSS_INHERIT;
			else
				doIt = false;
		}
		else
		{
			if (inStyle.GetUnderline() == TRUE || (inStyle.GetUnderline() == UNDEFINED_STYLE && styleInherit->GetUnderline() == TRUE)) //as one CSS prop is used for both VTextStyle properties, we need to take account inherited value
				value = "underline";
			if (inStyle.GetStrikeout() == TRUE || (inStyle.GetStrikeout() == UNDEFINED_STYLE && styleInherit->GetStrikeout() == TRUE)) 
			{
				if (value.IsEmpty())
					value = "line-through";
				else
					value = value + " line-through";
			}
			if (value.IsEmpty())
			{
				if (version != kDOC_VERSION_SPAN4D)
					//CSS compliancy: text-decoration is not inherited in regular CSS so default value is none yet if XHTML11 or XHTML4D
					doIt = false;
				else
					//SPAN4D compliancy: as value is inherited, we need to explicitly reset it to "none"
					value = "none";
			}
		}

		if (doIt)
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_TEXT_DECORATION);
			outCSSDeclarations.AppendUniChar( ':');
			outCSSDeclarations.AppendString( value);
			count++;
		}
	}
	/* //it is a paragraph style only
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
