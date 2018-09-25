// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "stdafx.h"

class ZLIB_CONTEXT
{
public:

    ZLIB_CONTEXT()
        : _fInitialized(FALSE),
          _fGzip(FALSE),
          _windowBits(ZLIB_DEF_WINDOW_BITS),
          _memLevel(ZLIB_DEF_MEM_LEVEL),
          _flushMode(Z_NO_FLUSH)
    {
        _strm.zalloc = Z_NULL;
        _strm.zfree  = Z_NULL;
        _strm.opaque = Z_NULL;

        _strm.next_in = Z_NULL;
        _strm.avail_in = 0;
        _strm.total_in = 0;

        _strm.next_out = Z_NULL;
        _strm.avail_out = 0;
        _strm.total_out = 0;
    }

    BOOL        _fInitialized;
    BOOL        _fGzip;

    // zlib parameters
    z_stream    _strm;
    INT         _windowBits;
    INT         _memLevel;
    INT         _flushMode;
};


HRESULT HresultFromZlib(INT zlibErrCode);
INT ConvertFlushMode(INT operation);

//
// Initialize global compression 
//
HRESULT WINAPI
InitCompression(
    VOID
)
{
    return S_OK;
}

//
// De-init global compression
//
VOID WINAPI
DeInitCompression(
    VOID
)
{
    return;
}

//
// Reset compression context
//
HRESULT WINAPI
ResetCompression(
    IN OUT PVOID context
)
{
    return S_OK;
}

//
// Create a compression context
//
HRESULT WINAPI
CreateCompression(
    OUT     PVOID *             context,
    IN      ULONG               flags
)
{
    HRESULT         hr = S_OK;
    ZLIB_CONTEXT*   pContext = NULL;

    if (flags == COMPRESSION_FLAG_INVALID)
    {
        DBGERROR((DBG_CONTEXT,
                  "Invalid flags\n"));
        hr = E_INVALIDARG;
        goto Finished;
    }

    pContext = new ZLIB_CONTEXT();
    if (pContext == NULL)
    {
        DBGERROR((DBG_CONTEXT,
                  "Failed to create ZLIB_CONTEXT\n"));
        hr = E_OUTOFMEMORY;
        goto Finished;
    }

    // window bits
    if (g_intWindowBits != ZLIB_PARAMETER_UNSET)
    {
        pContext->_windowBits = g_intWindowBits;
    }

    // mem level
    if (g_intMemLevel != ZLIB_PARAMETER_UNSET)
    {
        pContext->_memLevel = g_intMemLevel;
    }

    // flush mode
    if (g_intFlushMode != ZLIB_PARAMETER_UNSET)
    {
        pContext->_flushMode = g_intFlushMode;
    }

    // compressed data format
    if (flags & COMPRESSION_FLAG_GZIP)
    {
        // gzip content-encoding:
        // Add 16 to windowBits to ask zlib to write gzip format header & trailer.
        // By default zlib write zlib format header & trailer used by deflate content encoding.
        pContext->_fGzip = TRUE;
        pContext->_windowBits |= 16;
    }
    else
    {
        // deflate content-encoding:
        if (g_fEnableZlibDeflate == FALSE)
        {
            // default mode: raw deflate for IE compatibility
            // Flip the sign of windowBits to force zlib to produce raw deflate
            // without zlib header and checksum
            pContext->_windowBits = - pContext->_windowBits;
        }
    }

    DBGINFO((DBG_CONTEXT,
             "ZLIB_CONTEXT initialized\n"
             "- ptr: %p\n"
             "- scheme: %s\n"
             "- windowBits: %d\n"
             "- memLevel: %d\n"
             "- flush mode: %d\n",
             pContext,
             pContext->_fGzip ? "gzip" : "deflate",
             pContext->_windowBits,
             pContext->_memLevel,
             pContext->_flushMode));

Finished:

    *context = (PVOID)pContext;
    return hr;
}

//
// Destroy a compression context
//
VOID WINAPI
DestroyCompression(
    IN PVOID context
)
{
    ZLIB_CONTEXT* pContext = static_cast<ZLIB_CONTEXT *>(context);

    if (pContext != NULL)
    {
        if (pContext->_fInitialized == TRUE)
        {
            deflateEnd(&pContext->_strm);
        }
        delete pContext;
    }
}

