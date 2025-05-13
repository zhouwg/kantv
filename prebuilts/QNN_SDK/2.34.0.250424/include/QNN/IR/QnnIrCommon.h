//=============================================================================
//
//  Copyright (c) 2023-2024 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

/** @file
 *  @brief QNN IR Common components
 *
 *         This file defines versioning and other identification details
 *         and supplements QnnCommon.h for Ir backend
 */

#ifndef QNN_IR_COMMON_H
#define QNN_IR_COMMON_H

#include "QnnCommon.h"

/// Ir Backend Identifier
#define QNN_BACKEND_ID_IR 9

/// Ir interface provider
#define QNN_IR_INTERFACE_PROVIDER_NAME "IR_QTI_AISW"

// Ir API Version Values
#define QNN_IR_API_VERSION_MAJOR 0
#define QNN_IR_API_VERSION_MINOR 1
#define QNN_IR_API_VERSION_PATCH 0

// clang-format off
// Macro to set Qnn_ApiVersion_t for Ir backend
#define QNN_IR_API_VERSION_INIT                                    \
  {                                                                      \
    {                                                                    \
      QNN_API_VERSION_MAJOR,    /* coreApiVersion.major */               \
      QNN_API_VERSION_MINOR,    /* coreApiVersion.minor */               \
      QNN_API_VERSION_PATCH     /* coreApiVersion.patch */               \
    },                                                                   \
    {                                                                    \
      QNN_IR_API_VERSION_MAJOR, /* backendApirVersion.major */     \
      QNN_IR_API_VERSION_MINOR, /* backendApirVersion.minor */     \
      QNN_IR_API_VERSION_PATCH, /* backendApirVersion.patch */     \
    }                                                                    \
  }

// clang-format on

#endif // QNN_IR_COMMON_H
