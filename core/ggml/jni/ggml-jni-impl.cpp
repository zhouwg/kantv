/*
 * Copyright (c) 2024- KanTV Authors
 *
 * this clean-room original implementation is for
 *
 * PoC(https://github.com/zhouwg/kantv/issues/64) in project KanTV. the initial implementation was done
 *
 * from 03-05-2024 to 03-16-2024. the initial implementation could be found at:
 *
 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/external/whispercpp/whisper.cpp#L6727

 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/external/whispercpp/whisper.h#L620

 * https://github.com/zhouwg/kantv/blob/kantv-poc-with-whispercpp/external/whispercpp/jni/whispercpp-jni.c

 *
 * in short, it's a very concise implementation and the method here is never seen in any other similar
 *
 * (whisper.cpp related) open-source project before 03-05-2024.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

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
#include <tuple>
#include <queue>
#include <unordered_map>
#include <vector>

#ifdef GGML_USE_HEXAGON
//03-31-2024,18:00, for PoC https://github.com/zhouwg/kantv/issues/121
#include "ggml-hexagon.h"

#include "QnnTypes.h"
#include "QnnCommon.h"
#include "QnnContext.h"
#include "QnnBackend.h"
#include "QnnGraph.h"
#include "QnnProperty.h"
#include "QnnTensor.h"
#include "QnnInterface.h"
#endif

extern "C" {
#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

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
    size_t n_backend;                                //added on 04-17-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
    size_t n_threads;

    //03-20-2024,referenced by:https://github.com/futo-org/whisper-acft
    size_t n_decoding_mode;                          // 0:WHISPER_SAMPLING_GREEDY 1:WHISPER_SAMPLING_BEAM_SEARCH

    size_t n_asr_mode;                               // 0: normal transcription  1: asr pressure test 2:benchmark 3: transcription + audio record
    size_t n_benchmark_type;                         // what to benchmark:
                                                     // 0: asr(transcription) 1: memcpy 2: mulmat  3: full/whisper_encode 4: matrix  5: LLAMA inference

    bool   b_use_gpu;
    size_t gpu_device;

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

static whisper_asr_context * p_asr_ctx   = NULL;

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
            LOGD("open file %s failed(reason:%s)", sz_file_name, strerror(errno));
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


const char * ggml_jni_get_ggmltype_str(enum ggml_type wtype) {
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


static int whisper_bench_full() {
    if (NULL == p_asr_ctx) {
        LOGGW("pls check whether asr_ctx already initialized?\n");
        return 1;
    }

    // whisper init
    struct whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = p_asr_ctx->b_use_gpu;
    cparams.gpu_device = p_asr_ctx->gpu_device;

    //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
    whisper_free(p_asr_ctx->p_context);

    struct whisper_context * ctx = whisper_init_from_file_with_params(p_asr_ctx->sz_model_path, cparams);
    //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
    p_asr_ctx->p_context = ctx;
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
    //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
    p_asr_ctx->p_context = nullptr;

    return 0;
}


/**
 * @param model_path       /sdcard/kantv/ggml-xxxxx.bin
 * @param audio_path       /sdcard/kantv/jfk.wav
 * @param num_threads      1 - 8
 * @param n_backend_type   0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
 *
 * @return asr result which generated by whispercpp's inference
 */
static const char * ggml_jni_transcribe_from_file(const char * sz_model_path, const char * sz_audio_path, int num_threads, int n_backend_type) {
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
        whisper_free(p_asr_ctx->p_context);
        struct whisper_context_params wcp = whisper_context_default_params();
        LOGGD("backend %d", n_backend_type);
        wcp.gpu_device  = n_backend_type;
        wcp.use_gpu     = false;
#ifdef GGML_USE_HEXAGON
        if (n_backend_type != HEXAGON_BACKEND_GGML) {
            wcp.use_gpu = true;
        } else {
            wcp.use_gpu = false;
        }
#endif
        context = whisper_init_from_file_with_params(sz_model_path, wcp);
        p_asr_ctx->p_context = context;
        if (nullptr == context) {
            LOGGW("whisper_init_from_file_with_params failure, pls check why\n");
            GGML_JNI_NOTIFY("whisper_init_from_file_with_params failure, pls check why? whether whispercpp model %s is valid ?\n", sz_model_path);
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
        p_asr_ctx->p_context = nullptr;
    }

    if (0 == result) {
        if (num_segments != 0) {
            GGML_JNI_NOTIFY("%s", asr_result.c_str());
            return "asr_result";
        } else {
            asr_result = "pls check why whisper.cpp inference failure: whether whisper model file exist? whether jfk.wav is correct?\n";
            GGML_JNI_NOTIFY("%s", asr_result.c_str());
            LOGGD("%s", asr_result.c_str());
            return NULL;
        }
    } else
        return NULL;
}


