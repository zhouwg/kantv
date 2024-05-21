/*
 * Copyright (c) 2024- KanTV Authors
 *
 * JNI header file of ncnn-jni for Project KanTV
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

#ifndef KANTV_NCNN_JNI_H
#define KANTV_NCNN_JNI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "libavutil/cde_log.h"
#include "kantv-asr.h"

#define JNI_BUF_LEN                 4096
#define JNI_TMP_LEN                 256

#ifdef __cplusplus
extern "C" {
#endif

#define NCNN_JNI_NOTIFY(...)        ncnn_jni_notify_c_impl(__VA_ARGS__)

void         ncnn_jni_notify_c_impl(const char * format, ...);

#ifdef __cplusplus
}
#endif


#endif

