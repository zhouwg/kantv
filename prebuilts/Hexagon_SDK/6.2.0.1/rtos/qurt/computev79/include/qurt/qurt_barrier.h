#ifndef QURT_BARRIER_H
#define QURT_BARRIER_H

/**
  @file qurt_barrier.h
  @brief Prototypes of Kernel barrier API functions.      

 EXTERNALIZED FUNCTIONS
 None.

 INITIALIZATION AND SEQUENCING REQUIREMENTS
 None.

 Copyright (c) 2021 Qualcomm Technologies, Inc. All rights reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup barrier_types
@{ */
/*=====================================================================
 Constants and macros
======================================================================*/
#define QURT_BARRIER_SERIAL_THREAD 1 /**< Serial thread. */
#define QURT_BARRIER_OTHER 0         /**< Other. */

#ifndef ASM
#include <qurt_mutex.h>

/*=====================================================================
Typedefs
======================================================================*/

/** QuRT barrier type.                                                 
 */
typedef union {
    /** @cond */
	struct {
        unsigned short threads_left;
		unsigned short count;
		unsigned int threads_total;
        unsigned int queue;
        unsigned int reserved;
	};
	unsigned long long int raw;
    /** @endcond */
} qurt_barrier_t;

/** @} */ /* end_addtogroup barrier_types */

/*=====================================================================
 Functions
======================================================================*/
 
/*======================================================================*/
/**@ingroup func_qurt_barrier_init
  Initializes a barrier object.
	
  @datatypes
  #qurt_barrier_t

  @param[out] barrier       Pointer to the barrier object to initialize.
  @param[in]  threads_total Total number of threads to synchronize on the barrier.


  @return
  Unused integer value.

  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_barrier_init(qurt_barrier_t *barrier, unsigned int threads_total);

/*======================================================================*/
/**@ingroup func_qurt_barrier_destroy
  Destroys the specified barrier.

  @note1hang Barriers must be destroyed when they are no longer in use. Failure
             to do this causes resource leaks in the QuRT kernel.\n
  @note1cont Barriers must not be destroyed while they are still in use. If this
             occurs, the behavior of QuRT is undefined.

  @datatypes
  #qurt_barrier_t
 
  @param[in] barrier Pointer to the barrier object to destroy.

  @return     		
  Unused integer value.

  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_barrier_destroy(qurt_barrier_t *barrier);

/*======================================================================*/
/**@ingroup func_qurt_barrier_wait
  Waits on the barrier.\n
  Suspends the current thread on the specified barrier. \n
  The function return value indicates whether the thread was the last one to
  synchronize on the barrier.
  When a thread waits on a barrier, it is suspended on the barrier: \n
  - If the total number of threads waiting on the barrier is less than the assigned value 
     of the barrier, no other action occurs. \n
  - If the total number of threads waiting on the barrier equals the assigned value of the
     barrier, all threads currently waiting on the barrier are awakened, allowing them to
     execute past the barrier.

  @note1hang After its waiting threads are awakened, a barrier is automatically reset 
            and can be used again in the program without the need for re-initialization.
	                
  @datatypes
  #qurt_barrier_t
  
  @param[in] barrier Pointer to the barrier object to wait on.

  @return 				
  #QURT_BARRIER_OTHER -- Current thread awakened from barrier. \n 
  #QURT_BARRIER_SERIAL_THREAD -- Current thread is last caller of barrier.

  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_barrier_wait(qurt_barrier_t *barrier);


#endif

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_BARRIER_H */

