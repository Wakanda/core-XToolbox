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
#ifndef __VJSGlobalClass__
#define __VJSGlobalClass__

#include "JS4D.h"

BEGIN_TOOLBOX_NAMESPACE

//======================================================


class XTOOLBOX_API VJSSpecifics : public XBOX::VObject
{
public:
								VJSSpecifics()									{}
								~VJSSpecifics();
								
	typedef OsType	key_type;
	typedef void (*value_destructor_type)( void*);
	
	// common value destructors
	static	void				DestructorVoid( void *inValue)					{}
	static	void				DestructorVObject( void *inValue)				{ delete static_cast<XBOX::VObject*>( inValue);}
	static	void				DestructorReleaseVObject( void *inValue)		{ XBOX::IRefCountable *o = dynamic_cast<XBOX::IRefCountable*>( static_cast<XBOX::VObject*>( inValue)); if (o) o->Release(); }
	static	void				DestructorReleaseIRefCountable( void *inValue)	{ XBOX::IRefCountable *o = static_cast<XBOX::IRefCountable*>( inValue); if (o) o->Release(); }
	static	void				DestructorReleaseCComponent( void *inValue)		{ XBOX::CComponent *o = static_cast<XBOX::CComponent*>( inValue); if (o) o->Release(); }

			void*				Get( key_type inKey) const;
			bool				Set( key_type inKey, void *inValue, value_destructor_type inDestructor);

private:
	typedef std::map<key_type, std::pair<void*,value_destructor_type> >	MapOfValues;
			MapOfValues			fMap;
};


//======================================================

class XTOOLBOX_API IJSRuntimeDelegate
{
public:

	virtual	XBOX::VFolder*					RetainScriptsFolder() = 0;

	virtual XBOX::VProgressIndicator*		CreateProgressIndicator( const XBOX::VString& inTitle) = 0;
};


//======================================================

/*
	VJSGlobalObject is the private data class for one context global object.
	not thread safe except specifically claimed as such.
*/
class XTOOLBOX_API VJSGlobalObject : public XBOX::VObject, public XBOX::IRefCountable
{
friend class VJSContext;
public:
											VJSGlobalObject( JS4D::GlobalContextRef inContext, VJSContextGroup *inContextGroup, IJSRuntimeDelegate *inRuntimeDelegate);
	
											/*
												'DB4D'	->	CDB4DBaseContext*
											*/
			void*							GetSpecific( VJSSpecifics::key_type inKey) const															{ return fSpecifics.Get( inKey);}
			bool							SetSpecific( VJSSpecifics::key_type inKey, void *inValue, VJSSpecifics::value_destructor_type inDestructor)	{ return fSpecifics.Set( inKey, inValue, inDestructor);}

			IJSRuntimeDelegate*				GetRuntimeDelegate() const		{ return fRuntimeDelegate;}
			VJSContextGroup*				GetContextGroup() const			{ return fContextGroup;}
			JS4D::GlobalContextRef			GetContext() const				{ return fContext; }

											operator VJSObject() const;
											
											// register a file for the 'include' function.
											// A file can't be registered twice.
											// returns true if the file has been successfully registered.
			bool							RegisterIncludedFile( VFile *inFile);

			bool							IsAnIncludedFile( const VFile *inFile) const;

											// Returns true if some files have been changed since they have been included
			bool							IsIncludedFilesHaveBeenChanged() const;

											// Ask a garbage collection on associated context.
											// If the context is used by another thread, the garbage collection will be deferred until the next call to some c++ code from javascript or when the context is marked unused.
											// thread safe
			void							GarbageCollect();

											// GarbageCollectIfNecessary will collect garbage if a previous call to GarbageCollect couldn't be executed because the context was in use by another thread.
											// Not thread safe! Must be called from the thread using the context like any other method of this class except GarbageCollect().
			void							GarbageCollectIfNecessary();

											// the context is being marked as 'in use' by VJSGlobalContext::EvaluateScript.
											// this is to track the use of a context by a thread and synchronize the calls to GarbageCollect.
			void							UseContext();
			void							UnuseContext();

			const unordered_map_VString<VRefPtr<VFile> >& GetIncludedFiles() const
			{
				return fIncludedFiles;
			}

											// See CommonJS, also needed for compatibility with NodeJS.
											// TODO: Should be able to "require" native C++ objects and JS written modules.
											// Currently only able to retrieve native objects for JS method named requireNative().
											// require() actually implemented in JS.

			XBOX::VJSObject					Require (XBOX::VJSContext inContext, const XBOX::VString &inClassName);

private:
											~VJSGlobalObject();
											VJSGlobalObject( const VJSGlobalObject&);	// forbidden
											VJSGlobalObject& operator=( const VJSGlobalObject&);	// forbidden

			void							AskDeferredGarbageCollection()	{ XBOX::VInterlocked::Increment( &fGarbageCollectionPending);}

	typedef	unordered_map_VString<VRefPtr<VFile> >		MapOfIncludedFile;
	typedef	std::vector< std::pair< VFile*, VTime> >	VectorOfFileModificationTime;

			JS4D::GlobalContextRef			fContext;
			VJSContextGroup*				fContextGroup;	// not retained (this is our parent)
			VJSSpecifics					fSpecifics;
			IJSRuntimeDelegate*				fRuntimeDelegate;
			MapOfIncludedFile				fIncludedFiles;
			VectorOfFileModificationTime	fIncludedFilesModificationTime;

