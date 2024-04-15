/*
 * the following is NOT original source code,
 *
 * just for personal study GGML(Georgi Gerganov Machine Learning, https://github.com/ggerganov/ggml)
 *
 * and various open source pure C/C++ AI projects based on GGML
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

#include "stable-diffusion.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#include "stb_image_write.h"
#include "stb_image_resize.h"


// =================================================================================================
//
// JNI helper function for llama.cpp

// all the following codes comes from examples/main.cpp in project llama.cpp
//
// trying to integrate llama.cpp from 03/26/2024 - 03/28/2024
// =================================================================================================
#include "sampling.h"

#define log_tostr(var) log_var_to_string_impl(var).c_str()

static uint32_t get_tensor_data_size(const ggml_tensor * tensor) {
    uint32_t data_size = tensor->ne[0] * tensor->ne[1] * tensor->ne[2] * tensor->ne[3] * ggml_type_size(tensor->type);

    return data_size;
}

static const char * get_qnn_backend_name(int n_backend_type) {
    switch (n_backend_type) {
        case 0:
            return "QNN CPU";
        case 1:
            return "QNN GPU";
        case 2:
            return "QNN DSP";
        case 3:
            return "ggml";
        default:
            return "unknown";
    }
}

// 03-26-2024,
// this function was referenced by this PR:https://github.com/ggerganov/llama.cpp/pull/5935/
// double check although this special case has been handled at the JNI layer
static bool is_valid_utf8(const char * string) {
    if (!string) {
        return true;
    }

    const unsigned char * bytes = (const unsigned char *)string;
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

static inline std::string log_var_to_string_impl(bool var)
{
    return var ? "true" : "false";
}

static inline std::string log_var_to_string_impl(std::string var)
{
    return var;
}

static inline std::string log_var_to_string_impl(const std::vector<int> & var)
{
    std::stringstream buf;
    buf << "[ ";
    bool first = true;
    for (auto e : var)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            buf << ", ";
        }
        buf << std::to_string(e);
    }
    buf << " ]";

    return buf.str();
}

template <typename C, typename T>
static inline std::string LOG_TOKENS_TOSTR_PRETTY(const C & ctx, const T & tokens)
{
    std::stringstream buf;
    buf << "[ ";

    bool first = true;
    for (const auto &token : tokens)
    {
        if (!first) {
            buf << ", ";
        } else {
            first = false;
        }

        auto detokenized = llama_token_to_piece(ctx, token);

        detokenized.erase(
                std::remove_if(
                        detokenized.begin(),
                        detokenized.end(),
                        [](const unsigned char c) { return !std::isprint(c); }),
                detokenized.end());

        buf
                << "'" << detokenized << "'"
                << ":" << std::to_string(token);
    }
    buf << " ]";

    return buf.str();
}


/**
 *
 * @param sz_model_path         /sdcard/kantv/gemma-2b.Q8_0.gguf,https://huggingface.co/ggerganov/gemma-2b-Q8_0-GGUF/resolve/main/gemma-2b.Q8_0.gguf(2.67 GB)
 * @param prompt
 * @param bench_type            not used currently
 * @param n_threads             1 - 8
 * @return
*/
int  llama_inference(const char * model_path, const char * prompt, int bench_type, int num_threads) {
    llama_context           ** g_ctx;
    llama_model             ** g_model;
    gpt_params               * g_params;
    std::vector<llama_token> * g_input_tokens;
    std::ostringstream       * g_output_ss;
    std::vector<llama_token> * g_output_tokens;
    bool is_interacting = false;

    gpt_params params;


    if (NULL == model_path)
        return -1;

    if (NULL == prompt)
        return -1;

    LOGGV("prompt:%s\n", prompt);
    LOGGV("model file %s\n", model_path);
    params.model = model_path;
    params.prompt = std::string(prompt);
    llama_sampling_params & sparams = params.sparams;

    if (params.n_ctx != 0 && params.n_ctx < 8) {
        LOGGV("%s: warning: minimum context size is 8, using minimum size.\n", __func__);
        params.n_ctx = 8;
    }

    if (params.rope_freq_base != 0.0) {
        LOGGV("%s: warning: changing RoPE frequency base to %g.\n", __func__, params.rope_freq_base);
    }

    if (params.rope_freq_scale != 0.0) {
        LOGGV("%s: warning: scaling RoPE frequency by %g.\n", __func__, params.rope_freq_scale);
    }

    if (params.seed == LLAMA_DEFAULT_SEED) {
        params.seed = time(NULL);
    }

    LOGGV("%s: seed  = %u\n", __func__, params.seed);

    std::mt19937 rng(params.seed);

    LOGGD("%s: llama backend init\n", __func__);
    llama_backend_init();
    llama_numa_init(params.numa);

    llama_model * model;
    llama_context * ctx;
    llama_context * ctx_guidance = NULL;
    g_model = &model;
    g_ctx = &ctx;

    // load the model and apply lora adapter, if any
    LOGGD("%s: load the model and apply lora adapter, if any\n", __func__);
    std::tie(model, ctx) = llama_init_from_gpt_params(params);
    if (sparams.cfg_scale > 1.f) {
        struct llama_context_params lparams = llama_context_params_from_gpt_params(params);
        ctx_guidance = llama_new_context_with_model(model, lparams);
    }

    if (model == NULL) {
        LOGGV("%s: error: unable to load model\n", __func__);
        return 1;
    }

    const int n_ctx_train = llama_n_ctx_train(model);
    const int n_ctx = llama_n_ctx(ctx);
    LOGGV("n_ctx: %d\n", n_ctx);

    if (n_ctx > n_ctx_train) {
        LOGGV("%s: warning: model was trained on only %d context tokens (%d specified)\n",
                __func__, n_ctx_train, n_ctx);
    }

    std::string path_session = params.path_prompt_cache;
    std::vector<llama_token> session_tokens;

    const bool add_bos = llama_should_add_bos_token(model);
    LOGGV("add_bos: %d\n", add_bos);

    std::vector<llama_token> embd_inp;

    embd_inp = ::llama_tokenize(ctx, params.prompt, add_bos, true);

    LOGGV("prompt: \"%s\"\n", log_tostr(params.prompt));
    LOGGV("tokens: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_inp).c_str());

    // Should not run without any tokens
    if (embd_inp.empty()) {
        embd_inp.push_back(llama_token_bos(model));
        LOGGV("embd_inp was considered empty and bos was added: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_inp).c_str());
    }

    // Tokenize negative prompt
    std::vector<llama_token> guidance_inp;
    int guidance_offset = 0;
    int original_prompt_len = 0;
    if (ctx_guidance) {
        LOGGV("cfg_negative_prompt: \"%s\"\n", log_tostr(sparams.cfg_negative_prompt));

        guidance_inp = ::llama_tokenize(ctx_guidance, sparams.cfg_negative_prompt, add_bos, true);
        LOGGV("guidance_inp tokenized: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx_guidance, guidance_inp).c_str());

        std::vector<llama_token> original_inp = ::llama_tokenize(ctx, params.prompt, add_bos, true);
        LOGGV("original_inp tokenized: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, original_inp).c_str());

        original_prompt_len = original_inp.size();
        guidance_offset = (int)guidance_inp.size() - original_prompt_len;
        LOGGV("original_prompt_len: %s", log_tostr(original_prompt_len));
        LOGGV("guidance_offset:     %s", log_tostr(guidance_offset));
    }

    if ((int) embd_inp.size() > n_ctx - 4) {
        LOGGV("%s: error: prompt is too long (%d tokens, max %d)\n", __func__, (int) embd_inp.size(), n_ctx - 4);
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
            LOGGV("%s: using full prompt from session file\n", __func__);
        } else if (n_matching_session_tokens >= embd_inp.size()) {
            LOGGV("%s: session file has exact match for prompt!\n", __func__);
        } else if (n_matching_session_tokens < (embd_inp.size() / 2)) {
            LOGGV("%s: warning: session file has low similarity to prompt (%zu / %zu tokens); will mostly be reevaluated\n",
                    __func__, n_matching_session_tokens, embd_inp.size());
        } else {
            LOGGV("%s: session file matches %zu / %zu tokens of prompt\n",
                    __func__, n_matching_session_tokens, embd_inp.size());
        }

        // remove any "future" tokens that we might have inherited from the previous session
        llama_kv_cache_seq_rm(ctx, -1, n_matching_session_tokens, -1);
    }

    LOGGD(
            "recalculate the cached logits (check): embd_inp.empty() %s, n_matching_session_tokens %zu, embd_inp.size() %zu, session_tokens.size() %zu, embd_inp.size() %zu",
            log_tostr(embd_inp.empty()), n_matching_session_tokens, embd_inp.size(), session_tokens.size(), embd_inp.size());

    // if we will use the cache for the full prompt without reaching the end of the cache, force
    // reevaluation of the last token token to recalculate the cached logits
    if (!embd_inp.empty() && n_matching_session_tokens == embd_inp.size() && session_tokens.size() > embd_inp.size()) {
        LOGGD("recalculate the cached logits (do): session_tokens.resize( %zu )", embd_inp.size() - 1);

        session_tokens.resize(embd_inp.size() - 1);
    }

    // number of tokens to keep when resetting context
    if (params.n_keep < 0 || params.n_keep > (int) embd_inp.size() || params.instruct || params.chatml) {
        params.n_keep = (int)embd_inp.size();
    } else {
        params.n_keep += add_bos; // always keep the BOS token
    }

    // prefix & suffix for instruct mode
    const auto inp_pfx = ::llama_tokenize(ctx, "\n\n### Instruction:\n\n", add_bos, true);
    const auto inp_sfx = ::llama_tokenize(ctx, "\n\n### Response:\n\n",    false,   true);

    LOGGV("inp_pfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, inp_pfx).c_str());
    LOGGV("inp_sfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, inp_sfx).c_str());

    // chatml prefix & suffix
    const auto cml_pfx = ::llama_tokenize(ctx, "\n<|im_start|>user\n", add_bos, true);
    const auto cml_sfx = ::llama_tokenize(ctx, "<|im_end|>\n<|im_start|>assistant\n", false, true);

    LOGGV("cml_pfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, cml_pfx).c_str());
    LOGGV("cml_sfx: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, cml_sfx).c_str());

    // in instruct mode, we inject a prefix and a suffix to each input by the user
    if (params.instruct) {
        params.interactive_first = true;
        params.antiprompt.emplace_back("### Instruction:\n\n");
    }
        // similar for chatml mode
    else if (params.chatml) {
        params.interactive_first = true;
        params.antiprompt.emplace_back("<|im_start|>user\n");
    }

    // enable interactive mode if interactive start is specified
    if (params.interactive_first) {
        params.interactive = true;
    }

    LOGGV("sampling: \n%s\n", llama_sampling_print(sparams).c_str());
    LOGGV("sampling order: \n%s\n", llama_sampling_order_print(sparams).c_str());
    LOGGV("generate: n_ctx = %d, n_batch = %d, n_predict = %d, n_keep = %d\n", n_ctx, params.n_batch, params.n_predict, params.n_keep);

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
        LOGGV("self-extend: n_ctx_train = %d, grp_attn_n = %d, grp_attn_w = %d\n", n_ctx_train, ga_n, ga_w);
    }
    LOGGV("\n\n");

    bool is_antiprompt        = false;
    bool input_echo           = true;
    bool display              = true;
    bool need_to_save_session = !path_session.empty() && n_matching_session_tokens < embd_inp.size();

    int n_past             = 0;
    int n_remain           = params.n_predict;
    int n_consumed         = 0;
    int n_session_consumed = 0;
    int n_past_guidance    = 0;

    std::vector<int>   input_tokens;  g_input_tokens  = &input_tokens;
    std::vector<int>   output_tokens; g_output_tokens = &output_tokens;
    std::ostringstream output_ss;     g_output_ss     = &output_ss;

    // the first thing we will do is to output the prompt, so set color accordingly
    //console::set_display(console::prompt);
    //display = params.display_prompt;

    std::vector<llama_token> embd;
    std::vector<llama_token> embd_guidance;

    // tokenized antiprompts
    std::vector<std::vector<llama_token>> antiprompt_ids;

    antiprompt_ids.reserve(params.antiprompt.size());
    for (const std::string & antiprompt : params.antiprompt) {
        antiprompt_ids.emplace_back(::llama_tokenize(ctx, antiprompt, false, true));
    }

    struct llama_sampling_context * ctx_sampling = llama_sampling_init(sparams);

    while ((n_remain != 0 && !is_antiprompt) || params.interactive) {
        // predict
        if (!embd.empty()) {
            // Note: (n_ctx - 4) here is to match the logic for commandline prompt handling via
            // --prompt or --file which uses the same value.
            int max_embd_size = n_ctx - 4;

            // Ensure the input doesn't exceed the context size by truncating embd if necessary.
            if ((int) embd.size() > max_embd_size) {
                const int skipped_tokens = (int) embd.size() - max_embd_size;
                embd.resize(max_embd_size);


                LOGGD("<<input too long: skipped %d token%s>>", skipped_tokens, skipped_tokens != 1 ? "s" : "");
            }

            if (ga_n == 1) {
                // infinite text generation via context shifting
                // if we run out of context:
                // - take the n_keep first tokens from the original prompt (via n_past)
                // - take half of the last (n_ctx - n_keep) tokens and recompute the logits in batches
                if (n_past + (int) embd.size() + std::max<int>(0, guidance_offset) > n_ctx) {
                    if (params.n_predict == -2) {
                        LOGGV("\n\n%s: context full and n_predict == -%d => stopping\n", __func__, params.n_predict);
                        break;
                    }

                    const int n_left    = n_past - params.n_keep;
                    const int n_discard = n_left/2;

                    LOGGV("context full, swapping: n_past = %d, n_left = %d, n_ctx = %d, n_keep = %d, n_discard = %d\n",
                        n_past, n_left, n_ctx, params.n_keep, n_discard);

                    llama_kv_cache_seq_rm (ctx, 0, params.n_keep            , params.n_keep + n_discard);
                    llama_kv_cache_seq_add(ctx, 0, params.n_keep + n_discard, n_past, -n_discard);

                    n_past -= n_discard;

                    if (ctx_guidance) {
                        n_past_guidance -= n_discard;
                    }

                    LOGGV("after swap: n_past = %d, n_past_guidance = %d\n", n_past, n_past_guidance);

                    LOGGV("embd: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd).c_str());

                    LOGGV("clear session path\n");
                    path_session.clear();
                }
            } else {
                // context extension via Self-Extend
                while (n_past >= ga_i + ga_w) {
                    const int ib = (ga_n*ga_i)/ga_w;
                    const int bd = (ga_w/ga_n)*(ga_n - 1);
                    const int dd = (ga_w/ga_n) - ib*bd - ga_w;

                    LOGGV("\n");
                    LOGGV("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i, n_past, ib*bd, ga_i + ib*bd, n_past + ib*bd);
                    LOGGV("div:   [%6d, %6d] / %6d -> [%6d, %6d]\n", ga_i + ib*bd, ga_i + ib*bd + ga_w, ga_n, (ga_i + ib*bd)/ga_n, (ga_i + ib*bd + ga_w)/ga_n);
                    LOGGV("shift: [%6d, %6d] + %6d -> [%6d, %6d]\n", ga_i + ib*bd + ga_w, n_past + ib*bd, dd, ga_i + ib*bd + ga_w + dd, n_past + ib*bd + dd);

                    llama_kv_cache_seq_add(ctx, 0, ga_i,                n_past,              ib*bd);
                    llama_kv_cache_seq_div(ctx, 0, ga_i + ib*bd,        ga_i + ib*bd + ga_w, ga_n);
                    llama_kv_cache_seq_add(ctx, 0, ga_i + ib*bd + ga_w, n_past + ib*bd,      dd);

                    n_past -= bd;

                    ga_i += ga_w/ga_n;

                    LOGGV("\nn_past_old = %d, n_past = %d, ga_i = %d\n\n", n_past + bd, n_past, ga_i);
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

            // evaluate tokens in batches
            // embd is typically prepared beforehand to fit within a batch, but not always
            if (ctx_guidance) {
                int input_size = 0;
                llama_token * input_buf = NULL;

                if (n_past_guidance < (int) guidance_inp.size()) {
                    // Guidance context should have the same data with these modifications:
                    //
                    // * Replace the initial prompt
                    // * Shift everything by guidance_offset
                    embd_guidance = guidance_inp;
                    if (embd.begin() + original_prompt_len < embd.end()) {
                        embd_guidance.insert(
                                embd_guidance.end(),
                                embd.begin() + original_prompt_len,
                                embd.end()
                        );
                    }

                    input_buf  = embd_guidance.data();
                    input_size = embd_guidance.size();

                    LOGGV("guidance context: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd_guidance).c_str());
                } else {
                    input_buf  = embd.data();
                    input_size = embd.size();
                }

                for (int i = 0; i < input_size; i += params.n_batch) {
                    int n_eval = std::min(input_size - i, params.n_batch);
                    if (llama_decode(ctx_guidance, llama_batch_get_one(input_buf + i, n_eval, n_past_guidance, 0))) {
                        LOGGV("%s : failed to eval\n", __func__);
                        return 1;
                    }

                    n_past_guidance += n_eval;
                }
            }

            for (int i = 0; i < (int) embd.size(); i += params.n_batch) {
                int n_eval = (int) embd.size() - i;
                if (n_eval > params.n_batch) {
                    n_eval = params.n_batch;
                }

                LOGGV("eval: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, embd).c_str());

                if (llama_decode(ctx, llama_batch_get_one(&embd[i], n_eval, n_past, 0))) {
                    LOGGV("%s : failed to eval\n", __func__);
                    return 1;
                }

                n_past += n_eval;

                LOGGV("n_past = %d\n", n_past);
                // Display total tokens alongside total time
                if (params.n_print > 0 && n_past % params.n_print == 0) {
                    LOGGV("\n\033[31mTokens consumed so far = %d / %d \033[0m\n", n_past, n_ctx);
                }
            }

            if (!embd.empty() && !path_session.empty()) {
                session_tokens.insert(session_tokens.end(), embd.begin(), embd.end());
                n_session_consumed = session_tokens.size();
            }
        }

        embd.clear();
        embd_guidance.clear();

        if ((int) embd_inp.size() <= n_consumed && !is_interacting) {
            const llama_token id = llama_sampling_sample(ctx_sampling, ctx, ctx_guidance);

            llama_sampling_accept(ctx_sampling, ctx, id, true);

            LOGGV("last: %s\n", LOG_TOKENS_TOSTR_PRETTY(ctx, ctx_sampling->prev).c_str());

            embd.push_back(id);

            // echo this to console
            input_echo = true;

            // decrement remaining sampling budget
            --n_remain;

            LOGGV("n_remain: %d\n", n_remain);
        } else {
            // some user input remains from prompt or interaction, forward it to processing
            LOGGV("embd_inp.size(): %d, n_consumed: %d\n", (int) embd_inp.size(), n_consumed);
            while ((int) embd_inp.size() > n_consumed) {
                embd.push_back(embd_inp[n_consumed]);

                // push the prompt in the sampling context in order to apply repetition penalties later
                // for the prompt, we don't apply grammar rules
                llama_sampling_accept(ctx_sampling, ctx, embd_inp[n_consumed], false);

                ++n_consumed;
                if ((int) embd.size() >= params.n_batch) {
                    break;
                }
            }
        }

        // display text
        if (input_echo && display) {
            for (auto id : embd) {
                const std::string token_str = llama_token_to_piece(ctx, id);
                printf("%s", token_str.c_str());
#ifdef TARGET_ANDROID
                if (is_valid_utf8(token_str.c_str())) { //ref:https://github.com/ggerganov/llama.cpp/pull/5935/
                    kantv_asr_notify_benchmark_c(token_str.c_str());
                }
#endif

                if (embd.size() > 1) {
                    input_tokens.push_back(id);
                } else {
                    output_tokens.push_back(id);
                    output_ss << token_str;
                }
            }
            fflush(stdout);
        }

        // if not currently processing queued inputs;
        if ((int) embd_inp.size() <= n_consumed) {
            // check for reverse prompt in the last n_prev tokens
            if (!params.antiprompt.empty()) {
                const int n_prev = 32;
                const std::string last_output = llama_sampling_prev_str(ctx_sampling, ctx, n_prev);

                is_antiprompt = false;
                // Check if each of the reverse prompts appears at the end of the output.
                // If we're not running interactively, the reverse prompt might be tokenized with some following characters
                // so we'll compensate for that by widening the search window a bit.
                for (std::string & antiprompt : params.antiprompt) {
                    size_t extra_padding = params.interactive ? 0 : 2;
                    size_t search_start_pos = last_output.length() > static_cast<size_t>(antiprompt.length() + extra_padding)
                                              ? last_output.length() - static_cast<size_t>(antiprompt.length() + extra_padding)
                                              : 0;

                    if (last_output.find(antiprompt, search_start_pos) != std::string::npos) {
                        if (params.interactive) {
                            is_interacting = true;
                        }
                        is_antiprompt = true;
                        break;
                    }
                }

                // check for reverse prompt using special tokens
                llama_token last_token = llama_sampling_last(ctx_sampling);
                for (std::vector<llama_token> ids : antiprompt_ids) {
                    if (ids.size() == 1 && last_token == ids[0]) {
                        if (params.interactive) {
                            is_interacting = true;
                        }
                        is_antiprompt = true;
                        break;
                    }
                }

            }
        }

        if (!embd.empty() && embd.back() == llama_token_eos(model)) {
            LOGGV(" [end of text]\n");
#ifdef TARGET_ANDROID
            kantv_asr_notify_benchmark_c("\n[end of text]\n\n");
#endif
            break;
        }
    }

    llama_print_timings(ctx);
    if (ctx_guidance) {
        llama_free(ctx_guidance);
    }

    llama_free(ctx);
    llama_free_model(model);

    llama_sampling_free(ctx_sampling);
    llama_backend_free();

    return 0;
}


static void ggml_graph_compute_helper(std::vector<uint8_t> & buf, ggml_cgraph * graph, int n_threads) {
    struct ggml_cplan plan = ggml_graph_plan(graph, n_threads);

    if (plan.work_size > 0) {
        buf.resize(plan.work_size);
        plan.work_data = buf.data();
    }

    ggml_graph_compute(graph, &plan);
}


static float tensor_sum_elements(const ggml_tensor * tensor) {
    double sum = 0;
    float  value = 0;
    std::ostringstream tmposs;
    if (tensor->type == GGML_TYPE_F32) {
        for (int h = 0; h < tensor->ne[3]; h++) {
            for (int i = 0; i < tensor->ne[2]; i++) {
                for (int j = 0; j < tensor->ne[1]; j++) {
                    for (int k = 0; k < tensor->ne[0]; k++) {
                        value = ((float *) tensor->data)[h * tensor->ne[2] + i * tensor->ne[1] + j * tensor->ne[0] + k];
                        sum += value;
                        //LOGGD("[%d][%d][%d][%d]%.2f \t", h, i, j, k, value);
                        tmposs << std::setw(8) << std::fixed << std::setprecision(2) << value << "\t";
                    }
                    if (strlen(tmposs.str().c_str()) > 4000) {

                    } else {
                        LOGGD("%s", tmposs.str().c_str());
                        GGML_JNI_NOTIFY("%s", tmposs.str().c_str());
                    }
                    tmposs.clear();
                    tmposs.str("");
                    LOGGD("\n");
                }
            }
        }
    }
    LOGGD("\n");
    return sum;
}

static void tensor_dump(const ggml_tensor * tensor, const char * name) {
    LOGGD("dump ggml tensor %s\n", name);
    GGML_JNI_NOTIFY("dump ggml tensor %s",name);
    LOGGD("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", name,
          tensor->type, ggml_type_name(tensor->type),
          tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    float sum = tensor_sum_elements(tensor);

    LOGGD("\n");
    LOGGD("Sum of tensor %s is %6.2f\n", name, sum);
}



#define TENSOR_DUMP(tensor) tensor_dump(tensor, #tensor)

struct benchmark_params_struct {
    int32_t n_threads     = 1;
    int32_t n_iterations  = 10;
};


void ggml_bench_matrix(int backend_type, int num_threads) {
    struct benchmark_params_struct benchmark_params;

    bool invalid_param = false;
    std::string arg;

    int64_t  n_begin_time           = 0LL;
    int64_t  n_end_time             = 0LL;
    int64_t  n_durtion              = 0LL;

    LOGGD("enter ggml_bench_matrix\n");

    GGML_JNI_NOTIFY("Starting GGML matrix benchmark\n");

    // create the ggml context
    struct ggml_context * ctx;
    //const int sizex = 4096;
    //const int sizey = 11008;

#undef VERBOSE_DEBUGGING
#ifndef VERBOSE_DEBUGGING
    const int sizey = 2048;
    const int sizex = 2048;
    const int sizez = 128;
#else
    /* Working - let's increase size */
    const int sizey = 1;
    const int sizex = (8*32);
    const int sizez = 1;

    /*const int sizey = 1;
    const int sizex = 3*(8*32);
    const int sizez = 1;*/
#endif

    GGML_JNI_NOTIFY("Memsize required = %i\n", sizex*sizex);

    // TODO: perform the bench for all types or for a user specified type
    const ggml_type qtype = GGML_TYPE_F32;

    n_begin_time                        = ggml_time_us();

    size_t ctx_size = 0;
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizez);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey); // BLAS
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey); // BLAS
    ctx_size += 1024*1024*16;

    GGML_JNI_NOTIFY("Allocating Memory of size %zi bytes, %zi MB\n",ctx_size, (ctx_size/1024/1024));

    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

#ifdef GGML_USE_QNN
    if (backend_type != 3) //original ggml
        params.use_hwaccel   = true;
#endif

    ctx = ggml_init(params);
    if (!ctx) {
        LOGGW("%s: ggml_init() failed\n");
        return;
    }

    GGML_JNI_NOTIFY("Creating new tensors\n");
    // GGML_JNI_NOTIFY("Creating new tensor m1\n");
    struct ggml_tensor * m11 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m11, 1.0f);

    // GGML_JNI_NOTIFY("Creating new tensor m1\n");
    struct ggml_tensor * m12 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m12, 2.0f);

    // GGML_JNI_NOTIFY("Creating new tensor m2\n");
    struct ggml_tensor * m2 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizez);
    ggml_set_f32(m2, 2.0f);

    GGML_JNI_NOTIFY("\n------Matrix Mult via F32 code\n");
    // GGML_JNI_NOTIFY("Creating new tensor m11xm2\n");
    struct ggml_tensor * m11xm12 = ggml_mul_mat(ctx, m11, m12);
    ggml_set_f32(m11xm12, 0.0f);

    // GGML_JNI_NOTIFY("Creating compute graph\n");
    struct ggml_cgraph * gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, m11xm12);

    GGML_JNI_NOTIFY("n_threads=%i\n", benchmark_params.n_threads);

    std::vector<uint8_t> work_buffer;

    ggml_graph_compute_helper(work_buffer, gf, benchmark_params.n_threads);

    if (get_tensor_data_size(m11) < 100) {
        TENSOR_DUMP(m11);
        TENSOR_DUMP(m12);
        TENSOR_DUMP(m11xm12);
    } else {
        LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", m11->name,
              m11->type, ggml_type_name(m11->type), m11->ne[0], m11->ne[1], m11->ne[2], m11->nb[0], m11->nb[1], m11->nb[2]);
        LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", m12->name,
              m12->type, ggml_type_name(m12->type), m12->ne[0], m12->ne[1], m12->ne[2], m12->nb[0], m12->nb[1], m12->nb[2]);
        LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", m11xm12->name,
              m11xm12->type, ggml_type_name(m11xm12->type), m11xm12->ne[0], m11xm12->ne[1], m11xm12->ne[2], m11xm12->nb[0], m11xm12->nb[1], m11xm12->nb[2]);
        GGML_JNI_NOTIFY("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", m11->name,
              m11->type, ggml_type_name(m11->type), m11->ne[0], m11->ne[1], m11->ne[2], m11->nb[0], m11->nb[1], m11->nb[2]);
        GGML_JNI_NOTIFY("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", m12->name,
              m12->type, ggml_type_name(m12->type), m12->ne[0], m12->ne[1], m12->ne[2], m12->nb[0], m12->nb[1], m12->nb[2]);
        GGML_JNI_NOTIFY("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", m11xm12->name,
              m11xm12->type, ggml_type_name(m11xm12->type), m11xm12->ne[0], m11xm12->ne[1], m11xm12->ne[2], m11xm12->nb[0], m11xm12->nb[1], m11xm12->nb[2]);
    }

    //TENSOR_DUMP(gf->nodes[0]);
    n_end_time  = ggml_time_us();
    n_durtion   = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of matrix with backend %d(%s) is: %lld milliseconds\n", backend_type, get_qnn_backend_name(backend_type), n_durtion);
    GGML_JNI_NOTIFY("duration of matrix with backend %d(%s) is: %lld milliseconds\n", backend_type, get_qnn_backend_name(backend_type), n_durtion);
    return;

    GGML_JNI_NOTIFY("\n------ Test 2 - Matrix Mult via %s code\n", ggml_type_name(qtype));

    int32_t nelements = sizex*sizey;

    // Set up a the benchmark matrices
    // GGML_JNI_NOTIFY("Creating new tensor q11 & Running quantize\n");
    struct ggml_tensor * q11 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
    ggml_quantize_chunk(qtype, (const float *) m11->data, q11->data, 0, nelements/m11->ne[0], m11->ne[0], nullptr);

    // Set up a the compute graph
    // GGML_JNI_NOTIFY("Creating new tensor q31\n");
    struct ggml_tensor * q31 = ggml_mul_mat(ctx, q11, m2);

    // GGML_JNI_NOTIFY("Creating compute graph\n");
    struct ggml_cgraph * gf31 = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf31, q31);

    // Set up a second graph computation to make sure we override the CPU cache lines
    // GGML_JNI_NOTIFY("Creating new tensor q12 & Running quantize\n");
    struct ggml_tensor * q12 = ggml_new_tensor_2d(ctx, qtype, sizex, sizey);
    ggml_quantize_chunk(qtype, (const float *) m12->data, q12->data, 0, nelements/m12->ne[0], m12->ne[0], nullptr);

    // GGML_JNI_NOTIFY("Creating new tensor q32\n");
    struct ggml_tensor * q32 = ggml_mul_mat(ctx, q12, m2);

    //GGML_JNI_NOTIFY("Creating compute graph\n");
    struct ggml_cgraph * gf32 = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf32, q32);
    GGML_JNI_NOTIFY("n_threads=%i\n", benchmark_params.n_threads);

    const int dimx = sizex;
    const int dimy = sizey;
    const int dimz = sizez;
    long long int flops_per_dot_product = dimy + dimy;
    long long int flops_per_matrix = flops_per_dot_product * dimx * dimz; ;
    GGML_JNI_NOTIFY("Matrix Multiplication of (%i,%i,%i) x (%i,%i,%i) - about %6.2f gFLOPS\n\n", sizex, sizey, 1, sizex, sizez, 1, 1.0f*flops_per_matrix / 1000 / 1000 / 1000);


    // Let's use the F32 result from above as a reference for the quantized multiplication
    float sum_of_F32_reference = tensor_sum_elements(gf->nodes[0]);

    GGML_JNI_NOTIFY("Iteration;NThreads; SizeX; SizeY; SizeZ; Required_FLOPS; Elapsed_u_Seconds; gigaFLOPS\n");
    GGML_JNI_NOTIFY("=====================================================================================\n");

    double  gflops_sum = 0;
    for (int i=0;i<benchmark_params.n_iterations ;i++) {

        long long int start = ggml_time_us();
        //GGML_JNI_NOTIFY("Running ggml_graph_compute\n");
        ggml_graph_compute_helper(work_buffer, gf31, benchmark_params.n_threads);

        long long int stop = ggml_time_us();
        long long int usec = stop-start;
        double gflops = (double)(flops_per_matrix)/usec/1000.0;
        gflops_sum += gflops;
        GGML_JNI_NOTIFY("%9i;%8i;%6i;%6i;%6i;%15lli;%18lli;%10.2f\n",
               i,
               benchmark_params.n_threads,
               sizex, sizey, sizez, flops_per_matrix,
               usec,gflops);

#ifdef VERBOSE_DEBUGGING
        TENSOR_DUMP("res",gf31.nodes[0])
#endif

        // Check that the matrix multiplication result is in the right ballpark
        // We cannot use the exact value from the F32 multiplication because the quantizuation will be slightly different
        float sum_of_Q4_result = tensor_sum_elements(gf31->nodes[0]);
        float delta = std::abs(sum_of_Q4_result - sum_of_F32_reference);
        float allowed_delta = (sum_of_F32_reference) / 1000 / 1000; //  Let's accept an epsilon of 10^-6

        if (delta > allowed_delta)  {
            GGML_JNI_NOTIFY("\nABORT - ERROR in Matrix Multiplication result - expected %6.2f, got %6.2f (delta %6.2f > allowed_delta %6.2f)\n",
                   sum_of_F32_reference,
                   sum_of_Q4_result,
                   delta,
                   allowed_delta
            );
            return;
        }

        // Running a different graph computation to make sure we override the CPU cache lines
        ggml_graph_compute_helper(work_buffer, gf32, benchmark_params.n_threads);
    }
    GGML_JNI_NOTIFY("\n");
    GGML_JNI_NOTIFY("Average%78.2f\n",gflops_sum/((double)benchmark_params.n_iterations));
    GGML_JNI_NOTIFY("=====================================================================================\n");

    LOGGD("leave ggml_bench_matrix\n");
}



// =================================================================================================
//
// JNI helper function for stablediffusion.cpp

// all the following codes comes from examples/main.cpp in project stablediffusion.cpp
//
// trying to integrate stablediffusion.cpp on 04-06-2024(April,6,2024)
// =================================================================================================

static const char* rng_type_to_str[] = {
        "std_default",
        "cuda",
};

// Names of the sampler method, same order as enum sample_method in stable-diffusion.h
static const char* sample_method_str[] = {
        "euler_a",
        "euler",
        "heun",
        "dpm2",
        "dpm++2s_a",
        "dpm++2m",
        "dpm++2mv2",
        "lcm",
};

// Names of the sigma schedule overrides, same order as sample_schedule in stable-diffusion.h
static const char* schedule_str[] = {
        "default",
        "discrete",
        "karras",
};

static const char* modes_str[] = {
        "txt2img",
        "img2img",
        "img2vid",
        "convert",
};

enum SDMode {
    TXT2IMG,
    IMG2IMG,
    IMG2VID,
    CONVERT,
    MODE_COUNT
};

struct SDParams {
    int n_threads = -1;
    SDMode mode   = TXT2IMG;

    std::string model_path;
    std::string vae_path;
    std::string taesd_path;
    std::string esrgan_path;
    std::string controlnet_path;
    std::string embeddings_path;
    std::string stacked_id_embeddings_path;
    std::string input_id_images_path;
    sd_type_t wtype = SD_TYPE_COUNT;
    std::string lora_model_dir;
    std::string output_path = "output.png";
    std::string input_path;
    std::string control_image_path;

    std::string prompt;
    std::string negative_prompt;
    float min_cfg     = 1.0f;
    float cfg_scale   = 7.0f;
    float style_ratio = 20.f;
    int clip_skip     = -1;  // <= 0 represents unspecified
    int width         = 512;
    int height        = 512;
    int batch_count   = 1;

    int video_frames         = 6;
    int motion_bucket_id     = 127;
    int fps                  = 6;
    float augmentation_level = 0.f;

    sample_method_t sample_method = EULER_A;
    schedule_t schedule           = DEFAULT;
    int sample_steps              = 20;
    float strength                = 0.75f;
    float control_strength        = 0.9f;
    rng_type_t rng_type           = CUDA_RNG;
    int64_t seed                  = 42;
    bool verbose                  = false;
    bool vae_tiling               = false;
    bool control_net_cpu          = false;
    bool normalize_input          = false;
    bool clip_on_cpu              = false;
    bool vae_on_cpu               = false;
    bool canny_preprocess         = false;
    bool color                    = false;
    int upscale_repeats           = 1;
};

static void print_params(SDParams params) {
    printf("Option: \n");
    printf("    n_threads:         %d\n", params.n_threads);
    printf("    mode:              %s\n", modes_str[params.mode]);
    printf("    model_path:        %s\n", params.model_path.c_str());
    printf("    wtype:             %s\n", params.wtype < SD_TYPE_COUNT ? sd_type_name(params.wtype) : "unspecified");
    printf("    vae_path:          %s\n", params.vae_path.c_str());
    printf("    taesd_path:        %s\n", params.taesd_path.c_str());
    printf("    esrgan_path:       %s\n", params.esrgan_path.c_str());
    printf("    controlnet_path:   %s\n", params.controlnet_path.c_str());
    printf("    embeddings_path:   %s\n", params.embeddings_path.c_str());
    printf("    stacked_id_embeddings_path:   %s\n", params.stacked_id_embeddings_path.c_str());
    printf("    input_id_images_path:   %s\n", params.input_id_images_path.c_str());
    printf("    style ratio:       %.2f\n", params.style_ratio);
    printf("    normzalize input image :  %s\n", params.normalize_input ? "true" : "false");
    printf("    output_path:       %s\n", params.output_path.c_str());
    printf("    init_img:          %s\n", params.input_path.c_str());
    printf("    control_image:     %s\n", params.control_image_path.c_str());
    printf("    clip on cpu:       %s\n", params.clip_on_cpu ? "true" : "false");
    printf("    controlnet cpu:    %s\n", params.control_net_cpu ? "true" : "false");
    printf("    vae decoder on cpu:%s\n", params.vae_on_cpu ? "true" : "false");
    printf("    strength(control): %.2f\n", params.control_strength);
    printf("    prompt:            %s\n", params.prompt.c_str());
    printf("    negative_prompt:   %s\n", params.negative_prompt.c_str());
    printf("    min_cfg:           %.2f\n", params.min_cfg);
    printf("    cfg_scale:         %.2f\n", params.cfg_scale);
    printf("    clip_skip:         %d\n", params.clip_skip);
    printf("    width:             %d\n", params.width);
    printf("    height:            %d\n", params.height);
    printf("    sample_method:     %s\n", sample_method_str[params.sample_method]);
    printf("    schedule:          %s\n", schedule_str[params.schedule]);
    printf("    sample_steps:      %d\n", params.sample_steps);
    printf("    strength(img2img): %.2f\n", params.strength);
    printf("    rng:               %s\n", rng_type_to_str[params.rng_type]);
    printf("    seed:              %ld\n", params.seed);
    printf("    batch_count:       %d\n", params.batch_count);
    printf("    vae_tiling:        %s\n", params.vae_tiling ? "true" : "false");
    printf("    upscale_repeats:   %d\n", params.upscale_repeats);
}

static void print_usage(int argc, const char* argv[]) {
    printf("usage: %s [arguments]\n", argv[0]);
    printf("\n");
    printf("arguments:\n");
    printf("  -h, --help                         show this help message and exit\n");
    printf("  -M, --mode [MODEL]                 run mode (txt2img or img2img or convert, default: txt2img)\n");
    printf("  -t, --threads N                    number of threads to use during computation (default: -1).\n");
    printf("                                     If threads <= 0, then threads will be set to the number of CPU physical cores\n");
    printf("  -m, --model [MODEL]                path to model\n");
    printf("  --vae [VAE]                        path to vae\n");
    printf("  --taesd [TAESD_PATH]               path to taesd. Using Tiny AutoEncoder for fast decoding (low quality)\n");
    printf("  --control-net [CONTROL_PATH]       path to control net model\n");
    printf("  --embd-dir [EMBEDDING_PATH]        path to embeddings.\n");
    printf("  --stacked-id-embd-dir [DIR]        path to PHOTOMAKER stacked id embeddings.\n");
    printf("  --input-id-images-dir [DIR]        path to PHOTOMAKER input id images dir.\n");
    printf("  --normalize-input                  normalize PHOTOMAKER input id images\n");
    printf("  --upscale-model [ESRGAN_PATH]      path to esrgan model. Upscale images after generate, just RealESRGAN_x4plus_anime_6B supported by now.\n");
    printf("  --upscale-repeats                  Run the ESRGAN upscaler this many times (default 1)\n");
    printf("  --type [TYPE]                      weight type (f32, f16, q4_0, q4_1, q5_0, q5_1, q8_0)\n");
    printf("                                     If not specified, the default is the type of the weight file.\n");
    printf("  --lora-model-dir [DIR]             lora model directory\n");
    printf("  -i, --init-img [IMAGE]             path to the input image, required by img2img\n");
    printf("  --control-image [IMAGE]            path to image condition, control net\n");
    printf("  -o, --output OUTPUT                path to write result image to (default: ./output.png)\n");
    printf("  -p, --prompt [PROMPT]              the prompt to render\n");
    printf("  -n, --negative-prompt PROMPT       the negative prompt (default: \"\")\n");
    printf("  --cfg-scale SCALE                  unconditional guidance scale: (default: 7.0)\n");
    printf("  --strength STRENGTH                strength for noising/unnoising (default: 0.75)\n");
    printf("  --style-ratio STYLE-RATIO          strength for keeping input identity (default: 20%%)\n");
    printf("  --control-strength STRENGTH        strength to apply Control Net (default: 0.9)\n");
    printf("                                     1.0 corresponds to full destruction of information in init image\n");
    printf("  -H, --height H                     image height, in pixel space (default: 512)\n");
    printf("  -W, --width W                      image width, in pixel space (default: 512)\n");
    printf("  --sampling-method {euler, euler_a, heun, dpm2, dpm++2s_a, dpm++2m, dpm++2mv2, lcm}\n");
    printf("                                     sampling method (default: \"euler_a\")\n");
    printf("  --steps  STEPS                     number of sample steps (default: 20)\n");
    printf("  --rng {std_default, cuda}          RNG (default: cuda)\n");
    printf("  -s SEED, --seed SEED               RNG seed (default: 42, use random seed for < 0)\n");
    printf("  -b, --batch-count COUNT            number of images to generate.\n");
    printf("  --schedule {discrete, karras}      Denoiser sigma schedule (default: discrete)\n");
    printf("  --clip-skip N                      ignore last layers of CLIP network; 1 ignores none, 2 ignores one layer (default: -1)\n");
    printf("                                     <= 0 represents unspecified, will be 1 for SD1.x, 2 for SD2.x\n");
    printf("  --vae-tiling                       process vae in tiles to reduce memory usage\n");
    printf("  --control-net-cpu                  keep controlnet in cpu (for low vram)\n");
    printf("  --canny                            apply canny preprocessor (edge detection)\n");
    printf("  -v, --verbose                      print extra info\n");
}

static void parse_args(int argc, const char** argv, SDParams& params) {
    bool invalid_arg = false;
    std::string arg;
    for (int i = 1; i < argc; i++) {
        arg = argv[i];

        if (arg == "-t" || arg == "--threads") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.n_threads = std::stoi(argv[i]);
        } else if (arg == "-M" || arg == "--mode") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            const char* mode_selected = argv[i];
            int mode_found            = -1;
            for (int d = 0; d < MODE_COUNT; d++) {
                if (!strcmp(mode_selected, modes_str[d])) {
                    mode_found = d;
                }
            }
            if (mode_found == -1) {
                fprintf(stderr,
                        "error: invalid mode %s, must be one of [txt2img, img2img, img2vid, convert]\n",
                        mode_selected);
                exit(1);
            }
            params.mode = (SDMode)mode_found;
        } else if (arg == "-m" || arg == "--model") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.model_path = argv[i];
        } else if (arg == "--vae") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.vae_path = argv[i];
        } else if (arg == "--taesd") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.taesd_path = argv[i];
        } else if (arg == "--control-net") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.controlnet_path = argv[i];
        } else if (arg == "--upscale-model") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.esrgan_path = argv[i];
        } else if (arg == "--embd-dir") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.embeddings_path = argv[i];
        } else if (arg == "--stacked-id-embd-dir") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.stacked_id_embeddings_path = argv[i];
        } else if (arg == "--input-id-images-dir") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.input_id_images_path = argv[i];
        } else if (arg == "--type") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            std::string type = argv[i];
            if (type == "f32") {
                params.wtype = SD_TYPE_F32;
            } else if (type == "f16") {
                params.wtype = SD_TYPE_F16;
            } else if (type == "q4_0") {
                params.wtype = SD_TYPE_Q4_0;
            } else if (type == "q4_1") {
                params.wtype = SD_TYPE_Q4_1;
            } else if (type == "q5_0") {
                params.wtype = SD_TYPE_Q5_0;
            } else if (type == "q5_1") {
                params.wtype = SD_TYPE_Q5_1;
            } else if (type == "q8_0") {
                params.wtype = SD_TYPE_Q8_0;
            } else {
                fprintf(stderr, "error: invalid weight format %s, must be one of [f32, f16, q4_0, q4_1, q5_0, q5_1, q8_0]\n",
                        type.c_str());
                exit(1);
            }
        } else if (arg == "--lora-model-dir") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.lora_model_dir = argv[i];
        } else if (arg == "-i" || arg == "--init-img") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.input_path = argv[i];
        } else if (arg == "--control-image") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.control_image_path = argv[i];
        } else if (arg == "-o" || arg == "--output") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.output_path = argv[i];
            LOGGD("output path %s\n", argv[i]);
            if (0 != access(argv[i], F_OK)) {
                LOGGD("path %s not exist\n", argv[i]);
                mkdir(argv[i], 0777);
            }
        } else if (arg == "-p" || arg == "--prompt") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.prompt = argv[i];
        } else if (arg == "--upscale-repeats") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.upscale_repeats = std::stoi(argv[i]);
            if (params.upscale_repeats < 1) {
                fprintf(stderr, "error: upscale multiplier must be at least 1\n");
                exit(1);
            }
        } else if (arg == "-n" || arg == "--negative-prompt") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.negative_prompt = argv[i];
        } else if (arg == "--cfg-scale") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.cfg_scale = std::stof(argv[i]);
        } else if (arg == "--strength") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.strength = std::stof(argv[i]);
        } else if (arg == "--style-ratio") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.style_ratio = std::stof(argv[i]);
        } else if (arg == "--control-strength") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.control_strength = std::stof(argv[i]);
        } else if (arg == "-H" || arg == "--height") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.height = std::stoi(argv[i]);
        } else if (arg == "-W" || arg == "--width") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.width = std::stoi(argv[i]);
        } else if (arg == "--steps") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.sample_steps = std::stoi(argv[i]);
        } else if (arg == "--clip-skip") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.clip_skip = std::stoi(argv[i]);
        } else if (arg == "--vae-tiling") {
            params.vae_tiling = true;
        } else if (arg == "--control-net-cpu") {
            params.control_net_cpu = true;
        } else if (arg == "--normalize-input") {
            params.normalize_input = true;
        } else if (arg == "--clip-on-cpu") {
            params.clip_on_cpu = true;  // will slow down get_learned_condiotion but necessary for low MEM GPUs
        } else if (arg == "--vae-on-cpu") {
            params.vae_on_cpu = true;  // will slow down latent decoding but necessary for low MEM GPUs
        } else if (arg == "--canny") {
            params.canny_preprocess = true;
        } else if (arg == "-b" || arg == "--batch-count") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.batch_count = std::stoi(argv[i]);
        } else if (arg == "--rng") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            std::string rng_type_str = argv[i];
            if (rng_type_str == "std_default") {
                params.rng_type = STD_DEFAULT_RNG;
            } else if (rng_type_str == "cuda") {
                params.rng_type = CUDA_RNG;
            } else {
                invalid_arg = true;
                break;
            }
        } else if (arg == "--schedule") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            const char* schedule_selected = argv[i];
            int schedule_found            = -1;
            for (int d = 0; d < N_SCHEDULES; d++) {
                if (!strcmp(schedule_selected, schedule_str[d])) {
                    schedule_found = d;
                }
            }
            if (schedule_found == -1) {
                invalid_arg = true;
                break;
            }
            params.schedule = (schedule_t)schedule_found;
        } else if (arg == "-s" || arg == "--seed") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            params.seed = std::stoll(argv[i]);
        } else if (arg == "--sampling-method") {
            if (++i >= argc) {
                invalid_arg = true;
                break;
            }
            const char* sample_method_selected = argv[i];
            int sample_method_found            = -1;
            for (int m = 0; m < N_SAMPLE_METHODS; m++) {
                if (!strcmp(sample_method_selected, sample_method_str[m])) {
                    sample_method_found = m;
                }
            }
            if (sample_method_found == -1) {
                invalid_arg = true;
                break;
            }
            params.sample_method = (sample_method_t)sample_method_found;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argc, argv);
            exit(0);
        } else if (arg == "-v" || arg == "--verbose") {
            params.verbose = true;
        } else if (arg == "--color") {
            params.color = true;
        } else {
            LOGGD( "error: unknown argument: %s\n", arg.c_str());
            print_usage(argc, argv);
            exit(1);
        }
    }
    if (invalid_arg) {
        LOGGD( "error: invalid parameter for argument: %s\n", arg.c_str());
        print_usage(argc, argv);
        exit(1);
    }
    if (params.n_threads <= 0) {
        params.n_threads = get_num_physical_cores();
    }

    if (params.mode != CONVERT && params.mode != IMG2VID && params.prompt.length() == 0) {
        LOGGD( "error: the following arguments are required: prompt\n");
        print_usage(argc, argv);
        exit(1);
    }

    if (params.model_path.length() == 0) {
        LOGGD("error: the following arguments are required: model_path\n");
        print_usage(argc, argv);
        exit(1);
    }

    if ((params.mode == IMG2IMG || params.mode == IMG2VID) && params.input_path.length() == 0) {
        LOGGD("error: when using the img2img mode, the following arguments are required: init-img\n");
        print_usage(argc, argv);
        exit(1);
    }

    if (params.output_path.length() == 0) {
        LOGGD("error: the following arguments are required: output_path\n");
        print_usage(argc, argv);
        exit(1);
    }

    if (params.width <= 0 || params.width % 64 != 0) {
        LOGGD( "error: the width must be a multiple of 64\n");
        exit(1);
    }

    if (params.height <= 0 || params.height % 64 != 0) {
        LOGGD("error: the height must be a multiple of 64\n");
        exit(1);
    }

    if (params.sample_steps <= 0) {
        LOGGD("error: the sample_steps must be greater than 0\n");
        exit(1);
    }

    if (params.strength < 0.f || params.strength > 1.f) {
        LOGGD( "error: can only work with strength in [0.0, 1.0]\n");
        exit(1);
    }

    if (params.seed < 0) {
        srand((int)time(NULL));
        params.seed = rand();
    }

    if (params.mode == CONVERT) {
        if (params.output_path == "output.png") {
            params.output_path = "output.gguf";
        }
    }
}


