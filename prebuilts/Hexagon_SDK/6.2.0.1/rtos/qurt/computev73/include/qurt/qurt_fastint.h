#ifndef QURT_FASTINT_H
#define QURT_FASTINT_H

/**
  @file qurt_fastint.h
  @brief QuRT fast interrupt functions      

   Copyright (c) 2013-2021  by Qualcomm Technologies, Inc.  All Rights Reserved.

 ======================================================================*/

/*======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup func_qurt_fastint_register
  Register fast interrupt callback function

  Fast interrupt callback should be designed to perform the minimal necessary 
  actions for the interrupt, and/or perform some operations, such as signaling 
  another regular software thread to start any additional processing. 
  The callback should be a fast and short function. When a fast interrupt callback 
  is running, the corresponding interrupt cannot be re-enabled until the callback 
  returns. 

  The fast interrupt callback must not use any system blocking calls, such as 
  mutex lock or signal wait. Otherwise, it results in errors.

  The fast interrupt callback function has a single integer argument and the 
  function ends with no return. The argument value passed in is the interrupt
  number, and therefore a single callback function can handle 
  multiple fast interrupts.

  @param[in] intno  Interrupt number to register. 
  @param[in] fn     Interrupt callback function. 
    
  @return
  #QURT_EOK -- Fast interrupt registration is successful. \n
  #QURT_EINVALID -- Interrupt is already registered. \n
  #QURT_EINT -- Invalid interrupt number.    
*/
/* ======================================================================*/
unsigned int qurt_fastint_register(int intno, void (*fn)(int));


/*======================================================================*/
/**@ingroup func_qurt_fastint_deregister
  Deregisters the fast interrupt callback function. 
	
  @param[in] intno  Level-one interrupt number to deregister. Valid range is 1 and 10 through 31 
                    (simulator only). 

  @return 				
  #QURT_EOK -- Interrupt deregistration is successful. \n
  #QURT_EINT -- Invalid interrupt number (not registered). \n
  #QURT_EINVALID -- Invalid interrupt number (already deregistered).

  @dependencies
  None.
*/
/* ======================================================================*/
unsigned int qurt_fastint_deregister(int intno);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_FASTINT_H */

