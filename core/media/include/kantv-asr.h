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
 */

#ifndef __KANTV_ASR_H__
#define __KANTV_ASR_H__

#include <inttypes.h>

#ifdef __cplusplus
    extern "C" {
#endif


typedef const char * ( * kantv_asr_callback)(void * opaque);
typedef const char * ( * kantv_inference_callback)(const float * pf32_audio_buffer, int num_samples);

void kantv_asr_set_callback(kantv_asr_callback callback);

void kantv_inference_set_callback(kantv_inference_callback callback);

kantv_asr_callback kantv_asr_get_callback(void);

kantv_inference_callback kantv_inference_get_callback(void);


/**
 * @param sz_saved_filename    NULL:  transcription
 *                             other: transcription and save audio data to filename for further usage
 *
 *
 */
void kantv_asr_init(const char * sz_saved_filename, uint32_t n_channels, uint32_t n_sample_rate, uint32_t n_sample_format);

void kantv_asr_finalize(void);

void kantv_asr_start(void);

void kantv_asr_stop(void);

/**
 * only used in ASRResearchFragment.java for benchmark
 */
void kantv_asr_notify_benchmark_c(const char * sz_info);

/**
 * used for realtime subtitle with online TV
 * @param sz_info
 */
void kantv_asr_notify_c(const char * sz_info);

#ifdef __cplusplus
    }
#endif


#ifdef __cplusplus

#include <string>

/**
 * only used in ASRResearchFragment.java for benchmark
 */
void kantv_asr_notify_benchmark(std::string &str_info);

#endif

#endif
