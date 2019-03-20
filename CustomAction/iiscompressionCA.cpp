// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include <precomp.h>

DECLARE_DEBUG_PRINT_OBJECT( "iiscompressionCA.dll" );

HINSTANCE g_hinst;

BOOL WINAPI
DllMain(
    HINSTANCE   hModule,
    DWORD       dwReason,
    LPVOID      lpReserved
)
{
    UNREFERENCED_PARAMETER( lpReserved );
    switch( dwReason )
    {
        case DLL_PROCESS_ATTACH:
            CREATE_DEBUG_PRINT_OBJECT;
            DisableThreadLibraryCalls( hModule );
            g_hinst = hModule;
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

struct COMP_SCHEME
{
    PCWSTR  pszName;
    PCWSTR  pszDll;
};

// The installer inserts each scheme in g_compressionScheme to the beginning of
// the scheme collection. Therefore, the scheme order appearing in the configuration
// collection is opposite to their order in g_compressionScheme.
COMP_SCHEME g_compressionScheme[] =
    { { L"gzip", L"%ProgramFiles%\\IIS\\IIS Compression\\iiszlib.dll"   },
      { L"br"  , L"%ProgramFiles%\\IIS\\IIS Compression\\iisbrotli.dll" } };

COMP_SCHEME g_defaultGzipScheme =
    { L"gzip", L"%windir%\\system32\\inetsrv\\gzip.dll" };

/*++

Routine Description:

    Custom action for installation:

    Register the iiszlib (gzip) and iisbrotli (br) compression schemes

--*/
UINT
RegisterCompressionSchemesCA(
    IN  MSIHANDLE hInstall
)
{
    IAppHostWritableAdminManager *  pAdminMgr = NULL;
    IAppHostElement *               pHttpCompressionSection = NULL;
    IAppHostElement *               pNewSchemeElement = NULL;
    IAppHostElementCollection *     pSchemeCollection = NULL;
    HRESULT                         hr = NOERROR;
    BOOL                            fDeleted;
    VARIANT                         varPropValue;

    IISLogInitialize(hInstall, UNITEXT(__FUNCTION__));
    IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                L"CA '%s' started to register iiszlib and iisbrotli compression schemes.",
                UNITEXT(__FUNCTION__));

    VariantInit(&varPropValue);

    hr = CoCreateInstance(__uuidof(AppHostWritableAdminManager),
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IAppHostWritableAdminManager),
                          (VOID **)&pAdminMgr);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to create an AppHostWritableAdminManager: hr=0x%x.",
                    hr);
        goto exit;
    }

    hr = pAdminMgr->GetAdminSection(L"system.webServer/httpCompression",
                                    L"MACHINE/WEBROOT/APPHOST",
                                    &pHttpCompressionSection);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to get the httpCompression section: hr=0x%x.",
                    hr);
        goto exit;
    }

    hr = pHttpCompressionSection->get_Collection(&pSchemeCollection);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to get the scheme collection of the httpCompression section: hr=0x%x.",
                    hr);
        goto exit;
    }

    for (DWORD i = 0; i < _countof(g_compressionScheme); i++)
    {
        hr = DeleteElementFromCollection(pSchemeCollection,
                                         L"name",
                                         g_compressionScheme[i].pszName,
                                         FIND_ELEMENT_CASE_SENSITIVE,
                                         &fDeleted);
        if (FAILED(hr))
        {
            IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                        L"Failed to delete the original scheme: '%s', hr=0x%x.",
                        g_compressionScheme[i].pszName,
                        hr);
            goto exit;
        }
        if (fDeleted == TRUE)
        {
            IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                        L"Successfully deleted the original scheme: '%s'.",
                        g_compressionScheme[i].pszName);
        }

        hr = pSchemeCollection->CreateNewElement(L"scheme",
                                                 &pNewSchemeElement);
        if (FAILED(hr))
        {
            IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                        L"Failed to create a configuration element for the scheme: '%s', hr=0x%x.",
                        g_compressionScheme[i].pszName,
                        hr);
            goto exit;
        }

        hr = VariantAssign(&varPropValue,
                           g_compressionScheme[i].pszName);
        if (FAILED(hr))
        {
            goto exit;
        }

        hr = SetElementProperty(pNewSchemeElement,
                                L"name",
                                &varPropValue);
        if (FAILED(hr))
        {
            IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                        L"Failed to set the name property for the scheme: '%s', hr=0x%x.",
                        g_compressionScheme[i].pszName,
                        hr);
            goto exit;
        }

        hr = VariantAssign(&varPropValue,
                           g_compressionScheme[i].pszDll);
        if (FAILED(hr))
        {
            goto exit;
        }

        hr = SetElementProperty(pNewSchemeElement,
                                L"dll",
                                &varPropValue);
        if (FAILED(hr))
        {
            IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                        L"Failed to set the dll property for the scheme: '%s', hr=0x%x.",
                        g_compressionScheme[i].pszName,
                        hr);
            goto exit;
        }

        hr = pSchemeCollection->AddElement(pNewSchemeElement, 0);
        if (FAILED(hr))
        {
            IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                        L"Failed to add the configuration element for the scheme: '%s', hr=0x%x.",
                        g_compressionScheme[i].pszName,
                        hr);
            goto exit;
        }

        IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                    L"Successfully created the new scheme: '%s'.",
                    g_compressionScheme[i].pszName);
    }

    hr = pAdminMgr->CommitChanges();
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to commit the configuration changes: hr=0x%x.",
                    hr);
        goto exit;
    }

