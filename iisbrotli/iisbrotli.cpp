// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "stdafx.h"

class BROTLI_CONTEXT
{
public:

    BROTLI_CONTEXT()
        : _pState(NULL),
          _op(BROTLI_OPERATION_PROCESS)
    {
    }

    // brotli parameters
    BrotliEncoderState*     _pState;
    BrotliEncoderOperation  _op;
};

BrotliEncoderOperation ConvertFlushMode(INT operation);

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
    HRESULT             hr = S_OK;
    BROTLI_CONTEXT*     pContext = NULL;

    pContext = new BROTLI_CONTEXT();
    if (pContext == NULL)
    {
        DBGERROR((DBG_CONTEXT,
                  "Failed to create BROTLI_CONTEXT\n"));
        hr = E_OUTOFMEMORY;
        goto Finished;
    }

    // encoder operation
    if (g_intEncoderOp != BROTLI_PARAMETER_UNSET)
    {
        pContext->_op = static_cast<BrotliEncoderOperation>(g_intEncoderOp);
    }

    DBGINFO((DBG_CONTEXT,
             "BROTLI_CONTEXT initialized\n"
             "- ptr: %p\n"
             "- encoder operation: %d\n",
             pContext,
             pContext->_op));

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
    BROTLI_CONTEXT* pContext = static_cast<BROTLI_CONTEXT *>(context);

    if (pContext != NULL)
    {
        if (pContext->_pState != NULL)
        {
            BrotliEncoderDestroyInstance(pContext->_pState);
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
    BROTLI_CONTEXT*         pContext            = static_cast<BROTLI_CONTEXT *>(context);
    size_t                  input_available     = static_cast<size_t>(input_buffer_size);
    size_t                  output_available    = static_cast<size_t>(output_buffer_size);
    size_t                  total_out           = 0;
    BrotliEncoderOperation  flushMode           = input_buffer_size ? pContext->_op : BROTLI_OPERATION_FINISH;
    BROTLI_BOOL             ret                 = BROTLI_TRUE;
    HRESULT                 hr                  = S_OK;

    if (pContext == NULL)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    // IIS schema specifies staticCompressionLevel and dynamicCompressionLevel as uint,
    // so only the upper bound needs to be checked.
    if (compression_level > BROTLI_MAX_QUALITY)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    if (pContext->_pState == NULL)
    {
        // Create a BrotliEncoderState instance
        pContext->_pState = BrotliEncoderCreateInstance(NULL,   // alloc_func
                                                        NULL,   // free_func
                                                        NULL);  // opaque
        if (pContext->_pState == NULL)
        {
            DBGERROR((DBG_CONTEXT,
                      "BrotliEncoderCreateInstance failed\n"));

            hr = E_OUTOFMEMORY;
            goto Finished;
        }

        // Set compression quality for the BrotliEncoderState
        ret = BrotliEncoderSetParameter(pContext->_pState,      // state
                                        BROTLI_PARAM_QUALITY,   // param
                                        compression_level);     // value
        if (ret == BROTLI_FALSE)
        {
            DBGERROR((DBG_CONTEXT,
                      "BrotliEncoderSetParameter failed\n"));

            BrotliEncoderDestroyInstance(pContext->_pState);
            pContext->_pState = NULL;

            hr = HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION);
            goto Finished;
        }
    }

    DBGINFO((DBG_CONTEXT,
             "Start BrotliEncoderCompressStream\n"
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

    ret = BrotliEncoderCompressStream(pContext->_pState,    // state
                                      flushMode,            // op
                                      &input_available,     // available_in
                                      &input_buffer,        // next_in
                                      &output_available,    // available_out
                                      &output_buffer,       // next_out
                                      &total_out);          // total_out
    if (ret == BROTLI_FALSE)
    {
        DBGERROR((DBG_CONTEXT,
                  "BrotliEncoderCompressStream failed"));
        hr = E_FAIL;
        goto Finished;
    }

    *input_used = input_buffer_size - static_cast<LONG>(input_available);
    *output_used = output_buffer_size - static_cast<LONG>(output_available);

    DBGINFO((DBG_CONTEXT,
             "End BrotliEncoderCompressStream\n"
             "- input bytes consumed: %d\n"
             "- output bytes produced: %d\n",
             *input_used,
             *output_used));

    if (input_buffer_size == 0 && 
        BrotliEncoderHasMoreOutput(pContext->_pState) == BROTLI_FALSE)
    {
        DBGINFO((DBG_CONTEXT,
                 "Completed Brotli compression of the stream\n"));

        // Return S_FALSE to indicate IIS that the stream compression is done.
        hr = S_FALSE;
    }

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
    BROTLI_CONTEXT*         pContext            = static_cast<BROTLI_CONTEXT *>(context);
    size_t                  input_available     = static_cast<size_t>(input_buffer_size);
    size_t                  output_available    = static_cast<size_t>(output_buffer_size);
    size_t                  total_out           = 0;
    BrotliEncoderOperation  flushMode           = ConvertFlushMode(operation);
    BROTLI_BOOL             ret                 = BROTLI_TRUE;
    HRESULT                 hr                  = S_OK;

    if (pContext == NULL)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    // IIS schema specifies staticCompressionLevel and dynamicCompressionLevel as uint,
    // so only the upper bound needs to be checked.
    if (compression_level > BROTLI_MAX_QUALITY)
    {
        hr = E_INVALIDARG;
        goto Finished;
    }

    if (pContext->_pState == NULL)
    {
        // Create a BrotliEncoderState instance
        pContext->_pState = BrotliEncoderCreateInstance(NULL,   // alloc_func
                                                        NULL,   // free_func
                                                        NULL);  // opaque
        if (pContext->_pState == NULL)
        {
            DBGERROR((DBG_CONTEXT,
                      "BrotliEncoderCreateInstance failed\n"));

            hr = E_OUTOFMEMORY;
            goto Finished;
        }

        // Set compression quality for the BrotliEncoderState
        ret = BrotliEncoderSetParameter(pContext->_pState,      // state
                                        BROTLI_PARAM_QUALITY,   // param
                                        compression_level);     // value
        if (ret == BROTLI_FALSE)
        {
            DBGERROR((DBG_CONTEXT,
                      "BrotliEncoderSetParameter failed\n"));

            BrotliEncoderDestroyInstance(pContext->_pState);
            pContext->_pState = NULL;

            hr = HRESULT_FROM_WIN32(ERROR_BAD_CONFIGURATION);
            goto Finished;
        }
    }

    DBGINFO((DBG_CONTEXT,
             "Start BrotliEncoderCompressStream\n"
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

    ret = BrotliEncoderCompressStream(pContext->_pState,    // state
                                      flushMode,            // op
                                      &input_available,     // available_in
                                      &input_buffer,        // next_in
                                      &output_available,    // available_out
                                      &output_buffer,       // next_out
                                      &total_out);          // total_out
    if (ret == BROTLI_FALSE)
    {
        DBGERROR((DBG_CONTEXT,
                  "BrotliEncoderCompressStream failed\n"));
        hr = E_FAIL;
        goto Finished;
    }

    *input_used = input_buffer_size - static_cast<LONG>(input_available);
    *output_used = output_buffer_size - static_cast<LONG>(output_available);

    DBGINFO((DBG_CONTEXT,
             "End BrotliEncoderCompressStream\n"
             "- input bytes consumed: %d\n"
             "- output bytes produced: %d\n",
             *input_used,
             *output_used));

    if (input_buffer_size == 0 &&
        BrotliEncoderHasMoreOutput(pContext->_pState) == BROTLI_FALSE)
    {
        // Return S_FALSE to indicate IIS that all output bytes have been produced.
        hr = S_FALSE;
    }

Finished:

    return hr;
}

BrotliEncoderOperation
ConvertFlushMode(
    INT operation
)
{
    BrotliEncoderOperation flushMode = BROTLI_OPERATION_PROCESS;

    if (g_intEncoderOp != BROTLI_PARAMETER_UNSET)
    {
        flushMode = static_cast<BrotliEncoderOperation>(g_intEncoderOp);
    }

    switch (operation)
    {
    case IIS_COMPRESSION_OPERATION_PROCESS:
        break;
    case IIS_COMPRESSION_OPERATION_FLUSH:
        flushMode = BROTLI_OPERATION_FLUSH;
        break;
    case IIS_COMPRESSION_OPERATION_FINISH:
        flushMode = BROTLI_OPERATION_FINISH;
        break;
    default:
        break;
    }

    return flushMode;
}