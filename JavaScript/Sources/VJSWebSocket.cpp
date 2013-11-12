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
#include "VJavaScriptPrecompiled.h"

#include "VJSWebSocket.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"

#include "VJSBuffer.h"
#include "VJSEvent.h"
#include "VJSEventEmitter.h"

#if 0

USING_TOOLBOX_NAMESPACE

VJSWebSocketObject::VJSWebSocketObject (bool inIsSynchronous)
{
	fIsSynchronous = inIsSynchronous;
	fBufferedAmount = 0;
	fWebSocketConnector = NULL;
	fWebSocket = NULL;
}

VJSWebSocketObject::~VJSWebSocketObject ()
{
	if (fWebSocket != NULL) {

		xbox_assert(fWebSocketConnector != NULL);

		if (fWebSocket->GetState() != VWebSocket::STATE_CLOSED) 

			fWebSocket->ForceClose();

		delete fWebSocket;
		fWebSocket = NULL;

	}

	if (fWebSocketConnector != NULL) {

		delete fWebSocketConnector;
		fWebSocketConnector = NULL;

	}
}

void VJSWebSocketObject::Initialize (const XBOX::VJSParms_initialize &inParms, VJSWebSocketObject *inWebSocket)
{
	if (inWebSocket == NULL)

		return;
	
	XBOX::VJSObject	thisObject	= inParms.GetObject();

	thisObject.SetProperty("CONNECTING", (sLONG) VWebSocket::STATE_CONNECTING, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("OPEN", (sLONG) VWebSocket::STATE_OPEN, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("CLOSING", (sLONG) VWebSocket::STATE_CLOSING, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
	thisObject.SetProperty("CLOSED", (sLONG) VWebSocket::STATE_CLOSED, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete);
}

void VJSWebSocketObject::Finalize (const XBOX::VJSParms_finalize &inParms, VJSWebSocketObject *inWebSocket)
{
	if (inWebSocket != NULL)

		delete inWebSocket;
}

void VJSWebSocketObject::GetProperty (XBOX::VJSParms_getProperty &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	XBOX::VString	propertyName;

	if (ioParms.GetPropertyName(propertyName)) {

		if (propertyName.EqualToUSASCIICString("readyState")) 

			ioParms.ReturnNumber<sLONG>(inWebSocket->GetState());

		else if (propertyName.EqualToUSASCIICString("bufferedAmount")) 

			ioParms.ReturnNumber<sLONG>(inWebSocket->fBufferedAmount);
		else 

			ioParms.ForwardToParent();

	} else 

		ioParms.ForwardToParent();
}

void VJSWebSocketObject::Construct (XBOX::VJSParms_callAsConstructor &ioParms, bool inIsSynchronous)
{
	ioParms.ReturnUndefined();

	XBOX::VString		url;
	VJSWebSocketObject	*webSocket;
	
	if (ioParms.CountParams() < 1 || !ioParms.GetStringParam(1, url)) 

		XBOX::vThrowError(VE_JVSC_WRONG_PARAMETER_TYPE_STRING, "1");

	else if ((webSocket = new VJSWebSocketObject(inIsSynchronous)) == NULL) 

		XBOX::vThrowError(XBOX::VE_MEMORY_FULL);
		
	else if (webSocket->_Connect(url) == XBOX::VE_OK) {

		XBOX::VJSObject	createdObject(ioParms.GetContext());
		
		if (inIsSynchronous)

			createdObject = VJSWebSocketSyncClass::CreateInstance(ioParms.GetContext(), webSocket);

		else

			createdObject = VJSWebSocketClass::CreateInstance(ioParms.GetContext(), webSocket);
		
		ioParms.ReturnConstructedObject(createdObject);

	} else 

		delete webSocket;	// _Connect() has thrown error.
}

void VJSWebSocketObject::Close (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);
}

void VJSWebSocketObject::Send (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

	XBOX::VJSObject	object(ioParms.GetContext());

	if (!ioParms.IsObjectParam(1) || !ioParms.GetParamObject(1, object) || !object.IsInstanceOf("Buffer")) {

		XBOX::vThrowError(XBOX::VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER, "1");
		return;

	}

	VJSBufferObject	*bufferObject;

	bufferObject = object.GetPrivateData<VJSBufferClass>();	
	xbox_assert(bufferObject != NULL);

	XBOX::VError	error;

	uBYTE	*sendbuffer	= (uBYTE *) bufferObject->GetDataPtr();
	sLONG	length	= (sLONG) bufferObject->GetDataSize();
	
	error = inWebSocket->fWebSocket->SendMessage(sendbuffer, length, false);
}

sLONG VJSWebSocketObject::GetState () const
{
	if (fWebSocket == NULL)

		return XBOX::VWebSocket::STATE_CONNECTING;

	else

		return fWebSocket->GetState();
}

XBOX::VError VJSWebSocketObject::_Connect (const XBOX::VString &inURL)
{
	fWebSocketConnector	= new XBOX::VWebSocketConnector(inURL);
	fWebSocketConnector->Connect("wakanda");
	fWebSocket = new XBOX::VWebSocket(fWebSocketConnector->StealEndPoint(), true);

	return XBOX::VE_OK;
}

XBOX::VError VJSWebSocketObject::_Close (bool inForce)
{
	return XBOX::VE_OK;
}

void VJSWebSocketClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>::StaticFunction functions[] =
	{
		{	"close",	js_callStaticFunction<VJSWebSocketObject::Close>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"send",		js_callStaticFunction<VJSWebSocketObject::Send>,	JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	0,			0,													0																	},
	};

    outDefinition.className			= "WebSocket";
	outDefinition.staticFunctions	= functions;
	outDefinition.callAsConstructor	= js_callAsConstructor<_Construct>;
    outDefinition.initialize		= js_initialize<VJSWebSocketObject::Initialize>;
	outDefinition.finalize			= js_finalize<VJSWebSocketObject::Finalize>;
	outDefinition.getProperty		= js_getProperty<VJSWebSocketObject::GetProperty>;
}

XBOX::VJSObject	VJSWebSocketClass::MakeConstructor (const XBOX::VJSContext &inContext)
{
	VJSWebSocketClass::Class();
	return VJSWebSocketClass::CreateInstance(inContext, NULL);
}

void VJSWebSocketClass::_Construct (XBOX::VJSParms_callAsConstructor &ioParms)
{
	VJSWebSocketObject::Construct(ioParms, false);
}

void VJSWebSocketSyncClass::GetDefinition (ClassDefinition &outDefinition)
{
	static XBOX::VJSClass<VJSWebSocketClass, VJSWebSocketObject>::StaticFunction functions[] =
	{
		{	"close",	js_callStaticFunction<VJSWebSocketObject::Close>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"send",		js_callStaticFunction<VJSWebSocketObject::Send>,		JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},
		{	"receive",	js_callStaticFunction<_Receive>,						JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete	},		
		{	0,			0,														0																	},
	};

    outDefinition.className			= "WebSocketSync";
	outDefinition.staticFunctions	= functions;
	outDefinition.callAsConstructor	= js_callAsConstructor<_Construct>;
    outDefinition.initialize		= js_initialize<VJSWebSocketObject::Initialize>;
	outDefinition.finalize			= js_finalize<VJSWebSocketObject::Finalize>;
	outDefinition.getProperty		= js_getProperty<VJSWebSocketObject::GetProperty>;
}

XBOX::VJSObject	VJSWebSocketSyncClass::MakeConstructor (const XBOX::VJSContext &inContext)
{
	VJSWebSocketSyncClass::Class();
	return VJSWebSocketSyncClass::CreateInstance(inContext, NULL);
}

void VJSWebSocketSyncClass::_Construct (XBOX::VJSParms_callAsConstructor &ioParms)
{
	VJSWebSocketObject::Construct(ioParms, true);
}

void VJSWebSocketSyncClass::_Receive (XBOX::VJSParms_callStaticFunction &ioParms, VJSWebSocketObject *inWebSocket)
{
	xbox_assert(inWebSocket != NULL);

#define BUFFER_SIZE	50000

	char			buffer[BUFFER_SIZE];
	uLONG			size;
	VSize			frameSize;
	XBOX::VError	error;

	ioParms.ReturnString("");

	size = BUFFER_SIZE;	//**
	if ((error = inWebSocket->fWebSocket->GetEndPoint()->Read(buffer, &size)) != XBOX::VE_OK) {

		XBOX::vThrowError(error);
		return;


	}

	frameSize = size;
	if (inWebSocket->fWebSocket->HandleFrame((const uBYTE *) buffer, &frameSize) == XBOX::VE_OK) {

		const XBOX::VWebSocketFrame	*frame;

		frame = inWebSocket->fWebSocket->GetLastFrame();
		if (frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_BINARY_FRAME 
		|| frame->GetOpcode() == XBOX::VWebSocketFrame::OPCODE_TEXT_FRAME) {

			char			unmasked[BUFFER_SIZE];
			XBOX::VString	result;

			if (frame->HasMaskingKey()) {

				XBOX::VWebSocketFrame::ApplyMaskingKey(frame->GetPayloadData(), (uBYTE *) unmasked, frame->GetMaskingKey(), frame->GetPayloadLength(), 0);
				result.FromBlock(unmasked, frame->GetPayloadLength(), VTC_StdLib_char);

			} else {

				result.FromBlock(frame->GetPayloadData(), frame->GetPayloadLength(), VTC_StdLib_char);


			}

			ioParms.ReturnString(result);

		}

	}

}

#endif