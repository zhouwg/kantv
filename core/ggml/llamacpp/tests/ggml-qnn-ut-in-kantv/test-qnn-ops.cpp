/*
 * Copyright (c) 2024- KanTV Authors
 *
 * this is implementation of Android command line application for verify GGML QNN backend, only used in this project
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
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <tuple>
#include <queue>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <memory>
#include <regex>
#include <random>
#include <functional>
#include <unordered_map>
#include <condition_variable>
#include <cassert>
#include <unordered_set>
#include <utility>

extern "C" {
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

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-qnn.h"
#include "whisper.h"

#define GGML_QNN_DEBUG      1
#define GGML_QNN_LOGBUF_LEN 4096

#define QNN_LOG_ERROR(...)  ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG,  __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define QNN_LOG_WARN(...)   ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG , __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define QNN_LOG_INFO(...)   ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG , __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#if GGML_QNN_DEBUG
#define QNN_LOG_DEBUG(...)  ggml_qnn_log_internal(GGML_LOG_LEVEL_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#else
#define QNN_LOG_DEBUG(...)
#endif

enum ut_test_type {
    TEST_SIMPLE_UT = 0,
    TEST_AUTOMATION_UT,
    TEST_WHISPERCPP,
    UT_COUNTS
};

static void tensor_dump(const ggml_tensor * tensor, const char * name);

#define TENSOR_DUMP(tensor) tensor_dump(tensor, #tensor)

static void ggml_qnn_log_internal(ggml_log_level level, const char * file, const char * func, int line, const char * format, ...) {
    static std::mutex ggml_qnn_log_internal_mutex;
    static char s_ggml_qnn_log_internal_buf[GGML_QNN_LOGBUF_LEN];

    {
        std::lock_guard<std::mutex> lock(ggml_qnn_log_internal_mutex);
        va_list args;
        va_start(args, format);
        int len_prefix = snprintf(s_ggml_qnn_log_internal_buf, GGML_QNN_LOGBUF_LEN, "[%s, %d]: ", func, line);
        int len = vsnprintf(s_ggml_qnn_log_internal_buf + len_prefix, GGML_QNN_LOGBUF_LEN - len_prefix, format, args);
        if (len < (GGML_QNN_LOGBUF_LEN - len_prefix)) {
            //for Android command line application or WoA
            printf("%s", s_ggml_qnn_log_internal_buf);
        }
        va_end(args);
    }
}


static const char * get_qnn_backend_name(int n_backend_type) {
    switch (n_backend_type) {
        case 0:
            return "QNN-CPU";
        case 1:
            return "QNN-GPU";
        case 2:
            return "QNN-NPU";
        case 3:
            return "ggml";
        default:
            return "unknown";
    }
}


static bool ggml_graph_compute_helper(
        struct ggml_backend * backend,
        struct ggml_cgraph * graph,
        std::vector<uint8_t> & buf,
        int n_threads,
        ggml_abort_callback abort_callback,
        void * abort_callback_data) {
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);

    plan.abort_callback = abort_callback;
    plan.abort_callback_data = abort_callback_data;

    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.data();
    }

    if (ggml_backend_is_cpu(backend)) {
        ggml_backend_cpu_set_n_threads(backend, n_threads);
    }

#ifdef GGML_USE_QNN
    if (ggml_backend_is_qnn(backend)) {
        ggml_backend_qnn_set_n_threads(backend, n_threads);
    }
#endif

    //a new approch of mixed inference
    if (nullptr != backend)
        return ggml_backend_graph_compute(backend, graph) == GGML_STATUS_SUCCESS;
    else
        return ggml_graph_compute(graph, &plan);
}


static void tensor_dump_elements(const ggml_tensor * tensor) {
    float value = 0;
    std::ostringstream tmposs;
    if (nullptr == tensor) {
        QNN_LOG_WARN("tensor is null");
        return;
    }
    if (tensor->type == GGML_TYPE_F32) {
        for (int h = 0; h < tensor->ne[3]; h++) {
            for (int i = 0; i < tensor->ne[2]; i++) {
                for (int j = 0; j < tensor->ne[1]; j++) {
                    for (int k = 0; k < tensor->ne[0]; k++) {
                        value = ((float *) tensor->data)[h * tensor->ne[2] + i * tensor->ne[1] +
                                                         j * tensor->ne[0] + k];
                        tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value
                               << " ";
                    }
                    if (strlen(tmposs.str().c_str()) <= (GGML_QNN_LOGBUF_LEN - 96)) {
                        QNN_LOG_DEBUG("%s\n", tmposs.str().c_str());
                    }
                    tmposs.clear();
                    tmposs.str("");
                    //QNN_LOG_DEBUG("\n");
                }
            }
        }
    }

    QNN_LOG_DEBUG("\n");
}


static void tensor_dump(const ggml_tensor * tensor, const char * name) {
    QNN_LOG_DEBUG("dump ggml tensor %s(%s)\n", name, tensor->name);
    QNN_LOG_DEBUG("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
          name,
          tensor->type, ggml_type_name(tensor->type),
          tensor->ne[0], tensor->ne[1], tensor->ne[2],
          tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    tensor_dump_elements(tensor);

    //QNN_LOG_DEBUG("\n");
}


static uint32_t get_tensor_rank(const ggml_tensor * tensor) {
    uint32_t rank = 0;
    for (int i = 0; i < GGML_MAX_DIMS; i++) {
        if ((0 != tensor->ne[i]) && (1 != tensor->ne[i])) {
            rank++;
        }
    }
    return rank;
}


static uint32_t get_tensor_data_size(const ggml_tensor * tensor) {
    size_t data_size = ggml_row_size(tensor->type, tensor->ne[0]);
    size_t n_dims = get_tensor_rank(tensor);
    for (int i = 1; i < n_dims; i++) {
        data_size *= tensor->ne[i];
    }

    QNN_LOG_DEBUG("get_tensor_data_size %d\n", data_size);
    QNN_LOG_DEBUG("ggml_nbytes(tensor) %d\n", ggml_nbytes(tensor));

    return ggml_nbytes(tensor);
}


//ref: https://github.com/ggerganov/llama.cpp/blob/master/tests/test-backend-ops.cpp#L20
static void init_tensor_uniform(ggml_tensor * tensor, float min = -1.0f, float max = 1.0f) {
    // static RNG initialization (revisit if n_threads stops being constant)
    static const size_t n_threads = std::thread::hardware_concurrency();
    static std::vector<std::default_random_engine> generators = []() {
        std::random_device rd;
        std::vector<std::default_random_engine> vec;
        vec.reserve(n_threads);
        //for (size_t i = 0; i < n_threads; i++) { vec.emplace_back(1234 + i); } // fixed seed
        for (size_t i = 0; i < n_threads; i++) { vec.emplace_back(rd()); }
        return vec;
    }();

    size_t size = ggml_nelements(tensor);
    std::vector<float> data(size);

    auto init_thread = [&](size_t ith, size_t start, size_t end) {
        std::uniform_real_distribution<float> distribution(min, max);
        for (size_t i = start; i < end; i++) {
            data[i] = distribution(generators[ith]);
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(n_threads);
    for (size_t i = 0; i < n_threads; i++) {
        size_t start =     i*size/n_threads;
        size_t end   = (i+1)*size/n_threads;
        threads.emplace_back(init_thread, i, start, end);
    }
    for (auto & t : threads) {
        t.join();
    }
    if (tensor->type == GGML_TYPE_F32 || tensor->type == GGML_TYPE_I32) {
        ggml_backend_tensor_set(tensor, data.data(), 0, size * sizeof(float));
    } else if (ggml_is_quantized(tensor->type) || tensor->type == GGML_TYPE_F16 || tensor->type == GGML_TYPE_BF16) {
        GGML_ASSERT(size % ggml_blck_size(tensor->type) == 0);
        std::vector<uint8_t> dataq(ggml_row_size(tensor->type, size));
        std::vector<float> imatrix(tensor->ne[0], 1.0f); // dummy importance matrix
        const float * im = imatrix.data();
        if (!ggml_quantize_requires_imatrix(tensor->type)) {
            // when the imatrix is optional, we want to test both quantization with and without imatrix
            // use one of the random numbers to decide
            if (data[0] > 0.5f*(min + max)) {
                im = nullptr;
            }
        }
        ggml_quantize_chunk(tensor->type, data.data(), dataq.data(), 0, size/tensor->ne[0], tensor->ne[0], im);
        GGML_ASSERT(ggml_validate_row_data(tensor->type, dataq.data(), dataq.size()));
        ggml_backend_tensor_set(tensor, dataq.data(), 0, dataq.size());
    } else if (tensor->type == GGML_TYPE_I8 || tensor->type == GGML_TYPE_I16 || tensor->type == GGML_TYPE_I32) {
        // This is going to create some weird integers though.
        ggml_backend_tensor_set(tensor, data.data(), 0, ggml_nbytes(tensor));
    } else {
        GGML_ASSERT(false);
    }
}


//ref: https://github.com/ggerganov/llama.cpp/blob/master/tests/test-backend-ops.cpp#L310
static void initialize_tensors(ggml_context * ctx) {
    for (ggml_tensor * t = ggml_get_first_tensor(ctx); t != nullptr; t = ggml_get_next_tensor(ctx, t)) {
        init_tensor_uniform(t);
    }
}


const char * get_ggmltype_str(enum ggml_type wtype) {
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


static int qnn_op_ut_automation(int num_threads, int n_backend_type, int n_ggml_op_type) {
    int result = 0;
    int64_t n_begin_time = 0LL;
    int64_t n_end_time = 0LL;
    int64_t n_durtion = 0LL;


    QNN_LOG_DEBUG("enter qnn_ggml_op_automation_ut\n");
    QNN_LOG_DEBUG("num_threads:%d", num_threads);
    QNN_LOG_DEBUG("backend_type:%d(%s)", n_backend_type, get_qnn_backend_name(n_backend_type));
    QNN_LOG_DEBUG("ggml op:%d(%s)", n_ggml_op_type, ggml_op_name((enum ggml_op) n_ggml_op_type));

    n_begin_time = ggml_time_us();

    srand(time(NULL));

    bool support_ops = (n_ggml_op_type == GGML_OP_MUL_MAT || n_ggml_op_type == GGML_OP_MUL ||
                        n_ggml_op_type == GGML_OP_ADD);
    if (!support_ops) {
        QNN_LOG_DEBUG("ggml op %d(%s) not supported  with backend %d(%s)", n_ggml_op_type,
              ggml_op_name((enum ggml_op) n_ggml_op_type), n_backend_type,
              get_qnn_backend_name(n_backend_type));
        QNN_LOG_DEBUG("leave qnn_ggml_op UT(unit test)\n");

        return 1;
    }


    char strbuf[256];
    std::string tipString = "";
    std::string s = "";

    const int n_max = 128;

    const std::vector<size_t> sizes = {
            64, 128, 256, 512, 1024, 2048, 4096,
    };

    const size_t N_max = sizes.back();

    // a: N*N*sizeof(float)
    // b: N*N*sizeof(float)
    // c: N*N*sizeof(float)
    // when F16 is used, there is an extra work buffer of size N*N*sizeof(float)
    std::vector<uint8_t> buf(
            3llu * N_max * N_max * sizeof(float) + 3 * ggml_tensor_overhead() +
            ggml_graph_overhead());
    std::vector<uint8_t> work;


    tipString += "\nprepare matrix";

    for (size_t i = 0; i < buf.size(); i++) buf[i] = i;

    for (int j = 0; j < (int) sizes.size(); j++) {
        int n_q4_0 = 0;
        int n_q4_1 = 0;
        int n_q5_0 = 0;
        int n_q5_1 = 0;
        int n_q8_0 = 0;
        int n_fp16 = 0;
        int n_fp32 = 0;

        // GFLOPS/s
        double s_q4_0 = 0.0;
        double s_q4_1 = 0.0;
        double s_q5_0 = 0.0;
        double s_q5_1 = 0.0;
        double s_q8_0 = 0.0;
        double s_fp16 = 0.0;
        double s_fp32 = 0.0;

        const size_t N = sizes[j];

#if 0
        for (int k = 0; k < 7; ++k) {
            const ggml_type wtype =
                    k == 0 ? GGML_TYPE_Q4_0 :
                    k == 1 ? GGML_TYPE_Q4_1 :
                    k == 2 ? GGML_TYPE_Q5_0 :
                    k == 3 ? GGML_TYPE_Q5_1 :
                    k == 4 ? GGML_TYPE_Q8_0 :
                    k == 5 ? GGML_TYPE_F16  : GGML_TYPE_F32;
#else
        for (int k = 5; k < 7; ++k) {
            const ggml_type wtype =
                    k == 5 ? GGML_TYPE_F16
                           : GGML_TYPE_F32; //TODO: only f16&f32 supported with QNN backend
#endif


            double &s =
                    k == 0 ? s_q4_0 : k == 1 ? s_q4_1 : k == 2 ? s_q5_0 : k == 3 ? s_q5_1 :
                                                                          k == 4 ? s_q8_0 :
                                                                          k == 5 ? s_fp16
                                                                                 : /*k == 6*/ s_fp32;
            int &n = k == 0 ? n_q4_0 : k == 1 ? n_q4_1 : k == 2 ? n_q5_0 : k == 3 ? n_q5_1 :
                                                                           k == 4 ? n_q8_0 :
                                                                           k == 5 ? n_fp16
                                                                                  : /*k == 6*/ n_fp32;

            ggml_backend_t backend = nullptr;
            ggml_backend_buffer_t buffer = nullptr;
