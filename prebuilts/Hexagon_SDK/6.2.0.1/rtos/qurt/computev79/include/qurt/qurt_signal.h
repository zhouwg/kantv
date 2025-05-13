#ifndef QURT_SIGNAL_H
#define QURT_SIGNAL_H

/**
  @file qurt_signal.h
  @brief  Prototypes of kernel signal API functions. 

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup signals_types
@{ */
#define QURT_SIGNAL_ATTR_WAIT_ANY 0x00000000  /**< Wait any. */
#define QURT_SIGNAL_ATTR_WAIT_ALL 0x00000001  /**< Wait all. */

/*=====================================================================
 Typedefs
 ======================================================================*/


/** QuRT signal type.                                           
 */
typedef union {
    /** @cond */
	unsigned long long int raw;
	struct {
		unsigned int signals;
		unsigned int waiting;
		unsigned int queue;
		unsigned int attribute;
	}X;
    /** @endcond */
} qurt_signal_t;


/** QuRT 64-bit signal type.                                           
 */
typedef struct {
    /** @cond */
    qurt_signal_t signal_sum;
    unsigned long long signals;
    unsigned long long waiting;
    /** @endcond */
} qurt_signal_64_t;
/** @} */ /* end_addtogroup signals_types */
/*=====================================================================
 Functions
======================================================================*/
 
/*======================================================================*/
/**@ingroup func_qurt_signal_init
  Initializes a signal object.
  Signal returns the initialized object.
  The signal object is initially cleared.

  @note1hang   Each signal-based object has one or more kernel resources associated with it;
               to prevent resource leaks, call qurt_signal_destroy()
               when this object is not used anymore
  @datatypes
  #qurt_signal_t

  @param[in] *signal Pointer to the initialized object.

  @return         
  None.
     
  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal_init(qurt_signal_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal_destroy
  Destroys the specified signal object.
  
  @note1hang Signal objects must be destroyed when they are no longer in use. Failure 
  to do this causes resource leaks in the QuRT kernel.\n
  @note1cont Signal objects must not be destroyed while they are still in use. If this
  occurs, the behavior of QuRT is undefined.
  
  @datatypes
  #qurt_signal_t

  @param[in] *signal  Pointer to the signal object to destroy.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal_destroy(qurt_signal_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal_wait 
  @xreflabel{hdr:qurt_signal_wait}
  Suspends the current thread until the specified signals are set.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 indicates 
  waiting on a signal, and 0 indicates not waiting on the signal.

  If a thread is waiting on a signal object for any of the specified set of signals to set, 
  and one or more of those signals is set in the signal object, the thread is awakened.

  If a thread is waiting on a signal object for all of the specified set of signals to be set, 
  and all of those signals are set in the signal object, the thread is awakened.

  The specified set of signals can be cleared when the signal is set.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to 
                         wait on.
  @param[in] attribute   Indicates whether the thread waits to set any of the signals, or to set all of 
                         them. \n
						 @note1hang The wait-any and wait-all types are mutually exclusive.\n Values:\n
                         - #QURT_SIGNAL_ATTR_WAIT_ANY \n
                         - #QURT_SIGNAL_ATTR_WAIT_ALL @tablebulletend

  @return     	
  A 32-bit word with current signals.
 
  @dependencies
  None.
*/
/* ======================================================================*/
unsigned int qurt_signal_wait(qurt_signal_t *signal, unsigned int mask, 
                unsigned int attribute);

/*======================================================================*/
/**@ingroup func_qurt_signal_wait_timed
  @xreflabel{hdr:qurt_signal_wait}
  Suspends the current thread until the specified signals are set or until timeout.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 indicates 
  waiting on a signal, and 0 indicates not waiting.

  If a thread is waiting on a signal object for any of the specified set of signals to be set, 
  and one or more of those signals is set in the signal object, the thread is awakened.

  If a thread is waiting on a signal object for all of the specified set of signals to be set, 
  and all of those signals are set in the signal object, the thread is awakened.

  The specified set of signals can be cleared after the signal is set.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value that identifies the individual signals in the signal object to wait on.
  @param[in] attribute   Indicates whether the thread must wait until any of the signals are set, or until all of 
                         them are set. \n
						 @note1hang The wait-any and wait-all types are mutually exclusive.\n Values:\n
                         - #QURT_SIGNAL_ATTR_WAIT_ANY \n
                         - #QURT_SIGNAL_ATTR_WAIT_ALL @tablebulletend
  @param[out] signals    Bitmask of signals that are set 
  @param[in] duration    Duration (microseconds) to wait. Must be in the range
                         [#QURT_TIMER_MIN_DURATION ... #QURT_TIMER_MAX_DURATION]

  @return 				
  #QURT_EOK -- Success; one or more signals were set \n
  #QURT_ETIMEDOUT -- Timed-out \n
  #QURT_EINVALID -- Duration out of range
  
  @dependencies
  Timed-waiting support in the kernel.
*/
/* ======================================================================*/
int qurt_signal_wait_timed(qurt_signal_t *signal, unsigned int mask, 
                unsigned int attribute, unsigned int *signals, unsigned long long int duration);

