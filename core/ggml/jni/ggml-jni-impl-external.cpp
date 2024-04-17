/*
 * the following is NOT 100% original source code,
 *
 * just for:
 *
 * study internal mechanism of GGML(Georgi Gerganov Machine Learning, https://github.com/ggerganov/ggml)
 *
 * study various open source pure C/C++ AI projects based on GGML(such as llama.cpp, stablediffusion.cpp)
 *
 * implementation of PoC S2 & PoC S3 (merged into this file on 04/15/2024), https://github.com/zhouwg/kantv/issues/121
 *
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


static const char * get_qnn_backend_name(int n_backend_type);
static float tensor_sum_elements(const ggml_tensor * tensor);
static void ggml_graph_compute_helper(std::vector<uint8_t> & buf, ggml_cgraph * graph, int n_threads);
static void tensor_dump(const ggml_tensor * tensor, const char * name);
#define TENSOR_DUMP(tensor) tensor_dump(tensor, #tensor)

static uint32_t get_tensor_data_size(const ggml_tensor * tensor) {
    /*
    size_t data_size = ggml_row_size(tensor->type, tensor->ne[0]);
    size_t n_dims = get_tensor_rank(tensor);
    for (int i = 1; i < n_dims; i++) {
        data_size *= tensor->ne[i];
    }

    return data_size;
     */
    return ggml_nbytes(tensor);
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
 * @param n_backend             0: QNN CPU 1: QNN GPU 2: QNN HTP(DSP) 3:ggml
 * @return
