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
#include "IPropertyCollector.h"
#include "VString.h"
#include "VValueBag.h"
#include "VUUID.h"


IPropertyCollector::IPropertyCollector()
{
}


IPropertyCollector::~IPropertyCollector()
{
}


Boolean IPropertyCollector::GetName(VString& outName) const
{
	Boolean succeed = GetString(CVSTR("name"), outName);
	
	if (!succeed)
		outName.Clear();
	
	return succeed;
}


void IPropertyCollector::SetName(const VString& inName)
{
	SetString(CVSTR("name"), inName);
}
	

Boolean IPropertyCollector::GetUUID(VUUID& outID) const
{
	Boolean	succeed = GetVUUID(CVSTR("uuid"), outID);
	
	if (!succeed)
		outID.Clear();
	
	return succeed;
}


void IPropertyCollector::SetUUID(const VUUID& inID)
{
	SetVUUID(CVSTR("uuid"), inID);
}


#pragma mark-

IBagPropertyCollector::IBagPropertyCollector(VValueBag* inBag)
{
	fPropertyBag = inBag;
	
	if (fPropertyBag != NULL)
		fPropertyBag->Retain();
}


IBagPropertyCollector::~IBagPropertyCollector()
{
	if (fPropertyBag != NULL)
		fPropertyBag->Release();
}


void IBagPropertyCollector::SetPropertyBag(VValueBag* inBag)
{
	IRefCountable::Copy(&fPropertyBag, inBag);
}


Boolean IBagPropertyCollector::GetString(const VString& inPropertyName, VString& outValue) const
{
	if (fPropertyBag == NULL) return false;
	
	return fPropertyBag->GetString(inPropertyName, outValue);
}


Boolean IBagPropertyCollector::GetBoolean(const VString& inPropertyName, Boolean& outValue) const
{
	if (fPropertyBag == NULL) return false;
	
	return fPropertyBag->GetBoolean(inPropertyName, outValue);
}


Boolean IBagPropertyCollector::GetLong(const VString& inPropertyName, sLONG& outValue) const
{
	if (fPropertyBag == NULL) return false;
	
	return fPropertyBag->GetLong(inPropertyName, outValue);
}


Boolean IBagPropertyCollector::GetReal(const VString& inPropertyName, Real& outValue) const
{
	if (fPropertyBag == NULL) return false;
	
	return fPropertyBag->GetReal(inPropertyName, outValue);
}


Boolean IBagPropertyCollector::GetVUUID(const VString& inPropertyName, VUUID& outValue) const
{
	if (fPropertyBag == NULL) return false;
	
	return fPropertyBag->GetVUUID(inPropertyName, outValue);
}


void IBagPropertyCollector::SetString(const VString& inPropertyName, const VString& inValue)
{
	if (!testAssert(fPropertyBag != NULL)) return;
	
	fPropertyBag->SetString(inPropertyName, inValue);
}


void IBagPropertyCollector::SetBoolean(const VString& inPropertyName, Boolean inValue)
{
	if (!testAssert(fPropertyBag != NULL)) return;
	
	fPropertyBag->SetBoolean(inPropertyName, inValue);
}


void IBagPropertyCollector::SetLong(const VString& inPropertyName, sLONG inValue)
{
	if (!testAssert(fPropertyBag != NULL)) return;
	
	fPropertyBag->SetLong(inPropertyName, inValue);
}


void IBagPropertyCollector::SetReal(const VString& inPropertyName, Real inValue)
{
	if (!testAssert(fPropertyBag != NULL)) return;
	
	fPropertyBag->SetReal(inPropertyName, inValue);
}


void IBagPropertyCollector::SetVUUID(const VString& inPropertyName, const VUUID& inValue)
{
	if (!testAssert(fPropertyBag != NULL)) return;
	
	fPropertyBag->SetVUUID(inPropertyName, inValue);
}