static fifo_buffer_t  * whisper_asr_getfifo() {
    if (NULL == p_asr_ctx)
        return NULL;

    if (NULL == p_asr_ctx->asr_fifo)
        return NULL;

    return p_asr_ctx->asr_fifo;
}


// =================================================================================================
//
// dirty method for fix UI issue when user cancel time-consuming bench task in UI layer.
// background computing task(it's a blocked task) in native layer might be not finished
// when user cancel time-consuming bench task in UI layer
//
// =================================================================================================
static int s_ggml_jni_abortbenchmark_flag = 0;
static std::mutex s_ggml_jni_abortbenchmark_mutex;

int ggml_jni_get_abortbenchmark_flag() {
    int n_abortbenchmark_value = 0;
    {
        std::lock_guard<std::mutex> lock(s_ggml_jni_abortbenchmark_mutex);
        n_abortbenchmark_value = s_ggml_jni_abortbenchmark_flag;
    }
    return n_abortbenchmark_value;
}


bool ggml_jni_abort_callback(void * data) {
    return ggml_jni_get_abortbenchmark_flag();
}


void ggml_jni_set_abortbenchmark_flag(int b_exit_benchmark) {
    LOGGI("set b_abort_benchmark to %d", b_exit_benchmark);
    {
        std::lock_guard<std::mutex> lock(s_ggml_jni_abortbenchmark_mutex);
        s_ggml_jni_abortbenchmark_flag = b_exit_benchmark;
    }
    if (NULL == p_asr_ctx)
        return;

    p_asr_ctx->b_abort_benchmark = ((1 == b_exit_benchmark) ? true : false);
}


// =================================================================================================
//
// JNI helper function for benchmark
// referenced with codes in examples/bench/bench.cpp of project whisper.cpp
//
// =================================================================================================
int ggml_jni_get_cpu_core_counts() {
    return std::thread().hardware_concurrency();
}


