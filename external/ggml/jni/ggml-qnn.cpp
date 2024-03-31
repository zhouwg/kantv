/*
 * MIT license
 * Copyright (C) 2024 KanTV Authors
 * SPDX-License-Identifier: MIT
 *
 *
 * this clean-room implementation is for
 *
 * PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121) in Project KanTV
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <tuple>
#include <queue>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <memory>
#include <regex>
#include <random>
#include <functional>
#include <unordered_map>
#include <condition_variable>

#include "QnnTypes.h"
#include "QnnCommon.h"
#include "QnnContext.h"
#include "QnnBackend.h"
#include "QnnGraph.h"
#include "QnnProperty.h"
#include "QnnTensor.h"
#include "QnnInterface.h"

#include "ggml-qnn.h"

#include "ggml-jni.h"  //should be removed after finished this PoC for purpose of submit to upstream GGML community


// =================================================================================================
//
// Qualcomm mobile SoC native backend for GGML
//
// =================================================================================================




// =================================================================================================
//
// JNI helper function for PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121)
// should move into ggml-jni-impl.cpp in the future
//
// =================================================================================================
//TODO:
// https://github.com/zhouwg/kantv/issues/121
// PoC-S26: mapping ggml_tensor to QNN_tensor
int qnn_matrix(int n_backend_type, int n_op_type) {
    LOGGD("enter qnn_matrix\n");
    LOGGV("[%s], op type:%d\n", __func__, n_op_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d, op type:%d\n", __func__, n_backend_type, n_op_type);
    LOGGD("leave qnn_matrix\n");

    return 0;
}


//TODO:
// https://github.com/zhouwg/kantv/issues/121
// PoC-S27: offload a simple GGML matrix manipulation
int qnn_ggml(int n_backend_type, int n_ggml_op_type) {
    LOGGD("enter qnn_ggml\n");
    LOGGV("op type:%d\n", n_ggml_op_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d, ggml op type:%d\n", __func__, n_backend_type, n_ggml_op_type);
    LOGGD("leave qnn_ggml\n");

    return 0;
}