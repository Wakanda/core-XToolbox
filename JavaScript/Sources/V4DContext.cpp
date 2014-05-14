#include "VJavaScriptPrecompiled.h"

#if USE_V8_ENGINE

#include <V4DContext.h>
#include <v8.h>
#include <VJSClass.h>
#include <VJSValue.h>

using namespace v8;
USING_TOOLBOX_NAMESPACE

XBOX::VCriticalSection											V4DContext::sLock;
std::map< xbox::VString, xbox::JS4D::ClassDefinition >			V4DContext::sNativeClassesMap;

std::map< xbox::VString, V4DContext::ConstructorCallbacks* >	V4DContext::sClassesCallbacks;

V4DContext::ConstructorCallbacks::ConstructorCallbacks(	JS4D::ClassDefinition* inClassDef,
														bool inWithoutConstructors,
														const JS4D::StaticFunction* inStaticFunction  )
:fClassName(inClassDef->className)
{
	fClassDef = *inClassDef;
	if (inWithoutConstructors)
	{
		xbox_assert( (inClassDef->callAsFunction == NULL) && (inClassDef->callAsConstructor == NULL) );
		//DebugMsg(":ConstructorCallbacks::ConstructorCallbacks for %s build without constructors\n",inClassDef->className);
		fObjectCallAsConstructorCallback = NULL;
		fObjectCallAsConstructorFunctionCallback = NULL;
	}
	else
	{
		fObjectCallAsConstructorCallback = inClassDef->callAsConstructor;
//		fObjectCallAsConstructorFunctionCallback = inClassDef->callAsFunction;
		if (!fObjectCallAsConstructorCallback && !fObjectCallAsConstructorFunctionCallback)
		{
			//DebugMsg(":ConstructorCallbacks::ConstructorCallbacks for %s both NULL default construc, using static function\n",inClassDef->className);
			fObjectCallAsConstructorFunctionCallback = inStaticFunction->callAsFunction;
		}
	}
}

void V4DContext::ConstructorCallbacks::Update(JS4D::ClassDefinition* inClassDef)
{
	xbox_assert(inClassDef->callAsConstructor || inClassDef->callAsFunction);
	if (!fObjectCallAsConstructorCallback)
	{
		//DebugMsg("ConstructorCallbacks::Update fObjectCallAsConstructorCallback NULL for %s\n",inClassDef->className);
		fObjectCallAsConstructorCallback = inClassDef->callAsConstructor;
	}
	if (fObjectCallAsConstructorFunctionCallback)
	{
		//DebugMsg("ConstructorCallbacks::Update fObjectCallAsConstructorFunctionCallback NULL for %s\n",inClassDef->className);
//		fObjectCallAsConstructorFunctionCallback = inClassDef->callAsFunction;
	}
}


