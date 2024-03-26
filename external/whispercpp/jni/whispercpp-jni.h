/*
 * Copyright (c) 2024, zhou.weiguo(zhouwg2000@gmail.com)
 *
 * Copyright (c) 2024- KanTV Authors
 *
 * this clean-room implementation is for
 *
 * PoC(https://github.com/zhouwg/kantv/issues/64) in project KanTV. the initial implementation was done
 *
 * from 03-05-2024 to 03-16-2024.the initial implementation could be found at:
 *
 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/external/whispercpp/whisper.cpp#L6727

 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/external/whispercpp/whisper.h#L620

 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/external/whispercpp/jni/whispercpp-jni.c

 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/cdeosplayer/cdeosplayer-lib/src/main/java/org/ggml/whispercpp/whispercpp.java
 *
 *
 * in short, it's a very concise implementation and the method here is never seen in any other similar
 *
 * (whisper.cpp related) open-source project before 03-05-2024.
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
 * The above statement and notice must be included in corresponding files in derived project
 */

// TODO: 03-26-2024, rename this file to ggmljni to unify the JNI of whisper.cpp and llama.cpp, as these projects are all based on ggml

#ifndef WHISPER_JNI_H
#define WHISPER_JNI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "libavutil/cde_log.h"

#ifdef __cplusplus
extern "C" {
#endif


    // JNI helper function for benchmark
    int          whisper_get_cpu_core_counts(void);
    void         whisper_set_benchmark_status(int b_exit_benchmark);
    void         whisper_bench(const char *model_path, const char *audio_path, int bench_type, int num_threads);
    const char * whisper_get_ggml_type_str(enum ggml_type wtype);

    // JNI helper function for ASR
    int          whisper_asr_init(const char *model_path, int num_threads, int n_devmode);
    void         whisper_asr_finalize(void);

    void         whisper_asr_start(void);
    void         whisper_asr_stop(void);
    int          whisper_asr_reset(const char * sz_model_path, int n_threads, int n_asrmode);

    // JNI helper function for LLAMA
    int         llama_bench(const char *model_path, const char *prompt, int bench_type, int num_threads);


#ifdef __cplusplus
}
#endif


#endif
