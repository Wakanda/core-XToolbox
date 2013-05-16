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


VPolygon::VPolygon() 
{
#if !USE_GDIPLUS
	fPointCount = 0;
	fPolygon = VMemory::NewHandle(0);
#endif
}


VPolygon::VPolygon(const VPolygon& inOriginal)
{
#if !USE_GDIPLUS
	fPointCount = inOriginal.fPointCount;
	fPolygon = VMemory::NewHandleFromHandle(inOriginal.fPolygon);
#else
	fPolygon = inOriginal.fPolygon;
#endif

	fBounds = inOriginal.fBounds;
}


VPolygon::VPolygon(const VArrayOf<VPoint*>& inList) 
{
#if !USE_GDIPLUS
	fPointCount = 0;
	fPolygon = VMemory::NewHandle(0);
#endif
	
	for (sLONG index = 1; index <= inList.GetCount(); index++)
		AddPoint(*inList.GetNth(index));
	
	Close();
}

VPolygon::VPolygon (const VRect& inRect)
{
	#if !USE_GDIPLUS
	fPointCount = 0;
	fPolygon=0;
	#endif
	FromVRect(inRect);
}
void VPolygon::FromVRect(const VRect& inRect)
{
	#if !USE_GDIPLUS
	fPointCount = 0;
	if(fPolygon)
		VMemory::SetHandleSize(fPolygon,0);
	else
		fPolygon=VMemory::NewHandle(0);
	#else
	fPolygon.Destroy();
	#endif
	SetCoords(0,0,0,0);
	AddPoint(VPoint(inRect.GetX(),inRect.GetY()));
	AddPoint(VPoint(inRect.GetX()+inRect.GetWidth(),inRect.GetY()));
	AddPoint(VPoint(inRect.GetX()+inRect.GetWidth(),inRect.GetY()+inRect.GetHeight()));
	AddPoint(VPoint(inRect.GetX(),inRect.GetY()+inRect.GetHeight()));
}


VPolygon::~VPolygon()
{
#if !USE_GDIPLUS
	VMemory::DisposeHandle(fPolygon);
#endif
}


void VPolygon::AddPoint(const VPoint& inLocalPoint)
{
#if !USE_GDIPLUS	
	if (VMemory::SetHandleSize(fPolygon, 2 * (fPointCount + 1) * sizeof(sLONG)))
	{
		sLONG*	data = (sLONG*) WIN_LockPolygon();
	
		if (data != NULL)
		{
			data += fPointCount * 2;
			*data = inLocalPoint.GetX();
			
			data++;
			*data = inLocalPoint.GetY();
			
			WIN_UnlockPolygon();
		}
		
		fPointCount++;
	}
#else
	fPolygon.AddTail(Gdiplus::PointF(inLocalPoint.GetX(), inLocalPoint.GetY()));
#endif

	_ComputeBounds(inLocalPoint);
}


void VPolygon::Close()
{
	sLONG	count = GetPointCount();
	if (count > 1)
	{
		VPoint	firstPt;
		GetNthPoint(1, firstPt);
		
		VPoint	lastPt;
		GetNthPoint(count, lastPt);
		
		if (firstPt != lastPt)
			AddPoint(firstPt);
	}
}


void VPolygon::Clear()
{
#if !USE_GDIPLUS
	// Handle will be resized at next AddPoint
	fPointCount = 0;
#else
	fPolygon.Destroy();
#endif
}


void VPolygon::SetPosBy (GReal inHoriz, GReal inVert)
{
	sLONG*	lMin;
	sLONG*	lMax;
	
#if !USE_GDIPLUS
	if (fPointCount > 0 && (inHoriz != 0 || inVert != 0))
	{
		lMin = (sLONG*) WIN_LockPolygon();
		assert(lMin);
		if(lMin)
		{
			lMax = lMin + (fPointCount * 2);
			
			while (lMin < lMax)
			{
				*lMin += inHoriz;
				lMin++;
				*lMin += inVert;
				lMin++;
			}
		}
		WIN_UnlockPolygon();
	}
#else
	if (fPolygon.GetCount() > 0 && (inHoriz != 0 || inVert != 0))
	{
		for (sLONG index = 0; index < fPointCount; index++)
		{
			fPolygon[index].X+=	inHoriz;
			fPolygon[index].Y+=	inVert;
		}
	}
#endif

	fBounds.SetPosBy(inHoriz, inVert);
}


void VPolygon::SetSizeBy (GReal inWidth, GReal inHeight)
{
	if (inWidth != 0 || inHeight != 0)
	{
		GReal	scaleX = (GReal)(inWidth + fBounds.GetWidth()) / (GReal)fBounds.GetWidth();
		GReal	scaleY = (GReal)(inHeight + fBounds.GetHeight()) / (GReal)fBounds.GetHeight();
#if !USE_GDIPLUS
		sLONG*	lMin = (sLONG*) WIN_LockPolygon();
		sLONG*	lMax = lMin + (fPointCount * 2);
		while (lMin < lMax)
		{
			*lMin = fBounds.GetLeft() + (*lMin - fBounds.GetLeft()) * scaleX;
			lMin++;
			*lMin = fBounds.GetTop() + (*lMin - fBounds.GetTop()) * scaleY;
			lMin++;
		}
		
		WIN_UnlockPolygon();
#else
		for (long index = 0; index < fPolygon.GetCount(); index++)
		{
			fPolygon[index].X =	fBounds.GetLeft() + (fPolygon[index].X - fBounds.GetLeft()) * scaleX;
			fPolygon[index].Y =	fBounds.GetTop() + (fPolygon[index].Y - fBounds.GetTop()) * scaleY;
		}
#endif

		fBounds.SetSizeTo(fBounds.GetWidth() * scaleX, fBounds.GetHeight() * scaleY);
	}
}


