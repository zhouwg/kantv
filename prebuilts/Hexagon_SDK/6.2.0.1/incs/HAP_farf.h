/*==============================================================================
  Copyright (c) 2012-2013, 2020 Qualcomm Technologies, Inc.
  All rights reserved. Qualcomm Proprietary and Confidential.
==============================================================================*/

#ifndef HAP_FARF_H
#define HAP_FARF_H

/**
 * @file HAP_farf.h
 * @brief FARF API
 */

#include "AEEStdDef.h"
#include "HAP_debug.h"

/**
 *\def FARF()
 * FARF is used to log debug messages from DSP
 *
 * `Compile time logging options:`
 *
 * Logging is controlled via conditional compilation.
 * The FARF level allows the user to selectively enable or disable certain types
 * of messages according to their priority level.
 * The following levels are supported and listed in increasing priority:
 *
 *   LOW
 *
 *   MEDIUM
 *
 *   HIGH
 *
 *   ERROR
 *
 *   FATAL
 *
 *   ALWAYS
 *
 * A FARF level should be defined to 1 for FARF macros to be compiled
 * in. For example:
 *
 * @code
 *    #define FARF_LOW 1
 *    #include "HAP_farf.h"
 *
 *    FARF(LOW, "something happened: %s", (const char*)string);
 *
 * @endcode
 *
 * FARF_LOW, FARF_MEDIM, FARF_HIGH are defined to 0 and FARF_ERROR,
 * FARF_FATAL, FARF_ALWAYS are defined to 1 by default.
 *
 * If FARF_LOW is defined to 0, as it is by default, the above
 * FARF string will not be compiled in, if it is defined to 1 it
 * will be compiled in.
 *
 * If both HIGH and LOW messages are used but only FARF_LOW is defined
 * as shown in below example then only LOW message will be compiled in and sent to DIAG.
 *
 * @code
 *    #define FARF_LOW 1
 *    #include "HAP_farf.h"
 *
 *    FARF(LOW, "LOW message");
 *    FARF(HIGH, "HIGH message"); // This message will not be compiled in
 *
 * @endcode
 *
 * Messages logged with ALWAYS level are always compiled in and logged.
 *
 * When building the Debug variant or builds defining _DEBUG the
 * following FARF levels will be enabled:
 *
 *    HIGH
 *
 *    ERROR
 *
 *    FATAL
 *
 * `Run time logging options:`
 *
 * In order to enable run-time logging (logging that can be enabled / disabled
 * at run-time), the FARF_RUNTIME_* macros should be used.
 *
 * Log messages sent with these macros are compiled in by default. However by
 * these messages WILL NOT be logged by default. In order to enable logging,
 * the FASTRPC process will need to either call the
 * HAP_SetFARFRuntimeLoggingParams() API, or by adding a `<process_name>`.farf
 * file to the HLOS file system with the appropriate contents.
 *
 * @code
 *
 *      #include "HAP_farf.h"
 *      FARF(RUNTIME_HIGH, "something happened: %s", (const char*)string);
 *
 * @endcode
 *
 * @param[in] x the FARF level defined to either 0 to disable compilation or 1 to enable.
 * @param[in] ... the format string and arguments.
 */
#define FARF(x, ...) _FARF_PASTE(_FARF_,_FARF_VAL(FARF_##x))(x, ##__VA_ARGS__)


/**
*   @defgroup static_FARF Compile-time macros
*
*   Set these compile time macros to 1 to enable logging at that
*   level. Setting them to 0 will cause them to be COMPILED out.
*
*   Usage Example:
*   @code
*
*    #define FARF_HIGH 1
*    FARF(HIGH,"Log message");
*
*   @endcode

*   The ALWAYS macro will cause log messages to be ALWAYS compiled in.
*   @code
*
*   FARF(ALWAYS,"Log message")
*
*   @endcode
*
*   Defining _DEBUG macro turns on ALWAYS, HIGH, ERROR, FATAL
*/
/*  @{ */

#ifdef _DEBUG
#ifndef FARF_HIGH
#define FARF_HIGH 1
#endif
#endif

/**
 * The FARF_ALWAYS macro causes log messages to be ALWAYS compiled in
 */
#ifndef FARF_ALWAYS
#define FARF_ALWAYS        1
#endif

