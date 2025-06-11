//==============================================================================
//
//  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
//  All rights reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

/** @file
 *  @brief Internal versioning details for QNN LPAI Graph components
 */

#ifndef QNN_LPAI_GRAPH_INTERNAL_H
#define QNN_LPAI_GRAPH_INTERNAL_H

#define QNN_LPAI_GRAPH_SET_CFG_BASE  1
#define QNN_LPAI_GRAPH_GET_PROP_BASE 1

// versions for setConfig options
typedef enum {
  QNN_LPAI_GRAPH_SET_CFG_SCRATCH_MEM_V1      = QNN_LPAI_GRAPH_SET_CFG_BASE,
  QNN_LPAI_GRAPH_SET_CFG_SCRATCH_MEM_DEFAULT = QNN_LPAI_GRAPH_SET_CFG_SCRATCH_MEM_V1
} QnnLpaiGraph_SetConfigOption_ScratchMemVersion_t;

typedef enum {
  QNN_LPAI_GRAPH_SET_CFG_PERSISTENT_MEM_V1      = QNN_LPAI_GRAPH_SET_CFG_BASE + 100,
  QNN_LPAI_GRAPH_SET_CFG_PERSISTENT_MEM_DEFAULT = QNN_LPAI_GRAPH_SET_CFG_PERSISTENT_MEM_V1
} QnnLpaiGraph_SetConfigOption_PersistentMemVersion_t;

typedef enum {
  QNN_LPAI_GRAPH_SET_CFG_PERF_CFG_V1      = QNN_LPAI_GRAPH_SET_CFG_BASE + 200,
  QNN_LPAI_GRAPH_SET_CFG_PERF_CFG_DEFAULT = QNN_LPAI_GRAPH_SET_CFG_PERF_CFG_V1
} QnnLpaiGraph_SetConfigOption_PerfConfigVersion_t;

typedef enum {
  QNN_LPAI_GRAPH_SET_CFG_CORE_AFFINITY_V1      = QNN_LPAI_GRAPH_SET_CFG_BASE + 300,
  QNN_LPAI_GRAPH_SET_CFG_CORE_AFFINITY_DEFAULT = QNN_LPAI_GRAPH_SET_CFG_CORE_AFFINITY_V1
} QnnLpaiGraph_SetConfigOption_CoreAffinityVersion_t;

typedef enum {
  QNN_LPAI_GRAPH_SET_CFG_PREPARE_V1      = QNN_LPAI_GRAPH_SET_CFG_BASE + 10000,
  QNN_LPAI_GRAPH_SET_CFG_PREPARE_DEFAULT = QNN_LPAI_GRAPH_SET_CFG_PREPARE_V1
} QnnLpaiGraph_SetConfigOption_PrepareVersion_t;

// versions for getProperty options
typedef enum {
  QNN_LPAI_GRAPH_GET_PROP_SCRATCH_MEM_SIZE_V1      = QNN_LPAI_GRAPH_GET_PROP_BASE,
  QNN_LPAI_GRAPH_GET_PROP_SCRATCH_MEM_SIZE_DEFAULT = QNN_LPAI_GRAPH_GET_PROP_SCRATCH_MEM_SIZE_V1
} QnnLpaiGraph_GetPropertyOption_ScratchMemSizeVersion_t;

typedef enum {
  QNN_LPAI_GRAPH_GET_PROP_PERSISTENT_MEM_SIZE_V1 = QNN_LPAI_GRAPH_GET_PROP_BASE + 100,
  QNN_LPAI_GRAPH_GET_PROP_PERSISTENT_MEM_SIZE_DEFAULT =
      QNN_LPAI_GRAPH_GET_PROP_PERSISTENT_MEM_SIZE_V1
} QnnLpaiGraph_GetPropertyOption_PersistentMemSizeVersion_t;

#endif  // QNN_LPAI_GRAPH_INTERNAL_H
