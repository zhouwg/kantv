#ifndef QURT_ISR_H
#define QURT_ISR_H

/*=====================================================================
 
  @file  qurt_isr.h

  @brief  Prototypes of Qurt ISR API functions      

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2017, 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#include <string.h>
#include <qurt_thread.h>

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                            Functions
=============================================================================*/


/**@ingroup func_qurt_isr_set_hw_config_callback
  Set callback function for the configuration related to interrupt hardware.
  In a process, the callback function can only be set once.

  @param[in]   cb_addr      address of the callback function.
   
  @return 
  #QURT_EOK     -- the callback function is set succssfully. \n
  #QURT_EFAILED -- Failure. The callback function has been set before. 

  @dependencies
  None.
 */
int qurt_isr_set_hw_config_callback(unsigned int cb_addr);


/**@ingroup func_qurt_isr_set_hw_enable_callback
  Set callback function for enabling the configuration related to interrupt hardware.
  In a process, the callback function can only be set once.

  @param[in]   cb_addr      address of the callback function.
   
  @return 
  #QURT_EOK     -- the callback function is set succssfully. \n
  #QURT_EFAILED -- Failure. The callback function has been set before. 

  @dependencies
  None.
 */
int qurt_isr_set_hw_enable_callback(unsigned int cb_addr);


/**@ingroup func_qurt_isr_set_hw_disable_callback
  Set callback function for disabling the configuration related to interrupt hardware.
  In a process, the callback function can only be set once.

  @param[in]   cb_addr      address of the callback function.
   
  @return 
  #QURT_EOK     -- the callback function is set succssfully. \n
  #QURT_EFAILED -- Failure. The callback function has been set before. 

  @dependencies
  None.
 */
int qurt_isr_set_hw_disable_callback(unsigned int cb_addr);


/**@ingroup func_qurt_isr_create
  Creates an ISR thread with the specified attributes, and makes it executable.

  @datatypes
  #qurt_thread_t \n
  #qurt_thread_attr_t
  
  @param[out]  thread_id    Returns a pointer to the thread identifier if the thread was 
                             successfully created.
  @param[in]   attr 	    Pointer to the initialized thread attribute structure that specifies 
                             the attributes of the created thread.
   
  @return 
  #QURT_EVAL    -- Invalid arguments
  #QURT_EOK -- Thread created. \n
  #QURT_EFAILED -- Thread not created. 

  @dependencies
  None.
 */
int qurt_isr_create (qurt_thread_t *thread_id, qurt_thread_attr_t *pAttr);

/**@ingroup func_qurt_isr_register2
  Registers an Interrupt Service Routine to an ISR thread. ISR callback with the specified attributes.
  The interrupt is enabled when this function returns success.

  @datatypes
   qurt_thread_t
  
  @param[in]   isr_thread_id ISR thread ID, returned from qurt_isr_create()
  @param[in]   int_num       The interrupt number
  @param[in]   prio          Priority of the ISR
  @param[in]   flags         Defines ACK type. Values : \n
                             QURT_INT_NON_DELAYED_ACK - ISR is acknowledged by the interrupt handle routine 
			                                     in the Kernel.
                             QURT_INT_DELAYED_ACK     - Client chooses to acknowledge. 
  @param[in]   int_type.     Notifies it to registered function. Values: \n 
                             - QURT_INT_TRIGGER_USE_DEFAULT
                             - QURT_INT_TRIGGER_LEVEL_HIGH 
                             - QURT_INT_TRIGGER_LEVEL_LOW 
                             - QURT_INT_TRIGGER_RISING_EDGE 
                             - QURT_INT_TRIGGER_FALLING_EDGE              
                             - QURT_INT_TRIGGER_DUAL_EDGE              
  @param[in]   isr           Interrupt Service Routine with proto type void isr (void *arg, int int_num)
  @param[in]   arg  	     1st argument of the ISR when it is called to service the interrupt
   
  @return 
   QURT_EOK          -- Successfully registered the ISR for the interrupt
   QURT_EINT         -- Interrupt not configured
   QURT_EINVALID     -- Invalid Thread ID
   QURT_EDISABLED    -- The feature is disabled
   QURT_EDUPLICATE   -- Interrupt is already registered

  @dependencies
   Thread ID should be created using qurt_isr_create()
 */
int qurt_isr_register2 (qurt_thread_t isr_thread_id, int int_num, unsigned short prio, unsigned short flags, unsigned int int_type, void (*isr) (void *, int), void *arg);

/**@ingroup func_qurt_isr_deregister2
  De-registers the ISR for the specified interrupt.
  The interrupt is disabled when this function returns success.

  @param[in]   int_num   The interrupt number
   
  @return 
   QURT_EOK            -- ISR deregistered successfully
   QURT_ENOREGISTERED  -- Interrupt with int_num is not registered

  @dependencies
  None.
 */
int qurt_isr_deregister2 (int int_num);

/**@ingroup func_qurt_isr_delete
   ISR thread will exit and releases Kernel resources

   @note1hang   The ISR thread shouldn't be actively processing interrupts,
                otherwise the call will fail and return an error.
  
   @param[in]   thread-id of the ISR thread that needs to be deleted.

   @return
    QURT_ENOTALLOWED   -- ISR thread is processing an interrupt
    QURT_EINVALID      -- Invalid ISR thread ID
    QURT_EOK           -- Success 

   @dependencies
   Thread ID should be created using qurt_isr_create()
 */
int qurt_isr_delete (qurt_thread_t isr_tid);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ISR_H */


