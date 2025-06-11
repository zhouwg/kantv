#ifndef QURT_INT_H
#define QURT_INT_H
/**
  @file  qurt_int.h
  @brief  QuRT interrupt functions.    



 Copyright (c) 2013-2021, 2023 Qualcomm Technologies, Inc.
 All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                            CONSTANTS AND MACROS
=============================================================================*/


/** @cond rest_reg_dist */
/** @addtogroup interrupts_constants
@{ */
#define SIG_INT_ABORT 0x80000000                                       /**< */
#define QURT_INT_NON_DELAYED_ACK           0 
#define QURT_INT_DELAYED_ACK               1
#define QURT_INT_ACK_DEFAULT               QURT_INT_NON_DELAYED_ACK
#define QURT_INT_DRV_DEFAULT               0
#define QURT_INT_PRIORITY_DEFAULT          0xFF

/** QuRT interrupt property. */
#define QURT_INT_CONFIGID_POLARITY        0x1U /**< */
#define QURT_INT_CONFIGID_LOCK            0x2U /**< */

/** QuRT interrupt lock.*/
#define QURT_INT_LOCK_DEFAULT             0x0  /**< Default. */
#define QURT_INT_LOCK_DISABLE             0x0  /**< Interrupt can be enabled or disabled or deregistered. */
#define QURT_INT_LOCK_ENABLE              0x1  /**< Interrupt is locked and cannot be enabled, disabled, or deregistered.*/
/** @} */ /* end_addtogroup interrupts_constants */

/** @addtogroup Qurt_interrupt_type
@{ */
/** Trigger type bit fields for a PDC interrupt:\n
    @verbatim
    Polarity  Edge  Output\n
    0         00    Level sensitive active low
    0         01    Rising edge sensitive
    0         10    Falling edge sensitive
    0         11    Dual edge sensitive
    1         00    Level sensitive active high
    1         01    Falling edge sensitive
    1         10    Rising edge sensitive
    1         11    Dual edge sensitive 
    @endverbatim
*/
#define QURT_INT_TRIGGER_TYPE_SET(pol, edge)   ((((pol) & 0x01U) << 2) | ((edge) & 0x03U)) /**< */
	 
#define QURT_INT_TRIGGER_LEVEL_LOW     QURT_INT_TRIGGER_TYPE_SET(0U, 0x00U)  /**< */
#define QURT_INT_TRIGGER_LEVEL_HIGH    QURT_INT_TRIGGER_TYPE_SET(1U, 0x00U)  /**< */
#define QURT_INT_TRIGGER_RISING_EDGE   QURT_INT_TRIGGER_TYPE_SET(1U, 0x02U)  /**< */
#define QURT_INT_TRIGGER_FALLING_EDGE  QURT_INT_TRIGGER_TYPE_SET(0U, 0x02U)  /**< */
#define QURT_INT_TRIGGER_DUAL_EDGE     QURT_INT_TRIGGER_TYPE_SET(0U, 0x03U)  /**< */
#define QURT_INT_TRIGGER_USE_DEFAULT   0xffU                                 /**< */
/** @} */ /* end_addtogroup Qurt_interrupt_type */

/*=====================================================================
 Functions
======================================================================*/

