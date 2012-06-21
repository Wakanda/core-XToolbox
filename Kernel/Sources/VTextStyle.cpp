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
#include "VTextStyle.h"
#include "VString.h"

//in order to be able to use std::min && std::max
#undef max
#undef min

VTextStyle::VTextStyle(VTextStyle* inTextStyle)
{
	fFontName = new VString();
	if(inTextStyle)
	{
		fIsBold = inTextStyle->fIsBold;
		fIsUnderline = inTextStyle->fIsUnderline;
		fIsStrikeout = inTextStyle->fIsStrikeout;
		fIsItalic = inTextStyle->fIsItalic;
		fBegin = inTextStyle->fBegin;
		fEnd = inTextStyle->fEnd;
		fFontSize = inTextStyle->fFontSize;
		*fFontName = *(inTextStyle->fFontName);
		fIsTransparent = inTextStyle->fIsTransparent;
		fHasForeColor = inTextStyle->fHasForeColor;
		fJust = inTextStyle->fJust;
		fColor = inTextStyle->fColor;
		fBkColor = inTextStyle->fBkColor;
	}
	else
	{
		fIsBold = fIsUnderline = fIsStrikeout = fIsItalic = UNDEFINED_STYLE;
		fBegin = fEnd = 0;
		fFontSize = -1;
		*fFontName = "";
		fColor = 0;
		fBkColor = 0xFFFFFF;
		fIsTransparent = true;
		fHasForeColor = false;
		fJust = JST_Notset;
	}
}

VTextStyle& VTextStyle::operator = ( const VTextStyle& inTextStyle)
{
	if (this == &inTextStyle)
		return *this;

	fFontName = new VString(*(inTextStyle.fFontName));
	fIsBold = inTextStyle.fIsBold;
	fIsUnderline = inTextStyle.fIsUnderline;
	fIsStrikeout = inTextStyle.fIsStrikeout;
	fIsItalic = inTextStyle.fIsItalic;
	fBegin = inTextStyle.fBegin;
	fEnd = inTextStyle.fEnd;
	fFontSize = inTextStyle.fFontSize;
	fIsTransparent = inTextStyle.fIsTransparent;
	fHasForeColor = inTextStyle.fHasForeColor;
	fJust = inTextStyle.fJust;
	fColor = inTextStyle.fColor;
	fBkColor = inTextStyle.fBkColor;

	return  *this;
}

/** return true if this style & the input style are equal */
bool VTextStyle::IsEqualTo( const VTextStyle& inStyle, bool inIncludeRange) const
{
	if (inIncludeRange)
	{
		if (!(	fBegin == inStyle.fBegin
				&&
				fEnd == inStyle.fEnd
			  )
		   )
		   return false;
	}
	return (fIsBold == inStyle.fIsBold
			&&
			fIsUnderline == inStyle.fIsUnderline
			&&
			fIsStrikeout == inStyle.fIsStrikeout
			&&
			fIsItalic == inStyle.fIsItalic
			&&
			fFontSize == inStyle.fFontSize
			&&
			fHasForeColor == inStyle.fHasForeColor
			&&
			fIsTransparent == inStyle.fIsTransparent
			&&
			(!fHasForeColor || (fHasForeColor && fColor == inStyle.fColor))
			&&
			(fIsTransparent || (!fIsTransparent && fBkColor == inStyle.fBkColor))
			&&
			fJust == inStyle.fJust
			&&
			fFontName->EqualToString( *(inStyle.fFontName))
			);
}


/** return style which have properties common to this style & the input style 
	and reset properties which are common in the two styles
	(NULL if none property is common)
*/
VTextStyle *VTextStyle::ExtractCommonStyle(VTextStyle *ioStyle2)
{
	if (!ioStyle2)
		return NULL;

	VTextStyle *retStyle = NULL;
	if (fIsBold != UNDEFINED_STYLE && fIsBold == ioStyle2->fIsBold)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsBold = fIsBold;
		fIsBold = UNDEFINED_STYLE;
		ioStyle2->fIsBold = UNDEFINED_STYLE;
	}
	if (fIsUnderline != UNDEFINED_STYLE && fIsUnderline == ioStyle2->fIsUnderline)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsUnderline = fIsUnderline;
		fIsUnderline = UNDEFINED_STYLE;
		ioStyle2->fIsUnderline = UNDEFINED_STYLE;
	}
	if (fIsStrikeout != UNDEFINED_STYLE && fIsStrikeout == ioStyle2->fIsStrikeout)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsStrikeout = fIsStrikeout;
		fIsStrikeout = UNDEFINED_STYLE;
		ioStyle2->fIsStrikeout = UNDEFINED_STYLE;
	}
	if (fIsItalic != UNDEFINED_STYLE && fIsItalic == ioStyle2->fIsItalic)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsItalic = fIsItalic;
		fIsItalic = UNDEFINED_STYLE;
		ioStyle2->fIsItalic = UNDEFINED_STYLE;
	}
	if (fJust != JST_Notset && fJust == ioStyle2->fJust)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fJust = fJust;
		fJust = JST_Notset;
		ioStyle2->fJust = JST_Notset;
	}
	if (fFontSize != UNDEFINED_STYLE && fFontSize == ioStyle2->fFontSize)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fFontSize = fFontSize;
		fFontSize = UNDEFINED_STYLE;
		ioStyle2->fFontSize = UNDEFINED_STYLE;
	}
	if (!fFontName->IsEmpty() && fFontName->EqualToString( *(ioStyle2->fFontName)))
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fFontName->FromString( *fFontName);
		fFontName->SetEmpty();
		ioStyle2->fFontName->SetEmpty();
	}
	if (fHasForeColor && ioStyle2->fHasForeColor && fColor == ioStyle2->fColor)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fHasForeColor = fHasForeColor;
		retStyle->fColor = fColor;

		SetHasForeColor( false);
		ioStyle2->SetHasForeColor( false);
	}
	if (!fIsTransparent && !ioStyle2->fIsTransparent && fBkColor == ioStyle2->fBkColor)
	{
		if (!retStyle)
			retStyle = new VTextStyle();
		retStyle->fIsTransparent = fIsTransparent;
		retStyle->fBkColor = fBkColor;

		SetTransparent( true);
		ioStyle2->SetTransparent( true);
	}
	return retStyle;
}

