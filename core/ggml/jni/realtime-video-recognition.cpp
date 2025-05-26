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

//ggml-jni
#include "ggml-jni.h"

//libllama
#include "llama.h"
#include "arg.h"
#include "chat.h"
#include "common.h"
#include "json-schema-to-grammar.h"
#include "llama.h"
#include "sampling.h"
#include "speculative.h"

//libmtmd
#include "mtmd.h"

//ncnn
#include "platform.h"
#include "benchmark.h"
#include "net.h"
#include "gpu.h"

//opencv-android
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "myndkcamera.h"

class multimodal_inference {
public:
    static multimodal_inference & get_instance() {
        static multimodal_inference instance;
        return instance;
    }

    void init() {
        if (!initialized) {
            LOGGD("init model");
            bool result = model_init("/sdcard/SmolVLM2-256M-Video-Instruct-f16.gguf",
                                     "/sdcard/mmproj-SmolVLM2-256M-Video-Instruct-f16.gguf");
            if (result)
                initialized = true;
        }
    }

    //TODO:for further usage: select different multimodal model in UI
    void init(const char * llm_model_name, const char * mmproj_model_name) {
        if (!initialized) {
            LOGGD("init model");
            bool result = model_init(llm_model_name, mmproj_model_name);
            if (result)
                initialized = true;
        }
    }

    void camera_init();
    void camera_finalize();
    bool camera_open(int facing);
    void camera_close();
    void camera_set_outputwindow(ANativeWindow * win);

    void finalize();
    void do_inference(cv::Mat & rgb);

private:
    multimodal_inference() {}
    multimodal_inference(const multimodal_inference &)              = delete;
    multimodal_inference(const multimodal_inference &&)             = delete;
    multimodal_inference & operator= (const multimodal_inference &) = delete;

    ~multimodal_inference() {
        LOGGD("~multimodal_inference()");
        finalize();
    }

private:
    //ref:https://github.com/ggml-org/llama.cpp/blob/master/tools/server/utils.hpp#L1300-L1309
    // Computes FNV-1a hash of the data
    std::string fnv_hash(const uint8_t * data, size_t len) {
        const uint64_t fnv_prime = 0x100000001b3ULL;
        uint64_t hash = 0xcbf29ce484222325ULL;

        for (size_t i = 0; i < len; ++i) {
            hash ^= data[i];
            hash *= fnv_prime;
        }
        return std::to_string(hash);
    }

private:
    bool model_init(const char * llm_model_name, const char * mmproj_model_name);

    void mtmd_inference(cv::Mat & rgb);
    
private:
    common_params      params;
    common_init_result llama_init;

    llama_model * model          = nullptr;
    llama_context * lctx         = nullptr;
    const llama_vocab * vocab    = nullptr;
    struct common_sampler * smpl = nullptr;
    llama_pos new_n_past;
    int llm_inference_interrupted = 0;
    llama_tokens generated_tokens;

    common_chat_templates_ptr chat_templates;
    llama_batch batch{};
    int n_batch;
    bool has_eos_token = false;

    int32_t n_ctx = 0;
    int32_t n_past = 0;
    int32_t n_predict = -1;

    mtmd_context * mctx          = nullptr;
    mtmd_context_params mparams;
    mtmd::bitmaps bitmaps;
    const char * tmp = nullptr;

    int backend_type = HEXAGON_BACKEND_GGML; //hardcode to the default ggml backend
    std::string prompt_str = "what do you see in this image?";

    int thread_counts = std::thread::hardware_concurrency();

    MyNdkCamera *    ndkcamera_instance        = nullptr;

    long long        frame_index       = 0;

    bool initialized = false;
};


bool multimodal_inference::model_init(const char * llm_model_name,
                                      const char * mmproj_model_name) {
    //step-1: common params parse
    int argc = 5;
    const char * argv[] = {"multimodal-inference-main",
                           "-m", llm_model_name,
                           "--mmproj", mmproj_model_name
    };
    params.sampling.temp        = 0.2; // lower temp by default for better quality
    params.cpuparams.n_threads  = thread_counts;
    if (!common_params_parse(argc, const_cast<char **>(argv), params, LLAMA_EXAMPLE_SERVER)) {
        LOGGD("common params parse failure\n");
        return false;
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
        return false;
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
        llama_backend_free();
        return false;
    }
    vocab = llama_model_get_vocab(model);
    n_ctx = llama_n_ctx(lctx);
    llama_vocab_get_add_bos(vocab);
    has_eos_token = llama_vocab_eos(vocab) != LLAMA_TOKEN_NULL;
    batch = llama_batch_init(params.n_batch, 0, 1);
    n_batch = params.n_batch;
    smpl = common_sampler_init(model, params.sampling);
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
        return false;
    }
    LOGGD("loaded multimodal model, '%s'\n", mmproj_path.c_str());

    //step-4: init chat template
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

    return true;
}

