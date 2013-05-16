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
#ifndef __VJSErrors__
#define __VJSErrors__


BEGIN_TOOLBOX_NAMESPACE


const OsType	kJAVASCRIPT_SIGNATURE	= 'jvsc';

// Generic errors.

const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_FILE		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4000);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_STRING		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4001);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_BOOLEAN	= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4002);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_NUMBER		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4003);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_FOLDER		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4004);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_ATTRIBUTE	= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4005);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_FUNCTION	= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4006);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_OBJECT		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4007);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_BUFFER		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4008);
const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_ARRAY		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4009);
const XBOX::VError	VE_JVSC_EXPECTING_PARAMETER				= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4010);

// If multiple types of data are possible, but the argument given doesn't match any.

const XBOX::VError	VE_JVSC_WRONG_PARAMETER					= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4011);

const XBOX::VError	VE_JVSC_WRONG_PARAMETER_TYPE_DATACLASS	= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4012);

// Try to create a File or Folder object but the path is not toward a file or folder.

const XBOX::VError	VE_JVSC_NOT_A_FILE						= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4013);
const XBOX::VError	VE_JVSC_NOT_A_FOLDER					= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4014);

// Web workers and ImportScripts()

const XBOX::VError	VE_JVSC_SYNTAX_ERROR					= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4020);
const XBOX::VError	VE_JVSC_SCRIPT_NOT_FOUND				= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4021);
const XBOX::VError	VE_JVSC_DATA_CLONE_ERROR				= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4022);

// Invalid state error if you try to call a function you are not supposed to call.
// For example, a TextStream is opened in read mode and you try to write in it. Or you try to read an 
// already closed TextStream.

const XBOX::VError	VE_JVSC_INVALID_STATE					= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4050);

// Trying to call an unsupported function.

const XBOX::VError	VE_JVSC_UNSUPPORTED_FUNCTION			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4051);

// Trying to call a function with an unsupported argument.

const XBOX::VError	VE_JVSC_UNSUPPORTED_ARGUMENT			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4052);

// Number given as argument is invalid.

const XBOX::VError	VE_JVSC_WRONG_NUMBER_ARGUMENT			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4053);

// It is forbidden to do this operation.

const XBOX::VError	VE_JVSC_FORBIDDEN						= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4054);

// Buffer errors:

// Buffer failed encoding.

const XBOX::VError	VE_JVSC_BUFFER_ENCODING_FAILED			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4100);

// Out of bound of the buffer.

const XBOX::VError	VE_JVSC_BUFFER_OUT_OF_BOUND				= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4101);

// Unknown or unsupported encoding type.

const XBOX::VError	VE_JVSC_BUFFER_UNSUPPORTED_ENCODING		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4102);

// ArrayBuffer errors:

// Undelying ArrayBuffer has been "neutered".

const XBOX::VError	VE_JVSC_ARRAY_BUFFER_IS_NEUTERED		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4120);

// Out of bound of the TypedArray.

const XBOX::VError	VE_JVSC_TYPED_ARRAY_OUT_OF_BOUND		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4121);

// Trying to set a TypedArray with an Array or TypedArray too big.

const XBOX::VError	VE_JVSC_TYPED_ARRAY_TOO_SMALL			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4122);

// Socket errors:

// Socket is already connected.

const XBOX::VError	VE_JVSC_ALREADY_CONNECTED				= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4200);

// Expecting SSL key and certificate.

const XBOX::VError	VE_JVSC_EXPECTING_SSL_INFO				= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4201);

// W3C File System errors:

// File system type must be TEMPORARY or PERSISTENT.

const XBOX::VError	VE_JVSC_FS_WRONG_TYPE					= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4300);

// Expecting DirectoryEntry or DirectoryEntrySync for moveTo() or copyTo() functions.

const XBOX::VError	VE_JVSC_FS_EXPECTING_DIR_ENTRY			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4301);
const XBOX::VError	VE_JVSC_FS_EXPECTING_DIR_ENTRY_SYNC		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4302);

// Optional new name for a moveTo() or copyTo() cannot be an empty string.

const XBOX::VError	VE_JVSC_FS_NEW_NAME_IS_EMTPY			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4303);

//DECLARE_VERROR( kJAVASCRIPT_SIGNATURE, 1, VE_UNKNOWN_ERROR)

// BinaryStream only supports net.SocketSync objects.

const XBOX::VError	VE_JVSC_BINARY_STREAM_SOCKET			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4400);

// VJSModule, source URL retrieval.

const XBOX::VError	VE_JVSC_UNABLE_TO_RETRIEVE_URL			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4500);
const XBOX::VError	VE_JVSC_MODULE_INTERNAL_ERROR			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 4501);


// Extension message
const XBOX::VError	VE_JVSC_EXTENSION_ARG_MISSING			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 5121);
const XBOX::VError	VE_JVSC_EXTENSION_PATH_INVALID			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 5122);
const XBOX::VError	VE_JVSC_EXTENSION_FILE_NO_EXIST			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 5123);
const XBOX::VError	VE_JVSC_EXTENSION_SOLUTION_MISSING		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 5124);
const XBOX::VError	VE_JVSC_EXTENSION_WRONG_ARG_NUMBER		= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 5125);
const XBOX::VError	VE_JVSC_EXTENSION_ARG_MISSMATCH			= MAKE_VERROR(kJAVASCRIPT_SIGNATURE, 5126);

END_TOOLBOX_NAMESPACE

#endif