VTextStyle::~VTextStyle()
{
	if(fFontName)
		delete fFontName;
}

void VTextStyle::SetRange(sLONG inBegin, sLONG inEnd)
{
	fBegin = inBegin; 
	fEnd = inEnd;
}
	
void VTextStyle::GetRange(sLONG& outBegin, sLONG& outEnd) const	
{
	outBegin = fBegin; 
	outEnd = fEnd;
}

void VTextStyle::SetBold(sLONG inValue)													
{
	fIsBold = inValue;
}

sLONG VTextStyle::GetBold() const															
{
	return fIsBold;
}

void VTextStyle::SetItalic(sLONG inValue)												
{
	fIsItalic = inValue;
}

sLONG VTextStyle::GetItalic() const														
{
	return fIsItalic;
}

void VTextStyle::SetUnderline(sLONG inValue)												
{
	fIsUnderline = inValue;
}

sLONG VTextStyle::GetUnderline() const													
{
	return fIsUnderline;
}

void VTextStyle::SetStrikeout(sLONG inValue)												
{
	fIsStrikeout = inValue;
}

sLONG VTextStyle::GetStrikeout() const													
{
	return fIsStrikeout;
}

void VTextStyle::SetColor(const RGBAColor& inValue)									
{
	fColor = inValue;
}

RGBAColor VTextStyle::GetColor() const														
{
	return fColor;
}

void VTextStyle::SetBackGroundColor(const RGBAColor& inValue)							
{
	fBkColor = inValue;
}

RGBAColor VTextStyle::GetBackGroundColor() const												
{
	return fBkColor;
}

void VTextStyle::SetTransparent(bool inValue)											
{
	fIsTransparent = inValue;
	if (inValue)
		fBkColor = 0xFFFFFF;
}

bool VTextStyle::GetTransparent() const													
{
	return fIsTransparent;
}

void VTextStyle::SetHasForeColor(bool inValue)											
{
	fHasForeColor = inValue;
	if (!fHasForeColor)
		fColor = 0;
}

bool VTextStyle::GetHasForeColor() const													
{
	return fHasForeColor;
}

void VTextStyle::SetFontSize(const Real& inValue)										
{
	fFontSize = inValue;
}

Real VTextStyle::GetFontSize() const													
{
	return fFontSize;
}

void VTextStyle::SetFontName(const VString& inValue)								
{
	//deal with CSS composite font name - ie '"Tahoma", "Sans Serif"' for instance - parsed from HTML or RTF
	VIndex posComma = inValue.Find(",");
	if (posComma >= 1)
	{
		//truncate to first font name & remove quotes
		VString value;
		value = inValue;
		value.Truncate(posComma-1);
		bool hasStartQuote =(value.GetLength() >= 1 
							&& 
							(
							value.BeginsWith("\"")
							||
							value.BeginsWith("'")
							)
							);
		bool hasEndQuote =	(value.GetLength() >= 1 
							&& 
							(
							value.EndsWith("\"")
							||
							value.EndsWith("'")
							)
							);
		if (hasStartQuote)
			value.SubString( 2, value.GetLength()-1);
		if (hasEndQuote && value.GetLength() >= 1)
			value.Truncate(value.GetLength()-1);
		*fFontName = value;
		return;
	}
	else
	{
		bool hasStartQuote =(inValue.GetLength() >= 1 
							&& 
							(
							inValue.BeginsWith("\"")
							||
							inValue.BeginsWith("'")
							)
							);
		bool hasEndQuote =	(inValue.GetLength() >= 1 
							&& 
							(
							inValue.EndsWith("\"")
							||
							inValue.EndsWith("'")
							)
							);
		if (hasStartQuote || hasEndQuote)
		{
			//remove quotes
			VString value = inValue;
			if (hasStartQuote)
				value.SubString( 2, value.GetLength()-1);
			if (hasEndQuote && value.GetLength() >= 1)
				value.Truncate(value.GetLength()-1);
			*fFontName = value;
			return;
		}
	}
	
	*fFontName = inValue;
}

const VString& VTextStyle::GetFontName() const													
{
	return *fFontName;
}

void VTextStyle::SetJustification(justificationStyle inValue)	
{
	fJust = inValue;
}

justificationStyle VTextStyle::GetJustification() const				
{
	return fJust;
}

void VTextStyle::Translate(sLONG inValue)
{
	if(inValue) 
	{ 
		fBegin+= inValue;
		fEnd += inValue;
	}
}