V4DContext::V4DContext(v8::Isolate* isolate,JS4D::ClassRef inGlobalClassRef)
:fIsOK(false)
,fIsolate(isolate)
,fGlobalClassRef((JS4D::ClassDefinition*)inGlobalClassRef)
{
	//XBOX::VPtr	stackAddr = VTask::GetCurrent()->GetStackAddress();
	//size_t		stackSize = VTask::GetCurrent()->GetStackSize();

	fIsolate->SetData(this);
	xbox_assert(fIsolate == Isolate::GetCurrent());
	HandleScope handle_scope(fIsolate);
	Handle<Context> initCtx = Context::New(fIsolate,NULL);
	fPersistentCtx.Reset(fIsolate,initCtx);
//	fBlobPersistentFunctionTemplate = new Persistent<v8::FunctionTemplate>();

	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(context);
	Handle<Object> global = context->Global();
	bool	isFound;
	Handle<FunctionTemplate> 	blobFuncTemplate = FunctionTemplate::New();
	if (fGlobalClassRef->staticFunctions)
	{
		for( const JS4D::StaticFunction *i = fGlobalClassRef->staticFunctions ; i->name != NULL ; ++i)
		{
			//DebugMsg("V4DContext::V4DContext treat static function %s for %s\n",i->name,fGlobalClassRef->className);

			if (false)
			{
				Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New();
				if (!strcmp(i->name,"Blob"))
				{
					blobFuncTemplate = classFuncTemplate;
				}
#if 0
				Local<ObjectTemplate> 		instance = classFuncTemplate->InstanceTemplate();
				if (classDefinition.staticFunctions)
				{
					classFuncTemplate->SetClassName(String::NewSymbol(i->name));
   					instance->SetInternalFieldCount(0);
					for( const JS4D::StaticFunction *j = classDefinition.staticFunctions ; j->name != NULL ; ++j)
					{
						DebugMsg("V4DContext::V4DContext treat static function %s for %s\n",j->name,classDefinition.className);
					
						Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(j->callAsFunction, v8::Undefined());
						functionTemplate->RemovePrototype();
						classFuncTemplate->PrototypeTemplate()->Set(
								v8::String::NewSymbol(j->name), functionTemplate, static_cast<v8::PropertyAttribute>(v8::DontDelete));

					}
				}
				if (classDefinition.staticValues)
				{
					for( const JS4D::StaticValue *j = classDefinition.staticValues ; j->name != NULL ; ++j)
					{
						DebugMsg("V4DContext::V4DContext treat static value %s for %s\n",j->name,classDefinition.className);
						JS4D::ObjectGetPropertyCallback cbkG = j->getProperty;
						JS4D::ObjectSetPropertyCallback cbkS = j->setProperty;
						if (testAssert(cbkG || cbkS))
						{
							v8::AccessorGetterCallback accGetterCbk = NULL;
							v8::AccessorSetterCallback accSetterCbk = NULL;
							if (cbkG)
							{
								accGetterCbk = (v8::AccessorGetterCallback)VJSClass__GetV8PropertyGetterCallback(cbkG());
							}
							if (cbkS)
							{
								accSetterCbk = (v8::AccessorSetterCallback)VJSClass__GetV8PropertySetterCallback(cbkS());
							}
							void*						accHandler = VJSClass__GetV8PropertyHandlerCallback(cbkG,cbkS);
							Handle<External>			extPtr = External::New(accHandler);
							instance->SetAccessor(v8::String::NewSymbol(j->name),accGetterCbk,accSetterCbk,extPtr);
						}
					}
				}
				classFuncTemplate->SetLength(0);
				DebugMsg("V4DContext::V4DContext treat static constructor %s\n",classDefinition.className);
					
				JS4D::ObjectCallAsConstructorCallback constructorCbk = JS4D::GetConstructorObject( classDefinition.className, NULL);
				if (constructorCbk)
				{
					DebugMsg("V4DContext::V4DContext CONSTRUCTOR FOUND for %s\n",classDefinition.className);
						//Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(constructorCbk, v8::Undefined());
						//functionTemplate->RemovePrototype();
						//classFuncTemplate->PrototypeTemplate()->Set(v8::String::NewSymbol(classDefinition.className:), functionTemplate, static_cast<v8::PropertyAttribute>(v8::DontDelete));
					classFuncTemplate->SetCallHandler(constructorCbk);
				}
				if (classDefinition.callAsConstructor)
				{
					//classFuncTemplate->SetCallHandler(classDefinition.callAsConstructor);
				}
				AddNativeFunctionTemplate(i->name,classFuncTemplate);
#endif
			}
			else
			{
				DebugMsg("V4DContext::V4DContext treat static function1 %s for %s\n",i->name,fGlobalClassRef->className);
				JS4D::ClassDefinition classDefinition;
				memset(&classDefinition,0,sizeof(classDefinition));
				
				/*
				// Build the constructor of the class and add it to global ctx
				sLock.Lock();
				std::map< xbox::VString, xbox::JS4D::ClassDefinition >::iterator	itClass = sNativeClassesMap.find(i->name);

				sLock.Unlock();
				if (itClass != sNativeClassesMap.end())
				{
					Handle<Function> newFunc = V4DContext::CreateFunctionTemplateForClass( (JS4D::ClassRef)&((*itClass).second),i );
					global->Set(String::NewSymbol(i->name),newFunc);
				}
				else*/
				{

					Handle<FunctionTemplate> ft = FunctionTemplate::New(i->callAsFunction);
					Handle<Function> newFunc = ft->GetFunction();
					global->Set(String::NewSymbol(i->name),newFunc);
				
				}
	
				/*if (!strcmp(i->name,"Blob"))
				{
					fBlobPersistentFunctionTemplate->Reset(fIsolate,ft);
					Handle<Function> newFunc = ft->GetFunction();

				}*/

			}

		}
	}
	
	if (fGlobalClassRef->staticValues)
	{
		for( const JS4D::StaticValue *sv = fGlobalClassRef->staticValues ; sv->name != NULL ; ++sv)
		{
			DebugMsg("V4DContext::V4DContext treat static value1 %s for %s\n",sv->name,fGlobalClassRef->className);
			JS4D::ObjectGetPropertyCallback cbkG = sv->getProperty;
			JS4D::ObjectSetPropertyCallback cbkS = sv->setProperty;
			if (testAssert(cbkG || cbkS))
			{
				v8::AccessorGetterCallback accGetterCbk = NULL;
				v8::AccessorSetterCallback accSetterCbk = NULL;
				if (cbkG)
				{
					accGetterCbk = (v8::AccessorGetterCallback)VJSClass__GetV8PropertyGetterCallback(cbkG());
				}
				if (cbkS)
				{
					accSetterCbk = (v8::AccessorSetterCallback)VJSClass__GetV8PropertySetterCallback(cbkS());
				}
				void*						accHandler = VJSClass__GetV8PropertyHandlerCallback(cbkG,cbkS);
				Handle<External>			extPtr = External::New(accHandler);
				if (!global->Has(v8::String::NewSymbol(sv->name)))
				{
					global->SetAccessor(v8::String::NewSymbol(sv->name),
										accGetterCbk,
										accSetterCbk,
										extPtr,
										v8::DEFAULT);
				}
				else
				{
					DebugMsg("V4DContext::V4DContext global obj already has static value1 %s for %s\n",sv->name,fGlobalClassRef->className);
				}
			}
		}
	}

#if 0 // do not add the classes by default
	sLock.Lock();
	std::map< xbox::VString, JS4D::ClassDefinition >::iterator	itClass = sNativeClassesMap.begin();
	while(itClass != sNativeClassesMap.end())
	{
		VStringConvertBuffer	bufferTmp((*itClass).first,VTC_UTF_8);
		const char*	className = bufferTmp.GetCPointer();
		if (!global->Has(v8::String::New(bufferTmp.GetCPointer())))// && !strcmp(className,"File"))
		{
			JS4D::ClassDefinition	classDef;
			memset(&classDef,0, sizeof(classDef));
			classDef = (*itClass).second;
			/*DebugMsg(	"V4DContext::V4DContext treat class %s parent=%s\n",
						className,
						(classDef.parentClass	? ((JS4D::ClassDefinition*)classDef.parentClass)->className
												: "None"));*/
			Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New();
   			Local<ObjectTemplate> 		instance = classFuncTemplate->InstanceTemplate();
			if (classDef.staticFunctions)
			{
				classFuncTemplate->SetClassName(String::NewSymbol(className));
				instance->SetInternalFieldCount(0);
				for( const JS4D::StaticFunction *j = classDef.staticFunctions ; j->name != NULL ; ++j)
				{
					//DebugMsg("V4DContext::V4DContext treat static function %s for %s\n",j->name,classDef.className);
					
					Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(j->callAsFunction, v8::Undefined());
					functionTemplate->RemovePrototype();
					classFuncTemplate->PrototypeTemplate()->Set(
							v8::String::NewSymbol(j->name), functionTemplate, static_cast<v8::PropertyAttribute>(v8::DontDelete));

				}
			}
			if (classDef.staticValues)
			{
				for( const JS4D::StaticValue *j = classDef.staticValues ; j->name != NULL ; ++j)
				{
					DebugMsg("V4DContext::V4DContext treat static value %s for %s\n",j->name,classDef.className);
					JS4D::ObjectGetPropertyCallback cbkG = j->getProperty;
					JS4D::ObjectSetPropertyCallback cbkS = j->setProperty;
					if (testAssert(cbkG || cbkS))
					{
						v8::AccessorGetterCallback accGetterCbk = NULL;
						v8::AccessorSetterCallback accSetterCbk = NULL;
						if (cbkG)
						{
							accGetterCbk = (v8::AccessorGetterCallback)VJSClass__GetV8PropertyGetterCallback(cbkG());
						}
						if (cbkS)
						{
							accSetterCbk = (v8::AccessorSetterCallback)VJSClass__GetV8PropertySetterCallback(cbkS());
						}
						void*						accHandler = VJSClass__GetV8PropertyHandlerCallback(cbkG,cbkS);
						Handle<External>			extPtr = External::New(accHandler);
						instance->SetAccessor(	v8::String::NewSymbol(j->name),
												accGetterCbk,
												accSetterCbk,
												extPtr,
												v8::DEFAULT);
					}
				}
			}

			classFuncTemplate->SetLength(0);
			//DebugMsg("V4DContext::V4DContext treat static constructor %s\n",classDef.className);
			ConstructorCallbacks*	cbks = NULL;
			std::map< xbox::VString, ConstructorCallbacks* >::iterator	itCbks = sClassesCallbacks.find(className);
			if ( itCbks != sClassesCallbacks.end() )
			{
				cbks = (*itCbks).second;
				cbks->Update(&classDef);
			}
			if (!cbks)
			{
				cbks = new ConstructorCallbacks(&classDef);
				sClassesCallbacks.insert( std::pair< xbox::VString, ConstructorCallbacks* >( className, cbks ) );
			}

			Handle<External> extPtr = External::New((void*)cbks);

			classFuncTemplate->SetCallHandler(V4DContext::ConstrCbk,extPtr);
			AddNativeFunctionTemplate(className,classFuncTemplate);
		}
		else
		{
			//DebugMsg("V4DContext::V4DContext already OK for class %s\n",bufferTmp.GetCPointer());
		}
		itClass++;
	}
	sLock.Unlock();
#endif

	fPersistentCtx.Reset(fIsolate,context);

}
void V4DContext::ConstrCbk(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	if (testAssert(args.Data()->IsExternal()))
	{
		Handle<External> extHdl = Handle<External>::Cast(args.Data());
		ConstructorCallbacks*	cbks = (ConstructorCallbacks*)extHdl->Value();
		V4DContext*	v8Ctx = (V4DContext*)(args.GetIsolate()->GetData());
		//Persistent<FunctionTemplate>*	pFT = v8Ctx->GetImplicitFunctionConstructor(cbks->fClassName);
		//Local<Function> locFunc = (pFT->val_)->GetFunction();
		Local<Function> locFunc = args.Callee();
		if (args.IsConstructCall())
		{
			if (cbks->fObjectCallAsConstructorCallback)
			{
				VJSParms_callAsConstructor	parms(args);
				cbks->fObjectCallAsConstructorCallback(&parms);
			}
			else
			{
				//DebugMsg("V4DContext::ConstrCbk has no specific treatment for '%s'\n",cbks->fClassDef.className);
			}
		}
		else
		{
			if (testAssert(cbks->fObjectCallAsConstructorFunctionCallback))
			{
				Handle<Value> valuesArray[32];
			    for( int idx=0; idx<32; idx++ )
			    {
					valuesArray[idx] = v8::Integer::New(0);
				}
				Local<Value> locVal = locFunc->CallAsConstructor(32,valuesArray);
				//VJSValue	rtnVal(locVal.val_);
				//parms.ReturnValue(rtnVal);
				VJSParms_callAsFunction	parms(args,locVal.val_);
				xbox_assert(false);
				//cbks->fObjectCallAsConstructorFunctionCallback(&parms);
				args.GetReturnValue().Set(locVal);
			}
		}
	}
}

