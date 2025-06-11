#ifndef QURT_COND_H
#define QURT_COND_H 
/**
  @file qurt_cond.h
  @brief  Prototypes of kernel condition variable object API functions.

 EXTERNALIZED FUNCTIONS
  None

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021 Qualcomm Technologies, Inc.
 All rights reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/


#include <qurt_mutex.h>
#include <qurt_rmutex2.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup condition_variables_types
@{ */
/*=====================================================================
 Typedefs
 ======================================================================*/

/** QuRT condition variable type.  */
typedef union {
    /** @cond */
	unsigned long long raw;
	struct {
		unsigned int count;
		unsigned int n_waiting;
        unsigned int queue;
        unsigned int reserved;
	}X;
    /** @endcond */
} qurt_cond_t;

/** @} */ /* end_addtogroup condition_variables_types */

/*=====================================================================
 Functions
======================================================================*/
 
/*======================================================================*/
/**@ingroup func_qurt_cond_init
  Initializes a conditional variable object.

  @datatypes
  #qurt_cond_t
	
  @param[out] cond Pointer to the initialized condition variable object. 

  @return
  None.
		 
  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_cond_init(qurt_cond_t *cond);

/*======================================================================*/
/**@ingroup func_qurt_cond_destroy
  Destroys the specified condition variable.

  @note1hang Conditions must be destroyed when they are no longer in use. Failure to do
             this causes resource leaks in the QuRT kernel.\n
  @note1cont Conditions must not be destroyed while they are still in use. If this occurs,
             the behavior of QuRT is undefined. 

  @datatypes
  #qurt_cond_t

  @param[in] cond Pointer to the condition variable object to destroy.

  @return
  None.

 */
/* ======================================================================*/
void qurt_cond_destroy(qurt_cond_t *cond);

/*======================================================================*/
/**@ingroup func_qurt_cond_signal
  Signals a waiting thread that the specified condition is true. \n

  When a thread wishes to signal that a condition is true on a shared data item, it must
  perform the following procedure: \n
  -# Lock the mutex that controls access to the data item. \n
  -# Perform the signal condition operation. \n
  -# Unlock the mutex.

  @note1hang Failure to properly lock and unlock a mutex of a condition variable can cause
             the threads to never be suspended (or suspended but never awakened). 

  @note1cont Use condition variables only with regular mutexes -- attempting to use
             recursive mutexes or priority inheritance mutexes results in undefined behavior.
             
  @datatypes
  #qurt_cond_t

  @param[in] cond Pointer to the condition variable object to signal.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_cond_signal(qurt_cond_t *cond);

/*======================================================================*/
/**@ingroup func_qurt_cond_broadcast
  Signals multiple waiting threads that the specified condition is true.\n
  When a thread wishes to broadcast that a condition is true on a shared data item, it must
  perform the following procedure: \n
  -# Lock the mutex that controls access to the data item. \n
  -# Perform the broadcast condition operation. \n
  -# Unlock the mutex.\n

  @note1hang Failure to properly lock and unlock the mutex of a condition variable can cause
             the threads to never be suspended (or suspended but never awakened).

  @note1cont Use condition variables only with regular mutexes -- attempting to use
  recursive mutexes or priority inheritance mutexes results in undefined behavior.
  
  @datatypes
  #qurt_cond_t

  @param[in] cond Pointer to the condition variable object to signal.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_cond_broadcast(qurt_cond_t *cond);

/*======================================================================*/
/**@ingroup func_qurt_cond_wait
  Suspends the current thread until the specified condition is true.
  When a thread wishes to wait for a specific condition on a shared data item, it must
  perform the following procedure: \n
  -# Lock the mutex that controls access to the data item. \n
  -# If the condition is not satisfied, perform the wait condition operation on the
  condition variable (suspends the thread and unlocks the mutex).

  @note1hang Failure to properly lock and unlock the mutex of a condition variable can cause
             the threads to never be suspended (or suspended but never awakened).

  @note1cont Use condition variables only with regular mutexes -- attempting to use
  recursive mutexes or priority inheritance mutexes results in undefined behavior.
  
  @datatypes
  #qurt_cond_t \n
  #qurt_mutex_t
  
  @param[in] cond     Pointer to the condition variable object to wait on.
  @param[in] mutex    Pointer to the mutex associated with condition variable to wait on.

  @return
  None.
		 
  @dependencies 
  None.
 */
/* ======================================================================*/
void qurt_cond_wait(qurt_cond_t *cond, qurt_mutex_t *mutex);

/*======================================================================*/
/**@ingroup func_qurt_cond_wait2
  Suspends the current thread until the specified condition is true.
  When a thread wishes to wait for a specific condition on a shared data item, it must
  perform the following procedure: \n
  -# Lock the mutex that controls access to the data item. \n
  -# If the condition is not satisfied, perform the wait condition operation on the
  condition variable, which suspends the thread and unlocks the mutex.
 
  @note1hang Failure to properly lock and unlock the mutex of a condition variable can cause
             the threads to never be suspended (or suspended but never awakened). 

  @note1cont Use condition variables only with regular mutexes -- attempting to use
             recursive mutexes or priority inheritance mutexes results in undefined behavior.
             
  @note1cont This is the same API as qurt_cond_wait(), use this version 
             when using mutexes of type #qurt_rmutex2_t.

  @datatypes
  #qurt_cond_t \n
  #qurt_rmutex2_t
  
  @param[in] cond     Pointer to the condition variable object to wait on.
  @param[in] mutex    Pointer to the mutex associated with the condition variable to wait on.

  @return
  None.
		 
  @dependencies 
  None.
 */
/* ======================================================================*/
void qurt_cond_wait2(qurt_cond_t *cond, qurt_rmutex2_t *mutex);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_COND_H */

