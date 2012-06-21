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
#include "VValueBag.h"
#include "VProgressIndicator.h"
#include "VSystem.h"


#define	DEBUG_PROGRESS_SESSIONS		0

namespace VProgressIndicatorInfoBagKeys
{
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fUpdateFlags,VLong,uLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fMessage,VString,VString);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fValue,VLong8,sLONG8);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fMax,VLong8,sLONG8);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fStartTime,VLong,uLONG);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fCanInterrupt,VBoolean,bool);
	CREATE_BAGKEY_NO_DEFAULT_SCALAR( fExtraInfoBagVersion,VLong,uLONG);
};


VRemoteProgressSessionInfo::VRemoteProgressSessionInfo()
{
	fMasterSessionVersion=0;
	fSessionVersion=0;
	fUserAbort=0;
	fCanInterrupt=0;
	fFlags=0;
	fSessionCount=0;
	fChangesVersion=0;
}
VRemoteProgressSessionInfo::VRemoteProgressSessionInfo(const VRemoteProgressSessionInfo* inFrom)
{
	if(!inFrom)
	{
		fMasterSessionVersion=0;
		fSessionVersion=0;
		fUserAbort=0;
		fCanInterrupt=0;
		fFlags=0;
		fSessionCount=0;
		fChangesVersion=0;
	}
	else
	{
		*this=*inFrom;
	}
}
void VRemoteProgressSessionInfo::Reset()
{
	fSessionVersion=0;
	fMasterSessionVersion=0;
	fUserAbort=0;
	fCanInterrupt=0;
	fFlags=0;
	fSessionCount=0;
	fChangesVersion=0;
	fSessionStack.clear();
}

VError	VRemoteProgressSessionInfo::LoadFromBag( const VValueBag& inBag)
{
	fSessionStack.clear();
	fCurrentSessionFlags=0;
	inBag.GetLong(L"fFlags",fFlags);
	
	if(fFlags & kSessionCount)
		inBag.GetLong(L"fSessionCount",fSessionCount);
	if(fFlags & kChangesVersion)
		inBag.GetLong("fChangesVersion",fChangesVersion);
	if(fFlags & kSessionVersion)
		inBag.GetLong(L"fSessionVersion",fSessionVersion);
	if(fFlags & kMasterSessionVersion)
		inBag.GetLong(L"fMasterSessionVersion",fMasterSessionVersion);
	if(fFlags & kUserAbort)
		inBag.GetBool(L"fUserAbort",fUserAbort);
	if(fFlags & kCanInterrupt)
		inBag.GetBool(L"fCanInterrupt",fCanInterrupt);

	if(fFlags & kUpdateCurrent)
	{
		const VValueBag* current=inBag.RetainUniqueElement(L"fCurrent");
		if(current)
		{
			VProgressIndicatorInfo info;
			info.LoadFromBag(*current);
			fSessionStack.push_back(info);
			VProgressIndicatorInfoBagKeys::fUpdateFlags.Get(current,fCurrentSessionFlags);
			current->Release();
		}
	}
	else if(fFlags & kUpdateSession)
	{
		const XBOX::VBagArray* session_stack_bag=inBag.RetainElements( L"fSessionStack");
		if(session_stack_bag)
		{
			for(sLONG i=1;i<=session_stack_bag->GetCount();i++)
			{
				const XBOX::VValueBag* session=session_stack_bag->RetainNth(i);
				if(session)
				{
					VProgressIndicatorInfo info;
					info.LoadFromBag(*session);
					info.SetRemote(true);
					fSessionStack.push_back(info);
					session->Release();
				}
			}
			session_stack_bag->Release();
		}
	}
	
	return VE_OK;
}
VError	VRemoteProgressSessionInfo::SaveToBag( VValueBag& ioBag, VString& outKind,const VRemoteProgressSessionInfo* inPrevious) const
{
	VString tmpbagkind;
	VBagArray *bagarr;
	VValueBag *tmpbag;
	
	
	ioBag.SetLong(L"fFlags",fFlags);

	if(fFlags & kSessionCount)
		ioBag.SetLong(L"fSessionCount",fSessionCount);
	if(fFlags & kChangesVersion)
		ioBag.SetLong(L"fChangesVersion",fChangesVersion);
	if(fFlags & kSessionVersion)
		ioBag.SetLong(L"fSessionVersion",fSessionVersion);
	if(fFlags & kMasterSessionVersion)
		ioBag.SetLong(L"fMasterSessionVersion",fMasterSessionVersion);
	if(fFlags & kUserAbort)
		ioBag.SetBool(L"fUserAbort",fUserAbort);
	if(fFlags & kCanInterrupt)
		ioBag.SetBool(L"fCanInterrupt",fCanInterrupt);	
		
	if(fFlags & kUpdateCurrent)
	{
		const VProgressIndicatorInfo* last=NULL;
		
		if(inPrevious && inPrevious->fSessionCount==fSessionCount)
			last=inPrevious->GetCurrent();

		VValueBag* current=new VValueBag();
		
		fSessionStack.back().SaveToBag(*current,tmpbagkind,last);
		
		ioBag.AddElement(L"fCurrent",current);
		
		current->Release();
	}
	else if(fFlags & kUpdateSession)
	{
		bagarr=new VBagArray();
		for(_ProgressSessionInfoStack_Const_Iterrator it=fSessionStack.begin();it!=fSessionStack.end();it++)
		{
			tmpbag=new VValueBag();
			(*it).SaveToBag(*tmpbag,tmpbagkind);
			bagarr->AddTail(tmpbag);
			tmpbag->Release();
		}
		ioBag.SetElements(L"fSessionStack",bagarr);
		bagarr->Release();
	}
	outKind = L"DistantSessionInfo";
	return VE_OK;
}

