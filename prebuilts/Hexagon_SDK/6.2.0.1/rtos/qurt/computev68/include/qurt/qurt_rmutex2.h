#ifndef QURT_RMUTEX2_H
#define QURT_RMUTEX2_H
/**
  @file qurt_rmutex2.h 
  @brief Prototypes of rmutex2 API  

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2013, 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/


#include <qurt_futex.h>
#include <qurt_mutex.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup mutex_types
@{ */
/*=============================================================================
                        TYPEDEFS
=============================================================================*/

/** QuRT rmutex2 type.                                       
   Mutex type used with rmutex2 APIs.
 */
typedef struct {
   /** @cond */
   unsigned int holder __attribute__((aligned(8)));    /* UGP value of the mutex holder. */
   unsigned short waiters;                             /* Number of waiting threads. */
   unsigned short refs;                                /* Number of references to this mutex. */
   unsigned int queue;                                 /* Kernel-maintained futex queuevalue. */
   unsigned int excess_locks;                          /* Number of excess times the holder has locked the mutex. */
   /** @endcond */
} qurt_rmutex2_t;
/** @} */ /* end_addtogroup mutex_types */
/** @cond internal_only*/
/*=============================================================================
												FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_rmutex2_init

   @deprecated use #qurt_rmutex_init instead.

   Initializes a recursive mutex object. 

   The recursive mutex is initially unlocked.
  
   Objects of type rmutex2 solve a potential race condition between
   unlock() and destroy() operations.

   @datatypes
   #qurt_rmutex2_t
   
   @param[out]  lock  Pointer to the recursive mutex object.

   @return 
   None.

   @dependencies
   None.
 */
void qurt_rmutex2_init(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_rmutex2_destroy

  @deprecated use #qurt_rmutex_destroy instead.

  Destroys the specified recursive mutex. \n
  @note1hang Recursive mutexes must not be destroyed while they are still in use. If this
             occurs, the behavior of QuRT is undefined. 
  @note1cont In general, application code must destroy an rmutex2 object prior to
             deallocating it; calling qurt_rmutex2_destroy() before deallocating it ensures
             that all qurt_rmutex2_unlock() calls complete.
  
  @datatypes
  #qurt_rmutex2_t
  
  @param[in]  lock  Pointer to the recursive mutex object to destroy.

  @return
  None.

  @dependencies
  None.
  
 */
void qurt_rmutex2_destroy(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_rmutex2_lock

  @deprecated use #qurt_rmutex_lock instead.

  Locks the specified recursive mutex. \n 

  If a thread performs a lock operation on a recursive mutex that is not in use, the
  thread gains access to the shared resource that the mutex protects, and continues
  to execute.

  If a thread performs a lock operation on a recursive mutex that another thread is using, 
  the thread is suspended. When the mutex becomes available again
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
void qurt_rmutex2_lock(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_rmutex2_unlock

   @deprecated use #qurt_rmutex_unlock instead.

   Unlocks the specified recursive mutex. \n 
   More than one thread can be suspended on a recursive mutex. When the mutex is
   unlocked, only the highest-priority thread waiting on the mutex awakens. If the
   awakened thread has higher priority than the current thread, a context switch occurs.
  
   @datatypes
   #qurt_rmutex2_t
  
   @param[in]  lock  Pointer to the recursive mutex object to unlock. 

   @return
   None.

   @dependencies
   None.
  
 */
void qurt_rmutex2_unlock(qurt_rmutex2_t *lock);

/**@ingroup func_qurt_rmutex2_try_lock

   @deprecated use #qurt_rmutex_try_lock instead.

   Attempts to lock the specified recursive mutex.\n

   Non-blocking version of qurt_rmutex2_lock(). When a call to qurt_rmutex2_lock() 
   succeeds immediately, this function behaves similarly, returning 0 for success.
   When a call to qurt_rmutex2_lock() does not succeed immediately, this function has
   no effect and returns nonzero for failure.

   @datatypes
   #qurt_rmutex2_t
   
   @param[in]  lock  Pointer to the recursive mutex object to lock.

   @return 
   0 -- Success. \n 
   Nonzero -- Failure. 
  
 */
int qurt_rmutex2_try_lock(qurt_rmutex2_t *lock);
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_RMUTEX2_H */