#ifdef GGML_USE_QNN
            if (n_backend_type !=
                3) {//3 is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
                backend = ggml_backend_qnn_init(n_backend_type,
                                                "/data/local/tmp/"); // the second param can be got by JNI from Java layer
                if (nullptr == backend) {
                    QNN_LOG_DEBUG("create qnn backend %d(%s) failed\n", n_backend_type,
                          get_qnn_backend_name(n_backend_type));
                    return 1;
                }
            }
            num_threads = 1;
#endif
            struct ggml_init_params gparams = {
                    /*.mem_size   =*/ buf.size(),
                    /*.mem_buffer =*/ buf.data(),
                    /*.no_alloc   =*/ false,
            };
#ifdef GGML_USE_QNN
            if (n_backend_type != QNN_BACKEND_GGML) {//QNN_BACKEND_GGML is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
                gparams.use_hwaccel = true;
                gparams.no_alloc = true;
            }
#endif
            struct ggml_context *ctx0 = ggml_init(gparams);

            struct ggml_tensor *b = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, N, N); //avoid assert failure in ggml.c
            struct ggml_tensor *a = ggml_new_tensor_2d(ctx0, GGML_TYPE_F32, N, N);
            ggml_set_input(a);
            ggml_set_input(b);

            struct ggml_tensor *c = nullptr;

            switch (n_ggml_op_type) {
                case GGML_OP_ADD:
                    c = ggml_add(ctx0, a, b);
                    break;
                case GGML_OP_MUL:
                    c = ggml_mul(ctx0, a, b);
                    break;
                case GGML_OP_MUL_MAT:
                    c = ggml_mul_mat(ctx0, a, b);
                    break;
            }
            ggml_set_output(c);
