/*
 * MIT license
 * Copyright (C) 2024 KanTV Authors
 * SPDX-License-Identifier: MIT
 */
#pragma once

#include "ggml.h"
#include "ggml-backend.h"

#ifdef __cplusplus
extern "C" {
#endif


#define GGML_QNN_NAME           "QNN"
#define GGML_QNN_MAX_DEVICES    5

GGML_API int            ggml_backend_qnn_reg_devices(void);

GGML_API ggml_backend_t ggml_backend_qnn_init(size_t dev_num);

GGML_API bool           ggml_backend_is_qnn(ggml_backend_t backend);
GGML_API int            ggml_backend_qnn_get_device_count(void);
GGML_API void           ggml_backend_qnn_get_device_description(int device, char * description, size_t description_size);
GGML_API void           ggml_backend_qnn_get_device_memory(int device, size_t * free, size_t * total);


GGML_API ggml_backend_buffer_type_t ggml_backend_qnn_buffer_type(size_t dev_num);

GGML_API ggml_backend_buffer_type_t ggml_backend_qnn_host_buffer_type(void);


#ifdef __cplusplus
}
#endif