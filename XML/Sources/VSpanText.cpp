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

#define VSTP_EXPLICIT_CR			7		//control character used to temporary mark explicit CR (defined with <br/> tags) to prevent it to be consolidated
											//(after consolidation it is replaced with 13)
#define VSTP_SPANREF_WHITESPACE		127		//control character used to temporary mark span reference whitespace to prevent it to be consolidated
											//(after consolidation it is replaced with ' ')

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


class VSpanHandler : public VObject, public IXMLHandler, public IDocNodeParserHandler
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

			/** overriden from IDocNodeParserHandler */
	virtual	void				IDNPH_SetAttribute(const VString& inName, const VString& inValue)
			{
				if (!fCSSDefaultStyles.IsEmpty())
					fCSSDefaultStyles.AppendUniChar(';');
				fCSSDefaultStyles.AppendString(inName);
				fCSSDefaultStyles.AppendUniChar(':');
				fCSSDefaultStyles.AppendString(inValue);
			}


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

			void				PreserveWhiteSpaces( bool inPre) { fPreserveWhiteSpaces = inPre; }
			uLONG				GetOptions() const { return fOptions; }

	static	bool				IsHTMLElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("html"),true); }
	static	bool				IsStyleElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("style"),true); }
	static	bool				IsHeadElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("head"),true); }
	static	bool				IsBodyElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("body"),true); }
	static	bool				IsDivElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("div"),true); }
	static	bool				IsParagraphElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("p"),true) || inElementName.EqualToString(CVSTR("li"),true); }
	static	bool				IsSpanElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("span"),false); }
	static	bool				IsUrlElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("a"),true); }
	static	bool				IsImageElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("img"),true); }

			void				ConsolidateWhiteSpaces(VString &ioText, VTreeTextStyle *ioStyles);

protected:
			void				_ParseCSSStyles(const VString& inStyles);
			void				_ParseCSSStyles();

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
					//as there are children styles, we cannot just append the span reference styles but we need to call ApplyStyle

					std::vector<VTreeTextStyle *>::iterator it = fStylesSpanRefs->begin();
					for (;it != fStylesSpanRefs->end(); it++)
					{
						fStyles->ApplyStyle( (*it)->GetData());
						(*it)->Release();
					}

					//as span references have been cloned (because of ApplyStyle) and so also the referenced VDocImage instances, 
					//we need to update span image references if any so it actually references the image(s) stored in the document
					//(also we need to update span reference with image source & alt text which have been parsed after image was bound to the span ref) 

#if ENABLE_VDOCTEXTLAYOUT
					if (fOptions & kHTML_PARSER_TAG_IMAGE_SUPPORTED)
						for (int i = 1; i <= fStyles->GetChildCount(); i++)
						{
							VTextStyle *style = fStyles->GetNthChild( i)->GetData();
							if (style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_Image)
							{
								//rebind span reference to the image stored in the document using the image ID which should not have changed
								VString id = style->GetSpanRef()->GetImage()->GetID();
								VDocNode *imageRef = fDocNode->GetDocumentNode()->RetainNode( id);
								xbox_assert(imageRef && imageRef->GetType() == kDOC_NODE_TYPE_IMAGE);
								VDocImage *image = dynamic_cast<VDocImage *>(imageRef);
								xbox_assert(image);
								style->GetSpanRef()->SetImage( image);
							}
						}