/**
  *
  * @param sz_model_path   /sdcard/kantv/models/file_name_of_gguf_model or qualcomm's prebuilt dedicated model.so or ""
  * @param sz_user_data    ASR: /sdcard/kantv/jfk.wav / LLM: user input / TEXT2IMAGE: user input / MNIST: image path / TTS: user input
  * @param n_bench_type    0: memcpy 1: mulmat 2: QNN GGML OP(QNN UT) 3: QNN UT automation 4: ASR(whisper.cpp) 5: LLM(llama.cpp)
  * @param n_threads       1 - 8
  * @param n_backend_type  0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
  * @param n_accel_type    0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
  * @return
*/
void ggml_jni_bench(const char * sz_model_path, const char * sz_user_data, int n_bench_type, int n_threads, int n_backend_type, int n_accel_type) {
    int result = 0;

    if (NULL == p_asr_ctx) {
        LOGGW("pls check whether asr_ctx already initialized?\n");
        return;
    }

    if (NULL == sz_model_path) {
        LOGGW("pls check model path\n");
        return;
    }

    if (NULL == sz_user_data) {
        LOGGW("pls check user data\n");
        return;
    }

    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("bench type:%d\n", n_bench_type);
    LOGGD("backend type:%d\n", n_backend_type);
    LOGGD("accel type:%d\n", n_accel_type);

#ifdef GGML_USE_HEXAGON
    if (HEXAGON_BACKEND_GGML == n_backend_type) {
        p_asr_ctx->b_use_gpu = false;
        p_asr_ctx->gpu_device = HEXAGON_BACKEND_GGML;
    } else {
        p_asr_ctx->b_use_gpu = true;
        p_asr_ctx->gpu_device = n_backend_type;
    }
#endif

    p_asr_ctx->n_threads                = n_threads;
    p_asr_ctx->n_benchmark_type         = n_bench_type;
    memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
    strncpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));

    kantv_asr_notify_benchmark_c("reset");

    switch (n_bench_type) {
        case GGML_BENCHMARK_MEMCPY:
            ggml_jni_bench_memcpy(n_threads);
            break;

        case GGML_BENCHMARK_MULMAT:
            ggml_jni_bench_mulmat(n_threads, n_backend_type);
            break;

        case GGML_BENCHMARK_ASR:
            ggml_jni_transcribe_from_file(sz_model_path, sz_user_data, n_threads, n_backend_type);
            break;

        case GGML_BENCHMARK_LLM:
            llama_inference(sz_model_path, sz_user_data, n_bench_type, n_threads, n_backend_type, n_accel_type);
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
        if (ggml_jni_is_valid_utf8(asr_result.c_str())) {
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
  * @param n_backend            0: Hexagon NPU 1: ggml
  */
int whisper_asr_init(const char * sz_model_path, int n_threads, int n_asrmode, int n_backend) {
     LOGGV("enter whisper_asr_init\n");
     int result         = 0;

     struct whisper_full_params params;
     struct whisper_context_params c_params = whisper_context_default_params();

     //FIXME:hardcoded binary ggml backend path
     char * backend_lib_path = "/data/data/com.kantvai.kantvplayer/";
     ggml_backend_load_all_from_path(backend_lib_path);

     if ((NULL == sz_model_path) || (n_asrmode > 3)) {
         LOGGW("invalid param\n");
         return 1;
     }
     LOGGV("model path:%s\n", sz_model_path);
     LOGGV("thread counts:%d\n", n_threads);
     LOGGV("asr mode:%d\n", n_asrmode);
     LOGGV("backend type:%d\n", n_backend);

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
     p_asr_ctx->n_backend = n_backend;

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
     c_params.gpu_device  = n_backend;
     c_params.use_gpu     = false;
#ifdef GGML_USE_HEXAGON
     if (n_backend != HEXAGON_BACKEND_GGML) {
         c_params.use_gpu = true;
     } else {
         c_params.use_gpu = false;
     }
#endif
     p_asr_ctx->p_context = whisper_init_from_file_with_params(sz_model_path, c_params);

     if (nullptr == p_asr_ctx->p_context) {
         result = 8;
         LOGGW("whispercpp initialize failure\n");
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

     params.debug_mode              = false;
     params.audio_ctx               = 0;

     params.suppress_blank          = true;

     //ref: https://github.com/ggerganov/whisper.cpp/issues/1507
     //reduce the maximum context size (--max-context). By default it is 224. Setting it to 64 or 32 can reduce the repetitions significantly. Setting it to 0 will most likely eliminate all repetitions, but the transcription quality can be affected because it will be losing the context from the previous transcript
     params.n_max_text_ctx              = 224; //default value
     //params.n_max_text_ctx              = 0;

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
 * @param n_backend            0: Hexagon NPU 1: ggml
 */
int whisper_asr_reset(const char * sz_model_path, int n_threads, int n_asrmode, int n_backend) {
    int result = 0;

    LOGGD("enter asr reset\n");
    LOGGD("backend type:%d\n", n_backend);
    if (NULL == p_asr_ctx) {
        LOGGW("asr instance not initialized, pls check why\n");
        GGML_JNI_NOTIFY("asr instance not initialized, pls check why\n");
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
        struct whisper_context_params wcp = whisper_context_default_params();
        wcp.gpu_device  = n_backend;
        wcp.use_gpu     = false;
#ifdef GGML_USE_HEXAGON
        if (n_backend != HEXAGON_BACKEND_GGML) {
            wcp.use_gpu = true;
        } else {
            wcp.use_gpu = false;
        }
#endif
        p_asr_ctx->p_context = whisper_init_from_file_with_params(sz_model_path, wcp);
        memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
        memcpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));
    } else if (n_backend != p_asr_ctx->n_backend) {
        LOGGI("backend changed from %d to %d", p_asr_ctx->n_backend, n_backend);
        if (nullptr != p_asr_ctx->p_context) {
            whisper_free(p_asr_ctx->p_context);
        }
        struct whisper_context_params wcp = whisper_context_default_params();
        wcp.gpu_device  = n_backend;
        wcp.use_gpu     = false;
#ifdef GGML_USE_HEXAGON
        if (n_backend != HEXAGON_BACKEND_GGML) {
            wcp.use_gpu = true;
        } else {
            wcp.use_gpu = false;
        }
#endif
        p_asr_ctx->p_context = whisper_init_from_file_with_params(sz_model_path, wcp);
        p_asr_ctx->n_backend = n_backend;
    } else if (nullptr == p_asr_ctx->p_context) {
        LOGGD("re-init whispercpp instance");
        struct whisper_context_params wcp = whisper_context_default_params();
        wcp.gpu_device  = n_backend;
        wcp.use_gpu     = false;
#ifdef GGML_USE_HEXAGON
        if (n_backend != HEXAGON_BACKEND_GGML) {
            wcp.use_gpu = true;
        } else {
            wcp.use_gpu = false;
        }
#endif
        p_asr_ctx->p_context = whisper_init_from_file_with_params(sz_model_path, wcp);
        p_asr_ctx->n_backend = n_backend;
    } else {
        LOGGD("using cached whispercpp instance\n");
    }

    if (nullptr == p_asr_ctx->p_context) {
        LOGGW("it should not happen and ASR feature would be not available, pls check why?\n");
        result = 3;
    } else {
        p_asr_ctx->n_threads            = n_threads;
        p_asr_ctx->p_params->n_threads  = n_threads;
        p_asr_ctx->n_asr_mode           = n_asrmode;
        p_asr_ctx->n_backend            = n_backend;
    }

    LOGGD("leave asr reset\n");
    return result;
}