Handle<Object> V4DContext::GetFunction(const char* inClassName)
{
	HandleScope					handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();

	Handle<Value>		funcVal = global->Get(String::NewSymbol(inClassName));
	Handle<Object>	result = Handle<Object>();
	if (funcVal->IsFunction())
	{
		result = funcVal->ToObject();
	}
	return handle_scope.Close(result);
}

Handle<Function>	V4DContext::CreateFunction(	JS4D::ObjectCallAsFunctionCallback	inFunctionCbk )
{
	xbox_assert(fIsolate == v8::Isolate::GetCurrent());
	HandleScope					handle_scope(fIsolate);
	Local<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(context);
	Handle<Function>			localFunction = Function::New(fIsolate,inFunctionCbk);
	return handle_scope.Close(localFunction);
}

Handle<Function> V4DContext::CreateFunctionTemplateForClassConstructor(
						JS4D::ClassRef inClassRef,
						const JS4D::ObjectCallAsFunctionCallback inConstructorCallback,
						void* inData,
						bool inAcceptCallAsFunction,
						JS4D::StaticFunction inContructorFunctions[] )
{
	HandleScope					handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;

	//if (testAssert(!global->Has(v8::String::NewSymbol(classDef->className))))
	{
		Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New();
		//if (inLockResources)
		sLock.Lock();

		JS4D::ClassDefinition*	parentClassDef = (JS4D::ClassDefinition*)classDef->parentClass;
    	Local<ObjectTemplate> 		instance = classFuncTemplate->InstanceTemplate();
		if (classDef->staticFunctions)
		{
			classFuncTemplate->SetClassName(String::NewSymbol(classDef->className));
   			instance->SetInternalFieldCount(0);
			for( const JS4D::StaticFunction *j = classDef->staticFunctions ; j->name != NULL ; ++j)
			{
				//DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor treat static function %s for %s\n",j->name,classDef->className);
					
				Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(j->callAsFunction, v8::Undefined());
				functionTemplate->RemovePrototype();
				classFuncTemplate->PrototypeTemplate()->Set(
					v8::String::NewSymbol(j->name), functionTemplate, static_cast<v8::PropertyAttribute>(v8::DontDelete));
			}
		}
		if (classDef->staticValues)
		{
			for( const JS4D::StaticValue *j = classDef->staticValues ; j->name != NULL ; ++j)
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor treat static value %s for %s\n",j->name,classDef->className);
				JS4D::ObjectGetPropertyCallback cbkG = j->getProperty;
				JS4D::ObjectSetPropertyCallback cbkS = j->setProperty;
				if (testAssert(cbkG || cbkS))
				{
					v8::AccessorGetterCallback accGetterCbk = NULL;
					v8::AccessorSetterCallback accSetterCbk = NULL;
					if (cbkG)
					{
						accGetterCbk = (v8::AccessorGetterCallback)VJSClass__GetV8PropertyGetterCallback(cbkG());
					}
					if (cbkS)
					{
						accSetterCbk = (v8::AccessorSetterCallback)VJSClass__GetV8PropertySetterCallback(cbkS());
					}
					void*						accHandler = VJSClass__GetV8PropertyHandlerCallback(cbkG,cbkS);
					Handle<External>			extPtr = External::New(accHandler);
					instance->SetAccessor(	v8::String::NewSymbol(j->name),
												accGetterCbk,
												accSetterCbk,
												extPtr,
												v8::DEFAULT);
				}
			}
		}
		/*DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor class '%s' parent= '%s'\n",
						classDef->className,
						(parentClassDef	? parentClassDef->className : "None"));*/

		if (parentClassDef)
		{
			//if (testAssert(!strcmp(parentClassDef->className,"Blob")))
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor class '%s' needs parent '%s'\n",
					 classDef->className,
					 parentClassDef->className );
				std::map< xbox::VString, Persistent<FunctionTemplate>* >::iterator
					itIFT =	fPersImplicitFunctionConstructorMap.find(parentClassDef->className);
				if (testAssert(itIFT != fPersImplicitFunctionConstructorMap.end()))
				{
					//Handle<FunctionTemplate>	parentFT = Handle<FunctionTemplate>::New(fIsolate,*fBlobPersistentFunctionTemplate);
					Handle<FunctionTemplate>	parentFT = Handle<FunctionTemplate>::New(fIsolate, *((*itIFT).second));
					classFuncTemplate->Inherit(parentFT);
				}
				else
				{
					DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor class '%s' needs NOT FOUND parent '%s !!!!!!'\n",
						classDef->className,
						parentClassDef->className);
				}
				v8::Local<v8::ObjectTemplate> prototype = classFuncTemplate->PrototypeTemplate();
				prototype->SetInternalFieldCount(0);
			}

		}

		classFuncTemplate->SetLength(0);
		//DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor treat static constructor %s\n",classDef->className);


		Handle<External> extPtr = External::New(inData);

		classFuncTemplate->SetCallHandler(inConstructorCallback,extPtr);
		/*if (!strcmp("Blob",classDef->className))
		{
			//DebugMsg(	"V4DContext::CreateFunctionTemplateForClass treat fBlobPersistentFunctionTemplate\n");
			fBlobPersistentFunctionTemplate->Reset(fIsolate,classFuncTemplate);
		}*/

		Handle<Function> localFunction = classFuncTemplate->GetFunction();
		if (inContructorFunctions)
		{
			for (JS4D::StaticFunction* tmpFunc = inContructorFunctions; tmpFunc->name; tmpFunc++)
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClassConstructor adding %s method in %s constructor\n",
					tmpFunc->name,
					classDef->className);
				Handle<FunctionTemplate>	newFuncTempl = v8::FunctionTemplate::New(tmpFunc->callAsFunction);
				//classFuncTemplate->SetAccessorProperty(v8::String::NewSymbol(tmpFunc->name), newFuncTempl);
				localFunction->Set(v8::String::NewSymbol(tmpFunc->name), newFuncTempl->GetFunction());
			}
		}
		//if (inAddPersistentConstructor)
		{
			AddNativeFunctionTemplate(classDef->className,classFuncTemplate);
		}

		sLock.Unlock();
		
		return handle_scope.Close(localFunction);
	}
	/*else
	{
		Handle<Function> localFunction = Handle<Function>::New(fIsolate,NULL);
		return handle_scope.Close(localFunction);
	}*/
}
Handle<Function> V4DContext::CreateFunctionTemplateForClass2(JS4D::ClassRef inClassRef,
																	JS4D::ObjectCallAsConstructorCallback inStaticFunction,
																	bool	inLockResources,
																	bool	inAddPersistentConstructor,	
				JS4D::ObjectCallAsFunctionCallback inP1,
				JS4D::ObjectCallAsFunctionCallback inP2)
{
	HandleScope					handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();
	if (0)
	{
	}
	else
	{

		Handle<Function> localFunction = Handle<Function>::New(fIsolate,NULL);
		return handle_scope.Close(localFunction);

	}
}


