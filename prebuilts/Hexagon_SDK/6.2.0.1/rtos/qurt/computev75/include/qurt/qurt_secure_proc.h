#ifndef QURT_SECURE_PROC_H
#define QURT_SECURE_PROC_H

/**
  @file qurt_secure_proc.h
  @brief  Definitions, macros, and prototypes used for handling secure process

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2015, 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc.
 ======================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/**@ingroup qurt_process_migrate_secure_process
  Migrate the user process to Qurt secure process 
   
  @param secure_phy_address  Physical starting address of secure memory
  @param secure_memory_size  Size of secure memory
  @param entry               Entry function to secure process 

  @return
  EOK
  Negative return value -- Error.

  @dependencies
  None.
*/
int qurt_process_migrate_secure_process(unsigned long long secure_phy_address, unsigned int secure_memory_size,  void entry(unsigned));

/**@ingroup qurt_process_get_migration_mem_size
  get the size of all writable memory regions in a user PD. This is for preparation on secure process migration.
   
  @return
  size of all writable memory regions in a user PD.
 
  @dependencies
  None.
*/
int qurt_process_get_migration_mem_size(void);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
