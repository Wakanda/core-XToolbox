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
#ifndef __VProgressIndicator__
#define __VProgressIndicator__


#include "Kernel/Sources/IRefCountable.h"
#include "Kernel/Sources/VArrayValue.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VString.h"
#include "Kernel/Sources/VSyncObject.h"
#include "Kernel/Sources/VTime.h"
#include "Kernel/Sources/VUUID.h"

BEGIN_TOOLBOX_NAMESPACE

class VValueBag;

class XTOOLBOX_API VReadWriteActivityIndicator : public VObject
{
	public:
	VReadWriteActivityIndicator(){;}
	virtual ~VReadWriteActivityIndicator(){;}
	
	virtual void SetActivityRead(bool State, sLONG Delay) = 0;  // if Delay == -1 then unlimited
	virtual void SetActivityWrite(bool State, sLONG Delay) = 0;  // if Delay == -1 then unlimited
	
};

const uLONG  kDelayToCallDoProgress = 500; // milliseconds




class XTOOLBOX_API VProgressIndicatorInfo : public VObject
{
	public:
	
	enum UpdateFlags
	{
		kMessage				=1,
		kValue					=2,
		kMax					=4,
		kStartTime				=8,
		kCanInterrupt			=16,
		kExtraInfoBagVersion	=32,
		kExtraInfoBag			=64,
		kAll					=1024
	};

	VError	SaveToBag( VValueBag& ioBag, VString& outKind,const VProgressIndicatorInfo* inPrevious=NULL) const;
	VError	LoadFromBag( const VValueBag& inBag);

	virtual ~VProgressIndicatorInfo();

	VProgressIndicatorInfo()
	{
		fExtraInfoBag=NULL;
		fIsRemote=false;
		Reset();
	}
	VProgressIndicatorInfo(const VString& inMessage,sLONG8 inValue,sLONG8 inMax,bool inCanInterrupt)
	{
		fIsRemote=false;
		fMessage=inMessage;
		fValue=inValue;
		fMax=inMax;
		fCanInterrupt=inCanInterrupt;
		fStartTime= XBOX::VSystem::GetCurrentTime();
		fExtraInfoBag=NULL;
	};
	VProgressIndicatorInfo(const VProgressIndicatorInfo &inInfo);
	
	VProgressIndicatorInfo&		operator = ( const VProgressIndicatorInfo* inOther);
	VProgressIndicatorInfo&		operator = ( const VProgressIndicatorInfo& inOther);
	bool						operator ==( const VProgressIndicatorInfo& inInfo) const;
	bool						operator !=( const VProgressIndicatorInfo& inInfo) const;
	
	void		GetMessage(VString &outMess)const;
	sLONG8		GetValue()const								{return fValue;}
	sLONG8		GetMax()const								{return fMax;}
	void		Reset();
	void		SetMax(sLONG8 inMax)						{fMax=inMax;}
	void		SetValue(sLONG8 inValue)					{fValue=inValue;}
	void		SetMessage(const VString& inMessage)		{fMessage=inMessage;}
	void		Increment(sLONG8 inInc=1)					{fValue+=inInc;}
	void		Decrement(sLONG8 inDec=1)					{fValue-=inDec;}
	void		GetStartTime(VTime& outTime)				const{return outTime.FromMilliseconds(fStartTime);}
	uLONG		GetStamp()									const{return fStartTime;} 
	bool		GetCanInterrupt()							const{return fCanInterrupt;}
	void		SetCanInterrupt(bool inCanInterrupt)		{fCanInterrupt=inCanInterrupt;}
	void		Set(const VString& inMessage,sLONG8 inValue,sLONG8 inMax,bool inCanIterrupt,bool inResetStamp=true);
	const VValueBag*	RetainExtraInfoBag();
	
	void		SetAttribute(const VString& inAttrName,const VValueSingle& inVal);
	void		RemoveAttribute(const VString& inAttrName);

	void		SetRemote(bool inSet)						{fIsRemote=inSet;}
	bool		IsRemote()									const{return fIsRemote;}