*/
int  llama_inference(const char * model_path, const char * prompt, int bench_type, int num_threads, int n_backend) {
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






struct benchmark_params_struct {
    int32_t n_threads     = 1;
    int32_t n_iterations  = 10;
};


void ggml_bench_matrix(int num_threads, int backend_type) {
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


#if 0
    const int sizey = 2048;
    const int sizex = 2048;
    const int sizez = 128;
#else
    const int sizey = 2;
    const int sizex = 2;
    const int sizez = 1;
#endif

    GGML_JNI_NOTIFY("Memsize required = %i\n", sizex*sizex);

    // TODO: perform the bench for all types or for a user specified type
    const ggml_type qtype = GGML_TYPE_F32;

    n_begin_time                        = ggml_time_us();

    benchmark_params.n_threads          = num_threads;

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
    ggml_free(ctx);
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
 * @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance)
 * @return
*/
int  stablediffusion_inference(const char * sz_model_path, const char * prompt, int bench_type, int num_threads, int n_backend_type) {
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


/*
 * MIT license
 * Copyright (C) 2024 KanTV Authors
 * SPDX-License-Identifier: MIT
 *
 * The most important two references are:
 *
 * (1) https://github.com/pytorch/executorch/tree/main/backends/qualcomm
 *
 * (2) /opt/qcom/aistack/qnn/2.20.0.240223/examples/Models/InceptionV3/model/Inception_v3.cpp
 *
 *     Inception_v3.cpp is generated automatically by Qualcomm's dedicated tool and it contains more then 20,000 lines C++ code
 *
 *
 * this implementation is preparation stage (PoC-S2, PoC-S3) of PoC-S42
 *
 * PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121) in Project KanTV
 *
 * should be renamed to ggml-jni-qnn.cpp after finish this PoC
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>

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

#include "QnnTypes.h"
#include "QnnCommon.h"
#include "QnnContext.h"
#include "QnnBackend.h"
#include "QnnGraph.h"
#include "QnnProperty.h"
#include "QnnTensor.h"
#include "QnnInterface.h"
#include "Saver/QnnSaver.h"
#include "System/QnnSystemInterface.h"
#include "HTP/QnnHtpDevice.h"

#include "ggml-qnn.h"

#include "ggml-jni.h"


// =================================================================================================
//
// self-defined structure / class / macro / const / pfn
//
// =================================================================================================
#define RPCMEM_DEFAULT_FLAGS                            1
#define RPCMEM_HEAP_ID_SYSTEM                           25
#define QNN_VER_PTR(x)                                  (&((x).v1))
#define TENSOR_DUMP(tensor)                             tensor_dump(tensor, #tensor)
#define QNN_OP_CFG_VALID(opConfig)                      ((opConfig).version == QNN_OPCONFIG_VERSION_1)

#define VALIDATE(value, status)                         \
  do {                                                  \
    status = value;                                     \
    if (status != QNN_SUCCESS) {                        \
      LOGGW("%s expected QNN_SUCCESS\n", #value);       \
      return status;                                    \
    }                                                   \
  } while (0)

#define VALIDATE_TENSOR_VERSION(tensor, err)            VALIDATE(validateTensorVersion(tensor), err)

#define VALIDATE_OP_CONFIG_VERSION(op, err)             VALIDATE(validateOpConfigVersion(op), err)


#define BINVARSTART(NAME)                                            \
  ({                                                                 \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_start[];    \
    (void *)_binary_obj_binary_##NAME##_raw_start;                   \
  })


#define BINVAREND(NAME)                                              \
  ({                                                                 \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_end[];      \
    (void *)_binary_obj_binary_##NAME##_raw_end;                     \
  })

#define BINLEN(NAME)                                                                             \
  ({                                                                                             \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_start[];                                \
    extern const uint8_t _binary_obj_binary_##NAME##_raw_end[];                                  \
    (uint32_t)((_binary_obj_binary_##NAME##_raw_end) - (_binary_obj_binary_##NAME##_raw_start)); \
  })


#define FREE_MEMORY(ptr1, ptr2, ptr3) \
  do {                                \
    free(ptr1);                       \
    free(ptr2);                       \
    free(ptr3);                       \
  } while (0)


enum class ggml_qnn_profile_level {
    profile_off         = 0,
    profile_basic       = 1,
    profile_detail      = 2
};


enum class OutputDataType {
    FLOAT_ONLY, NATIVE_ONLY, FLOAT_AND_NATIVE, INVALID
};


enum class InputDataType {
    FLOAT, NATIVE, INVALID
};


typedef struct GraphInfo {
    Qnn_GraphHandle_t graph;
    char * graphName;
    Qnn_Tensor_t * inputTensors;
    uint32_t numInputTensors;
    Qnn_Tensor_t * outputTensors;
    uint32_t numOutputTensors;
} GraphInfo_t;
typedef GraphInfo_t * GraphInfoPtr_t;


typedef struct GraphConfigInfo {
    char * graphName;
    const QnnGraph_Config_t ** graphConfigs;
} GraphConfigInfo_t;


typedef int ( * ComposeGraphsHandleType_t)(
        Qnn_BackendHandle_t,
        QNN_INTERFACE_VER_TYPE,
        Qnn_ContextHandle_t,
        const GraphConfigInfo_t **,
        const uint32_t,
        GraphInfo_t ***,
        uint32_t *,
        bool,
        QnnLog_Callback_t,
        QnnLog_Level_t);


typedef int ( * FreeGraphsHandleType_t)(GraphInfo_t ***, uint32_t);


using pfn_rpc_mem_alloc                     = void * (*)(int, uint32_t, int);
using pfn_rpc_mem_free                      = void (*)(void *);
using pfn_rpc_mem_to_fd                     = int (*)(void *);

using _pfn_QnnSaver_initialize              = decltype(QnnSaver_initialize);
using _pfn_QnnInterface_getProviders        = decltype(QnnInterface_getProviders);
using _pfn_QnnSystemInterface_getProviders  = decltype(QnnSystemInterface_getProviders);

using PopulateInputTensorsRetType_t         = std::tuple<int, size_t, size_t>;
using ReadBatchDataRetType_t                = std::tuple<int, size_t, size_t>;
using ReadInputListRetType_t                = std::tuple<std::vector<std::vector<std::string>>, std::unordered_map<std::string, uint32_t>, bool>;
using ReadInputListsRetType_t               = std::tuple<std::vector<std::vector<std::vector<std::string>>>, std::vector<std::unordered_map<std::string, uint32_t>>, bool>;


inline int validateTensorVersion(Qnn_Tensor_t tensor) {
    if (tensor.version != QNN_TENSOR_VERSION_1) {
        LOGGW("validateTensorVersion() tensor %s, got unsupported version %d\n",
              tensor.v1.name,
              tensor.version);
        return 1;
    }
    return 0;
}


inline int validateOpConfigVersion(Qnn_OpConfig_t opConfig) {
    if (opConfig.version != QNN_OPCONFIG_VERSION_1) {
        LOGGW("validateOpConfigVersion() op %s, got unsupported version %d\n",
              opConfig.v1.name,
              opConfig.version);
        return 1;
    }
    return 0;
}


inline const char* getQnnOpConfigName(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.name;
    }
    return NULL;
}


inline const char* getQnnOpConfigName(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigName(*opConfig);
}


inline const char* getQnnOpConfigPackageName(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.packageName;
    }
    return NULL;
}


inline const char* getQnnOpConfigPackageName(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigPackageName(*opConfig);
}

inline const char* getQnnOpConfigTypeName(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.typeName;
    }
    return NULL;
}


inline const char* getQnnOpConfigTypeName(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigTypeName(*opConfig);
}

inline uint32_t getQnnOpConfigNumParams(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfParams;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumParams(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigNumParams(*opConfig);
}


inline const Qnn_Param_t* getQnnOpConfigParams(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.params;
    }
    return NULL;
}


inline const Qnn_Param_t* getQnnOpConfigParams(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigParams(*opConfig);
}


inline uint32_t getQnnOpConfigNumInputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfInputs;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumInputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigNumInputs(*opConfig);
}


inline const Qnn_Tensor_t* getQnnOpConfigInputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.inputTensors;
    }
    return NULL;
}

inline const Qnn_Tensor_t* getQnnOpConfigInputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigInputs(*opConfig);
}


inline uint32_t getQnnOpConfigNumOutputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.numOfOutputs;
    }
    return 0u;
}


inline uint32_t getQnnOpConfigNumOutputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigNumOutputs(*opConfig);
}


inline const Qnn_Tensor_t* getQnnOpConfigOutputs(const Qnn_OpConfig_t& opConfig) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        return opConfig.v1.outputTensors;
    }
    return NULL;
}


inline const Qnn_Tensor_t* getQnnOpConfigOutputs(const Qnn_OpConfig_t* opConfig) {
    return getQnnOpConfigOutputs(*opConfig);
}

inline void setQnnOpConfigName(Qnn_OpConfig_t& opConfig, const char* name) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.name = name;
    }
}


inline void setQnnOpConfigName(Qnn_OpConfig_t* opConfig, const char* name) {
    setQnnOpConfigName(*opConfig, name);
}

inline void setQnnOpConfigPackageName(Qnn_OpConfig_t& opConfig, const char* packageName) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.packageName = packageName;
    }
}


inline void setQnnOpConfigPackageName(Qnn_OpConfig_t* opConfig, const char* packageName) {
    setQnnOpConfigPackageName(*opConfig, packageName);
}


inline void setQnnOpConfigTypeName(Qnn_OpConfig_t& opConfig, const char* typeName) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.typeName = typeName;
    }
}


inline void setQnnOpConfigTypeName(Qnn_OpConfig_t* opConfig, const char* typeName) {
    setQnnOpConfigTypeName(*opConfig, typeName);
}


inline void setQnnOpConfigParams(Qnn_OpConfig_t& opConfig,
                                 uint32_t numOfParams,
                                 Qnn_Param_t* params) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfParams = numOfParams;
        opConfig.v1.params      = params;
    }
}


inline void setQnnOpConfigParams(Qnn_OpConfig_t* opConfig,
                                 uint32_t numOfParams,
                                 Qnn_Param_t* params) {
    setQnnOpConfigParams(*opConfig, numOfParams, params);
}


inline void setQnnOpConfigInputs(Qnn_OpConfig_t& opConfig,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t* inputTensors) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfInputs  = numOfInputs;
        opConfig.v1.inputTensors = inputTensors;
    }
}


inline void setQnnOpConfigInputs(Qnn_OpConfig_t* opConfig,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t* inputTensors) {
    setQnnOpConfigInputs(*opConfig, numOfInputs, inputTensors);
}


inline void setQnnOpConfigOutputs(Qnn_OpConfig_t& opConfig,
                                  uint32_t numOfOutputs,
                                  Qnn_Tensor_t* outputTensors) {
    if (opConfig.version == QNN_OPCONFIG_VERSION_1) {
        opConfig.v1.numOfOutputs  = numOfOutputs;
        opConfig.v1.outputTensors = outputTensors;
    }
}


inline void setQnnOpConfigOutputs(Qnn_OpConfig_t* opConfig,
                                  uint32_t numOfOutputs,
                                  Qnn_Tensor_t* outputTensors) {
    setQnnOpConfigOutputs(*opConfig, numOfOutputs, outputTensors);
}

inline uint32_t getQnnTensorId(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.id;
    }
    return 0u;
}


inline uint32_t getQnnTensorId(const Qnn_Tensor_t* tensor) { return getQnnTensorId(*tensor); }

inline const char* getQnnTensorName(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.name;
    }
    return 0u;
}


inline const char* getQnnTensorName(const Qnn_Tensor_t* tensor) {
    return getQnnTensorName(*tensor);
}

inline Qnn_TensorType_t getQnnTensorType(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.type;
    }
    return QNN_TENSOR_TYPE_UNDEFINED;
}

inline Qnn_TensorType_t getQnnTensorType(const Qnn_Tensor_t* tensor) {
    return getQnnTensorType(*tensor);
}


inline Qnn_TensorDataFormat_t getQnnTensorDataFormat(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dataFormat;
    }
    return QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER;
}


inline Qnn_TensorDataFormat_t getQnnTensorDataFormat(const Qnn_Tensor_t* tensor) {
    return getQnnTensorDataFormat(*tensor);
}

inline Qnn_DataType_t getQnnTensorDataType(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dataType;
    }
    return QNN_DATATYPE_UNDEFINED;
}

inline Qnn_DataType_t getQnnTensorDataType(const Qnn_Tensor_t* tensor) {
    return getQnnTensorDataType(*tensor);
}

inline Qnn_QuantizeParams_t getQnnTensorQuantParams(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.quantizeParams;
    }
    return QNN_QUANTIZE_PARAMS_INIT;
}

inline Qnn_QuantizeParams_t getQnnTensorQuantParams(const Qnn_Tensor_t* tensor) {
    return getQnnTensorQuantParams(*tensor);
}

inline uint32_t getQnnTensorRank(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.rank;
    }
    return 0u;
}

inline uint32_t getQnnTensorRank(const Qnn_Tensor_t* tensor) { return getQnnTensorRank(*tensor); }


inline uint32_t* getQnnTensorDimensions(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.dimensions;
    }
    return NULL;
}


inline uint32_t* getQnnTensorDimensions(const Qnn_Tensor_t* tensor) {
    return getQnnTensorDimensions(*tensor);
}


inline Qnn_TensorMemType_t getQnnTensorMemType(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.memType;
    }
    return QNN_TENSORMEMTYPE_UNDEFINED;
}


inline Qnn_TensorMemType_t getQnnTensorMemType(const Qnn_Tensor_t* tensor) {
    return getQnnTensorMemType(*tensor);
}

inline Qnn_ClientBuffer_t getQnnTensorClientBuf(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.clientBuf;
    }
    return QNN_CLIENT_BUFFER_INIT;
}

inline Qnn_ClientBuffer_t getQnnTensorClientBuf(const Qnn_Tensor_t* tensor) {
    return getQnnTensorClientBuf(*tensor);
}


inline Qnn_MemHandle_t getQnnTensorMemHandle(const Qnn_Tensor_t& tensor) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        return tensor.v1.memHandle;
    }
    return NULL;
}


inline Qnn_MemHandle_t getQnnTensorMemHandle(const Qnn_Tensor_t* tensor) {
    return getQnnTensorMemHandle(*tensor);
}


inline void setQnnTensorId(Qnn_Tensor_t& tensor, uint32_t id) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.id = id;
    }
}


inline void setQnnTensorId(Qnn_Tensor_t* tensor, uint32_t id) { setQnnTensorId(*tensor, id); }

inline void setQnnTensorName(Qnn_Tensor_t& tensor, const char* name) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.name = name;
    }
}


inline void setQnnTensorName(Qnn_Tensor_t* tensor, const char* name) {
    setQnnTensorName(*tensor, name);
}


inline void setQnnTensorType(Qnn_Tensor_t& tensor, Qnn_TensorType_t type) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.type = type;
    }
}


inline void setQnnTensorType(Qnn_Tensor_t* tensor, Qnn_TensorType_t type) {
    setQnnTensorType(*tensor, type);
}


inline void setQnnTensorDataFormat(Qnn_Tensor_t& tensor, Qnn_TensorDataFormat_t format) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dataFormat = format;
    }
}


inline void setQnnTensorDataFormat(Qnn_Tensor_t* tensor, Qnn_TensorDataFormat_t format) {
    setQnnTensorDataFormat(*tensor, format);
}


inline void setQnnTensorDataType(Qnn_Tensor_t& tensor, Qnn_DataType_t dataType) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dataType = dataType;
    }
}


inline void setQnnTensorDataType(Qnn_Tensor_t* tensor, Qnn_DataType_t dataType) {
    setQnnTensorDataType(*tensor, dataType);
}


inline void setQnnTensorQuantParams(Qnn_Tensor_t& tensor, Qnn_QuantizeParams_t params) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.quantizeParams = params;
    }
}


inline void setQnnTensorQuantParams(Qnn_Tensor_t* tensor, Qnn_QuantizeParams_t params) {
    setQnnTensorQuantParams(*tensor, params);
}


inline void setQnnTensorRank(Qnn_Tensor_t& tensor, uint32_t rank) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.rank = rank;
    }
}


inline void setQnnTensorRank(Qnn_Tensor_t* tensor, uint32_t rank) {
    setQnnTensorRank(*tensor, rank);
}


inline void setQnnTensorDimensions(Qnn_Tensor_t& tensor, uint32_t* dims) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.dimensions = dims;
    }
}


inline void setQnnTensorDimensions(Qnn_Tensor_t* tensor, uint32_t* dims) {
    setQnnTensorDimensions(*tensor, dims);
}


inline void setQnnTensorMemType(Qnn_Tensor_t& tensor, Qnn_TensorMemType_t memType) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.memType = memType;
    }
}


inline void setQnnTensorMemType(Qnn_Tensor_t* tensor, Qnn_TensorMemType_t memType) {
    setQnnTensorMemType(*tensor, memType);
}


inline void setQnnTensorClientBuf(Qnn_Tensor_t& tensor, Qnn_ClientBuffer_t clientBuf) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.clientBuf = clientBuf;
    }
}


inline void setQnnTensorClientBuf(Qnn_Tensor_t* tensor, Qnn_ClientBuffer_t clientBuf) {
    setQnnTensorClientBuf(*tensor, clientBuf);
}


inline void setQnnTensorMemHandle(Qnn_Tensor_t& tensor, Qnn_MemHandle_t handle) {
    if (tensor.version == QNN_TENSOR_VERSION_1) {
        tensor.v1.memHandle = handle;
    }
}


inline void setQnnTensorMemHandle(Qnn_Tensor_t* tensor, Qnn_MemHandle_t handle) {
    setQnnTensorMemHandle(*tensor, handle);
}






// Accessors for QNN Op Config
#define QNN_OP_CFG_GET_NAME(opConfig)         getQnnOpConfigName(opConfig)
#define QNN_OP_CFG_GET_PACKAGE_NAME(opConfig) getQnnOpConfigPackageName(opConfig)
#define QNN_OP_CFG_GET_TYPE_NAME(opConfig)    getQnnOpConfigTypeName(opConfig)
#define QNN_OP_CFG_GET_NUM_PARAMS(opConfig)   getQnnOpConfigNumParams(opConfig)
#define QNN_OP_CFG_GET_PARAMS(opConfig)       getQnnOpConfigParams(opConfig)
#define QNN_OP_CFG_GET_NUM_INPUTS(opConfig)   getQnnOpConfigNumInputs(opConfig)
#define QNN_OP_CFG_GET_INPUTS(opConfig)       getQnnOpConfigInputs(opConfig)
#define QNN_OP_CFG_GET_NUM_OUTPUTS(opConfig)  getQnnOpConfigNumOutputs(opConfig)
#define QNN_OP_CFG_GET_OUTPUTS(opConfig)      getQnnOpConfigOutputs(opConfig)

// Modifiers for QNN Op Config
#define QNN_OP_CFG_SET_NAME(opConfig, value)         setQnnOpConfigName(opConfig, value)
#define QNN_OP_CFG_SET_PACKAGE_NAME(opConfig, value) setQnnOpConfigPackageName(opConfig, value)
#define QNN_OP_CFG_SET_TYPE_NAME(opConfig, value)    setQnnOpConfigTypeName(opConfig, value)
#define QNN_OP_CFG_SET_PARAMS(opConfig, numOfParams, params) \
  setQnnOpConfigParams(opConfig, numOfParams, params)
#define QNN_OP_CFG_SET_INPUTS(opConfig, numOfInputs, inputTensors) \
  setQnnOpConfigInputs(opConfig, numOfInputs, inputTensors)
#define QNN_OP_CFG_SET_OUTPUTS(opConfig, numOfOutputs, outputTensors) \
  setQnnOpConfigOutputs(opConfig, numOfOutputs, outputTensors)

// Accessors for QNN Tensor
#define QNN_TENSOR_GET_ID(tensor)           getQnnTensorId(tensor)
#define QNN_TENSOR_GET_NAME(tensor)         getQnnTensorName(tensor)
#define QNN_TENSOR_GET_TYPE(tensor)         getQnnTensorType(tensor)
#define QNN_TENSOR_GET_DATA_FORMAT(tensor)  getQnnTensorDataFormat(tensor)
#define QNN_TENSOR_GET_DATA_TYPE(tensor)    getQnnTensorDataType(tensor)
#define QNN_TENSOR_GET_QUANT_PARAMS(tensor) getQnnTensorQuantParams(tensor)
#define QNN_TENSOR_GET_RANK(tensor)         getQnnTensorRank(tensor)
#define QNN_TENSOR_GET_DIMENSIONS(tensor)   getQnnTensorDimensions(tensor)
#define QNN_TENSOR_GET_MEM_TYPE(tensor)     getQnnTensorMemType(tensor)
#define QNN_TENSOR_GET_CLIENT_BUF(tensor)   getQnnTensorClientBuf(tensor)
#define QNN_TENSOR_GET_MEM_HANDLE(tensor)   getQnnTensorMemHandle(tensor)

// Modifiers for QNN Tensor
#define QNN_TENSOR_SET_ID(tensor, value)           setQnnTensorId(tensor, value)
#define QNN_TENSOR_SET_NAME(tensor, value)         setQnnTensorName(tensor, value)
#define QNN_TENSOR_SET_TYPE(tensor, value)         setQnnTensorType(tensor, value)
#define QNN_TENSOR_SET_DATA_FORMAT(tensor, value)  setQnnTensorDataFormat(tensor, value)
#define QNN_TENSOR_SET_DATA_TYPE(tensor, value)    setQnnTensorDataType(tensor, value)
#define QNN_TENSOR_SET_QUANT_PARAMS(tensor, value) setQnnTensorQuantParams(tensor, value)
#define QNN_TENSOR_SET_RANK(tensor, value)         setQnnTensorRank(tensor, value)
#define QNN_TENSOR_SET_DIMENSIONS(tensor, value)   setQnnTensorDimensions(tensor, value)
#define QNN_TENSOR_SET_MEM_TYPE(tensor, value)     setQnnTensorMemType(tensor, value)
#define QNN_TENSOR_SET_CLIENT_BUF(tensor, value)   setQnnTensorClientBuf(tensor, value)
#define QNN_TENSOR_SET_MEM_HANDLE(tensor, value)   setQnnTensorMemHandle(tensor, value)



// =================================================================================================
//
//  internal helper function
//
// =================================================================================================
static const char * get_qnn_backend_name(int n_backend_type) {
    switch (n_backend_type) {
        case 0:
            return "QNN-CPU";
        case 1:
            return "QNN-GPU";
        case 2:
            return "QNN-HTP(DSP)";
        case 3:
            return "ggml";
        default:
            return "unknown";
    }
}


static size_t memscpy(void *dst, size_t dstSize, const void *src, size_t copySize) {
    if (!dst || !src || !dstSize || !copySize) return 0;

    size_t minSize = dstSize < copySize ? dstSize : copySize;

    memcpy(dst, src, minSize);

    return minSize;
}


static char *ggml_qnn_strndup(const char *source, size_t maxlen) { return ::strndup(source, maxlen); }


static int deepCopyQnnTensors(Qnn_Tensor_t &src, Qnn_Tensor_t &dst) {
    int err = 0;
    VALIDATE_TENSOR_VERSION(src, err);

    dst.version = src.version;
    QNN_TENSOR_SET_NAME(
            dst, ggml_qnn_strndup(QNN_TENSOR_GET_NAME(src), std::string(QNN_TENSOR_GET_NAME(src)).size()));
    if (QNN_TENSOR_GET_NAME(dst) == nullptr) {
        return 1;
    }
    QNN_TENSOR_SET_ID(dst, QNN_TENSOR_GET_ID(src));
    QNN_TENSOR_SET_TYPE(dst, QNN_TENSOR_GET_TYPE(src));
    QNN_TENSOR_SET_DATA_FORMAT(dst, QNN_TENSOR_GET_DATA_FORMAT(src));
    QNN_TENSOR_SET_DATA_TYPE(dst, QNN_TENSOR_GET_DATA_TYPE(src));
    QNN_TENSOR_SET_MEM_TYPE(dst, QNN_TENSOR_GET_MEM_TYPE(src));

    // Only metadata (i.e. non-static data) is copied from source to destination. The union still
    // must be initialized so that the clientBuf/memHandle do not contain garbage data
    if (QNN_TENSOR_GET_MEM_TYPE(src) == QNN_TENSORMEMTYPE_RAW) {
        Qnn_ClientBuffer_t clientBuf = {nullptr, 0};
        QNN_TENSOR_SET_CLIENT_BUF(dst, clientBuf);
    } else if (QNN_TENSOR_GET_MEM_TYPE(src) == QNN_TENSORMEMTYPE_MEMHANDLE) {
        QNN_TENSOR_SET_MEM_HANDLE(dst, nullptr);
    } else {
        return 1;
    }

    Qnn_QuantizeParams_t srcQParam      = QNN_TENSOR_GET_QUANT_PARAMS(src);
    Qnn_QuantizationEncoding_t encoding = srcQParam.quantizationEncoding;
    if (encoding == QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
        // need to allocate and copy memory for scaleOffset as it is a pointer array
        Qnn_QuantizeParams_t srcQParamCpy      = srcQParam;
        Qnn_AxisScaleOffset_t &axisScaleOffset = srcQParamCpy.axisScaleOffsetEncoding;
        Qnn_ScaleOffset_t **scaleOffset        = &axisScaleOffset.scaleOffset;
        size_t scaleOffsetSize = axisScaleOffset.numScaleOffsets * sizeof(Qnn_ScaleOffset_t);
        *scaleOffset           = (Qnn_ScaleOffset_t *)malloc(scaleOffsetSize);
        memscpy(*scaleOffset,
                scaleOffsetSize,
                srcQParam.axisScaleOffsetEncoding.scaleOffset,
                scaleOffsetSize);
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParamCpy);
    } else if (encoding == QNN_QUANTIZATION_ENCODING_BW_AXIS_SCALE_OFFSET) {
        // need to allocate and copy memory for scaleOffset as it is a pointer array
        Qnn_QuantizeParams_t srcQParamCpy          = srcQParam;
        Qnn_BwAxisScaleOffset_t &bwAxisScaleOffset = srcQParamCpy.bwAxisScaleOffsetEncoding;
        size_t scaleSize                           = bwAxisScaleOffset.numElements * sizeof(float);
        float **scales                             = &bwAxisScaleOffset.scales;
        int32_t **offsets                          = &bwAxisScaleOffset.offsets;
        *scales                                    = (float *)malloc(scaleSize);
        memscpy(*scales, scaleSize, srcQParam.bwAxisScaleOffsetEncoding.scales, scaleSize);

        // Only copy offsets if present, nullptr implies all offsets are 0
        if (bwAxisScaleOffset.offsets != nullptr) {
            size_t offsetSize = bwAxisScaleOffset.numElements * sizeof(int32_t);
            *offsets          = (int32_t *)malloc(offsetSize);
            memscpy(*offsets, offsetSize, srcQParam.bwAxisScaleOffsetEncoding.offsets, offsetSize);
        }
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParamCpy);
    } else {
        QNN_TENSOR_SET_QUANT_PARAMS(dst, srcQParam);
    }

    // need to allocate and copy memory for all the pointer members
    uint32_t rank = QNN_TENSOR_GET_RANK(src);
    QNN_TENSOR_SET_RANK(dst, rank);
    size_t dimSize       = rank * sizeof(uint32_t);
    uint32_t *dimensions = (uint32_t *)malloc(dimSize);
    if (dimensions == nullptr) {
        LOGGW("deepCopyQnnTensors() allocation error while copying tensor %s\n", QNN_TENSOR_GET_NAME(src));
        return 1;
    }
    memscpy(dimensions, dimSize, QNN_TENSOR_GET_DIMENSIONS(src), dimSize);
    QNN_TENSOR_SET_DIMENSIONS(dst, dimensions);

    return err;
}


static bool deepCopyQnnTensorInfo(Qnn_Tensor_t *dst, const Qnn_Tensor_t *src) {
    if (nullptr == dst || nullptr == src) {
        LOGGW("Received nullptr");
        return false;
    }
    // set tensor.version before using QNN_TENSOR_SET macros, as they require the version to be set
    // to correctly assign values
    dst->version = src->version;
    const char *tensorName = QNN_TENSOR_GET_NAME(src);
    if (!tensorName) {
        QNN_TENSOR_SET_NAME(dst, nullptr);
    } else {
        QNN_TENSOR_SET_NAME(dst, strndup(tensorName, strlen(tensorName)));
    }
    QNN_TENSOR_SET_ID(dst, QNN_TENSOR_GET_ID(src));
    QNN_TENSOR_SET_TYPE(dst, QNN_TENSOR_GET_TYPE(src));
    QNN_TENSOR_SET_DATA_FORMAT(dst, QNN_TENSOR_GET_DATA_FORMAT(src));
    QNN_TENSOR_SET_DATA_TYPE(dst, QNN_TENSOR_GET_DATA_TYPE(src));
    Qnn_QuantizeParams_t qParams = QNN_QUANTIZE_PARAMS_INIT;
    qParams.encodingDefinition = QNN_TENSOR_GET_QUANT_PARAMS(src).encodingDefinition;
    qParams.quantizationEncoding = QNN_QUANTIZATION_ENCODING_UNDEFINED;
    if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
        QNN_QUANTIZATION_ENCODING_SCALE_OFFSET) {
        qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
        qParams.scaleOffsetEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).scaleOffsetEncoding;
    } else if (QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding ==
               QNN_QUANTIZATION_ENCODING_AXIS_SCALE_OFFSET) {
        qParams.quantizationEncoding = QNN_TENSOR_GET_QUANT_PARAMS(src).quantizationEncoding;
        qParams.axisScaleOffsetEncoding.axis =
                QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.axis;
        qParams.axisScaleOffsetEncoding.numScaleOffsets =
                QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
        if (QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets > 0) {
            qParams.axisScaleOffsetEncoding.scaleOffset = (Qnn_ScaleOffset_t *) malloc(
                    QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets *
                    sizeof(Qnn_ScaleOffset_t));
            if (qParams.axisScaleOffsetEncoding.scaleOffset) {
                for (size_t idx = 0;
                     idx < QNN_TENSOR_GET_QUANT_PARAMS(src).axisScaleOffsetEncoding.numScaleOffsets;
                     idx++) {
                    qParams.axisScaleOffsetEncoding.scaleOffset[idx].scale =
                            QNN_TENSOR_GET_QUANT_PARAMS(
                                    src).axisScaleOffsetEncoding.scaleOffset[idx].scale;
                    qParams.axisScaleOffsetEncoding.scaleOffset[idx].offset =
                            QNN_TENSOR_GET_QUANT_PARAMS(
                                    src).axisScaleOffsetEncoding.scaleOffset[idx].offset;
                }
            }
        }
    }
    QNN_TENSOR_SET_QUANT_PARAMS(dst, qParams);
    QNN_TENSOR_SET_RANK(dst, QNN_TENSOR_GET_RANK(src));
    QNN_TENSOR_SET_DIMENSIONS(dst, nullptr);
    if (QNN_TENSOR_GET_RANK(src) > 0) {
        QNN_TENSOR_SET_DIMENSIONS(dst,
                                  (uint32_t *) malloc(QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t)));
        if (QNN_TENSOR_GET_DIMENSIONS(dst)) {
            memscpy(QNN_TENSOR_GET_DIMENSIONS(dst),
                    QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t),
                    QNN_TENSOR_GET_DIMENSIONS(src),
                    QNN_TENSOR_GET_RANK(src) * sizeof(uint32_t));
        }
    }
    return true;
}


static int freeQnnTensor(Qnn_Tensor_t &tensor) {
    int err = 0;
    VALIDATE_TENSOR_VERSION(tensor, err);

    free((void *)QNN_TENSOR_GET_NAME(tensor));
    free(QNN_TENSOR_GET_DIMENSIONS(tensor));

    return err;
}

static int freeQnnTensors(Qnn_Tensor_t *&tensors, uint32_t numTensors) {
    int err = 0;

    // free all pointer allocations in struct
    for (size_t i = 0; i < numTensors; i++) {
        freeQnnTensor(tensors[i]);
    }
    free(tensors);

    return err;
}


static int freeGraphsInfo(GraphInfoPtr_t ** graphsInfo, uint32_t numGraphs) {
    if (graphsInfo == nullptr || * graphsInfo == nullptr) {
        return 1;
    }

    for (uint32_t i = 0; i < numGraphs; i++) {
        free((*graphsInfo)[i]->graphName);
        freeQnnTensors((*graphsInfo)[i]->inputTensors, (*graphsInfo)[i]->numInputTensors);
        freeQnnTensors((*graphsInfo)[i]->outputTensors, (*graphsInfo)[i]->numOutputTensors);
    }

    free(**graphsInfo);
    free(*graphsInfo);
    *graphsInfo = nullptr;

    return 0;
}

const std::map<Qnn_DataType_t, size_t> g_dataTypeToSize = {
        {QNN_DATATYPE_INT_8,           1},
        {QNN_DATATYPE_INT_16,          2},
        {QNN_DATATYPE_INT_32,          4},
        {QNN_DATATYPE_INT_64,          8},
        {QNN_DATATYPE_UINT_8,          1},
        {QNN_DATATYPE_UINT_16,         2},
        {QNN_DATATYPE_UINT_32,         4},
        {QNN_DATATYPE_UINT_64,         8},
        {QNN_DATATYPE_FLOAT_16,        2},
        {QNN_DATATYPE_FLOAT_32,        4},
        {QNN_DATATYPE_FLOAT_64,        8},
        {QNN_DATATYPE_SFIXED_POINT_8,  1},
        {QNN_DATATYPE_SFIXED_POINT_16, 2},
        {QNN_DATATYPE_SFIXED_POINT_32, 4},
        {QNN_DATATYPE_UFIXED_POINT_8,  1},
        {QNN_DATATYPE_UFIXED_POINT_16, 2},
        {QNN_DATATYPE_UFIXED_POINT_32, 4},
        {QNN_DATATYPE_BOOL_8,          1},
};
static std::tuple<int, size_t> getDataTypeSizeInBytes(Qnn_DataType_t dataType) {
    if (g_dataTypeToSize.find(dataType) == g_dataTypeToSize.end()) {
        LOGGW("Invalid qnn data type provided");
        return std::make_tuple(1, 0);
    }
    return std::make_tuple(0, g_dataTypeToSize.find(dataType)->second);
}


static size_t calculateElementCount(std::vector<size_t> dims) {
    if (dims.size() == 0) {
        return 0;
    }
    return std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>());
}


static std::tuple<int, size_t> calculateLength(std::vector<size_t> dims,
                                               Qnn_DataType_t dataType) {
    if (dims.size() == 0) {
        LOGGW("dims.size() is zero");
        return std::make_tuple(1, 0);
    }
    int  returnStatus = 0;
    size_t length{0};
    std::tie(returnStatus, length) = getDataTypeSizeInBytes(dataType);
    if (0 != returnStatus) {
        return std::make_tuple(returnStatus, 0);
    }
    length *= calculateElementCount(dims);
    return std::make_tuple(0, length);
}


static ReadBatchDataRetType_t readBatchData(const std::vector<std::string> &filePaths,
                                            const size_t filePathsIndexOffset,
                                            const bool loopBackToStart,
                                            const std::vector<size_t> &dims,
                                            const Qnn_DataType_t dataType,
                                            uint8_t *buffer) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return std::make_tuple(1, 0, 0);
    }
    int err = 0;
    size_t tensorLength{0};
    std::tie(err, tensorLength) = calculateLength(dims, dataType);
    if (0 != err) {
        return std::make_tuple(err, 0, 0);
    }
    size_t numInputsCopied = 0;
    size_t numBatchSize = 0;
    size_t totalLength = 0;
    size_t fileIndex = filePathsIndexOffset;
    while (true) {
        if (fileIndex >= filePaths.size()) {
            if (loopBackToStart) {
                fileIndex = fileIndex % filePaths.size();
            } else {
                numBatchSize += (tensorLength - totalLength) / (totalLength / numBatchSize);
                // pad the vector with zeros
                memset(buffer + totalLength, 0, (tensorLength - totalLength) * sizeof(char));
                break;
            }
        }
        std::ifstream in(filePaths[fileIndex], std::ifstream::binary);
        if (!in) {
            LOGGW("failed to open input file: %s", (filePaths[fileIndex]).c_str());
            return std::make_tuple(1, numInputsCopied, numBatchSize);
        }
        in.seekg(0, in.end);
        const size_t fileSize = in.tellg();
        in.seekg(0, in.beg);
        if ((tensorLength % fileSize) != 0 || fileSize > tensorLength || fileSize == 0) {
            LOGGW(
                    "Given input file %s with file size in bytes %d. If the model expects a batch size of "
                    "one, the file size should match the tensor extent: %d bytes. If the model expects a "
                    "batch size > 1, the file size should evenly divide the tensor extent: %d bytes.",
                    filePaths[fileIndex].c_str(),
                    fileSize,
                    tensorLength,
                    tensorLength);
            return std::make_tuple(1, numInputsCopied, numBatchSize);
        }
        if (!in.read(reinterpret_cast<char *>(buffer + (numInputsCopied * fileSize)), fileSize)) {
            LOGGW("Failed to read the contents of: %s", filePaths.front().c_str());
            return std::make_tuple(1, numInputsCopied, numBatchSize);
        }
        totalLength += fileSize;
        numInputsCopied += 1;
        numBatchSize += 1;
        fileIndex += 1;
        if (totalLength >= tensorLength) {
            break;
        }
    }
    return std::make_tuple(0, numInputsCopied, numBatchSize);
}


static ReadBatchDataRetType_t readBatchDataAndUpdateQueue(
        std::queue<std::string> & filePaths,
        std::vector<size_t> dims,
        Qnn_DataType_t dataType,
        uint8_t * buffer) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return std::make_tuple(1, 0, 0);
    }

    int err = 0;
    size_t len = 0;
    std::tie(err, len) = calculateLength(dims, dataType);
    if (0 != err) {
        return std::make_tuple(err, 0, 0);
    }

    size_t numInputsCopied = 0;
    size_t numBatchSize    = 0;
    size_t totalLength     = 0;
    do {
        if (filePaths.empty()) {
            numBatchSize += (len - totalLength) / (totalLength / numBatchSize);
            // pad the vector with zeros
            memset(buffer + totalLength, 0, (len - totalLength) * sizeof(char));
            totalLength = len;
        } else {
            std::ifstream in(filePaths.front(), std::ifstream::binary);
            if (!in) {
                LOGGW("failed to open input file: %s", filePaths.front().c_str());
                return std::make_tuple(1, numInputsCopied, numBatchSize);
            }
            in.seekg(0, in.end);
            const size_t length = in.tellg();
            in.seekg(0, in.beg);
            if ((len % length) != 0 || length > len || length == 0) {
                LOGGW("input file %s: file size in bytes (%d), should be multiples of: %d",
                      filePaths.front().c_str(),
                      length,
                      len);
                return std::make_tuple(1, numInputsCopied, numBatchSize);
            }
            if (!in.read(reinterpret_cast<char*>(buffer + (numInputsCopied * length)), length)) {
                LOGGW("failed to read the contents of: %s", filePaths.front().c_str());
                return std::make_tuple(1, numInputsCopied, numBatchSize);
            }
            LOGGI("return from readDataFromFile()");
            totalLength += length;
            numInputsCopied += 1;
            numBatchSize += 1;
            filePaths.pop();
        }
    } while (totalLength < len);

    return std::make_tuple(0, numInputsCopied, numBatchSize);
}


static std::tuple<int, size_t> getFileSize(std::string & filePath) {
    std::ifstream in(filePath, std::ifstream::binary);
    if (!in) {
        LOGGW("failed to open input file: %s", filePath.c_str());
        return std::make_tuple(1, 0);
    }
    in.seekg(0, in.end);
    const size_t length = in.tellg();
    in.seekg(0, in.beg);

    return std::make_tuple(0, length);
}


static int readBinaryFromFile(std::string & filePath, uint8_t * buffer, size_t bufferSize) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    std::ifstream in(filePath, std::ifstream::binary);
    if (!in) {
        LOGGW("failed to open input file: %s", filePath.c_str());
        return 1;
    }
    if (!in.read(reinterpret_cast<char*>(buffer), bufferSize)) {
        LOGGW("failed to read the contents of: %s", filePath.c_str());
        return 1;
    }

    return 0;
}


static int writeDataToFile(std::string fileDir,
                           std::string fileName,
                           std::vector<size_t> dims,
                           Qnn_DataType_t dataType,
                           uint8_t * buffer) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    if (!mkdir(fileDir.c_str(), 0777)) {
        LOGGW("Failed to create output directory: %s", fileDir.c_str());
        return 2;
    }
    const std::string outputPath(fileDir + "/" + fileName);
    std::ofstream os(outputPath, std::ofstream::binary);
    if (!os) {
        LOGGW("Failed to open output file for writing: %s", outputPath.c_str());
        return 3;
    }
    int err = 0;
    size_t length = 0;
    std::tie(err, length) = calculateLength(dims, dataType);
    if (0 != err) {
        LOGGW("failed to get length of data");
        return err;
    }
    for (size_t l = 0; l < length; l++) {
        os.write(reinterpret_cast<char*>(&(*(buffer + l))), 1);
    }

    return 0;
}


static int writeBatchDataToFile(std::vector<std::string> fileDirs,
                                std::string fileName,
                                std::vector<size_t> dims,
                                Qnn_DataType_t dataType,
                                uint8_t * buffer,
                                const size_t batchSize) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    int err = 0;
    size_t length{0};
    std::tie(err, length) = calculateLength(dims, dataType);
    if (0 != err) {
        return err;
    }
    auto outputSize = (length / batchSize);
    for (size_t batchIndex = 0; batchIndex < fileDirs.size(); batchIndex++) {
        std::string fileDir = fileDirs[batchIndex];
        if (0 == mkdir(fileDir.c_str(), 0777)) {
            LOGGW("Failed to create output directory: %s", fileDir.c_str());
            return 1;
        }
        const std::string outputPath(fileDir + "/" + fileName);
        std::ofstream os(outputPath, std::ofstream::binary);
        if (!os) {
            LOGGW("Failed to open output file for writing: %s", outputPath.c_str());
            return 1;
        }
        for (size_t l = 0; l < outputSize; l++) {
            size_t bufferIndex = l + (batchIndex * outputSize);
            os.write(reinterpret_cast<char *>(&(*(buffer + bufferIndex))), 1);
        }
    }
    return 0;
}


static int writeBinaryToFile(std::string fileDir,
                             std::string fileName,
                             uint8_t * buffer,
                             size_t bufferSize) {
    if (nullptr == buffer) {
        LOGGW("buffer is nullptr");
        return 1;
    }
    if (!mkdir(fileDir.c_str(), 0777)) {
        LOGGW("Failed to create output directory: %s", fileDir.c_str());
        return 1;
    }
    const std::string outputPath(fileDir + "/" + fileName);
    std::ofstream os(outputPath, std::ofstream::binary);
    if (!os) {
        LOGGW("Failed to open output file for writing: %s", outputPath.c_str());
        return 1;
    }
    os.write(reinterpret_cast<char *>(buffer), bufferSize);
    return 0;
}


template<typename T_QuantType>
int floatToTfN(
        T_QuantType *out, float *in, int32_t offset, float scale, size_t numElements) {
    static_assert(std::is_unsigned<T_QuantType>::value, "floatToTfN supports unsigned only!");

    if (nullptr == out || nullptr == in) {
        LOGGW("received a nullptr");
        return 1;
    }

    size_t dataTypeSizeInBytes = sizeof(T_QuantType);
    size_t bitWidth = dataTypeSizeInBytes * 8;;
    double trueBitWidthMax = pow(2, bitWidth) - 1;
    double encodingMin = offset * scale;
    double encodingMax = (trueBitWidthMax + offset) * scale;
    double encodingRange = encodingMax - encodingMin;

    for (size_t i = 0; i < numElements; ++i) {
        int quantizedValue = round(trueBitWidthMax * (in[i] - encodingMin) / encodingRange);
        if (quantizedValue < 0)
            quantizedValue = 0;
        else if (quantizedValue > (int) trueBitWidthMax)
            quantizedValue = (int) trueBitWidthMax;
        out[i] = static_cast<T_QuantType>(quantizedValue);
    }
    return 0;
}

template int floatToTfN<uint8_t>(
        uint8_t *out, float *in, int32_t offset, float scale, size_t numElements);

template int floatToTfN<uint16_t>(
        uint16_t *out, float *in, int32_t offset, float scale, size_t numElements);

template<typename T_QuantType>
int tfNToFloat(
        float *out, T_QuantType *in, int32_t offset, float scale, size_t numElements) {
    static_assert(std::is_unsigned<T_QuantType>::value, "tfNToFloat supports unsigned only!");

    if (nullptr == out || nullptr == in) {
        LOGGW("received a nullptr");
        return 1;
    }
    for (size_t i = 0; i < numElements; i++) {
        double quantizedValue = static_cast<double>(in[i]);
        double offsetDouble = static_cast<double>(offset);
        out[i] = static_cast<double>((quantizedValue + offsetDouble) * scale);
    }
    return 0;
}

template int tfNToFloat<uint8_t>(
        float *out, uint8_t *in, int32_t offset, float scale, size_t numElements);

template int tfNToFloat<uint16_t>(
        float *out, uint16_t *in, int32_t offset, float scale, size_t numElements);

template<typename T_QuantType>
int castToFloat(float *out, T_QuantType *in, size_t numElements) {
    if (nullptr == out || nullptr == in) {
        LOGGW("Received a nullptr");
        return 1;
    }
    for (size_t i = 0; i < numElements; i++) {
        out[i] = static_cast<float>(in[i]);
    }
    return 0;
}

template int castToFloat<uint8_t>(float *out,
                                  uint8_t *in,
                                  size_t numElements);

template int castToFloat<uint16_t>(float *out,
                                   uint16_t *in,
                                   size_t numElements);

template int castToFloat<uint32_t>(float *out,
                                   uint32_t *in,
                                   size_t numElements);

template int castToFloat<int8_t>(float *out,
                                 int8_t *in,
                                 size_t numElements);

template int castToFloat<int16_t>(float *out,
                                  int16_t *in,
                                  size_t numElements);

template int castToFloat<int32_t>(float *out,
                                  int32_t *in,
                                  size_t numElements);

template<typename T_QuantType>
int castFromFloat(T_QuantType *out, float *in, size_t numElements) {
    if (nullptr == out || nullptr == in) {
        LOGGW("received a nullptr");
        return 1;
    }
    for (size_t i = 0; i < numElements; i++) {
        out[i] = static_cast<T_QuantType>(in[i]);
    }
    return 0;
}

template int castFromFloat<uint8_t>(uint8_t *out,
                                    float *in,
                                    size_t numElements);

template int castFromFloat<uint16_t>(uint16_t *out,
                                     float *in,
                                     size_t numElements);

template int castFromFloat<uint32_t>(uint32_t *out,
                                     float *in,
                                     size_t numElements);

template int castFromFloat<int8_t>(int8_t *out,
                                   float *in,
                                   size_t numElements);

template int castFromFloat<int16_t>(int16_t *out,
                                    float *in,
                                    size_t numElements);

template int castFromFloat<int32_t>(int32_t *out,
                                    float *in,
                                    size_t numElements);


static intptr_t alignTo(size_t alignment, intptr_t offset) {
    return offset % alignment == 0 ? offset
                                   : offset +
                                     (static_cast<intptr_t>(alignment) -
                                      offset % static_cast<intptr_t>(alignment));
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
                    //LOGGD("\n");
                }
            }
        }
    }
    //LOGGD("\n");
    return sum;
}

static void tensor_dump(const ggml_tensor * tensor, const char * name) {
    LOGGD("dump ggml tensor %s", name);
    GGML_JNI_NOTIFY("dump ggml tensor %s",name);
    LOGGD("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)", name,
          tensor->type, ggml_type_name(tensor->type),
          tensor->ne[0], tensor->ne[1], tensor->ne[2], tensor->nb[0], tensor->nb[1], tensor->nb[2]);
    float sum = tensor_sum_elements(tensor);

    LOGGD("\n");
    //LOGGD("Sum of tensor %s is %6.2f\n", name, sum);
}



template<typename Fn>
Fn load_qnn_functionpointers(void * handle, const char * function_name) {
    return reinterpret_cast<Fn>(dlsym(handle, function_name));
}


static void ggml_qnn_create_directory(const std::string& path) {
    if (path.empty()) {
        LOGGD("invalid param\n");
        return;
    }
    std::size_t pos = path.find_last_of('/');
    std::string subdir = (std::string::npos == pos) ? "" : path.substr(0, pos);
    if (subdir.empty() || subdir == "." || subdir == "..") {
        return;
    }
    ggml_qnn_create_directory(subdir);
    int mkdir_err = mkdir(subdir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
    if (mkdir_err != 0 && errno != EEXIST) {
        std::string err_msg = "failed to create " + subdir + " folder\n";
        LOGGW(err_msg.c_str());
    }
}

static std::string sanitizeTensorName(std::string name) {
    std::string sanitizedName = std::regex_replace(name, std::regex("\\W+"), "_");
    if (!std::isalpha(sanitizedName[0]) && sanitizedName[0] != '_') {
        sanitizedName = "_" + sanitizedName;
    }
    return sanitizedName;
}

static void split(std::vector<std::string> &splitString,
                  const std::string &tokenizedString,
                  const char separator) {
    splitString.clear();
    std::istringstream tokenizedStringStream(tokenizedString);
    while (!tokenizedStringStream.eof()) {
        std::string value;
        getline(tokenizedStringStream, value, separator);
        if (!value.empty()) {
            splitString.push_back(value);
        }
    }
}
static std::unordered_map<std::string, uint32_t> extractInputNameIndices(
        const std::string &inputLine, const std::string &separator) {
    std::vector<std::string> inputFilePaths;
    std::unordered_map<std::string, uint32_t> inputNameToIndex;
    split(inputFilePaths, inputLine, ' ');
    size_t inputCount = 0;
    for (uint32_t idx = 0; idx < inputFilePaths.size(); idx++) {
        auto position = inputFilePaths[idx].find(separator);
        if (position != std::string::npos) {
            auto unsanitizedTensorName = inputFilePaths[idx].substr(0, position);
            auto sanitizedTensorName = sanitizeTensorName(unsanitizedTensorName);
            if (sanitizedTensorName != unsanitizedTensorName) {
                inputNameToIndex[unsanitizedTensorName] = idx;
            }
            inputNameToIndex[sanitizedTensorName] = idx;
            inputCount = inputCount + 1;
        }
    }
    return inputCount == inputFilePaths.size() ? inputNameToIndex
                                               : std::unordered_map<std::string, uint32_t>{};
}


static void parseInputFilePaths(std::vector<std::string> &inputFilePaths,
                                std::vector<std::string> &paths,
                                std::string separator) {
    for (auto &inputInfo : inputFilePaths) {
        auto position = inputInfo.find(separator);
        if (position != std::string::npos) {
            auto path = inputInfo.substr(position + separator.size());
            paths.push_back(path);
        } else {
            paths.push_back(inputInfo);
        }
    }
}


static ReadInputListRetType_t readInputList(const std::string inputFileListPath) {
    std::queue<std::string> lines;
    std::ifstream fileListStream(inputFileListPath);
    if (!fileListStream) {
        LOGGW("failed to open input file: %s", inputFileListPath.c_str());
        return std::make_tuple(std::vector<std::vector<std::string>>{},
                               std::unordered_map<std::string, uint32_t>{},
                               false);
    }

    std::string fileLine;
    while (std::getline(fileListStream, fileLine)) {
        if (fileLine.empty()) continue;
        lines.push(fileLine);
    }

    if (!lines.empty() && lines.front().compare(0, 1, "#") == 0) {
        lines.pop();
    }

    if (!lines.empty() && lines.front().compare(0, 1, "%") == 0) {
        lines.pop();
    }

    std::string separator = ":=";
    std::vector<std::vector<std::string>> filePathsList;
    std::unordered_map<std::string, uint32_t> inputNameToIndex;
    if (!lines.empty()) {
        inputNameToIndex = extractInputNameIndices(lines.front(), separator);
    }
    while (!lines.empty()) {
        std::vector<std::string> paths{};
        std::vector<std::string> inputFilePaths;
        split(inputFilePaths, lines.front(), ' ');
        parseInputFilePaths(inputFilePaths, paths, separator);
        filePathsList.reserve(paths.size());
        for (size_t idx = 0; idx < paths.size(); idx++) {
            if (idx >= filePathsList.size()) {
                filePathsList.push_back(std::vector<std::string>());
            }
            filePathsList[idx].push_back(paths[idx]);
        }
        lines.pop();
    }
    return std::make_tuple(filePathsList, inputNameToIndex, true);
}


static ReadInputListsRetType_t readInputLists(
        std::vector<std::string> inputFileListPaths) {
    std::vector<std::vector<std::vector<std::string>>> filePathsLists;
    std::vector<std::unordered_map<std::string, uint32_t>> inputNameToIndexMaps;
    for (auto const &path : inputFileListPaths) {
        bool readSuccess;
        std::vector<std::vector<std::string>> filePathList;
        std::unordered_map<std::string, uint32_t> inputNameToIndex;
        std::tie(filePathList, inputNameToIndex, readSuccess) = readInputList(path);
        if (!readSuccess) {
            filePathsLists.clear();
            return std::make_tuple(filePathsLists, inputNameToIndexMaps, false);
        }
        filePathsLists.push_back(filePathList);
        inputNameToIndexMaps.push_back(inputNameToIndex);
    }
    return std::make_tuple(filePathsLists, inputNameToIndexMaps, true);
}


// =================================================================================================
//
//  wrapper class of Qualcomm QNN(Qualcomm Neural Network, aka Qualcomm AI Engine Direct) SDK
//
// =================================================================================================
class qnn_interface {

#define DEFINE_SHIM_FUNCTION_INTERFACE(F, pointer_name)           \
  template <typename... Args>                                     \
  inline auto qnn_##F(Args... args) const {                       \
    return (_qnn_interface->QNN_INTERFACE_VER_NAME.pointer_name)( \
        std::forward<Args>(args)...);                             \
  }


#define DEFINE_SHIM_FUNCTION_SYS_INTERFACE(F, pointer_name)                  \
  template <typename... Args>                                                \
  inline auto qnn_##F(Args... args) const {                                  \
    return (_qnn_sys_interface->QNN_SYSTEM_INTERFACE_VER_NAME.pointer_name)( \
        std::forward<Args>(args)...);                                        \
  }

    friend class qnn_implementation;

public:
    qnn_interface() = default;

    // QnnBackend
    DEFINE_SHIM_FUNCTION_INTERFACE(backend_create, backendCreate);
    DEFINE_SHIM_FUNCTION_INTERFACE(backend_free, backendFree);
    DEFINE_SHIM_FUNCTION_INTERFACE(backend_register_op_package, backendRegisterOpPackage);
    DEFINE_SHIM_FUNCTION_INTERFACE(backend_validate_op_config, backendValidateOpConfig);
    DEFINE_SHIM_FUNCTION_INTERFACE(backend_get_api_version, backendGetApiVersion);

    // QnnDevice
    DEFINE_SHIM_FUNCTION_INTERFACE(device_create, deviceCreate);
    DEFINE_SHIM_FUNCTION_INTERFACE(device_free, deviceFree);
    DEFINE_SHIM_FUNCTION_INTERFACE(device_get_infrastructure, deviceGetInfrastructure);
    DEFINE_SHIM_FUNCTION_INTERFACE(device_get_platform_info, deviceGetPlatformInfo);
    DEFINE_SHIM_FUNCTION_INTERFACE(device_get_info, deviceGetInfo);

    // QnnContext
    DEFINE_SHIM_FUNCTION_INTERFACE(context_create, contextCreate);
    DEFINE_SHIM_FUNCTION_INTERFACE(context_get_binary_size, contextGetBinarySize);
    DEFINE_SHIM_FUNCTION_INTERFACE(context_get_binary, contextGetBinary);
    DEFINE_SHIM_FUNCTION_INTERFACE(context_create_from_binary, contextCreateFromBinary);
    DEFINE_SHIM_FUNCTION_INTERFACE(context_free, contextFree);

    // QnnGraph
    DEFINE_SHIM_FUNCTION_INTERFACE(graph_create, graphCreate);
    DEFINE_SHIM_FUNCTION_INTERFACE(graph_add_node, graphAddNode);
    DEFINE_SHIM_FUNCTION_INTERFACE(graph_finalize, graphFinalize);
    DEFINE_SHIM_FUNCTION_INTERFACE(graph_execute, graphExecute);
    DEFINE_SHIM_FUNCTION_INTERFACE(graph_retrieve, graphRetrieve);

    // QnnLog
    DEFINE_SHIM_FUNCTION_INTERFACE(log_create, logCreate);
    DEFINE_SHIM_FUNCTION_INTERFACE(log_free, logFree);
    DEFINE_SHIM_FUNCTION_INTERFACE(log_set_log_level, logSetLogLevel);

    // QnnProfile
    DEFINE_SHIM_FUNCTION_INTERFACE(profile_create, profileCreate);
    DEFINE_SHIM_FUNCTION_INTERFACE(profile_get_events, profileGetEvents);
    DEFINE_SHIM_FUNCTION_INTERFACE(profile_get_sub_events, profileGetSubEvents);
    DEFINE_SHIM_FUNCTION_INTERFACE(profile_get_event_data, profileGetEventData);
    DEFINE_SHIM_FUNCTION_INTERFACE(profile_free, profileFree);

    // QnnMem
    DEFINE_SHIM_FUNCTION_INTERFACE(mem_register, memRegister);
    DEFINE_SHIM_FUNCTION_INTERFACE(mem_de_register, memDeRegister);

    // QnnProperty
    DEFINE_SHIM_FUNCTION_INTERFACE(property_has_capability, propertyHasCapability);

    // QnnTensor
    DEFINE_SHIM_FUNCTION_INTERFACE(tensor_create_context_tensor, tensorCreateContextTensor);
    DEFINE_SHIM_FUNCTION_INTERFACE(tensor_create_graph_tensor, tensorCreateGraphTensor);

    // QnnSystem
    DEFINE_SHIM_FUNCTION_SYS_INTERFACE(system_context_create, systemContextCreate);
    DEFINE_SHIM_FUNCTION_SYS_INTERFACE(system_context_get_binary_info, systemContextGetBinaryInfo);
    DEFINE_SHIM_FUNCTION_SYS_INTERFACE(system_context_free, systemContextFree);

    void set_qnn_interface(const QnnInterface_t * qnn_interface) {
        _qnn_interface = qnn_interface;
    }

    void set_qnn_system_interface(const QnnSystemInterface_t * qnn_sys_interface) {
        _qnn_sys_interface = qnn_sys_interface;
    }

    uint32_t get_backend_id() const {
        return _qnn_interface->backendId;
    }

    bool is_loaded() const {
        return ((_qnn_sys_interface != nullptr) && (_qnn_interface != nullptr));
    }

private:
    const QnnInterface_t * _qnn_interface           = nullptr;

    const QnnSystemInterface_t * _qnn_sys_interface = nullptr;
};


class qnn_io_tensor {
public:
    int setupInputAndOutputTensors(Qnn_Tensor_t ** inputs,
                                   Qnn_Tensor_t ** outputs,
                                   GraphInfo_t graphInfo);

    int  writeOutputTensors(uint32_t graphIdx,
                            size_t startIdx,
                            char *graphName,
                            Qnn_Tensor_t *outputs,
                            uint32_t numOutputs,
                            OutputDataType outputDatatype,
                            uint32_t graphsCount,
                            std::string outputPath,
                            size_t numInputFilesPopulated,
                            size_t outputBatchSize);

    PopulateInputTensorsRetType_t populateInputTensors(
            uint32_t graphIdx,
            const std::vector<std::vector<std::string>> &filePathsVector,
            const size_t filePathsIndexOffset,
            const bool loopBackToStart,
            const std::unordered_map<std::string, uint32_t> &inputNameToIndex,
            Qnn_Tensor_t *inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType);

    int  tearDownInputAndOutputTensors(Qnn_Tensor_t *inputs,
                                       Qnn_Tensor_t *outputs,
                                       size_t numInputTensors,
                                       size_t numOutputTensors);


    // Helper method to read data from files to a buffer.
    int readDataAndAllocateBuffer(
            std::queue<std::string>& filePaths,
            std::vector<size_t> dims,
            Qnn_DataType_t dataType,
            uint8_t** bufferToCopy) {
        int returnStatus = 0;
        *bufferToCopy           = nullptr;
        returnStatus            = allocateBuffer(bufferToCopy, dims, dataType);
        if (0 == returnStatus) {
            int status = 0;
            std::tie(status, m_numFilesPopulated, m_batchSize) = readBatchDataAndUpdateQueue(
                    filePaths, dims, dataType, reinterpret_cast<uint8_t*>(*bufferToCopy));
            if (0 != status) {
                LOGGW("failed to readBatchDataAndUpdateQueue");
                returnStatus = 1;
            }
        }
        if (0 != returnStatus) {
            if (nullptr != *bufferToCopy) {
                free(*bufferToCopy);
                *bufferToCopy = nullptr;
            }
        }

        return returnStatus;
    }


    // Helper method to populate an input tensor in the graph during execution.
// It relies on reading data from files provided during app creation.
    int populateInputTensor(
            std::queue<std::string>& filePaths,
            Qnn_Tensor_t* input,
            InputDataType inputDataType) {
        if (nullptr == input) {
            LOGGW("input is nullptr");
            return 1;
        }

        int  returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));

        if (inputDataType == InputDataType::FLOAT &&
            QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
            uint8_t* fileToBuffer = nullptr;
            returnStatus = readDataAndAllocateBuffer(filePaths, dims, QNN_DATATYPE_FLOAT_32, &fileToBuffer);
            if (0 == returnStatus) {
                LOGGW("readDataFromFileToBuffer successful");
                returnStatus = copyFromFloatToNative(reinterpret_cast<float*>(fileToBuffer), input);
            }
            if (nullptr != fileToBuffer) {
                free(fileToBuffer);
                fileToBuffer = nullptr;
            }
        } else {
            int status = 0;
            std::tie(status, m_numFilesPopulated, m_batchSize) = readBatchDataAndUpdateQueue(
                    filePaths,
                    dims,
                    QNN_TENSOR_GET_DATA_TYPE(input),
                    static_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(input).data));
            if (0 != status) {
                LOGGW("Failure in datautil::readBatchDataAndUpdateQueue");
                returnStatus = 1;
            }
        }
        return returnStatus;
    }

    // Helper method to populate all input tensors during execution.
    int populateInputTensors(
            uint32_t graphIdx,
            std::vector<std::queue<std::string>>& filePathsQueue,
            Qnn_Tensor_t* inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType) {
        LOGGW("populateInputTensors() graphIndx %d", graphIdx);
        if (nullptr == inputs) {
            LOGGW("inputs is nullptr");
            return 1;
        }
        auto inputCount = graphInfo.numInputTensors;
        if (filePathsQueue.size() != inputCount) {
            LOGGW(
                    "Incorrect amount of Input files for graphIdx: %d. Expected: %d, "
                    "received: %d",
                    graphIdx,
                    inputCount,
                    filePathsQueue.size());
            return 1;
        }

        for (size_t inputIdx = 0; inputIdx < inputCount; inputIdx++) {
            if (0 !=
                populateInputTensor(filePathsQueue[inputIdx], &(inputs[inputIdx]), inputDataType)) {
                LOGGW("populateInputTensor() failure for input: %d", inputIdx);
                return 1;
            }
        }
        return 0;
    }

    // Helper method to populate an input tensor in the graph during execution.
    // It relies on reading data from buffer provided during executeGraph() call.
    int populateInputTensor(
            uint8_t* buffer, Qnn_Tensor_t* input, InputDataType inputDataType) {
        if (nullptr == input) {
            LOGGW("input is nullptr");
            return 1;
        }
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));
        if (inputDataType == InputDataType::FLOAT &&
            QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
            LOGGW("Received FLOAT input, but model needs non-float input");
            if (0 != copyFromFloatToNative(reinterpret_cast<float*>(buffer), input)) {
                LOGGW("copyFromFloatToNative failure");
                return 1;
            }
        } else {
            size_t length;
            int returnStatus;
            std::tie(returnStatus, length) = calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(input));
            if (0 != returnStatus) {
                LOGGW("failed to calculate length");
                return 1;
            }
            memscpy(reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(input).data), length, buffer, length);
        }
        return 0;
    }

    // Helper method to populate all input tensors.
    int populateInputTensors(
            uint32_t graphIdx,
            std::vector<uint8_t*> inputBuffers,
            Qnn_Tensor_t* inputs,
            GraphInfo_t graphInfo,
            InputDataType inputDataType) {
        if (nullptr == inputs) {
            LOGGW("inputs is nullptr");
            return 1;
        }
        auto inputCount = graphInfo.numInputTensors;
        if (inputBuffers.size() != inputCount) {
            LOGGW("Incorrect amount of Input Buffers for graphIdx: %d. Expected: %d, received: %d",
                  graphIdx,
                  inputCount,
                  inputBuffers.size());
            return 1;
        }
        for (size_t inputIdx = 0; inputIdx < inputCount; inputIdx++) {
            if (0 !=
                populateInputTensor(inputBuffers[inputIdx], &(inputs[inputIdx]), inputDataType)) {
                LOGGW("populateInputTensor() failure for input: %d", inputIdx);
                return 1;
            }
        }
        return 0;
    }

    // Helper method to allocate a buffer.
    template <typename T>
    int allocateBuffer(T ** buffer, size_t & elementCount) {
        LOGGD("elementCount: %d, sizeof(T): %d, total size: %d",
              elementCount,
              sizeof(T),
              elementCount * sizeof(T));
        *buffer = (T*)malloc(elementCount * sizeof(T));
        if (nullptr == *buffer) {
            LOGGW("mem alloc failed for *buffer");
            return 1;
        }

        return 0;
    }

    //Helper method convert output to floatbuffer
    int converQnntensortoFloatBuffer(Qnn_Tensor_t* output,float** floatBuffer){
        int result = 0;
        if (nullptr == output) {
            return 1;
        }

        result     = convertToFloat(floatBuffer, output);
        if (0 != result) {
            return 2;
        }
        return result;
    }

    // Helper method to convert Output tensors to float and write them to files.
    int convertAndWriteOutputTensorInFloat(
            Qnn_Tensor_t* output, std::vector<std::string> outputPaths, std::string fileName) {
        if (nullptr == output) {
            return 1;
        }

        auto returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        float* floatBuffer = nullptr;
        returnStatus       = convertToFloat(&floatBuffer, output);
        if (0 != returnStatus) {
            LOGGD("failure in convertToFloat");
            return 1;
        }
        uint8_t* bufferToWrite = reinterpret_cast<uint8_t*>(floatBuffer);
        if (0 != writeBatchDataToFile(
                outputPaths, fileName, dims, QNN_DATATYPE_FLOAT_32, bufferToWrite, m_batchSize)) {
            LOGGW("failure in writeBatchDataToFile");
            returnStatus = 1;
        }
        if (nullptr != floatBuffer) {
            LOGGD("freeing floatBuffer");
            free(floatBuffer);
            floatBuffer = nullptr;
        }
        return returnStatus;
    }


    // Helper method to write out output. There is no de-quantization here.
    // Just write output as is to files
    int writeOutputTensor(Qnn_Tensor_t* output,
                          std::vector<std::string> outputPaths,
                          std::string & fileName) {
        if (nullptr == output) {
            LOGGW("output is nullptr");
            return 1;
        }
        int returnStatus = 0;
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
        uint8_t* bufferToWrite = reinterpret_cast<uint8_t*>(QNN_TENSOR_GET_CLIENT_BUF(output).data);
        if (0 != writeBatchDataToFile(outputPaths,
                                      fileName,
                                      dims,
                                      QNN_TENSOR_GET_DATA_TYPE(output),
                                      bufferToWrite,
                                      m_batchSize)) {
            LOGGW("failure in writeBatchDataToFile");
            returnStatus = 1;
        }
        return returnStatus;
    }



private:
    size_t m_batchSize          = 0;
    size_t m_numFilesPopulated  = 0;

    PopulateInputTensorsRetType_t
    populateInputTensor(const std::vector<std::string> &filePaths,
                        const size_t filePathsIndexOffset,
                        const bool loopBackToStart,
                        Qnn_Tensor_t *input,
                        InputDataType inputDataType);

    PopulateInputTensorsRetType_t
    readDataAndAllocateBuffer(const std::vector<std::string> &filePaths,
                              const size_t filePathsIndexOffset,
                              const bool loopBackToStart,
                              std::vector<size_t> dims,
                              Qnn_DataType_t dataType,
                              uint8_t **bufferToCopy);



    int  convertToFloat(float ** out, Qnn_Tensor_t * output);

    int  convertAndWriteOutputTensorInFloat(Qnn_Tensor_t *output,
                                            std::vector<std::string> outputPaths,
                                            std::string fileName,
                                            size_t outputBatchSize);

    int  writeOutputTensor(Qnn_Tensor_t *output,
                           std::vector<std::string> outputPaths,
                           std::string fileName,
                           size_t outputBatchSize);

    int  allocateAndCopyBuffer(uint8_t **buffer, Qnn_Tensor_t *tensor);

    int  tearDownTensors(Qnn_Tensor_t *tensors, uint32_t tensorCount);

    int  allocateBuffer(uint8_t **buffer, std::vector<size_t> dims, Qnn_DataType_t dataType);

    int  copyFromFloatToNative(float *floatBuffer, Qnn_Tensor_t *tensor);

    int  setupTensors(Qnn_Tensor_t **tensors, uint32_t tensorCount,
                      Qnn_Tensor_t *tensorsInfo);

    int fillDims(std::vector<size_t> &dims, uint32_t *inDimensions, uint32_t rank);
};


// Helper method to read data from files to a buffer.
PopulateInputTensorsRetType_t qnn_io_tensor::readDataAndAllocateBuffer(
        const std::vector<std::string> &filePaths,
        const size_t filePathsIndexOffset,
        const bool loopBackToStart,
        std::vector<size_t> dims,
        Qnn_DataType_t dataType,
        uint8_t **bufferToCopy) {
    int returnStatus = 0;
    *bufferToCopy = nullptr;
    returnStatus = allocateBuffer(bufferToCopy, dims, dataType);
    size_t numFilesPopulated = 0;
    size_t batchSize = 0;
    int status = 0;
    std::tie(status, numFilesPopulated, batchSize) =
            readBatchData(filePaths,
                          filePathsIndexOffset,
                          loopBackToStart,
                          dims,
                          dataType,
                          reinterpret_cast<uint8_t *>(*bufferToCopy));
    if (0 != status) {
        LOGGD("readBatchData failure\n");
        returnStatus = 1;
    }
    if (0 != returnStatus) {
        if (nullptr != *bufferToCopy) {
            free(*bufferToCopy);
            *bufferToCopy = nullptr;
        }
    }
    return {returnStatus, numFilesPopulated, batchSize};
}


// helper method to copy a float buffer, quantize it, and copy it to a tensor (Qnn_Tensor_t) buffer.
int qnn_io_tensor::copyFromFloatToNative(float * floatBuffer, Qnn_Tensor_t * tensor) {
    if (nullptr == floatBuffer || nullptr == tensor) {
        LOGGW("copyFromFloatToNative(): received a nullptr");
        return 1;
    }

    int returnStatus = 0;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));

    switch (QNN_TENSOR_GET_DATA_TYPE(tensor)) {
        case QNN_DATATYPE_UFIXED_POINT_8:
            floatToTfN<uint8_t>(
                    static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                    floatBuffer,
                    QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                    QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                    calculateElementCount(dims));
            break;

        case QNN_DATATYPE_UFIXED_POINT_16:
            floatToTfN<uint16_t>(
                    static_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                    floatBuffer,
                    QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                    QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                    calculateElementCount(dims));
            break;

        case QNN_DATATYPE_UINT_8:
            if ( 0  !=
                 castFromFloat<uint8_t>(
                         static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<uint8_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_16:
            if ( 0  !=
                 castFromFloat<uint16_t>(
                         static_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<uint16_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_32:
            if ( 0  !=
                 castFromFloat<uint32_t>(
                         static_cast<uint32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<uint32_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_8:
            if ( 0  !=
                 castFromFloat<int8_t>(
                         static_cast<int8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<int8_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_16:
            if ( 0  !=
                 castFromFloat<int16_t>(
                         static_cast<int16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<int16_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_32:
            if ( 0  !=
                 castFromFloat<int32_t>(
                         static_cast<int32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<int32_t>");
                returnStatus =  1 ;
            }
            break;

        case QNN_DATATYPE_BOOL_8:
            if ( 0  !=
                 castFromFloat<uint8_t>(
                         static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         floatBuffer,
                         calculateElementCount(dims))) {
                LOGGW("failure in castFromFloat<bool>");
                returnStatus =  1 ;
            }
            break;

        default:
            LOGGW("Datatype not supported yet!");
            returnStatus =  1 ;
            break;
    }
    return returnStatus;
}


// Helper method to populate an input tensor in the graph during execution.
// It relies on reading data from files provided during app creation.
PopulateInputTensorsRetType_t qnn_io_tensor::populateInputTensor(
        const std::vector<std::string> &filePaths,
        const size_t filePathsIndexOffset,
        const bool loopBackToStart,
        Qnn_Tensor_t *input,
        InputDataType inputDataType) {
    if (nullptr == input) {
        LOGGW("input is nullptr");
        return { 1 , 0, 0};
    }

    auto returnStatus =  0 ;
    size_t numFilesPopulated = 0;
    size_t batchSize = 0;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(input), QNN_TENSOR_GET_RANK(input));

    if (inputDataType == InputDataType::FLOAT &&
        QNN_TENSOR_GET_DATA_TYPE(input) != QNN_DATATYPE_FLOAT_32) {
        uint8_t *fileToBuffer = nullptr;
        std::tie(returnStatus, numFilesPopulated, batchSize) =
                readDataAndAllocateBuffer(filePaths,
                                          filePathsIndexOffset,
                                          loopBackToStart,
                                          dims,
                                          QNN_DATATYPE_FLOAT_32,
                                          &fileToBuffer);
        if ( 0  == returnStatus) {
            LOGGD("readDataFromFileToBuffer successful");
            returnStatus = copyFromFloatToNative(reinterpret_cast<float *>(fileToBuffer), input);
        }
        if (nullptr != fileToBuffer) {
            free(fileToBuffer);
            fileToBuffer = nullptr;
        }
    } else {
        int status = 0;
        std::tie(status, numFilesPopulated, batchSize) =
                readBatchData(filePaths,
                              filePathsIndexOffset,
                              loopBackToStart,
                              dims,
                              QNN_TENSOR_GET_DATA_TYPE(input),
                              static_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(
                                      input).data));
        if ( 0  != status) {
            LOGGW("failure in readBatchData");
            returnStatus =  1 ;
        }
    }
    return {returnStatus, numFilesPopulated, batchSize};
}


// Helper method to populate all input tensors during execution.
PopulateInputTensorsRetType_t qnn_io_tensor::populateInputTensors(
        uint32_t graphIdx,
        const std::vector<std::vector<std::string>> &filePathsVector,
        const size_t filePathsIndexOffset,
        const bool loopBackToStart,
        const std::unordered_map<std::string, uint32_t> &inputNameToIndex,
        Qnn_Tensor_t *inputs,
        GraphInfo_t graphInfo,
        InputDataType inputDataType) {
    LOGGD("populateInputTensors() graphIndx %d", graphIdx);
    if (nullptr == inputs) {
        LOGGW("inputs is nullptr");
        return { 1 , 0, 0};
    }
    auto inputCount = graphInfo.numInputTensors;
    if (filePathsVector.size() != inputCount) {
        LOGGW(
                "Incorrect amount of Input files for graphIdx: %d. Expected: %d, "
                "received: %d",
                graphIdx,
                inputCount,
                filePathsVector.size());
        return { 1 , 0, 0};
    }
    size_t numFilesPopulated = 0;
    size_t numBatchSize = 0;
    for (size_t inputIdx = 0; inputIdx < inputCount; inputIdx++) {
        size_t inputNameIdx = inputIdx;
        LOGGD("index = %d input column index = %d", inputIdx, inputNameIdx);
        std::string inputNodeName;
        if (QNN_TENSOR_GET_NAME(graphInfo.inputTensors[inputIdx]))
            inputNodeName = QNN_TENSOR_GET_NAME(graphInfo.inputTensors[inputIdx]);
        if (!inputNodeName.empty() &&
            inputNameToIndex.find(inputNodeName) != inputNameToIndex.end()) {
            inputNameIdx = inputNameToIndex.at(inputNodeName);
        }
        int returnStatus = 0;
        size_t currentInputNumFilesPopulated = 0;
        size_t currentInputNumBatchSize = 0;
        std::tie(returnStatus, currentInputNumFilesPopulated, currentInputNumBatchSize) =
                populateInputTensor(filePathsVector[inputNameIdx],
                                    filePathsIndexOffset,
                                    loopBackToStart,
                                    &(inputs[inputIdx]),
                                    inputDataType);
        if ( 0  != returnStatus) {
            LOGGW("populateInputTensorFromFiles failed for input %s with index %d",
                  inputNodeName.c_str(),
                  inputIdx);
            return { 1 , currentInputNumFilesPopulated, currentInputNumBatchSize};
        }
        if (inputIdx == 0) {
            numFilesPopulated = currentInputNumFilesPopulated;
            numBatchSize = currentInputNumBatchSize;
        } else {
            if (numFilesPopulated != currentInputNumFilesPopulated ||
                numBatchSize != currentInputNumBatchSize) {
                LOGGW(
                        "Current input tensor with name: %s with index %d files populated = %d, batch size = %d"
                        " does not match with expected files populated = %d, batch size = %d",
                        inputNodeName.c_str(),
                        inputIdx,
                        currentInputNumFilesPopulated,
                        currentInputNumBatchSize,
                        numFilesPopulated,
                        numBatchSize);
                return { 1 , numFilesPopulated, numBatchSize};
            }
        }
    }
    return { 0 , numFilesPopulated, numBatchSize};
}


// Setup details for Qnn_Tensor_t for execution
// based on information in Qnn_TensorWrapper_t provided by model.so.
int qnn_io_tensor::setupTensors(Qnn_Tensor_t ** tensors, uint32_t tensorCount, Qnn_Tensor_t * tensorWrappers) {
    int result =  0 ;

    if (nullptr == tensorWrappers) {
        LOGGW("tensorWrappers is nullptr");
        return 1 ;
    }
    if (0 == tensorCount) {
        LOGGI("why tensor count is 0, pls check\n");
        return 2 ;
    }

    *tensors = (Qnn_Tensor_t *)calloc(1, tensorCount * sizeof(Qnn_Tensor_t));
    if (nullptr == *tensors) {
        LOGGW("mem alloc failed for *tensors");
        return 3;
    }

    for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
        Qnn_Tensor_t wrapperTensor = tensorWrappers[tensorIdx];
        std::vector<size_t> dims;
        fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(wrapperTensor),
                 QNN_TENSOR_GET_RANK(wrapperTensor));
        if (0 == result) {
            LOGGD("allocateBuffer successful");
            (*tensors)[tensorIdx] = QNN_TENSOR_INIT;
            result = (deepCopyQnnTensorInfo(((*tensors) + tensorIdx), &wrapperTensor) ==
                      true
                      ?  0
                      :  1);
        }

        if (0 == result) {
            LOGGD("deepCopyQnnTensorInfo successful");
            QNN_TENSOR_SET_MEM_TYPE(((*tensors) + tensorIdx), QNN_TENSORMEMTYPE_RAW);
        }

        Qnn_ClientBuffer_t clientBuffer = QNN_CLIENT_BUFFER_INIT;
        result =  allocateBuffer(reinterpret_cast<uint8_t **>(&clientBuffer.data),
                                 dims,
                                 QNN_TENSOR_GET_DATA_TYPE((*tensors) + tensorIdx));
        int     status = 0;
        size_t  length = 0;
        std::tie(status, length) =
                calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE((*tensors) + tensorIdx));
        if (status !=  0 ) {
            result = 1;
        }
        clientBuffer.dataSize = length;
        QNN_TENSOR_SET_CLIENT_BUF(((*tensors) + tensorIdx), clientBuffer);

        if (0  != result) {
            LOGGW("failure in setupTensors, cleaning up resources");
            if (nullptr != (QNN_TENSOR_GET_CLIENT_BUF((*tensors) + tensorIdx)).data) {
                free(QNN_TENSOR_GET_CLIENT_BUF((*tensors) + tensorIdx).data);
            }
            tearDownTensors(*tensors, tensorIdx);
            *tensors        = nullptr;
            result    =  1 ;
            LOGGW("failure in setupTensors, done cleaning up resources");
            return result;
        }
    }

    return result;
}


// Setup details for all input and output tensors for graph execution.
int qnn_io_tensor::setupInputAndOutputTensors(
        Qnn_Tensor_t **inputs, Qnn_Tensor_t **outputs, GraphInfo_t graphInfo) {
    int returnStatus =  0 ;
    if (0 !=
        setupTensors(inputs, graphInfo.numInputTensors, (graphInfo.inputTensors))) {
        LOGGW("failure in setting up input tensors");
        returnStatus =  1 ;
    } else {
        LOGGI("succeed in setting up input tensors");
    }

    if (0 !=
        setupTensors(outputs, graphInfo.numOutputTensors, (graphInfo.outputTensors))) {
        LOGGW("failure in setting up output tensors");
        returnStatus =  1 ;
    } else {
        LOGGI("succeed in setting up output tensors");
    }

    if (0 != returnStatus) {
        if (nullptr != *inputs) {
            LOGGD("cleaning up input tensors");
            tearDownTensors(*inputs, graphInfo.numInputTensors);
            *inputs = nullptr;
        }
        if (nullptr != *outputs) {
            LOGGD("cleaning up output tensors");
            tearDownTensors(*outputs, graphInfo.numOutputTensors);
            *outputs = nullptr;
        }
        LOGGW("failure in setupInputAndOutputTensors, done cleaning up resources");
    }

    return returnStatus;
}


// Clean up all tensors related data after execution.
int qnn_io_tensor::tearDownTensors(Qnn_Tensor_t *tensors, uint32_t tensorCount) {
    for (size_t tensorIdx = 0; tensorIdx < tensorCount; tensorIdx++) {
        //LOGGD("freeing resources for tensor: %d", tensorIdx);
        //GGML_JNI_NOTIFY("freeing resources for tensor: %d", tensorIdx);
        if (nullptr != QNN_TENSOR_GET_DIMENSIONS(tensors[tensorIdx])) {
            //LOGGD("freeing dimensions");
            //GGML_JNI_NOTIFY("freeing dimensions");
            free(QNN_TENSOR_GET_DIMENSIONS(tensors[tensorIdx]));
        }
        if (nullptr != QNN_TENSOR_GET_CLIENT_BUF(tensors[tensorIdx]).data) {
            //LOGGD("freeing clientBuf.data");
            //GGML_JNI_NOTIFY("freeing clientBuf.data");
            free(QNN_TENSOR_GET_CLIENT_BUF(tensors[tensorIdx]).data);
        }
    }
    free(tensors);
    return  0 ;
}


// Clean up all input and output tensors after execution.
int qnn_io_tensor::tearDownInputAndOutputTensors(Qnn_Tensor_t *inputs,
                                                 Qnn_Tensor_t *outputs,
                                                 size_t numInputTensors,
                                                 size_t numOutputTensors) {
    if (nullptr != inputs) {
        //LOGGD("cleaning up resources for input tensors");
        //GGML_JNI_NOTIFY("cleaning up resources for input tensors");
        tearDownTensors(inputs, numInputTensors);
        inputs = nullptr;
    }
    if (nullptr != outputs) {
        //LOGGD("cleaning up resources for output tensors");
        //GGML_JNI_NOTIFY("cleaning up resources for output tensors");
        tearDownTensors(outputs, numOutputTensors);
        outputs = nullptr;
    }
    return  0 ;
}


// Helper method to allocate a buffer.
int qnn_io_tensor::allocateBuffer(uint8_t **buffer, std::vector<size_t> dims, Qnn_DataType_t dataType) {
    size_t elementCount = calculateElementCount(dims);
    auto returnStatus =  0 ;
    switch (dataType) {
        case QNN_DATATYPE_FLOAT_32:
            LOGGD("allocating float buffer");
            returnStatus =
                    allocateBuffer < float > (reinterpret_cast<float **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_UINT_8:
        case QNN_DATATYPE_UFIXED_POINT_8:
            LOGGD("allocating uint8_t buffer");
            returnStatus =
                    allocateBuffer < uint8_t > (reinterpret_cast<uint8_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_UINT_16:
        case QNN_DATATYPE_UFIXED_POINT_16:
            LOGGD("allocating uint16_t buffer");
            returnStatus = allocateBuffer < uint16_t >
                    (reinterpret_cast<uint16_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_UINT_32:
            LOGGD("allocating uint32_t buffer");
            returnStatus = allocateBuffer < uint32_t >
                    (reinterpret_cast<uint32_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_INT_8:
            LOGGD("allocating int8_t buffer");
            returnStatus =
                    allocateBuffer < int8_t > (reinterpret_cast<int8_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_INT_16:
            LOGGD("allocating int16_t buffer");
            returnStatus =
                    allocateBuffer < int16_t > (reinterpret_cast<int16_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_INT_32:
            LOGGD("allocating int32_t buffer");
            returnStatus =
                    allocateBuffer < int32_t > (reinterpret_cast<int32_t **>(buffer), elementCount);
            break;

        case QNN_DATATYPE_BOOL_8:
            LOGGD("allocating bool buffer");
            returnStatus =
                    allocateBuffer < uint8_t > (reinterpret_cast<uint8_t **>(buffer), elementCount);
            break;

        default:
            LOGGW("Datatype not supported yet!");
            returnStatus =  1 ;
            break;
    }
    return returnStatus;
}


// Convert data to float or de-quantization. This is used when
// user requests for float output and the model produces
// non-float output.
int qnn_io_tensor::convertToFloat(float **out, Qnn_Tensor_t *tensor) {
    if (nullptr == tensor) {
        LOGGW("tensors is nullptr");
        return  1 ;
    }
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));
    int result =  0 ;
    size_t elementCount = calculateElementCount(dims);
    result = allocateBuffer<float>(out, elementCount);
    if ( 0  != result) {
        LOGGW("failure in allocateBuffer<float>");
        return result;
    }
    Qnn_DataType_t data_type = QNN_TENSOR_GET_DATA_TYPE(tensor);
    LOGGI("tensor data type %d\n", data_type);
    switch (data_type) {
        case QNN_DATATYPE_UFIXED_POINT_8:
            if ( 0  !=
                 tfNToFloat<uint8_t>(
                         *out,
                         reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                         QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                         elementCount)) {
                LOGGW("failure in tfNToFloat<uint8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UFIXED_POINT_16:
            if ( 0  !=
                 tfNToFloat<uint16_t>(
                         *out,
                         reinterpret_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.offset,
                         QNN_TENSOR_GET_QUANT_PARAMS(tensor).scaleOffsetEncoding.scale,
                         elementCount)) {
                LOGGW("failure in tfNToFloat<uint8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_8:
            if ( 0  !=
                 castToFloat<uint8_t>(
                         *out,
                         reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<uint8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_16:
            if ( 0  !=
                 castToFloat<uint16_t>(
                         *out,
                         reinterpret_cast<uint16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<uint16_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_UINT_32:
            if ( 0  !=
                 castToFloat<uint32_t>(
                         *out,
                         reinterpret_cast<uint32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<uint32_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_8:
            if ( 0  !=
                 castToFloat<int8_t>(
                         *out,
                         reinterpret_cast<int8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<int8_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_16:
            if ( 0  !=
                 castToFloat<int16_t>(
                         *out,
                         reinterpret_cast<int16_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<int16_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_INT_32:
            if ( 0  !=
                 castToFloat<int32_t>(
                         *out,
                         reinterpret_cast<int32_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<int32_t>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_BOOL_8:
            if ( 0  !=
                 castToFloat<uint8_t>(
                         *out,
                         reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(tensor).data),
                         elementCount)) {
                LOGGW("failure in castToFloat<bool>");
                result =  1 ;
            }
            break;

        case QNN_DATATYPE_FLOAT_32:
        {
            float * src = (float*)(QNN_TENSOR_GET_CLIENT_BUF(tensor).data);
            float * dst = (*out);
            LOGGI("dump output tensor:\n");
            for (size_t i = 0; i < elementCount; i++) {
                LOGGI("%.2f\t", src[i]);
                dst[i] = src[i];
            }
        }
            break;

        default:
            LOGGW("datatype not supported yet!");
            result =  1 ;
            break;
    }
    if ( 0  != result) {
        LOGGD("freeing *out");
        if (*out != nullptr) {
            free(*out);
            *out = nullptr;
        }
    }

    return result;
}


// Helper method to convert Output tensors to float and write them
// out to files.
int qnn_io_tensor::convertAndWriteOutputTensorInFloat(
        Qnn_Tensor_t *output,
        std::vector<std::string> outputPaths,
        std::string fileName,
        size_t outputBatchSize) {
    if (nullptr == output) {
        LOGGW("output is nullptr");
        return  1 ;
    }

    auto returnStatus =  0 ;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
    float *floatBuffer = nullptr;
    returnStatus = convertToFloat(&floatBuffer, output);
    if ( 0  != returnStatus) {
        LOGGW("failure in convertToFloat");
        return  1 ;
    }
    uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(floatBuffer);
    if ( 0  !=
         writeBatchDataToFile(
                 outputPaths, fileName, dims, QNN_DATATYPE_FLOAT_32, bufferToWrite,
                 outputBatchSize)) {
        LOGGW("failure in writeBatchDataToFile");
        returnStatus =  1 ;
    }
    if (nullptr != floatBuffer) {
        LOGGD("freeing floatBuffer");
        free(floatBuffer);
        floatBuffer = nullptr;
    }
    return returnStatus;
}


// Helper method to write out output. There is no de-quantization here.
// Just write output as is to files.
int qnn_io_tensor::writeOutputTensor(Qnn_Tensor_t *output,
                                     std::vector<std::string> outputPaths,
                                     std::string fileName,
                                     size_t outputBatchSize) {
    if (nullptr == output) {
        LOGGW("output is nullptr");
        return  1 ;
    }
    auto returnStatus =  0 ;
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(output), QNN_TENSOR_GET_RANK(output));
    uint8_t *bufferToWrite = reinterpret_cast<uint8_t *>(QNN_TENSOR_GET_CLIENT_BUF(output).data);
    if ( 0  !=
         writeBatchDataToFile(outputPaths,
                              fileName,
                              dims,
                              QNN_TENSOR_GET_DATA_TYPE(output),
                              bufferToWrite,
                              outputBatchSize)) {
        LOGGW("failure in writeBatchDataToFile");
        returnStatus =  1 ;
    }
    return returnStatus;
}


// write out all output tensors to files. If output_data_type is float,
// then all outputs will be raw floats regardless of what the model outputs.
// If the output_data_type is native, then output is written as produced by the model.
// Also, for native option, a json with quantization parameters is written out.
// If output_data_type is float_and_native, both above are done.
// If the output in the graph is float, then output_data_type has no effect.
int qnn_io_tensor::writeOutputTensors(uint32_t graphIdx,
                                      size_t startIdx,
                                      char *graphName,
                                      Qnn_Tensor_t *outputs,
                                      uint32_t numOutputs,
                                      OutputDataType outputDatatype,
                                      uint32_t graphsCount,
                                      std::string outputPath,
                                      size_t numInputFilesPopulated,
                                      size_t outputBatchSize) {
    if (nullptr == outputs) {
        LOGGW("Received nullptr");
        return  1 ;
    }
    if (graphsCount > 1) {
        if (nullptr != graphName && strlen(graphName) > 0) {
            outputPath += ("/" + std::string(graphName));
        } else {
            outputPath += ("/" + std::string("Graph_") +
                           std::to_string(graphIdx));
        }
    }
    auto returnStatus =  0 ;
    std::vector<std::string> outputPaths;
    for (size_t idx = 0; idx < numInputFilesPopulated; idx++) {
        std::string output = outputPath + ("/" + std::string("Result_") +
                                           std::to_string(startIdx + idx));
        outputPaths.push_back(output);
    }
    for (size_t outputIdx = 0; outputIdx < numOutputs; outputIdx++) {
        LOGGD("Writing output for outputIdx: %d", outputIdx);
        GGML_JNI_NOTIFY("Writing output for outputIdx: %d", outputIdx);
        std::string outputFilePrefix;
        if (nullptr != QNN_TENSOR_GET_NAME(outputs[outputIdx]) &&
            strlen(QNN_TENSOR_GET_NAME(outputs[outputIdx])) > 0) {
            outputFilePrefix = std::string(QNN_TENSOR_GET_NAME(outputs[outputIdx]));
        } else {
            outputFilePrefix = std::string("Output_") + std::to_string(outputIdx);
        }
        auto outputFile = outputFilePrefix + std::string(".raw");
        auto outputFileNative = outputFilePrefix + std::string("_native.raw");
        if (QNN_TENSOR_GET_DATA_TYPE(outputs[outputIdx]) == QNN_DATATYPE_FLOAT_32) {
            LOGGD("Writing in output->dataType == QNN_DATATYPE_FLOAT_32");
            returnStatus =
                    writeOutputTensor(&(outputs[outputIdx]), outputPaths, outputFile,
                                      outputBatchSize);
        } else if (outputDatatype == OutputDataType::FLOAT_ONLY) {
            LOGGD("Writing in output->dataType == OutputDataType::FLOAT_ONLY");
            returnStatus = convertAndWriteOutputTensorInFloat(
                    &(outputs[outputIdx]), outputPaths, outputFile, outputBatchSize);
        } else if (outputDatatype == OutputDataType::NATIVE_ONLY) {
            LOGGD("Writing in output->dataType == OutputDataType::NATIVE_ONLY");
            returnStatus =
                    writeOutputTensor(&(outputs[outputIdx]), outputPaths, outputFileNative,
                                      outputBatchSize);
        } else if (outputDatatype == OutputDataType::FLOAT_AND_NATIVE) {
            LOGGD("Writing in output->dataType == OutputDataType::FLOAT_AND_NATIVE");
            returnStatus = convertAndWriteOutputTensorInFloat(
                    &(outputs[outputIdx]), outputPaths, outputFile, outputBatchSize);
            if ( 0  == returnStatus) {
                returnStatus = writeOutputTensor(
                        &(outputs[outputIdx]), outputPaths, outputFileNative, outputBatchSize);
            }
        }
    }
    return returnStatus;
}


// Helper method to allocate a buffer and copy data to it.
int qnn_io_tensor::allocateAndCopyBuffer(uint8_t **buffer, Qnn_Tensor_t *tensor) {
    if (nullptr == tensor) {
        return  1 ;
    }
    std::vector<size_t> dims;
    fillDims(dims, QNN_TENSOR_GET_DIMENSIONS(tensor), QNN_TENSOR_GET_RANK(tensor));
    int datautilStatus = 0;
    size_t length;
    std::tie(datautilStatus, length) =
            calculateLength(dims, QNN_TENSOR_GET_DATA_TYPE(tensor));
    if (datautilStatus !=  0 ) {
        return  1 ;
    }
    if ( 0  != allocateBuffer(buffer, dims, QNN_TENSOR_GET_DATA_TYPE(tensor))) {
        LOGGW("failure in allocateBuffer");
        return  1 ;
    }
    memscpy(*buffer,
            length * sizeof(uint8_t),
            QNN_TENSOR_GET_CLIENT_BUF(tensor).data,
            length * sizeof(uint8_t));
    return 0;
}


int qnn_io_tensor::fillDims(std::vector<size_t> &dims, uint32_t *inDimensions, uint32_t rank) {
    if (nullptr == inDimensions) {
        LOGGW("input dimensions is nullptr\n");
        return 1;
    }
    LOGGD("rank %d\n", rank);
    for (size_t r = 0; r < rank; r++) {
        dims.push_back(inDimensions[r]);
    }
    return 0;
}


class qnn_implementation {
public:
    using BackendIdType = decltype(QnnInterface_t{}.backendId);

    explicit qnn_implementation(const std::string & lib_path, const std::string & backend_name, const std::string & model_name) :
            _lib_path(std::move(lib_path)) ,
            _backend_name(std::move(backend_name)),
            _model_name(std::move(model_name))
    {};

    ~qnn_implementation() {
        //TODO:
        _input_tensors.clear();
        _output_tensors.clear();
    }

    int qnn_init(const QnnSaver_Config_t ** saver_config);

    int qnn_finalize();

    const qnn_interface & get_qnn_interface() {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_interface;
    }


    const QNN_INTERFACE_VER_TYPE & get_qnn_raw_interface () {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_raw_interface;
    }

    const QNN_SYSTEM_INTERFACE_VER_TYPE & get_qnn_raw_system_interface () {
        if (!_qnn_interface.is_loaded()) {
            LOGGW("pls check why _qnn_interface is not loaded\n");
        }
        return _qnn_raw_system_interface;
    }

    const Qnn_LogHandle_t get_qnn_log_handle() { return _qnn_log_handle; }

    const Qnn_ProfileHandle_t get_qnn_profile_handle() { return _qnn_profile_handle; }

    const Qnn_DeviceHandle_t get_qnn_device_handle() { return _qnn_device_handle; }

    const Qnn_BackendHandle_t get_qnn_backend_handle() { return _qnn_backend_handle; }

    const Qnn_ContextHandle_t get_qnn_context_handle() { return _qnn_context_handle; }

    const QnnSystemContext_Handle_t get_qnn_system_handle() { return _qnn_system_handle; }

    const Qnn_GraphHandle_t get_qnn_graph_handle() { return _qnn_graph_handle; }


    int init_qnn_model(const char* graph_name,
                       bool debug,
                       uint8_t do_node_validation              = 1,
                       const QnnGraph_Config_t ** graph_configs = nullptr
    );

    int finalize_qnn_model();

    int init_htp_perfinfra() {
        QnnDevice_Infrastructure_t device_infra = nullptr;
        int error = _qnn_raw_interface.deviceGetInfrastructure(&device_infra);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to get qnn device infra\n");
            return 1;
        }

        QnnHtpDevice_Infrastructure_t * htp_infra = static_cast<QnnHtpDevice_Infrastructure_t *>(device_infra);
        QnnHtpDevice_PerfInfrastructure_t * htp_perfinfra = &htp_infra->perfInfra;
        uint32_t power_configid = 1;
        uint32_t device_id = 0;
        uint32_t core_id = 0;
        htp_perfinfra->createPowerConfigId(device_id, core_id, &power_configid);
        _qnn_htp_perfinfra = htp_perfinfra;
        _qnn_power_configid = power_configid;

        return 0;
    }


    int set_rpc_polling() {
        if (_qnn_rpc_pollingtime > 0) {
            QnnHtpPerfInfrastructure_PowerConfig_t rpc_pollingTime;
            memset(&rpc_pollingTime, 0, sizeof(rpc_pollingTime));
            rpc_pollingTime.option =
                    QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_RPC_POLLING_TIME;
            rpc_pollingTime.rpcPollingTimeConfig = _qnn_rpc_pollingtime;
            const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&rpc_pollingTime, NULL};
            if (_qnn_htp_perfinfra) {
                _qnn_htp_perfinfra->setPowerConfig(_qnn_power_configid, powerConfigs);
            }
        }
        return 0;
    }


    int set_high_performance_mode() {
        if (nullptr == _qnn_htp_perfinfra) {
            LOGGD("perf intra is null\n");
            return 1;
        }

        QnnHtpPerfInfrastructure_PowerConfig_t powerConfig;
        memset(&powerConfig, 0, sizeof(powerConfig));
        powerConfig.option                              = QNN_HTP_PERF_INFRASTRUCTURE_POWER_CONFIGOPTION_DCVS_V3;
        powerConfig.dcvsV3Config.dcvsEnable             = 0;
        powerConfig.dcvsV3Config.setDcvsEnable          = 1;
        powerConfig.dcvsV3Config.contextId              = _qnn_power_configid;
        powerConfig.dcvsV3Config.powerMode              = QNN_HTP_PERF_INFRASTRUCTURE_POWERMODE_PERFORMANCE_MODE;
        powerConfig.dcvsV3Config.setSleepLatency        = 1; // True to consider Latency parameter otherwise False
        powerConfig.dcvsV3Config.setBusParams           = 1; // True to consider Bus parameter otherwise False
        powerConfig.dcvsV3Config.setCoreParams          = 1; // True to consider Core parameter otherwise False
        powerConfig.dcvsV3Config.sleepDisable           = 0; // True to consider sleep/LPM modes, False to enable
        powerConfig.dcvsV3Config.setSleepDisable        = 0; // True to consider sleep disable/enable parameter otherwise False
        // Set Sleep latency parameter
        uint32_t latencyValue                           = 40;
        powerConfig.dcvsV3Config.sleepLatency           = latencyValue; // range 40-2000 micro sec
        // set Bus Clock Parameters (refer QnnHtpPerfInfrastructure_VoltageCorner_t enum)
        powerConfig.dcvsV3Config.busVoltageCornerMin    = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.busVoltageCornerTarget = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.busVoltageCornerMax    = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        // set Core Clock Parameters (refer QnnHtpPerfInfrastructure_VoltageCorner_t enum)
        powerConfig.dcvsV3Config.coreVoltageCornerMin   = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.coreVoltageCornerTarget= DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        powerConfig.dcvsV3Config.coreVoltageCornerMax   = DCVS_VOLTAGE_VCORNER_MAX_VOLTAGE_CORNER;
        // Set power config with different performance parameters
        const QnnHtpPerfInfrastructure_PowerConfig_t *powerConfigs[] = {&powerConfig, NULL};

        _qnn_htp_perfinfra->setPowerConfig(_qnn_power_configid, powerConfigs);

        return 0;
    }


    /**
     * @param node_name        Lookup name for node/layer
     * @param p_tensor         A pointer to a struct containing information on the tensor
     * @param b_save_tensor    Flag to indicate if tensor should be saved in object for later retrieval
     *                         with class getter functions
     * @return
     */
    int add_tensor(const char * node_name, Qnn_Tensor_t * p_tensor, bool b_save_tensor = true);

    int add_tensor(const char *node_name, Qnn_Tensor_t tensor, bool b_save_tensor = true) {
        return add_tensor(node_name, &tensor, b_save_tensor);
    }


    /**
     * @param version      version The QNN version for Op_Config_t structure to use,QNN_OPCONFIG_VERSION_1
     * @param name         The node name to use
     * @param packageName  The node package name (e.g. qti.aisw)
     * @param type         The QNN_OP_QNN_OP_H node type (e.g. QNN_OP_ARGMAX)
     * @param params       A struct object containing all the params for the node to be added. For
     *                     tensorParam case. The tensor will be created within the function and the
     *                     data will be retrieved from the binary blob to set the tensor data
     * @param numOfParams  The number of elements in above params object
     * @param inputNames   List of tensor names for inputs to node. Note: the corresponding qnn
     * tensor objects must be created within this instance prior to being listed as input to a node
     * @param numOfInputs  The number of elements in above inputNames object
     * @param outputTensors List of Qnn_Tensor_t objects for outputs from node.
     *                      Note1: the corresponding qnn tensor objects will be created in function
     *                      and must not already exist.
     *                      Note2: the output names must be unique per graph
     * @param numOfOutputs  The number of elements in above outputs object
     * @return
     */
    int add_node(Qnn_OpConfigVersion_t version,
                 const char * name,
                 const char * packageName,
                 const char * type,
                 Qnn_Param_t * params,
                 uint32_t numOfParams,
                 const char ** inputNames,
                 uint32_t numOfInputs,
                 Qnn_Tensor_t * outputTensors,
                 uint32_t numOfOutputs);

    int get_tensor(const char *& node_name, const char *& tensor_name, Qnn_Tensor_t & tensor) {
        std::string map_entry = std::string(tensor_name);
        if (_tensors_map.find(tensor_name) == _tensors_map.end()) {
            LOGGW("tensor %s not found on node %s\n", map_entry.c_str(), node_name);
            return 1;
        }

        tensor = _tensors_map[map_entry];
        return 0;
    }

    int add_node(const Qnn_OpConfig_t & op_config) {
        return _qnn_interface.qnn_graph_add_node(_qnn_graph_handle, op_config);
    };

    int get_graphinfo_from_model(GraphInfoPtr_t ** graphs_info, uint32_t * num_graphsinfo);


    // compose all QNN graphs from prebuilt Qualcomm's dedicated MODEL.so which generated by Qualcomm's dedicated tool
    int compose_special_graph();

    int finalize_graph() {
        LOGGI("enter %s\n", __func__);

        int error = 0;
        LOGGD("qnn graph handle %s, %p\n", get_qnn_graph_name().c_str(), get_qnn_graph_handle());
        for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
            LOGGD("graph handle %p\n", (*_graphs_info)[graph_idx].graph);
            error = _qnn_raw_interface.graphFinalize((*_graphs_info)[graph_idx].graph, _qnn_profile_handle, nullptr);
            if (QNN_GRAPH_NO_ERROR != error) {
                LOGGW("qnn graph finalize failure, error %d\n", error);
                GGML_JNI_NOTIFY("qnn graph finalize failure, error %d\n", error);
                //return 1;
            } else {
                LOGGW("qnn graph finalize successfully\n");
                GGML_JNI_NOTIFY("qnn graph finalize successfully\n");
            }
        }

        if (ggml_qnn_profile_level::profile_off != _profile_level) {
            //TODO: handle profile info with _qnn_profile_handle
        }

        LOGGD("qnn graph handle %p\n", _qnn_graph_handle);

        LOGGI("leave %s\n", __func__);

        return 0;
    }

    int execute_graph(
            const std::vector<Qnn_Tensor_t>& input_tensor_structs,
            std::vector<Qnn_Tensor_t>& output_tensor_structs) {
        return _qnn_interface.qnn_graph_execute(
                _qnn_graph_handle,
                input_tensor_structs.data(),
                input_tensor_structs.size(),
                output_tensor_structs.data(),
                output_tensor_structs.size(),
                _qnn_profile_handle, nullptr);
    }

    int execute_graph() {
        int result = 0;
        auto before_exec = std::chrono::high_resolution_clock::now();
        result = _qnn_interface.qnn_graph_execute(_qnn_graph_handle,
                                                  _input_tensors.data(),
                                                  _input_tensors.size(),
                                                  _output_tensors.data(),
                                                  _output_tensors.size(),
                                                  _qnn_profile_handle, nullptr);

        auto after_exec = std::chrono::high_resolution_clock::now();
        double exec_duration =
                std::chrono::duration_cast<std::chrono::microseconds>(
                        after_exec - before_exec)
                        .count() /
                1000.0;

        LOGGD("matrix operation cost %.2f ms\n", exec_duration);

        if (1) {
            std::string dir = "/sdcard/kantv/qnn/debug/";
            ggml_qnn_create_directory(dir);
            LOGGD("dump output tensor to the path: %s\n", dir.c_str());
            for (std::size_t out_idx = 0; out_idx < _output_tensors.size();
                 ++out_idx) {
                const Qnn_Tensor_t& output_tensor = _output_tensors[out_idx];

                std::string output_path =
                        dir + QNN_VER_PTR(output_tensor)->name + "_tensor.raw";

                std::ofstream fout(output_path, std::ios::binary);
                if (fout.fail()) {
                    LOGGW("dump tensor name: %s failure\n", QNN_VER_PTR(output_tensor)->name);
                    return 1;
                }
                fout.write(static_cast<const char*>(QNN_VER_PTR(output_tensor)->clientBuf.data),
                           QNN_VER_PTR(output_tensor)->clientBuf.dataSize);
            }
        }

        return result;
    }

    int execute_special_graph();

    int execute_common_graph(const std::vector<uint8_t *> & input_node_values,
                             std::vector<float *> & output_node_values,
                             std::vector<size_t> output_node_sizes);

    int run_qnn_matrix();

    std::string get_qnn_graph_name() { return _graph_name; }

    std::vector<Qnn_Tensor_t> get_graph_input_tensors() { return _input_tensors; }

    std::vector<Qnn_Tensor_t> get_graph_output_tensors() { return _output_tensors; }

    std::map<std::string, std::vector<std::string>> get_output_tensormap() {
        return _output_tensor_map;
    }

    bool is_rpcmem_initialized() {
        return _rpcmem_initialized;
    }

    void set_rpcmem_initialized(bool initialized) {
        _rpcmem_initialized = initialized;
    }

    int32_t rpcmem_to_fd(void * buf);

    int  register_rpcmem(void * p_data, Qnn_Tensor_t * p_tensor);

    void unregister_rpcmem();

    void * alloc_rpcmem(size_t bytes, size_t alignment);

    void free_rpcmem(void * buf);

    bool is_rpcmem_allocated(void * buf);

    bool is_rpcmem_registered(Qnn_MemHandle_t handle) {
        return _qnn_mem_set.count(handle) != 0U;
    }

private:
    int load_system();
    int unload_system();

    int load_prebuit_qnn_model();
    int unload_prebuilt_qnn_model();

    int load_backend(std::string & lib_path, const QnnSaver_Config_t ** saver_config);
    int unload_backend();

    void set_qnn_raw_interface(QNN_INTERFACE_VER_TYPE & raw_interface) {
        _qnn_raw_interface = raw_interface;
    }

    void set_qnn_raw_system_interface(QNN_SYSTEM_INTERFACE_VER_TYPE & raw_interface) {
        _qnn_raw_system_interface = raw_interface;
    }

    int free_cached_tensors() {
        int error = 0;

        ENTER_FUNC();
        for (std::map<std::string, Qnn_Tensor_t>::iterator tensorIt = _tensors_map.begin();
             tensorIt != _tensors_map.end();) {
            Qnn_Tensor_t &tensor = tensorIt->second;
            if (QNN_TENSOR_GET_TYPE(tensor) != QNN_TENSOR_TYPE_APP_WRITE &&
                QNN_TENSOR_GET_TYPE(tensor) != QNN_TENSOR_TYPE_APP_READ) {
                VALIDATE(freeQnnTensor(tensor), error);
                tensorIt = _tensors_map.erase(tensorIt);
            } else {
                tensorIt++;
            }
        }

        _tensors_map.clear();
        LEAVE_FUNC();

        return error;
    }



private:
    static constexpr const int _required_num_providers = 1;

private:
    std::string _lib_path;
    std::string _backend_name;
    std::string _model_name;                                 // prebuilt QNN model name
    BackendIdType _backend_id;

    bool _debug_tensor                              = false; // flag to indicate if requested graph is to be run in debug mode
    bool _do_node_validations                       = true;  // flag to indicate whether all add_node calls need to be validated
    QnnLog_Level_t _qnn_log_level                   = QNN_LOG_LEVEL_DEBUG;
    //QnnLog_Level_t _qnn_log_level                   = QNN_LOG_LEVEL_VERBOSE;

    ggml_qnn_profile_level _profile_level           = ggml_qnn_profile_level::profile_detail;

    qnn_interface _qnn_interface;

    void * _system_lib_handle                       = nullptr;
    void * _model_lib_handle                        = nullptr;

    Qnn_GraphHandle_t _qnn_graph_handle             = nullptr;

    Qnn_LogHandle_t _qnn_log_handle                 = nullptr;

    Qnn_ProfileHandle_t _qnn_profile_handle         = nullptr;

    Qnn_DeviceHandle_t _qnn_device_handle           = nullptr;

    Qnn_BackendHandle_t _qnn_backend_handle         = nullptr;

    Qnn_ContextHandle_t _qnn_context_handle         = nullptr;

    QnnSystemContext_Handle_t _qnn_system_handle    = nullptr;

    QnnHtpDevice_PerfInfrastructure_t * _qnn_htp_perfinfra  = nullptr;
    uint32_t _qnn_power_configid                            = 1;
    uint32_t _qnn_rpc_pollingtime                           = 9999; // 0-10000 us for high performing

    QNN_INTERFACE_VER_TYPE _qnn_raw_interface;
    QNN_SYSTEM_INTERFACE_VER_TYPE _qnn_raw_system_interface;

    ComposeGraphsHandleType_t _qnn_composegraphs_handle     = nullptr;
    FreeGraphsHandleType_t _qnn_freegraphs_handle           = nullptr;

    std::unordered_set<Qnn_MemHandle_t> _qnn_mem_set;

    GraphInfo_t ** _graphs_info                             = nullptr;
    uint32_t _graphs_count                                  = 0;

    std::vector<Qnn_Tensor_t> _input_tensors;
    std::vector<Qnn_Tensor_t> _output_tensors;
    std::map<std::string, Qnn_Tensor_t> _tensors_map;
    std::map<std::string, std::vector<std::string>> _output_tensor_map;
    std::string _graph_name;

    std::vector<const char *> input_node_names;
    std::vector<std::vector<int64_t>> input_node_dims;
    std::vector<size_t> input_node_sizes;

    std::vector<const char *> output_node_names;
    std::vector<std::vector<int64_t>> output_node_dims;
    std::vector<size_t> output_node_sizes;

    std::vector<uint8_t *> input_node_values;
    std::vector<float *> output_node_values;

    static std::mutex _init_mutex;
    static std::unordered_map<BackendIdType, void *> _loaded_lib_handle;
    static std::unordered_map<std::string, BackendIdType> _lib_path_to_backend_id;
    static std::unordered_map<BackendIdType, const QnnInterface_t *> _loaded_backend;

    void * _rpc_lib_handle                          = nullptr;
    std::atomic_bool _rpcmem_initialized            { false };
    pfn_rpc_mem_alloc _pfn_rpc_mem_alloc;
    pfn_rpc_mem_free  _pfn_rpc_mem_free;
    pfn_rpc_mem_to_fd _pfn_rpc_mem_to_fd;
    std::unordered_map<void *, void *> _rpcmem_store_map;

    qnn_io_tensor _io_tensor;

};


