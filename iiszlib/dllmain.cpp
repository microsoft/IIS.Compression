// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

DECLARE_DEBUG_PRINT_OBJECT2("iiszlib", DEBUG_FLAGS_ERROR);

INT         g_intWindowBits = ZLIB_PARAMETER_UNSET;
INT         g_intMemLevel   = ZLIB_PARAMETER_UNSET;
INT         g_intFlushMode  = ZLIB_PARAMETER_UNSET;
BOOL        g_fEnableZlibDeflate = FALSE;

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
                     L"SOFTWARE\\Microsoft\\IIS Extensions\\IIS Compression\\IISZlib\\Parameters",
                     0,
                     KEY_READ,
                     &hKey) == ERROR_SUCCESS)
    {
        cbData = sizeof(dwData);
        if (RegQueryValueEx(hKey,
                            L"WindowBits",
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &cbData) == ERROR_SUCCESS &&
            dwType == REG_DWORD)
        {
            g_intWindowBits = (INT) dwData;
        }

        cbData = sizeof(dwData);
        if (RegQueryValueEx(hKey,
                            L"MemLevel",
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &cbData) == ERROR_SUCCESS &&
            dwType == REG_DWORD)
        {
            g_intMemLevel = (INT) dwData;
        }

        cbData = sizeof(dwData);
        if (RegQueryValueEx(hKey,
                            L"FlushMode",
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &cbData) == ERROR_SUCCESS &&
            dwType == REG_DWORD)
        {
            g_intFlushMode = (INT) dwData;
        }

        cbData = sizeof(dwData);
        if (RegQueryValueEx(hKey,
                            L"EnableZlibDeflate",
                            NULL,
                            &dwType,
                            (LPBYTE) &dwData,
                            &cbData) == ERROR_SUCCESS &&
            dwType == REG_DWORD)
        {
            g_fEnableZlibDeflate = !!dwData;
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
