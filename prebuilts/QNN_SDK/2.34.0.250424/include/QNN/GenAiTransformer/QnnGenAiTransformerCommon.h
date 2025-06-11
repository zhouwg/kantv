//=============================================================================
//
//  Copyright (c) 2024 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//=============================================================================

/** @file
 *  @brief QNN GenAiTransformer Common components
 *
 *         This file defines versioning and other identification details
 *         and supplements QnnCommon.h for GenAiTransformer backend
 */

#ifndef QNN_GENAI_TRANSFORMER_COMMON_H
#define QNN_GENAI_TRANSFORMER_COMMON_H

#include "QnnCommon.h"

/// GenAiTransformer Backend identifier
#define QNN_BACKEND_ID_GENAI_TRANSFORMER 14

/// GenAiTransformer interface provider
#define QNN_GENAI_TRANSFORMER_INTERFACE_PROVIDER_NAME "GENAI_TRANSFORMER_QTI_AISW"

// GenAiTransformer API Version values
#define QNN_GENAI_TRANSFORMER_API_VERSION_MAJOR 1
#define QNN_GENAI_TRANSFORMER_API_VERSION_MINOR 0
#define QNN_GENAI_TRANSFORMER_API_VERSION_PATCH 0

// clang-format off
/// Macro to set Qnn_ApiVersion_t for GENAI_TRANSFORMER backend
#define QNN_GENAI_TRANSFORMER_API_VERSION_INIT                                 \
  {                                                              \
    {                                                            \
      QNN_API_VERSION_MAJOR,     /*coreApiVersion.major*/        \
      QNN_API_VERSION_MINOR,     /*coreApiVersion.major*/        \
      QNN_API_VERSION_PATCH      /*coreApiVersion.major*/        \
    },                                                           \
    {                                                            \
      QNN_GENAI_TRANSFORMER_API_VERSION_MAJOR, /*backendApiVersion.major*/     \
      QNN_GENAI_TRANSFORMER_API_VERSION_MINOR, /*backendApiVersion.minor*/     \
      QNN_GENAI_TRANSFORMER_API_VERSION_PATCH  /*backendApiVersion.patch*/     \
    }                                                            \
  }

// clang-format on

#endif  // QNN_GENAI_TRANSFORMER_COMMON_H