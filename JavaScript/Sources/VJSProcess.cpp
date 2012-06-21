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
#include "VJavaScriptPrecompiled.h"

#include "VJSProcess.h"

#include "VJSContext.h"
#include "VJSGlobalClass.h"


USING_TOOLBOX_NAMESPACE


void VJSProcess::Initialize ( const XBOX::VJSParms_initialize& inParms, void* )
{
	;
}

void VJSProcess::Finalize ( const XBOX::VJSParms_finalize& inParms, void* )
{
	;
}

void VJSProcess::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] =
	{
		{ "version", js_getProperty<_Version>, NULL, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "buildNumber", js_getProperty<_BuildNumber>, NULL, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "process";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}

void VJSProcess::_Version ( XBOX::VJSParms_getProperty& ioParms, void* )
{
	XBOX::VString		vstrVersion;
	XBOX::VProcess::Get ( )-> GetProductVersion ( vstrVersion );

	ioParms. ReturnString ( vstrVersion );
}

void VJSProcess::_BuildNumber ( XBOX::VJSParms_getProperty& ioParms, void* )
{
	VString s;
	VProcess::Get()->GetBuildNumber( s);
	ioParms.ReturnString( s);
}

void VJSProcess::_getAccessor( XBOX::VJSParms_getProperty& ioParms, XBOX::VJSGlobalObject* inGlobalObject)
{
	ioParms.ReturnValue( VJSProcess::CreateInstance( ioParms.GetContextRef(), NULL));
}


void VJSOS::Initialize ( const XBOX::VJSParms_initialize& inParms, void* )
{
	;
}

void VJSOS::Finalize ( const XBOX::VJSParms_finalize& inParms, void* )
{
	;
}

void VJSOS::GetDefinition( ClassDefinition& outDefinition)
{
	static inherited::StaticFunction functions[] =
	{
		{ "type", js_callStaticFunction<_type>, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0}
	};

	static inherited::StaticValue values[] =
	{
		{ "isWindows", js_getProperty<_IsWindows>, NULL, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "isMac", js_getProperty<_IsMac>, NULL, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ "isLinux", js_getProperty<_IsLinux>, NULL, JS4D::PropertyAttributeReadOnly | JS4D::PropertyAttributeDontDelete },
		{ 0, 0, 0, 0}
	};

	outDefinition.className = "os";
	outDefinition.initialize = js_initialize<Initialize>;
	outDefinition.finalize = js_finalize<Finalize>;
	outDefinition.staticFunctions = functions;
	outDefinition.staticValues = values;
}

void VJSOS::_type( XBOX::VJSParms_callStaticFunction& ioParms, void* )
{
	XBOX::VString		vstrOSVersion;
	XBOX::VSystem::GetOSVersionString ( vstrOSVersion );

	ioParms. ReturnString ( vstrOSVersion );
}

void VJSOS::_IsMac ( XBOX::VJSParms_getProperty& ioParms, void* )
{
#if VERSIONMAC
	ioParms. ReturnBool ( true );
#else
	ioParms. ReturnBool ( false );
#endif
}

void VJSOS::_IsWindows ( XBOX::VJSParms_getProperty& ioParms, void* )
{
#if VERSIONWIN
	ioParms. ReturnBool ( true );
#else
	ioParms. ReturnBool ( false );
#endif
}

void VJSOS::_IsLinux ( XBOX::VJSParms_getProperty& ioParms, void* )
{
#if VERSION_LINUX
	ioParms. ReturnBool ( true );
#else
	ioParms. ReturnBool ( false );
#endif
}
