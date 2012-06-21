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
#include "VSpanText.h"
#include "XMLSaxParser.h"
#include "XML/VXML.h"

BEGIN_TOOLBOX_NAMESPACE

#if VERSIONWIN
static sLONG syPerInch; 
#endif


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


typedef struct StyleProperties
{
	XBOX::VString*	fontName;	// on output, returns empty string if mixed
	Real*			fontSize;	// on output, returns -1 if mixed
	justificationStyle* just;	// on output, returns eNTE_mixed if mixed
	sLONG*			bold;		// on input 1 or 0, on output returns -1 if mixed
	sLONG*			italic;		// on input 1 or 0, on output returns -1 if mixed
	sLONG*			underline;	// on input 1 or 0, on output returns -1 if mixed
	sLONG*			strikeout;// on input 1 or 0, on output returns -1 if mixed
	RGBAColor*		foreColor;	// on output, returns alpha = 0 if mixed
	RGBAColor*		backColor;	// on output, returns alpha = 0 if mixed
} StyleProperties;

static void InitSystemPramFontSize();
static Real ConvertFromSystemFontSize(Real inFontSize);
static void JustificationToString(justificationStyle inValue, VString& outValue);
static void ToXMLCompatibleText(const XBOX::VString& inText, XBOX::VString& outText);
static void ColorToValue(const RGBAColor inColor, XBOX::VString& outValue);
static void AddStyleToListUniformStyle(const VTreeTextStyle* inUniformStyles, VTreeTextStyle* ioListUniformStyles, sLONG inBegin, sLONG inEnd);
static void GetPropertiesValue(const VTreeTextStyle*	inUniformStyles, StyleProperties& ioStyle);
static void GetPropertyValue(const VTreeTextStyle*	inUniformStyles, StyleProperties& ioStyle);

/** map of default style per element name */
typedef std::map<std::string,VTreeTextStyle *> MapOfStylePerHTMLElement;

/** map of HTML element names with break line ending */
typedef std::set<std::string> SetOfHTMLElementWithBreakLineEnding;

class VTaggedTextSAXHandler : public VObject, public IXMLHandler
{
public:
	/** tagged text SAX handler 
	@param ioTabUniformStyles
		in: must be NULL
		out: parsed styles
	@param ioText
		in: must be not NULL
		out: parsed text
	@param inParseSpanOnly
		true (default for backward compatibility): parse only <span> element(s)
		false: parse all HTML text in ioText
	*/
	VTaggedTextSAXHandler(VTreeTextStyle*& ioTabUniformStyles ,VString* ioText, bool inParseSpanOnly = true)
	{
		xbox_assert(ioTabUniformStyles == NULL);
		xbox_assert( ioText);
		fText=ioText;
		fStyles=&ioTabUniformStyles;
		fParseSpanOnly = inParseSpanOnly;
		if (!fParseSpanOnly)
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

				//html link
				style = RetainDefaultStyle("a");
				style->GetData()->SetUnderline(TRUE);
				style->GetData()->SetHasForeColor(true);
				style->GetData()->SetColor( kCSS_COLOR_BLUE);
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
	virtual ~VTaggedTextSAXHandler()
	{
		if (*fStyles && ((*fStyles)->IsUndefined(false)))
		{
			//if no styles defined or range is empty, remove styles

			(*fStyles)->Release();
			*fStyles = NULL;
		}
	}

	static void ClearShared()
	{
		MapOfStylePerHTMLElement::iterator it = fMapOfStyle.begin();
		for(;it != fMapOfStyle.end();it++)
			it->second->Release();
		fMapOfStyle.clear();
	}

	IXMLHandler*	StartElement(const VString& inElementName);
	void			SetAttribute(const VString& inName, const VString& inValue);
	void			SetText( const VString& inText);
	void			EndElement( const VString& inElementName);

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
	VString* fText;
	VTreeTextStyle**fStyles;
	bool fParseSpanOnly;

	/** map of default style per HTML element */
	static MapOfStylePerHTMLElement fMapOfStyle;

	/** set of HTML elements for which to add a ending end of line */
	static SetOfHTMLElementWithBreakLineEnding fSetOfBreakLineEnding;

	static VCriticalSection fMutexStyles;
};

/** map of default style per HTML element */
MapOfStylePerHTMLElement VTaggedTextSAXHandler::fMapOfStyle;

SetOfHTMLElementWithBreakLineEnding VTaggedTextSAXHandler::fSetOfBreakLineEnding;

VCriticalSection VTaggedTextSAXHandler::fMutexStyles;

class VSpanHandler : public VObject, public IXMLHandler
{
public:
	VSpanHandler(const VString &inElementName, VTreeTextStyle*	ioTabUniformStyles ,VString* ioText, bool inParseSpanOnly = true);
	~VSpanHandler();

	IXMLHandler*		StartElement(const VString& inElementName);
	void				EndElement( const VString& inElementName);
	void				SetAttribute(const VString& inName, const VString& inValue);
	void				SetText( const VString& inText);

	void				SetHTMLAttribute(const VString& inName, const VString& inValue);
	void				TraiteCSS(const VString& inAttribut, const VString& inValue);
private:
	void				SetFont(const VString& inValue);
	void				SetFontSize(const VString& inValue, float inDPI = 0.0f);
	void				SetJustification(const VString& inValue);
	void				SetColor(const VString& inValue);
	void				SetBackgroundColor(const VString& inValue);
	void				SetBold(const VString& inValue);
	void				SetItalic(const VString& inValue);
	void				SetUnderLine(const VString& inValue);