#endif
				}
				else
				{
					//no children styles: just append the span reference styles

					std::vector<VTreeTextStyle *>::iterator it = fStylesSpanRefs->begin();
					for (;it != fStylesSpanRefs->end(); it++)
					{
						VTextStyle *style = (*it)->GetData();
						if (style->GetSpanRef()->GetType() == kSpanTextRef_Image)
							style->GetSpanRef()->SetImage( style->GetSpanRef()->GetImage()); //ensure span ref reference & text are updated according to image src & alt text

						style->ResetCommonStyles( fStyles->GetData(), true);
						fStyles->AddChild( *it);
						(*it)->Release();
					}
				}
			}
			void _StartStyleSheet();
			void _EndStyleSheet();
			void _CreateCSSMatch();

			VSpanHandler*		fSpanHandlerRoot;
			VSpanHandler*		fSpanHandlerParent;
			VSpanHandler*		fSpanHandlerParagraph;
			VString*			fText;
			uLONG				fOptions;
			SpanRefCombinedTextTypeMask	fShowSpanRef;
			VTreeTextStyle*		fStyles;
			VTreeTextStyle*		fStylesParent;
			VTreeTextStyle*		fStylesRoot;
			VTextStyle*			fStyleInherit;
			bool				fStyleInheritForChildrenDone;
			VString				fElementName;
			sLONG				fStart;
			bool				fIsInBody;
			bool				fIsInHead;
			bool				fIsNodeParagraph, fIsNodeSpan, fIsNodeImage, fIsNodeStyle;
				
			std::vector<VTreeTextStyle *> *fStylesSpanRefs;
			VIndex				fDocNodeLevel;
			VDocNode*			fParentDocNode;
			VRefPtr<VDocNode>	fDocNode;
			bool				fIsUnknownTag;
			bool				fIsOpenTagClosed;
			bool				fIsUnknownRootTag;
			VString				fUnknownRootTagText;
			VString*			fUnknownTagText;
			bool				fPreserveWhiteSpaces;

			VString				fCSSInlineStyles, fCSSDefaultStyles;
			CSSRuleSet::MediaRules *fCSSRules;
			VCSSMatch*			fCSSMatch;

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
								VParserXHTML(uLONG inVersion = kDOC_VERSION_SPAN4D, const uLONG inDPI = 72, SpanRefCombinedTextTypeMask inShowSpanRefs = 0, VDBLanguageContext *inLC = NULL, uLONG inOptions = kHTML_PARSER_OPTIONS_DEFAULT)
								:VSpanHandler(NULL, CVSTR("html"), NULL, NULL, NULL, inVersion, inLC)
			{
				fShowSpanRef = inShowSpanRefs;
				fDBLanguageContext = fShowSpanRef & (kSRCTT_4DExp_Value|kSRCTT_4DExp_Normalized) ? inLC : NULL;
				fOptions = inOptions;
				fPreserveWhiteSpaces = (fOptions & kHTML_PARSER_PRESERVE_WHITESPACES) || inVersion == kDOC_VERSION_SPAN4D ? true : false; //override legacy XHTML default 

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
						//IMPORTANT: here only tags which can be embedded in 'span' or 'p' tags are allowed 
						//			 -> while parsing SPAN4D, XHTML11 ot XHTML4D these tags are converted to 'span' tag with the corresponding CSS style

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

						//style = RetainDefaultStyle("address"); 
						//style->GetData()->SetItalic(TRUE);
						//style->Release();

						//style = RetainDefaultStyle("center");
						//style->GetData()->SetJustification(JST_Center);
						//style->Release();

						//set HTML elements for which to add a extra break line 
						//(only mandatory for XHTML11 - because XHTML11 is parsed as a single paragraph 
						// - SPAN4D interprets these tags as unknown tags
						// - XHTML4D parses 'p' tags as VDocParagraph and skip other tags parsing - for now)
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
				if (fCSSRules)
					delete fCSSRules;
				if (fCSSMatch)
					delete fCSSMatch;
			}
	

			void				CloseParsing()
			{
				VDocText *docText = fDocNode->RetainDocumentNode();
				if (!docText->GetChildCount())
				{
					//at least one empty paragraph (empty text)
					VDocParagraph *para = new VDocParagraph();
					docText->AddChild( para);
					ReleaseRefCountable(&para);
				}
				ReleaseRefCountable(&docText);
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

			void				SetDPI( const uLONG inDPI) { fDocNode->GetDocumentNode()->SetDPI( inDPI); }

			/** SPAN format version - on default, it is assumed to be kDOC_VERSION_SPAN_1 if parsing SPAN tagged text (that is v12/v13 compatible) & kDOC_VERSION_XHTML11 if parsing XHTML (but 4D SPAN or 4D XHTML) 
			@remarks
				might be overriden with "d4-version" property
			*/
			uLONG				GetVersion() { return fDocNode->GetDocumentNode()->GetVersion(); }

			void				SetVersion( const uLONG inVersion) { fDocNode->GetDocumentNode()->SetVersion( inVersion); }

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
	fSpanHandlerRoot = inParent ? inParent->fSpanHandlerRoot : this;
	fSpanHandlerParent = inParent;
	fSpanHandlerParagraph = inParent ? inParent->fSpanHandlerParagraph : NULL;
	fOptions = inParent ? inParent->fOptions : 0;
	fElementName = inElementName;
	fShowSpanRef = inParent ? inParent->fShowSpanRef : 0;
	fDBLanguageContext = inLC;
	fStylesSpanRefs = NULL;
	fText = ioText; 
	fStart = fText ? fText->GetLength() : 0;
	fStylesRoot = NULL;
	fStyles = NULL;
	fStylesParent = NULL;
	fStyleInherit = NULL;
	fStyleInheritForChildrenDone = false;
	fIsInHead = inParent ? inParent->fIsInHead : false;
	fIsInBody = inParent ? inParent->fIsInBody : false;
	fIsNodeParagraph = false;
	fIsNodeSpan = false;
	fIsNodeImage = inParent ? inParent->fIsNodeImage : false;
	fIsNodeStyle = false;

	fDocNodeLevel = -1;
	fParentDocNode = NULL;
	fIsUnknownTag = false;
	fIsOpenTagClosed = false;
	fIsUnknownRootTag = false;
	fUnknownTagText = inParent ? inParent->fUnknownTagText : NULL;
	fPreserveWhiteSpaces = inParent ? inParent->fPreserveWhiteSpaces : false; //default for XHTML (XHTML4D always set CSS property 'white-space:pre-wrap' for paragraphs as we need to preserve paragraph whitespaces - especially for tabs)

	fCSSRules = NULL;
	fCSSMatch = NULL; //remark: do not inherit fCSSMatch here as it should be enabled only for some nodes

	if (inParent != NULL)
	{
		fParentDocNode = inParent->fDocNode.Get();
		fDocNode = inParent->fDocNode;
		fDocNodeLevel = inParent->fDocNodeLevel+1;

		uLONG version = fDocNode->GetDocumentNode()->GetVersion();
		if (!fIsInBody)
		{
			if (version == kDOC_VERSION_SPAN4D || IsBodyElementName( fElementName))
			{
#if ENABLE_VDOCTEXTLAYOUT
				if (version >= kDOC_VERSION_XHTML4D)
					//body element is CSS matchable
					_CreateCSSMatch();
#endif
				fIsInBody = true;
			}
#if ENABLE_VDOCTEXTLAYOUT
			else if (IsHeadElementName( fElementName))
				fIsInHead = true;  
#endif
		}	
#if ENABLE_VDOCTEXTLAYOUT
		if (fIsInHead)
		{
			//parse 'style' element style sheet if in 'head' element
			if (version >= kDOC_VERSION_XHTML4D && IsStyleElementName( fElementName))
			{
				fIsNodeStyle = true;
				xbox_assert(fText == NULL);
				fText = new VString();
				fStart = 0;
				_StartStyleSheet();
			}
		}
		else
#endif
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

					VDocText *doc = inParent->fDocNode->GetDocumentNode();
					
					VDocParagraph *para = new VDocParagraph();
					doc->AddChild( static_cast<VDocNode *>(para));
					fDocNode = static_cast<VDocNode *>(para);

#if ENABLE_VDOCTEXTLAYOUT
					if (version >= kDOC_VERSION_XHTML4D)
						//paragraph element is CSS matchable
						_CreateCSSMatch();
#endif

					fStylesParent = NULL;
					fStyles = new VTreeTextStyle( new VTextStyle());
					xbox_assert(fStylesRoot == NULL);
					fStylesRoot = fStyles;
					para->SetStyles( fStyles, false); //paragraph retains/shares current styles
					
					xbox_assert(ioText == NULL);
					fText = para->GetTextPtr();
					fText->SetEmpty();
					fStart = 0;

					para->Release();

					fSpanHandlerParagraph = this;
					fStylesSpanRefs = new std::vector<VTreeTextStyle *>();
				}
				else 
				{
					//paragraph or span child node (might be span, url, table, etc... but we parse all as span nodes for now)
					//TODO: we should parse tables as VDocTable

					xbox_assert(fText && !fIsNodeImage);

					fIsNodeSpan = IsSpanElementName( fElementName);

					xbox_assert(inStylesParent);
					fStylesParent = inStylesParent;
					fStyleInherit = inStyleInheritParent ? new VTextStyle(inStyleInheritParent) : NULL;

					bool isKnownTag = false;
					VTreeTextStyle *stylesDefault = VParserXHTML::GetAndRetainDefaultStyle(inElementName);
					if (stylesDefault)
					{
						fStyles = new VTreeTextStyle( stylesDefault);
						fStyles->GetData()->SetRange( fStart, fStart);
						if (fStylesParent)
							fStylesParent->AddChild( fStyles);
						ReleaseRefCountable(&stylesDefault);
						isKnownTag = true; //we treat tags such as 'b', 'i' and so on as 'span' with the corresponding style
					}
					fStylesRoot = fSpanHandlerParagraph->fStylesRoot;
					fStylesSpanRefs = NULL;

#if ENABLE_VDOCTEXTLAYOUT
					if ((fOptions & kHTML_PARSER_TAG_IMAGE_SUPPORTED) && IsImageElementName( fElementName))
					{
						//image node

						fIsNodeImage = true;

						//create image & attach it to the document
						VDocImage *image = new VDocImage();
						fDocNode->GetDocumentNode()->AddImage( image);

						if (!fStyles)
						{
							fStyles = new VTreeTextStyle( new VTextStyle());
							fStyles->GetData()->SetRange( fStart, fStart);
							if (testAssert(fStylesParent))
								fStylesParent->AddChild( fStyles);
						}

						//bind image to the current span style
						VDocSpanTextRef *spanRef = new VDocSpanTextRef( image);
						fStyles->GetData()->SetSpanRef( spanRef);
						if (fStyles->GetData()->IsSpanRef())
							fStyles->NotifySetSpanRef();

						fDocNode = static_cast<VDocNode *>(image);

#if ENABLE_VDOCTEXTLAYOUT
						if (version >= kDOC_VERSION_XHTML4D)
							//image element is CSS matchable
							_CreateCSSMatch();
#endif

						image->Release();
					}
					else
#endif
					if (!isKnownTag && version != kDOC_VERSION_XHTML11 && !IsSpanElementName( fElementName) && !IsUrlElementName( fElementName))
					{
						//SPAN4D or XHTML4D only -> unknown tag (for legacy XHTML11, we parse unknown tags as span text)

						fIsUnknownTag = true;
						fIsUnknownRootTag = true;
						_CreateStyles();
						VDocSpanTextRef *spanRef = new VDocSpanTextRef( kSpanTextRef_UnknownTag, VDocSpanTextRef::fSpace, VDocSpanTextRef::fSpace);
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
	
//VCSSMatch custom handlers (see VCSSMatch class) - here element is VDocNode instance and VDocNode parser VSpanHandler instance
namespace CSSHandlers
{

	/** GetParentElement handler
	@param inElement
		element opaque reference
	@return
		parent element opaque reference (can be NULL)
	*/

	VCSSMatch::opaque_ElementPtr GetParentElement( VCSSMatch::opaque_ElementPtr inElement)
	{
		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		return reinterpret_cast<VCSSMatch::opaque_ElementPtr>(node->GetParent());
	}


	/** GetPrevElement handler
	@param inElement
		element opaque reference
	@return
		previous element opaque reference (can be NULL)
	*/
	VCSSMatch::opaque_ElementPtr GetPrevElement( VCSSMatch::opaque_ElementPtr inElement)
	{
		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		if (node->GetParent() && node->GetChildIndex() > 0)
			return reinterpret_cast<VCSSMatch::opaque_ElementPtr>(node->GetParent()->GetChild(node->GetChildIndex()-1));
		return NULL;
	}


	/** GetElementName handler
	@param inElement
		element opaque reference (must be valid)
	@return
		element name
	*/
	const VString& GetElementName(VCSSMatch::opaque_ElementPtr inElement)
	{
		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		return node->GetElementName();
	}


	/** GetElementLangType handler
	@param inElement
		element opaque reference (must be valid)
	@return
		element inherited language type
	*/
	const VString& GetElementLangType(VCSSMatch::opaque_ElementPtr inElement)
	{
		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		return node->GetLangBCP47();
	}

	/** GetElementClass handler
	@param inElement
		element opaque reference (must be valid)
	@return
		element inherited class
	*/
	const VString& GetElementClass(VCSSMatch::opaque_ElementPtr inElement)
	{
		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		return node->GetClass();
	}


	/** GetAttributeValue handler
	@param inElement
		element opaque reference
	@param inName
		attribute name
	@param outValue
		returned attribute value
	@return 
		true if element has the attribute (in this case, outValue contains attribute value)
	*/
	bool GetAttributeValue(VCSSMatch::opaque_ElementPtr inElement, const VString& inName, VString& outValue)
	{
		//remark: here for VDoctext we allow only CSS rules matching id, class or lang but not attributes 
		//(as only CSS styles are mandatory in XHMTL4D)

		//VDocNode *node = reinterpret_cast<VSVGNode *>(inElement);
		outValue.SetEmpty();
		return false;
	}

	/** SetAttribute callback handler
		
		this callback is called by VCSSMatch::Match() for each attribute that is overridden by CSS rules (only one time per attribute)

	@param inElement
		element opaque reference 
	@param inName
		name of attribute to override
	@param inValue
		new value of attribute  
	@param inIsImportant
		true if attribute is important
	@param inValueParsed
		new value of attribute (pre-parsed by CSS parser) 
	*/
	void SetAttribute(VCSSMatch::opaque_ElementPtr inElement, const VString& inName, const VString& inValue, bool /*inIsImportant*/, const CSSProperty::Value* inValueParsed)
	{
		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		if (!testAssert(node->GetParserHandler()))
			return;
		node->GetParserHandler()->IDNPH_SetAttribute( inName, inValue);
	}
}

void VSpanHandler::_StartStyleSheet()
{
#if ENABLE_VDOCTEXTLAYOUT
	if (fSpanHandlerRoot->fCSSRules == NULL)
		fSpanHandlerRoot->fCSSRules = new CSSRuleSet::MediaRules();
#endif
}


void VSpanHandler::_EndStyleSheet()
{
#if ENABLE_VDOCTEXTLAYOUT
	if (!testAssert(fText))
		return;
	if (fSpanHandlerRoot->fCSSRules == NULL)
	{
		if (fText)
			delete fText;
		fText = NULL;
		return;
	}

	//parse embedded style sheet & store rules locally

	VCSSParser *parserCSS = new VCSSParser();
	try
	{
		//TODO: we should use database resource path as base url & support base url in XHTML4D document (in order to reference external style sheet)
		parserCSS->Parse( *fText, CVSTR(""), fSpanHandlerRoot->fCSSRules);
	}
	catch( VCSSException e)
	{
		//silently trap CSS errors - normally we should not throw error while parsing XHTML4D or SPAN4D 
		//TODO: we should optionally throw CSS errors (via xhtml reserved attribute ?)
	}
	delete parserCSS;

	delete fText;
	fText = NULL;
#endif
}

void VSpanHandler::_CreateCSSMatch()
{
#if ENABLE_VDOCTEXTLAYOUT
	if (fSpanHandlerRoot->fCSSRules == NULL)
		return;
	if (fSpanHandlerRoot->fCSSMatch == NULL)
	{
		fSpanHandlerRoot->fCSSMatch = new VCSSMatch();
		fSpanHandlerRoot->fCSSMatch->SetCSSRules( fSpanHandlerRoot->fCSSRules);
		fSpanHandlerRoot->fCSSMatch->SetHandlerGetAttributeValue( CSSHandlers::GetAttributeValue);
		fSpanHandlerRoot->fCSSMatch->SetHandlerGetElementLangType( CSSHandlers::GetElementLangType);
		fSpanHandlerRoot->fCSSMatch->SetHandlerGetElementClass( CSSHandlers::GetElementClass);
		fSpanHandlerRoot->fCSSMatch->SetHandlerGetElementName( CSSHandlers::GetElementName);
		fSpanHandlerRoot->fCSSMatch->SetHandlerGetParentElement( CSSHandlers::GetParentElement);
		fSpanHandlerRoot->fCSSMatch->SetHandlerGetPrevElement( CSSHandlers::GetPrevElement);
		fSpanHandlerRoot->fCSSMatch->SetHandlerSetAttribute( CSSHandlers::SetAttribute);
	}
	fCSSMatch = fSpanHandlerRoot->fCSSMatch;
	fDocNode->SetParserHandler( this);
#endif
}

void VSpanHandler::CloseParagraph()
{
	//finalize text & styles

	xbox_assert( fSpanHandlerParagraph == this && fStyles == fStylesRoot && fText);

	fText->ConvertCarriageReturns(eCRM_CR); //we need to convert back to CR because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)

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
	para->SetTextDirty();
}

VSpanHandler::~VSpanHandler()
{
	ReleaseRefCountable(&fStyles);
	if (fStyleInherit)
		delete fStyleInherit;
}

void  VSpanHandler::_CreateStyles()
{
	if (fStyles == NULL && fSpanHandlerParagraph) //we create character styles only if we currently parse a paragraph node or a paragraph child node 
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

	if (fIsNodeStyle)
		//'style' element should not have children
		return NULL;
	
	_ParseCSSStyles();

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
		//if we do not preserve line endings, we temporary replace <br> tag with bell character - which is forbidden in XML: 
		//it will be replaced on EndElement with CR while consolidating whitespaces (because we should not consolidate xml explicit line endings)

		xbox_assert(fSpanHandlerParagraph);
			
		if (fUnknownTagText)
			fUnknownTagText->AppendUniChar(VSTP_EXPLICIT_CR); //for unknown tag text value, spaces are always consolidated (only source text whitespaces may be preserved)

		if (fPreserveWhiteSpaces)
			*fText+="\r";
		else
			fText->AppendUniChar(VSTP_EXPLICIT_CR);
	}
	else
	{
		uLONG version = fDocNode->GetDocumentNode()->GetVersion();
		bool createHandler = fIsInBody || version == kDOC_VERSION_SPAN4D || IsHTMLElementName( inElementName) || IsBodyElementName( inElementName);
#if ENABLE_VDOCTEXTLAYOUT
		if (!createHandler && version != kDOC_VERSION_SPAN4D && (fIsInHead || IsHeadElementName( inElementName)))
			createHandler = true;
#endif
		if (createHandler && !fIsNodeImage) //we assume image node has none children 
		{
			_CreateStyles();
			VSpanHandler *spanHandler;
			result = spanHandler = new VSpanHandler(this, inElementName, fStyles, _GetStyleInheritForChildren(), fText, fDocNode->GetDocumentNode()->GetVersion(), fDBLanguageContext);	
			if (!fIsUnknownTag && fText && fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D && inElementName.EqualToString("q"))
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
	if (fIsNodeStyle)
		_EndStyleSheet();

	_ParseCSSStyles();

	if (!fSpanHandlerParagraph)
	{
		if (fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D && fDocNode->GetChildCount() > 0)
		{
			//maybe a paragraph container -> replace FF character by CR but truncate FF character for the last paragraph
			//(while parsing paragraphs, we automatically append FF on EndElement - see below - 
			//but last paragraph should not get automatic FF as it is not followed by another paragraph - otherwise it would make a extra line)
			VIndex count = fDocNode->GetChildCount();
			for (int i = 0; i < count; i++)
			{
				bool isLast = i == count-1;
				VDocNode *node = fDocNode->RetainChild(i);
				if (node->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
				{
					VDocParagraph *para = dynamic_cast<VDocParagraph *>(node);
					if (testAssert(para))
					{
						VString *text = para->GetTextPtr();
						if (text->GetLength() > 0 && text->GetUniChar(text->GetLength()) == '\f')
						{
							if (isLast)
								//last paragraph -> truncate ff
								para->ReplaceStyledText(CVSTR(""), NULL, text->GetLength()-1, text->GetLength(), false, true);
							else
							{
								//not last paragraph -> replace ff with cr
								VString cr;
								cr.AppendUniChar(13);
								para->ReplaceStyledText(cr, NULL, text->GetLength()-1, text->GetLength(), false, true);
							}
						}
					}
				}
				ReleaseRefCountable(&node);
			}
		}
		return;
	}
	xbox_assert(fText);

	VTextStyle *style = fStyles ? fStyles->GetData() : NULL;
	sLONG start, end;

	if (fSpanHandlerParagraph == this && !fPreserveWhiteSpaces)
	{
		xbox_assert(fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D);

		//XHTML11 or XHTML4D -> on default, consolidate whitespaces & line endings if 'white-space' property is not set to preserve whitespaces 
		//SPAN4D -> always preserve whitespaces
		//(property is automatically set to preserve whitespaces for paragraphs while exporting to XHTML4D:
		// so only legacy XHTML (XHTML11) might consolidate whitespaces here if exported XHTML4D document is not modified externally)

		if (style)
		{
			style->GetRange(start, end);
			end = fText->GetLength();
			style->SetRange(start, end);
		}

		ConsolidateWhiteSpaces(*fText, fStyles);
	}
	
	if (!fIsUnknownTag && fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D)
	{
		//XHTML parsing:

		if (inElementName.EqualToString("q"))
			//end of quote
			*fText += "\"";

		//add break lines for elements which need it - as paragraphs (normally only while parsing XHTML)
		if (VParserXHTML::NeedBreakLine(inElementName)) 
			*fText += "\f"; //will be replaced after by CR
	}

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
		style->GetSpanRef()->ShowRef( style->GetSpanRef()->CombinedTextTypeMaskToTextTypeMask(fShowSpanRef));
		if (style->GetSpanRef()->GetType() == kSpanTextRef_4DExp && style->GetSpanRef()->ShowRefNormalized() && (style->GetSpanRef()->ShowRef() & kSRTT_Src) != 0)
			style->GetSpanRef()->UpdateRefNormalized( fDBLanguageContext);

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
				if (!fPreserveWhiteSpaces)
					ConsolidateWhiteSpaces(reftext, NULL);
				style->GetSpanRef()->SetRef( reftext);
				ConsolidateWhiteSpaces(fUnknownRootTagText, NULL);	//for displaying unknown tag value as embedded plain text, we always consolidate whitespace
																	//(to prevent annoying CR for span ref value)
																	//(but we preserve unknown tag source text - parsed in fText - if white-space property is set to preserve it
																	//in order unknown tag source text is preserved in case of serializing)
				style->GetSpanRef()->SetDefaultValue( fUnknownRootTagText);

				fText->Truncate(fStart);
				VString value = style->GetSpanRef()->GetDisplayValue();
				fText->AppendString(value); 
				style->SetRange(fStart, fText->GetLength());
			}
		}
		else if (start <= end)
		{
			if (style->GetSpanRef()->GetType() != kSpanTextRef_Image)
			{
				if (start < end)
					fText->GetSubString( start+1, end-start, reftext);			
				else
					reftext.SetEmpty();

				reftext.ConvertCarriageReturns(eCRM_CR);
				style->GetSpanRef()->SetDefaultValue( reftext); //default value is set to initial parsed plain text
			}

			fText->Truncate(fStart);
			VString value = style->GetSpanRef()->GetDisplayValue();
			fText->AppendString(value); 
			style->SetRange(fStart, fText->GetLength());
		}
	}

	if (fSpanHandlerParagraph == this)
		CloseParagraph();
}

void VSpanHandler::_ParseCSSStyles()
{
#if ENABLE_VDOCTEXTLAYOUT
	uLONG version = fDocNode->GetDocumentNode()->GetVersion();
	if (version < kDOC_VERSION_XHTML4D)
		return;

	if (fCSSMatch && fDocNode->GetParserHandler())
	{
		if (fDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH)
		{
			//build paragraph styles from paragraph class names

			VString sclasses = fDocNode->GetClass();
			if (!sclasses.IsEmpty())
			{
				//store styles for this class as paragraph style sheets 

				VCSSLexParser *parser = new VCSSLexParser();
				parser->Start( sclasses.GetCPointer());
				parser->Next(true);
				while (parser->GetCurToken() == CSSToken::IDENT)
				{
					VString sclass = parser->GetCurTokenValue();

					//check first if paragraph style has not been parsed yet

					VDocParagraph *paraStyle = fDocNode->GetDocumentNode()->RetainParagraphStyle( sclass);
					if (paraStyle)
					{
						ReleaseRefCountable(&paraStyle);
						parser->Next(true);
						continue;
					}

					//add new paragraph style

					paraStyle = new VDocParagraph();
					VTreeTextStyle *styles = new VTreeTextStyle( new VTextStyle());
					paraStyle->SetStyles( styles, false);
					ReleaseRefCountable(&styles);

					paraStyle->SetClass( sclass); //temporary in order to apply CSS rules which match this node 
					paraStyle->SetParserHandler( this);

					//bind to the document
					fDocNode->GetDocumentNode()->AddOrReplaceParagraphStyle( sclass, paraStyle); 

					//gets styles from style sheet which match this paragraph
					fCSSDefaultStyles.SetEmpty();
					fCSSMatch->Match( reinterpret_cast<VCSSMatch::opaque_ElementPtr>(static_cast<VDocNode *>(paraStyle))); 
					if (!fCSSDefaultStyles.IsEmpty())
						VSpanTextParser::Get()->ParseCSSToVDocNode( static_cast<VDocNode *>(paraStyle), fCSSDefaultStyles, this);
					paraStyle->SetClass(CVSTR(""));
					paraStyle->SetParserHandler(NULL);
					ReleaseRefCountable(&paraStyle);

					parser->Next(true);
				}
				delete parser;
			}
		}

		//now get styles from style sheet which match current node (styles from style sheet will be stored in fCSSDefaultStyles)
		fCSSDefaultStyles.SetEmpty();
		fCSSMatch->Match( reinterpret_cast<VCSSMatch::opaque_ElementPtr>(fDocNode.Get()));
	}

	//now parsed both inline styles & styles from style sheet 
	if (!fCSSInlineStyles.IsEmpty())
	{
		if (fCSSDefaultStyles.IsEmpty())
			_ParseCSSStyles(fCSSInlineStyles);
		else
		{
			//inline styles override styles from style sheet
			fCSSInlineStyles = fCSSDefaultStyles+";"+fCSSInlineStyles;
			_ParseCSSStyles(fCSSInlineStyles);
		}
	}
	else if (!fCSSDefaultStyles.IsEmpty())
		_ParseCSSStyles(fCSSDefaultStyles);

	fCSSInlineStyles.SetEmpty();
	fCSSDefaultStyles.SetEmpty();
	//clear parser handler
	fDocNode->SetParserHandler(NULL);
#endif
}

void VSpanHandler::_ParseCSSStyles(const VString& inStyles)
{
	uLONG version = fDocNode->GetDocumentNode()->GetVersion();

	if (version != kDOC_VERSION_SPAN4D && IsBodyElementName(fElementName))
	{
		//XHTML11 or XHTML4D
		//node is body node: parse document properties
		xbox_assert( fDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT);
		VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, inStyles, this);
	}
	else if (fSpanHandlerParagraph)
	{
		//node is a paragraph or a span or a image

		_CreateStyles();
		
		if (this == fSpanHandlerParagraph) //paragraph element (XHTML4D) or span root element (SPAN4D)
		{
			//parse paragraph or character properties
			xbox_assert(!fIsNodeImage);
			VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, inStyles, this);
		}
		else 
		{
#if ENABLE_VDOCTEXTLAYOUT
			if (fIsNodeImage)
			{
				//embedded image
				xbox_assert((fOptions & kHTML_PARSER_TAG_IMAGE_SUPPORTED) != 0);

				//parse image common props
				VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, inStyles, this);
				//parse character styles (note that character styles are attached to the span styles & not to the embedded image node)
				VSpanTextParser::Get()->_ParseCSSToVTreeTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, inStyles, fStyles, _GetStyleInherit());

				//as background-color is image element color here & not character style, we reset it if it has been parsed again by _ParseCSSToVTreeTextStyle
				if (fStyles && fStyles->GetData()->GetHasBackColor())
					fStyles->GetData()->SetHasBackColor(false);
			}
			else
#endif
				//span or url node -> parse character styles
				VSpanTextParser::Get()->_ParseCSSToVTreeTextStyle( fDBLanguageContext, (VDocNode *)fDocNode, inStyles, fStyles, _GetStyleInherit());
		}
	}
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
#if ENABLE_VDOCTEXTLAYOUT
	else if (	fDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D 
				&& inName.EqualToString("id") 
				&& (IsBodyElementName(fElementName) || IsParagraphElementName(fElementName) || IsImageElementName(fElementName)))
	{
		if (testAssert(fDocNode.Get()))
		{
			VDocNode *node = fDocNode->GetDocumentNode()->RetainNode( inValue);

			fDocNode->SetID( inValue);
		}
	}
	else if (	fDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D 
				&& inName.EqualToString("class") 
				&& (IsBodyElementName(fElementName) || IsParagraphElementName(fElementName) || IsImageElementName(fElementName)))
	{
		if (testAssert(fDocNode.Get()))
			fDocNode->SetClass( inValue);
	}
