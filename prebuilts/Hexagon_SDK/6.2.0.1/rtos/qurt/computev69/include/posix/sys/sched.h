#ifndef _POSIX_SCHED_H_
#define _POSIX_SCHED_H_

/*==========================================================================
 * FILE:         sched.c
 *
 * SERVICES:     POSIX Thread sched API interface
 *
 * DESCRIPTION:  POSIX Thread sched API interface based upon POSIX 1003.1-2004
 *
 *               Copyright (c) 2013, 2016  by Qualcomm Technologies, Inc.  All Rights Reserved. QUALCOMM Proprietary and Confidential.


 *==========================================================================*/

#include <qurt.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCHED_FIFO        0 /* First in, first out (FIFO) scheduling policy. */
#define SCHED_RR          1 /* Round robin scheduling policy. */
#define SCHED_SPORADIC    2 /* Sporadic server scheduling policy. */
#define SCHED_OTHER       3 /* Another scheduling policy. */

typedef struct sched_param   sched_param;
struct sched_param
{
    void *unimplemented;
    int  sched_priority;
};

/** \details 
 * This provides POSIX sched API. 
 */

/** \defgroup sched POSIX sched API */
/** \ingroup sched */
/** @{ */

/** Relinquish the CPU.
 * Please refer to POSIX standard for details.
 */
static inline int sched_yield(void)
{
   return 0;
}

/** Get the maximum priority.
 * Please refer to POSIX standard for details.
 * @param policy [in] SCHED_FIFO is the only valid input for this implementation.
 */
int sched_get_priority_max(int policy);

/** Get the minimum priority.
 * Please refer to POSIX standard for details.
 * @param policy [in] SCHED_FIFO is the only valid input for this implementation.
 */
int sched_get_priority_min(int policy);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* _POSIX_SCHED_H_ */
