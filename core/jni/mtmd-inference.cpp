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

#if defined(__ANDROID__)
#include <android/log.h>
#endif

//ggml-jni
#include "ggml-jni.h"
// For Phase 2 model singleton: brings in the g_jni_ctx extern
// declaration and the singleton's method declarations.
#include "ggml-jni-context.h"

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
#include "mtmd-helper.h"


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

    // model and mctx are owned by the ggml_jni_context singleton - we
    // borrow them via accessors after ensure_model_loaded() succeeds and
    // do NOT free them here (only the per-call llama_context, sampler
    // and batch are freed).
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
    // NOTE: do NOT pre-set params.n_gpu_layers / params.main_gpu here. The
    // upstream common_params_parse() has a hard GGML_ASSERT(params.n_gpu_layers < 0)
    // (see common/arg.cpp:2621) that aborts the process if the value is >= 0.
    // The default is -1 ("auto"), which passes the assert; the actual value is
    // then applied by the -ngl/--gpu-layers option handler later in
    // common_params_parse() based on the argv built by the JNI bridge:
    //   - HEXAGON_BACKEND_CDSP -> -ngl 99 (offload all layers to DSP)
    //   - HEXAGON_BACKEND_GGML -> -ngl 0  (CPU only)
    if (!common_params_parse(argc, const_cast<char **>(argv), params, LLAMA_EXAMPLE_MTMD)) {
        LOGGD("common params parse failure\n");
        return 2;
    }
    // runtime decision log: report the value that common_params_parse settled on
    if (params.n_gpu_layers > 0) {
        LOGGD("using hexagon CDSP backend (runtime decision, n_gpu_layers=%d)\n",
              params.n_gpu_layers);
    } else {
        LOGGD("using default ggml CPU backend (runtime decision, n_gpu_layers=0)\n");
    }
    // NOTE: common_init() and llama_backend_init() used to be called here.
    // They are now installed exactly once in ggml_jni_context::init() and
    // torn down in ggml_jni_context::cleanup() (called from
    // AIResearchFragment.onDestroy() via ggmljava.backendCleanup()).
    // The previous per-call cycle tore the Hexagon DSP down and back up on
    // every MTMD inference, which was the dominant heat source and
    // contributed to the "2 succeed, 2 crash" intermittent abort on
    // CDSP. We keep llama_numa_init() here because it depends on the
    // n_threads value chosen by the user for *this* inference.
    llama_numa_init(params.numa);
    LOGGD("system info: n_threads = %d, n_threads_batch = %d, total_threads = %d\n",
          params.cpuparams.n_threads, params.cpuparams_batch.n_threads,
          std::thread::hardware_concurrency());
    LOGGD("\n");
    LOGGD("%s\n", common_params_get_system_info(params).c_str());
    LOGGD("\n");

    //step-2: ensure model is loaded (singleton - skips 4GB reload on cache hit)
    //
    // Previously this function called common_init_from_params() here to
    // read the 4GB model file from disk on every inference. On a phone
    // this is several seconds of full-bandwidth storage I/O and is the
    // dominant heat source of the AI Research page. We now delegate the
    // load to ggml_jni_context::ensure_model_loaded(), which only
    // touches disk on the first call (or when the model file /
    // mmproj path / backend triple changes). Subsequent inferences on
    // the same model reuse the cached llama_model + mtmd_context.
    LOGGD("loading model '%s'\n", params.model.path.c_str());
    // Pass n_gpu_layers (not the user-facing backend_type) as the cache
    // key. After common_params_parse() above we know the runtime ngl
    // value (0 for MTMD on this device - the JNI bridge forces -ngl 0
    // regardless of the user-selected backend). Caching by ngl
    // guarantees that if a previous LLM run had loaded the same model
    // file with ngl=99 (CDSP offload) into the Hexagon ION pool, we
    // will NOT reuse that pool-resident model for a CPU MTMD
    // inference - we will unload it and re-load with ngl=0 into
    // system memory. This is the fix for the "MTMD garbage output"
    // regression.
    int load_rc = g_jni_ctx.ensure_model_loaded(
        params.model.path.c_str(),
        params.mmproj.path.c_str(),
        params.n_gpu_layers);
    if (load_rc != 0) {
        LOGGD("ensure_model_loaded failed (rc=%d) for '%s'\n",
              load_rc, params.model.path.c_str());
        return 3;
    }
    model = g_jni_ctx.get_loaded_model();
    mctx  = g_jni_ctx.get_loaded_mctx();
    if (model == nullptr) {
        LOGGD("cached model is null after ensure_model_loaded\n");
        return 3;
    }
    // Create a per-call llama_context from the cached model. This
    // gives every inference a fresh KV cache (no leakage of prior
    // conversations) while the heavy 4GB model is reused. Uses
    // common_context_params_to_llama() so the n_ctx / n_threads /
    // flash-attention / batch settings from the caller's params are
    // honored on the new context. llama_init_from_model is the modern
    // (non-deprecated) replacement for llama_new_context_with_model.
    {
        llama_context_params ctx_params = common_context_params_to_llama(params);
        lctx = llama_init_from_model(model, ctx_params);
    }
    if (lctx == nullptr) {
        LOGGD("failed to create llama_context from cached model\n");
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

    //step-3: multimodal model (vision encoder) is already loaded by
    //ensure_model_loaded() above. The mtmd context pointer was retrieved
    //from the singleton into the local `mctx` variable. We sanity-check
    //it here before proceeding - the singleton can legitimately hold a
    //null mctx only if the caller passed an empty mmproj_path, which is
    //not the case for the MTMD path that always sets a non-empty
    //mmproj.
    if (mctx == nullptr) {
        LOGGD("multimodal context is null after ensure_model_loaded; this should not happen for MTMD\n");
        common_sampler_free(smpl);
        llama_batch_free(batch);
        llama_free(lctx);
        return 4;
    }
    LOGGD("loaded multimodal model, '%s'\n", params.mmproj.path.c_str());

    //step-4: load media(image / audio)
    for (const auto & image : params.image) {
        //mtmd_bitmap * bitmap = mtmd_helper_bitmap_init_from_file(params.image.front().c_str());
        auto res = mtmd_helper_bitmap_init_from_file(mctx, image.c_str(), false);
        mtmd::bitmap bmp(res.bitmap);
        if (!bmp.ptr) {
            LOGGD("failed to load media\n");
            GGML_JNI_NOTIFY("failed to load media\n");
            common_sampler_free(smpl);
            llama_batch_free(batch);
            // lctx is per-call and must be freed; mctx is in the
            // singleton and is NOT freed here (see failure: label).
            llama_free(lctx);
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
        common_chat_format_example(chat_templates.get(), params.use_jinja, params.default_template_kwargs);
    } catch (const std::exception &e) {
        LOGGD("%s: Chat template parsing error: %s\n", __func__, e.what());
        LOGGD("%s: The chat template that comes with this model is not yet supported, falling back to chatml."
              "This may cause the model to output suboptimal responses\n", __func__);
        chat_templates = common_chat_templates_init(model, "chatml");
    }
    // The second example-format call is also wrapped in try-catch. If the
    // fallback chatml template still throws (e.g. on a model whose vocab
    // does not declare the chatml special tokens), the previous version of
    // this code would let std::exception escape into std::terminate -> abort()
    // and silently kill the process between the chat-template example log
    // and the "starting media encoding" notify. Catching here lets the
    // inference continue with a minimal raw-prompt fallback.
    try {
        LOGGD("%s: chat template example:\n%s\n", __func__, common_chat_format_example(chat_templates.get(), params.use_jinja, params.default_template_kwargs).c_str());
    } catch (const std::exception &e) {
        LOGGD("%s: chat template example generation failed (%s); continuing with empty example\n",
              __func__, e.what());
    }
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
        // common_chat_templates_apply() can throw std::runtime_error (e.g.
        // "this custom template is not supported, try using --jinja") when the
        // built-in chat template does not match the model's expected format
        // (commonly seen with Gemma3 and similar newer models). Without this
        // try-catch the exception escapes into std::terminate -> abort() and
        // silently kills the native process without surfacing any error to
        // the Java layer. On failure, fall back to the raw prompt so the user
        // still gets a response instead of a crash.
        std::string formatted_prompt;
        try {
            auto formatted_chat = common_chat_templates_apply(chat_templates.get(), tmpl_inputs);
            formatted_prompt = formatted_chat.prompt;
        } catch (const std::exception & e) {
            LOGGD("%s: chat template apply failed (%s); using raw prompt as fallback\n",
                  __func__, e.what());
            formatted_prompt = msg.content;
        }
        LOGGD("formatted_chat.prompt: %s\n", formatted_prompt.c_str());
        mtmd_input_text inp_txt = {
                formatted_prompt.c_str(),
                /* text_len */       formatted_prompt.size(),
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

        {
            std::string token_str = common_token_to_piece(lctx, token_id);
            if (ggml_jni_is_valid_utf8(token_str.c_str())) {
                if (0 == llm_is_running_state()) {
                    llm_inference_interrupted = 1;
                    break;
                } else {
                    GGML_JNI_NOTIFY(token_str.c_str());
                }
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
#if (defined __ANDROID__) || (defined ANDROID)
        //send PP/TG timing data to Java UI for display
        {
            llama_perf_context_data perf_data = llama_perf_context(lctx);
            char perf_str[512];
            double pp_ms_per_tok = perf_data.n_p_eval > 0 ? perf_data.t_p_eval_ms / perf_data.n_p_eval : 0.0;
            double pp_tok_per_s   = perf_data.t_p_eval_ms > 0 ? 1e3 / perf_data.t_p_eval_ms * perf_data.n_p_eval : 0.0;
            double tg_ms_per_tok = perf_data.n_eval > 0 ? perf_data.t_eval_ms / perf_data.n_eval : 0.0;
            double tg_tok_per_s   = perf_data.t_eval_ms > 0 ? 1e3 / perf_data.t_eval_ms * perf_data.n_eval : 0.0;
            snprintf(perf_str, sizeof(perf_str),
                "llama-timings:\n"
                "  prompt eval time = %10.2f ms / %5d tokens (%8.2f ms per token, %8.2f tokens per second)\n"
                "         eval time = %10.2f ms / %5d runs   (%8.2f ms per token, %8.2f tokens per second)\n",
                perf_data.t_p_eval_ms, perf_data.n_p_eval, pp_ms_per_tok, pp_tok_per_s,
                perf_data.t_eval_ms,   perf_data.n_eval,   tg_ms_per_tok, tg_tok_per_s
            );
            GGML_JNI_NOTIFY("%s", perf_str);
            LOGGD("%s", perf_str);
        }
#endif
    }

failure:
    //step-7: cleanup
    common_sampler_free(smpl);
    // lctx is per-call: it owns the KV cache for this conversation, so
    // we always free it on exit. The cached llama_model and mtmd
    // context (mctx) live in the ggml_jni_context singleton and are
    // NOT freed here - they will be reused on the next MTMD inference
    // call (assuming the same model / mmproj / backend) and are
    // finally released by ggml_jni_context::unload_model() / cleanup()
    // on app exit or explicit model switch.
    if (lctx != nullptr) {
        llama_free(lctx);
        lctx = nullptr;
    }
    // llama_batch was created with llama_batch_init(params.n_batch, 0, 1)
    // (see step-2). It owns three heap-allocated arrays (token / pos /
    // logits) of size n_batch each. Without llama_batch_free() those
    // allocations leak on every call. Over a few MTMD inference calls in
    // the same process this can grow large enough to interact badly with
    // the ggml-hexagon ION pool and trigger a crash that looks
    // non-deterministic. The upstream mtmd-cli.cpp does this in
    // ~mtmd_cli_context(); we mirror it here for the goto-failure path.
    llama_batch_free(batch);

    LOGGD("return");
    if (0 == llm_inference_interrupted)
        return 0;
    else
        return AI_INFERENCE_INTERRUPTED;
}
