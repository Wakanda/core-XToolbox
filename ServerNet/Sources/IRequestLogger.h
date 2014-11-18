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
#include "ServerNetTypes.h"


#ifndef __SNET_I_REQUEST_LOGGER__
#define __SNET_I_REQUEST_LOGGER__


class XTOOLBOX_API IRequestLogger
{
public:
	IRequestLogger() {}
	virtual ~IRequestLogger() {}
	
	virtual void Log( OsType inComponentID,void* inCDB4DBaseContext,sLONG inRequestNo, sLONG inRequestNbBytes, sLONG inReplyNbBytes, sLONG inElapsedTime)=0;
	virtual void Log( OsType inComponentID,void* inCDB4DBaseContext,const XBOX::VString& inMessage,sLONG inElapsedTime,bool inCleanString=false)=0;
	virtual void Log( OsType inComponentID,void* inCDB4DBaseContext,const char* inMessage,sLONG inElapsedTime)=0;

	virtual bool IsEnable()const =0;
};


#endif