#endif
	else if (inName.EqualToString("style"))
	{
#if ENABLE_VDOCTEXTLAYOUT
		uLONG version = fDocNode->GetDocumentNode()->GetVersion();
		if (version < kDOC_VERSION_XHTML4D)
#endif
			_ParseCSSStyles( inValue);
#if ENABLE_VDOCTEXTLAYOUT
		else
			fCSSInlineStyles = inValue; //will be parsed after all other attributes have been parsed
#endif
	} 
#if ENABLE_VDOCTEXTLAYOUT
	else if (fIsNodeImage)
	{	
		if (inName.EqualToString("src"))
			VSpanTextParser::Get()->ParseAttributeImage( dynamic_cast<VDocImage *>(fDocNode.Get()), kDOC_PROP_IMG_SOURCE, inValue);
		else if (inName.EqualToString("alt"))
			VSpanTextParser::Get()->ParseAttributeImage( dynamic_cast<VDocImage *>(fDocNode.Get()), kDOC_PROP_IMG_ALT_TEXT, inValue);
		else
			SetHTMLAttribute( inName, inValue);
	}
#endif
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
#if ENABLE_VDOCTEXTLAYOUT
	else if ((!(fOptions & kHTML_PARSER_DISABLE_XHTML4D)) && inName.EqualToString("xmlns:d4"))
		fDocNode->GetDocumentNode()->SetVersion( kDOC_VERSION_XHTML4D);
#endif
	else if (fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D)
	{
		//we tolerate some HTML attributes with XHTML 
		SetHTMLAttribute( inName, inValue);
	}
}

void VSpanHandler::SetHTMLAttribute(const VString& inName, const VString& inValue)
{
	//convert to CSS - no performance hit here as we should never fall here while parsing private 4D SPAN or 4D XHTML (which uses only CSS styles)
	//but only while parsing XHTML fragment from paste or drag drop from external source like from Web browser
	//(and paste and drag drop action is not done multiple times per frame)

	VString valueCSS;
	if (inName.EqualToString(kCSS_NAME_WIDTH) && fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D) //elements as images might not use inline CSS style but html attribute 
	{
		valueCSS.AppendString(VString(kCSS_NAME_WIDTH)+":"+inValue);
		_CreateStyles();
		VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, valueCSS, this);
	}
	else if (inName.EqualToString(kCSS_NAME_HEIGHT) && fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D) //elements as images might not use inline CSS style but html attribute 
	{
		valueCSS.AppendString(VString(kCSS_NAME_HEIGHT)+":"+inValue);
		_CreateStyles();
		VSpanTextParser::Get()->ParseCSSToVDocNode( (VDocNode *)fDocNode, valueCSS, this);
	}
	else if (inName.EqualToString("face"))
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
	if (!fSpanHandlerParagraph && !fIsNodeStyle)
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
		VSpanTextParser::Get()->TextToXML( text, true, true, false); //do not convert CR to BR tag otherwise it may screw up tag 
		*fText += text;
		if (fUnknownTagText)
			*fUnknownTagText += inText;
	}
	else
		*fText += inText;
}


void VSpanHandler::ConsolidateWhiteSpaces(VString &ioText, VTreeTextStyle *ioStyles)
{
	//consolidate whitespaces according to XHTML W3C

	VIntlMgr *intlMgr = VIntlMgr::GetDefaultMgr();
	xbox_assert(intlMgr);

	sLONG countStyles = 0;
	sLONG countSpanRefs = fStylesSpanRefs ? fStylesSpanRefs->size() : 0;
	if (ioStyles && countSpanRefs)
	{
		//temporary append span ref styles 
		xbox_assert(!intlMgr->IsSpace(VSTP_SPANREF_WHITESPACE, true));

		countStyles = ioStyles->GetChildCount();
		for (int i = 0; i < countSpanRefs; i++)
		{
			VTreeTextStyle *styles = (*fStylesSpanRefs)[i];
			sLONG start, end;
			styles->GetData()->GetRange(start, end);
			xbox_assert(start < end);

			//ensure we preserve span ref whitespaces
			VString srctext, dsttext;
			dsttext.EnsureSize(srctext.GetLength());
			ioText.GetSubString(start+1,end-start,srctext);
			const UniChar *c = srctext.GetCPointer();
			while(*c)
			{
				if (intlMgr->IsSpace( *c, true))
					dsttext.AppendUniChar(VSTP_SPANREF_WHITESPACE); //temporary replace span ref whitespaces with 127 (invalid char in XML) - because span ref whitespaces need to be preserved
				else
					dsttext.AppendUniChar(*c);
				c++;
			}
			ioText.Replace(dsttext, start+1, end-start);

			//append styles
			ioStyles->AddChild(styles);
		}
	}

	VString text;
	const UniChar *c = ioText.GetCPointer();
	const UniChar *cSpaceStart = NULL;
	text.EnsureSize(ioText.GetLength());
	bool parsingSpaces = false;
	sLONG index = 0;
	sLONG startTruncate;
	sLONG startLeadingSpaces = 0;
	while (*c)
	{
		if (intlMgr->IsSpace( *c, true)) //excluding no-break spaces
		{
			if (!parsingSpaces)
			{
				parsingSpaces = true;
				if (index > startLeadingSpaces)
				{
					startTruncate = index+1;
					text.AppendUniChar(' ');	//any space (including CR & LF) is consolidated with whitespace (cf XHTML W3C) 
												//- only <br/> tags can add a line ending if 'white-space' property is not set to preserve spaces 
				}
				else
					//leading spaces -> truncate all
					startTruncate = startLeadingSpaces;
			}
			c++;
		}
		else
		{
			if (parsingSpaces)
			{
				parsingSpaces = false;

				if (*c == VSTP_EXPLICIT_CR) //explicit CR -> trailing spaces -> truncate all trailing spaces
				{
					if (startTruncate > startLeadingSpaces)
					{
						startTruncate--;
						if (text.GetLength() > 0)
							text.Truncate(text.GetLength()-1);
					}
				}
				if (index > startTruncate)
				{
					if (ioStyles)
						ioStyles->Truncate(startTruncate, index-startTruncate);
					index = startTruncate;
				}
			}
			if (*c == VSTP_EXPLICIT_CR) //as it is forbidden in XML, it is a temporary replacement for explicit CR (see StartElement)
			{
				text.AppendUniChar(13);
				c++;
				startLeadingSpaces = index+1;
			}
			else
				text.AppendUniChar(*c++);
		}
		index++;
	}
	if (parsingSpaces)
	{
		//trailing spaces -> truncate all
		parsingSpaces = false;
		if (startTruncate > startLeadingSpaces)
		{
			startTruncate--;
			if (text.GetLength() > 0)
				text.Truncate(text.GetLength()-1);
		}
		if (index > startTruncate)
		{
			if (ioStyles)
				ioStyles->Truncate(startTruncate, index-startTruncate);
		}
	}
	ioText = text; //we always copy as generally legacy XHMTL documents have line endings...

	if (ioStyles && countSpanRefs)
	{
		//detach span ref styles 
		int childIndex = ioStyles->GetChildCount();
		for (int i = 0; i < countSpanRefs; i++, childIndex--)
			ioStyles->RemoveChildAt(childIndex);
		//xbox_assert(ioStyles->GetChildCount() == countStyles);

		ioText.ExchangeAll(VSTP_SPANREF_WHITESPACE,' '); //restore span references whitespaces
	}
}

//////////////////////////////////////////////////////////////////////////////////////


	


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
	if (!inParseSpanOnly)
		outText.RemoveWhiteSpaces(true, true, NULL, NULL, VIntlMgr::GetDefaultMgr());
	outText.ConvertCarriageReturns( eCRM_CR);

	TextToXML(outText, true, false, inParseSpanOnly); //fix XML invalid chars & convert CR if not done yet but do not convert XML markup as text should be XML text yet
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
	true (default): input text is 4D SPAN tagged text (version depending on "d4-version" CSS property) 
	false: input text is XHTML 1.1 fragment (generally from external source than 4D) 
*/
void VSpanTextParser::ParseSpanText(	const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly, const uLONG inDPI, 
										SpanRefCombinedTextTypeMask inShowSpanRefs, VDBLanguageContext *inLC, uLONG inOptions)
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

	//fixed ACI0082557: if text is not valid HTML, we should assume it is plain text only without triggering error 
	//(as tagged text might be either HTML text or plain text which are both valid datasources)
	StErrorContextInstaller errorContext(false);

	VParserXHTML vsaxhandler(inParseSpanOnly ? kDOC_VERSION_SPAN4D : kDOC_VERSION_XHTML11, inDPI, inShowSpanRefs, inLC, inOptions);
	VXMLParser vSAXParser;

	if (vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever))
	{
		vsaxhandler.CloseParsing(); //finalize text & styles if appropriate
		
		VDocText *doc = vsaxhandler.RetainDocument();
		doc->GetText(outPlainText);
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
	VDocText *docDefault = NULL;
	if (inDefaultStyle)
	{
		paraDefault = new VDocParagraph();
		VTreeTextStyle *stylesDefault = new VTreeTextStyle( new VTextStyle( inDefaultStyle));
		paraDefault->SetStyles( stylesDefault, false);
		ReleaseRefCountable(&stylesDefault);

		docDefault = new VDocText();
		docDefault->AddChild( paraDefault);
	}
	SerializeDocument( doc, outTaggedText, docDefault);

	ReleaseRefCountable(&para);
	ReleaseRefCountable(&paraDefault);
	ReleaseRefCountable(&docDefault);
	ReleaseRefCountable(&doc);
}


