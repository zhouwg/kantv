//==============================================================================
//
// Copyright (c) 2023-2024 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

/**
 *  @file
 *  @brief  A header which defines the QNN Ir specialization of the QnnGraph.h interface.
 */

#ifndef QNN_IR_GRAPH_H
#define QNN_IR_GRAPH_H

#ifdef __cplusplus
#include <cstdint>
#else
#include <stdint.h>
#endif

#include "QnnGraph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  QNN_IR_GRAPH_SERIALIZATION_TYPE_FLAT_BUFFER = 1
} QnnIrGraph_SerializationType_t;

typedef enum {
  QNN_IR_GRAPH_CONFIG_OPTION_SERIALIZATION = 1,
  QNN_IR_GRAPH_CONFIG_OPTION_UNKNOWN       = 0x7fffffff
} QnnIrGraph_ConfigOption_t;

typedef struct {
  QnnIrGraph_SerializationType_t serializationType;
  const char *outputPath;
} QnnIrGraph_SerializationOption_t;

/**
 * @brief A struct which Structure describing the set of configurations supported by graph.

*/
typedef struct {
  QnnIrGraph_ConfigOption_t option;
  union {
    QnnIrGraph_SerializationOption_t serializationOption;
  };
} QnnIrGraph_CustomConfig_t;

// clang-format off
/// QnnIrGraph_CustomConfig_t initializer macro

#define QNN_IR_GRAPH_SERIALIZATION_OPTION_INIT \
  {                                                  \
    QNN_IR_GRAPH_SERIALIZATION_TYPE_FLAT_BUFFER        \
    ""                                               \
  }

#define QNN_IR_GRAPH_CUSTOM_CONFIG_INIT                       \
  {                                                                 \
    QNN_IR_GRAPH_CONFIG_OPTION_SERIALIZATION, /*option*/      \
    {                                                               \
      QNN_IR_GRAPH_SERIALIZATION_OPTION_INIT                  \
    }                                                               \
  }
// clang-format on

#ifdef __cplusplus
}  // extern "C"
#endif

#endif
