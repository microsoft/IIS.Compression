;/*++
;  Copyright (c) Microsoft Corporation. All rights reserved.
;  Licensed under the MIT license.
;--*/
;
;#ifndef _IISBROTLI_MSG_H_
;#define _IISBROTLI_MSG_H_
;

SeverityNames=(Success=0x0
               Informational=0x1
               Warning=0x2
               Error=0x3
              )

Messageid=1000 Severity=Error SymbolicName=BROTLI_COMPRESSION_LEVEL_OUT_OF_BOUNDS
Language=English
The Brotli compression level '%1' is larger than the maximum allowed value '%2'.
.

;
;#endif     // _IISBROTLI_MSG_H_
;
