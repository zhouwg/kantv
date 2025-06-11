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

//ref:https://github.com/ggml-org/llama.cpp/blob/master/tools/mtmd/mtmd-cli.cpp
int mtmd_inference_main(int argc, char ** argv, int backend_type) {
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
    common_chat_msg msg;
    common_chat_templates_inputs tmpl_inputs;

    llama_batch batch{};
    int n_batch;
    bool has_eos_token = false;
    llama_pos new_n_past;
    int32_t n_ctx = 0;
    int32_t n_past = 0;
    int32_t n_predict = -1;

    llama_tokens generated_tokens;
    int thread_counts = 4;
    thread_counts = std::thread::hardware_concurrency();
    int32_t tokenized = 0;

    //step-1: common params parse
    params.sampling.temp = 0.2; // lower temp by default for better quality
    params.cpuparams.n_threads  = thread_counts;
    LOGGD("mtmd_inference_main backend_type %d", backend_type);
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
    if (!common_params_parse(argc, const_cast<char **>(argv), params, LLAMA_EXAMPLE_MTMD)) {
        LOGGD("common params parse failure\n");
        return 2;
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
        return 3;
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
        return 4;
    }
    LOGGD("loaded multimodal model, '%s'\n", mmproj_path.c_str());

    //step-4: load media(image / audio)
    for (const auto & image : params.image) {
        //mtmd_bitmap * bitmap = mtmd_helper_bitmap_init_from_file(params.image.front().c_str());
        mtmd_bitmap * bitmap = mtmd_helper_bitmap_init_from_file(image.c_str());
        mtmd::bitmap bmp(bitmap);
        if (!bmp.ptr) {
            LOGGD("failed to load media\n");
            GGML_JNI_NOTIFY("failed to load media\n");
            common_sampler_free(smpl);
            mtmd_free(mctx);
            llama_backend_free();
            return 5;
        }
        // calculate bitmap hash (for KV caching)
        std::string hash = fnv_hash(bmp.data(), bmp.nx() * bmp.ny() * 3);
        bmp.set_id(hash.c_str());
        bitmaps.entries.push_back(std::move(bmp));
    }

    if (0 == llm_is_running_state()) {
        llm_inference_interrupted = 1;
        goto failure;
    }

    //step-5: create embedding tokens from media(image or audio) & prompt
    //ref:https://github.com/ggml-org/llama.cpp/discussions/13759#discussioncomment-13294811
    if (params.prompt.find(mtmd_default_marker()) == std::string::npos) {
        for (size_t i = 0; i < params.image.size(); i++) {
            params.prompt += mtmd_default_marker();
        }
    }
    chat_templates = common_chat_templates_init(model, params.chat_template);
    try {
        common_chat_format_example(chat_templates.get(), params.use_jinja);
    } catch (const std::exception &e) {
        LOGGD("%s: Chat template parsing error: %s\n", __func__, e.what());
        LOGGD("%s: The chat template that comes with this model is not yet supported, falling back to chatml."
              "This may cause the model to output suboptimal responses\n", __func__);
        chat_templates = common_chat_templates_init(model, "chatml");
    }
    LOGGD("%s: chat template example:\n%s\n", __func__, common_chat_format_example(chat_templates.get(), params.use_jinja).c_str());
    //params.prompt = prompt_str;
    //ref:https://github.com/ggml-org/llama.cpp/discussions/13759#discussioncomment-13294811
    //if (params.prompt.find("<__media__>") == std::string::npos) {
    //    params.prompt += " <__media__>";
    //}
    if (0 == llm_is_running_state()) {
        llm_inference_interrupted = 1;
        goto failure;
    } else {
        GGML_JNI_NOTIFY("starting media encoding & decoding, pls waiting...\n\n");
    }

    msg.role = "user";
    msg.content = params.prompt;

    tmpl_inputs.messages = {msg};
    tmpl_inputs.add_generation_prompt = true;
    tmpl_inputs.use_jinja = false; // jinja is buggy here
    {
        auto formatted_chat = common_chat_templates_apply(chat_templates.get(), tmpl_inputs);
        LOGGD("formatted_chat.prompt: %s\n", formatted_chat.prompt.c_str());
        mtmd_input_text inp_txt = {
                formatted_chat.prompt.c_str(),
                /* add_special */   true,
                /* parse_special */ true,
        };

        mtmd::input_chunks chunks(mtmd_input_chunks_init());

        auto bitmaps_c_ptr = bitmaps.c_ptr();
        tokenized = mtmd_tokenize(mctx,
                                  chunks.ptr.get(),
                                  &inp_txt,
                                  bitmaps_c_ptr.data(),
                                  bitmaps_c_ptr.size());
        if (tokenized != 0) {
            LOGGD("Failed to tokenize prompt");
            goto failure;
        }
        bitmaps.entries.clear();
        if (0 == llm_is_running_state()) {
            llm_inference_interrupted = 1;
            goto failure;
        }

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
        if (0 == llm_is_running_state()) {
            llm_inference_interrupted = 1;
            goto failure;
        }
    }
    n_past = new_n_past;

    if (0 == llm_is_running_state()) {
        llm_inference_interrupted = 1;
        goto failure;
    }

    //step-6: LLM inference with the generated tokens
    for (int i = 0; i < n_predict; i++) {
        if (i > n_predict) {
            LOGGD("End of Text\n");
            break;
        }
        if (0 == llm_is_running_state()) {
            llm_inference_interrupted = 1;
            goto failure;
        }
        llama_token token_id = common_sampler_sample(smpl, lctx, -1);
        generated_tokens.push_back(token_id);
        common_sampler_accept(smpl, token_id, true);
        if (0 == llm_is_running_state()) {
            llm_inference_interrupted = 1;
            goto failure;
        }
        if (llama_vocab_is_eog(vocab, token_id)) {
            LOGGD("End of Text\n");
            break; // end of generation
        }

        tmp = common_token_to_piece(lctx, token_id).c_str();
        if (ggml_jni_is_valid_utf8(tmp)) {
            if (0 == llm_is_running_state()) {
                llm_inference_interrupted = 1;
                break;
            } else {
                GGML_JNI_NOTIFY(tmp);
            }
        }

        // eval the token
        common_batch_clear(batch);
        common_batch_add(batch, token_id, n_past++, {0}, true);
        if (llama_decode(lctx, batch)) {
            LOGGD("failed to decode token\n");
            goto failure;
        }
        if (0 == llm_is_running_state()) {
            llm_inference_interrupted = 1;
            goto failure;
        }
    }

    if (0 == llm_inference_interrupted) {
        llama_perf_context_print(lctx);
    }

failure:
    //step-7: cleanup
    common_sampler_free(smpl);
    mtmd_free(mctx);
    llama_backend_free();

    LOGGD("return");
    if (0 == llm_inference_interrupted)
        return 0;
    else
        return AI_INFERENCE_INTERRUPTED;
}
