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
//add new AI benchmark type / new backend using GGML inference framework here, keep sync with KANTVUtils.java

// available bench type for ggml-jni
enum ggml_jni_bench_type {
    GGML_BENCHMARK_MEMCPY = 0,                //memcpy  benchmark
    GGML_BENCHMARK_MULMAT,                    //mulmat  benchmark
    GGML_BENCHMARK_ASR,                       //ASR(using whisper.cpp based on GGML) benchmark
    GGML_BENCHMARK_LLM,                       //LLM benchmark using llama.cpp based on GGML
    GGML_BENCHMARK_MAX
};
//=============================================================================================


// =================================================================================================
// JNI helper function in ggml-jni
// =================================================================================================

#define GGML_JNI_NOTIFY(...)        ggml_jni_notify_c_impl(__VA_ARGS__)

    void         ggml_jni_notify_c_impl(const char * format, ...);
    int          ggml_jni_get_cpu_core_counts(void);

    bool         ggml_jni_abort_callback(void * data);
    void         ggml_jni_set_abortbenchmark_flag(int b_exit_benchmark);
    int          ggml_jni_get_abortbenchmark_flag(void);

    /**
     *
     * @param sz_model_path     /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/xxxxxx.gguf
     * @param sz_user_data      ASR: /sdcard/kantv/jfk.wav or LLM: user input from UI
     * @param n_bench_type      0: memcpy 1: mulmat 2: ASR(whisper.cpp) 3: LLM(llama.cpp)
     * @param n_threads         1 - 8
     * @param n_backend_type    0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU/HEXAGON_BACKEND_CDSP 3: ggml
     * @param n_accel_type      0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
     * @return
    */
    void         ggml_jni_bench(const char * sz_model_path, const char * sz_user_data, int n_bench_type, int num_threads, int n_backend_type, int n_accel_type);

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
    * @param n_backend            0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
    */
    int          whisper_asr_init(const char * sz_model_path, int n_threads, int n_asrmode, int n_backend);
    void         whisper_asr_finalize(void);

    void         whisper_asr_start(void);
    void         whisper_asr_stop(void);
    /**
    * @param sz_model_path
    * @param n_threads
    * @param n_asrmode            0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
    * @param n_backend            0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
    */
    int          whisper_asr_reset(const char * sz_model_path, int n_threads, int n_asrmode, int n_backend);


// =================================================================================================
// trying to integrate llama.cpp from 03/26/2024 to 03/28/2024
// =================================================================================================
    /**
    *
    * @param sz_model_path         /sdcard/kantv/models/xxxxxx.gguf
    * @param prompt
    * @param llm_type              not used currently
    * @param n_threads             1 - 8
    * @param n_backend             0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
    * @param n_accel_type          0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
    * @return
    */
    int          llama_inference_ng(const char * model_path, const char * prompt, int llm_type, int num_threads, int n_backend, int n_hwaccel_type);

#ifdef __cplusplus
}
#endif


#endif
