//==============================================================================
//
//  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
//  All rights reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

/** @file
 *  @brief Internal versioning details for QNN LPAI Context components
 */

#ifndef QNN_LPAI_CONTEXT_INT_H
#define QNN_LPAI_CONTEXT_INT_H

#define QNN_LPAI_CONTEXT_SET_CFG_BASE  1

// versions for setConfig options
typedef enum {
  QNN_LPAI_CONTEXT_SET_CFG_MODEL_BUFFER_MEM_TYPE_V1 = QNN_LPAI_CONTEXT_SET_CFG_BASE,
  QNN_LPAI_CONTEXT_SET_CFG_MODEL_BUFFER_MEM_TYPE_DEFAULT =
      QNN_LPAI_CONTEXT_SET_CFG_MODEL_BUFFER_MEM_TYPE_V1
} QnnLpaiContext_SetConfigOption_ModelBufferMemTypeVersion_t;

#endif  // QNN_LPAI_CONTEXT_INT_H
