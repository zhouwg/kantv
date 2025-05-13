#ifndef QURT_SIGNAL2_H
#define QURT_SIGNAL2_H

/**
  @file qurt_signal2.h
  @brief  Prototypes of kernel signal2 API functions.

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2013, 2021, 2023  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

#define QURT_SIGNAL_ATTR_WAIT_ANY 0x00000000
#define QURT_SIGNAL_ATTR_WAIT_ALL 0x00000001

/*=====================================================================
 Typedefs
 ======================================================================*/

/** @addtogroup signals2_types
@{ */
/** qurt_signal2 type.
 */
typedef union {
   /** @cond */
  struct{
   unsigned int cur_mask;                              /* Current set of signal bits that are set. */
   unsigned int sig_state;                             /* Current state. */
                                                       /* Bit 0 -- in anysignal wait. */
                                                       /* Bit 1 -- in allsignal wait. */
                                                       /* Bit 2 -- in interrupt wait. */
                                                       /* Bits 31-3 -- reference count field. */
   unsigned int queue;                                 /* Kernel-maintained futex queue value. */
   unsigned int wait_mask;                             /* When sig_state indicates a waiter is present, this is the wait mask. */
   };
  unsigned long long int raw;
  /** @endcond */
} qurt_signal2_t;
/* @} */ /* end_addtogroup signals2_types */

/*=====================================================================
 Functions
======================================================================*/

/*======================================================================*/
/**@ingroup func_qurt_signal2_init

  @deprecated use #qurt_signal_init instead.

  Initializes a signal2 object.
  Signal returns the initialized object.
  The signal object is initially cleared.

  Objects of type signal2 solve a potential race condition between
  set() and destroy() operations.

  @datatypes
  #qurt_signal2_t

  @param[in] *signal Pointer to the initialized object.

  @return
  None.

  @dependencies
  Each mutex-based object has an associated
       kernel resource(s), therefore users must call qurt_signal2_destroy()
       when this object no longer in use.
 */
