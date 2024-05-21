/*
 * Copyright (c) 2024- KanTV Authors
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
 *
 * The above statement and notice must be included in corresponding files
 * in derived project
 */
#include <jni.h>

#include "ncnn-jni.h"

#define UNUSED(x)       (void)(x)


void  ncnn_jni_notify_c_impl(const char * format,  ...) {
    static unsigned char s_ggml_jni_buf[JNI_BUF_LEN];

    va_list va;
    int len_content = 0;

    memset(s_ggml_jni_buf, 0, JNI_BUF_LEN);

    va_start(va, format);

    len_content = vsnprintf(s_ggml_jni_buf, JNI_BUF_LEN, format, va);
    snprintf(s_ggml_jni_buf + len_content, JNI_BUF_LEN - len_content, "\n");

    kantv_asr_notify_benchmark_c(s_ggml_jni_buf);

    va_end(va);
}