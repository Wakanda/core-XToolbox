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
#include "VKernelPrecompiled.h"
#include "VString.h"

#include "XLinuxStackCrawl.h"

#include <execinfo.h>
#include <stdio.h>


bool XLinuxStackCrawl::operator < (const XLinuxStackCrawl& other) const
{
	bool res;
	if (fCount == other.fCount)
	{
		res = false;
		for (int i = 0; i < fCount; ++i)
		{
			if (fFrames[i] < other.fFrames[i])
			{
				res = true;
				break;
			}
		}
	}
	else
	{
		if (fCount <= 0 && other.fCount <= 0)
			res = false;
		else
			res = (fCount < other.fCount);
	}
	return res;
}


void XLinuxStackCrawl::LoadFrames( uLONG inStartFrame, uLONG inNumFrames)
{
	xbox_assert( inStartFrame < 10);
	xbox_assert( inNumFrames < kMaxScrawlFrames);

	void *frames[kMaxScrawlFrames+10];
	int count = backtrace( frames, kMaxScrawlFrames+10);
	if (count > inStartFrame)
	{
		fCount = std::min( count - inStartFrame, inNumFrames);
		memmove( fFrames, &frames[inStartFrame], fCount * sizeof( void*));
	}
	else
	{
		fCount = 0;
	}
}

void XLinuxStackCrawl::Dump( FILE* inFile) const
{
	if (fCount > 0)
	{
		char** strs = backtrace_symbols( fFrames, fCount);
		for( int i = 0; i < fCount; ++i)
		{
			fprintf( inFile, "%s\n", strs[i]);
		}
		free(strs);
	}
}


void XLinuxStackCrawl::Dump( VString& ioString) const
{
	if (fCount > 0)
	{
		bool first = true;
		char** strs = backtrace_symbols( fFrames, fCount);
		for( int i = 0; i < fCount; ++i)
		{
			long int n;
			char module[40];
			unsigned long int address;
			char symbol[2048];
			long int offset;
			int r = sscanf( strs[i], "%ld %s %lx %2000s + %ld", &n, module, &address, symbol, &offset);
			
			if ( (r >= 4) && strcmp( module, "???") != 0 && strcmp( symbol, "0x0") != 0)
			{
				VString s;
				VSystem::DemangleSymbol( symbol, s);
				if (!first)
					ioString += "\n";
				first = false;
				ioString += s;
			}
		}
		free(strs);
	}
}
