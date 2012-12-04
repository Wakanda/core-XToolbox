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
#ifndef __VJSONValue__
#define __VJSONValue__

#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VString_ExtendedSTL.h"

BEGIN_TOOLBOX_NAMESPACE


enum JSONType
{
	JSON_null		= 0,
	JSON_true		= 1,
	JSON_false		= 2,
	JSON_string		= 3,
	JSON_number		= 4,
	JSON_object		= 5,
	JSON_array		= 6,
	JSON_undefined	= 7
};

class VJSONObject;
class VJSONArray;
class VJSONWriter;
class VJSONCloner;
class VJSONGraph;

/*
	VJSONValue is a utility class to ease the manipulation of json structures.
	It wraps following JavaScript data types:
		null
		undefined
		bool
		number
		string
		object
		array
	
	VJSONValue acts as smart pointer to the wrapped value.
	
	VJSONValue value1;
	value1.SetNumber( 1);
	value1.SetString( CVSTR( "hello"));
	
	VJSONValue value2( JSON_object);
	value2.SetProperty( CVSTR( "name"), VJSONValue( CVSTR( "john")));
	value2.SetProperty( CVSTR( "age"), VJSONValue( 42));
	
	value = value2;

	value.SetUndefined();
	value2.SetUndefined();

*/
class XTOOLBOX_API VJSONValue : public VObject
{
public:
			// default constructor builds an undefined value.
									VJSONValue():fType( JSON_undefined)	{}
			
			// constructor using a json type.
			// JSON_object and JSON_array types creates a new VJSONObject or a VJSONArray.
	explicit						VJSONValue( JSONType inType);

			// copy constructor.
			// JSON_object and JSON_array types are not copied but simply retained.
									VJSONValue( const VJSONValue& inOther);
			
			// constructs a JSON_string value or JSON_null if VString::IsNull() is true.
	explicit						VJSONValue( const VString& inString);
	explicit						VJSONValue( const VInlineString& inString);

			// constructs a JSON_number value
	explicit						VJSONValue( double inNumber);

			// constructs a JSON_object value or a JSON_null if inObject is NULL.
			// inObject is retained.
	explicit						VJSONValue( VJSONObject *inObject);

			// constructs a JSON_array value or a JSON_null if inArray is NULL.
			// inArray is retained.
	explicit						VJSONValue( VJSONArray *inArray);

			// releases VString, VJSONArray or VJSONObject resources if any.
									~VJSONValue();

			// assignment
			VJSONValue&				operator=( const VJSONValue& inOther);

			bool					IsUndefined() const	{ return fType == JSON_undefined;}
			bool					IsNull() const		{ return fType == JSON_null;}
			bool					IsBool() const		{ return fType == JSON_false || fType == JSON_true;}
			bool					IsString() const	{ return fType == JSON_string;}
			bool					IsNumber() const	{ return fType == JSON_number;}
			bool					IsObject() const	{ return fType == JSON_object;}
			bool					IsArray() const		{ return fType == JSON_array;}
			
			JSONType				GetType() const		{ return fType;}

			// changes value to JSON_string value or JSON_null if VString::IsNull() is true.
			void					SetString( const VString& inString);
			void					SetString( const VInlineString& inString);

			// changes value to JSON_number value
			void					SetNumber( double inNumber);
			
			// changes value to JSON_true or JSON_false
			void					SetBool( bool inValue);

			// changes value to JSON_object value or a JSON_null if inObject is NULL.
			// inObject is retained.
			void					SetObject( VJSONObject *inObject);

			// changes value to JSON_array value or a JSON_null if inArray is NULL.
			// inArray is retained.
			void					SetArray( VJSONArray *inArray);

			// changes value to JSON_null
			void					SetNull();

			// changes value to JSON_undefined
			void					SetUndefined();
			
			// coerces the value into a number.
			// applies JavaScript-style data type conversions.
			double					GetNumber() const;

			// coerces the value into a bool.
			// applies JavaScript-style data type conversions.
			bool					GetBool() const;
			
			// GetString() mimics the toString() JavaScript method (coerces double, bools into string, doesn't encode characters, doesn't recurse objects).
			// null and undefined produce an empty string.
			// You may prefer to use Stringify() for complete JSON stringification.
			VError					GetString( VString& outString) const;

			// if IsObject() is true, GetObject() is guaranteed to be non NULL
			VJSONObject*			GetObject() const	{ return (fType == JSON_object) ? fObject : NULL;}

			// if IsArray() is true, GetArray() is guaranteed to be non NULL
			VJSONArray*				GetArray() const	{ return (fType == JSON_array) ? fArray : NULL;}
			
			// access properties of a JSON_object. Does nothing or returns a JSON_undefined if IsObject() is false.
			// This is for conveniency. Later we may support some standard properties for other types.
			VJSONValue				GetProperty( const VString& inName) const;
			void					SetProperty( const VString& inName, const VJSONValue& inValue);
			
