/*
 * Copyright (c) 2024- KanTV Authors. All Rights Reserved.
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

//TODO: re-write entire whispercpp-jni.c with standard Android JNI specification
// interaction between kantv-core and whispercpp-jni

#include <jni.h>

#include "whisper.h"
#include "ggml.h"

#include "cde_log.h"
#include "kantv-asr.h"
#include "whispercpp-jni.h"

#define UNUSED(x) (void)(x)

JNIEXPORT jstring JNICALL
Java_org_ggml_whispercpp_whispercpp_get_1systeminfo(
        JNIEnv *env, jobject thiz
) {
    UNUSED(thiz);
    LOGGD("enter getSystemInfo");
    const char *sysinfo = whisper_print_system_info();
    jstring string = (*env)->NewStringUTF(env, sysinfo);
    LOGGD("leave getSystemInfo");

    return string;
}


JNIEXPORT void JNICALL
Java_org_ggml_whispercpp_whispercpp_set_1benchmark_1status(JNIEnv *env, jclass clazz,
                                                           jint b_exit_benchmark) {
    UNUSED(env);
    UNUSED(clazz);

    whisper_set_benchmark_status((int) b_exit_benchmark);
}


JNIEXPORT jstring JNICALL
Java_org_ggml_whispercpp_whispercpp_bench(JNIEnv *env, jclass clazz, jstring model_path,
                                       jstring audio_path, jint bench_type, jint num_threads) {
    UNUSED(clazz);

    const char *sz_model_path = NULL;
    const char *sz_audio_path = NULL;
    const char *sz_bench_result = "unknown";
    const char *bench_result = NULL;

    sz_model_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_model_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_audio_path = (*env)->GetStringUTFChars(env, audio_path, NULL);
    if (NULL == sz_audio_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("audio file:%s\n", sz_audio_path);
    LOGGV("bench type: %d\n", bench_type);
    LOGGV("thread counts:%d\n", num_threads);

    if (bench_type > 3) {
        LOGGW("pls check bench type\n");
        goto failure;
    }

    if (0 == num_threads)
        num_threads = 1;

    whisper_bench(sz_model_path, sz_audio_path, bench_type, num_threads);

    if (BECHMARK_ASR == bench_type) { // asr
        //just return "asr_result" even get correct asr result because I'll try to do everything in native layer
        sz_bench_result = "asr_result";
    }

failure:
    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_model_path);
    }

    if (NULL != sz_audio_path) {
        (*env)->ReleaseStringUTFChars(env, audio_path, sz_audio_path);
    }

    jstring string = (*env)->NewStringUTF(env, sz_bench_result);
    return string;
}


JNIEXPORT jint JNICALL
Java_org_ggml_whispercpp_whispercpp_get_1cpu_1core_1counts(JNIEnv *env, jclass clazz) {
    UNUSED(env);
    UNUSED(clazz);

    return whisper_get_cpu_core_counts();
}


//TODO: not a good idea, just skip this during PoC stage
JNIEXPORT void JNICALL
Java_org_ggml_whispercpp_whispercpp_asr_1init(JNIEnv *env, jclass clazz, jstring model_path, jint num_threads, jint n_devmode) {
    UNUSED(clazz);

    const char *sz_model_path = NULL;

    sz_model_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_model_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("thread counts:%d\n", num_threads);
    LOGGV("dev mode:%d\n", n_devmode);

    whisper_asr_init(sz_model_path, num_threads, n_devmode);

failure:
    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_model_path);
    }
}


JNIEXPORT void JNICALL
Java_org_ggml_whispercpp_whispercpp_asr_1finalize(JNIEnv *env, jclass clazz) {
    UNUSED(env);
    UNUSED(clazz);

    whisper_asr_finalize();
}