/*======================================================================*/
/**@ingroup func_qurt_signal_wait_any
  Suspends the current thread until any of the specified signals are set.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 indicates
  to wait on a signal, and 0 indicates not to wait on the thread.

  If a thread is waiting on a signal object for any of the specified set of signals to be set, 
  and one or more of those signals is set in the signal object, the thread is awakened.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to  
                         wait on.
	
  @return     	
  32-bit word with current signals.
 
  @dependencies
  None.
*/
/* ======================================================================*/
static inline unsigned int qurt_signal_wait_any(qurt_signal_t *signal, unsigned int mask)
{
  return qurt_signal_wait(signal, mask, QURT_SIGNAL_ATTR_WAIT_ANY);
}

/*======================================================================*/
/**@ingroup func_qurt_signal_wait_all
  Suspends the current thread until all of the specified signals are set.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 indicates 
  to wait on a signal, and 0 indicates not to wait on it.

  If a thread is waiting on a signal object for all of the specified set of signals to be set, 
  and all of those signals are set in the signal object, the thread is awakened.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal_t

  @param[in] signal      Pointer to the signal object to wait on. 
  @param[in] mask        Mask value identifying the individual signals in the signal object to  
                         wait on.
	
  @return      	
  A 32-bit word with current signals.
 
  @dependencies
  None.
*/
/* ======================================================================*/
static inline unsigned int qurt_signal_wait_all(qurt_signal_t *signal, unsigned int mask)
{
  return qurt_signal_wait(signal, mask, QURT_SIGNAL_ATTR_WAIT_ALL);
}

