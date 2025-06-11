#ifndef QURT_ANYSIGNAL_H
#define QURT_ANYSIGNAL_H 
/**
  @file qurt_anysignal.h
  Prototypes of kernel signal API functions.      

 EXTERNALIZED FUNCTIONS
  None

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2021 Qualcomm Technologies, Inc.
All rights reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#include <qurt_signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=====================================================================
Typedefs
======================================================================*/

/**@ingroup anysignals_types                                                 
 qurt_signal_t supersedes qurt_anysignal_t. This type definition was added for backwards compatibility.  */  
typedef qurt_signal_t qurt_anysignal_t;

/*=====================================================================
 Functions
======================================================================*/
 
/*======================================================================*/
/**@ingroup func_qurt_anysignal_init
  Initializes an any-signal object.\n
  The any-signal object is initially cleared.

  @datatypes
  #qurt_anysignal_t

  @param[out] signal	Pointer to the initialized any-signal object.  
  
  @return         
  None.

  @dependencies  
  None.
 */
/* ======================================================================*/
static inline void qurt_anysignal_init(qurt_anysignal_t *signal)
{
  qurt_signal_init(signal);
}

/*======================================================================*/
/**@ingroup func_qurt_anysignal_destroy
  Destroys the specified any-signal object. 

  @note1hang Any-signal objects must be destroyed when they are no longer in use. Failure
             to do this causes resource leaks in the QuRT kernel.\n
  @note1cont Any-signal objects must not be destroyed while they are still in use. If this
             occurs, the behavior of QuRT is undefined.

  @datatypes
  #qurt_anysignal_t
  
  @param[in] signal Pointer to the any-signal object to destroy.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
static inline void qurt_anysignal_destroy(qurt_anysignal_t *signal)
{
  qurt_signal_destroy(signal);
}

/*======================================================================*/
/**@ingroup func_qurt_anysignal_wait
  Wait on the any-signal object. \n
  Suspends the current thread until any one of the specified signals is set.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be waited on, and 0 indicates not to wait on the signal.
  If a signal is set in an any-signal object, and a thread is waiting on the any-signal object for
  that signal, the thread is awakened. If the awakened thread has higher priority than
  the current thread, a context switch can occur.

  @note1hang At most, one thread can wait on an any-signal object at any given time.

  @datatypes
  #qurt_anysignal_t
	
  @param[in] signal Pointer to the any-signal object to wait on. 
  @param[in] mask   Signal mask value, which specifies the individual signals in the any-signal
                      object to wait on.

  @return 				
  Bitmask of current signal values.

  @dependencies
  None.
 */
/* ======================================================================*/
static inline unsigned int qurt_anysignal_wait(qurt_anysignal_t *signal, unsigned int mask)
{
  return qurt_signal_wait(signal, mask, QURT_SIGNAL_ATTR_WAIT_ANY);
}

/*======================================================================*/
/**@ingroup func_qurt_anysignal_set
  Sets signals in the specified any-signal object. \n
  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be set, and 0 indicates not to set the sigmal.

  @datatypes
  #qurt_anysignal_t
	
  @param[in] signal Pointer to the any-signal object to modify. 
  @param[in]  mask  Signal mask value identifying the individual signals to  
                       set in the any-signal object.

  @return 				
  Bitmask of old signal values (before set).

  @dependencies
  None.
 */
/* ======================================================================*/
unsigned int qurt_anysignal_set(qurt_anysignal_t *signal, unsigned int mask);



/*======================================================================*/
/**@ingroup func_qurt_anysignal_get
  Gets signal values from the any-signal object.\n
  Returns the current signal values of the specified any-signal object.

  @datatypes
  #qurt_anysignal_t
 	
  @param[in] signal Pointer to the any-signal object to access. 

  @return 				
  A bitmask with the current signal values of the specified any-signal object.

  @dependencies
  None.
 */
/* ======================================================================*/
static inline unsigned int qurt_anysignal_get(qurt_anysignal_t *signal)
{
  return qurt_signal_get(signal);
}


/*======================================================================*/
/**@ingroup func_qurt_anysignal_clear
   @xreflabel{sec:anysignal_clear}
  Clears signals in the specified any-signal object.\n
  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be cleared, and 0 indicates not to clear the signal.

  @datatypes
  #qurt_anysignal_t
	
  @param[in] signal Pointer to the any-signal object, which specifies the any-signal object to modify. 
  @param[in] mask   Signal mask value identifying the individual signals to  
                    clear in the any-signal object.
	
  @return 				
  Bitmask -- Old signal values (before clear). 

  @dependencies
  None.
 */
/* ======================================================================*/
unsigned int qurt_anysignal_clear(qurt_anysignal_t *signal, unsigned int mask);

/*======================================================================*/
/**@ingroup func_qurt_anysignal_wait_timed
  Waits on the any-signal object. \n
  Suspends the current thread until any of the specified signals is set or timeout expires.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be waited on, and 0 indicates not to wait on the signal.
  If a signal is set in an any-signal object, and a thread was waiting on the any-signal object for
  that signal, the thread is awakened. If the awakened thread has higher priority than
  the current thread, a context switch can occur.

  @note1hang At most, one thread can wait on an any-signal object at any given time.

  @datatypes
  #qurt_anysignal_t
	
  @param[in] signal Pointer to the any-signal object to wait on. 
  @param[in] mask   Signal mask value, which specifies the individual signals in the any-signal
                      object to wait on.
  @param[out] signals Bitmask of current signal values.
  @param[in] duration Interval (in microseconds) duration value must be between #QURT_TIMER_MIN_DURATION and
  #QURT_TIMER_MAX_DURATION.

  @return 				
   #QURT_EOK -- Success \n
   #QURT_ETIMEDOUT -- timeout
   #QURT_EINVALID -- Duration out of range

  @dependencies
  None.
 */
/* ======================================================================*/

int qurt_anysignal_wait_timed(qurt_anysignal_t *signal, unsigned int mask, unsigned int *signals, unsigned long long int duration);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ANYSIGNAL_H */
