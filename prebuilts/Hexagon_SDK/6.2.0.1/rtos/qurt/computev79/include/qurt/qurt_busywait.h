#ifndef QURT_BUSYWAIT_H
#define QURT_BUSYWAIT_H

/**
  @file qurt_busywait.h 
  @brief Implementation of the busywait() function for 
   hardware based blocking waits that use the QTIMER as a reference.   

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ============================================================================*/
/*=============================================================================
 *
 *                       EDIT HISTORY FOR FILE
 *
 *   This section contains comments describing changes made to the
 *   module. Changes are listed in reverse chronological
 *   order.
 *
 * 
 * when         who     what, where, why
 * ----------   ---     -------------------------------------------------------
 * 2018-03-20   pg      Add Header file
 ============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                                    FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_busywait
  Pauses the execution of a thread for a specified time.\n
  Use for small microsecond delays.
  
  @note1hang The function does not return to the caller until
  the time duration has expired.
             
  @param[in] pause_time_us Time to pause in microseconds. 
 
  @return
  None.

  @dependencies
  None.
 */
void qurt_busywait (unsigned int pause_time_us);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_BUSYWAIT_H */

