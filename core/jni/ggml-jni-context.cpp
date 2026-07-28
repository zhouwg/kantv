
#include "ggml-jni-context.h"

#include "ggml.h"
#include "ggml-alloc.h"
#include "ggml-backend.h"
#include "ggml-cpu.h"
#include "llamacpp/ggml/include/ggml-hexagon.h"

#include <chrono>
#include <thread>

// common.h provides common_init() which used to be called at the top of
// mtmd_inference_main / llm_inference_main; now it is called once from
// ggml_jni_context::init() and must be in scope here.
// We use the explicit relative path "llamacpp/common/common.h" instead
// of bare "common.h" because the build's include path also contains
// core/llamacpp/ggml/src/ggml-cpu/ which has its own unrelated
// "common.h" (ggml-cpu internal). Without the explicit path, the
// compiler picks the ggml-cpu one and common_init() ends up undeclared.
#include "llamacpp/common/common.h"

// mtmd.h provides mtmd_log_set() which is called from ggml_jni_context::init()
// to route libmtmd internal errors to logcat. Transitive includes from the
// ggml-jni.h chain do not expose this declaration, so we include it directly.
#include "mtmd.h"

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

// Forward declare the GGML abort callback signature
// (typedef void (*ggml_abort_callback_t)(const char * error_message))
// ggml.h is already included via ggml-jni.h transitively.
static void kantv_ggml_abort_callback(const char * error_message) {
    // Route GGML_ASSERT / GGML_ABORT failure messages to logcat so they are
    // visible in adb logcat. Otherwise ggml_abort() prints to stderr which
    // Android typically discards.
    if (error_message != nullptr) {
        // __android_log_print is already declared as extern in cde_log.c and
        // is the canonical logcat entry point in this codebase. The priority
        // value 6 corresponds to ANDROID_LOG_FATAL in the NDK enum.
        __android_log_print(6 /* ANDROID_LOG_FATAL */, "KANTV_GGML",
                            "%s", error_message);
    }
}

void ggml_jni_context::init() {
    if (initialized) {
        LOGGD("already initialize");
        return;
    }
    llm_temperature = 0.8;
    llm_top_p       = 0.9;
    // Install a custom GGML abort callback so that GGML_ASSERT / GGML_ABORT
    // failures inside libllama/libmtmd (e.g. common_params_parse pre-condition
    // checks) reach logcat *before* the process is killed by abort(). Without
    // this, the abort message is printed to stderr which Android typically
    // drops, leaving us with only the SIGABRT tombstone and no actionable
    // error message.
    ggml_set_abort_callback(kantv_ggml_abort_callback);
#if defined(__ANDROID__)
    // Same rationale for libmtmd: route its internal errors to logcat under
    // the KANTV_MTMD tag. Without this, mtmd errors go to stderr (which
    // Android drops) and we only see a SIGABRT with no actionable message.
    // Use the cde_log.h CDE_LOG_* priority constants (transitively included
    // via ggml-jni.h) to stay consistent with the rest of the JNI layer
    // rather than dragging in <android/log.h> here.
    mtmd_log_set([](ggml_log_level level, const char * text, void * /*user_data*/) {
        int prio;
        switch (level) {
            case GGML_LOG_LEVEL_ERROR: prio = CDE_LOG_ERROR; break;
            case GGML_LOG_LEVEL_WARN:  prio = CDE_LOG_WARN;  break;
            case GGML_LOG_LEVEL_INFO:  prio = CDE_LOG_INFO;  break;
            case GGML_LOG_LEVEL_DEBUG: prio = CDE_LOG_DEBUG; break;
            default:                   prio = CDE_LOG_DEBUG; break;
        }
        __android_log_print(prio, "KANTV_MTMD", "%s", text);
    }, nullptr);
#endif
    // Install the llama backend (ggml backend registry + quant tables) once
    // for the whole JNI lifetime. The previous design called
    // llama_backend_init() at the top of every mtmd_inference_main /
    // llm_inference_main invocation and llama_backend_free() at the end -
    // on the Hexagon CDSP/NPU build each such cycle tears down the DSP
    // microcode + ION/DMA + RPC setup, which is both heat-generating and
    // the source of the "2 succeed, 2 crash" MTMD regression. Doing it
    // here, paired with the matching cleanup() called from the Java
    // onDestroy() path, keeps the DSP alive across all inference calls
    // in a session.
    common_init();
    llama_backend_init();
    m_backend_initialized = true;
    initialized     = true;
}

