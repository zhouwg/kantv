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


#ifdef __cplusplus
    extern "C" {
#endif

#define BECHMARK_ASR                0
#define BECHMARK_MEMCPY             1
#define BECHMARK_MULMAT             2
#define BECHMARK_FULL               3


typedef const char* (*kantv_asr_callback)(void * opaque);

void kantv_asr_set_callback(kantv_asr_callback callback);

kantv_asr_callback kantv_asr_get_callback(void);


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

void kantv_asr_notify_c(const char *sz_info);

#ifdef __cplusplus
    }
#endif


#ifdef __cplusplus

#include <string>

void kantv_asr_notify(std::string &str_info);

#endif

#endif