/**@ingroup func_qurt_interrupt_register
  @xreflabel{sec:interrupt_register} 
  Registers the interrupt.\n
  Enables the specified interrupt and associates it with the specified QuRT signal object and
  signal mask.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be waited on, and 0 indicates not to wait.

  When the interrupt occurs, the signal specified in the signal mask is set in the signal
  object. An IST conventionally waits on that signal to
  handle the interrupt. The thread that registers the interrupt is set as the IST.

  Up to 31 separate interrupts can be registered to a single signal object, as determined by
  the number of individual signals the object can store. QuRT reserves signal 31. Thus a
  single IST can handle several different interrupts.

  QuRT reserves some interrupts for internal use -- the remainder are available for use by
  applications, and thus are valid interrupt numbers. If the specified interrupt number is
  outside the valid range, the register operation returns the status value QURT_EINT.

  Only one thread can be registered at a time to a specific interrupt. Attempting to register
  an already-registered interrupt returns the status value QURT_EVAL.

  Only one signal bit in a signal object can be registered at a time to a specific interrupt.
  Attempting to register multiple signal bits to an interrupt returns the status value
  QURT_ESIG.

  When the signal registers an interrupt, QuRT can only set its signal bits 
  when receiving the interrupt. The QuRT signal API from another
  software thread cannot set the signal even for unused signal bits.

  @note1hang The valid range for an interrupt number can differ on target execution
             environments other than the simulator. For more information, see the
             appropriate hardware document.
								 
  @datatypes
  #qurt_anysignal_t

  @param[in] int_num      L2VIC interrupt to deregister; valid range is 0 to 1023.
  @param[in] int_signal   Any-signal object to wait on (Section @xref{dox:any_signals}).
  @param[in] signal_mask  Signal mask value indicating signal to receive the interrupt.

   @return
   #QURT_EOK -- Interrupt successfully registered.\n
   #QURT_EINT -- Invalid interrupt number. \n
   #QURT_ESIG -- Invalid signal bitmask (cannot set more than one
                signal at a time). \n
   #QURT_EVAL -- Interrupt already registered.

   @dependencies
   None.
*/
 unsigned int qurt_interrupt_register(int int_num, qurt_anysignal_t *int_signal, int signal_mask);

/**@ingroup func_qurt_interrupt_register2
  @xreflabel{sec:interrupt_register2} 
  Registers the interrupt.\n
  Enables the specified interrupt, associates it with the specified QuRT signal object and
  signal mask, and sets interrupt flags.

  Signals are represented as bits 0 through 31 in the 32-bit mask value. A mask bit value of 1
  indicates that a signal must be waited on, and 0 indicates not to wait.

  When the interrupt occurs, the signal specified in the signal mask is set in the signal
  object. An IST conventionally waits on that signal to
  handle the interrupt. The thread that registers the interrupt is set as the IST.

  Up to 31 separate interrupts can be registered to a single signal object, as determined by
  the number of individual signals that the object can store. QuRT reserves signal 31. Thus a
  single IST can handle several different interrupts.

  QuRT reserves some interrupts for internal use -- the remainder are available for use by
  applications, and thus are valid interrupt numbers. If the specified interrupt number is
  outside the valid range, the register operation returns the status value #QURT_EINT.

  Only one thread can be registered at a time to a specific interrupt. Attempting to register
  an already-registered interrupt returns the status value #QURT_EVAL.

  Only one signal bit in a signal object can be registered at a time to a specific interrupt.
  Attempting to register multiple signal bits to an interrupt returns the status value
  #QURT_ESIG.

  When the signal registers an interrupt, QuRT can only set its signal bits 
  when receiving the interrupt. The QuRT signal API from another
  software thread cannot set the signal even for unused signal bits.

  @note1hang The valid range for an interrupt number can differ on target execution
             environments other than the simulator. For more information, see the
             appropriate hardware document.
								 
  @datatypes
  #qurt_anysignal_t

  @param[in] int_num      L2VIC interrupt to deregister; valid range is 0 to 1023.
  @param[in] int_signal   Any-signal object to wait on (Section @xref{dox:any_signals}).
  @param[in] signal_mask  Signal mask value indicating signal to receive the interrupt.
  @param[in] flags        Defines interrupt property, supported property is interrupt lock enable/disable. 
                          Possible values for flags: \n
                           - #QURT_INT_LOCK_ENABLE
                           - #QURT_INT_LOCK_DISABLE @tablebulletend

   @return
   #QURT_EOK -- Interrupt successfully registered.\n
   #QURT_EINT -- Invalid interrupt number. \n
   #QURT_ESIG -- Invalid signal bitmask (cannot set more than one
                signal at a time). \n
   #QURT_EVAL -- Interrupt already registered.

   @dependencies
   None.
*/
 unsigned int qurt_interrupt_register2(int int_num, qurt_anysignal_t *int_signal, int signal_mask, unsigned int flags);
