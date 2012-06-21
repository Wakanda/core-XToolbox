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
#include "VChrmDebugHandler.h"
#include "VChrmDebugHandlerPrv.h"

USING_TOOLBOX_NAMESPACE


#define K_METHOD_STR		"{\"method\":"
#define K_PARAMS_STR		"\"params\":"
#define K_ID_STR			"\"id\":"
#define K_EXPRESSION_STR	"\"expression\":\""

const XBOX::VString			K_DQUOT_STR("\"");
const XBOX::VString			K_CHR_DBG_ROOT("/devtools");
const XBOX::VString			K_CHR_DBG_FILTER("/devtools/page/");
const XBOX::VString			K_CHR_DBG_JSON_FILTER("/devtools/json.wka");


const XBOX::VString			K_UNKNOWN_CMD("UNKNOWN_CMD");

const XBOX::VString			K_TIM_SET_INC_MEM_DET("Timeline.setIncludeMemoryDetails");

const XBOX::VString			K_DBG_PAU("Debugger.paused");

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
const XBOX::VString			K_LINE_FEED_STR("\\n");

#define K_NODE_ID_STR				"\"nodeId\":"

#define K_DBG_LINE_NUMBER_STR		"\"lineNumber\":"
#define K_DBG_COLUMN_STR			"\"columnNumber\":"
#define K_DBG_URL_STR				"\"url\":"

#define K_ID_FMT_STR				"},\"id\":"


const XBOX::VString			K_RESULT_TRUE_STR("\"result\":true");
const XBOX::VString			K_RESULT_FALSE_STR("\"result\":false");

const XBOX::VString			K_EMPTY_STR("");
const XBOX::VString			K_SEND_RESULT_STR1("{\"result\":{");
const XBOX::VString			K_SEND_RESULT_STR2("},\"id\":");

#define K_MAX_NODE_ID_SIZE		(16)

#define K_FRAME_ID_VALUE		"100"
#define K_LOADER_ID_VALUE		"200"
static uLONG8					s_node_id = 1000;


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
//function myWrite(test) {\n\t\n\tvar test1 = 123;\n\t\n\ttest2 = \\"GH\\";\n\tdocument.write('\u003Cb\u003EHello World '+test+' '+test2+' '+test1+' !!!!!\u003C/b\u003E');\n\n}\n\nmyWrite(3);\n\n\u003C/script\u003E\n\u003C/body\u003E\n\u003C/html\u003E\n\n\",\"base64Encoded\":false},\"id\":22}


	
//	"{\"result\":{\"body\":\"\\u003C!DOCTYPE HTML PUBLIC \\\"-//W3C//DTD HTML 4.01 Transitional//EN\\\" \\\"http://www.w3.org/TR/html4/loose.dtd\\\"\\u003E\\u003C!--DVID=00000745--\\u003E\\n\\u003CHTML\\u003E\\n\\u003Chead\\u003E\\n\\u003Ctitle\\u003E\\nHello World2\\n\\u003C/title\\u003E\\n\\u003C/HEAD\\u003E\\n\\u003Cbody\\u003E\\n\\u003Cscript type=\\\"text/javascript\\\"\\u003E\\n");
//document.write('Hello World 2!!!!!\\u003C/br\\u003E');\\n
//document.write('Hello World 3!!!!!\\u003C/br\\u003E');\\n
//\\u003C/script\\u003E\\n\\u003C/body\\u003E\\n\\u003C/html\\u003E\\n\\n\",\"base64Encoded\":false},\"id\":");

const XBOX::VString				K_DOM_PUS_NOD_BY_PAT_TO_FRO_END_STR1("{\"result\":{\"nodeId\":");


#define K_URL_VALUE				"file:///home/guy/Desktop/hw3.html"
//#define	K_URL_JS_VALUE			"file:///home/guy/Desktop/hw3.js"

