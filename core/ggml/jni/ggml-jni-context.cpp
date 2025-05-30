
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
    llm_top_p = 0.9;
    initialized = true;
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
    static long realtimemtmd_idx = 0;
    realtimemtmd_idx++;
    if (0 == realtimemtmd_idx % 100) {
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

//other helper functions
void llm_set_temperature(float temperature) {
    LOGGD("here");
    g_jni_ctx.set_temperature(temperature);
}

float llm_get_temperature() {
    LOGGD("here");
    return g_jni_ctx.get_temperature();
}

void llm_set_top_p(float value) {
    LOGGD("here");
    g_jni_ctx.set_top_p(value);
}

float llm_get_top_p() {
    LOGGD("here");
    return g_jni_ctx.get_top_p();
}