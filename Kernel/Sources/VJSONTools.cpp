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
#include "VTime.h"
#include "VJSONValue.h"
#include "VJSONTools.h"
#include "VError.h"
#include "VValueBag.h"
#include "VUnicodeTableFull.h"

BEGIN_TOOLBOX_NAMESPACE


// ===========================================================
#pragma mark -
#pragma mark VJSONImporter
// ===========================================================


VJSONImporter::VJSONImporter(const VString& inJSONString)
: fString( inJSONString)
{
	fInputLen = fString.GetLength();
	fStartChar = fString.GetCPointer();
	fCurChar = fStartChar;
	fRecursiveCallCount = 0;
}


/* WARNING: this GetNextJSONToken(VString&, bool*) is the same as the GetNextJSONToken() with no parameters,
			except it fills outString and withQuotes.

		=> if something must be changed here, think about changing the parsing also in the other GetNextJSONToken()

	It looks better to write 2 routines instead of filling this one with more parameter and testing, such as writting
	a lot of if(!doNotFillString) or something
*/
VJSONImporter::JsonToken VJSONImporter::GetNextJSONToken(VString& outString, bool* withQuotes)
{
	outString.Clear();
	
	bool		eof;
	
	if (withQuotes != NULL)
		*withQuotes = false;
	
	UniChar c;
	
	do 
	{
		c = _GetNextChar(eof);
		if (c > 32)
		{
			switch (c)
			{
				case '{':
					return jsonBeginObject;
					break;
					
				case '}':
					return jsonEndObject;
					break;
					
				case '[':
					return jsonBeginArray;
					break;
					
				case ']':
					return jsonEndArray;
					break;
					
				case ',':
					return jsonSeparator;
					break;
					
				case ':':
					return jsonAssigne;
					break;
					
				case '"':
				{
					if (withQuotes != NULL)
						*withQuotes = true;
					
					do 
					{
						c = _GetNextChar(eof);
						if (c != 0)
						{
							if (c == '\\')
							{
								c = _GetNextChar(eof);
								switch(c)
								{
									case '\\':
									case '"':
									case '/':
										//  c is the same
										break;
										
									case 't':
										c = 9;
										break;
										
									case 'r':
										c = 13;
										break;
										
									case 'n':
										c = 10;
										break;
										
									case 'b':
										c = 8;
										break;
										
									case 'f':
										c = 12;
										break;
										
									case 'u':
								/*	2010-02-22 - T.A.
									http://www.ietf.org/rfc/rfc4627.txt
									. . .
									The application/json Media Type for JavaScript Object Notation (JSON)
									. . .
									Any character may be escaped.  If the character is in the Basic
									Multilingual Plane (U+0000 through U+FFFF), then it may be
									represented as a six-character sequence: a reverse solidus, followed
									by the lowercase letter u, followed by four hexadecimal digits that
									encode the character's code point.  The hexadecimal letters A though
									F can be upper or lowercase.  So, for example, a string containing
									only a single reverse solidus character may be represented as
									"\u005C".
									. . .
							   */
										if ( testAssert( fCurChar - fStartChar + 4 <= fInputLen))
										{
											c = 0;
											for(sLONG i_fromHex = 1; i_fromHex <= 4; ++i_fromHex)
											{
												UniChar theChar = *fCurChar++;
												if (theChar >= CHAR_DIGIT_ZERO && theChar <= CHAR_DIGIT_NINE)
												{
													c *= 16L;
													c += (theChar - CHAR_DIGIT_ZERO);
												}
												else if (theChar >= CHAR_LATIN_CAPITAL_LETTER_A && theChar <= CHAR_LATIN_CAPITAL_LETTER_F)
												{
													c *= 16L;
													c += (theChar - CHAR_LATIN_CAPITAL_LETTER_A) + 10L;
												}
												else if (theChar >= CHAR_LATIN_SMALL_LETTER_A && theChar <= CHAR_LATIN_SMALL_LETTER_F)
												{
													c *= 16L;
													c += (theChar - CHAR_LATIN_SMALL_LETTER_A) + 10L;
												}
											}
										}
										else
										{
											eof = true;
											c = 0;
										}
										break;
								}
								if (c != 0)
									outString.AppendUniChar(c);
							}
							else if (c == '"')
							{
								return jsonString;
							}
							else
							{
								outString.AppendUniChar(c);
							}
						}							
					} while(!eof);
					return jsonString;
				}
					break;
					
				default:
				{
					outString.AppendUniChar(c);
					do 
					{
						const UniChar* oldpos = fCurChar;
						c = _GetNextChar(eof);
						if (c <= 32)
						{
							return jsonString;
						}
						else
						{
							switch(c)
							{
								case '{':
								case '}':
								case '[':
								case ']':
								case ',':
								case ':':
								case '"':
									fCurChar = oldpos;
									return jsonString;
									break;
									
								default:
									outString.AppendUniChar(c);
									break;
							}
						}
					} while(!eof);
					return jsonString;
				}
					break;
			}
		}
	} while(!eof);
	
	return jsonNone;
}


