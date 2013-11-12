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
#ifndef __VSpanText__
#define __VSpanText__

BEGIN_TOOLBOX_NAMESPACE

//span properties (character styles)

#define kCSS_NAME_FONT_FAMILY			"font-family"
#define kCSS_NAME_FONT_SIZE				"font-size"
#define kCSS_NAME_COLOR					"color"
#define kCSS_NAME_FONT_STYLE			"font-style"
#define kCSS_NAME_FONT_WEIGHT			"font-weight"
#define kCSS_NAME_TEXT_DECORATION		"text-decoration"
#define kCSS_NAME_BACKGROUND_COLOR		"background-color" 
#define kCSS_NAME_BACKGROUND_IMAGE		"background-image" 
#define kCSS_NAME_BACKGROUND_POSITION	"background-position" 
#define kCSS_NAME_BACKGROUND_REPEAT		"background-repeat" 
#define kCSS_NAME_BACKGROUND_CLIP		"background-clip" 
#define kCSS_NAME_BACKGROUND_ORIGIN		"background-origin" 
#define kCSS_NAME_BACKGROUND_SIZE		"background-size" 
#define kCSS_NAME_D4_REF				"-d4-ref"
#define kCSS_NAME_D4_REF_USER			"-d4-ref-user" 

//document node properties

#define kCSS_NAME_D4_DPI				"-d4-dpi"
#define kCSS_NAME_D4_VERSION			"-d4-version" //if "d4-version" is not defined, it is assumed to be v12/v13 4D SPAN compatible format 
#define kCSS_NAME_D4_ZOOM				"-d4-zoom" 
#define kCSS_NAME_D4_DWRITE				"-d4-dwrite" 

//document node common properties

#define kCSS_NAME_TEXT_ALIGN			"text-align"
#define kCSS_NAME_VERTICAL_ALIGN		"vertical-align"
#define kCSS_NAME_MARGIN				"margin"
#define kCSS_NAME_PADDING				"padding"
#define kCSS_NAME_WIDTH					"width"
#define kCSS_NAME_HEIGHT				"height"
#define kCSS_NAME_MIN_WIDTH				"min-width"
#define kCSS_NAME_MIN_HEIGHT			"min-height"
#define kCSS_NAME_BORDER_STYLE			"border-style"
#define kCSS_NAME_BORDER_WIDTH			"border-width"
#define kCSS_NAME_BORDER_COLOR			"border-color"
#define kCSS_NAME_BORDER_RADIUS			"border-radius"


//#define kCSS_NAME_BACKGROUND_COLOR	"background-color" //here it is applied to element background (paragraph background for instance) - including padding if any but not margin

//paragraph node properties

#define kCSS_NAME_DIRECTION				"direction"
#define kCSS_NAME_LINE_HEIGHT			"line-height"
#define kCSS_NAME_TEXT_INDENT			"text-indent"
#define kCSS_NAME_D4_TAB_STOP_OFFSET	"-d4-tab-stop-offset"
#define kCSS_NAME_D4_TAB_STOP_TYPE		"-d4-tab-stop-type"


class XTOOLBOX_API VSpanTextParser : public VObject , public ISpanTextParser
{
public:
	VSpanTextParser():VObject() { fDelegateRef = NULL; }

#if VERSIONWIN
virtual float GetSPANFontSizeDPI() const;
#endif

/** return true if character is valid XML 1.1 
	(some control & special chars are not supported by Xerces & XML 1.1 like [#x1-#x8], [#xB-#xC], [#xE-#x1F], [#x7F-#x84], [#x86-#x9F] - cf W3C) */
virtual bool IsXMLChar(UniChar inChar) const;

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
	if true, convert "\" to "\\" because in CSS '\' is used for escaping characters (!)  
@param inEscapeCR
	if true, escapes CR to "\0D" in CSS or "&#xD;" in XML (exclusive with inConvertCRtoBRTag) - should be used only while serializing XML or CSS strings
@remarks
	see IsXMLChar 

