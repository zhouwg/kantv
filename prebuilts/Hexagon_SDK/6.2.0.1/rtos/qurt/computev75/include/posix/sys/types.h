#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

/*==========================================================================
 * FILE:         types.c
 *
 * SERVICES:     types usded in POSIX API interface
 *
 * DESCRIPTION:  POSIX API interface based upon POSIX 1003.1-2004
 *
 *               Copyright (c) 2013, 2016  by Qualcomm Technologies, Inc.  All Rights Reserved. QUALCOMM Proprietary and Confidential.

 *==========================================================================*/

#if !defined( _PID_T ) || !defined( __pid_t_defined )
/* POSIX defines pid_t as signed 32-bit type. Hexagon toolchain's header
   defines it as unsigned 32-bit type citing conflict with QuRT POSIX
   compatibility later. If any such conflicts exist, we should fix them.
   pid_t is being defined *BEFORE* inclusion of generic/sys/types.h
   *INTENTIONALLY* to fix this */
typedef int        pid_t;
#define _PID_T
#define __pid_t_defined
#endif
#include <bits/confname.h>
#include <hooks/unistd.h>
#include <generic/sys/types.h>
#include <pthread_types.h>

#ifndef __DEFINED_off_t
typedef long       off_t;
#define __DEFINED_off_t
#endif

#endif /* _SYS_TYPES_H_ */
