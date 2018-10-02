// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "stdafx.h"

DECLARE_DEBUG_PRINT_OBJECT2("iisbrotli", DEBUG_FLAGS_ERROR);

INT g_intEncoderOp = BROTLI_PARAMETER_UNSET;
HANDLE g_hEventLog = NULL;
BOOL g_fEventRaised = FALSE;

VOID LoadGlobalConfiguration(VOID);

BOOL APIENTRY
DllMain(
    HMODULE hModule,
    DWORD   dwReason,
    LPVOID  lpReserved
)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
        LoadGlobalConfiguration();
        CREATE_DEBUG_PRINT_OBJECT;
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

VOID
LoadGlobalConfiguration(
    VOID
)
{
    DWORD dwType = 0;
    DWORD dwData = 0;
    DWORD cbData = 0;
    HKEY  hKey;

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     L"SOFTWARE\\Microsoft\\IIS Extensions\\IIS Compression\\IISBrotli\\Parameters",
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        cbData = sizeof(dwData);
        if (RegQueryValueEx(hKey,
                            L"FlushMode",
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &cbData) == ERROR_SUCCESS &&
            dwType == REG_DWORD)
        {
            if (dwData <= 3)
            {
                // BrotliEncoderOperation enum only supports four enumerators
                // 0: BROTLI_OPERATION_PROCESS
                // 1: BROTLI_OPERATION_FLUSH
                // 2: BROTLI_OPERATION_FINISH
                // 3: BROTLI_OPERATION_EMIT_METADATA
                g_intEncoderOp = (INT)dwData;
            }
        }

        cbData = sizeof(dwData);
        if (RegQueryValueEx(hKey,
                            L"DebugFlags",
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &cbData) == ERROR_SUCCESS &&
            dwType == REG_DWORD)
        {
            DEBUG_FLAGS_VAR = dwData;
        }

        RegCloseKey(hKey);
    }
}
