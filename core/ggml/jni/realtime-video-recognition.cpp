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

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"

#include "kantv-asr.h"
#include "ggml-jni.h"

#include "whisper.h"

#include "llama.h"

#include "llamacpp/ggml/include/ggml-hexagon.h"

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <android/bitmap.h>
#include <android/log.h>

//ncnn
#include "platform.h"
#include "benchmark.h"
#include "net.h"
#include "gpu.h"

//opencv-android
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "ndkcamera.h"

#if __ARM_NEON
#include <arm_neon.h>
#endif // __ARM_NEON

#include "arg.h"
#include "chat.h"
#include "common.h"
#include "json-schema-to-grammar.h"
#include "llama.h"
#include "sampling.h"
#include "speculative.h"
#include "mtmd.h"


class MyNdkCamera;

static ncnn::Mutex      g_ncnn_lock;
static MyNdkCamera *    g_camera        = nullptr;
static long long        g_bmp_idx       = 0;
static char             g_bmp_filename[JNI_TMP_LEN];

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

static int draw_unsupported(cv::Mat &rgb) {
    const char text[] = "unsupported";

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

    int y = (rgb.rows - label_size.height) / 2;
    int x = (rgb.cols - label_size.width) / 2;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0));

    return 0;
}

static int draw_fps(cv::Mat & rgb) {
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f) {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--) {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f) {
            return 0;
        }

        for (int i = 0; i < 10; i++) {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    snprintf(text, 32,"FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}


//ref:https://github.com/ggml-org/llama.cpp/blob/master/tools/server/utils.hpp#L1300-L1309
// Computes FNV-1a hash of the data
static std::string fnv_hash(const uint8_t * data, size_t len) {
    const uint64_t fnv_prime = 0x100000001b3ULL;
    uint64_t hash = 0xcbf29ce484222325ULL;

    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= fnv_prime;
    }
    return std::to_string(hash);
}

/**
 * @brief  MTMD inference by MTMD API and libllama API
 * @param  rgb  rgb data in OpenCV::Mat
 */
static void mtmd_inference(cv::Mat & rgb) {
    common_params params;
    common_init_result llama_init;

    llama_model * model         = nullptr;
    llama_context * lctx        = nullptr;
    const llama_vocab * vocab   = nullptr;

    mtmd_context * mctx         = nullptr;
    mtmd_context_params mparams;
    mtmd::bitmaps bitmaps;
    int llm_inference_interrupted = 0;

    const char * tmp = nullptr;

    common_chat_templates_ptr chat_templates;

    llama_batch batch{};
    int n_batch;
    bool has_eos_token = false;
    llama_pos new_n_past;
    int32_t n_ctx = 0;
    int32_t n_past = 0;
    int32_t n_predict = -1;

    llama_tokens generated_tokens;

    int backend_type = HEXAGON_BACKEND_GGML; //hardcode to the default ggml backend
    int thread_counts = 4;

    std::string prompt_str = "what do you see in this image?";
    thread_counts = std::thread::hardware_concurrency();

    //step-1: common params parse
    int argc = 5;
    const char * argv[] = {"llava-inference-main",
                          "-m", "/sdcard/SmolVLM2-256M-Video-Instruct-f16.gguf",
                          "--mmproj", "/sdcard/mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf",
    };
    params.sampling.temp = 0.2; // lower temp by default for better quality
    params.cpuparams.n_threads  = thread_counts;
    if (!common_params_parse(argc, const_cast<char **>(argv), params, LLAMA_EXAMPLE_SERVER)) {
        LOGGD("common params parse failure\n");
        return;
    }
    LOGGD("enter llama_inference_main backend_type %d", backend_type);
    if (backend_type != HEXAGON_BACKEND_GGML) {
#ifdef GGML_USE_HEXAGON
        LOGGD("using hexagon backend %d", backend_type);
        params.main_gpu = backend_type;
        params.n_gpu_layers = 99;
#else
        LOGGW("hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(backend_type));
        GGML_JNI_NOTIFY("hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(backend_type));
        return 1;
#endif
    } else {
        params.main_gpu = backend_type;
    }
    common_init();

    llama_backend_init();
    llama_numa_init(params.numa);
    LOGGD("system info: n_threads = %d, n_threads_batch = %d, total_threads = %d\n",
          params.cpuparams.n_threads, params.cpuparams_batch.n_threads,
          std::thread::hardware_concurrency());
    LOGGD("\n");
    LOGGD("%s\n", common_params_get_system_info(params).c_str());
    LOGGD("\n");

    //step-2: load LLM model
    LOGGD("loading model '%s'\n", params.model.path.c_str());
    llama_init = common_init_from_params(params);
    model = llama_init.model.get();
    lctx = llama_init.context.get();
    if (model == nullptr) {
        LOGGD("failed to load model, '%s'\n", params.model.path.c_str());
        return;
    }
    vocab = llama_model_get_vocab(model);
    n_ctx = llama_n_ctx(lctx);
    llama_vocab_get_add_bos(vocab);
    has_eos_token = llama_vocab_eos(vocab) != LLAMA_TOKEN_NULL;
    batch = llama_batch_init(params.n_batch, 0, 1);
    n_batch = params.n_batch;
    struct common_sampler * smpl = common_sampler_init(model, params.sampling);
    n_predict = params.n_predict < 0 ? INT_MAX : params.n_predict;

    //step-3: load multimodal model
    std::string & mmproj_path = params.mmproj.path;
    mparams = mtmd_context_params_default();
    mparams.use_gpu = false;
    mparams.print_timings = false;
    mparams.n_threads = thread_counts;
    mparams.verbosity = GGML_LOG_LEVEL_DEBUG;
    mctx = mtmd_init_from_file(mmproj_path.c_str(), model, mparams);
    if (mctx == nullptr) {
        LOGGD("failed to load multimodal model, '%s'\n", mmproj_path.c_str());
        common_sampler_free(smpl);
        llama_backend_free();
        return;
    }
    LOGGD("loaded multimodal model, '%s'\n", mmproj_path.c_str());

    //step-4: load image from memory
    mtmd_bitmap * bitmap = mtmd_bitmap_init(rgb.cols, rgb.rows, rgb.data);
    mtmd::bitmap bmp(bitmap);
    if (!bmp.ptr) {
        LOGGD("failed to load image\n");
        GGML_JNI_NOTIFY("failed to load image\n");
        common_sampler_free(smpl);
        mtmd_free(mctx);
        llama_backend_free();
        return;
    }
    // calculate bitmap hash (for KV caching)
    std::string hash = fnv_hash(bmp.data(), bmp.nx() * bmp.ny() * 3);
    bmp.set_id(hash.c_str());
    bitmaps.entries.push_back(std::move(bmp));

    //step-4: create embedding tokens from image & prompt
    chat_templates = common_chat_templates_init(model, params.chat_template);
    try {
        common_chat_format_example(chat_templates.get(), params.use_jinja);
    } catch (const std::exception &e) {
        LOGGD("%s: Chat template parsing error: %s\n", __func__, e.what());
        LOGGD("%s: The chat template that comes with this model is not yet supported, falling back to chatml. This may cause the model to output suboptimal responses\n",
              __func__);
        chat_templates = common_chat_templates_init(model, "chatml");
    }
    LOGGD("%s: chat template example:\n%s\n", __func__, common_chat_format_example(chat_templates.get(), params.use_jinja).c_str());
    params.prompt = prompt_str;
    if (params.prompt.find("<__image__>") == std::string::npos) {
        params.prompt += " <__image__>";
    }
    common_chat_msg msg;
    msg.role = "user";
    msg.content = params.prompt;
    common_chat_templates_inputs tmpl_inputs;
    tmpl_inputs.messages = {msg};
    tmpl_inputs.add_generation_prompt = true;
    tmpl_inputs.use_jinja = false; // jinja is buggy here
    auto formatted_chat = common_chat_templates_apply(chat_templates.get(), tmpl_inputs);
    LOGGD("formatted_chat.prompt: %s\n", formatted_chat.prompt.c_str());
    mtmd_input_text inp_txt = {
            formatted_chat.prompt.c_str(),
            /* add_special */   true,
            /* parse_special */ true,
    };
    mtmd::input_chunks chunks(mtmd_input_chunks_init());
    auto bitmaps_c_ptr = bitmaps.c_ptr();
    int32_t tokenized = mtmd_tokenize(mctx,
                                      chunks.ptr.get(),
                                      &inp_txt,
                                      bitmaps_c_ptr.data(),
                                      bitmaps_c_ptr.size());
    if (tokenized != 0) {
        LOGGD("Failed to tokenize prompt");
        goto failure;
    }
    bitmaps.entries.clear();
    if (mtmd_helper_eval_chunks(mctx,
                                lctx, // lctx
                                chunks.ptr.get(), // chunks
                                n_past, // n_past
                                0, // seq_id
                                n_batch, // n_batch
                                true, // logits_last
                                &new_n_past)) {
        LOGGD("Unable to eval prompt\n");
        goto failure;
    }
    n_past = new_n_past;

    //step-5: LLM inference with the generated tokens
    for (int i = 0; i < n_predict; i++) {
        if (i > n_predict) {
            LOGGD("End of Text\n");
            break;
        }

        llama_token token_id = common_sampler_sample(smpl, lctx, -1);
        generated_tokens.push_back(token_id);
        common_sampler_accept(smpl, token_id, true);

        if (llama_vocab_is_eog(vocab, token_id)) {
            LOGGD("End of Text\n");
            break; // end of generation
        }

        tmp = common_token_to_piece(lctx, token_id).c_str();
#if (defined __ANDROID__) || (defined ANDROID)
        if (ggml_jni_is_valid_utf8(tmp)) {
            if (0 == inference_is_running_state()) {
                llm_inference_interrupted = 1;
                break;
            } else {
                GGML_JNI_NOTIFY(tmp);
            }
        }
#endif
        // eval the token
        common_batch_clear(batch);
        common_batch_add(batch, token_id, n_past++, {0}, true);
        if (llama_decode(lctx, batch)) {
            LOGGD("failed to decode token\n");
            goto failure;
        }
    }

    if (0 == llm_inference_interrupted) {
        llama_perf_context_print(lctx);
    }

failure:
    common_sampler_free(smpl);
    mtmd_free(mctx);
    llama_backend_free();

    return;
}

void MyNdkCamera::on_image_render(cv::Mat & rgb) const {
    g_bmp_idx++;
    //TODO: stability issue between UI layer and native layer
    if (0 == (g_bmp_idx % 100)) { // 100 / 30 ~= 3 seconds
        GGML_JNI_NOTIFY("realtime-cam-reset");
#if 0
        //snprintf(g_bmp_filename, JNI_TMP_LEN, "/sdcard/bmp-%04d.bmp", g_bmp_idx);
        snprintf(g_bmp_filename, JNI_TMP_LEN, "/sdcard/bmp-tmp.bmp");
        LOGGD("write bmp %s, width %d, height %d\n", g_bmp_filename, rgb.cols, rgb.rows);

        //this is a lazy/dirty method to implement "realtime"(not realtime at the moment) video recognition via MTMD feature in llama.cpp
        //using MTMD API directly is a better approach, can be seen in: https://github.com/ggml-org/llama.cpp/blob/master/tools/server/server.cpp#L4121
        write_bmp(g_bmp_filename, rgb.cols, rgb.rows, 24, rgb.data);
        inference_init_running_state();
        llava_inference("/sdcard/SmolVLM2-256M-Video-Instruct-f16.gguf",
                        "/sdcard/mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf",
                        g_bmp_filename, "what do you see in this image?",
                        GGML_BENCHMARK_LLM, 8, HEXAGON_BACKEND_GGML, HWACCEL_CDSP);
        inference_reset_running_state();
#else
        //better approach: using MTMD API directly to avoid write bmp data to storage
        inference_init_running_state();
        mtmd_inference(rgb);
        inference_reset_running_state();
#endif
    }

    draw_fps(rgb);
}


extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM * vm, void * reserved) {
    LOGGD("JNI_OnLoad");

    ncnn::create_gpu_instance();

    g_camera = new MyNdkCamera;

    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM * vm, void * reserved) {
    LOGGD("JNI_OnUnload");
    ncnn::destroy_gpu_instance();
    delete g_camera;
    g_camera = NULL;
}

JNIEXPORT jboolean JNICALL
Java_kantvai_ai_ggmljava_openCamera(JNIEnv * env, jclass clazz, jint facing) {
    if (facing < 0 || facing > 1)
        return JNI_FALSE;

    LOGGD("openCamera %d", facing);

    g_camera->open((int) facing);

    return JNI_TRUE;
}

JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_closeCamera(JNIEnv * env, jclass clazz) {
    LOGGD("closeCamera");

    g_camera->close();
}

JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_setOutputWindow(JNIEnv * env, jclass clazz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);

    LOGGD("setOutputWindow %p", win);

    g_camera->set_window(win);
}

} //end extern "C" {