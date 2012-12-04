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
#include "VGraphicsPrecompiled.h"
#include "VPolygon.h"


bool operator == (const CGPoint a, const CGPoint b);
bool operator == (const CGPoint a, const CGPoint b) { return (a.x == b.x && a.y == b.y); };


VPolygon::VPolygon()
{	
	fTransform = CGAffineTransformIdentity;
}


VPolygon::VPolygon(const VPolygon& inOriginal)
{
	fTransform = inOriginal.fTransform;
	fPolygon = inOriginal.fPolygon;
	fBounds = inOriginal.fBounds;
}


VPolygon::VPolygon(const VArrayOf<VPoint*>& inList) 
{
	fTransform = CGAffineTransformIdentity;
	
	VIndex	nbPoint = inList.GetCount();
	fPolygon.AddNSpaces(nbPoint, false);
	
	for (sLONG index = 0; index < nbPoint; index++)
	{
		CGPoint	point = { inList[index]->GetX(), inList[index]->GetY() };
		fPolygon.SetNth(point, index + 1);
	}
	
	Close();
}

VPolygon::VPolygon (const VRect& inRect)
{
	FromVRect(inRect);
}
VPolygon::~VPolygon()
{
}

void VPolygon::FromVRect(const VRect& inRect)
{
	fTransform = CGAffineTransformIdentity;
	SetCoords(0,0,0,0);
	fPolygon.Destroy();
	AddPoint(VPoint(inRect.GetX(),inRect.GetY()));
	AddPoint(VPoint(inRect.GetX()+inRect.GetWidth(),inRect.GetY()));
	AddPoint(VPoint(inRect.GetX()+inRect.GetWidth(),inRect.GetY()+inRect.GetHeight()));
	AddPoint(VPoint(inRect.GetX(),inRect.GetY()+inRect.GetHeight()));

}

void VPolygon::AddPoint(const VPoint& inPoint)
{
	CGPoint	point = { inPoint.GetX(), inPoint.GetY() };
	fPolygon.AddTail(point);
	
	_ComputeBounds(inPoint);
}


void VPolygon::Close()
{
	if (fPolygon.GetCount() > 1)
		fPolygon.AddTail(fPolygon[0]);
}


void VPolygon::Clear()
{
	fPolygon.Destroy();
	fTransform = CGAffineTransformIdentity;
}


void VPolygon::SetPosBy(GReal inHoriz, GReal inVert)
{
	if (inHoriz == 0 && inVert == 0) return;
	
	fTransform = ::CGAffineTransformTranslate(fTransform, inHoriz, inVert);
	fBounds.SetPosBy(inHoriz, inVert);
}


void VPolygon::SetSizeBy(GReal inWidth, GReal inHeight)
{
	if (inWidth == 0 && inHeight == 0) return;
	
	fTransform = ::CGAffineTransformScale(fTransform, 1 + inWidth / fBounds.GetWidth(), 1 + inHeight / fBounds.GetHeight());
	fBounds.SetSizeBy(inWidth, inHeight);
}


Boolean VPolygon::HitTest(const VPoint& inPoint) const
{
	// Should test is point is inside the closed polygon
	//	or on any polygon segment if opened.
	assert(false);
	return fBounds.HitTest(inPoint);
}


void VPolygon::Inset(GReal inHoriz, GReal inVert)
{
	SetPosBy(inHoriz, inVert);
	SetSizeBy(-2 * inHoriz, -2 * inVert);
}


sLONG VPolygon::GetPointCount () const
{
	return fPolygon.GetCount();
}


void VPolygon::GetNthPoint(sLONG inIndex, VPoint& outPoint) const
{
	assert(inIndex > 0 && inIndex <= fPolygon.GetCount());
	
	if (inIndex <= 0 || inIndex > fPolygon.GetCount())
		outPoint.SetPosTo(kMIN_sWORD, kMIN_sWORD);
	else
		outPoint.SetPosTo(fPolygon[inIndex - 1].x, fPolygon[inIndex - 1].y);	// Could apply tranform matrix if required
}


VError VPolygon::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{	
	assert(ioStream);
	sLONG	sign = ioStream->GetLong();
		
	if (sign == 'POLY')
	{
		VPoint	point;
		sLONG	nbPoint = ioStream->GetWord();
		
		for (sLONG	index = 0; index < nbPoint; index++)
		{
			sLONG	x = ioStream->GetLong();
			sLONG	y = ioStream->GetLong();
			point.SetPos(x, y);
			
			AddPoint(point);
		}
	}
	
	return ioStream->GetLastError();
}


VError VPolygon::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	assert(ioStream);
	
	VPoint	point;
	sLONG	nbPoint = fPolygon.GetCount();
	
	ioStream->PutLong('POLY');
	ioStream->PutWord(nbPoint);
	
	for (sLONG index = 0; index < nbPoint; index++)
	{
		ioStream->PutLong((sLONG)(fPolygon[index].x));
		ioStream->PutLong((sLONG)(fPolygon[index].y));
	}
		
	return ioStream->GetLastError();
}


void VPolygon::_ComputeBounds(const VPoint& inLastPoint)
{
	if (fPolygon.GetCount() == 0)
	{
		fBounds.SetCoords(0, 0, 0, 0);
	}
	else if (fPolygon.GetCount() == 1)
	{
		fBounds.SetPosTo(inLastPoint);
		fBounds.SetSizeTo(0, 0);
	}
	else
	{
		if (inLastPoint.GetX() < fBounds.GetLeft()) 
			fBounds.SetLeft(inLastPoint.GetX());
		else if (inLastPoint.GetX() > fBounds.GetRight()) 
			fBounds.SetRight(inLastPoint.GetX());
			
		if (inLastPoint.GetY() < fBounds.GetTop()) 
			fBounds.SetTop(inLastPoint.GetY());
		else if (inLastPoint.GetY() > fBounds.GetBottom()) 
			fBounds.SetBottom(inLastPoint.GetY());
	}
}


VPolygon& VPolygon::operator = (const VPolygon& inOriginal)
{	
	fTransform = inOriginal.fTransform;
	fPolygon = inOriginal.fPolygon;
	fBounds = inOriginal.fBounds;
	
	return *this;
}