VRemoteProgressSessionInfo&	VRemoteProgressSessionInfo::operator = ( const VRemoteProgressSessionInfo& inOther)
{
	fMasterSessionVersion=inOther.fMasterSessionVersion;
	fSessionVersion=inOther.fSessionVersion;
	fChangesVersion=inOther.fChangesVersion;
	fUserAbort=inOther.fUserAbort;
	fCanInterrupt=inOther.fCanInterrupt;
	fFlags=inOther.fFlags;
	fSessionCount=inOther.fSessionCount;
	
	fSessionStack=inOther.fSessionStack;
	return *this;
}
inline void	VRemoteProgressSessionInfo::SetChanged()
{
	fChangesVersion++;
}
inline const VProgressIndicatorInfo* VRemoteProgressSessionInfo::GetCurrent() const
{
	if(fSessionCount>=0)
	{
		return &fSessionStack.back();
	}
	else
		return NULL;
} ;
inline bool VRemoteProgressSessionInfo::operator !=( const VRemoteProgressSessionInfo& inInfo) const
{
	return !((*this)==inInfo);
}
inline bool VRemoteProgressSessionInfo::operator ==( const VRemoteProgressSessionInfo& inInfo) const
{
	bool result=false;
	result = inInfo.fChangesVersion==fChangesVersion;
	return result;
}
inline void VRemoteProgressSessionInfo::FromProgressInfo(const VProgressIndicatorInfo* inCurrent,const _ProgressSessionInfoStack* inStack,sLONG nbSession,uLONG inChangesVersion, uLONG inSessionVersion, uLONG inMasterSessionVersion)
{
	fFlags|= (kSessionCount | kChangesVersion | kSessionVersion | kMasterSessionVersion);
	fSessionCount = nbSession;
	fSessionVersion = inSessionVersion;
	fMasterSessionVersion = inMasterSessionVersion;
	fChangesVersion = inChangesVersion;
	
	if(!inStack)
	{
		if(fSessionCount>0 && inCurrent)
		{
			fSessionStack.push_back(*inCurrent);
			fFlags|=kUpdateCurrent;
		}
	}
	else
	{
		fFlags|=kUpdateSession;
		fSessionStack=*inStack;
		fSessionStack.push_back(*inCurrent);
	}
}

