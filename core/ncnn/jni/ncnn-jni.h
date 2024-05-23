/*
 * Copyright (c) 2024- KanTV Authors
 *
 * header file of ncnn-jni for Project KanTV
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
#include "kantv-media.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================================
//add new AI benchmark type / new realtime inference / new backend for NCNN here, keep sync with CDEUtils.java

// available bench type for NCNN
enum ncnn_jni_bench_type {
    NCNN_BENCHMARK_RESNET = 0,
    NCNN_BENCHMARK_SQUEEZENET,
    NCNN_BENCHMARK_MNIST,
    NCNN_BENCHMARK_ASR,
    NCNN_BENCHMARK_TTS,
    NCNN_BENCHARK_YOLOV5,
    NCNN_BENCHMARK_MAX
};

// available realtime inference type for NCNN
enum ncnn_jni_realtimeinference_type {
    NCNN_REALTIMEINFERENCE_FACEDETECT = 0,  //reserved for multimodal poc(CV, NLP, LLM, TTS... with live camera)
    NCNN_REALTIMEINFERENCE_NANODAT
};

// available backend for NCNN
enum ncnn_jni_backend_type {
    NCNN_BACKEND_CPU = 0,
    NCNN_BACKEND_GPU,
    NCNN_BACKEND_MAX
};
//=============================================================================================


#define NCNN_JNI_NOTIFY(...)        ncnn_jni_notify_c_impl(__VA_ARGS__)


void         ncnn_jni_notify_c_impl(const char * format, ...);

bool         is_zero_floatvalue(float value);

/**
*
* @param sz_ncnnmodel_param   param file of ncnn model
* @param sz_ncnnmodel_bin     bin   file of ncnn model
* @param sz_user_data         ASR: /sdcard/kantv/jfk.wav / LLM: user input / TEXT2IMAGE: user input / ResNet&SqueezeNet&MNIST: image path / TTS: user input
* @param bitmap
* @param n_bench_type         1: NCNN RESNET 2: NCNN SQUEEZENET 3: NCNN MNIST
* @param n_threads            1 - 8
* @param n_backend_type       0: NCNN_BACKEND_CPU  1: NCNN_BACKEND_GPU
* @param n_op_type            type of NCNN OP
* @return
*/
void         ncnn_jni_bench(JNIEnv *env, const char * sz_ncnnmodel_param, const char * sz_ncnnmodel_bin, const char * sz_user_data, jobject bitmap, int n_bench_type, int num_threads, int n_backend_type, int n_op_type);



#ifdef __cplusplus
}
#endif

#endif