/*
 * Waits for registered interrupt signal

 * Suspend the current thread until one of its registered interrupts occurs. The second input mask, 
 * contains the interrupt signals the IST expects to receive. The interrupt signals are registered 
 * with interrupts via qurt_register_interrupt API.
 *
 * The signals returned in the signal variable indicate which interrupts occurred. Use function 
 * qurt_anysignal_get to read the signals. IST must locally maintain a table that maps a signal to 
 * a specific interrupt. IST also checks if signal #SIG_INT_ABORT is received. If so, the IST 
 * must quit from interrupt receiving loop.
 *
 * For detail information on this API, see QuRT User Manual Section 4.2.5
 *
 * Prototype
 *
 * unsigned int qurt_anysignal_wait(qurt_anysignal_t *int_signal, unsigned int mask)
 */

/**@ingroup func_qurt_interrupt_acknowledge
  Acknowledges an interrupt after it has been processed.\n
  Re-enables an interrupt and clears its pending status. This is done after an interrupt is
  processed by an IST.

  Interrupts are automatically disabled after they occur. To re-enable an interrupt, an IST
  performs the acknowledge operation after it has finished processing the interrupt and
  just before suspending itself (such as by waiting on the interrupt signal).

  @note1hang To prevent losing or reprocessing subsequent occurrences of the interrupt,
           an IST must clear the interrupt signal (Section @xref{sec:anysignal_clear}) before
           acknowledging the interrupt.

  @param[in] int_num Interrupt that is being re-enabled.

  @return 
  #QURT_EOK -- Interrupt acknowledge was successful. \n
  #QURT_EDEREGISTERED -- Interrupt is already de-registered.

  @dependencies
  None.	
*/
int qurt_interrupt_acknowledge(int int_num);

/**@ingroup func_qurt_interrupt_deregister
  Disables the specified interrupt and disassociates it from a QuRT signal object.
  If the specified interrupt was never registered (Section @xref{sec:interrupt_register}), the deregister operation
  returns the status value #QURT_EINT.

  @note1hang If an interrupt is deregistered while an IST waits
             to receive it, the IST might wait indefinitely for the interrupt to occur. To avoid
             this problem, the QuRT kernel sends the signal #SIG_INT_ABORT to awaken an
             IST after determining that it has no interrupts registered.

  @param[in] int_num L2VIC to deregister; valid range is 0 to 1023.

  @return
  #QURT_EOK -- Success.\n
  #QURT_EINT -- Invalid interrupt number (not registered).

  @dependencies
  None.

*/
unsigned int qurt_interrupt_deregister(int int_num);
/** @endcond */

/**@ingroup func_qurt_interrupt_disable
  Disables an interrupt with its interrupt number.\n
  The interrupt must be registered prior to calling this function. 
  After qurt_interrupt_disable() returns, the Hexagon subsystem
  can no longer send the corresponding interrupt to the Hexagon
  core, until qurt_interrupt_enable() is called 
  for the same interrupt. 
  
  Avoid calling qurt_interrupt_disable() and qurt_interrupt_enable() frequently within 
  a short period of time.\n
  - A pending interrupt can already be in the Hexagon core when qurt_interrupt_disable() 
    is called. Therefore, some time later, the pending interrupt is received on a Hexagon 
    hardware thread.\n
  - After the Hexagon subsystem sends an interrupt to the Hexagon core, the Hexagon 
    hardware automatically disables the interrupt until kernel software re-enables the interrupt 
    at the interrupt acknowledgement stage. If qurt_interrupt_enable() is called from a certain 
    thread at an ealier time, the interrupt is re-enabled earlier and can trigger 
  sending a new interrupt to the Hexagon core while kernel software is still processing
  the previous interrupt.

  @param[in] int_num Interrupt number.

  @return
  #QURT_EOK  -- Interrupt successfully disabled.\n 
  #QURT_EINT -- Invalid interrupt number.\n
  #QURT_ENOTALLOWED -- Interrupt is locked. \n
  #QURT_EVAL -- Interrupt is not registered. 

  @dependencies
  None.
*/
 unsigned int qurt_interrupt_disable(int int_num);

 
