#include "arg.h"
#include "common.h"
#include "log.h"
#include "sampling.h"
#include "llama.h"
#include "chat.h"

#include <cstdio>
#include <cstring>
#include <ctime>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <signal.h>
#include <unistd.h>
#elif defined (_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <signal.h>
#endif

#if defined(_MSC_VER)
#pragma warning(disable: 4244 4267) // possible loss of data
#endif

#if defined(__ANDROID__) || defined(ANDROID)
extern "C" {
#include "libavutil/cde_log.h"
#include "libavutil/cde_assert.h"
}
#include "ggml-jni.h"
// For Phase 2 model singleton: brings in g_jni_ctx extern declaration
// and the accessors used by ensure_model_loaded() below.
#include "ggml-jni-context.h"
#include "llamacpp/ggml/include/ggml-hexagon.h"
#endif

static llama_context           ** g_ctx;
static llama_model             ** g_model;
static common_sampler          ** g_smpl;
static common_params            * g_params;
static std::vector<llama_token> * g_input_tokens;
static std::ostringstream       * g_output_ss;
static std::vector<llama_token> * g_output_tokens;

// =============================================================================================
// Singleton-aware gating flags for llm_bench_inference (see class below).
// When the same model is re-used across multiple inference calls, we want to skip the
// (expensive) model load / init phase and the (expensive) backend / threadpool / sampler
// free phase, while still executing the (cheap) inference phase inside llama_inference_main().
// Default: both phases execute (the original llama-cli one-shot semantics).
// The wrapper class flips these flags around its call to llama_inference_main().
// =============================================================================================
static bool g_llm_bench_skip_init_phase      = false;  // true => skip lines from common_params_parse through sampler init
static bool g_llm_bench_skip_finalize_phase  = false;  // true => skip common_sampler_free / llama_backend_free / threadpool free
static std::string g_llm_bench_active_model_path;      // tracked by wrapper for debug / re-entry checks
static int         g_llm_bench_active_backend_type = -1;

static void print_usage(int argc, char ** argv) {
    (void) argc;

    LOG("\nexample usage:\n");
    LOG("\n  text generation:     %s -m your_model.gguf -p \"I believe the meaning of life is\" -n 128 -no-cnv\n", argv[0]);
    LOG("\n  chat (conversation): %s -m your_model.gguf -sys \"You are a helpful assistant\"\n", argv[0]);
    LOG("\n");
}

static bool file_exists(const std::string & path) {
    std::ifstream f(path.c_str());
    return f.good();
}