VError	VProgressIndicatorInfo::LoadFromBag( const VValueBag& inBag)
{
	uLONG flags=VProgressIndicatorInfo::kAll;
	const VValueBag* extra;
	
	VProgressIndicatorInfoBagKeys::fUpdateFlags.Get(&inBag,flags);
	if(flags & VProgressIndicatorInfo::kAll)
	{
		VProgressIndicatorInfoBagKeys::fMessage.Get(&inBag,fMessage);
		VProgressIndicatorInfoBagKeys::fValue.Get(&inBag,fValue);
		VProgressIndicatorInfoBagKeys::fMax.Get(&inBag,fMax);
		VProgressIndicatorInfoBagKeys::fStartTime.Get(&inBag,fStartTime);
		VProgressIndicatorInfoBagKeys::fCanInterrupt.Get(&inBag,fCanInterrupt);
		extra=inBag.RetainUniqueElement( L"Extra");
		VProgressIndicatorInfoBagKeys::fExtraInfoBagVersion.Get(&inBag,fExtraInfoBagVersion);
		XBOX::ReleaseRefCountable(&fExtraInfoBag);
		if(extra) 
		{
			fExtraInfoBag=extra->Clone();
			extra->Release();
		}
	}
	else
	{
		if(flags & VProgressIndicatorInfo::kMessage)
			VProgressIndicatorInfoBagKeys::fMessage.Get(&inBag,fMessage);
		
		if(flags & VProgressIndicatorInfo::kValue)
			VProgressIndicatorInfoBagKeys::fValue.Get(&inBag,fValue);
		
		if(flags & VProgressIndicatorInfo::kMax)
			VProgressIndicatorInfoBagKeys::fMax.Get(&inBag,fMax);
		
		if(flags & VProgressIndicatorInfo::kCanInterrupt)
			VProgressIndicatorInfoBagKeys::fCanInterrupt.Get(&inBag,fCanInterrupt);

		if(flags & VProgressIndicatorInfo::kStartTime)
			VProgressIndicatorInfoBagKeys::fStartTime.Get(&inBag,fStartTime);
		
		if(flags & VProgressIndicatorInfo::kExtraInfoBagVersion)
			VProgressIndicatorInfoBagKeys::fExtraInfoBagVersion.Get(&inBag,fExtraInfoBagVersion);
		
		if(flags & VProgressIndicatorInfo::kExtraInfoBag)
		{
			ReleaseRefCountable(&fExtraInfoBag);
			
			extra=inBag.RetainUniqueElement( L"Extra");
			fExtraInfoBag=extra->Clone();
			extra->Release();
		}
	}
	return VE_OK;
}
VError	VProgressIndicatorInfo::SaveToBag( VValueBag& ioBag, VString& outKind,const VProgressIndicatorInfo* inPrevious) const
{
	uLONG flags=0;
	if(!inPrevious)
	{
		flags=VProgressIndicatorInfo::kAll;
		VProgressIndicatorInfoBagKeys::fUpdateFlags.Set(&ioBag,flags);
		VProgressIndicatorInfoBagKeys::fMessage.Set(&ioBag,fMessage);
		VProgressIndicatorInfoBagKeys::fValue.Set(&ioBag,fValue);
		VProgressIndicatorInfoBagKeys::fMax.Set(&ioBag,fMax);
		VProgressIndicatorInfoBagKeys::fStartTime.Set(&ioBag,fStartTime);
		VProgressIndicatorInfoBagKeys::fCanInterrupt.Set(&ioBag,fCanInterrupt);
		VProgressIndicatorInfoBagKeys::fExtraInfoBagVersion.Set(&ioBag,fExtraInfoBagVersion);
		if(fExtraInfoBag)
		{
			VValueBag* clone=fExtraInfoBag->Clone();
			ioBag.AddElement("ExtraInfo",clone);
			clone->Release();
		}
	}
	else
	{
		if(fMessage!=inPrevious->fMessage)
		{
			flags|=VProgressIndicatorInfo::kMessage;
			VProgressIndicatorInfoBagKeys::fMessage.Set(&ioBag,fMessage);
		}
		if(fValue!=inPrevious->fValue)
		{		
			flags|=VProgressIndicatorInfo::kValue;
			VProgressIndicatorInfoBagKeys::fValue.Set(&ioBag,fValue);
		}
		if(fMax!=inPrevious->fMax)
		{
			flags|=VProgressIndicatorInfo::kMax;
			VProgressIndicatorInfoBagKeys::fMax.Set(&ioBag,fMax);
		}
		if(fStartTime!=inPrevious->fStartTime)
		{
			flags|=VProgressIndicatorInfo::kStartTime;
			VProgressIndicatorInfoBagKeys::fStartTime.Set(&ioBag,fStartTime);
		}
		if(fCanInterrupt!=inPrevious->fCanInterrupt)
		{
			flags|=VProgressIndicatorInfo::kCanInterrupt;
			VProgressIndicatorInfoBagKeys::fCanInterrupt.Set(&ioBag,fCanInterrupt);
		}
		if(fExtraInfoBagVersion!=inPrevious->fExtraInfoBagVersion)
		{
			flags|= VProgressIndicatorInfo::kExtraInfoBagVersion ;
			VProgressIndicatorInfoBagKeys::fExtraInfoBagVersion.Set(&ioBag,fExtraInfoBagVersion);
		}
		if(fExtraInfoBag && fExtraInfoBagVersion!=inPrevious->fExtraInfoBagVersion)
		{
			flags|= VProgressIndicatorInfo::kExtraInfoBag ;
			VValueBag* clone=fExtraInfoBag->Clone();
			ioBag.AddElement("ExtraInfo",clone);
			clone->Release();
		}
		VProgressIndicatorInfoBagKeys::fUpdateFlags.Set(&ioBag,flags);
	}
	outKind = L"VProgressIndicatorInfo";
	return VE_OK;
}
void VProgressIndicatorInfo::UpdateFromRemoteInfo(const VProgressIndicatorInfo& inFrom,uLONG inFlags)
{
	if(inFlags & kAll)
	{
		operator=(inFrom);
	}
	else
	{
		if(inFlags & VProgressIndicatorInfo::kMessage)
			fMessage=inFrom.fMessage;
		if(inFlags & VProgressIndicatorInfo::kValue)
			fValue=inFrom.fValue;
		if(inFlags & VProgressIndicatorInfo::kMax)
			fMax=inFrom.fMax;
		if(inFlags & VProgressIndicatorInfo::kCanInterrupt)
			fCanInterrupt=inFrom.fCanInterrupt;
		if(inFlags & VProgressIndicatorInfo::kExtraInfoBagVersion)
			fExtraInfoBagVersion=inFrom.fExtraInfoBagVersion;
		if(inFlags & VProgressIndicatorInfo::kExtraInfoBag)
		{
			ReleaseRefCountable(&fExtraInfoBag);
			if(inFrom.fExtraInfoBag)
				fExtraInfoBag=inFrom.fExtraInfoBag->Clone();
		}
	}
}
VProgressIndicatorInfo::VProgressIndicatorInfo(const VProgressIndicatorInfo &inInfo)
{
	fIsRemote=inInfo.fIsRemote;
	fMessage=inInfo.fMessage;
	fValue=inInfo.fValue;
	fMax=inInfo.fMax;
	fStartTime=inInfo.fStartTime;
	fCanInterrupt=inInfo.fCanInterrupt;
	fExtraInfoBagVersion= inInfo.fExtraInfoBagVersion;
	if(inInfo.fExtraInfoBag)
		fExtraInfoBag = inInfo.fExtraInfoBag->Clone();
	else
		fExtraInfoBag=NULL;
}