void VSpanTextParser::GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly, 
												SpanRefCombinedTextTypeMask inShowSpanRefs, VDBLanguageContext *inLC)
{
	VTreeTextStyle*	vUniformStyles = NULL;
	ParseSpanText(inTaggedText, vUniformStyles, outPlainText, inParseSpanOnly, 72, inShowSpanRefs, inLC);
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

	VDocText *doc = VSpanTextParser::Get()->ParseDocument( inSpanText, kDOC_VERSION_SPAN4D, 72, 0, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
	if (!doc)
		return false;

	sLONG docTextLength = doc->GetTextLength();
	
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D, 72, 0, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
	if (!doc)
		return false;

	VIndex docTextLength = doc->GetTextLength();
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
		inDoc = VSpanTextParser::Get()->ParseDocument( inSpanTextOrPlainText, kDOC_VERSION_SPAN4D, 72, 0, inSilentlyTrapParsingErrors);
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D, 72, 0, inSilentlyTrapParsingErrors); 
	if (!doc)
		return false;

	VIndex docTextLength = doc->GetTextLength();
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( inSpanText, kDOC_VERSION_SPAN4D, 72, 0, true); 
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
	VDocText *doc = VSpanTextParser::Get()->ParseDocument( ioSpanText, kDOC_VERSION_SPAN4D, 72, 0, inSilentlyTrapParsingErrors); //no need to eval span refs here (and as input range is uniform we need uniform span ref range that is 1 char size)
	if (!doc)
		return false;

	VIndex docTextLength = doc->GetTextLength();
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
											SpanRefCombinedTextTypeMask inShowSpanRefs, bool inSilentlyTrapParsingErrors, VDBLanguageContext *inLC, uLONG inOptions)
{
	VTaskLock protect(&fMutexParser); //protect parsing

	//if (inDocText.IsEmpty())
	//	return NULL;

	uLONG version = inVersion;
	if (inDocText.BeginsWith("<html")) //for compat with future 4D XHTML format
		version = kDOC_VERSION_XHTML11;

	VString vtext;
	_CheckAndFixXMLText( inDocText, vtext, version ==  kDOC_VERSION_SPAN4D);

	VParserXHTML vsaxhandler(version, inDPI, inShowSpanRefs, inLC, inOptions);
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


/** serialize document (formatting depending on document version) 
@remarks
	if (inDocDefaultStyle != NULL):
		if inSerializeDefaultStyle, serialize inDocDefaultStyle properties & styles
		if !inSerializeDefaultStyle, only serialize properties & styles which are not equal to inDocDefaultStyle properties & styles
*/
void VSpanTextParser::SerializeDocument( const VDocText *inDoc, VString& outDocText, const VDocText *inDocDefaultStyle, bool inSerializeDefaultStyle)
{
	xbox_assert(inDoc);

	outDocText.SetEmpty();

	fIsOnlyPlainText = true;

	sLONG indent = 0;

	const VDocParagraph *inParaDefaultStyle = inDocDefaultStyle ? inDocDefaultStyle->GetFirstParagraph() : NULL;

	VTextStyle *styleInherit = NULL;
	styleInherit = new VTextStyle();
	styleInherit->SetBold( 0);
	styleInherit->SetItalic( 0);
	styleInherit->SetStrikeout( 0);
	styleInherit->SetUnderline( 0);

	if (inParaDefaultStyle && inParaDefaultStyle->GetStyles())
		//do not serialize default styles (they are serialized only as style sheet rule or in root span if inSerializeDefaultStyle == true)
		styleInherit->MergeWithStyle( inParaDefaultStyle->GetStyles()->GetData());

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
	{
		_OpenTagDoc(outDocText); 
		_CloseTagDoc(outDocText, false);

		//header & style element
		_OpenTagHead( outDocText, ++indent);
		_CloseTagHead( outDocText, false);
		_OpenTagStyle( outDocText, ++indent);
		_CloseTagStyle( outDocText, false);

		if (inSerializeDefaultStyle && inDocDefaultStyle)
		{
			xbox_assert(inDocDefaultStyle->GetVersion() >= kDOC_VERSION_XHTML4D);

			//append default body styles

			if (!inDoc->HasProps())
			{
				sLONG lengthBefore = outDocText.GetLength();
				_NewLine( outDocText);
				_Indent( outDocText, indent);
				outDocText.AppendCString("body { ");
				sLONG lengthBeforeStyle = outDocText.GetLength();
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDocDefaultStyle), outDocText);
				if (lengthBeforeStyle == outDocText.GetLength())
					outDocText.Truncate(lengthBefore);
				else
					outDocText.AppendCString( " }");
			}
			
			if (inParaDefaultStyle)
			{
				//append default paragraph styles

				VTextStyle *styleInheritDefault = new VTextStyle();
				styleInheritDefault->SetBold( 0);
				styleInheritDefault->SetItalic( 0);
				styleInheritDefault->SetStrikeout( 0);
				styleInheritDefault->SetUnderline( 0);

				sLONG lengthBefore = outDocText.GetLength();
				_NewLine( outDocText);
				_Indent( outDocText, indent);
				outDocText.AppendCString("p,li { white-space:pre-wrap"); //always preserve paragraph whitespaces (otherwise tabs would be consolidated while parsing - cf W3C)
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), outDocText, NULL, false);
				VTextStyle *style = inParaDefaultStyle->GetStyles() ? inParaDefaultStyle->GetStyles()->GetData() : NULL;
				if (style)
					_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), *style, outDocText, styleInheritDefault, false, false);
				outDocText.AppendCString( " }");

				delete styleInheritDefault;
			}
		}
		if (!inSerializeDefaultStyle || !inParaDefaultStyle)
		{
			_NewLine( outDocText);
			_Indent( outDocText, ++indent);
			outDocText.AppendCString("p,li { white-space:pre-wrap }"); 	//always preserve paragraph whitespaces (otherwise tabs would be consolidated while parsing - cf W3C)
																		//note: it is important because on default whitespaces are consolidated by browsers
																		//(while parsing legacy XHTML11 or XHTML4D, we consolidate white-spaces if CSS property is not set to preserve whitespaces - see ConsolidateWhiteSpaces)
		}

		//append body styles (but default)
		if (inDoc->HasProps()) 
		{
			sLONG lengthBefore = outDocText.GetLength();
			_NewLine( outDocText);
			_Indent( outDocText, indent);
			outDocText.AppendCString("body { ");
			sLONG lengthBeforeStyle = outDocText.GetLength();
			if (inSerializeDefaultStyle && inDocDefaultStyle)
			{
				VDocText *doc = dynamic_cast<VDocText *>(inDocDefaultStyle->Clone());
				xbox_assert(doc);
				doc->AppendPropsFrom( static_cast<const VDocNode *>(inDoc), false, true);

				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(doc), outDocText);
			}
			else
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDoc), outDocText, static_cast<const VDocNode *>(inDocDefaultStyle));

			if (lengthBeforeStyle == outDocText.GetLength())
				outDocText.Truncate(lengthBefore);
			else
				outDocText.AppendCString( " }");
		}

		//append class style sheets if any
		if (inDoc->GetParagraphStyleCount() > 0)
		{
			for (int iStyle = 0; iStyle < inDoc->GetParagraphStyleCount(); iStyle++)
			{
				VString sclass;
				VDocParagraph *para = inDoc->RetainParagraphStyle( iStyle, sclass);
				xbox_assert(para && !sclass.IsEmpty());

				sLONG lengthBefore = outDocText.GetLength();
				_NewLine( outDocText);
				_Indent( outDocText, indent);
				outDocText.AppendCString("p.");
				outDocText.AppendString(sclass);
				outDocText.AppendCString(" { ");

				sLONG lengthBeforeStyle = outDocText.GetLength();
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(para), outDocText, inParaDefaultStyle);
				VTextStyle *style = para->GetStyles() ? para->GetStyles()->GetData() : NULL;
				if (style)
					_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), *style, outDocText, styleInherit, false, outDocText.GetLength() == lengthBeforeStyle);
				
				if (lengthBeforeStyle == outDocText.GetLength())
					//we append at least white-space property in order class style sheet to be not empty
					//(might happen if inParaDefaultStyle is equal to para but we need to keep class style sheet so serialize at least one property)
					outDocText.AppendCString("white-space:pre-wrap");
				outDocText.AppendCString( " }");
			}
		}

		indent--;
		_CloseTagStyle( outDocText, true, indent--);
		_CloseTagHead( outDocText, true, indent--);
		xbox_assert(indent == 0);

		//serialize document
		_OpenTagBody(outDocText, ++indent);
		_SerializeAttributeClass( static_cast<const VDocNode *>(inDoc), outDocText);
		/* //now it is serialized in style sheet
		if (inDoc->HasProps()) 
		{
			//serialize document properties

			VIndex lengthBeforeStyle = outDocText.GetLength();
			outDocText.AppendCString(" style=\"");
			VIndex start = outDocText.GetLength();

			_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDoc), outDocText, static_cast<const VDocNode *>(inDocDefaultStyle));

			if (outDocText.GetLength() > start)
			{
				outDocText.AppendUniChar('\"');
				fIsOnlyPlainText = false;
			}
			else
				outDocText.Truncate( lengthBeforeStyle);
		}
		*/
		_CloseTagBody(outDocText, false);
	}
#endif

	//serialize paragraphs

	for (VIndex iChild = 0; iChild < inDoc->GetChildCount(); iChild++)
	{
		const VDocNode *child = inDoc->GetChild(iChild);
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(child);
		xbox_assert(para); //TODO: serialize div

		VIndex lengthBeforeStyle;
		VIndex startCSS;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
		{
			//XHTML4D

			_OpenTagParagraph(outDocText, ++indent); 
			_SerializeAttributeClass( static_cast<const VDocNode *>(inDoc), outDocText);

			lengthBeforeStyle = outDocText.GetLength();
		}
		else
#endif
		{
			//SPAN4D

			xbox_assert(iChild == 0); //only one paragraph for span4D
			_OpenTagSpan(outDocText);
			
			lengthBeforeStyle = outDocText.GetLength();

			if (inSerializeDefaultStyle && inParaDefaultStyle)
			{
				//span4D: we cannot use style sheet so serialize default properties here

				outDocText.AppendCString(" style=\"");
				startCSS = outDocText.GetLength();

				//serialize default paragraph properties & styles

				xbox_assert(inDocDefaultStyle->GetVersion() < kDOC_VERSION_XHTML4D);

				//serialize paragraph properties (but character styles)
				if (inParaDefaultStyle->HasProps())
					_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), outDocText, NULL, outDocText.GetLength() == startCSS);
				
				//serialize paragraph character styles
				VTextStyle *style = inParaDefaultStyle->GetStyles() ? inParaDefaultStyle->GetStyles()->GetData() : NULL;
				if (style)
				{
					VTextStyle *styleInheritDefault = new VTextStyle();
					styleInheritDefault->SetBold( 0);
					styleInheritDefault->SetItalic( 0);
					styleInheritDefault->SetStrikeout( 0);
					styleInheritDefault->SetUnderline( 0);

					_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), *style, outDocText, styleInheritDefault, false, outDocText.GetLength() == startCSS);
					
					delete styleInheritDefault;
				}

				if (outDocText.GetLength() == startCSS)
					outDocText.Truncate(lengthBeforeStyle);
			}
		}

		if (para->HasProps() || para->GetStyles()) 
		{
			//serialize paragraph props + paragraph character styles
			if (outDocText.GetLength() == lengthBeforeStyle)
			{
				outDocText.AppendCString(" style=\"");
				startCSS = outDocText.GetLength();
			}

			const VDocParagraph *paraDefaultStyle = inParaDefaultStyle;
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
			VDocParagraph *paraDefaultStyleOverride = NULL;
			VTextStyle *styleInheritBackup = NULL;
			if (!para->GetClass().IsEmpty())
			{
				xbox_assert(inDoc->GetVersion() >= kDOC_VERSION_XHTML4D);

				VCSSLexParser *parser = new VCSSLexParser();
				parser->Start( para->GetClass().GetCPointer());
				parser->Next(true);
				while (parser->GetCurToken() == CSSToken::IDENT)
				{
					VString sclass = parser->GetCurTokenValue();

					//do not serialize styles from class style sheet (because they are serialized in head style element)
					VDocParagraph* paraStyle = inDoc->RetainParagraphStyle(sclass);
					if (testAssert(paraStyle))
					{
						//ensure character styles from class style sheet are not serialized
						if (!styleInheritBackup)
						{
							styleInheritBackup = styleInherit;
							styleInherit = new VTextStyle( styleInherit);
						}
						styleInherit->MergeWithStyle( paraStyle->GetStyles()->GetData());

						if (paraDefaultStyle)
						{
							//merge paragraph style sheet with default style sheet 
							//(paragraph style sheet overrides default and we do not need to serialize both)

							VDocParagraph *paraOverride = dynamic_cast<VDocParagraph *>(paraStyle->Clone());
							ReleaseRefCountable(&paraStyle);
							xbox_assert(paraOverride);
							paraOverride->AppendPropsFrom( paraDefaultStyle, true, true);
							CopyRefCountable(&paraDefaultStyleOverride, paraOverride);
						}
						else
							paraDefaultStyleOverride = paraStyle;
						paraDefaultStyle = paraDefaultStyleOverride;
					}

					parser->Next(true);
				}
				delete parser;
			}
#endif

			//serialize paragraph properties (but character styles)
			if (para->HasProps())
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(para), outDocText, static_cast<const VDocNode *>(paraDefaultStyle), outDocText.GetLength() == startCSS);

			//serialize paragraph character styles
			VTextStyle *style = para->GetStyles() ? para->GetStyles()->GetData() : NULL;
			if (style)
				_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(para), *style, outDocText, styleInherit, false, outDocText.GetLength() == startCSS);
			
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
			if (styleInheritBackup)
			{
				delete styleInherit;
				styleInherit = styleInheritBackup;
				styleInheritBackup = NULL;
			}
			ReleaseRefCountable(&paraDefaultStyleOverride);
#endif
			if (outDocText.GetLength() > startCSS)
			{
				outDocText.AppendUniChar('\"');
				fIsOnlyPlainText = false;
			}
			else
				outDocText.Truncate( lengthBeforeStyle);
		}
		else if (outDocText.GetLength() != lengthBeforeStyle)
		{
			if (outDocText.GetLength() > startCSS)
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
			_CloseTagParagraph( outDocText, true, indent--); 
		else
#endif
			_CloseTagSpan( outDocText);

	}

	if (styleInherit)
		delete styleInherit;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	//close document tag
	if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
	{
		_CloseTagBody( outDocText, true, indent--);
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

		VString intext;
		inDoc->GetText( intext);
		VDocText *doc = ParseDocument( intext); 
		if (!doc)
			//invalid XML -> we can return plain text
			outDocText.FromString( intext);
		else
		{
			VString text;
			doc->GetText( text);
			VTreeTextStyle *styles = doc->RetainStyles();
			if (styles == NULL && text.EqualToStringRaw( intext))
				//plain text does not contain any XML tag or escaped characters -> we can return plain text
				outDocText.FromString( intext);
			ReleaseRefCountable(&styles);
		}
	}
}

void VSpanTextParser::_OpenTagHead(VString& ioText, sLONG inIndent)
{
	_NewLine(ioText);
	_Indent( ioText, inIndent);
	ioText.AppendCString( "<head");
}

void VSpanTextParser::_CloseTagHead(VString& ioText, bool inAfterChildren, sLONG inIndent)
{
	if (inAfterChildren)
	{
		_NewLine(ioText);
		_Indent( ioText, inIndent);
		ioText.AppendCString( "</head>");
	}
	else
		ioText.AppendCString( ">");
}


void VSpanTextParser::_OpenTagStyle(VString& ioText, sLONG inIndent)
{
	_NewLine(ioText);
	_Indent( ioText, inIndent);
	ioText.AppendCString( "<style");
}

void VSpanTextParser::_CloseTagStyle(VString& ioText, bool inAfterChildren, sLONG inIndent)
{
	if (inAfterChildren)
	{
		_NewLine(ioText);
		_Indent( ioText, inIndent);
		ioText.AppendCString( "</style>");
	}
	else
		ioText.AppendCString( ">");
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
	//note: 'xmlns:d4' namespace is mandatory for XHTML4D html document (it identifies html document as a XHTML4D document)
	ioText.AppendCString("<html xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:d4=\"http://www.4D.com\"");
}

