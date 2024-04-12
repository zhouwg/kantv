/*
 * MIT license
 * Copyright (C) 2024 KanTV Authors
 * SPDX-License-Identifier: MIT
 *
 * PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121)
 *
 * this is implementation of ggml QNN(Qualcomm Nerual Network, aka AI Engine Direct) backend
 *
 * this file will be submitted to upstream ggml as ggml-qnn.h
 */
#pragma once

#include "ggml.h"
#include "ggml-backend.h"

#ifdef __cplusplus
extern "C" {
#endif


#define GGML_QNN_NAME           "QNN"
#define GGML_QNN_MAX_DEVICES    5

enum QNNBackend {
    QNN_CPU,
    QNN_GPU,
    QNN_HTP,
    QNN_CDSP,
    QNN_HTA
};

GGML_API int            ggml_backend_qnn_reg_devices(void);

GGML_API ggml_backend_t ggml_backend_qnn_init(size_t dev_num);

GGML_API bool           ggml_backend_is_qnn(ggml_backend_t backend);
GGML_API int            ggml_backend_qnn_get_device_count(void);
GGML_API void           ggml_backend_qnn_get_device_description(int device, char * description, size_t description_size);


GGML_API ggml_backend_buffer_type_t ggml_backend_qnn_buffer_type(size_t dev_num);


#ifdef __cplusplus
}
#endif