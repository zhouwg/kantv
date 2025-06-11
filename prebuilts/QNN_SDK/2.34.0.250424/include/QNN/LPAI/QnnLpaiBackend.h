//==============================================================================
//
//  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
//  All rights reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

/** @file
 *  @brief QNN LPAI component Backend API.
 *
 *         The interfaces in this file work with the top level QNN
 *         API and supplements QnnBackend.h for LPAI backend
 */

#ifndef QNN_LPAI_BACKEND_H
#define QNN_LPAI_BACKEND_H

#include "QnnBackend.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Macros
//=============================================================================

//=============================================================================
// Data Types
//=============================================================================

/**
 * @brief An enum which defines the different backend custom config options
 */
typedef enum {
  // see QnnLpaiBackend_CustomConfigHwInfo_t
  QNN_LPAI_BACKEND_CUSTOM_CFG_HW_INFO,
  QNN_LPAI_BACKEND_CUSTOM_CFG_UNDEFINED = 0x7fffffff
} QnnLpaiBackend_CustomConfigOption_t;

/**
 * @brief An enum which defines the different targets supported by LPAI compilation.
 */
typedef enum {
  /// LPAI model will be compiled for x86
  QNN_LPAI_BACKEND_TARGET_X86 = 0,
  /// LPAI model will be compiled for ARM
  QNN_LPAI_BACKEND_TARGET_ARM = 1,
  /// LPAI model will be compiled for ADSP
  QNN_LPAI_BACKEND_TARGET_ADSP = 2,
  /// LPAI model will be compiled for TENSILICA
  QNN_LPAI_BACKEND_TARGET_TENSILICA = 3,
  /// UNKNOWN enum event that must not be used
  QNN_LPAI_BACKEND_TARGET_UNKNOWN = 0x7fffffff,
} QnnLpaiBackend_Target_t;

/**
 * @brief An enum which defines the version of LPAI Hardware.
 */
typedef enum {
  /// No LPAI HW will be used
  QNN_LPAI_BACKEND_HW_VERSION_NA   = 0,
  /// LPAI HW version v1
  QNN_LPAI_BACKEND_HW_VERSION_V1   = 0x00000001,
  /// LPAI HW version v2
  QNN_LPAI_BACKEND_HW_VERSION_V2   = 0x00000002,
  /// LPAI HW version v3
  QNN_LPAI_BACKEND_HW_VERSION_V3   = 0x00000003,
  /// LPAI HW version v4
  QNN_LPAI_BACKEND_HW_VERSION_V4   = 0x00000004,
  /// LPAI HW version v5
  QNN_LPAI_BACKEND_HW_VERSION_V5   = 0x00000005,
  /// LPAI HW version v5.1
  QNN_LPAI_BACKEND_HW_VERSION_V5_1 = 0x00010005,
  /// LPAI HW version v6
  QNN_LPAI_BACKEND_HW_VERSION_V6   = 0x00000006,
  /// LPAI HW default version v5
  QNN_LPAI_BACKEND_HW_VERSION_DEFAULT = QNN_LPAI_BACKEND_HW_VERSION_V5,
  /// UNKNOWN enum event that must not be used
  QNN_LPAI_BACKEND_HW_VERSION_UNKNOWN = 0x7fffffff,
} QnnLpaiBackend_HwVersion_t;

//=============================================================================
// Public Functions
//=============================================================================

//------------------------------------------------------------------------------
//   Implementation Definition
//------------------------------------------------------------------------------

/**
 * @brief Structure describing the set of configurations supported by the backend.
 *        Objects of this type are to be referenced through QnnBackend_CustomConfig_t.
 */
typedef struct {
  uint32_t option;
  void* config;
} QnnLpaiBackend_CustomConfig_t;

// clang-format off
/// QnnLpaiBackend_CustomConfig_t initializer macro
#define QNN_LPAI_BACKEND_CUSTOM_CONFIG_INIT                        \
  {                                                                \
    QNN_LPAI_BACKEND_CUSTOM_CFG_UNDEFINED,           /*option*/    \
    NULL                                             /*config*/    \
  }
// clang-format on

typedef struct {
  QnnLpaiBackend_Target_t lpaiTarget;
  QnnLpaiBackend_HwVersion_t hwVersion;
} QnnLpaiBackend_CustomConfigHwInfo_t;

// clang-format off
/// QnnLpaiBackend_CustomConfigHwInfo_t initializer macro
#define QNN_LPAI_BACKEND_CUSTOM_CONFIG_HW_INFO_INIT                                 \
  {                                                                         \
    QNN_LPAI_BACKEND_TARGET_ADSP,        /*lpaiTarget*/                     \
    QNN_LPAI_BACKEND_HW_VERSION_DEFAULT, /*hwVersion*/                      \
  }
// clang-format on

/**
 * @brief Enum describing the set of properties supported by the backend.
 *        Objects of this type are to be referenced through QnnBackend_CustomProperty_t.
 */
typedef enum {
  // get the start address alignment and size alignment requirement of buffers, see
  // QnnLpaiBackend_BufferAlignmentReq_t
  QNN_LPAI_BACKEND_GET_PROP_ALIGNMENT_REQ,
  // indicate if cached binary buffer need to be persistent until QnnContext_free is called, return
  // bool
  // if true is returned, need to specify QNN_CONTEXT_CONFIG_PERSISTENT_BINARY during
  // QnnContext_createFromBinary
  QNN_LPAI_BACKEND_GET_PROP_REQUIRE_PERSISTENT_BINARY,
  // Unused, present to ensure 32 bits.
  QNN_LPAI_BACKEND_GET_PROP_UNDEFINED = 0x7fffffff
} QnnLpaiBackend_GetPropertyOption_t;

typedef struct {
  // the start address of the buffer must be startAddrAlignment-byte aligned
  uint32_t startAddrAlignment;
  // the allocated buffer must be a multiple of sizeAlignment bytes
  uint32_t sizeAlignment;
} QnnLpaiBackend_BufferAlignmentReq_t;

// clang-format off
/// QnnLpaiBackend_BufferAlignmentReq_t initializer macro
#define QNN_LPAI_BACKEND_ALIGNMENT_REQ_INIT                          \
  {                                                                  \
    0u,                                      /*startAddrAlignment*/  \
    0u                                       /*sizeAlignment*/       \
  }
// clang-format on

// used by QnnBackend_getProperty
typedef struct {
  uint32_t option;
  void* property;
} QnnLpaiBackend_CustomProperty_t;

// clang-format off
/// QnnLpaiBackend_CustomProperty_t initializer macro
#define QNN_LPAI_BACKEND_CUSTOM_PROPERTY_INIT                        \
  {                                                                  \
    QNN_LPAI_BACKEND_GET_PROP_UNDEFINED,               /*option*/    \
    NULL                                               /*property*/  \
  }
// clang-format on

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