VProgressIndicatorInfo::~VProgressIndicatorInfo()
{
	ReleaseRefCountable(&fExtraInfoBag);
}

void VProgressIndicatorInfo::Reset()										
{
	fMax=0;
	fValue=0;
	fMessage.SetEmpty();
	fStartTime=0;
	ReleaseRefCountable(&fExtraInfoBag);
	fExtraInfoBagVersion++;
}

void VProgressIndicatorInfo::SetAttribute(const VString& inAttrName,const VValueSingle& inVal)
{
	if(!fExtraInfoBag)
		fExtraInfoBag=new VValueBag();
	
	if(!fExtraInfoBag->SetExistingAttribute(inAttrName,inVal))
		fExtraInfoBag->SetAttribute(inAttrName,inVal);
	fExtraInfoBagVersion++;
}

void VProgressIndicatorInfo::RemoveAttribute(const VString& inAttrName)
{
	if(fExtraInfoBag)
	{
		fExtraInfoBag->RemoveAttribute(inAttrName);
		fExtraInfoBagVersion++;
	}
}
void VProgressIndicatorInfo::GetMessage(VString& outMess) const
{
	VValueBag parameterbag;
	VString	tmpstr;	
	VTime t;
	VDuration dur;
	
	parameterbag.SetLong8("maxValue",fMax);
	parameterbag.SetLong8("curValue",fValue);
	parameterbag.SetLong8("rateValue",(sLONG8)( Max((Real) fValue / (Real) fMax * 100.0, 0.0)));
	
	t.FromMilliseconds(fStartTime);
	t.GetString(tmpstr);
	parameterbag.SetTime("startTime",t);
	
	VTime now( eInitWithCurrentTime);
	now.Substract (t, dur);
	parameterbag.SetDuration("duration",dur);
	
	outMess=fMessage;
	outMess.Format(&parameterbag);
	
}

VProgressIndicatorInfo& VProgressIndicatorInfo::operator = ( const VProgressIndicatorInfo* inOther)
{
	if(inOther)
	{
		fValue=inOther->fValue;
		fMax=inOther->fMax;
		fStartTime=inOther->fStartTime;
		fMessage=inOther->fMessage;
		fCanInterrupt = inOther->fCanInterrupt;
		ReleaseRefCountable(&fExtraInfoBag);
		fExtraInfoBagVersion=inOther->fExtraInfoBagVersion;
		if(inOther->fExtraInfoBag)
			fExtraInfoBag = inOther->fExtraInfoBag->Clone();
	}
	else
	{
		fValue=0;
		fMax=0;
		fStartTime=0;
		fMessage.SetEmpty();
		fCanInterrupt = true;
		fExtraInfoBagVersion=0;
		ReleaseRefCountable(&fExtraInfoBag);
	}
	return *this;
};

VProgressIndicatorInfo& VProgressIndicatorInfo::operator = ( const VProgressIndicatorInfo& inOther)
{
	fValue=inOther.fValue;
	fMax=inOther.fMax;
	fStartTime=inOther.fStartTime;
	fMessage=inOther.fMessage;
	fCanInterrupt = inOther.fCanInterrupt;
	ReleaseRefCountable(&fExtraInfoBag);
	fExtraInfoBagVersion=inOther.fExtraInfoBagVersion;
	if(inOther.fExtraInfoBag)
		fExtraInfoBag = inOther.fExtraInfoBag->Clone();
	
	return *this;
}

const VValueBag*	VProgressIndicatorInfo::RetainExtraInfoBag()
{
	if(fExtraInfoBag)
		fExtraInfoBag->Retain();
	return fExtraInfoBag;
}

/************************************************************************/
// session info
/************************************************************************/

VProgressSessionInfo&		VProgressSessionInfo::operator = ( const VProgressSessionInfo& inOther)
{
	fSessionCount=inOther.fSessionCount;
	fInfoCount=inOther.fInfoCount;
	fPercentDone=inOther.fPercentDone;
	fChangesVersion=inOther.fChangesVersion;
	fProgressArray[0]=inOther.fProgressArray[0];
	fProgressArray[1]=inOther.fProgressArray[1];
	fProgressArray[2]=inOther.fProgressArray[2];
	return *this;
}

VProgressIndicator::VProgressIndicator()
:fUUID(true)
{
	_Init();
	VProgressManager::RegisterProgress(this);
}

VProgressIndicator::VProgressIndicator(const VUUID& inUUID,sLONG inSessionVersion)
:fUUID(inUUID)
{
	_Init();
	fSessionVersion = inSessionVersion;
	VProgressManager::RegisterProgress(this);
}