/* WARNING: this GetNextJSONToken() is the same as the GetNextJSONToken(VString&, bool*),
			except it does'nt fill anything.

		=> if something must be changed here, think about changing the parsing also in the other GetNextJSONToken()
*/
VJSONImporter::JsonToken VJSONImporter::GetNextJSONToken()
{	
	bool		eof;
	UniChar		c;
	
	do 
	{
		c = _GetNextChar(eof);
		if (c > 32)
		{
			switch (c)
			{
				case '{':
					return jsonBeginObject;
					break;
					
				case '}':
					return jsonEndObject;
					break;
					
				case '[':
					return jsonBeginArray;
					break;
					
				case ']':
					return jsonEndArray;
					break;
					
				case ',':
					return jsonSeparator;
					break;
					
				case ':':
					return jsonAssigne;
					break;
					
				case '"':
				{
					do 
					{
						c = _GetNextChar(eof);
						if (c == '\\')
						{
							c = _GetNextChar(eof);
						}
						else if (c == '"')
						{
							return jsonString;
						}						
					} while(!eof);
					return jsonString;
				}
					break;
					
				default:
				{
					do 
					{
						const UniChar* oldpos = fCurChar;
						c = _GetNextChar(eof);
						if (c <= 32)
						{
							return jsonString;
						}
						else
						{
							switch(c)
							{
								case '{':
								case '}':
								case '[':
								case ']':
								case ',':
								case ':':
								case '"':
									fCurChar = oldpos;
									return jsonString;
									break;
									
								default:
									break;
							}
						}
					} while(!eof);
					return jsonString;
				}
				break;
			}
		}
	} while(!eof);
	
	return jsonNone;
}

UniChar VJSONImporter::_GetNextChar(bool& outIsEOF)
{
	UniChar result = 0;
	if (fCurChar - fStartChar >= fInputLen)
	{
		outIsEOF = true;
	}
	else
	{
		outIsEOF = false;
		result = *fCurChar;
		fCurChar++;
	}
	return result;
}