const XBOX::VString				K_PAG_REL_STR1("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"13.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"documentURL\":\"" K_URL_VALUE "\",\"request\":{\"url\":\"" K_URL_VALUE "\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\"}},\"timestamp\":1330438948.88588,\"initiator\":{\"type\":\"other\"},\"stackTrace\":[]}}");
const XBOX::VString				K_PAG_REL_STR2("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"13.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"timestamp\":1330438948.890455,\"type\":\"Document\",\"response\":{\"url\":\"" K_URL_VALUE "\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"text/html\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1330438948.886399,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_STR3("{\"method\":\"Console.messagesCleared\"}");
const XBOX::VString				K_PAG_REL_STR4("{\"method\":\"Page.frameNavigated\",\"params\":{\"frame\":{\"id\":\"" K_FRAME_ID_VALUE "\",\"url\":\"" K_URL_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"mimeType\":\"text/html\"}}}");
const XBOX::VString				K_PAG_REL_STR5("{\"method\":\"Debugger.globalObjectCleared\"}");
const XBOX::VString				K_PAG_REL_STR6("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"13.16\",\"timestamp\":1330438938.902258,\"dataLength\":374,\"encodedDataLength\":0}}");
const XBOX::VString				K_PAG_REL_STR7("{\"method\":\"Debugger.scriptParsed\",\"params\":{\"scriptId\":\"141\",\"url\":\"" K_URL_VALUE "\",\"startLine\":8,\"startColumn\":31,\"endLine\":11,\"endColumn\":45}}");
const XBOX::VString				K_PAG_REL_STR8("{\"method\":\"Page.loadEventFired\",\"params\":{\"timestamp\":1330425280.213189}}");
const XBOX::VString				K_PAG_REL_STR9("{\"method\":\"DOM.documentUpdated\"}");
const XBOX::VString				K_PAG_REL_STR10("{\"method\":\"Page.domContentEventFired\",\"params\":{\"timestamp\":1330425280.214836}}");
const XBOX::VString				K_PAG_REL_STR11("{\"method\":\"Network.loadingFinished\",\"params\":{\"requestId\":\"13.16\",\"timestamp\":1330438948.889889}}");


const XBOX::VString				K_PAG_GET_RES_TRE_STR1A("{\"result\":{\"frameTree\":{\"frame\":{\"id\":\"" K_FRAME_ID_VALUE "\",\"url\":\"" K_URL_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"securityOrigin\":\"null\",\"mimeType\":\"text/html\"},\"resources\":[{\"url\":\"");
const XBOX::VString				K_PAG_GET_RES_TRE_STR1B("\",\"type\":\"Script\",\"mimeType\":\"application/javascript\"}]}},\"id\":");

/*const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"55.22\",\"frameId\":\"55.1\",\"loaderId\":\"55.16\",\"documentURL\":\"file:///home/guy/Desktop/hw3.html\",\"request\":{\"url\":\"file:///home/guy/Desktop/hw3.html\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\"}},\"timestamp\":1330613160.281554,\"initiator\":{\"type\":\"script\",\"stackTrace\":[{\"functionName\":\"\",\"url\":\"file:///home/guy/Desktop/hw3.html\",\"lineNumber\":20,\"columnNumber\":1}]},\"stackTrace\":[{\"functionName\":\"\",\"url\":\"file:///home/guy/Desktop/hw3.html\",\"lineNumber\":20,\"columnNumber\":1}]}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR2("{\"method\":\"Debugger.resumed\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR3("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"55.22\",\"frameId\":\"55.1\",\"loaderId\":\"55.16\",\"timestamp\":1330613160.288625,\"type\":\"Document\",\"response\":{\"url\":\"file:///home/guy/Desktop/hw3.html\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"text/html\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1330613160.281925,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR4("{\"method\":\"Console.messagesCleared\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR5("{\"method\":\"Page.frameNavigated\",\"params\":{\"frame\":{\"id\":\"55.1\",\"url\":\"file:///home/guy/Desktop/hw3.html\",\"loaderId\":\"55.16\",\"mimeType\":\"text/html\"}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR6("{\"method\":\"Debugger.globalObjectCleared\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"55.22\",\"timestamp\":1330613160.30471,\"dataLength\":394,\"encodedDataLength\":0}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR8("{\"method\":\"Debugger.scriptParsed\",\"params\":{\"scriptId\":\"141\",\"url\":\"file:///home/guy/Desktop/hw3.html\",\"startLine\":8,\"startColumn\":31,\"endLine\":20,\"endColumn\":1}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9("{\"method\":\"Debugger.breakpointResolved\",\"params\":{\"breakpointId\":\"file:///home/guy/Desktop/hw3.html:19:0\",\"location\":{\"scriptId\":\"141\",\"lineNumber\":19,\"columnNumber\":0}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR10("{\"method\":\"Debugger.paused\",\"params\":{\"callFrames\":[{\"callFrameId\":\"{\\\"ordinal\\\":0,\\\"injectedScriptId\\\":11}\",\"functionName\":\"\",\"location\":{\"scriptId\":\"141\",\"lineNumber\":19,\"columnNumber\":0},\"scopeChain\":[{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":11,\\\"id\\\":1}\",\"className\":\"DOMWindow\",\"description\":\"DOMWindow\"},\"type\":\"global\"}],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":11,\\\"id\\\":2}\",\"className\":\"DOMWindow\",\"description\":\"DOMWindow\"}}],\"reason\":\"other\"}}");
*/

const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1A("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"30.15\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"documentURL\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1B("\",\"request\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR1C("\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\"}},\"timestamp\":1331300225.457657,\"initiator\":{\"type\":\"other\"},\"stackTrace\":[]}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR2("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"30.15\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"timestamp\":1331300225.460701,\"type\":\"Document\",\"response\":{\"url\":\"" K_URL_VALUE "\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"text/html\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1331300225.458127,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR3("{\"method\":\"Console.messagesCleared\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR4("{\"method\":\"Page.frameNavigated\",\"params\":{\"frame\":{\"id\":\"" K_FRAME_ID_VALUE "\",\"url\":\"" K_URL_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"mimeType\":\"text/html\"}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR5("{\"method\":\"Debugger.globalObjectCleared\"}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR6("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"30.15\",\"timestamp\":1331300225.473018,\"dataLength\":252,\"encodedDataLength\":0}}");

const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A1("{\"method\":\"Network.requestWillBeSent\",\"params\":{\"requestId\":\"30.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"documentURL\":\"" K_URL_VALUE "\",\"request\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7A2("\",\"method\":\"GET\",\"headers\":{\"User-Agent\":\"Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/535.11 (KHTML, like Gecko) Chrome/17.0.963.56 Safari/535.11\",\"Accept\":\"*/*\"}},\"timestamp\":1331300225.473645,\"initiator\":{\"type\":\"parser\",\"url\":\"" K_URL_VALUE "\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR7B("},\"stackTrace\":[]}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR8("{\"method\":\"Network.loadingFinished\",\"params\":{\"requestId\":\"30.15\",\"timestamp\":1331300225.463939}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9A("{\"method\":\"Network.responseReceived\",\"params\":{\"requestId\":\"30.16\",\"frameId\":\"" K_FRAME_ID_VALUE "\",\"loaderId\":\"" K_LOADER_ID_VALUE "\",\"timestamp\":1331300225.475966,\"type\":\"Script\",\"response\":{\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR9B("\",\"status\":0,\"statusText\":\"\",\"mimeType\":\"application/javascript\",\"connectionReused\":false,\"connectionId\":0,\"fromDiskCache\":false,\"timing\":{\"requestTime\":1331300225.473833,\"proxyStart\":-1,\"proxyEnd\":-1,\"dnsStart\":-1,\"dnsEnd\":-1,\"connectStart\":-1,\"connectEnd\":-1,\"sslStart\":-1,\"sslEnd\":-1,\"sendStart\":0,\"sendEnd\":0,\"receiveHeadersEnd\":0},\"headers\":{},\"requestHeaders\":{}}}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR10("{\"method\":\"Network.dataReceived\",\"params\":{\"requestId\":\"30.16\",\"timestamp\":1331300225.476164,\"dataLength\":154,\"encodedDataLength\":0}}");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11A("{\"method\":\"Debugger.scriptParsed\",\"params\":{\"scriptId\":\"141\",\"url\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR11B("\",\"startLine\":0,\"startColumn\":0,\"endLine\":11,\"endColumn\":1}}");	
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A1("{\"method\":\"Debugger.breakpointResolved\",\"params\":{\"breakpointId\":\"");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12A2(":6:0\",\"location\":{\"scriptId\":\"141\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR12B(",\"columnNumber\":1}}}");

const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13A(",\"params\":{\"callFrames\":[{\"callFrameId\":\"{\\\"ordinal\\\":0,\\\"injectedScriptId\\\":3} \",\"functionName\":\"myWrite\",\"location\":{\"scriptId\":\"141\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13B(",\"columnNumber\":1},\"scopeChain\":[{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":1}\",\"className\":\"Object\",\"description\":\"Object\"},\"type\":\"local\"},{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":2}\",\"className\":\"Window\",\"description\":\"Window\"},\"type\":\"global\"}],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":3}\",\"className\":\"Window\",\"description\":\"Window\"}},{\"callFrameId\":\"{\\\"ordinal\\\":1,\\\"injectedScriptId\\\":3}\",\"functionName\":\"\",\"location\":{\"scriptId\":\"41\",\"lineNumber\":");
const XBOX::VString				K_PAG_REL_AFT_BKPT_STR13C(",\"columnNumber\":0},\"scopeChain\":[{\"object\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":4}\",\"className\":\"Window\",\"description\":\"Window\"},\"type\":\"global\"}],\"this\":{\"type\":\"object\",\"objectId\":\"{\\\"injectedScriptId\\\":3,\\\"id\\\":5}\",\"className\":\"Window\",\"description\":\"Window\"}}],\"reason\":\"other\"}}");

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
const XBOX::VString				K_GET_DOC_STR7(",\"nodeType\":1,\"nodeName\":\"BODY\",\"localName\":\"body\",\"nodeValue\":\"\",\"childNodeCount\":4,\"attributes\":[]}],\"attributes\":[]}],\"documentURL\":\"" K_URL_VALUE "\",\"xmlVersion\":\"\"}},\"id\":");

const XBOX::VString				K_DBG_GET_SCR_SOU_STR1("{\"result\":{\"scriptSource\":\"\\n");

const XBOX::VString				K_RUN_GET_PRO_STR1("{\"result\":{\"result\":[{\"value\":{\"type\":\"number\",\"value\":123,\"description\":\"123\"},\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"test1\"},{\"value\":{\"type\":\"number\",\"value\":3,\"description\":\"3\"},\"writable\":true,\"enumerable\":true,\"configurable\":true,\"name\":\"test\"}]},\"id\":");
//const XBOX::VString				K_RUN_GET_PRO_STR2("{\"result\":{\"result\":[{\"value\":{\"type\":\"function\",\"objectId\":\"{\\\"injectedScriptId\\\":11,\\\"id\\\":26}\",\"className\":\"Object\",\"description\":\"function myWrite(test) {\n\t\n\tvar test1 = 123;\n\t\n\ttest2 = \\\"GH\\\";\n\tdocument.write('\\u003Cb\\u003EHello World '+test+' '+test2+' '+test1+' !!!!!\\u003C/b\\u003E');\\n\\n}\"},\"writable\":true,\"enumerable\":true,\"configurable\":false,\"name\":\"myWrite\"}]},\"id\":");

#if defined(WKA_USE_CHR_REM_DBG)
	VChrmDebugHandler*				VChrmDebugHandler::sDebugger=NULL;


static bool		s_vchrm_debug_handler_init = false;

static VString		s_brkpt_line_nb = VString("");
static VString		s_brkpt_col_nb = VString("");
static VString		s_brkpt_url = VString("");


ChrmDbgHdlPage::ChrmDbgHdlPage() : fURL(K_URL_VALUE)
{
	fPageNb = (uLONG)-1;
}

ChrmDbgHdlPage::~ChrmDbgHdlPage()
{
	xbox_assert(false);
}

void ChrmDbgHdlPage::Init(uLONG inPageNb,CHTTPServer* inHTTPSrv,XBOX::VCppMemMgr* inMemMgr)
{
	fWS = inHTTPSrv->NewHTTPWebsocketHandler();
	fActive = false;
	fUsed = false;
	fPageNb = inPageNb;
	fMemMgr = inMemMgr;
}

VChrmDebugHandler* VChrmDebugHandler::Get( )
{
	if ( sDebugger == 0 )
	{
		sDebugger = new VChrmDebugHandler();
		XBOX::RetainRefCountable(sDebugger);
	}
	return sDebugger;
}

VChrmDebugHandler::VChrmDebugHandler() : fMemMgr(VObject::GetMainMemMgr())
{
	if (!s_vchrm_debug_handler_init)
	{
		CHTTPServer *httpServer = VComponentManager::RetainComponent< CHTTPServer >();
		for( int l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
		{
			fPages[l_i].Init(l_i,httpServer,fMemMgr);
		}
		httpServer->Release();
		s_vchrm_debug_handler_init = true;
	}
	else
	{
		//fWS = NULL;
		//DebugMsg("VChrmDebugHandler should be a singleton\n");
		xbox_assert(0);
	}
}



VChrmDebugHandler::~VChrmDebugHandler()
{
	xbox_assert(false);
	/*if (fWS)
	{
		if (fWS->Close())
		{
			DebugMsg("~VChrmDebugHandler error closing websocket\n");
		}
		VTask::Sleep(1500);
		s_vchrm_debug_handler_init = false;
		fWS = NULL;
	}*/
}




/*
   checks the syntax of the command requested by the client. Syntax is:
	{"method":"name_of_method",["params":{"name_of_param":"value_of_param},]"id":"number"}
*/
XBOX::VError ChrmDbgHdlPage::CheckInputData()
{
	int				l_pos;
	char*			l_tmp;
	char*			l_method;
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




XBOX::VError ChrmDbgHdlPage::SendMsg(const XBOX::VString& inValue)
{
	XBOX::VError		l_err;
	VSize				l_size;
	char*				l_buff;

	l_err = VE_OK;
	l_size = inValue.GetLength() * 3; /* overhead between VString's UTF_16 to CHAR* UTF_8 */ 
	/* malloc costly in mem qty and fragmentation but only performed for debug */
DebugMsg("len=%d size=%d\n",inValue.GetLength(),l_size);
	l_buff = (char *)fMemMgr->Malloc( l_size, false, 'memb');
	if (!l_buff)
	{
		l_err = VE_MEMORY_FULL;
	}
	if (!l_err)
	{
		l_size = inValue.ToBlock(l_buff,l_size,VTC_UTF_8,false,false);
		//DebugMsg("writ_size=%d len=%d\n",l_size,inValue.GetLength());

/*
DebugMsg("\n\n\n <<<<");
for(int l_i=0; l_i<l_size; l_i++)
{
	DebugMsg("%c",fBuff[l_i]);
}
DebugMsg(">>>>>\n\n\n");
DebugMsg("\n\n\n <<<<%S",&inValue);
DebugMsg(">>>>>\n\n\n");
DebugMsg("\n\n\n");
*/

		DebugMsg(" treat '%S' ... resp-><%S>\n",&fMethod,&inValue);
		l_err = fWS->WriteMessage( l_buff, l_size, true );

	}
	if (l_buff)
	{
		fMemMgr->Free(l_buff);
	}
	return l_err;
}

XBOX::VError ChrmDbgHdlPage::SendResult(const XBOX::VString& inValue)
{
	VString				l_tmp;

	l_tmp = K_SEND_RESULT_STR1;
	l_tmp += inValue;
	l_tmp += K_SEND_RESULT_STR2;
	l_tmp += fId;
	l_tmp += "}";

	return SendMsg(l_tmp);

}

XBOX::VError ChrmDbgHdlPage::TreatPageReload()
{
	XBOX::VError		l_err;
	XBOX::VString		l_resp;
	l_resp = K_PAG_REL_AFT_BKPT_STR1A;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR1B;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR1C;
	l_err = SendMsg(l_resp);
	if (!l_err)
	{
		l_err = SendResult(K_EMPTY_STR);
		l_resp = K_PAG_REL_AFT_BKPT_STR2;
	}
	if (!l_err)
	{
		l_err = SendMsg(l_resp);
		l_resp = K_PAG_REL_AFT_BKPT_STR3;
	}
	if (!l_err)
	{
		l_err = SendMsg(l_resp);
		l_resp = K_PAG_REL_AFT_BKPT_STR4;
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
		l_resp += fURL;
		l_resp += K_PAG_REL_AFT_BKPT_STR7A2;
		l_resp.AppendULong8(fPageNb);
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
		l_resp += fURL;
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
		l_resp += fURL;
		l_resp += K_PAG_REL_AFT_BKPT_STR11B;
	}
	if (!l_err)
	{
		l_err = SendMsg(l_resp);
		l_resp = K_PAG_REL_AFT_BKPT_STR12A1;
		l_resp += fURL;
		l_resp += K_PAG_REL_AFT_BKPT_STR12A2;
		l_resp.AppendULong8(fPageNb);
		l_resp += K_PAG_REL_AFT_BKPT_STR12B;
	}
	if (!l_err)
	{
		l_err = SendMsg(l_resp);
		l_resp = K_METHOD_STR;
		l_resp += K_DQUOT_STR;
		l_resp += K_DBG_PAU;
		l_resp += K_DQUOT_STR;
		l_resp += K_PAG_REL_AFT_BKPT_STR13A;
		l_resp.AppendULong8(fLineNb);
		l_resp += K_PAG_REL_AFT_BKPT_STR13B;
		l_resp.AppendULong8(fLineNb);
		l_resp += K_PAG_REL_AFT_BKPT_STR13C;
	}
	if (!l_err)
	{
		l_err = SendMsg(l_resp);
	}
	return l_err;
}

XBOX::VError ChrmDbgHdlPage::TreatMsg()
{		
	XBOX::VError		l_err;
	XBOX::VString		l_cmd;
	XBOX::VString		l_resp;
	bool				l_found;
	char*				l_id;

	l_err = VE_OK;

	l_cmd = K_UNKNOWN_CMD;
	l_found = false;
	l_err = CheckInputData();

	if (!l_err)
	{
		DebugMsg("VChrmDebugHandler::TreatMsg Method='%S', Id='%s',Params='%s'\n",&fMethod,fId,(fParams ? fParams : "."));
	}
	else
	{
		DebugMsg("VChrmDebugHandler::TreatMsg CheckInputData pb !!!!\n");
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
		l_err = SendResult(K_EMPTY_STR);
	}
	l_cmd = K_DBG_EVA_ON_CAL_FRA;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		char	*l_tmp_exp;
		l_tmp_exp = strstr(fParams,K_EXPRESSION_STR);
		
		if (l_tmp_exp)
		{
			l_tmp_exp += strlen(K_EXPRESSION_STR);
			char *l_end_str;
			l_end_str = strchr(l_tmp_exp,'"');
			if (l_end_str)
			{
				*l_end_str = 0;
				VString l_exp(l_tmp_exp);
				DebugMsg("requiring value of '%S'\n",&l_exp);
				l_resp = CVSTR("\"result\":{\"type\":\"number\",\"value\":3,\"description\":\"3\"}");
				l_err = SendResult(l_resp);
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
		char	*l_line_nb;
		char	*l_col_nb;
		char	*l_url;
		l_line_nb = strstr(fParams,K_DBG_LINE_NUMBER_STR);
		l_col_nb = strstr(fParams,K_DBG_COLUMN_STR);
		l_url = strstr(fParams,K_DBG_URL_STR);
		char	*l_end;

		/* GH faire une fonction */
		l_url += strlen(K_DBG_URL_STR);
		l_end = strchr(l_url,',');
		if (l_end)
		{
			*l_end = 0;
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
		l_col_nb += strlen(K_DBG_COLUMN_STR);
		l_end = strchr(l_col_nb,',');
		if (!l_err && l_end)
		{
			*l_end = 0;
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
		l_line_nb += strlen(K_DBG_LINE_NUMBER_STR);
		l_end = strchr(l_line_nb,',');
		if (!l_err && l_end)
		{
			*l_end = 0;
		}
		else
		{
			l_err = VE_INVALID_PARAMETER;
		}
		if (l_err || !l_line_nb || !l_col_nb || !l_url ) 
		{
			DebugMsg("bad params for cmd <%S>\n",&fMethod);
			l_err = VE_INVALID_PARAMETER;
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
			l_err = SendMsg(l_resp);
			s_brkpt_line_nb = VString(l_line_nb);
			s_brkpt_col_nb = VString(l_col_nb);
			s_brkpt_url = VString(l_url);
	
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
		l_resp.AppendULong8(fBodyNodeId);
		l_resp += K_ID_FMT_STR;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_PAG_REL;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
if (s_brkpt_line_nb.GetLength())
{
	return TreatPageReload();
	/*l_resp = K_PAG_REL_AFT_BKPT_STR1A;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR1B;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR1C;
	l_err = SendMsg(l_resp);
	l_err = SendResult(K_EMPTY_STR);
	l_resp = K_PAG_REL_AFT_BKPT_STR2;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR3;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR4;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR5;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR6;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR7A1;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR7A2;
	l_resp.AppendULong8(fPageNb);
	l_resp += K_PAG_REL_AFT_BKPT_STR7B;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR8;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR9A;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR9B;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR10;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR11A;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR11B;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR12A1;
	l_resp += fURL;
	l_resp += K_PAG_REL_AFT_BKPT_STR12A2;
	l_resp.AppendULong8(fPageNb);
	l_resp += K_PAG_REL_AFT_BKPT_STR12B;
	l_err = SendMsg(l_resp);
	l_resp = K_PAG_REL_AFT_BKPT_STR13A;
	l_resp.AppendULong8(fLineNb);
	l_resp += K_PAG_REL_AFT_BKPT_STR13B;
	l_resp.AppendULong8(fLineNb);
	l_resp += K_PAG_REL_AFT_BKPT_STR13C;
	l_err = SendMsg(l_resp);*/
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
	}

	l_cmd = K_DBG_GET_SCR_SOU;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		/*l_resp = VString("{\"method\":\"DOMStorage.addDOMStorage\",\"params\":{\"storage\":{\"host\":\"\",\"isLocalStorage\":false,\"id\":");
		l_resp.AppendULong8(s_node_id++);
		l_resp += "}}}";
		l_err = SendMsg(l_resp);
		l_resp = VString("{\"method\":\"DOMStorage.addDOMStorage\",\"params\":{\"storage\":{\"host\":\"\",\"isLocalStorage\":true,\"id\":");
		l_resp.AppendULong8(s_node_id++);
		l_resp += "}}}";
		l_err = SendMsg(l_resp);*/
		l_resp = K_DBG_GET_SCR_SOU_STR1;
		/*l_resp += K_LINE_1;
		l_resp += K_LINE_2;
		l_resp += K_LINE_3;
		l_resp += K_LINE_4;
		l_resp += K_LINE_5;
		l_resp += K_LINE_6;*/

		VectorOfVString::const_iterator l_it = fSource.begin();
		for(; l_it != fSource.end(); l_it++)
		{
			l_resp += *l_it;
			l_resp += K_LINE_FEED_STR;
		}
		l_resp += "\"},\"id\":";
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
	}
	
	l_cmd = K_RUN_GET_PRO;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_resp = K_RUN_GET_PRO_STR1;
		l_resp += fId;
		l_resp += "}";
		l_err = SendMsg(l_resp);
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
			l_resp.AppendULong8(fBodyNodeId);
			l_resp += K_DOM_REQ_CHI_NOD_STR2;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR3;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR4A;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4B;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4C;
			//l_resp += K_DOM_REQ_CHI_NOD_STR4D;
#if 1
			l_resp += "n";
			VectorOfVString::const_iterator l_it = fSource.begin();
			for(; l_it != fSource.end(); l_it++)
			{
				l_resp += *l_it;
				l_resp += K_LINE_FEED_STR;
			}
			l_resp+= "\\";
#endif
			l_resp += K_DOM_REQ_CHI_NOD_STR4E;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR5;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR6;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR7;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR8;
			l_resp.AppendULong8(s_node_id++);
			l_resp += K_DOM_REQ_CHI_NOD_STR9;
			l_resp.AppendULong8(s_node_id++);
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
		l_resp.AppendULong8(s_node_id++);
		l_resp += K_GET_DOC_STR2;
		l_resp.AppendULong8(s_node_id++);
		l_resp += K_GET_DOC_STR3;
		l_resp.AppendULong8(s_node_id++);
		l_resp += K_GET_DOC_STR4;
		l_resp.AppendULong8(s_node_id++);
		l_resp += K_GET_DOC_STR5;
		l_resp.AppendULong8(s_node_id++);
		l_resp += K_GET_DOC_STR6;
		fBodyNodeId = s_node_id;
		l_resp.AppendULong8(s_node_id++);
		l_resp += K_GET_DOC_STR7;
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
		
		l_resp = VString("{\"method\":\"Debugger.scriptParsed\",\"params\":{\"scriptId\":\"141\",\"url\":\"");
		l_resp += fURL;
		l_resp += VString("\",\"startLine\":0,\"startColumn\":0,\"endLine\":11,\"endColumn\":1}}");						
		l_err = SendMsg(l_resp);
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
		l_resp += fURL;
		l_resp = K_PAG_GET_RES_TRE_STR1B;
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

	l_cmd = K_DBG_CAU_REC;
	if (!l_err && !l_found && (l_found = fMethod.EqualToString(l_cmd)) ) {
		l_err = SendResult(K_RESULT_FALSE_STR);
	}
	if (!l_found)
	{
		DebugMsg("VChrmDebugHandler::TreatMsg Method='%S' UNKNOWN!!!\n",&fMethod);
		exit(-1);
		return VE_OK;// --> pour l'instant on renvoit OK  !!! VE_INVALID_PARAMETER;
	}
#endif
	return l_err;

}

XBOX::VError VChrmDebugHandler::TreatJSONFile(IHTTPResponse* ioResponse)
{
	XBOX::VError		l_err;
	XBOX::VString		l_resp;
	VSize				l_size;
	char*				l_buff;
	std::vector< XBOX::VString >	l_addrs;

	l_err = VE_OK;
	if (ioResponse->GetRequestMethod() != HTTP_GET)
	{
		l_err = VE_INVALID_PARAMETER;
	}
	if (!l_err)
	{
#if WITH_DEPRECATED_IPV4_API
		GetLocalIPAdresses(l_addrs);
#elif DEPRECATED_IPV4_API_SHOULD_NOT_COMPILE
		#error NEED AN IP V6 UPDATE
#endif

		if (l_addrs.size() < 1)
		{
			xbox_assert(false);
			DebugMsg("VChrmDebugHandler::TreatJSONFile network config pb\n");
			l_err = VE_UNKNOWN_ERROR;
		}
		else
		{
			std::vector< XBOX::VString >::iterator	l_it=l_addrs.begin();
			l_resp = CVSTR("[");
			for( uLONG8 l_i=0; l_i<K_NB_MAX_PAGES; )
			{
				l_resp += CVSTR("{\n");
				l_resp += CVSTR("\"devtoolsFrontendUrl\": \"/wka_dbg/devtools/devtools.html?host=");
				l_resp += *l_it;
				l_resp += CVSTR(":8080&page=");
				l_resp.AppendULong8(l_i);
				l_resp += CVSTR("\",\n");
				l_resp += CVSTR("\"faviconUrl\": \"\",\n");
				l_resp += CVSTR("\"thumbnailUrl\": \"/thumb/file:///Users/ghermann/Perforce/guy.hermann_Mac/depot/Wakanda/main/Server/Resources/Default%20Solution/Admin/WebFolder/hw3.html\",\n");
				l_resp += CVSTR("\"title\": \"MainThread\",\n");
				l_resp += CVSTR("\"url\": \"file:///Users/ghermann/Perforce/guy.hermann_Mac/depot/Wakanda/main/Server/Resources/Default%20Solution/Admin/WebFolder/hw3.html\",\n");
				l_resp += CVSTR("\"webSocketDebuggerUrl\": \"ws://");
				l_resp += *l_it;
				l_resp += CVSTR(":8080/devtools/page/");
				l_resp.AppendULong8(l_i);
				l_resp += CVSTR("\"\n");
				l_i++;
				if (l_i >= K_NB_MAX_PAGES)
				{
					l_resp += CVSTR("}");
				}
				else
				{
					l_resp += CVSTR("}, ");
				}
			}
			l_resp += CVSTR("]");
			l_size = l_resp.GetLength() * 3; /* overhead between VString's UTF_16 to CHAR* UTF_8 */
			l_buff = (char *)fMemMgr->Malloc( l_size, false, 'memb');
			if (l_buff)
			{
				l_size = l_resp.ToBlock(l_buff,l_size,VTC_UTF_8,false,false);
				l_err = ioResponse->SetResponseBody(l_buff,l_size);
			}
			if (!l_buff)
			{
				l_err = VE_MEMORY_FULL;
			}
			else
			{
				fMemMgr->Free(l_buff);
			}
		}
	}
	return l_err;
}

XBOX::VError ChrmDbgHdlPage::TreatWS(IHTTPResponse* ioResponse)
{
	XBOX::VError	l_err;

	l_err = fWS->TreatNewConnection(ioResponse);

	fOffset = 0;
	while(!l_err)
	{
		fMsgLen = K_MAX_SIZE - fOffset;
		l_err = fWS->ReadMessage(fMsgData+fOffset,&fMsgLen,&fIsMsgTerminated);
		
		if (!l_err)
		{
			if (!fMsgLen)
			{
				VTask::Sleep(1000);
				continue;
			}
			//printf("ReadMessage len=%d term=%d\n",fMsgLen,fIsMsgTerminated);
			fOffset += fMsgLen;
			if (fIsMsgTerminated)
			{
				l_err = TreatMsg();
				if (l_err)
				{
					DebugMsg("ChrmDbgHdlPage::TreatWS TreatMsg pb...closing\n");
					fWS->Close();
				}
				fOffset = 0;
			}
		}
		else
		{
			DebugMsg("ReadMessage ERR!!!\n");
		}

	}

	return l_err;
}

XBOX::VError VChrmDebugHandler::HandleRequest( IHTTPResponse* ioResponse )
{
	XBOX::VError	l_err;
	uLONG			l_page_nb;

	DebugMsg("VChrmDebugHandler::HandleRequest called\n");

	if (ioResponse == NULL)
		return VE_INVALID_PARAMETER;

	VString			l_url = ioResponse->GetRequest().GetURL();
	VString			l_tmp;
	VIndex			l_i;
	
	l_err = VE_OK;
	l_i = l_url.Find(K_CHR_DBG_JSON_FILTER);
	if (l_i > 0)
	{
		l_err = TreatJSONFile(ioResponse);
	}
	else
	{
		l_i = l_url.Find(K_CHR_DBG_FILTER);
		if (l_i > 0)
		{
			l_url.GetSubString(
				l_i+K_CHR_DBG_FILTER.GetLength(),
				l_url.GetLength() - K_CHR_DBG_FILTER.GetLength(),
				l_tmp);

			l_page_nb = (uLONG)l_tmp.GetLong();
			if (l_page_nb >= K_NB_MAX_PAGES)
			{
				l_err = XBOX::VE_INVALID_PARAMETER;
			}
			else
			{
				fLock.Lock();
				if (fPages[l_page_nb].fActive)
				{
					xbox_assert(false);
					l_err = XBOX::VE_INVALID_PARAMETER;
				}
				else
				{
					fPages[l_page_nb].fActive = true;
				}
				fLock.Unlock();
				if (!l_err)
				{
					l_err = fPages[l_page_nb].TreatWS(ioResponse);
					fLock.Lock();
					fPages[l_page_nb].fActive = false;
					fLock.Unlock();
				}
			}
		}
		else
		{
			l_err = XBOX::VE_INVALID_PARAMETER;
		}
	}
	return l_err;

}


XBOX::VError VChrmDebugHandler::GetPatterns(XBOX::VectorOfVString *outPatterns) const
{
	if (NULL == outPatterns)
		return VE_HTTP_INVALID_ARGUMENT;
	outPatterns->clear();
	outPatterns->push_back(K_CHR_DBG_ROOT);
	return XBOX::VE_OK;
}


bool VChrmDebugHandler::HasClients()
{
	return true;
	/*
	bool	l_res;
	l_res = false;
	fLock.Lock();
	for( int l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
	{
		if (fPages[l_i].fActive)
		{
			l_res = true;
			break;
		}
	}
	fLock.Unlock();
	return l_res;*/
}

void* VChrmDebugHandler::GetCtx()
{
	void*		l_res;
	
	l_res = NULL;
	fLock.Lock();
	for( int l_i=0; l_i<K_NB_MAX_PAGES; l_i++ )
	{
		if (/*fPages[l_i].fActive &&*/ !fPages[l_i].fUsed)
		{
			fPages[l_i].fUsed = true;
			l_res = (void *)(fPages+l_i);
			DebugMsg("GetCtx granted page%d\n",l_i);
			break;
		}
	}
	fLock.Unlock();
	if (!l_res)
	{
		DebugMsg("VChrmDebugHandler::GetCtx not enough resources\n");
		xbox_assert(false);
	}
	return l_res;
}
bool VChrmDebugHandler::ReleaseCtx(void* inContext)
{
	bool	l_res;

	l_res = false;
	if (!inContext)
	{
		return l_res;
	}
	ChrmDbgHdlPage	*l_page;
	fLock.Lock();
	l_page = (ChrmDbgHdlPage *)inContext;
	if (l_page->fUsed)
	{
		if (l_page->fActive)
		{
			//l_page->fWS->Close();
		}
		l_page->fSource.clear();
		l_page->fUsed = false;
		l_res = true;
	}
	else
	{
		xbox_assert(false);
	}
	fLock.Unlock();
	return l_res;
}

int VChrmDebugHandler::WaitForClientCommand( void* inContext )
{
	int		l_res;
	l_res = 0;
	if (!inContext)
	{
		return l_res;
	}
	//VTask::Sleep(100000000);
	return l_res;
}
bool VChrmDebugHandler::BreakpointReached(void* inContext, int inLineNb)
{
	bool	l_res;

	l_res = false;
	if (!inContext)
	{
		return l_res;
	}
	ChrmDbgHdlPage	*l_page;
	fLock.Lock();
	l_page = (ChrmDbgHdlPage *)inContext;
	l_page->fLineNb = inLineNb;
	XBOX::VError	 l_err;
	l_err = l_page->TreatPageReload();
	fLock.Unlock();
	return l_res;
}
bool VChrmDebugHandler::SetSource( void* inContext, const char *inData, int inLength, const char* inURL, unsigned int inURLLen )
{
	bool	l_res;

	l_res = false;
	if (!inContext)
	{
		return l_res;
	}

	ChrmDbgHdlPage	*l_page;
	VString			l_src;

	fLock.Lock();
	l_page = (ChrmDbgHdlPage *)inContext;
	l_src.Clear();
	l_src = VString( inData, inLength*2,  VTC_UTF_16 );
	VIndex l_len = l_src.GetLength();

	if (l_page->GetPageNb() < K_NB_MAX_PAGES)
	{
		l_page->fURL.Clear();
		l_page->fURL.AppendBlock( inURL, inURLLen*2, VTC_UTF_16 );
		VURL::Decode( l_page->fURL );
		l_res = l_src.GetSubStrings(K_LF_STR,l_page->fSource);
		if (l_res)
		{
			VectorOfVString::iterator l_it = l_page->fSource.begin();
			for(; (l_res && (l_it != l_page->fSource.end())); l_it++)
			{
				VString l_tmp = *l_it;
				if (l_tmp.GetJSONString(*l_it) != VE_OK)
				{
					l_res = false;
					xbox_assert(false);
				}
			}
		}
	}
	else
	{
		DebugMsg("VChrmDebugHandler::SetSource corrupted data\n");
		xbox_assert(false);
	}
	fLock.Unlock();
	return l_res;
}

#else
// minimal stubs when not WKA_USE_CHR_REM_DBG

XBOX::VError VChrmDebugHandler::HandleRequest( IHTTPResponse* ioResponse )
{

	return VE_UNKNOWN_ERROR;

}


XBOX::VError VChrmDebugHandler::GetPatterns(XBOX::VectorOfVString *outPatterns) const
{

	return VE_HTTP_INVALID_ARGUMENT;
}
bool VChrmDebugHandler::HasClients()
{
	return false;
}

#endif
