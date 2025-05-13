/**=============================================================================
@file
    os_defines.h

@brief
    Abstract operating system specific defines, includes and global variables
    to make it convenient for developers to code for multiple OS platforms.

Copyright (c) 2021 Qualcomm Technologies Incorporated.
All Rights Reserved. Qualcomm Proprietary and Confidential.
=============================================================================**/

#ifndef OS_DEFINES_H_
#define OS_DEFINES_H_


#ifdef __cplusplus
  extern "C" {
#endif


/* Offset to differentiate HLOS and Hexagon error codes.
   Stores the value of AEE_EOFFSET for Hexagon. */
#ifndef DSP_OFFSET
  #define DSP_OFFSET 0x80000400
#endif


/* Errno for connection reset by peer. */
#ifndef ECONNRESET
  #ifdef __hexagon__
    #define ECONNRESET 104
  #endif
#endif


/* Abstraction of different OS specific sleep APIs.
   SLEEP accepts input in seconds. */
#ifndef SLEEP
  #ifdef __hexagon__
    #define SLEEP(x) {/* Do nothing for simulator. */}
  #else
    #ifdef _WINDOWS
      #define SLEEP(x) Sleep(1000*x) /* Sleep accepts input in milliseconds. */
    #else
      #define SLEEP(x) sleep(x) /* sleep accepts input in seconds. */
    #endif
  #endif
#endif


/* Include windows specific header files. */
#ifdef _WINDOWS
  #include <windows.h>
  #include <sysinfoapi.h>
  #define _CRT_SECURE_NO_WARNINGS 1
  #define _WINSOCK_DEPRECATED_NO_WARNINGS 1
  /* Including this file for custom implementation of getopt function. */
  #include "getopt_custom.h"
#endif


/* Includes and defines for all HLOS except windows */
#if !defined(__hexagon__) && !defined (_WINDOWS)
  #include "unistd.h"
  #include <sys/time.h>
#endif


/* Includes and defines for Hexagon and all HLOS except Windows. */
#if !defined (_WINDOWS)
  /* Weak reference to remote symbol for compilation. */
  #pragma weak remote_session_control
  #pragma weak remote_handle_control
  #pragma weak remote_handle64_control
  #pragma weak fastrpc_mmap
  #pragma weak fastrpc_munmap
#endif


/* Includes and defines for hexagon */
#ifdef __hexagon__
#endif


/* Includes and defines for Android */
#ifdef ANDROID
#endif


#ifdef __cplusplus
}
#endif



#endif //OS_DEFINES_H_
