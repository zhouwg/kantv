
#include "ggml-jni-context.h"

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-cpu.h"
#include "llamacpp/ggml/include/ggml-hexagon.h"

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
}

void ggml_jni_context::init() {
    if (initialized) {
        LOGGD("already initialize");
        return;
    }
    llm_temperature = 0.8;
    llm_top_p       = 0.9;
    initialized     = true;
}

void ggml_jni_context::finalize() {
    LOGGD("finalize");
    if (initialized) {
        initialized = false;
    } else {
        LOGGD("already finalize");
    }
}

ggml_jni_context::ggml_jni_context():
    llm_inference_is_running(0),
    realtimemtmd_inference_is_running(0),
    initialized(false) {
    init();
}

ggml_jni_context::~ggml_jni_context() {
    finalize();
}

static class ggml_jni_context & g_jni_ctx = ggml_jni_context::get_instance();

/**
*helper functions to check whether normal LLM(LLM or normal MTMD) inference is running
*/
void llm_init_running_state() {
    LOGGD("here");
    g_jni_ctx.llm_init_running_state();
}

void llm_reset_running_state() {
    LOGGD("here");
    g_jni_ctx.llm_reset_running_state();
}

int llm_is_running_state() {
    return g_jni_ctx.llm_is_running_state();
}

/**
*helper functions to check whether realtime MTMD inference is running
*/
void realtimemtmd_init_running_state() {
    LOGGD("here");
    g_jni_ctx.realtimemtmd_init_running_state();
}

void realtimemtmd_reset_running_state() {
    LOGGD("here");
    g_jni_ctx.realtimemtmd_reset_running_state();
}

int realtimemtmd_is_running_state() {
    static long realtimemtmd_counter = 0;
    realtimemtmd_counter++;
    if (0 == realtimemtmd_counter % 100) {
        LOGGD("here");
    }
    return g_jni_ctx.realtimemtmd_is_running_state();
}

/**
*helper functions to check whether stablediffusion inference is running
*/
void sd_init_running_state() {
    LOGGD("here");
    g_jni_ctx.sd_init_running_state();
}

void sd_reset_running_state() {
    LOGGD("here");
    g_jni_ctx.sd_reset_running_state();
}

int sd_is_running_state() {
    return g_jni_ctx.sd_is_running_state();
}

//helper functions for adjust LLM inference parameters
void llm_set_temperature(float temperature) {
    g_jni_ctx.set_temperature(temperature);
}

float llm_get_temperature() {
    return g_jni_ctx.get_temperature();
}

void llm_set_top_p(float value) {
    g_jni_ctx.set_top_p(value);
}

float llm_get_top_p() {
    return g_jni_ctx.get_top_p();
}