	note that end of lines are normalized to one CR only (0x0D) - if end of lines are not converted to <BR/>

	if inFixInvalidChars == true, invalid chars are converted to whitespace (but for 0x0B which is replaced with 0x0D) 
	otherwise Xerces would fail parsing invalid XML 1.1 characters - parser would throw exception
*/
virtual void TextToXML(VString& ioText, bool inFixInvalidChars = true, bool inEscapeXMLMarkup = true, bool inConvertCRtoBRTag = false, bool inConvertForCSS = false, bool inEscapeCR = false);


/** parse span text element or XHTML fragment 
@param inTaggedText
	tagged text to parse (span element text only if inParseSpanOnly == true, XHTML fragment otherwise)
@param outStyles
	parsed styles
@param outPlainText
	parsed plain text
@param inParseSpanOnly
	true (default): input text is 4D SPAN tagged text - 4D CSS rules apply 
	false: input text is XHTML 1.1 fragment (generally from external source than 4D) 
@param inDPI
	dpi used to convert from pixel to point
@param inShowSpanRef
	false (default): plain text contains span ref computed values if any (4D expression result, url link user text, etc...)
	true: plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
@param inNoUpdateRefs
	false: replace plain text by source or computed span ref values after parsing (call VDocText::UpdateSpanRefs)
	true (default): span refs plain text is set to only one unbreakable space

CAUTION: this method returns only parsed character styles: you should use now ParseDocument method which returns a VDocText * if you need to preserve paragraph or document properties
*/
virtual void ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly = true, const uLONG inDPI = 72, bool inShowSpanRef = false, bool inNoUpdateRefs = true, VDBLanguageContext *inLC = NULL);

/** generate span text from text & character styles 

CAUTION: you should use now SerializeDocument (especially if text has been parsed before with ParseDocument otherwise you would lost paragraph & document props)
*/
virtual void GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle = NULL);

/** get plain text from span text 
@remarks
	span references are evaluated on default
*/
virtual void GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly = true, bool inShowSpanRef = false, bool inNoUpdateRefs = false, VDBLanguageContext *inLC = NULL);

/** get styled text on the specified range */
virtual bool GetStyledText(	const VString& inSpanText, VString& outSpanText, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true);

/** replace styled text at the specified range with text 
@remarks
	replaced text will inherit only uniform style on replaced range
*/
virtual bool ReplaceStyledText(	VString& ioSpanText, const VString& inSpanTextOrPlainText, sLONG inStart = 0, sLONG inEnd = -1, bool inTextPlainText = false, bool inInheritUniformStyle = true, bool inSilentlyTrapParsingErrors = true);

/** replace styled text with span text reference on the specified range
	
	you should no more use or destroy passed inSpanRef even if method returns false
*/
virtual bool ReplaceAndOwnSpanRef( VString& ioSpanText, VDocSpanTextRef* inSpanRef, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true);

/** replace 4D expressions references with evaluated plain text & discard 4D expressions references on the passed range */
virtual bool FreezeExpressions( VDBLanguageContext *inLC, VString& ioSpanText, sLONG inStart = 0, sLONG inEnd = -1, bool inSilentlyTrapParsingErrors = true);

/** return the first span reference which intersects the passed range 
@return
	eDocSpanTextRef enum value or -1 if none ref found
*/
virtual sLONG GetReferenceAtRange(	VDBLanguageContext *inLC,
								    const VString& inSpanText, 
									const sLONG inStart = 0, const sLONG inEnd = -1, 
									sLONG *outFirstRefStart = NULL, sLONG *outFirstRefEnd = NULL, 
									VString *outFirstRefSource  = NULL, VString *outFirstRefValue = NULL
									);

//document parsing