//
// Compress data
//
HRESULT WINAPI
Compress(
    IN OUT  PVOID               context,
    IN      CONST BYTE *        input_buffer,
    IN      LONG                input_buffer_size,
    IN      PBYTE               output_buffer,
    IN      LONG                output_buffer_size,
    OUT     PLONG               input_used,
    OUT     PLONG               output_used,
    IN      INT                 compression_level
)
{
    ZLIB_CONTEXT*   pContext = static_cast<ZLIB_CONTEXT *>(context);
    HRESULT         hr = S_OK;
    INT             ret = Z_OK;
    INT             flushMode = input_buffer_size ? pContext->_flushMode : Z_FINISH;

    if (pContext == NULL)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    // IIS schema specifies staticCompressionLevel and dynamicCompressionLevel as uint,
    // so only the upper bound needs to be checked.
    if (compression_level > Z_BEST_COMPRESSION)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    if (pContext->_fInitialized == FALSE)
    {
        ret = deflateInit2(&pContext->_strm,            // strm
                           compression_level,           // level
                           Z_DEFLATED,                  // method
                           pContext->_windowBits,       // windowBits
                           pContext->_memLevel,         // memLevel
                           Z_DEFAULT_STRATEGY);         // strategy

        if (FAILED(hr = HresultFromZlib(ret)))
        {
            DBGERROR((DBG_CONTEXT,
                      "deflateInit2 failed: %d\n",
                      ret));
            deflateEnd(&pContext->_strm);
            goto Finished;
        }

        pContext->_fInitialized = TRUE;
    }

    pContext->_strm.next_in = (z_const Bytef *) input_buffer;
    pContext->_strm.avail_in = input_buffer_size;
    pContext->_strm.next_out = output_buffer;
    pContext->_strm.avail_out = output_buffer_size;

    DBGINFO((DBG_CONTEXT,
             "Start deflate\n"
             "- input buffer: %p\n"
             "- input buffer size: %d\n"
             "- output buffer: %p\n"
             "- output buffer size: %d\n"
             "- compression level: %d\n"
             "- flush mode: %d\n",
             input_buffer,
             input_buffer_size,
             output_buffer,
             output_buffer_size,
             compression_level,
             flushMode));

    ret = deflate(&pContext->_strm, flushMode);

    if (ret == Z_OK || ret == Z_BUF_ERROR)
    {
        DBGINFO((DBG_CONTEXT, "Need to call Compress again\n"));
        hr = S_OK;
    }
    else if (ret == Z_STREAM_END)
    {
        DBGINFO((DBG_CONTEXT, "End of stream\n"));
        hr = S_FALSE;
    }
    else
    {
        hr = HresultFromZlib(ret);
        DBGERROR((DBG_CONTEXT,
                  "deflate failed: %d\n",
                  ret));
        goto Finished;
    }

    *input_used = input_buffer_size - pContext->_strm.avail_in;
    *output_used = output_buffer_size - pContext->_strm.avail_out;

    DBGINFO((DBG_CONTEXT,
             "End deflate\n"
             "- input bytes consumed: %d\n"
             "- output bytes produced: %d\n",
             *input_used,
             *output_used));

Finished:

    return hr;
}