std::mutex qnn_implementation::_init_mutex;

std::unordered_map<qnn_implementation::BackendIdType, void *> qnn_implementation::_loaded_lib_handle;

std::unordered_map<std::string, qnn_implementation::BackendIdType> qnn_implementation::_lib_path_to_backend_id;

std::unordered_map<qnn_implementation::BackendIdType, const QnnInterface_t *> qnn_implementation::_loaded_backend;


void * qnn_implementation::alloc_rpcmem(size_t bytes, size_t alignment) {
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
        return nullptr;
    }

    auto allocate_bytes = static_cast<int32_t>(bytes + alignment);
    void* buf = _pfn_rpc_mem_alloc(RPCMEM_HEAP_ID_SYSTEM, RPCMEM_DEFAULT_FLAGS, allocate_bytes);
    if (buf == nullptr) {
        LOGGW("failed to allocate rpc memory\n");
        return nullptr;
    }

    auto aligned_buf = reinterpret_cast<void*>(alignTo(alignment, reinterpret_cast<intptr_t>(buf)));
    bool status = _rpcmem_store_map.insert(std::pair<void*, void*>(aligned_buf, buf)).second;
    if (!status) {
        LOGGW("failed to allocate rpc memory\n");
        _pfn_rpc_mem_free(buf);
    }

    return aligned_buf;
}


