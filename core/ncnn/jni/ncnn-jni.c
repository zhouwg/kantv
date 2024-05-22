/*
 * Copyright (c) 2024- KanTV Authors
 *
 * implementation of ncnn-jni for Project KanTV
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


bool is_zero_floatvalue(float value) {
    const float EPSION = 0.00000001;
    if ((value >= -EPSION) && (value < EPSION))
        return true;
    else
        return false;
}

JNIEXPORT jstring JNICALL
Java_org_ncnn_ncnnjava_ncnn_1bench(JNIEnv *env, jclass clazz, jstring ncnnmodel_param,
                                   jstring ncnnmodel_bin, jstring user_data, jobject bitmap, jint n_bench_type,
                                   jint n_thread_counts, jint n_backend_type, jint n_op_type) {
    UNUSED(clazz);

    const char *sz_model_param = NULL;
    const char *sz_model_bin  = NULL;
    const char *sz_user_data = NULL;
    const char *sz_bench_result = "unknown";

    sz_model_param = (*env)->GetStringUTFChars(env, ncnnmodel_param, NULL);
    if (NULL == sz_model_param) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_model_bin = (*env)->GetStringUTFChars(env, ncnnmodel_bin, NULL);
    if (NULL == sz_model_bin) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_user_data = (*env)->GetStringUTFChars(env, user_data, NULL);
    if (NULL == sz_user_data) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model param:%s\n", sz_model_param);
    LOGGV("model bin:%s", sz_model_bin);
    LOGGV("user_data:%s\n", sz_user_data);
    LOGGV("bench type: %d\n", n_bench_type);
    LOGGV("thread counts:%d\n", n_thread_counts);
    LOGGV("backend type:%d\n", n_backend_type);
    LOGGV("op type:%d\n", n_op_type);

    if (n_bench_type > NCNN_BENCH_MAX) {
        LOGGW("pls check bench type\n");
        NCNN_JNI_NOTIFY("benchmark type %d not supported currently", n_bench_type);
        goto failure;
    }

    if (n_backend_type > NCNN_BACKEND_MAX) {
        LOGGW("pls check ncnn backend type\n");
        goto failure;
    }

    if (0 == n_thread_counts)
        n_thread_counts = 1;

    ncnn_jni_bench(env, sz_model_param, sz_model_bin, sz_user_data, bitmap, n_bench_type, n_thread_counts, n_backend_type, n_op_type);


    failure:
    if (NULL != sz_model_param) {
        (*env)->ReleaseStringUTFChars(env, ncnnmodel_param, sz_model_param);
    }

    if (NULL != sz_model_bin) {
        (*env)->ReleaseStringUTFChars(env, ncnnmodel_bin, sz_model_bin);
    }

    if (NULL != sz_user_data) {
        (*env)->ReleaseStringUTFChars(env, user_data, sz_user_data);
    }

    jstring string = (*env)->NewStringUTF(env, sz_bench_result);
    return string;
}