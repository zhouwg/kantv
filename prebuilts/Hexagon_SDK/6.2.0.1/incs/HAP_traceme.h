#ifndef HAP_TRACEME_H
#define HAP_TRACEME_H
/*==============================================================================
  Copyright (c) 2012-2013 Qualcomm Technologies Incorporated.
  All Rights Reserved Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S.
  Government. Diversion contrary to U.S. law prohibited. 
==============================================================================*/

#include "AEEStdDef.h"
#include "HAP_debug.h"

#if defined(_DEBUG)

static __inline void HAP_traceme(void)
{
  (void)HAP_debug_ptrace(HAP_DEBUG_TRACEME, 0, 0, 0);
}
  
#else  /* #if defined(_DEBUG) */

#define HAP_traceme() 

#endif /* #if defined(_DEBUG) */

#endif /* #ifndef HAP_TRACEME_H */