void qnn_implementation::free_rpcmem(void * buf) {
    if (!_rpcmem_initialized) {
        LOGGW("rpc memory not initialized\n");
    } else if (0 == _rpcmem_store_map.count(buf)) {
        LOGGW("no allocated tensor\n");
    } else {
        _pfn_rpc_mem_free(_rpcmem_store_map[buf]);
        _rpcmem_store_map.erase(buf);
    }
}


int32_t qnn_implementation::rpcmem_to_fd(void * buf) {
    int32_t mem_fd = -1;
    if (!is_rpcmem_initialized()) {
        LOGGW("rpc memory not initialized\n");
    } else {
        mem_fd = _pfn_rpc_mem_to_fd(buf);
    }

    return mem_fd;
}


int qnn_implementation::register_rpcmem(void * p_data, Qnn_Tensor_t * p_tensor) {
    if (nullptr == p_data || (nullptr == p_tensor)) {
        LOGGW("invalid param\n");
        return 1;
    }

    if (!is_rpcmem_initialized()) {
        LOGGW("rpc memory not initialized\n");
        return 2;
    }

    if (is_rpcmem_allocated(p_data)) {
        LOGGW("rpc memory already allocated\n");
        //return 3;
    }
    if (is_rpcmem_registered((QNN_VER_PTR(*p_tensor)->memHandle))) {
        LOGGW("tensor %s has been registered shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
        return 4;
    }

    int32_t mem_fd = rpcmem_to_fd(p_data);
    if (-1 == mem_fd) {
        LOGGW("failed to get file descriptor\n");
        return 5;
    }
    LOGGD("mem_fd %d\n", mem_fd);
    Qnn_MemDescriptor_t descriptor = {
            {QNN_VER_PTR(*p_tensor)->rank, QNN_VER_PTR(*p_tensor)->dimensions, nullptr},
            QNN_VER_PTR(*p_tensor)->dataType,
            QNN_MEM_TYPE_ION,
            {{mem_fd}}};
    Qnn_MemHandle_t handle = nullptr;
    int error = QNN_SUCCESS;
    error = _qnn_interface.qnn_mem_register(
            _qnn_context_handle,
            &descriptor,
            /*numDescriptors=*/1,
            &handle);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to register shared memory, error %d, %s\n", QNN_GET_ERROR_CODE(error), strerror(error));
        return 6;
    } else {
        LOGGI("tensor %s successfully register shared memory\n", (QNN_VER_PTR(*p_tensor)->name));
    }
    QNN_VER_PTR(*p_tensor)->memHandle = handle;
    _qnn_mem_set.insert(handle);

    return 0;
}