Handle<Function> V4DContext::CreateFunctionTemplateForClass(JS4D::ClassRef inClassRef,
															const JS4D::StaticFunction* inStaticFunction,
															bool	inLockResources,
															bool inAddPersistentConstructor )
{
	HandleScope					handle_scope(fIsolate);
	Handle<Context>				context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object>				global = context->Global();
	JS4D::ClassDefinition*		classDef = (JS4D::ClassDefinition*)inClassRef;
	
	//DebugMsg(	"V4DContext::CreateFunctionTemplateForClass treat class %s \n",classDef->className);
	
	//if (!global->Has(v8::String::NewSymbol(classDef->className)))
	{
		Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New();
		if (inLockResources)
			sLock.Lock();

		JS4D::ClassDefinition*	parentClassDef = (JS4D::ClassDefinition*)classDef->parentClass;
		/*DebugMsg(	"V4DContext::CreateFunctionTemplateForClass treat class %s parent=%s\n",
						classDef->className,
						(parentClassDef	? parentClassDef->className : "None"));*/
    	Local<ObjectTemplate> 		instance = classFuncTemplate->InstanceTemplate();
		
		if (classDef->callAsFunction)
		{
			instance->SetCallAsFunctionHandler(classDef->callAsFunction);
		}

		if (classDef->staticFunctions)
		{
			classFuncTemplate->SetClassName(String::NewSymbol(classDef->className));
   			instance->SetInternalFieldCount(0);
			for( const JS4D::StaticFunction *j = classDef->staticFunctions ; j->name != NULL ; ++j)
			{
				//DebugMsg("V4DContext::CreateFunctionTemplateForClass treat static function %s for %s\n",j->name,classDef->className);
					
				Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(j->callAsFunction, v8::Undefined());
				functionTemplate->RemovePrototype();
				classFuncTemplate->PrototypeTemplate()->Set(
								v8::String::NewSymbol(j->name), functionTemplate, static_cast<v8::PropertyAttribute>(v8::DontDelete));
			}
		}
		if (classDef->staticValues)
		{
			for( const JS4D::StaticValue *j = classDef->staticValues ; j->name != NULL ; ++j)
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClass treat static value %s for %s\n",j->name,classDef->className);
				JS4D::ObjectGetPropertyCallback cbkG = j->getProperty;
				JS4D::ObjectSetPropertyCallback cbkS = j->setProperty;
				if (testAssert(cbkG || cbkS))
				{
					v8::AccessorGetterCallback accGetterCbk = NULL;
					v8::AccessorSetterCallback accSetterCbk = NULL;
					if (cbkG)
					{
						accGetterCbk = (v8::AccessorGetterCallback)VJSClass__GetV8PropertyGetterCallback(cbkG());
					}
					if (cbkS)
					{
						accSetterCbk = (v8::AccessorSetterCallback)VJSClass__GetV8PropertySetterCallback(cbkS());
					}
					void*						accHandler = VJSClass__GetV8PropertyHandlerCallback(cbkG,cbkS);
					Handle<External>			extPtr = External::New(accHandler);
					instance->SetAccessor(	v8::String::NewSymbol(j->name),
												accGetterCbk,
												accSetterCbk,
												extPtr,
												v8::DEFAULT);
				}
			}
		}
		/*DebugMsg("V4DContext::CreateFunctionTemplateForClass class '%s' parent= '%s'\n",
						classDef->className,
						(parentClassDef	? parentClassDef->className : "None"));*/

		if (!testAssert(!parentClassDef))
		{
			int a = 1;
			/*if (testAssert(!strcmp(parentClassDef->className,"Blob")))
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClass class '%s' needs parent '%s'\n",
					 classDef->className,
					 parentClassDef->className );
				Handle<FunctionTemplate>	parentFT = Handle<FunctionTemplate>::New(fIsolate,*fBlobPersistentFunctionTemplate);
				classFuncTemplate->Inherit(parentFT);
				v8::Local<v8::ObjectTemplate> prototype = classFuncTemplate->PrototypeTemplate();
				prototype->SetInternalFieldCount(0);
			}
			else
			{
				DebugMsg("V4DContext::CreateFunctionTemplateForClass class '%s' parent '%s' NOT FOUND\n",
					 classDef->className,
					 parentClassDef->className );
			}*/
		}

		classFuncTemplate->SetLength(0);
		{

			//DebugMsg("V4DContext::CreateFunctionTemplateForClass treat static constructor %s\n",classDef->className);

			ConstructorCallbacks* cbks = NULL;
			std::map< xbox::VString, ConstructorCallbacks* >::iterator	itCbks = sClassesCallbacks.find(classDef->className);
			if (itCbks == sClassesCallbacks.end())
			{
				cbks = new ConstructorCallbacks(classDef, false, inStaticFunction);
				sClassesCallbacks.insert(std::pair< xbox::VString, ConstructorCallbacks* >(classDef->className, cbks));
			}
			else
			{
				cbks = (*itCbks).second;
				cbks->Update(classDef);
			}

			Handle<External> extPtr = External::New((void*)cbks);

			classFuncTemplate->SetCallHandler(V4DContext::ConstrCbk, extPtr);
		}

		/*if (!strcmp("Blob",classDef->className))
		{
			//DebugMsg(	"V4DContext::CreateFunctionTemplateForClass treat fBlobPersistentFunctionTemplate\n");
			fBlobPersistentFunctionTemplate->Reset(fIsolate,classFuncTemplate);
		}*/

		Handle<Function> localFunction =classFuncTemplate->GetFunction();

		if (inAddPersistentConstructor)
		{
			AddNativeFunctionTemplate(classDef->className,classFuncTemplate);
		}
		if (inLockResources)
			sLock.Unlock();
		
		return handle_scope.Close(localFunction);
	}
	/*else
	{
		Handle<Value>	locVal = global->Get(v8::String::New(classDef->className));
		Function*	fn = Function::Cast(locVal.val_);
		Handle<Function> localFunction = Handle<Function>::New(fIsolate,fn);
		return handle_scope.Close(localFunction);

	}*/
}