static bool file_is_empty(const std::string & path) {
    std::ifstream f;
    f.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    f.open(path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    return f.tellg() == 0;
}

int llama_inference_main(int argc, char ** argv, int backend_type) {
    int llm_inference_interrupted = 0;
    common_params params;
    g_params = &params;
    if (!common_params_parse(argc, argv, params, LLAMA_EXAMPLE_CLI, print_usage)) {
        return 1;
    }

    // Timing: measure TTFT breakdown to identify bottleneck
    auto t_start = std::chrono::high_resolution_clock::now();
    auto t_prev  = t_start;
    auto log_timing = [&](const char * tag) {
        auto t_now = std::chrono::high_resolution_clock::now();
        auto since_start = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_start).count();
        auto since_prev  = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_prev).count();
        LOGGD("[TTFT timing] %-28s total=%lldms  step=%lldms", tag, (long long)since_start, (long long)since_prev);
        t_prev = t_now;
    };

    LOGGD("enter llama_inference_main backend_type %d", backend_type);
    //runtime decision based on backend_type:
    //  HEXAGON_BACKEND_CDSP: offload all layers to DSP
    //  HEXAGON_BACKEND_GGML: CPU only, no offload
    if (backend_type == HEXAGON_BACKEND_CDSP) {
        LOGGD("using hexagon CDSP backend (runtime decision, -ngl 99)");
        params.main_gpu = 0;
        params.n_gpu_layers = 99;
    } else {
        LOGGD("using default ggml CPU backend (runtime decision, -ngl 0)");
        params.main_gpu = 0;
        params.n_gpu_layers = 0;
    }
    LOGGD("model path %s", params.model.path.c_str());
    // ---------------------------------------------------------------
    // Model-type detection and per-family configuration
    // ---------------------------------------------------------------
    // We detect the model family from the file path (case-insensitive)
    // so that we can apply the right sampling parameters and
    // enable_thinking flag. This avoids hard-coding a single model
    // family's behaviour and makes the pipeline robust to any model
    // that llama.cpp's chat templates support.
    //
    //   Family          | enable_thinking | Notes
    //   ----------------+-----------------+----------------------------
    //   gemma-3/4       | true            | Gemma uses <|channel> markers
    //                   |                 | which are handled by the
    //                   |                 | Java-side filter. Setting
    //                   |                 | enable_thinking=false
    //                   |                 | for Gemma causes the template
    //                   |                 | to produce malformed output,
    //                   |                 | so we keep it enabled.
    //   qwen / deepseek | false           | These models respect
    //   glm / minimax   |                 | enable_thinking in their
    //   (thinking fam.) |                 | Jinja templates, so we can
    //                   |                 | suppress thinking at the
    //                   |                 | source.
    //   unknown / any   | true            | Safe default; Java side
    //                   |                 | handles filtering for any
    //                   |                 | model that emits markers.
    // ---------------------------------------------------------------
    bool enable_thinking = true;
    {
        const std::string & path = params.model.path;
        // Lowercase for case-insensitive matching
        std::string lower_path = path;
        for (auto & c : lower_path) {
            if (c >= 'A' && c <= 'Z') { c = (char)(c + ('a' - 'A')); }
        }

        bool is_gemma     = lower_path.find("gemma") != std::string::npos;
        bool is_qwen      = lower_path.find("qwen")  != std::string::npos;
        bool is_deepseek  = lower_path.find("deepseek") != std::string::npos;
        bool is_glm       = lower_path.find("glm")    != std::string::npos
                          && lower_path.find("glm-")   != std::string::npos;

        if (is_gemma) {
            LOGGD("model family: GEMMA (enable_thinking=true, sampling preset)");
            enable_thinking = true;
            // according to the Gemma team, the optimal config for inference is
            // temperature = 1.0, top_k = 64, top_p = 0.95, min_p = 0.0
            params.sampling.temp = 1.0;
            params.sampling.top_k = 64;
            params.sampling.top_p = 0.95;
            params.sampling.min_p = 0.0;
        } else if (is_qwen || is_deepseek || is_glm) {
            LOGGD("model family: THINKING-AWARE (enable_thinking=false, model=%s)",
                  is_qwen ? "qwen" : (is_deepseek ? "deepseek" : "glm"));
            // For models that respect enable_thinking, disabling it at the
            // template level prevents thinking tokens from being generated
            // at all - the cleanest possible suppression.
            enable_thinking = false;
            params.sampling.temp = llm_get_temperature();
            LOGGD("temp %.2f\n", params.sampling.temp);
            params.sampling.top_p = llm_get_top_p();
            LOGGD("top_p %.2f\n", params.sampling.top_p);
        } else {
            LOGGD("model family: UNKNOWN (enable_thinking=true, default sampling)");
            enable_thinking = true;
            params.sampling.temp = llm_get_temperature();
            LOGGD("temp %.2f\n", params.sampling.temp);
            params.sampling.top_p = llm_get_top_p();
            LOGGD("top_p %.2f\n", params.sampling.top_p);
        }
    }
    // NOTE: common_init() and llama_backend_init() used to be called here.
    // They are now installed exactly once in ggml_jni_context::init() and
    // torn down in ggml_jni_context::cleanup() (called from
    // AIResearchFragment.onDestroy() via ggmljava.backendCleanup()).
    // The previous per-call cycle tore the Hexagon DSP down and back up on
    // every LLM inference, which was the dominant heat source.

    auto & sparams = params.sampling;
    if (params.embedding) {
        LOG_ERR("************\n");
        LOG_ERR("%s: please use the 'embedding' tool for embedding calculations\n", __func__);
        LOG_ERR("************\n\n");

        return 0;
    }

    if (params.n_ctx != 0 && params.n_ctx < 8) {
        LOG_WRN("%s: warning: minimum context size is 8, using minimum size.\n", __func__);
        params.n_ctx = 8;
    }

    if (params.rope_freq_base != 0.0) {
        LOG_WRN("%s: warning: changing RoPE frequency base to %g.\n", __func__, params.rope_freq_base);
    }

    if (params.rope_freq_scale != 0.0) {
        LOG_WRN("%s: warning: scaling RoPE frequency by %g.\n", __func__, params.rope_freq_scale);
    }

    LOG_INF("%s: llama backend init\n", __func__);
    // Diagnostic: print registered backends before init
    {
        size_t dev_count = ggml_backend_dev_count();
        LOGGD("ggml backend dev count: %zu", dev_count);
        for (size_t i = 0; i < dev_count; i++) {
            ggml_backend_dev_t dev = ggml_backend_dev_get(i);
            if (dev) {
                const char * name = ggml_backend_dev_name(dev);
                const char * desc = ggml_backend_dev_description(dev);
                LOGGD("  backend[%zu]: name=%s, desc=%s", i, name ? name : "(null)", desc ? desc : "(null)");
            }
        }
    }
    // hexagon backend is statically linked into libkantv-core.so, no need to
    // load separate .so via ggml_backend_load_all_from_path()
    // NOTE: llama_backend_init() used to be called here. It is now installed
    // once for the whole JNI lifetime in ggml_jni_context::init() and torn
    // down in ggml_jni_context::cleanup() (called from
    // AIResearchFragment.onDestroy() via ggmljava.backendCleanup()).
    // The previous per-call cycle tore the Hexagon DSP down and back up on
    // every LLM inference, which was the dominant heat source.
    llama_numa_init(params.numa);
    log_timing("after llama_backend_init");

    llama_model * model = nullptr;
    llama_context * ctx = nullptr;
    common_sampler * smpl = nullptr;

    g_model = &model;
    g_ctx = &ctx;
    g_smpl = &smpl;

    std::vector<common_chat_msg> chat_msgs;

    // load the model via the singleton (Phase 2). On a cache hit this
    // returns in microseconds and skips the multi-second 4GB read that
    // common_init_from_params() used to do here. On a cache miss
    // (first call, or model path / backend mismatch) it loads the
    // model into the singleton and we borrow the pointer.
    LOG_INF("%s: load the model and apply lora adapter, if any\n", __func__);
    int load_rc = g_jni_ctx.ensure_model_loaded(
        params.model.path.c_str(),
        /* mmproj_path */ "",
        /* n_gpu_layers */ params.n_gpu_layers); // LLM path; pass the actual ngl
                                                 // so a CPU LLM (-ngl 0) does NOT
                                                 // reuse a model previously loaded
                                                 // with -ngl 99 into Hexagon ION
    if (load_rc != 0) {
        LOG_ERR("%s: ensure_model_loaded failed (rc=%d) for '%s'\n",
                __func__, load_rc, params.model.path.c_str());
        return 1;
    }
    model = g_jni_ctx.get_loaded_model();
    if (model == nullptr) {
        LOG_ERR("%s: error: unable to load model\n", __func__);
        return 1;
    }
    // Per-call llama_context from the cached model. Fresh KV cache per
    // inference, with n_ctx / n_threads / batch settings from the
    // caller's params.
    {
        llama_context_params ctx_params = common_context_params_to_llama(params);
        ctx = llama_init_from_model(model, ctx_params);
    }
    if (ctx == nullptr) {
        LOG_ERR("%s: failed to create llama_context from cached model\n", __func__);
        return 1;
    }
    log_timing("after model load (singleton cache)");

    const llama_vocab * vocab = llama_model_get_vocab(model);
    auto chat_templates = common_chat_templates_init(model, params.chat_template);

    LOG_INF("%s: llama threadpool init, n_threads = %d\n", __func__, (int) params.cpuparams.n_threads);

    auto * cpu_dev = ggml_backend_dev_by_type(GGML_BACKEND_DEVICE_TYPE_CPU);
    if (!cpu_dev) {
        LOG_ERR("%s: no CPU backend found\n", __func__);
        LOGGD("%s: no CPU backend found\n", __func__);
        GGML_JNI_NOTIFY("%s: no CPU backend found\n", __func__);
        return 1;
    }
    auto * reg = ggml_backend_dev_backend_reg(cpu_dev);
    auto * ggml_threadpool_new_fn = (decltype(ggml_threadpool_new) *) ggml_backend_reg_get_proc_address(reg, "ggml_threadpool_new");
    auto * ggml_threadpool_free_fn = (decltype(ggml_threadpool_free) *) ggml_backend_reg_get_proc_address(reg, "ggml_threadpool_free");

    struct ggml_threadpool_params tpp_batch =
            ggml_threadpool_params_from_cpu_params(params.cpuparams_batch);
    struct ggml_threadpool_params tpp =
            ggml_threadpool_params_from_cpu_params(params.cpuparams);

    set_process_priority(params.cpuparams.priority);

    struct ggml_threadpool * threadpool_batch = NULL;
    if (!ggml_threadpool_params_match(&tpp, &tpp_batch)) {
        threadpool_batch = ggml_threadpool_new_fn(&tpp_batch);
        if (!threadpool_batch) {
            LOG_ERR("%s: batch threadpool create failed : n_threads %d\n", __func__, tpp_batch.n_threads);
            return 1;
        }

        // Start the non-batch threadpool in the paused state
        tpp.paused = true;
    }

    struct ggml_threadpool * threadpool = ggml_threadpool_new_fn(&tpp);
    if (!threadpool) {
        LOG_ERR("%s: threadpool create failed : n_threads %d\n", __func__, tpp.n_threads);
        return 1;
    }

    llama_attach_threadpool(ctx, threadpool, threadpool_batch);
    log_timing("after threadpool init");

    const int n_ctx_train = llama_model_n_ctx_train(model);
    const int n_ctx = llama_n_ctx(ctx);

    if (n_ctx > n_ctx_train) {
        LOG_WRN("%s: model was trained on only %d context tokens (%d specified)\n", __func__, n_ctx_train, n_ctx);
    }

    // auto enable conversation mode if chat template is available.
    // Note: we use common_chat_templates_has_default() rather than just
    // common_chat_templates_was_explicit(), because common_chat_templates_init
    // always sets template_default (to either the model's built-in template or
    // the CHATML fallback). Relying solely on has_explicit_template would
    // disable conversation mode for models that don't ship with a
    // built-in template, causing them to echo raw KANTV_CHAT_V1 content
    // instead of producing a proper chat response.
    const bool has_explicit_template = common_chat_templates_was_explicit(chat_templates.get());
    const bool has_any_chat_template = common_chat_templates_has_default(chat_templates.get());
    if (params.conversation_mode == COMMON_CONVERSATION_MODE_AUTO) {
        if (has_any_chat_template) {
            LOGGD("%s: chat template is available (explicit=%d), enabling conversation mode",
                    __func__, has_explicit_template ? 1 : 0);
            params.conversation_mode = COMMON_CONVERSATION_MODE_ENABLED;
        } else {
            LOGGD("%s: no chat template available, disabling conversation mode", __func__);
            params.conversation_mode = COMMON_CONVERSATION_MODE_DISABLED;
        }
    }

    // in case user force-activate conversation mode (via -cnv) without proper chat template, we show a warning
    if (params.conversation_mode && !has_any_chat_template) {
        LOGGD("%s: chat template is not available or is not supported. This may cause the model to output suboptimal responses", __func__);
    }

    // print chat template example in conversation mode
    if (params.conversation_mode) {
        if (params.enable_chat_template) {
            if (!params.prompt.empty() && params.system_prompt.empty()) {
                LOG_WRN("*** User-specified prompt will pre-start conversation, did you mean to set --system-prompt (-sys) instead?\n");
            }

            LOG_INF("%s: chat template example:\n%s\n", __func__, common_chat_format_example(chat_templates.get(), params.use_jinja, params.default_template_kwargs).c_str());
        } else {
            LOG_INF("%s: in-suffix/prefix is specified, chat template will be disabled\n", __func__);
        }
    }

    // print system information
    {
        LOG_INF("\n");
        LOG_INF("%s\n", common_params_get_system_info(params).c_str());
        LOG_INF("\n");
    }

    std::string path_session = params.path_prompt_cache;
    std::vector<llama_token> session_tokens;

    if (!path_session.empty()) {
        LOG_INF("%s: attempting to load saved session from '%s'\n", __func__, path_session.c_str());
        if (!file_exists(path_session)) {
            LOG_INF("%s: session file does not exist, will create.\n", __func__);
        } else if (file_is_empty(path_session)) {
            LOG_INF("%s: The session file is empty. A new session will be initialized.\n", __func__);
        } else {
            // The file exists and is not empty
            session_tokens.resize(n_ctx);
            size_t n_token_count_out = 0;
            if (!llama_state_load_file(ctx, path_session.c_str(), session_tokens.data(), session_tokens.capacity(), &n_token_count_out)) {
                LOG_ERR("%s: failed to load session file '%s'\n", __func__, path_session.c_str());
                return 1;
            }
            session_tokens.resize(n_token_count_out);
            LOG_INF("%s: loaded a session with prompt size of %d tokens\n", __func__, (int)session_tokens.size());
        }
    }

    const bool add_bos = llama_vocab_get_add_bos(vocab) && !params.use_jinja;
    if (!llama_model_has_encoder(model)) {
        GGML_ASSERT(!llama_vocab_get_add_eos(vocab));
    }

    LOG_DBG("n_ctx: %d, add_bos: %d\n", n_ctx, add_bos);

    std::vector<llama_token> embd_inp;

    bool waiting_for_first_input = false;
    auto chat_add_and_format = [&chat_msgs, &chat_templates](const std::string & role, const std::string & content) {
        common_chat_msg new_msg;
        new_msg.role = role;
        new_msg.content = content;
        auto formatted = common_chat_format_single(chat_templates.get(), chat_msgs, new_msg, role == "user", g_params->use_jinja);
        chat_msgs.push_back(new_msg);
        LOG_DBG("formatted: '%s'\n", formatted.c_str());
        return formatted;
    };

    std::string prompt;
    {
        LOGGD("%s: conversation_mode=%d, enable_chat_template=%d, prompt_size=%zu",
                __func__, params.conversation_mode, params.enable_chat_template, params.prompt.size());
        if (params.conversation_mode && params.enable_chat_template) {
            //
            // Multi-turn chat history fast-path (KanTV-specific).
            //
            // When the Java caller (AIResearchFragment.runInference) has
            // an accumulated conversation in its ChatAdapter, it packs
            // the full role/content history into a single string with
            // the "KANTV_CHAT_V1\n" magic prefix. Each message is
            // encoded as "{role}\x1E{content}\x1E" where \x1E is the
            // ASCII Record Separator (extremely unlikely to appear in
            // normal user input). This avoids needing a JSON parser
            // on the C++ side and stays binary-safe for newlines in
            // the content (the previous Java-side manual template
            // approach produced the "model echoes the entire prompt"
            // regression on small Q4_0 models because the model was
            // being fed raw text in -no-cnv mode, with no knowledge
            // that the input was a chat history).
            //
            // The parsed history is fed into chat_msgs and the
            // model's own chat template (read from the gguf metadata
            // by common_chat_templates_init above) is applied via
            // common_chat_templates_apply(). This is the llama.cpp
            // native path that the upstream CLI uses for `-cnv`
            // interactive mode; reusing it gives us correctly-formatted
            // prompts for Gemma, Qwen, Llama3, SmolVLM2, etc. without
            // per-model Java templates.
            //
            const std::string kantv_chat_magic = "KANTV_CHAT_V1\n";
            bool is_kantv_chat_history = params.prompt.size() >= kantv_chat_magic.size() &&
                                         params.prompt.compare(0, kantv_chat_magic.size(), kantv_chat_magic) == 0;
            LOGGD("%s: is_kantv_chat_history=%d, prompt_prefix='%s'",
                    __func__, is_kantv_chat_history,
                    params.prompt.substr(0, std::min((size_t)50, params.prompt.size())).c_str());
            if (is_kantv_chat_history) {
                const std::string payload = params.prompt.substr(kantv_chat_magic.size());
                const char SEP = 0x1E;
                size_t pos = 0;
                while (pos < payload.size()) {
                    size_t role_end = payload.find(SEP, pos);
                    if (role_end == std::string::npos) {
                        break;
                    }
                    std::string role = payload.substr(pos, role_end - pos);
                    pos = role_end + 1;
                    if (pos > payload.size()) {
                        break;
                    }
                    size_t content_end = payload.find(SEP, pos);
                    if (content_end == std::string::npos) {
                        // Final message has no trailing separator; consume the rest
                        content_end = payload.size();
                    }
                    std::string content = payload.substr(pos, content_end - pos);
                    pos = content_end + 1;
                    if (role.empty() && content.empty()) {
                        continue;
                    }
                    common_chat_msg new_msg;
                    new_msg.role    = role;
                    new_msg.content = content;
                    chat_msgs.push_back(new_msg);
                }
                LOG_INF("%s: parsed %zu chat messages from KANTV_CHAT_V1 payload (last role: '%s')\n",
                        __func__, chat_msgs.size(),
                        chat_msgs.empty() ? "<empty>" : chat_msgs.back().role.c_str());
            } else {
                // Original llama-cli single-prompt flow (system + one user message).
                if (!params.system_prompt.empty()) {
                    // format the system prompt (will use template default if empty)
                    chat_add_and_format("system", params.system_prompt);
                }

                if (!params.prompt.empty()) {
                    // format and append the user prompt
                    chat_add_and_format("user", params.prompt);
                } else {
                    waiting_for_first_input = true;
                }
            }

            if (!chat_msgs.empty()) {
                common_chat_templates_inputs inputs;
                inputs.messages              = chat_msgs;
                inputs.add_generation_prompt = true;
                // Propagate the model-family-specific enable_thinking
                // flag into the template inputs. Without this line the
                // template always uses its default (true), which means
                // that disabling thinking at the params level has no
                // effect - the Jinja template still generates thinking
                // tokens.
                inputs.enable_thinking       = enable_thinking;
                LOGGD("%s: inputs.enable_thinking = %d (model-family flag propagated)",
                        __func__, inputs.enable_thinking);
                // Pre-log the chat history so that if the 3rd question
                // crashes inside the jinja template engine (which can
                // happen on a longer multi-turn history), we have the
                // last known-good inputs in logcat for debugging. The
                // log line is bounded to 4KB to keep the logcat ring
                // buffer from filling up with the entire history.
                {
                    std::string history_dbg;
                    history_dbg.reserve(1024);
                    for (size_t mi = 0; mi < chat_msgs.size() && history_dbg.size() < 4096; ++mi) {
                        const auto & m = chat_msgs[mi];
                        history_dbg += "[" + std::to_string(mi) + " role=" + m.role + " len=" +
                                       std::to_string(m.content.size()) + "] ";
                    }
                    LOGGD("%s: applying chat template to %zu messages; history: %s",
                            __func__, chat_msgs.size(), history_dbg.c_str());
                }
                // common_chat_templates_apply invokes the jinja engine which
                // can throw std::runtime_error (e.g. on malformed templates,
                // undefined variables, or excessive nesting). We let the
                // exception propagate up to the ggml-jni-context.cpp top-level
                // try-catch (which logs the .what() and returns -1 to Java),
                // but the pre-log above means that even if the call does
                // exit(1) instead of throwing (e.g. via the arg.cpp
                // exception path), we still have the message list in logcat.
                LOGGD("%s: calling common_chat_templates_apply...", __func__);
                try {
                    prompt = common_chat_templates_apply(chat_templates.get(), inputs).prompt;
                    LOGGD("%s: chat template applied; %zu messages, prompt length %zu chars",
                            __func__, chat_msgs.size(), prompt.size());
                    LOGGD("%s: prompt preview (first 200 chars): '%s'",
                            __func__, prompt.substr(0, std::min((size_t)200, prompt.size())).c_str());
                } catch (const std::exception & e) {
                    LOGGD("%s: EXCEPTION in common_chat_templates_apply: %s", __func__, e.what());
                    LOGGD("%s: falling back to raw prompt (size=%zu)", __func__, params.prompt.size());
                    prompt = params.prompt;
                } catch (...) {
                    LOGGD("%s: UNKNOWN EXCEPTION in common_chat_templates_apply", __func__);
                    prompt = params.prompt;
                }
            }
        } else {
            // otherwise use the prompt as is
            LOGGD("%s: SKIP chat template (conversation_mode=%d, enable_chat_template=%d), using raw prompt, size=%zu",
                    __func__, params.conversation_mode, params.enable_chat_template, params.prompt.size());
            prompt = params.prompt;
        }

        if (params.interactive_first || !prompt.empty() || session_tokens.empty()) {
            LOG_DBG("tokenize the prompt\n");
            embd_inp = common_tokenize(ctx, prompt, true, true);
        } else {
            LOG_DBG("use session tokens\n");
            embd_inp = session_tokens;
        }

        LOG_DBG("prompt: \"%s\"\n", prompt.c_str());
        LOG_DBG("tokens: %s\n", string_from(ctx, embd_inp).c_str());
    }
    log_timing("after prompt tokenize");

    // Should not run without any tokens
    if (!waiting_for_first_input && embd_inp.empty()) {
        if (add_bos) {
            embd_inp.push_back(llama_vocab_bos(vocab));
            LOG_WRN("embd_inp was considered empty and bos was added: %s\n", string_from(ctx, embd_inp).c_str());
        } else {
            LOG_ERR("input is empty\n");
            return -1;
        }
    }

    // Tokenize negative prompt
    if ((int) embd_inp.size() > n_ctx - 4) {
        LOG_ERR("%s: prompt is too long (%d tokens, max %d)\n", __func__, (int) embd_inp.size(), n_ctx - 4);
        return 1;
    }

    // debug message about similarity of saved session, if applicable
    size_t n_matching_session_tokens = 0;
    if (!session_tokens.empty()) {
        for (llama_token id : session_tokens) {
            if (n_matching_session_tokens >= embd_inp.size() || id != embd_inp[n_matching_session_tokens]) {
                break;
            }
            n_matching_session_tokens++;
        }
        if (params.prompt.empty() && n_matching_session_tokens == embd_inp.size()) {
            LOG_INF("%s: using full prompt from session file\n", __func__);
        } else if (n_matching_session_tokens >= embd_inp.size()) {
            LOG_INF("%s: session file has exact match for prompt!\n", __func__);
        } else if (n_matching_session_tokens < (embd_inp.size() / 2)) {
            LOG_WRN("%s: session file has low similarity to prompt (%zu / %zu tokens); will mostly be reevaluated\n",
                    __func__, n_matching_session_tokens, embd_inp.size());
        } else {
            LOG_INF("%s: session file matches %zu / %zu tokens of prompt\n",
                    __func__, n_matching_session_tokens, embd_inp.size());
        }

        // remove any "future" tokens that we might have inherited from the previous session
        llama_memory_seq_rm(llama_get_memory(ctx), -1, n_matching_session_tokens, -1);
    }

    LOG_DBG("recalculate the cached logits (check): embd_inp.size() %zu, n_matching_session_tokens %zu, embd_inp.size() %zu, session_tokens.size() %zu\n",
         embd_inp.size(), n_matching_session_tokens, embd_inp.size(), session_tokens.size());

    // if we will use the cache for the full prompt without reaching the end of the cache, force
    // reevaluation of the last token to recalculate the cached logits
    if (!embd_inp.empty() && n_matching_session_tokens == embd_inp.size() && session_tokens.size() > embd_inp.size()) {
        LOG_DBG("recalculate the cached logits (do): session_tokens.resize( %zu )\n", embd_inp.size() - 1);

        session_tokens.resize(embd_inp.size() - 1);
    }

    // number of tokens to keep when resetting context
    if (params.n_keep < 0 || params.n_keep > (int) embd_inp.size()) {
        params.n_keep = (int)embd_inp.size();
    } else {
        params.n_keep += add_bos; // always keep the BOS token
    }

    if (params.conversation_mode) {
        if (params.single_turn && !params.prompt.empty()) {
            params.interactive = false;
            params.interactive_first = false;
        } else {
            params.interactive_first = true;
        }
    }

    // enable interactive mode if interactive start is specified
    if (params.interactive_first) {
        params.interactive = true;
    }

    if (params.verbose_prompt) {
        LOG_INF("%s: prompt: '%s'\n", __func__, prompt.c_str());
        LOG_INF("%s: number of tokens in prompt = %zu\n", __func__, embd_inp.size());
        for (int i = 0; i < (int) embd_inp.size(); i++) {
            LOG_INF("%6d -> '%s'\n", embd_inp[i], common_token_to_piece(ctx, embd_inp[i]).c_str());
        }

        if (params.n_keep > add_bos) {
            LOG_INF("%s: static prompt based on n_keep: '", __func__);
            for (int i = 0; i < params.n_keep; i++) {
                LOG_CNT("%s", common_token_to_piece(ctx, embd_inp[i]).c_str());
            }
            LOG_CNT("'\n");
        }
        LOG_INF("\n");
    }

    if (params.interactive) {
        LOG_INF("%s: interactive mode on.\n", __func__);

        if (!params.antiprompt.empty()) {
            for (const auto & antiprompt : params.antiprompt) {
                LOG_INF("Reverse prompt: '%s'\n", antiprompt.c_str());
                if (params.verbose_prompt) {
                    auto tmp = common_tokenize(ctx, antiprompt, false, true);
                    for (int i = 0; i < (int) tmp.size(); i++) {
                        LOG_INF("%6d -> '%s'\n", tmp[i], common_token_to_piece(ctx, tmp[i]).c_str());
                    }
                }
            }
        }

        if (params.input_prefix_bos) {
            LOG_INF("Input prefix with BOS\n");
        }

        if (!params.input_prefix.empty()) {
            LOG_INF("Input prefix: '%s'\n", params.input_prefix.c_str());
            if (params.verbose_prompt) {
                auto tmp = common_tokenize(ctx, params.input_prefix, true, true);
                for (int i = 0; i < (int) tmp.size(); i++) {
                    LOG_INF("%6d -> '%s'\n", tmp[i], common_token_to_piece(ctx, tmp[i]).c_str());
                }
            }
        }

        if (!params.input_suffix.empty()) {
            LOG_INF("Input suffix: '%s'\n", params.input_suffix.c_str());
            if (params.verbose_prompt) {
                auto tmp = common_tokenize(ctx, params.input_suffix, false, true);
                for (int i = 0; i < (int) tmp.size(); i++) {
                    LOG_INF("%6d -> '%s'\n", tmp[i], common_token_to_piece(ctx, tmp[i]).c_str());
                }
            }
        }
    }

    smpl = common_sampler_init(model, sparams);
    if (!smpl) {
        LOG_ERR("%s: failed to initialize sampling subsystem\n", __func__);
        return 1;
    }

    LOG_INF("sampler seed: %u\n",     common_sampler_get_seed(smpl));
    LOG_INF("sampler params: \n%s\n", sparams.print().c_str());
    LOG_INF("sampler chain: %s\n",    common_sampler_print(smpl).c_str());

    LOG_INF("generate: n_ctx = %d, n_batch = %d, n_predict = %d, n_keep = %d\n", n_ctx, params.n_batch, params.n_predict, params.n_keep);

    // group-attention state
    // number of grouped KV tokens so far (used only if params.grp_attn_n > 1)
    int ga_i = 0;

    const int ga_n = params.grp_attn_n;
    const int ga_w = params.grp_attn_w;

    if (ga_n != 1) {
        GGML_ASSERT(ga_n > 0                    && "grp_attn_n must be positive");                     // NOLINT
        GGML_ASSERT(ga_w % ga_n == 0            && "grp_attn_w must be a multiple of grp_attn_n");     // NOLINT
      //GGML_ASSERT(n_ctx_train % ga_w == 0     && "n_ctx_train must be a multiple of grp_attn_w");    // NOLINT
      //GGML_ASSERT(n_ctx >= n_ctx_train * ga_n && "n_ctx must be at least n_ctx_train * grp_attn_n"); // NOLINT
        LOG_INF("self-extend: n_ctx_train = %d, grp_attn_n = %d, grp_attn_w = %d\n", n_ctx_train, ga_n, ga_w);
    }
    LOG_INF("\n");

    bool is_antiprompt        = false;
    bool input_echo           = false;  // was true: prompt tokens were leaked to Java
                                        // via GGML_JNI_NOTIFY, causing the multi-turn
                                        // "previous Q&A visible in UI" bug. Only the
                                        // model's generated tokens should be forwarded.
    bool need_to_save_session = !path_session.empty() && n_matching_session_tokens < embd_inp.size();

    int n_past             = 0;
    int n_remain           = params.n_predict;
    int n_consumed         = 0;
    int n_session_consumed = 0;
    int n_generated        = 0; // Diagnostic: count generated tokens

    std::vector<int>   input_tokens;  g_input_tokens  = &input_tokens;
    std::vector<int>   output_tokens; g_output_tokens = &output_tokens;
    std::ostringstream output_ss;     g_output_ss     = &output_ss;

    std::vector<llama_token> embd;

    if (llama_model_has_encoder(model)) {
        int enc_input_size = embd_inp.size();
        llama_token * enc_input_buf = embd_inp.data();

        if (llama_encode(ctx, llama_batch_get_one(enc_input_buf, enc_input_size))) {
            LOG_ERR("%s : failed to eval\n", __func__);
            return 1;
        }

        llama_token decoder_start_token_id = llama_model_decoder_start_token(model);
        if (decoder_start_token_id == LLAMA_TOKEN_NULL) {
            decoder_start_token_id = llama_vocab_bos(vocab);
        }

        embd_inp.clear();
        embd_inp.push_back(decoder_start_token_id);
    }

    // Single-shot generation loop. With --single-turn (KANTV_CHAT_V1 path)
    // the model runs in non-interactive mode: it tokenizes the prompt, runs
    // the forward pass, samples tokens until EOG or n_predict tokens are
    // produced, and exits cleanly. There is no readline() loop and no
    // post-EOG chat_add_and_format() re-entry, so multi-turn history is
    // processed by the C++ chat template on entry (see KANTV_CHAT_V1
    // branch above) and not re-injected on every turn.
    while (n_remain != 0 && !is_antiprompt) {
        // predict
        if (!embd.empty()) {
            // Note: (n_ctx - 4) here is to match the logic for commandline prompt handling via
            // --prompt or --file which uses the same value.
            int max_embd_size = n_ctx - 4;

            // Ensure the input doesn't exceed the context size by truncating embd if necessary.
            if ((int) embd.size() > max_embd_size) {
                const int skipped_tokens = (int) embd.size() - max_embd_size;
                embd.resize(max_embd_size);

                LOG_WRN("<<input too long: skipped %d token%s>>", skipped_tokens, skipped_tokens != 1 ? "s" : "");
            }

            if (ga_n == 1) {
                // infinite text generation via context shifting
                // if we run out of context:
                // - take the n_keep first tokens from the original prompt (via n_past)
                // - take half of the last (n_ctx - n_keep) tokens and recompute the logits in batches

                if (n_past + (int) embd.size() >= n_ctx) {
                    if (!params.ctx_shift){
                        LOG_DBG("\n\n%s: context full and context shift is disabled => stopping\n", __func__);
                        break;
                    }

                    if (params.n_predict == -2) {
                        LOG_DBG("\n\n%s: context full and n_predict == -%d => stopping\n", __func__, params.n_predict);
                        break;
                    }

                    const int n_left    = n_past - params.n_keep;
                    const int n_discard = n_left/2;

                    LOG_DBG("context full, swapping: n_past = %d, n_left = %d, n_ctx = %d, n_keep = %d, n_discard = %d\n",
                            n_past, n_left, n_ctx, params.n_keep, n_discard);

                    llama_memory_seq_rm(llama_get_memory(ctx), 0, params.n_keep            , params.n_keep + n_discard);
                    llama_memory_seq_add(llama_get_memory(ctx), 0, params.n_keep + n_discard, n_past, -n_discard);

                    n_past -= n_discard;

                    LOG_DBG("after swap: n_past = %d\n", n_past);

                    LOG_DBG("embd: %s\n", string_from(ctx, embd).c_str());

                    LOG_DBG("clear session path\n");
                    path_session.clear();
                }
            } else {
                // context extension via Self-Extend
                while (n_past >= ga_i + ga_w) {
                    const int ib = (ga_n*ga_i)/ga_w;
                    const int bd = (ga_w/ga_n)*(ga_n - 1);
                    const int dd = (ga_w/ga_n) - ib*bd - ga_w;

                    LOG_DBG("\n");
                    LOG_DBG("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i, n_past, ib*bd, ga_i + ib*bd, n_past + ib*bd);
                    LOG_DBG("div:   [%6d, %6d] / %6d -> [%6d, %6d]\n", ga_i + ib*bd, ga_i + ib*bd + ga_w, ga_n, (ga_i + ib*bd)/ga_n, (ga_i + ib*bd + ga_w)/ga_n);
                    LOG_DBG("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i + ib*bd + ga_w, n_past + ib*bd, dd, ga_i + ib*bd + ga_w + dd, n_past + ib*bd + dd);

                    llama_memory_seq_add(llama_get_memory(ctx), 0, ga_i,                n_past,              ib*bd);
                    llama_memory_seq_div(llama_get_memory(ctx), 0, ga_i + ib*bd,        ga_i + ib*bd + ga_w, ga_n);
                    llama_memory_seq_add(llama_get_memory(ctx), 0, ga_i + ib*bd + ga_w, n_past + ib*bd,      dd);

                    n_past -= bd;

                    ga_i += ga_w/ga_n;

                    LOG_DBG("\nn_past_old = %d, n_past = %d, ga_i = %d\n\n", n_past + bd, n_past, ga_i);
                }
            }

            // try to reuse a matching prefix from the loaded session instead of re-eval (via n_past)
            if (n_session_consumed < (int) session_tokens.size()) {
                size_t i = 0;
                for ( ; i < embd.size(); i++) {
                    if (embd[i] != session_tokens[n_session_consumed]) {
                        session_tokens.resize(n_session_consumed);
                        break;
                    }

                    n_past++;
                    n_session_consumed++;

                    if (n_session_consumed >= (int) session_tokens.size()) {
                        ++i;
                        break;
                    }
                }
                if (i > 0) {
                    embd.erase(embd.begin(), embd.begin() + i);
                }
            }

            for (int i = 0; i < (int) embd.size(); i += params.n_batch) {
                int n_eval = (int) embd.size() - i;
                if (n_eval > params.n_batch) {
                    n_eval = params.n_batch;
                }

                //LOG_DBG("eval: %s\n", string_from(ctx, embd).c_str());

                if (llama_decode(ctx, llama_batch_get_one(&embd[i], n_eval))) {
                    LOG_ERR("%s : failed to eval\n", __func__);
                    return 1;
                }

                n_past += n_eval;

                //LOG_DBG("n_past = %d\n", n_past);
                // Display total tokens alongside total time
                if (params.n_print > 0 && n_past % params.n_print == 0) {
                    LOG_DBG("\n\033[31mTokens consumed so far = %d / %d \033[0m\n", n_past, n_ctx);
                }
            }

            if (!embd.empty() && !path_session.empty()) {
                session_tokens.insert(session_tokens.end(), embd.begin(), embd.end());
                n_session_consumed = session_tokens.size();
            }
        }

        embd.clear();

        if ((int) embd_inp.size() <= n_consumed) {
            // optionally save the session on first sample (for faster prompt loading next time)
            if (!path_session.empty() && need_to_save_session && !params.prompt_cache_ro) {
                need_to_save_session = false;
                llama_state_save_file(ctx, path_session.c_str(), session_tokens.data(), session_tokens.size());

                LOG_DBG("saved session to %s\n", path_session.c_str());
            }

            const llama_token id = common_sampler_sample(smpl, ctx, -1);

            common_sampler_accept(smpl, id, /* accept_grammar= */ true);

            // Log TTFT (time to first token) on the first generated token
            if (n_generated == 0) {
                log_timing("first token generated (TTFT)");
            }

            // LOG_DBG("last: %s\n", string_from(ctx, smpl->prev.to_vector()).c_str());

            // Diagnostic: log first 5 generated tokens
            if (n_generated < 5) {
                std::string token_str = common_token_to_piece(ctx, id);
                bool is_eog = llama_vocab_is_eog(vocab, id);
                LOGGD("generated[%d]: id=%d, is_eog=%d, str='%s', n_past=%d",
                      n_generated, (int)id, (int)is_eog, token_str.c_str(), n_past);
            }
            n_generated++;

            embd.push_back(id);

            // echo this to console
            input_echo = true;

            // decrement remaining sampling budget
            --n_remain;

            //LOG_DBG("n_remain: %d\n", n_remain);
        } else {
            // some user input remains from prompt or interaction, forward it to processing
            LOG_DBG("embd_inp.size(): %d, n_consumed: %d\n", (int) embd_inp.size(), n_consumed);
            while ((int) embd_inp.size() > n_consumed) {
                embd.push_back(embd_inp[n_consumed]);

                // push the prompt in the sampling context in order to apply repetition penalties later
                // for the prompt, we don't apply grammar rules
                common_sampler_accept(smpl, embd_inp[n_consumed], /* accept_grammar= */ false);

                ++n_consumed;
                if ((int) embd.size() >= params.n_batch) {
                    break;
                }
            }
        }

        // display text
        if (input_echo) {
            for (auto id : embd) {
                const std::string token_str = common_token_to_piece(ctx, id, params.special);

                // Stream to Java via JNI notification
#if (defined __ANDROID__) || (defined ANDROID)
                if (ggml_jni_is_valid_utf8(token_str.c_str())) {
                    if (0 == llm_is_running_state()) {
                        llm_inference_interrupted = 1;
                    } else {
                        GGML_JNI_NOTIFY(token_str.c_str());
                    }
                }
#endif
                // Record Displayed Tokens To Log
                // Note: Generated tokens are created one by one hence this check
                if (embd.size() > 1) {
                    // Incoming Requested Tokens
                    input_tokens.push_back(id);
                } else {
                    // Outgoing Generated Tokens
                    output_tokens.push_back(id);
                    output_ss << token_str;
                }
            }
        }

        // if not currently processing queued inputs;
        if ((int) embd_inp.size() <= n_consumed) {
            // check for reverse prompt in the last n_prev tokens
            if (!params.antiprompt.empty()) {
                const int n_prev = 32;
                const std::string last_output = common_sampler_prev_str(smpl, ctx, n_prev);

                is_antiprompt = false;
                // Check if each of the reverse prompts appears at the end of the output.
                // In single-shot (non-interactive) mode the antiprompt might be
                // tokenized with some following characters, so widen the search
                // window by 2 chars to compensate.
                for (std::string & antiprompt : params.antiprompt) {
                    size_t extra_padding = 2;
                    size_t search_start_pos = last_output.length() > static_cast<size_t>(antiprompt.length() + extra_padding)
                        ? last_output.length() - static_cast<size_t>(antiprompt.length() + extra_padding)
                        : 0;

                    if (last_output.find(antiprompt, search_start_pos) != std::string::npos) {
                        is_antiprompt = true;
                        break;
                    }
                }

                if (is_antiprompt) {
                    LOG_DBG("found antiprompt: %s\n", last_output.c_str());
                }
            }

            // End of generation: EOG token. In single-shot mode we just break
            // the loop; the post-loop code sends the EOG marker to Java and
            // prints perf data. (Previously this block had an
            // `if (params.interactive)` branch that called
            // chat_add_and_format("assistant", ...), is_interacting = true,
            // and re-entered via console::readline - that whole path was
            // removed when we deleted the terminal code.)
            if (!waiting_for_first_input && llama_vocab_is_eog(vocab, common_sampler_last(smpl))) {
                LOG_DBG("found an EOG token\n");
            }
        }

        if (0 == llm_is_running_state()) {
            llm_inference_interrupted = 1;
            break;
        }
        // end of generation
        if (!embd.empty() && llama_vocab_is_eog(vocab, embd.back())) {
            LOG(" [end of text]\n");
#if (defined __ANDROID__) || (defined ANDROID)
            GGML_JNI_NOTIFY("\n[end of text]\n\n");
#endif
            break;
        }
    }

    if (!path_session.empty() && params.prompt_cache_all && !params.prompt_cache_ro) {
        LOG("\n%s: saving final output to session file '%s'\n", __func__, path_session.c_str());
        llama_state_save_file(ctx, path_session.c_str(), session_tokens.data(), session_tokens.size());
    }

    LOG("\n\n");
    if (1 == llm_is_running_state()) {
        llm_inference_interrupted = 0;
        common_perf_print(ctx, smpl);
#if (defined __ANDROID__) || (defined ANDROID)
        //send PP/TG timing data to Java UI for display
        {
            llama_perf_context_data perf_data = llama_perf_context(ctx);
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
    } else {
        llm_inference_interrupted = 1;
    }

    common_sampler_free(smpl);

    // lctx is per-call: it owns the KV cache for this conversation,
    // so we always free it on exit. The cached llama_model lives in
    // the ggml_jni_context singleton and is NOT freed here - it will
    // be reused on the next LLM inference call (assuming the same
    // model path) and is finally released by
    // ggml_jni_context::unload_model() / cleanup() on app exit or
    // explicit model switch.
    if (ctx != nullptr) {
        llama_free(ctx);
        ctx = nullptr;
    }

    // llama_backend_free() used to be called here. The DSP backend is now
    // owned by ggml_jni_context and released in cleanup(); calling free
    // here would tear down the DSP that the next inference call (and the
    // MTMD path) depends on.

    ggml_threadpool_free_fn(threadpool);
    ggml_threadpool_free_fn(threadpool_batch);

    if (1 == llm_inference_interrupted)
        return AI_INFERENCE_INTERRUPTED;

    return 0;
}