/**@ingroup func_qurt_interrupt_enable
  Enables an interrupt with its interrupt number.\n
  The interrupt must be registered prior to calling this function. 

  @param[in] int_num Interrupt number.

  @return
  #QURT_EOK -- Interrupt successfully enabled.\n 
  #QURT_EINT -- Invalid interrupt number.\n
  #QURT_ENOTALLOWED -- Interrupt is locked. \n
  #QURT_EVAL -- Interrupt is not registered.

  @dependencies
  None.

*/
 unsigned int qurt_interrupt_enable(int int_num);


/**@ingroup func_qurt_interrupt_status
  Returns a value that indicates the pending status of the specified interrupt.

  @param[in]  int_num  Interrupt number that is being checked.
  @param[out] status   Interrupt status; 1 indicates that an interrupt is
                       pending, 0 indicates that an interrupt is not pending.
 
  @return 
  #QURT_EOK -- Success. \n
  #QURT_EINT -- Failure; invalid interrupt number.

  @dependencies
  None.
 */
unsigned int qurt_interrupt_status(int int_num, int *status);


/**@ingroup func_qurt_interrupt_get_status
  Gets the status of the specified interrupt in L2VIC.

  @param[in]  int_num  Interrupt number that is being checked.
  @param[in]  status_type     0 -- interrupt pending status \n
                              1 -- interrupt enabling status
  @param[out] status          0 -- OFF \n
                              1 -- ON
 
  @return 
  #QURT_EOK -- Success. \n
  #QURT_EINT -- Failure; invalid interrupt number.

  @dependencies
  None.
 */
unsigned int qurt_interrupt_get_status(int int_num, int status_type, int *status);

/** @cond rest_reg_dist */
/**@ingroup func_qurt_interrupt_clear
  Clears the pending status of the specified interrupt.

  @note1hang This operation is intended for system-level use, and must be used with care.
             
  @param[in] int_num Interrupt that is being re-enabled.
 
  @return 
  #QURT_EOK -- Success.\n
  #QURT_EINT -- Invalid interrupt number.
  
  @dependencies
  None.
 */
unsigned int qurt_interrupt_clear(int int_num);


/**@ingroup func_qurt_interrupt_get_config
  Gets the L2VIC interrupt configuration. \n
  This function returns the type and polarity of the specified L2VIC interrupt.

  @param[in]   int_num       L2VIC interrupt that is being re-enabled.
  @param[out]  int_type      Pointer to an interrupt type. \n
                             0 -- Level-triggered interrupt \n
                             1 -- Eedge-triggered interrupt
  @param[out]  int_polarity  Pointer to interrupt polarity.\n
                             0 -- Active-high interrupt \n
                             1 -- Active-low interrupt.
 
  @return 
  #QURT_EOK -- Configuration successfully returned.\n
  #QURT_EINT -- Invalid interrupt number. 

  @dependencies
  None.
 */
unsigned int qurt_interrupt_get_config(unsigned int int_num, unsigned int *int_type, unsigned int *int_polarity);

/**@ingroup func_qurt_interrupt_set_config
  Sets the type and polarity of the specified L2VIC interrupt.

  @note1hang Deregister L2VIC interrupts before reconfiguring them.

  @param[in] int_num        L2VIC interrupt that is being re-enabled.
  @param[in] int_type       Interrupt type. \n
                            0 -- Level-triggered interrupt\n
                            1 -- Edge-triggered interrupt
  @param[in] int_polarity   Interrupt polarity. \n
                            0 -- Active-high interrupt \n
                            1 -- Active-low interrupt
 
  @return
  #QURT_EOK -- Success. \n
  #QURT_ENOTALLOWED -- Not allowed; the interrupt is being registered.\n
  #QURT_EINT -- Invalid interrupt number.
  
  @dependencies
  None.
 */
unsigned int qurt_interrupt_set_config(unsigned int int_num, unsigned int int_type, unsigned int int_polarity);