#ifdef GGML_USE_QNN
            if (n_backend_type !=
                QNN_BACKEND_GGML) {//QNN_BACKEND_GGML is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
                QNN_LOG_DEBUG("creating backend buffer\n");
                buffer = ggml_backend_alloc_ctx_tensors(ctx0, backend);
                if (!buffer) {
                    QNN_LOG_DEBUG("%s: failed to allocate backend buffer\n", __func__);
                    ggml_backend_free(backend);
                    return false;
                }
            }
#endif

            struct ggml_cgraph *gf = ggml_new_graph(ctx0);

            ggml_build_forward_expand(gf, c);

            double tsum = 0.0;

            // heat-up
            ggml_graph_compute_helper(backend, gf, work, num_threads, nullptr, nullptr);

            for (int i = 0; i < n_max; ++i) {
                const int64_t t0 = ggml_time_us();

                //tipString = "calling ggml_graphic_compute_helper:\n";
                tipString = "";
                tipString += "j= " + std::to_string(j) + "(matrix dimension = " +
                             std::to_string(N) + ",n_max=" + std::to_string(n_max) + ")"
                             + ",k=" + std::to_string(k) + "(ggml quant type=" +
                             std::string(get_ggmltype_str(
                                     static_cast<ggml_type>(wtype))) + ")"
                             + ",i=" + std::to_string(i) + "\n";

                QNN_LOG_DEBUG("%s\n", tipString.c_str());

                ggml_graph_compute_helper(backend, gf, work, num_threads, nullptr, nullptr);

                const int64_t t1 = ggml_time_us();

                tsum += (t1 - t0) * 1e-6;
                n++;

                if (tsum > 1.0 && n >= 3) {
                    break;
                }
            }

            ggml_free(ctx0);
            ggml_backend_buffer_free(buffer);
            ggml_backend_free(backend);

            s = ((2.0 * N * N * N * n) / tsum) * 1e-9;
        }

