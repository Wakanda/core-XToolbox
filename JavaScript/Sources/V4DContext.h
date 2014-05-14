#if USE_V8_ENGINE


#ifndef V4D__CONTEXT__H__
#define V4D__CONTEXT__H__

#include <v8.h>
#include <JS4D.h>

BEGIN_TOOLBOX_NAMESPACE

/*class JS4D::V8ObjectGetPropertyCallback
{
public:
									V8ObjectGetPropertyCallback(XBOX::GetPropertyFn inFunction) { fGetPropertyFn = inFunction; }

	static	void					AccessorGetterCallback(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value>& info);

private:
	XBOX::GetPropertyFn				fGetPropertyFn;
};*/

class V4DContext: public XBOX::VObject
{
public:
		V4DContext(v8::Isolate* isolate,XBOX::JS4D::ClassRef inGlobalClassRef);


		bool					EvaluateScript(const XBOX::VString& inScript, const VString& inUrl, VJSValue *outResult);
		bool					CheckScriptSyntax(const XBOX::VString& inScript, const VString& inUrl);

		v8::Persistent<v8::FunctionTemplate>*	GetImplicitFunctionConstructor(const xbox::VString& inName);

static	void 					AddNativeFunction( const xbox::JS4D::StaticFunction& inStaticFunction );

static	void 					AddNativeClass( xbox::JS4D::ClassDefinition* inClassDefinition );
static	void 					AddNativeConstructorForClass(const char *inClassName, JS4D::ObjectCallAsConstructorCallback inCallback );

static	void 					AddNativeObjectTemplate(
							const xbox::VString& inName,
							v8::Handle<v8::ObjectTemplate>& inObjTemp );




		v8::Handle<v8::Object>	GetFunction(const char* inClassName);

		v8::Handle<v8::Function>	CreateFunction(	JS4D::ObjectCallAsFunctionCallback	inFunctionCbk );

		v8::Handle<v8::Function>	CreateFunctionTemplateForClass(	JS4D::ClassRef inClassRef,
																	const JS4D::StaticFunction* inStaticFunction = NULL,
																	bool	inLockResources = true,
																	bool	inAddPersistentConstructor = false );

		v8::Handle<v8::Function>	CreateFunctionTemplateForClass2(	JS4D::ClassRef inClassRef,
																	JS4D::ObjectCallAsConstructorCallback inStaticFunction = NULL,
																	bool	inLockResources = true,
																	bool	inAddPersistentConstructor = false,
				JS4D::ObjectCallAsFunctionCallback inP1 = NULL,
				JS4D::ObjectCallAsFunctionCallback inP2 = NULL);

		v8::Handle<v8::Function>	CreateFunctionTemplateForClassConstructor(
											JS4D::ClassRef inClassRef,
											const JS4D::ObjectCallAsFunctionCallback inConstructorCallback,
											void* inData,
											bool inAcceptCallAsFunction = false,
											JS4D::StaticFunction inContructorFunctions[] = NULL);


		void*					GetGlobalObjectPrivateInstance();
		void					SetGlobalObjectPrivateInstance(void* inPrivateData);

		void*					GetObjectPrivateData(v8::Value* inObject);
		void					SetObjectPrivateData(v8::Value* inObject, void* inPrivateData);
	
static 	v8::Persistent <v8::Context,v8::NonCopyablePersistentTraits<v8::Context> >&		GetPersistentContext(v8::Isolate* inIsolate);
static	void*					GetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject);
static	void					SetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject, void* inPrivateData);

	v8::Isolate*			GetIsolate() { return fIsolate; }

private:
	bool								fIsOK;
	v8::Isolate* 						fIsolate;
	xbox::JS4D::ClassDefinition* 		fGlobalClassRef;
	v8::Persistent <v8::Context,v8::NonCopyablePersistentTraits<v8::Context> >	fPersistentCtx;
	std::map< xbox::VString , v8::Persistent<v8::FunctionTemplate>* >			fPersImplicitFunctionConstructorMap;
	
		void						UpdateContext();
		void						AddNativeFunctionTemplate(	const xbox::VString& inName,
																v8::Handle<v8::FunctionTemplate>& inFuncTemp);


static	void						ConstrCbk(const v8::FunctionCallbackInfo<v8::Value>& args);
static	bool 						CheckIfNew( const XBOX::VString& inName );
static	void						Print(const v8::FunctionCallbackInfo<v8::Value>& args);

static	XBOX::VCriticalSection											sLock;
static	std::map< xbox::VString, xbox::JS4D::ClassDefinition >			sNativeClassesMap;



class ConstructorCallbacks
{
public:
				ConstructorCallbacks(	JS4D::ClassDefinition* inClassDef,
										bool inWithoutConstructors = true,
										const JS4D::StaticFunction* inStaticFunction = NULL );

	bool		HasCallbacks() { return (fObjectCallAsConstructorCallback != NULL) || ( fObjectCallAsConstructorFunctionCallback != NULL); }
	void		Update(JS4D::ClassDefinition* inClassDef);

	xbox::JS4D::ClassDefinition							fClassDef;
	JS4D::ObjectCallAsConstructorCallback				fObjectCallAsConstructorCallback;
	JS4D::ObjectCallAsFunctionCallback					fObjectCallAsConstructorFunctionCallback;
//	JS4D::ObjectCallAsConstructorFunctionCallback		fObjectCallAsConstructorFunctionCallback;
	XBOX::VString										fClassName;
};

static	std::map< xbox::VString, ConstructorCallbacks* >					sClassesCallbacks;

};
#endif

END_TOOLBOX_NAMESPACE

#endif