	void		UpdateFromRemoteInfo(const VProgressIndicatorInfo& inFrom,uLONG inFlags);

private:
	VString		fMessage;
	sLONG8		fValue;
	sLONG8		fMax;
	uLONG		fStartTime; // debut session et stamp
	bool		fCanInterrupt;
	bool		fIsRemote;
	VValueBag*	fExtraInfoBag;
	uLONG		fExtraInfoBagVersion;
};

inline bool	VProgressIndicatorInfo::operator !=( const VProgressIndicatorInfo& inInfo) const						
{ 
	return !((*this)==inInfo);
};
inline bool	VProgressIndicatorInfo::operator ==( const VProgressIndicatorInfo& inInfo) const						
{ 
	if(inInfo.fValue == fValue)
	{
		if(inInfo.fMax == fMax)
		{
			if(fStartTime==inInfo.fStartTime)
			{
				if(fCanInterrupt==inInfo.fCanInterrupt)
					return inInfo.fMessage==fMessage;
				else
					return false;
			}
			else
				return false;
		}
		else
			return false;
	}
	else
		return false;
};
inline void VProgressIndicatorInfo::Set(const VString& inMessage,sLONG8 inValue,sLONG8 inMax,bool inCanIterrupt,bool inResetStamp)
{
	fMessage=inMessage;
	fValue=inValue;
	fMax=inMax;
	fCanInterrupt=inCanIterrupt;
	if(inResetStamp)
	{
		fStartTime= XBOX::VSystem::GetCurrentTime();
	}
};

class XTOOLBOX_API VProgressSessionInfo : public VObject
{
	public:
	
	VProgressSessionInfo()
	{
		fSessionCount=0;
		fInfoCount=0;
		fPercentDone=0;
		fChangesVersion=0;
	}
	virtual ~VProgressSessionInfo()
	{
		
	}
	VProgressSessionInfo&		operator = ( const VProgressSessionInfo& inOther);

	void									SetInfo(sLONG inSessionVersion,sLONG inSessionCount,SmallReal inPercent,const VProgressIndicatorInfo* in0,const VProgressIndicatorInfo* in1=NULL,const VProgressIndicatorInfo* in2=NULL);
	bool									operator ==( const VProgressSessionInfo& inInfo) const;
	bool									operator !=( const VProgressSessionInfo& inInfo) const;
	VProgressIndicatorInfo*					GetCurrent();
	
	sLONG									GetSessionCount()const		{return fSessionCount;}
	sLONG									GetInfoCount()const			{return fInfoCount;}
	SmallReal								GetPercentDone()const		{return fPercentDone;}
	void									SetChanged();
	sLONG									GetChangesVersion()const		{return fChangesVersion;}
	
public:
	
	VProgressIndicatorInfo					fProgressArray[3];
	sLONG									fSessionCount;
	sLONG									fInfoCount;
	SmallReal								fPercentDone;
	
	uLONG									fChangesVersion;
};
inline void	VProgressSessionInfo::SetChanged()
{
	fChangesVersion++;
}
inline VProgressIndicatorInfo* VProgressSessionInfo::GetCurrent()
{
	if(fSessionCount==0)
		return NULL;
	if(fSessionCount==1)
		return &fProgressArray[0];
	else if(fSessionCount==2)
		return &fProgressArray[1];
	else 
		return &fProgressArray[2];
} ;
inline bool VProgressSessionInfo::operator !=( const VProgressSessionInfo& inInfo) const
{
	return !((*this)==inInfo);
}
inline bool VProgressSessionInfo::operator ==( const VProgressSessionInfo& inInfo) const
{
	return inInfo.fChangesVersion==fChangesVersion;
}
inline void VProgressSessionInfo::SetInfo(sLONG inChangesVersion,sLONG inSessionCount,SmallReal inPercent,const VProgressIndicatorInfo* in0,const VProgressIndicatorInfo* in1,const VProgressIndicatorInfo* in2)
{
	inSessionCount>=3 ? fInfoCount=3 : fInfoCount=inSessionCount;
	fSessionCount=inSessionCount;
	fPercentDone=inPercent;
	fProgressArray[0]=in0;
	fProgressArray[1]=in1;
	fProgressArray[2]=in2;
	fChangesVersion=inChangesVersion;
};