void V4DContext::AddNativeFunctionTemplate(	const xbox::VString& inName,
											Handle<FunctionTemplate>& inFuncTemp )
{
	//DebugMsg("V4DContext::AddNativeFunctionTemplate called for '%S' \n",&inName);
	std::map< VString , Persistent<FunctionTemplate>* >::iterator	itIFT =
		fPersImplicitFunctionConstructorMap.find(inName);
	if (testAssert(itIFT == fPersImplicitFunctionConstructorMap.end()))
	{
		VStringConvertBuffer	bufferTmp(inName,VTC_UTF_8);
		HandleScope handle_scope(fIsolate);
		Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
		Context::Scope context_scope(context);
		Persistent<FunctionTemplate>*	newPersFT = new Persistent<FunctionTemplate>();
		std::pair<VString , Persistent<FunctionTemplate>* > newElt(inName,newPersFT );
		newElt.second->Reset(fIsolate,inFuncTemp);
		fPersImplicitFunctionConstructorMap.insert(newElt);
		/*Handle<Object> global = context->Global();
		Local<Function> localFunction = inFuncTemp->GetFunction();
		global->Set(String::New(bufferTmp.GetCPointer()),localFunction);*/
		fPersistentCtx.Reset(fIsolate,context);
	}
}
/*
void V4DContext::BuildConstructor(const xbox::VString& inClassName)
{
	std::map< xbox::VString, xbox::JS4D::ClassDefinition >::iterator	itClass = sNativeClassesMap.find(inClassName);
	if (itClass != sNativeClassesMap.end())
	{
		JS4D::ClassDefinition*	classDef = (JS4D::ClassDefinition*)&(*itClass).second;
		JS4D::ClassDefinition*	parentClassDef = (JS4D::ClassDefinition*)classDef->parentClass;

		DebugMsg(	"V4DContext::BuildConstructor treat class %S parent=%s\n",
					&inClassName,
					(parentClassDef	? parentClassDef->className : "None"));

		Handle<FunctionTemplate> 	classFuncTemplate = FunctionTemplate::New();
    	Local<ObjectTemplate> 		instance = classFuncTemplate->InstanceTemplate();
		if (classDef->staticFunctions)
		{
			classFuncTemplate->SetClassName(String::NewSymbol(classDef->className));
			instance->SetInternalFieldCount(0);
			for( const JS4D::StaticFunction *j = classDef->staticFunctions ; j->name != NULL ; ++j)
			{
				DebugMsg("V4DContext::BuildConstructor treat static function %S for %S\n",j->name,&inClassName);
			
				Local<FunctionTemplate> functionTemplate = FunctionTemplate::New(j->callAsFunction, v8::Undefined());
				functionTemplate->RemovePrototype();
				classFuncTemplate->PrototypeTemplate()->Set(
					v8::String::NewSymbol(j->name), functionTemplate, static_cast<v8::PropertyAttribute>(v8::DontDelete));

			}
		}
		if (classDef->staticValues)
		{
			for( const JS4D::StaticValue *j = classDef->staticValues ; j->name != NULL ; ++j)
			{
				DebugMsg("V4DContext::BuildConstructor treat static value %s for %S\n",j->name,&inClassName);
				JS4D::ObjectGetPropertyCallback cbkG = j->getProperty;
				JS4D::ObjectSetPropertyCallback cbkS = j->setProperty;
				if (testAssert(cbkG || cbkS))
				{
					v8::AccessorGetterCallback accGetterCbk = NULL;
					v8::AccessorSetterCallback accSetterCbk = NULL;
					if (cbkG)
					{
						accGetterCbk = (v8::AccessorGetterCallback)VJSClass__GetV8PropertyGetterCallback(cbkG());
					}
					if (cbkS)
					{
						accSetterCbk = (v8::AccessorSetterCallback)VJSClass__GetV8PropertySetterCallback(cbkS());
					}
					void*						accHandler = VJSClass__GetV8PropertyHandlerCallback(cbkG,cbkS);
					Handle<External>			extPtr = External::New(accHandler);
					instance->SetAccessor(	v8::String::NewSymbol(j->name),
											accGetterCbk,
											accSetterCbk,
											extPtr,
											v8::DEFAULT);
				}
			}
		}

		classFuncTemplate->SetLength(0);
		DebugMsg("V4DContext::V4DContext treat static constructor %S\n",&inClassName);
		ConstructorCallbacks*	cbks = NULL;
		std::map< xbox::VString, ConstructorCallbacks* >::iterator	itCbks = sClassesCallbacks.find(inClassName);
		if ( itCbks != sClassesCallbacks.end() )
		{
			cbks = (*itCbks).second;
			cbks->Update(&(*itClass).second);
		}
		if (!cbks)
		{
			cbks = new ConstructorCallbacks(&(*itClass).second);
			sClassesCallbacks.insert( std::pair< xbox::VString, ConstructorCallbacks* >( inClassName, cbks ) );
		}

		Handle<External> extPtr = External::New((void*)cbks);
		classFuncTemplate->SetCallHandler(V4DContext::ConstrCbk,extPtr);
	}
	else
	{
		DebugMsg("V4DContext::BuildConstructor UNDECLARED class %S\n",&inClassName);
	}
}
*/
v8::Persistent<v8::FunctionTemplate>*	V4DContext::GetImplicitFunctionConstructor(const xbox::VString& inName)
{
	Persistent<FunctionTemplate>*	result = NULL;
	sLock.Lock();
	
	std::map< xbox::VString , Persistent<FunctionTemplate>* >::iterator	itIFT = fPersImplicitFunctionConstructorMap.find(inName);
	if (itIFT != fPersImplicitFunctionConstructorMap.end() )
	{
		result = (*itIFT).second;
	}
	else
	{
		std::map< xbox::VString, xbox::JS4D::ClassDefinition >::iterator	itClass = sNativeClassesMap.find(inName);
		if (itClass != sNativeClassesMap.end())
		{
			HandleScope handle_scope(fIsolate);
			Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
			Handle<Object> global = context->Global();

			JS4D::ClassDefinition* clRef = (JS4D::ClassDefinition*)&((*itClass).second);
			Handle<Function> newFunc = V4DContext::CreateFunctionTemplateForClass( clRef, clRef->staticFunctions,false, true );
			//global->Set(String::NewSymbol(clRef->className),newFunc);
			fPersistentCtx.Reset(fIsolate,context);
			itIFT = fPersImplicitFunctionConstructorMap.find(inName);
			if (itIFT != fPersImplicitFunctionConstructorMap.end() )
			{
				result = (*itIFT).second;
			}
		}
		else
		{
			DebugMsg("V4DContext::GetImplicitFunctionConstructor %S NOT FOUND\n",&inName);
		}
	}
	sLock.Unlock();
	return result;	
}
void* V4DContext::GetGlobalObjectPrivateInstance()
{
	void*	prvData = NULL;
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object> global = context->Global();

	Local<Value>	extVal = global->GetHiddenValue(String::New(K_WAK_PRIVATE_DATA_ID));
	if (extVal->IsExternal())
	{
		Handle<External> extHdl = Handle<External>::Cast(extVal);
		prvData = extHdl->Value();
	}
	return prvData;
}