static std::string sd_basename(const std::string& path) {
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    pos = path.find_last_of('\\');
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

static std::string get_image_params(SDParams params, int64_t seed) {
    std::string parameter_string = params.prompt + "\n";
    if (params.negative_prompt.size() != 0) {
        parameter_string += "Negative prompt: " + params.negative_prompt + "\n";
    }
    parameter_string += "Steps: " + std::to_string(params.sample_steps) + ", ";
    parameter_string += "CFG scale: " + std::to_string(params.cfg_scale) + ", ";
    parameter_string += "Seed: " + std::to_string(seed) + ", ";
    parameter_string += "Size: " + std::to_string(params.width) + "x" + std::to_string(params.height) + ", ";
    parameter_string += "Model: " + sd_basename(params.model_path) + ", ";
    parameter_string += "RNG: " + std::string(rng_type_to_str[params.rng_type]) + ", ";
    parameter_string += "Sampler: " + std::string(sample_method_str[params.sample_method]);
    if (params.schedule == KARRAS) {
        parameter_string += " karras";
    }
    parameter_string += ", ";
    parameter_string += "Version: stable-diffusion.cpp";
    return parameter_string;
}

/* Enables Printing the log level tag in color using ANSI escape codes */
static void sd_log_cb(enum sd_log_level_t level, const char* log, void* data) {
    SDParams* params = (SDParams*)data;
    int tag_color;
    const char* level_str;
    FILE* out_stream = (level == SD_LOG_ERROR) ? stderr : stdout;

    if (!log || (!params->verbose && level <= SD_LOG_DEBUG)) {
        return;
    }

    switch (level) {
        case SD_LOG_DEBUG:
            tag_color = 37;
            level_str = "DEBUG";
            break;
        case SD_LOG_INFO:
            tag_color = 34;
            level_str = "INFO";
            break;
        case SD_LOG_WARN:
            tag_color = 35;
            level_str = "WARN";
            break;
        case SD_LOG_ERROR:
            tag_color = 31;
            level_str = "ERROR";
            break;
        default: /* Potential future-proofing */
            tag_color = 33;
            level_str = "?????";
            break;
    }
    LOGGD("[%-5s]%s ", level_str, log);
}


static int stablediffusion_main(int argc, const char* argv[]) {
    SDParams params;
    parse_args(argc, argv, params);

    sd_set_log_callback(sd_log_cb, (void*)&params);

    if (params.verbose) {
        print_params(params);
        LOGGD("%s", sd_get_system_info());
    }

    if (params.mode == CONVERT) {
        bool success = convert(params.model_path.c_str(), params.vae_path.c_str(), params.output_path.c_str(), params.wtype);
        if (!success) {
            LOGGD("convert '%s'/'%s' to '%s' failed\n",
                    params.model_path.c_str(),
                    params.vae_path.c_str(),
                    params.output_path.c_str());
            return 1;
        } else {
            LOGGD("convert '%s'/'%s' to '%s' success\n",
                   params.model_path.c_str(),
                   params.vae_path.c_str(),
                   params.output_path.c_str());
            return 0;
        }
    }

    if (params.mode == IMG2VID) {
        LOGGD("SVD support is broken, do not use it!!!\n");
        return 1;
    }

    bool vae_decode_only        = true;
    uint8_t* input_image_buffer = NULL;
    if (params.mode == IMG2IMG || params.mode == IMG2VID) {
        vae_decode_only = false;

        int c              = 0;
        input_image_buffer = stbi_load(params.input_path.c_str(), &params.width, &params.height, &c, 3);
        if (input_image_buffer == NULL) {
            LOGGD("load image from '%s' failed\n", params.input_path.c_str());
            return 1;
        }
        if (c < 3) {
            LOGGD("the number of channels for the input image must be >= 3, but got %d channels\n", c);
            free(input_image_buffer);
            return 1;
        }
        if (params.width <= 0) {
            LOGGD("error: the width of image must be greater than 0\n");
            free(input_image_buffer);
            return 1;
        }
        if (params.height <= 0) {
            LOGGD("error: the height of image must be greater than 0\n");
            free(input_image_buffer);
            return 1;
        }

        // Resize input image ...
        if (params.height % 64 != 0 || params.width % 64 != 0) {
            int resized_height = params.height + (64 - params.height % 64);
            int resized_width = params.width + (64 - params.width % 64);

            uint8_t *resized_image_buffer = (uint8_t *)malloc(resized_height * resized_width * 3);
            if (resized_image_buffer == NULL) {
                LOGGD("error: allocate memory for resize input image\n");
                free(input_image_buffer);
                return 1;
            }
            stbir_resize(input_image_buffer, params.width, params.height, 0,
                         resized_image_buffer, resized_width, resized_height, 0, STBIR_TYPE_UINT8,
                         3 /*RGB channel*/, STBIR_ALPHA_CHANNEL_NONE, 0,
                         STBIR_EDGE_CLAMP, STBIR_EDGE_CLAMP,
                         STBIR_FILTER_BOX, STBIR_FILTER_BOX,
                         STBIR_COLORSPACE_SRGB, nullptr
            );

            // Save resized result
            free(input_image_buffer);
            input_image_buffer = resized_image_buffer;
            params.height = resized_height;
            params.width = resized_width;
        }
    }

    sd_ctx_t* sd_ctx = new_sd_ctx(params.model_path.c_str(),
                                  params.vae_path.c_str(),
                                  params.taesd_path.c_str(),
                                  params.controlnet_path.c_str(),
                                  params.lora_model_dir.c_str(),
                                  params.embeddings_path.c_str(),
                                  params.stacked_id_embeddings_path.c_str(),
                                  vae_decode_only,
                                  params.vae_tiling,
                                  true,
                                  params.n_threads,
                                  params.wtype,
                                  params.rng_type,
                                  params.schedule,
                                  params.clip_on_cpu,
                                  params.control_net_cpu,
                                  params.vae_on_cpu);

    if (sd_ctx == NULL) {
        LOGGD("new_sd_ctx_t failed\n");
        return 1;
    }

    sd_image_t* results;
    if (params.mode == TXT2IMG) {
        sd_image_t* control_image = NULL;
        if (params.controlnet_path.size() > 0 && params.control_image_path.size() > 0) {
            int c              = 0;
            input_image_buffer = stbi_load(params.control_image_path.c_str(), &params.width, &params.height, &c, 3);
            if (input_image_buffer == NULL) {
                LOGGD("load image from '%s' failed\n", params.control_image_path.c_str());
                return 1;
            }
            control_image = new sd_image_t{(uint32_t)params.width,
                                           (uint32_t)params.height,
                                           3,
                                           input_image_buffer};
            if (params.canny_preprocess) {  // apply preprocessor
                control_image->data = preprocess_canny(control_image->data,
                                                       control_image->width,
                                                       control_image->height,
                                                       0.08f,
                                                       0.08f,
                                                       0.8f,
                                                       1.0f,
                                                       false);
            }
        }
        results = txt2img(sd_ctx,
                          params.prompt.c_str(),
                          params.negative_prompt.c_str(),
                          params.clip_skip,
                          params.cfg_scale,
                          params.width,
                          params.height,
                          params.sample_method,
                          params.sample_steps,
                          params.seed,
                          params.batch_count,
                          control_image,
                          params.control_strength,
                          params.style_ratio,
                          params.normalize_input,
                          params.input_id_images_path.c_str());
    } else {
        sd_image_t input_image = {(uint32_t)params.width,
                                  (uint32_t)params.height,
                                  3,
                                  input_image_buffer};

        if (params.mode == IMG2VID) {
            results = img2vid(sd_ctx,
                              input_image,
                              params.width,
                              params.height,
                              params.video_frames,
                              params.motion_bucket_id,
                              params.fps,
                              params.augmentation_level,
                              params.min_cfg,
                              params.cfg_scale,
                              params.sample_method,
                              params.sample_steps,
                              params.strength,
                              params.seed);
            if (results == NULL) {
                LOGGD("generate failed\n");
                free_sd_ctx(sd_ctx);
                return 1;
            }
            size_t last            = params.output_path.find_last_of(".");
            std::string dummy_name = last != std::string::npos ? params.output_path.substr(0, last) : params.output_path;
            for (int i = 0; i < params.video_frames; i++) {
                if (results[i].data == NULL) {
                    continue;
                }
                std::string final_image_path = i > 0 ? dummy_name + "_" + std::to_string(i + 1) + ".png" : dummy_name + ".png";
                stbi_write_png(final_image_path.c_str(), results[i].width, results[i].height, results[i].channel,
                               results[i].data, 0, get_image_params(params, params.seed + i).c_str());
                LOGGI("save result image to '%s'\n", final_image_path.c_str());
                free(results[i].data);
                results[i].data = NULL;
            }
            free(results);
            free_sd_ctx(sd_ctx);
            return 0;
        } else {
            results = img2img(sd_ctx,
                              input_image,
                              params.prompt.c_str(),
                              params.negative_prompt.c_str(),
                              params.clip_skip,
                              params.cfg_scale,
                              params.width,
                              params.height,
                              params.sample_method,
                              params.sample_steps,
                              params.strength,
                              params.seed,
                              params.batch_count);
        }
    }

    if (results == NULL) {
        LOGGD("generate failed\n");
        free_sd_ctx(sd_ctx);
        return 1;
    }

    int upscale_factor = 4;  // unused for RealESRGAN_x4plus_anime_6B.pth
    if (params.esrgan_path.size() > 0 && params.upscale_repeats > 0) {
        upscaler_ctx_t* upscaler_ctx = new_upscaler_ctx(params.esrgan_path.c_str(),
                                                        params.n_threads,
                                                        params.wtype);

        if (upscaler_ctx == NULL) {
            LOGGD("new_upscaler_ctx failed\n");
        } else {
            for (int i = 0; i < params.batch_count; i++) {
                if (results[i].data == NULL) {
                    continue;
                }
                sd_image_t current_image = results[i];
                for (int u = 0; u < params.upscale_repeats; ++u) {
                    sd_image_t upscaled_image = upscale(upscaler_ctx, current_image, upscale_factor);
                    if (upscaled_image.data == NULL) {
                        LOGGD("upscale failed\n");
                        break;
                    }
                    free(current_image.data);
                    current_image = upscaled_image;
                }
                results[i] = current_image;  // Set the final upscaled image as the result
            }
        }
    }

    size_t last            = params.output_path.find_last_of(".");
    std::string dummy_name = last != std::string::npos ? params.output_path.substr(0, last) : params.output_path;
    for (int i = 0; i < params.batch_count; i++) {
        if (results[i].data == NULL) {
            continue;
        }
        std::string final_image_path = i > 0 ? dummy_name + "_" + std::to_string(i + 1) + ".png" : dummy_name + ".png";
        stbi_write_png(final_image_path.c_str(), results[i].width, results[i].height, results[i].channel,
                       results[i].data, 0, get_image_params(params, params.seed + i).c_str());
        LOGGI("save result image to '%s'\n", final_image_path.c_str());
        free(results[i].data);
        results[i].data = NULL;
    }
    free(results);
    free_sd_ctx(sd_ctx);

    return 0;
}


/**
 *
 * @param sz_model_path         /sdcard/kantv/v2-1_768-nonema-pruned.q8_0.gguf
 * @param prompt
 * @param bench_type            not used currently
 * @param n_threads             1 - 8
 * @return
*/
int  stablediffusion_inference(const char * sz_model_path, const char * prompt, int bench_type, int num_threads) {
    int result = 0;

    //TODO: this is a lazy method, just for fun with stablediffusion.cpp on Xiaomi 14
    int argc = 7;
    LOGGD("stable diffusion model name:%s, prompt:%s\n", sz_model_path, prompt);
    if (NULL == sz_model_path || NULL == prompt)
        return 1;

    const char *argv[] = {"stablediffusion-main",
                    "-m", sz_model_path,
                    "-p", prompt,
                    "-o", "/sdcard/kantv/stablediffusion/"
    };
    stablediffusion_main(argc, argv);

    return result;
}