void qnn_implementation::unregister_rpcmem() {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    if (_qnn_mem_set.empty()) {
        LOGGW("no rpcmem registered\n");
    }

    for (auto& mem_handle : _qnn_mem_set) {
        error = _qnn_interface.qnn_mem_de_register(&mem_handle, 1);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to unregister shared memory, error %d\n", QNN_GET_ERROR_CODE(error));
        }
    }
    _qnn_mem_set.clear();
}


bool qnn_implementation::is_rpcmem_allocated(void * buf) {
    return _rpcmem_store_map.count(buf) != 0U;
}


int qnn_implementation::load_backend(std::string & lib_path, const QnnSaver_Config_t ** saver_config) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    LOGGD("lib_path:%s\n", lib_path.c_str());

    void * lib_handle = dlopen(lib_path.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (nullptr == lib_handle) {
        LOGGW("can not open QNN library %s, with error: %s", lib_path.c_str(), dlerror());
        return 1;
    }

    // load get_provider function
    auto get_providers = load_qnn_functionpointers<_pfn_QnnInterface_getProviders *>(lib_handle, "QnnInterface_getProviders");
    if (nullptr == get_providers) {
        LOGGW("can not load symbol QnnInterface_getProviders : %s", dlerror());
        return 2;
    }

    // get QnnInterface Providers
    std::uint32_t num_providers = 0;
    const QnnInterface_t ** provider_list = nullptr;
    error = get_providers(&provider_list, &num_providers);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to get providers, error %d", QNN_GET_ERROR_CODE(error));
        return 3;
    }
    LOGGD("num_providers=%d\n", num_providers);
    if (num_providers != _required_num_providers) {
        LOGGW("providers is %d instead of required %d", num_providers, _required_num_providers);
        //return 4;
    }

    if (nullptr == provider_list) {
        LOGGW("failed to get qnn interface providers\n");
        return 5;
    }
    bool found_valid_interface = false;
    QNN_INTERFACE_VER_TYPE qnn_interface;
    for (size_t idx = 0; idx < num_providers; idx++) {
        if (QNN_API_VERSION_MAJOR == provider_list[idx]->apiVersion.coreApiVersion.major &&
            QNN_API_VERSION_MINOR <= provider_list[idx]->apiVersion.coreApiVersion.minor) {
            found_valid_interface = true;
            qnn_interface = provider_list[idx]->QNN_INTERFACE_VER_NAME;
            break;
        }
    }

    if (!found_valid_interface) {
        LOGGW("unable to find a valid qnn interface\n");
        return 6;
    }
    set_qnn_raw_interface(qnn_interface);

    BackendIdType backend_id = provider_list[0]->backendId;
    _lib_path_to_backend_id[lib_path] = backend_id;
    if (_loaded_backend.count(backend_id) > 0) {
        LOGGW("lib_path %s is loaded, but backend %d already exists\n",
              lib_path.c_str(), backend_id);
    }
    _loaded_backend[backend_id] = provider_list[0];
    if (_loaded_lib_handle.count(backend_id) > 0) {
        LOGGW("closing %p\n", _loaded_lib_handle[backend_id]);
        int dlclose_error = dlclose(_loaded_lib_handle[backend_id]);
        if (dlclose_error != 0) {
            LOGGW("fail to close %p with error %s\n", _loaded_lib_handle[backend_id], dlerror());
        }
    }
    _loaded_lib_handle[backend_id] = lib_handle;
    _backend_id = backend_id;

    QnnSaver_Config_t outputdir_cfg;
    outputdir_cfg.option = QNN_SAVER_CONFIG_OPTION_OUTPUT_DIRECTORY;
    outputdir_cfg.outputDirectory = "/data/data/com.cdeos.kantv/qnn/";

    QnnSaver_Config_t backendid_cfg;
    backendid_cfg.option = QNN_SAVER_CONFIG_OPTION_BACKEND_ID;
    backendid_cfg.backendId = _backend_id;
    const QnnSaver_Config_t *saverCfg[] = {&outputdir_cfg, &backendid_cfg, NULL};
    if (0 == QnnSaver_initialize(saverCfg)) {
        LOGGI("QnnSaver_initialize successfully");
    } else {
        LOGGI("QnnSaver_initialize failure");
    }

    // saver_initialize must be set before backend initialization
    auto saver_initialize = load_qnn_functionpointers<_pfn_QnnSaver_initialize *>(_loaded_lib_handle[backend_id], "QnnSaver_initialize");
    if (nullptr != saver_initialize) {
        error = saver_initialize(saver_config);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to saver_initialize，error %d", QNN_GET_ERROR_CODE(error));
            return 7;
        }
    } else {
        LOGGW("saver_initialize is null\n");
    }

    return 0;
}


int qnn_implementation::unload_backend() {
    ENTER_FUNC();
    int dlclose_error = 0;
    for (auto &it : _loaded_lib_handle) {
        dlclose_error = dlclose(it.second);
        if (dlclose_error != 0) {
            LOGGW("failed to close QNN backend %d, error %s\n", it.first, dlerror());
        }
    }

    _loaded_lib_handle.clear();
    _lib_path_to_backend_id.clear();
    _loaded_backend.clear();

    LEAVE_FUNC();

    return 0;
}


int qnn_implementation::load_system() {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;

    std::string system_lib_path = _lib_path + "libQnnSystem.so";
    LOGGD("system_lib_path:%s\n", system_lib_path.c_str());

    _system_lib_handle = dlopen(system_lib_path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _system_lib_handle) {
        LOGGW("can not pen QNN library %s, error: %s\n", system_lib_path.c_str(), dlerror());
        return 1;
    }

    auto * get_providers = reinterpret_cast<_pfn_QnnSystemInterface_getProviders *>(dlsym(_system_lib_handle, "QnnSystemInterface_getProviders"));
    if (nullptr == get_providers) {
        LOGGW("can not load QNN symbol QnnSystemInterface_getProviders: %s\n", dlerror());
        return 2;
    }

    uint32_t num_providers = 0;
    const QnnSystemInterface_t ** provider_list = nullptr;
    error = get_providers(&provider_list, &num_providers);
    if (error != QNN_SUCCESS) {
        LOGGW("failed to get providers, error %d\n", QNN_GET_ERROR_CODE(error));
        return 3;
    }

    if (num_providers != _required_num_providers) {
        //LOGGW("providers is %d instead of required %d\n", num_providers, _required_num_providers);
        return 4;
    }

    if (nullptr == provider_list) {
        LOGGW("can not get providers\n");
        return 5;
    }

    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_system_interface;
    bool found_valid_system_interface = false;
    for (size_t idx = 0; idx < num_providers; idx++) {
        if (QNN_SYSTEM_API_VERSION_MAJOR ==
            provider_list[idx]->systemApiVersion.major &&
            QNN_SYSTEM_API_VERSION_MINOR <=
            provider_list[idx]->systemApiVersion.minor) {
            found_valid_system_interface = true;
            qnn_system_interface = provider_list[idx]->QNN_SYSTEM_INTERFACE_VER_NAME;
            break;
        }
    }
    if (!found_valid_system_interface) {
        LOGGW("unable to find a valid qnn system interface\n");
        return 6;
    }
    set_qnn_raw_system_interface(qnn_system_interface);

    _qnn_interface.set_qnn_system_interface(provider_list[0]);

    _qnn_interface.qnn_system_context_create(&_qnn_system_handle);
    if (nullptr == _qnn_system_handle) {
        LOGW("can not create QNN system contenxt\n");
    } else {
        LOGGD("initialize qnn system successfully\n");
    }

    return 0;
}


int qnn_implementation::unload_system() {
    ENTER_FUNC();

    int result = 0;

    if (nullptr == _system_lib_handle) {
        LOGGD("system lib handle is null\n");
        return 1;
    }

    if (nullptr != _qnn_system_handle) {
        result = _qnn_interface.qnn_system_context_free(_qnn_system_handle);
        if (result != QNN_SUCCESS) {
            LOGGW("failed to free QNN system context\n");
        }
        _qnn_system_handle = nullptr;
    }

    int dlclose_error = dlclose(_system_lib_handle);
    if (dlclose_error != 0) {
        LOGGW("failed to close QnnSystem library, error %s\n", dlerror());
        return 2;
    }

    _system_lib_handle = nullptr;
    LEAVE_FUNC();

    return 0;
}


int qnn_implementation::load_prebuit_qnn_model() {

    LOGGD("model name %s\n", _model_name.c_str());
    if (_model_name.empty()) {
        LOGGD("model name is empty\n");
        return 0;
    }

    void * model_handle = dlopen(_model_name.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (nullptr == model_handle) {
        LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
        return 1;
    }

    _model_lib_handle = model_handle;

    _qnn_composegraphs_handle = (ComposeGraphsHandleType_t)dlsym(_model_lib_handle, "QnnModel_composeGraphs");
    if (nullptr == _qnn_composegraphs_handle) {
        LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
        dlclose(_model_lib_handle);
        _model_lib_handle = nullptr;
        return 2;
    }

    _qnn_freegraphs_handle = (FreeGraphsHandleType_t)dlsym(_model_lib_handle, "QnnModel_freeGraphsInfo");
    if (nullptr == _qnn_freegraphs_handle) {
        LOGGW("failed to load prebuilt qnn model:%s, error:%s\n", _model_name.c_str(), dlerror());
        dlclose(_model_lib_handle);
        _model_lib_handle = nullptr;
        return 3;
    }

    return 0;
}


int qnn_implementation::unload_prebuilt_qnn_model() {
    if (nullptr != _model_lib_handle) {
        dlclose(_model_lib_handle);
        _model_lib_handle           = nullptr;
        _qnn_composegraphs_handle   = nullptr;
        _qnn_freegraphs_handle      = nullptr;
    }

    return 0;
}


static void ggml_qnn_logcallback(const char* fmt,
                                 QnnLog_Level_t level,
                                 uint64_t timestamp,
                                 va_list argp) {

    //don't care static variable in PoC stage
    static std::mutex _log_mutex;
    static unsigned char s_qnn_jni_buf[JNI_BUF_LEN];

    const char * levelStr = "";
    switch (level) {
        case QNN_LOG_LEVEL_ERROR:
            levelStr = " ERROR ";
            break;
        case QNN_LOG_LEVEL_WARN:
            levelStr = "WARNING";
            break;
        case QNN_LOG_LEVEL_INFO:
            levelStr = "  INFO ";
            break;
        case QNN_LOG_LEVEL_DEBUG:
            levelStr = " DEBUG ";
            break;
        case QNN_LOG_LEVEL_VERBOSE:
            levelStr = "VERBOSE";
            break;
        case QNN_LOG_LEVEL_MAX:
            levelStr = "UNKNOWN";
            break;
    }

    double ms = (double)timestamp / 1000000.0;

    {
        std::lock_guard<std::mutex> lock(_log_mutex);

        int len_content = 0;
        memset(s_qnn_jni_buf, 0, JNI_BUF_LEN);
        len_content = vsnprintf(reinterpret_cast<char *const>(s_qnn_jni_buf), JNI_BUF_LEN, fmt, argp);
        //snprintf(reinterpret_cast<char *const>(s_qnn_jni_buf + len_content), JNI_BUF_LEN - len_content, "\n");
        LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
        //if (level <= QNN_LOG_LEVEL_INFO)
        {
            GGML_JNI_NOTIFY("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
        }
    }
}


//TODO:error handle & memory leak handle
int qnn_implementation::qnn_init(const QnnSaver_Config_t ** saver_config) {
    BackendIdType backend_id = QNN_BACKEND_ID_NULL;
    LOGGD("enter qni_init\n");

    const std::lock_guard<std::mutex> lock(_init_mutex);

    if (0 != load_system()) {
        LOGGW("can not load QNN system lib, pls check why?\n");
        return 1;
    } else {
        LOGGD("load QNN system lib succesfully\n");
    }

    if (0 != load_prebuit_qnn_model()) {
        LOGGW("can not load prebuilt QNN model, pls check why?\n");
        return 1;
    }

    std::string bakend_lib_path = _lib_path + _backend_name;
    if (0 == _lib_path_to_backend_id.count(bakend_lib_path)) {
        int is_load_ok = load_backend(bakend_lib_path, saver_config);
        if (0 != is_load_ok) {
            LOGGW("failed to load QNN backend\n");
            return 2;
        }
    }

    backend_id = _lib_path_to_backend_id[bakend_lib_path];
    if (0 == _loaded_backend.count(backend_id) ||
        0 == _loaded_lib_handle.count(backend_id)) {
        LOGGW("library %s is loaded but loaded backend count=%zu, loaded lib_handle count=%zu\n",
              bakend_lib_path.c_str(),
              _loaded_backend.count(backend_id),
              _loaded_lib_handle.count(backend_id));
        return 3;
    }

    _qnn_interface.set_qnn_interface(_loaded_backend[backend_id]);

    _qnn_interface.qnn_log_create(ggml_qnn_logcallback, _qnn_log_level, &_qnn_log_handle);
    if (nullptr == _qnn_log_handle) {
        LOGGW("why failed to initialize qnn log\n");
        return 4;
    } else {
        LOGGD("initialize qnn log successfully\n");
    }

    std::vector<const QnnBackend_Config_t*> temp_backend_config; //TODO:now is empty because I don't know how to use QnnBackend_Config_t currently
    _qnn_interface.qnn_backend_create(_qnn_log_handle, temp_backend_config.empty() ? nullptr : temp_backend_config.data(), &_qnn_backend_handle);
    if (nullptr == _qnn_backend_handle) {
        LOGGW("why failed to initialize qnn backend\n");
        return 5;
    } else {
        LOGGD("initialize qnn backend successfully\n");
    }

    if (nullptr != _qnn_raw_interface.propertyHasCapability) {
        auto qnnStatus = _qnn_raw_interface.propertyHasCapability(QNN_PROPERTY_GROUP_DEVICE);
        if (QNN_PROPERTY_NOT_SUPPORTED == qnnStatus) {
            LOGGW("device property is not supported\n");
        }
        if (QNN_PROPERTY_ERROR_UNKNOWN_KEY == qnnStatus) {
            LOGGW("device property is not known to backend\n");
        }
    }

    auto qnnStatus = _qnn_raw_interface.deviceCreate(_qnn_log_handle, nullptr, &_qnn_device_handle);
    if (QNN_SUCCESS != qnnStatus && QNN_DEVICE_ERROR_UNSUPPORTED_FEATURE != qnnStatus) {
        LOGGW("failed to create QNN device\n");
        GGML_JNI_NOTIFY("failed to create QNN device\n");
    } else {
        LOGGI("create device successfully\n");
    }

    /*
    std::vector<const QnnDevice_Config_t*> temp_device_config;  //TODO:now is empty because I don't know how to use QnnDevice_Config_t currently
    _qnn_interface.qnn_device_create(_qnn_log_handle, temp_device_config.empty() ? nullptr : temp_device_config.data(), &_qnn_device_handle);
    if (nullptr == _qnn_device_handle) {
        LOGGW("why failed to initialize qnn device\n");
        //return 6;
    }
    */

    if (ggml_qnn_profile_level::profile_off != _profile_level) {
        LOGGI("profiling turned on; level = %d", _profile_level);
        if (ggml_qnn_profile_level::profile_basic == _profile_level) {
            LOGGI("basic profiling requested. creating Qnn Profile object\n");
            if (QNN_PROFILE_NO_ERROR != _qnn_raw_interface.profileCreate(
                    _qnn_backend_handle, QNN_PROFILE_LEVEL_BASIC, &_qnn_profile_handle)) {
                LOGGW("unable to create profile handle in the backend\n");
                return 7;
            } else {
                LOGGD("initialize qnn profile successfully\n");
            }
        } else if (ggml_qnn_profile_level::profile_detail == _profile_level) {
            LOGGI("detailed profiling requested. Creating Qnn Profile object\n");
            if (QNN_PROFILE_NO_ERROR != _qnn_raw_interface.profileCreate(
                    _qnn_backend_handle, QNN_PROFILE_LEVEL_DETAILED, &_qnn_profile_handle)) {
                LOGGW("unable to create profile handle in the backend\n");
                return 7;
            } else {
                LOGGD("initialize qnn profile successfully\n");
            }
        }
    }

    _rpc_lib_handle = dlopen("libcdsprpc.so", RTLD_NOW | RTLD_LOCAL);
    if (nullptr == _rpc_lib_handle) {
        LOGGW("failed to load qualcomm's rpc lib, error:%s\n", dlerror());
        return 9;
    } else {
        LOGGD("load rpcmem lib successfully\n");
        set_rpcmem_initialized(true);
    }
    _pfn_rpc_mem_alloc  = reinterpret_cast<pfn_rpc_mem_alloc>(dlsym(_rpc_lib_handle, "rpcmem_alloc"));
    _pfn_rpc_mem_free   = reinterpret_cast<pfn_rpc_mem_free>(dlsym(_rpc_lib_handle, "rpcmem_free"));
    _pfn_rpc_mem_to_fd  = reinterpret_cast<pfn_rpc_mem_to_fd>(dlsym(_rpc_lib_handle, "rpcmem_to_fd"));
    if (nullptr == _pfn_rpc_mem_alloc || nullptr == _pfn_rpc_mem_free || nullptr == _pfn_rpc_mem_to_fd) {
        LOGGW("unable to access symbols in shared buffer. dlerror(): %s", dlerror());
        dlclose(_rpc_lib_handle);
        return 10;
    }

    std::vector<const QnnContext_Config_t*> temp_context_config; //TODO:now is empty because I don't know how to use QnnContext_Config_t currently
    _qnn_interface.qnn_context_create(_qnn_backend_handle, _qnn_device_handle, temp_context_config.empty() ? nullptr : temp_context_config.data(), &_qnn_context_handle);
    if (nullptr == _qnn_context_handle) {
        LOGGW("why failed to initialize qnn context\n");
        GGML_JNI_NOTIFY("why failed to initialize qnn context\n");
        return 8;
    } else {
        LOGGD("initialize qnn context successfully\n");
    }

    LOGGD("leave qni_init\n");

    return 0;
}


int qnn_implementation::qnn_finalize() {
    int ret_status = 0;
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    ENTER_FUNC();

    if (dlclose(_rpc_lib_handle) != 0) {
        LOGGW("failed to unload qualcomm's rpc lib, error:%s\n", dlerror());
    } else {
        LOGGD("succeed to close rpcmem lib\n");
    }

    VALIDATE(free_cached_tensors(), error);

    if (nullptr != _qnn_context_handle) {
        error = _qnn_interface.qnn_context_free(_qnn_context_handle, _qnn_profile_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN context_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_context_handle = nullptr;
    }

    if (nullptr != _qnn_profile_handle) {
        error = _qnn_interface.qnn_profile_free(_qnn_profile_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN profile_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_profile_handle = nullptr;
    }

    if (nullptr != _qnn_device_handle) {
        error = _qnn_interface.qnn_device_free(_qnn_device_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN device_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));

        }
        _qnn_device_handle = nullptr;
    }

    if (nullptr != _qnn_backend_handle) {
        error = _qnn_interface.qnn_backend_free(_qnn_backend_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN backend_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));
        }
        _qnn_backend_handle = nullptr;

    }

    if (nullptr != _qnn_log_handle) {
        error = _qnn_interface.qnn_log_free(_qnn_log_handle);
        if (error != QNN_SUCCESS) {
            LOGGW("failed to free QNN log_handle: ID %u, error %d\n",
                  _qnn_interface.get_backend_id(), QNN_GET_ERROR_CODE(error));
        }
        _qnn_log_handle = nullptr;
    }

    unload_backend();

    unload_system();

    unload_prebuilt_qnn_model();

    LEAVE_FUNC();

    return ret_status;
}


int qnn_implementation::add_tensor(const char * node_name, Qnn_Tensor_t * p_tensor, bool b_save_tensor) {
    Qnn_ErrorHandle_t error = QNN_SUCCESS;
    int counts = 0;
    std::string tensor_name;

    if (nullptr == node_name || nullptr == p_tensor) {
        LOGGW("invalid param\n");
        return 1;
    }
    VALIDATE_TENSOR_VERSION((*p_tensor), error);

    std::string map_entry = std::string((QNN_VER_PTR(*p_tensor))->name);
    if (_tensors_map.find(map_entry) != _tensors_map.end()) {
        LOGGW("tensor %s already exists on node %s\n", map_entry.c_str(), node_name);
        return 2;
    }

    const std::map<Qnn_DataType_t, float> data_type_to_size = {
            {QNN_DATATYPE_INT_8, 1},
            {QNN_DATATYPE_INT_16, 2},
            {QNN_DATATYPE_INT_32, 4},
            {QNN_DATATYPE_INT_64, 8},
            {QNN_DATATYPE_UINT_8, 1},
            {QNN_DATATYPE_UINT_16, 2},
            {QNN_DATATYPE_UINT_32, 4},
            {QNN_DATATYPE_UINT_64, 8},
            {QNN_DATATYPE_FLOAT_16, 2},
            {QNN_DATATYPE_FLOAT_32, 4},
            {QNN_DATATYPE_FLOAT_64, 8},
            {QNN_DATATYPE_BOOL_8, 1},
            {QNN_DATATYPE_SFIXED_POINT_4, 0.5},
            {QNN_DATATYPE_SFIXED_POINT_8, 1},
            {QNN_DATATYPE_SFIXED_POINT_16, 2},
            {QNN_DATATYPE_SFIXED_POINT_32, 4},
            {QNN_DATATYPE_UFIXED_POINT_4, 0.5},
            {QNN_DATATYPE_UFIXED_POINT_8, 1},
            {QNN_DATATYPE_UFIXED_POINT_16, 2},
            {QNN_DATATYPE_UFIXED_POINT_32, 4},
    };
    if (data_type_to_size.find((QNN_VER_PTR(*p_tensor))->dataType) == data_type_to_size.end()) {
        LOGGW("invalid tensor type\n");
        return 3;
    }

    if ((QNN_VER_PTR(*p_tensor))->type == QNN_TENSOR_TYPE_STATIC) {
        if ((QNN_VER_PTR(*p_tensor))->memType != QNN_TENSORMEMTYPE_RAW) {
            LOGGW("mismatch between tensor type and tensor memtype\n");
            return 4;
        }

        // verify size expressed by the dims matches the raw tensor size
        uint32_t qnn_tensor_size =
                std::lround(std::accumulate(
                        (QNN_VER_PTR(*p_tensor))->dimensions,
                        (uint32_t*)((QNN_VER_PTR(*p_tensor))->dimensions) + (QNN_VER_PTR(*p_tensor))->rank,
                        data_type_to_size.find((QNN_VER_PTR(*p_tensor)->dataType))->second,
                        std::multiplies<float>()));
        LOGGD("qnn tensor size %d\n", qnn_tensor_size);
        qnn_tensor_size =
                std::lround(std::accumulate(QNN_TENSOR_GET_DIMENSIONS(p_tensor),
                                            QNN_TENSOR_GET_DIMENSIONS(p_tensor) + QNN_TENSOR_GET_RANK(p_tensor),
                                            data_type_to_size.find(QNN_TENSOR_GET_DATA_TYPE(p_tensor))->second,
                                            std::multiplies<float>()));

        LOGGD("qnn tensor size %d\n", qnn_tensor_size);
        if (qnn_tensor_size != ((QNN_VER_PTR(*p_tensor)->clientBuf.dataSize))) {
            LOGGW("this is a static tensor %s but length mismatch\n", QNN_VER_PTR(*p_tensor)->name);
            LOGGW("adding STATIC tensor, length mismatch between clientBuf"
                  "size and tensor Dims(dim * rank * sizeof(datatype) for nodeName: %s, tensorName: %s,"
                  "got tensorSize: %d, tensor.clientBuf.dataSize: %d\n",
                  node_name,
                  QNN_TENSOR_GET_NAME(p_tensor),
                  qnn_tensor_size,
                  QNN_TENSOR_GET_CLIENT_BUF(p_tensor).dataSize);

            return 5;
        }
    }

    if (_debug_tensor && QNN_TENSOR_GET_TYPE(*p_tensor) == QNN_TENSOR_TYPE_NATIVE) {
        // for debug, make all tensors accessible by client
        QNN_TENSOR_SET_TYPE(*p_tensor, QNN_TENSOR_TYPE_APP_READ);
    }

    error = _qnn_interface.qnn_tensor_create_graph_tensor(_qnn_graph_handle, p_tensor);
    //error = _qnn_interface.qnn_tensor_create_context_tensor(_qnn_context_handle, p_tensor);
    if (error == QNN_TENSOR_ERROR_NAME_HASH_COLLISION) {
        LOGGW("tensor name \"qnn_matrix\" hash collision\n");
        return 6;
    }

    if (error != QNN_TENSOR_NO_ERROR) {
        LOGGW("add tensor failure\n");
        return 7;
    } else {
        LOGGD("add tensor %s successfully\n", (QNN_VER_PTR(*p_tensor)->name));
    }

    if (b_save_tensor) {
        Qnn_Tensor_t tensor_copy;
        VALIDATE(deepCopyQnnTensors(*p_tensor, tensor_copy), error);

        if ((QNN_VER_PTR(*p_tensor))->type == QNN_TENSOR_TYPE_APP_WRITE) {
            _input_tensors.push_back(tensor_copy);
        } else if ((QNN_VER_PTR(*p_tensor))->type == QNN_TENSOR_TYPE_APP_READ) {
            _output_tensors.push_back(tensor_copy);
        }

        _tensors_map[map_entry] = tensor_copy;
    }


    return 0;
}


