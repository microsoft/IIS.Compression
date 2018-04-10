// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// Brotli Header Files
#include "port.h"
#include "types.h"
#include "encode.h"

// IIS Common Header Files
// VS Debug build only automatically defines _DEBUG, but debug utilities require DEBUG
#ifdef _DEBUG
    #ifndef DEBUG
        #define DEBUG
    #endif
#endif
#include "dbgutil.h"

#define DLLEXP extern "C" __declspec(dllexport)

// Constants
#define COMPRESSION_OPERATION_PROCESS   0
#define COMPRESSION_OPERATION_FLUSH     1
#define COMPRESSION_OPERATION_FINISH    2

#define BROTLI_PARAMETER_UNSET          -1

// Global variables
extern INT                              g_intEncoderOp;
