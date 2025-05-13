#ifndef QURT_PIMUTEX2_H
#define QURT_PIMUTEX2_H
/**
  @file qurt_pimutex2.h 
  @brief Prototypes of pimutex2 API  

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2013, 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/


#include <qurt_futex.h>
#include <qurt_mutex.h>
#include <qurt_rmutex2.h>

/*=============================================================================
												FUNCTIONS
=============================================================================*/
#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup func_qurt_pimutex2_init
   Initializes a recursive mutex object. 

   @deprecated use #qurt_pimutex_init instead.

   The recursive mutex is initially unlocked.
  
   Objects of type pimutex2 solve a potential race condition between
   unlock() and destroy() operations.

   @datatypes
   #qurt_rmutex2_t
   
   @param[out]  lock  Pointer to the recursive mutex object.

   @return 
   None.

   @dependencies
   None.
  
 */
void qurt_pimutex2_init(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_pimutex2_destroy

  @deprecated use #qurt_pimutex_destroy instead.

  Destroys the specified recursive mutex. \n
  @note1cont Recursive mutexes must not be destroyed while they are still in use. If this
             occurs, the behavior of QuRT is undefined. 
  @note1cont In general, application code should destroy an pimutex2 object prior to
             deallocating it; calling qurt_pimutex2_destroy() before deallocating it ensures
             that all qurt_pimutex2_unlock() calls complete.
  
  @datatypes
  #qurt_rmutex2_t
  
  @param[in]  lock  Pointer to the recursive mutex object to destroy.

  @return
  None.

  @dependencies
  None.
  
 */
void qurt_pimutex2_destroy(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_pimutex2_lock

  @deprecated use #qurt_pimutex_lock instead.

  Locks the specified recursive mutex. \n 

  If a thread performs a lock operation on a recursive mutex that is not being used, the
  thread gains access to the shared resource that is protected by the mutex, and continues
  executing.

  If a thread performs a lock operation on a recursive mutex that is already being used by
  another thread, the thread is suspended. When the mutex becomes available again
  (because the other thread has unlocked it), the thread is awakened and given access to the
  shared resource.
  
  @note1hang A thread is not suspended if it locks a recursive mutex that it has already
             locked, but the mutex does not become available until the thread performs a
             balanced number of unlocks on the mutex.
  
   @datatypes
   #qurt_rmutex2_t
  
   @param[in]  lock  Pointer to the recursive mutex object to lock. 

   @return
   None.

   @dependencies
   None.
  
 */
void qurt_pimutex2_lock(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_pimutex2_unlock

   @deprecated use #qurt_pimutex_unlock instead.

   Unlocks the specified recursive mutex. \n 
   More than one thread can be suspended on a recursive mutex. When the mutex is
   unlocked, only the highest-priority thread waiting on the mutex is awakened. If the
   awakened thread has higher priority than the current thread, a context switch occurs.
  
   @datatypes
   #qurt_rmutex2_t
  
   @param[in]  lock  Pointer to the recursive mutex object to unlock. 

   @return
   None.

   @dependencies
   None.
  
 */
void qurt_pimutex2_unlock(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_rmutex2_try_lock

   @deprecated use #qurt_pimutex_try_lock instead.

   Attempts to lock the specified recursive mutex.\n

   Non-blocking version of qurt_pimutex2_lock().  If a call to qurt_pimutex2_lock() would
   succeed immediately, this function behaves similarly, and returns 0 for success.
   If a call to qurt_pimutex2_lock() would not succeed immediately, this function has
   no effect and returns non-zero for failure.

   @datatypes
   #qurt_rmutex2_t
   
   @param[in]  lock  Pointer to the recursive mutex object to lock.

   @return 
   0 -- Success. \n 
   Nonzero -- Failure. 
  
 */
int qurt_pimutex2_try_lock(qurt_rmutex2_t *lock);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_PIMUTEX2_H */