void multimodal_inference::camera_init() {
    LOGGD("initialize camera");
    if (nullptr == ndkcamera_instance) {
        ncnn::create_gpu_instance();
        ndkcamera_instance = new MyNdkCamera;
    } else {
        LOGGD("camera already initialized");
    }
}

bool multimodal_inference::camera_open(int facing) {
    LOGGD("open camera");
    if (facing < 0 || facing > 1) {
        LOGGD("invalid param");
        return false;
    }
    if (nullptr != ndkcamera_instance) {
        ndkcamera_instance->open(facing);
        inference_reset_running_state();
    } else {
        LOGGD("camera already opened");
    }
    return true;
}

void multimodal_inference::camera_finalize() {
    LOGGD("finalize camera");
    if (nullptr != ndkcamera_instance) {
        ncnn::destroy_gpu_instance();
        delete ndkcamera_instance;
        ndkcamera_instance = nullptr;
    } else {
        LOGGD("camera already finalize");
    }
}

void multimodal_inference::camera_close() {
    LOGGD("close camera");
    if (nullptr != ndkcamera_instance) {
        inference_reset_running_state();
        ndkcamera_instance->close();
    } else {
        LOGGD("camera already closed");
    }
}


void multimodal_inference::camera_set_outputwindow(ANativeWindow * win) {
    LOGGD("setOutputWindow %p", win);
    if (nullptr != ndkcamera_instance) {
        ndkcamera_instance->set_window(win);
    }
}

void multimodal_inference::mtmd_inference(cv::Mat & rgb) {
    llm_inference_interrupted = 0;
    //load image from memory
    mtmd_bitmap * bitmap = mtmd_bitmap_init(rgb.cols, rgb.rows, rgb.data);
    mtmd::bitmap bmp(bitmap);
    if (!bmp.ptr) {
        LOGGD("failed to load image\n");
        GGML_JNI_NOTIFY("failed to load image\n");
        return;
    }
    // calculate bitmap hash (for KV caching)
    std::string hash = fnv_hash(bmp.data(), bmp.nx() * bmp.ny() * 3);
    bmp.set_id(hash.c_str());
    bitmaps.entries.push_back(std::move(bmp));

    //create embedding tokens from image & prompt
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
        LOGGD("failed to tokenize prompt");
        return;
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
        LOGGD("unable to eval prompt\n");
        return;
    }
    n_past = new_n_past;

    if (0 == inference_is_running_state()) {
        llm_inference_interrupted = 1;
        return;
    } else {
        GGML_JNI_NOTIFY("realtime-cam-reset");
    }

    //LLM inference with the generated tokens
    for (int i = 0; i < n_predict; i++) {
        if (i > n_predict) {
            LOGGD("end of text\n");
            break;
        }

        llama_token token_id = common_sampler_sample(smpl, lctx, -1);
        generated_tokens.push_back(token_id);
        common_sampler_accept(smpl, token_id, true);

        if (llama_vocab_is_eog(vocab, token_id)) {
            LOGGD("end of text\n");
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
            return;
        }
    }

    if (0 == llm_inference_interrupted) {
        llama_perf_context_print(lctx);
    }
    LOGGD("return");
}

void multimodal_inference::finalize() {
    LOGGD("finalize");
    if (initialized) {
        if (nullptr != smpl)
            common_sampler_free(smpl);

        if (nullptr != mctx)
            mtmd_free(mctx);

        llama_backend_free();

        camera_finalize();

        initialized = false;
    } else {
        LOGGD("already finalize");
    }
}

void multimodal_inference::do_inference(cv::Mat & rgb) {
    frame_index++;

    if (0 == inference_is_running_state()) {
        return;
    }

    if (!initialized)
        return;

    if (0 != (frame_index % 100)) { // 100 / 30 ~= 3 seconds
        return;
    }

    mtmd_inference(rgb);
}

static class multimodal_inference & g_mmi_instance = multimodal_inference::get_instance();

bool jni_open_camera(int facing) {
    LOGGD("open camera");
    g_mmi_instance.init();
    g_mmi_instance.camera_init();
    bool result = g_mmi_instance.camera_open(facing);
    return result;
}

void jni_close_camera() {
    LOGGD("close camera");
    g_mmi_instance.camera_close();
}

void jni_set_outputwindow(ANativeWindow  * win) {
    g_mmi_instance.camera_set_outputwindow(win);
}

void jni_cleanup_llm_resource() {
    g_mmi_instance.finalize();
}

void multimodal_inference(cv::Mat & rgb) {
    g_mmi_instance.do_inference(rgb);
}