/** parse text document 
@param inDocText
	span or xhtml text
@param inVersion
	span or xhtml version (might be overriden by -d4-version)
@param inDPI
	dpi used to convert from pixel to point
@param inShowSpanRef
	false (default): after call to VDocText::UpdateSpanRefs, plain text would contain span ref computed values if any (4D expression result, url link user text, etc...)
	true: after call to VDocText::UpdateSpanRefs, plain text contains span ref source if any (tokenized 4D expression, the actual url, etc...) 
@param inNoUpdateRefs
	false: replace plain text by source or computed span ref values after parsing (call VDocText::UpdateSpanRefs)
	true (default): span refs plain text is set to only one unbreakable space
@remarks
	on default, document format is assumed to be V13 compatible (but version might be modified if "-d4-version" CSS prop is present)
*/
virtual VDocText *ParseDocument( const VString& inDocText, const uLONG inVersion = kDOC_VERSION_SPAN4D, const uLONG inDPI = 72, bool inShowSpanRef = false, 
								 bool inNoUpdateRefs = true, bool inSilentlyTrapParsingErrors = true, VDBLanguageContext *inLC = NULL);

/** initialize VTextStyle from CSS declaration string */ 
virtual void ParseCSSToVTextStyle( VDBLanguageContext *inLC, VDocNode *ioDocNode, const VString& inCSSDeclarations, VTextStyle &outStyle, const VTextStyle *inStyleInherit = NULL);

virtual void _ParseCSSToVTreeTextStyle( VDBLanguageContext *inLC, VDocNode *ioDocNode, const VString& inCSSDeclarations, VTreeTextStyle *styles, const VTextStyle *inStyleInherit = NULL)
{
	bool isSpanRef = styles->GetData()->IsSpanRef();
	ParseCSSToVTextStyle( inLC, ioDocNode, inCSSDeclarations, *(styles->GetData()), inStyleInherit);
	if (!isSpanRef && styles->GetData()->IsSpanRef())
		styles->NotifySetSpanRef();
}

/** initialize VDocNode props from CSS declaration string */ 
virtual void ParseCSSToVDocNode( VDocNode *ioDocNode, const VString& inCSSDeclarations);

/** parse document CSS property */	
virtual bool ParsePropDoc( VDocText *ioDocText, const uLONG inProperty, const VString& inValue)
{
	return _ParsePropDoc( NULL, ioDocText, (eDocProperty)inProperty, inValue);  
}

/** parse paragraph CSS property */	
virtual bool ParsePropParagraph( VDocParagraph *ioDocPara, const uLONG inProperty, const VString& inValue)
{
	return _ParsePropParagraph( NULL, ioDocPara, (eDocProperty)inProperty, inValue);
}


/** parse common CSS property */	
virtual bool ParsePropCommon( VDocNode *ioDocNode, const uLONG inProperty, const VString& inValue)
{
	return _ParsePropCommon( NULL, ioDocNode, (eDocProperty)inProperty, inValue);
}

/** parse span CSS style */	
virtual bool ParsePropSpan( VDocNode *ioDocNode, const uLONG inProperty, const VString& inValue, VTextStyle &outStyle, VDBLanguageContext *inLC = NULL)
{
	return _ParsePropSpan( NULL, ioDocNode, (eDocProperty)inProperty, inValue, outStyle, NULL, NULL, NULL, inLC);
}

/** parse image HTML attributes (image 'src' and 'alt' attributes are HTML, not CSS) */	
bool ParseAttributeImage( VDocImage *ioDocImage, const uLONG inProperty, const VString& inValue);

bool ParseAttributeHRef( VDocNode *ioDocNode, const VString& inValue, VTextStyle& outStyle);


//document serializing

/** serialize document (formatting depending on version) 
@remarks
	//kDOC_VERSION_SPAN_1 format compatible with v13:
	//	- root span node is parsed for document, paragraph & character properties
	//  - span nodes which have a parent span node are parsed for character styles only
	//
	// for instance: <span style="zoom:150%;text-align:right;margin:0 60pt;font-family:'Arial'">mon texte <span style="font-weight:bold">bold</span></span>

	if (inDocParaDefault != NULL): only properties and character styles different from the passed paragraph properties & character styles will be serialized
*/
virtual void SerializeDocument( const VDocText *inDoc, VString& outDocText, const VDocParagraph *inDocParaDefault = NULL);