//
// Compress data with a specified flush mode
//
HRESULT WINAPI
Compress2(
    IN OUT  PVOID               context,
    IN      CONST BYTE *        input_buffer,
    IN      LONG                input_buffer_size,
    IN      PBYTE               output_buffer,
    IN      LONG                output_buffer_size,
    OUT     PLONG               input_used,
    OUT     PLONG               output_used,
    IN      INT                 compression_level,
    IN      INT                 operation
)
{
    ZLIB_CONTEXT*   pContext = static_cast<ZLIB_CONTEXT *>(context);
    HRESULT         hr = S_OK;
    INT             ret = Z_OK;
    INT             flushMode = ConvertFlushMode(operation);

    if (pContext == NULL)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    // IIS schema specifies staticCompressionLevel and dynamicCompressionLevel as uint,
    // so only the upper bound needs to be checked.
    if (compression_level > Z_BEST_COMPRESSION)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    if (pContext->_fInitialized == FALSE)
    {
        ret = deflateInit2(&pContext->_strm,            // strm
                           compression_level,           // level
                           Z_DEFLATED,                  // method
                           pContext->_windowBits,       // windowBits
                           pContext->_memLevel,         // memLevel
                           Z_DEFAULT_STRATEGY);         // strategy

        if (FAILED(hr = HresultFromZlib(ret)))
        {
            DBGERROR((DBG_CONTEXT,
                      "deflateInit2 failed: %d\n",
                      ret));
            deflateEnd(&pContext->_strm);
            goto Finished;
        }

        pContext->_fInitialized = TRUE;
    }

    pContext->_strm.next_in = (z_const Bytef *) input_buffer;
    pContext->_strm.avail_in = input_buffer_size;
    pContext->_strm.next_out = output_buffer;
    pContext->_strm.avail_out = output_buffer_size;

    DBGINFO((DBG_CONTEXT,
             "Start deflate\n"
             "- input buffer: %p\n"
             "- input buffer size: %d\n"
             "- output buffer: %p\n"
             "- output buffer size: %d\n"
             "- compression level: %d\n"
             "- flush mode: %d\n",
             input_buffer,
             input_buffer_size,
             output_buffer,
             output_buffer_size,
             compression_level,
             flushMode));

    ret = deflate(&pContext->_strm, flushMode);

    if (ret == Z_OK)
    {
        if (flushMode == Z_SYNC_FLUSH &&
            input_buffer_size == 0 &&
            pContext->_strm.avail_out > 0)      // sufficient output buffer size
        {
            DBGINFO((DBG_CONTEXT,"End of flush\n"));
            hr = S_FALSE;
        }
        else
        {
            DBGINFO((DBG_CONTEXT, "Need to call Compress2 again\n"));
            hr = S_OK;
        }
    }
    else if (ret == Z_BUF_ERROR)
    {
        DBGINFO((DBG_CONTEXT, "Need to call Compress2 again\n"));
        hr = S_OK;
    }
    else if (ret == Z_STREAM_END)
    {
        DBGINFO((DBG_CONTEXT, "End of stream\n"));
        hr = S_FALSE;
    }
    else
    {
        hr = HresultFromZlib(ret);
        DBGERROR((DBG_CONTEXT,
                  "deflate failed: %d\n",
                  ret));
        goto Finished;
    }

    *input_used = input_buffer_size - pContext->_strm.avail_in;
    *output_used = output_buffer_size - pContext->_strm.avail_out;

    DBGINFO((DBG_CONTEXT,
             "End deflate\n"
             "- input bytes consumed: %d\n"
             "- output bytes produced: %d\n",
             *input_used,
             *output_used));

Finished:

    return hr;
}

HRESULT
HresultFromZlib(
    INT zlibErrCode
)
{
    HRESULT hr = S_OK;

    switch (zlibErrCode)
    {
    case Z_OK:
    case Z_BUF_ERROR:   // output buffer full, not a real error
        hr = S_OK;
        break;
    case Z_STREAM_END:  // all input consumed, all output generated
        hr = S_FALSE;   // S_FALSE is required by IIS to indicate final block done
        break;
    case Z_MEM_ERROR:
        hr = E_OUTOFMEMORY;
        break;
    case Z_DATA_ERROR:
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        break;
    case Z_VERSION_ERROR:
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DLL);
        break;
    case Z_STREAM_ERROR:
        hr = E_INVALIDARG;
        break;
    case Z_ERRNO:
    default:
        hr = HRESULT_FROM_WIN32(zlibErrCode);
        break;
    }

    return hr;
}

INT
ConvertFlushMode(
    INT operation
)
{
    INT flushMode = Z_NO_FLUSH;

    if (g_intFlushMode != ZLIB_PARAMETER_UNSET)
    {
        flushMode = g_intFlushMode;
    }

    switch (operation)
    {
    case IIS_COMPRESSION_OPERATION_PROCESS:
        break;
    case IIS_COMPRESSION_OPERATION_FLUSH:
        flushMode = Z_SYNC_FLUSH;
        break;
    case IIS_COMPRESSION_OPERATION_FINISH:
        flushMode = Z_FINISH;
        break;
    default:
        break;
    }

    return flushMode;
}