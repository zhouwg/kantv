/*-----------------------------------------------------------------------
   Copyright (c) 2017-2020 QUALCOMM Technologies, Incorporated.
   All Rights Reserved.
   QUALCOMM Proprietary.
-----------------------------------------------------------------------*/
#ifndef SYSMON_MARKER_H
#define SYSMON_MARKER_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file sysmon_marker.h
 * @brief Sysmon profiling marker API
 *        Allows the user to profile a piece of code or
 *        algorithm of interest.
 */

/**
 * Enables or disables a profiling marker.

 * @param[in] marker  Any unique, customer-defined, unsigned number to identify profiling 
                      data mapped to a section of code.
 * @param[in] enable  Flag to enable (1) or disable (1) the profiling marker.
 *
 * For example:
 * @code
 * #include <sysmon_marker.h>
 * // or, alternatively,
 * // extern HP_profile(unsigned int marker, unsigned char enable);
 *
 * HP_profile(10, 1);
 * // ...
 * // User code to profile
 * // ...
 * HP_profile(10, 0);
 * @endcode
 */
void HP_profile(unsigned int marker, unsigned char enable);

#ifdef __cplusplus
}
#endif

#endif /*SYSMON_MARKER_H*/