/** merge current style with the specified style 
@remarks
	input style properties override current style properties without taking account input style range
	(current style range is preserved)
*/
void VTextStyle::MergeWithStyle( const VTextStyle *inStyle, const VTextStyle *inStyleParent)
{
	if (inStyleParent)
	{
		if (inStyle->GetBold() != UNDEFINED_STYLE)
			SetBold( inStyleParent->GetBold() == inStyle->GetBold() ? UNDEFINED_STYLE : inStyle->GetBold());
		if (inStyle->GetItalic() != UNDEFINED_STYLE)
			SetItalic( inStyleParent->GetItalic() == inStyle->GetItalic() ? UNDEFINED_STYLE : inStyle->GetItalic());
		if (inStyle->GetUnderline() != UNDEFINED_STYLE)
			SetUnderline( inStyleParent->GetUnderline() == inStyle->GetUnderline() ? UNDEFINED_STYLE : inStyle->GetUnderline());
		if (inStyle->GetStrikeout() != UNDEFINED_STYLE)
			SetStrikeout( inStyleParent->GetStrikeout() == inStyle->GetStrikeout() ? UNDEFINED_STYLE : inStyle->GetStrikeout());
		if (inStyle->GetFontSize() != UNDEFINED_STYLE)
			SetFontSize( inStyleParent->GetFontSize() == inStyle->GetFontSize() ? UNDEFINED_STYLE : inStyle->GetFontSize());
		if (!inStyle->GetFontName().IsEmpty())
			SetFontName( inStyleParent->GetFontName().EqualToString(inStyle->GetFontName(), true) ? VString("") : inStyle->GetFontName());

		if (inStyle->GetHasForeColor())
		{
			if	((!inStyleParent->GetHasForeColor()) || inStyleParent->GetColor() != inStyle->GetColor())
			{
				SetHasForeColor(true);
				SetColor(inStyle->GetColor());
			}
			else
			{
				SetHasForeColor(false);
				SetColor(0);
			}
		}
		if (!inStyle->GetTransparent())
		{
			if (inStyleParent->GetTransparent() || inStyleParent->GetBackGroundColor() != inStyle->GetBackGroundColor())
			{
				SetTransparent( false);
				SetBackGroundColor( inStyle->GetBackGroundColor());
			}
			else
			{
				SetTransparent(true);
			}
		}
		if (inStyle->GetJustification() != JST_Notset)
			SetJustification( inStyleParent->GetJustification() == inStyle->GetJustification() ? JST_Notset : inStyle->GetJustification());
	}
	else
	{
		if (inStyle->GetBold() != UNDEFINED_STYLE)
			SetBold( inStyle->GetBold());
		if (inStyle->GetItalic() != UNDEFINED_STYLE)
			SetItalic( inStyle->GetItalic());
		if (inStyle->GetUnderline() != UNDEFINED_STYLE)
			SetUnderline( inStyle->GetUnderline());
		if (inStyle->GetStrikeout() != UNDEFINED_STYLE)
			SetStrikeout( inStyle->GetStrikeout());
		if (inStyle->GetFontSize() != UNDEFINED_STYLE)
			SetFontSize( inStyle->GetFontSize());
		if (!inStyle->GetFontName().IsEmpty())
			SetFontName( inStyle->GetFontName());

		if (inStyle->GetHasForeColor())
		{
			SetHasForeColor(true);
			SetColor(inStyle->GetColor());
		}
		if (!inStyle->GetTransparent())
		{
			SetTransparent( false);
			SetBackGroundColor( inStyle->GetBackGroundColor());
		}
		if (inStyle->GetJustification() != JST_Notset)
			SetJustification( inStyle->GetJustification());
	}
}

/** return true if none property is defined (such style is useless & should be discarded) */
bool VTextStyle::IsUndefined() const
{
	return	GetBold() == UNDEFINED_STYLE
			&&
			GetItalic() == UNDEFINED_STYLE
			&&
			GetUnderline() == UNDEFINED_STYLE
			&&
			GetStrikeout() == UNDEFINED_STYLE
			&&
			GetFontSize() == UNDEFINED_STYLE
			&&
			GetFontName().IsEmpty()
			&&
			(!GetHasForeColor())
			&&
			GetTransparent()
			&&
			GetJustification() == JST_Notset;
}

void VTextStyle::Reset()
{
	fBegin = fEnd = 0;
	fIsBold = fIsUnderline = fIsStrikeout = fIsItalic = UNDEFINED_STYLE;
	fColor = 0;
	fBkColor = 0xFFFFFF;
	fIsTransparent = true;
	fHasForeColor = false;
	fFontSize = -1;
	if (fFontName)
		fFontName->SetEmpty();
	else 
		fFontName = new VString();
	fJust = JST_Notset;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////

VTreeTextStyle::VTreeTextStyle(VTextStyle* inData):IRefCountable()
{
	fParent = NULL;
	fdata = inData;
}

VTreeTextStyle::VTreeTextStyle(VTreeTextStyle* inStyle, bool inDeepCopy):IRefCountable()
{
	fParent = NULL;
	fdata = NULL;
	if(inStyle)
	{
		VTextStyle* data = inStyle->GetData();
		if(data)
		{
			fdata = new VTextStyle(data);

			if(inDeepCopy)
			{
				for( VectorOfStyle::const_iterator i = inStyle->fChildren.begin() ; i != inStyle->fChildren.end() ; ++i)
				{
					VTreeTextStyle* newstyle = new VTreeTextStyle( *i);
					AddChild(newstyle);
					newstyle->Release();
				}
			}
		}
	}
}

VTreeTextStyle::~VTreeTextStyle()
{
	for( VectorOfStyle::iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
	{
		(*i)->SetParent(NULL);
		(*i)->Release();
	}
	delete fdata;
}

void VTreeTextStyle::SetParent(VTreeTextStyle* inValue)
{
	fParent = inValue;
}

/** return true if font family is defined */
bool VTreeTextStyle::HasFontFamilyOverride() const
{
	bool fontoverride = false;

	VTextStyle *style = GetData();
	if (style)
	{
		sLONG start, end;
		style->GetRange( start, end);

		if (start < end)
			if (!style->GetFontName().IsEmpty())
				fontoverride = true;
	}
	if (!fontoverride)
	{
		for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			VTreeTextStyle* vstyle = *i;
			fontoverride = vstyle->HasFontFamilyOverride();
			if (fontoverride)
				break;
		}
	}
	return fontoverride;
}

void VTreeTextStyle::AddChild(VTreeTextStyle* inValue, bool inAutoGrowRange)
{
	if(inValue)
	{
		fChildren.push_back(inValue);
		inValue->Retain();
		inValue->SetParent(this);

		if (inAutoGrowRange && inValue->GetData() && GetData())
		{
			sLONG start, end;
			GetData()->GetRange( start, end);

			sLONG startChild, endChild;
			inValue->GetData()->GetRange( startChild, endChild);
			if (startChild < start)
				start = startChild;
			if (endChild > end)
				end = endChild;
			GetData()->SetRange(start, end);
		}

	}
}

void VTreeTextStyle::RemoveChildAt(sLONG inNth)
{
	if (fChildren.size() == 0) 
		return;

	if(testAssert(inNth >= 1 && inNth <= fChildren.size()))
	{
		VTreeTextStyle* child = fChildren[inNth - 1];

		VectorOfStyle::iterator del_Iter;
		del_Iter = fChildren.begin();
		fChildren.erase(del_Iter+(inNth -1));
		
		child->SetParent(NULL);
		child->Release();
	}
}

void VTreeTextStyle::Translate(sLONG inValue)
{
	if(inValue)
	{
		GetData()->Translate(inValue);
		for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
		{
			VTreeTextStyle* vstyle = *i;
			vstyle->Translate(inValue);
		}
	}
}


/** expand styles ranges from the specified range position by the specified offset 
@remarks
	should be called just after text has been inserted in a multistyled text
	(because inserted text should inherit style at current inserted text position-1)
	for instance:
		text.Insert(textToInsert, 10); //caution: here 1-based index
		styles->ExpandRangesAtPosBy( 9, textToInsert.GetLength()); //caution: here 0-based index so 10-1 and not 10
*/
void VTreeTextStyle::ExpandAtPosBy(sLONG inStart, sLONG inInflate)
{
	VTextStyle *style = GetData();
	sLONG start, end;
	style->GetRange( start, end);

	if (end < inStart)
		return;
	if (start >= inStart && start > 0) 
	{
		Translate(inInflate);
		return;
	}
	style->SetRange( start, end+inInflate);

	for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ; ++i)
	{
		VTreeTextStyle* vstyle = *i;
		vstyle->ExpandAtPosBy( inStart, inInflate);
	}
}