/**
 * The FARF_LOW macro causes log messages to be compiled in when FARF_LOW is defined to 1
*/
#ifndef FARF_LOW
#define FARF_LOW           0
#endif

/**
* The FARF_MEDIUM macro causes log messages to be compiled in when FARF_MEDIUM is defined to 1
*/
#ifndef FARF_MEDIUM
#define FARF_MEDIUM        0
#endif

/**
* The FARF_HIGH macro causes log messages to be compiled in when FARF_HIGH is defined to 1
*/
#ifndef FARF_HIGH
#define FARF_HIGH          0
#endif

/**
* The FARF_ERROR macro causes log messages to be compiled in when FARF_ERROR is defined to 1
*/
#ifndef FARF_ERROR
#define FARF_ERROR         1
#endif

/**
* The FARF_FATAL macro causes log messages to be compiled in when FARF_FATAL is defined to 1
*/
#ifndef FARF_FATAL
#define FARF_FATAL         1
#endif

//! @cond Doxygen_Suppress
#define FARF_ALWAYS_LEVEL  HAP_LEVEL_HIGH
#define FARF_LOW_LEVEL     HAP_LEVEL_LOW
#define FARF_MEDIUM_LEVEL  HAP_LEVEL_MEDIUM
#define FARF_HIGH_LEVEL    HAP_LEVEL_HIGH
#define FARF_ERROR_LEVEL   HAP_LEVEL_ERROR
#define FARF_FATAL_LEVEL   HAP_LEVEL_FATAL
//! @endcond

/* @} */


/**
*   @defgroup Runtime_FARF Runtime macros
*
*   Runtime FARF macros can be enabled at runtime.
*   They are turned OFF by default.
*
*   Usage Example:
*   @code
*
*   FARF(RUNTIME_HIGH,"Log message");
*
*   @endcode
*/
/*  @{ */
//! @cond Doxygen_Suppress
#ifndef FARF_RUNTIME_LOW
#define FARF_RUNTIME_LOW           1
#endif
#define FARF_RUNTIME_LOW_LEVEL     (HAP_LEVEL_RUNTIME | HAP_LEVEL_LOW)

#ifndef FARF_RUNTIME_MEDIUM
#define FARF_RUNTIME_MEDIUM        1
#endif
#define FARF_RUNTIME_MEDIUM_LEVEL  (HAP_LEVEL_RUNTIME | HAP_LEVEL_MEDIUM)

#ifndef FARF_RUNTIME_HIGH
#define FARF_RUNTIME_HIGH          1
#endif
#define FARF_RUNTIME_HIGH_LEVEL    (HAP_LEVEL_RUNTIME | HAP_LEVEL_HIGH)

#ifndef FARF_RUNTIME_ERROR
#define FARF_RUNTIME_ERROR         1
#endif
#define FARF_RUNTIME_ERROR_LEVEL   (HAP_LEVEL_RUNTIME | HAP_LEVEL_ERROR)

#ifndef FARF_RUNTIME_FATAL
#define FARF_RUNTIME_FATAL         1
#endif
#define FARF_RUNTIME_FATAL_LEVEL   (HAP_LEVEL_RUNTIME | HAP_LEVEL_FATAL)
//! @endcond
/* @} */


//! @cond Doxygen_Suppress

#define _FARF_PASTE(a,b) _FARF_PASTE_(a,b)
#define _FARF_PASTE_(a,b) a##b
#define _FARF_VAL(a) a


#define _FARF_0(x, ...)

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

#define _FARF_1(x, ...) \
    do { \
            if(0 == (HAP_debug_v2)) { \
                _HAP_debug_v2(FARF_##x##_LEVEL, __FILENAME__, __LINE__, ##__VA_ARGS__); \
            } else { \
                if (FARF_##x##_LEVEL & HAP_LEVEL_RUNTIME) { \
                    if (0 != HAP_debug_runtime) { \
                        HAP_debug_runtime(FARF_##x##_LEVEL ^ HAP_LEVEL_RUNTIME , __FILENAME__, __LINE__, ##__VA_ARGS__); \
                    } else { \
                        break; \
                    } \
                } else { \
                    HAP_debug_v2(FARF_##x##_LEVEL, __FILENAME__, __LINE__, ##__VA_ARGS__); \
                } \
            } \
        } while (0)

#endif /* #ifndef HAP_FARF_H */
//! @endcond
