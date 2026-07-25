
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

// ggml_jni_bench_memcpy() removed

// ggml_jni_bench_memcpy() and ggml_jni_bench_mulmat() removed

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
        case HEXAGON_BACKEND_CDSP:
            return "HEXAGON_BACKEND_CDSP";
        case HEXAGON_BACKEND_GGML:
            return "ggml"; //"fake" hexagon backend, used for compare performance between hexagon backend and the default ggml backend
        default:
            return "unknown";
    }
}

void set_hexagon_cfg(int new_hexagon_backend, int new_hwaccel_approach) {

}
#else
// upstream ggml-hexagon.h no longer declares ggml_backend_hexagon_get_devname;
// provide a local C-linkage implementation so JNI C code can link with GGML_USE_HEXAGON
extern "C" const char * ggml_backend_hexagon_get_devname(size_t dev_num) {
    switch (dev_num) {
        case HEXAGON_BACKEND_CDSP:    return "HEXAGON_BACKEND_CDSP";
        case HEXAGON_BACKEND_GGML:    return "ggml";
        default:                      return "unknown";
    }
}

void set_hexagon_cfg(int new_hexagon_backend, int new_hwaccel_approach) {

}
#endif

/**
 * helper function to perform normal llama inference(text-to-text) in native layer
 * @param sz_model_path
 * @param sz_user_data
 * @param llm_type
 * @param n_backend_type   HEXAGON_BACKEND_CDSP: offload all layers to DSP (-ngl 99)
 *                         HEXAGON_BACKEND_GGML: CPU only (-ngl 0)
 * @return
 * Backend type selects inference parameters at runtime. The actual backend
 * implementation is decided at build time (GGML_USE_HEXAGON), but -ngl controls
 * whether layers are offloaded to the DSP (CDSP) or run on CPU (GGML).
 */
int llama_inference(const char * sz_model_path, const char * sz_user_data, int llm_type,
                    int n_backend_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);
    LOGGD("backend type:%d\n", n_backend_type);

    if (nullptr == sz_model_path || nullptr == sz_user_data) {
        LOGGD("pls check params\n");
        return 1;
    }
    //this is a lazy/dirty method for merge latest source codes of upstream llama.cpp on Android port
    //easily and quickly,so we can do everything in native C/C++ layer rather than write a complicated Java wrapper
    //attention: std::to_string returns a temporary std::string, must keep it alive in a local variable
    //otherwise .c_str() becomes a dangling pointer after the initializer ends
    std::string threads_str = "6";

    // Build argv based on backend type:
    //   CDSP: offload all layers to DSP with flash attention + DSP-optimized params
    //   GGML: CPU only, no offload
    std::vector<std::string> args_storage;
    std::vector<const char *> argv_vec;
    argv_vec.push_back("llama-inference-main");
    argv_vec.push_back("-no-cnv");
    argv_vec.push_back("-m");
    argv_vec.push_back(sz_model_path);
    argv_vec.push_back("-p");
    argv_vec.push_back(sz_user_data);
    argv_vec.push_back("-t");
    args_storage.push_back(threads_str);
    argv_vec.push_back(args_storage.back().c_str());

    if (n_backend_type == HEXAGON_BACKEND_CDSP) {
        // DSP offload: large context, all layers on DSP, flash attention, poll mode
        argv_vec.push_back("-c");
        argv_vec.push_back("8192");
        argv_vec.push_back("-ngl");
        argv_vec.push_back("99");
        argv_vec.push_back("-fa");
        argv_vec.push_back("on");
        argv_vec.push_back("--ubatch-size");
        argv_vec.push_back("64");
        argv_vec.push_back("--poll");
        argv_vec.push_back("1000");
        argv_vec.push_back("--no-mmap");
    } else {
        // CPU only: small context, no offload
        argv_vec.push_back("-c");
        argv_vec.push_back("2048");
        argv_vec.push_back("-ngl");
        argv_vec.push_back("0");
    }

    int argc = (int)argv_vec.size();
    llm_init_running_state();
    ret = llama_inference_main(argc, const_cast<char **>(argv_vec.data()), n_backend_type);
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
 * @param n_backend_type   HEXAGON_BACKEND_CDSP: offload all layers to DSP (-ngl 99)
 *                         HEXAGON_BACKEND_GGML: CPU only (-ngl 0)
 * @return
 * Backend type selects inference parameters at runtime. The actual backend
 * implementation is decided at build time (GGML_USE_HEXAGON), but -ngl controls
 * whether layers are offloaded to the DSP (CDSP) or run on CPU (GGML).
 */
int mtmd_inference(const char * sz_model_path, const char * sz_mmproj_model_path, const char * sz_media_path,
                   const char * sz_user_data, int llm_type, int n_backend_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("mmproj path:%s\n", sz_mmproj_model_path);
    LOGGD("media path:%s\n", sz_media_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);
    LOGGD("backend type:%d\n", n_backend_type);

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
    //attention: std::to_string returns a temporary std::string, must keep it alive in a local variable
    std::string threads_str = "6";
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

    // Build argv based on backend type
    std::vector<std::string> args_storage;
    std::vector<const char *> argv_vec;
    argv_vec.push_back("mtmd-inference-main");
    argv_vec.push_back("-m");
    argv_vec.push_back(sz_model_path);
    argv_vec.push_back("--mmproj");
    argv_vec.push_back(sz_mmproj_model_path);
    argv_vec.push_back(type);
    argv_vec.push_back(sz_media_path);
    argv_vec.push_back("-p");
    argv_vec.push_back(sz_user_data);
    argv_vec.push_back("-t");
    args_storage.push_back(threads_str);
    argv_vec.push_back(args_storage.back().c_str());

    if (n_backend_type == HEXAGON_BACKEND_CDSP) {
        argv_vec.push_back("-ngl");
        argv_vec.push_back("99");
        argv_vec.push_back("-fa");
        argv_vec.push_back("on");
        argv_vec.push_back("--ubatch-size");
        argv_vec.push_back("64");
        argv_vec.push_back("--poll");
        argv_vec.push_back("1000");
        argv_vec.push_back("--no-mmap");
    } else {
        argv_vec.push_back("-ngl");
        argv_vec.push_back("0");
    }

    int argc = (int)argv_vec.size();
    llm_init_running_state();
    ret = mtmd_inference_main(argc, const_cast<char **>(argv_vec.data()), n_backend_type);
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
 * @return
 * backend is decided at build time (GGML_USE_HEXAGON), no runtime backend param.
 */
int sd_inference(const char *sz_model_path, const char *sz_aux_model_path, const char *sz_user_data, int llm_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("aux path:%s\n", sz_aux_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);

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
                          "-t", "6"
    };
    sd_init_running_state();
    //ret = sd_inference_main(argc, argv, HEXAGON_BACKEND_GGML);
    ret = 0;
    sd_reset_running_state();
    LOGGD("ret %d", ret);
    return ret;
}