exit:

    IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                L"CA '%s' completed with return code hr=0x%x",
                UNITEXT(__FUNCTION__),
                hr);

    IISLogClose();
    VariantClear(&varPropValue);

    if (pNewSchemeElement != NULL)
    {
        pNewSchemeElement->Release();
        pNewSchemeElement = NULL;
    }

    if (pSchemeCollection != NULL)
    {
        pSchemeCollection->Release();
        pSchemeCollection = NULL;
    }

    if (pHttpCompressionSection != NULL)
    {
        pHttpCompressionSection->Release();
        pHttpCompressionSection = NULL;
    }

    if (pAdminMgr != NULL)
    {
        pAdminMgr->Release();
        pAdminMgr = NULL;
    }

    return (SUCCEEDED(hr)) ? ERROR_SUCCESS : ERROR_SUCCESS;
}

/*++

Routine Description:

    Custom action for uninstallation:

    Unregister the iiszlib (gzip) and iisbrotli (br) compression schemes
    and re-register the default in-box gzip compression scheme (gzip.dll)

--*/
UINT
CleanupCompressionSchemesCA(
    IN  MSIHANDLE hInstall
)
{
    IAppHostWritableAdminManager *  pAdminMgr = NULL;
    IAppHostElement *               pHttpCompressionSection = NULL;
    IAppHostElement *               pNewSchemeElement = NULL;
    IAppHostElementCollection *     pSchemeCollection = NULL;
    HRESULT                         hr = NOERROR;
    BOOL                            fDeleted;
    VARIANT                         varPropValue;

    IISLogInitialize(hInstall, UNITEXT(__FUNCTION__));
    IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                L"CA '%s' started to unregister iiszlib and iisbrotli compression schemes.",
                UNITEXT(__FUNCTION__));

    VariantInit(&varPropValue);

    hr = CoCreateInstance(__uuidof(AppHostWritableAdminManager),
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IAppHostWritableAdminManager),
                          (VOID **)&pAdminMgr);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to create an AppHostWritableAdminManager: hr=0x%x.",
                    hr);
        goto exit;
    }

    hr = pAdminMgr->GetAdminSection(L"system.webServer/httpCompression",
                                    L"MACHINE/WEBROOT/APPHOST",
                                    &pHttpCompressionSection);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to get the httpCompression section: hr=0x%x.",
                    hr);
        goto exit;
    }

    hr = pHttpCompressionSection->get_Collection(&pSchemeCollection);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to get the scheme collection of the httpCompression section: hr=0x%x.",
                    hr);
        goto exit;
    }

    for (DWORD i = 0; i < _countof(g_compressionScheme); i++)
    {
        hr = DeleteElementFromCollection(pSchemeCollection,
                                         L"name",
                                         g_compressionScheme[i].pszName,
                                         FIND_ELEMENT_CASE_SENSITIVE,
                                         &fDeleted);
        if (FAILED(hr))
        {
            IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                        L"Failed to delete the scheme: '%s', hr=0x%x.",
                        g_compressionScheme[i].pszName,
                        hr);
            goto exit;
        }
        if (fDeleted == TRUE)
        {
            IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                        L"Successfully deleted the scheme: '%s'.",
                        g_compressionScheme[i].pszName);
        }
    }

    hr = pSchemeCollection->CreateNewElement(L"scheme",
                                             &pNewSchemeElement);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to create a configuration element for the scheme: '%s', hr=0x%x.",
                    g_defaultGzipScheme.pszName,
                    hr);
        goto exit;
    }

    hr = VariantAssign(&varPropValue,
                       g_defaultGzipScheme.pszName);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = SetElementProperty(pNewSchemeElement,
                            L"name",
                            &varPropValue);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to set the name property for the scheme: '%s', hr=0x%x.",
                    g_defaultGzipScheme.pszName,
                    hr);
        goto exit;
    }

    hr = VariantAssign(&varPropValue,
                       g_defaultGzipScheme.pszDll);
    if (FAILED(hr))
    {
        goto exit;
    }

    hr = SetElementProperty(pNewSchemeElement,
                            L"dll",
                            &varPropValue);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to set the dll property for the scheme: '%s', hr=0x%x.",
                    g_defaultGzipScheme.pszName,
                    hr);
        goto exit;
    }

    hr = pSchemeCollection->AddElement(pNewSchemeElement, 0);
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to add the configuration element for the scheme: '%s', hr=0x%x.",
                    g_defaultGzipScheme.pszName,
                    hr);
        goto exit;
    }

    IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                L"Successfully recreated the default scheme: '%s'.",
                g_defaultGzipScheme.pszName);

    hr = pAdminMgr->CommitChanges();
    if (FAILED(hr))
    {
        IISLogWrite(SETUP_LOG_SEVERITY_ERROR,
                    L"Failed to commit the configuration changes: hr=0x%x.",
                    hr);
        goto exit;
    }

 exit:

    IISLogWrite(SETUP_LOG_SEVERITY_INFORMATION,
                L"CA '%s' completed with return code hr=0x%x",
                UNITEXT(__FUNCTION__),
                hr);

    IISLogClose();
    VariantClear(&varPropValue);

    if (pNewSchemeElement != NULL)
    {
        pNewSchemeElement->Release();
        pNewSchemeElement = NULL;
    }

    if (pSchemeCollection != NULL)
    {
        pSchemeCollection->Release();
        pSchemeCollection = NULL;
    }

    if (pHttpCompressionSection != NULL)
    {
        pHttpCompressionSection->Release();
        pHttpCompressionSection = NULL;
    }

    if (pAdminMgr != NULL)
    {
        pAdminMgr->Release();
        pAdminMgr = NULL;
    }

    return (SUCCEEDED(hr)) ? ERROR_SUCCESS : ERROR_SUCCESS;
}