VProgressIndicator::~VProgressIndicator()
{
	VProgressManager::UnregisterProgress(this);
}

void VProgressIndicator::_Init()
{
	fRemoteSessionOpen=0;
	fSaveSessionCount=0;

	fSessionCount=0;
	fCanInterrupt = true;
	fCanRevert = true;
	fSessionVersion=0;
	fGUIEnable=true;
	fUserAbort=0;
	fChangesVersion=0;
	
}

void VProgressIndicator::WriteInfoForRemote(VStream& inStream)
{
	inStream.PutWord(1);
	inStream.Put(fUUID);
	inStream.Put(fSessionVersion);
}
void VProgressIndicator::WriteInfoForRemote(VValueBag& inBag,VString& outKind)
{
	inBag.SetVUUID(L"uuid",fUUID);
	inBag.SetLong(L"version",fSessionVersion);
	outKind=L"VProgressIndicator";
}
void VProgressIndicator::Lock()const 
{
	fCrit.Lock();
}
void VProgressIndicator::Unlock()const
{
	fCrit.Unlock();
}

sLONG VProgressIndicator::GetSessionCount()const 
{
	return fSessionCount;
}
bool VProgressIndicator::Increment(sLONG8 inInc)
{
	bool	result = true;
	
	assert(fSessionCount);
	if (fSessionCount)
	{	
		fChangesVersion++;
		fCurrent.Increment(inInc);
		result = DoProgress();
		if (fCanInterrupt && (VTask::GetCurrent()->GetState() >= TS_DYING))
			result = false;
	}
	return result;
	
}
bool VProgressIndicator::Decrement(sLONG8 inDec)
{
	bool	result = true;
	
	assert(fSessionCount);
	if (fSessionCount)
	{
		fChangesVersion++;
		fCurrent.Decrement(inDec);	
		result = DoProgress();
		if (fCanInterrupt && (VTask::GetCurrent()->GetState() >= TS_DYING))
			result = false;
	}
	return result;
}

bool VProgressIndicator::Progress(sLONG8 inCurValue)
{
	bool	result = true;
	assert(fSessionCount);
	if (fSessionCount)
	{
		fChangesVersion++;
		fCurrent.SetValue(inCurValue);
		result = DoProgress();
		if (fCanInterrupt && (VTask::GetCurrent()->GetState() >= TS_DYING))
			result = false;
	}
	return result;
}


bool VProgressIndicator::DoProgress()
{
	VTask::Yield ( ); // Sergiy - 2009 July 31 - Bug fix ACI0062831. Progress called for long operations calls "Yield" to avoid blocking cooperative tasks.

	return !IsInterrupted();
}

sLONG8	VProgressIndicator::GetCurrentValue () const 
{ 
	return fCurrent.GetValue(); 
}
void VProgressIndicator::Reset(const VString& inMessage,sLONG8 inMaxValue,sLONG8 inValue)
{
	if(fSessionCount)
	{
		fCurrent.Set(inMessage,inValue,inMaxValue,true);
	}
}	
void VProgressIndicator::SetMaxValue (sLONG8 inMaxValue) 
{
	Lock();
	fChangesVersion++;
	fCurrent.SetMax(inMaxValue);
	Unlock();
}
sLONG8	VProgressIndicator::GetMaxValue () const 
{ 
	return fCurrent.GetMax(); 
}

bool VProgressIndicator::GetProgressInfo(VProgressIndicatorInfo& outInfo) const
{
	bool result=false;
	if(fCrit.TryToLock()) // pour eviter les dead lock,
	{
		if(fSessionCount)
		{
			outInfo=fCurrent;
		}
		result = fSessionCount>0;
		Unlock();
	}
	return result;
	
}

bool VProgressIndicator::GetProgressInfo(VProgressSessionInfo& outInfo)const
{
	bool result=false;
	if(fCrit.TryToLock()) // pour eviter les dead lock,
	{
		if(fSessionCount)
		{
			const VProgressIndicatorInfo *n0=NULL,*n1=NULL,*n2=NULL;
			
			if(fSessionCount==1)
			{
				n0=&fCurrent;
			}
			else if(fSessionCount==2)
			{
				n0=&fSessionStack[0];
				n1=&fCurrent;
			}
			else
			{
				n0=&fSessionStack[0];
				n1=&fSessionStack[fSessionCount-2];
				n2=&fCurrent;
			}
			outInfo.SetInfo(fChangesVersion,fSessionCount,ComputePercentDone(),n0,n1,n2);
		}
		result = fSessionCount>0;
		Unlock();
	}
	return result;
	
}