			VCriticalSection				fUseMutex;				// mutex to guard access to fContextUseTaskID
			sLONG							fContextUseCount;		// incremented each time the associated context has been marked in use by VJSGlobalContext::Use
			VTaskID							fContextUseTaskID;		// task id of the task that marked the context in use
			sLONG							fGarbageCollectionPending;	// incremented each time a deferred garbage collection is being asked
			sLONG							fLastGarbageCollection;	// the deferred garbage collection stamp read during last garbage collect
		
			class CCompare {

			public:
		
				bool operator() (const XBOX::VString &inStringA, const XBOX::VString &inStringB) const
				{ 
					return inStringA.CompareToString(inStringB, true) == XBOX::CR_SMALLER;				
				}

			};
			
			typedef	std::map<XBOX::VString, XBOX::JS4D::ObjectRef, CCompare>	SObjectMap;
				
			SObjectMap						fRequireMap;
};


//======================================================


class XTOOLBOX_API VJSGlobalClass : public VJSClass<VJSGlobalClass, VJSGlobalObject >
{
public:
	typedef VJSClass<VJSGlobalClass, void >	inherited;

	/*
		You can add static functions and static values to this class definition before it's get registered.
		
		Beware that as soon as VJSGlobalClass::Class() is being called, calling AddStaticFunction or AddStaticValue won't have any effect.
		(an assert will remind it to you)
	*/
	static	void	AddStaticFunction( const char *inName, JSObjectCallAsFunctionCallback inCallBack, JS4D::PropertyAttributes inAttributes);
	static	void	AddStaticValue( const char *inName, JSObjectGetPropertyCallback inGetCallback, JSObjectSetPropertyCallback inSetCallback, JS4D::PropertyAttributes inAttributes);
	
	/*
		Add a constructor object as static value.
		
		Your constructor should be a function like this:
			static	void			Construct( XBOX::VJSParms_construct& inParms);
		
		And you should call AddConstructorObjectStaticValue like this:
			AddConstructorObjectStaticValue( "myClass", XBOX::VJSParms_construct::Callback<Construct>);

	*/
	static	void	AddConstructorObjectStaticValue( const char *inClassName, JSObjectCallAsConstructorCallback inCallback);

	static	void	GetDefinition( ClassDefinition& outDefinition);
	static	void	CreateGlobalClasses();

	static	void	Initialize( const VJSParms_initialize& inParms, VJSGlobalObject* inGlobalObject);
	static	void	Finalize( const VJSParms_finalize& inParms, VJSGlobalObject* inGlobalObject);

	static	void	do_trace( VJSParms_callStaticFunction& inParms, VJSGlobalObject*);
	static	void	do_displayNotification( VJSParms_callStaticFunction& inParms, VJSGlobalObject*);
	static	void	do_include( VJSParms_callStaticFunction& inParms, VJSGlobalObject* inContext);
	static	void	do_testMe( VJSParms_callStaticFunction& inParms, VJSGlobalObject*);

	static	void	do_GetProgressIndicator(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inContext);
	static	void	do_ProgressIndicator(VJSParms_callStaticFunction& ioParms, VJSGlobalObject *inContext);
	static	void	do_BinaryStream(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_XmlToJSON(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_JSONToXml(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_loadImage(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_garbageCollect(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_isoToDate(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_dateToIso(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_AtomicSection(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_SyncEvent(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_System(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);	// Not used.

	static	void	do_LoadText(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_SaveText(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_GetURLPath(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	static	void	do_GetURLQuery(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);
	
	static	void	do_GenerateUUID(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_Require(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);

	static	void	do_SetCurrentUser(VJSParms_callStaticFunction& ioParms, VJSGlobalObject*);


private:
	static	void	_CheckInitStaticFunctions();
	static	void	_CheckInitStaticValues();
	static	void	_PushBackStaticFunction( const char *inName, JSObjectCallAsFunctionCallback inCallBack, JS4D::PropertyAttributes inAttributes);
	static	void	_PushBackStaticValue( const char *inName, JSObjectGetPropertyCallback inGetCallback, JSObjectSetPropertyCallback inSetCallback, JS4D::PropertyAttributes inAttributes);

// have to redefine this struct to remove the stupid const and use it in a std::vector
	typedef struct {
		const char* name;
		JSObjectCallAsFunctionCallback callAsFunction;
		JSPropertyAttributes attributes;
	} _StaticFunction;
	typedef		std::vector<_StaticFunction>	VectorOfStaticFunction;
	static VectorOfStaticFunction	sStaticFunctions;

	
// have to redefine this struct to remove the stupid const and use it in a std::vector
	typedef struct {
		const char* name;
		JSObjectGetPropertyCallback getProperty;
		JSObjectSetPropertyCallback setProperty;
		JSPropertyAttributes attributes;
	} _StaticValue;
	typedef		std::vector<_StaticValue>	VectorOfStaticValue;
	static VectorOfStaticValue	sStaticValues;
	
	static	bool	sGetDefinitionHasBeenCalled;	// spy to know if one can still call AddStaticFunction & AddStaticValue
};

END_TOOLBOX_NAMESPACE

#endif
