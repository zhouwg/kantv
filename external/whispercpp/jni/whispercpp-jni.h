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

#ifndef WHISPER_JNI_H
#define WHISPER_JNI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

    // =================================================================================================
    //
    // the following is for PoC(https://github.com/cdeos/kantv/issues/64) in project KanTV
    //
    // =================================================================================================
    // JNI helper function for benchmark
    int          whisper_get_cpu_core_counts(void);
    void         whisper_set_benchmark_status(int b_exit_benchmark);
    void         whisper_bench(const char *model_path, const char *audio_path, int bench_type, int num_threads);
    const char * whisper_get_ggml_type_str(enum ggml_type wtype);

    // JNI helper function for ASR
    void         whisper_asr_init(const char *model_path, int num_threads, int n_devmode);
    void         whisper_asr_finalize(void);





#ifdef __cplusplus
}
#endif


#endif
