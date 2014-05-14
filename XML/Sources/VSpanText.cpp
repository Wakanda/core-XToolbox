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
@param inEscapeCR
	if true, escapes CR to "\0D" in CSS or "&#xD;" in XML (exclusive with inConvertCRtoBRTag) - should be used only while serializing XML or CSS strings
@param inNoEncodeQuotes
	if true, characters ''' and '"' are not encoded 
@remarks
	see IsXMLChar 

	note that end of lines are normalized to one CR only (0x0D) - if end of lines are not converted to <BR/>

	if inFixInvalidChars == true, invalid chars are converted to whitespace (but for 0x0B which is replaced with 0x0D) 
	otherwise Xerces would fail parsing invalid XML 1.1 characters - parser would throw exception
*/
void VSpanTextParser::TextToXML(VString& ioText, bool inFixInvalidChars, bool inEscapeXMLMarkup, bool inConvertCRtoBRTag, bool inConvertForCSS, bool inEscapeCR, bool inNoEncodeQuotes)
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
				{
					if (inNoEncodeQuotes)
						text.AppendUniChar(*c);
					else
						text.AppendString(CVSTR("&quot;"));
				}
				else if (*c == CHAR_LESS_THAN_SIGN)
					text.AppendString(CVSTR("&lt;"));
				else if (*c == CHAR_APOSTROPHE)
				{
					if (inNoEncodeQuotes)
						text.AppendUniChar(*c);
					else
						text.AppendString(CVSTR("&apos;"));
				}
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
			{
				if (inNoEncodeQuotes)
					text.AppendUniChar(*c);
				else
					text.AppendString(CVSTR("&quot;"));
			}
			else if (*c == CHAR_LESS_THAN_SIGN)
				text.AppendString(CVSTR("&lt;"));
			else if (*c == CHAR_APOSTROPHE)
			{
				if (inNoEncodeQuotes)
					text.AppendUniChar(*c);
				else
					text.AppendString(CVSTR("&apos;"));
			}
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
			bool				IsNodeImage() const { return fIsNodeImage; } 
			bool				IsNodeStyle() const { return fIsNodeStyle; } 
			bool				IsNodeList() const { return fIsNodeList; }
			bool				IsNodeListItem() const { return fIsNodeListItem; }
			
			void				PreserveWhiteSpaces( bool inPre) { fPreserveWhiteSpaces = inPre; }
			uLONG				GetOptions() const { return fOptions; }

			virtual	VDocClassStyleManager*	GetLocalCSMgr() { return NULL; }

	static	bool				IsHTMLElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("html"),true); }
	static	bool				IsStyleElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("style"),true); }
	static	bool				IsHeadElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("head"),true); }
	static	bool				IsBodyElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("body"),true); }
	static	bool				IsDivElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("div"),true); }
	static	bool				IsParagraphElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("p"),true) || inElementName.EqualToString(CVSTR("li"),true); }
	static	bool				IsUnOrderedListElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("ul"),true); }
	static	bool				IsOrderedListElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("ol"),true); }
	static	bool				IsListItemElementName(const VString& inElementName) { return inElementName.EqualToString(CVSTR("li"),true); }
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
			VString*			fText;						//if we are inside a paragraph, it is equal to current parsed paragraph text
			uLONG				fOptions;
			SpanRefCombinedTextTypeMask	fShowSpanRef;
			VTreeTextStyle*		fStyles;
			VTreeTextStyle*		fStylesParent;
			VTreeTextStyle*		fStylesRoot;
			VTextStyle*			fStyleInherit;
			bool				fStyleInheritForChildrenDone;
			VString				fElementName;
			sLONG				fStart;						//this node character start index relatively to fText 
			bool				fIsInBody;
			bool				fIsInHead;
			bool				fIsNodeParagraph,			//<p> or <li> 
								fIsNodeSpan,				//<span>
								fIsNodeImage,				//<img>
								fIsNodeStyle;				//<style>
			bool				fIsNodeListItem;			//<li> node
			bool				fIsNodeList;				//<ul> or <ol> node
			VRefPtr<VDocClassStyle> fNodeListProps;			//if fIsNodeList, contains parsed <ul> or <ol> list properties if any -> will be merged to <li> properties while parsing <li> as we keep only <li> paragraphs in parsed document
				
			std::vector<VTreeTextStyle *> *fStylesSpanRefs;	//temporary storage for parsed span references -> it is inserted in paragraph text styles in CloseParagraph
			 
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
			bool				fShouldApplyCSSRules;

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
				fCSMgr = new VDocClassStyleManager( docText);
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
						fSetOfBreakLineEnding.insert(std::string("li"));
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
				ReleaseRefCountable(&fCSMgr);
				ReleaseRefCountable(&fStyles);
				if (fCSSRules)
					delete fCSSRules;
				if (fCSSMatch)
					delete fCSSMatch;
			}
	

			void				CloseParsing()
			{
				//ensure local class style manager is unbound from the document (as class styles stored here are temporary)
				fCSMgr->DetachFromOwnerDocument();

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

			VDocClassStyleManager*	GetLocalCSMgr() { return fCSMgr; }

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
protected:
			/** class style manager used to store parsed class styles which are not stored permanently in parsed document
			@remarks
				we parse all class styles but we preserve only one class name per element (the first class name if there are more than one)
				which defines the element style
			*/
			VDocClassStyleManager*				fCSMgr;
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
	fIsNodeListItem = false;	
	fIsNodeList = false;	
	fDocNodeLevel = -1;
	fParentDocNode = NULL;
	fIsUnknownTag = false;
	fIsOpenTagClosed = false;
	fIsUnknownRootTag = false;
	fUnknownTagText = inParent ? inParent->fUnknownTagText : NULL;
	fPreserveWhiteSpaces = inParent ? inParent->fPreserveWhiteSpaces : false; //default for XHTML (XHTML4D always set CSS property 'white-space:pre-wrap' for paragraphs as we need to preserve paragraph whitespaces - especially for tabs)

	fCSSRules = NULL;
	fCSSMatch = NULL; //remark: do not inherit fCSSMatch here as it should be enabled only for some nodes
	fShouldApplyCSSRules = true;

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
						{
							if (IsOrderedListElementName( fElementName))
							{
								fIsNodeList = true;
								fNodeListProps.Adopt( new VDocClassStyle());
								fNodeListProps.Get()->SetListStyleType( kDOC_LIST_STYLE_TYPE_DECIMAL); //default is decimal for <ol>
								fNodeListProps.Get()->SetListStart( 1); //ensure to restart from 1 on default for new list
							}
							else if (IsUnOrderedListElementName( fElementName))
							{
								fIsNodeList = true;
								fNodeListProps.Adopt( new VDocClassStyle());
								fNodeListProps.Get()->SetListStyleType( kDOC_LIST_STYLE_TYPE_DISC); //default is disc for <ul>
								fNodeListProps.Get()->SetListStart( 1); //ensure to restart from 1 on default for new list (even for <ul> as list style type might be overriden to ordered list)
							}
							return;
						}
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
					
					if (fSpanHandlerParent->fIsNodeList) //parent node is <ul> or <ol>
						//it is a list paragraph (<li> - only <li> can be children for <ul> or <ol>)
						fIsNodeListItem = true;
					
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
		//remark: here for VDoctext we allow only CSS rules matching id, class or lang but not other attributes 
		//(as only CSS styles are mandatory in XHMTL4D)

		VDocNode *node = reinterpret_cast<VDocNode *>(inElement);
		if (inName.EqualToString("id"))
		{
			outValue = node->GetID();
			return true;
		}
		else if (inName.EqualToString("class"))
		{
			outValue = node->GetClass();
			return true;
		}
		else if (inName.EqualToString("lang"))
		{
			outValue = node->GetLangBCP47();
			return true;
		}
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
		if (fDocNode->GetDocumentNode()->GetVersion() != kDOC_VERSION_SPAN4D && fDocNode->GetChildCount() > 0 && !fIsNodeList)
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
						if (fDocNode->GetDocumentNode()->GetVersion() >= kDOC_VERSION_XHTML4D)
						{
							//XHTML4D: a single paragraph per html paragraph and so only one terminating '\f' per paragraph 
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
						else
						{
							//XHTML11: all html text is parsed as a single paragraph (as we need to parse text from any element, not only from paragraph element) and so it might contain more than one '\f'

							xbox_assert(count == 1);
							if (text->GetLength() > 0 && text->GetUniChar(text->GetLength()) == '\f')
							{
								//truncate last ff
								para->ReplaceStyledText(CVSTR(""), NULL, text->GetLength()-1, text->GetLength(), false, true);
							}

							//now replace remainting '\f' with CR
							VString cr;
							cr.AppendUniChar(13);
							const UniChar *c = para->GetTextPtr()->GetCPointer();
							VIndex pos = 0;
							for(;*c != 0;c++, pos++)
							{
								if (*c == '\f')
								{
									para->ReplaceStyledText(cr, NULL, pos, pos+1, false, true);
									c = para->GetTextPtr()->GetCPointer()+pos;
								}
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
	if (!fShouldApplyCSSRules) //deal with reentrance (as it might be called on start or end element)
		return;
	fShouldApplyCSSRules = false;
	uLONG version = fDocNode->GetDocumentNode()->GetVersion();
	if (version < kDOC_VERSION_XHTML4D)
		return;

	VString firstClass;
	if (fCSSMatch && fDocNode->GetParserHandler())
	{
		if (fStyles && fText)
		{
			//at this point, styles should be consistent with parsed text
			sLONG start, end;
			fStyles->GetData()->GetRange( start, end);
			end = fText->GetLength();
			fStyles->GetData()->SetRange(start, end);
		}

		//parse class names
		VectorOfVString classes;
		VString sclasses = fDocNode->GetClass();
		bool isFirstClassAuto = false;
		if (!sclasses.IsEmpty())
		{
			VCSSLexParser *parser = new VCSSLexParser();
			parser->Start( sclasses.GetCPointer());
			parser->Next(true);
			while (parser->GetCurToken() == CSSToken::IDENT)
			{
				if (firstClass.IsEmpty())
				{
					firstClass = parser->GetCurTokenValue(); //we preserve only first class style in parsed document - other class styles are still parsed and applied but not retained by the document
					isFirstClassAuto = !firstClass.IsEmpty() && firstClass[0] == '_'; //if it begins with '_' it is assumed to be auto-class & should not be preserved by document
				}
				classes.push_back(parser->GetCurTokenValue()); 

				parser->Next(true);
			}
			delete parser;
		}

		//first apply styles from style sheet which match current node (but without class name selector)
		fCSSDefaultStyles.SetEmpty();
		fDocNode->SetClass(""); //here we apply only rules without class name selector as rules with class name selector are parsed yet as class style(s)
		fCSSMatch->Match( reinterpret_cast<VCSSMatch::opaque_ElementPtr>(fDocNode.Get()));
		_ParseCSSStyles(fCSSDefaultStyles);

		//now apply class rules

		VectorOfVString::const_iterator itClass = classes.begin();
		sLONG index = 0;
		for (;itClass != classes.end(); itClass++, index++)
		{
			//build class style

			VDocClassStyle *nodeStyle = fDocNode->GetDocumentNode()->GetClassStyleManager()->RetainClassStyle( fDocNode->GetType(), *itClass);
			if (!nodeStyle)
			{
				//try with temporary class style manager
				nodeStyle = fSpanHandlerRoot->GetLocalCSMgr()->RetainClassStyle( fDocNode->GetType(), *itClass);
			}
			if (!nodeStyle)  
			{
				//add new class style

				nodeStyle = new VDocClassStyle();
				nodeStyle->SetParserHandler( this);

				if (index == 0 && !isFirstClassAuto && fDocNode->GetType() >= kDOC_NODE_TYPE_DIV && fDocNode->GetType() < kDOC_NODE_TYPE_CLASS_STYLE)
					//bind first class style to the document
					fDocNode->GetDocumentNode()->GetClassStyleManager()->AddOrReplaceClassStyle( fDocNode->GetType(), *itClass, nodeStyle); 
				else
					//for other classes, add class style to temporary class style manager (other class styles are not preserved by parsed document)
					fSpanHandlerRoot->GetLocalCSMgr()->AddOrReplaceClassStyle( fDocNode->GetType(), *itClass, nodeStyle);

				//get styles from style sheet which match this class style
				fCSSDefaultStyles.SetEmpty();
				if (fIsNodeListItem)
					nodeStyle->SetElementName( CVSTR("li"));
				fCSSMatch->Match( reinterpret_cast<VCSSMatch::opaque_ElementPtr>(nodeStyle)); 
				if (!fCSSDefaultStyles.IsEmpty())
					VSpanTextParser::Get()->ParseCSSToVDocNode( static_cast<VDocNode *>(nodeStyle), fCSSDefaultStyles, this);
				nodeStyle->SetParserHandler(NULL);
				if (fIsNodeListItem)
					nodeStyle->SetElementName( kDOC_NODE_TYPE_PARAGRAPH);
			}

			//apply class style
			fDocNode->AppendPropsFrom( nodeStyle, false, false, 0, -1, fDocNode->GetType());
			ReleaseRefCountable(&nodeStyle);
		}
		if (isFirstClassAuto) //skip if class name begins with '_' as it is assumed to be automatic class - and we do not retain auto-classes
			fDocNode->SetClass( CVSTR(""));
		else
			fDocNode->SetClass(firstClass); //we keep only first class name (only one class name is allowed per element for XHTML4D) - i.e. one style sheet per paragraph, or image, and so on
											//note: other class names are still parsed and used by CSS rules but are not preserved in document
	}
	else if (!fSpanHandlerParagraph || fSpanHandlerParagraph == this || fIsNodeImage) //for paragraph span or url child node, fDocNode is the paragraph node so do not override class here
		fDocNode->SetClass(firstClass);

	//now parsed inline styles (it overrides class styles)
	if (!fCSSInlineStyles.IsEmpty())
		_ParseCSSStyles(fCSSInlineStyles);

	if (fIsNodeListItem) //<li> paragraph
	{
		xbox_assert(fSpanHandlerParent->fNodeListProps.Get() != NULL);

		//append previously parsed <ul> or <ol> properties (but do not override properties which are defined locally) - list style properties are inherited only from <ul> and <ol> by <li> elements
		fDocNode->AppendPropsFrom( static_cast<VDocNode *>(fSpanHandlerParent->fNodeListProps.Get()), true, true, 0, -1, kDOC_NODE_TYPE_PARAGRAPH);
		if (fDocNode->GetListStart() == kDOC_LIST_START_AUTO)
			fDocNode->RemoveProp( kDOC_PROP_LIST_START);
		
		//'start' can only be applied once on the first list item so remove it now
		fSpanHandlerParent->fNodeListProps.Get()->RemoveProp( kDOC_PROP_LIST_START); 
	}

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
	else if (fIsNodeList)
	{
		//parse properties & store temporary to fNodeListProps

		VDocClassStyle *cs = NULL;
		if (fNodeListProps.IsNull())
		{
			cs = new VDocClassStyle();
			fNodeListProps = cs;
		}
		else
			cs = RetainRefCountable(fNodeListProps.Get());
		cs->AttachToDocument( static_cast<VDocNode *>(fDocNode->GetDocumentNode()));
		VSpanTextParser::Get()->ParseCSSToVDocNode( static_cast<VDocNode *>(cs), inStyles, this);
		cs->DetachFromDocument( static_cast<VDocNode *>(fDocNode->GetDocumentNode()));
		ReleaseRefCountable(&cs);
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
		if (version < kDOC_VERSION_XHTML4D || fIsNodeList)
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
	else if (fIsNodeList)
	{
		if (inName.EqualToString("start")) //for compat with legacy html
		{
			VString styleCSS = VString(kCSS_NAME_D4_LIST_START)+":"+inValue;
			_ParseCSSStyles( styleCSS);
		}
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
		if (intlMgr->IsSpace( *c, true) && *c != '\f') //excluding no-break spaces
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

				if (*c == VSTP_EXPLICIT_CR || *c == '\f') //explicit CR -> trailing spaces -> truncate all trailing spaces
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
			else if (*c == '\f')
			{
				text.AppendUniChar(*c++);
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

	VDocClassStyleManager *docDefaultMgr = NULL;
	if (inDefaultStyle)
	{
		docDefaultMgr = new VDocClassStyleManager();

		VDocClassStyle *paraDefault = new VDocClassStyle();
		paraDefault->VTextStyleCopyToCharProps( *inDefaultStyle, true);
		
		docDefaultMgr->AddOrReplaceClassStyle( kDOC_NODE_TYPE_PARAGRAPH, VString(""), paraDefault);
		
		ReleaseRefCountable(&paraDefault);
	}
	SerializeDocument( doc, outTaggedText, docDefaultMgr);

	ReleaseRefCountable(&docDefaultMgr);
	ReleaseRefCountable(&para);
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

void VSpanTextParser::_GenerateNewClassName( const VDocText *inDoc, eDocNodeType inType, sLONG *ioNextAutoClassID, VString& outClass)
{
	//generate automatic class name 
	while (true)
	{
		sLONG id = *ioNextAutoClassID;
		(*ioNextAutoClassID)++;
		outClass.FromLong(id);
		outClass = VString("_")+VDocNode::GetElementName( inType)+outClass;
		//ensure class name does not exist yet
		VDocClassStyle *style = inDoc->GetClassStyleManager()->RetainClassStyle( inType, outClass);
		if (!style)
			break;
		else
			style->Release();
	}
}

const VString& VSpanTextParser::_AddAutoClassStyle(const VDocText *inDoc, eDocNodeType inType, const VString& inAutoClassStyle, 
												   sLONG *ioNextAutoClassID, MapOfAutoClassStyle& ioAutoClassStyles, const VString &inClass)
{
	VString sclass;
	MapOfAutoClassStyle::iterator itClassStyle = ioAutoClassStyles.find(inType);
	if (itClassStyle == ioAutoClassStyles.end())
	{
		//none class style for this document node type yet -> add new class name & CSS declaration
		ioAutoClassStyles[inType] = AutoClassStyles();
		itClassStyle = ioAutoClassStyles.find(inType);
		xbox_assert(itClassStyle != ioAutoClassStyles.end());
		if (inClass.IsEmpty())
			_GenerateNewClassName( inDoc, inType, ioNextAutoClassID, sclass);
		else
			sclass = inClass;
		itClassStyle->second.push_back( AutoClassStyle( sclass, inAutoClassStyle));
		return itClassStyle->second.back().first; //return class name
	}
	//add new class name & CSS declaration
	if (inClass.IsEmpty())
	{
		AutoClassStyles::const_iterator it = itClassStyle->second.begin();
		for (;it != itClassStyle->second.end(); it++)
		{
			if (!it->first.BeginsWith(CVSTR("_")))
				continue; //we re-use previously stored automatic style if and only if it is actually automatic style (auto style class name always begins with '_')
			if (it->second.EqualToStringRaw( inAutoClassStyle))
				return it->first; //CSS declarations are equal to previously parsed CSS declarations -> return associated class name
		}
		_GenerateNewClassName( inDoc, inType, ioNextAutoClassID, sclass);
	}
	else
		sclass = inClass;
	itClassStyle->second.push_back( AutoClassStyle( sclass, inAutoClassStyle));
	return itClassStyle->second.back().first; //return class name
}

void VSpanTextParser::_FinalizeAutoClassStyles( const MapOfAutoClassStyle& inAutoClassStyles, VString& outClassStyles, sLONG inIndent)
{
	VString listClassStyles;
	MapOfAutoClassStyle::const_iterator it = inAutoClassStyles.begin(); //iterate on class styles
	for (;it != inAutoClassStyles.end(); it++)
	{
		eDocNodeType nodeType = it->first;
		VString sElemName = VDocNode::GetElementName(nodeType); //get element name for the document node type
		AutoClassStyles::const_iterator itClassStyle = it->second.begin(); //iterate on class styles for the current document node type
		for (;itClassStyle != it->second.end(); itClassStyle++)
		{
			if (nodeType == kDOC_NODE_TYPE_PARAGRAPH)
			{
				bool hasListType = itClassStyle->second.Find( CVSTR("list-style-type"), 1) > 0;
				if (hasListType) //can only be for list item 
				{
					_NewLine( listClassStyles);
					_Indent( listClassStyles, inIndent);
					listClassStyles.AppendCString( "li");
					listClassStyles.AppendUniChar( '.');
					listClassStyles.AppendString( itClassStyle->first); //class name
					listClassStyles.AppendCString( " { ");
					listClassStyles.AppendString( itClassStyle->second); //CSS declarations
					listClassStyles.AppendCString(" }");
				}
				else //can be both for paragraph or list item
				{
					_NewLine( outClassStyles);
					_Indent( outClassStyles, inIndent);
					outClassStyles.AppendString( sElemName);
					outClassStyles.AppendUniChar( '.');
					outClassStyles.AppendString( itClassStyle->first); //class name
					outClassStyles.AppendCString( ",li");
					outClassStyles.AppendUniChar( '.');
					outClassStyles.AppendString( itClassStyle->first); //class name
					outClassStyles.AppendCString( " { ");
					outClassStyles.AppendString( itClassStyle->second); //CSS declarations
					outClassStyles.AppendCString(" }");
				}
			}
			else
			{
				_NewLine( outClassStyles);
				_Indent( outClassStyles, inIndent);
				outClassStyles.AppendString( sElemName);
				outClassStyles.AppendUniChar( '.');
				outClassStyles.AppendString( itClassStyle->first); //class name
				outClassStyles.AppendCString( " { ");
				outClassStyles.AppendString( itClassStyle->second); //CSS declarations
				outClassStyles.AppendCString(" }");
			}
		}
	}
	if (!listClassStyles.IsEmpty())
		outClassStyles.Insert(listClassStyles, 1);
}


/** serialize document (formatting depending on document version) 
@remarks
	if (inDocDefaultStyle != NULL):
		if inSerializeDefaultStyle, serialize inDocDefaultStyle properties & styles
		if !inSerializeDefaultStyle, only serialize properties & styles which are not equal to inDocDefaultStyle properties & styles
*/
void VSpanTextParser::SerializeDocument( const VDocText *inDoc, VString& outDocText, const VDocClassStyleManager *inDocDefaultStyleMgr, bool inSerializeDefaultStyle)
{
	xbox_assert(inDoc);

	outDocText.SetEmpty();

	fIsOnlyPlainText = true;
	fShouldSerializeListStartAuto = false;

	sLONG indent = 0;
	sLONG indentClassStyle = 0;
	sLONG insertClassStyleAt = 0;
	MapOfAutoClassStyle autoClassStyles;

	sLONG autoClassStyleNextID[kDOC_NODE_TYPE_COUNT] = { 0};
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
		for (int i = kDOC_NODE_TYPE_DIV; i < kDOC_NODE_TYPE_CLASS_STYLE; i++)
			autoClassStyleNextID[i] = 1;
#endif

	const VDocClassStyle *inDocDefaultStyle = inDocDefaultStyleMgr ? inDocDefaultStyleMgr->RetainFirstClassStyle( kDOC_NODE_TYPE_DOCUMENT) : NULL;
	const VDocClassStyle *inParaDefaultStyle = inDocDefaultStyleMgr ? inDocDefaultStyleMgr->RetainFirstClassStyle( kDOC_NODE_TYPE_PARAGRAPH) : NULL;

	VTextStyle *styleInherit = NULL;
	styleInherit = new VTextStyle();
	styleInherit->SetBold( 0);
	styleInherit->SetItalic( 0);
	styleInherit->SetStrikeout( 0);
	styleInherit->SetUnderline( 0);
	

	if (inParaDefaultStyle)
		//do not serialize default character styles (they are serialized only as style sheet rule or in root span if inSerializeDefaultStyle == true)
		inParaDefaultStyle->CharPropsCopyToVTextStyle( *styleInherit, true, false);

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
		indent++;
		indentClassStyle = indent;

		if (inSerializeDefaultStyle && inDocDefaultStyle && !inDoc->HasProps())
		{
			//append default body styles

			sLONG lengthBefore = outDocText.GetLength();
			_NewLine( outDocText);
			_Indent( outDocText, indent);
			outDocText.AppendCString("body { ");
			sLONG lengthBeforeStyle = outDocText.GetLength();
			_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDocDefaultStyle), outDocText, NULL, true, kDOC_NODE_TYPE_DOCUMENT, inDoc);
			if (lengthBeforeStyle == outDocText.GetLength())
				outDocText.Truncate(lengthBefore);
			else
				outDocText.AppendCString( " }");
		}
		//append body styles 
		if (inDoc->HasProps()) 
		{
			sLONG lengthBefore = outDocText.GetLength();
			_NewLine( outDocText);
			_Indent( outDocText, indent);
			outDocText.AppendCString("body { ");
			sLONG lengthBeforeStyle = outDocText.GetLength();
			if (inSerializeDefaultStyle && inDocDefaultStyle)
			{
				VDocClassStyle *docStyle = dynamic_cast<VDocClassStyle *>(inDocDefaultStyle->Clone());
				xbox_assert(docStyle);
				docStyle->AppendPropsFrom( static_cast<const VDocNode *>(inDoc), false, true);

				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(docStyle), outDocText, NULL, true, kDOC_NODE_TYPE_DOCUMENT, inDoc);

				ReleaseRefCountable(&docStyle);
			}
			else
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inDoc), outDocText, static_cast<const VDocNode *>(inDocDefaultStyle));

			if (lengthBeforeStyle == outDocText.GetLength())
				outDocText.Truncate(lengthBefore);
			else
				outDocText.AppendCString( " }");
		}

		if (inSerializeDefaultStyle && inParaDefaultStyle)
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
			_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), outDocText, NULL, false, kDOC_NODE_TYPE_PARAGRAPH, inDoc);
			outDocText.AppendCString( " }");

			delete styleInheritDefault;
		}
		if (!inSerializeDefaultStyle || !inParaDefaultStyle)
		{
			_NewLine( outDocText);
			_Indent( outDocText, indent);
			outDocText.AppendCString("p,li { white-space:pre-wrap }"); 	//always preserve paragraph whitespaces (otherwise tabs would be consolidated while parsing - cf W3C)
																		//note: it is important because on default whitespaces are consolidated by browsers
																		//(while parsing legacy XHTML11 or XHTML4D, we consolidate white-spaces if CSS property is not set to preserve whitespaces - see ConsolidateWhiteSpaces)
		}


		//append class style sheets if any
		for (int docType = kDOC_NODE_TYPE_DIV; docType < kDOC_NODE_TYPE_CLASS_STYLE; docType++)
		{
			VString sElemName = VDocNode::GetElementName( (eDocNodeType)docType);
			if (sElemName.IsEmpty())
				continue;
			for (int iStyle = 0; iStyle < inDoc->GetClassStyleManager()->GetClassStyleCount((eDocNodeType)docType); iStyle++)
			{
				VString sclass;
				VDocClassStyle *node = inDoc->GetClassStyleManager()->RetainClassStyle( (eDocNodeType)docType, iStyle, &sclass);
				if (sclass.IsEmpty())
					continue; //skip if default style -> we only store here actual class styles

				xbox_assert(node && !sclass.IsEmpty());

				VString stylesCSS;
				_SerializeVDocNodeToCSS( static_cast<VDocNode *>(node), stylesCSS, static_cast<const VDocNode *>(inParaDefaultStyle), true, (eDocNodeType)docType, inDoc);
				
				if (0 == stylesCSS.GetLength())
					//we append at least white-space property in order class style sheet to be not empty
					//(might happen if inParaDefaultStyle is equal to para but we need to keep class style sheet so serialize at least one property)
					stylesCSS.AppendCString(docType == kDOC_NODE_TYPE_PARAGRAPH ? "white-space:pre-wrap" : "white-space:normal");

				_AddAutoClassStyle( inDoc, (eDocNodeType)docType, stylesCSS, &(autoClassStyleNextID[docType]), autoClassStyles, sclass); //add to automatic class styles

				ReleaseRefCountable(&node);
			}
		}
		insertClassStyleAt = outDocText.GetLength();

		indent--;
		_CloseTagStyle( outDocText, true, indent--);
		_CloseTagHead( outDocText, true, indent--);
		xbox_assert(indent == 0);

		//serialize document
		_OpenTagBody(outDocText, ++indent);
		_SerializeAttributeClass( static_cast<const VDocNode *>(inDoc), outDocText);
		_CloseTagBody(outDocText, false);
	}
#endif

	//serialize paragraphs

	bool isListItem = false;
	bool isOrdered = false;
	const VDocParagraph *paraPrev = NULL;
	for (VIndex iChild = 0; iChild < inDoc->GetChildCount(); iChild++)
	{
		const VDocNode *child = inDoc->GetChild(iChild);
		const VDocParagraph *para = dynamic_cast<const VDocParagraph *>(child);
		xbox_assert(para); //TODO: serialize div

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
		{
			//XHTML4D

			fShouldSerializeListStartAuto = false;
			if (para->GetListStyleType() != kDOC_LIST_STYLE_TYPE_NONE)
			{
				bool doNewList = false;
				if (!isListItem)
					doNewList = true;
				else
				{
					xbox_assert(paraPrev && paraPrev->GetListStyleType() != kDOC_LIST_STYLE_TYPE_NONE); 
					bool newIsOrdered = para->GetListStyleType() >= kDOC_LIST_STYLE_TYPE_OL_FIRST && para->GetListStyleImage().IsEmpty();
					if (newIsOrdered != isOrdered)
					{
						_CloseTagList( outDocText, true, indent--, isOrdered);
						doNewList = true;
					}
				}

				if (doNewList)
				{
					isListItem = true;
					isOrdered = para->GetListStyleType() >= kDOC_LIST_STYLE_TYPE_OL_FIRST && para->GetListStyleImage().IsEmpty();
					_OpenTagList( outDocText, ++indent, isOrdered);

					if (isOrdered && para->GetCurListNumber() != 1)
					{
						//for compat with legacy html, set 'start' attribute to current first list item value
						outDocText.AppendCString( " start=\"");
						VString svalue;
						svalue.FromLong( para->GetCurListNumber());
						outDocText.AppendString(svalue);
						outDocText.AppendCString("\"");
						fShouldSerializeListStartAuto = true;
					}

					_CloseTagList( outDocText, false);
				}
			}
			else if (isListItem)
			{
				_CloseTagList( outDocText, true, indent--, isOrdered);
				isListItem = false;
			}

			_OpenTagParagraph(outDocText, ++indent, isListItem); 

			VString autoClassStyle;

			if (para->HasProps() || para->GetStyles()) 
			{
				//serialize paragraph props + paragraph character styles

				const VDocClassStyle *paraDefaultStyle = inParaDefaultStyle;
				VDocClassStyle *paraDefaultStyleOverride = NULL;
				VTextStyle *styleInheritBackup = NULL;
				if (!para->GetClass().IsEmpty())
				{
					xbox_assert(inDoc->GetVersion() >= kDOC_VERSION_XHTML4D);
					VString sclass = para->GetClass();

					//do not serialize styles from class style sheet (because they are serialized in head style element)
					VDocClassStyle* nodeStyle = inDoc->GetClassStyleManager()->RetainClassStyle(kDOC_NODE_TYPE_PARAGRAPH, sclass);
					if (testAssert(nodeStyle))
					{
						//ensure character styles from class style sheet are not serialized
						if (!styleInheritBackup)
						{
							styleInheritBackup = styleInherit;
							styleInherit = new VTextStyle( styleInherit);
						}
						nodeStyle->CharPropsCopyToVTextStyle(*styleInherit, true, false);
				
						if (paraDefaultStyle)
						{
							paraDefaultStyleOverride = dynamic_cast<VDocClassStyle *>(paraDefaultStyle->Clone());
							xbox_assert(paraDefaultStyleOverride);
							paraDefaultStyleOverride->AppendPropsFrom( nodeStyle, false, true);
							ReleaseRefCountable(&nodeStyle);
						}
						else
							paraDefaultStyleOverride = nodeStyle;

						paraDefaultStyle = paraDefaultStyleOverride;
					}
				}

				//serialize paragraph properties (but character styles)
				if (para->HasProps())
					_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(para), autoClassStyle, static_cast<const VDocNode *>(paraDefaultStyle), autoClassStyle.GetLength() == 0);

				//serialize paragraph character styles
				VTextStyle *style = para->GetStyles() ? para->GetStyles()->GetData() : NULL;
				if (style)
					_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(para), *style, autoClassStyle, styleInherit, false, false, autoClassStyle.GetLength() == 0);
				
				if (styleInheritBackup)
				{
					delete styleInherit;
					styleInherit = styleInheritBackup;
					styleInheritBackup = NULL;
				}
				ReleaseRefCountable(&paraDefaultStyleOverride);

				if (autoClassStyle.GetLength() > 0)
				{
					const VString& sclass = _AddAutoClassStyle( inDoc, kDOC_NODE_TYPE_PARAGRAPH, autoClassStyle, &(autoClassStyleNextID[kDOC_NODE_TYPE_PARAGRAPH]), autoClassStyles);
					_SerializeAttributeClass( static_cast<const VDocNode *>(para), outDocText, sclass);
				}
				else
					_SerializeAttributeClass( static_cast<const VDocNode *>(para), outDocText);
			}
		}
		else
#endif
		{
			//SPAN4D

			xbox_assert(iChild == 0); //only one paragraph for span4D
			_OpenTagSpan(outDocText);
			
			VIndex lengthBeforeStyle;
			VIndex startCSS;

			lengthBeforeStyle = outDocText.GetLength();

			if (inSerializeDefaultStyle && inParaDefaultStyle && inParaDefaultStyle->HasProps())
			{
				//span4D: we cannot use style sheet so serialize default properties here

				outDocText.AppendCString(" style=\"");
				startCSS = outDocText.GetLength();

				//serialize default paragraph properties & styles

				//serialize paragraph properties
				_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(inParaDefaultStyle), outDocText, NULL, outDocText.GetLength() == startCSS, kDOC_NODE_TYPE_PARAGRAPH, inDoc);
				
				if (outDocText.GetLength() == startCSS)
					outDocText.Truncate(lengthBeforeStyle);
			}


			if (para->HasProps() || para->GetStyles()) 
			{
				//serialize paragraph props + paragraph character styles
				if (outDocText.GetLength() == lengthBeforeStyle)
				{
					outDocText.AppendCString(" style=\"");
					startCSS = outDocText.GetLength();
				}

				//serialize paragraph properties (but character styles)
				if (para->HasProps())
					_SerializeVDocNodeToCSS( static_cast<const VDocNode *>(para), outDocText, static_cast<const VDocNode *>(inParaDefaultStyle), outDocText.GetLength() == startCSS);

				//serialize paragraph character styles
				VTextStyle *style = para->GetStyles() ? para->GetStyles()->GetData() : NULL;
				if (style)
					_SerializeVTextStyleToCSS( static_cast<const VDocNode *>(para), *style, outDocText, styleInherit, false, false, outDocText.GetLength() == startCSS);
				
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
		}

		//close root opening tag
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
			_CloseTagParagraph( outDocText, false); 
		else
#endif
			_CloseTagSpan( outDocText, false);

		//serialize paragraph plain text & character styles

		fShouldSerializeListStartAuto = false;
		_SerializeParagraphTextAndStyles( child, para->GetStyles(), para->GetText(), outDocText, &(autoClassStyleNextID[0]), autoClassStyles, inDocDefaultStyleMgr, styleInherit, true);

		//close paragraph tag
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
			_CloseTagParagraph( outDocText, true, indent--, isListItem); 
		else
#endif
			_CloseTagSpan( outDocText);

		paraPrev = para;
	}

	if (styleInherit)
		delete styleInherit;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	//close document tag
	if (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D)
	{
		if (isListItem)
			_CloseTagList( outDocText, true, indent--, isOrdered);
		
		_CloseTagBody( outDocText, true, indent--);
		_CloseTagDoc( outDocText);

		//finalize & insert automatic class styles
		VString classStyles;
		_FinalizeAutoClassStyles(autoClassStyles, classStyles, indentClassStyle);
		if (!classStyles.IsEmpty())
			outDocText.Insert( classStyles, insertClassStyleAt+1);
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

	ReleaseRefCountable(&inParaDefaultStyle);
	ReleaseRefCountable(&inDocDefaultStyle);
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

void VSpanTextParser::_OpenTagParagraph(VString& ioText, sLONG inIndent, bool isListItem)
{
	_NewLine(ioText);
	_Indent( ioText, inIndent);
	ioText.AppendCString( isListItem ? "<li" : "<p"); 
}

void VSpanTextParser::_CloseTagParagraph(VString& ioText, bool inAfterChildren, sLONG inIndent, bool isListItem)
{
	if (inAfterChildren)
		//do not indent here as paragraph content white-spaces are preserved between <p> and </p> (!) - otherwise tabs would be consolidated
		ioText.AppendCString( isListItem ? "</li>" : "</p>");
	else
		ioText.AppendCString( ">");
}


void VSpanTextParser::_OpenTagList(VString& ioText, sLONG inIndent, bool inOrdered)
{
	_NewLine(ioText);
	_Indent( ioText, inIndent);
	ioText.AppendCString( inOrdered ? "<ol" : "<ul"); 
}

void VSpanTextParser::_CloseTagList(VString& ioText, bool inAfterChildren, sLONG inIndent, bool inOrdered)
{
	if (inAfterChildren)
	{
		_NewLine(ioText);
		_Indent( ioText, inIndent);
		ioText.AppendCString( inOrdered ? "</ol>" : "</ul>");
	}
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

void VSpanTextParser::_SerializeSpanRefImageTagAndStyles(	const VDocText *inDoc, VDocSpanTextRef *inSpanRef, VString& ioText, 
															sLONG *ioNextAutoClassID, MapOfAutoClassStyle& ioAutoClassStyles, 
															const VDocClassStyleManager *inDocDefaultStyleMgr)
{
	VDocImage *image = inSpanRef->GetImage();
	xbox_assert(image);

	//ensure we do not serialize here default styles or class styles (both are serialized in 'style' element)
	const VDocClassStyle *inImageDefaultStyle = inDocDefaultStyleMgr ? inDocDefaultStyleMgr->RetainFirstClassStyle( kDOC_NODE_TYPE_IMAGE) : NULL;
	const VDocClassStyle *imageDefaultStyle = inImageDefaultStyle;
	VDocClassStyle *imageDefaultStyleOverride = NULL;
	if ((!image->GetClass().IsEmpty()) && (inDoc->GetVersion() >= kDOC_VERSION_XHTML4D))
	{
		VString sclass = image->GetClass();

		//do not serialize styles from class style sheet (because they are serialized in head style element)
		VDocClassStyle* nodeStyle = inDoc->GetClassStyleManager()->RetainClassStyle(kDOC_NODE_TYPE_IMAGE, sclass);
		if (testAssert(nodeStyle))
		{
			if (imageDefaultStyle)
			{
				imageDefaultStyleOverride = dynamic_cast<VDocClassStyle *>(imageDefaultStyle->Clone());
				xbox_assert(imageDefaultStyleOverride);
				imageDefaultStyleOverride->AppendPropsFrom( nodeStyle, false, true); //class styles override default styles
				ReleaseRefCountable(&nodeStyle);
			}
			else
				imageDefaultStyleOverride = nodeStyle;

			imageDefaultStyle = imageDefaultStyleOverride;
		}
	}

	ioText.AppendCString( "<img");
	SerializeAttributesImage( static_cast<const VDocNode *>(image), inSpanRef, ioText);
	
	VString autoClassStyle;
	_SerializePropsCommon( static_cast<const VDocNode *>(image), autoClassStyle, static_cast<const VDocNode *>(imageDefaultStyle), true, kDOC_NODE_TYPE_IMAGE, inDoc);

	if (inDoc->GetVersion() < kDOC_VERSION_XHTML4D)
	{
		//for span4D, serialize here image properties (as for span4D, we do not export style sheet)
		if (autoClassStyle.GetLength() > 0)
		{
			ioText.AppendCString(" style=\"");
			ioText.AppendString(autoClassStyle);
			ioText.AppendUniChar('\"');
		}
	}
	else 
		//for xhtml4D, store class styles temporary (it will be serialized later to style sheet) and serialize class
		if (autoClassStyle.GetLength() > 0)
		{
			const VString& sclass = _AddAutoClassStyle( inDoc, kDOC_NODE_TYPE_IMAGE, autoClassStyle, ioNextAutoClassID+kDOC_NODE_TYPE_IMAGE, ioAutoClassStyles);
			_SerializeAttributeClass( static_cast<const VDocNode *>(image), ioText, sclass);
		}
		else
			_SerializeAttributeClass( static_cast<const VDocNode *>(image), ioText);

	ReleaseRefCountable(&imageDefaultStyleOverride);
	ReleaseRefCountable(&inImageDefaultStyle);
}

void VSpanTextParser::_SerializeParagraphTextAndStyles( const VDocNode *inDocNode, const VTreeTextStyle *inStyles, const VString& inPlainText, VString &ioText, 
														sLONG *ioNextAutoClassID, MapOfAutoClassStyle& ioAutoClassStyles,
														const VDocClassStyleManager *inDocDefaultStyleMgr, const VTextStyle *_inStyleInherit, bool inSkipCharStyles)
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
			TextToXML(text, true, true, true, false, false, true);
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

			sLONG lengthBeforeTagSpan = ioText.GetLength();

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
					_SerializeSpanRefImageTagAndStyles( inDocNode->GetDocumentNode(), style->GetSpanRef(), ioText, ioNextAutoClassID, ioAutoClassStyles, inDocDefaultStyleMgr);
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
				sLONG lengthBeforeStyle = ioText.GetLength();

				ioText.AppendCString(" style=\"");

				sLONG startStyle = ioText.GetLength();

				//serialize CSS character styles

				_SerializeVTextStyleToCSS( inDocNode, *style, ioText, styleInherit);

				//close style 

				if (ioText.GetLength() == startStyle)
				{
					ioText.Truncate( lengthBeforeStyle);
					hasStyle = false;
				}
				else
					ioText.AppendUniChar('\"');
			}

			if (needToEmbedSpanRef)
			{
				if (!hasStyle)
				{
					ioText.Truncate( lengthBeforeTagSpan);
					needToEmbedSpanRef = false;
				}
				else
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
						_SerializeSpanRefImageTagAndStyles( inDocNode->GetDocumentNode(), style->GetSpanRef(), ioText, ioNextAutoClassID, ioAutoClassStyles, inDocDefaultStyleMgr);
					}
#endif
					else 
						_OpenTagSpan( ioText);
				}
			}
			
			if (!style->IsSpanRef() && !hasStyle)
			{
				ioText.Truncate( lengthBeforeTagSpan);
				hasTag = false;
			}
			else if (!isUnknownTag && !isImage)
				_CloseTagSpan(ioText, false); //only close span opening tag
		}
		if (doDeleteStyleInherit)
			delete styleInherit;
	}
	
	if (childCount > 0 && !style->IsSpanRef() && !isImage)
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
				TextToXML(text, true, true, true, false, false, true);
				ioText.AppendString(text);
			}
			_SerializeParagraphTextAndStyles( inDocNode, childStyles, inPlainText, ioText, ioNextAutoClassID, ioAutoClassStyles, inDocDefaultStyleMgr, styleInheritForChildren ? styleInheritForChildren : _inStyleInherit);
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
			TextToXML(text, true, true, true, false, false, true);
			ioText.AppendString( text);
			}
			break;
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		case kSpanTextRef_Image: 
			fIsOnlyPlainText = false; //image does not have plain text (cf HTML W3C)
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
		TextToXML(text, true, true, true, false, false, true);
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
				ioText.AppendCString( "/>");
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
	cssParser.Start(inCSSDeclarations);
	VString attribut, value;

	bool parseSpanCharacterProperties = false;
	bool parseCommonProperties = false;
	bool parseParagraphProperties = false;
	VDocText *doc = NULL;
	VTextStyle *style = NULL;

	switch (ioDocNode->GetType())
	{
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		case kDOC_NODE_TYPE_DOCUMENT:	//root node (<body> node) -> parse document properties 
			{
				xbox_assert(isXHTML4D);

				doc = dynamic_cast<VDocText *>(ioDocNode);
				xbox_assert(doc);

				style = new VTextStyle();
				parseSpanCharacterProperties = true;
				parseCommonProperties = true;
				parseParagraphProperties = true;
			}
			break;
#endif
		case kDOC_NODE_TYPE_PARAGRAPH:
			{
				//parse paragraph properties

				VDocParagraph *para = NULL;
				const VTreeTextStyle *styles = NULL;

				para = dynamic_cast<VDocParagraph *>(ioDocNode);
				xbox_assert(para);

				styles = para->GetStyles();
				xbox_assert(styles);
				style = styles->GetData();
				
				parseSpanCharacterProperties = true;
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
				parseCommonProperties = isXHTML4D;
				parseParagraphProperties = isXHTML4D;
#endif
			}
			break;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		case kDOC_NODE_TYPE_IMAGE:
			{
				//parse image properties
				xbox_assert((inSpanHandler->GetOptions() & kHTML_PARSER_TAG_IMAGE_SUPPORTED) != 0);
				parseCommonProperties = true;
			}
			break;
		case kDOC_NODE_TYPE_CLASS_STYLE:	
			{
				xbox_assert(isXHTML4D);
				style = new VTextStyle();
				parseSpanCharacterProperties = true;
				if (!inSpanHandler->IsNodeList())
					parseCommonProperties = true;
				parseParagraphProperties = true;
			}
			break;
#endif
		default:
			break;
	}

	while (cssParser.GetNextAttribute(attribut, value))
	{
		if (doc)
		{
			bool parsed = true;
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
			else
				parsed = false;
			if (parsed)
				continue;
		}
		
		if (parseSpanCharacterProperties)
		{
			//span character properties  
			xbox_assert(style);

			bool parsed = true;
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
#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
			else if (isXHTML4D && attribut == kCSS_NAME_TEXT_BACKGROUND_COLOR) 
				_ParsePropSpan( lexParser, ioDocNode, kDOC_PROP_BACKGROUND_COLOR, value, *style);
#endif
			else if (isSpan4D && attribut == kCSS_NAME_TEXT_ALIGN) //if SPAN4D format, it is a span node 
				_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_TEXT_ALIGN, value);
			else 
				parsed = false;
			if (parsed)
				continue;
		}

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
		if (parseCommonProperties)
		{
			//node common properties
			bool parsed = true;
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
			else 
				parsed = false;
			if (parsed)
				continue;
		}
		if (parseParagraphProperties)
		{
			//paragraph properties
			
			bool parsed = true;
			if (attribut == kCSS_NAME_TEXT_ALIGN)
				_ParsePropCommon(lexParser, ioDocNode, kDOC_PROP_TEXT_ALIGN, value);
			else if (attribut == kCSS_NAME_DIRECTION)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_DIRECTION, value);
			else if (attribut == kCSS_NAME_LINE_HEIGHT)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LINE_HEIGHT, value);
			else if (attribut == kCSS_NAME_TEXT_INDENT)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_TEXT_INDENT, value);
			else if (attribut == kCSS_NAME_D4_TAB_STOP_OFFSET)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_TAB_STOP_OFFSET, value);
			else if (attribut == kCSS_NAME_D4_TAB_STOP_TYPE)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_TAB_STOP_TYPE, value);
			else if (attribut == kCSS_NAME_LIST_STYLE_TYPE)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_STYLE_TYPE, value);
			else if (attribut == kCSS_NAME_D4_NEW_PARA_CLASS)
				_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_NEW_PARA_CLASS, value);
			else if (ioDocNode->GetType() == kDOC_NODE_TYPE_CLASS_STYLE || inSpanHandler->IsNodeList() || inSpanHandler->IsNodeListItem())
			{
				if (attribut == kCSS_NAME_D4_LIST_STYLE_TYPE)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_STYLE_TYPE, value);
				else if (attribut == kCSS_NAME_D4_LIST_STRING_FORMAT_LTR)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_STRING_FORMAT_LTR, value);
				else if (attribut == kCSS_NAME_D4_LIST_STRING_FORMAT_RTL)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_STRING_FORMAT_RTL, value);
				else if (attribut == kCSS_NAME_D4_LIST_FONT_FAMILY)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_FONT_FAMILY, value);
				else if (attribut == kCSS_NAME_LIST_STYLE_IMAGE)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_STYLE_IMAGE, value);
				else if (attribut == kCSS_NAME_D4_LIST_STYLE_IMAGE_HEIGHT)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT, value);
				else if (attribut == kCSS_NAME_D4_LIST_START)
					_ParsePropParagraph(lexParser, ioDocNode, kDOC_PROP_LIST_START, value);
				else
					parsed = false;
			}
			else
				parsed = false;
			if (parsed)
				continue;
		}
#endif
		if (!isSpan4D && attribut == kCSS_NAME_WHITE_SPACE)
			_ParseCSSWhiteSpace( inSpanHandler, value);
	}

	if (style && ioDocNode->GetType() != kDOC_NODE_TYPE_PARAGRAPH) //for paragraph, char properties are embedded in fStyles after parsing but not for other elements
	{
		ioDocNode->VTextStyleCopyToCharProps( *style, true);
		delete style;
	}
	delete lexParser;
}


/** serialize VDocNode props to CSS declaration string */ 
void VSpanTextParser::_SerializeVDocNodeToCSS(  const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault, bool inIsFirst, eDocNodeType inClassStyleNodeType, const VDocText *inClassStyleOwnerDoc)
{
	VIndex count = inIsFirst ? 0 : 1;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	if (inDocNode->GetType() == kDOC_NODE_TYPE_DOCUMENT || (inDocNode->GetType() == kDOC_NODE_TYPE_CLASS_STYLE && inClassStyleNodeType == kDOC_NODE_TYPE_DOCUMENT))
	{
		//serialize document properties (this document node contains only document properties)
		const VDocText *doc = inDocNode->GetDocumentNode();
		if (!doc)
			doc = inClassStyleOwnerDoc;
		xbox_assert(doc);
		
		if (doc->GetVersion() >= kDOC_VERSION_XHTML4D) 
		{
			//CSS d4-version (d4-version value = inDocNode->GetDocumentNode()->GetVersion()-1)
			bool isOverriden = inDocNode->IsOverriden( kDOC_PROP_VERSION) && !inDocNode->IsInherited( kDOC_PROP_VERSION) && (!inDocNodeDefault || inDocNode->GetVersion() != inDocNodeDefault->GetVersion());
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_VERSION);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)(inDocNode->GetVersion() < kDOC_VERSION_SPAN4D ? kDOC_VERSION_SPAN4D-1 : inDocNode->GetVersion()-1));
				count++;
			}
			//CSS d4-dpi
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_DPI) && !inDocNode->IsInherited( kDOC_PROP_DPI) && (!inDocNodeDefault || inDocNode->GetDPI() != inDocNodeDefault->GetDPI());
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_DPI);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)inDocNode->GetDPI());
				count++;
			}

			//CSS d4-zoom
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_ZOOM) && !inDocNode->IsInherited( kDOC_PROP_ZOOM) && (!inDocNodeDefault || inDocNode->GetZoom() != inDocNodeDefault->GetZoom());
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_ZOOM);
				outCSSDeclarations.AppendUniChar( ':');
				outCSSDeclarations.AppendLong( (sLONG)inDocNode->GetZoom());
				outCSSDeclarations.AppendUniChar('%');
				count++;
			}

			//CSS d4-dwrite
			isOverriden = inDocNode->IsOverriden( kDOC_PROP_DWRITE) && !inDocNode->IsInherited( kDOC_PROP_DWRITE) && (!inDocNodeDefault || inDocNode->GetDWrite() != inDocNodeDefault->GetDWrite());
			if (isOverriden)
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_DWRITE);
				outCSSDeclarations.AppendUniChar( ':');
				if (inDocNode->GetDWrite())
					outCSSDeclarations.AppendCString( "true");
				else
					outCSSDeclarations.AppendCString( "false");
				count++;
			}

			VIndex length = outCSSDeclarations.GetLength();
			_SerializePropsCommon( inDocNode, outCSSDeclarations, inDocNodeDefault, count == 0, inClassStyleNodeType, inClassStyleOwnerDoc);
			if (length != outCSSDeclarations.GetLength())
				count++;
		}
	}
	else 
#endif
	{
		//serialize common properties

		VIndex length = outCSSDeclarations.GetLength();
		_SerializePropsCommon( inDocNode, outCSSDeclarations, inDocNodeDefault, inIsFirst, inClassStyleNodeType, inClassStyleOwnerDoc);
		if (length != outCSSDeclarations.GetLength())
			count++;
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
		return inDocNode->GetFontSize();
}

/** get paragraph font size (in TWIPS) */
sLONG VSpanTextParser::_GetParagraphFontSize( const VDocParagraph *inDocPara)
{
	if (inDocPara->GetStyles())
	{
		Real rfontsize = inDocPara->GetStyles()->GetData()->GetFontSize();
		if (rfontsize > 0)
			return ICSSUtil::PointToTWIPS(rfontsize);
	}
	return inDocPara->GetFontSize();
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
		ioDocImage->SetAltText( value);
		result = true;
	}
	return result;
}

/** parse paragraph CSS property */	
bool VSpanTextParser::_ParsePropParagraph( VCSSLexParser *inLexParser, VDocNode *ioDocNode, const eDocProperty inProperty, const VString& inValue)
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
				ioDocNode->SetInherit( kDOC_PROP_DIRECTION);
			else
				ioDocNode->SetDirection((eDocPropDirection)valueCSS->v.css.fDirection);
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
				ioDocNode->SetInherit( kDOC_PROP_LINE_HEIGHT);
			else
			{
				if (valueCSS->v.css.fLength.fAuto)
					ioDocNode->SetLineHeight( kDOC_PROP_LINE_HEIGHT_NORMAL);
				else if (valueCSS->v.css.fLength.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
					ioDocNode->SetLineHeight( -floor(valueCSS->v.css.fLength.fNumber * 100 + 0.5f));
				else
					ioDocNode->SetLineHeight( ICSSUtil::CSSDimensionToTWIPS( valueCSS->v.css.fLength.fNumber, 
																			(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																			ioDocNode->GetDocumentNode()->GetDPI(),
																			_GetFontSize(ioDocNode)));
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
				ioDocNode->SetInherit( kDOC_PROP_TEXT_INDENT);
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
					ioDocNode->SetTextIndent( ICSSUtil::CSSDimensionToTWIPS( valueCSS->v.css.fLength.fNumber, 
																			(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																			ioDocNode->GetDocumentNode()->GetDPI(),
																			_GetFontSize(ioDocNode)));
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
				ioDocNode->SetInherit( kDOC_PROP_TAB_STOP_OFFSET);
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
					ioDocNode->SetTabStopOffset( (uLONG)ICSSUtil::CSSDimensionToTWIPS(	valueCSS->v.css.fLength.fNumber, 
																						(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																						ioDocNode->GetDocumentNode()->GetDPI(),
																						_GetFontSize(ioDocNode)));
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
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::TABSTOPTYPE4D, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_TAB_STOP_TYPE);
			else
				ioDocNode->SetTabStopType((eDocPropTabStopType)valueCSS->v.css.fTabStopType);
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
			ioDocNode->SetNewParaClass( inLexParser->GetCurTokenValue()); 
			result = true;

		}
	}
	else if (inProperty == kDOC_PROP_LIST_STYLE_TYPE)
	{
		//CSS list-style-type or -d4-list-style-type

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LISTSTYLETYPE4D, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
				ioDocNode->SetInherit( kDOC_PROP_LIST_STYLE_TYPE);
			else
				ioDocNode->SetListStyleType((eDocPropListStyleType)valueCSS->v.css.fListStyleType);
			result = true;
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LIST_STRING_FORMAT_LTR)
	{
		//CSS -d4-list-string-format-ltr

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::STRING, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_LIST_STRING_FORMAT_LTR);
				result = true;
			}
			else if (valueCSS->v.css.fString)
			{
				ioDocNode->SetListStringFormatLTR( (*(valueCSS->v.css.fString)));
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LIST_STRING_FORMAT_RTL)
	{
		//CSS -d4-list-string-format-rtl

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::STRING, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_LIST_STRING_FORMAT_RTL);
				result = true;
			}
			else if (valueCSS->v.css.fString)
			{
				ioDocNode->SetListStringFormatRTL( (*(valueCSS->v.css.fString)));
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LIST_FONT_FAMILY)
	{
		//CSS -d4-list-font-family

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::FONTFAMILY, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_LIST_FONT_FAMILY);
				result = true;
			}
			else if (valueCSS->v.css.fFontFamily)
			{
				ioDocNode->SetFontFamily( (*(valueCSS->v.css.fFontFamily))[0]);
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LIST_STYLE_IMAGE)
	{
		//CSS list-style-image

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::URI, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_LIST_STYLE_IMAGE);
				result = true;
			}
			else if (valueCSS->v.css.fURI)
			{
				ioDocNode->SetListStyleImage( *(valueCSS->v.css.fURI));
				result = true;
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT)
	{
		//CSS -d4-list-style-image-height

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::LENGTH, NULL, true);
		if (valueCSS)
		{
			if (valueCSS->fInherit)
			{
				ioDocNode->SetInherit( kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT);
				result = true;
			}
			else
			{
				if (valueCSS->v.css.fLength.fAuto || valueCSS->v.css.fLength.fUnit == CSSProperty::LENGTH_TYPE_PERCENT)
				{
					ioDocNode->SetListStyleImageHeight( 0);
				}
				else
				{
					ioDocNode->SetListStyleImageHeight( (uLONG)ICSSUtil::CSSDimensionToTWIPS(	valueCSS->v.css.fLength.fNumber, 
																						(eCSSUnit)valueCSS->v.css.fLength.fUnit, 
																						ioDocNode->GetDocumentNode()->GetDPI(),
																						_GetFontSize(ioDocNode)));
					result = true;
				}
			}
		}
		ReleaseRefCountable(&valueCSS);
	}
	else if (inProperty == kDOC_PROP_LIST_START)
	{
		//CSS -d4-list-start (or HTML 'start' attribute)

		inLexParser->Start( inValue.GetCPointer());
		inLexParser->Next(true);
		if (inLexParser->GetCurToken() == CSSToken::IDENT && inLexParser->GetCurTokenValue() == "auto")
			ioDocNode->SetListStart( kDOC_LIST_START_AUTO);
		else
		{
			CSSProperty::Value *valueCSS = VCSSParser::ParseValue(inLexParser, CSSProperty::NUMBER, NULL, true);
			if (valueCSS)
			{
				if (valueCSS->fInherit)
				{
					ioDocNode->SetInherit( kDOC_PROP_LIST_START);
					result = true;
				}
				else
					ioDocNode->SetListStart( (sLONG)valueCSS->v.css.fNumber);
			}
			ReleaseRefCountable(&valueCSS);
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
		if (inLexParser->GetCurToken() == CSSToken::IDENT && inLexParser->GetCurTokenValue().EqualToString(CVSTR("none")))
			ioDocNode->SetBackgroundImage( CVSTR(""));
		else
		{
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

bool VSpanTextParser::_SerializeAttributeClass( const VDocNode *inDocNode, VString& outText, const VString& inAutoClass)
{
	if (inDocNode->GetClass().IsEmpty() && inAutoClass.IsEmpty())
		return false;

	outText.AppendCString(" class=\"");
	VString sclass = inDocNode->GetClass();
	if (!inAutoClass.IsEmpty())
	{
		if (!sclass.IsEmpty())
		{
			sclass.AppendUniChar(' ');
			sclass.AppendString(inAutoClass);
		}
		else
			sclass = inAutoClass;
	}
	TextToXML( sclass, true, true, true, false, true); 
	outText.AppendString( sclass);
	outText.AppendUniChar('\"');
	return true;
}

/** serialize common CSS properties to CSS styles */
void VSpanTextParser::_SerializePropsCommon( const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault, bool inIsFirst, eDocNodeType inClassStyleNodeType, const VDocText *inClassStyleOwnerDoc)
{
	const VDocText *doc = inDocNode->GetDocumentNode();
	if (!doc)
		doc = inClassStyleOwnerDoc;
	xbox_assert(doc);

	outCSSDeclarations.EnsureSize( outCSSDeclarations.GetLength()+256); //for perf: avoid multiple memory alloc

	VIndex count = inIsFirst ? 0 : 1;
	bool isOverriden;

#if kDOC_VERSION_MAX_SUPPORTED >= kDOC_VERSION_XHTML4D  
	bool isXHTML4D = (doc && doc->GetVersion() >= kDOC_VERSION_XHTML4D) || (inDocNode->GetType() > kDOC_NODE_TYPE_PARAGRAPH && inDocNode->GetType() <= kDOC_NODE_TYPE_CLASS_STYLE);
	if (isXHTML4D)
	{
	//CSS margin
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_MARGIN, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_PADDING, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_WIDTH, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_MIN_WIDTH, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_HEIGHT, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_MIN_HEIGHT, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_COLOR, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_IMAGE, inDocNodeDefault);
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
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_BACKGROUND_IMAGE);
				if (uri.IsEmpty())
					outCSSDeclarations.AppendCString(":none");
				else
				{
					outCSSDeclarations.AppendCString( ":url('");
					TextToXML( uri, true, true, true, true, true); //convert for CSS 
					outCSSDeclarations.AppendString( uri);
					outCSSDeclarations.AppendCString( "')");
				}
				count++;
			}
		}
	}
	//CSS background-position 
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_POSITION, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_REPEAT, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_CLIP, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_ORIGIN, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BACKGROUND_SIZE, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_STYLE, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_WIDTH, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_COLOR, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_BORDER_RADIUS, inDocNodeDefault);
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
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_VERTICAL_ALIGN, inDocNodeDefault);
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
			if ((textAlign != JST_Default 
				&& 
				textAlign != JST_Mixed
				&&
				textAlign != JST_Baseline //default is equal to baseline or top depending on element (for instance it is baseline for embedded image and top for paragraph in cell)
				)
				||
				(inDocNodeDefault && inDocNodeDefault->IsOverriden(kDOC_PROP_VERTICAL_ALIGN))
				)
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

	if (inDocNode->GetType() == kDOC_NODE_TYPE_IMAGE)
		return;

	//serialize text only properties

	//CSS text-align
	isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TEXT_ALIGN, inDocNodeDefault);
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
				(textAlign != JST_Default 
				&& 
				textAlign != JST_Mixed)
				||
				(inDocNodeDefault && inDocNodeDefault->IsOverriden(kDOC_PROP_TEXT_ALIGN))
				)
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
	if (isXHTML4D)
	{
		//CSS direction
		bool isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_DIRECTION, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_DIRECTION))
			{
				//inherited on default
			}
			else
			{
				eDocPropDirection direction = inDocNode->GetDirection();
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
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TEXT_INDENT, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_TEXT_INDENT))
			{
				//inherited on default
			}
			else
			{
				sLONG padding = inDocNode->GetTextIndent();
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
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LINE_HEIGHT, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LINE_HEIGHT))
			{
				//inherited on default
			}
			else
			{
				sLONG lineHeight = inDocNode->GetLineHeight();
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
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TAB_STOP_OFFSET, inDocNodeDefault);
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
				uLONG offset = inDocNode->GetTabStopOffset();
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
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_TAB_STOP_TYPE, inDocNodeDefault);
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
				eDocPropTabStopType type = inDocNode->GetTabStopType();
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
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_NEW_PARA_CLASS, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_NEW_PARA_CLASS))
			{
				//inherited on default
				xbox_assert(false); //should not be inherited
			}
			else
			{
				VString newparaclass = inDocNode->GetNewParaClass();
				if ((!newparaclass.IsEmpty() && newparaclass != inDocNode->GetClass()) || (inDocNodeDefault && inDocNodeDefault->IsOverriden(kDOC_PROP_NEW_PARA_CLASS)))
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

		//CSS list-style-type & -d4-list-style-type (we need to serialize both for compatibility with legacy css)
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LIST_STYLE_TYPE, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LIST_STYLE_TYPE))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_LIST_STYLE_TYPE);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;

				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STYLE_TYPE);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				eDocPropListStyleType type = inDocNode->GetListStyleType();
				{
					//legacy css
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_LIST_STYLE_TYPE);
					outCSSDeclarations.AppendUniChar( ':');

					VString value;
					switch (type)
					{
						case kDOC_LIST_STYLE_TYPE_DISC:
							value = "disc";
							break;
						case kDOC_LIST_STYLE_TYPE_CIRCLE:
							value = "circle";
							break;
						case kDOC_LIST_STYLE_TYPE_SQUARE:
							value = "square";
							break;
						case kDOC_LIST_STYLE_TYPE_DECIMAL:
							value = "decimal";
							break;
						case kDOC_LIST_STYLE_TYPE_DECIMAL_LEADING_ZERO:
							value = "decimal-leading-zero";
							break;
						case kDOC_LIST_STYLE_TYPE_LOWER_LATIN:
							value = "lower-latin";
							break;
						case kDOC_LIST_STYLE_TYPE_LOWER_ROMAN:
							value = "lower-roman";
							break;
						case kDOC_LIST_STYLE_TYPE_UPPER_LATIN:
							value = "upper-latin";
							break;
						case kDOC_LIST_STYLE_TYPE_UPPER_ROMAN:
							value = "upper-roman";
							break;
						case kDOC_LIST_STYLE_TYPE_LOWER_GREEK:
							value = "lower-greek";
							break;
						case kDOC_LIST_STYLE_TYPE_ARMENIAN:
							value = "armenian";
							break;
						case kDOC_LIST_STYLE_TYPE_GEORGIAN:
							value = "georgian";
							break;
						case kDOC_LIST_STYLE_TYPE_HEBREW:
							value = "hebrew";
							break;
						case kDOC_LIST_STYLE_TYPE_HIRAGANA:
							value = "hiragana";
							break;
						case kDOC_LIST_STYLE_TYPE_KATAKANA:
							value = "katakana";
							break;
						case kDOC_LIST_STYLE_TYPE_CJK_IDEOGRAPHIC:
							value = "cjk-ideographic";
							break;
						case kDOC_LIST_STYLE_TYPE_NONE:
							value = "none";
							break;
						case kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK:
							value = "lower-greek";
							break;
						default:
							value = "disc";
							break;
					}
					outCSSDeclarations.AppendString( value);
					count++;

					//4D css
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STYLE_TYPE);
					outCSSDeclarations.AppendUniChar( ':');

					switch (type)
					{
						case kDOC_LIST_STYLE_TYPE_HOLLOW_SQUARE:
							value = "hollow-square";
							break;
						case kDOC_LIST_STYLE_TYPE_DIAMOND:
							value = "diamond";
							break;
						case kDOC_LIST_STYLE_TYPE_CLUB:
							value = "club";
							break;
						case kDOC_LIST_STYLE_TYPE_CUSTOM:
							value = "custom";
							break;
						case kDOC_LIST_STYLE_TYPE_DECIMAL_GREEK:
							value = "decimal-greek";
							break;
						default:
							break;
					}
					outCSSDeclarations.AppendString( value);
					count++;
				}
			}
		}

		//CSS list-style-image 
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LIST_STYLE_IMAGE, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LIST_STYLE_IMAGE))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_LIST_STYLE_IMAGE);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				VString uri = inDocNode->GetListStyleImage();
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');

					outCSSDeclarations.AppendCString( kCSS_NAME_LIST_STYLE_IMAGE);
					if (uri.IsEmpty())
						outCSSDeclarations.AppendCString(":none");
					else
					{
						outCSSDeclarations.AppendCString( ":url('");
						TextToXML( uri, true, true, true, true, true); //convert for CSS 
						outCSSDeclarations.AppendString( uri);
						outCSSDeclarations.AppendCString( "')");
					}
					count++;
				}
			}
		}

		//CSS -d4-list-style-image-height
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LIST_STYLE_IMAGE_HEIGHT))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STYLE_IMAGE_HEIGHT);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				uLONG height = inDocNode->GetListStyleImageHeight();
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STYLE_IMAGE_HEIGHT);
					outCSSDeclarations.AppendUniChar( ':');

					VString sValue;
					_TWIPSToCSSDimPoint( (sLONG)height, sValue);
					outCSSDeclarations.AppendString( sValue);
					count++;
				}
			}
		}

		//CSS -d4-list-start 
		sLONG start = inDocNode->GetListStart();
		isOverriden = inDocNode->IsOverriden(kDOC_PROP_LIST_START) && (!inDocNodeDefault || !inDocNodeDefault->IsOverriden(kDOC_PROP_LIST_START) || start != inDocNodeDefault->GetListStart());
		if (inDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH && fShouldSerializeListStartAuto)
		{
			isOverriden = true;
			start = kDOC_LIST_START_AUTO;
		}
		else if (start == kDOC_LIST_START_AUTO && !fShouldSerializeListStartAuto)
			isOverriden = false;
		if (isOverriden)
		{
			if (inDocNode->IsInherited(  kDOC_PROP_LIST_START))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');
				outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_START);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');
					outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_START);
					outCSSDeclarations.AppendUniChar( ':');

					if (start == kDOC_LIST_START_AUTO)
						outCSSDeclarations.AppendCString( "auto");
					else
					{
						VString sValue;
						sValue.FromLong( start);
						outCSSDeclarations.AppendString( sValue);
					}
					count++;
				}
			}
		}

		//CSS -d4-list-string-format-ltr 
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LIST_STRING_FORMAT_LTR, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LIST_STRING_FORMAT_LTR))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STRING_FORMAT_LTR);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				VString text = inDocNode->GetListStringFormatLTR();
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');

					outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STRING_FORMAT_LTR);
					outCSSDeclarations.AppendCString( ":'");
					TextToXML( text, true, true, true, true, true); //convert for CSS 
					outCSSDeclarations.AppendString( text);
					outCSSDeclarations.AppendUniChar( '\'');
					count++;
				}
			}
		}

		//CSS -d4-list-string-format-rtl
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LIST_STRING_FORMAT_RTL, inDocNodeDefault);
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LIST_STRING_FORMAT_RTL))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STRING_FORMAT_RTL);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				VString text = inDocNode->GetListStringFormatRTL();
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');

					outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_STRING_FORMAT_RTL);
					outCSSDeclarations.AppendCString( ":'");
					TextToXML( text, true, true, true, true, true); //convert for CSS 
					outCSSDeclarations.AppendString( text);
					outCSSDeclarations.AppendUniChar( '\'');
					count++;
				}
			}
		}

		//CSS -d4-list-font-family
		isOverriden = inDocNode->ShouldSerializeProp( kDOC_PROP_LIST_FONT_FAMILY, inDocNodeDefault);
		if (!isOverriden 
			&& 
			inDocNode->GetListStyleType() != kDOC_LIST_STYLE_TYPE_NONE 
			&& 
			inDocNode->IsOverriden( kDOC_PROP_LIST_FONT_FAMILY) 
			&& 
			!VDocNode::GetDefaultListFontFamily( inDocNode->GetListStyleType()).EqualToStringRaw( inDocNode->GetListFontFamily()))
			isOverriden = true;
		if (isOverriden)
		{
			if (inDocNode->IsInherited( kDOC_PROP_LIST_FONT_FAMILY))
			{
				if (count)
					outCSSDeclarations.AppendUniChar(';');

				outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_FONT_FAMILY);
				outCSSDeclarations.AppendCString( ":inherit");
				count++;
			}
			else
			{
				VString text = inDocNode->GetFontFamily();
				{
					if (count)
						outCSSDeclarations.AppendUniChar(';');

					outCSSDeclarations.AppendCString( kCSS_NAME_D4_LIST_FONT_FAMILY);
					outCSSDeclarations.AppendCString( ":'");
					TextToXML( text, true, true, true, true, true); //convert for CSS 
					outCSSDeclarations.AppendString( text);
					outCSSDeclarations.AppendUniChar( '\'');
					count++;
				}
			}
		}
	}
#endif

	//serialize span character properties
	VTextStyle *style = new VTextStyle();
	inDocNode->CharPropsCopyToVTextStyle( *style, true, false);
	if (!style->IsUndefined())
	{
		VTextStyle *styleInherit = NULL;
		if (inDocNodeDefault)
		{
			styleInherit = new VTextStyle();
			inDocNodeDefault->CharPropsCopyToVTextStyle( *styleInherit, true, false);
		}
		_SerializeVTextStyleToCSS( inDocNode, *style, outCSSDeclarations, styleInherit, false, true, count == 0, inClassStyleNodeType, inClassStyleOwnerDoc);
		if (styleInherit)
			delete styleInherit;
	}
	delete style;
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
	VString text = inSpanRef->GetImage()->GetAltText();
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
	cssParser.Start(inCSSDeclarations);
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
void VSpanTextParser::_SerializeVTextStyleToCSS(const VDocNode *inDocNode, const VTextStyle &inStyle, VString& outCSSDeclarations, 
												const VTextStyle *_inStyleInherit, bool inIgnoreColors, bool inD4TextBackgroundColor, bool inIsFirst,
												eDocNodeType inClassStyleNodeType, const VDocText *inClassStyleOwnerDoc)
{
	const VDocText *doc = inDocNode->GetDocumentNode();
	if (!doc)
		doc = inClassStyleOwnerDoc;
	xbox_assert(doc);

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
	RGBAColor paraBackgroundColor = inDocNode->GetType() == kDOC_NODE_TYPE_PARAGRAPH || (inDocNode->GetType() == kDOC_NODE_TYPE_CLASS_STYLE && inClassStyleNodeType == kDOC_NODE_TYPE_PARAGRAPH) ? 
									inDocNode->GetBackgroundColor() : RGBACOLOR_TRANSPARENT;

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
		{
#if VERSIONWIN
			uLONG version = doc->GetVersion();
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
			//background-color (span node) or -d4-text-background-color (other node - to distinguish between element & character back color)

			if (!(paraBackgroundColor != RGBACOLOR_TRANSPARENT 
				&& 
				((inStyle.GetHasBackColor() && inStyle.GetBackGroundColor() == paraBackgroundColor) 
				 || 
				 (!inStyle.GetHasBackColor() && styleInherit->GetHasBackColor() && styleInherit->GetBackGroundColor() == paraBackgroundColor))))
			{
				//we serialize background color character style if and only if it is different from the paragraph background color

				bool doIt = true;

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
					outCSSDeclarations.AppendCString( inD4TextBackgroundColor ? kCSS_NAME_TEXT_BACKGROUND_COLOR : kCSS_NAME_BACKGROUND_COLOR);
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