/** truncate the specified style range
@param inStart
	start of truncated range
@param inLength
	length of truncated range (-1 to truncate from inStart to the end of this VTreeTextStyle range)
@remarks
	should be called just after text has been deleted in a multistyled text
	for instance:
		text.Remove(10, 5); //caution: here 1-based index
		styles->Truncate(9, 5); //caution: here 0-based index so 10-1 and not 10
*/
void VTreeTextStyle::Truncate(sLONG inStart, sLONG inLength)
{
	VTextStyle *style = GetData();
	sLONG start, end;
	style->GetRange( start, end);

	sLONG inEnd;
	if (inLength < 0)
		inEnd =  end;
	else
		inEnd = inStart+inLength;

	if (inStart > end)
		return;
	if (inEnd <= start)
	{
		Translate(-(inEnd-inStart));
		return;
	}
	
	sLONG inStartBase = inStart;
	sLONG inEndBase = inEnd;

	if (inEnd > end)
		inEnd = end;
	//if (inEnd < start)
	//	inEnd = start;
	if (inStart < start)
		inStart = start;
	//if (inStart > end)
	//	inStart = end;
	if (inStart >= inEnd)
		return;

	if (inStartBase > start)
		style->SetRange( start, end-(inEnd-inStart));
	else
		style->SetRange( inStartBase, inStartBase+end-start-(inEnd-inStart));

	style->GetRange( start, end);

	sLONG index = 0;
	for( VectorOfStyle::const_iterator i = fChildren.begin() ; i != fChildren.end() ;)
	{
		VTreeTextStyle* vstyle = *i;
		sLONG childStart, childEnd;
		vstyle->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inStartBase && childEnd <= inEndBase)
		{
			//child style is fully truncated: skip & remove it
			RemoveChildAt( index+1);
			i = fChildren.begin();
			std::advance( i, index);
			if (i == fChildren.end())
				break;
			else
				continue;
		}

		vstyle->Truncate( inStartBase, inEndBase-inStartBase);

		vstyle->GetData()->GetRange( childStart, childEnd);
		if (childStart <= start && childEnd >= end && vstyle->GetChildCount() <= 0)
		{
			//truncated child style cover same range as actual style: merge with child style...
			style->MergeWithStyle( vstyle->GetData(), fParent ? fParent->GetData() : NULL);
			
			//...and remove child style
			RemoveChildAt( index+1);
			i = fChildren.begin();
			std::advance( i, index);
			if (i == fChildren.end())
				break;
		}
		else
		{
			i++;
			index++;
		}
	}
}

VTreeTextStyle* VTreeTextStyle::GetParent()const 
{
	return fParent;
}

void VTreeTextStyle::InsertChild(VTreeTextStyle* inValue, sLONG pos, bool inAutoGrowRange)
{
	if(inValue)
	{
		inValue->SetParent(this);
		fChildren.insert( fChildren.begin() + (pos -1),inValue);
		inValue->Retain();

		if (inAutoGrowRange && inValue->GetData() && GetData())
		{
			sLONG start, end;
			GetData()->GetRange( start, end);

			sLONG startChild, endChild;
			inValue->GetData()->GetRange( startChild, endChild);
			if (startChild < start)
				start = startChild;
			if (endChild > end)
				end = endChild;
			GetData()->SetRange(start, end);
		}
	}
}

void VTreeTextStyle::SetData(VTextStyle* inValue)	
{
	fdata = inValue;
}

VTextStyle* VTreeTextStyle::GetData () const		
{
	return fdata;
}

sLONG VTreeTextStyle::GetChildCount() const					
{
	return (sLONG) fChildren.size();
}

VTreeTextStyle* VTreeTextStyle::GetNthChild(sLONG inNth) const
{
	if(inNth > GetChildCount() || inNth <= 0)
		return NULL;
	else
		return fChildren[inNth-1];
}


/** build a vector of sequential styles from the current cascading styles 
@remarks
	output styles ranges are disjoint & contain merged cascading styles & n+1 style range follows immediately n style range
*/
void VTreeTextStyle::BuildSequentialStylesFromCascadingStyles( std::vector<VTextStyle *>& outStyles, VTextStyle *inStyleParent, bool inForMetrics)
{
	VTextStyle *styleCur = GetData();
	if (styleCur)
	{
		VTextStyle *style = NULL;
		if (inStyleParent)
		{
			//merge both styles
			style = new VTextStyle( inStyleParent);
			if (styleCur->GetBold() != UNDEFINED_STYLE)
				style->SetBold( styleCur->GetBold());
			if (styleCur->GetItalic() != UNDEFINED_STYLE)
				style->SetItalic( styleCur->GetItalic());
			if (styleCur->GetUnderline() != UNDEFINED_STYLE)
				style->SetUnderline( styleCur->GetUnderline());
			if (styleCur->GetStrikeout() != UNDEFINED_STYLE)
				style->SetStrikeout( styleCur->GetStrikeout());
			if (styleCur->GetFontSize() != UNDEFINED_STYLE)
				style->SetFontSize( styleCur->GetFontSize());
			if (!styleCur->GetFontName().IsEmpty())
				style->SetFontName( styleCur->GetFontName());

			if (!inForMetrics)
			{
				//if output styles are used only for metrics, we do not care about color or justify mode
				if (styleCur->GetHasForeColor())
				{
					style->SetHasForeColor(true);
					style->SetColor(styleCur->GetColor());
				}
				if (!styleCur->GetTransparent())
				{
					style->SetTransparent( false);
					style->SetBackGroundColor( styleCur->GetBackGroundColor());
				}
				if (styleCur->GetJustification() != JST_Notset)
					style->SetJustification( styleCur->GetJustification());
			}
		}
		else
			style = new VTextStyle( styleCur);

		sLONG start,end;
		styleCur->GetRange( start, end);
		sLONG endCur = end;
		if (GetChildCount() > 0)
		{
			sLONG childStart, childEnd;
			GetNthChild(1)->GetData()->GetRange(childStart, childEnd);
			end = childStart;
			style->SetRange( start, end);
			bool doDeleteStyle = false;
			if (end > start)
				outStyles.push_back( style);
			else
				doDeleteStyle = true;

			for (int i = 1; i <= GetChildCount(); i++)
			{
				GetNthChild(i)->BuildSequentialStylesFromCascadingStyles( outStyles, style, inForMetrics);
				outStyles.back()->GetRange(childStart, childEnd);
				start = childEnd;
				if (start >= endCur)
				{
					if (doDeleteStyle)
						delete style;
					return;
				}
				else
				{
					if (i+1 <= GetChildCount())
					{
						GetNthChild(i+1)->GetData()->GetRange(childStart, childEnd);
						if (start < childStart)
						{
							if (!doDeleteStyle)
								style = new VTextStyle( style);
							doDeleteStyle = false;
							style->SetRange( start, childStart);
							outStyles.push_back( style);
						}
					}
					else
					{
						if (!doDeleteStyle)
							style = new VTextStyle( style);
						doDeleteStyle = false;
					}
				}
			}
		}
		if (start < endCur)
		{
			style->SetRange( start, endCur);
			outStyles.push_back( style);
		}
		else
			delete style;
	}
}


