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
#ifndef __VXML_HTTP_REQUEST__
#define __VXML_HTTP_REQUEST__



#include "JavaScript/VJavaScript.h"


const OsType kXHR_SIGNATURE='XHRQ';

const XBOX::VError VE_XHRQ_SYNTAX_ERROR             = MAKE_VERROR(kXHR_SIGNATURE, 1000);    //thrown
const XBOX::VError VE_XHRQ_SECURITY_ERROR           = MAKE_VERROR(kXHR_SIGNATURE, 1001);    //thrown
const XBOX::VError VE_XHRQ_NOT_SUPPORTED_ERROR      = MAKE_VERROR(kXHR_SIGNATURE, 1002);    //thrown
const XBOX::VError VE_XHRQ_INVALID_STATE_ERROR      = MAKE_VERROR(kXHR_SIGNATURE, 1003);    //thrown
const XBOX::VError VE_XHRQ_JS_BINDING_ERROR         = MAKE_VERROR(kXHR_SIGNATURE, 1004);    //thrown
const XBOX::VError VE_XHRQ_SEND_ERROR               = MAKE_VERROR(kXHR_SIGNATURE, 1005);    //thrown
const XBOX::VError VE_XHRQ_NO_HEADER_ERROR          = MAKE_VERROR(kXHR_SIGNATURE, 1006);    //not thrown
const XBOX::VError VE_XHRQ_NO_RESPONSE_CODE_ERROR   = MAKE_VERROR(kXHR_SIGNATURE, 1007);    //not thrown
const XBOX::VError VE_XHRQ_IMPL_FAIL_ERROR          = MAKE_VERROR(kXHR_SIGNATURE, 1008);    //thrown



class XTOOLBOX_API XMLHttpRequest
{
 public :

    class Command
    {
    private :
        XBOX::VJSObject fReceiver;
        const XBOX::VJSObject fFunction;

    public :

        Command(XBOX::VJSObject inReceiver, const XBOX::VJSObject& inFunction) : fReceiver(inReceiver), fFunction(inFunction) {}

        void Execute()
        {
            XBOX::JS4D::ExceptionRef except;

            if(fReceiver && fFunction && fFunction.IsFunction())
                bool res=fReceiver.CallFunction(fFunction, NULL, NULL, &except); //todo : Que faire de l'exception ?
        }
    };


    typedef enum {UNSENT=0, OPENED, HEADERS_RECEIVED, LOADING, DONE} ReadyState;


    //XMLHttpRequest(const XBOX::VString& inProxy="", uLONG inProxyPort=-1);
    XMLHttpRequest();
    virtual ~XMLHttpRequest();

    XBOX::VError    Open                    (const XBOX::VString& inMethod, const XBOX::VString& inUrl, bool inAsync=true, const XBOX::VString& inUser="", const XBOX::VString& inPasswd="");
    XBOX::VError    SetProxy                (const XBOX::VString& inProxy, const uLONG inPort);
    XBOX::VError    SetRequestHeader        (const XBOX::VString& inKey, const XBOX::VString& inValue);
    XBOX::VError    OnReadyStateChange      (XBOX::VJSObject inReceiver, const XBOX::VJSObject& inFunction);
    XBOX::VError    Send                    (const XBOX::VString& inData="", XBOX::VError* outImplErr=NULL);
    XBOX::VError    Abort                   ();
    XBOX::VError    GetReadyState           (XBOX::VLong* outValue) const;
    XBOX::VError    GetStatus               (XBOX::VLong* outValue) const;
    XBOX::VError    GetStatusText           (XBOX::VString* outValue) const;
    XBOX::VError    GetResponseHeader       (const XBOX::VString& inKey, XBOX::VString* outValue) const;
    XBOX::VError    GetAllResponseHeaders   (XBOX::VString* outValue) const;
    XBOX::VError    GetResponseText         (XBOX::VString* outValue) const;


 private :
    //XBOX::CharSet   GuessRequestCharSet     () const;
    //XBOX::CharSet   GuessResponseCharSet    () const;
    //XBOX::CharSet   GetCharSet              (const char* inHeader) const;
    XBOX::CharSet   GuessCharSet            () const;
    const bool      fAsync;
    ReadyState      fReadyState;
    unsigned short  fStatus;
    XBOX::VString   fStatusText;
    XBOX::VString   fResponseText;
    XBOX::VString   fResponseXML;
    bool            fSendFlag;
    bool            fErrorFlag;
    XBOX::VString   fProxy;
    uLONG           fPort;
    XBOX::VString   fUser;
    XBOX::VString   fPasswd;
    Command*        fChangeHandler;

    class CWImpl;
    CWImpl*         fCWImpl;

};


class XTOOLBOX_API VJSXMLHttpRequest : public XBOX::VJSClass<VJSXMLHttpRequest, XMLHttpRequest>
{
 public:

    typedef XBOX::VJSClass<VJSXMLHttpRequest, XMLHttpRequest> inherited;

    static void GetDefinition           (ClassDefinition& outDefinition);
    static void Initialize              (const XBOX::VJSParms_initialize& inParms,   XMLHttpRequest* inXhr);
    static void Finalize                (const XBOX::VJSParms_finalize& inParms,     XMLHttpRequest* inXhr);
    static void Construct               (XBOX::VJSParms_construct& inParms);
    static void CallAsFunction          (XBOX::VJSParms_callAsFunction& ioParms);
    //static void CallAsConstructor     (XBOX::VJSParms_callAsConstructor& ioParms);

    static void _Open                   (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    //static void _SetProxy               (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    static void _SetRequestHeader       (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    static bool _OnReadyStateChange     (XBOX::VJSParms_setProperty& ioParms, XMLHttpRequest* inXhr);
    static void _Send                   (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    static void _Abort                  (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    static void _GetReadyState          (XBOX::VJSParms_getProperty& ioParms, XMLHttpRequest* inXhr);
    static void _GetStatus              (XBOX::VJSParms_getProperty& ioParms, XMLHttpRequest* inXhr);
    static void _GetStatusText          (XBOX::VJSParms_getProperty& ioParms, XMLHttpRequest* inXhr);
    static void _GetResponseHeader      (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    static void _GetAllResponseHeaders  (XBOX::VJSParms_callStaticFunction& ioParms, XMLHttpRequest* inXhr);
    static void _GetResponseText        (XBOX::VJSParms_getProperty& ioParms, XMLHttpRequest* inXhr);
};


#endif