			// conveniency method to instantiate a VJSONWriter and call VJSONWriter::StringifyValue( *this);
			VError					Stringify( VString& outString, JSONOption inOptions = JSON_WithQuotesIfNecessary) const;

			// common values
	static	const VJSONValue		sUndefined;
	static	const VJSONValue		sNull;
	static	const VJSONValue		sTrue;
	static	const VJSONValue		sFalse;

			// strings used to stringify common values
	static	const VString			sUndefinedString;
	static	const VString			sNullString;
	static	const VString			sTrueString;
	static	const VString			sFalseString;
	static	const VString			sObjectString;

	static	void					test();

protected:
			void					_Dispose();
			void					_CopyFrom( const VJSONValue& inOther);

			JSONType				fType;
			union
			{
				double				fNumber;
				VInlineString		fString;
				VJSONObject*		fObject;
				VJSONArray*			fArray;
			};
};


/*
	VJSONObject is a collection of (key,value) property pairs.
	
	key is a VString
	value is a VJSONValue
	
	keys are unique and not ordered.
	Comparison for uniqueness test is based on utf-16 code point equality as in JavaScript.
	
	A property cannot have JSON_undefined as value.
	
	VJSONObject *o = new VJSONObject;
	o->SetProperty( CVSTR( "name"), VJSONValue( CVSTR( "john")));
	o->SetProperty( CVSTR( "age"), VJSONValue( 42));
	ReleaseRefCountable( &o);
	
*/
class XTOOLBOX_API VJSONObject : public VObject, public IRefCountable
{
	friend class VJSONPropertyIterator;
	friend class VJSONPropertyConstIterator;
	friend class VJSONWriter;
public:
			// construct an empty collection
									VJSONObject();
	virtual	sLONG					Release( const char* inDebugInfo = 0) const;

			// gets the property named inName or returns a JSON_undefined value if property is not found.
	virtual	VJSONValue				GetProperty( const VString& inName) const;

			// if inValue is not JSON_undefined, SetProperty sets the property named inName with specified value, replacing previous one if it exists.
			// else the property is removed from the collection.
	virtual	bool					SetProperty( const VString& inName, const VJSONValue& inValue);

			// remove the property named inName.
			// equivalent to SetProperty( inName, VJSONValue( JSON_undefined));
	virtual	void					RemoveProperty( const VString& inName);
			
			// an empty object has no property
	virtual	bool					IsEmpty() const;

			// Remove all properties.
	virtual	void					Clear();

			// GetString() mimics the toString() JavaScript method and returns "[object Object]" except if GetString has been overriden.
			// You may prefer to use VJSONWriter::StringifyObject() instead for complete JSON stringification.
	virtual	VError					GetString( VString& outString) const;

			// Clone this object and all its properties recursively.
			// The result is not necessarily a JSON_object in case Clone() has been overriden.
	virtual	VError					Clone( VJSONValue& outValue, VJSONCloner& inCloner) const;

			VJSONGraph**			GetGraph() const		{ return &fGraph;}

			// to detect leaks
	static	sLONG					GetInstancesCount()					{ return sCount;}

protected:
	virtual							~VJSONObject();
	
			// override DoStringify() and return true if you don't want the default VJSONWriter default behavior for this object.
			// similar to the toJSON() JavaScript function property.
	virtual	bool					DoStringify( VString& outString, VJSONWriter& inWriter, VError *outError) const;

			void					Connect( VJSONGraph** inOtherGraph);

private:
	static	sLONG					sCount;
			typedef unordered_map_VString<VJSONValue>	MapType;
			MapType					fMap;
	mutable	VJSONGraph*				fGraph;
};


/*
	Iterators to iterates on a VJSONObject properties.
	
	VJSONPropertyIterator requires a VJSONObject* object and lets you modify property values.
	VJSONConstPropertyIterator accepts a const VJSONObject* object but doesn't let you modify property values.
	
	VJSONObject *jsonObject = ...;
	for( VJSONPropertyIterator i( jsonObject) ; i.IsValid() ; ++i)
	{
		VString propertyName = i.GetName();
		VJSONValue propertyValue = i.GetValue();
	}
	
*/
class VJSONPropertyIterator : public XBOX::VObject
{
public:
			VJSONPropertyIterator( VJSONObject *inObject):fIterator( inObject->fMap.begin()), fIterator_end( inObject->fMap.end()) {}
	
			const VString&			GetName() const		{ return fIterator->first;}
			VJSONValue&				GetValue() const	{ return fIterator->second;}
			
			VJSONPropertyIterator&	operator++()		{ ++fIterator; return *this;}
			
			bool					IsValid() const		{ return fIterator != fIterator_end;}
			
private:
			VJSONPropertyIterator( const VJSONPropertyIterator&);				// forbidden
			VJSONPropertyIterator&	operator=( const VJSONPropertyIterator&);	// forbidden
			VJSONObject::MapType::iterator	fIterator;
			VJSONObject::MapType::iterator	fIterator_end;
};