#if 0
        // Q4_0 | Q4_1
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: Q4_0 %7.1f GFLOPS (%3d runs) | Q4_1 %7.1f GFLOPS (%3d runs)\n",
                 N, N, s_q4_0, n_q4_0, s_q4_1, n_q4_1);
        s += strbuf;

        // Q5_0 | Q5_1 | Q8_0
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: Q5_0 %7.1f GFLOPS (%3d runs) | Q5_1 %7.1f GFLOPS (%3d runs) | Q8_0 %7.1f GFLOPS (%3d runs)\n",
                 N, N, s_q5_0, n_q5_0, s_q5_1, n_q5_1, s_q8_0, n_q8_0);
        s += strbuf;
#endif
        // F16 | F32
        snprintf(strbuf, sizeof(strbuf),
                 "%4zu x %4zu: F16  %10.1f GFLOPS (%3d runs) | F32  %8.1f GFLOPS (%3d runs)\n",
                 N, N, s_fp16, n_fp16, s_fp32, n_fp32);
        s += strbuf;


        QNN_LOG_DEBUG("%s\n", s.c_str());
    }

    n_end_time = ggml_time_us();
    n_durtion = (n_end_time - n_begin_time) / 1000;
    QNN_LOG_DEBUG("duration of qnn_ggml_op_automation_ut GGML_OP_%s with backend %d(%s) is: %lld milliseconds\n",
          ggml_op_name((enum ggml_op) n_ggml_op_type), n_backend_type,
          get_qnn_backend_name(n_backend_type), n_durtion);
    QNN_LOG_DEBUG("leave qnn_ggml_op_automation_ut(automation unit test)\n");

    return 0;
}


static int qnn_op_ut(int num_threads, int n_backend_type, int n_ggml_op_type) {
    int64_t n_begin_time        = 0LL;
    int64_t n_end_time          = 0LL;
    int64_t n_duration          = 0LL;
    size_t  ctx_size            = 0;
    int     sizey               = 4;
    int     sizex               = 4;

    struct ggml_context * ctx   = nullptr;
    struct ggml_cgraph  * gf    = nullptr;
    struct ggml_tensor  * src0  = nullptr;
    struct ggml_tensor  * src1  = nullptr;
    struct ggml_tensor  * dst   = nullptr;
    ggml_backend_t backend      = nullptr;
    ggml_backend_buffer_t buffer= nullptr;
    ggml_type qtype             = GGML_TYPE_F32;
    std::vector<uint8_t> work_buffer;
    QNN_LOG_DEBUG("enter qnn_ggml_op\n");
    QNN_LOG_DEBUG("ggml op:%d(%s)\n", n_ggml_op_type, ggml_op_name((enum ggml_op) n_ggml_op_type));


    n_begin_time = ggml_time_us();
    srand(time(NULL));

    ctx_size += 1024 * 1024 * 32;
    QNN_LOG_DEBUG("Allocating Memory of size %zi bytes, %zi MB\n", ctx_size,
                  (ctx_size / 1024 / 1024));

    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

    if (n_backend_type != QNN_BACKEND_GGML) {
        params.no_alloc = true;
        backend = ggml_backend_qnn_init(n_backend_type, "/data/local/tmp/");
        if (nullptr == backend) {
            QNN_LOG_ERROR("create qnn backend %d(%s) failed\n", n_backend_type, get_qnn_backend_name(n_backend_type));
            return 1;
        }
    }

    ctx = ggml_init(params);
    if (!ctx) {
        QNN_LOG_ERROR("%s: ggml_init() failed\n");
        return 2;
    }

    QNN_LOG_DEBUG("creating new tensors\n");
    QNN_LOG_DEBUG("ggml_blck_size(%s) %d\n", ggml_type_name(qtype), ggml_blck_size(qtype));
    QNN_LOG_DEBUG("ggml_type_size(%s) %d\n", ggml_type_name(qtype), ggml_type_size(qtype));
    if (qtype != GGML_TYPE_F32) {
        sizex = ggml_blck_size(qtype);
    }

    src0 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
    ggml_set_input(src0);
    src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_input(src1);

    switch (n_ggml_op_type) {
        case GGML_OP_ADD:
            dst = ggml_add(ctx, src0, src1);
            break;
        case GGML_OP_MUL:
            dst = ggml_mul(ctx, src0, src1);
            break;
        case GGML_OP_MUL_MAT:
            dst = ggml_mul_mat(ctx, src0, src1);
            break;
        default:
            QNN_LOG_WARN("ggml op %d(%s) not supported", n_ggml_op_type,
                         ggml_op_name((enum ggml_op) n_ggml_op_type));
            ggml_free(ctx);
            ggml_backend_free(backend);
            return 3;
    }

    ggml_set_output(dst);
#ifdef GGML_USE_QNN
    if (n_backend_type != QNN_BACKEND_GGML) {
        buffer = ggml_backend_alloc_ctx_tensors(ctx, backend);
        if (!buffer) {
            QNN_LOG_ERROR("%s: failed to allocate backend buffer\n", __func__);
            ggml_free(ctx);
            ggml_backend_free(backend);
            return 4;
        }
    }
#endif

    QNN_LOG_DEBUG("creating compute graph\n");
    gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, dst);

