#ifndef _POSIX_TIME_H_
#define _POSIX_TIME_H_

/*==========================================================================
 * FILE:         time.h
 *
 * SERVICES:     POSIX Timer API interface
 *
 * DESCRIPTION:  POSIX Timer API interface based upon POSIX 1003.1-2004
 *
 *               Copyright (c) 2013,2016  by Qualcomm Technologies, Inc.  All Rights Reserved. QUALCOMM Proprietary and Confidential.
 *==========================================================================*/


#include <sys/types.h>

typedef int              clockid_t; /* ignored */
#define _CLOCKID_T
#define _PROVIDE_POSIX_TIME_DECLS 1
#include <generic/time.h>
/* @todo anandj sys/time.h has definition for struct timeval but is not
         included by generic/time.h */
#include <sys/time.h>

#define CLOCK_FREQ_NOT_DEFINED          -1
/* Frequency of Sclk used */
#define TIME_CONV_SCLK_FREQ             19200000

#define RES_CONV_FACTOR1                1
#define RES_CONV_FACTOR2                1000000000

#if !defined(CLOCK_REALTIME)
# define CLOCK_REALTIME 0
#endif

#if !defined(CLOCK_MONOTONIC)
# define CLOCK_MONOTONIC 1
#endif

#if !defined(CLOCK_THREAD_CPUTIME_ID)
# define CLOCK_THREAD_CPUTIME_ID 2
#endif

#if !defined(CLOCK_PROCESS_CPUTIME_ID)
# define CLOCK_PROCESS_CPUTIME_ID 3
#endif

#if !defined(CLOCK_MONOTONIC_RAW)
# define CLOCK_MONOTONIC_RAW 4
#endif

#if !defined(CLOCK_REALTIME_COARSE)
# define CLOCK_REALTIME_COARSE 5
#endif

#if !defined(CLOCK_MONOTONIC_COARSE)
# define CLOCK_MONOTONIC_COARSE 6
#endif

#if !defined(CLOCK_BOOTTIME)
# define CLOCK_BOOTTIME 7
#endif

struct itimerspec
{
    struct timespec it_interval;  /* Timer period.     */
    struct timespec it_value;     /* Timer expiration. */
};

/* have to move #include here to solve circular include problems between time.h and signal.h */
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Timer functions */

/** \details
 * POSIX timers can be either of two types: a one-shot type or a periodic 
 * type.
 *
 * A one-shot is an armed timer that is set to an expiration time relative 
 * to either a current time or an absolute time. The timer expires once and 
 * is disarmed. 
 *
 * A periodic timer is armed with an initial expiration time and a repetition 
 * interval. Every time the interval timer 
 * expires, the timer is reloaded with the repetition interval. The timer 
 * is then rearmed. 
 */

/** \defgroup timer POSIX Timer API */

/** \ingroup timer */
/** @{ */

/** Create a POSIX timer. 
 * Please refer to POSIX standard for details.
 * @param clockid [in] ignored in this implementation
 * @param evp     [in] if non-NULL, points to a sigevent structure. This 
 * structure, allocated by the application, defines the asynchronous 
 * notification to occur when the timer expires. If the evp argument is 
 * NULL, the effect is as if the evp argument pointed to a sigevent 
 * structure with the sigev_notify member having the value SIGEV_SIGNAL, 
 * the sigev_signo having a default signal number (SIGALRM), and the 
 * sigev_value member having the value of the timer ID.
 */
int timer_create(clockid_t clockid, struct sigevent *restrict evp,
                 timer_t *restrict timerid);

/** Delete a POSIX timer. 
 * Please refer to POSIX standard for details.
 */                 
int timer_delete(timer_t timerid);

/** Get the time remaining on a POSIX timer. 
 * Please refer to POSIX standard for details.
 */                 
int timer_gettime(timer_t timerid, struct itimerspec *value);


/** Set the time remaining on a POSIX timer. 
 * Please refer to POSIX standard for details.
 * @param flags [in] ignored in this implementation
 */                 
int timer_settime(timer_t timerid, int flags,
                  const struct itimerspec *restrict value,
                  struct itimerspec *restrict ovalue);
/** Obtain ID of a process CPU-time clock
 *  @param pid [in] Process ID
 *  @param clock_id [out] Clock ID
 *  @return Error values as per POSIX standard
 */
int clock_getcpuclockid (pid_t pid, clockid_t * clock_id);
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* _POSIX_TIME_H_ */