int qnn_implementation::add_node(Qnn_OpConfigVersion_t version,
                                 const char *name,
                                 const char *packageName,
                                 const char *type,
                                 Qnn_Param_t *params,
                                 uint32_t numOfParams,
                                 const char **inputNames,
                                 uint32_t numOfInputs,
                                 Qnn_Tensor_t *outputTensors,
                                 uint32_t numOfOutputs) {
    int nodeError = 0;
    Qnn_OpConfig_t opDefinition = QNN_OPCONFIG_INIT;
    opDefinition.version = version;
    VALIDATE_OP_CONFIG_VERSION((opDefinition), nodeError);

    // populate Qnn param for node
    Qnn_Param_t *nodeParams = (Qnn_Param_t *) malloc(numOfParams * sizeof(Qnn_Param_t));

    // populate input tensors for node
    Qnn_Tensor_t *inputs = (Qnn_Tensor_t *) malloc(numOfInputs * sizeof(Qnn_Tensor_t));

    // populate output tensors of node
    Qnn_Tensor_t *outputs = (Qnn_Tensor_t *) malloc(numOfOutputs * sizeof(Qnn_Tensor_t));

    if (nodeParams == nullptr || inputs == nullptr || outputs == nullptr) {
        LOGGW("failed for allocate memory for creating QNN OpConfig for node %s\n", name);
        FREE_MEMORY(nodeParams, inputs, outputs);
        return 1;
    }

    uint32_t nodeParamsCounter = 0;
    for (size_t i = 0; i < numOfParams; i++) {
        switch (params[i].paramType) {
            case QNN_PARAMTYPE_TENSOR: {
                Qnn_Tensor_t &tensor = params[i].tensorParam;
                //TODO: set b_save_tensor to false as no need to save tensor because this function call just for setup params
                nodeError = add_tensor(name, &tensor, false);
                if (nodeError != 0) {
                    if (nodeError != 2) {
                        LOGGW("failed to add tensor %s on node %s\n",
                              QNN_TENSOR_GET_NAME(tensor), name);
                        FREE_MEMORY(nodeParams, inputs, outputs);
                        return nodeError;
                    }
                }
                LOGGI("succeed to add tensor %s on node %s\n", QNN_TENSOR_GET_NAME(tensor), name);
                nodeParams[nodeParamsCounter].paramType     = QNN_PARAMTYPE_TENSOR;
                nodeParams[nodeParamsCounter].name          = params[i].name;
                nodeParams[nodeParamsCounter++].tensorParam = tensor;
                break;
            }
            case QNN_PARAMTYPE_SCALAR: {
                nodeParams[nodeParamsCounter].paramType     = QNN_PARAMTYPE_SCALAR;
                nodeParams[nodeParamsCounter].name          = params[i].name;
                nodeParams[nodeParamsCounter++].scalarParam = params[i].scalarParam;
                break;
            }
            default: {
                LOGGW("unknown param type passed for param %s on node %s\n", params[i].name, name);
                FREE_MEMORY(nodeParams, inputs, outputs);
                return 2;
            }
        }
    }

    size_t inputsCounter = 0;
    for (size_t j = 0; j < numOfInputs; j++) {
        nodeError = get_tensor(name, inputNames[j], inputs[inputsCounter++]);
        if (nodeError != 0) {
            LOGGW("get_tensor() failed for input tensor %s on node %s\n", inputNames[j], name);
            FREE_MEMORY(nodeParams, inputs, outputs);
            return nodeError;
        }
    }

    size_t outputsCounter        = 0;
    _output_tensor_map[name] = {};
    for (size_t k = 0; k < numOfOutputs; k++) {
        // create node output tensors
        nodeError = add_tensor(name, outputTensors[k]);
        if (nodeError != 0) {
            LOGGW("add_tensor() failed for output tensor %s on node %s\n", QNN_TENSOR_GET_NAME(outputTensors[k]), name);
            FREE_MEMORY(nodeParams, inputs, outputs);
            return nodeError;
        } else {
            LOGGI("add_tensor() ok for output tensor %s on node %s\n", QNN_TENSOR_GET_NAME(outputTensors[k]), name);
        }
        const char *outTensorName = QNN_TENSOR_GET_NAME(outputTensors[k]);
        _output_tensor_map[name].push_back(outTensorName);
        nodeError = get_tensor(name, outTensorName, outputs[outputsCounter++]);
        if (nodeError != 0) {
            LOGGW("get_tensor() failed for tensor %s on node %s\n", outTensorName, name);
            FREE_MEMORY(nodeParams, inputs, outputs);
            return nodeError;
        }
    }

    // define and add node to graph
    QNN_OP_CFG_SET_NAME(opDefinition, name);
    QNN_OP_CFG_SET_PACKAGE_NAME(opDefinition, packageName);
    QNN_OP_CFG_SET_TYPE_NAME(opDefinition, type);
    QNN_OP_CFG_SET_PARAMS(opDefinition, numOfParams, nodeParams);
    QNN_OP_CFG_SET_INPUTS(opDefinition, numOfInputs, inputs);
    QNN_OP_CFG_SET_OUTPUTS(opDefinition, numOfOutputs, outputs);

    if (_do_node_validations) {
        auto validationStatus = _qnn_interface.qnn_backend_validate_op_config(_qnn_backend_handle, opDefinition); //TODO: not work
        if (validationStatus == QNN_BACKEND_ERROR_NOT_SUPPORTED) {
            LOGGD("validation API not supported\n");
        } else if (validationStatus != QNN_SUCCESS) {
            LOGGW("validating op config %s failed\n", name); //TODO:[ ERROR ] OpConfig validation failed for ElementWiseAdd
            //FREE_MEMORY(nodeParams, inputs, outputs);
            //return 3;
        }
    }

    if (_qnn_interface.qnn_graph_add_node(_qnn_graph_handle, opDefinition) != QNN_GRAPH_NO_ERROR) {
        LOGGW("adding node %s failed\n", name);
        GGML_JNI_NOTIFY("adding qnn graph node %s failed", name);
        FREE_MEMORY(nodeParams, inputs, outputs);
        return 4;
    } else {
        LOGGW("adding qnn graph node %s succesfully\n", name);
    }

    FREE_MEMORY(nodeParams, inputs, outputs);

    return 0;
}


int qnn_implementation::init_qnn_model(const char * graph_name, bool debug, uint8_t do_node_validation, const QnnGraph_Config_t ** graph_configs) {
    int result = 0;

    ENTER_FUNC();

    if (nullptr == graph_name) {
        LOGGW("graph name is null\n");
        return 1;
    }

    if (!_graph_name.empty()) {
        //TODO: only one graph is allowed
        LOGGW("qnn model for graph %s already initialized\n", graph_name);
        return 2;
    }

    if (!do_node_validation) {
        LOGGW("node validation disabled, backend will not perform op validation prior to adding node\n");
    }

    _graph_name = graph_name;
    _debug_tensor = debug;
    _do_node_validations = do_node_validation;

    result = _qnn_raw_interface.graphCreate(_qnn_context_handle, graph_name, graph_configs, &_qnn_graph_handle);
    if (result != QNN_GRAPH_NO_ERROR || nullptr == _qnn_graph_handle) {
        LOGGW("failed to create graph in qnn context\n");
        return 3;
    } else {
        LOGGI("succeed to create graph %s, %p\n", graph_name, _qnn_graph_handle);
    }

    LEAVE_FUNC();
    return 0;
}


int qnn_implementation::get_graphinfo_from_model(GraphInfoPtr_t ** graphsInfo, uint32_t * numGraphsInfo) {
    int  err = 0;
    int  numModels = 1;

    ENTER_FUNC();

    *graphsInfo = (GraphInfo_t **)malloc(numModels * sizeof(GraphInfo_t *));
    if (nullptr == *graphsInfo) {
        LOGGW("malloc failure\n");
        return 1;
    }

    GraphInfo_t *graphArr = (GraphInfo_t *)malloc(numModels * sizeof(GraphInfo_t));
    if (nullptr == graphArr) {
        LOGGW("malloc failure\n");
        return 2;
    }

    for (uint32_t i = 0; i < numModels; i++) {
        graphArr[i].graph = _qnn_graph_handle;
        graphArr[i].graphName = ::strndup(_graph_name.c_str(), _graph_name.size());
        if (nullptr == graphArr[i].graphName) {
            LOGGW("graph name is null\n");
            return 3;
        }

        // allocate and add graph input/output TensorsWrapper. Note: no need to make deep copies of
        // the tensor's pointer members as they are already allocated on heap in the addTensor
        // function call.
        std::vector<Qnn_Tensor_t> graphInputTensors = get_graph_input_tensors();
        size_t numInputTensors                      = graphInputTensors.size();
        LOGGD("numInputTensors %d\n", numInputTensors);
        size_t inputTensorsSize                     = numInputTensors * sizeof(Qnn_Tensor_t);
        graphArr[i].inputTensors                    = (Qnn_Tensor_t *)malloc(inputTensorsSize);
        memscpy(graphArr[i].inputTensors, inputTensorsSize, graphInputTensors.data(), inputTensorsSize);
        graphArr[i].numInputTensors = (uint32_t)numInputTensors;
        // allocate and add graph outputTensors
        std::vector<Qnn_Tensor_t> graphOutputTensors = get_graph_output_tensors();
        size_t numOutputTensors                      = graphOutputTensors.size();
        LOGGD("numOutputTensors %d\n", numOutputTensors);
        size_t outputTensorsSize                     = numOutputTensors * sizeof(Qnn_Tensor_t);
        graphArr[i].outputTensors                    = (Qnn_Tensor_t *)malloc(outputTensorsSize);
        memscpy(graphArr[i].outputTensors, outputTensorsSize, graphOutputTensors.data(), outputTensorsSize);
        graphArr[i].numOutputTensors = (uint32_t)numOutputTensors;

        // have return object point to the populated graph struct
        (*graphsInfo)[i] = graphArr + i;

        // graph composition is complete by this stage, free if any cached tensors remaining
        free_cached_tensors();
    }

    *numGraphsInfo = numModels;

    LEAVE_FUNC();

    return err;
}


// compose all QNN graphs from prebuilt Qualcomm's dedicated MODEL.so which generated by Qualcomm's dedicated tool
int qnn_implementation::compose_special_graph() {
    ENTER_FUNC();

    int result = 0;

    if (nullptr == _qnn_composegraphs_handle) {
        LOGGW("no prebuilt QNN model\n");
        LEAVE_FUNC();
        return 0;
    }

    result = _qnn_composegraphs_handle(
            _qnn_backend_handle,
            _qnn_raw_interface,
            _qnn_context_handle,
            (const GraphConfigInfo_t **) nullptr,
            0,
            &_graphs_info,
            &_graphs_count,
            _debug_tensor,
            ggml_qnn_logcallback,
            _qnn_log_level);
    if (result != 0) {
        LOGGW("failed to compose graphs via prebuilt QNN mode:%s\n", _model_name.c_str());
        result = 1;
    }

    LEAVE_FUNC();

    return result;
}


// execute_special_graph() is currently used by qnn-sample-app's main.cpp.
// This function runs all the graphs present in model.so by reading
// inputs from input_list based files and writes output to .raw files.
int qnn_implementation::execute_special_graph() {
    ENTER_FUNC();
    int return_value                                = 0;
    bool read_success                               = false;
    std::string input_list_path                     = "/sdcard/kantv/raw_list.txt";
    std::string output_path                         = "/sdcard/kantv/out/";

    std::vector<std::string> input_list_paths;
    std::vector<std::vector<std::vector<std::string>>> input_file_lists;
    std::vector<std::unordered_map<std::string, uint32_t>> input_name_to_index;


    OutputDataType output_datatype                  = OutputDataType::FLOAT_ONLY;
    InputDataType input_datatype                    = InputDataType::FLOAT;

    split(input_list_paths, input_list_path, ',');


    std::tie(input_file_lists, input_name_to_index, read_success) = readInputLists(input_list_paths);
    if (!read_success) {
        LOGGW("can not read data from file\n");
        return 1;
    } else {
        LOGGD("succeed to read data from file\n", input_list_path.c_str());
    }
    LOGGD("input_file_lists.size() %d", input_file_lists.size());

    for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
        LOGGD("Starting execution for graph_idx: %d", graph_idx);
        if (graph_idx >= input_file_lists.size()) {
            LOGGW("No Inputs available for: %d", graph_idx);
            return_value = 1;
            break;
        }

        Qnn_Tensor_t *inputs    = nullptr;
        Qnn_Tensor_t *outputs   = nullptr;
        if (0 != _io_tensor.setupInputAndOutputTensors(&inputs, &outputs, (*_graphs_info)[graph_idx])) {
            LOGGE("Error in setting up Input and output Tensors for graph_idx: %d", graph_idx);
            return_value = 1;
            break;
        }
        auto graphs_info = (*_graphs_info)[graph_idx];
        auto input_file_list = input_file_lists[graph_idx];
        if (!input_file_list.empty()) {
            size_t total_count = input_file_list[0].size();
            size_t input_file_index_offset = 0;
            while (input_file_index_offset < total_count) {
                int iotReturnStatus;
                size_t numInputFilesPopulated;
                size_t batchSize;
                std::tie(iotReturnStatus, numInputFilesPopulated, batchSize) =
                        _io_tensor.populateInputTensors(graph_idx,
                                                        input_file_list,
                                                        input_file_index_offset,
                                                        false,
                                                        input_name_to_index[graph_idx],
                                                        inputs,
                                                        graphs_info,
                                                        input_datatype);
                if (0 != iotReturnStatus) {
                    return_value = 1;
                }
                if (0 == return_value) {
                    LOGGD("successfully populated input tensors for graph_idx: %d", graph_idx);
                    GGML_JNI_NOTIFY("successfully populated input tensors for graph_idx: %d", graph_idx);
                    Qnn_ErrorHandle_t executeStatus = QNN_GRAPH_NO_ERROR;
                    executeStatus = _qnn_raw_interface.graphExecute(graphs_info.graph,
                                                                    inputs,
                                                                    graphs_info.numInputTensors,
                                                                    outputs,
                                                                    graphs_info.numOutputTensors,
                                                                    _qnn_profile_handle,
                                                                    nullptr);
                    if (QNN_GRAPH_NO_ERROR != executeStatus) {
                        return_value = 1;
                    }
                    if (0 == return_value) {
                        LOGGD("successfully executed graph_idx: %d ", graph_idx);
                        GGML_JNI_NOTIFY("successfully executed graph_idx: %d ", graph_idx);

                        if (0 !=
                            _io_tensor.writeOutputTensors(graph_idx,
                                                          input_file_index_offset,
                                                          graphs_info.graphName,
                                                          outputs,
                                                          graphs_info.numOutputTensors,
                                                          output_datatype,
                                                          _graphs_count,
                                                          output_path,
                                                          numInputFilesPopulated,
                                                          batchSize)) {
                            return_value = 1;
                        }
                    }
                    input_file_index_offset += numInputFilesPopulated;
                }
                if (0 != return_value) {
                    LOGGE("execution of graph: %d failure\n", graph_idx);
                    break;
                }
            }
        }
        _io_tensor.tearDownInputAndOutputTensors(inputs, outputs, graphs_info.numInputTensors, graphs_info.numOutputTensors);
        inputs = nullptr;
        outputs = nullptr;
        if (0 != return_value) {
            break;
        }
    }

    freeGraphsInfo(&_graphs_info, _graphs_count);
    _graphs_info = nullptr;

    LEAVE_FUNC();

    return return_value;
}


int qnn_implementation::execute_common_graph(const std::vector<uint8_t *> & input_node_values,
                                             std::vector<float *> & output_node_values,
                                             std::vector<size_t> output_node_sizes) {

    int result                      = 0;
    float * output_tmp              = nullptr;

    InputDataType input_datatype    = InputDataType::FLOAT;
    OutputDataType output_datatype  = OutputDataType::FLOAT_ONLY;

    LOGGI("enter %s\n", __func__);

    LOGGD("graph count %d\n", _graphs_count);

    if (1 != _graphs_count) {
        LOGGW("only support one graph current\n");
        result = 1;
        return 1;
    }

    for (size_t graph_idx = 0; graph_idx < _graphs_count; graph_idx++) {
        LOGGI("starting setup input and output tensor for graph : %zu", graph_idx);

        Qnn_Tensor_t *inputs    = nullptr;
        Qnn_Tensor_t *outputs   = nullptr;
        if (0 != _io_tensor.setupInputAndOutputTensors(&inputs, &outputs, (*_graphs_info)[graph_idx])) {
            LOGGW("failed to setting up input and output QNN Tensors for graph_idx: %zu", graph_idx);
            result  = 2;
        }

        auto graph_info = (*_graphs_info)[graph_idx];
        std::vector<uint8_t *> input_vec;

        if (0 == result) {
            for (auto input_node_value : input_node_values) {
                input_vec.push_back(input_node_value);
            }

            if (0 != _io_tensor.populateInputTensors(graph_idx, input_vec, inputs, graph_info, input_datatype)) {
                LOGGE("failed to populateInputTensors");
                result  = 3;
            }
        }

        if (0 == result) {
            LOGGD("successfully populated input tensors for graph_idx: %zu", graph_idx);
            result  = _qnn_raw_interface.graphExecute(graph_info.graph,
                                                      inputs,
                                                      graph_info.numInputTensors,
                                                      outputs,
                                                      graph_info.numOutputTensors,
                                                      _qnn_profile_handle,
                                                      nullptr);


            if (0 != result) {
                LOGGE("graph execution failure\n");
                GGML_JNI_NOTIFY("graph execution failure");
                result = 4;
            } else {
                LOGGI("got it, qnn graph execution successfully\n"); //04-05-2024(April,5,2024),13:00, but the compute result is not correct
                GGML_JNI_NOTIFY("qnn graph execution successfully");
            }

            for (int i = 0; i < graph_info.numOutputTensors; ++i) {
                if (nullptr == output_node_values.at(i)) {
                    LOGGW("output node is null\n");
                    continue;
                }

                output_tmp              = nullptr;

                if (0 != result)
                    continue;

                if (0 !=  _io_tensor.converQnntensortoFloatBuffer(&(outputs[i]), &output_tmp)) {
                    LOGGE("error in convert output at idx %d", i);
                    result = 5;
                }

                if ((0 == result) && (nullptr != output_tmp)) {
                    memcpy(output_node_values.at(i), output_tmp, output_node_sizes[i] * sizeof(float));
                    free(output_tmp);
                    output_tmp = nullptr;
                }

            }
        }

        _io_tensor.tearDownInputAndOutputTensors(inputs, outputs, graph_info.numInputTensors, graph_info.numOutputTensors);

        inputs  = nullptr;
        outputs = nullptr;
    }

    if (0 != result) {
        LOGGE("execute common graph failure\n");
        GGML_JNI_NOTIFY("execute common graph failure");
    }
    LOGGI("leave %s\n", __func__);


    return result;
}


int qnn_implementation::finalize_qnn_model() {
    ENTER_FUNC();
    if (nullptr != _qnn_graph_handle) {
        if (_qnn_raw_interface.graphFinalize(_qnn_graph_handle, _qnn_profile_handle, nullptr) !=
            QNN_GRAPH_NO_ERROR) {
            LOGGW("finalizing graph failure\n");
            //return 1;
        }
        _qnn_graph_handle = nullptr;
    } else {
        LOGGD("qnn graph handle is null\n");
    }

    free_cached_tensors();

    LEAVE_FUNC();

    return 0;
}


extern "C" int ggml_qqn_debug_composegraphs(GraphInfoPtr_t ** graphsInfo,uint32_t * numGraphsInfo);
// this function is used to troubleshooting issue in
// PoC-S26: offload a simple f32 2x2 matrix addition operation to QNN CPU backend
// https://github.com/zhouwg/kantv/issues/121
int qnn_implementation::run_qnn_matrix() {
    LOGGD("enter run_qnn_matrix\n");

    int result = 0;

    //attention here
    int32_t input_matrix[2][4]  = { {1, 1, 1, 1}, {2, 2, 2, 2}};
    float   output_matrix[1][4] = { {1.0,1.0,1.0,1.0} };

    Qnn_Tensor_t tensor_0               = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1               = QNN_TENSOR_INIT;

    input_node_values.push_back((uint8_t*)input_matrix[0]);
    input_node_values.push_back((uint8_t*)input_matrix[1]);

    output_node_values.push_back(output_matrix[0]);

    output_node_sizes.push_back(4); // 2 * 2 = 4

    init_qnn_model("qnn_matrix_addition", true);

    //step-1
    //compose_special_graph();
    //step-2
    //ggml_qqn_debug_composegraphs(&_graphs_info, &_graphs_count);
    //get_graphinfo_from_model(&_graphs_info, &_graphs_count);
    //finalize_graph();
    //execute_special_graph();

    //step-3
    //attention here
    uint32_t dimensions_input_0[] = {2, 2};
    uint32_t dimensions_input_1[] = {2, 2};
    uint32_t dimensions_output[]  = {2, 2};
    uint32_t temp_buf[1024]       = { 0 };

    tensor_0 =  {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=0,
                    .name= "tensor_0",
                    .type= QNN_TENSOR_TYPE_APP_WRITE,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions=dimensions_input_0,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= { .data=nullptr,
                            .dataSize=0}}}}
    };

    tensor_1 =  {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=0,
                    .name= "tensor_1",
                    .type= QNN_TENSOR_TYPE_APP_WRITE,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions=dimensions_input_1,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= { .data=nullptr,
                            .dataSize=0}}}}
    };

    Qnn_Param_t qnn_params[] = {
            {.paramType=QNN_PARAMTYPE_TENSOR,
                    .name="node_param_0",
                    {.tensorParam=(Qnn_Tensor_t) {
                            .version= QNN_TENSOR_VERSION_1,
                            {.v1= {
                                    .id=0,
                                    .name= "param_0",
                                    .type= QNN_TENSOR_TYPE_STATIC,
                                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                    .dataType= QNN_DATATYPE_UINT_32,
                                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                                    .rank= 2,
                                    .dimensions=dimensions_input_0,
                                    .memType= QNN_TENSORMEMTYPE_RAW,
                                    {.clientBuf= { .data=(uint8_t*)temp_buf,
                                            .dataSize=16}}}}}}},

            {.paramType=QNN_PARAMTYPE_TENSOR,
                    .name="node_param_1",
                    {.tensorParam=(Qnn_Tensor_t) {
                            .version= QNN_TENSOR_VERSION_1,
                            {.v1= {
                                    .id=0,
                                    .name= "param_1",
                                    .type= QNN_TENSOR_TYPE_STATIC,
                                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                    .dataType= QNN_DATATYPE_UINT_32,
                                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                                    .rank= 2,
                                    .dimensions=dimensions_input_1,
                                    .memType= QNN_TENSORMEMTYPE_RAW,
                                    {.clientBuf= { .data=(uint8_t*)temp_buf + 32,
                                            .dataSize=16}}}}}}},

    };

    const char * inputs[] = {
            "tensor_0",
            "tensor_1"
    };

    Qnn_Tensor_t  tensor_2 = (Qnn_Tensor_t) {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=0,
                    .name= "tensor_2",
                    .type= QNN_TENSOR_TYPE_APP_READ,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                       QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                       {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions= dimensions_output,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= { .data=nullptr,
                            .dataSize=0}}}}};

    Qnn_Tensor_t outputs[] = {
            tensor_2
    };

    Qnn_Param_t context_params[] = {}; // 04-05-2024(April-5,2024), to make QNN SDK happy

    add_tensor("qnn_matrix_addtition", &tensor_0);
    add_tensor("qnn_matrix_addtition", &tensor_1);
    add_node(QNN_OPCONFIG_VERSION_1, // Op_Config_t Version
             get_qnn_graph_name().c_str(), // Node Name
             QNN_OP_PACKAGE_NAME_QTI_AISW, // Package Name,  must be "qti.aisw" otherwise error occurs
             QNN_OP_ELEMENT_WISE_ADD, // Qnn Node Type,
             context_params, // Node Params
             0, // Num Node Params
             inputs, // Input Tensor Names
             2, // Num Input Tensor Names
             outputs, // Output Tensors
             1// Num Output Tensors
    );
    get_graphinfo_from_model(&_graphs_info, &_graphs_count);

    LOGGD("graphs count %d\n", _graphs_count);
    finalize_graph();

    //TODO:hardcode in PoC stage, shape of input matrix is 2x2
    for (size_t i = 0; i < input_node_values.size(); i++) {
        if (0 == i) {
            LOGGD("input matrix A:\n");
            GGML_JNI_NOTIFY("input matrix A:");
        } else if (1 == i) {
            LOGGD("input matrix B:\n");
            GGML_JNI_NOTIFY("input matrix B:");
        }
        uint32_t *temp = (uint32_t*)input_node_values.at(i);
        LOGGD("%d \t %d\n", temp[0], temp[1]);
        LOGGD("%d \t %d\n", temp[2], temp[3]);
        GGML_JNI_NOTIFY("%d \t %d\n", temp[0], temp[1]);
        GGML_JNI_NOTIFY("%d \t %d\n", temp[2], temp[3]);
    }

    result = execute_common_graph(input_node_values, output_node_values, output_node_sizes);
    if (0 == result) {
        //TODO:hardcode in PoC stage, shape of output matrix is 2x2
        LOGGD("output matrix:\n");
        GGML_JNI_NOTIFY("output matrix:");
        for (size_t i = 0; i < output_node_values.size(); i++) {
            float *temp = output_node_values.at(i);
            LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
            LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
            GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[0], temp[1]);
            GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[2], temp[3]);
        }
        LOGGD("\n");
    }

    input_node_values.clear();
    output_node_values.clear();

    finalize_qnn_model();

    LOGGD("leave run_qnn_matrix\n");

    return 0;
}


// =================================================================================================
//
// JNI helper function for PoC#121:Add Qualcomm mobile SoC native backend for GGML(https://github.com/zhouwg/kantv/issues/121)
// should move into ggml-jni-impl.cpp in the future
//
// =================================================================================================

// https://github.com/zhouwg/kantv/issues/121
// PoC-S26: offload a simple f32 2x2 matrix addition operation to QNN CPU backend