/** convert cascading styles to sequential styles 
@remarks
	cascading styles are merged in order to build a tree which has only one level deeper;
	on output, child VTreeTextStyle ranges will be disjoint & n+1 style range follows immediately n style range
*/
void VTreeTextStyle::BuildSequentialStylesFromCascadingStyles( VTextStyle *inStyleParent)
{
	std::vector<VTextStyle *> vstyle;
	BuildSequentialStylesFromCascadingStyles( vstyle, inStyleParent);

	for (int j = GetChildCount(); j >= 1; j--)
		RemoveChildAt(j);
	xbox_assert(GetChildCount() == 0);

	sLONG start, end;
	GetData()->GetRange( start, end);
	delete GetData();
	SetData( new VTextStyle());
	GetData()->SetRange(start, end);
	for (int i = 0; i < vstyle.size(); i++)
	{
		VTreeTextStyle *styles = new VTreeTextStyle( vstyle[i]);
		AddChild( styles);
		styles->Release();
	}
}

/** build cascading styles from the input sequential styles 
@remarks
	input styles ranges should not overlap  

	this methods overrides current styles with the input styles
	this class owns input VTextStyle instances so caller should not delete it
*/
void VTreeTextStyle::BuildCascadingStylesFromSequentialStyles( std::vector<VTextStyle *>& inStyles)
{
	if (!GetData())
		SetData( new VTextStyle());

	for (int j = GetChildCount(); j >= 1; j--)
		RemoveChildAt(j);
	xbox_assert(GetChildCount() == 0);

	VTextStyle *curStyle = GetData();
	if (inStyles.size() == 1)
	{
		*curStyle = *(inStyles[0]);
		delete inStyles[0];
	}
	else
	{
		curStyle->Reset();
		if (!inStyles.size())
			return;

		curStyle->SetRange(0, XBOX::MaxLongInt);
		curStyle->SetJustification( inStyles[0]->GetJustification()); //get global justification from first style
		
		sLONG start = XBOX::MaxLongInt;
		sLONG end = 0;

		//first determine full range
		std::vector<VTextStyle *>::iterator it = inStyles.begin();
		for (;it != inStyles.end(); it++)
		{
			VTextStyle *styleChild = *it;
			xbox_assert(styleChild);

			styleChild->SetJustification( JST_Notset); //justification is global & set on parent
			if (styleChild->IsUndefined())
			{
				delete styleChild;
				*it = NULL;
			}
			else
			{
				sLONG childStart, childEnd;
				styleChild->GetRange( childStart, childEnd);
				if (childStart <= 0 || childStart < childEnd)
				{
					start = std::min( start, childStart);
					end = std::max( end, childEnd);
				}
				else
				{
					delete styleChild;
					*it = NULL;
				}
			}
		}
		curStyle->SetRange( start, end);

		//then apply styles
		it = inStyles.begin();
		for (;it != inStyles.end(); it++)
		{
			VTextStyle *styleChild = *it;
			if (!styleChild)
				continue;

			ApplyStyle( styleChild);

			delete styleChild;
		}
	}
}



/** apply style to the specified range 
@param inStyle
	style to apply 
*/
void VTreeTextStyle::ApplyStyle( VTextStyle *inStyle)
{
	_ApplyStyleRec( inStyle, 0);
}