/* ======================================================================*/
void qurt_signal2_init(qurt_signal2_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal2_destroy

  @deprecated use #qurt_signal_destroy instead.

  Destroys the specified signal object.

  @note1cont Signal objects must not be destroyed while they are still in use. If this
  occurs, the behavior of QuRT is undefined.
  @note1cont Application code should destroy a signal2 object prior to deallocating it.
             Calling qurt_signal2_destroy() before deallocating a 
             signal2 object ensures completion of all qurt_signal2_set() calls.

  @datatypes
  #qurt_signal2_t

  @param[in] signal  Pointer to the signal object to destroy.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal2_destroy(qurt_signal2_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal2_wait

  @deprecated use #qurt_signal_wait instead.

  Suspends the current thread until the specified signals are set.

  Signals are represented as bits [31:0] in the 32-bit mask value. A mask bit value of 1 indicates
  a signal to wait on.

  If a thread calls this API with QURT_SIGNAL_ATTR_WAIT_ANY, the thread will be awakened when
  any of the signals specified in the mask are set.

  If a thread calls this API with QURT_SIGNAL_ATTR_WAIT_ALL, the thread will be awakened only
  when all the signals specified in the mask are set.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal2_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to wait on.
  @param[in] attribute   Specifies whether the thread waits for any of the signals to be set, or for all of
                         them to be set. Values:\n
                         - QURT_SIGNAL_ATTR_WAIT_ANY \n
                         - QURT_SIGNAL_ATTR_WAIT_ALL @tablebulletend
  @return
  A 32-bit word with current signals.

  @dependencies
  None.
*/
/* ======================================================================*/
unsigned int qurt_signal2_wait(qurt_signal2_t *signal, unsigned int mask,
                unsigned int attribute);

/*======================================================================*/
/**@ingroup func_qurt_signal2_wait_any

  @deprecated use #qurt_signal_wait_any instead.

  Suspends the current thread until any of the specified signals are set.

  Signals are represented as bits [31:0] in the 32-bit mask value. A mask bit value of 1 indicates
  a signal to wait on.

  The thread will be awakened when any of the signals specified in the mask are set.

  @note1hang At most, one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal2_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to 
                         wait on.

  @return
  32-bit word with current signals.

  @dependencies
  None.
*/
/* ======================================================================*/
static inline unsigned int qurt_signal2_wait_any(qurt_signal2_t *signal, unsigned int mask)
{
  return qurt_signal2_wait(signal, mask, QURT_SIGNAL_ATTR_WAIT_ANY);
}

/*======================================================================*/
/**@ingroup func_qurt_signal2_wait_all

  @deprecated use #qurt_signal_wait_all instead.

  Suspends the current thread until all of the specified signals are set.

  Signals are represented as bits [31:0] in the 32-bit mask value. A mask bit value of 1 indicates
  a signal to wait on.

  The thread will be awakened only when all the signals specified in the mask are set.

  @note1hang At most one thread can wait on a signal object at any given time.

  @datatypes
  #qurt_signal2_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to 
                         wait on.

  @return
  32-bit word with current signals.

  @dependencies
  None.
*/
/* ======================================================================*/
static inline unsigned int qurt_signal2_wait_all(qurt_signal2_t *signal, unsigned int mask)
{
  return qurt_signal2_wait(signal, mask, QURT_SIGNAL_ATTR_WAIT_ALL);
}

/*======================================================================*/
/**@ingroup func_qurt_signal2_set

  @deprecated use #qurt_signal_set instead.

  Sets signals in the specified signal object.

  Signals are represented as bits [31:0] in the 32-bit mask value. A mask bit value of 1 indicates
  that a signal must be set, and 0 indicates not to set the signal.

  @datatypes
  #qurt_signal2_t

  @param[in]    signal  Pointer to the signal object to modify.
  @param[in]    mask    Mask value identifying the individual signals to set in the signal
                        object.

  @return
  None.

  @dependencies
  None.
*/
/* ======================================================================*/
void qurt_signal2_set(qurt_signal2_t *signal, unsigned int mask);

/*======================================================================*/
/**@ingroup func_qurt_signal2_get

  @deprecated use #qurt_signal_get instead.

   Gets a signal from a signal object.

   Returns the current signal values of the specified signal object.

  @datatypes
  #qurt_signal2_t

  @param[in] *signal Pointer to the signal object to access.

  @return
   32-bit word with current signals.

  @dependencies
  None.
*/
/* ======================================================================*/
unsigned int qurt_signal2_get(qurt_signal2_t *signal);

/*======================================================================*/
/**@ingroup func_qurt_signal2_clear

  @deprecated use #qurt_signal_clear instead.

  Clear signals in the specified signal object.

  Signals are represented as bits [31:0] in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be cleared, and 0 indicates not to clear the signal.

  @note1hang Signals must be explicitly cleared by a thread when it is awakened -- the wait
           operations do not automatically clear them.

  @datatypes
  #qurt_signal2_t

  @param[in] signal   Pointer to the signal object to modify.
  @param[in] mask     Mask value identifying the individual signals to clear in the signal object.

  @return
  None.

  @dependencies
  None.
 */
/* ======================================================================*/
void qurt_signal2_clear(qurt_signal2_t *signal, unsigned int mask);

/**@ingroup func_qurt_signal2_wait_cancellable  
  
  @deprecated use #qurt_signal_wait_cancellable instead.

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
  #qurt_signal2_t

  @param[in] signal      Pointer to the signal object to wait on.
  @param[in] mask        Mask value identifying the individual signals in the signal object to  
                         wait on.
  @param[in] attribute   Indicates whether the thread must wait until any of the signals are set, or until all of 
                         them are set.  Values:\n
                         - #QURT_SIGNAL_ATTR_WAIT_ANY \n
                         - #QURT_SIGNAL_ATTR_WAIT_ALL @tablebulletend
  @param[out] p_returnmask Pointer to the 32-bit mask value that was originally passed to the function.


  @return     	
  #QURT_EOK -- Wait completed. \n
  #QURT_ECANCEL -- Wait cancelled.

 
  @dependencies
  None.
*/
int qurt_signal2_wait_cancellable(qurt_signal2_t *signal,
                                  unsigned int mask,
                                  unsigned int attribute,
                                  unsigned int *p_returnmask);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_SIGNAL2_H */