//ref:https://github.com/ggml-org/whisper.cpp/blob/master/src/whisper.cpp#L8046
const char * ggml_jni_bench_memcpy(int n_threads) {
    std::string s;
    s = "";
    char strbuf[256];

    GGML_JNI_NOTIFY("calling ggml_time_init\n\n");
    ggml_time_init();

    size_t n = 20;
    size_t arr = n_threads > 0 ? 1024llu : n_threads; // trick to avoid compiler optimizations

    // 1GB array
    const size_t size = arr * 1e6;

    double sum = 0.0;

    // heat-up
    {
        char *src = (char *) malloc(size);
        char *dst = (char *) malloc(size);

        for (size_t i = 0; i < size; i++) src[i] = i;

        memcpy(dst, src, size); // heat-up

        double tsum = 0.0;

        for (size_t i = 0; i < n; i++) {
            const int64_t t0 = ggml_time_us();

            memcpy(dst, src, size);

            const int64_t t1 = ggml_time_us();

            tsum += (t1 - t0) * 1e-6;

            src[rand() % size] = rand() % 256;
        }

        snprintf(strbuf, sizeof(strbuf), "memcpy: %7.2f GB/s (heat-up)\n\n",
                 (double) (n * size) / (tsum * 1e9));
        GGML_JNI_NOTIFY(strbuf);
        s += strbuf;


        // needed to prevent the compiler from optimizing the memcpy away
        {
            for (size_t i = 0; i < size; i++) sum += dst[i];
        }

        free(src);
        free(dst);
    }

    // single-thread
    {
        char *src = (char *) malloc(size);
        char *dst = (char *) malloc(size);

        for (size_t i = 0; i < size; i++) src[i] = i;

        memcpy(dst, src, size); // heat-up

        double tsum = 0.0;

        for (size_t i = 0; i < n; i++) {
            const int64_t t0 = ggml_time_us();

            memcpy(dst, src, size);

            const int64_t t1 = ggml_time_us();

            tsum += (t1 - t0) * 1e-6;

            src[rand() % size] = rand() % 256;
        }

        snprintf(strbuf, sizeof(strbuf), "memcpy: %7.2f GB/s ( 1 thread)\n\n",
                 (double) (n * size) / (tsum * 1e9));
        GGML_JNI_NOTIFY(strbuf);
        s += strbuf;


        // needed to prevent the compiler from optimizing the memcpy away
        {
            for (size_t i = 0; i < size; i++) sum += dst[i];
        }

        free(src);
        free(dst);
    }

    // multi-thread

    for (int32_t k = 1; k <= n_threads; k++) {
        char *src = (char *) malloc(size);
        char *dst = (char *) malloc(size);

        for (size_t i = 0; i < size; i++) src[i] = i;

        memcpy(dst, src, size); // heat-up

        double tsum = 0.0;

        auto helper = [&](int th) {
            const int64_t i0 = (th + 0) * size / k;
            const int64_t i1 = (th + 1) * size / k;

            for (size_t i = 0; i < n; i++) {
                memcpy(dst + i0, src + i0, i1 - i0);

                src[i0 + rand() % (i1 - i0)] = rand() % 256;
            };
        };

        const int64_t t0 = ggml_time_us();

        std::vector<std::thread> threads(k - 1);
        for (int32_t th = 0; th < k - 1; ++th) {
            threads[th] = std::thread(helper, th);
        }

        helper(k - 1);

        for (int32_t th = 0; th < k - 1; ++th) {
            threads[th].join();
        }

        const int64_t t1 = ggml_time_us();

        tsum += (t1 - t0) * 1e-6;

        snprintf(strbuf, sizeof(strbuf), "memcpy: %7.2f GB/s (%2d thread)\n\n",
                 (double) (n * size) / (tsum * 1e9), k);
        GGML_JNI_NOTIFY(strbuf);
        s += strbuf;


        // needed to prevent the compiler from optimizing the memcpy away
        {
            for (size_t i = 0; i < size; i++) sum += dst[i];
        }

        free(src);
        free(dst);
    }

    snprintf(strbuf, sizeof(strbuf), "sum:    %f\n\n", sum);
    GGML_JNI_NOTIFY(strbuf);
    s += strbuf;

    return s.c_str();
}

const char * ggml_jni_bench_mulmat(int n_threads, int n_backend) {
    GGML_JNI_NOTIFY("this function removed/deprecated since 05/30/2025");
    return "deprecated";
}

// ref:https://github.com/ggerganov/llama.cpp/pull/5935/
bool ggml_jni_is_valid_utf8(const char *string) {
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

#ifndef GGML_USE_HEXAGON //make compiler happy when disable GGML_USE_HEXAGON manually
const char * ggml_backend_hexagon_get_devname(size_t dev_num) {
    switch (dev_num) {
        case HEXAGON_BACKEND_QNNCPU:
            return "HEXAGON_BACKEND_QNN_CPU";
        case HEXAGON_BACKEND_QNNGPU:
            return "HEXAGON_BACKEND_QNN_GPU";
        case HEXAGON_BACKEND_QNNNPU:
            return "HEXAGON_BACKEND_QNN_NPU";
        case HEXAGON_BACKEND_CDSP:
            return "HEXAGON_BACKEND_CDSP";
        case HEXAGON_BACKEND_GGML:
            return "ggml"; //"fake" hexagon backend, used for compare performance between hexagon backend and the default ggml backend
        default:
            return "unknown";
    }
}
#endif

/**
 * helper function to perform normal llama inference(text-to-text) in native layer
 * @param sz_model_path
 * @param sz_user_data
 * @param llm_type
 * @param n_threads
 * @param n_backend_type
 * @param n_hwaccel_type
 * @return
 */
int llama_inference(const char * sz_model_path, const char * sz_user_data, int llm_type,
                    int n_threads, int n_backend_type, int n_hwaccel_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);
    LOGGD("num_threads:%d\n", n_threads);
    LOGGD("backend type:%d\n", n_backend_type);
    LOGGD("hwaccel type:%d\n", n_hwaccel_type);

    if (nullptr == sz_model_path || nullptr == sz_user_data) {
        LOGGD("pls check params\n");
        return 1;
    }
    //this is a lazy/dirty method for merge latest source codes of upstream llama.cpp on Android port
    //easily and quickly,so we can do everything in native C/C++ layer rather than write a complicated Java wrapper
    int argc = 8;
    const char *argv[] = {"llama-inference-main",
                          "-no-cnv",
                          "-m", sz_model_path,
                          "-p", sz_user_data,
                          "-t", std::to_string(n_threads).c_str()
    };
    llm_init_running_state();
    ret = llama_inference_main(argc, const_cast<char **>(argv), n_backend_type);
    llm_reset_running_state();

    return ret;
}

