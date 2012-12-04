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

#include "VChromeDebugHandlerPrv.h"
#include "VChromeDbgHdlPage.h"


USING_TOOLBOX_NAMESPACE

extern VLogHandler*			sPrivateLogHandler;

#define WKA_USE_UNIFIED_DBG

#define K_METHOD_STR		"{\"method\":"
#define K_PARAMS_STR		"\"params\":"
#define K_ID_STR			"\"id\":"
#define K_EXPRESSION_STR	"\"expression\":\""
#define K_ORDINAL_STR		"\\\"ordinal\\\":"

const XBOX::VString			K_DQUOT_STR("\"");

const XBOX::VString			K_UNKNOWN_CMD("UNKNOWN_CMD");

const XBOX::VString			K_TIM_SET_INC_MEM_DET("Timeline.setIncludeMemoryDetails");

const XBOX::VString			K_DBG_PAU("Debugger.pause");
const XBOX::VString			K_DBG_STE_OVE("Debugger.stepOver");
const XBOX::VString			K_DBG_STE_INT("Debugger.stepInto");
const XBOX::VString			K_DBG_STE_OUT("Debugger.stepOut");
const XBOX::VString			K_DBG_RES("Debugger.resume");

const XBOX::VString			K_DBG_PAU_STR("Debugger.paused");
const XBOX::VString			K_DBG_RES_STR("{\"method\":\"Debugger.resumed\"}");

const XBOX::VString			K_DBG_CAU_REC("Debugger.causesRecompilation");
const XBOX::VString			K_DBG_SUP_NAT_BRE("Debugger.supportsNativeBreakpoints");
const XBOX::VString			K_PRO_CAU_REC("Profiler.causesRecompilation");

const XBOX::VString			K_PRO_IS_SAM("Profiler.isSampling");
const XBOX::VString			K_PRO_HAS_HEA_PRO("Profiler.hasHeapProfiler");
const XBOX::VString			K_NET_ENA("Network.enable");
const XBOX::VString			K_PAG_ENA("Page.enable");
const XBOX::VString			K_PAG_GET_RES_TRE("Page.getResourceTree");
const XBOX::VString			K_NET_CAN_CLE_BRO_CAC("Network.canClearBrowserCache");
const XBOX::VString			K_NET_CAN_CLE_BRO_COO("Network.canClearBrowserCookies");
const XBOX::VString			K_WOR_SET_WOR_INS_ENA("Worker.setWorkerInspectionEnabled");
const XBOX::VString			K_DBG_CAN_SET_SCR_SOU("Debugger.canSetScriptSource");
const XBOX::VString			K_DBG_ENA("Debugger.enable");
const XBOX::VString			K_PRO_ENA("Profiler.enable");
const XBOX::VString			K_CON_ENA("Console.enable");
const XBOX::VString			K_INS_ENA("Inspector.enable");
const XBOX::VString			K_DAT_ENA("Database.enable");
const XBOX::VString			K_DOM_STO_ENA("DOMStorage.enable");
const XBOX::VString			K_CSS_ENA("CSS.enable");
const XBOX::VString			K_CSS_GET_SUP_PRO("CSS.getSupportedCSSProperties");
const XBOX::VString			K_DBG_SET_PAU_ON_EXC("Debugger.setPauseOnExceptions");
const XBOX::VString			K_DOM_GET_DOC("DOM.getDocument");
const XBOX::VString			K_DOM_HID_HIG("DOM.hideHighlight");
const XBOX::VString			K_CSS_GET_COM_STY_FOR_NOD("CSS.getComputedStyleForNode");
const XBOX::VString			K_CSS_GET_INL_STY_FOR_NOD("CSS.getInlineStylesForNode");
const XBOX::VString			K_CSS_GET_MAT_STY_FOR_NOD("CSS.getMatchedStylesForNode");
const XBOX::VString			K_CON_ADD_INS_NOD("Console.addInspectedNode");
const XBOX::VString			K_DOM_REQ_CHI_NOD("DOM.requestChildNodes");
const XBOX::VString			K_DOM_HIG_NOD("DOM.highlightNode");
const XBOX::VString			K_DOM_HIG_FRA("DOM.highlightFrame");
const XBOX::VString			K_RUN_REL_OBJ_GRO("Runtime.releaseObjectGroup");
const XBOX::VString			K_PAG_REL("Page.reload");
const XBOX::VString			K_DOM_SET_INS_MOD_ENA("DOM.setInspectModeEnabled");
const XBOX::VString			K_CON_MES_CLE("Console.messagesCleared");
const XBOX::VString			K_DOM_PUS_NOD_BY_PAT_TO_FRO_END("DOM.pushNodeByPathToFrontend");
const XBOX::VString			K_NET_GET_RES_BOD("Network.getResponseBody");
const XBOX::VString			K_DBG_SET_BRE_BY_URL("Debugger.setBreakpointByUrl");
const XBOX::VString			K_DBG_REM_BRE("Debugger.removeBreakpoint");
const XBOX::VString			K_APP_CAC_ENA("ApplicationCache.enable");
const XBOX::VString			K_APP_CAC_GET_FRA_WIT_MAN("ApplicationCache.getFramesWithManifests");
const XBOX::VString			K_DBG_GET_SCR_SOU("Debugger.getScriptSource");
const XBOX::VString			K_RUN_GET_PRO("Runtime.getProperties");
const XBOX::VString			K_DOM_STO_ADD_DOM_STO("DOMStorage.addDOMStorage");
const XBOX::VString			K_PAG_GET_RES_CON("Page.getResourceContent");
const XBOX::VString			K_DBG_EVA_ON_CAL_FRA("Debugger.evaluateOnCallFrame");

const XBOX::VString			K_LF_STR("\n");
const XBOX::VString			K_CR_STR("\r");
const XBOX::VString			K_LINE_FEED_STR("\\n");

#if VERSIONWIN || VERSION_LINUX
	const XBOX::VString			K_DEFAULT_LINE_SEPARATOR(K_LF_STR);
	const XBOX::VString			K_ALTERNATIVE_LINE_SEPARATOR(K_CR_STR);
#else

	#if VERSIONMAC
		const XBOX::VString			K_DEFAULT_LINE_SEPARATOR(K_CR_STR);
		const XBOX::VString			K_ALTERNATIVE_LINE_SEPARATOR(K_LF_STR);
	#else
		#error "UNKNOWN TARGET --> SHOULD NOT HAPPEN!!!"
	#endif