	bool				ToColor(const VString& inValue, RGBAColor& outColor);

	VString* fText;
	VTreeTextStyle*fStyles;
	VString fElementName;
	bool fParseSpanOnly;
};

IXMLHandler*	VTaggedTextSAXHandler::StartElement(const VString& inElementName)
{
	IXMLHandler* result = NULL; 

	if(inElementName == "SPAN")
	{
		if (*fStyles == NULL)
		{
			*fStyles = new VTreeTextStyle();
			result = new VSpanHandler(inElementName, *fStyles, fText, fParseSpanOnly);	
		}
		else
		{
			VTreeTextStyle* style = new VTreeTextStyle();
			(*fStyles)->AddChild(style);
			result = new VSpanHandler(inElementName, style, fText, fParseSpanOnly);	
			style->Release();
		}
	}
	else if (inElementName == "BR")
		*fText+="\r";
	else if (!fParseSpanOnly)
	{
		//parse other HTML element
		VTreeTextStyle *styleDefault = GetAndRetainDefaultStyle(inElementName);

		if (inElementName.EqualToString("q"))
			//start of quote
			*fText += "\"";

		if (*fStyles == NULL)
		{
			*fStyles = new VTreeTextStyle(styleDefault);
			result = new VSpanHandler(inElementName, *fStyles, fText, false);	
		}
		else
		{
			VTreeTextStyle* style = new VTreeTextStyle(styleDefault);
			(*fStyles)->AddChild(style);
			result = new VSpanHandler(inElementName, style, fText, false);	
			style->Release();
		}
		if (styleDefault)
			styleDefault->Release();
	}
	return result;
}


void VTaggedTextSAXHandler::EndElement( const VString& inElementName)
{
	VTextStyle *style = (*fStyles)->GetData();
	if (style)
	{
		sLONG vbegin, vend;
		style->GetRange(vbegin,vend);
		vend = fText->GetLength();
		style->SetRange(vbegin,vend);
	}
	if (!fParseSpanOnly)
	{
		//HTML parsing:

		if (inElementName.EqualToString("q"))
			//end of quote
			*fText += "\"";

		//add break lines for elements which need it
		if (VTaggedTextSAXHandler::NeedBreakLine(inElementName))
			*fText += "\r";
	}
}


void VTaggedTextSAXHandler::SetAttribute(const VString& inName, const VString& inValue)
{
	if( inName == "STYLE")
	{
		if (*fStyles == NULL)
		{
			*fStyles = new VTreeTextStyle();
			VTextStyle* style = new VTextStyle();
			style->SetRange(0,fText->GetLength());
			(*fStyles)->SetData(style);
		}

		VCSSParserInlineDeclarations cssParser;

		VSpanHandler *handler = new VSpanHandler(VString("body"), *fStyles, fText, fParseSpanOnly);	
		cssParser.Start(inValue);
		VString attribut, value;
		while(cssParser.GetNextAttribute(attribut, value))
		{
			handler->TraiteCSS(attribut, value);
		}
		handler->Release();
	} else if (!fParseSpanOnly)
	{
		if (*fStyles == NULL)
		{
			*fStyles = new VTreeTextStyle();
			VTextStyle* style = new VTextStyle();
			style->SetRange(0,fText->GetLength());
			(*fStyles)->SetData(style);
		}
		VSpanHandler *handler = new VSpanHandler(VString("body"), *fStyles, fText, fParseSpanOnly);	
		handler->SetHTMLAttribute( inName, inValue);
		handler->Release();
	}
}


void VTaggedTextSAXHandler::SetText( const VString& inText)
{
	*fText+=inText;
}

//
// VSpanHandler
//

VSpanHandler::VSpanHandler(const VString& inElementName, VTreeTextStyle*	ioTabUniformStyles, VString* ioText, bool inParseSpanOnly)
{	
	fElementName = inElementName;
	fText = ioText; 
	xbox_assert(ioTabUniformStyles);
	fStyles = ioTabUniformStyles;
	fStyles->Retain();
	VTextStyle *style = fStyles->GetData();
	if (!style)
	{
		style = new VTextStyle();
		fStyles->SetData( style);
	}
	style->SetRange(fText->GetLength(),fText->GetLength());
	fParseSpanOnly = inParseSpanOnly;
}
	
VSpanHandler::~VSpanHandler()
{
	if (fStyles && fStyles->IsUndefined(false))
	{
		//range is empty or style not defined (can happen while parsing node without any text or without styles): remove this useless style from parent if any
		VTreeTextStyle *parent = fStyles->GetParent();
		if (parent)
		{
			for (int i = 1; i <= parent->GetChildCount(); i++)
				if (parent->GetNthChild(i) == fStyles)
				{
					parent->RemoveChildAt(i);
					break;
				}
		}
	}
	fStyles->Release();
}

IXMLHandler* VSpanHandler::StartElement(const VString& inElementName)
{
	IXMLHandler* result = NULL; 

	if(inElementName == "SPAN")
	{
		VTreeTextStyle* style = new VTreeTextStyle();
		fStyles->AddChild(style);
		result = new VSpanHandler(inElementName, style,fText,fParseSpanOnly);	
		style->Release();
	}
	else if (inElementName == "BR")
		*fText+="\r";
	else if (!fParseSpanOnly)
	{
		//parse other HTML element
		VTreeTextStyle *styleDefault = VTaggedTextSAXHandler::GetAndRetainDefaultStyle(inElementName);

		if (inElementName.EqualToString("q"))
			//start of quote
			*fText += "\"";

		VTreeTextStyle* style = new VTreeTextStyle(styleDefault);
		fStyles->AddChild(style);
		result = new VSpanHandler(inElementName, style, fText,false);	
		style->Release();
		
		if (styleDefault)
			styleDefault->Release();
	}
	return result;
}

void VSpanHandler::EndElement( const VString& inElementName)
{
	xbox_assert(inElementName == fElementName);

	VTextStyle *style = fStyles->GetData();
	if (style)
	{
		sLONG vbegin, vend;
		style->GetRange(vbegin,vend);
		vend = fText->GetLength();
		style->SetRange(vbegin,vend);

	}
	if (!fParseSpanOnly)
	{
		//HTML parsing:

		if (inElementName.EqualToString("q"))
			//end of quote
			*fText += "\"";

		//add break lines for elements which need it
		if (VTaggedTextSAXHandler::NeedBreakLine(inElementName))
			*fText += "\r";
	}
}

void VSpanHandler::SetAttribute(const VString& inName, const VString& inValue)
{
	if( inName == "STYLE")
	{
		VCSSParserInlineDeclarations cssParser;

		cssParser.Start(inValue);
		VString attribut, value;
		while(cssParser.GetNextAttribute(attribut, value))
		{
			TraiteCSS(attribut, value);
		}
	} else if (!fParseSpanOnly)
		SetHTMLAttribute( inName, inValue);
}

void VSpanHandler::SetHTMLAttribute(const VString& inName, const VString& inValue)
{
	//here we need to parse only mandatory font attributes
	if (inName.EqualToString("face"))
		SetFont(inValue);
	else if (inName.EqualToString("color"))
		SetColor( inValue);
	else if (inName.EqualToString("bgcolor"))
		SetBackgroundColor( inValue);
	else if (inName.EqualToString("size"))
	{
		//HTML font size is a number from 1 to 7; default is 3
		sLONG size = inValue.GetLong();
		if (size >= 1 && size <= 7)
		{
			size = size*12/3; //convert to point
			VString fontSize;
			fontSize.FromLong(size);
			SetFontSize( fontSize, 72.0f);
		}
	}
}

void VSpanHandler::SetText( const VString& inText)
{
	*fText+=inText;
}

void VSpanHandler::TraiteCSS(const VString& inAttribut, const VString& inValue)
{
	if(inAttribut == MS_FONT_NAME)
	{
		SetFont(inValue);	
	}
	else if(inAttribut == MS_FONT_SIZE)
	{
		SetFontSize(inValue);
	}
	else if(inAttribut == MS_COLOR)
	{
		SetColor(inValue);
	}
	else if(inAttribut == MS_ITALIC)
	{
		SetItalic(inValue);
	}
	else if(inAttribut == MS_BACKGROUND)
	{
		SetBackgroundColor(inValue);
	}
	else if(inAttribut == MS_BOLD)
	{
		SetBold(inValue);
	}
	else if(inAttribut == MS_UNDERLINE)
	{
		SetUnderLine(inValue);
	}
	else if(inAttribut == MS_JUSTIFICATION)
	{
		SetJustification(inValue);
	}
}

void VSpanHandler::SetFont(const VString& inValue)
{
	VTextStyle* TheStyle = fStyles->GetData();
	if(TheStyle)
	{
		VString fontname(inValue);
		if(fontname.BeginsWith("'") || fontname.BeginsWith("\""))
			fontname.Remove(1,1);
		if(fontname.EndsWith("'") || fontname.EndsWith("\""))
			fontname.Remove(fontname.GetLength(),1);
		TheStyle->SetFontName(fontname);
	}
}

void VSpanHandler::SetFontSize(const VString& inValue, float inDPI)
{
	Real size = 0;
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;
	if(inValue.EndsWith("pt", true))
	{
		VString textsize = inValue;
		textsize.Truncate( inValue.GetLength()-2);
		size = textsize.GetReal();
		//we need convert from format dpi to 72 dpi
#if VERSIONWIN
		TheStyle->SetFontSize(floor(size*(inDPI ? inDPI : VSpanTextParser::Get()->GetDPI())/72.0f+0.5f));
#else
		TheStyle->SetFontSize(size);
#endif
	}
}

void VSpanHandler::SetJustification(const VString& inValue)
{
	sLONG size = 0;
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;
	if(inValue == "left")
		TheStyle->SetJustification(JST_Left);
	else if(inValue == "right")
		TheStyle->SetJustification(JST_Right);
	else if(inValue == "center")
		TheStyle->SetJustification(JST_Center);
	else if(inValue == "justify")
		TheStyle->SetJustification(JST_Justify);
	else
	{
		assert(false);
		TheStyle->SetJustification(JST_Left);
	}
}

bool VSpanHandler::ToColor(const VString& inValue, RGBAColor& outColor)
{
	bool result = false;
	uLONG colorARGB = 0;

	result = VCSSUtil::ParseColor(inValue, &colorARGB);
	if(result)
	{
		outColor = colorARGB;
	}
	
	return result;
}

void VSpanHandler::SetColor(const VString& inValue)
{
	RGBAColor color;
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;

	bool doit = ToColor(inValue, color);
	if(doit)
	{
		TheStyle->SetColor(color);
		TheStyle->SetHasForeColor(true);
	}
}

void VSpanHandler::SetBackgroundColor(const VString& inValue)
{
	RGBAColor color;
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;

	bool doit = ToColor(inValue, color);
	if(doit)
	{
		TheStyle->SetBackGroundColor(color);
		TheStyle->SetTransparent(false);
	}
}

void VSpanHandler::SetBold(const VString& inValue)
{
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;

	if(inValue == "bold")
		TheStyle->SetBold(TRUE);
	else if(inValue == "normal")
		TheStyle->SetBold(FALSE);
}

void VSpanHandler::SetItalic(const VString& inValue)
{
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;

	if(inValue == "italic" || inValue == "oblique")
		TheStyle->SetItalic(TRUE);
	else if(inValue == "normal")
		TheStyle->SetItalic(FALSE);
}

void VSpanHandler::SetUnderLine(const VString& inValue)
{
	VTextStyle* TheStyle = fStyles->GetData();
	if(!TheStyle)
		return;

	std::vector<XBOX::VString> values;
	inValue.GetSubStrings( ' ', values);

	for( std::vector<XBOX::VString>::const_iterator i = values.begin() ; i != values.end() ; ++i)
	{
		if(*i  == "underline")
			TheStyle->SetUnderline(TRUE);
		else if(*i == "line-through")
			TheStyle->SetStrikeout(TRUE);
		else if(*i == "none")
		{
			TheStyle->SetUnderline(FALSE);
			TheStyle->SetStrikeout(FALSE);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////
static void InitSystemPramFontSize()
{
	#if VERSIONWIN
	if(syPerInch == 0)
	{
		HDC hdc = GetDC(NULL);
		syPerInch =	GetDeviceCaps(hdc, LOGPIXELSY);
	}
	#endif
}

static Real ConvertFromSystemFontSize(Real inFontSize)
{
	InitSystemPramFontSize();
	Real result = inFontSize;
	#if VERSIONWIN
	result = (inFontSize* (Real)syPerInch)/72.0;
	#endif

	return result;
}

	
void JustificationToString(justificationStyle inValue, VString& outValue)
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

void ToXMLCompatibleText(const XBOX::VString& inText, XBOX::VString& outText)
{
	outText = inText;
	sLONG len = inText.GetLength();
	outText.Exchange( CVSTR("&"), CVSTR("&amp;"), 1, len);
	len = outText.GetLength();
	outText.Exchange( CVSTR("<"), CVSTR("&lt;"), 1, len);
	len = outText.GetLength();
	outText.Exchange( CVSTR(">"), CVSTR("&gt;"), 1, len);	
	outText.ConvertCarriageReturns(XBOX::eCRM_CR);
	len = outText.GetLength();
	outText.Exchange( CVSTR("\r"), CVSTR("<BR/>"), 1, len);
	len = outText.GetLength();
	outText.Exchange( CVSTR("\13"), CVSTR("<BR/>"), 1, len);//[MI] le 28/12/2010 ACI0069253
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

void AddStyleToListUniformStyle(const VTreeTextStyle* inUniformStyles, VTreeTextStyle* ioListUniformStyles, sLONG inBegin, sLONG inEnd)
{
	if(inUniformStyles->GetParent())
	{
		StyleProperties vStyle;
		sLONG isbold, isunderline, isitalic, istrikeout;
		Real vfontsize = -1;
		RGBAColor forecolor;
		VString vfontname;
		bool hasForeColor, istransparent;
		justificationStyle vjust = JST_Left;
		bool doit = false;
		
		memset(&vStyle,0, sizeof(StyleProperties));
		vStyle.bold = &isbold;
		vStyle.italic = &isitalic;
		vStyle.underline = &isunderline;
		vStyle.fontSize = &vfontsize;
		vStyle.fontName = &vfontname;
		vStyle.just = &vjust;
		vStyle.foreColor = &forecolor;
		vStyle.strikeout = &istrikeout;
		
		GetPropertiesValue(inUniformStyles, vStyle);
		
		VTextStyle* uniformStyle = new VTextStyle();
		uniformStyle->SetRange(inBegin, inEnd);
		if(vfontname.GetLength() > 0)
		{
			uniformStyle->SetFontName(vfontname);
		}
		
		if(vfontsize != -1)
		{
			uniformStyle->SetFontSize(vfontsize);
		}
		
		if(isbold != -1)
		{
			uniformStyle->SetBold(isbold);
		}
		
		if(isitalic != -1)
		{
			uniformStyle->SetItalic(isitalic);
		}
		
		if(isunderline != -1)
		{
			uniformStyle->SetUnderline(isunderline);
		}
		
		if(istrikeout != -1)
		{
			uniformStyle->SetStrikeout(istrikeout);
		}
		
		uniformStyle->SetHasForeColor(true);
		uniformStyle->SetColor(forecolor);
		
		if(vjust != JST_Notset)
		{
			uniformStyle->SetJustification(vjust);
		}
		
		//if(doit)
		{
			VTreeTextStyle* child = new VTreeTextStyle(uniformStyle);	
			//ioListUniformStyles->AddChild(child);
			//[MI] le 24/11/2011 : les element de la liste doivent etre rang par ordre croissant de debut de selection
			sLONG vbegin, vend; 
			sLONG vcount = ioListUniformStyles->GetChildCount();
			sLONG index = vcount;
			child->GetData()->GetRange(vbegin,vend);
			//recherche dichotomique de l index ou le child  doit etre inser
			if(vcount)
			{
				sLONG bornsup = vcount;
				sLONG borninf = 1;
				sLONG b,e,b1,e1;
				index = bornsup;
				VTreeTextStyle* thechild = ioListUniformStyles->GetNthChild(bornsup);
				thechild->GetData()->GetRange(b,e);
				thechild = ioListUniformStyles->GetNthChild(borninf);
				thechild->GetData()->GetRange(b1,e1);
				if(vbegin >=  b)
					index = bornsup;
				else if(vbegin <  b1)
					index = 0;
				else
				{
					//fixed ACI0073686: c'était n'importe quoi cette recherche dicho...??
					bool found = false;
					while(!found && (borninf<= bornsup))
					{
						index = (borninf+bornsup)/2;

						thechild = ioListUniformStyles->GetNthChild(index);
						thechild->GetData()->GetRange(b,e);
						if(vbegin >=  b)
							borninf = index; //ensure we insert after index
						else
							bornsup = index-1; //ensure we insert before index
						if(bornsup == (borninf+1) || bornsup == borninf)
						{
							index = borninf;
							found = true;
						}
					}
				}
			}
			ioListUniformStyles->InsertChild(child,index+1);	
		}
	}
}

void GetPropertyValue(const VTreeTextStyle*	inUniformStyles, StyleProperties& ioStyle)
{
	if(inUniformStyles)
	{
		if(ioStyle.fontName)
		{
			VString vfont;
			vfont = inUniformStyles->GetData()->GetFontName();
			if(vfont == "")
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.fontName) = vfont;
			}
			else
				*(ioStyle.fontName) = vfont;
		}
		else if(ioStyle.fontSize)
		{
			Real vfontsize;
			vfontsize = inUniformStyles->GetData()->GetFontSize();
			if(vfontsize == -1)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.fontSize) = vfontsize;
			}
			else
				*(ioStyle.fontSize) = vfontsize;
		}
		else if(ioStyle.bold)
		{
			sLONG vbold;
			vbold = inUniformStyles->GetData()->GetBold();
			if(vbold == -1)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.bold) = vbold;
			}
			else
				*(ioStyle.bold) = vbold;
		}
		else if(ioStyle.italic)
		{
			sLONG vitalic;
			vitalic = inUniformStyles->GetData()->GetItalic();
			if(vitalic == -1)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.italic) = vitalic;
			}
			else
				*(ioStyle.italic) = vitalic;
		}
		else if(ioStyle.underline)
		{
			sLONG vunderline;
			vunderline = inUniformStyles->GetData()->GetUnderline();
			if(vunderline == -1)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.underline) = vunderline;
			}
			else
				*(ioStyle.underline) = vunderline;
		}
		else if(ioStyle.just)
		{
			justificationStyle vjust;
			vjust = inUniformStyles->GetData()->GetJustification();
			if(vjust == JST_Notset)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.just) = vjust;
			}
			else
				*(ioStyle.just) = vjust;
		}
		else if(ioStyle.strikeout)
		{
			sLONG vstrikeout;
			vstrikeout = inUniformStyles->GetData()->GetStrikeout();
			if(vstrikeout == -1)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.strikeout) = vstrikeout;
			}
			else
				*(ioStyle.strikeout) = vstrikeout;
		}
		else if(ioStyle.foreColor)
		{
			RGBAColor vcolor;
			bool hascolor = inUniformStyles->GetData()->GetHasForeColor();
			
			if(!hascolor)
			{
				VTreeTextStyle* parent = inUniformStyles->GetParent();
				if(parent)
					GetPropertyValue(parent,ioStyle);
				else
					*(ioStyle.foreColor) = inUniformStyles->GetData()->GetColor();
			}
			else
			{
				*(ioStyle.foreColor) = inUniformStyles->GetData()->GetColor();
			}
		}

	}
}