typedef std::deque < VProgressIndicatorInfo>			_ProgressSessionInfoStack ;
typedef _ProgressSessionInfoStack::const_iterator		_ProgressSessionInfoStack_Const_Iterrator ;
typedef _ProgressSessionInfoStack::iterator				_ProgressSessionInfoStack_Iterrator ;

class XTOOLBOX_API VRemoteProgressSessionInfo : public VObject
{
	public:
	
	enum UpdateFlags
	{
		kSessionCount			=1,
		kChangesVersion			=2,
		kSessionVersion			=4,
		kMasterSessionVersion	=8,
		kUserAbort				=16,
		kCanInterrupt			=32,
		kUpdateCurrent			=64,
		kUpdateSession			=128	
	};

	VError	SaveToBag( VValueBag& ioBag, VString& outKind,const VRemoteProgressSessionInfo* inPrevious=NULL) const;
	VError	LoadFromBag( const VValueBag& inBag);
		
	VRemoteProgressSessionInfo();
	VRemoteProgressSessionInfo(const VRemoteProgressSessionInfo* inFrom);
	virtual ~VRemoteProgressSessionInfo()
	{
		
	}
	VRemoteProgressSessionInfo&				operator = ( const VRemoteProgressSessionInfo& inOther);

	void									FromProgressInfo(const VProgressIndicatorInfo* inCurrent,const _ProgressSessionInfoStack* inStack,sLONG nbSession,uLONG inChangesVersion,uLONG inVersion,uLONG inMasterVersion);
	bool									operator ==( const VRemoteProgressSessionInfo& inInfo) const;
	bool									operator !=( const VRemoteProgressSessionInfo& inInfo) const;
	const VProgressIndicatorInfo*			GetCurrent() const;
	
	sLONG									GetSessionCount()const			{return fSessionCount;}
	void									SetChanged();
	sLONG									GetChangesVersion()const		{return fChangesVersion;}
	void									Reset();
	void									SetUserAbort(bool inUserAbort){fUserAbort=inUserAbort;fFlags|=kUserAbort;};
	void									SetCanInterrupt(bool inCanInterrupt){fCanInterrupt=inCanInterrupt;fFlags|=kCanInterrupt;};
public:
	sLONG											fSessionCount;
	_ProgressSessionInfoStack						fSessionStack;
	
	uLONG											fChangesVersion;
	uLONG											fMasterSessionVersion;
	uLONG											fSessionVersion;
	bool											fUserAbort;
	bool											fCanInterrupt;
	sLONG											fFlags;
	uLONG											fCurrentSessionFlags;
};


class XTOOLBOX_API VProgressIndicator : public VObject, public IRefCountable
{
public:
	
	VProgressIndicator ();
	VProgressIndicator(const VUUID& inUUID,sLONG inVersion);
	virtual	~VProgressIndicator ();

	void WriteInfoForRemote(VStream& inStream);
	void WriteInfoForRemote(VValueBag& inBag,VString& outKind);

	// Title support
	virtual void	SetTitle (const VString& inTitle) { fTitle.FromString(inTitle); };
	const VString&	GetTitle () const { return fTitle; };

	// Progress support
	
	virtual bool					Progress (sLONG8 inCurValue);
	virtual bool					Increment(sLONG8 inInc=1);
	virtual bool					Decrement(sLONG8 inDec=1);
	sLONG8							GetCurrentValue () const;

	virtual void					SetMaxValue (sLONG8 inMaxValue);
	sLONG8							GetMaxValue () const ;
	bool							IsEndKnown () const { return (GetMaxValue() != -1); };
	sLONG							GetSessionCount()const;
	// Session support
	void							BeginSession (sLONG8 inMaxValue, const VString& inMessage);
	virtual void					BeginSession (sLONG8 inMaxValue, const VString& inMessage,bool inCanInterrupt);
	
