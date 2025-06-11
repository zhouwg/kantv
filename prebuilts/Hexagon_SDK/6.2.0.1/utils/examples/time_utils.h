/**=============================================================================
@file
    time_utils.h

@brief
    Abstract operating system specific timing APIs.

Copyright (c) 2021 Qualcomm Technologies Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
=============================================================================**/

#ifndef TIME_UTILS_H_
#define TIME_UTILS_H_


#ifdef __cplusplus
  extern "C" {
#endif

#ifdef _WINDOWS
    #include <windows.h>
#else
#ifdef __hexagon__
    #include "hexagon_sim_timer.h"
#else
    #include <sys/time.h>
#endif
#endif

unsigned long long get_time(void);
void sleep_in_microseconds(unsigned long long);

/* Abstraction of different OS specific usleep APIs.
   USLEEP accepts input in microseconds. */
#ifndef USLEEP
  #ifdef __hexagon__
    #define USLEEP(x) {/* Do nothing for simulator. */}
  #else
    #ifdef _WINDOWS
      #define USLEEP(x) sleep_in_microseconds(x)
    #else
	  #include <unistd.h>
      #define USLEEP(x) usleep(x)
    #endif
  #endif
#endif

/* Abstraction of different OS specific timer APIs.
   GET_TIME returns the value of time*/
#ifndef GET_TIME
  #ifdef __hexagon__
    #define GET_TIME hexagon_sim_read_pcycles
  #else
    #define GET_TIME get_time
  #endif
#endif


#ifdef __cplusplus
}
#endif


#endif //TIME_UTILS_H_
