// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <stdlib.h>

// zlib Header Files:
#include "zlib.h"

// IIS Common Header Files
// VS Debug build only automatically defines _DEBUG, but debug utilities require DEBUG
#ifdef _DEBUG
    #ifndef DEBUG
        #define DEBUG
    #endif
#endif
#include "dbgutil.h"
#include "iiszlib_msg.h"

// Constants
#define COMPRESSION_FLAG_DEFLATE            0x00000000
#define COMPRESSION_FLAG_GZIP               0x00000001
#define COMPRESSION_FLAG_INVALID            0xFFFFFFFF

#define IIS_COMPRESSION_OPERATION_PROCESS   0
#define IIS_COMPRESSION_OPERATION_FLUSH     1
#define IIS_COMPRESSION_OPERATION_FINISH    2

#define ZLIB_DEF_MEM_LEVEL                  8   // default memLevel
#define ZLIB_DEF_WINDOW_BITS                15  // default windowBits, 2^15 ~ 32K window size, RFC 1951
#define ZLIB_PARAMETER_UNSET                -1

#define COMPRESSION_LEVEL_BUFFER_LENGTH     33

#define IISZLIB_EVENT_SOURCE_NAME           L"IISZlib"

// Global variables
extern INT                              g_intWindowBits;
extern INT                              g_intMemLevel;
extern INT                              g_intFlushMode;
extern BOOL                             g_fEnableZlibDeflate;
extern HANDLE                           g_hEventLog;
extern BOOL                             g_fEventRaised;