// done on 04-07-2024,17:00(April-07-2024,17:00), not perfect because there are some unresolved problems
// in this function but there are more much valuable things in next steps)

//qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", "libQnnCpu.so", "");
int qnn_matrix(int n_backend_type, int n_op_type) {
    int result                      = 0;
    std::string graph_name          = "qnn_matrix";
    const char * qnn_backend_lib    = "libQnnCpu.so";
    uint32_t matrix_input_0[]       = {1, 2, 3, 4};
    uint32_t matrix_input_1[]       = {1, 2, 3, 4};

    int64_t  n_begin_time           = 0LL;
    int64_t  n_end_time             = 0LL;
    int64_t  n_durtion              = 0LL;


    auto is_io_tensor = [](Qnn_TensorType_t type) {
        return type < QNN_TENSOR_TYPE_STATIC;
    };
    uint8_t * qnn_buffer                = nullptr;
    Qnn_Tensor_t tensor_0               = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1               = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_2               = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param = QNN_QUANTIZE_PARAMS_INIT;
    Qnn_OpConfig_t qnn_opconfig         = QNN_OPCONFIG_INIT;


    ggml_time_init();
    n_begin_time                        = ggml_time_us();
    LOGGD("enter qnn_matrix\n");
    LOGGI("qnn matrix addition operation, backend type:%d(%s), op type:%d\n",
          n_backend_type, get_qnn_backend_name(n_backend_type), n_op_type);
    GGML_JNI_NOTIFY("qnn matrix addition operation, backend_type:%d(%s), op type:%d",
                    n_backend_type, get_qnn_backend_name(n_backend_type), n_op_type);

    switch (n_backend_type) {
        case 0:
            qnn_backend_lib = "libQnnCpu.so";
            break;

        case 1:
            qnn_backend_lib = "libQnnGpu.so";
            break;

        case 2: {
            qnn_backend_lib = "libQnnHtp.so";
            std::string path = "/data/data/com.cdeos.kantv/";
            LOGGI("path:%s\n", path.c_str());
            LOGGI("qnn backend lib:%s\n", qnn_backend_lib);
            if (0 == setenv("LD_LIBRARY_PATH",
                            (path + ":/vendor/dsp/cdsp:/vendor/lib64:/vendor/dsp/dsp:/vendor/dsp/images").c_str(),
                            1)) {
                LOGGI("QNN DSP backend setenv successfully");
            } else {
                LOGGE("QNN DSP backend setenv failure");
            }
            if (0 == setenv("ADSP_LIBRARY_PATH",
                            (path + ";/vendor/dsp/cdsp;/vendor/lib/rfsa/adsp;/system/lib/rfsa/adsp;/vendor/dsp/dsp;/vendor/dsp/images;/dsp").c_str(),
                            1)) {
                LOGGI("QNN DSP backend setenv successfully");
            } else {
                LOGGE("QNN DSP backend setenv failure");
            }
            break;
        }
        case 3:
            //qnn_backend_lib = "libQnnSaver.so"; //not used since 04-08-2024(Apri,08,2024) because "PoC-S26: offload a simple f32 2x2 matrix addition operation to QNN CPU backend" done
            // fall into ggml
            break;

        default:
            LOGGW("backend type %d not supported\n", n_backend_type);
            break;
    }

    if (3 == n_backend_type) {  //this "fake" backend is just used for compare performance
        const int sizey                             = 2;
        const int sizex                             = 2;
        size_t ctx_size                             = 0;
        struct ggml_context * ctx                   = nullptr;
        struct ggml_tensor  * m0                    = nullptr;
        struct ggml_tensor  * m1                    = nullptr;
        struct ggml_tensor  * m2                    = nullptr;
        struct ggml_cgraph  * gf                    = nullptr;
        std::vector<uint8_t> work_buffer;
        const ggml_type qtype                       = GGML_TYPE_F32;
        struct ggml_init_params params = {
                /*.mem_size   =*/ 0,
                /*.mem_buffer =*/ NULL,
                /* no_alloc   =*/ 0
        };

        ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex * sizey);
        ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex * sizey);
        ctx_size        += ggml_row_size(qtype,         sizex * sizey);
        ctx_size        += ggml_row_size(qtype,         sizex * sizey);
        ctx_size        += 1024 * 1024 * 16;
        params.mem_size =  ctx_size;
        ctx             =  ggml_init(params);
        if (!ctx) {
            LOGGW("ggml_init failure\n");
            return 1;
        }
        m0              = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
        ggml_set_f32(m0, 1.0f);
        m1              = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
        ggml_set_f32(m1, 2.0f);
        m2              = ggml_add(ctx, m0, m1); // GGML_OP_ADD
        gf              = ggml_new_graph(ctx);
        ggml_build_forward_expand(gf, m2);
        ggml_graph_compute_helper(work_buffer, gf, 4);
        TENSOR_DUMP(m0);
        TENSOR_DUMP(m1);
        TENSOR_DUMP(m2);

        ggml_free(ctx);

        n_end_time  = ggml_time_us();
        n_durtion   = (n_end_time - n_begin_time) / 1000;
        LOGGD("duration of qnn_matrix with fake qnn backend ggml is: %lld milliseconds\n", n_durtion);
        GGML_JNI_NOTIFY("duration of qnn_matrix with fake qnn backend ggml is: %lld milliseconds\n", n_durtion);

        LOGGD("leave qnn_matrix\n");

        return 0;
    }

    LOGGD("qnn_backend:%s\n", qnn_backend_lib);
    //QNN prebuilt model.so not used in this PoC but using QNN lowlevel API directly, so mode name is ""
    qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", qnn_backend_lib, "");
    result = qnn_backend.qnn_init(nullptr);
    if (0 != result) {
        LOGGW("init qnn subsystem failed with qnn backend %s, pls check why\n", get_qnn_backend_name(n_backend_type));
        GGML_JNI_NOTIFY("init qnn subsystem failed with qnn backend %s, pls check why\n", get_qnn_backend_name(n_backend_type));
        result = 1;
        return 1;
    }
    QNN_INTERFACE_VER_TYPE qnn_raw_interface                = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface qnn_interface                             = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
        goto failure;
    }
    qnn_buffer = static_cast<uint8_t *>(qnn_backend.alloc_rpcmem(8192, 4));
    if (nullptr == qnn_buffer) {
        LOGGW("alloc rpcmem failure, %s\n", strerror(errno));
        goto failure;
    } else {
        LOGGD("alloc rpcmem successfully\n");
    }

    if (0) {
        qnn_backend.run_qnn_matrix();   //TODO: QNN pipeline works but result of output tensor is incorrect
    } else {                            //the following workaround method works fine as expect
        int error = 0;
        Qnn_GraphHandle_t graph_handle = nullptr;
        error = qnn_raw_interface.graphCreate(qnn_backend.get_qnn_context_handle(), "qnn_matrix_addition", nullptr, &graph_handle);
        LOGGI("error = %d\n", error);
        uint32_t dimensions_input_0[] = {2, 2};
        uint32_t dimensions_input_1[] = {2, 2};
        uint32_t dimensions_output[]  = {2, 2};

        float input_matrix[2][4]      = { {1, 1, 1, 1}, {2, 2, 2, 2}};
        float output_matrix[1][4]     = { {1.0,1.0,1.0,1.0} };

        tensor_0 =  {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_0",
                        .type= QNN_TENSOR_TYPE_APP_WRITE,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                           QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                           {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= 2,
                        .dimensions=dimensions_input_0,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= { .data=nullptr,
                                .dataSize=0}}}}
        };

        tensor_1 =  {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_1",
                        .type= QNN_TENSOR_TYPE_APP_WRITE,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                           QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                           {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= 2,
                        .dimensions=dimensions_input_1,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= { .data=nullptr,
                                .dataSize=0}}}}
        };

        Qnn_Tensor_t  tensor_2 = (Qnn_Tensor_t) {
                .version= QNN_TENSOR_VERSION_1,
                {.v1= {
                        .id=0,
                        .name= "tensor_2",
                        .type= QNN_TENSOR_TYPE_APP_READ,
                        .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                        .dataType= QNN_DATATYPE_FLOAT_32,
                        .quantizeParams= { QNN_DEFINITION_UNDEFINED,
                                           QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                           {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                        .rank= 2,
                        .dimensions= dimensions_output,
                        .memType= QNN_TENSORMEMTYPE_RAW,
                        {.clientBuf= { .data=nullptr,
                                .dataSize=0}}}}};


        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_0);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_1);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_2);
        LOGGI("error = %d\n", error);

        //here is similar to OpenMAX IL
        QNN_VER_PTR(tensor_0)->clientBuf =  { input_matrix[0], 16};
        QNN_VER_PTR(tensor_1)->clientBuf =  { input_matrix[1], 16};
        QNN_VER_PTR(tensor_2)->clientBuf =  { output_matrix[0], 16};

        //for this single computation graph which contains three nodes(every node is a tensor,
        //the GGML_OP_ADD is the edge of this single computation graph), nullptr is ok
        //
        //tensor_2 = tensor_0 + tensor_1
        //
        // tensor_0
        //          + (GGML_OP_ADD)    =   tensor_2
        // tensor_1
        //
        Qnn_Param_t params[] = {};
        Qnn_Tensor_t tensor_inputs[] = {
                tensor_0,
                tensor_1
        };

        Qnn_Tensor_t tensor_outputs[] = {
                tensor_2
        };

        Qnn_OpConfig_t opconfig = {
                (Qnn_OpConfigVersion_t) 1, .v1 = {
                        "qnn_matrix_addition",
                        QNN_OP_PACKAGE_NAME_QTI_AISW,
                        QNN_OP_ELEMENT_WISE_ADD,//https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-50/MasterOpDef.html#elementwiseadd
                        0,
                        params,
                        2,
                        tensor_inputs,
                        1,
                        tensor_outputs
                }
        };
        error = qnn_raw_interface.graphAddNode(graph_handle, opconfig);
        LOGGI("error = %d\n", error);
        error = qnn_raw_interface.graphFinalize(graph_handle, nullptr, nullptr);
        LOGGI("error = %d\n", error);
        //TODO:hardcode in PoC stage, shape of input matrix is 2x2
        for (size_t i = 0; i < 2; i++) {
            if (0 == i) {
                LOGGD("input matrix A:\n");
                GGML_JNI_NOTIFY("input matrix A:");
            } else if (1 == i) {
                LOGGD("input matrix B:\n");
                GGML_JNI_NOTIFY("input matrix B:");
            }
            float *temp = input_matrix[i];
            LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
            LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
            GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[0], temp[1]);
            GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[2], temp[3]);
        }
        error = qnn_raw_interface.graphExecute(graph_handle, tensor_inputs, 2, tensor_outputs, 1, nullptr, nullptr);
        LOGGI("error = %d\n", error);
        if (0 == error) {
            float * temp = (float*)((QNN_VER_PTR(tensor_2)->clientBuf.data));
            LOGGD("output tensor:%.2f %.2f %.2f %.2f\n", temp[0], temp[1], temp[2], temp[3]);

            if (0 == result) { // works fine as expected at the first time on 04-07(April,7),17:00, 2024
                //TODO:hardcode in PoC stage, shape of output matrix is 2x2
                LOGGD("output matrix:\n");
                GGML_JNI_NOTIFY("output matrix:");
                for (size_t i = 0; i < 1; i++) {
                    float * temp = output_matrix[i];
                    LOGGD("%.2f \t %.2f\n", temp[0], temp[1]);
                    LOGGD("%.2f \t %.2f\n", temp[2], temp[3]);
                    GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[0], temp[1]);
                    GGML_JNI_NOTIFY("%.2f \t %.2f\n", temp[2], temp[3]);
                }
                LOGGD("\n");
            }

        }
    }


    failure:
    qnn_backend.unregister_rpcmem();
    qnn_backend.free_rpcmem(qnn_buffer);
    qnn_backend.qnn_finalize();

    n_end_time  = ggml_time_us();
    n_durtion   = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_matrix is: %lld milliseconds\n", n_durtion);
    GGML_JNI_NOTIFY("duration of qnn_matrix is: %lld milliseconds\n", n_durtion);

    LOGGD("leave qnn_matrix\n");

    return result;
}
//not used because PoC-S26 done
//#include "Inception_v3.cpp"


// https://github.com/zhouwg/kantv/issues/121
// PoC-S29:  mapping ggml_tensor to QNN_tensor and offload a simple 2x2 matrix addition operation to QNN CPU backend
int qnn_ggml(int n_backend_type, int n_ggml_op_type) {
    uint32_t i                                  = 0;
    uint32_t j                                  = 0;
    int error                                   = 0;
    int result                                  = 0;
    std::string graph_name                      = "qnn_ggml";
    const char * qnn_backend_lib                = "libQnnCpu.so";

    int64_t  n_begin_time                       = 0LL;
    int64_t  n_end_time                         = 0LL;
    int64_t  n_durtion                          = 0LL;

    Qnn_GraphHandle_t graph_handle              = nullptr;
    Qnn_Tensor_t tensor_0                       = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1                       = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_2                       = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param         = QNN_QUANTIZE_PARAMS_INIT;
    Qnn_OpConfig_t qnn_opconfig                 = QNN_OPCONFIG_INIT;
    Qnn_Param_t qnn_params[]                    = {};
    const char * qnn_op_typename                = QNN_OP_ELEMENT_WISE_ADD;

    const int sizey                             = 2;
    const int sizex                             = 2;
    size_t ctx_size                             = 0;
    struct ggml_context * ctx                   = nullptr;
    struct ggml_tensor  * m0                    = nullptr;
    struct ggml_tensor  * m1                    = nullptr;
    struct ggml_tensor  * m2                    = nullptr;
    struct ggml_cgraph  * gf                    = nullptr;
    enum ggml_op     ggmlop                     = GGML_OP_ADD;
    std::vector<uint8_t> work_buffer;
    const ggml_type qtype                       = GGML_TYPE_F32;
    struct ggml_init_params params = {
            /*.mem_size   =*/ 0,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

    ggml_time_init();
    n_begin_time                                = ggml_time_us();

    LOGGD("enter qnn_ggml\n");
    LOGGI("[%s], backend type:%d(%s), op type:%d\n", __func__,
          n_backend_type, get_qnn_backend_name(n_backend_type), n_ggml_op_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d(%s), ggml op type:%d", __func__,
                    n_backend_type, get_qnn_backend_name(n_backend_type), n_ggml_op_type);
    switch (n_backend_type) {
        case 0:
            qnn_backend_lib = "libQnnCpu.so";
            break;

        case 1:
            qnn_backend_lib = "libQnnGpu.so";
            break;

        case 2: {
            qnn_backend_lib = "libQnnHtp.so";
            std::string path = "/data/data/com.cdeos.kantv/";
            LOGGI("path:%s\n", path.c_str());
            LOGGI("qnn backend lib:%s\n", qnn_backend_lib);
            if (0 == setenv("LD_LIBRARY_PATH",
                            (path + ":/vendor/dsp/cdsp:/vendor/lib64:/vendor/dsp/dsp:/vendor/dsp/images").c_str(),
                            1)) {
                LOGGI("QNN DSP backend setenv successfully");
            } else {
                LOGGE("QNN DSP backend setenv failure");
            }
            if (0 == setenv("ADSP_LIBRARY_PATH",
                            (path + ";/vendor/dsp/cdsp;/vendor/lib/rfsa/adsp;/system/lib/rfsa/adsp;/vendor/dsp/dsp;/vendor/dsp/images;/dsp").c_str(),
                            1)) {
                LOGGI("QNN DSP backend setenv successfully");
            } else {
                LOGGE("QNN DSP backend setenv failure");
            }
            break;
        }
        case 3:
            // fall into ggml, just for compare performance between QNN SDK and original GGML
            break;

        default:
            LOGGW("backend type %d not supported\n", n_backend_type);
            break;
    }

    ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex * sizey);
    ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex * sizey);
    ctx_size        += ggml_row_size(qtype,         sizex * sizey);
    ctx_size        += ggml_row_size(qtype,         sizex * sizey);
    ctx_size        += 1024 * 1024 * 16;
    params.mem_size =  ctx_size;
    ctx             =  ggml_init(params);
    if (!ctx) {
        LOGGW("ggml_init failure\n");
        return 1;
    }

    //construct input tensor, this is graph node in a computation graph
    m0              = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m0, 1.0f);
    m1              = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m1, 2.0f);

    //https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-50/SupportedOps.html
    switch (n_ggml_op_type) {
        case GGML_OP_ADD:
            ggmlop          = GGML_OP_ADD;              //this is edge of a computation graph
            m2              = ggml_add(ctx, m0, m1);    //construct output tensor, another graph node in this single computation graph
            qnn_op_typename = QNN_OP_ELEMENT_WISE_ADD;
            break;

        case GGML_OP_MUL:
            ggmlop          = GGML_OP_MUL;
            m2              = ggml_mul(ctx, m0, m1);
            qnn_op_typename = QNN_OP_ELEMENT_WISE_MULTIPLY;
            break;

        case GGML_OP_MUL_MAT:
            ggmlop          = GGML_OP_MUL_MAT;
            m2              = ggml_mul_mat(ctx, m0, m1);
            qnn_op_typename = QNN_OP_MAT_MUL;
            break;

        default:
            LOGGD("only ADD, MUL, MUL_MAT supported currently");
            GGML_JNI_NOTIFY("only ADD, MUL, MUL_MAT supported currently");
            return 0;
    }


    if (3 == n_backend_type) {  // this "fake" backend is just used for compare performance between QNN SDK and original GGML
        gf              = ggml_new_graph(ctx);
        ggml_set_f32(m2, 0.0f);
        ggml_build_forward_expand(gf, m2);
        ggml_graph_compute_helper(work_buffer, gf, 4);

        TENSOR_DUMP(m0);
        TENSOR_DUMP(m1);
        TENSOR_DUMP(m2);

        ggml_free(ctx);

        n_end_time  = ggml_time_us();
        n_durtion   = (n_end_time - n_begin_time) / 1000;
        LOGGD("duration of qnn_ggml with fake qnn backend ggml is: %lld milliseconds\n", n_durtion);
        GGML_JNI_NOTIFY("duration of qnn_ggml with fake qnn backend ggml is: %lld milliseconds\n", n_durtion);

        LOGGD("leave qnn_ggml\n");

        return 0;
    }

    qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", qnn_backend_lib, "");
    error  = qnn_backend.qnn_init(nullptr);
    if (0 != error) {
        LOGGW("init qnn subsystem failed, pls check why\n");
        ggml_free(ctx);
        result = 1;
        return 1;
    }

    QNN_INTERFACE_VER_TYPE qnn_raw_interface                = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface qnn_interface                             = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
    }

    error = qnn_raw_interface.graphCreate(qnn_backend.get_qnn_context_handle(),
                                          "qnn_ggml", nullptr, &graph_handle);
    LOGGI("error = %d\n", error);
    uint32_t dimensions_input_0[] = {2, 2};
    uint32_t dimensions_input_1[] = {2, 2};
    uint32_t dimensions_output[]  = {2, 2};

    tensor_0 = {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=0,
                    .name= "tensor_0",
                    .type= QNN_TENSOR_TYPE_APP_WRITE,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                      QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                      {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions=dimensions_input_0,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= {.data=nullptr,
                            .dataSize=0}}}}
    };

    tensor_1 = {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=0,
                    .name= "tensor_1",
                    .type= QNN_TENSOR_TYPE_APP_WRITE,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                      QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                      {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions=dimensions_input_1,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= {.data=nullptr,
                            .dataSize=0}}}}
    };

    tensor_2 = (Qnn_Tensor_t) {
            .version= QNN_TENSOR_VERSION_1,
            {.v1= {
                    .id=0,
                    .name= "tensor_2",
                    .type= QNN_TENSOR_TYPE_APP_READ,
                    .dataFormat= QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                    .dataType= QNN_DATATYPE_FLOAT_32,
                    .quantizeParams= {QNN_DEFINITION_UNDEFINED,
                                      QNN_QUANTIZATION_ENCODING_UNDEFINED,
                                      {.scaleOffsetEncoding= {.scale= 0.0000000000000000f, .offset= 0}}},
                    .rank= 2,
                    .dimensions= dimensions_output,
                    .memType= QNN_TENSORMEMTYPE_RAW,
                    {.clientBuf= {.data=nullptr,
                            .dataSize=0}}}}};


    error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_0);
    LOGGI("error = %d\n", error);
    error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_1);
    LOGGI("error = %d\n", error);
    error = qnn_raw_interface.tensorCreateGraphTensor(graph_handle, &tensor_2);
    LOGGI("error = %d\n", error);

    // mapping GGML tensor to QNN tensor
    QNN_VER_PTR(tensor_0)->clientBuf = {m0->data, 16};
    QNN_VER_PTR(tensor_1)->clientBuf = {m1->data, 16};
    QNN_VER_PTR(tensor_2)->clientBuf = {m2->data, 16};

    Qnn_Tensor_t tensor_inputs[] = {
            tensor_0,
            tensor_1
    };

    Qnn_Tensor_t tensor_outputs[] = {
            tensor_2
    };

    Qnn_OpConfig_t opconfig = {
            (Qnn_OpConfigVersion_t) 1, .v1 = {
                    "qnn_matrix_addition",
                    QNN_OP_PACKAGE_NAME_QTI_AISW,
                    qnn_op_typename,
                    0,
                    qnn_params,
                    2,
                    tensor_inputs,
                    1,
                    tensor_outputs
            }
    };
    error = qnn_raw_interface.graphAddNode(graph_handle, opconfig);
    LOGGI("error = %d\n", error);
    error = qnn_raw_interface.graphFinalize(graph_handle, nullptr, nullptr);
    LOGGI("error = %d\n", error);
    error = qnn_raw_interface.graphExecute(graph_handle, tensor_inputs, 2, tensor_outputs, 1,
                                           nullptr, nullptr);
    LOGGI("error = %d\n", error);
    if (0 == error) {
        TENSOR_DUMP(m0);
        TENSOR_DUMP(m1);
        TENSOR_DUMP(m2);
    }

    failure:
    ggml_free(ctx);
    qnn_backend.qnn_finalize();

    n_end_time  = ggml_time_us();
    n_durtion   = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_ggml with qnn backend %s is: %lld milliseconds\n", get_qnn_backend_name(n_backend_type), n_durtion);
    GGML_JNI_NOTIFY("duration of qnn_ggml with qnn backend %s is: %lld milliseconds\n", get_qnn_backend_name(n_backend_type), n_durtion);


    LOGGD("leave qnn_ggml\n");

    return result;
}


//LayerNorm in C, https://github.com/karpathy/llm.c/blob/master/doc/layernorm/layernorm.c
static void layernorm_forward(float * out, float * mean, float * rstd,
                              float * inp, float * weight, float * bias,
                              int B, int T, int C) {
    float eps = 1e-5f;
    for (int b = 0; b < B; b++) {
        for (int t = 0; t < T; t++) {
            // seek to the input position inp[b,t,:]
            float * x = inp + b * T * C + t * C;
            // calculate the mean
            float m = 0.0f;
            for (int i = 0; i < C; i++) {
                m += x[i];
            }
            m = m / C;
            // calculate the variance (without any bias correction)
            float v = 0.0f;
            for (int i = 0; i < C; i++) {
                float xshift = x[i] - m;
                v += xshift * xshift;
            }
            v = v / C;
            // calculate the rstd
            float s = 1.0f / sqrtf(v + eps);
            // seek to the output position in out[b,t,:]
            float * out_bt = out + b * T * C + t * C;
            for (int i = 0; i < C; i++) {
                float n = (s * (x[i] - m)); // normalized output
                float o = n * weight[i] + bias[i]; // scale and shift it
                out_bt[i] = o; // write
            }
            // cache the mean and rstd for the backward pass later
            mean[b * T + t] = m;
            rstd[b * T + t] = s;
        }
    }
}


static void layernorm_backward(float * dinp, float * dweight, float * dbias,
                               float * dout, float * inp, float * weight, float * mean, float * rstd,
                               int B, int T, int C) {
    for (int b = 0; b < B; b++) {
        for (int t = 0; t < T; t++) {
            float * dout_bt = dout + b * T * C + t * C;
            float * inp_bt = inp + b * T * C + t * C;
            float * dinp_bt = dinp + b * T * C + t * C;
            float mean_bt = mean[b * T + t];
            float rstd_bt = rstd[b * T + t];

            // first: two reduce operations
            float dnorm_mean = 0.0f;
            float dnorm_norm_mean = 0.0f;
            for (int i = 0; i < C; i++) {
                float norm_bti = (inp_bt[i] - mean_bt) * rstd_bt;
                float dnorm_i = weight[i] * dout_bt[i];
                dnorm_mean += dnorm_i;
                dnorm_norm_mean += dnorm_i * norm_bti;
            }
            dnorm_mean = dnorm_mean / C;
            dnorm_norm_mean = dnorm_norm_mean / C;

            // now iterate again and accumulate all the gradients
            for (int i = 0; i < C; i++) {
                float norm_bti = (inp_bt[i] - mean_bt) * rstd_bt;
                float dnorm_i = weight[i] * dout_bt[i];
                // gradient contribution to bias
                dbias[i] += dout_bt[i];
                // gradient contribution to weight
                dweight[i] += norm_bti * dout_bt[i];
                // gradient contribution to input
                float dval = 0.0f;
                dval += dnorm_i; // term 1
                dval -= dnorm_mean; // term 2
                dval -= norm_bti * dnorm_norm_mean; // term 3
                dval *= rstd_bt; // final scale
                dinp_bt[i] += dval;
            }
        }
    }
}


// poor man's tensor checker
static int check_tensor(float * a, float * b, int n, char * label) {
    int ok = 1;
    LOGGD("%s\n", label);
    GGML_JNI_NOTIFY("%s\n", label);
    for (int i = 0; i < n; i++) {
        if (fabs(a[i] - b[i]) <= 1e-5) {
            LOGGD("OK ");
            GGML_JNI_NOTIFY("OK ");
        } else {
            LOGGD("NOT OK ");
            GGML_JNI_NOTIFY("NOT OK ");
            ok = 0;
        }
        LOGGD("%f %f\n", a[i], b[i]);
        GGML_JNI_NOTIFY("%f %f\n", a[i], b[i]);
    }
    return ok;
}

