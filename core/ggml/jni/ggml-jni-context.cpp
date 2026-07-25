
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
 * @return
 * backend is decided at build time (GGML_USE_HEXAGON), no runtime backend param.
 * llama_inference_main still takes a backend arg for internal use; pass fixed
 * HEXAGON_BACKEND_GGML to preserve its existing behaviour.
 */
int llama_inference(const char * sz_model_path, const char * sz_user_data, int llm_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);

    if (nullptr == sz_model_path || nullptr == sz_user_data) {
        LOGGD("pls check params\n");
        return 1;
    }
    //this is a lazy/dirty method for merge latest source codes of upstream llama.cpp on Android port
    //easily and quickly,so we can do everything in native C/C++ layer rather than write a complicated Java wrapper
    //attention: std::to_string returns a temporary std::string, must keep it alive in a local variable
    //otherwise .c_str() becomes a dangling pointer after the initializer ends
    std::string threads_str = "6";
    int argc = 10;
    const char *argv[] = {"llama-inference-main",
                          "-no-cnv",
                          "-m", sz_model_path,
                          "-p", sz_user_data,
                          "-t", threads_str.c_str(),
                          "-c", "2048"
    };
    llm_init_running_state();
    ret = llama_inference_main(argc, const_cast<char **>(argv), HEXAGON_BACKEND_GGML);
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
 * @return
 * backend is decided at build time (GGML_USE_HEXAGON), no runtime backend param.
 * mtmd_inference_main still takes a backend arg for internal use; pass fixed
 * HEXAGON_BACKEND_GGML to preserve its existing behaviour.
 */
int mtmd_inference(const char * sz_model_path, const char * sz_mmproj_model_path, const char * sz_media_path,
                   const char * sz_user_data, int llm_type) {
    int ret = 0;
    LOGGD("model path:%s\n", sz_model_path);
    LOGGD("mmproj path:%s\n", sz_mmproj_model_path);
    LOGGD("media path:%s\n", sz_media_path);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("llm_type: %d\n", llm_type);

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
                           "-t", threads_str.c_str()
    };
    llm_init_running_state();
    ret = mtmd_inference_main(argc, const_cast<char **>(argv), HEXAGON_BACKEND_GGML);
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
