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

#define MS_FONT_NAME		"font-family"
#define MS_FONT_SIZE		"font-size"
#define MS_COLOR			"color"
#define MS_ITALIC			"font-style"
#define MS_BACKGROUND		"background-color"
#define MS_BOLD				"font-weight"
#define MS_UNDERLINE		"text-decoration"
#define MS_JUSTIFICATION	"text-align"

class XTOOLBOX_API VSpanTextParser : public VObject , public ISpanTextParser
{
public:

#if VERSIONWIN
virtual float GetDPI() const;
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
virtual void ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText, bool inParseSpanOnly = true);
virtual void GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle = NULL);
virtual void GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText, bool inParseSpanOnly = true);
virtual void ConverUniformStyleToList(const VTreeTextStyle* inUniformStyles, VTreeTextStyle* ioListUniformStyles);

static VSpanTextParser* Get();

static void	Init();
static void	DeInit();

private:
static VSpanTextParser *sInstance;
};


//Des fonctions qui transforment du XML en VTreeUniformStyle + VString et inversement.
//void XTOOLBOX_API ParseSpanText( const VString& inTaggedText, VTreeTextStyle*& outStyles, VString& outPlainText);
//void XTOOLBOX_API GenerateSpanText( const VTreeTextStyle& inStyles, const VString& inPlainText, VString& outTaggedText, VTextStyle* inDefaultStyle = NULL);
//void XTOOLBOX_API GetPlainTextFromSpanText( const VString& inTaggedText, VString& outPlainText); // sera utilisé par DB4D
//void XTOOLBOX_API ConverUniformStyleToList(const VTreeTextStyle* inUniformStyles, VTreeTextStyle* ioListUniformStyles);
//

END_TOOLBOX_NAMESPACE

#endif