	void							BeginUndeterminedSession ( const VString& inMessage);
	void							BeginUndeterminedSession ( const VString& inMessage,bool inCanInterrupt);

	virtual void					EndSession ();
	virtual void					Reset(const VString& inMessage,sLONG8 inMaxValue,sLONG8 inValue=0);
	
	virtual void					GetMessage(VString &outMess)const;
	virtual void					SetMessage(const VString& inMessage);
	virtual void					SetMessageAndIncrementValue (const VString& inMessage,sLONG8 inIncrement=1);
	virtual void					SetMessageAndValue (const VString& inMessage,sLONG8 inValue);
	
	virtual void					SetAttribute (const VString& inAttr,const VValueSingle& inVal);
	virtual void					RemoveAttribute (const VString& inAttr);
	
	// Accessors
	virtual void					SetCanInterrupt (bool inSet,bool inCurrentSessionOnly=false);
	bool							GetCanInterrupt () const { return fCanInterrupt; };
	
	virtual void					SetInterruptWillRevert (bool inSet) { fCanRevert = inSet; };
	bool							GetInterruptWillRevert () const { return fCanRevert; };
	bool							GetProgressInfo(VProgressIndicatorInfo& outInfo)const;
	bool							GetProgressInfo(VProgressSessionInfo& outInfo)const;
	bool							GetProgressInfo(VRemoteProgressSessionInfo& outInfo,VRemoteProgressSessionInfo* inPrevious)const;

	void							SetProgressInfo(const VProgressIndicatorInfo& inInfo);
	SmallReal						ComputePercentDone () const;
	uLONG							GetSessionVersion();
	void							EnableGUI(bool inEnableState){fGUIEnable=inEnableState;}
	
	void							UserAbort();
	bool							IsInterrupted();
	const VUUID&					GetUUID()const{return fUUID;}

	void							UpdateFromRemoteInfo(const VRemoteProgressSessionInfo& inInfo);

	void							SetUserInfo(const VString& inUserInfo)
	{
		fUserInfo = inUserInfo;
	}

	const VString&					GetUserInfo() const
	{
		return fUserInfo;
	}

	virtual VError					SaveInfoToBag(VValueBag& outBag);

protected:

	VUUID											fUUID;

	void Lock()const;
	void Unlock()const;
	mutable VCriticalSection						fCrit;
	
	_ProgressSessionInfoStack						fSessionStack;
	mutable VProgressIndicatorInfo					fCurrent;
	sLONG											fSessionCount;

	sLONG											fRemoteSessionOpen;
	sLONG											fRemoteSessionVersion;
	sLONG											fSaveSessionCount;
	
	bool											fCanInterrupt;	// Value returned from Progress is handled by caller
	bool											fCanRevert;		// Interrupt will return to initial state
	bool											fGUIEnable;
	
	VString											fTitle;
	uLONG											fSessionVersion;			// unique id, change a chaque ouverture de la session principal	
	sLONG											fUserAbort;
	
	uLONG											fChangesVersion;

	VString											fUserInfo;

	virtual void	DoBeginSession(sLONG inSessionNumber){;}
	virtual void	DoEndSession(sLONG inSessionNumber){;}

	virtual bool	DoProgress ();	// Return false if you want to stop

	SmallReal		_ComputePercentDone () const;
			
private:
	void	_CloseRemoteSession();
	void	_CleanRemoteSession();
	void	_EndSession();
	void	_Init ();

};
inline void	VProgressIndicator::UserAbort()
{
	XBOX::VInterlocked::Exchange(&fUserAbort,(sLONG)1);
}
inline bool	VProgressIndicator::IsInterrupted()									
{
	return fUserAbort==1;
}