/**@ingroup func_qurt_interrupt_set_config2
  Sets the type and polarity of the specified L2VIC interrupt.

  @note1hang L2VIC interrupts must be deregistered before they can be reconfigured.

  @param[in] int_num        L2VIC interrupt that is being re-enabled.
  @param[in] int_type       Notified to the hardware configuration callback function and used to 
                            modify the L2VIC type. Possible values: \n 
                            - #QURT_INT_TRIGGER_USE_DEFAULT \n 
                            - #QURT_INT_TRIGGER_LEVEL_HIGH  \n 
                            - #QURT_INT_TRIGGER_LEVEL_LOW  \n 
                            - #QURT_INT_TRIGGER_RISING_EDGE  \n 
                            - #QURT_INT_TRIGGER_FALLING_EDGE  \n              
                            - #QURT_INT_TRIGGER_DUAL_EDGE  @tablebulletend
 
  @return
  #QURT_EOK -- Success. \n
  #QURT_ENOTALLOWED -- Not allowed; the interrupt is being registered.\n
  #QURT_EINT -- Invalid interrupt number.
  
  @dependencies
  None.
 */
unsigned int qurt_interrupt_set_config2(unsigned int int_num, unsigned int int_type);

/**@ingroup func_ qurt_interrupt_set_config3
  Sets the specified configuration value for the specified property of the specified L2VIC interrupt.

  @note1hang L2VIC interrupts must be deregistered before they can be reconfigured for polarity.
    
  @param[in] int_num        L2VIC interrupt to re-enable.
  @param[in] config_id      Property to configure: \n
                            - #QURT_INT_CONFIGID_POLARITY \n
                            - #QURT_INT_CONFIGID_LOCK @tablebulletend
  @param[in] config_val    Dependent on the second argument config_id, specifies the value to set. \n
                           Values for #QURT_INT_CONFIGID_POLARITY: \n 
                            - #QURT_INT_TRIGGER_USE_DEFAULT \n
                            - #QURT_INT_TRIGGER_LEVEL_HIGH  \n
                            - #QURT_INT_TRIGGER_LEVEL_LOW \n
                            - #QURT_INT_TRIGGER_RISING_EDGE \n
                            - #QURT_INT_TRIGGER_FALLING_EDGE \n             
                            - #QURT_INT_TRIGGER_DUAL_EDGE \n

                           Values for #QURT_INT_CONFIGID_LOCK: \n
                            - #QURT_INT_LOCK_ENABLE\n
                            - #QURT_INT_LOCK_DISABLE @tablebulletend
          
  @return
  #QURT_EOK -- Success. \n
  #QURT_ENOTALLOWED -- Not allowed; the interrupt is being registered or is locked for enable/disable.\n
  #QURT_EINT -- Invalid interrupt number.

  @dependencies
  None.
*/
unsigned int qurt_interrupt_set_config3(unsigned int int_num, unsigned int config_id, unsigned int config_val);


/**@ingroup func_qurt_interrupt_raise
  Raises the interrupt. \n
  This function triggers a level-triggered L2VIC
  interrupt, and accepts interrupt numbers in the range of 0 to 1023.

  @param[in] interrupt_num Interrupt number.
  
  @return
  #QURT_EOK --  Success \n
  -1  --  Failure; the interrupt is not supported.

  @dependencies
  None.
 */
int qurt_interrupt_raise(unsigned int interrupt_num);

/**@ingroup func_qurt_interrupt_raise2
  Raises the interrupt and returns the current pcycle value.

  @param[in] interrupt_num Interrupt number.
  
  @return
  0xFFFFFFFFFFFFFFFF -- Failure; the interrupt is not supported.\n
  Other value        -- pcycle count at the time the interrupt is raised.

  @dependencies
  None.
 */
unsigned long long qurt_interrupt_raise2(unsigned int interrupt_num);
/** @endcond */

/** @cond internal_only */
/**@ingroup func_qurt_isr_subcall
  Indicates whether the current function is called from a callback procedure (either short or long).
  
  @return
  #QURT_EOK -- TRUE \n
  #QURT_EVAL -- FALSE.
  
  @dependencies
  None.
 */
int qurt_isr_subcall(void);
/** @endcond */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_INT_H */

