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
#ifndef __IPropertyCollector__
#define __IPropertyCollector__

BEGIN_TOOLBOX_NAMESPACE

// Needed declaration
class VValueBag;
class VUUID;
class VString;

class XTOOLBOX_API IPropertyCollector
{
public:
			IPropertyCollector ();
	virtual	~IPropertyCollector ();
	
	// Property support (tries to coerce, returns false if not found or coercion failed)
	virtual Boolean GetString (const VString& inPropertyName, VString& outValue) const = 0;
	virtual Boolean GetBoolean (const VString& inPropertyName, Boolean& outValue) const = 0;
	virtual Boolean	GetLong (const VString& inPropertyName, sLONG& outValue) const = 0;
	virtual Boolean	GetVUUID (const VString& inPropertyName, VUUID& outValue) const = 0;
	virtual Boolean	GetReal (const VString& inAttributeName, Real& outValue) const = 0;

	virtual void	SetString (const VString& inPropertyName, const VString& inValue) = 0;
	virtual void	SetBoolean (const VString& inPropertyName, Boolean inValue) = 0;
	virtual void	SetLong (const VString& inPropertyName, sLONG inValue) = 0;
	virtual void	SetVUUID (const VString& inPropertyName, const VUUID& inValue) = 0;
	virtual void	SetReal (const VString& inAttributeName, Real inValue) = 0;
	
	// Attributes support (name and uuid are attributes not accessors)
	virtual Boolean	GetName (VString& outName) const;	// name is cleared if no "name" property
	virtual void	SetName (const VString& inName);

	virtual Boolean	GetUUID (VUUID& outID) const;
	virtual void	SetUUID (const VUUID& inID);
};


class XTOOLBOX_API IBagPropertyCollector : public IPropertyCollector
{
			IBagPropertyCollector (VValueBag* inBag = NULL);
	virtual	~IBagPropertyCollector ();
	
	// Inherited from IPropertyCollector
	virtual Boolean GetString (const VString& inPropertyName, VString& outValue) const;
	virtual Boolean GetBoolean (const VString& inPropertyName, Boolean& outValue) const;
	virtual Boolean	GetLong (const VString& inPropertyName, sLONG& outValue) const;
	virtual Boolean	GetVUUID (const VString& inPropertyName, VUUID& outValue) const;
	virtual Boolean	GetReal (const VString& inAttributeName, Real& outValue) const;

	virtual void	SetString (const VString& inPropertyName, const VString& inValue);
	virtual void	SetBoolean (const VString& inPropertyName, Boolean inValue);
	virtual void	SetLong (const VString& inPropertyName, sLONG inValue);
	virtual void	SetVUUID (const VString& inPropertyName, const VUUID& inValue);
	virtual void	SetReal (const VString& inAttributeName, Real inValue);
	
	// Accessor
	virtual void	SetPropertyBag (VValueBag* inBag);

protected:
	VValueBag*	fPropertyBag;	
};


END_TOOLBOX_NAMESPACE


#endif