/** apply style to the specified range 
@param inStyle
	style to apply 
*/
void VTreeTextStyle::_ApplyStyleRec( VTextStyle *inStyle, sLONG inRecLevel, VTextStyle *inStyleInherit)
{
	sLONG inStart, inEnd;
	inStyle->GetRange( inStart, inEnd);
	if (inStart >= inEnd && inStart > 0)
		return;

	if (inStyle->IsUndefined())
		return;

	sLONG curStart, curEnd;
	GetData()->GetRange( curStart, curEnd);

	if (inStart <= curStart && inEnd >= curEnd)
	{
		//merge directly with current style if input style range fully includes current style range
		_MergeWithStyle( inStyle, inStyleInherit);
		return;
	}

	if (inStart < curStart)
		inStart = curStart;
	if (inStart > curEnd)
		inStart = curEnd;

	if (inEnd < curStart)
		inEnd = curStart;
	if (inEnd > curEnd)
		inEnd = curEnd;

	if (inStart >= inEnd)
	{
		return;
	}

	//compute inherited styles to pass to children = inherited styles + current styles
	VTextStyle *styleInheritForChilds = NULL;
	if (inStyleInherit)
	{
		styleInheritForChilds = new VTextStyle( inStyleInherit);
		styleInheritForChilds->MergeWithStyle( GetData(), NULL);
	}
	else if (GetParent())
	{
		styleInheritForChilds = new VTextStyle(GetParent()->GetData());
		styleInheritForChilds->MergeWithStyle( GetData(), NULL);
	}
	else
		styleInheritForChilds = NULL;

	//ensure inStyle overrides only styles not equal to inherited styles
	VTextStyle *newStyle = new VTextStyle();
	xbox_assert(newStyle);
	newStyle->SetRange( inStart, inEnd);
	newStyle->MergeWithStyle( inStyle, styleInheritForChilds ? styleInheritForChilds : GetData());
	bool applyOnCurrent = !newStyle->IsUndefined();

	//recursively apply to child style

	//apply style recursively 
	sLONG start = inStart;
	sLONG indexLastChild = 0;
	VTextStyle *styleLastChild = NULL;
	VTreeTextStyle *stylesLastChild = NULL;
	sLONG endLastChild = 0;
	sLONG startLastChild = 0;
	int i = 1;
	for (;i <= GetChildCount();)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		sLONG endTemp = std::min(childStart, inEnd);
		if (applyOnCurrent && start < endTemp)
		{
			if (styleLastChild && endLastChild == start && styleLastChild->IsEqualTo( *newStyle, false))
			{
				//concat with previous child style
				styleLastChild->SetRange( startLastChild, endTemp);
			}
			else 
			{
				bool childInserted = false;
				if (inRecLevel <= 5 && styleLastChild && endLastChild == start)
				{
					VTextStyle *newStyle2 = new VTextStyle( newStyle);
					VTextStyle *styleCommon = styleLastChild->ExtractCommonStyle( newStyle2);
					if (styleCommon)
					{
						if (styleLastChild->IsUndefined())
						{
							//set last child styles to common styles, expand range & add new styles minus common prop as children
							*styleLastChild = *styleCommon;
							delete styleCommon;
							styleLastChild->SetRange( startLastChild, endTemp);
							VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
							newStyle2->SetRange( start, endTemp);
							stylesLastChild->AddChild( newStyles2);
							newStyles2->Release();
							endLastChild = endTemp;
							childInserted = true;
						}
						else
						{
							//insert new child with common styles & for children the last child & new styles minus common properties
							xbox_assert(stylesLastChild == GetNthChild( i-1));
							stylesLastChild->Retain();
							RemoveChildAt(i-1);
							i = i-1;
							xbox_assert(childStyles == GetNthChild( i));

							styleCommon->SetRange( startLastChild, endTemp);
							VTreeTextStyle *newStyles = new VTreeTextStyle( styleCommon);
							newStyles->AddChild( stylesLastChild);
							stylesLastChild->Release();
							if (!newStyle2->IsUndefined())
							{
								VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
								newStyle2->SetRange( start, endTemp);
								newStyles->AddChild( newStyles2);
								newStyles2->Release();
							}	
							else
								delete newStyle2;
							
							stylesLastChild = newStyles;
							InsertChild( newStyles, i);
							indexLastChild = i;
							endLastChild = endTemp;
							newStyles->Release();
							i++;
							xbox_assert(childStyles == GetNthChild( i));

							childInserted = true;
						}
					}
					else
						delete newStyle2;
				}
				if (!childInserted)
				{
					//insert new child style
					VTreeTextStyle *newStyles = stylesLastChild = new VTreeTextStyle( styleLastChild = new VTextStyle( newStyle));
					newStyles->GetData()->SetRange( start, endTemp);
					InsertChild( newStyles, i);
					indexLastChild = i;
					startLastChild = start;
					endLastChild = endTemp;
					newStyles->Release();
					i++;
					xbox_assert(childStyles == GetNthChild( i));
				}
			}
		}
		if (childStart >= inEnd)
		{
			start = inEnd;
			break;
		}

		childStyles->_ApplyStyleRec( inStyle, inRecLevel+1, styleInheritForChilds);

		if (childStyles->GetChildCount() == 0 && childStyles->IsUndefined(true))
			RemoveChildAt(i);
		else if (styleLastChild && endLastChild == childStart && childStyles->GetChildCount() == 0 && styleLastChild->IsEqualTo( *(childStyles->GetData()), false))
		{
			//concat with previous child style
			styleLastChild->SetRange( startLastChild, childEnd);
			RemoveChildAt(i);
		}
		else
		{
			bool childInserted = false;
			if (inRecLevel <= 5 && styleLastChild && endLastChild == childStart)
			{
				VTextStyle *styleCommon = styleLastChild->ExtractCommonStyle( childStyles->GetData());
				if (styleCommon)
				{
					if (styleLastChild->IsUndefined())
					{
						//set last child styles to common styles, expand range & add current child styles minus common prop as children for last child styles
						*styleLastChild = *styleCommon;
						delete styleCommon;
						styleLastChild->SetRange( startLastChild, childEnd);
			
						childStyles->Retain();
						RemoveChildAt(i);
						stylesLastChild->AddChild( childStyles);
						childStyles->Release();
						endLastChild = childEnd;
						childInserted = true;
					}
					else
					{
						//insert new child with common styles & for children the last child & currend child styles minus common properties
						xbox_assert(stylesLastChild == GetNthChild( i-1));
						stylesLastChild->Retain();
						RemoveChildAt(i-1);
						i = i-1;
						xbox_assert(childStyles == GetNthChild( i));
						childStyles->Retain();
						RemoveChildAt(i);

						styleCommon->SetRange( startLastChild, childEnd);
						VTreeTextStyle *newStyles = new VTreeTextStyle( styleCommon);
						newStyles->AddChild( stylesLastChild);
						stylesLastChild->Release();
						newStyles->AddChild( childStyles);
						childStyles->Release();

						stylesLastChild = newStyles;
						InsertChild( newStyles, i);
						indexLastChild = i;
						endLastChild = childEnd;
						newStyles->Release();
						i++;
						childInserted = true;
					}
				}
			}
			if (!childInserted)
			{
				stylesLastChild = childStyles; 
				styleLastChild = childStyles->GetData();
				indexLastChild = i;
				startLastChild = childStart;
				endLastChild = childEnd;
				i++;
			}
		}

		start = std::max(start,childEnd);
		if (start >= inEnd)
			break;
	}

	if (applyOnCurrent && start < inEnd)
	{
		if (styleLastChild && endLastChild == start && styleLastChild->IsEqualTo( *newStyle, false))
		{
			//concat with previous child style
			styleLastChild->SetRange( startLastChild, inEnd);
		}
		else
		{
			bool childInserted = false;
			if (inRecLevel <= 5 && styleLastChild && endLastChild == start)
			{
				VTextStyle *newStyle2 = new VTextStyle( newStyle);
				VTextStyle *styleCommon = styleLastChild->ExtractCommonStyle( newStyle2);
				if (styleCommon)
				{
					if (styleLastChild->IsUndefined())
					{
						//set last child styles to common styles, expand range & add new styles minus common prop as children
						*styleLastChild = *styleCommon;
						delete styleCommon;
						styleLastChild->SetRange( startLastChild, inEnd);
						VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
						newStyle2->SetRange( start, inEnd);
						stylesLastChild->AddChild( newStyles2);
						newStyles2->Release();
						childInserted = true;
					}
					else
					{
						//insert new child with common styles & for children the last child & new styles minus common properties
						xbox_assert(stylesLastChild == GetNthChild( i-1));
						stylesLastChild->Retain();
						RemoveChildAt(i-1);

						styleCommon->SetRange( startLastChild, inEnd);
						VTreeTextStyle *newStyles = new VTreeTextStyle( styleCommon);
						newStyles->AddChild( stylesLastChild);
						stylesLastChild->Release();
						if (!newStyle2->IsUndefined())
						{
							VTreeTextStyle *newStyles2 = new VTreeTextStyle( newStyle2);
							newStyle2->SetRange( start, inEnd);
							newStyles->AddChild( newStyles2);
							newStyles2->Release();
						}	
						else
							delete newStyle2;
						
						AddChild( newStyles);
						newStyles->Release();
						childInserted = true;
					}
				}
				else
					delete newStyle2;
			}
			if (!childInserted)
			{
				//insert new child style
				VTreeTextStyle *newStyles = new VTreeTextStyle( new VTextStyle( newStyle));
				newStyles->GetData()->SetRange( start, inEnd);
				AddChild( newStyles);
				newStyles->Release();
			}
		}
	}
	if (GetChildCount() == 1)
	{
		VTreeTextStyle *childStyles = GetNthChild(1);
		sLONG end;
		childStyles->GetData()->GetRange( start, end);
		if (start <= curStart && end >= curEnd)
		{
			//merge cur style whith child node style & branch child node children to current node
			childStyles->Retain();
			RemoveChildAt(1);
			GetData()->MergeWithStyle( childStyles->GetData(), inStyleInherit);
			sLONG childCount = childStyles->GetChildCount();
			for (sLONG j = 1; j <= childCount; j++)
			{
				VTreeTextStyle *child = childStyles->GetNthChild(1);
				child->Retain();
				childStyles->RemoveChildAt(1);
				AddChild( child);
				child->Release();
			}
			childStyles->Release();
		}
	}

	if (styleInheritForChilds)
		delete styleInheritForChilds;
	delete newStyle;

	//in order to prevent stack overflow, 
	//we convert cascading styles to sequential styles if recursivity level is too high
	if (inRecLevel > 5 && GetChildCount())
		BuildSequentialStylesFromCascadingStyles( NULL);
}


