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
#ifndef __VJS_WEB_SOCKET__
#define __VJS_WEB_SOCKET__

#include "VJSClass.h"
#include "VJSValue.h"
#include "VJSWorker.h"
#include "VJSEventEmitter.h"
#include "VJSBuffer.h"

#if 0

/*
#include "ServerNet/VServerNet.h"			//**
#include "ServerNet/Sources/VWebSocket.h"	//**
*/

BEGIN_TOOLBOX_NAMESPACE



class XTOOLBOX_API VJSWebSocketObject : public XBOX::VObject
{
public:

								VJSWebSocketObject (bool inIsSynchronous);
	virtual						~VJSWebSocketObject ();

	static void					Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketObject *inWebSocket);
	static void					Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketObject *inWebSocket);
	static void					GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSWebSocketObject *inWebSocket);

	static void					Construct (XBOX::VJSParms_callAsConstructor &ioParms, bool inIsSynchronous);

	static void					Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);	
	static void					Send (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);	

	sLONG						GetState () const;

//private:

	bool						fIsSynchronous;
	sLONG						fBufferedAmount;		// Always zero, writes are synchronous.

	XBOX::VWebSocketConnector	*fWebSocketConnector;	// Thread-safe.
	XBOX::VWebSocket			*fWebSocket;			// Thread-safe.

	XBOX::VError				_Connect (const XBOX::VString &inURL);
	XBOX::VError				_Close (bool inForce);
};

class XTOOLBOX_API VJSWebSocketClass : public XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>
{
public:

	static void					GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject		MakeConstructor (const XBOX::VJSContext &inContext);

private: 

	static void					_Construct (XBOX::VJSParms_callAsConstructor &ioParms);
};

class XTOOLBOX_API VJSWebSocketSyncClass : public XBOX::VJSClass<VJSWebSocketSyncClass, VJSWebSocketObject>
{
public:

	static void					GetDefinition (ClassDefinition &outDefinition);
	static XBOX::VJSObject		MakeConstructor (const XBOX::VJSContext &inContext);

private: 

	static void					_Construct (XBOX::VJSParms_callAsConstructor &ioParms);	

	static void					_Receive (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket);
};

/*

enum BinaryType { "blob", "arraybuffer" };
[Constructor(DOMString url, optional (DOMString or DOMString[]) protocols)]
interface WebSocket : EventTarget {
  readonly attribute DOMString url;

  // ready state
 
  readonly attribute unsigned short readyState;
  readonly attribute unsigned long bufferedAmount;

  // networking
           attribute EventHandler onopen;
           attribute EventHandler onerror;
           attribute EventHandler onclose;
  readonly attribute DOMString extensions;
  readonly attribute DOMString protocol;
  void close([Clamp] optional unsigned short code, optional DOMString reason);

  // messaging
           attribute EventHandler onmessage;
           attribute BinaryType binaryType;
  void send(DOMString data);
  void send(Blob data);
  void send(ArrayBuffer data);
  void send(ArrayBufferView data);

*/

END_TOOLBOX_NAMESPACE

#endif

#endif