#if 0
    ggml_set_f32(src0, (rand() % 100 + 1));
    ggml_set_f32(src1, (rand() % 100 + 1));
    ggml_set_f32(dst, 0.0f);
#else
    if (n_backend_type != QNN_BACKEND_GGML) {
        initialize_tensors(ctx);
    }
#endif

    ggml_graph_compute_helper(backend, gf, work_buffer, num_threads, nullptr, nullptr);

    if (get_tensor_data_size(dst) < (32 * 32)) {
        QNN_LOG_DEBUG("dump tensors:\n");
        TENSOR_DUMP(src0);
        TENSOR_DUMP(src1);
        TENSOR_DUMP(dst);
    } else {
        QNN_LOG_DEBUG("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
                      src0->name,
                      src0->type, ggml_type_name(src0->type), src0->ne[0], src0->ne[1], src0->ne[2],
                      src0->nb[0], src0->nb[1], src0->nb[2]);
        QNN_LOG_DEBUG("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
                      src1->name,
                      src1->type, ggml_type_name(src1->type), src1->ne[0], src1->ne[1], src1->ne[2],
                      src1->nb[0], src1->nb[1], src1->nb[2]);
        QNN_LOG_DEBUG("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n",
                      dst->name,
                      dst->type, ggml_type_name(dst->type), dst->ne[0], dst->ne[1], dst->ne[2], dst->nb[0],
                      dst->nb[1], dst->nb[2]);
    }

    ggml_free(ctx);
    ggml_backend_buffer_free(buffer);
    ggml_backend_free(backend);
    n_end_time = ggml_time_us();
    n_duration = (n_end_time - n_begin_time) / 1000;
    QNN_LOG_DEBUG("duration of ut GGML_OP_%s using QNN backend %s: %lld milliseconds\n", ggml_op_name((enum ggml_op)n_ggml_op_type), get_qnn_backend_name(n_backend_type), n_duration);
    return 0;
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

    pthread_mutex_t  mutex;                          // not used since 03-19-2024

    //only for troubleshooting issue
    bool     b_pre_convert;
    bool     b_enable_dump_16k_data;
} whisper_asr_context;

static whisper_asr_context * p_asr_ctx   = NULL;

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
static bool ggml_jni_is_valid_utf8(const char *string) {
    if (!string) {
        return true;
    }

    const unsigned char *bytes = (const unsigned char *) string;
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
        QNN_LOG_WARN("pls check whether asr_ctx already initialized?\n");
        return NULL;
    }

    if ((NULL == pf32_audio_buffer) || (num_samples < 1)) {
        //QNN_LOG_WARN("pls check params\n");
        return NULL;
    }

    if (1 == p_asr_ctx->n_asr_mode) { //ASR pressure test, already handled in whisper_asr_callback
        //QNN_LOG_WARN("are you in ASR pressure test mode?\n");
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
    QNN_LOG_DEBUG("decoding_mode=%d, audio_ctx=%d\n", p_asr_ctx->n_decoding_mode, p_asr_ctx->p_params->audio_ctx);

    result = whisper_full(p_asr_ctx->p_context, *p_asr_ctx->p_params, pf32_audio_buffer, num_samples);
    if (0 != result) {
        LOGW("whisper inference failure, pls check why?\n");
        result = -1;
        goto failure;
    }
    end_time = ggml_time_ms();

    QNN_LOG_WARN("whisper inference cost %d ms\n", end_time - begin_time);
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

        QNN_LOG_DEBUG("asr result:\n%s\n", asr_result.c_str());
    }

    text = asr_result.c_str();

    failure:
    if (0 != result) {
        asr_result = whisper_get_time_string() + "\n" +  "whisper inference failure, pls check why?\n";
        text = asr_result.c_str();
    }

    return text;
}


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
        QNN_LOG_WARN("pls check params");
        return NULL;
    }

    in                              = in_audio_data;
    in_audio_channels               = (in[23] << 8) | in[22];
    in_sample_rates                 = (in[27] << 24) | (in[26] << 16) | (in[25] << 8) | in[24];
    in_sample_bits                  = (in[35] << 8) | in[34];
    skip_len                        = (in[43] << 24) | (in[42] << 16) | (in[41] << 8) | in[40];
    payload_len                     = (in[51 + skip_len] << 24) | (in[50 + skip_len] << 16) | (in[49 + skip_len] << 8) | in[48 + skip_len];
    QNN_LOG_DEBUG("%c %c %c %c\n", in[44 + skip_len], in[45 + skip_len], in[46 + skip_len], in[47 + skip_len]);
    QNN_LOG_DEBUG("in_sample_rates:    %d\n", in_sample_rates);
    QNN_LOG_DEBUG("in_audio_channels:  %d\n", in_audio_channels);
    QNN_LOG_DEBUG("in_sample_bits:     %d\n", in_sample_bits);
    QNN_LOG_DEBUG("in_audio_size:      %d\n", in_audio_size);
    QNN_LOG_DEBUG("skip_len:           %d\n", skip_len);
    QNN_LOG_DEBUG("payload_len:        %d\n", payload_len);
    payload                         = in_audio_data + 44 + skip_len + 8;
    payload_len                     = in_audio_size - 44 - skip_len - 8;
    QNN_LOG_DEBUG("payload_len:        %d\n", payload_len);
    in_sample_counts                = payload_len / 2;
    *out_sample_counts              = payload_len / 2;
    out_audio_channels              = in_audio_channels;
    out_sample_rates                = in_sample_rates;
    QNN_LOG_DEBUG("in_sample_counts:   %d\n", in_sample_counts);
    HEXDUMP(in_audio_data, in_audio_size);
    HEXDUMP(payload, payload_len);

    swr_ctx = swr_alloc();
    if (NULL == swr_ctx) {
        QNN_LOG_WARN("could not allocate FFmpeg resample context\n");
        return NULL;
    }

    av_opt_set_int       (swr_ctx, "in_channel_count",    in_audio_channels,  0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",      in_sample_rates,    0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",   AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (swr_ctx, "out_channel_count",   out_audio_channels,0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",     out_sample_rates,  0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",  AV_SAMPLE_FMT_FLT, 0);
    if ((result = swr_init(swr_ctx)) < 0) {
        QNN_LOG_INFO( "Failed to initialize the resampling context\n");
        goto failure;
    }

    out_audio_data  = (float*)malloc( (in_sample_counts) * sizeof(float));
    if (NULL == out_audio_data) {
        QNN_LOG_WARN("malloc failed\n");
        goto failure;
    }

    dst_sample_counts = av_rescale_rnd(swr_get_delay(swr_ctx, in_sample_rates) + in_sample_counts, out_sample_rates, in_sample_rates, AV_ROUND_UP);
    QNN_LOG_INFO("dst_sample_counts %d\n", dst_sample_counts);
    if (dst_sample_counts != *out_sample_counts) {
        QNN_LOG_WARN("it shouldn't happen, pls check");
    }

    result = swr_convert(swr_ctx,
                         (uint8_t **)(&out_audio_data),
                         *out_sample_counts,
                         (const uint8_t**)(&payload),
                         in_sample_counts);

    if (result < 0) {
        QNN_LOG_WARN("audio sample convert failure");
        free(out_audio_data);
        out_audio_data = NULL;
    }
    QNN_LOG_DEBUG("audio sample convert's result: %d\n", result);

    failure:
    if (NULL != swr_ctx) {
        swr_free(&swr_ctx);
        swr_ctx = NULL;
    }

    return out_audio_data;
}


