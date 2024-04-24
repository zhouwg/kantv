/*
 * Copyright (c) 2024- KanTV Authors
 *
 * JNI implementation of GGML for Project KanTV
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
#include "ggml.h"

#ifdef __cplusplus
extern "C" {
#endif

#define JNI_BUF_LEN                 4096
#define JNI_TMP_LEN                 256

#define BECHMARK_ASR                0       //whisper.cpp ASR benchmark
#define BECHMARK_MEMCPY             1       //memcpy  benchmark
#define BECHMARK_MULMAT             2       //mulmat  benchmark
#define BECHMARK_FULL               3       //whisper.cpp full benchmark
#define BENCHMAKR_LLAMA             4       //llama.cpp benchmark
#define BENCHMAKR_STABLEDIFFUSION   5       //stable diffusion benchmark, not work on Xiaomi 14 currently
// there are three killer/heavyweight AI applications based on GGML currently: whisper.cpp, llama.cpp, stablediffusion.cpp, so they are here
// then
// step-by-step for PoC:Add Qualcomm mobile SoC native backend for GGML https://github.com/zhouwg/kantv/issues/121
#define BENCHMAKR_QNN_SAMPLE        6       //"play with /say hello to" QNN Sample
#define BENCHMAKR_QNN_SAVER         7       //study QNN SDK mechanism by QNN Saver
#define BENCHMARK_QNN_MATRIX        8       //offload a simple fp32 2x2 matrix addition operation to QNN
#define BENCHMARK_QNN_GGML          9       //mapping ggml tensor to QNN tensor
#define BENCHMARK_QNN_COMPLEX       10      //complex/complicated computation graph in C/C++ or GGML and then offload them to QNN
#define BENCHMARK_QNN_GGML_OP       11      //UT for PoC-S49: implementation of GGML OPs using QNN API
#define BENCHMARK_QNN_AUTO_UT       12      //automation UT for PoC-S49: implementation of GGML OPs using QNN API
#define BENCHMAKR_MAX               12

#define BACKEND_CPU                 0
#define BACKEND_GPU                 1
#define BACKEND_DSP                 2
#define BACKEND_GGML                3       //"fake" QNN backend just for compare performance between QNN and original GGML
#define BACKEND_MAX                 3


#define GGML_JNI_NOTIFY(...)        ggml_jni_notify_c_impl(__VA_ARGS__)

// JNI helper function for whisper.cpp benchmark
    void         ggml_jni_notify_c_impl(const char * format, ...);
    int          whisper_get_cpu_core_counts(void);
    void         whisper_set_benchmark_status(int b_exit_benchmark);
    /**
    *
    * @param sz_model_path         /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/kantv/xxxxxx.gguf or qualcomm's prebuilt dedicated model.so or ""
    * @param sz_audio_path         /sdcard/kantv/jfk.wav
    * @param n_bench_type          0: whisper asr 1: memcpy 2: mulmat  3: whisper full 4: LLAMA 5: stable diffusion 6: QNN sample 7: QNN saver 8: QNN matrix 9: QNN GGML 10: QNN complex 11: QNN GGML OP(QNN UT) 12: QNN UT automation
    * @param n_threads             1 - 8
    * @param n_backend_type        0: CPU  1: GPU  2: DSP 3: ggml("fake" QNN backend, just for compare performance)
    * @param n_op_type             type of matrix manipulate / GGML OP / type of various complex/complicated computation graph
    * @return
    */
    // renamed to ggml_jni_bench for purpose of unify JNI layer of whisper.cpp and llama.cpp
    // and QNN(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK
    void         ggml_jni_bench(const char *model_path, const char *audio_path, int n_bench_type, int num_threads, int n_backend_type, int n_op_type);


    const char * whisper_get_ggml_type_str(enum ggml_type wtype);


    // JNI helper function for ASR(whisper.cpp)
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


    // JNI helper function for llama.cpp
    /**
    *
    * @param sz_model_path         /sdcard/kantv/xxxxxx.gguf
    * @param prompt
    * @param bench_type            not used currently
    * @param n_threads             1 - 8
    * @param n_backend            0: QNN CPU 1: QNN GPU 2: QNN HTP(DSP) 3:ggml
    * @return
    */
    int          llama_inference(const char * model_path, const char * prompt, int bench_type, int num_threads, int n_backend);

    /**
     *
     * @param num_threads  1-8
     * @param n_backend            0: QNN CPU 1: QNN GPU 2: QNN HTP(DSP) 3:ggml
     */
    void         ggml_bench_matrix(int num_threads, int n_backend);

    int          ggml_bench_llama(const char * sz_model_path, int num_threads, int n_backend);




// =================================================================================================
// PoC#121:Add Qualcomm mobile SoC native backend for GGML from 03-29-2024
// =================================================================================================

    int qnn_sample_main(int argc, char** argv);

    int qnn_saver_main(int argc, char** argv);

    /**
     * @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance)
     * @param n_op_type not used in this function
     * @return
     */
    int qnn_matrix(int n_backend_type, int n_op_type);


    /**
    * @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance)
    * @param n_op_type GGML OP type
    * @return
    */
    int qnn_ggml(int n_backend_type, int n_ggml_op_type);


    /**
    * @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance)
    * @param n_graph_type type of various computation graph
    * @return
    */
    int qnn_complex_graph(int n_backend_type, int n_graph_type);


    /**
     * this special function is for PoC-S49: implementation of other GGML OP(non-mulmat) using QNN API, https://github.com/zhouwg/kantv/issues/121
     * it's similar to qnn_ggml but different with qnn_ggml, because data path in these two function is totally different
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


// JNI helper function for stablediffusion.cpp
/**
*
* @param sz_model_path         /sdcard/kantv/xxxxxx.gguf
* @param prompt
* @param bench_type            not used currently
* @param n_threads             1 - 8
* @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance)
* @return
*/
int          stablediffusion_inference(const char * model_path, const char * prompt, int bench_type, int num_threads, int n_backend_type);


#ifdef __cplusplus
}
#endif


#endif