/** merge current style recursively with the specified style 
@remarks
	input style properties which are defined override current style properties without taking account input style range
	(current style range is preserved)
*/
void VTreeTextStyle::_MergeWithStyle( const VTextStyle *inStyle, VTextStyle *inStyleInherit)
{
	VTextStyle *styleInherit = NULL;

	GetData()->MergeWithStyle( inStyle, inStyleInherit ? inStyleInherit : (GetParent() ? GetParent()->GetData() : NULL));

	if (GetChildCount() > 0)
	{
		if (inStyleInherit)
		{
			styleInherit = new VTextStyle( inStyleInherit);
			styleInherit->MergeWithStyle( GetData(), NULL);
		}
		else if (GetParent())
		{
			styleInherit = new VTextStyle(GetParent()->GetData());
			styleInherit->MergeWithStyle( GetData(), NULL);		
		}
	}

	for (int j = GetChildCount(); j >= 1; j--)
	{
		GetNthChild(j)->_MergeWithStyle( inStyle, styleInherit);
		
		if (GetNthChild(j)->GetChildCount() == 0 && GetNthChild(j)->IsUndefined(true))
			RemoveChildAt(j);
	}

	if (styleInherit)
		delete styleInherit;
}

/** return true if it is undefined */
bool VTreeTextStyle::IsUndefined(bool inTrueIfRangeEmpty) const
{
	if (!GetData())
		return true;

	if (inTrueIfRangeEmpty)
	{
		sLONG start = 0, end = 0;
		GetData()->GetRange( start, end);
		if (start >= end)
			return true;
	}

	if (!(GetData()->IsUndefined()))
		return false;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *style = GetNthChild(i);
		if (!style->IsUndefined( inTrueIfRangeEmpty))
			return false;
	}
	return true;
}



/** create new tree text style from the current but constrained to the specified range & eventually relative to the start of the specified range */ 
VTreeTextStyle *VTreeTextStyle::CreateTreeTextStyleOnRange( const sLONG inStart, const sLONG inEnd, bool inRelativeToStart, bool inNULLIfRangeEmpty)
{
	if (inNULLIfRangeEmpty && inStart == inEnd)
		return NULL;

	VTextStyle *style = GetData();
	if (style)
	{
		sLONG s,e;
		style->GetRange( s, e);
		if (s < inStart)
			s = inStart;
		if (s > inEnd)
			s = inEnd;
		if (e < inStart)
			e = inStart;
		if (e > inEnd)
			e = inEnd;
		if (inNULLIfRangeEmpty && e-s <= 0)
			return NULL;
		if (inRelativeToStart)
		{
			s -= inStart;
			e -= inStart;
		}
		VTextStyle *newStyle = new VTextStyle( style);
		newStyle->SetRange( s, e);

		VTreeTextStyle *styles = new VTreeTextStyle( newStyle);

		for (int i = 1; i <= GetChildCount(); i++)
		{
			VTreeTextStyle *childStyles = GetNthChild( i);
			VTreeTextStyle *newChildStyles = childStyles->CreateTreeTextStyleOnRange( inStart, inEnd, inRelativeToStart, inNULLIfRangeEmpty);
			if (newChildStyles)
			{
				sLONG childStart, childEnd;
				newChildStyles->GetData()->GetRange( childStart, childEnd);
				if (childStart == s && childEnd == e)
				{
					//optimization: merge child style with current style if child style range is equal to current style range
					styles->_MergeWithStyle( newChildStyles->GetData());
					for (int j = 1; j <= newChildStyles->GetChildCount(); j++)
					{
						VTreeTextStyle *stylesToAdd = newChildStyles->GetNthChild( j);
						styles->AddChild( stylesToAdd);
					}
					for (int j = newChildStyles->GetChildCount(); j >= 1; j--)
						newChildStyles->RemoveChildAt(j);
					xbox_assert(newChildStyles->GetChildCount() == 0);
					newChildStyles->Release();
				}
				else
				{
					styles->AddChild( newChildStyles);
					newChildStyles->Release();
				}
			}
		}
		return styles;
	}
	return NULL;
}