bool VProgressIndicator::GetProgressInfo(VRemoteProgressSessionInfo& outInfo,VRemoteProgressSessionInfo* inPrevious)const
{
	bool result=false;
	if(fCrit.TryToLock()) // pour eviter les dead lock,
	{
		outInfo.fFlags=0;
		if(!inPrevious)
		{
			outInfo.SetUserAbort(fUserAbort!=0);
			outInfo.SetCanInterrupt(fCanInterrupt);
			
			outInfo.FromProgressInfo(&fCurrent,&fSessionStack,fSessionCount,fChangesVersion,fSessionVersion,fRemoteSessionVersion);
			
			result=true;
		}
		else
		{
			if(inPrevious->GetChangesVersion()!=fChangesVersion)
			{
				const _ProgressSessionInfoStack* tmpstack=NULL;
				
				if(inPrevious->fUserAbort != (fUserAbort!=0))
					outInfo.SetUserAbort(fUserAbort!=0);
				
				if(inPrevious->fCanInterrupt!=fCanInterrupt)
					outInfo.SetCanInterrupt(fCanInterrupt);

				if(fSessionCount!=inPrevious->GetSessionCount())
				{
					// need to update session stack
					tmpstack=&fSessionStack;
				}
				else
				{
					if(fSessionCount>=1)
					{
						if(fCurrent.GetStamp() != inPrevious->GetCurrent()->GetStamp())
						{
							tmpstack=&fSessionStack;
						}
						else
						{
							// update current session;
						}
					}
				}
				outInfo.FromProgressInfo(&fCurrent,tmpstack,fSessionCount,fChangesVersion,fSessionVersion,fRemoteSessionVersion);
				result=true;
			}
			else
			{
				// no change
				result=false;
			}
		}
		Unlock();
	}
	return result;
	
}

void VProgressIndicator::SetProgressInfo(const VProgressIndicatorInfo& inInfo) 
{
	Lock();
	if(fSessionCount)
	{
		fChangesVersion++;
		fCurrent=inInfo;
	}
	Unlock();
}


VError VProgressIndicator::SaveInfoToBag(VValueBag& outBag)
{
	VError err = VE_OK;

	Lock();

	outBag.SetString(L"UserInfo", fUserInfo);
	outBag.SetLong("SessionCount", fSessionCount);
	outBag.SetString("Title", fTitle);
	outBag.SetBool("CanInterrupt", fCanInterrupt);

	for (_ProgressSessionInfoStack::iterator cur = fSessionStack.begin(), end = fSessionStack.end(); cur != end; cur++)
	{
		BagElement sessionbag(outBag, "SessionInfo");
		VString kind;
		(*cur).SaveToBag(*sessionbag, kind, NULL);
	}

	if (fSessionCount > 0)
	{
		BagElement sessionbag(outBag, "SessionInfo");
		VString kind;
		fCurrent.SaveToBag(*sessionbag, kind, NULL);
	}

	Unlock();

	return err;
}


void VProgressIndicator::UpdateFromRemoteInfo(const VRemoteProgressSessionInfo& inInfo)
{
	Lock();
	
	if(inInfo.fFlags & VRemoteProgressSessionInfo::kUserAbort && !fUserAbort)
		fUserAbort=inInfo.fUserAbort;
	if(inInfo.fFlags & VRemoteProgressSessionInfo::kCanInterrupt)
		fCanInterrupt=inInfo.fCanInterrupt;

	if(!fRemoteSessionOpen)
	{
		if(inInfo.GetSessionCount())
		{
			if(inInfo.fFlags & VRemoteProgressSessionInfo::kUpdateSession)
			{
				fRemoteSessionOpen=true;
				fSaveSessionCount=GetSessionCount();
				if(fSaveSessionCount)
				{
					// une session est deja ouverte, on ne touche pas au master stamp
					fRemoteSessionVersion = inInfo.fSessionVersion;
				}
				else
				{
					// pas de session ouverte en local, sync des master stamp sur la valeur distante
					fRemoteSessionVersion = inInfo.fSessionVersion;
					fSessionVersion = inInfo.fSessionVersion;
				}

				fChangesVersion=inInfo.fChangesVersion;
				
				for(_ProgressSessionInfoStack_Const_Iterrator it=inInfo.fSessionStack.begin();it!=inInfo.fSessionStack.end();it++)
				{
					if(fSessionCount>=1)
						fSessionStack.push_back(fCurrent);
					fCurrent = *it;
					fSessionCount++;
					DoBeginSession(fSessionCount);
				}
			}
		}
	}
	else
	{
		if(inInfo.GetSessionCount()==0)
		{
			// plus de session distante, cleanup
			_CloseRemoteSession();
		}
		else
		{
			if(fSaveSessionCount==0 || ( fSaveSessionCount>0  && inInfo.fMasterSessionVersion==fSessionVersion))
			{
				if(inInfo.fFlags & VRemoteProgressSessionInfo::kUpdateCurrent)
				{
					if(fChangesVersion!=inInfo.fChangesVersion)
					{
						fCurrent.UpdateFromRemoteInfo(inInfo.fSessionStack.back(),inInfo.fCurrentSessionFlags);
						fChangesVersion=inInfo.fChangesVersion;
					}
				}
				else if(inInfo.fFlags & VRemoteProgressSessionInfo::kUpdateSession)
				{
					if(fRemoteSessionVersion != inInfo.fSessionVersion)
					{
						_CleanRemoteSession();
					
						fSessionVersion = inInfo.fSessionVersion;
						fChangesVersion = inInfo.fChangesVersion;
						
						for(_ProgressSessionInfoStack_Const_Iterrator it=inInfo.fSessionStack.begin();it!=inInfo.fSessionStack.end();it++)
						{
							if(fSessionCount>=1)
								fSessionStack.push_back(fCurrent);
							fCurrent = *it;
							fSessionCount++;
							DoBeginSession(fSessionCount);
						}
					}
					else
					{
						if(fChangesVersion!=inInfo.fChangesVersion)
						{
							fCurrent=inInfo.fSessionStack.back();
							for(sLONG i=0;i<fSessionStack.size();i++)
							{
								fSessionStack[i]=inInfo.fSessionStack[i];
							}
							fChangesVersion=inInfo.fChangesVersion;
						}
					}
				}
			}
			else
			{
				// mauvais stamp, fermeture des session remote
				_CloseRemoteSession();
			}
		}
	}
	Unlock();
}