void VSpanTextParser::_CloseTagDoc(VString& ioText, bool inAfterChildren)
{
	if (inAfterChildren)
	{
		_NewLine(ioText);
		ioText.AppendCString( "</html>");
	}
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagBody(VString& ioText, sLONG inIndent)
{
	_NewLine(ioText);
	_Indent( ioText, inIndent);
	ioText.AppendCString("<body");
}

void VSpanTextParser::_CloseTagBody(VString& ioText, bool inAfterChildren, sLONG inIndent)
{
	if (inAfterChildren)
	{
		_NewLine(ioText);
		_Indent( ioText, inIndent);
		ioText.AppendCString( "</body>");
	}
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_OpenTagParagraph(VString& ioText, sLONG inIndent)
{
	_NewLine(ioText);
	_Indent( ioText, inIndent);
	ioText.AppendCString( "<p"); 
}

void VSpanTextParser::_CloseTagParagraph(VString& ioText, bool inAfterChildren, sLONG inIndent)
{
	if (inAfterChildren)
		//do not indent here as paragraph content white-spaces are preserved between <p> and </p> (!) - otherwise tabs would be consolidated
		ioText.AppendCString( "</p>");
	else
		ioText.AppendCString( ">");
}

void VSpanTextParser::_TruncateTrailingEndOfLine(VString& ioText)
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
			if (isXHTML4D && !inDocNode->IsLastChild(true))
				//truncate trailing CR if any (the paragraph closing tag itself defines the end of line)
				_TruncateTrailingEndOfLine( text);
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
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isImage = style->IsSpanRef() && style->GetSpanRef()->GetType() == kSpanTextRef_Image;
#else
	bool isImage = false;
#endif

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

			if ((isUnknownTag || isURL || isImage) && hasStyle)
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
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
				else if (isImage)
				{
					ioText.AppendCString( "<img");
					_SerializeAttributeClass( static_cast<VDocNode *>(style->GetSpanRef()->GetImage()), ioText);
					SerializeAttributesImage( static_cast<VDocNode *>(style->GetSpanRef()->GetImage()), style->GetSpanRef(), ioText);
					sLONG lengthBeforeStyle = ioText.GetLength();
					ioText.AppendCString(" style=\"");
					sLONG start = ioText.GetLength();
					_SerializePropsCommon( static_cast<VDocNode *>(style->GetSpanRef()->GetImage()), ioText, NULL);
					if (ioText.GetLength() > start)
						ioText.AppendUniChar('\"');
					else
						ioText.Truncate(lengthBeforeStyle);
				}
#endif
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
					if (isURL)
					{
						ioText.AppendCString( "<a");
						SerializeAttributeHRef( inDocNode, style->GetSpanRef(), ioText);
					}
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
					else if (isImage)
					{
						ioText.AppendCString( "<img");
						SerializeAttributesImage( static_cast<VDocNode *>(style->GetSpanRef()->GetImage()), style->GetSpanRef(), ioText);
						ioText.AppendCString(" style=\"");
						_SerializePropsCommon( static_cast<VDocNode *>(style->GetSpanRef()->GetImage()), ioText, NULL);
						ioText.AppendUniChar('\"');
					}
#endif
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
				if (isXHTML4D && childStart >= inPlainText.GetLength() && !inDocNode->IsLastChild(true))
					//truncate trailing CR if any (the paragraph closing tag itself defines the end of line)
					_TruncateTrailingEndOfLine( text);
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
		case kSpanTextRef_4DExp: //default value is space
			{
			fIsOnlyPlainText = false;

			VString text;
			text = style->GetSpanRef()->GetDefaultValue(); 
			if (text.IsEmpty() || style->GetSpanRef()->GetType() == kSpanTextRef_4DExp)
				text = VDocSpanTextRef::fSpace;
			TextToXML(text, true, true, true);
			ioText.AppendString( text);
			}
			break;
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		case kSpanTextRef_Image: 
			{
			fIsOnlyPlainText = false;
			ioText.AppendString( VDocSpanTextRef::fSpace);
			}
			break;
#endif
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
		if (isXHTML4D && end >= inPlainText.GetLength() && !inDocNode->IsLastChild(true))
			//truncate trailing CR if any (the paragraph closing tag itself defines the end of line)
			_TruncateTrailingEndOfLine( text);
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
			if (isURL)
				ioText.AppendCString( "</a>");
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
			else if (isImage)
				ioText.AppendCString( "</img>");
#endif
			else
				_CloseTagSpan(ioText, true);
		}
		
		if (needToEmbedSpanRef)
			_CloseTagSpan(ioText, true); //close span container 
	}
}

/** parse CSS white-space property */ 
void VSpanTextParser::_ParseCSSWhiteSpace( VSpanHandler *inSpanHandler, const VString& inValue)
{
	if (!inSpanHandler)
		return;
	if (inValue.EqualToString("normal"))
		inSpanHandler->PreserveWhiteSpaces(false);
	else if (inValue.EqualToString("nowrap"))
		inSpanHandler->PreserveWhiteSpaces(false);
	else if (!inValue.EqualToString("inherit"))
		inSpanHandler->PreserveWhiteSpaces(true);
}


/** initialize VDocNode props from CSS declaration string */ 
void VSpanTextParser::ParseCSSToVDocNode( VDocNode *ioDocNode, const VString& inCSSDeclarations, VSpanHandler *inSpanHandler)
{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isXHTML4D = ioDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D;
#endif
	bool isSpan4D = ioDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D;
	
	xbox_assert(ioDocNode && ioDocNode->GetDocumentNode());

	//prepare inline CSS parsing

	VCSSLexParser *lexParser = new VCSSLexParser();

	VCSSParserInlineDeclarations cssParser(true);
	cssParser.Start(inCSSDeclarations, ioDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D);
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
					else if (attribut == kCSS_NAME_D4_DWRITE)
						//CSS -d4-dwrite
						_ParsePropDoc(lexParser, doc, kDOC_PROP_DWRITE, value);
					else if (!isSpan4D && attribut == kCSS_NAME_WHITE_SPACE)
						_ParseCSSWhiteSpace( inSpanHandler, value);
					else if (isXHTML4D) //only XHTML4D format is capable to manage common properties
					{
						//node common properties
						if (attribut == kCSS_NAME_TEXT_ALIGN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_TEXT_ALIGN, value);
						else if (attribut == kCSS_NAME_VERTICAL_ALIGN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_VERTICAL_ALIGN, value);
						else if (attribut == kCSS_NAME_MARGIN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MARGIN, value);
						else if (attribut == kCSS_NAME_PADDING)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_PADDING, value);
						else if (attribut == kCSS_NAME_BORDER_STYLE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_STYLE, value);
						else if (attribut == kCSS_NAME_BORDER_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_WIDTH, value);
						else if (attribut == kCSS_NAME_BORDER_COLOR)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_COLOR, value);
						else if (attribut == kCSS_NAME_BORDER_RADIUS)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_RADIUS, value);
						else if (attribut == kCSS_NAME_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_WIDTH, value);
						else if (attribut == kCSS_NAME_HEIGHT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_HEIGHT, value);
						else if (attribut == kCSS_NAME_MIN_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MIN_WIDTH, value);
						else if (attribut == kCSS_NAME_MIN_HEIGHT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MIN_HEIGHT, value);
						else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_COLOR, value); //here it is element back color
						else if (attribut == kCSS_NAME_BACKGROUND_IMAGE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_IMAGE, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_POSITION)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_POSITION, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_REPEAT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_REPEAT, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_CLIP)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_CLIP, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_ORIGIN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_ORIGIN, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_SIZE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_SIZE, value); 
					}
				}
			}
			break;
#endif
		case kDOC_NODE_TYPE_PARAGRAPH:
			{
				//parse paragraph properties

				VDocParagraph *para = NULL;
				const VTreeTextStyle *styles = NULL;
				VTextStyle *style = NULL;

				para = dynamic_cast<VDocParagraph *>(ioDocNode);
				xbox_assert(para);

				styles = para->GetStyles();
				xbox_assert(styles);
				style = styles->GetData();
				
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
						_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_TEXT_ALIGN, value);
					else if (!isSpan4D && attribut == kCSS_NAME_WHITE_SPACE)
						_ParseCSSWhiteSpace( inSpanHandler, value);
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
					else if (isXHTML4D) //only XHTML4D format is capable to manage all paragraph properties
					{
						//node common properties
						if (attribut == kCSS_NAME_TEXT_ALIGN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_TEXT_ALIGN, value);
						else if (attribut == kCSS_NAME_VERTICAL_ALIGN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_VERTICAL_ALIGN, value);
						else if (attribut == kCSS_NAME_MARGIN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MARGIN, value);
						else if (attribut == kCSS_NAME_PADDING)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_PADDING, value);
						else if (attribut == kCSS_NAME_BORDER_STYLE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_STYLE, value);
						else if (attribut == kCSS_NAME_BORDER_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_WIDTH, value);
						else if (attribut == kCSS_NAME_BORDER_COLOR)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_COLOR, value);
						else if (attribut == kCSS_NAME_BORDER_RADIUS)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_RADIUS, value);
						else if (attribut == kCSS_NAME_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_WIDTH, value);
						else if (attribut == kCSS_NAME_HEIGHT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_HEIGHT, value);
						else if (attribut == kCSS_NAME_MIN_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MIN_WIDTH, value);
						else if (attribut == kCSS_NAME_MIN_HEIGHT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MIN_HEIGHT, value);
						else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_COLOR, value); //here it is element back color
						else if (attribut == kCSS_NAME_BACKGROUND_IMAGE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_IMAGE, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_POSITION)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_POSITION, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_REPEAT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_REPEAT, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_CLIP)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_CLIP, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_ORIGIN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_ORIGIN, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_SIZE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_SIZE, value); 

						//paragraph node private properties
						else if (attribut == kCSS_NAME_DIRECTION)
							_ParsePropParagraph(lexParser, para, kDOC_PROP_DIRECTION, value);
						else if (attribut == kCSS_NAME_LINE_HEIGHT)
							_ParsePropParagraph(lexParser, para, kDOC_PROP_LINE_HEIGHT, value);
						else if (attribut == kCSS_NAME_TEXT_INDENT)
							_ParsePropParagraph(lexParser, para, kDOC_PROP_TEXT_INDENT, value);
						else if (attribut == kCSS_NAME_D4_TAB_STOP_OFFSET)
							_ParsePropParagraph(lexParser, para, kDOC_PROP_TAB_STOP_OFFSET, value);
						else if (attribut == kCSS_NAME_D4_TAB_STOP_TYPE)
							_ParsePropParagraph(lexParser, para, kDOC_PROP_TAB_STOP_TYPE, value);
						else if (attribut == kCSS_NAME_D4_NEW_PARA_CLASS)
							_ParsePropParagraph(lexParser, para, kDOC_PROP_NEW_PARA_CLASS, value);
					}
#endif
				}
			}
			break;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		case kDOC_NODE_TYPE_IMAGE:
			{
				//parse image properties

				if (inSpanHandler->GetOptions() & kHTML_PARSER_TAG_IMAGE_SUPPORTED) 
				{
					while (cssParser.GetNextAttribute(attribut, value))
					{
						//node common properties

						if (attribut == kCSS_NAME_VERTICAL_ALIGN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_VERTICAL_ALIGN, value);
						else if (attribut == kCSS_NAME_MARGIN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MARGIN, value);
						else if (attribut == kCSS_NAME_PADDING)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_PADDING, value);
						else if (attribut == kCSS_NAME_BORDER_STYLE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_STYLE, value);
						else if (attribut == kCSS_NAME_BORDER_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_WIDTH, value);
						else if (attribut == kCSS_NAME_BORDER_COLOR)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_COLOR, value);
						else if (attribut == kCSS_NAME_BORDER_RADIUS)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BORDER_RADIUS, value);
						else if (attribut == kCSS_NAME_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_WIDTH, value);
						else if (attribut == kCSS_NAME_HEIGHT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_HEIGHT, value);
						else if (attribut == kCSS_NAME_MIN_WIDTH)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MIN_WIDTH, value);
						else if (attribut == kCSS_NAME_MIN_HEIGHT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_MIN_HEIGHT, value);
						else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_COLOR, value); //here it is element back color
						else if (attribut == kCSS_NAME_BACKGROUND_IMAGE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_IMAGE, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_POSITION)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_POSITION, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_REPEAT)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_REPEAT, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_CLIP)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_CLIP, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_ORIGIN)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_ORIGIN, value); 
						else if (attribut == kCSS_NAME_BACKGROUND_SIZE)
							_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_BACKGROUND_SIZE, value); 
					}
				}
			}
			break;
