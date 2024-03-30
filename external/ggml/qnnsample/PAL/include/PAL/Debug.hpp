//============================================================================
//
//  Copyright (c) 2020-2022 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//============================================================================

#pragma once

#include "ggml-jni.h"

#define DEBUG_ON 1

#if DEBUG_ON
/*
#define DEBUG_MSG(...)            \
  {                               \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n");        \
  }
*/
#define DEBUG_MSG LOGGD
#else
#define DEBUG_MSG(...)
#endif