void ggml_jni_context::cleanup() {
    LOGGD("cleanup");
    // Drop any model that is still held in the singleton before tearing
    // down the DSP backend - if a model is still loaded and the backend
    // goes away, the model is left with dangling backend references and
    // any subsequent operation would crash. unload_model() is a no-op
    // when nothing is loaded, so it is safe to call unconditionally.
    unload_model();
    if (m_backend_initialized) {
        // Symmetric to llama_backend_init() in init(). llama_backend_free()
        // just releases the quant tables (ggml_quantize_free) - it is
        // cheap and safe to call once on app exit. Do NOT call this from
        // per-inference paths: the DSP stays up for the whole session.
        llama_backend_free();
        m_backend_initialized = false;
    }
    if (initialized) {
        initialized = false;
    }
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
    initialized(false),
    m_backend_initialized(false),
    loaded_model_path(),
    loaded_mmproj_path(),
    loaded_ngl(-100),  // sentinel; no valid request matches -100
    loaded_model(nullptr),
    loaded_mctx(nullptr) {
    init();
}

ggml_jni_context::~ggml_jni_context() {
    // At process exit the static singleton is destroyed. Mirror the Java
    // onDestroy() cleanup so the DSP / quant tables are released exactly
    // once. Idempotent because cleanup() is guarded by m_backend_initialized.
    cleanup();
}

// ===== Phase 2: model singleton implementation =====
//
// Design intent (see also the matching documentation in the header):
//   * The model and mtmd context (mmproj) are heavy to load (4GB+ I/O,
//     ~hundreds of ms of init on the Hexagon DSP). We hold them in this
//     singleton for the lifetime of the JNI process and only reload when
//     the (model_path, mmproj_path, backend_type) key changes.
//   * llama_context (lctx), common_sampler, and llama_batch are NOT
//     held in the singleton - they own per-conversation state (KV cache,
//     sampling state) and must be created/freed per inference call.
//   * m_model_mutex is the single-flight gate that makes a model switch
//     atomic with respect to an in-flight inference: either the inference
//     finishes on the old model, or it sees the new model - never a
//     mid-flight unload while a forward pass is running.
int ggml_jni_context::ensure_model_loaded(const char * model_path,
                                          const char * mmproj_path,
                                          int         n_gpu_layers) {
    std::lock_guard<std::mutex> lock(m_model_mutex);

    // Normalize inputs: callers may pass nullptr for mmproj_path on the
    // pure-LLM path. Treat nullptr and "" identically for the cache key.
    const std::string req_model_path = model_path ? model_path : "";
    const std::string req_mmproj_path = mmproj_path ? mmproj_path : "";

    // Fast path: cache hit on all three key fields. Avoids the 4GB read.
    //
    // The third key field is n_gpu_layers (not the user-facing backend
    // type). Two callers with the same backend can request different
    // ngl values (e.g. LLM with ngl=99 vs MTMD with ngl=0 even when
    // both pick the CDSP backend); using backend_type alone as the
    // cache key caused the 4GB model to be loaded with the wrong
    // offload configuration, which led to the vision encoder running
    // out of Hexagon ION pool memory and producing garbage output.
    if (loaded_model != nullptr
        && loaded_model_path  == req_model_path
        && loaded_mmproj_path == req_mmproj_path
        && loaded_ngl         == n_gpu_layers) {
        LOGGD("ensure_model_loaded: cache hit, reusing model '%s'%s%s (ngl=%d)",
              req_model_path.c_str(),
              req_mmproj_path.empty() ? "" : " + mmproj '",
              req_mmproj_path.empty() ? "" : req_mmproj_path.c_str(),
              n_gpu_layers);
        return 0;
    }

    // Slow path: either nothing is loaded yet, or the key changed.
    // Unload any old model first so the new load starts from a clean
    // state. unload_model() is a no-op if loaded_model == nullptr.
    //
    // The unload also has a forced sleep before the new load: freeing
    // a 4GB llama_model gives the C++ heap the memory back, but the
    // kernel is under no obligation to release the anonymous pages
    // back to the page pool / ION allocator immediately. If we jump
    // straight into llama_model_load_from_file (which mmaps the next
    // 4GB model file) we end up with peak RSS of OLD_FREEING +
    // NEW_MAPPING, which on a 12GB phone is enough to trigger
    // OOM-killer (signal 9) before the new model finishes loading
    // (this happened on a gemma-3-4b cache miss right after a
    // Qwen2.5-Omni audio run, see project memory).
    //
    // 500ms is a heuristic - long enough for the page reclaimer to
    // finish the unmap, short enough that the user doesn't notice.
    // malloc_trim(0) below actively nudges glibc to release the freed
    // heap back to the OS so the next 4GB allocation doesn't have to
    // compete with the just-freed pages.
    if (loaded_model != nullptr) {
        LOGGD("ensure_model_loaded: cache miss, unloading previous model '%s' (ngl=%d)",
              loaded_model_path.c_str(), loaded_ngl);
        // Call the _unlocked variant because this function already holds
        // m_model_mutex (line 184). Calling unload_model() here would
        // re-acquire the same non-recursive std::mutex and deadlock the
        // worker thread forever -- which is exactly what we saw on the
        // Qwen2.5-Omni -> gemma-3-4b cache miss after a successful
        // audio run: native inference stalled at the first cache-miss
        // switch, the Java side never got a "starting media encoding"
        // event, and the chat RecyclerView sat on "..." until force-stop.
        // 2026-07-28 fix.
        unload_model_unlocked();
#if defined(__GLIBC__)
        // glibc-specific: actively release freed heap back to OS.
        // No-op on Android bionic (malloc_trim is a stub there).
        extern int malloc_trim(size_t);
        malloc_trim(0);
#endif
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // Actually load the model. The full loading sequence is identical to
    // what mtmd_inference_main / llm_inference_main used to do, just
    // moved here so it runs at most once per (path, ngl) pair.
    LOGGD("ensure_model_loaded: loading model '%s'%s%s (ngl=%d)",
          req_model_path.c_str(),
          req_mmproj_path.empty() ? "" : " + mmproj '",
          req_mmproj_path.empty() ? "" : req_mmproj_path.c_str(),
          n_gpu_layers);

    // CRITICAL: the n_gpu_layers value MUST be propagated into the
    // common_params used to convert to llama_model_params. Previously
    // we built a minimal common_params with only params.model.path set,
    // which left params.n_gpu_layers at its default (-1 = "auto"); the
    // resulting llama_model_params told the model loader to use GPU
    // offload automatically, which on a device with a Hexagon backend
    // caused the entire 4GB of weights to be allocated from the
    // 4GB Hexagon ION pool. Once that pool was full, the vision
    // encoder's compute buffers could not be allocated and produced
    // garbage output (the "[multimodal]" token string in the user
    // screenshot). Callers that want CPU-only must pass ngl=0 here so
    // the weights stay in system memory.
    {
        common_params params;
        params.model.path    = req_model_path;
        params.n_gpu_layers  = n_gpu_layers;
        // CRITICAL WARNING (do not repeat the previous fix):
        // llama's `llama_load_mode` enum does NOT control whether the
        // model file is mmap'd. The actual decision is hardcoded in
        // `llama_model_base::load_tensors()` at llama-model.cpp:1252
        // as `const bool use_mmap_buffer = true;` regardless of
        // load_mode. So setting `params.load_mode = LLAMA_LOAD_MODE_NONE`
        // here does nothing for our peak-RSS problem.
        //
        // The `load_mode` enum actually only controls:
        //   LLAMA_LOAD_MODE_MLOCK -> use_mlock = true (force RAM, no swap)
        //   everything else       -> uses the default mmap path
        //
        // The real peak-RSS driver on a 12GB phone is the 4GB Hexagon
        // ION pool that gets allocated unconditionally by
        // ggmlhexagon_init_rpcmempool() at ggml-hexagon-jz.cpp:1721
        // during llama_backend_init(), plus the mmap 4GB file +
        // separate CPU-side weights copy for a Q8 model. That is
        // >8GB, which on this device triggers the Oplus ION
        // memory_leak alert (ion=4063MB, threshold=2048MB, see
        // 18:17:08-18:17:10 logcat) and SurfaceFlinger removes the
        // app's surfaces, freezing the UI.
        //
        // Practical mitigations to discuss:
        //  (1) use a Q4_K_M model instead of Q8_0 (cuts the CPU copy
        //      in half, ~2.5GB vs 4.5GB)
        //  (2) make Hexagon backend init conditional (e.g. skip
        //      ggml_backend_hexagon_init_ext() when the inference
        //      path is MTMD/CPU-only, so the 4GB ION pool is never
        //      reserved at app startup). This is a deeper change
        //      and may affect LLM/ASR paths.
        //  (3) drop the use_mmap_buffer = true hardcode in
        //      llama-model.cpp to honor a new params flag - this is
        //      upstream territory, not practical here.
        //
        // We deliberately do NOT set load_mode here. Leaving the
        // default (MMAP) is correct for this llama.cpp version; any
        // change would have to be at the hardcode site above.

        llama_model_params mparams = common_model_params_to_llama(params);
        loaded_model = llama_model_load_from_file(req_model_path.c_str(), mparams);
        if (loaded_model == nullptr) {
            LOGGD("ensure_model_loaded: llama_model_load_from_file failed for '%s'",
                  req_model_path.c_str());
            return 1;
        }
    }

    // Optional mtmd context (multimodal / vision encoder). Pure LLM
    // calls pass an empty mmproj_path, in which case we leave
    // loaded_mctx as nullptr.
    if (!req_mmproj_path.empty()) {
        mtmd_context_params mparams = mtmd_context_params_default();
        // mparams.use_gpu = false to match the "force ngl=0" policy
        // applied in mtmd-inference.cpp: the Hexagon DSP backend does
        // not fully implement the vision encoder (mmproj) op patterns
        // (convolutions + image-shaped attention) and aborts
        // intermittently. Keeping the mmproj on the CPU here makes the
        // backend choice consistent between the model and the mmproj.
        mparams.use_gpu = false;
        mparams.print_timings = false;
        mparams.n_threads = std::thread::hardware_concurrency();
        loaded_mctx = mtmd_init_from_file(req_mmproj_path.c_str(),
                                           loaded_model, mparams);
        if (loaded_mctx == nullptr) {
            LOGGD("ensure_model_loaded: failed to load mmproj '%s'",
                  req_mmproj_path.c_str());
            // The model loaded successfully but mmproj failed. We
            // intentionally do not roll back the model load - the
            // caller may want to retry with a different mmproj. The
            // next call to ensure_model_loaded() will hit the
            // mmproj_path mismatch and retry.
            return 3;
        }
        LOGGD("ensure_model_loaded: mmproj '%s' loaded", req_mmproj_path.c_str());
    }

    // Update the cache key only after both loads succeeded.
    loaded_model_path  = req_model_path;
    loaded_mmproj_path = req_mmproj_path;
    loaded_ngl         = n_gpu_layers;

    LOGGD("ensure_model_loaded: success");
    return 0;
}

void ggml_jni_context::unload_model() {
    std::lock_guard<std::mutex> lock(m_model_mutex);
    unload_model_unlocked();
}

// Internal helper. Assumes the caller already holds m_model_mutex (this
// is the case when called from ensure_model_loaded() which is the
// primary call site). Exposed separately so that ensure_model_loaded()
// can call it without re-locking m_model_mutex and deadlocking on the
// non-recursive std::mutex (the original bug).
void ggml_jni_context::unload_model_unlocked() {
    if (loaded_mctx != nullptr) {
        LOGGD("unload_model: freeing mmproj context '%s'", loaded_mmproj_path.c_str());
        mtmd_free(loaded_mctx);
        loaded_mctx = nullptr;
    }
    if (loaded_model != nullptr) {
        LOGGD("unload_model: freeing model '%s' (ngl=%d)",
              loaded_model_path.c_str(), loaded_ngl);
        llama_model_free(loaded_model);
        loaded_model = nullptr;
    }
    // Reset the cache key so a subsequent load with the same path is
    // not treated as a no-op cache hit.
    loaded_model_path.clear();
    loaded_mmproj_path.clear();
    loaded_ngl = -100; // sentinel; valid ngl values are >= -1 so -100
                        // can never collide with a real request
}

// Free-standing C entry point for unload_model(). Pairs with
// ggml_jni_context_cleanup() and is intended to be called from a
// Java unloadModel() JNI method (e.g. on LLMSettingFragment model
// change). Idempotent.
extern "C" void ggml_jni_unload_model() {
    g_jni_ctx.unload_model();
}

// The single global reference to the JNI context singleton. Bound to
// the Meyers-singleton inside ggml_jni_context::get_instance() so the
// underlying object is constructed on first use and destroyed at
// process exit (its destructor calls cleanup()). The reference itself
// is a file-scope definition (not `static`) so that other translation
// units (mtmd-inference.cpp, llm-inference.cpp,
// realtime-video-recognition.cpp) can see it via the matching
// `extern` declaration in ggml-jni-context.h. We still need the
// header extern - the bare global reference at file scope without
// `static` is not visible across translation units on its own.
ggml_jni_context & g_jni_ctx = ggml_jni_context::get_instance();

// Free-standing C entry point so the JNI bridge (ggml-jni.c) can invoke
// the C++ singleton's cleanup() without dragging in the C++ class type.
// Wrapped in extern "C" so the symbol has C linkage and is callable from
// the C translation unit that holds the JNIEXPORT functions. Must come
// after the g_jni_ctx static above because it references g_jni_ctx.
extern "C" void ggml_jni_context_cleanup() {
    g_jni_ctx.cleanup();
}

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
*helper functions to check whether LLM inference is running
*/
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
    // Same guard as mtmd_inference: catch uncaught C++ exceptions from the
    // llama.cpp internals so they do not abort() the whole process.
    try {
        ret = llama_inference_main(argc, const_cast<char **>(argv_vec.data()), n_backend_type);
    } catch (const std::exception & e) {
        __android_log_print(CDE_LOG_ERROR, "KANTV",
            "llama_inference_main threw std::exception: %s", e.what());
        ret = -1;
    } catch (...) {
        __android_log_print(CDE_LOG_ERROR, "KANTV",
            "llama_inference_main threw unknown (non-std) exception");
        ret = -1;
    }
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
        // WORKAROUND: MTMD (multimodal) inference on the Hexagon CDSP/NPU backend
        // is unstable and intermittently aborts (SIGABRT after a few calls). The
        // vision encoder (mmproj) emits ggml op patterns (convolutions and
        // attention with image-embedding shapes) that the Hexagon backend does
        // not fully implement, which causes a hard crash inside
        // ggml_backend_hexagon_graph_compute. Text-only LLM inference on the
        // same Hexagon backend works fine, so we keep -ngl 99 there; for MTMD
        // we force -ngl 0 (CPU) regardless of the user-selected backend. The
        // CPU path uses the same model file and the same chat template, just
        // without the DSP offload - it is several times slower but stable.
        LOGGW("mtmd_inference: forcing CPU backend (forcing -ngl 0) regardless of selected backend %d;"
              " Hexagon NPU does not fully support the MTMD vision encoder ops, so the CPU path is"
              " the only stable option for multimodal inference on this device.\n", n_backend_type);
        argv_vec.push_back("-ngl");
        argv_vec.push_back("0");
    } else {
        argv_vec.push_back("-ngl");
        argv_vec.push_back("0");
    }

    int argc = (int)argv_vec.size();
    llm_init_running_state();
    // Wrap mtmd_inference_main in a top-level try-catch to prevent uncaught C++
    // exceptions (e.g. from mtmd_context constructor when mmproj/text-model
    // embeddings mismatch, or any other deep call) from calling std::terminate
    // -> abort() and silently killing the process without surfacing an error
    // to the Java layer.
    try {
        ret = mtmd_inference_main(argc, const_cast<char **>(argv_vec.data()), n_backend_type);
    } catch (const std::exception & e) {
        // Reachable when the inner catch in common_params_parse (which calls exit(1)
        // for std::exception) does not catch the exception -- e.g. when the throw
        // originates from a noexcept function or a destructor along the way.
        __android_log_print(CDE_LOG_ERROR, "KANTV",
            "mtmd_inference_main threw std::exception: %s", e.what());
        ret = -1;
    } catch (...) {
        __android_log_print(CDE_LOG_ERROR, "KANTV",
            "mtmd_inference_main threw unknown (non-std) exception");
        ret = -1;
    }
    llm_reset_running_state();

    LOGGD("mtmd_inference return %d", ret);
    return ret;
}