static int read_data_from_file(const char * sz_file_name, uint8_t ** pp_data, size_t * p_datalen) {
    int result          = 0;
    FILE* fp            = NULL;
    uint8_t * p_data    = NULL;
    uint32_t datalen    = 0;

    if ((NULL == sz_file_name) || (NULL == pp_data) || (NULL == p_datalen)) {
        result = -1;
    }

    if (0 == result) {
        QNN_LOG_DEBUG("open file: %s", sz_file_name);
        fp = fopen(sz_file_name, "rb");
        if (NULL == fp) {
            result = -errno;
            LOGD("open file %s failed(reason:%s)", sz_file_name, strerror(errno));
        }
    }

    if (0 == result) {
        fseek(fp, 0, SEEK_END);
        datalen = (uint32_t) ftell(fp);
        QNN_LOG_DEBUG("file size %d\n", datalen);
        if (0 == datalen) {
            QNN_LOG_WARN("pls check why size is zero of file %s\n", sz_file_name);
            result = -2;
        }
    }

    if (0 == result) {
        p_data = (uint8_t *) malloc(datalen);
        if (NULL == p_data) {
            QNN_LOG_WARN("malloc memory failure, pls check why?");
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


static int whisper_asr_init(const char * sz_model_path, int n_threads, int n_asrmode, int n_backend) {
     QNN_LOG_INFO("enter whisper_asr_init\n");
     int result         = 0;

     struct whisper_full_params params;
     struct whisper_context_params c_params = whisper_context_default_params();

     if ((NULL == sz_model_path) || (n_asrmode > 3)) {
         QNN_LOG_WARN("invalid param\n");
         return 1;
     }


     //dynamic ISA dectect by RUAPU, prepare for SIMD optimization on Android device. but not used currently
     //not used since v1.3.8 because Tencent's NCNN inference framework was used in this project and ruapu.h already exist in libncnn.a
#if 0
     ruapu_init();
     const char* const* supported = ruapu_rua();
     while (*supported) {
         QNN_LOG_DEBUG("%s\n", *supported);
         supported++;
     }
#endif

     QNN_LOG_INFO("model path:%s\n", sz_model_path);
     QNN_LOG_INFO("thread counts:%d\n", n_threads);
     QNN_LOG_INFO("asr mode:%d\n", n_asrmode);
     QNN_LOG_INFO("backend type:%d\n", n_backend);

     //kantv_asr_callback pfn_asr_callback = whisper_asr_callback;
     //kantv_asr_set_callback(pfn_asr_callback);

     //kantv_inference_callback  pfn_inference_callback = whisper_asr_audio_to_text;
     //kantv_inference_set_callback(pfn_inference_callback);

     if (NULL != p_asr_ctx) {
         QNN_LOG_WARN("asr instance already initialized\n");
         return 2;
     }

     if (NULL == p_asr_ctx) {
         p_asr_ctx = (whisper_asr_context *) malloc(sizeof(whisper_asr_context));
         if (NULL == p_asr_ctx) {
             QNN_LOG_WARN("initialize failure\n");
             return 3;
         }
     }
     memset(p_asr_ctx->sz_model_path, 0, MAX_PATH_LEN);
     memcpy(p_asr_ctx->sz_model_path, sz_model_path, strlen(sz_model_path));
     p_asr_ctx->n_backend = n_backend;//added on 04-17-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121

     result  = pthread_mutex_init(&p_asr_ctx->mutex, NULL);
     if (result != 0) {
         result = 4;
         QNN_LOG_WARN("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->asr_fifo = fifo_new("asr_fifo", 1200, 1024 * 8 * 20);
     if (NULL == p_asr_ctx->asr_fifo) {
         result = 5;
         QNN_LOG_WARN("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->swr_ctx = swr_alloc();      // attention memory leak
     if (NULL == p_asr_ctx->swr_ctx) {
         result  = 7;
         QNN_LOG_WARN("initialize failure\n");
         goto failure;
     }

     p_asr_ctx->n_asr_mode = n_asrmode;
     p_asr_ctx->n_threads  = n_threads;

     QNN_LOG_DEBUG("calling whisper_init_from_file");
#if 0
     p_asr_ctx->p_context = whisper_init_from_file(sz_model_path);
#else  //04-11-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
     // ref:https://github.com/ggerganov/llama.cpp/pull/6022
     // the user could specify the devices that they want to use by name. For example,
     // the user could specify to use devices cpu, sycl_igpu0 and sycl_dgpu0 to select CPU, iGPU and dGPU
     //c_params.gpu_device    = QNN_HTP;//TODO:Failed in loading stub: dlopen failed: library "libQnnHtpV66Stub.so" not found
     c_params.gpu_device  = n_backend;
     if (n_backend != QNN_BACKEND_GGML) { // QNN_BACKEND_GGML is a fake QNN backend, just used to compare performance between QNN backend and original GGML
         c_params.use_gpu = true;
     } else {
         c_params.use_gpu = false;
     }
     p_asr_ctx->p_context = whisper_init_from_file_with_params(sz_model_path, c_params);
#endif
     if (nullptr == p_asr_ctx->p_context) {
         result = 8;
         QNN_LOG_WARN("whispercpp initialize failure\n");
         goto failure;
     }
     QNN_LOG_DEBUG("after calling whisper_init_from_file");

     p_asr_ctx->p_params = (struct whisper_full_params *)malloc(sizeof(struct whisper_full_params));
     if (NULL == p_asr_ctx->p_params) {
         result = 9;
         QNN_LOG_WARN("initialize failure\n");
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

     QNN_LOG_INFO("leave whisper_asr_init\n");

     return result;

failure:

     if (nullptr != p_asr_ctx->p_context) {
         whisper_free(p_asr_ctx->p_context);
     }

     if (NULL != p_asr_ctx->swr_ctx) {
         swr_free(&p_asr_ctx->swr_ctx);
     }

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


static void whisper_asr_finalize() {
    QNN_LOG_INFO("enter whisper_asr_finalize\n");

    if (NULL == p_asr_ctx) {
        QNN_LOG_WARN("whisper.cpp not initialized or already finalized\n");
        return;
    }
    whisper_free(p_asr_ctx->p_context);

    swr_free(&p_asr_ctx->swr_ctx);

    p_asr_ctx->asr_fifo->destroy(p_asr_ctx->asr_fifo);

    free(p_asr_ctx->p_params);

    pthread_mutex_destroy(&p_asr_ctx->mutex);

    free(p_asr_ctx);
    p_asr_ctx = NULL;

    QNN_LOG_INFO("leave whisper_asr_finalize\n");
}


static int qnn_test_whispercpp(const char * sz_model_path, const char * sz_audio_path, int num_threads, int n_backend_type) {
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
        QNN_LOG_WARN("pls check params");
        return NULL;
    }

    QNN_LOG_INFO("mode path:   %s\n", sz_model_path);
    QNN_LOG_INFO("audio file:  %s\n", sz_audio_path);
    QNN_LOG_INFO("num threads: %d\n", num_threads);

    if (0 != access(sz_model_path, F_OK)) {
        QNN_LOG_WARN("pls check whether the ggml model file %s is exist", sz_model_path);
        return NULL;
    }

    if (0 != access(sz_audio_path, F_OK)) {
        QNN_LOG_WARN("pls check whether the ggml sample file %s is exist", sz_audio_path);
        return NULL;
    }

    asr_result = "";

    whisper_asr_init(sz_model_path, num_threads, 0, n_backend_type);

    result = read_data_from_file(sz_audio_path, &audio_data, &audio_size);
    if (0 != result) {
        QNN_LOG_WARN("read data from file %s failure,pls check why?\n", sz_audio_path);
        result = -2;
        goto failure;
    }
    QNN_LOG_DEBUG("audio size %d\n", audio_size);
    float_audio_data = convert_to_float(audio_data, audio_size, &float_sample_counts);
    if (NULL == float_audio_data) {
        QNN_LOG_WARN("convert audio sample failure,pls check why?\n");
        result = -3;
        goto failure;
    }
    QNN_LOG_DEBUG("float_sample_counts %d\n", float_sample_counts);

    if (0) { //2024-03-15,16:28, validate whether whisper_asr_audio_to_text can works well as expected during PoC stage
        text = whisper_asr_audio_to_text(float_audio_data, float_sample_counts);
        if (NULL != text) {
            QNN_LOG_DEBUG("asr reulst:\n%s\n", text);
            asr_result = text;
#ifdef TARGET_ANDROID
            kantv_asr_notify_benchmark(asr_result);
#endif
        } else {
            QNN_LOG_DEBUG("whisper_asr_audio_to_text failed\n");
            result = -1;
            goto failure;
        }
    } else {
        //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
        whisper_free(p_asr_ctx->p_context);
        struct whisper_context_params wcp = whisper_context_default_params();
        QNN_LOG_DEBUG("backend %d", n_backend_type);
        wcp.gpu_device  = n_backend_type;//added on 04-17-2024, for PoC:Add Qualcomm mobile SoC native backend for GGML, https://github.com/zhouwg/kantv/issues/121
        if (n_backend_type != QNN_BACKEND_GGML) { // QNN_BACKEND_GGML is a fake QNN backend, just used to compare performance between QNN backend and original GGML
            wcp.use_gpu = true;
        } else {
            wcp.use_gpu = false;
        }
        context = whisper_init_from_file_with_params(sz_model_path, wcp);
        //PoC-S53: fix stability issue during toggle between different backend(QNN CPU/GPU/DSP backend, ggml...) in ggml-qnn.cpp(4th milestone)
        p_asr_ctx->p_context = context;
        if (nullptr == context) {
            QNN_LOG_WARN("whisper_init_from_file_with_params failure, pls check why\n");
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

        QNN_LOG_INFO("calling whisper_full\n");
        begin_time = ggml_time_ms();
        result = whisper_full(context, whisper_params, float_audio_data, float_sample_counts);
        if (0 != result) {
            QNN_LOG_WARN("inference failure, pls check why?\n");
            result = -4;
            goto failure;
        }
        QNN_LOG_INFO("whispercpp inference successfully\n");
        num_segments = whisper_full_n_segments(context);
        QNN_LOG_DEBUG("num_segments:%d\n", num_segments);
        for (index = 0; index < num_segments; index++) {
            text = whisper_full_get_segment_text(context, index);
            if (NULL == text) {
                QNN_LOG_WARN("whisper_full_get_segment_text failure, pls check why\n");
                result = -5;
                goto failure;
            }
            t0_segment = whisper_full_get_segment_t0(context, index);
            t1_segment = whisper_full_get_segment_t1(context, index);

            std::string cur_line = "";
            QNN_LOG_INFO("text[%d]:[%s --> %s] %s\n",
                  index,
                  to_timestamp(t0_segment).c_str(),
                  to_timestamp(t1_segment).c_str(),
                  text);
            cur_line +=
                    "[ " + to_timestamp(t0_segment) + " ---> " + to_timestamp(t1_segment) + "  ]" +
                    std::string(text);
            asr_result += cur_line + "\n";
            QNN_LOG_DEBUG("asr result:\n%s\n", cur_line.c_str());
        }
        end_time = ggml_time_ms();
        QNN_LOG_INFO("inference cost %d ms\n", end_time - begin_time);
        QNN_LOG_INFO("after calling whisper_full\n");
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

    whisper_asr_finalize();

    if (0 == result) {
        if (num_segments != 0) {
            QNN_LOG_DEBUG("whisper ASR result:\n%s", asr_result.c_str());
            return 0;
        } else {
            asr_result = "pls check why whisper.cpp inference failure\n";
            QNN_LOG_DEBUG("%s", asr_result.c_str());
            return 1;
        }
    } else
        return 2;
}


static void show_usage() {
    printf(" " \
        "\nUsage: test_qnn_ops [options]\n" \
        "\n" \
        "Options:\n" \
        " -t 0(simple UT) 1(automation UT) 2(whisper)\n" \
        " -o ADD / MUL / MULMAT\n" \
        " -b 0(QNN_CPU) 1(QNN_GPU) 2(QNN_NPU) 3(ggml)\n" \
        " ?/h print usage infomation\n\n"
    );
}


int main(int argc, char * argv[]) {
    int num_threads             = 4;
    int n_test_type             = TEST_SIMPLE_UT;
    int n_backend_type          = QNN_BACKEND_CPU;
    int n_ggml_op_type          = GGML_OP_ADD;

    for (int i = 1; i < argc; i++) {
        if (0 == strcmp(argv[i], "-t")) { // test type
            if (i + 1 < argc) {
                int type = atoi(argv[i + 1]);
                if (type < UT_COUNTS)
                    n_test_type = type;
                i++;
            }
        } else if (0 == strcmp(argv[i], "-o")) { //GGML OPs
            if (i + 1 < argc) {
                if (0 == memcmp(argv[i + 1], "ADD", 3)) {
                    n_ggml_op_type = GGML_OP_ADD;
                } else if (0 == memcmp(argv[i + 1], "MULMAT", 6)) {
                    n_ggml_op_type = GGML_OP_MUL_MAT;
                } else if (0 == memcmp(argv[i + 1], "MUL", 3)) {
                    n_ggml_op_type = GGML_OP_MUL;
                } else {
                    show_usage();
                    return 1;
                }
                i++;
            }
        } else if (0 == strcmp(argv[i], "-b")) { //QNN backend
            if (i + 1 < argc) {
                int backend = atoi(argv[i + 1]);
                if (backend <= QNN_BACKEND_GGML)
                    n_backend_type     = backend;
                else {
                    show_usage();
                    return 1;
                }
                i++;
            }
        } else {
            show_usage();
            return 1;
        }
    }

    QNN_LOG_DEBUG("type %d, backend %d, op %d\n", n_test_type, n_backend_type, n_ggml_op_type);
    switch (n_test_type) {
        case TEST_SIMPLE_UT:
            qnn_op_ut(num_threads, n_backend_type, n_ggml_op_type);
            break;

        case TEST_AUTOMATION_UT:
            qnn_op_ut_automation(num_threads, n_backend_type, n_ggml_op_type);
            break;

        case TEST_WHISPERCPP:
            qnn_test_whispercpp("/sdcard/kantv/models/ggml-tiny.en-q8_0.bin",
                                "/sdcard/kantv/jfk.wav",
                                num_threads,
                                n_backend_type);
            break;

        default:
            break;
    }

    QNN_LOG_DEBUG("exit main\n");
    //FIXME:why report "Bus error" here after this Android command line program executed on Android phone?
    return 0;
}
