#ifndef _MMDEFS_H
#define _MMDEFS_H
/*==============================================================================
  Copyright (c) 2012-2013 Qualcomm Technologies Incorporated.
  All Rights Reserved Qualcomm Technologies Proprietary

  Export of this technology or software is regulated by the U.S.
  Government. Diversion contrary to U.S. law prohibited.
==============================================================================*/

/*------------------------------------------------------------------------------
   Standard Integer Types
------------------------------------------------------------------------------*/

#include "stdint.h"

/*------------------------------------------------------------------------------
   Constants
------------------------------------------------------------------------------*/

#undef TRUE
#undef FALSE

#define TRUE   (1)  /* Boolean true value */
#define FALSE  (0)  /* Boolean false value */

#ifndef NULL
  #define NULL (0)
#endif

/*------------------------------------------------------------------------------
   Character and boolean
------------------------------------------------------------------------------*/

typedef char char_t;           /* Character type */
typedef unsigned char bool_t;  /* Boolean value type */

/*==============================================================================
  FUNCTION : align_to_8_byte                                                
  DESCRIPTION: Ceil to the next multiple of 8                               
==============================================================================*/
static inline uint32_t align_to_8_byte(const uint32_t num)
{
   return ((num + 7) & (0xFFFFFFF8));
}

#endif /* _MMDEFS_H */