void GetPropertiesValue(const VTreeTextStyle*	inUniformStyles, StyleProperties& ioStyle)
{
	if(inUniformStyles)
	{	
		if(ioStyle.fontName)
		{
			VString vfont;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.fontName = &vfont;
			GetPropertyValue(inUniformStyles, vstyle);

			 *(ioStyle.fontName) = vfont;
		}

		if(ioStyle.fontSize)
		{
			Real vfontsize;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.fontSize = &vfontsize;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.fontSize) = vfontsize;
		}

		if(ioStyle.bold)
		{
			sLONG vbold;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.bold = &vbold;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.bold) = vbold;
		}

		if(ioStyle.italic)
		{
			sLONG vitalic;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.italic = &vitalic;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.italic) = vitalic;
		}

		if(ioStyle.underline)
		{
			sLONG vunderline;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.underline = &vunderline;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.underline) = vunderline;
		}

		if(ioStyle.strikeout)
		{
			sLONG vstrikeout;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.strikeout = &vstrikeout;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.strikeout) = vstrikeout;
		}

		if(ioStyle.foreColor)
		{
			RGBAColor vforeColor;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.foreColor = &vforeColor;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.foreColor) = vforeColor;
		}

		if(ioStyle.just)
		{
			justificationStyle vjust;
			StyleProperties vstyle;
			memset(&vstyle,0,sizeof(StyleProperties));
			vstyle.just = &vjust;
			GetPropertyValue(inUniformStyles, vstyle);

			*(ioStyle.just) = vjust;
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////////
// class VSpanTextParser
//////////////////////////////////////////////////////////////////////////////////////

VSpanTextParser *VSpanTextParser::sInstance = NULL;

#if VERSIONWIN
float VSpanTextParser::GetDPI() const
{
	//screen dpi for compatibility with v12
	//(in v14, it will be bound to a database parameter to allow res independent font size)
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

/** parse span text element or XHTML fragment 
@param inTaggedText
	tagged text to parse (span element text only if inParseSpanOnly == true, XHTML fragment otherwise)
@param outStyles
	parsed styles
@param outPlainText
	parsed plain text
@param inParseSpanOnly
	true (default): only <span> element(s) with CSS styles are parsed
	false: all XHTML text fragment is parsed (parse also mandatory HTML text styles)
*/
void VSpanTextParser::ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly)
{
	outPlainText = "";
	if(outStyles)
		outStyles->Release();
	outStyles = NULL;
	if (inTaggedText.IsEmpty())
		return;
	//fixed ACI0076343: for compatibility with older bases which do not have support for multistyle var or field
	//					we need to convert to xml text the text which is not bracketed with SPAN 
	//					otherwise parser would fail
	VString vtext;
	sLONG posStartSPAN = 1; //pos of first SPAN tag
	sLONG posEndSPAN = inTaggedText.GetLength()+1; //pos of character following ending SPAN tag
	if (inParseSpanOnly)
	{
		//search for opening SPAN tag
		posStartSPAN = inTaggedText.Find("<SPAN", 1, true);
		if (!posStartSPAN)
		{
			//convert full text to xml
			inTaggedText.GetXMLString( vtext, XSO_Default); 
			vtext = VString("<SPAN>")+vtext+"</SPAN>";
		}
		else 
		{
			VString before, after;
			if (posStartSPAN > 1)
			{
				//convert to XML text preceeding first SPAN 
				VString temp;
				inTaggedText.GetSubString( 1, posStartSPAN-1, temp);
				temp.GetXMLString( before, XSO_Default);
			}
			//search for ending SPAN tag
			const UniChar *c = inTaggedText.GetCPointer()+inTaggedText.GetLength()-1;
			sLONG pos = inTaggedText.GetLength()-1;
			VString spanEndTag = "</SPAN>";
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
					if (memcmp( (void*)(c+1-spanEndTag.GetLength()), spanEndTag.GetCPointer(), spanEndTag.GetLength()*sizeof(UniChar)) == 0)
					{
						posEndSPAN = pos+2;
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
			if (posEndSPAN <= inTaggedText.GetLength())
			{
				//convert to XML text following ending SPAN tag
				VString temp;
				inTaggedText.GetSubString( posEndSPAN, inTaggedText.GetLength()-posEndSPAN+1, temp);
				temp.GetXMLString( after, XSO_Default);
			}
			if (!before.IsEmpty() || !after.IsEmpty())
			{
				inTaggedText.GetSubString( posStartSPAN, posEndSPAN-posStartSPAN, vtext);
				vtext = VString("<SPAN>")+before+vtext+after+"</SPAN>";
			}
			else
				vtext = VString("<SPAN>")+inTaggedText+"</SPAN>";
		}
	}
	else
		vtext = inTaggedText;

	vtext.ExchangeAll(0x0B,0x0D);//[MI] le 28/12/2010 ACI0069253
#if VERSIONWIN
	vtext.ConvertCarriageReturns(eCRM_CR); //important here on Windows before parsing because styles are assumed to use one character size for line ending on any platform 
										   //and so pair of 0x0D 0x0A should be treated as a single 0x0D character
#endif											
	VTaggedTextSAXHandler vsaxhandler(outStyles, &outPlainText, inParseSpanOnly);
	VXMLParser vSAXParser;

	vSAXParser.Parse(vtext, &vsaxhandler, XML_ValidateNever);

#if VERSIONWIN
	outPlainText.ConvertCarriageReturns(eCRM_CR); //we need to convert to CR (on any platform but Linux here) because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)
#elif VERSIONMAC
	outPlainText.ConvertCarriageReturns(eCRM_CR); //we need to convert to CR (on any platform but Linux here) because xerces parser has converted all line endings to LF (cf W3C XML line ending uniformization while parsing)
#endif
}

void VSpanTextParser::GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle)
{
	sLONG begin, end;
	bool vhasForecolore, vistransparent;
	VString fontName, defaultfontName;
	sLONG vbold,vitalic,vunderline, vstrikeout, vdefaultbold,vdefaultitalic,vdefaultunderline,vdefaultstrikeout;
	Real fontSize, defaultfontSize;
	RGBAColor vforecolor, vdefaultforecolor, vbackcolor, vdefaultbackcolor;
	justificationStyle justification, defaultjustification;
	VTextStyle* uniformStyle = inStyles.GetData();
	
	justification = defaultjustification = JST_Left;

	uniformStyle->GetRange(begin, end);
	fontName = uniformStyle->GetFontName();
	fontSize = uniformStyle->GetFontSize();
	vbold = uniformStyle->GetBold();
	vitalic = uniformStyle->GetItalic();
	vunderline = uniformStyle->GetUnderline();
	vstrikeout = uniformStyle->GetStrikeout();
	vhasForecolore = uniformStyle->GetHasForeColor();
	vistransparent = uniformStyle->GetTransparent();
	vforecolor = uniformStyle->GetColor();
	vbackcolor = uniformStyle->GetBackGroundColor();
	justification = uniformStyle->GetJustification();
	VTreeTextStyle* parenttree = inStyles.GetParent();
	bool hasDefaultForeColor = false;
	bool hasDefaultBackColor = false;

	if(inDefaultStyle)
	{
		defaultfontName = inDefaultStyle->GetFontName();
		defaultfontSize = inDefaultStyle->GetFontSize();
		defaultjustification = inDefaultStyle->GetJustification();
		vdefaultbold = inDefaultStyle->GetBold();
		vdefaultitalic = inDefaultStyle->GetItalic();
		vdefaultunderline = inDefaultStyle->GetUnderline();
		vdefaultstrikeout = inDefaultStyle->GetStrikeout();
		hasDefaultForeColor = inDefaultStyle->GetHasForeColor();
		hasDefaultBackColor = !inDefaultStyle->GetTransparent();
		vdefaultforecolor = inDefaultStyle->GetColor();
		vdefaultbackcolor = inDefaultStyle->GetBackGroundColor();
	}
	else
	{
		defaultfontName = "";
		defaultfontSize = -1;
		vdefaultbold = 0;
		vdefaultitalic = 0;
		vdefaultunderline = 0;
		vdefaultstrikeout = 0;
		defaultjustification = JST_Notset;
	}

	//as we inherit styles from parent, -1 or default value should be assumed equal (otherwise comparison would trigger a useless tag)
	if (vdefaultbold < 0)
		vdefaultbold = 0;
	if (vdefaultitalic < 0)
		vdefaultitalic = 0;
	if (vdefaultunderline < 0)
		vdefaultunderline = 0;
	if (vdefaultstrikeout < 0)
		vdefaultstrikeout = 0;
	if (defaultjustification < 0)
		defaultjustification = JST_Default;

	bool addfontag, addfonsizetag, addboldtag, additalictag, addunderlinetag, addstrikeouttag, addforecolortag, addbackcolortag, addjustificationtag;
	addfontag = addfonsizetag = addboldtag = additalictag = addunderlinetag = addstrikeouttag =  addforecolortag = addbackcolortag = addjustificationtag = false;

	//we pass inherited default style to children to avoid to redefine style already defined by parent node:
	//so current inDefaultStyle always inherits parent styles
	VTextStyle *defaultStyleInherit = inStyles.GetChildCount() > 0 ? new VTextStyle( inDefaultStyle) : NULL;
	if( fontName.GetLength() > 0 && fontName != defaultfontName)
	{
		addfontag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetFontName( fontName);
	}
	if( fontSize != -1 && fontSize != defaultfontSize)
	{
		addfonsizetag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetFontSize( fontSize);
	}
	if( justification != JST_Notset && justification != defaultjustification)
	{
		addjustificationtag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetJustification( justification);
	}
	if( vbold != UNDEFINED_STYLE && vbold != vdefaultbold)
	{
		addboldtag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetBold( vbold);
	}

	if( vitalic != UNDEFINED_STYLE  && vitalic != vdefaultitalic)
	{
		additalictag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetItalic( vitalic);
	}

	if( vunderline != UNDEFINED_STYLE && vunderline != vdefaultunderline)
	{
		addunderlinetag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetUnderline( vunderline);
	}

	if( vstrikeout != UNDEFINED_STYLE && vstrikeout != vdefaultstrikeout)
	{
		addstrikeouttag = true;
		if (defaultStyleInherit)
			defaultStyleInherit->SetStrikeout( vstrikeout);
	}

	if( vhasForecolore && ((!hasDefaultForeColor) || vforecolor != vdefaultforecolor))
	{
		addforecolortag = true;
		if (defaultStyleInherit)
		{
			defaultStyleInherit->SetHasForeColor( vhasForecolore);
			defaultStyleInherit->SetColor( vforecolor);
		}
	}

	if( !vistransparent && ((!hasDefaultBackColor) || vbackcolor != vdefaultbackcolor))
	{
		addbackcolortag = true;
		if (defaultStyleInherit)
		{
			defaultStyleInherit->SetTransparent( false);
			defaultStyleInherit->SetBackGroundColor( vbackcolor);
		}
	}

	bool addtag = addfontag || addfonsizetag || addboldtag || additalictag || addunderlinetag || addstrikeouttag || addforecolortag || addbackcolortag || addjustificationtag;

	bool isInitialEmpty = outTaggedText.IsEmpty();
	if(addtag)
	{
		bool addcoma = false;
		outTaggedText += "<SPAN STYLE=\"";
	
		if(addfontag)
		{
			outTaggedText += MS_FONT_NAME;
			outTaggedText += ":'";
			outTaggedText += fontName + "'";
			addcoma = true;
		}

		if(addfonsizetag)
		{
			VString strFontSize;
			//we need to convert from 72dpi font size to the desired dpi (screen dpi for v12 compatibility)
#if VERSIONWIN
			strFontSize.FromReal(floor(fontSize*72.0f/VSpanTextParser::Get()->GetDPI()+0.5f));
#else
			strFontSize.FromReal(fontSize);
#endif
			if(addcoma)
				outTaggedText+=";";
			outTaggedText += MS_FONT_SIZE ;
			outTaggedText += ":";
			outTaggedText += strFontSize;
			outTaggedText += "pt";
			addcoma = true;
		}
		
		if(addjustificationtag)
		{
			VString value;
			JustificationToString(justification,value);
			if(addcoma)
				outTaggedText+=";";
			outTaggedText += MS_JUSTIFICATION;
			outTaggedText += ":";
			outTaggedText += value;
			addcoma = true;
		}

		if(addboldtag)
		{
			VString value;
			if(addcoma)
				outTaggedText+=";";
			vbold ? value = "bold" : value = "normal";
			outTaggedText += MS_BOLD ;
			outTaggedText += ":" + value;
			addcoma = true;
		}

		if(additalictag)
		{
			VString value;
			if(addcoma)
				outTaggedText+=";";
			vitalic ? value = "italic" : value = "normal";
			outTaggedText += MS_ITALIC ;
			outTaggedText += ":"+ value;
			addcoma = true;
		}

		if(addunderlinetag && addstrikeouttag)
		{
			VString value;
			if(addcoma)
				outTaggedText+=";";
			if(!vunderline && !vstrikeout)
				value = "none";
			if(vunderline && vstrikeout)
				value = "underline line-through";
			else if(vunderline)
				value = "underline";
			else if(vstrikeout)
				value = "line-through";				
			outTaggedText += MS_UNDERLINE ;
			outTaggedText += ":"+ value;
			addcoma = true;
		}
		else
		{
			if(addunderlinetag)
			{
				VString value;
				if(addcoma)
					outTaggedText+=";";
				vunderline ? value = "underline" : value = "none";
				outTaggedText += MS_UNDERLINE ;
				outTaggedText += ":"+ value;
				addcoma = true;
			}
					
			if(addstrikeouttag)
			{
				VString value;
				if(addcoma)
					outTaggedText+=";";
				vstrikeout ? value = "line-through" : value = "none";
				outTaggedText += MS_UNDERLINE ;
				outTaggedText += ":"+ value;
				addcoma = true;
			}
		}

		if(addforecolortag)
		{
			VString value;
			if(addcoma)
				outTaggedText+=";";
			ColorToValue(vforecolor,value);
			outTaggedText += MS_COLOR;
			outTaggedText += ":"+ value;
			addcoma = true;
		}

		if(addbackcolortag)
		{
			VString value;
			if(addcoma)
				outTaggedText+=";";
			ColorToValue(vbackcolor,value);
			outTaggedText += MS_BACKGROUND;
			outTaggedText += ":"+ value;
			addcoma = true;
		}

		outTaggedText +="\">";				

	}

	
	VString text;
	VString tmp;

	sLONG count = inStyles.GetChildCount();

	//prevent assert in GetSubString
	if (end > inPlainText.GetLength())
		end = inPlainText.GetLength();
	if (begin > inPlainText.GetLength())
		begin = inPlainText.GetLength();

	if(count == 0)
	{
		if(end>begin)
		{
			inPlainText.GetSubString(begin+1, end-begin, text);
			ToXMLCompatibleText(text, tmp);
			outTaggedText += tmp;
		}
	}
	else
	{
		sLONG start = begin;
		for(sLONG i = 1; i <= count; i++)
		{
			VTreeTextStyle* child = inStyles.GetNthChild(i);
			sLONG b,e;
			child->GetData()->GetRange(b,e);

			//prevent assert in GetSubString
			if (e > inPlainText.GetLength())
				e = inPlainText.GetLength();
			if (b > inPlainText.GetLength())
				b = inPlainText.GetLength();

			//if(start < b-1)
			if(start < b)
			{
				inPlainText.GetSubString(start+1, b-start, text);
				ToXMLCompatibleText(text, tmp);
				outTaggedText += tmp;
			}
			
			GenerateSpanText(*child, inPlainText, outTaggedText, defaultStyleInherit);
			start = e;
		}
		
		if(start < end)
		{
			inPlainText.GetSubString(start+1, end-start, text);
			ToXMLCompatibleText(text, tmp);
			outTaggedText += tmp;
		}

	}

	if (addtag)
		outTaggedText +="</SPAN>";
	//JQ 24/12/2012: ensure tagged text is bracketed with SPAN tag
	else if (isInitialEmpty)
	{
		if (!outTaggedText.BeginsWith(CVSTR("<SPAN"), true))
			outTaggedText = CVSTR("<SPAN>")+outTaggedText+CVSTR("</SPAN>");
	}

	if (defaultStyleInherit)
		delete defaultStyleInherit;
}

void VSpanTextParser::GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly)
{
	VTreeTextStyle*	vUniformStyles = NULL;
	ParseSpanText(inTaggedText, vUniformStyles, outPlainText, inParseSpanOnly);
	if(vUniformStyles)
		vUniformStyles->Release();
}