void V4DContext::SetGlobalObjectPrivateInstance(void* inPrivateData)
{
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	Handle<Object> global = context->Global();
	Handle<External> extPtr = External::New((void*)inPrivateData);
	testAssert(global->SetHiddenValue(String::New(K_WAK_PRIVATE_DATA_ID),extPtr));
	fPersistentCtx.Reset(fIsolate,context);
}

bool V4DContext::CheckScriptSyntax(const XBOX::VString& inScript, const VString& inUrl)
{
	bool	ok=false;
	DebugMsg("V4DContext::CheckScriptSyntax <%S> ....\n",&inUrl);

	VStringConvertBuffer	bufferTmp(inScript,VTC_UTF_8);
	HandleScope handle_scope(fIsolate);
	Local<Context> locCtx = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(locCtx);
	Handle<String> scriptStr =
		String::New(bufferTmp.GetCPointer(), bufferTmp.GetSize());
	TryCatch try_catch;
	Handle<Script> compiled_script = Script::Compile(scriptStr);
	if (compiled_script.IsEmpty()) {
		String::Utf8Value error(try_catch.Exception());
		DebugMsg("V4DContext::CheckScriptSyntax error -> '%s'  on script '%S'\n",*error,&inUrl);
	}
	else
	{
		ok = true;
	}
	return ok;
}