Boolean VPolygon::HitTest (const VPoint& inPoint) const
{
	// Should test is point is inside the closed polygon
	//	or on any polygon segment if opened.
	assert(false);
	return fBounds.HitTest(inPoint);
}


void VPolygon::Inset (GReal inHoriz, GReal inVert)
{
	SetPosBy(inHoriz, inVert);
	SetSizeBy(-2.0 * inHoriz, -2.0 * inVert);
}


sLONG VPolygon::GetPointCount () const
{
#if !USE_GDIPLUS
	return fPointCount;
#else
	return fPolygon.GetCount();
#endif
}


void VPolygon::GetNthPoint (sLONG inIndex, VPoint& outPoint) const
{
	assert(inIndex > 0 && inIndex <= GetPointCount());
	
#if !USE_GDIPLUS
	assert(fPolygon != NULL);
	
	if (fPolygon == NULL || inIndex <= 0 || inIndex > fPointCount)
	{
		outPoint.SetPosTo(kMIN_sWORD, kMIN_sWORD);
	}
	else
	{
		VPolygon*	constThis = (VPolygon*) this;
		
		sLONG*	ptr = (sLONG*) constThis->WIN_LockPolygon();
		ptr += (inIndex - 1) * 2;
		
		outPoint.SetPosTo(*ptr, *(ptr + 1));
		
		constThis->WIN_UnlockPolygon();
	}
#else
	outPoint.SetPosTo(fPolygon[inIndex - 1].X, fPolygon[inIndex - 1].Y);
#endif
}


VError VPolygon::ReadFromStream(VStream* ioStream, sLONG /*inParam*/)
{
	assert(ioStream != NULL);
	sLONG	sign = ioStream->GetLong();
		
	if (sign == 'POLY')
	{
	#if !USE_GDIPLUS
		sLONG	h, v;
		if (fPolygon != NULL)
			VMemory::DisposeHandle(fPolygon);
			
		fPolygon = VMemory::NewHandle(0);
		fPointCount = 0;
	#else
		GReal	h, v;
		fPolygon.Destroy();
	#endif


		sWORD	i;
		sWORD	nbpoint = ioStream->GetWord();
		
		for (i = 0; i < nbpoint; i++)
		{
	#if !USE_GDIPLUS
			h = ioStream->GetLong();
			v = ioStream->GetLong();
			AddPoint(VPoint(h, v));
	#else
			h = ioStream->GetReal();
			v = ioStream->GetReal();
			AddPoint(VPoint(h, v));
	#endif
		}
	}
	
	return ioStream->GetLastError();
}


VError VPolygon::WriteToStream(VStream* ioStream, sLONG /*inParam*/) const
{
	assert(ioStream);
#if !USE_GDIPLUS
	assert(fPolygon);
#endif
	
	ioStream->PutLong('POLY');
	ioStream->PutWord(GetPointCount());

#if !USE_GDIPLUS
	sWORD	i;
	VPolygon*	constPoly = (VPolygon*) this;
	sLONG*	data = (sLONG*) constPoly->WIN_LockPolygon();
	for (i = 0; i < fPointCount; i++)
	{
		ioStream->PutLong(*data);
		data++;
		ioStream->PutLong(*data);
		data++;
	}
	
	constPoly->WIN_UnlockPolygon();
#else
	for (sLONG index = 0; index < fPolygon.GetCount(); index++)
	{
		ioStream->PutReal(fPolygon[index].X);
		ioStream->PutReal(fPolygon[index].Y);
	}
#endif

	return ioStream->GetLastError();
}


void VPolygon::_ComputeBounds(const VPoint& inLastPoint)
{
	sLONG	count = GetPointCount();
	if (count == 0)
	{
		fBounds.SetCoords(0.0, 0.0, 0.0, 0.0);
	}
	else if (count == 1)
	{
		fBounds.SetPosTo(inLastPoint);
		fBounds.SetSizeTo(0.0, 0.0);
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
#if !USE_GDIPLUS
	VSize	size = VMemory::GetHandleSize(inOriginal.fPolygon);
	VMemory::SetHandleSize(fPolygon, size);
	
	CopyBlock(VMemory::LockHandle(inOriginal.fPolygon), VMemory::LockHandle(fPolygon), size);
	
	VMemory::UnlockHandle(inOriginal.fPolygon);
	VMemory::UnlockHandle(fPolygon);
	
	fPointCount = inOriginal.fPointCount;
#else
	fPolygon = inOriginal.fPolygon;
#endif

	fBounds = inOriginal.fBounds;
	
	return *this;
}


Boolean VPolygon::operator == (const VPolygon& inPolygon) const
{
	Boolean isEqual = true;

#if !USE_GDIPLUS
	assert(false);	// TBD
#else
	for (long index = 0; index < fPolygon.GetCount() && isEqual; index++)
	{
		isEqual &= fPolygon[index].X == inPolygon.fPolygon[index].X;
		isEqual &= fPolygon[index].Y == inPolygon.fPolygon[index].Y;
	}
#endif

	return isEqual;
}
