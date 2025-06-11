/*==============================================================================
@file
   HAP_perf.h

@brief
   Header file for DSP Perf APIs

Copyright (c) 2012-2017, 2020 QUALCOMM Technologies, Incorporated.
All Rights Reserved.
QUALCOMM Proprietary.
==============================================================================*/
#ifndef HAP_PERF_H
#define HAP_PERF_H

#include "AEEStdDef.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup timer_functionality Timer functionality.
 *  @{
 */

/**
  * Gets the current value of 56-bit, 19.2MHz hardware counter converted
  * to micro-seconds. This value should be treated as relative and
  * not absolute. The value wraps around to zero once it exceeds the
  * maxiumum value. This function performs an integer division in order
  * to convert ticks to time, which adds some overhead. Consider using
  * HAP_perf_get_qtimer_count for a lower overhead.
*/
#ifdef __hexagon__
#include "hexagon_sim_timer.h"
static inline uint64 HAP_perf_get_time_us(void)
{
       /* Converts ticks into microseconds
       1 tick = 1/19.2MHz seconds
       Micro Seconds = Ticks * 10ULL/192ULL */
  unsigned long long count;
  asm volatile (" %0 = c31:30 " : "=r"(count));
  return (uint64)(count) * 10ull / 192ull;
}
#else
uint64 HAP_perf_get_time_us(void)
{
     static long long start = 0;
  // TODO
  // assume 500 MHz on simulator
  //return HAP_perf_get_pcycles() / 500;
  return start++;
}
#endif

/**
  * Gets the current 56 bit, 19.2MHz global hardware clock count.
  * This value should be treated as relative and not absolute.
  * The value wraps around to zero once it exceeds the maxiumum value.
*/
static inline uint64 HAP_perf_get_qtimer_count(void) {
    unsigned long long cur_count;
    asm volatile(" %0 = c31:30 " : "=r"(cur_count));
    return (uint64)cur_count;
}

/**
  * Converts the 19.2 MHz global hardware count to micro-seconds.
  * @param[in] count			- 19.2 MHz global hardware count
  * @returns				- Time in micro-seconds.
*/
uint64 HAP_perf_qtimer_count_to_us(uint64 count);

/**
  * Gets the current 64-bit Hexagon Processor cycle count.
  * The processor cycle count is the current number of processor
  * cycles executed since the processor was last reset.  Note
  * that this counter stops incrementing whenever the DSP enters
  * a low-power  state (such as clock gating), as opposed to the
  * qtimer, which increments regardless of the DSP power state.
*/
#ifdef __hexagon__
#include "hexagon_sim_timer.h"
static inline uint64 HAP_perf_get_pcycles(void)
{
  uint64_t pcycle;
  asm volatile ("%[pcycle] = C15:14" : [pcycle] "=r"(pcycle));
  return pcycle;
}
#else
uint64 HAP_perf_get_pcycles(void)
{
  return (uint64)0;
}
#endif

/**
 * @}
 */

/** @defgroup sleep_functionality Sleep functionality.
 *  @{
 */

/**
  * Suspends the calling thread from execution until the
  * specified duration has elapsed.
  * @param[in] sleep_duration:			- Sleep duration in micro-seconds.
  * @returns					- returns 0 on success, non zero in error case.
*/
int HAP_timer_sleep(unsigned long long sleep_duration);

/**
 * @} // sleep_functionality
 */

#ifdef __cplusplus
}
#endif

#endif // HAP_PERF_H