bool SerializeAttributeHRef( const VDocNode *ioDocNode, const VDocSpanTextRef *inSpanRef, VString& outText);

bool SerializeAttributesImage( const VDocNode *ioDocNode, const VDocSpanTextRef *inSpanRef, VString& outText);

static VSpanTextParser* Get();

static void	Init();
static void	DeInit();

protected:

void _BRToCR( VString& ioText);

void _CheckAndFixXMLText(const VString& inText, VString& outText, bool inParseSpanOnly = true);

/** parse document CSS property */	
bool _ParsePropDoc( VCSSLexParser *inLexParser, VDocText *ioDocText, const eDocProperty inProperty, const VString& inValue);

/** parse paragraph CSS property */	
bool _ParsePropParagraph( VCSSLexParser *inLexParser, VDocParagraph *ioDocPara, const eDocProperty inProperty, const VString& inValue);

/** parse common CSS property */	
bool _ParsePropCommon( VCSSLexParser *inLexParser, VDocNode *ioDocNode, const eDocProperty inProperty, const VString& inValue);

/** parse span CSS style */	
bool _ParsePropSpan(	VCSSLexParser *inLexParser, VDocNode *ioDocNode, const eDocProperty inProperty, const VString& inValue, VTextStyle &outStyle, 
						bool *outBackColorInherit = NULL, bool *outTextDecorationInherit = NULL, const VTextStyle *inStyleInherit = NULL,
						VDBLanguageContext *inLC = NULL);

/** get node font size (in TWIPS) */
sLONG _GetFontSize( const VDocNode *inDocNode);

/** get paragraph font size (in TWIPS) */
sLONG _GetParagraphFontSize( const VDocParagraph *inDocPara);

/** create paragraph default character styles */
VTextStyle *_CreateParagraphDefaultCharStyles( const VDocParagraph *inDocPara); 

/** convert TWIPS value to CSS dimension value (in point) */
void _TWIPSToCSSDimPoint( const sLONG inValueTWIPS, VString& outValueCSS);


/** serialize VTextStyle to CSS declaration string */ 
void _SerializeVTextStyleToCSS(  const VDocNode *inDocNode, const VTextStyle &inStyle, VString& outCSSDeclarations, const VTextStyle *inStyleInherit = NULL, bool inIgnoreColors = false, bool inIsFirst = true);

/** serialize VDocNode props to CSS declaration string */ 
void _SerializeVDocNodeToCSS(  const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault = NULL, bool inIsFirst = true);

/** serialize common CSS properties to CSS styles */
void _SerializePropsCommon( const VDocNode *inDocNode, VString& outCSSDeclarations, const VDocNode *inDocNodeDefault = NULL, bool inIsFirst = true);

void _OpenTagSpan(VString& ioText);
void _CloseTagSpan(VString& ioText, bool inAfterChildren = true);

void _OpenTagDoc(VString& ioText);
void _CloseTagDoc(VString& ioText, bool inAfterChildren = true);

void _OpenTagBody(VString& ioText);
void _CloseTagBody(VString& ioText, bool inAfterChildren = true);

void _OpenTagParagraph(VString& ioText);
void _CloseTagParagraph(VString& ioText, bool inAfterChildren = true);

void _TruncateTrailingEndOfLine(VString& ioText);

void _SerializeParagraphTextAndStyles(	const VDocNode *inDocNode, const VTreeTextStyle *inStyles, const VString& inPlainText, VString &ioText, 
										const VTextStyle *inStyleInherit = NULL, bool inSkipCharStyles = false);
 
private:
static	VSpanTextParser*	sInstance;

		VCriticalSection	fMutexParser;

		/** span reference custom delegate */
		IDocSpanTextRef*	fDelegateRef;

		bool				fIsOnlyPlainText;
};


END_TOOLBOX_NAMESPACE

#endif