bool VTreeTextStyle::GetFontName(sLONG inStart, sLONG inEnd, VString& outFontName) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (!style->GetFontName().IsEmpty())
		outFontName = style->GetFontName();

	VString prevFontName = outFontName;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetFontName( inStart, inEnd, outFontName);
		if (!ok)
			return false;
		if (!outFontName.EqualToString( prevFontName, true))
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				outFontName = prevFontName;
				return false;
			}
			prevFontName = outFontName;
		}
	}
	return true;
}

bool VTreeTextStyle::GetFontSize(sLONG inStart, sLONG inEnd, Real& ioFontSize) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetFontSize() >= 0)
		ioFontSize = style->GetFontSize();

	Real prevFontSize = ioFontSize;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetFontSize( inStart, inEnd, ioFontSize);
		if (!ok)
			return false;
		if (ioFontSize != prevFontSize)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				ioFontSize = prevFontSize;
				return false;
			}
			prevFontSize = ioFontSize;
		}
	}
	return true;
}

/** get text color on the specified range 
@return
	true: text color is used on all range
	false: text color is mixed: return text color of the font used by range start character
*/
bool VTreeTextStyle::GetTextForeColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetHasForeColor())
		outColor = style->GetColor();

	RGBAColor prevColor = outColor;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetTextForeColor( inStart, inEnd, outColor);
		if (!ok)
			return false;
		if (outColor != prevColor)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				outColor = prevColor;
				return false;
			}
			prevColor = outColor;
		}
	}
	return true;
}

/** get back text color on the specified range 
@return
	true: back text color is used on all range
	false: back text color is mixed: return back text color of the font used by range start character
*/
bool VTreeTextStyle::GetTextBackColor(sLONG inStart, sLONG inEnd, RGBAColor& outColor) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (!style->GetTransparent())
		outColor = style->GetBackGroundColor();

	RGBAColor prevColor = outColor;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetTextBackColor( inStart, inEnd, outColor);
		if (!ok)
			return false;
		if (outColor != prevColor)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				outColor = prevColor;
				return false;
			}
			prevColor = outColor;
		}
	}
	return true;
}


bool VTreeTextStyle::GetBold( sLONG inStart, sLONG inEnd, sLONG& ioBold) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetBold() >= 0)
		ioBold = style->GetBold();

	sLONG prevBold = ioBold;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetBold( inStart, inEnd, ioBold);
		if (!ok)
			return false;
		if (ioBold != prevBold)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				ioBold = prevBold;
				return false;
			}
			prevBold = ioBold;
		}
	}
	return true;
}

bool VTreeTextStyle::GetItalic(sLONG inStart, sLONG inEnd, sLONG& ioItalic) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetItalic() >= 0)
		ioItalic = style->GetItalic();

	sLONG prevItalic = ioItalic;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetItalic( inStart, inEnd, ioItalic);
		if (!ok)
			return false;
		if (ioItalic != prevItalic)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				ioItalic = prevItalic;
				return false;
			}
			prevItalic = ioItalic;
		}
	}
	return true;
}

bool VTreeTextStyle::GetUnderline(sLONG inStart, sLONG inEnd, sLONG& ioUnderline) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetUnderline() >= 0)
		ioUnderline = style->GetUnderline();

	sLONG prevUnderline = ioUnderline;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetUnderline( inStart, inEnd, ioUnderline);
		if (!ok)
			return false;
		if (ioUnderline != prevUnderline)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				ioUnderline = prevUnderline;
				return false;
			}
			prevUnderline = ioUnderline;
		}
	}
	return true;
}


bool VTreeTextStyle::GetStrikeout(sLONG inStart, sLONG inEnd, sLONG& ioStrikeout) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetStrikeout() >= 0)
		ioStrikeout = style->GetStrikeout();

	sLONG prevStrikeout = ioStrikeout;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetStrikeout( inStart, inEnd, ioStrikeout);
		if (!ok)
			return false;
		if (ioStrikeout != prevStrikeout)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				ioStrikeout = prevStrikeout;
				return false;
			}
			prevStrikeout = ioStrikeout;
		}
	}
	return true;
}


bool VTreeTextStyle::GetJustification(sLONG inStart, sLONG inEnd, justificationStyle& ioJustification) const
{
	VTextStyle *style = GetData();
	if (!style)
		return true;
	sLONG start, end;
	style->GetRange( start, end);

	if (inStart > inEnd)
		return true;

	if (start < inStart)
		start = inStart;
	if (end > inEnd)
		end = inEnd;

	if (start > end)
		return true;

	if (style->GetJustification() >= 0)
		ioJustification = style->GetJustification();

	justificationStyle prevJustification = ioJustification;
	for (int i = 1; i <= GetChildCount(); i++)
	{
		VTreeTextStyle *childStyles = GetNthChild( i);
		sLONG childStart, childEnd;
		childStyles->GetData()->GetRange( childStart, childEnd);
		if (childStart >= inEnd)
			break;
		if (childEnd <= inStart || childStart >= childEnd)
			continue;

		bool ok = childStyles->GetJustification( inStart, inEnd, ioJustification);
		if (!ok)
			return false;
		if (ioJustification != prevJustification)
		{
			if (childStart > inStart || childEnd < inEnd)
			{
				ioJustification = prevJustification;
				return false;
			}
			prevJustification = ioJustification;
		}
	}
	return true;
}