void VSpanTextParser::ConverUniformStyleToList(const VTreeTextStyle* inUniformStyles, VTreeTextStyle* ioListUniformStyles)
{
	sLONG begin, end;
	inUniformStyles->GetData()->GetRange(begin, end);
	sLONG count = inUniformStyles->GetChildCount();
	
	if(count > 0)
	{
		bool doit = true;
		sLONG parsebegin = begin;
		for(sLONG i = 1; i <= count; i++)
		{
			sLONG beginchild, endchild;
			VTreeTextStyle* child = inUniformStyles->GetNthChild(i);
			
			child->GetData()->GetRange(beginchild, endchild);
			if(parsebegin < beginchild)
			{
				AddStyleToListUniformStyle(inUniformStyles, ioListUniformStyles, parsebegin, beginchild);
			}
			
			ConverUniformStyleToList(child, ioListUniformStyles);
			parsebegin = endchild;
		}
		
		if(parsebegin < end)
		{
			AddStyleToListUniformStyle(inUniformStyles, ioListUniformStyles, parsebegin, end);	
		}
	}
	else
	{		
		AddStyleToListUniformStyle(inUniformStyles, ioListUniformStyles, begin, end);
	}
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

	VTaggedTextSAXHandler::ClearShared();
}

END_TOOLBOX_NAMESPACE
