#ifndef QURT_STID_H
#define QURT_STID_H
/**
  @file qurt_stid.h 
  Prototypes of software thread identifier(stid) interface APIs.  
  A stid is 8 bit identifier that can be assigned to a software thread.
  The performance monitor logic uses stid as a counting match criteria
  for maskable events. stid is also used by the hardware debugger 
  (ISDB) to match breakpoints. 

  EXTERNAL FUNCTIONS
   None.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

  Copyright (c) 2024 Qualcomm Technologies, Inc.
  All rights reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif


/*=============================================================================
                            FUNCTIONS
=============================================================================*/

/**@ingroup func_qurt_stid_alloc
  Allocate a unique stid 

  @param[in]  pid   Process identifier
  @param[out] stid  Pointer to a variable to return stid
 
  @return
  QURT_EOK - Allocation success
  QURT_ENORESOURCE  - No stid available for allocation
  QURT_EINVALID - Invalid input
   
  @dependencies
  None.
 */
int qurt_stid_alloc(unsigned int pid, unsigned int *stid);

/**@ingroup func_qurt_stid_release
   Release the stid. 


  @param[in]  pid   Process identifier
  @param[in]  stid  STID to release
  
  @note1hang 
  User shall ensure to clear the released stid from process or thread(s)
  to default value (QURT_STID_DEFAULT) before releasing that stid
 
  @return
  QURT_EOK - Release success
  QURT_ENOTALLOWED   - Operation not allowed for a pid
  QURT_EINVALID  - Invalid stid
   
  @dependencies
  None.
 */
int qurt_stid_release(unsigned int pid, unsigned int stid);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_STID_H */