void VProgressIndicator::BeginUndeterminedSession ( const VString& inMessage)
{
	BeginSession(-1,inMessage,fCanInterrupt);
}
void VProgressIndicator::BeginUndeterminedSession ( const VString& inMessage,bool inCanInterrupt)
{
	BeginSession(-1,inMessage,inCanInterrupt);
}
void VProgressIndicator::BeginSession (sLONG8 inMaxValue, const VString& inMessage)
{
	BeginSession(inMaxValue,inMessage,fCanInterrupt);
}
void VProgressIndicator::BeginSession(sLONG8 inMaxValue, const VString& inMessage,bool inCanInterrupt)
{
	Lock();
	sLONG tmpsession;
	
	fChangesVersion++;
	if(fSessionCount==0)
		fSessionVersion++;
	
	if(fSessionCount>=1)
		fSessionStack.push_back(fCurrent);
	
	fCurrent.Set(inMessage,0,inMaxValue,inCanInterrupt);
	
	fSessionCount++;
	
	DoBeginSession(fSessionCount);
	Unlock();
}
void VProgressIndicator::_CleanRemoteSession()
{
	if (testAssert(fRemoteSessionOpen))
	{
		for(sLONG i= fSessionCount-fSaveSessionCount ;i>0;--i)
		{
			_EndSession();
		}
	}
}
void VProgressIndicator::_CloseRemoteSession()
{
	_CleanRemoteSession();
	fRemoteSessionOpen=false;
}
void VProgressIndicator::_EndSession()
{
	fChangesVersion++;

	if(!fSessionStack.empty())
	{
		fCurrent=fSessionStack.back();
		fSessionStack.pop_back();
	}
	
	DoEndSession(fSessionCount);

	fSessionCount--;

	if(fSessionCount==0)
		fSessionVersion++;
}
void VProgressIndicator::EndSession()
{
	Lock();
	if (testAssert(fSessionCount))
	{	
		if(fRemoteSessionOpen)
		{
			_CloseRemoteSession();
		}
		_EndSession();
	}
	Unlock();
}

void VProgressIndicator::SetCanInterrupt (bool inSet,bool inCurrentSessionOnly) 
{ 
	if(!inCurrentSessionOnly)
		fCanInterrupt = inSet; 
	else
	{
		Lock();
		if(testAssert(fSessionCount))
		{
			fChangesVersion++;
			fCurrent.SetCanInterrupt(inSet);
		}
		Unlock();
	}
}

void VProgressIndicator::SetMessageAndIncrementValue (const VString& inMessage,sLONG8 inIncrement)
{
	Lock();
	if(testAssert(fSessionCount))
	{
		fChangesVersion++;
		fCurrent.SetMessage(inMessage);
		fCurrent.Increment(inIncrement);
	}
	Unlock();
}

void VProgressIndicator::SetAttribute (const VString& inAttr,const VValueSingle& inVal)
{
	Lock();
	if(testAssert(fSessionCount))
	{
		fChangesVersion++;
		fCurrent.SetAttribute(inAttr,inVal);
	}
	Unlock();
}

void VProgressIndicator::RemoveAttribute (const VString& inAttr)
{
	Lock();
	if(testAssert(fSessionCount))
	{
		fChangesVersion++;
		fCurrent.RemoveAttribute(inAttr);
	}
	Unlock();
}

void VProgressIndicator::SetMessageAndValue (const VString& inMessage,sLONG8 inValue)
{
	Lock();
	if(testAssert(fSessionCount))
	{
		fChangesVersion++;
		fCurrent.SetMessage(inMessage);
		fCurrent.SetValue(inValue);
	}
	Unlock();
}
void VProgressIndicator::SetMessage(const VString& inMessage)
{
	Lock();
	if(testAssert(fSessionCount))
	{
		fChangesVersion++;
		fCurrent.SetMessage(inMessage);
	}
	Unlock();
}
void VProgressIndicator::GetMessage(VString& outMess) const
{
	if(testAssert(fSessionCount))
	{
		fCurrent.GetMessage(outMess);
	}
}

