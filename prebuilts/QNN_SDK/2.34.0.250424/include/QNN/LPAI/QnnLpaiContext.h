//=============================================================================
//
//  Copyright (c) 2024 Qualcomm Technologies, Inc.
//  All rights reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

/** @file
 *  @brief QNN LPAI Context components
 */

#ifndef QNN_LPAI_CONTEXT_H
#define QNN_LPAI_CONTEXT_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "QnnLpaiContextInt.h"

typedef struct {
  uint32_t option;
  void* config;
} QnnLpaiContext_CustomConfig_t;
// clang-format on

typedef enum {
  // see QnnLpaiMem_MemType_t
  QNN_LPAI_CONTEXT_SET_CFG_MODEL_BUFFER_MEM_TYPE =
      QNN_LPAI_CONTEXT_SET_CFG_MODEL_BUFFER_MEM_TYPE_DEFAULT,
  // Unused, present to ensure 32 bits.
  QNN_LPAI_CONTEXT_SET_CFG_UNDEFINED = 0x7fffffff
} QnnLpaiContext_SetConfigOption_t;

// clang-format off
// QnnLpaiContext_CustomConfig_t initializer macro
#define QNN_LPAI_CONTEXT_CUSTOM_CONFIG_INIT                        \
  {                                                                \
    QNN_LPAI_CONTEXT_SET_CFG_UNDEFINED,               /*option*/   \
    NULL                                              /*config*/   \
  }

// clang-format on
#ifdef __cplusplus
}  // extern "C"
#endif

#endif // QNN_LPAI_CONTEXT_H
