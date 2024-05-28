/*
 * Copyright (c) 2024- KanTV Authors
 *
 * header file of ggml-jni for Project KanTV
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

#ifndef KANTV_GGML_JNI_H
#define KANTV_GGML_JNI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "libavutil/cde_log.h"
#ifdef ANDROID //for build MiniCPM-V command line application on Linux
#include "kantv-asr.h"
#include "kantv-media.h"
#endif

#include "ggml.h"

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================================
//add new AI benchmark type / new backend using GGML inference framework here, keep sync with CDEUtils.java

// available bench type for ggml-jni
enum ggml_jni_bench_type {
    GGML_BENCHMARK_MEMCPY = 0,                //memcpy  benchmark
    GGML_BENCHMARK_MULMAT,                    //mulmat  benchmark
    GGML_BENCHMARK_QNN_GGML_OP,               //UT for PoC-S49: implementation of GGML OPs using QNN API
    GGML_BENCHMARK_QNN_AUTO_UT,               //automation UT for PoC-S49: implementation of GGML OPs using QNN API
    GGML_BENCHMARK_ASR,                       //ASR(using whisper.cpp based on GGML) benchmark
    GGML_BENCHMARK_LLM,                       //A GPT-4  style LLM benchmark using llama.cpp based on GGML
    GGML_BENCHMARK_LLM_V,                     //A GPT-4V style Multimodal LLM benchmark using llama.cpp based on GGML
    GGML_BENCHMARK_LLM_O,                     //A GPT-4o style Multimodal LLM benchmark using llama.cpp based on GGML
    GGML_BENCHMARK_TEXT2IMAGE,                //TEXT2IMAGE(stablediffusion.cpp) benchmark using GGML
    GGML_BENCHMARK_CV_MNIST,                  //MNIST inference using GGML
    GGML_BENCHMARK_TTS,                       //TTS(bark.cpp) benchmark using GGML
    GGML_BENCHMARK_MAX
};


// available backend for ggml-jni
enum ggml_jni_backend_type {
    GGML_BACKEND_CPU = 0,
    GGML_BACKEND_GPU,
    GGML_BACKEND_DSP,
    GGML_BACKEND_GGML, //"fake" QNN backend just for compare performance between QNN and original GGML
    GGML_BACKEND_MAX
};
//=============================================================================================


// =================================================================================================
// JNI helper function in ggml-jni
// =================================================================================================

#define GGML_JNI_NOTIFY(...)        ggml_jni_notify_c_impl(__VA_ARGS__)

    void         ggml_jni_notify_c_impl(const char * format, ...);
    int          ggml_jni_get_cpu_core_counts(void);

    bool         ggml_jni_abort_callback(void * data);
    void         ggml_jni_set_benchmark_status(int b_exit_benchmark);
    int          ggml_jni_get_abortbenchmark_value(void);
    /**
    *
    * @param sz_model_path   /sdcard/kantv/models/file_name_of_gguf_model or qualcomm's prebuilt dedicated model.so or ""
    * @param sz_user_data    ASR: /sdcard/kantv/jfk.wav / LLM: user input / TEXT2IMAGE: user input / MNIST: image path / TTS: user input
    * @param n_bench_type    0: memcpy 1: mulmat 2: QNN GGML OP(QNN UT) 3: QNN UT automation 4: ASR(whisper.cpp) 5: LLM(llama.cpp) 6: TEXT2IMAGE(stablediffusion.cpp) 7:MNIST 8: TTS
    * @param n_threads       1 - 8
    * @param n_backend_type  0: CPU  1: GPU  2: NPU 3: ggml("fake" QNN backend, just for compare performance)
    * @param n_op_type       type of GGML OP
    * @return
    */
    void         ggml_jni_bench(const char * sz_model_path, const char * sz_user_data, int n_bench_type, int num_threads, int n_backend_type, int n_op_type);
    //"m" for "multimodal", GPT4-V or GPT4-o
    void         ggml_jni_bench_m(const char * sz_model_path, const char * sz_img_path, const char * sz_user_data, int n_bench_type, int num_threads, int n_backend_type);

    const char * ggml_jni_bench_memcpy(int n_threads);

    const char * ggml_jni_bench_mulmat(int n_threads, int n_backend);

    const char * ggml_jni_get_ggmltype_str(enum ggml_type wtype);

    bool         ggml_jni_is_valid_utf8(const char * string);


