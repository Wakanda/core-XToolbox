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
#ifndef __CServer__
#define __CServer__

#include "Kernel/VKernel.h"

BEGIN_TOOLBOX_NAMESPACE

class CEndPoint : public VObject
{
	public:

		virtual VError Read(void *oBuff, uLONG *ioLen) = 0;

		virtual VError ReadExactly(void *oBuff, uLONG Len) = 0;

		/* bWithEmptyTail is required only on Windows, really. It is used to
		determine broken socket connection. */
		virtual VError Write(void *iBuff, uLONG *ioLen, bool bWithEmptyTail = false) = 0;

		virtual VError Close ( ) = 0;
};



class CConnectionHandler : public VObject
{
	public:

		/* Will return current execution status instead, like
		S_DONE, S_NOT_DONE, S_WAITING_FOR_INPUT, etc... */
		virtual Boolean DoWork() = 0;
};



class CConnectionHandlerFactory : public VObject
{
	public:

		virtual CConnectionHandler* CreateConnectionHandler ( CEndPoint* inEndPoint ) = 0;
};


class CListener : public VObject
{
	public:

		virtual VError AddConnectionHandlerFactory ( CConnectionHandlerFactory* inFactory ) = 0;
		//virtual VError RegisterWorkerPool ( CWorkerPool* wPool ) = 0;

		virtual VError StartListening ( ) = 0;
		virtual VError StopListening ( ) = 0;
};



class CServer : public XBOX::CComponent
{
	public:
		enum {Component_Type = 'SRVC'};

		virtual	VError Start(sLONG inTimeout = -1) = 0;
		virtual	VError Stop(Boolean WaitUntilCompletelyStopped = false) = 0;

		virtual VError AddListener ( CListener* inListener ) = 0;
};


END_TOOLBOX_NAMESPACE

#endif
