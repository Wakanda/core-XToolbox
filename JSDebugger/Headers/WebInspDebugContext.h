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
#ifndef __WEBINSP_DEBUG_CONTEXT__
#define __WEBINSP_DEBUG_CONTEXT__

#include "HTTPServer/Interfaces/CHTTPServer.h"
#include "ChrmDbgHdlPage.h"

class WebInspDebugContext {

public:
static bool						Init(	CHTTPServer*			inHTTPServer,
										const XBOX::VString		inIP,
										uLONG					inPort,
										XBOX::VCppMemMgr*		inMemMgr);

static WebInspDebugContext*		Get();

static WebInspDebugContext*		Check(WebInspDebugContext* inContext);

static ChrmDbgHdlPage*			GetPageById(uLONG inPageNb);

	unsigned int			GetId() const {return fId;}
	ChrmDbgHdlPage*			GetPage();
	bool					GetURL();
	bool					WaitUntilActivated(bool* outAlreadyActivated);
	void					SetPage( ChrmDbgHdlPage* inPage);
	void					Release();
	

private:

typedef enum WebInspDebugContextMsgType_enum {
		GET_URL_MSG,
} WebInspDebugContextMsgType_t;

typedef struct WebInspDebugContextMsg_st {
	WebInspDebugContextMsgType_t			type;
	unsigned int					id;
} WebInspDebugContextMsg_t;

	WebInspDebugContext();
	~WebInspDebugContext();
	//TemplProtectedFifo<DebugContextMsg_t>		fFifo;
	uLONG										fPageNb;//ChrmDbgHdlPage*								fPage;
	bool										fUsed;
	unsigned int								fId;

static unsigned int								sDebugContextId;
static WebInspDebugContext*						sContextArray;
static XBOX::VCriticalSection					sLock;
#define K_NB_MAX_PAGES		(8)

typedef struct PageDescriptor_st {
	bool				used;
	ChrmDbgHdlPage		value;
} PageDescriptor_t;
static PageDescriptor_t*						sPages;



};

#endif

