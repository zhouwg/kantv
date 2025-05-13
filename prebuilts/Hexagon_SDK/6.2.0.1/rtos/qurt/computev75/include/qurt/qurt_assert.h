#ifndef QURT_ASSERT_H
#define QURT_ASSERT_H
/**
  @file qurt_assert.h   
  @brief  Prototypes of qurt_assert API  

  EXTERNAL FUNCTIONS
   None.

  INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

  Copyright (c) 2021  by Qualcomm Technologies, Inc.  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

=============================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/**@ingroup func_qurt_assert_error
  Writes diagnostic information to the debug buffer, and raises an error to the QuRT kernel.
  
  @datatypes
  None.
  
  @param[in] filename     Pointer to the file name string.
  @param[in] lineno       Line number.
  
  @return
  None.

  @dependencies
  None.  
 */
void qurt_assert_error(const char *filename, int lineno) __attribute__((noreturn));

#define qurt_assert(cond) ((cond)?(void)0:qurt_assert_error(__QURTFILENAME__,__LINE__))

/** @} */ /* end_ingroup func_qurt_assert */

#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

#endif /* QURT_ASSERT_H */