bool VJSONImporter::_ParseNumber( const UniChar *inString)
{
	const UniChar *p = inString;
	
	bool failed = false;

	if ( (*p == CHAR_HYPHEN_MINUS) || (*p == CHAR_MINUS_SIGN) )
		++p;

	if (*p == CHAR_DIGIT_ZERO)
		++p;
	else
	{
		while( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
			++p;
	}

	// consume '.'
	
	bool withDot = (*p == CHAR_FULL_STOP);
	if (withDot)
	{
		++p;
		if ( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
		{
			do
			{
				++p;
			} while( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE);
		}
		else
		{
			failed = true;	// the dot must be followed by an integer
		}
	}
	
	// consume 'e' or 'E'
	bool withExponent = (*p == CHAR_LATIN_SMALL_LETTER_E || *p == CHAR_LATIN_CAPITAL_LETTER_E);
	if (withExponent)
	{
		++p;

		// comsume '+' or '-'
		if ( (*p == CHAR_PLUS_SIGN) || (*p == CHAR_HYPHEN_MINUS) || (*p == CHAR_MINUS_SIGN) )
			++p;
	
		// consume exponent
		if (*p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE)
		{
			do {
				++p;
			} while( *p >= CHAR_DIGIT_ZERO && *p <= CHAR_DIGIT_NINE);
		}
		else
		{
			failed = true;	// 'e' or 'E' must be followed by an integer
		}
	}
	
	return !failed && (*p == 0);
}



VError VJSONImporter::_StringToValue( const VString& inString, bool inWithQuotes, VJSONValue& outValue)
{
	if (inWithQuotes)
		outValue.SetString( inString);
	else if (inString.EqualToUSASCIICString( "true"))
		outValue = VJSONValue::sTrue;
	else if (inString.EqualToUSASCIICString( "false"))
		outValue = VJSONValue::sFalse;
	else if (inString.EqualToUSASCIICString( "null"))
		outValue = VJSONValue::sNull;
	else if (!inString.IsEmpty())
	{
		// check if it's a number
		if (_ParseNumber( inString.GetCPointer()))
			outValue.SetNumber( inString.GetReal());
		else
			outValue.SetString( inString);
	}
	else
		outValue.SetString( inString);
	
	return VE_OK;
}


VError VJSONImporter::_ThrowError()
{
	return vThrowError( VE_MALFORMED_JSON_DESCRIPTION);
}


VError VJSONImporter::_ParseObject( VJSONValue& outValue)
{
	VError err = VE_OK;
	VJSONObject *object = new VJSONObject;
	do
	{
		VString name;
		JsonToken token = GetNextJSONToken( name, NULL);
		if (token == jsonString)
		{
			token = GetNextJSONToken();
			if (token == jsonAssigne)
			{
				VJSONValue value;
				err = Parse( value);
				if (err == VE_OK)
				{
					object->SetProperty( name, value);

					token = GetNextJSONToken();
					if (token == jsonEndObject)
						break;

					if (token != jsonSeparator)
						err = _ThrowError();
				}
			}
			else
			{
				err = _ThrowError();
			}
		}
		else if (token == jsonEndObject)
		{
			break;
		}
		else
		{
			err = _ThrowError();
		}
	} while( err == VE_OK);
	outValue.SetObject( object);
	ReleaseRefCountable( &object);
	return err;
}


VError VJSONImporter::_ParseArray( VJSONValue& outValue)
{
	VError err = VE_OK;
	VJSONArray *array = new VJSONArray;
	bool expectSeparator = false;
	do
	{
		VJSONValue value;
		VString s;
		bool withQuotes;

		JsonToken token = GetNextJSONToken( s, &withQuotes);
		if (token == jsonString)
		{
			err = _StringToValue( s, withQuotes, value);
		}
		else if (token == jsonBeginObject)
		{
			err = _ParseObject( value);
		}
		else if (token == jsonBeginArray)
		{
			err = _ParseArray( value);
		}
		else if (token == jsonEndArray)
		{
			break;
		}
		else
		{
			err = _ThrowError();
		}

		if (err == VE_OK)
		{
			xbox_assert( !value.IsUndefined());
			array->Push( value);

			token = GetNextJSONToken();
			if (token == jsonEndArray)
				break;
			
			if (token != jsonSeparator)
				err = _ThrowError();
		}

	} while( err == VE_OK);
	outValue.SetArray( array);
	ReleaseRefCountable( &array);
	return err;
}


VError VJSONImporter::Parse( VJSONValue& outValue)
{
	VError err = VE_OK;

	VJSONValue value;
	
	bool withQuotes;
	VString name;
	JsonToken token = GetNextJSONToken( name, &withQuotes);
	switch( token)
	{
		case jsonString:
			{
				err = _StringToValue( name, withQuotes, value);
				break;
			}
		
		case jsonBeginObject:
			{
				err = _ParseObject( value);
				break;
			}
		
		case jsonBeginArray:
			{
				err = _ParseArray( value);
				break;
			}
		
		default:
			{
				err = _ThrowError();
				break;
			}
	}
	
	outValue = value;

	return err;
}


VError VJSONImporter::JSONObjectToBag( VValueBag& outBag)
{
	VError			err = VE_OK;
	VString			aStr;
	bool			withQuotes;
	
	// When JSONObjectToBag() recursively calls itself (because it finds a jsonBeginObject token)
	// it must not error-check this, because the token has already been read.
	if (fRecursiveCallCount == 0 && GetNextJSONToken(aStr, &withQuotes) != jsonBeginObject)
	{
		err = VE_MALFORMED_JSON_DESCRIPTION;
	}
	else
	{
		VString		name;
		
		JsonToken token = GetNextJSONToken(name, &withQuotes);
		while (token != jsonEndObject && token != jsonNone && err == VE_OK)
		{
			if (token == jsonString)
			{
				token = GetNextJSONToken(aStr, &withQuotes);
				if (token == jsonAssigne)
				{
					token = GetNextJSONToken(aStr, &withQuotes);
					switch (token)
					{
						case jsonString:
							if(name == "__CDATA")
							{
								outBag.SetCData(aStr);
							}
							else if (withQuotes)
							{
								outBag.SetString(name, aStr);
							}
							else if (aStr == L"null")
							{
								//aStr.SetNull(true);
								outBag.SetString(name, aStr);
								VValueSingle* temp = outBag.GetAttribute(name);
								aStr.SetNull(true);
								outBag.SetString(name, aStr);
							}
							else if (aStr == L"true")
							{
								outBag.SetBool(name, true);
							}
							else if (aStr == L"false")
							{
								outBag.SetBool(name, false);
							}
							else if (aStr.FindUniChar('.') > 0)
							{
								outBag.SetReal(name, aStr.GetReal());
							}
							else
							{
								outBag.SetLong8(name, aStr.GetLong8());
							}
							break;
							
						case jsonBeginObject:
						{
							VValueBag* subBag = new VValueBag();
							subBag->SetBool(L"____objectunic", true);
						++fRecursiveCallCount;
							err = JSONObjectToBag(*subBag);
						--fRecursiveCallCount;
							if (err == VE_OK)
							{
								outBag.AddElement(name, subBag);
							}
							subBag->Release();
						}
							break;
							
						case jsonBeginArray:
						{
							token = GetNextJSONToken(aStr, &withQuotes);
							while (token != jsonEndArray && token != jsonNone && err == VE_OK)
							{
								if (token == jsonBeginObject)
								{
									VValueBag* subBag = new VValueBag();
								++fRecursiveCallCount;
									err = JSONObjectToBag(*subBag);
								--fRecursiveCallCount;
									if (err == VE_OK)
									{
										outBag.AddElement(name, subBag);
									}
									subBag->Release();
								}
								token = GetNextJSONToken(aStr, &withQuotes);
								if (token == jsonSeparator)
									token = GetNextJSONToken(aStr, &withQuotes);
							}
							
						}
							break;
							
						default:
							err = VE_MALFORMED_JSON_DESCRIPTION;
							break;
					}
				}
				else
					err = VE_MALFORMED_JSON_DESCRIPTION;
			}
			else
				err = VE_MALFORMED_JSON_DESCRIPTION;
			
			if (err == VE_OK)
			{
				token = GetNextJSONToken(name, &withQuotes);
				if (token == jsonSeparator)
					token = GetNextJSONToken(name, &withQuotes);
			}
		}
	}
	
	return vThrowError(err);
}

// ===========================================================
#pragma mark -
#pragma mark VJSONArrayWriter
// ===========================================================
void VJSONArrayWriter::_ReopenIfNeeded()
{
	if(fIsClosed)
	{
		// Remove the last ']'
		fArrayRef.Remove(fArrayRef.GetLength(), 1);
		fIsClosed = false;
	}
}

void VJSONArrayWriter::Add(const VValueSingle &inAny, JSONOption inModifier)
{
	VString		aStr;
	
	if(testAssert(inAny.GetJSONString(aStr, inModifier) == VE_OK))
	{
		_ReopenIfNeeded();
		
		fArrayRef += aStr;
		fArrayRef += ",";
	}
}

void VJSONArrayWriter::AddString(const char *inCStr, JSONOption inModifier)
{
	if(testAssert(inCStr != NULL))
	{
		_ReopenIfNeeded();

		VString		aStr(inCStr);
		Add(aStr, inModifier);
	}
}

void VJSONArrayWriter::AddBool(bool inValue)
{
	_ReopenIfNeeded();
	
	if(inValue)
		fArrayRef += "true,";
	else
		fArrayRef += "false,";
}

void VJSONArrayWriter::AddLong(sLONG inValue)
{
	_ReopenIfNeeded();

	fArrayRef.AppendLong(inValue);
	fArrayRef += ",";
}

void VJSONArrayWriter::AddReal(Real inValue)
{
	VString	valueStr;
	
	VReal::RealToXMLString(inValue, valueStr);
	
	_ReopenIfNeeded();

	fArrayRef += valueStr;
	fArrayRef += ",";
}

void VJSONArrayWriter::AddString(const VString& inValue, JSONOption inModifier)
{
	VString	valueStr;
	
	inValue.GetJSONString(valueStr, inModifier);
	
	_ReopenIfNeeded();

	fArrayRef += valueStr;
	fArrayRef += ",";
}

void VJSONArrayWriter::AddJSONArray(VJSONArrayWriter &inArray)
{
	VString	arrayStr;
	
	inArray.GetArray(arrayStr, false);
	
	_ReopenIfNeeded();

	fArrayRef += arrayStr;
	fArrayRef += ",";
}

void VJSONArrayWriter::AddLongs(const sLONG *inArray, sLONG inCountOfElements)
{
	if(testAssert(inArray != NULL && inCountOfElements >= 0))
	{
		if(inCountOfElements > 0)
		{
			_ReopenIfNeeded();

			for(sLONG i = 0; i < inCountOfElements; ++i)
				AddLong(*inArray++);	//Add(VLong(*inArray++))
		}
	}
}

void VJSONArrayWriter::AddWords(const sWORD *inArray, sLONG inCountOfElements)
{
	if(testAssert(inArray != NULL && inCountOfElements >= 0))
	{
		if(inCountOfElements > 0)
		{
			_ReopenIfNeeded();

			for(sLONG i = 0; i < inCountOfElements; ++i)
				AddLong((sLONG) *inArray++);	//Add(VLong( (sLONG) (*inArray++)));
		}
	}
}

void VJSONArrayWriter::AddReals(const Real *inArray, sLONG inCountOfElements)
{	
	if(testAssert(inArray != NULL && inCountOfElements >= 0))
	{
		if(inCountOfElements > 0)
		{
			_ReopenIfNeeded();

			for(sLONG i = 0; i < inCountOfElements; ++i)
				AddReal(*inArray++);	//Add(VReal(*inArray++));
		}
	}
}

void VJSONArrayWriter::AddBools(const char *inArray, sLONG inCountOfElements)
{
	if(testAssert(inArray != NULL && inCountOfElements >= 0))
	{
		if(inCountOfElements > 0)
		{
			_ReopenIfNeeded();

			for(sLONG i = 0; i < inCountOfElements; ++i)
				AddBool(*inArray++ != 0);	// Add(VBoolean(*inArray++ != 0));
		}
	}
}

void VJSONArrayWriter::AddStrings(const std::vector<VString> &inStrings, JSONOption inModifier)
{
	if(!inStrings.empty())
	{
		_ReopenIfNeeded();

		std::vector<VString>::const_iterator	strIt = inStrings.begin();
		while(strIt != inStrings.end())
		{
			AddString(*strIt, inModifier);
			++strIt;
		}
	}
}

void VJSONArrayWriter::Close()
{
	if(fIsClosed)
		return;
	
	if(fArrayRef.IsEmpty())
	{
		fArrayRef = "[]";
	}
	else if(fArrayRef == "[") // was opened, but nothing put in it
	{
		fArrayRef += "]";
	}
	else
	{
#if VERSIONDEBUG
		assert(fArrayRef[fArrayRef.GetLength() - 1] == ',');
#endif
		// Remove the last ','
		fArrayRef.Remove(fArrayRef.GetLength(), 1);
		// Close the array
		fArrayRef += "]";
	}
	fIsClosed = true;
}

void VJSONArrayWriter::GetArray(VString &outValue, bool inAndClearIt)
{
	Close();
	outValue = fArrayRef;
	if(!inAndClearIt)
		Clear();
}

// ===========================================================
#pragma mark -
#pragma mark VJSONArrayReader
// ===========================================================
VJSONArrayReader::VJSONArrayReader() :
fLastParseError(VE_OK),
fVectorSize(0)
{
	
}

VJSONArrayReader::VJSONArrayReader(const VString& inJSONString) :
fLastParseError(VE_OK),
fVectorSize(0)
{
	_Parse(inJSONString);
}

void VJSONArrayReader::FromJSONString(const VString& inJSONString)
{
	fLastParseError = VE_OK;
	fValues.clear();
	fVectorSize = 0;
	
	_Parse(inJSONString);
}

void VJSONArrayReader::_Parse(const VString &inJSONString)
{
	if(inJSONString.IsEmpty())
	{
		fLastParseError = VE_MALFORMED_JSON_DESCRIPTION;
	}
	else
	{
		VJSONImporter	jsonImport(inJSONString);
		VString			theStr;
		bool			withquotes;
		VJSONImporter::JsonToken	token = VJSONImporter::jsonNone;
		
		do
		{
			token = jsonImport.GetNextJSONToken(theStr, &withquotes);
			
		} while(token != VJSONImporter::jsonBeginArray && token != VJSONImporter::jsonNone);
		
		if(token == VJSONImporter::jsonBeginArray)
		{
			while (token != VJSONImporter::jsonEndArray && token != VJSONImporter::jsonNone && fLastParseError == VE_OK)
			{
				token = jsonImport.GetNextJSONToken(theStr, &withquotes);
				
				// Nested arrays are just ignored
				if (token == VJSONImporter::jsonBeginArray)
				{
					sLONG	count = 1;
					while(count > 0 && token != VJSONImporter::jsonNone)
					{
						token = jsonImport.GetNextJSONToken();
						if(token ==  VJSONImporter::jsonEndArray)
							--count;
						else if(token ==  VJSONImporter::jsonBeginArray)
							++count;
					}
					if(count == 0 && token == VJSONImporter::jsonEndArray)
						token = jsonImport.GetNextJSONToken(theStr, &withquotes);
					else
						fLastParseError = VE_MALFORMED_JSON_DESCRIPTION;
				}
				
				if(fLastParseError == VE_OK && token == VJSONImporter::jsonString)
				{
					fValues.push_back(theStr);
				}
			};
			
			if(token != VJSONImporter::jsonEndArray && fLastParseError == VE_OK)
				fLastParseError = VE_MALFORMED_JSON_DESCRIPTION;
		}
		else
		{
			fLastParseError = VE_MALFORMED_JSON_DESCRIPTION;
		}
	}
	
	if(fLastParseError !=  VE_OK)
		fValues.clear();
	
	fVectorSize = (sLONG) fValues.size();
}

sLONG VJSONArrayReader::GetCountOfElements(VError *outParsingError)
{
	if(outParsingError != NULL)
		*outParsingError = fLastParseError;
	
	return fVectorSize;
}

VError VJSONArrayReader::ToArrayShort(sWORD *outArray)
{
	if(fLastParseError == VE_OK)
	{
		if(testAssert(outArray != NULL))
		{			
			for(sLONG i = 0; i < fVectorSize; ++i)
				outArray[i] = fValues[i].GetWord();
		}
	}
	
	return fLastParseError;
}

VError VJSONArrayReader::ToArrayLong(sLONG *outArray)
{
	if(fLastParseError == VE_OK)
	{
		if(testAssert(outArray != NULL))
		{				
			for(sLONG i = 0; i < fVectorSize; ++i)
				outArray[i] = fValues[i].GetLong();
		}
	}
	
	return fLastParseError;
}

VError VJSONArrayReader::ToArrayReal(Real *outArray)
{
	if(fLastParseError == VE_OK)
	{
		if(testAssert(outArray != NULL))
		{				
			for(sLONG i = 0; i < fVectorSize; ++i)
				outArray[i] = fValues[i].GetReal();
		}
	}
	
	return fLastParseError;
}

VError VJSONArrayReader::ToArrayBool(char *outArray)
{
	if(fLastParseError == VE_OK)
	{
		if(testAssert(outArray != NULL))
		{			
			for(sLONG i = 0; i < fVectorSize; ++i)
				outArray[i] = (char) (fValues[i] == "true" ? 1 : 0);
		}
	}
	
	return fLastParseError;
}

/*
template <typename ScalarType>
VError VJSONArrayReader::ToArray(ScalarType *outArray)
{
	if(fLastParseError == VE_OK)
	{
		if(testAssert(outArray != NULL))
		{			
			const std::type_info&	scalarType = typeid(ScalarType);
			
		// Loops are unrolled, to avoid testing the type for each iteration (speed optimization)
		// static_cast(> is used to avoid compiler warnings (conversion long to byte for example, "loss of data")
			if(scalarType == typeid(sWORD) || scalarType == typeid(uWORD))
			{
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = static_cast<ScalarType>(fValues[i].GetWord());
			}
			else if(scalarType == typeid(sLONG) || scalarType == typeid(uLONG))
			{
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = static_cast<ScalarType>(fValues[i].GetLong());
			}
			else if(scalarType == typeid(Real))
			{
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = static_cast<ScalarType>(fValues[i].GetReal());
			}
			else if(scalarType == typeid(sBYTE) || scalarType == typeid(uBYTE) || scalarType == typeid(sCHAR) || scalarType == typeid(uCHAR) || scalarType == typeid(char))
			{
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = static_cast<ScalarType>(fValues[i].GetByte());
			}
			else if(scalarType == typeid(bool))
			{
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = static_cast<ScalarType>(fValues[i].GetBoolean());
			}
			else if(scalarType == typeid(sLONG8) || scalarType == typeid(uLONG8))
			{
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = static_cast<ScalarType>(fValues[i].GetLong8());
			}
			else
			{
				xbox_assert(false);
				for(sLONG i = 0; i < fVectorSize; ++i)
					outArray[i] = (ScalarType) fValues[i].GetReal(); // Let the compiler cast the thing
			}
		}
	}
	
	return fLastParseError;
}
*/

VError VJSONArrayReader::ToArrayString(VectorOfVString &outStrings, bool inDoAppend)
{
	if(!inDoAppend)
		outStrings.clear();

	if(fLastParseError == VE_OK)
	{
		if(!inDoAppend)
		{
			outStrings = fValues;
		}
		else if(!fValues.empty())
		{
			outStrings.insert(outStrings.end(), fValues.begin(), fValues.end());
		}
	}
	
	return fLastParseError;
}




VJSONSingleObjectWriter::VJSONSingleObjectWriter()
: fIsClosed(false), fMembersCount(0)
{
	fObject.AppendUniChar( '{');
}


VJSONSingleObjectWriter::~VJSONSingleObjectWriter()
{
}


bool VJSONSingleObjectWriter::AddMember( const VString& inName, const VValueSingle &inValue, JSONOption inModifier)
{
	bool ok = false;
	VString valueString;

	if(testAssert(inValue.GetJSONString( valueString, inModifier) == VE_OK) && !inName.IsEmpty())
	{
		if (fIsClosed)
		{
			fObject.Remove( fObject.GetLength(), 1);
			fIsClosed = false;
		}

		if (fMembersCount > 0)
			fObject.AppendUniChar( ',');
		
		fObject.AppendUniChar( '"');
		fObject.AppendString( inName);
		fObject.AppendUniChar( '"');
		fObject.AppendUniChar( ':');
		fObject.AppendString( valueString);
		++fMembersCount;
		ok = true;
	}
	return ok;
}


void VJSONSingleObjectWriter::GetObject( VString& outObject)
{
	if (!fIsClosed)
	{
		fObject.AppendUniChar( '}');
		fIsClosed = true;
	}
	outObject.FromString( fObject);
}


END_TOOLBOX_NAMESPACE