#endif
const XBOX::VString			K_EMPTY_FILENAME("empty-filename");

#define K_NODE_ID_STR				"\"nodeId\":"
#define K_BACKSLASHED_ID_STR		"\\\"id\\\":"
#define K_SCRIPTID_ID_STR			"\"scriptId\":\""

#define K_DBG_LINE_NUMBER_STR		"\"lineNumber\":"
#define K_DBG_COLUMN_STR			"\"columnNumber\":"
#define K_DBG_URL_STR				"\"url\":"
#define K_BRK_PT_ID_STR				"\"breakpointId\":"
#define K_FILE_STR					"file://"

#define K_ID_FMT_STR				"},\"id\":"


const XBOX::VString			K_RESULT_TRUE_STR("\"result\":true");
const XBOX::VString			K_RESULT_FALSE_STR("\"result\":false");

const XBOX::VString			K_EMPTY_STR("");
const XBOX::VString			K_SEND_RESULT_STR1("{\"result\":{");
const XBOX::VString			K_SEND_RESULT_STR2("},\"id\":");

#define K_MAX_NODE_ID_SIZE		(16)

#define K_FRAME_FACTOR			(100)
#define K_FRAME_ID_VALUE		"100"
#define K_LOADER_ID_VALUE		"200"
static sLONG					s_node_id = 1000;


const XBOX::VString				K_APP_CAC_GET_FRA_WIT_MAN_STR1("\"frameIds\":[]");

const XBOX::VString				K_APP_CAC_ENA_STR1("{\"method\":\"ApplicationCache.networkStateUpdated\",\"params\":{\"isNowOnline\":true}}");

const XBOX::VString				K_DBG_SET_BRE_BY_URL_STR1("{\"result\":{\"breakpointId\":");
const XBOX::VString				K_DBG_SET_BRE_BY_URL_STR2("\",\"locations\":[]},\"id\":");

const XBOX::VString				K_LINE_1("function myWrite(test) {\\n");
const XBOX::VString				K_LINE_2("\\t\\n\\tvar test1 = 123;\\n");
const XBOX::VString				K_LINE_3("\\t\\n\\ttest2 = \\\"GH\\\";\\n");
const XBOX::VString				K_LINE_4("\\tdocument.write('\\u003Cb\\u003EHello World \'+test+\' \'+test2+\' \'+test1+\' !!!!!\\u003C/b\\u003E\');\\n");
const XBOX::VString				K_LINE_5("\\n}\\n\\n");
const XBOX::VString				K_LINE_6("myWrite(3);\\n\\n");


const XBOX::VString				K_NET_GET_RES_BOD_STR1("{\"result\":{\"body\":\"\\u003C!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.01 Transitional//EN\\\" \\\"http://www.w3.org/TR/html4/loose.dtd\\\"\\u003E\\u003C!--DVID=00000745--\\u003E\n\\u003CHTML\\u003E\n\\u003Chead\\u003E\n\\u003Ctitle\\u003E\nHello World\n\\u003C/title\\u003E\n\\u003C/HEAD\\u003E\n\\u003Cbody\\u003E\n\\u003Cscript type=\\\"text/javascript\\\"\\u003E\n\n");
const XBOX::VString				K_NET_GET_RES_BOD_STR2("\n\\u003C/script\\u003E\n\\u003C/body\\u003E\n\\u003C/html\\u003E\n\n\",\"base64Encoded\":false},\"id\":");


const XBOX::VString				K_PAG_GET_RES_CON_STR1("{\"result\":{\"content\":\"\\u003C!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.01 Transitional//EN\\\" \\\"http://www.w3.org/TR/html4/loose.dtd\\\"\\u003E\\u003C!--DVID=00000745--\\u003E\\n\\u003CHTML\\u003E\\n\\u003Chead\\u003E\\n\\u003Ctitle\\u003E\\nHello World\\n\\u003C/title\\u003E\\n\\u003C/HEAD\\u003E\\n\\u003Cbody\\u003E\\n\\u003Cscript type=\\\"text/javascript\\\"\\u003E\\n\\n");
const XBOX::VString				K_PAG_GET_RES_CON_STR2("\\n\\u003C/script\\u003E\\n\\u003C/body\\u003E\\n\\u003C/html\\u003E\\n\\n\",\"base64Encoded\":false},\"id\":");


const XBOX::VString				K_DOM_PUS_NOD_BY_PAT_TO_FRO_END_STR1("{\"result\":{\"nodeId\":");


const XBOX::VString				K_PAG_GET_RES_TRE_STR1A("{\"result\":{\"frameTree\":{\"frame\":{\"id\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1B("\",\"url\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1C("\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"securityOrigin\":\"null\",\"mimeType\":\"text/html\"},\"resources\":[{\"url\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1D("\",\"type\":\"Script\",\"mimeType\":\"application/javascript\"}]}},\"id\":");


