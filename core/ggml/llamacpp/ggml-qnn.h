/*
 * Copyright (c) 2024- KanTV Authors
 *
 * this is implementation of "PoC: Add Qualcomm mobile SoC native backend for GGML", https://github.com/zhouwg/kantv/issues/121
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "ggml.h"
#include "ggml-backend.h"

#ifdef __cplusplus
extern "C" {
#endif


#define GGML_QNN_MAX_DEVICES    3

enum QNNBackend {
    QNN_BACKEND_CPU,
    QNN_BACKEND_GPU,
    QNN_BACKEND_NPU,
    QNN_BACKEND_GGML, //"fake" QNN backend, used for compare performance between QNN and original GGML
};

GGML_API int            ggml_backend_qnn_reg_devices(void);

/**
 *
 * @param device            0: QNN_BACKEND_CPU 1: QNN_BACKEND_GPU 2: QNN_BACKEND_NPU
 * @param qnn_lib_path      qnn library path, such as "/data/local/tmp/" on Android or specified in JNI layer
 * @return
 */
GGML_API ggml_backend_t ggml_backend_qnn_init(size_t dev_num, const char * qnn_lib_path);

GGML_API bool           ggml_backend_is_qnn(ggml_backend_t backend);

GGML_API void           ggml_backend_qnn_set_n_threads(ggml_backend_t backend, int thread_counts);

GGML_API int            ggml_backend_qnn_get_device_count(void);

GGML_API void           ggml_backend_qnn_get_device_description(size_t dev_num, char * description, size_t description_size);

GGML_API GGML_CALL ggml_backend_buffer_type_t ggml_backend_qnn_buffer_type(size_t dev_num);

#ifdef __cplusplus
}
#endif
