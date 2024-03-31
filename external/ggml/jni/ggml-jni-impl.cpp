/*
 * Copyright (c) 2024, zhou.weiguo(zhouwg2000@gmail.com)
 *
 * Copyright (c) 2024- KanTV Authors
 *
 * this clean-room implementation is for
 *
 * PoC(https://github.com/zhouwg/kantv/issues/64) in project KanTV. the initial implementation was done
 *
 * from 03-05-2024 to 03-16-2024. the initial implementation could be found at:
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

#define RUAPU_IMPLEMENTATION
#include "ruapu.h"

#include "whisper.h"

#include "llama.h"

#include "kantv-asr.h"
#include "ggml-jni.h"

#include "tinywav.h"

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"

#include <atomic>
#include <algorithm>
#include <cassert>
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <memory>
#include <vector>
#include <regex>
#include <random>
#include <functional>

extern "C" {
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
//#include <stdatomic.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"
#include "libavutil/log.h"
#include "libavutil/avutil.h"
#include "libavutil/opt.h"
#include "libavutil/samplefmt.h"
#include "libswresample/swresample.h"
#include "libavutil/myfifo.h"
#include "libavutil/cde_log.h"
#include "libavutil/cde_assert.h"

#if CONFIG_AVFILTER
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#endif

}

#define MAX_PATH_LEN                    512

#define MAX_SAMPLE_SIZE                 (1024 * 8 * 32)

#define MAX_WHISPER_IN_BUFFER_SIZE      (1024 * 1024 * 5)

class whisper_asr;

typedef struct {
    struct whisper_context *        p_context;
    struct whisper_full_params *    p_params;

    char  sz_model_path[MAX_PATH_LEN];
    size_t n_threads;

    //03-20-2024,referenced by:https://github.com/futo-org/whisper-acft
    size_t n_decoding_mode;                          // 0:WHISPER_SAMPLING_GREEDY 1:WHISPER_SAMPLING_BEAM_SEARCH

    size_t n_asr_mode;                               // 0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
    size_t n_benchmark_type;                         // what to benchmark:
                                                     // 0: asr(transcription) 1: memcpy 2: mulmat  3: full/whisper_encode 4: matrix  5: LLAMA inference

    bool   b_use_gpu;                                // TODO: not used on Android device currently, ref: https://github.com/ggerganov/ggml/issues/771

    bool   b_abort_benchmark;                        // TODO: for abort time-consuming task from UI layer. not works as expected

    fifo_buffer_t   * asr_fifo;                      // fifo for ASR data producer-consumer

    size_t   n_sample_size;

    struct SwrContext * swr_ctx;

    uint8_t  p_sample_buffer[MAX_SAMPLE_SIZE];       // temp buffer for convert audio frame
    uint8_t  p_audio_buffer[MAX_SAMPLE_SIZE];        // temp buffer for convert audio frame

    class whisper_asr * p_asr;                       // attention memory leak, smart pointer should not be used here for performance consideration

    pthread_mutex_t  mutex;                          // not used since 03-19-2024

    //only for troubleshooting issue
    bool     b_pre_convert;
    bool     b_enable_dump_16k_data;
} whisper_asr_context;

static whisper_asr_context *p_asr_ctx   = NULL;

static fifo_buffer_t  * whisper_asr_getfifo();
static const char     * whisper_asr_callback(void * opaque);
static const char     * whisper_asr_audio_to_text(const float * pf32_audio_buffer, int num_samples);


// =================================================================================================
//
// internal helper function
//
// some codes referenced with codes original project whisper.cpp
//
// =================================================================================================
//
static std::string to_timestamp(int64_t t, bool comma = false) {
    int64_t msec = t * 10;
    int64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    int64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    int64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int) hr, (int) min, (int) sec, comma ? "," : ".", (int) msec);

    return std::string(buf);
}


/**
 * @param file_name
 * @param pp_data
 * @param p_datalen
 *
 * @return  0  : ok , other return value means failed to read binary data from file
 */
static int read_data_from_file(const char * sz_file_name, uint8_t ** pp_data, size_t * p_datalen) {
    int result          = 0;
    FILE* fp            = NULL;
    uint8_t * p_data    = NULL;
    uint32_t datalen    = 0;

    if ((NULL == sz_file_name) || (NULL == pp_data) || (NULL == p_datalen)) {
        result = -1;
    }

    if (0 == result) {
        LOGGD("open file: %s", sz_file_name);
        fp = fopen(sz_file_name, "rb");
        if (NULL == fp) {
            result = -errno;
            LOGD("open file %s failed(reason:%s)", file_name, strerror(errno));
        }
    }

    if (0 == result) {
        fseek(fp, 0, SEEK_END);
        datalen = (uint32_t) ftell(fp);
        LOGGD("file size %d\n", datalen);
        if (0 == datalen) {
            LOGGW("pls check why size is zero of file %s\n", sz_file_name);
            result = -2;
        }
    }

    if (0 == result) {
        p_data = (uint8_t *) malloc(datalen);
        if (NULL == p_data) {
            LOGGW("malloc memory failure, pls check why?");
            result = -3;
        }
    }

    if (0 == result) {
        fseek(fp, 0, SEEK_SET);
        datalen = (uint32_t)fread(p_data, 1, datalen, fp);
    }

    if (0 == result) {
        *pp_data    = p_data;
        *p_datalen  = datalen;
        p_data      = NULL;
    }

    if (NULL != fp) {
        fclose(fp);
        fp = NULL;
    }

    //attention here, just for avoid memory leak
    if (NULL != p_data) {
        free(p_data);
        p_data = NULL;
    }

    return result;
}


//2024-03-08, zhou.weiguo, integrate customized FFmpeg to whisper.cpp:
//(1) it would be very important for PoC stage 3 in project KanTV(https://github.com/zhouwg/kantv/issues/64)
//(2) customized FFmpeg would/might be heavily used within customized whisper.cpp in the future


// ./ffmpeg  -i ./test.mp4 -ac 2 -ar 16000 test.wav
// ./main  -m ./models/ggml-base.bin  -f ./test.wav
//this is a reverse engineering hardcode function and should not be used in real project
static float * convert_to_float(uint8_t * in_audio_data, size_t in_audio_size, size_t * out_sample_counts) {
    int result = 0;
    int in_audio_channels       = 0;
    int in_sample_counts        = 0;
    int in_sample_rates         = 0;
    int in_sample_bits          = 0;
    int out_audio_channels      = 0;
    int out_sample_rates        = 0;
    int dst_sample_counts       = 0;
    struct SwrContext *swr_ctx  = NULL;
    uint8_t *in                 = NULL;
    uint8_t *payload            = NULL;
    size_t payload_len          = 0;
    size_t skip_len             = 0;
    int i                       = 0;
    float *out_audio_data       = NULL;

    if ((NULL == in_audio_data) || (NULL == out_sample_counts)) {
        LOGGW("pls check params");
        return NULL;
    }

    in                              = in_audio_data;
    in_audio_channels               = (in[23] << 8) | in[22];
    in_sample_rates                 = (in[27] << 24) | (in[26] << 16) | (in[25] << 8) | in[24];
    in_sample_bits                  = (in[35] << 8) | in[34];
    skip_len                        = (in[43] << 24) | (in[42] << 16) | (in[41] << 8) | in[40];
    payload_len                     = (in[51 + skip_len] << 24) | (in[50 + skip_len] << 16) | (in[49 + skip_len] << 8) | in[48 + skip_len];
    LOGGD("%c %c %c %c\n", in[44 + skip_len], in[45 + skip_len], in[46 + skip_len], in[47 + skip_len]);
    LOGGD("in_sample_rates:    %d\n", in_sample_rates);
    LOGGD("in_audio_channels:  %d\n", in_audio_channels);
    LOGGD("in_sample_bits:     %d\n", in_sample_bits);
    LOGGD("in_audio_size:      %d\n", in_audio_size);
    LOGGD("skip_len:           %d\n", skip_len);
    LOGGD("payload_len:        %d\n", payload_len);
    payload                         = in_audio_data + 44 + skip_len + 8;
    payload_len                     = in_audio_size - 44 - skip_len - 8;
    LOGGD("payload_len:        %d\n", payload_len);
    in_sample_counts                = payload_len / 2;
    *out_sample_counts              = payload_len / 2;
    out_audio_channels              = in_audio_channels;
    out_sample_rates                = in_sample_rates;
    LOGGD("in_sample_counts:   %d\n", in_sample_counts);
    HEXDUMP(in_audio_data, in_audio_size);
    HEXDUMP(payload, payload_len);

    swr_ctx = swr_alloc();
    if (NULL == swr_ctx) {
        LOGGW("could not allocate FFmpeg resample context\n");
        return NULL;
    }

    av_opt_set_int       (swr_ctx, "in_channel_count",    in_audio_channels,  0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",      in_sample_rates,    0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",   AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (swr_ctx, "out_channel_count",   out_audio_channels,0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",     out_sample_rates,  0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",  AV_SAMPLE_FMT_FLT, 0);
    if ((result = swr_init(swr_ctx)) < 0) {
        LOGGI( "Failed to initialize the resampling context\n");
        goto failure;
    }

    out_audio_data  = (float*)malloc( (in_sample_counts) * sizeof(float));
    if (NULL == out_audio_data) {
        LOGGW("malloc failed\n");
        goto failure;
    }

    dst_sample_counts = av_rescale_rnd(swr_get_delay(swr_ctx, in_sample_rates) + in_sample_counts, out_sample_rates, in_sample_rates, AV_ROUND_UP);
    LOGGI("dst_sample_counts %d\n", dst_sample_counts);
    if (dst_sample_counts != *out_sample_counts) {
        LOGGW("it shouldn't happen, pls check");
    }

    result = swr_convert(swr_ctx,
                         (uint8_t **)(&out_audio_data),
                         *out_sample_counts,
                         (const uint8_t**)(&payload),
                         in_sample_counts);

    if (result < 0) {
        LOGGW("audio sample convert failure");
        free(out_audio_data);
        out_audio_data = NULL;
    }
    LOGGD("audio sample convert's result: %d\n", result);

failure:
    if (NULL != swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = NULL;
    }

    return out_audio_data;
}


const char * whisper_get_ggml_type_str(enum ggml_type wtype) {
    switch (wtype) {
        case GGML_TYPE_Q4_0:
            return "GGML_TYPE_Q4_0";
        case GGML_TYPE_Q4_1:
            return "GGML_TYPE_Q4_1";
        case GGML_TYPE_Q5_0:
            return "GGML_TYPE_Q5_0";
        case GGML_TYPE_Q5_1:
            return "GGML_TYPE_Q5_1";
        case GGML_TYPE_Q8_0:
            return "GGML_TYPE_Q8_0";
        case GGML_TYPE_F16:
            return "GGML_TYPE_F16";
        case GGML_TYPE_F32:
            return "GGML_TYPE_F32";
        default:
            return "unknown";
    }
}


static const char * whisper_sampleformat_string(int format)
{
    switch (format)
    {
        case AV_SAMPLE_FMT_U8:
            return "AV_SAMPLE_FMT_U8";          ///< unsigned 8 bits
        case AV_SAMPLE_FMT_S16:
            return "AV_SAMPLE_FMT_S16";         ///< signed 16 bits
        case AV_SAMPLE_FMT_S32:
            return "AV_SAMPLE_FMT_S32";         ///< signed 32 bits
        case AV_SAMPLE_FMT_FLT:
            return "AV_SAMPLE_FMT_FLT";         ///< float
        case AV_SAMPLE_FMT_DBL:
            return "AV_SAMPLE_FMT_DBL";
        case AV_SAMPLE_FMT_U8P:
            return "AV_SAMPLE_FMT_U8P";         ///< unsigned 8 bits, planar
        case AV_SAMPLE_FMT_S16P:
            return "AV_SAMPLE_FMT_S16P";        ///< signed 16 bits, planar
        case AV_SAMPLE_FMT_S32P:
            return "AV_SAMPLE_FMT_S32P";        ///< signed 32 bits, planar
        case AV_SAMPLE_FMT_FLTP:
            return "AV_SAMPLE_FMT_FLTP";        ///< float, planar
        case AV_SAMPLE_FMT_DBLP:
            return "AV_SAMPLE_FMT_DBLP";        ///< double, planar
        case AV_SAMPLE_FMT_S64:
            return "AV_SAMPLE_FMT_S64";         ///< signed 64 bits
        case AV_SAMPLE_FMT_S64P:
            return "AV_SAMPLE_FMT_S64P";        ///< signed 64 bits, planar

    }
    return "unknown audio sample format";
}


static std::string whisper_get_time_string()
{
    auto to_string = [](const std::chrono::system_clock::time_point& t)->std::string
    {
        auto as_time_t = std::chrono::system_clock::to_time_t(t);
        struct tm tm;

        localtime_r(&as_time_t, &tm);

        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(t.time_since_epoch());
        char buf[128];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d %03lld ",
                 tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, ms.count() % 1000);
        return buf;
    };

    std::chrono::system_clock::time_point t = std::chrono::system_clock::now();
    return to_string(t);
}