bool V4DContext::EvaluateScript(const VString& inScript, const VString& inUrl,  VJSValue *outResult)
{
	bool	ok=false;
	DebugMsg("V4DContext::EvaluateScript <%S> ....\n",&inUrl);
	//DebugMsg("V4DContext::EvaluateScript <%S> ....\n",&inScript);
	if (!fIsOK)
	{
		//UpdateContext();
	}
	VStringConvertBuffer	bufferTmp(inScript,VTC_UTF_8);
	HandleScope handle_scope(fIsolate);
	Local<Context> locCtx = Handle<Context>::New(fIsolate,fPersistentCtx);
	Context::Scope context_scope(locCtx);
	Handle<String> scriptStr =
		String::New(bufferTmp.GetCPointer(), bufferTmp.GetSize());
	TryCatch try_catch;
	Handle<Script> compiled_script = Script::Compile(scriptStr);
	if (compiled_script.IsEmpty()) {
		String::Utf8Value error(try_catch.Exception());
		DebugMsg("V4DContext::EvaluateScript error -> '%s'  on script '%S'\n",*error,&inUrl);
	}
	else
	{
		Handle<Value> result = compiled_script->Run();
		if (result.IsEmpty()) {
			String::Utf8Value error(try_catch.Exception());
			DebugMsg("V4DContext::EvaluateScript exec error on script '%S' -> %s \n",&inUrl,*error);
			Local<Value>	st = try_catch.StackTrace();
			if (!st.IsEmpty())
			{
				String::Utf8Value stValue(st);
				DebugMsg("V4DContext::EvaluateScript Stack -> <%s>\n",*stValue);			
			}
		}
		else
		{
			//Local<v8::Boolean>	bres = result->ToBoolean();
			//DebugMsg("V4DContext::EvaluateScript result is bool=%d val=%d\n",result->IsBoolean(),bres->Value());
			if (outResult)
			{
				*outResult = VJSValue(fIsolate,result.val_);
			}
			ok = true;
		}
	}
	return ok;
}