// =================================================================================================
// PoC#64:Add/implement realtime AI subtitle for online English TV using whisper.cpp from 03-05-2024 to 03-16-2024
// https://github.com/zhouwg/kantv/issues/64
// =================================================================================================
    /**
    * @param sz_model_path
    * @param n_threads
    * @param n_asrmode            0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
    * @param n_backend            0: QNN CPU 1: QNN GPU 2: QNN HTP(DSP) 3:ggml
    */
    int          whisper_asr_init(const char *sz_model_path, int n_threads, int n_asrmode, int n_backend);
    void         whisper_asr_finalize(void);

    void         whisper_asr_start(void);
    void         whisper_asr_stop(void);
    /**
    * @param sz_model_path
    * @param n_threads
    * @param n_asrmode            0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
    * @param n_backend            0: QNN CPU 1: QNN GPU 2: QNN HTP(DSP) 3:ggml
    */
    int          whisper_asr_reset(const char * sz_model_path, int n_threads, int n_asrmode, int n_backend);


// =================================================================================================
// trying to integrate llama.cpp from 03/26/2024 to 03/28/2024
// =================================================================================================
    /**
    *
    * @param sz_model_path         /sdcard/kantv/models/xxxxxx.gguf
    * @param prompt
    * @param bench_type            not used currently
    * @param n_threads             1 - 8
    * @param n_backend            0: QNN CPU 1: QNN GPU 2: QNN HTP(DSP) 3:ggml
    * @return
    */
    int          llama_inference(const char * model_path, const char * prompt, int bench_type, int num_threads, int n_backend);
    int          llama_inference_ng(const char * model_path, const char * prompt, int bench_type, int num_threads, int n_backend);


// =================================================================================================
// PoC#121:Add/implement Qualcomm mobile SoC native backend for GGML inference framework from 03-29-2024 to 04-26-2024
// https://github.com/zhouwg/kantv/issues/121
// PR to upstream llama.cpp
// https://github.com/ggerganov/llama.cpp/pull/6869
// =================================================================================================
    /**
     * this special function is for PoC-S49: implementation of other GGML OP(non-mulmat) using QNN API
     *
     * this function will calling GGML QNN backend directly
     *
     * this function used to validate PoC-S49:implementation of other GGML OP(non-mulmat) using QNN API
     * or this function is UT for PoC-S49:implementation of other GGML OP(non-mulmat) using QNN API
     *
     * @param model_path whisper.cpp model at the first step, llama.cpp model at the second step
     * @param num_threads 1 - 8
     * @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance between QNN and original GGML)
     * @param n_op_type GGML OP type
     * @return
     */
    int qnn_ggml_op(const char * model_path, int num_threads, int n_backend_type, int n_ggml_op_type);

    /**
     * similar to qnn_ggml_op, but an automation UT for a specify GGML OP with a specify backend
     */
    int qnn_ggml_op_automation_ut(const char * model_path, int num_threads, int n_backend_type, int n_ggml_op_type);


// =================================================================================================
// trying to integrate stablediffusion.cpp on 04-06-2024(Apri,6,2024)
// =================================================================================================
/**
*
* @param sz_model_path         /sdcard/kantv/models/xxxxxx.gguf
* @param prompt
* @param bench_type            not used currently
* @param n_threads             1 - 8
* @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance)
* @return
*/
int  stablediffusion_inference(const char * model_path, const char * prompt, int bench_type, int num_threads, int n_backend_type);


// =================================================================================================
// trying to integrate MNIST inference using ggml
// =================================================================================================
int  mnist_inference(const char * sz_model_path, const char * sz_image_path, int bench_type, int num_threads, int n_backend_type);


// =================================================================================================
// trying to integrate TTS(bark.cpp) inference using ggml
// =================================================================================================
int  tts_inference(const char * sz_model_path, const char * prompt, int bench_type, int num_threads, int n_backend_type);


// =================================================================================================
// trying to integrate MiniCPM-V(A GPT-4V Level Multimodal LLM, https://github.com/OpenBMB/MiniCPM-V) inference using llama.cpp on 05-25-2024
// =================================================================================================
int minicpmv_inference(const char * sz_model_path, const char * sz_img_path, const char * sz_user_data, int num_threads, int n_backend_type);

#ifdef __cplusplus
}
#endif


#endif
