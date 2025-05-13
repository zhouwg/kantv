//=============================================================================
//
//  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
//  All rights reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

/** @file
 *  @brief QNN LPAI Device components
 */

#ifndef QNN_LPAI_DEVICE_H
#define QNN_LPAI_DEVICE_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure is being used in QnnDevice_HardwareDeviceInfoV1_t
 * QnnDevice_getPlatformInfo use this structure to list the supported device features/info
 */
typedef struct _QnnDevice_DeviceInfoExtension_t {
  uint32_t socModel;        // An enum value defined in Qnn Header that represent SoC model
  uint32_t arch;            // This field shows the architecture of this device
  const char* domainName;   // This field shows the domain name of this device
} QnnLpaiDevice_DeviceInfoExtension_t;

// clang-format off
/// QnnLpaiDevice_DeviceInfoExtension_t initializer macro
#define QNN_LPAI_DEVICE_INFO_EXTENSION_INIT                          \
  {                                                                  \
    0u,                                         /*socModel*/         \
    0u,                                         /*arch*/             \
    "adsp"                                      /*domainName*/       \
  }
// clang-format on

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // QNN_LPAI_DEVICE_H