inline uLONG VProgressIndicator::GetSessionVersion()
{
	uLONG result;
	XBOX::VInterlocked::Exchange((sLONG*)&result,(sLONG)fSessionVersion);
	return result;
};

class XTOOLBOX_API IRemoteProgressCreator
{
	public:
		IRemoteProgressCreator(){};
		virtual ~IRemoteProgressCreator(){};

	virtual VProgressIndicator* CreateRemoteProgress(VStream& inStream, const VUUID& inClientUUID)=0;
	virtual VProgressIndicator* CreateRemoteProgress(const VUUID& inUUID,sLONG inVersionp, const VUUID& inClientUUID)=0;
	virtual VProgressIndicator* CreateRemoteProgress(const VValueBag& inValueBag,const VUUID& inClientUUID)=0;
};

class XTOOLBOX_API VProgressManager : public VObject
{
public:
	virtual ~VProgressManager();
	
private:
	VProgressManager();
	void _RegisterProgress(VProgressIndicator* inProgress);
	void _UnregisterProgress(VProgressIndicator* inProgress);
	VProgressIndicator* _RetainProgress(const VUUID& inUUID);
	VError _GetProgressInfo(VValueBag& outBag, const VString* userinfo = NULL);
	VProgressIndicator* _RetainProgressIndicator(const VString& userinfo);
	
	typedef std::map < const VUUID, VProgressIndicator* > _VProgressIndicatorMap ;
	typedef _VProgressIndicatorMap::iterator _VProgressIndicatorMap_Iterrator ;
	typedef _VProgressIndicatorMap::const_iterator _VProgressIndicatorMap_Const_Iterrator ;

	static VProgressManager*				sProgressManager;
	
	VCriticalSection						fCrit;
	_VProgressIndicatorMap					fMap;
	
	IRemoteProgressCreator*					fRemoteCreator;

public:

	static bool Init();
	static bool Deinit();
	
	static VProgressIndicator* CreateRemoteProgress(VStream& inStream, const VUUID& inClientUUID)
	{
		VProgressIndicator* result=NULL;
		if(sProgressManager)
		{
			if(sProgressManager->fRemoteCreator)
				result = sProgressManager->fRemoteCreator->CreateRemoteProgress(inStream,inClientUUID);
		}
		return  result;
	}
	static VProgressIndicator* CreateRemoteProgress(const VUUID& inUUID,sLONG inVersion,const VUUID& inClientUUID)
	{
		VProgressIndicator* result=NULL;
		if(sProgressManager)
		{
			if(sProgressManager->fRemoteCreator)
				result = sProgressManager->fRemoteCreator->CreateRemoteProgress(inUUID,inVersion,inClientUUID);
		}
		return  result;
	}
	
	static VProgressIndicator* CreateRemoteProgress(const VValueBag& inValueBag,const VUUID& inClientUUID)
	{
		VProgressIndicator* result=NULL;
		if(sProgressManager)
		{
			if(sProgressManager->fRemoteCreator)
				result = sProgressManager->fRemoteCreator->CreateRemoteProgress(inValueBag,inClientUUID);
		}
		return  result;
	}

	static void RegisterRemoteProgressCreator(IRemoteProgressCreator* inCreator)
	{
		if(sProgressManager)
			sProgressManager->fRemoteCreator=inCreator;
	}
	static void RegisterProgress(VProgressIndicator* inProgress);
	static void UnregisterProgress(VProgressIndicator* inProgress);
	static VProgressIndicator* RetainProgress(const VUUID& inUUID);

	static VError GetProgressInfo(VValueBag& outBag, const VString* userinfo = NULL) // if userinfo == nil then get all progress infos
	{
		if (sProgressManager)
			return sProgressManager->_GetProgressInfo(outBag, userinfo);
		else
			return VE_OK;
	}

	static VProgressIndicator* RetainProgressIndicator(const VString& userinfo)
	{
		if (sProgressManager)
			return sProgressManager->_RetainProgressIndicator(userinfo);
		else
			return NULL;
	}
};

END_TOOLBOX_NAMESPACE

#endif