/**
 * helper function to perform MTMD(multimodal) inference in native layer, this is not realtime-MTMD inference
 * @param sz_model_path
 * @param sz_mmproj_model_path
 * @param sz_media_path
 * @param sz_user_data
 * @param llm_type
 * @param n_threads
 * @param n_backend_type
 * @param n_hwaccel_type
 * @return
 */
int mtmd_inference(const char * sz_model_path, const char * sz_mmproj_model_path, const char * sz_media_path,
                   const char * sz_user_data, int llm_type, int n_threads, int n_backend_type, int n_hwaccel_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("mmproj path:%s\n", sz_mmproj_model_path);
    LOGGD("media path:%s\n", sz_media_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);
    LOGGD("num_threads:%d\n", n_threads);
    LOGGD("backend type:%d\n", n_backend_type);
    LOGGD("hwaccel type:%d\n", n_hwaccel_type);

    if (nullptr == sz_model_path || nullptr == sz_user_data) {
        LOGGD("pls check params\n");
        return 1;
    }
    if (nullptr == sz_mmproj_model_path || nullptr == sz_media_path) {
        LOGGD("pls check params\n");
        return 1;
    }

    if (0 != access(sz_model_path, F_OK)) {
        return 1;
    }

    if (0 != access(sz_mmproj_model_path, F_OK)) {
        return 1;
    }

    if (0 != access(sz_media_path, F_OK)) {
        return 1;
    }

    //this is a lazy/dirty method for merge latest source codes of upstream llama.cpp on Android port
    //easily and quickly,so we can do everything in native C/C++ layer rather than write a complicated Java wrapper
    int argc = 11;
    const char * type = "--image";
    switch (llm_type) {
        case 1:
            break;
        case 2:
            type = "--audio";
            break;
        default:
            break;
    }
    const char * argv[] = {"mtmd-inference-main",
                           "-m", sz_model_path,
                           "--mmproj", sz_mmproj_model_path,
                           type, sz_media_path,
                           "-p", sz_user_data,
                           "-t", std::to_string(n_threads).c_str()
    };
    llm_init_running_state();
    ret = mtmd_inference_main(argc, const_cast<char **>(argv), n_backend_type);
    llm_reset_running_state();

    LOGGD("mtmd_inference return %d", ret);
    return ret;
}

/**
 * helper function to perform stable-diffusion inference in native layer
 * @param sz_model_path
 * @param sz_aux_model_path
 * @param sz_user_data
 * @param llm_type
 * @param n_threads
 * @param n_backend_type
 * @param n_hwaccel_type
 * @return
 */
int sd_inference(const char *sz_model_path, const char *sz_aux_model_path, const char *sz_user_data, int llm_type,
                 int n_threads, int n_backend_type, int n_hwaccel_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("aux path:%s\n", sz_aux_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);
    LOGGD("num_threads:%d\n", n_threads);
    LOGGD("backend type:%d\n", n_backend_type);
    LOGGD("hwaccel type:%d\n", n_hwaccel_type);

    if (nullptr == sz_model_path) {
        LOGGD("pls check params\n");
        return 1;
    }
    if (nullptr == sz_user_data) {
        LOGGD("pls check params\n");
        return 2;
    }
    //this is a lazy/dirty method to integrate stable-diffusion.cpp quickly
    int argc = 11;
    const char *argv[] = {"sd-inference-main",
                          "-m", sz_model_path,
                          "-p", sz_user_data,
                          "--width", "512",
                          "--height", "512",
                          "-t", std::to_string(n_threads).c_str()
    };
    sd_init_running_state();
    ret = sd_inference_main(argc, argv, n_backend_type);
    sd_reset_running_state();
    LOGGD("ret %d", ret);
    return ret;
}
