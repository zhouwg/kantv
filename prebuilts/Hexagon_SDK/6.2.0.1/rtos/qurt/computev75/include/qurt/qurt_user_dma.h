#ifndef QURT_USER_DMA_H
#define QURT_USER_DMA_H

/**
  @file qurt_user_dma.h
  @brief  Definitions, macros, and prototypes used for handling user DMA.

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

/**@ingroup qurt_user_dma_dmsyncht
  Sends the DMSyncht command to the user DMA engine.
   
   Call this function to ensure all posted DMA memory operations are
   complete. 
   
   This stalls the current thread until the instruction
   is complete and returns.

  @return
  QURT_EOK - On dmsyncht completion \n
  QURT_ENOTSUPPORTED - User DMA not supported
  
  @dependencies
  None.
*/
int qurt_user_dma_dmsyncht(void);

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif
