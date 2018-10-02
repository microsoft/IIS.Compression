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
#include "iisbrotli_msg.h"

// Constants
#define IIS_COMPRESSION_OPERATION_PROCESS   0
#define IIS_COMPRESSION_OPERATION_FLUSH     1
#define IIS_COMPRESSION_OPERATION_FINISH    2

#define BROTLI_PARAMETER_UNSET              -1

#define COMPRESSION_LEVEL_BUFFER_LENGTH     33

#define IISBROTLI_EVENT_SOURCE_NAME         L"IISBrotli"

// Global variables
extern INT                              g_intEncoderOp;
extern HANDLE                           g_hEventLog;
extern BOOL                             g_fEventRaised;
