#ifndef HAP_DEBUG_H
#define HAP_DEBUG_H
/*==============================================================================
  Copyright (c) 2012-2013 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#include "AEEStdDef.h"
#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HAP_LEVEL_LOW       0
#define HAP_LEVEL_MEDIUM    1
#define HAP_LEVEL_HIGH      2
#define HAP_LEVEL_ERROR     3
#define HAP_LEVEL_FATAL     4

#define HAP_LEVEL_RUNTIME   (1 << 5)

//Add a weak reference so shared objects work with older images
#pragma weak HAP_debug_v2

//Add a weak reference for enabling FARF in autogen stub files
#pragma weak HAP_debug

//Add a weak reference so runtime FARFs are ignored on older images
#pragma weak HAP_debug_runtime

/**************************************************************************
    These HAP_debug* functions are not meant to be called directly.
    Please use the FARF() macros to call them instead
**************************************************************************/
void HAP_debug_v2(int level, const char* file, int line, const char* format, ...);
void HAP_debug_runtime(int level, const char* file, int line, const char* format, ...);
int HAP_setFARFRuntimeLoggingParams(unsigned int mask, const char* files[],
                                    unsigned short numberOfFiles);

// Keep these around to support older shared objects and older images
void HAP_debug(const char *msg, int level, const char *filename, int line);

static __inline void _HAP_debug_v2(int level, const char* file, int line,
                          const char* format, ...){
   char buf[256];
   va_list args;
   va_start(args, format);
   vsnprintf(buf, sizeof(buf), format, args);
   va_end(args);
   HAP_debug(buf, level, file, line);
}

/*!
This function is called to log an accumlated log entry. If logging is
enabled for the entry by the external device, then the entry is copied
into the diag allocation manager and commited.

    [in] log_code_type    ID of the event to be reported
    [in] *data            data points to the log which is to be submitted
    [in] dataLen          The length of the data to be logged.

Returns
    TRUE if log is submitted successfully into diag buffers
    FALSE if there is no space left in the buffers.

*/
boolean HAP_log_data_packet(unsigned short log_code_type, unsigned int dataLen,
                    byte* data);

#define HAP_DEBUG_TRACEME 0

long HAP_debug_ptrace(int req, unsigned int pid, void* addr, void* data);

#ifdef __cplusplus
}
#endif

#endif // HAP_DEBUG_H