SmallReal VProgressIndicator::ComputePercentDone() const
{
	SmallReal	result = 0;
	Lock();
	result=_ComputePercentDone();
	Unlock();
	return result;
}
SmallReal VProgressIndicator::_ComputePercentDone() const
{
	SmallReal	result = 0;
	SmallReal	scale = 1;

	if(fSessionCount)
	{
		if(fCurrent.GetMax()<=0)
		{
			result=fCurrent.GetValue();
		}
		else
		{
			for(_ProgressSessionInfoStack_Const_Iterrator it=fSessionStack.begin();it!=fSessionStack.end();it++)
			{
				if((*it).GetMax()<=0)
				{
					return 0.0;
				}
				scale *= (SmallReal) (*it).GetMax();
				result += (*it).GetValue() / scale;
			}
			// traite le current
			scale *= (SmallReal) fCurrent.GetMax();
			result += fCurrent.GetValue() / scale;
		}
	}
	return result;
}
VProgressManager*	VProgressManager::sProgressManager=NULL;

VProgressManager::VProgressManager()
{
	fRemoteCreator = 0;
}

VProgressManager::~VProgressManager()
{
	fMap.clear();
}

bool VProgressManager::Init()
{
	if(!sProgressManager)
		sProgressManager = new VProgressManager();
	return true;
}
bool VProgressManager::Deinit()
{
	if(sProgressManager)
	{
		delete sProgressManager;
		sProgressManager=NULL;
	}
	return true;
}

void VProgressManager::RegisterProgress(VProgressIndicator* inProgress)
{
	if(sProgressManager)
		sProgressManager->_RegisterProgress(inProgress);
	else
		assert(false);
}
void VProgressManager::UnregisterProgress(VProgressIndicator* inProgress)
{
	if(sProgressManager)
		sProgressManager->_UnregisterProgress(inProgress);
}
VProgressIndicator* VProgressManager::RetainProgress(const VUUID& inUUID)
{
	if(sProgressManager)
		return sProgressManager->_RetainProgress(inUUID);
	else
		return NULL;
}
void VProgressManager::_RegisterProgress(VProgressIndicator* inProgress)
{
	VTaskLock lock(&fCrit);
	_VProgressIndicatorMap_Const_Iterrator it=fMap.find(inProgress->GetUUID());
	if(it==fMap.end())
	{
		fMap[inProgress->GetUUID()]=inProgress;
	}
}

void VProgressManager::_UnregisterProgress(VProgressIndicator* inProgress)
{
	VTaskLock lock(&fCrit);
	_VProgressIndicatorMap_Iterrator it=fMap.find(inProgress->GetUUID());
	if(it!=fMap.end())
	{
		fMap.erase(it);
	}
}

VProgressIndicator* VProgressManager::_RetainProgress(const VUUID& inUUID)
{
	VTaskLock lock(&fCrit);
	
	VProgressIndicator* result=NULL;

	_VProgressIndicatorMap_Const_Iterrator it=fMap.find(inUUID);
	if(it!=fMap.end())
	{
		result = it->second;
		result->Retain();
	}

	return result;
}


VError _ProgressInfoToBag(VValueBag& outBag, VProgressIndicator* prog)
{
	BagElement progressbag(outBag, "ProgressInfo");
	return prog->SaveInfoToBag(*progressbag);
}

VProgressIndicator* VProgressManager::_RetainProgressIndicator(const VString& userinfo)
{
	VProgressIndicator* result = NULL;
	VTaskLock lock(&fCrit);
	for (_VProgressIndicatorMap::iterator cur = fMap.begin(), end = fMap.end(); cur != end; cur++)
	{
		VProgressIndicator* p = cur->second;
		if (p->GetUserInfo() == userinfo)
		{
			result = RetainRefCountable(p);
			break;
		}
	}

	return result;
}


VError VProgressManager::_GetProgressInfo(VValueBag& outBag, const VString* userinfo) // if userinfo == nil then get all progress infos
{
	VError err = VE_OK;
	std::vector<VProgressIndicator*> allprogress;
	{
		VTaskLock lock(&fCrit);
		allprogress.reserve(fMap.size());
		for (_VProgressIndicatorMap::iterator cur = fMap.begin(), end = fMap.end(); cur != end; cur++)
		{
			allprogress.push_back(RetainRefCountable(cur->second));
		}
	}

	if (userinfo != NULL)
	{
		for (std::vector<VProgressIndicator*>::iterator cur = allprogress.begin(), end = allprogress.end(); cur != end; cur++)
		{
			if ((*cur)->GetUserInfo() == *userinfo)
				_ProgressInfoToBag(outBag, *cur);
		}
	}
	else
	{
		for (std::vector<VProgressIndicator*>::iterator cur = allprogress.begin(), end = allprogress.end(); cur != end; cur++)
		{
			_ProgressInfoToBag(outBag, *cur);
		}
	}

	for (std::vector<VProgressIndicator*>::iterator cur = allprogress.begin(), end = allprogress.end(); cur != end; cur++)
	{
		(*cur)->Release();
	}

	return err;
}

