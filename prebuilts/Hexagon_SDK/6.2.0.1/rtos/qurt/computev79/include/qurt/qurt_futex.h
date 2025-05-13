#ifndef QURT_FUTEX_H
#define QURT_FUTEX_H
/**
  @file  qurt_futex.h

  @brief  Prototypes of QuRT futex API functions      
  
 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013, 2020-2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=====================================================================
 Functions
======================================================================*/


/**@ingroup func_qurt_futex_wait
  Moves the caller thread into waiting state when a memory object address
  contains a value that is the same as a specified value. 

   @param[in]  lock  Pointer to the object memory. 
   @param[in]  val   Value to check against the object content. 

   @return
   #QURT_EOK -- Success \n
   Other values -- Failure

   @dependencies
   None.
 */
int qurt_futex_wait(void *lock, int val);


/**@ingroup func_qurt_futex_wait_cancellable
  If a memory object address contains a value that is same as a specified 
  value, move the caller thread into waiting state. 
  The kernal can cancel the waiting state when there is a special need. 

   @param[in]  lock  Pointer to the object memory. 
   @param[in]  val   Value to check against the object content. 

   @return
   #QURT_EOK -- Success \n
   Other values  -- Failure

   @dependencies
   None.
 */
int qurt_futex_wait_cancellable(void *lock, int val);


/**@ingroup func_qurt_futex_wake
  Wakes up a specified number of threads that have been waiting 
  for the object change with qurt_futex_wait().

   @param[in]  lock        Pointer to the object memory. 
   @param[in]  n_to_wake   Maximum number of threads to wake up.

   @return
   number of threads to be woken up by this function

   @dependencies
   None.
 */
int qurt_futex_wake(void *lock, int n_to_wake);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_FUTEX_H */