void V4DContext::AddNativeClass( xbox::JS4D::ClassDefinition* inClassDefinition )
{
//static const char*  sClassList[] = { "Blob", "DynamicObject", "Storage", "HttpRequest","HttpResponse", "Application", "Datastore", NULL };
DebugMsg("V4DContext::AddNativeClass called for '%s' !!!\n",inClassDefinition->className);

	sLock.Lock();
	//for (const char** tmp = sClassList; *tmp; ++tmp)
	{
		//if (!strcmp(inClassDefinition->className,*tmp))
		{
			std::map< xbox::VString, xbox::JS4D::ClassDefinition >::iterator	itClass = sNativeClassesMap.find(inClassDefinition->className);
			if (itClass == sNativeClassesMap.end())
			{
				std::pair< xbox::VString, xbox::JS4D::ClassDefinition >	newClass =
					std::pair< xbox::VString, xbox::JS4D::ClassDefinition >(inClassDefinition->className,*inClassDefinition);
				sNativeClassesMap.insert( newClass );
				//break;
			}
		}
	}

	std::map< xbox::VString, ConstructorCallbacks* >::iterator	itCbks = sClassesCallbacks.find(inClassDefinition->className);
	if (itCbks == sClassesCallbacks.end())
	{
		ConstructorCallbacks* cbks = new ConstructorCallbacks(inClassDefinition);
		sClassesCallbacks.insert( std::pair< xbox::VString, ConstructorCallbacks* >( inClassDefinition->className, cbks ) );
	}
	sLock.Unlock();
}

void V4DContext::AddNativeConstructorForClass( const char *inClassName, JS4D::ObjectCallAsConstructorCallback inCallback )
{
	DebugMsg("V4DContext::AddNativeConstructorForClass called for class %s\n",inClassName);
	sLock.Lock();
	std::map< xbox::VString, ConstructorCallbacks* >::iterator	itCbks = sClassesCallbacks.find(inClassName);
	if (testAssert(itCbks != sClassesCallbacks.end()))
	{
		if (testAssert( (*itCbks).second->HasCallbacks() == false ))
		{
			(*itCbks).second->fObjectCallAsConstructorCallback = inCallback;
		}
	}
	sLock.Unlock();
}


void* V4DContext::GetObjectPrivateData(v8::Value* inObject)
{
	void*	result = NULL;
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	v8::Local<v8::Object>	locObj = inObject->ToObject();
  	Local<Value>	extVal = locObj->GetHiddenValue(String::NewSymbol(K_WAK_PRIVATE_DATA_ID));
	if (extVal.val_)
	{
		if (!extVal->IsNull() && testAssert(extVal->IsExternal()))
		{
			External* 	ext = External::Cast(extVal.val_);
			result = (ext ? ext->Value() : NULL);
		}
	}
//DebugMsg("V4DContext::GetObjectPrivateData called prvData=%x!!!\n",result);
	return result;
}

void V4DContext::SetObjectPrivateData(v8::Value* inObject, void* inPrivateData)
{
//DebugMsg("V4DContext::SetObjectPrivateData called !!!\n");
	HandleScope handle_scope(fIsolate);
	Handle<Context> context = Handle<Context>::New(fIsolate,fPersistentCtx);
	v8::Local<v8::Object>	locObj = inObject->ToObject();
	Handle<External> extPtr = External::New((void*)inPrivateData);
	testAssert(locObj->SetHiddenValue(String::New(K_WAK_PRIVATE_DATA_ID),extPtr));
	fPersistentCtx.Reset(fIsolate,context);
	
}


v8::Persistent <v8::Context,v8::NonCopyablePersistentTraits<v8::Context> >&		V4DContext::GetPersistentContext(v8::Isolate* inIsolate)
{
	V4DContext* v8Ctx = (V4DContext*)inIsolate->GetData();
	xbox_assert(inIsolate == v8::Isolate::GetCurrent());
	return v8Ctx->fPersistentCtx;
}
void* V4DContext::GetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject)
{
	V4DContext* v8Ctx = (V4DContext*)inIsolate->GetData();
	return v8Ctx->GetObjectPrivateData(inObject);

}

void V4DContext::SetObjectPrivateData(v8::Isolate* inIsolate,v8::Value* inObject, void* inPrivateData)
{
	V4DContext* v8Ctx = (V4DContext*)inIsolate->GetData();
	return v8Ctx->SetObjectPrivateData(inObject,inPrivateData);
}

#endif