static bool whisper_abort_callback(void * data) {
    if (NULL == p_asr_ctx)
        return false;

    return p_asr_ctx->b_abort_benchmark;
}


static int whisper_bench_full() {
    if (NULL == p_asr_ctx) {
        LOGGW("pls check whether asr_ctx already initialized?\n");
        return 1;
    }

    // whisper init
    struct whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = p_asr_ctx->b_use_gpu;

    struct whisper_context * ctx = whisper_init_from_file_with_params(p_asr_ctx->sz_model_path, cparams);
    LOGGD("system_info: n_threads = %d / %d | %s\n", p_asr_ctx->n_threads, std::thread::hardware_concurrency(), whisper_print_system_info());
    kantv_asr_notify_benchmark_c("start whisper bench full\n");
    if (ctx == nullptr) {
        LOGGW("error: failed to initialize whisper context\n");
        return 2;
    }

    const int n_mels = whisper_model_n_mels(ctx);
    if (int ret = whisper_set_mel(ctx, nullptr, 0, n_mels)) {
        LOGGW("error: failed to set mel: %d\n", ret);
        return 3;
    }

    kantv_asr_notify_benchmark_c("start whisper_encode\n");
    // heat encoder
    if (int ret = whisper_encode(ctx, 0, p_asr_ctx->n_threads) != 0) {
        LOGGW("error: failed to encode: %d\n", ret);
        return 4;
    }

    whisper_token tokens[512];
    memset(tokens, 0, sizeof(tokens));

    kantv_asr_notify_benchmark_c("start whisper_decode\n");
    // prompt heat
    if (int ret = whisper_decode(ctx, tokens, 256, 0, p_asr_ctx->n_threads) != 0) {
        LOGGW("error: failed to decode: %d\n", ret);
        return 5;
    }

    // text-generation heat
    if (int ret = whisper_decode(ctx, tokens, 1, 256, p_asr_ctx->n_threads) != 0) {
        LOGGW( "error: failed to decode: %d\n", ret);
        return 6;
    }

    whisper_reset_timings(ctx);

    // actual run
    if (int ret = whisper_encode(ctx, 0, p_asr_ctx->n_threads) != 0) {
        LOGGW( "error: failed to encode: %d\n", ret);
        return 7;
    }

    // text-generation
    for (int i = 0; i < 256; i++) {
        if (int ret = whisper_decode(ctx, tokens, 1, i, p_asr_ctx->n_threads) != 0) {
            LOGGW( "error: failed to decode: %d\n", ret);
            return 8;
        }
    }

    // batched decoding
    for (int i = 0; i < 64; i++) {
        if (int ret = whisper_decode(ctx, tokens, 5, 0, p_asr_ctx->n_threads) != 0) {
            LOGGW( "error: failed to decode: %d\n", ret);
            return 9;
        }
    }

    // prompt processing
    for (int i = 0; i < 16; i++) {
        if (int ret = whisper_decode(ctx, tokens, 256, 0, p_asr_ctx->n_threads) != 0) {
            LOGGW( "error: failed to decode: %d\n", ret);
            return 10;
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}


/**
 * @param model_path       /sdcard/kantv/ggml-xxxxx.bin
 * @param audio_path       /sdcard/kantv/jfk.wav
 * @param num_threads      1 - 8
 *
 * @return asr result which generated by whispercpp's inference
 */
static const char * whisper_transcribe_from_file(const char * sz_model_path, const char * sz_audio_path, int num_threads) {
    struct whisper_context * context                = nullptr;
    struct whisper_full_params whisper_params;
    uint8_t * audio_data                            = NULL;
    size_t   audio_size                             = 0;
    float   * float_audio_data                      = NULL;
    size_t   float_sample_counts                    = 0;
    int      result                                 = 0;
    int      num_segments                           = 0;
    int      index                                  = 0;
    int64_t  begin_time                             = 0LL;
    int64_t  end_time                               = 0LL;
    int64_t  t0_segment                             = 0;
    int64_t  t1_segment                             = 0;
    const char * text                               = NULL;

    //TODO: static variable should not be used in real project
    static std::string asr_result;

    if ((NULL == sz_model_path) || (NULL == sz_audio_path)) {
        LOGGW("pls check params");
        return NULL;
    }

    LOGGI("mode path:   %s\n", sz_model_path);
    LOGGI("audio file:  %s\n", sz_audio_path);
    LOGGI("num threads: %d\n", num_threads);

    asr_result = "";

    result = read_data_from_file(sz_audio_path, &audio_data, &audio_size);
    if (0 != result) {
        LOGGW("read data from file %s failure,pls check why?\n", sz_audio_path);
        result = -2;
        goto failure;
    }
    LOGGD("audio size %d\n", audio_size);
    float_audio_data = convert_to_float(audio_data, audio_size, &float_sample_counts);
    if (NULL == float_audio_data) {
        LOGGW("convert audio sample failure,pls check why?\n");
        result = -3;
        goto failure;
    }
    LOGGD("float_sample_counts %d\n", float_sample_counts);

    if (0) { //2024-03-15,16:28, validate whether whisper_asr_audio_to_text can works well as expected during PoC stage
        text = whisper_asr_audio_to_text(float_audio_data, float_sample_counts);
        if (NULL != text) {
            LOGGD("asr reulst:\n%s\n", text);
            asr_result = text;
#ifdef TARGET_ANDROID
            kantv_asr_notify_benchmark(asr_result);
#endif
        } else {
            LOGGD("whisper_asr_audio_to_text failed\n");
            result = -1;
            goto failure;
        }

    } else {
        context = whisper_init_from_file(sz_model_path);
        if (nullptr == context) {
            LOGGW("whisper_init_from_file failure, pls check why\n");
            GGML_JNI_NOTIFY("whisper_init_from_file failure, pls check why(pls check whether whispercpp model is valid)\n");
            result = -1;
            goto failure;
        }

        whisper_params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
        whisper_params.print_realtime = true;
        whisper_params.print_progress = true;
        whisper_params.print_timestamps = true;
        whisper_params.print_special = false;
        whisper_params.translate = false;
        whisper_params.language = "en";
        whisper_params.n_threads = num_threads;
        whisper_params.offset_ms = 0;
        whisper_params.no_context = true;
        whisper_params.single_segment = false;
        whisper_reset_timings(context);

        LOGGV("calling whisper_full\n");
        begin_time = ggml_time_ms();
        result = whisper_full(context, whisper_params, float_audio_data, float_sample_counts);
        if (0 != result) {
            LOGGW("inference failure, pls check why?\n");
            result = -4;
            goto failure;
        }
        LOGGI("whispercpp inference successfully\n");
        //whisper_print_timings(context);
        num_segments = whisper_full_n_segments(context);
        LOGGD("num_segments:%d\n", num_segments);
        for (index = 0; index < num_segments; index++) {
            text = whisper_full_get_segment_text(context, index);
            if (NULL == text) {
                LOGGW("whisper_full_get_segment_text failure, pls check why\n");
                result = -5;
                goto failure;
            }
            t0_segment = whisper_full_get_segment_t0(context, index);
            t1_segment = whisper_full_get_segment_t1(context, index);

            std::string cur_line = "";
            LOGGV("text[%d]:[%s --> %s] %s\n",
                  index,
                  to_timestamp(t0_segment).c_str(),
                  to_timestamp(t1_segment).c_str(),
                  text);
            cur_line +=
                    "[ " + to_timestamp(t0_segment) + " ---> " + to_timestamp(t1_segment) + "  ]" +
                    std::string(text);
            asr_result += cur_line + "\n";
            LOGGD("asr result:\n%s\n", cur_line.c_str());
        }
        end_time = ggml_time_ms();
        LOGGI("inference cost %d ms\n", end_time - begin_time);
        LOGGV("after calling whisper_full\n");
    }

failure:
    if (NULL != float_audio_data) {
        free(float_audio_data);
        float_audio_data = NULL;
    }

    if (NULL != audio_data) {
        free(audio_data);
        audio_data = NULL;
    }

    if (nullptr != context) {
        whisper_free(context);
    }

    if (0 == result) {
#ifdef TARGET_ANDROID
        kantv_asr_notify_benchmark(asr_result);
#endif
        return asr_result.c_str();
    } else
        return NULL;
}


static fifo_buffer_t  *whisper_asr_getfifo() {
    if (NULL == p_asr_ctx)
        return NULL;

    if (NULL == p_asr_ctx->asr_fifo)
        return NULL;

    return p_asr_ctx->asr_fifo;
}

// 03-26-2024,
// this function was referenced by this PR:https://github.com/ggerganov/llama.cpp/pull/5935/
// double check although this special case has been handled at the JNI layer
static bool is_valid_utf8(const char * string) {
    if (!string) {
        return true;
    }

    const unsigned char * bytes = (const unsigned char *)string;
    int num;

    while (*bytes != 0x00) {
        if ((*bytes & 0x80) == 0x00) {
            // U+0000 to U+007F
            num = 1;
        } else if ((*bytes & 0xE0) == 0xC0) {
            // U+0080 to U+07FF
            num = 2;
        } else if ((*bytes & 0xF0) == 0xE0) {
            // U+0800 to U+FFFF
            num = 3;
        } else if ((*bytes & 0xF8) == 0xF0) {
            // U+10000 to U+10FFFF
            num = 4;
        } else {
            return false;
        }

        bytes += 1;
        for (int i = 1; i < num; ++i) {
            if ((*bytes & 0xC0) != 0x80) {
                return false;
            }
            bytes += 1;
        }
    }

    return true;
}

// =================================================================================================
//
// JNI helper function for benchmark
// referenced with codes in examples/bench/bench.cpp of project whisper.cpp
//
// =================================================================================================
int whisper_get_cpu_core_counts() {
    return std::thread().hardware_concurrency();
}


void whisper_set_benchmark_status(int b_exit_benchmark) {
    LOGGI("set b_abort_benchmark to %d", b_exit_benchmark);

    if (NULL == p_asr_ctx)
        return;

    p_asr_ctx->b_abort_benchmark = ((1 == b_exit_benchmark) ? true : false);
}


/**
 *
 * @param sz_model_path         /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/kantv/xxxxxx.gguf or qualcomm's dedicated model
 * @param sz_audio_path         /sdcard/kantv/jfk.wav
 * @param n_bench_type          0: asr(transcription) 1: memcpy 2: mulmat  3: full/whisper_encode 4: matrix  5: LLAMA 6: QNN
 * @param n_threads             1 - 8
 * @param n_backend_type        0: CPU  1: GPU  2: DSP
 * @return
*/
void ggml_jni_bench(const char * sz_model_path, const char *sz_audio_path, int n_bench_type, int n_threads, int n_backend_type) {
    int result = 0;

    if (NULL == p_asr_ctx) {
        LOGGW("pls check whether asr_ctx already initialized?\n");
        return;
    }

    if (NULL == sz_model_path) {
        LOGGW("pls check model path\n");
        return;
    }

    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("backend type:%d\n", n_backend_type);

    p_asr_ctx->b_use_gpu                = false;        // TODO:not used currently
    p_asr_ctx->n_threads                = n_threads;
    p_asr_ctx->n_benchmark_type         = n_bench_type;
    memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
    strncpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));

    kantv_asr_notify_benchmark_c("reset");

    switch (n_bench_type) {
        case BECHMARK_ASR:
            if (NULL == sz_audio_path) {
                LOGGW("pls check audio data path\n");
                return;
            }
            whisper_transcribe_from_file(sz_model_path, sz_audio_path, n_threads);
            break;

        case BECHMARK_MEMCPY:
            whisper_bench_memcpy(n_threads);
            break;

        case BECHMARK_MULMAT:
            whisper_bench_ggml_mul_mat(n_threads);
            break;

        case BECHMARK_FULL:
            whisper_bench_full();
            break;

        case BENCHMARK_MATRIX:
            ggml_bench_matrix(n_threads);
            break;

        case BENCHMAKR_LLAMA:
            ggml_bench_llama(sz_model_path, n_threads);
            break;

        case BENCHMAKR_QNN:
            {
                //TODO: this is a lazy method in PoC stage
                int argc = 11;
                char *qnn_backend_lib = "/data/data/com.cdeos.kantv/libQnnCpu.so";
                switch (n_backend_type) {
                    case 0:
                        qnn_backend_lib = "/data/data/com.cdeos.kantv/libQnnCpu.so";
                        break;
                    case 1:
                        qnn_backend_lib = "/data/data/com.cdeos.kantv/libQnnGpu.so";
                        break;
                    case 2:
                        qnn_backend_lib = "/data/data/com.cdeos.kantv/libQnnDsp.so";
                        break;
                    default:
                        LOGGW("backend type %d not supported\n", n_backend_type);
                        break;
                }

                char *argv[] = {"qnn-net-run", "--backend", qnn_backend_lib,
                                "--model", const_cast<char *>(sz_model_path),
                                "--input_list", "/sdcard/kantv/raw_list.txt",
                                "--output_dir", "/sdcard/kantv/qnn/",
                                "--log_level", "debug"
                               };
                //TODO:
                // problem occurs when switching back and forth between cpu backend and gpu backend
                // dsp backend not work
                qnn_sample_main(argc, argv); //works on Xiaomi 14 on 03-30-2024,18:09 at the first time
            }
            break;

        default:
            break;
    }
}