const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1A("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"30.15\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"documentURL\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1B("\",\"request\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1C("\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\"}},\"timestamp\":1331300225.457657,\"initiator\":{\"type\":\"other\"},\"stackTrace\":[]}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR2A("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"30.15\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"timestamp\":1331300225.460701,\"type\":\"Document\",\"response\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR2B("\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"text/html\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1331300225.458127,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR3("{\"method\":\"Console.messagesCleared\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR4A("{\"method\":\"Page.frameNavigated\",\"params\":{\"frame\":{\"id\":\"" K_FRAME_ID_VALUE "\",\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR4B("\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"securityOrigin\":\"null\",\"mimeType\":\"text/html\"}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR5("{\"method\":\"Debugger.globalObjectCleared\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR6("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"30.15\",\"timestamp\":1331300225.473018,\"dataLength\":252,\"encodedDataLength\":0}}");

const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A1("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"30.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"documentURL\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A2("\",\"request\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A3("\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\",\"Accept\":\"*/*\"}},\"timestamp\":1331300225.473645,\"initiator\":{\"type\":\"parser\",\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A4("\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7B("},\"stackTrace\":[]}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR8("{\"method\":\"Network.loadingFinished\",\"params\":{\"requestId\":\"30.15\",\"timestamp\":1331300225.463939}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9A("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"30.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"timestamp\":1331300225.475966,\"type\":\"Script\",\"response\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9B("\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"application/javascript\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1331300225.473833,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR10("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"30.16\",\"timestamp\":1331300225.476164,\"dataLength\":154,\"encodedDataLength\":0}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11A("{\"method\":\"Debugger.scriptParsed\",\"params\":{\"scriptId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11B("\",\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11C("\",\"startLine\":0,\"startColumn\":0,\"endLine\":11,\"endColumn\":1}}");	
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A1("{\"method\":\"Debugger.breakpointResolved\",\"params\":{\"breakpointId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A2(":6:0\",\"location\":{\"scriptId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A3("\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12B(",\"columnNumber\":1}}}");

const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13A1(",\"params\":{\"callFrames\":[{\"callFrameId\":\"{\\\"ordinal\\\":0,\\\"injectedScriptId\\\":3} \",\"functionName\":\"myWrite\",\"location\":{\"scriptId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13A2("\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13B(",\"columnNumber\":1},\"scopeChain\":[{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":1}\",\"className\":\"Object\",\"description\":\"Object\"},\"type\":\"local\"},{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":2}\",\"className\":\"Application\",\"description\":\"Application\"},\"type\":\"global\"}],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":3}\",\"className\":\"Application\",\"description\":\"Application\"}},{\"callFrameId\":\"{\\\"ordinal\\\":1,\\\"injectedScriptId\\\":3}\",\"functionName\":\"\",\"location\":{\"scriptId\":\"41\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13C(",\"columnNumber\":0},\"scopeChain\":[{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":4}\",\"className\":\"Application\",\"description\":\"Application\"},\"type\":\"global\"}],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":5}\",\"className\":\"Application\",\"description\":\"Application\"}}],\"reason\":\"other\"}}");

const XBOX::VString				K_DOM_REQ_CHI_NOD_STR1("{\"method\":\"DOM.setChildNodes\",\"params\":{\"parentId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR2(",\"nodes\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR3(",\"nodeType\":1,\"nodeName\":\"SCRIPT\",\"localName\":\"script\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4A(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4B("ndocument.write('Hello World !!!!!/b');\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4C("ndocument.write('Hello World 2!!!!!/b');\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4D("ndocument.write('Hello World 3!!!!!/b');\\");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR4E("n\"}],\"attributes\":[\"type\",\"text/javascript\"]},{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR5(",\"nodeType\":1,\"nodeName\":\"B\",\"localName\":\"b\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR6(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"Hello World !!!!!\"}],\"attributes\":[]},{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR7(",\"nodeType\":1,\"nodeName\":\"B\",\"localName\":\"b\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR8(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"Hello World 2!!!!!\"}],\"attributes\":[]},{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR9(",\"nodeType\":1,\"nodeName\":\"B\",\"localName\":\"b\",\"nodeValue\":\"\",\"childNodeCount\":1,\"children\":[{\"nodeId\":");
const XBOX::VString				K_DOM_REQ_CHI_NOD_STR10(",\"nodeType\":3,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"Hello World 3!!!!!\"}],\"attributes\":[]}]}}");

const XBOX::VString				K_GET_DOC_STR1("{\"result\":{\"root\":{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR2(",\"nodeType\":9,\"nodeName\":\"#document\",\"localName\":\"\",\"nodeValue\":\"\",\"childNodeCount\":3,\"children\":[{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR3(",\"nodeType\":10,\"nodeName\":\"html\",\"localName\":\"\",\"nodeValue\":\"\",\"publicId\":\"-//W3C//DTD HTML 4.01 Transitional//EN\",\"systemId\":\"http://www.w3.org/TR/html4/loose.dtd\",\"internalSubset\":\"\"},{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR4(",\"nodeType\":8,\"nodeName\":\"\",\"localName\":\"\",\"nodeValue\":\"DVID=00000745\"},{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR5(",\"nodeType\":1,\"nodeName\":\"HTML\",\"localName\":\"html\",\"nodeValue\":\"\",\"childNodeCount\":2,\"children\":[{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR6(",\"nodeType\":1,\"nodeName\":\"HEAD\",\"localName\":\"head\",\"nodeValue\":\"\",\"childNodeCount\":1,\"attributes\":[]},{\"nodeId\":");
const XBOX::VString				K_GET_DOC_STR7A(",\"nodeType\":1,\"nodeName\":\"BODY\",\"localName\":\"body\",\"nodeValue\":\"\",\"childNodeCount\":4,\"attributes\":[]}],\"attributes\":[]}],\"documentURL\":\"");
const XBOX::VString				K_GET_DOC_STR7B("\",\"xmlVersion\":\"\"}},\"id\":");

const XBOX::VString				K_DBG_GET_SCR_SOU_STR1("{\"result\":{\"scriptSource\":\"\\n");

const XBOX::VString			K_TEMPLATE_WAKSERV_HOST_STR("__TEMPLATE_WAKANDA_SERVER_HOST__");
const XBOX::VString			K_TEMPLATE_WAKSERV_PORT_STR("__TEMPLATE_WAKANDA_SERVER_PORT__");





VChromeDbgHdlPage::VChromeDbgHdlPage() : fFileName(K_EMPTY_FILENAME),fState(IDLE_STATE),fSem(1)
{
	fPageNb = -1;
}

VChromeDbgHdlPage::~VChromeDbgHdlPage()
{

	xbox_assert(false);
}


void VChromeDbgHdlPage::Init(sLONG inPageNb,CHTTPServer* inHTTPSrv,XBOX::VString inRelUrl)
{
	fWS = inHTTPSrv->NewHTTPWebsocketHandler();
	//fUsed = false;
	fEnabled = false;
	fFileName = K_EMPTY_FILENAME;
	fPageNb = inPageNb;
	fRelURL = inRelUrl;
}



/*
   checks the syntax of the command requested by the client. Syntax is:
	{"method":"name_of_method",["params":{"name_of_param":"value_of_param},]"id":"number"}
*/
XBOX::VError VChromeDbgHdlPage::CheckInputData()
{
	sLONG			l_pos;
	sBYTE*			l_tmp;
	sBYTE*			l_method;
	XBOX::VError	l_err;

	l_err = VE_OK;
	l_pos = memcmp(fMsgData,K_METHOD_STR,strlen(K_METHOD_STR));
	
	if (l_pos)
	{
		l_err = VE_INVALID_PARAMETER;
		xbox_assert(!l_pos);
		DebugMsg("Messages should start with '%s'!!!!\n",K_METHOD_STR);
	}

	if (!l_err)
	{
		l_pos = fOffset - 1;
		while( (l_pos >= 0) && (fMsgData[l_pos] != '}') )
		{
			l_pos--;
		}
		if (l_pos < 0)
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (!l_err)
	{
		fMsgData[l_pos] = 0;
		l_method = strchr(fMsgData + strlen(K_METHOD_STR),'"');
		if (!l_method)
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (!l_err)
	{
		l_method += 1;
		l_tmp = strchr(l_method,'"');
		if (!l_tmp)
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (!l_err)
	{
		*l_tmp = 0;
		fId = strstr(l_tmp+1,K_ID_STR);
		if (!fId)
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (!l_err)
	{
		fParams =  strstr(l_tmp+1,K_PARAMS_STR);
		fId += strlen(K_ID_STR);
		l_tmp = fId;
		while ( l_tmp && ( (*l_tmp == 32) || ( (*l_tmp >= 0x30) && (*l_tmp <=0x39) ) ) )
		{
			l_tmp++;
		}
		*l_tmp = 0;
		fMethod = VString(l_method);
	}
	return l_err;
}




XBOX::VError VChromeDbgHdlPage::SendMsg(const XBOX::VString& inValue)
{
	XBOX::VError			err;
	VStringConvertBuffer	buffer(inValue,VTC_UTF_8);
	VSize					size;
	VString					trace;

	err = VE_OK;

	DebugMsg(" treat '%S' ... resp-><%S>\n",&fMethod,&inValue);
	size = buffer.GetSize();
	err = fWS->WriteMessage( buffer.GetCPointer(), size, true );
	trace = CVSTR("VChromeDbgHdlPage::SendMsg WriteMessage err=");
	trace.AppendLong8(err);
	trace += ", size:";
	trace.AppendLong8(size);
	trace += ", val <";
	trace += inValue;
	trace += ">";
	sPrivateLogHandler->Put( (err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),trace);

	return err;
}

XBOX::VError VChromeDbgHdlPage::SendResult(const XBOX::VString& inValue)
{
	VString				l_tmp;

	l_tmp = K_SEND_RESULT_STR1;
	l_tmp += inValue;
	l_tmp += K_SEND_RESULT_STR2;
	l_tmp += fId;
	l_tmp += "}";

	return SendMsg(l_tmp);

}

XBOX::VError VChromeDbgHdlPage::SendDbgPaused(XBOX::VString* inStr, intptr_t inSrcId)
{
	XBOX::VError		l_err;
	XBOX::VString		l_resp;

	l_resp = K_METHOD_STR;
	l_resp += K_DQUOT_STR;
	//if (fState == IJSWChrmDebugger::PAUSE_STATE)
	//{
		l_resp += K_DBG_PAU_STR;
	//}
	/*else
	{
		xbox_assert(false);
	}*/
	l_resp += K_DQUOT_STR;
	if (!inStr)
	{
		l_resp += K_PAG_REL_AFT_BKPT_STR13A1;
		l_resp.AppendULong8(inSrcId);
		l_resp += K_PAG_REL_AFT_BKPT_STR13A2;
		l_resp.AppendLong(fLineNb);
		l_resp += K_PAG_REL_AFT_BKPT_STR13B;
		l_resp.AppendLong(fLineNb);
		l_resp += K_PAG_REL_AFT_BKPT_STR13C;
	}
	else
	{
		l_resp += *inStr;
	}
	l_err = SendMsg(l_resp);
	return l_err;
}

XBOX::VError VChromeDbgHdlPage::TreatPageReload(VString *inStr, intptr_t inSrcId)
{
	XBOX::VError		l_err;
	XBOX::VString		l_resp;

	VString l_trace("VChromeDbgHdlPage::TreatPageReload called");
	sPrivateLogHandler->Put( WAKDBG_INFO_LEVEL,l_trace);

	/*if (fSrcSent)
	{
		l_err = SendDbgPaused(inStr,inSrcId);
	}*/
	//else
	{
		l_resp = K_PAG_REL_AFT_BKPT_STR1A;
		l_resp += fFileName;
		l_resp += K_PAG_REL_AFT_BKPT_STR1B;
		l_resp += fFileName;
		l_resp += K_PAG_REL_AFT_BKPT_STR1C;
		l_err = SendMsg(l_resp);
		if (!l_err)
		{
			l_err = SendResult(K_EMPTY_STR);
			l_resp = K_PAG_REL_AFT_BKPT_STR2A;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR2B;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR3;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR4A;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR4B;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR5;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR6;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR7A1;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR7A2;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR7A3;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR7A4;
			l_resp.AppendLong(fPageNb);
			l_resp += K_PAG_REL_AFT_BKPT_STR7B;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR8;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR9A;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR9B;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR10;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR11A;
			l_resp.AppendULong8(inSrcId);
			l_resp += K_PAG_REL_AFT_BKPT_STR11B;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR11C;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			l_resp = K_PAG_REL_AFT_BKPT_STR12A1;
			l_resp += fFileName;
			l_resp += K_PAG_REL_AFT_BKPT_STR12A2;
			l_resp.AppendULong8(inSrcId);
			l_resp += K_PAG_REL_AFT_BKPT_STR12A3;
			l_resp.AppendLong(fPageNb);
			l_resp += K_PAG_REL_AFT_BKPT_STR12B;
		}
		if (!l_err)
		{
			l_err = SendMsg(l_resp);
			if (!l_err)
			{
				l_err = SendDbgPaused(inStr,inSrcId);
			}
			//fSrcSent = true;
		}
	}
	return l_err;
}

XBOX::VError VChromeDbgHdlPage::GetBrkptParams(sBYTE* inStr, sBYTE** outUrl, sBYTE** outColNb, sBYTE **outLineNb)
{
	VError		l_err;
	sBYTE		*l_end;
	
	*outLineNb = strstr(inStr,K_DBG_LINE_NUMBER_STR);
	*outColNb = strstr(inStr,K_DBG_COLUMN_STR);
	*outUrl = strstr(inStr,K_DBG_URL_STR);

	l_err = VE_OK;
	if ( (!*outUrl) || (!*outColNb) || (!*outLineNb) )
	{
		DebugMsg("bad params for cmd <%S>\n",&fMethod);
		l_err = VE_INVALID_PARAMETER;
	}
	if (!l_err)
	{
		*outUrl += strlen(K_DBG_URL_STR);
		l_end = strchr(*outUrl,',');
		if (l_end)
		{
			*l_end = 0;
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (!l_err)
	{
		*outColNb += strlen(K_DBG_COLUMN_STR);
		l_end = strchr(*outColNb,',');
		if (l_end)
		{
			*l_end = 0;
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (!l_err)
	{
		*outLineNb += strlen(K_DBG_LINE_NUMBER_STR);
		l_end = strchr(*outLineNb,',');
		if (l_end)
		{
			*l_end = 0;
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}
	if (l_err) 
	{
		DebugMsg("bad params for cmd <%S>\n",&fMethod);
		l_err = VE_INVALID_PARAMETER;
	}
	return l_err;
}

XBOX::VError VChromeDbgHdlPage::TreatMsg()
{		
	XBOX::VError		l_err;
	XBOX::VString		l_cmd;
	XBOX::VString		l_resp;
	bool				l_found;
	sBYTE*				l_id;
	ChrmDbgMsg_t		l_msg;
	bool				l_msg_found;

	l_err = VE_OK;

	l_cmd = K_UNKNOWN_CMD;
	l_found = false;
	l_err = CheckInputData();

	VString l_trace("VChromeDbgHdlPage::TreatMsg Method=");
	l_trace += fMethod;
	if (fParams)
	{
		l_trace += " ,";
		VString l_params = "params=<";
		l_params.AppendCString(fParams);
		l_params += ">";
		l_trace += l_params;
	}
	sPrivateLogHandler->Put( (l_err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_trace);

	if (!l_err)
	{
		DebugMsg("VChromeDbgHdlPage::TreatMsg Method='%S', Id='%s',Params='%s'\n",&fMethod,fId,(fParams ? fParams : "."));
	}
	else
	{
		DebugMsg("VChromeDbgHdlPage::TreatMsg CheckInputData pb !!!!\n");
	}

	l_cmd = K_APP_CAC_GET_FRA_WIT_MAN;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_APP_CAC_GET_FRA_WIT_MAN_STR1);
	}
#if 1
	l_cmd = K_APP_CAC_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	

		l_resp = K_APP_CAC_ENA_STR1;
		l_err = SendMsg(l_resp);
		if (!l_err)
		{
			l_err = SendResult(K_EMPTY_STR);
		}
	}

	l_cmd = K_DBG_REM_BRE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		sBYTE	*l_end;
		sBYTE	*l_tmp;
		l_tmp = strstr(fParams,K_BRK_PT_ID_STR);
		if (l_tmp)
		{
			l_tmp += strlen(K_BRK_PT_ID_STR)+1;// +1 to skip the '"'

			l_end = strchr(l_tmp,'"');
			if (l_end)
			{
				*l_end = 0;
				/*if (strstr(l_tmp,K_FILE_STR) == l_tmp)
				{
					l_tmp += strlen(K_FILE_STR);
				}*/
				l_end = strchr(l_tmp,':');
				if (l_end)
				{
					sscanf(l_end,":%d:",&l_msg.data.Msg.fLineNumber);
					*l_end = 0;
					l_msg.type = SEND_CMD_MSG;
					l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_REMOVE_BREAKPOINT_MSG;
					l_msg.data.Msg.fSrcId = fSrcId;
					//l_msg.data.Msg.fUrl[fURL.ToBlock(l_msg.data.Msg.fUrl,K_MAX_FILESIZE-1,VTC_UTF_8,false,false)] = 0;
					l_err = fOutFifo.Put(l_msg);
					if (testAssert( l_err == VE_OK ))
					{
						// we answer as success since wakanda is not able to return a status regarding bkpt handling
						l_err = SendResult(K_EMPTY_STR);
					}
				}
			}
		}
	}

	l_cmd = K_DBG_EVA_ON_CAL_FRA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		sBYTE*		l_tmp_exp;
		sBYTE*		l_ord;
		l_tmp_exp = strstr(fParams,K_EXPRESSION_STR);
		l_ord = strstr(fParams,K_ORDINAL_STR);
		if (l_tmp_exp && l_ord)
		{
			l_tmp_exp += strlen(K_EXPRESSION_STR);
			sBYTE *l_end_str;
			l_end_str = strchr(l_tmp_exp,'"');
			if (l_end_str)
			{
				l_msg.data.Msg.fLineNumber = atoi(l_ord+strlen(K_ORDINAL_STR));
				*l_end_str = 0;
				VString l_exp(l_tmp_exp);
				DebugMsg("requiring value of '%S'\n",&l_exp);
				//l_resp = CVSTR("\"result\":{\"type\":\"number\",\"value\":3,\"description\":\"3\"}");
				l_msg.type = SEND_CMD_MSG;
				l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_EVALUATE_MSG;
				l_msg.data.Msg.fSrcId = fSrcId;
				l_msg.data.Msg.fString[l_exp.ToBlock(l_msg.data.Msg.fString,K_MAX_FILESIZE-1,VTC_UTF_8,false,false)] = 0;
				l_err = fOutFifo.Put(l_msg);
				if (testAssert( l_err == VE_OK ))
				{
					l_err = fFifo.Get(&l_msg,&l_msg_found,0);
					if (testAssert( l_err == VE_OK ))
					{
						if (l_msg_found && (l_msg.type == EVAL_MSG))
						{
						}
						else
						{
							l_err = VE_INVALID_PARAMETER;
							xbox_assert(false);
						}
					}
				}
				l_resp = l_msg.data._dataStr;
				VString		l_hdr = CVSTR("\"result\":{\"value\":{");
				VString		l_sub;
				if (l_resp.GetLength() > l_hdr.GetLength())
				{
					l_resp.GetSubString(l_hdr.GetLength(),l_resp.GetLength()-l_hdr.GetLength()+1,l_sub);
				}
				l_resp = CVSTR("\"result\":");
				l_resp += l_sub;
				if (!l_err)
				{
					l_err = SendResult(l_resp);
				}
			}
			else
			{
				l_err = VE_INVALID_PARAMETER;
			}
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
	}

	l_cmd = K_DBG_SET_BRE_BY_URL;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		sBYTE	*l_line_nb;
		sBYTE	*l_col_nb;
		sBYTE	*l_url;
		l_err = GetBrkptParams(fParams,&l_url,&l_col_nb,&l_line_nb);

		if (!l_err)
		{
			l_msg.type = SEND_CMD_MSG;
			l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_SET_BREAKPOINT_MSG;
			l_msg.data.Msg.fSrcId = fSrcId;
			l_msg.data.Msg.fLineNumber = atoi(l_line_nb);
			//l_msg.data.Msg.fUrl[fURL.ToBlock(l_msg.data.Msg.fUrl,K_MAX_FILESIZE-1,VTC_UTF_8,false,false)] = 0;
			l_err = fOutFifo.Put(l_msg);
			xbox_assert( l_err == VE_OK );
		}
		if (!l_err)
		{
			l_resp = K_DBG_SET_BRE_BY_URL_STR1;
			l_url[strlen(l_url)-1] = 0;// -1 is aimed to remove the final '"'
			l_resp += l_url;
			l_resp += ":";
			l_resp += l_line_nb;
			l_resp += ":";
			l_resp += l_col_nb;
			l_resp += K_DBG_SET_BRE_BY_URL_STR2;
			l_resp += fId;
			l_resp += "}";
			// we answer as success since wakanda is not able to return a status regarding bkpt handling
			l_err = SendMsg(l_resp);
		}
	}

	l_cmd = K_NET_GET_RES_BOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_resp = K_NET_GET_RES_BOD_STR1;
		l_resp += K_LINE_1;
		l_resp += K_LINE_2;
		l_resp += K_LINE_3;
		l_resp += K_LINE_4;
		l_resp += K_LINE_5;
		l_resp += K_LINE_6;
		l_resp += K_NET_GET_RES_BOD_STR2;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_PAG_GET_RES_CON;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_resp = K_PAG_GET_RES_CON_STR1;
		l_resp += K_LINE_1;
		l_resp += K_LINE_2;
		l_resp += K_LINE_3;
		l_resp += K_LINE_4;
		l_resp += K_LINE_5;
		l_resp += K_LINE_6;
		l_resp += K_PAG_GET_RES_CON_STR2;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}

	l_cmd = K_DOM_SET_INS_MOD_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DOM_PUS_NOD_BY_PAT_TO_FRO_END;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_resp = K_DOM_PUS_NOD_BY_PAT_TO_FRO_END_STR1;
		l_resp.AppendLong(fBodyNodeId);
		l_resp += K_ID_FMT_STR;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_PAG_REL;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		// page reload from CHrome not handled
		xbox_assert(false);
		l_err = VE_UNKNOWN_ERROR;
		return l_err;
#if 0

if (s_brkpt_line_nb.GetLength())
{
	return TreatPageReload(NULL);
	return l_err;
}
		l_resp = K_PAG_REL_STR1;
		l_err = SendMsg(l_resp);
		if (!l_err)
		{
			l_err = SendResult(K_EMPTY_STR);
		}
		if (!l_err)
		{
			l_resp = K_PAG_REL_STR2;
			l_err = SendMsg(l_resp);
		}
		if (!l_err)
		{
			l_resp = K_PAG_REL_STR3;
			l_err = SendMsg(l_resp);
		}
		if (!l_err)
		{
			l_resp = K_PAG_REL_STR4;
			l_err = SendMsg(l_resp);
		}
		if (!l_err)
		{
			l_resp = K_PAG_REL_STR5;
			l_err = SendMsg(l_resp);
		}
		if (!l_err)
		{
			l_resp = K_PAG_REL_STR6;
			l_err = SendMsg(l_resp);
		}

		if (!l_err)
		{
			l_resp = K_PAG_REL_STR7;
			l_err = SendMsg(l_resp);
		}

		if (!l_err)
		{
			l_resp = K_PAG_REL_STR8;
			l_err = SendMsg(l_resp);
		}
				if (!l_err)
		{
			l_resp = K_PAG_REL_STR9;
			l_err = SendMsg(l_resp);
		}

		if (!l_err)
		{
			l_resp = K_PAG_REL_STR10;
			l_err = SendMsg(l_resp);
		}		

		if (!l_err)
		{
			l_resp = K_PAG_REL_STR11;
			l_err = SendMsg(l_resp);
		}	
#endif
	}

	l_cmd = K_DBG_GET_SCR_SOU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_SCRIPTID_ID_STR);
		if (!l_id)
		{
			DebugMsg("NO %s param found for %S\n",K_SCRIPTID_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_id += strlen(K_SCRIPTID_ID_STR);
			sBYTE*	l_end = strchr(l_id,'"');
			if (l_end)
			{
				*l_end = 0;
				strncpy(l_msg.data.Msg.fString,l_id,K_STR_MAX_SIZE);
				l_msg.data.Msg.fString[K_STR_MAX_SIZE-1] = 0;
			}
			else
			{
				l_err = VE_INVALID_PARAMETER;
			}
		}
		l_resp = K_DBG_GET_SCR_SOU_STR1;

		if (!l_err)
		{
			for(VectorOfVString::iterator l_it = fSource.begin(); (l_it != fSource.end()); )
			{
				l_resp += *l_it;
				l_resp += K_LINE_FEED_STR;
				++l_it;
			}
		}
		else
		{
			l_resp += CVSTR(" NO SOURCE AVAILABLE ");
			l_resp += K_LINE_FEED_STR;
		}
		l_resp += "\"},\"id\":";
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_RUN_GET_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_BACKSLASHED_ID_STR);
		if (!l_id)
		{
			DebugMsg("NO %s param found for %S\n",K_BACKSLASHED_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_msg.type = SEND_CMD_MSG;
			l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_LOOKUP_MSG;
			l_msg.data.Msg.fObjRef = atoi(l_id+strlen(K_BACKSLASHED_ID_STR));
			l_err = fOutFifo.Put(l_msg);
			if (testAssert( l_err == VE_OK ))
			{
				l_err = fFifo.Get(&l_msg,&l_msg_found,0);
				xbox_assert( l_err == VE_OK );
			}
			if (!l_err)
			{
				if (l_msg_found && (l_msg.type != LOOKUP_MSG))
				{
					l_err = VE_INVALID_PARAMETER;
				}
				xbox_assert(l_err == VE_OK);
			}
		}
		if (!l_err)
		{
			l_resp = l_msg.data._dataStr;
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}

	l_cmd = K_DOM_HIG_FRA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DOM_HIG_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);

	}

	l_cmd = K_RUN_REL_OBJ_GRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DOM_REQ_CHI_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DebugMsg("NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DebugMsg("INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp = K_DOM_REQ_CHI_NOD_STR1;
			l_resp.AppendLong(fBodyNodeId);
			l_resp += K_DOM_REQ_CHI_NOD_STR2;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR3;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR4A;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4B;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4C;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4D;
#if 1
			l_resp += "n";
			/*VectorOfVString::const_iterator l_it = fSource.begin();
			for(; l_it != fSource.end(); l_it++)
			{
				l_resp += *l_it;
				l_resp += K_LINE_FEED_STR;
			}*/
			l_resp+= "\\";
#endif
			l_resp += K_DOM_REQ_CHI_NOD_STR4E;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR5;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR6;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR7;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR8;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR9;
			l_resp.AppendLong(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR10;

			l_err = SendMsg(l_resp);

			if (!l_err)
			{
				l_err = SendResult(K_EMPTY_STR);
			}
		}

	}

	l_cmd = K_CON_ADD_INS_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}
	
	l_cmd = K_CSS_GET_MAT_STY_FOR_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DebugMsg("NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DebugMsg("INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp.AppendBlock(_binary_getMatchedStylesForNode,sizeof(_binary_getMatchedStylesForNode),VTC_UTF_8);
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}

	l_cmd = K_CSS_GET_INL_STY_FOR_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DebugMsg("NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DebugMsg("INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp.AppendBlock(_binary_getInlineStylesForNode,sizeof(_binary_getInlineStylesForNode),VTC_UTF_8);
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}
	l_cmd = K_CSS_GET_COM_STY_FOR_NOD;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_id = strstr(fParams,K_NODE_ID_STR);
		if (!l_id)
		{
			DebugMsg("NO %s param found for %S\n",K_NODE_ID_STR,&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err && (atoi(l_id+strlen(K_NODE_ID_STR)) != fBodyNodeId) )
		{
			DebugMsg("INCORRECT BODY node id for %S\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
		}
		if (!l_err)
		{
			l_resp.AppendBlock(_binary_getComputedStyleForNode,sizeof(_binary_getComputedStyleForNode),VTC_UTF_8);
			l_resp += fId;
			l_resp += "}";
			l_err = SendMsg(l_resp);
		}
	}
	
	l_cmd = K_DOM_GET_DOC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {

		l_resp = K_GET_DOC_STR1;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR2;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR3;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR4;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR5;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR6;
		fBodyNodeId = s_node_id;
		l_resp.AppendLong(s_node_id++);
		l_resp += K_GET_DOC_STR7A;
		l_resp += fFileName;
		l_resp += K_GET_DOC_STR7B;
		l_resp += fId;
		l_resp += "}";

		l_err = SendMsg(l_resp);
	}

	l_cmd = K_DOM_HID_HIG;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_SET_PAU_ON_EXC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_TIM_SET_INC_MEM_DET;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_CSS_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_CSS_GET_SUP_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_resp.AppendBlock(_binary_getSupportedCSSProperties,sizeof(_binary_getSupportedCSSProperties),VTC_UTF_8);
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);

	}	
	
	l_cmd = K_DOM_STO_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DAT_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_INS_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		fEnabled = true;
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_CON_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_PRO_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_DBG_CAN_SET_SCR_SOU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_WOR_SET_WOR_INS_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_NET_CAN_CLE_BRO_CAC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_NET_CAN_CLE_BRO_COO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PAG_GET_RES_TRE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_resp = K_PAG_GET_RES_TRE_STR1A;
		l_resp.AppendLong(fPageNb*K_FRAME_FACTOR);
		l_resp += K_PAG_GET_RES_TRE_STR1B;
		l_resp += fFileName;
		l_resp.AppendLong(fPageNb);
		l_resp += ".html";
		l_resp += K_PAG_GET_RES_TRE_STR1C;
		l_resp += fFileName;
		l_resp += K_PAG_GET_RES_TRE_STR1D;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);

	}
	l_cmd = K_PAG_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_NET_ENA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}

	l_cmd = K_PRO_HAS_HEA_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PRO_IS_SAM;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_PRO_CAU_REC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {	
		l_err = SendResult(K_RESULT_FALSE_STR);
	}

	l_cmd = K_DBG_SUP_NAT_BRE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_TRUE_STR);
	}

	l_cmd = K_DBG_RES;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		fState = RUNNING_STATE;
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_CONTINUE_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_STE_INT;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		//fState = IJSWChrmDebugger::RUNNING_STATE;
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_STEP_INTO_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_STE_OUT;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		//fState = IJSWChrmDebugger::RUNNING_STATE;
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_STEP_OUT_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_STE_OVE;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
		//fState = IJSWChrmDebugger::RUNNING_STATE;
		if (!l_err)
		{
			l_resp = K_DBG_RES_STR;
			l_err = SendMsg(l_resp);
		}
		l_msg.type = SEND_CMD_MSG;
		l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_STEP_OVER_MSG;
		l_err = fOutFifo.Put(l_msg);
		xbox_assert( l_err == VE_OK );
	}
	l_cmd = K_DBG_CAU_REC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_FALSE_STR);
	}
	l_cmd = K_DBG_PAU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_EMPTY_STR);
	}
	if (!l_found)
	{
		DebugMsg("VChromeDbgHdlPage::TreatMsg Method='%S' UNKNOWN!!!\n",&fMethod);
		exit(-1);
		return VE_OK;// --> pour l'instant on renvoit OK  !!! VE_INVALID_PARAMETER;
	}