#endif

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
		VDocText *doc = inDocNode->GetDocumentNode();

		if (doc->GetVersion() >= kDOC_VERSION_XHTML4D) 
		{
			//CSS d4-version (d4-version value = inDocNode->GetDocumentNode()->GetVersion()-1)
			bool isOverriden = inDocNode->IsOverriden( kDOC_PROP_VERSION) && !inDocNode->IsInherited( kDOC_PROP_VERSION) && (!inDocNodeDefault || doc->GetVersion() != inDocNodeDefault->GetDocumentNode()->GetVersion());
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_VERSION);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)(doc->GetVersion() < kDOC_VERSION_SPAN4D ? kDOC_VERSION_SPAN4D-1 : doc->GetVersion()-1));
				count++;
			}
			//CSS d4-dpi
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_DPI) && !inDocNode->IsInherited( kDOC_PROP_DPI) && (!inDocNodeDefault || doc->GetDPI() != inDocNodeDefault->GetDocumentNode()->GetDPI());
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_DPI);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)doc->GetDPI());
				count++;
			}

			//CSS d4-zoom
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_ZOOM) && !inDocNode->IsInherited( kDOC_PROP_ZOOM) && (!inDocNodeDefault || doc->GetZoom() != inDocNodeDefault->GetDocumentNode()->GetZoom());
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_ZOOM);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)doc->GetZoom());
				outCSSDeclarations.AppendUniChar('%');
				count++;
			}

			//CSS d4-dwrite
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_DWRITE) && !inDocNode->IsInherited( kDOC_PROP_DWRITE) && (!inDocNodeDefault || doc->GetDWrite() != inDocNodeDefault->GetDocumentNode()->GetDWrite());
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_DWRITE);
				outCSSDeclarations.AppendUniChar( ':');
				if (doc->GetDWrite())
					outCSSDeclarations.AppendCString( "true");
				else
					outCSSDeclarations.AppendCString( "false");
				count++;
			}

			VIndex length = outCSSDeclarations.GetLength();
			_SerializePropsCommon( inDocNode, outCSSDeclarations, inDocNodeDefault, count == 0);
			if (length != outCSSDeclarations.GetLength())
				count++;
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
		bool isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_DIRECTION);
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
						case kDOC_DIRECTION_LTR:
							outCSSDeclarations.AppendString("ltr");
							break;
						default:
							outCSSDeclarations.AppendString("rtl");
							break;
					}
					outCSSDeclarations.AppendCString( ";unicode-bidi: bidi-override"); //CSS compliancy: otherwise 'direction' as alone would have not effect in a standard browser
																					   //(but on parsing XHTML4D, we parse only 'direction' property)
					count++;
				}
			}
		}

		//CSS text-indent
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TEXT_INDENT);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_TEXT_INDENT))
			{
				//inherited on default
			}
			else
			{
				sLONG padding = para->GetTextIndent();

				if (!(paraDefault && padding == paraDefault->GetTextIndent()))
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_TEXT_INDENT);
					outCSSDeclarations.AppendUniChar( ':');

					VString sValue;
					_TWIPSToCSSDimPoint( padding, sValue);
					outCSSDeclarations.AppendString( sValue);
					count++;
				}
			}
		}

		//CSS line-height
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LINE_HEIGHT);
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

		//CSS d4-tab-stop-offset
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TAB_STOP_OFFSET);
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
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TAB_STOP_TYPE);
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
						case kDOC_TAB_STOP_TYPE_LEFT:
							outCSSDeclarations.AppendCString("left");
							break;
						case kDOC_TAB_STOP_TYPE_RIGHT:
							outCSSDeclarations.AppendCString("right");
							break;
						case kDOC_TAB_STOP_TYPE_CENTER:
							outCSSDeclarations.AppendCString("center");
							break;
						case kDOC_TAB_STOP_TYPE_DECIMAL:
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

		//CSS -d4-new-para-class
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_NEW_PARA_CLASS);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_NEW_PARA_CLASS))
			{
				//inherited on default
				xbox_assert(false); //should not be inherited
			}
			else
			{
				VString newparaclass = para->GetNewParaClass();
				if (!newparaclass.IsEmpty())
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_NEW_PARA_CLASS);
					outCSSDeclarations.AppendUniChar( ':');
					
					TextToXML(newparaclass, true, true, true, false, true);
					outCSSDeclarations.AppendString( newparaclass); 
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
		//CSS -d4-version
		if (ioDocText->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D)
		{
			uLONG version = (uLONG)inValue.GetLong()+1;
			ioDocText->SetVersion(version >= kDOC_VERSION_XHTML4D && version <= kDOC_VERSION_MAX_SUPPORTED ? version : kDOC_VERSION_XHTML4D);
		}
		result = true;
	}
	else if (inProperty == kDOC_PROP_DPI)
	{
		//CSS -d4-dpi

		uLONG dpi = inValue.GetLong();
		if (dpi >= 72)
		{
			ioDocText->SetDPI(dpi);
			result = true;
		}
	}
	else if (inProperty == kDOC_PROP_ZOOM)
	{
		//CSS -d4-zoom

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
	else if (inProperty == kDOC_PROP_DWRITE)
	{
		//CSS -d4-dwrite

		bool dwrite = inValue.GetBoolean() != FALSE;
		if (dwrite)
		{
			ioDocText->SetDWrite(dwrite);
			result = true;
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

/** parse image HTML attributes (image 'src' and 'alt' attributes are HTML, not CSS) */	
bool VSpanTextParser::ParseAttributeImage( VDocImage *ioDocImage, const uLONG inProperty, const VString& inValue)
{
	bool result = false;

	if (inProperty == kDOC_PROP_IMG_SOURCE)
	{
		//HTML src

		VString value = inValue;
		value.RemoveWhiteSpaces(true, true, NULL, NULL, VIntlMgr::GetDefaultMgr());
		if (!value.IsEmpty())
			value.ConvertCarriageReturns(eCRM_CR);
		ioDocImage->SetSource( value);
		result = true;
	}
	else if (inProperty == kDOC_PROP_IMG_ALT_TEXT)
	{
		//HTML alt

		VString value = inValue;
		value.RemoveWhiteSpaces(true, true, NULL, NULL, VIntlMgr::GetDefaultMgr());
		if (!value.IsEmpty())
			value.ConvertCarriageReturns(eCRM_CR);
		else
			value = VDocSpanTextRef::fSpace; //at least one space
		ioDocImage->SetText( value);
		result = true;
	}
	return result;
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
	else if (inProperty == kDOC_PROP_TEXT_INDENT)
	{
		//CSS text-indent

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::COORD, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocPara->SetInherit( kDOC_PROP_TEXT_INDENT);
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
					ioDocPara->SetTextIndent( ICSSUtil::CSSDimensionToTWIPS( valueCSS->v.css.fLength.fNumber, 
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
		//CSS -d4-tab-stop-offset

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
		//CSS -d4-tab-stop-type

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
	else if (inProperty == kDOC_PROP_NEW_PARA_CLASS)
	{
		//CSS -d4-new-para-class

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		if (inLexParser->GetCurToken() == CSSToken::IDENT)
		{
			ioDocPara->SetNewParaClass( inLexParser->GetCurTokenValue()); 
			result = true;

		}
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

	if (inProperty == kDOC_PROP_TEXT_ALIGN)
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
				case CSSProperty::kVERTICAL_ALIGN_BOTTOM:
				case CSSProperty::kVERTICAL_ALIGN_TEXT_BOTTOM:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Bottom);
					break;
				case CSSProperty::kVERTICAL_ALIGN_MIDDLE:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Center);
					break;
				case CSSProperty::kVERTICAL_ALIGN_BASELINE:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Baseline);
					break;
				case CSSProperty::kVERTICAL_ALIGN_SUB:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Subscript);
					break;
				case CSSProperty::kVERTICAL_ALIGN_SUPER:
					ioDocNode->SetVerticalAlign( (eDocPropTextAlign)JST_Superscript);
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
		bool inherit = false;
		bool automatic = false;
		bool error = false;
		while (inLexParser->GetCurToken() != CSSToken::END)
		{
			CSSProperty::Value *value = VCSSParser::ParseValue( inLexParser, CSSProperty::LENGTH, NULL, true, true);
			if (value)
			{
				if (value->fInherit)
				{
					if (margin.size())
						error = true;
					inherit = true;
					break;
				}
				else if (value->v.css.fLength.fAuto)
				{
					if (margin.size())
						error = true;
					automatic = true;
					break;
				}
				else
				{
					//add length converted to TWIPS
					if (value->v.css.fLength.fUnit == kCSSUNIT_PERCENT) //forbidden
						error = true;
					else
						margin.push_back( ICSSUtil::CSSDimensionToTWIPS(value->v.css.fLength.fNumber, (eCSSUnit)value->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode)));
				}
				ReleaseRefCountable(&value);
			}
			else
			{
				error = true;
				break;
			}
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

				sDocPropRect smargin;
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
	else if (inProperty == kDOC_PROP_PADDING)
	{
		//CSS padding (remark: percentage values are forbidden)

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);

		std::vector<sLONG> margin;
		bool inherit = false;
		bool automatic = false;
		bool error = false;
		while (inLexParser->GetCurToken() != CSSToken::END)
		{
			CSSProperty::Value *value = VCSSParser::ParseValue( inLexParser, CSSProperty::LENGTH, NULL, true, true);
			if (value)
			{
				if (value->fInherit)
				{
					if (margin.size())
						error = true;
					inherit = true;
					break;
				}
				else if (value->v.css.fLength.fAuto)
				{
					if (margin.size())
						error = true;
					automatic = true;
					break;
				}
				else
				{
					//add length converted to TWIPS
					if (value->v.css.fLength.fUnit == kCSSUNIT_PERCENT) //forbidden
						error = true;
					else
						margin.push_back( ICSSUtil::CSSDimensionToTWIPS(value->v.css.fLength.fNumber, (eCSSUnit)value->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode)));
				}
				ReleaseRefCountable(&value);
			}
			else
			{
				error = true;
				break;
			}
			inLexParser->Next( true); 
		} 
		if (!error)
		{
			if (inherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_PADDING);
				result = true;
			}
			else if (automatic)
			{
				ioDocNode->RemoveProp( kDOC_PROP_PADDING); //reset to default margin
				result = true;
			}
			else if (margin.size() >= 1 && margin.size() <= 4)
			{
				//parse top, right, bottom & left margins 

				sDocPropRect smargin;
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
				ioDocNode->SetPadding( smargin);
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
	else if (inProperty == kDOC_PROP_BACKGROUND_IMAGE)
	{
		//CSS background-image

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::URI, NULL, true, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_IMAGE);
				result = true;
			}
			else if (valueCSS->v.css.fURI)
			{
				ioDocNode->SetBackgroundImage( *(valueCSS->v.css.fURI));
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_POSITION)
	{
		//CSS background-position (background image horizontal & vertical alignment)

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::BACKGROUNDPOSITION, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_POSITION);
				result = true;
			}
			else 
			{
				switch( valueCSS->v.css.fBackgroundPosition.fTextAlign)
				{
				case CSSProperty::kTEXT_ALIGN_LEFT:
					ioDocNode->SetBackgroundImageHAlign( JST_Left);
					break;
				case CSSProperty::kTEXT_ALIGN_RIGHT:
					ioDocNode->SetBackgroundImageHAlign( JST_Right);
					break;
				case CSSProperty::kTEXT_ALIGN_CENTER:
					ioDocNode->SetBackgroundImageHAlign( JST_Center);
					break;
				default:
					xbox_assert(false); //impossible as it is filtered yet by CSS parser
					break;
				}
				switch( valueCSS->v.css.fBackgroundPosition.fVerticalAlign)
				{
				case CSSProperty::kVERTICAL_ALIGN_TOP:
					ioDocNode->SetBackgroundImageVAlign( JST_Top);
					break;
				case CSSProperty::kVERTICAL_ALIGN_BOTTOM:
					ioDocNode->SetBackgroundImageVAlign( JST_Bottom);
					break;
				case CSSProperty::kVERTICAL_ALIGN_MIDDLE:
					ioDocNode->SetBackgroundImageVAlign( JST_Center);
					break;
				default:
					xbox_assert(false); //impossible as it is filtered yet by CSS parser
					break;
				}
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_REPEAT)
	{
		//CSS background-repeat 

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::BACKGROUNDREPEAT, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_REPEAT);
				result = true;
			}
			else 
			{
				switch( valueCSS->v.css.fBackgroundRepeat)
				{
				case CSSProperty::kBR_REPEAT:
					ioDocNode->SetBackgroundImageRepeat( kDOC_BACKGROUND_REPEAT);
					break;
				case CSSProperty::kBR_REPEAT_X:
					ioDocNode->SetBackgroundImageRepeat( kDOC_BACKGROUND_REPEAT_X);
					break;
				case CSSProperty::kBR_REPEAT_Y:
					ioDocNode->SetBackgroundImageRepeat( kDOC_BACKGROUND_REPEAT_Y);
					break;
				case CSSProperty::kBR_NO_REPEAT:
					ioDocNode->SetBackgroundImageRepeat( kDOC_BACKGROUND_NO_REPEAT);
					break;
				default:
					xbox_assert(false); 
					break;
				}
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_CLIP)
	{
		//CSS background-clip

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::BACKGROUNDCLIP, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_CLIP);
				result = true;
			}
			else 
			{
				switch( valueCSS->v.css.fBackgroundClip)
				{
				case CSSProperty::kBB_BORDER_BOX:
					ioDocNode->SetBackgroundClip( kDOC_BACKGROUND_BORDER_BOX);
					break;
				case CSSProperty::kBB_PADDING_BOX:
					ioDocNode->SetBackgroundClip( kDOC_BACKGROUND_PADDING_BOX);
					break;
				case CSSProperty::kBB_CONTENT_BOX:
					ioDocNode->SetBackgroundClip( kDOC_BACKGROUND_CONTENT_BOX);
					break;
				default:
					xbox_assert(false); 
					break;
				}
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_ORIGIN)
	{
		//CSS background-origin

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::BACKGROUNDORIGIN, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_ORIGIN);
				result = true;
			}
			else 
			{
				switch( valueCSS->v.css.fBackgroundOrigin)
				{
				case CSSProperty::kBB_BORDER_BOX:
					ioDocNode->SetBackgroundImageOrigin( kDOC_BACKGROUND_BORDER_BOX);
					break;
				case CSSProperty::kBB_PADDING_BOX:
					ioDocNode->SetBackgroundImageOrigin( kDOC_BACKGROUND_PADDING_BOX);
					break;
				case CSSProperty::kBB_CONTENT_BOX:
					ioDocNode->SetBackgroundImageOrigin( kDOC_BACKGROUND_CONTENT_BOX);
					break;
				default:
					xbox_assert(false); 
					break;
				}
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BACKGROUND_SIZE)
	{
		//CSS background-size

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::BACKGROUNDSIZE, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BACKGROUND_SIZE);
				result = true;
			}
			else 
			{
				switch (valueCSS->v.css.fBackgroundSize.fPreset)
				{
				case CSSProperty::kBS_COVER:
					ioDocNode->SetBackgroundImageWidth( kDOC_BACKGROUND_SIZE_COVER);
					break;
				case CSSProperty::kBS_CONTAIN:
					ioDocNode->SetBackgroundImageWidth( kDOC_BACKGROUND_SIZE_CONTAIN);
					break;
				default:
					{
						if (valueCSS->v.css.fBackgroundSize.fWidth.fAuto)
							ioDocNode->SetBackgroundImageWidth( kDOC_BACKGROUND_SIZE_AUTO);	
						else if (valueCSS->v.css.fBackgroundSize.fWidth.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
						{
							Real number = ICSSUtil::RoundTWIPS(valueCSS->v.css.fBackgroundSize.fWidth.fNumber)*100;
							if (number < 1)
								number = 1;
							if (number > 1000)
								number = 1000;
							number = -number;
							ioDocNode->SetBackgroundImageWidth( (sLONG)number);
						}
						else
						{
							sLONG length =  ICSSUtil::CSSDimensionToTWIPS(
														valueCSS->v.css.fBackgroundSize.fWidth.fNumber, 
														(eCSSUnit)valueCSS->v.css.fBackgroundSize.fWidth.fUnit, 
														ioDocNode->GetDocumentNode()->GetDPI(), 
														_GetFontSize( ioDocNode));
							ioDocNode->SetBackgroundImageWidth( length);
						}

						if (valueCSS->v.css.fBackgroundSize.fHeight.fAuto)
							ioDocNode->SetBackgroundImageHeight( kDOC_BACKGROUND_SIZE_AUTO);	
						else if (valueCSS->v.css.fBackgroundSize.fHeight.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
						{
							Real number = ICSSUtil::RoundTWIPS(valueCSS->v.css.fBackgroundSize.fHeight.fNumber)*100;
							if (number < 1)
								number = 1;
							if (number > 1000)
								number = 1000;
							number = -number;
							ioDocNode->SetBackgroundImageHeight( (sLONG)number);
						}
						else
						{
							sLONG length =  ICSSUtil::CSSDimensionToTWIPS(
														valueCSS->v.css.fBackgroundSize.fHeight.fNumber, 
														(eCSSUnit)valueCSS->v.css.fBackgroundSize.fHeight.fUnit, 
														ioDocNode->GetDocumentNode()->GetDPI(), 
														_GetFontSize( ioDocNode));
							ioDocNode->SetBackgroundImageHeight( length);
						}
					}
					break;
				}
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_WIDTH)
	{
		//CSS width

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_WIDTH);
			else if (valueCSS->v.css.fLength.fAuto)
				ioDocNode->RemoveProp( kDOC_PROP_WIDTH);
			else
			{
				sLONG length = ICSSUtil::CSSDimensionToTWIPS(valueCSS->v.css.fLength.fNumber, (eCSSUnit)valueCSS->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode));
				ioDocNode->SetWidth( (uLONG)length);
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_HEIGHT)
	{
		//CSS height

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_HEIGHT);
			else if (valueCSS->v.css.fLength.fAuto)
				ioDocNode->RemoveProp( kDOC_PROP_HEIGHT);
			else
			{
				sLONG length = ICSSUtil::CSSDimensionToTWIPS(valueCSS->v.css.fLength.fNumber, (eCSSUnit)valueCSS->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode));
				ioDocNode->SetHeight( (uLONG)length);
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_MIN_WIDTH)
	{
		//CSS min-width

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_MIN_WIDTH);
			else if (valueCSS->v.css.fLength.fAuto)
				ioDocNode->RemoveProp( kDOC_PROP_MIN_WIDTH);
			else
			{
				sLONG length = ICSSUtil::CSSDimensionToTWIPS(valueCSS->v.css.fLength.fNumber, (eCSSUnit)valueCSS->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode));
				ioDocNode->SetMinWidth( (uLONG)length);
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_MIN_HEIGHT)
	{
		//CSS min-height

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_MIN_HEIGHT);
			else if (valueCSS->v.css.fLength.fAuto)
				ioDocNode->RemoveProp( kDOC_PROP_MIN_HEIGHT);
			else
			{
				sLONG length = ICSSUtil::CSSDimensionToTWIPS(valueCSS->v.css.fLength.fNumber, (eCSSUnit)valueCSS->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode));
				ioDocNode->SetMinHeight( (uLONG)length);
			}
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_BORDER_STYLE)
	{
		//CSS border-style 

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);

		std::vector<sLONG> border;
		bool inherit = false;
		bool error = false;
		while (inLexParser->GetCurToken() != CSSToken::END)
		{
			CSSProperty::Value *value = VCSSParser::ParseValue( inLexParser, CSSProperty::BORDERSTYLE, NULL, true, true);
			if (value)
			{
				if (value->fInherit)
				{
					if (border.size())
						error = true;
					inherit = true;
					break;
				}
				else
					border.push_back( (sLONG)(value->v.css.fBorderStyle));
				ReleaseRefCountable(&value);
			}
			else
			{
				error = true;
				break;
			}
			inLexParser->Next( true); 
		} 
		if (!error)
		{
			if (inherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BORDER_STYLE);
				result = true;
			}
			else if (border.size() >= 1 && border.size() <= 4)
			{
				//parse top, right, bottom & left borders 

				sDocPropRect sBorderRect;
				if (border.size() == 4)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = border[3];
				}
				else if (border.size() == 3)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = sBorderRect.right;
				}
				else if (border.size() == 2)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.right;
				}
				else
				{
					sBorderRect.top = border[0];
					sBorderRect.right = sBorderRect.top;
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.top;
				}
				ioDocNode->SetBorderStyle( sBorderRect);
				result = true;
			}
		}
	}
	else if (inProperty == kDOC_PROP_BORDER_WIDTH)
	{
		//CSS border-width 

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);

		std::vector<sLONG> border;
		bool inherit = false;
		bool error = false;
		while (inLexParser->GetCurToken() != CSSToken::END)
		{
			CSSProperty::Value *value = VCSSParser::ParseValue( inLexParser, CSSProperty::BORDERWIDTH, NULL, true, true);
			if (value)
			{
				if (value->fInherit)
				{
					if (border.size())
						error = true;
					inherit = true;
					break;
				}
				else
				{
					//add border width as length in TWIPS
					sLONG length = ICSSUtil::CSSDimensionToTWIPS(value->v.css.fLength.fNumber, (eCSSUnit)value->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode));
					border.push_back( length);
				}
				ReleaseRefCountable(&value);
			}
			else
			{
				error = true;
				break;
			}
			inLexParser->Next( true); 
		} 
		if (!error)
		{
			if (inherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BORDER_WIDTH);
				result = true;
			}
			else if (border.size() >= 1 && border.size() <= 4)
			{
				//parse top, right, bottom & left borders 

				sDocPropRect sBorderRect;
				if (border.size() == 4)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = border[3];
				}
				else if (border.size() == 3)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = sBorderRect.right;
				}
				else if (border.size() == 2)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.right;
				}
				else
				{
					sBorderRect.top = border[0];
					sBorderRect.right = sBorderRect.top;
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.top;
				}
				ioDocNode->SetBorderWidth( sBorderRect);
				result = true;
			}
		}
	}
	else if (inProperty == kDOC_PROP_BORDER_COLOR)
	{
		//CSS border-color 

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);

		std::vector<sLONG> border;
		bool inherit = false;
		bool error = false;
		while (inLexParser->GetCurToken() != CSSToken::END)
		{
			CSSProperty::Value *value = VCSSParser::ParseValue( inLexParser, CSSProperty::COLOR, NULL, true, true);
			if (value)
			{
				if (value->fInherit)
				{
					if (border.size())
						error = true;
					inherit = true;
					break;
				}
				else if (value->v.css.fColor.fTransparent)
					border.push_back( RGBACOLOR_TRANSPARENT);
				else
					border.push_back( (sLONG)(value->v.css.fColor.fColor));
				ReleaseRefCountable(&value);
			}
			else
			{
				error = true;
				break;
			}
			inLexParser->Next( true); 
		} 
		if (!error)
		{
			if (inherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BORDER_COLOR);
				result = true;
			}
			else if (border.size() >= 1 && border.size() <= 4)
			{
				//parse top, right, bottom & left borders 

				sDocPropRect sBorderRect;
				if (border.size() == 4)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = border[3];
				}
				else if (border.size() == 3)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = sBorderRect.right;
				}
				else if (border.size() == 2)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.right;
				}
				else
				{
					sBorderRect.top = border[0];
					sBorderRect.right = sBorderRect.top;
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.top;
				}
				ioDocNode->SetBorderColor( sBorderRect);
				result = true;
			}
		}
	}
	else if (inProperty == kDOC_PROP_BORDER_RADIUS)
	{
		//CSS border-radius

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);

		std::vector<sLONG> border;
		bool inherit = false;
		bool error = false;
		while (inLexParser->GetCurToken() != CSSToken::END)
		{
			CSSProperty::Value *value = VCSSParser::ParseValue( inLexParser, CSSProperty::BORDERRADIUS, NULL, true, true);
			if (value)
			{
				if (value->fInherit)
				{
					if (border.size())
						error = true;
					inherit = true;
					break;
				}
				else
				{
					//add border radius as length in TWIPS
					sLONG length = ICSSUtil::CSSDimensionToTWIPS(value->v.css.fLength.fNumber, (eCSSUnit)value->v.css.fLength.fUnit, ioDocNode->GetDocumentNode()->GetDPI(), _GetFontSize( ioDocNode));
					border.push_back( length);
				}
				ReleaseRefCountable(&value);
			}
			else
			{
				error = true;
				break;
			}
			inLexParser->Next( true); 
		} 
		if (!error)
		{
			if (inherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_BORDER_RADIUS);
				result = true;
			}
			else if (border.size() >= 1 && border.size() <= 4)
			{
				//parse top, right, bottom & left borders 

				sDocPropRect sBorderRect;
				if (border.size() == 4)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = border[3];
				}
				else if (border.size() == 3)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = border[2];
					sBorderRect.left = sBorderRect.right;
				}
				else if (border.size() == 2)
				{
					sBorderRect.top = border[0];
					sBorderRect.right = border[1];
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.right;
				}
				else
				{
					sBorderRect.top = border[0];
					sBorderRect.right = sBorderRect.top;
					sBorderRect.bottom = sBorderRect.top;
					sBorderRect.left = sBorderRect.top;
				}
				ioDocNode->SetBorderRadius( sBorderRect);
				result = true;
			}
		}
	}

	if (doDeleteLexParser)
		delete inLexParser;
	return result;
}

