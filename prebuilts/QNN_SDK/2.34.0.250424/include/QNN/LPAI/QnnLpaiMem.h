//==============================================================================
//
//  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
//  All rights reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

/** @file
 *  @brief QNN LPAI Memory components
 */

#ifndef QNN_LPAI_MEM_H
#define QNN_LPAI_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

typedef enum {
  QNN_LPAI_MEM_TYPE_DDR       = 1,
  QNN_LPAI_MEM_TYPE_LLC       = 2,
  QNN_LPAI_MEM_TYPE_TCM       = 3,
  QNN_LPAI_MEM_TYPE_UNDEFINED = 0x7fffffff
} QnnLpaiMem_MemType_t;

/**
 * @brief Definition of custom mem info
 */
typedef struct {
  /// file descriptor for memory
  int32_t fd;
  /// offset from start of fd
  uint32_t offset;
} QnnLpaiMem_MemInfoCustom_t;

// clang-format off
#define QNN_LPAI_MEM_INFO_CUSTOM_INIT                             \
  {                                                               \
    0,                                          /*fd*/            \
    0u                                          /*offset*/        \
  }
// clang-format on

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // QNN_LPAI_MEM_H
