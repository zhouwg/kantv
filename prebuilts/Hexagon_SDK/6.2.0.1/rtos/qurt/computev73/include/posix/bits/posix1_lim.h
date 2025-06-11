#ifndef POSIX1_LIM_H
#define POSIX1_LIM_H
/**
  @file posix1_lim.h
  @brief POSIX Minimum values

EXTERNAL FUNCTIONS    
   None 

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None 
    
TODO    
   This header should be ideally relocated under api/posix/bits (something that 
   doesnt exist today) and be included from api/posix/bits/limits.h which inturn 
   should be included from toolchain's limits.h 

Copyright (c) 2018, 2023 by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.    
    
==============================================================================*/

#ifndef _POSIX_PATH_MAX
/** @brief Maximum number of bytes in a pathname, including the terminating
    nul character */
#define _POSIX_PATH_MAX 256
#endif

#ifndef _POSIX_SEM_NSEMS_MAX
/** @brief Maximum number of semaphores that a process may have */
#define _POSIX_SEM_NSEMS_MAX 16
#endif
#endif