bool VSpanTextParser::_SerializeAttributeID( const VDocNode *inDocNode, VString& outText)
{
	if (inDocNode->GetID().BeginsWith(D4ID_PREFIX_RESERVED))
		//skip if automatic id (we serialize only custom ids)
		return false;

	outText.AppendCString(" id=\"");
	VString sID = inDocNode->GetID();
	TextToXML( sID, true, true, true, false, true); 
	outText.AppendString( sID);
	outText.AppendUniChar('\"');
	return true;

}

bool VSpanTextParser::_SerializeAttributeClass( const VDocNode *inDocNode, VString& outText)
{
	if (inDocNode->GetClass().IsEmpty())
		return false;

	outText.AppendCString(" class=\"");
	VString sclass = inDocNode->GetClass();
	TextToXML( sclass, true, true, true, false, true); 
	outText.AppendString( sclass);
	outText.AppendUniChar('\"');
	return true;
}

/** serialize common CSS properties to CSS styles */
void VSpanTextParser::_SerializePropsCommon( const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault, bool inIsFirst)
{
	VIndex count = inIsFirst ? 0 : 1;
	bool isOverriden;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isXHTML4D = inDocNode->GetType() > kDOC_NODE_TYPE_PARAGRAPH || (inDocNode->GetDocumentNode() && inDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D);
	if (isXHTML4D)
	{
	//CSS margin
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_MARGIN);
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
			sDocPropRect margin = inDocNode->GetMargin();
			if (inDocNodeDefault)
			{
				sDocPropRect marginDefault = inDocNodeDefault->GetMargin();
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
				sLONG top, right, bottom, left;
				sLONG number[4];
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
	//CSS padding
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_PADDING);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_PADDING))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_PADDING);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			//CSS order: top, right, bottom & left 
			bool doIt = true;
			sDocPropRect margin = inDocNode->GetPadding();
			if (inDocNodeDefault)
			{
				sDocPropRect marginDefault = inDocNodeDefault->GetPadding();
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
				sLONG top, right, bottom, left;
				sLONG number[4];
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

				outCSSDeclarations.AppendCString( kCSS_NAME_PADDING);
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
	//CSS width
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_WIDTH);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_WIDTH))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_WIDTH);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			uLONG width = inDocNode->GetWidth();
			if (width != 0 
				&& 
				(!inDocNodeDefault || (!(width == inDocNodeDefault->GetWidth()))))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_WIDTH);
				outCSSDeclarations.AppendUniChar( ':');
				VString sValue;
				_TWIPSToCSSDimPoint( width, sValue);
				outCSSDeclarations.AppendString( sValue);

				count++;
			}
		}
	}
	//CSS min-width
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_MIN_WIDTH);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_MIN_WIDTH))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_MIN_WIDTH);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			uLONG width = inDocNode->GetMinWidth();
			if (width != 0 
				&& 
				(!inDocNodeDefault || (!(width == inDocNodeDefault->GetMinWidth()))))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_MIN_WIDTH);
				outCSSDeclarations.AppendUniChar( ':');
				VString sValue;
				_TWIPSToCSSDimPoint( width, sValue);
				outCSSDeclarations.AppendString( sValue);

				count++;
			}
		}
	}
	//CSS height
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_HEIGHT);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_HEIGHT))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_HEIGHT);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			uLONG height = inDocNode->GetHeight();
			if (height != 0 
				&& 
				(!inDocNodeDefault || (!(height == inDocNodeDefault->GetHeight()))))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_HEIGHT);
				outCSSDeclarations.AppendUniChar( ':');
				VString sValue;
				_TWIPSToCSSDimPoint( height, sValue);
				outCSSDeclarations.AppendString( sValue);

				count++;
			}
		}
	}
	//CSS min-height
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_MIN_HEIGHT);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_MIN_HEIGHT))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_MIN_HEIGHT);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			uLONG height = inDocNode->GetMinHeight();
			if (height != 0 
				&& 
				(!inDocNodeDefault || (!(height == inDocNodeDefault->GetMinHeight()))))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_MIN_HEIGHT);
				outCSSDeclarations.AppendUniChar( ':');
				VString sValue;
				_TWIPSToCSSDimPoint( height, sValue);
				outCSSDeclarations.AppendString( sValue);

				count++;
			}
		}
	}
	//CSS background-color (here element background color, not character back color)
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_COLOR);
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
	//CSS background-image 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_IMAGE);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_IMAGE))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_IMAGE);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			VString uri = inDocNode->GetBackgroundImage();
			if (!(inDocNodeDefault && uri == inDocNodeDefault->GetBackgroundImage()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_IMAGE);
				outCSSDeclarations.AppendCString( ":url('");
				TextToXML( uri, true, true, true, true, true); //convert for CSS 
				outCSSDeclarations.AppendString( uri);
				outCSSDeclarations.AppendCString( "')");
				count++;
			}
		}
	}
	//CSS background-position 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_POSITION);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_POSITION))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_POSITION);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			eStyleJust halign = inDocNode->GetBackgroundImageHAlign();
			eStyleJust valign = inDocNode->GetBackgroundImageVAlign();
			if (!(inDocNodeDefault && halign == inDocNodeDefault->GetBackgroundImageHAlign() && valign == inDocNodeDefault->GetBackgroundImageVAlign()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_POSITION);
				outCSSDeclarations.AppendUniChar( ':');
				bool halignSerialized = false;
				switch( halign)
				{
				case JST_Center:
					break;
				case JST_Right:
					{
					outCSSDeclarations.AppendCString("right");
					halignSerialized = true;
					}
					break;
				default:
					{
					outCSSDeclarations.AppendCString("left");
					halignSerialized = true;
					}
					break;
				}
				switch ( valign)
				{
				case JST_Center:
					if (!halignSerialized)
						outCSSDeclarations.AppendCString("center");
					break;
				case JST_Top:
					{
					if (halignSerialized)
						outCSSDeclarations.AppendUniChar(' ');
					outCSSDeclarations.AppendCString("top");
					}
					break;
				default:
					{
					if (halignSerialized)
						outCSSDeclarations.AppendUniChar(' ');
					outCSSDeclarations.AppendCString("bottom");
					}
					break;
				}
				count++;
			}
		}
	}
	//CSS background-repeat 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_REPEAT);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_REPEAT))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_REPEAT);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			eDocPropBackgroundRepeat repeat = inDocNode->GetBackgroundImageRepeat();
			if (!(inDocNodeDefault && repeat == inDocNodeDefault->GetBackgroundImageRepeat()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_REPEAT);
				outCSSDeclarations.AppendUniChar( ':');
				switch( repeat)
				{
				case kDOC_BACKGROUND_REPEAT:
					outCSSDeclarations.AppendCString( "repeat");
					break;
				case kDOC_BACKGROUND_REPEAT_X:
					outCSSDeclarations.AppendCString( "repeat-x");
					break;
				case kDOC_BACKGROUND_REPEAT_Y:
					outCSSDeclarations.AppendCString( "repeat-y");
					break;
				default:
					outCSSDeclarations.AppendCString( "no-repeat");
					break;
				}
				count++;
			}
		}
	}
	//CSS background-clip 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_CLIP);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_CLIP))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_CLIP);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			eDocPropBackgroundBox clip = inDocNode->GetBackgroundClip();
			if (!(inDocNodeDefault && clip == inDocNodeDefault->GetBackgroundClip()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_CLIP);
				outCSSDeclarations.AppendUniChar( ':');
				switch( clip)
				{
				case kDOC_BACKGROUND_BORDER_BOX:
					outCSSDeclarations.AppendCString( "border-box");
					break;
				case kDOC_BACKGROUND_PADDING_BOX:
					outCSSDeclarations.AppendCString( "padding-box");
					break;
				default:
					outCSSDeclarations.AppendCString( "content-box");
					break;
				}
				count++;
			}
		}
	}
	//CSS background-origin 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_ORIGIN);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_ORIGIN))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_ORIGIN);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			eDocPropBackgroundBox origin = inDocNode->GetBackgroundImageOrigin();
			if (!(inDocNodeDefault && origin == inDocNodeDefault->GetBackgroundImageOrigin()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_ORIGIN);
				outCSSDeclarations.AppendUniChar( ':');
				switch( origin)
				{
				case kDOC_BACKGROUND_BORDER_BOX:
					outCSSDeclarations.AppendCString( "border-box");
					break;
				case kDOC_BACKGROUND_PADDING_BOX:
					outCSSDeclarations.AppendCString( "padding-box");
					break;
				default:
					outCSSDeclarations.AppendCString( "content-box");
					break;
				}
				count++;
			}
		}
	}
	//CSS background-size 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_SIZE);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BACKGROUND_SIZE))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');

			outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_SIZE);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			sLONG width = inDocNode->GetBackgroundImageWidth();
			sLONG height = inDocNode->GetBackgroundImageHeight();
			if (!(inDocNodeDefault && width == inDocNodeDefault->GetBackgroundImageWidth() && height == inDocNodeDefault->GetBackgroundImageHeight()))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_SIZE);
				outCSSDeclarations.AppendUniChar( ':');
				switch( width)
				{
				case kDOC_BACKGROUND_SIZE_COVER:
					outCSSDeclarations.AppendCString( "cover");
					break;
				case kDOC_BACKGROUND_SIZE_CONTAIN:
					outCSSDeclarations.AppendCString( "contain");
					break;
				default:
					{
						if (width != kDOC_BACKGROUND_SIZE_AUTO)
						{
							if (width < 0)
							{
								//percentage
								width = -width;
								VString sValue;
								sValue.FromLong(width);
								outCSSDeclarations.AppendString( sValue);
								outCSSDeclarations.AppendUniChar('%');
							}
							else
							{
								//width in TWIPS
								VString sValue;
								_TWIPSToCSSDimPoint( width, sValue);
								outCSSDeclarations.AppendString( sValue);
							}
						}

						if (height == kDOC_BACKGROUND_SIZE_AUTO)
						{
							if (width != kDOC_BACKGROUND_SIZE_AUTO)
								outCSSDeclarations.AppendUniChar(' ');
							outCSSDeclarations.AppendCString("auto");
						}
						else 
						{
							if (width != kDOC_BACKGROUND_SIZE_AUTO)
								outCSSDeclarations.AppendUniChar(' ');
							else
								outCSSDeclarations.AppendCString("auto ");
							if (height < 0)
							{
								//percentage

								height = -height;
								VString sValue;
								sValue.FromLong(height);
								outCSSDeclarations.AppendString( sValue);
								outCSSDeclarations.AppendUniChar('%');
							}
							else
							{
								//height in TWIPS
								VString sValue;
								_TWIPSToCSSDimPoint( height, sValue);
								outCSSDeclarations.AppendString( sValue);
							}
						}
					}
					break;
				}
				count++;
			}
		}
	}
	//CSS border-style
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_STYLE);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BORDER_STYLE))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_STYLE);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			//CSS order: top, right, bottom & left 
			bool doIt = true;
			sDocPropRect border = inDocNode->GetBorderStyle();
			if (inDocNodeDefault)
			{
				sDocPropRect borderDefault = inDocNodeDefault->GetBorderStyle();
				if (border.left == borderDefault.left
					&&
					border.top == borderDefault.top
					&&
					border.right == borderDefault.right
					&&
					border.bottom == borderDefault.bottom)
					doIt = false;
			}
			if (doIt)
			{
				sLONG top, right, bottom, left;
				sLONG number[4];
				number[0] = border.top;
				number[1] = border.right;
				number[2] = border.bottom;
				number[3] = border.left;
				
				VIndex numDim = 4;
				if (border.left == border.right)
				{
					numDim = 3;
					if (border.bottom == border.top)
					{
						numDim = 2;
						if (border.right == border.top)
							numDim = 1;
					}
				}

				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_STYLE);
				outCSSDeclarations.AppendUniChar( ':');

				for (int i = 0; i < numDim; i++)
				{
					if (i > 0)
						outCSSDeclarations.AppendUniChar(' ');
					VString sValue;
					switch( number[i])
					{
					case kDOC_BORDER_STYLE_NONE:
						sValue = "none";
						break;
					case kDOC_BORDER_STYLE_DOTTED:
						sValue = "dotted";
						break;
					case kDOC_BORDER_STYLE_DASHED:
						sValue = "dashed";
						break;
					case kDOC_BORDER_STYLE_SOLID:
						sValue = "solid";
						break;
					case kDOC_BORDER_STYLE_DOUBLE:
						sValue = "double";
						break;
					case kDOC_BORDER_STYLE_GROOVE:
						sValue = "groove";
						break;
					case kDOC_BORDER_STYLE_RIDGE:
						sValue = "ridge";
						break;
					case kDOC_BORDER_STYLE_INSET:
						sValue = "inset";
						break;
					default:
						sValue = "outset";
						break;
					}
					outCSSDeclarations.AppendString( sValue);
				}
				count++;
			}
		}
	}
	//CSS border-width
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_WIDTH);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BORDER_WIDTH))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_WIDTH);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			//CSS order: top, right, bottom & left 
			bool doIt = true;
			sDocPropRect border = inDocNode->GetBorderWidth();
			if (inDocNodeDefault)
			{
				sDocPropRect borderDefault = inDocNodeDefault->GetBorderWidth();
				if (border.left == borderDefault.left
					&&
					border.top == borderDefault.top
					&&
					border.right == borderDefault.right
					&&
					border.bottom == borderDefault.bottom)
					doIt = false;
			}
			if (doIt)
			{
				sLONG top, right, bottom, left;
				sLONG number[4];
				number[0] = border.top;
				number[1] = border.right;
				number[2] = border.bottom;
				number[3] = border.left;
				
				VIndex numDim = 4;
				if (border.left == border.right)
				{
					numDim = 3;
					if (border.bottom == border.top)
					{
						numDim = 2;
						if (border.right == border.top)
							numDim = 1;
					}
				}

				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_WIDTH);
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
	//CSS border-color
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_COLOR);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BORDER_COLOR))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_COLOR);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			//CSS order: top, right, bottom & left 
			bool doIt = true;
			sDocPropRect border = inDocNode->GetBorderColor();
			if (inDocNodeDefault)
			{
				sDocPropRect borderDefault = inDocNodeDefault->GetBorderColor();
				if (border.left == borderDefault.left
					&&
					border.top == borderDefault.top
					&&
					border.right == borderDefault.right
					&&
					border.bottom == borderDefault.bottom)
					doIt = false;
			}
			if (doIt)
			{
				sLONG top, right, bottom, left;
				sLONG number[4];
				number[0] = border.top;
				number[1] = border.right;
				number[2] = border.bottom;
				number[3] = border.left;
				
				VIndex numDim = 4;
				if (border.left == border.right)
				{
					numDim = 3;
					if (border.bottom == border.top)
					{
						numDim = 2;
						if (border.right == border.top)
							numDim = 1;
					}
				}

				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_COLOR);
				outCSSDeclarations.AppendUniChar( ':');

				for (int i = 0; i < numDim; i++)
				{
					if (i > 0)
						outCSSDeclarations.AppendUniChar(' ');
					VString sValue;
					if (number[i] == RGBACOLOR_TRANSPARENT)
						sValue = "transparent";
					else
						ColorToValue( (RGBAColor)number[i], sValue);
					outCSSDeclarations.AppendString( sValue);
				}
				count++;
			}
		}
	}
	//CSS border-radius
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_RADIUS);
	if (isOverriden)
	{
		if (inDocNode->IsInherited( kDOC_PROP_BORDER_RADIUS))
		{
			if (count)
				outCSSDeclarations.AppendUniChar(';');
			outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_RADIUS);
			outCSSDeclarations.AppendCString( ":inherit");
			count++;
		}
		else
		{
			//CSS order: top, right, bottom & left 
			bool doIt = true;
			sDocPropRect border = inDocNode->GetBorderRadius();
			if (inDocNodeDefault)
			{
				sDocPropRect borderDefault = inDocNodeDefault->GetBorderRadius();
				if (border.left == borderDefault.left
					&&
					border.top == borderDefault.top
					&&
					border.right == borderDefault.right
					&&
					border.bottom == borderDefault.bottom)
					doIt = false;
			}
			if (doIt)
			{
				sLONG top, right, bottom, left;
				sLONG number[4];
				number[0] = border.top;
				number[1] = border.right;
				number[2] = border.bottom;
				number[3] = border.left;
				
				VIndex numDim = 4;
				if (border.left == border.right)
				{
					numDim = 3;
					if (border.bottom == border.top)
					{
						numDim = 2;
						if (border.right == border.top)
							numDim = 1;
					}
				}

				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BORDER_RADIUS);
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
	//CSS vertical-align
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_VERTICAL_ALIGN);
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
				textAlign != JST_Baseline //default is equal to baseline or top depending on element (for instance it is baseline for embedded image and top for paragraph in cell)
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
					case JST_Subscript:
						outCSSDeclarations.AppendString("sub");
						break;
					case JST_Superscript:
						outCSSDeclarations.AppendString("super");
						break;
					default:
						outCSSDeclarations.AppendString("top");
						break;
				}
				count++;
			}
		}
	}

	}