class VJSONPropertyConstIterator : public XBOX::VObject
{
public:
			VJSONPropertyConstIterator( const VJSONObject *inObject):fIterator( inObject->fMap.begin()), fIterator_end( inObject->fMap.end()) {}

			const VString&			GetName() const		{ return fIterator->first;}
			const VJSONValue&		GetValue() const	{ return fIterator->second;}

			VJSONPropertyConstIterator&	operator++()	{ ++fIterator; return *this;}

			bool					IsValid() const		{ return fIterator != fIterator_end;}

private:
			VJSONPropertyConstIterator( const VJSONPropertyConstIterator&);				// forbidden
			VJSONPropertyConstIterator&	operator=( const VJSONPropertyConstIterator&);	// forbidden
			VJSONObject::MapType::const_iterator	fIterator;
			VJSONObject::MapType::const_iterator	fIterator_end;
};


/*
	Array of VJSONValue.
	
	Vvalues are not necessarily of the same type.
	
*/
class XTOOLBOX_API VJSONArray : public VObject, public IRefCountable
{
public:
									VJSONArray();
	virtual	sLONG					Release( const char* inDebugInfo = 0) const;

			const VJSONValue&		operator[]( size_t inPos) const		{ return fVector[inPos];}
			
			// Set nth value in array (inIndex must be (> 0) and (<= GetCount())
			void					SetNth( size_t inIndex, const VJSONValue& inValue);

			size_t					GetCount() const					{ return fVector.size();}
			bool					IsEmpty() const						{ return fVector.empty();}

			// Resize the array removing values or adding undefined values as necessary
			bool					Resize( size_t inSize);

			// Remove all values.
			// equivalent to Resize( 0)
			void					Clear();

			// add a value. returns true if successul.
			bool					Push( const VJSONValue& inValue);

			// Builds a new VJSONArray filled with cloned values recursively.
			// result value is JSON_undefined on error.
			VError					Clone( VJSONValue& outValue, VJSONCloner& inCloner) const;
			
			// GetString() mimics the toString() JavaScript method, calling GetString() on each individual value.
			// The result is a string like "val1,val2,val3" without brackets.
			// You may prefer to use VJSONWriter::StringifyArray() instead for complete stringification.
			VError					GetString( VString& outString) const;

			VJSONGraph**			GetGraph()							{ return &fGraph;}

			// to detect leaks
	static	sLONG					GetInstancesCount()					{ return sCount;}
			
protected:
	virtual							~VJSONArray();
			
private:
	static	sLONG					sCount;
			typedef std::vector<VJSONValue>	VectorType;
			VectorType				fVector;
	mutable	VJSONGraph*				fGraph;
};

/*
	VJSONWriter let you stringify a VJSONValue, a VJSONObject or a VJSONArray.
	
	Options let you control the way things are stringified.
	
	To customize the way an object is stringified, you may want to override VJSONObject::DoStringify()
	and subclass VJSONWriter if you need to pass specific parameters or options.
*/
class XTOOLBOX_API VJSONWriter : public VObject
{
public:
			// pass option JSON_WithQuotesIfNecessary for standard JavaScript stringification
	explicit						VJSONWriter( JSONOption inOptions = JSON_WithQuotesIfNecessary);

			JSONOption				GetOptions() const					{ return fOptions;}
			
			// Recursively stringiy value, object or array the same way as JSON.stringify() in JavaScript.
			// Cyclic structures are detected and throw error VE_JSON_STRINGIFY_CIRCULAR.
			VError					StringifyValue( const VJSONValue& inValue, VString& outString);
			VError					StringifyObject( const VJSONObject *inObject, VString& outString);
			VError					StringifyArray( const VJSONArray *inArray, VString& outString);
			
protected:
			void					IncrementLevel();
			void					DecrementLevel();
			void					InsertIndentString( VString& ioString);
			void					AppendIndentString( VString& ioString);

private:
			JSONOption				fOptions;
			sLONG					fLevel;
			VString					fIndentString;
			VString					fIndentStringCurrentLevel;
			std::vector<const VJSONObject*>	fStack;	// used to detect recursion
};

/*
	VJSONCloner lets you clone a VJSONObject recursively.
	
	A map of cloned objects is maintained so that the same object is not cloned twice.
	Cyclic structures are supported.
*/
class XTOOLBOX_API VJSONCloner : public VObject
{
public:
	explicit						VJSONCloner();
	
			// Clone an object recursively.
			// The result is not necessarily a JSON_object in case VJSONObject::Clone() has been overriden.
			VError					CloneObject( const VJSONObject *inObject, VJSONValue& outValue);

private:
	typedef std::map<const VJSONObject*,VJSONValue> MapOfValueByObject;
			MapOfValueByObject		fClonedObjects;
};


END_TOOLBOX_NAMESPACE

#endif