static int qnn_complex_graph_inception(int n_backend_type, int n_graph_type);
// 04-11-2024, skip this step because I already know how to do it, move to PoC-S41&S42
// https://github.com/zhouwg/kantv/issues/121
// PoC-S35&S37:implement a complex/complicated computation graph
int qnn_complex_graph(int n_backend_type, int n_graph_type) {
    uint32_t i                                  = 0;
    uint32_t j                                  = 0;
    int error                                   = 0;
    int result                                  = 0;
    std::string graph_name                      = "qnn_complex_graph";
    const char * qnn_backend_lib                = "libQnnCpu.so";

    int64_t  n_begin_time                       = 0LL;
    int64_t  n_end_time                         = 0LL;
    int64_t  n_durtion                          = 0LL;

    Qnn_GraphHandle_t graph_handle              = nullptr;
    Qnn_Tensor_t tensor_0                       = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_1                       = QNN_TENSOR_INIT;
    Qnn_Tensor_t tensor_2                       = QNN_TENSOR_INIT;
    Qnn_QuantizeParams_t quantize_param         = QNN_QUANTIZE_PARAMS_INIT;
    Qnn_OpConfig_t qnn_opconfig                 = QNN_OPCONFIG_INIT;
    Qnn_Param_t qnn_params[]                    = {};
    const char * qnn_op_typename                = QNN_OP_ELEMENT_WISE_ADD;

    const int sizey                             = 2;
    const int sizex                             = 2;
    size_t ctx_size                             = 0;
    struct ggml_context * ctx                   = nullptr;
    struct ggml_tensor  * m0                    = nullptr;
    struct ggml_tensor  * m1                    = nullptr;
    struct ggml_tensor  * m2                    = nullptr;
    struct ggml_cgraph  * gf                    = nullptr;
    enum ggml_op     ggmlop                     = GGML_OP_ADD;
    std::vector<uint8_t> work_buffer;
    const ggml_type qtype                       = GGML_TYPE_F32;
    struct ggml_init_params params = {
            /*.mem_size   =*/ 0,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

    if (1 == n_graph_type) {
        result = qnn_complex_graph_inception(n_backend_type, 1);
        return result;
    }
    ggml_time_init();
    n_begin_time                                = ggml_time_us();
    LOGGD("enter qnn_complex_graph\n");
    n_graph_type                                = 0; //TODO: hardcode to 0

    LOGGI("[%s], backend type:%d(%s), graph type:%d\n", __func__,
          n_backend_type, get_qnn_backend_name(n_backend_type), n_graph_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d(%s), graph type:%d", __func__,
                    n_backend_type, get_qnn_backend_name(n_backend_type), n_graph_type);
    switch (n_backend_type) {
        case 0:
            qnn_backend_lib = "libQnnCpu.so";
            break;

        case 1:
            qnn_backend_lib = "libQnnGpu.so";
            break;

        case 2: {
            qnn_backend_lib = "libQnnHtp.so";
            std::string path = "/data/data/com.cdeos.kantv/";
            LOGGI("path:%s\n", path.c_str());
            LOGGI("qnn backend lib:%s\n", qnn_backend_lib);
            if (0 == setenv("LD_LIBRARY_PATH",
                            (path + ":/vendor/dsp/cdsp:/vendor/lib64:/vendor/dsp/dsp:/vendor/dsp/images").c_str(),
                            1)) {
                LOGGI("QNN DSP backend setenv successfully");
            } else {
                LOGGE("QNN DSP backend setenv failure");
            }
            if (0 == setenv("ADSP_LIBRARY_PATH",
                            (path + ";/vendor/dsp/cdsp;/vendor/lib/rfsa/adsp;/system/lib/rfsa/adsp;/vendor/dsp/dsp;/vendor/dsp/images;/dsp").c_str(),
                            1)) {
                LOGGI("QNN DSP backend setenv successfully");
            } else {
                LOGGE("QNN DSP backend setenv failure");
            }
            break;
        }
        case 3:
            // fall into ggml, just for compare performance between QNN SDK and original GGML
            break;

        default:
            LOGGW("backend type %d not supported\n", n_backend_type);
            break;
    }

    ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex * sizey);
    ctx_size        += ggml_row_size(GGML_TYPE_F32, sizex * sizey);
    ctx_size        += ggml_row_size(qtype,         sizex * sizey);
    ctx_size        += ggml_row_size(qtype,         sizex * sizey);
    ctx_size        += 1024 * 1024 * 16;
    params.mem_size =  ctx_size;
    ctx             =  ggml_init(params);
    if (!ctx) {
        LOGGW("ggml_init failure\n");
        return 1;
    }


    m0              = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m0, 1.0f);
    m1              = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(m1, 2.0f);

    //https://docs.qualcomm.com/bundle/publicresource/topics/80-63442-50/SupportedOps.html
    switch (n_graph_type) {
        case 0: // LayerNorm, https://github.com/karpathy/llm.c/blob/master/doc/layernorm/layernorm.md
            //the following code comes from https://github.com/karpathy/llm.c/blob/master/doc/layernorm/layernorm.c
        {
            //what's the Bias & Variance: https://scott.fortmann-roe.com/docs/BiasVariance.html
            int B = 2; // batch
            int T = 3; // time / sequence length
            int C = 4; // number of channels

            float *x = (float *) malloc(B * T * C * sizeof(float));
            float *w = (float *) malloc(C * sizeof(float));
            float *b = (float *) malloc(C * sizeof(float));
            float *out = (float *) malloc(B * T * C * sizeof(float));
            float *mean = (float *) malloc(B * T * sizeof(float));
            float *rstd = (float *) malloc(B * T * sizeof(float));
            float *dout = (float *) malloc(B * T * C * sizeof(float));
            float *dx = (float *) malloc(B * T * C * sizeof(float));
            float *dw = (float *) malloc(C * sizeof(float));
            float *db = (float *) malloc(C * sizeof(float));

            // read reference information from Python
            FILE *file = fopen("/sdcard/kantv/ln.bin", "rb");
            if (file == NULL) {
                LOGGW("Error opening file\n");
                GGML_JNI_NOTIFY("Error opening file\n");
                ggml_free(ctx);
                return 2;
            }
            fread(x, sizeof(float), B * T * C, file);
            fread(w, sizeof(float), C, file);
            fread(b, sizeof(float), C, file);
            fread(out, sizeof(float), B * T * C, file);
            fread(mean, sizeof(float), B * T, file);
            fread(rstd, sizeof(float), B * T, file);
            fread(dout, sizeof(float), B * T * C, file);
            fread(dx, sizeof(float), B * T * C, file);
            fread(dw, sizeof(float), C, file);
            fread(db, sizeof(float), C, file);
            fclose(file);

            // now let's calculate everything ourselves

            // forward pass
            float *c_out = (float *) malloc(B * T * C * sizeof(float));
            float *c_mean = (float *) malloc(B * T * sizeof(float));
            float *c_rstd = (float *) malloc(B * T * sizeof(float));
            layernorm_forward(c_out, c_mean, c_rstd, x, w, b, B, T, C);

            // check correctness of forward pass
            check_tensor(out, c_out, B * T * C, "out");
            check_tensor(mean, c_mean, B * T, "mean");
            check_tensor(rstd, c_rstd, B * T, "rstd");

            // backward pass (note calloc inits grads to zero)
            float *c_dx = (float *) calloc(B * T * C, sizeof(float));
            float *c_dw = (float *) calloc(B * T, sizeof(float));
            float *c_db = (float *) calloc(B * T, sizeof(float));
            layernorm_backward(c_dx, c_dw, c_db, dout, x, w, c_mean, c_rstd, B, T, C);

            // check correctness of backward pass
            check_tensor(c_dx, dx, B * T * C, "dx");
            check_tensor(c_dw, dw, C, "dw");
            check_tensor(c_db, db, C, "db");

            free(c_db);
            free(c_dw);
            free(c_dx);

            free(c_rstd);
            free(c_mean);
            free(c_out);

            free(db);
            free(dw);
            free(dx);
            free(dout);
            free(rstd);
            free(mean);
            free(out);
            free(b);
            free(w);
            free(x);
        }
            break;

        default:
            ggml_free(ctx);
            LOGGD("only LayerNorm supported currently");
            GGML_JNI_NOTIFY("only LayerNorm supported currently");
            return 0;
    }


    if (3 == n_backend_type) {  // this "fake" backend is just used for compare performance between QNN SDK and original GGML

        n_end_time  = ggml_time_us();
        n_durtion   = (n_end_time - n_begin_time) / 1000;
        ggml_free(ctx);
        LOGGD("duration of qnn_complex_graph with fake qnn backend ggml is: %lld milliseconds\n", n_durtion);
        GGML_JNI_NOTIFY("duration of qnn_complex_graph with fake qnn backend ggml is: %lld milliseconds\n", n_durtion);

        LOGGD("leave qnn_ggml\n");

        return 0;
    }

    qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", qnn_backend_lib, "");
    error  = qnn_backend.qnn_init(nullptr);
    if (0 != error) {
        LOGGW("init qnn subsystem failed, pls check why\n");
        result = 1;
        ggml_free(ctx);
        return 1;
    }

    QNN_INTERFACE_VER_TYPE qnn_raw_interface                = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface qnn_interface                             = qnn_backend.get_qnn_interface();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        result = 2;
    }

    error = qnn_raw_interface.graphCreate(qnn_backend.get_qnn_context_handle(),
                                          "qnn_complex_graph", nullptr, &graph_handle);

    if (0 == error) {
        TENSOR_DUMP(m0);
        TENSOR_DUMP(m1);
    }

    failure:
    ggml_free(ctx);
    qnn_backend.qnn_finalize();

    n_end_time  = ggml_time_us();
    n_durtion   = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_complex_graph with qnn backend %s is: %lld milliseconds\n", get_qnn_backend_name(n_backend_type), n_durtion);
    GGML_JNI_NOTIFY("duration of qnn_complex_graph with qnn backend %s is: %lld milliseconds\n", get_qnn_backend_name(n_backend_type), n_durtion);


    LOGGD("leave qnn_complex_graph\n");

    return result;
}


// https://github.com/zhouwg/kantv/issues/121
// PoC-S37:implement a complex/complicated computation graph
// how to mapping GGML tensor to QNN tensor could be found at PoC-S29(function qnn_ggml)
static int qnn_complex_graph_inception(int n_backend_type, int n_graph_type) {
    int error   = 0;
    int result  = 0;
    std::string graph_name                      = "qnn_complex_graph";
    const char * qnn_backend_lib                = "libQnnCpu.so"; //TODO hardcode

    int64_t  n_begin_time                       = 0LL;
    int64_t  n_end_time                         = 0LL;
    int64_t  n_durtion                          = 0LL;

    ggml_time_init();
    n_begin_time                                = ggml_time_us();

    LOGGD("enter qnn_complex_graph_inception\n");
    n_graph_type                                = 1; //TODO: hardcode to 1

    LOGGI("[%s], backend type:%d(%s), graph type:%d\n", __func__,
          n_backend_type, get_qnn_backend_name(n_backend_type), n_graph_type);
    GGML_JNI_NOTIFY("[%s], backend_type:%d(%s), graph type:%d", __func__,
                    n_backend_type, get_qnn_backend_name(n_backend_type), n_graph_type);

    qnn_implementation qnn_backend = qnn_implementation("/data/data/com.cdeos.kantv/", qnn_backend_lib, "");
    error  = qnn_backend.qnn_init(nullptr);
    if (0 != error) {
        LOGGW("init qnn subsystem failed, pls check why\n");
        return 1;
    }

    QNN_INTERFACE_VER_TYPE qnn_raw_interface                = qnn_backend.get_qnn_raw_interface();
    QNN_SYSTEM_INTERFACE_VER_TYPE qnn_raw_system_interface  = qnn_backend.get_qnn_raw_system_interface();
    qnn_interface qnn_interface                             = qnn_backend.get_qnn_interface();
    Qnn_BackendHandle_t backend_0                           = qnn_backend.get_qnn_backend_handle();
    Qnn_ContextHandle_t context_0                           = qnn_backend.get_qnn_context_handle();
    Qnn_DeviceHandle_t device_0                             = qnn_backend.get_qnn_device_handle();
    if (!qnn_interface.is_loaded()) {
        LOGGW("qnn subsystem failure\n");
        return 2;
    }

    //prebuild data for this complex computation graph
    FILE *fp = fopen("/sdcard/kantv/params.bin", "rb");
    if (!fp) {
        error = -1;
        LOGGI("ERROR! Could not open params.bin, ensure this file is in the current working directory when executing this program\n");
        GGML_JNI_NOTIFY("ERROR! Could not open params.bin, ensure this file is in the current working directory when executing this program\n");
        return error;
    }

    const QnnGraph_Config_t *context_0_convReluModel_config_0[] = {NULL};
    Qnn_GraphHandle_t context_0_convReluModel;
    qnn_raw_interface.graphCreate(context_0, graph_name.c_str(), context_0_convReluModel_config_0, &context_0_convReluModel);

    //how to compose a complex qnn computation graph
    //step-1:
    uint32_t context_0_convReluModel_tensor_0_dims[] = {1, 299, 299, 3};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_0_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}};
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_0_clientBuf = {NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_tensor_0_v1 = {0, "input_0",
                                                          QNN_TENSOR_TYPE_APP_WRITE,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_FLOAT_32,
                                                          context_0_convReluModel_tensor_0_quantizeParams,
                                                          4, context_0_convReluModel_tensor_0_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_0_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_0 = {
            (Qnn_TensorVersion_t) 1,
            .v1 = context_0_convReluModel_tensor_0_v1
    };
    error = qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel,&context_0_convReluModel_tensor_0);
    LOGGI("error = %d\n", error);

    //step-2:
    uint32_t context_0_convReluModel_tensor_1_dims[] = {3, 3, 3, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_1_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    float context_0_convReluModel_tensor_1_data[864];
    fread(context_0_convReluModel_tensor_1_data, 4, 864, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_1_clientBuf = {
            (void *) context_0_convReluModel_tensor_1_data, 3456
    };
    Qnn_TensorV1_t context_0_convReluModel_tensor_1_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_weight",
                                                          QNN_TENSOR_TYPE_STATIC,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_FLOAT_32,
                                                          context_0_convReluModel_tensor_1_quantizeParams,
                                                          4, context_0_convReluModel_tensor_1_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_1_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_1 = {
            (Qnn_TensorVersion_t) 1,
            .v1 = context_0_convReluModel_tensor_1_v1
    };
    error = qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_1);
    LOGGI("error = %d\n", error);

    //step-3:
    uint32_t context_0_convReluModel_tensor_2_dims[] = {32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_2_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    float context_0_convReluModel_tensor_2_data[32];
    fread(context_0_convReluModel_tensor_2_data, 4, 32, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_2_clientBuf = {
            (void *) context_0_convReluModel_tensor_2_data, 128
    };
    Qnn_TensorV1_t context_0_convReluModel_tensor_2_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_bias",
                                                          QNN_TENSOR_TYPE_STATIC,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_FLOAT_32,
                                                          context_0_convReluModel_tensor_2_quantizeParams,
                                                          1, context_0_convReluModel_tensor_2_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_2_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_2 = {
            (Qnn_TensorVersion_t) 1,
            .v1 = context_0_convReluModel_tensor_2_v1
    };
    error=qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_2);
    LOGGI("error = %d\n", error);

    //step-4:
    uint32_t context_0_convReluModel_tensor_3_dims[] = {2};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_3_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    static uint32_t context_0_convReluModel_tensor_3_data[2];
    fread(context_0_convReluModel_tensor_3_data, 4, 2, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_3_clientBuf = {
            (void *) context_0_convReluModel_tensor_3_data, 8
    };
    Qnn_TensorV1_t context_0_convReluModel_tensor_3_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_dilation",
                                                          QNN_TENSOR_TYPE_STATIC,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_UINT_32,
                                                          context_0_convReluModel_tensor_3_quantizeParams,
                                                          1, context_0_convReluModel_tensor_3_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_3_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_3 = {
            (Qnn_TensorVersion_t) 1,
            .v1 = context_0_convReluModel_tensor_3_v1
    };
    error = qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_3);
    LOGGI("error = %d\n", error);

    //step-5:
    uint32_t context_0_convReluModel_tensor_4_dims[] = {2, 2};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_4_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    static uint32_t context_0_convReluModel_tensor_4_data[4];
    fread(context_0_convReluModel_tensor_4_data, 4, 4, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_4_clientBuf = {
            (void *) context_0_convReluModel_tensor_4_data,
            16
    };
    Qnn_TensorV1_t context_0_convReluModel_tensor_4_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_pad_amount",
                                                          QNN_TENSOR_TYPE_STATIC,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_UINT_32,
                                                          context_0_convReluModel_tensor_4_quantizeParams,
                                                          2, context_0_convReluModel_tensor_4_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_4_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_4 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_4_v1};
    error =qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_4);
    LOGGI("error = %d\n", error);



    //step-6:
    uint32_t context_0_convReluModel_tensor_5_dims[] = {2};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_5_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    static uint32_t context_0_convReluModel_tensor_5_data[2];
    fread(context_0_convReluModel_tensor_5_data, 4, 2, fp);
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_5_clientBuf = {
            (void *) context_0_convReluModel_tensor_5_data, 8
    };
    Qnn_TensorV1_t context_0_convReluModel_tensor_5_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_stride",
                                                          QNN_TENSOR_TYPE_STATIC,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_UINT_32,
                                                          context_0_convReluModel_tensor_5_quantizeParams,
                                                          1, context_0_convReluModel_tensor_5_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_5_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_5 = {
            (Qnn_TensorVersion_t) 1,
            .v1 = context_0_convReluModel_tensor_5_v1
    };
    error = qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_5);
    LOGGI("error = %d\n", error);

    //step-7:
    uint32_t context_0_convReluModel_tensor_6_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_6_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_6_clientBuf = {NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_tensor_6_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_BatchNorm_FusedBatchNorm_0",
                                                          QNN_TENSOR_TYPE_NATIVE,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_FLOAT_32,
                                                          context_0_convReluModel_tensor_6_quantizeParams,
                                                          4, context_0_convReluModel_tensor_6_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_6_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_6 = {
            (Qnn_TensorVersion_t) 1, .v1 = context_0_convReluModel_tensor_6_v1};
    error = qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_6);
    LOGGI("error = %d\n", error);


    //step-8:
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0 = {
            QNN_PARAMTYPE_TENSOR,
            "dilation",
            .tensorParam = context_0_convReluModel_tensor_3
    };
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1 = {
            QNN_PARAMTYPE_TENSOR,
            "pad_amount",
            .tensorParam = context_0_convReluModel_tensor_4
    };
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2 = {
            QNN_PARAMTYPE_TENSOR,
            "stride",
            .tensorParam = context_0_convReluModel_tensor_5
    };
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3 = {
            QNN_PARAMTYPE_SCALAR,
            "group",
            .scalarParam = {
                    QNN_DATATYPE_UINT_32, .uint32Value = 1}
    };
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_params[] = {
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_0,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_1,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_2,
            context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_param_3};

    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_inputs[] = {
            context_0_convReluModel_tensor_0,
            context_0_convReluModel_tensor_1,
            context_0_convReluModel_tensor_2};

    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_outputs[] = {
            context_0_convReluModel_tensor_6
    };
    //attention here, key point of how to construct QNN OP config for a complex computation graph
    //similar to CFG in front-end phase/stage of GCC compiler
    Qnn_OpConfig_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0 = {
            (Qnn_OpConfigVersion_t) 1,
            .v1 = {
                    "InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D",
                    "qti.aisw",
                    "Conv2d",
                    4,
                    context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_params,
                    3,
                    context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_inputs,
                    1,
                    context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0_outputs
            }
    };
    //attention here
    error = qnn_raw_interface.graphAddNode(context_0_convReluModel, context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Conv2D_0);
    LOGGI("error = %d\n", error);

    //step-9:
    uint32_t context_0_convReluModel_tensor_7_dims[] = {1, 149, 149, 32};
    Qnn_QuantizeParams_t context_0_convReluModel_tensor_7_quantizeParams = {
            QNN_DEFINITION_UNDEFINED,
            QNN_QUANTIZATION_ENCODING_UNDEFINED,
            .scaleOffsetEncoding = {0.0, 0}
    };
    Qnn_ClientBuffer_t context_0_convReluModel_tensor_7_clientBuf = {NULL, 0};
    Qnn_TensorV1_t context_0_convReluModel_tensor_7_v1 = {0,
                                                          "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0",
                                                          QNN_TENSOR_TYPE_APP_READ,
                                                          QNN_TENSOR_DATA_FORMAT_FLAT_BUFFER,
                                                          QNN_DATATYPE_FLOAT_32,
                                                          context_0_convReluModel_tensor_7_quantizeParams,
                                                          4, context_0_convReluModel_tensor_7_dims,
                                                          QNN_TENSORMEMTYPE_RAW,
                                                          context_0_convReluModel_tensor_7_clientBuf
    };
    Qnn_Tensor_t context_0_convReluModel_tensor_7 = {
            (Qnn_TensorVersion_t) 1,
            .v1 = context_0_convReluModel_tensor_7_v1
    };
    error = qnn_raw_interface.tensorCreateGraphTensor(context_0_convReluModel, &context_0_convReluModel_tensor_7);
    LOGGI("error = %d\n", error);

    //step-10:
    //attention here: similar to what I did in PoC-S26
    Qnn_Param_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_params[] = {};
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_inputs[] = {
            context_0_convReluModel_tensor_6
    };
    Qnn_Tensor_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_outputs[] = {
            context_0_convReluModel_tensor_7
    };
    Qnn_OpConfig_t context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0 = {
            (Qnn_OpConfigVersion_t) 1, .v1 = {
                    "InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu",
                    "qti.aisw",
                    "Relu",
                    0,
                    context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_params,
                    1,
                    context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_inputs,
                    1,
                    context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0_outputs
            }
    };
    //attention here
    error = qnn_raw_interface.graphAddNode(context_0_convReluModel,context_0_convReluModel_InceptionV3_InceptionV3_Conv2d_1a_3x3_Relu_0);
    LOGGI("error = %d\n", error);

    //step-11:
    error = qnn_raw_interface.graphFinalize(context_0_convReluModel, NULL, NULL);
    LOGGI("error = %d\n", error);
    Qnn_Tensor_t context_0_convReluModel_inputTensors_0[] = {context_0_convReluModel_tensor_0};
    Qnn_Tensor_t context_0_convReluModel_outputTensors_0[] = {context_0_convReluModel_tensor_7};
    //attention here
    error = qnn_raw_interface.graphExecute(context_0_convReluModel, context_0_convReluModel_inputTensors_0, 1,
                                           context_0_convReluModel_outputTensors_0, 1, NULL, NULL);
    LOGGI("error = %d\n", error);

    qnn_backend.qnn_finalize();

    n_end_time  = ggml_time_us();
    n_durtion   = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_complex_graph_inception with qnn backend %s is: %lld milliseconds\n", get_qnn_backend_name(n_backend_type), n_durtion);
    GGML_JNI_NOTIFY("duration of qnn_complex_graph_inception with qnn backend %s is: %lld milliseconds\n", get_qnn_backend_name(n_backend_type), n_durtion);

    LOGGD("leave qnn_complex_graph_inception\n");

    return result;
}


/**
  * this special function is for PoC-S49: implementation of other GGML OP(non-mulmat) using QNN API, https://github.com/zhouwg/kantv/issues/121
  * it's similar to qnn_ggml but different with qnn_ggml, because data path in these two function is totally different
  *
  * the function qnn_ggml calling QNN API directly in JNI layer, bypass the framekwork in ggml's internal
  *
  * this function will calling function in ggml-qnn.cpp via ggml layer(which executed in GGML QNN backend using QNN SDK --- ggml-qnn.cpp)
  *
  * this function used to validate PoC-S49:implementation of other GGML OP(non-mulmat) using QNN API
  * or this function is UT for PoC-S49:implementation of other GGML OP(non-mulmat) using QNN API
  *
  * @param model_path whisper.cpp model at the first step, llama.cpp model at the second step
  * @param num_threads 1 - 8
  * @param n_backend_type 0: QNN CPU, 1: QNN GPU, 2: QNN DSP(HTA), 3: ggml(fake QNN backend, just used to compare performance between QNN and original GGML)
  * @param n_op_type GGML OP type
  * @return
  */
int qnn_ggml_op(const char * model_path, int num_threads, int n_backend_type, int n_ggml_op_type) {
    int result                      = 0;
    int64_t  n_begin_time           = 0LL;
    int64_t  n_end_time             = 0LL;
    int64_t  n_durtion              = 0LL;
    size_t  ctx_size                = 0;
    struct ggml_context * ctx       = nullptr;
    struct ggml_cgraph * gf         = nullptr;
    struct ggml_tensor * src0       = nullptr;
    struct ggml_tensor * src1       = nullptr;
    struct ggml_tensor * dst        = nullptr;
    std::vector<uint8_t> work_buffer;

    LOGGD("enter qnn_ggml_op\n");
    LOGGI("mode path:%s", model_path);
    LOGGI("num_threads:%d", num_threads);
    LOGGI("backend_type:%d(%s)", n_backend_type, get_qnn_backend_name(n_backend_type));
    LOGGI("ggml op:%d(%s)", n_ggml_op_type, ggml_op_name((enum ggml_op)n_ggml_op_type));


    GGML_JNI_NOTIFY("starting qnn_ggml_op UT(unit test)\n");
#if 0 // for performance comparison between QNN backend and original GGML
    const int sizey = 4096;
    const int sizex = 4096;
    const int sizez = 128;
#else // for UT with PoC-S49: implementation of other GGML OP(non-mulmat) using QNN API,https://github.com/zhouwg/kantv/issues/121
    //troubleshooting issue in previous commit: mulmat's result using QNN CPU backend is not correct in ggml-qnn.cpp
    int sizey = 2;
    int sizex = 2;
    int sizez = 1;
#endif

    const ggml_type qtype = GGML_TYPE_F32;

    n_begin_time                        = ggml_time_us();
    srand(time(NULL));


    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizey);
    ctx_size += ggml_row_size(GGML_TYPE_F32, sizex*sizez);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += ggml_row_size(qtype,         sizex*sizey);
    ctx_size += 1024 *1024 *16;
    GGML_JNI_NOTIFY("Allocating Memory of size %zi bytes, %zi MB\n",ctx_size, (ctx_size/1024/1024));

    struct ggml_init_params params = {
            /*.mem_size   =*/ ctx_size,
            /*.mem_buffer =*/ NULL,
            /* no_alloc   =*/ 0
    };

#ifdef GGML_USE_QNN
    if (n_backend_type != 3) //3 is fake QNN backend "ggml", just used to compare performance between QNN backend and original GGML
        params.use_hwaccel   = true;
#endif

    ctx = ggml_init(params);
    if (!ctx) {
        LOGGW("%s: ggml_init() failed\n");
        return 1;
    }

    GGML_JNI_NOTIFY("creating new tensors\n");
    src0 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(src0, 1.0f);
    ggml_set_f32(src0, (rand() % 100 + 1));

    src1 = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, sizex, sizey);
    ggml_set_f32(src1, 2.0f);
    ggml_set_f32(src1, (rand() % 100 + 1));

    switch (n_ggml_op_type) {
        case GGML_OP_ADD:
            dst = ggml_add(ctx, src0, src1);
            break;
        case GGML_OP_MUL:
            dst = ggml_mul(ctx, src0, src1);
            break;
        case GGML_OP_MUL_MAT:
            dst = ggml_mul_mat(ctx, src0, src1);
            break;
        default:
            LOGGD("ggml op %d(%s) not supported  with backend %d(%s)", n_ggml_op_type, ggml_op_name((enum ggml_op)n_ggml_op_type), n_backend_type, get_qnn_backend_name(n_backend_type));
            GGML_JNI_NOTIFY("ggml op %d(%s) not supported  with backend %d(%s)", n_ggml_op_type, ggml_op_name((enum ggml_op)n_ggml_op_type), n_backend_type, get_qnn_backend_name(n_backend_type));
            LOGGD("leave qnn_ggml_op UT(unit test)\n");
            ggml_free(ctx);
            return 2;
            //break;
    }
    ggml_set_f32(dst, 0.0f);

    GGML_JNI_NOTIFY("creating compute graph\n");
    gf = ggml_new_graph(ctx);
    ggml_build_forward_expand(gf, dst);
    ggml_graph_compute_helper(work_buffer, gf, num_threads);

    if (get_tensor_data_size(dst) < 100) {
        TENSOR_DUMP(src0);
        TENSOR_DUMP(src1);
        TENSOR_DUMP(dst);
    } else {
        LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", src0->name,
              src0->type, ggml_type_name(src0->type), src0->ne[0], src0->ne[1], src0->ne[2], src0->nb[0], src0->nb[1], src0->nb[2]);
        LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", src1->name,
              src1->type, ggml_type_name(src1->type), src1->ne[0], src1->ne[1], src1->ne[2], src1->nb[0], src1->nb[1], src1->nb[2]);
        LOGGI("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", dst->name,
              dst->type, ggml_type_name(dst->type), dst->ne[0], dst->ne[1], dst->ne[2], dst->nb[0], dst->nb[1], dst->nb[2]);

        GGML_JNI_NOTIFY("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", src0->name,
              src0->type, ggml_type_name(src0->type), src0->ne[0], src0->ne[1], src0->ne[2], src0->nb[0], src0->nb[1], src0->nb[2]);
        GGML_JNI_NOTIFY("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", src1->name,
              src1->type, ggml_type_name(src1->type), src1->ne[0], src1->ne[1], src1->ne[2], src1->nb[0], src1->nb[1], src1->nb[2]);
        GGML_JNI_NOTIFY("%15s: type = %i (%5s) ne = %5" PRIi64 " x %5" PRIi64 " x %5" PRIi64 ", nb = (%5zi, %5zi, %5zi)\n", dst->name,
              dst->type, ggml_type_name(dst->type), dst->ne[0], dst->ne[1], dst->ne[2], dst->nb[0], dst->nb[1], dst->nb[2]);

    }
    ggml_free(ctx);

    n_end_time  = ggml_time_us();
    n_durtion   = (n_end_time - n_begin_time) / 1000;
    LOGGD("duration of qnn_ggml_op %d(%s) with backend %d(%s) is: %lld milliseconds\n", n_ggml_op_type, ggml_op_name((enum ggml_op)n_ggml_op_type), n_backend_type, get_qnn_backend_name(n_backend_type), n_durtion);
    GGML_JNI_NOTIFY("duration of qnn_ggml_op %d(%s) with backend %d(%s) is: %lld milliseconds\n", n_ggml_op_type, ggml_op_name((enum ggml_op)n_ggml_op_type), n_backend_type, get_qnn_backend_name(n_backend_type), n_durtion);
    LOGGD("leave qnn_ggml_op UT(unit test)\n");

    return 0;
}