// =================================================================================================
//
// ASR for real-time subtitle in UI layer
//
// the following code is just for PoC of realtime subtitle with online TV and SHOULD NOT be used in real project
// =================================================================================================
class whisper_asr {
public:

    whisper_asr() {
        _p_whisper_in               = _whisper_in;
        _n_whisper_in_size          = 0;
        _n_total_sample_counts      = 0;

        _n_channels                 = 0;
        _n_sample_rate              = 0;

        _b_exit_thread              = false;

        _asr_fifo                   = whisper_asr_getfifo();
        CHECK(NULL != _asr_fifo);
    }

    ~whisper_asr() {

    }

    void start() {
        _thread_poll = std::thread(&whisper_asr::threadPoll, this);
    }

    void threadPoll() {
        buf_element_t *buf      = NULL;
        const char *asr_result  = NULL;
        int64_t  n_begin_time   = 0LL;
        int64_t  n_end_time     = 0LL;
        int64_t  n_durtion      = 0LL;
        uint8_t  *p_samples     = NULL;

        int result              = 0;
        char sz_filename[256];

        _p_whisper_in           = _whisper_in;
        _n_whisper_in_size      = 0;
        _n_total_sample_counts  = 0;

        _swr_ctx                = swr_alloc();
        CHECK(NULL != _swr_ctx);

        memset(sz_filename, 0, 256);

        //only for troubleshooting issue
        _b_enable_dump_raw_data = false;
        _b_enable_dump_16k_data = false;

        n_begin_time = ggml_time_us();

        while (1) {
            if (_b_exit_thread)
                break;

            n_durtion = 0LL;

            buf = _asr_fifo->get(_asr_fifo);
            if (buf != NULL) {
                memcpy(_whisper_in + _n_whisper_in_size, buf->mem, buf->size);
                _p_whisper_in           += buf->size;
                _n_whisper_in_size      += buf->size;
                _n_total_sample_counts  += buf->samplecounts;
                _n_channels             = buf->channels;
                _n_sample_rate          = buf->samplerate;
                _sample_format          = buf->sampleformat;
                buf->free_buffer(buf);
            } else {
                continue;
            }

            n_end_time = ggml_time_us();
            n_durtion = (n_end_time - n_begin_time) / 1000;

            // 1 second, very good on Xiaomi 14, about 500-700 ms with GGML model ggml-tiny.en-q8_0.bin
            // 300 -900 ms are both ok with latest upstream whisper.cpp(as of 03-22-2024), but whisper.cpp would produce sketchy/incorrect/repeat tokens
            if (n_durtion > 300) {
                LOGGD("duration of audio data gathering is: %d milliseconds\n", n_durtion);
                LOGGD("size of gathered audio data: %d\n", _n_whisper_in_size);
                LOGGD("total audio sample counts %d\n", _n_total_sample_counts);

                if (_b_enable_dump_raw_data) {
                    //this dump file is ok
                    snprintf(sz_filename, 256, "/sdcard/kantv/raw.wav");
                    TinyWavSampleFormat twSF    = TW_FLOAT32;
                    TinyWavChannelFormat twCF   = TW_INTERLEAVED;

                    if (_sample_format == AV_SAMPLE_FMT_FLT)
                        twSF = TW_FLOAT32;
                    if (_sample_format == AV_SAMPLE_FMT_S16)
                        twSF = TW_INT16;

                    if (1 == _n_channels)
                        twCF = TW_INLINE;
                    if (2 == _n_channels)
                        twCF = TW_INTERLEAVED;

                    tinywav_open_write(&_st_tinywav_raw, _n_channels, _n_sample_rate, twSF,twCF, sz_filename);
                    tinywav_write_f(&_st_tinywav_raw, _whisper_in, _n_total_sample_counts);
                    tinywav_close_write(&_st_tinywav_raw);
                    LOGGD("dump raw audio data to file %s\n", sz_filename);

                    _b_enable_dump_raw_data = false;
                }


                av_opt_set_int(_swr_ctx, "in_channel_count", _n_channels, 0);
                av_opt_set_int(_swr_ctx, "in_sample_rate", _n_sample_rate, 0);
                av_opt_set_sample_fmt(_swr_ctx, "in_sample_fmt", _sample_format, 0);
                av_opt_set_int(_swr_ctx, "out_channel_count", 1, 0);
                av_opt_set_int(_swr_ctx, "out_sample_rate", 16000, 0);
                av_opt_set_sample_fmt(_swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
                if ((result = swr_init(_swr_ctx)) < 0) {
                    LOGGW("failed to initialize the resampling context\n");
                    continue;
                }

                uint8_t *in_buf[] = {_whisper_in, NULL};
                p_samples = _u8_audio_buffer;
                result = swr_convert(_swr_ctx,
                                     (uint8_t **) (&p_samples),
                                     _n_total_sample_counts,
                                     (const uint8_t **) (in_buf),
                                     _n_total_sample_counts);

                if (result < 0) {
                    LOGGW("resample failed, pls check why?\n");
                    continue;
                }

                //LOGGD("got resampled samples:%d, total samples:%d \n", result, _n_total_sample_counts);
                while (1) {
                    p_samples += (result * sizeof(float));
                    result = swr_convert(_swr_ctx,
                                         (uint8_t **) (&p_samples),
                                         _n_total_sample_counts,
                                         NULL,
                                         0);
                    //LOGGD("got resampled samples:%d, total samples:%d \n", result, _n_total_sample_counts);
                    if (0 == result) {
                        break;
                    }
                }

                if (_b_enable_dump_16k_data) {  //2024-03-16,13:05, finally, this dump file is also ok and then ASR subtitle is ok
                    memset(sz_filename, 0, 256);
                    snprintf(sz_filename, 256, "/sdcard/kantv/16k_out.wav");
                    tinywav_open_write(&_st_tinywav_16k, 1, 16000, TW_FLOAT32, TW_INLINE, sz_filename);
                    tinywav_write_f(&_st_tinywav_16k, _u8_audio_buffer, _n_total_sample_counts);
                    tinywav_close_write(&_st_tinywav_16k);
                    LOGGD("dump converted audio data to file %s\n", sz_filename);
                    _b_enable_dump_16k_data = false;
                }

                p_samples = _u8_audio_buffer;
                whisper_asr_audio_to_text(reinterpret_cast<const float *>(p_samples), _n_total_sample_counts);

                n_end_time              = ggml_time_us();
                n_begin_time            = n_end_time;
                _p_whisper_in           = _whisper_in;
                _n_whisper_in_size      = 0;
                _n_total_sample_counts  = 0;
            }

            usleep(10);
        }

        swr_free(&_swr_ctx);
    }

    void stop() {
        _b_exit_thread = true;
        if (_thread_poll.joinable()) {
            _thread_poll.join();
        }
        _b_exit_thread = false;
    }

private:
    std::thread _thread_poll;
    bool        _b_exit_thread;

    int         _n_channels;
    int         _n_sample_rate;

    int         _n_total_sample_counts;

    //buffer for gather sufficient audio data ( > 1 seconds)
    uint8_t     * _p_whisper_in;
    uint8_t     _whisper_in[MAX_WHISPER_IN_BUFFER_SIZE];
    int         _n_whisper_in_size;

    //temp buffer for resample audio data in _whisper_in from stero + f32 + 48KHz to mono + f32 + 16kHz for purpose of make whispercpp happy
    uint8_t     _u8_audio_buffer[MAX_WHISPER_IN_BUFFER_SIZE * 2];


    fifo_buffer_t * _asr_fifo;
    struct SwrContext * _swr_ctx;
    enum AVSampleFormat _sample_format;


    //only for troubleshooting issue
    TinyWav     _st_tinywav_raw;
    TinyWav     _st_tinywav_16k;
    bool        _b_enable_dump_raw_data;
    bool        _b_enable_dump_16k_data;
};


/**
 *
 * @param  opaque          uncompressed pcm data, presented as AVFrame
 *
 * @return always NULL, or "tip information" in pressure test mode
 *
 * this function should NOT do time-consuming operation, otherwise unexpected behaviour would happen
 *
 *
 */
static const char * whisper_asr_callback(void * opaque) {
    size_t frame_size               = 0;
    size_t sample_size              = 0;
    size_t samples_size             = 0;

    AVFrame * audioframe            = NULL;
    fifo_t *  asr_fifo              = NULL;

    uint8_t * p_samples             = NULL;
    int num_samples                 = 0;
    int result                      = 0;
    enum AVSampleFormat sample_format;


    if (NULL == opaque)
        return NULL;

    if ((NULL == p_asr_ctx))
        return NULL;

    if (1 == p_asr_ctx->n_asr_mode) { //ASR pressure test during online-TV playback
        static std::string test_info;

        test_info = whisper_get_time_string() + "\n" +
                    " First line:this is ASR pressure test powered by whisper.cpp(https://github.com/ggerganov/whisper.cpp)\n "
                    "second line:thanks for your interesting in project KanTV(https://github.com/zhouwg/kantv)\n"
                    " third line:have fun with great and amazing whisper.cpp\n";
        return test_info.c_str();
    }

    if (2 == p_asr_ctx->n_asr_mode) { //ASR benchmark in standalone ASRResearchFragment.java
        return NULL;
    }

    //pthread_mutex_lock(&p_asr_ctx->mutex); // remove the mutex since 03-19-2024, crash would happen before 03-19 without mutex

    audioframe  = (AVFrame *) opaque;
    num_samples = audioframe->nb_samples;

    frame_size = av_samples_get_buffer_size(NULL, audioframe->channels, audioframe->nb_samples,
                                           static_cast<AVSampleFormat>(audioframe->format), 1);

    LOGD("Audio AVFrame(size %d): "
          "nb_samples %d, sample_rate %d, channels %d, channel_layout %d, "
          "format 0x%x(%s), pts %lld,"
          "pitches[0] %d, pitches[1] %d, pitches[2] %d, "
          "pixels[0] %p, pixels[1] %p, pixels[2] %p \n",
          frame_size,
          audioframe->nb_samples, audioframe->sample_rate, audioframe->channels,
          audioframe->channel_layout,
          audioframe->format, whisper_sampleformat_string(audioframe->format), audioframe->pts,
          audioframe->linesize[0], audioframe->linesize[1], audioframe->linesize[2],
          audioframe->data[0], audioframe->data[1], audioframe->data[2]
    );

    sample_size = av_get_bytes_per_sample(static_cast<AVSampleFormat>(audioframe->format));
    p_samples   = p_asr_ctx->p_sample_buffer;
    if (av_sample_fmt_is_planar(static_cast<AVSampleFormat>(audioframe->format))) {
        for (int i = 0; i < audioframe->nb_samples; i++) {
            for (int ch = 0; ch < audioframe->channels; ch++) {
                memcpy(p_samples, audioframe->data[ch] + sample_size * i, sample_size);
                p_samples       += sample_size;
                samples_size    += sample_size;
            }
        }
    } else {
        memcpy(p_samples, audioframe->data[0], audioframe->linesize[0]);
        samples_size = audioframe->linesize[0];
    }
    p_samples = p_asr_ctx->p_sample_buffer; //reset pointer

    sample_format = av_get_packed_sample_fmt(static_cast<AVSampleFormat>(audioframe->format));
    CHECK(sample_format == AV_SAMPLE_FMT_FLT);//make whisper happy
    CHECK(samples_size == num_samples * sizeof(float) * audioframe->channels);


    /*
    if (p_asr_ctx->b_pre_convert) { //only for troubleshooting issue, not work as expected
        av_opt_set_int(p_asr_ctx->swr_ctx, "in_channel_count", audioframe->channels, 0);
        av_opt_set_int(p_asr_ctx->swr_ctx, "in_sample_rate", audioframe->sample_rate, 0);
        av_opt_set_sample_fmt(p_asr_ctx->swr_ctx, "in_sample_fmt", static_cast<AVSampleFormat>(audioframe->format), 0);
        av_opt_set_int(p_asr_ctx->swr_ctx, "out_channel_count", 1, 0);
        av_opt_set_int(p_asr_ctx->swr_ctx, "out_sample_rate", 16000, 0);
        av_opt_set_sample_fmt(p_asr_ctx->swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);
        if ((result = swr_init(p_asr_ctx->swr_ctx)) < 0) {
            LOGGW("failed to initialize the resampling context\n");
        }
        uint8_t *out_buf[] = {p_asr_ctx->p_audio_buffer, NULL};
        result = swr_convert(p_asr_ctx->swr_ctx,
                             (uint8_t **)(out_buf),
                             audioframe->nb_samples,
                             (const uint8_t **) (audioframe->data),
                             audioframe->nb_samples);
        if (result < 0) {
            LOGGW("resample failed, pls check why?\n");
            pthread_mutex_unlock(&p_asr_ctx->mutex);

            return NULL;
        }


        if (p_asr_ctx->b_enable_dump_16k_data) {
            char sz_filename[256];
            TinyWav _st_tinywav_16k;
            snLOGGV(sz_filename, 256, "/sdcard/kantv/16k_in.wav");
            tinywav_open_write(&_st_tinywav_16k, 1, 16000, TW_FLOAT32, TW_INLINE, sz_filename);
            tinywav_write_f(&_st_tinywav_16k, p_asr_ctx->p_audio_buffer, audioframe->nb_samples);
            tinywav_close_write(&_st_tinywav_16k);
            LOGGD("dump pre-convert 16k audio data to file %s\n", sz_filename);
            p_asr_ctx->b_enable_dump_16k_data = false;
        }

        p_samples   = p_asr_ctx->p_audio_buffer; //attention here
        asr_fifo    = whisper_asr_getfifo();
        if (NULL != asr_fifo) {
            buf_element_t *buf = asr_fifo->buffer_try_alloc(asr_fifo);
            if (NULL != buf) {
                memcpy(buf->mem, p_samples, samples_size);
                buf->size           = 4 * audioframe->nb_samples;
                buf->samplecounts   = audioframe->nb_samples;
                buf->channels       = 1;
                buf->samplerate     = 16000;
                buf->sampleformat   = AV_SAMPLE_FMT_FLT;
                asr_fifo->put(asr_fifo, buf);
            }
        }
    } else */{
        //normal logic
        asr_fifo = whisper_asr_getfifo();
        if (NULL != asr_fifo) {
            buf_element_t *buf = asr_fifo->buffer_try_alloc(asr_fifo);
            if (NULL != buf) {
                memcpy(buf->mem, p_samples, samples_size);
                buf->size           = samples_size;
                buf->samplecounts   = audioframe->nb_samples;
                buf->channels       = audioframe->channels;
                buf->samplerate     = audioframe->sample_rate;
                buf->sampleformat   = static_cast<AVSampleFormat>(sample_format);
                asr_fifo->put(asr_fifo, buf);
            }
        }
    }

    //pthread_mutex_unlock(&p_asr_ctx->mutex); // remove the mutex since 03-19-2024, crash would happen before 03-19 without mutex

    return NULL;
}



/**
 *
 * @param pf32_audio_buffer           audio data with mono channel + float 32 + 160000 sample rate to make whisper.cpp happy
 * @param num_samples                 sample counts in audio data
 * @return asr result                 expected asr result or NULL
 */
static const char * whisper_asr_audio_to_text(const float * pf32_audio_buffer, int num_samples) {
    int result                          = 0;
    int index                           = 0;
    const char *text                    = NULL;
    int64_t begin_time                  = 0LL;
    int64_t end_time                    = 0LL;
    int64_t t0_segment                  = 0;
    int64_t t1_segment                  = 0;
    int num_segments                    = 0;

    static std::string asr_result;


    if (NULL == p_asr_ctx) {
        LOGGW("pls check whether asr_ctx already initialized?\n");
        return NULL;
    }

    if ((NULL == pf32_audio_buffer) || (num_samples < 1)) {
        //LOGGW("pls check params\n");
        return NULL;
    }

    if (1 == p_asr_ctx->n_asr_mode) { //ASR pressure test, already handled in whisper_asr_callback
        //LOGGW("are you in ASR pressure test mode?\n");
        return NULL;
    }

    if (2 == p_asr_ctx->n_asr_mode) { //ASR benchmark
        //2024-03-15,16:24
        //there is a special case:launch asr performance test in ASRResearchFragment.java
        //and calling whisper_asr_audio_to_text() directly in whisper_transcribe_from_file
    }

    asr_result = whisper_get_time_string() + " AI subtitle powered by whisper.cpp\n";

    begin_time = ggml_time_ms();
    whisper_reset_timings(p_asr_ctx->p_context);

    //03-20-2024, ref:https://github.com/futo-org/whisper-acft
    p_asr_ctx->p_params->max_tokens        = 256;
    p_asr_ctx->p_params->temperature_inc   = 0.0f;
    //03-22-2024, don't use this new fine-tune method because it will brings side-effect:app crash randomly
    //p_asr_ctx->p_params->audio_ctx         = std::min(1500, (int)ceil((double)num_samples / (double)(320.0)) + 16);


    //replaced with default value, ref: https://github.com/ggerganov/whisper.cpp/blob/master/whisper.h#L499
    p_asr_ctx->p_params->audio_ctx         = 0;

    //03-24-2024, works ok/stable/good performance/... on Xiaomi 14
    p_asr_ctx->p_params->audio_ctx = std::min(1500, (int)ceil((double)num_samples / (double)(32.0)) + 16);

    //p_asr_ctx->p_params->initial_prompt    = "\" English online TV \"";
    /*
    p_asr_ctx->p_params->abort_callback_user_data = p_asr_ctx;
    p_asr_ctx->p_params->abort_callback = [](void * user_data) -> bool {
        auto *asr_ctx = reinterpret_cast<whisper_asr_context*>(user_data);
        return true;
    };
    */

    p_asr_ctx->n_decoding_mode  = WHISPER_SAMPLING_GREEDY;
    if (WHISPER_SAMPLING_GREEDY == p_asr_ctx->n_decoding_mode) {
        p_asr_ctx->p_params->strategy = WHISPER_SAMPLING_GREEDY;
        p_asr_ctx->p_params->greedy.best_of         = 1;    //ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264
    } else {
        p_asr_ctx->p_params->strategy               = WHISPER_SAMPLING_BEAM_SEARCH;
        p_asr_ctx->p_params->beam_search.beam_size  = 5;    //ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265
        p_asr_ctx->p_params->greedy.best_of         = 5;
    }
    LOGGD("decoding_mode=%d, audio_ctx=%d\n", p_asr_ctx->n_decoding_mode, p_asr_ctx->p_params->audio_ctx);

    result = whisper_full(p_asr_ctx->p_context, *p_asr_ctx->p_params, pf32_audio_buffer, num_samples);
    if (0 != result) {
        LOGW("whisper inference failure, pls check why?\n");
        result = -1;
        goto failure;
    }
    end_time = ggml_time_ms();

    LOGGW("whisper inference cost %d ms\n", end_time - begin_time);
    //whisper_print_timings(p_asr_ctx->p_context); // DO NOT uncomment this line

    num_segments = whisper_full_n_segments(p_asr_ctx->p_context);
    for (index = 0; index < num_segments; index++) {
        text = whisper_full_get_segment_text(p_asr_ctx->p_context, index);
        if (NULL == text) {
            LOGW("whisper_full_get_segment_text failure, pls check why\n");
            result = -2;
            goto failure;
        }
        t0_segment = whisper_full_get_segment_t0(p_asr_ctx->p_context, index);
        t1_segment = whisper_full_get_segment_t1(p_asr_ctx->p_context, index);
        /*
        asr_result +=
                "[ " + to_timestamp(t0_segment) + " ---> " + to_timestamp(t1_segment) + "  ]" +
                std::string(text) + "\n";
                */
        asr_result += std::string(text);
        result = 0;

        LOGGD("asr result:\n%s\n", asr_result.c_str());
        // 03-26-2024,
        // this function was referenced by this PR:https://github.com/ggerganov/llama.cpp/pull/5935/
        // double check although this special case has been handled at the JNI layer
        if (is_valid_utf8(asr_result.c_str())) {
            kantv_asr_notify_c(asr_result.c_str());
        }
    }

    text = asr_result.c_str();

failure:
    if (0 != result) {
        asr_result = whisper_get_time_string() + "\n" +  "whisper inference failure, pls check why?\n";
        text = asr_result.c_str();
    }

    return text;
}




// =================================================================================================
//
// JNI helper function for asr
// the following code is just for PoC of realtime subtitle with online TV and SHOULD NOT be used in real project
//
// =================================================================================================