#endif  

	//CSS text-align
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TEXT_ALIGN);
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
}

bool VSpanTextParser::SerializeAttributeHRef( const VDocNode *ioDocNode, const VDocSpanTextRef *inSpanRef, VString& outText)
{
	xbox_assert(inSpanRef);
	if (inSpanRef->GetType() == kSpanTextRef_URL)
	{
		outText.AppendCString(" href=\"");
		VString url = inSpanRef->GetRef();
		TextToXML( url, true, true, true, false, true); //end of line should be escaped if any
		outText.AppendString( url);
		outText.AppendUniChar('\"');
		return true;
	}
	return false;
}

bool VSpanTextParser::SerializeAttributesImage( const VDocNode *ioDocNode, const VDocSpanTextRef *inSpanRef, VString& outText)
{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	xbox_assert(inSpanRef && inSpanRef->GetType() == kSpanTextRef_Image);

	outText.AppendCString(" src=\"");
	VString url = inSpanRef->GetImage()->GetSource();
	TextToXML( url, true, true, true, false, true); //end of line should be escaped if any
	outText.AppendString( url);
	outText.AppendUniChar('\"');

	outText.AppendCString(" alt=\"");
	VString text = inSpanRef->GetImage()->GetText();
	TextToXML( text, true, true, true, false, true); //end of line should be escaped if any
	outText.AppendString( text);
	outText.AppendUniChar('\"');
#endif
	return true;
}

bool VSpanTextParser::ParseAttributeHRef( VDocNode *ioDocNode, const VString& inValue, VTextStyle& outStyle)
{
	VString value = inValue;
	value.RemoveWhiteSpaces(true, true, NULL, NULL, VIntlMgr::GetDefaultMgr());
	if (!value.IsEmpty())
	{
		value.ConvertCarriageReturns(eCRM_CR);
		VDocSpanTextRef *spanRef = new VDocSpanTextRef( kSpanTextRef_URL,
														value,
														VDocSpanTextRef::fSpace);
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

		if (testAssert(inDocNode->GetType() != kDOC_NODE_TYPE_IMAGE)) //-d4 ref is allowed only on span nodes (as image node is embedded as a span reference yet)
		{
			VString value = inValue.GetCPointer();
			TextToXML( value, false, false, false, true, false); //Xerces parses escaped CSS sequences too
																 //so we need to escape again parsed '\' to ensure our CSS parser will properly parse it & not parse it as the escape char

			inLexParser->Start( value.GetCPointer());
			inLexParser->Next(true);
			CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::STRING, NULL, true);
			if (valueCSS && !valueCSS->fInherit)
			{
				VString value = *(valueCSS->v.css.fString);
				value.RemoveWhiteSpaces(true, true, NULL, NULL, VIntlMgr::GetDefaultMgr());
				if (!value.IsEmpty())
				{
					value.ConvertCarriageReturns(eCRM_CR);
					VDocSpanTextRef *spanRef = new VDocSpanTextRef( inProperty == kDOC_PROP_4DREF ? kSpanTextRef_4DExp : kSpanTextRef_User,
																	value,
																	VDocSpanTextRef::fSpace);
					if (inLC)
						spanRef->EvalExpression( inLC);
					outStyle.SetSpanRef( spanRef);
				}
				result = true;
			}
			ReleaseRefCountable(&valueCSS);
		}
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
			VDocNode *child = inDocNode->GetDocumentNode()->RetainChild(0);
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
	cssParser.Start(inCSSDeclarations, inDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D);
	VString attribut, value;

	while(cssParser.GetNextAttribute(attribut, value))
	{
		if (attribut == kCSS_NAME_FONT_FAMILY)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_FAMILY, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_FONT_SIZE)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_SIZE, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_COLOR)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_COLOR, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_BACKGROUND_COLOR)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_BACKGROUND_COLOR, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_FONT_STYLE)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_STYLE, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_TEXT_DECORATION)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_TEXT_DECORATION, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_FONT_WEIGHT)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_FONT_WEIGHT, value, outStyle, NULL, NULL, inStyleInherit);
		else if (attribut == kCSS_NAME_D4_REF)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_4DREF, value, outStyle, NULL, NULL, NULL, inLC);
		else if (attribut == kCSS_NAME_D4_REF_USER)
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_4DREF_USER, value, outStyle, NULL, NULL, NULL);
		else if (attribut == kCSS_NAME_TEXT_ALIGN && inDocNode->GetDocumentNode()->GetVersion() == kDOC_VERSION_SPAN4D) 
			_ParsePropSpan( lexParser, inDocNode, kDOC_PROP_TEXT_ALIGN, value, outStyle, NULL, NULL, NULL);
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
		/*
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
		*/
		{
#if VERSIONWIN
			uLONG version = inDocNode->GetDocumentNode()->GetVersion();
			if (version == kDOC_VERSION_SPAN4D)
			{
				//for compat with v13, we still need to store this nasty font size (screen resolution dependent)
				rfontSize = rfontSize*72/VSpanTextParser::Get()->GetSPANFontSizeDPI();
				if (fmod(rfontSize,(Real)1.0) != (Real)0.5) //if multiple of 0.5 do not round it otherwise it would transform back to wrong value while parsing
					rfontSize = floor(rfontSize+0.5f);
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
					doIt = false;
				else if (styleInherit->GetHasBackColor() && inStyle.GetBackGroundColor() == styleInherit->GetBackGroundColor())
					doIt = false;
				else if (inStyle.GetBackGroundColor() == RGBACOLOR_TRANSPARENT)
				{
					if (styleInherit->GetHasBackColor() 
						&& 
						styleInherit->GetBackGroundColor() != RGBACOLOR_TRANSPARENT
						)
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
				value = "none";
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
