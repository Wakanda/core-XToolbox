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
#include "VByteSwap.h"


BEGIN_TOOLBOX_NAMESPACE
/* A Supprimer car rien n'est utilisé
void Real8toReal10(void* inReal8, void* inReal10)
{
#if VERSIONWIN && ARCH_32 //N'est active que pour les version 32 bits de windows
	unsigned char x[10];
	
	_asm LEA EAX,  x
	_asm MOV ECX,  inReal8
	_asm FLD qword ptr[ECX]
	_asm FSTP tbyte ptr[EAX] 
	
	((unsigned char*)inReal10)[9] = x[0];
	((unsigned char*)inReal10)[8] = x[1];
	((unsigned char*)inReal10)[7] = x[2];
	((unsigned char*)inReal10)[6] = x[3];
	((unsigned char*)inReal10)[5] = x[4];
	((unsigned char*)inReal10)[4] = x[5];
	((unsigned char*)inReal10)[3] = x[6];
	((unsigned char*)inReal10)[2] = x[7];
	((unsigned char*)inReal10)[1] = x[8];
	((unsigned char*)inReal10)[0] = x[9];
	
#elif VERSIONMAC

#if VERSION_MACHO
	dtox80((double*) inReal8, (extended80*)inReal10);
#else
	long double ld;
	ld = *(double*)inReal8;
	ldtox80(&ld, (Float80*)inReal10);
#endif	// VERSION_MACHO

#endif	// VERSIONMAC
}


void Real10toReal8(void* inReal10, void* inReal8)
{
#if VERSIONWIN && ARCH_32 //N'est active que pour les version 32 bits de windows
	unsigned char x[10];

	x[0] = ((unsigned char*)inReal10)[9];
	x[1] = ((unsigned char*)inReal10)[8];
	x[2] = ((unsigned char*)inReal10)[7];
	x[3] = ((unsigned char*)inReal10)[6];
	x[4] = ((unsigned char*)inReal10)[5];
	x[5] = ((unsigned char*)inReal10)[4];
	x[6] = ((unsigned char*)inReal10)[3];
	x[7] = ((unsigned char*)inReal10)[2];
	x[8] = ((unsigned char*)inReal10)[1];
	x[9] = ((unsigned char*)inReal10)[0];
	
	_asm LEA EAX,  x
	_asm MOV ECX,  inReal8
	_asm FLD tbyte ptr[EAX]
	_asm FSTP qword ptr[ECX] 
	
#elif VERSIONMAC

#if VERSION_MACHO 
	*(double*)inReal8 = x80tod((extended80*) inReal10);
#else
	long double ld;
	x80told((Float80*)inReal10, &ld);
	*(double*)inReal8 = ld;
#endif	// VERSION_MACHO
	
#endif	// VERSIONMAC
}

void Real8toReal10Array(void* inReal8, void* inReal10, long inCount)
{
	char* end;
	
	for (end=((char*)inReal8)+(8*inCount);inReal8<end;(*(char**)&inReal8)+=8, (*(char**)&inReal10)+=10) Real8toReal10(inReal8, inReal10);
}


void Real10toReal8Array(void* inReal10, void* inReal8, long inCount)
{
	char* end;
	
	for (end=((char*)inReal10)+(10*inCount);inReal10<end;(*(char**)&inReal10)+=10, (*(char**)&inReal8)+=8) Real10toReal8(inReal10, inReal8);
}


Real FixedToReal(sLONG inLong)
{
	return (Real)((inLong>>16)&0x0000FFFF) + (Real)((Real)(inLong&0x0000FFFF)/(Real)(0xFFFF));
}
*/
END_TOOLBOX_NAMESPACE