 /**
  *
  * @param model_path
  * @param num_threads
  * @param n_asrmode            0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
  */
int whisper_asr_init(const char * sz_model_path, int n_threads, int n_asrmode) {
     LOGGV("enter whisper_asr_init\n");
     int result         = 0;

     struct whisper_full_params params;

     if ((NULL == sz_model_path) || (n_asrmode > 3)) {
         LOGGW("invalid param\n");
         return 1;
     }


     // dynamic ISA dectect by RUAPU, prepare for SIMD optimization on Android device. but not used currently
     ruapu_init();
     const char* const* supported = ruapu_rua();
     while (*supported) {
         LOGGD("%s\n", *supported);
         supported++;
     }


     LOGGV("model path:%s\n", sz_model_path);
     LOGGV("thread counts:%d\n", n_threads);
     LOGGV("asr mode:%d\n", n_asrmode);

     kantv_asr_callback pfn_asr_callback = whisper_asr_callback;
     kantv_asr_set_callback(pfn_asr_callback);

     kantv_inference_callback  pfn_inference_callback = whisper_asr_audio_to_text;
     kantv_inference_set_callback(pfn_inference_callback);

     if (NULL != p_asr_ctx) {
         LOGGW("asr instance already initialized\n");
         return 2;
     }

     if (NULL == p_asr_ctx) {
         p_asr_ctx = (whisper_asr_context *) malloc(sizeof(whisper_asr_context));
         if (NULL == p_asr_ctx) {
             LOGGW("initialize failure\n");
             return 3;
         }
     }
     memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
     memcpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));

     result  = pthread_mutex_init(&p_asr_ctx->mutex, NULL);
     if (result != 0) {
         result = 4;
         LOGGW("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->asr_fifo = fifo_new("asr_fifo", 1200, 1024 * 8 * 20);
     if (NULL == p_asr_ctx->asr_fifo) {
         result = 5;
         LOGGW("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->p_asr = new (std::nothrow)whisper_asr();  // attention memory leak
     if (NULL == p_asr_ctx->p_asr) {
         result = 6;
         LOGGW("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->swr_ctx = swr_alloc();      // attention memory leak
     if (NULL == p_asr_ctx->swr_ctx) {
         result  = 7;
         LOGGW("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->n_asr_mode = n_asrmode;
     p_asr_ctx->n_threads  = n_threads;

     LOGGD("calling whisper_init_from_file");
     p_asr_ctx->p_context = whisper_init_from_file(sz_model_path);
     if (nullptr == p_asr_ctx->p_context) {
         result = 8;
         LOGGW("initialize failure\n");
         goto failure;
     }
     LOGGD("after calling whisper_init_from_file");

     p_asr_ctx->p_params = (struct whisper_full_params *)malloc(sizeof(struct whisper_full_params));
     if (NULL == p_asr_ctx->p_params) {
         result = 9;
         LOGGW("initialize failure\n");
         goto failure;
     }

     params = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
     params.print_realtime          = false;
     params.print_progress          = false;
     params.print_timestamps        = false;
     params.print_special           = false;
     params.translate               = false; //first step is transcription, the second step is English -> Chinese
     //params.initial_prompt        = "hello,whisper.cpp";
     //params.language                = "en";
     params.n_threads               = n_threads;;
     params.offset_ms               = 0;
     params.no_context              = true;
     params.single_segment          = true;
     params.no_timestamps           = true;

     params.speed_up                = false;
     params.debug_mode              = false;
     params.audio_ctx               = 0;

     params.suppress_blank              = true;
     params.suppress_non_speech_tokens  = true;

     //03-20-2024, ref:https://github.com/futo-org/whisper-acft
     p_asr_ctx->n_decoding_mode         = WHISPER_SAMPLING_BEAM_SEARCH;


     //params.tdrz_enable                  = false;//whisper complain failed to compute log mel spectrogram when this flag was enabled
     //params.suppress_blank               = true;
     //params.suppress_non_speech_tokens   = true;

     memcpy(p_asr_ctx->p_params, &params, sizeof(struct whisper_full_params));

     p_asr_ctx->b_pre_convert = p_asr_ctx->b_enable_dump_16k_data = false;

     LOGGV("leave whisper_asr_init\n");

     return result;

failure:

     if (nullptr != p_asr_ctx->p_context) {
         whisper_free(p_asr_ctx->p_context);
     }

     if (NULL != p_asr_ctx->swr_ctx) {
         swr_free(&p_asr_ctx->swr_ctx);
     }

     if (NULL != p_asr_ctx->p_asr)
        delete p_asr_ctx->p_asr;

     if (NULL != p_asr_ctx->asr_fifo) {
         p_asr_ctx->asr_fifo->destroy(p_asr_ctx->asr_fifo);
     }

     if (4 != result)
         pthread_mutex_destroy(&p_asr_ctx->mutex);

     if (NULL != p_asr_ctx) {
         free(p_asr_ctx);
     }
     p_asr_ctx = NULL;

     return result;
 }


void whisper_asr_finalize() {
    LOGGV("enter whisper_asr_finalize\n");

    if (NULL == p_asr_ctx) {
        LOGGW("whisper.cpp not initialized or already finalized\n");
        return;
    }
    whisper_free(p_asr_ctx->p_context);

    swr_free(&p_asr_ctx->swr_ctx);

    delete p_asr_ctx->p_asr;

    p_asr_ctx->asr_fifo->destroy(p_asr_ctx->asr_fifo);

    free(p_asr_ctx->p_params);

    pthread_mutex_destroy(&p_asr_ctx->mutex);

    free(p_asr_ctx);
    p_asr_ctx = NULL;

    LOGGV("leave whisper_asr_finalize\n");
}


void whisper_asr_start() {
    LOGGD("start asr thread\n");

    if ((NULL == p_asr_ctx) || (NULL == p_asr_ctx->p_asr)) {
        LOGGW("pls check why asr subsystem not initialized\n");
        return;
    }

    if ((0 == p_asr_ctx->n_asr_mode) || (3 == p_asr_ctx->n_asr_mode)) { // normal transcription  || normal transcription + audio recording
        p_asr_ctx->p_asr->start();
    }
    kantv_inference_set_callback(whisper_asr_audio_to_text);
    LOGGD("after start asr thread\n");
}


void whisper_asr_stop() {
    LOGGD("stop asr thread\n");

    if ((NULL == p_asr_ctx) || (NULL == p_asr_ctx->p_asr)) {
        LOGGW("pls check why asr subsystem not initialized\n");
        return;
    }

    if ((0 == p_asr_ctx->n_asr_mode) || (3 == p_asr_ctx->n_asr_mode)) { // normal transcription || normal transcription + audio recording
        p_asr_ctx->p_asr->stop();
    }
    kantv_inference_set_callback(NULL);
    LOGGD("after stop asr thread\n");
}


/**
 *
 * @param sz_model_path
 * @param n_threads
 * @param n_asrmode            0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
 */
int whisper_asr_reset(const char * sz_model_path, int n_threads, int n_asrmode) {
    int result = 0;

    LOGGD("enter asr reset\n");
    if (NULL == p_asr_ctx) {
        LOGGW("asr instance already initialized\n");
        return 1;
    }

    if ((NULL == sz_model_path) || (n_threads <= 0)) {
        LOGGW("invalid param\n");
        return 2;
    }

    if (nullptr == p_asr_ctx->p_context) {
        LOGGW("it should not happen, pls check why?\n");
    }

    if (0 != memcmp(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path))) {
        LOGGD("re-init whispercpp instance\n");
        if (nullptr != p_asr_ctx->p_context) {
            whisper_free(p_asr_ctx->p_context);
        }
        p_asr_ctx->p_context = whisper_init_from_file(sz_model_path);
        memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
        memcpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));
    } else {
        LOGGD("using cached whispercpp instance\n");
    }

    if (nullptr == p_asr_ctx->p_context) {
        result = 3;
    } else {
        p_asr_ctx->n_threads            = n_threads;
        p_asr_ctx->p_params->n_threads  = n_threads;
        p_asr_ctx->n_asr_mode           = n_asrmode;
    }

    LOGGD("leave asr reset\n");
    return result;
}


// =================================================================================================
//
// JNI helper function for llama.cpp

// all the following codes comes from examples/main.cpp in project llama.cpp
//
// trying to integrate llama.cpp from 03-26-2024
// =================================================================================================
#include "sampling.h"

#define log_tostr(var) log_var_to_string_impl(var).c_str()

static inline std::string log_var_to_string_impl(bool var)
{
    return var ? "true" : "false";
}

static inline std::string log_var_to_string_impl(std::string var)
{
    return var;
}

static inline std::string log_var_to_string_impl(const std::vector<int> & var)
{
    std::stringstream buf;
    buf << "[ ";
    bool first = true;
    for (auto e : var)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            buf << ", ";
        }
        buf << std::to_string(e);
    }
    buf << " ]";

    return buf.str();
}

template <typename C, typename T>
static inline std::string LOG_TOKENS_TOSTR_PRETTY(const C & ctx, const T & tokens)
{
    std::stringstream buf;
    buf << "[ ";

    bool first = true;
    for (const auto &token : tokens)
    {
        if (!first) {
            buf << ", ";
        } else {
            first = false;
        }

        auto detokenized = llama_token_to_piece(ctx, token);

        detokenized.erase(
                std::remove_if(
                        detokenized.begin(),
                        detokenized.end(),
                        [](const unsigned char c) { return !std::isprint(c); }),
                detokenized.end());

        buf
                << "'" << detokenized << "'"
                << ":" << std::to_string(token);
    }
    buf << " ]";

    return buf.str();
}


/**
 *
 * @param sz_model_path         /sdcard/kantv/llama-2-7b-chat.Q4_K_M.gguf
 * @param prompt
 * @param bench_type            not used currently
 * @param n_threads             1 - 8
 * @return
*/
int  llama_inference(const char * model_path, const char * prompt, int bench_type, int num_threads) {
    llama_context           ** g_ctx;
    llama_model             ** g_model;
    gpt_params               * g_params;
    std::vector<llama_token> * g_input_tokens;
    std::ostringstream       * g_output_ss;
    std::vector<llama_token> * g_output_tokens;
    bool is_interacting = false;

    gpt_params params;


    if (NULL == model_path)
        return -1;

    if (NULL == prompt)
        return -1;

    LOGGV("prompt:%s\n", prompt);
    LOGGV("model file %s\n", model_path);
    params.model = model_path;
    params.prompt = std::string(prompt);
    llama_sampling_params & sparams = params.sparams;

    if (params.n_ctx != 0 && params.n_ctx < 8) {
        LOGGV("%s: warning: minimum context size is 8, using minimum size.\n", __func__);
        params.n_ctx = 8;
    }

    if (params.rope_freq_base != 0.0) {
        LOGGV("%s: warning: changing RoPE frequency base to %g.\n", __func__, params.rope_freq_base);
    }

    if (params.rope_freq_scale != 0.0) {
        LOGGV("%s: warning: scaling RoPE frequency by %g.\n", __func__, params.rope_freq_scale);
    }

    if (params.seed == LLAMA_DEFAULT_SEED) {
        params.seed = time(NULL);
    }

    LOGGV("%s: seed  = %u\n", __func__, params.seed);

    std::mt19937 rng(params.seed);

    LOGGD("%s: llama backend init\n", __func__);
    llama_backend_init();
    llama_numa_init(params.numa);

    llama_model * model;
    llama_context * ctx;
    llama_context * ctx_guidance = NULL;
    g_model = &model;
    g_ctx = &ctx;

    // load the model and apply lora adapter, if any
    LOGGD("%s: load the model and apply lora adapter, if any\n", __func__);
    std::tie(model, ctx) = llama_init_from_gpt_params(params);
    if (sparams.cfg_scale > 1.f) {
        struct llama_context_params lparams = llama_context_params_from_gpt_params(params);
        ctx_guidance = llama_new_context_with_model(model, lparams);
    }

    if (model == NULL) {
        LOGGV("%s: error: unable to load model\n", __func__);
        return 1;
    }

    const int n_ctx_train = llama_n_ctx_train(model);
    const int n_ctx = llama_n_ctx(ctx);
    LOGGV("n_ctx: %d\n", n_ctx);

    if (n_ctx > n_ctx_train) {
        LOGGV("%s: warning: model was trained on only %d context tokens (%d specified)\n",
                __func__, n_ctx_train, n_ctx);
    }

    std::string path_session = params.path_prompt_cache;
    std::vector<llama_token> session_tokens;

    const bool add_bos = llama_should_add_bos_token(model);
    LOGGV("add_bos: %d\n", add_bos);

    std::vector<llama_token> embd_inp;

    embd_inp = ::llama_tokenize(ctx, params.prompt, add_bos, true);

    LOGGV("prompt: \"%s\"\n", log_tostr(params.prompt));
    LOGGV("tokens: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_inp).c_str());

    // Should not run without any tokens
    if (embd_inp.empty()) {
        embd_inp.push_back(llama_token_bos(model));
        LOGGV("embd_inp was considered empty and bos was added: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_inp).c_str());
    }

    // Tokenize negative prompt
    std::vector<llama_token> guidance_inp;
    int guidance_offset = 0;
    int original_prompt_len = 0;
    if (ctx_guidance) {
        LOGGV("cfg_negative_prompt: \"%s\"\n", log_tostr(sparams.cfg_negative_prompt));

        guidance_inp = ::llama_tokenize(ctx_guidance, sparams.cfg_negative_prompt, add_bos, true);
        LOGGV("guidance_inp tokenized: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx_guidance, guidance_inp).c_str());

        std::vector<llama_token> original_inp = ::llama_tokenize(ctx, params.prompt, add_bos, true);
        LOGGV("original_inp tokenized: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, original_inp).c_str());

        original_prompt_len = original_inp.size();
        guidance_offset = (int)guidance_inp.size() - original_prompt_len;
        LOGGV("original_prompt_len: %s", log_tostr(original_prompt_len));
        LOGGV("guidance_offset:     %s", log_tostr(guidance_offset));
    }

    if ((int) embd_inp.size() > n_ctx - 4) {
        LOGGV("%s: error: prompt is too long (%d tokens, max %d)\n", __func__, (int) embd_inp.size(), n_ctx - 4);
        return 1;
    }

    // debug message about similarity of saved session, if applicable
    size_t n_matching_session_tokens = 0;
    if (!session_tokens.empty()) {
        for (llama_token id : session_tokens) {
            if (n_matching_session_tokens >= embd_inp.size() || id != embd_inp[n_matching_session_tokens]) {
                break;
            }
            n_matching_session_tokens++;
        }
        if (params.prompt.empty() && n_matching_session_tokens == embd_inp.size()) {
            LOGGV("%s: using full prompt from session file\n", __func__);
        } else if (n_matching_session_tokens >= embd_inp.size()) {
            LOGGV("%s: session file has exact match for prompt!\n", __func__);
        } else if (n_matching_session_tokens < (embd_inp.size() / 2)) {
            LOGGV("%s: warning: session file has low similarity to prompt (%zu / %zu tokens); will mostly be reevaluated\n",
                    __func__, n_matching_session_tokens, embd_inp.size());
        } else {
            LOGGV("%s: session file matches %zu / %zu tokens of prompt\n",
                    __func__, n_matching_session_tokens, embd_inp.size());
        }

        // remove any "future" tokens that we might have inherited from the previous session
        llama_kv_cache_seq_rm(ctx, -1, n_matching_session_tokens, -1);
    }

    LOGGD(
            "recalculate the cached logits (check): embd_inp.empty() %s, n_matching_session_tokens %zu, embd_inp.size() %zu, session_tokens.size() %zu, embd_inp.size() %zu",
            log_tostr(embd_inp.empty()), n_matching_session_tokens, embd_inp.size(), session_tokens.size(), embd_inp.size());

    // if we will use the cache for the full prompt without reaching the end of the cache, force
    // reevaluation of the last token token to recalculate the cached logits
    if (!embd_inp.empty() && n_matching_session_tokens == embd_inp.size() && session_tokens.size() > embd_inp.size()) {
        LOGGD("recalculate the cached logits (do): session_tokens.resize( %zu )", embd_inp.size() - 1);

        session_tokens.resize(embd_inp.size() - 1);
    }

    // number of tokens to keep when resetting context
    if (params.n_keep < 0 || params.n_keep > (int) embd_inp.size() || params.instruct || params.chatml) {
        params.n_keep = (int)embd_inp.size();
    } else {
        params.n_keep += add_bos; // always keep the BOS token
    }

    // prefix & suffix for instruct mode
    const auto inp_pfx = ::llama_tokenize(ctx, "\n\n### Instruction:\n\n", add_bos, true);
    const auto inp_sfx = ::llama_tokenize(ctx, "\n\n### Response:\n\n",    false,   true);

    LOGGV("inp_pfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, inp_pfx).c_str());
    LOGGV("inp_sfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, inp_sfx).c_str());

    // chatml prefix & suffix
    const auto cml_pfx = ::llama_tokenize(ctx, "\n<|im_start|>user\n", add_bos, true);
    const auto cml_sfx = ::llama_tokenize(ctx, "<|im_end|>\n<|im_start|>assistant\n", false, true);

    LOGGV("cml_pfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, cml_pfx).c_str());
    LOGGV("cml_sfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, cml_sfx).c_str());

    // in instruct mode, we inject a prefix and a suffix to each input by the user
    if (params.instruct) {
        params.interactive_first = true;
        params.antiprompt.emplace_back("### Instruction:\n\n");
    }
        // similar for chatml mode
    else if (params.chatml) {
        params.interactive_first = true;
        params.antiprompt.emplace_back("<|im_start|>user\n");
    }

    // enable interactive mode if interactive start is specified
    if (params.interactive_first) {
        params.interactive = true;
    }

    LOGGV("sampling: \n%s\n", llama_sampling_print(sparams).c_str());
    LOGGV("sampling order: \n%s\n", llama_sampling_order_print(sparams).c_str());
    LOGGV("generate: n_ctx = %d, n_batch = %d, n_predict = %d, n_keep = %d\n", n_ctx, params.n_batch, params.n_predict, params.n_keep);

    // group-attention state
    // number of grouped KV tokens so far (used only if params.grp_attn_n > 1)
    int ga_i = 0;

    const int ga_n = params.grp_attn_n;
    const int ga_w = params.grp_attn_w;

    if (ga_n != 1) {
        GGML_ASSERT(ga_n > 0                    && "grp_attn_n must be positive");                     // NOLINT
        GGML_ASSERT(ga_w % ga_n == 0            && "grp_attn_w must be a multiple of grp_attn_n");     // NOLINT
        //GGML_ASSERT(n_ctx_train % ga_w == 0     && "n_ctx_train must be a multiple of grp_attn_w");    // NOLINT
        //GGML_ASSERT(n_ctx >= n_ctx_train * ga_n && "n_ctx must be at least n_ctx_train * grp_attn_n"); // NOLINT
        LOGGV("self-extend: n_ctx_train = %d, grp_attn_n = %d, grp_attn_w = %d\n", n_ctx_train, ga_n, ga_w);
    }
    LOGGV("\n\n");

    bool is_antiprompt        = false;
    bool input_echo           = true;
    bool display              = true;
    bool need_to_save_session = !path_session.empty() && n_matching_session_tokens < embd_inp.size();

    int n_past             = 0;
    int n_remain           = params.n_predict;
    int n_consumed         = 0;
    int n_session_consumed = 0;
    int n_past_guidance    = 0;

    std::vector<int>   input_tokens;  g_input_tokens  = &input_tokens;
    std::vector<int>   output_tokens; g_output_tokens = &output_tokens;
    std::ostringstream output_ss;     g_output_ss     = &output_ss;

    // the first thing we will do is to output the prompt, so set color accordingly
    //console::set_display(console::prompt);
    //display = params.display_prompt;

    std::vector<llama_token> embd;
    std::vector<llama_token> embd_guidance;

    // tokenized antiprompts
    std::vector<std::vector<llama_token>> antiprompt_ids;

    antiprompt_ids.reserve(params.antiprompt.size());
    for (const std::string & antiprompt : params.antiprompt) {
        antiprompt_ids.emplace_back(::llama_tokenize(ctx, antiprompt, false, true));
    }

    struct llama_sampling_context * ctx_sampling = llama_sampling_init(sparams);

    while ((n_remain != 0 && !is_antiprompt) || params.interactive) {
        // predict
        if (!embd.empty()) {
            // Note: (n_ctx - 4) here is to match the logic for commandline prompt handling via
            // --prompt or --file which uses the same value.
            int max_embd_size = n_ctx - 4;

            // Ensure the input doesn't exceed the context size by truncating embd if necessary.
            if ((int) embd.size() > max_embd_size) {
                const int skipped_tokens = (int) embd.size() - max_embd_size;
                embd.resize(max_embd_size);


                LOGGD("<<input too long: skipped %d token%s>>", skipped_tokens, skipped_tokens != 1 ? "s" : "");
            }

            if (ga_n == 1) {
                // infinite text generation via context shifting
                // if we run out of context:
                // - take the n_keep first tokens from the original prompt (via n_past)
                // - take half of the last (n_ctx - n_keep) tokens and recompute the logits in batches
                if (n_past + (int) embd.size() + std::max<int>(0, guidance_offset) > n_ctx) {
                    if (params.n_predict == -2) {
                        LOGGV("\n\n%s: context full and n_predict == -%d => stopping\n", __func__, params.n_predict);
                        break;
                    }

                    const int n_left    = n_past - params.n_keep;
                    const int n_discard = n_left/2;

                    LOGGV("context full, swapping: n_past = %d, n_left = %d, n_ctx = %d, n_keep = %d, n_discard = %d\n",
                        n_past, n_left, n_ctx, params.n_keep, n_discard);

                    llama_kv_cache_seq_rm (ctx, 0, params.n_keep            , params.n_keep + n_discard);
                    llama_kv_cache_seq_add(ctx, 0, params.n_keep + n_discard, n_past, -n_discard);

                    n_past -= n_discard;

                    if (ctx_guidance) {
                        n_past_guidance -= n_discard;
                    }

                    LOGGV("after swap: n_past = %d, n_past_guidance = %d\n", n_past, n_past_guidance);

                    LOGGV("embd: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd).c_str());

                    LOGGV("clear session path\n");
                    path_session.clear();
                }
            } else {
                // context extension via Self-Extend
                while (n_past >= ga_i + ga_w) {
                    const int ib = (ga_n*ga_i)/ga_w;
                    const int bd = (ga_w/ga_n)*(ga_n - 1);
                    const int dd = (ga_w/ga_n) - ib*bd - ga_w;

                    LOGGV("\n");
                    LOGGV("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i, n_past, ib*bd, ga_i + ib*bd, n_past + ib*bd);
                    LOGGV("div:   [%6d, %6d] / %6d -> [%6d, %6d]\n", ga_i + ib*bd, ga_i + ib*bd + ga_w, ga_n, (ga_i + ib*bd)/ga_n, (ga_i + ib*bd + ga_w)/ga_n);
                    LOGGV("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i + ib*bd + ga_w, n_past + ib*bd, dd, ga_i + ib*bd + ga_w + dd, n_past + ib*bd + dd);

                    llama_kv_cache_seq_add(ctx, 0, ga_i,                n_past,              ib*bd);
                    llama_kv_cache_seq_div(ctx, 0, ga_i + ib*bd,        ga_i + ib*bd + ga_w, ga_n);
                    llama_kv_cache_seq_add(ctx, 0, ga_i + ib*bd + ga_w, n_past + ib*bd,      dd);

                    n_past -= bd;

                    ga_i += ga_w/ga_n;

                    LOGGV("\nn_past_old = %d, n_past = %d, ga_i = %d\n\n", n_past + bd, n_past, ga_i);
                }
            }

            // try to reuse a matching prefix from the loaded session instead of re-eval (via n_past)
            if (n_session_consumed < (int) session_tokens.size()) {
                size_t i = 0;
                for ( ; i < embd.size(); i++) {
                    if (embd[i] != session_tokens[n_session_consumed]) {
                        session_tokens.resize(n_session_consumed);
                        break;
                    }

                    n_past++;
                    n_session_consumed++;

                    if (n_session_consumed >= (int) session_tokens.size()) {
                        ++i;
                        break;
                    }
                }
                if (i > 0) {
                    embd.erase(embd.begin(), embd.begin() + i);
                }
            }

            // evaluate tokens in batches
            // embd is typically prepared beforehand to fit within a batch, but not always
            if (ctx_guidance) {
                int input_size = 0;
                llama_token * input_buf = NULL;

                if (n_past_guidance < (int) guidance_inp.size()) {
                    // Guidance context should have the same data with these modifications:
                    //
                    // * Replace the initial prompt
                    // * Shift everything by guidance_offset
                    embd_guidance = guidance_inp;
                    if (embd.begin() + original_prompt_len < embd.end()) {
                        embd_guidance.insert(
                                embd_guidance.end(),
                                embd.begin() + original_prompt_len,
                                embd.end()
                        );
                    }

                    input_buf  = embd_guidance.data();
                    input_size = embd_guidance.size();

                    LOGGV("guidance context: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_guidance).c_str());
                } else {
                    input_buf  = embd.data();
                    input_size = embd.size();
                }

                for (int i = 0; i < input_size; i += params.n_batch) {
                    int n_eval = std::min(input_size - i, params.n_batch);
                    if (llama_decode(ctx_guidance, llama_batch_get_one(input_buf + i, n_eval, n_past_guidance, 0))) {
                        LOGGV("%s : failed to eval\n", __func__);
                        return 1;
                    }

                    n_past_guidance += n_eval;
                }
            }

            for (int i = 0; i < (int) embd.size(); i += params.n_batch) {
                int n_eval = (int) embd.size() - i;
                if (n_eval > params.n_batch) {
                    n_eval = params.n_batch;
                }

                LOGGV("eval: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd).c_str());

                if (llama_decode(ctx, llama_batch_get_one(&embd[i], n_eval, n_past, 0))) {
                    LOGGV("%s : failed to eval\n", __func__);
                    return 1;
                }

                n_past += n_eval;

                LOGGV("n_past = %d\n", n_past);
                // Display total tokens alongside total time
                if (params.n_print > 0 && n_past % params.n_print == 0) {
                    LOGGV("\n\033[31mTokens consumed so far = %d / %d \033[0m\n", n_past, n_ctx);
                }
            }

            if (!embd.empty() && !path_session.empty()) {
                session_tokens.insert(session_tokens.end(), embd.begin(), embd.end());
                n_session_consumed = session_tokens.size();
            }
        }

        embd.clear();
        embd_guidance.clear();

        if ((int) embd_inp.size() <= n_consumed && !is_interacting) {
            const llama_token id = llama_sampling_sample(ctx_sampling, ctx, ctx_guidance);

            llama_sampling_accept(ctx_sampling, ctx, id, true);

            LOGGV("last: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, ctx_sampling->prev).c_str());

            embd.push_back(id);

            // echo this to console
            input_echo = true;

            // decrement remaining sampling budget
            --n_remain;

            LOGGV("n_remain: %d\n", n_remain);
        } else {
            // some user input remains from prompt or interaction, forward it to processing
            LOGGV("embd_inp.size(): %d, n_consumed: %d\n", (int) embd_inp.size(), n_consumed);
            while ((int) embd_inp.size() > n_consumed) {
                embd.push_back(embd_inp[n_consumed]);

                // push the prompt in the sampling context in order to apply repetition penalties later
                // for the prompt, we don't apply grammar rules
                llama_sampling_accept(ctx_sampling, ctx, embd_inp[n_consumed], false);

                ++n_consumed;
                if ((int) embd.size() >= params.n_batch) {
                    break;
                }
            }
        }

        // display text
        if (input_echo && display) {
            for (auto id : embd) {
                const std::string token_str = llama_token_to_piece(ctx, id);
                printf("%s", token_str.c_str());
#ifdef TARGET_ANDROID
                if (is_valid_utf8(token_str.c_str())) { //ref:https://github.com/ggerganov/llama.cpp/pull/5935/
                    kantv_asr_notify_benchmark_c(token_str.c_str());
                }
#endif

                if (embd.size() > 1) {
                    input_tokens.push_back(id);
                } else {
                    output_tokens.push_back(id);
                    output_ss << token_str;
                }
            }
            fflush(stdout);
        }

        // if not currently processing queued inputs;
        if ((int) embd_inp.size() <= n_consumed) {
            // check for reverse prompt in the last n_prev tokens
            if (!params.antiprompt.empty()) {
                const int n_prev = 32;
                const std::string last_output = llama_sampling_prev_str(ctx_sampling, ctx, n_prev);

                is_antiprompt = false;
                // Check if each of the reverse prompts appears at the end of the output.
                // If we're not running interactively, the reverse prompt might be tokenized with some following characters
                // so we'll compensate for that by widening the search window a bit.
                for (std::string & antiprompt : params.antiprompt) {
                    size_t extra_padding = params.interactive ? 0 : 2;
                    size_t search_start_pos = last_output.length() > static_cast<size_t>(antiprompt.length() + extra_padding)
                                              ? last_output.length() - static_cast<size_t>(antiprompt.length() + extra_padding)
                                              : 0;

                    if (last_output.find(antiprompt, search_start_pos) != std::string::npos) {
                        if (params.interactive) {
                            is_interacting = true;
                        }
                        is_antiprompt = true;
                        break;
                    }
                }

                // check for reverse prompt using special tokens
                llama_token last_token = llama_sampling_last(ctx_sampling);
                for (std::vector<llama_token> ids : antiprompt_ids) {
                    if (ids.size() == 1 && last_token == ids[0]) {
                        if (params.interactive) {
                            is_interacting = true;
                        }
                        is_antiprompt = true;
                        break;
                    }
                }

            }
        }

        if (!embd.empty() && embd.back() == llama_token_eos(model)) {
            LOGGV(" [end of text]\n");
#ifdef TARGET_ANDROID
            kantv_asr_notify_benchmark_c("\n[end of text]\n\n");
#endif
            break;
        }
    }

    llama_print_timings(ctx);
    if (ctx_guidance) {
        llama_free(ctx_guidance);
    }

    llama_free(ctx);
    llama_free_model(model);

    llama_sampling_free(ctx_sampling);
    llama_backend_free();

    return 0;
}


static void ggml_graph_compute_helper(std::vector<uint8_t> & buf, ggml_cgraph * graph, int n_threads) {
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);

    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.data();
    }

    ggml_graph_compute(graph, &plan);
}


static float tensor_sum_elements(const ggml_tensor * tensor) {
    double sum = 0;
    if (tensor->type == GGML_TYPE_F32) {
        for (int j = 0; j < tensor->ne[1]; j++) {
            for (int k = 0; k < tensor->ne[0]; k++) {
                sum += ((float *) tensor->data)[j*tensor->ne[0] + k];
            }
        }
    }
    return sum;
}


static void tensor_dump(const ggml_tensor * tensor, const char * name) {
    printf("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi) - ", name,
            tensor->type, ggml_type_name(tensor->type),
            tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    float sum = tensor_sum_elements(tensor);
    printf("Sum of tensor %s is %6.2f\n", name, sum);
}


#define TENSOR_DUMP(tensor) tensor_dump(tensor, #tensor)

struct benchmark_params_struct {
    int32_t n_threads     = 1;
    int32_t n_iterations  = 10;
};

void ggml_bench_matrix(int num_threads) {
    struct benchmark_params_struct benchmark_params;

    bool invalid_param = false;
    std::string arg;

    LOGGD("enter ggml_bench_matrix\n");

    GGML_JNI_NOTIFY("Starting GGML matrix benchmark\n");

    // create the ggml context
    struct ggml_context * ctx;
    //const int sizex = 4096;
    //const int sizey = 11008;

#undef VERBOSE_DEBUGGING
#ifndef VERBOSE_DEBUGGING
    const int sizey = 4096;
    const int sizex = 11008;
    const int sizez = 128;
#else
    /* Working - let's increase size */
    const int sizey = 1;
    const int sizex = (8*32);
    const int sizez = 1;

    /*const int sizey = 1;
    const int sizex = 3*(8*32);
    const int sizez = 1;*/
#endif

    GGML_JNI_NOTIFY("Memsize required = %i\n", sizex*sizex);

    // TODO: perform the bench for all types or for a user specified type
    const ggml_type qtype = GGML_TYPE_Q4_1;

    size_t ctx_size = 0;
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizez);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey); // BLAS
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey); // BLAS
    ctx_size += 1024*1024*16;

    GGML_JNI_NOTIFY("Allocating Memory of size %zi bytes, %zi MB\n",ctx_size, (ctx_size/1024/1024));

    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

    ctx = ggml_init(params);
    if (!ctx) {
        LOGGW("%s: ggml_init() failed\n");
        return;
    }

    GGML_JNI_NOTIFY("Creating new tensors\n");
    // GGML_JNI_NOTIFY("Creating new tensor m1\n");
    struct ggml_tensor * m11 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m11, 1.0f);

    // GGML_JNI_NOTIFY("Creating new tensor m1\n");
    struct ggml_tensor * m12 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m12, 1.5f);

    // GGML_JNI_NOTIFY("Creating new tensor m2\n");
    struct ggml_tensor * m2 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizez);
    ggml_set_f32(m2, 2.0f);

    GGML_JNI_NOTIFY("\n------ Test 1 - Matrix Mult via F32 code\n");
    // GGML_JNI_NOTIFY("Creating new tensor m11xm2\n");
    struct ggml_tensor * m11xm2 = ggml_mul_mat(ctx, m11, m2);

    // GGML_JNI_NOTIFY("Creating compute graph\n");
    struct ggml_cgraph * gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, m11xm2);

    GGML_JNI_NOTIFY("n_threads=%i\n", benchmark_params.n_threads);

    TENSOR_DUMP(m11);
    TENSOR_DUMP(m2);

    std::vector<uint8_t> work_buffer;

    ggml_graph_compute_helper(work_buffer, gf, benchmark_params.n_threads);

    TENSOR_DUMP(gf->nodes[0]);

    GGML_JNI_NOTIFY("\n------ Test 2 - Matrix Mult via %s code\n", ggml_type_name(qtype));

    int32_t nelements = sizex*sizey;

    // Set up a the benchmark matrices
    // GGML_JNI_NOTIFY("Creating new tensor q11 & Running quantize\n");
    struct ggml_tensor * q11 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
    ggml_quantize_chunk(qtype, (const float *) m11->data, q11->data, 0, nelements/m11->ne[0], m11->ne[0], nullptr);

    // Set up a the compute graph
    // GGML_JNI_NOTIFY("Creating new tensor q31\n");
    struct ggml_tensor * q31 = ggml_mul_mat(ctx, q11, m2);

    // GGML_JNI_NOTIFY("Creating compute graph\n");
    struct ggml_cgraph * gf31 = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf31, q31);

    // Set up a second graph computation to make sure we override the CPU cache lines
    // GGML_JNI_NOTIFY("Creating new tensor q12 & Running quantize\n");
    struct ggml_tensor * q12 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
    ggml_quantize_chunk(qtype, (const float *) m12->data, q12->data, 0, nelements/m12->ne[0], m12->ne[0], nullptr);

    // GGML_JNI_NOTIFY("Creating new tensor q32\n");
    struct ggml_tensor * q32 = ggml_mul_mat(ctx, q12, m2);

    //GGML_JNI_NOTIFY("Creating compute graph\n");
    struct ggml_cgraph * gf32 = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf32, q32);
    GGML_JNI_NOTIFY("n_threads=%i\n", benchmark_params.n_threads);

    const int dimx = sizex;
    const int dimy = sizey;
    const int dimz = sizez;
    long long int flops_per_dot_product = dimy + dimy;
    long long int flops_per_matrix = flops_per_dot_product * dimx * dimz; ;
    GGML_JNI_NOTIFY("Matrix Multiplication of (%i,%i,%i) x (%i,%i,%i) - about %6.2f gFLOPS\n\n", sizex, sizey, 1, sizex, sizez, 1, 1.0f*flops_per_matrix / 1000 / 1000 / 1000);


    // Let's use the F32 result from above as a reference for the quantized multiplication
    float sum_of_F32_reference = tensor_sum_elements(gf->nodes[0]);

    GGML_JNI_NOTIFY("Iteration;NThreads; SizeX; SizeY; SizeZ; Required_FLOPS; Elapsed_u_Seconds; gigaFLOPS\n");
    GGML_JNI_NOTIFY("=====================================================================================\n");

    double  gflops_sum = 0;
    for (int i=0;i<benchmark_params.n_iterations ;i++) {

        long long int start = ggml_time_us();
        //GGML_JNI_NOTIFY("Running ggml_graph_compute\n");
        ggml_graph_compute_helper(work_buffer, gf31, benchmark_params.n_threads);

        long long int stop = ggml_time_us();
        long long int usec = stop-start;
        double gflops = (double)(flops_per_matrix)/usec/1000.0;
        gflops_sum += gflops;
        GGML_JNI_NOTIFY("%9i;%8i;%6i;%6i;%6i;%15lli;%18lli;%10.2f\n",
               i,
               benchmark_params.n_threads,
               sizex, sizey, sizez, flops_per_matrix,
               usec,gflops);

#ifdef VERBOSE_DEBUGGING
        TENSOR_DUMP("res",gf31.nodes[0])
#endif

        // Check that the matrix multiplication result is in the right ballpark
        // We cannot use the exact value from the F32 multiplication because the quantizuation will be slightly different
        float sum_of_Q4_result = tensor_sum_elements(gf31->nodes[0]);
        float delta = std::abs(sum_of_Q4_result - sum_of_F32_reference);
        float allowed_delta = (sum_of_F32_reference) / 1000 / 1000; //  Let's accept an epsilon of 10^-6

        if (delta > allowed_delta)  {
            GGML_JNI_NOTIFY("\nABORT - ERROR in Matrix Multiplication result - expected %6.2f, got %6.2f (delta %6.2f > allowed_delta %6.2f)\n",
                   sum_of_F32_reference,
                   sum_of_Q4_result,
                   delta,
                   allowed_delta
            );
            return;
        }

        // Running a different graph computation to make sure we override the CPU cache lines
        ggml_graph_compute_helper(work_buffer, gf32, benchmark_params.n_threads);
    }
    GGML_JNI_NOTIFY("\n");
    GGML_JNI_NOTIFY("Average%78.2f\n",gflops_sum/((double)benchmark_params.n_iterations));
    GGML_JNI_NOTIFY("=====================================================================================\n");

    LOGGD("leave ggml_bench_matrix\n");
}