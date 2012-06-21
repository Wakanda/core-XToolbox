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
#ifndef __VQuickTimeSDK__
#define __VQuickTimeSDK__

#if USE_QUICKTIME

/*
	To avoid conflicts with legacy code (Mac2Win, OLDBOX),
	users of XToolBox should not have to include QT headers directly.
	
	This header file is for xtoolbox sources only.
*/

#include "VQuickTime.h"

// On Mac VQuickTime.h already include system headers.
// On Windows, you need to add an Access Path to Quicktime headers to your project
//	example: "../../../quicktime/6.0.0/QTDevWin/CIncludes"

#if VERSIONWIN
BEGIN_QTIME_NAMESPACE
	#include <QTML.h>
	#include <Gestalt.h>
	#include <Resources.h>
	#include <ImageCodec.h>
	#include <ImageCompression.h>
	#include <QuickTimeComponents.h>
	#include <movies.h>
END_QTIME_NAMESPACE

inline void QDGetPictureBounds( qtime::PicPtr *picH, qtime::Rect *outRect ){*outRect=(**picH).picFrame;};

#endif //VERSIONWIN

#endif

#endif