#endif
	return l_err;

}

XBOX::VError VChromeDbgHdlPage::TreatWS(IHTTPResponse* ioResponse)
{
	XBOX::VError	l_err;
	ChrmDbgMsg_t	l_msg;
	bool			l_msg_found;
	intptr_t		l_src_id;
	RemoteDebugPilot*	l_pilot;
	VString			l_tmp;
	l_err = fSem.TryToLock();

	if (!l_err)
	{
		DebugMsg("VChromeDbgHdlPage::TreatWS already in use\n"); 
		return VE_OK;// so that HTTP server does not send error
	}

	l_err = fWS->TreatNewConnection(ioResponse);

	l_tmp = CVSTR("VChromeDbgHdlPage::TreatWS TreatNewConnection status=");
	l_tmp.AppendLong8(l_err);
	sPrivateLogHandler->Put( (l_err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_tmp);
	
	fFileName = K_EMPTY_FILENAME;
	fOffset = 0;

	while(!l_err)
	{
		fMsgLen = K_MAX_SIZE - fOffset;
		l_err = fWS->ReadMessage(fMsgData+fOffset,(uLONG*)&fMsgLen,&fIsMsgTerminated);

		// only display this trace on error (otherwise there are lots of msgs due to polling
		if (l_err)
		{
			l_tmp = CVSTR("VChromeDbgHdlPage::TreatWS ReadMessage status=");
			l_tmp.AppendLong8(l_err);
			l_tmp += " len=";
			l_tmp.AppendLong(fMsgLen);
			l_tmp += " fIsMsgTerminated=";
			l_tmp += (fIsMsgTerminated ? "1" : "0" );
			sPrivateLogHandler->Put( (l_err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_tmp);
		}

		if (!l_err)
		{
			if (!fMsgLen)
			{
				l_err = fFifo.Get(&l_msg,&l_msg_found,200);
				
				if (l_err || l_msg_found)
				{
					l_tmp = VString("VChromeDbgHdlPage::TreatWS page=");
					l_tmp.AppendLong(fPageNb);
					l_tmp += " ,fFifo.get status=";
					l_tmp.AppendLong8(l_err);
					l_tmp += ", msg_found=";
					l_tmp += (l_msg_found ? "1" : "0" );
					l_tmp += ", type=";
					l_tmp.AppendLong8(l_msg.type);
					sPrivateLogHandler->Put( (l_err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_tmp);
				}

				if (testAssert( l_err == VE_OK ))
				{
					if (!l_msg_found)
					{
						l_msg.type = NO_MSG;
					}
					switch(l_msg.type)
					{
					case NO_MSG:
						break;
					case BRKPT_REACHED_MSG:
						fLineNb = (sLONG)l_msg.data.Msg.fLineNumber;
					
						l_tmp = VString("VChromeDbgHdlPage::TreatWS page=");
						l_tmp.AppendLong(fPageNb);
						l_tmp += " BRKPT_REACHED_MSG fLineNb=";
						l_tmp.AppendLong(fLineNb);
						l_tmp += ", found=";
						l_tmp += (l_msg_found ? "1" : "0" );
						l_tmp += ", msgType=";
						l_tmp.AppendLong8(l_msg.type);
						sPrivateLogHandler->Put(WAKDBG_INFO_LEVEL,l_tmp);

						fSrcId = l_msg.data.Msg.fSrcId;
						fFileName = l_msg.data._urlStr;
						VURL::Decode( fFileName );
						VIndex				l_idx;
						l_pilot = l_msg.data._pilot;
						l_idx = fFileName.FindUniChar( L'/', fFileName.GetLength(), true );
						if (l_idx  > 1)
						{
							VString	l_tmp_str;
							fFileName.GetSubString(l_idx+1,fFileName.GetLength()-l_idx,l_tmp_str);
							fFileName = l_tmp_str;
						}
						if (!l_msg.data._dataStr.GetSubStrings(K_DEFAULT_LINE_SEPARATOR,fSource,true))
						{
							if (!l_msg.data._dataStr.GetSubStrings(K_ALTERNATIVE_LINE_SEPARATOR,fSource,true))
							{
								fSource = VectorOfVString(1,VString("No line separator detected in source CODE"));
							}
						}
						for(VectorOfVString::iterator l_it = fSource.begin(); (l_it != fSource.end()); )
						{
							l_tmp = *l_it;
							if (l_tmp.GetJSONString(*l_it) != VE_OK)
							{
								//l_res = false;
								xbox_assert(false);
							}
							++l_it;
						}

#if 1
						l_src_id = l_msg.data.Msg.fSrcId;

						l_msg.type = SEND_CMD_MSG;
						l_msg.data.Msg.fType = WAKDebuggerServerMessage::SRV_GET_CALLSTACK_MSG;
						l_err = fOutFifo.Put(l_msg);
						xbox_assert( (l_err == VE_OK) );
again:
						l_err = fFifo.Get(&l_msg,&l_msg_found,0);

						l_tmp = CVSTR("VChromeDbgHdlPage::TreatWS page=");
						l_tmp.AppendLong(fPageNb);
						l_tmp += " fFifo.Get2 found=";
						l_tmp += ( l_msg_found ? "1" : "0" );
						l_tmp += ", type=";
						l_tmp.AppendLong8(l_msg.type);
						sPrivateLogHandler->Put( (l_err != VE_OK ? WAKDBG_ERROR_LEVEL : WAKDBG_INFO_LEVEL),l_tmp);

						xbox_assert( (l_err == VE_OK) );
						{
							if (!l_msg_found)
							{
								l_msg.type = NO_MSG;
							}
							if (l_msg.type == CALLSTACK_MSG)
							{
								XBOX::VError	 l_err;
								l_err = TreatPageReload(&l_msg.data._dataStr,l_src_id);
								if (!l_err)
								{
									//l_pilot->ContextUpdated(l_msg.data._debugContext);
								}
							}
							else
							{
								/*if (l_msg.type == SET_SOURCE_MSG)
								{
									// already treated (we consider source does not change for the moment
									goto again;
								}*/
								xbox_assert(false);
								DebugMsg("VChromeDbgHdlPage::TreatWS page=%d unexpected msg=%d\n",fPageNb,l_msg.type);
							}
						}
#endif
						break;
					default:
						DebugMsg("VChromeDbgHdlPage::TreatWS bad msg=%d\n",l_msg.type);
						l_tmp = CVSTR("VChromeDbgHdlPage::TreatWS bad msg=");
						l_tmp.AppendLong8(l_msg.type);
						sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,l_tmp);
						xbox_assert(false);
					}
				}
				continue;
			}
			//printf("ReadMessage len=%d term=%d\n",fMsgLen,fIsMsgTerminated);
			fOffset += fMsgLen;
			if (fIsMsgTerminated)
			{
				l_err = TreatMsg();
				if (l_err)
				{
					l_tmp = CVSTR("VChromeDbgHdlPage::TreatWS TreatMsg pb...closing");
					sPrivateLogHandler->Put(WAKDBG_ERROR_LEVEL,l_tmp);
					DebugMsg("VChromeDbgHdlPage::TreatWS TreatMsg pb...closing\n");
					fWS->Close();
				}
				fOffset = 0;
			}
		}
		else
		{
			DebugMsg("VChromeDbgHdlPage::TreatWS ReadMessage ERR!!!\n");
		}
	}
	fSem.Unlock();
	return l_err;
}