/*======================================================================*/
/**@ingroup func_qurt_signal_set
  Sets signals in the specified signal object.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 indicates 
  to set the signal, and 0 indicates not to set it.
  	
  @datatypes
  #qurt_signal_t

  @param[in]    signal  Pointer to the signal object to modify.
  @param[in]    mask    Mask value identifying the individual signals to set in the signal
                        object.

  @return 
  None.
  
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_signal_set(qurt_signal_t *signal, unsigned int mask);

/*======================================================================*/
/**@ingroup func_qurt_signal_get
   Gets a signal from a signal object.
   
   Returns the current signal values of the specified signal object.

  @datatypes
  #qurt_signal_t

  @param[in] *signal Pointer to the signal object to access.

  @return         
  A 32-bit word with current signals
    
  @dependencies
  None.
*/
/* ======================================================================*/
unsigned int qurt_signal_get(qurt_signal_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal_clear
  Clear signals in the specified signal object.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 
  indicates that a signal must be cleared, and 0 indicates not to clear it.

  @note1hang Signals must be explicitly cleared by a thread when it is awakened -- the wait 
           operations do not automatically clear them.

  @datatypes
  #qurt_signal_t

  @param[in] signal   Pointer to the signal object to modify.
  @param[in] mask     Mask value identifying the individual signals to clear in the signal object.

  @return 		  
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal_clear(qurt_signal_t *signal, unsigned int mask);

/**@ingroup func_qurt_signal_wait_cancellable  
  @xreflabel{hdr:qurt_signal_wait_cancellable}
  Suspends the current thread until either the specified signals are set or the wait operation is cancelled.
  The operation is cancelled if the user process of the calling thread is killed, or if the calling thread 
  must finish its current QDI invocation and return to user space. 

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1 indicates 
  that a signal must be waited on, and 0 indicates not to wait on it.

  If a thread is waiting on a signal object for any of the specified set of signals to be set, and one or 
  more of those signals is set in the signal object, the thread is awakened.

  If a thread is waiting on a signal object for all of the specified set of signals to be set, and all of 
  those signals are set in the signal object, the thread is awakened.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @note1cont When the operation is cancelled, the caller must assume that the signal is never set.

  @datatypes
  #qurt_signal_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to  
                         wait on.
  @param[in] attribute   Indicates whether the thread must wait until any of the signals are set, or until all of 
                         them are set.  Values:\n
                         - #QURT_SIGNAL_ATTR_WAIT_ANY \n
                         - #QURT_SIGNAL_ATTR_WAIT_ALL @tablebulletend
  @param[out] return_mask Pointer to the 32-bit mask value that was originally passed to the function.


  @return     	
  #QURT_EOK -- Wait completed. \n
  #QURT_ECANCEL -- Wait cancelled.

 
  @dependencies
  None.
*/
/* ======================================================================*/
int qurt_signal_wait_cancellable(qurt_signal_t *signal, unsigned int mask, 
                                 unsigned int attribute,
                                 unsigned int *return_mask);

/*======================================================================*/
/**@ingroup func_qurt_signal_64_init
  Initializes a 64-bit signal object.\n
  The signal argument returns the initialized object.
  The signal object is initially cleared.

  @note1hang   Each signal-based object has one or more kernel resources associated with it;
               to prevent resource leaks, call qurt_signal_destroy()
               when this object is not used anymore.
  @datatypes
  #qurt_signal_64_t

  @param[in] signal Pointer to the initialized object.

  @return         
  None.
     
  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal_64_init(qurt_signal_64_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal_64_destroy
  Destroys the specified signal object.
  
  @note1hang 64-bit signal objects must be destroyed when they are no longer in use. Failure 
  to do this causes resource leaks in the QuRT kernel.\n
  @note1cont Signal objects must not be destroyed while they are still in use. If this
  occurs, the behavior of QuRT is undefined.
  
  @datatypes
  #qurt_signal_64_t

  @param[in] signal  Pointer to the signal object to destroy.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal_64_destroy(qurt_signal_64_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal_64_wait
  Suspends the current thread until all of the specified signals are set.

  Signals are represented as bits 0 through 63 in the 64-bit mask value. A mask bit value of 1 indicates 
  that a signal must be waited on, and 0 indicates not wait on it.

  If a thread is waiting on a signal object for all of the specified set of signals to be set, 
  and all of those signals are set in the signal object, the thread is awakened.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal_64_t

  @param[in] signal      Pointer to the signal object to wait on. 
  @param[in] mask        Mask value, which identifies the individual signals in the signal object to  
                         wait on.
  @param[in] attribute   Indicates whether the thread must wait until any of the signals are set, or until all of 
                         them are set.  \n
						 @note1hang The wait-any and wait-all types are mutually exclusive.\n Values:\n
                         - #QURT_SIGNAL_ATTR_WAIT_ANY \n
                         - #QURT_SIGNAL_ATTR_WAIT_ALL @tablebulletend
	
  @return      	
  A 32-bit word with current signals.
 
  @dependencies
  None.
*/
/* ======================================================================*/
unsigned long long qurt_signal_64_wait(qurt_signal_64_t *signal, unsigned long long mask, 
                unsigned int attribute);

/*======================================================================*/
/**@ingroup func_qurt_signal_64_set
  Sets signals in the specified signal object.

  Signals are represented as bits 0 through 63 in the 64-bit mask value. A mask bit value of 1 indicates 
  that a signal must be set, and 0 indicates not to set it.
  	
  @datatypes
  #qurt_signal_64_t

  @param[in]    signal  Pointer to the signal object to modify.
  @param[in]    mask    Mask value identifiying the individual signals to set in the signal
                        object.

  @return 
  None.
  
  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_signal_64_set(qurt_signal_64_t *signal, unsigned long long mask);

/*======================================================================*/
/**@ingroup func_qurt_signal_64_get
   Gets a signal from a signal object.
   
   Returns the current signal values of the specified signal object.

  @datatypes
  #qurt_signal_64_t

  @param[in] *signal Pointer to the signal object to access.

  @return         
  A 64-bit double word with current signals.
    
  @dependencies
  None.
*/
/* ======================================================================*/
unsigned long long qurt_signal_64_get(qurt_signal_64_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal_64_clear
  Clears signals in the specified signal object.

  Signals are represented as bits 0 through 63 in the 64-bit mask value. A mask bit value of 1 
  indicates that a signal must be cleared, and 0 indicates not to clear it.

  @note1hang Signals must be explicitly cleared by a thread when it is awakened -- the wait 
           operations do not automatically clear them.

  @datatypes
  #qurt_signal_64_t

  @param[in] signal   Pointer to the signal object to modify.
  @param[in] mask     Mask value identifying the individual signals to clear in the signal object.

  @return 		  
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal_64_clear(qurt_signal_64_t *signal, unsigned long long mask);

#ifdef __cplusplus
}
#endif

#endif /* QURT_SIGNAL_H */
