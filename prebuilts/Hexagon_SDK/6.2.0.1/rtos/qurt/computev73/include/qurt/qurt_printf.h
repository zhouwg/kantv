#ifndef QURT_PRINTF_H
#define QURT_PRINTF_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
  @file qurt_printf.h   
  Prototypes of printf API.  

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/



/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/** @addtogroup chapter_function_tracing
@{ */

int qurt_printf(const char* format, ...);

int qurt_vprintf(const char* format, va_list args);

/** @} */ /* end_addtogroup chapter_function_tracing */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_PRINTF_H */

