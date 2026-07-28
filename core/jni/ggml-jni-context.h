#pragma once

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

// Forward declarations for the llama.cpp and libmtmd types we hold across
// calls. We keep these as pointers in the singleton; the full definitions
// live in llama.h / mtmd.h which are only included in the .cpp file
// (including them in the header would leak C++ types into every .cpp that
// includes ggml-jni-context.h, plus they'd want a chunk of <vector>/<string>
// etc. that the existing code does not need).
struct llama_model;
struct llama_context;
struct mtmd_context;

class ggml_jni_context {
public:
    static ggml_jni_context & get_instance() {
        static ggml_jni_context instance;
        return instance;
    }

    void    init();

    void    cleanup();

    void    finalize();

    // ===== Phase 2: model singleton =====
    // ensure_model_loaded() is the idempotent "load the model if not already
    // loaded under the given (model_path, mmproj_path, backend) key" entry
    // point. Replaces the per-call common_init_from_params +
    // mtmd_init_from_file pair inside mtmd_inference_main / llm_inference_main.
    // The model and mtmd context (mmproj) are heavy to load (4GB+ I/O and
    // hundreds of ms of init on Hexagon DSP) so we keep them alive between
    // inference calls and only reload when the user switches model file or
    // backend. The returned pointers are owned by the singleton; callers
    // MUST NOT free them.
    //
    // Returns 0 on success, non-zero on failure (matching the convention
    // used by mtmd_inference_main / llm_inference_main). On failure, the
    // singleton is left in a consistent state (either old model still
    // loaded, or fully unloaded).
    int     ensure_model_loaded(const char * model_path,
                                const char * mmproj_path,
                                int         n_gpu_layers);

    // Release the currently loaded model + mmproj. Safe to call when
    // nothing is loaded (no-op). Called explicitly from the Java side on
    // model switch (LLMSettingFragment) and from process exit (cleanup()).
    void    unload_model();
    // Internal: same as unload_model() but assumes the caller already
    // holds m_model_mutex. Used by ensure_model_loaded() which is
    // itself holding the lock; calling unload_model() from there
    // would re-acquire a non-recursive std::mutex and deadlock.
    void    unload_model_unlocked();

    bool    is_model_loaded() const { return loaded_model != nullptr; }

    // Accessors for the cached model + mtmd context. The returned pointers
    // are owned by the singleton and MUST NOT be freed by the caller.
    // Callers must have already called ensure_model_loaded() to guarantee
    // these are non-null. Used by mtmd_inference_main / llm_inference_main
    // to skip the per-call 4GB reload on cache hits.
    llama_model *  get_loaded_model() const { return loaded_model; }
    mtmd_context * get_loaded_mctx()  const { return loaded_mctx;  }

    void    set_top_p(float value) { llm_temperature  = value; }
    float   get_top_p()  { return llm_top_p; }
    void    set_temperature(float value) { llm_temperature = value; }
    float   get_temperature() { return llm_temperature; }

    void llm_init_running_state() {
        llm_inference_is_running.store(1);
    }

    void llm_reset_running_state() {
        llm_inference_is_running.store(0);
    }

    int llm_is_running_state() {
        return llm_inference_is_running.load();
    }

    void realtimemtmd_init_running_state() {
        realtimemtmd_inference_is_running.store(1);
    }

    void realtimemtmd_reset_running_state() {
        realtimemtmd_inference_is_running.store(0);
    }

    int realtimemtmd_is_running_state() {
        return realtimemtmd_inference_is_running.load();
    }

private:
    ggml_jni_context();

    ggml_jni_context(const ggml_jni_context &)              = delete;
    ggml_jni_context(const ggml_jni_context &&)             = delete;
    ggml_jni_context & operator= (const ggml_jni_context &) = delete;

    ~ggml_jni_context();

private:
    float llm_temperature;
    float llm_top_p;
    //TODO:add other LLM parameters

    std::atomic<uint32_t> llm_inference_is_running;
    std::atomic<uint32_t> realtimemtmd_inference_is_running;

    bool initialized;
    // Set true after llama_backend_init() in init() and false again after
    // llama_backend_free() in cleanup(). Guards against double-free when
    // cleanup() is invoked both from the Java onDestroy path and from the
    // static singleton destructor at process exit.
    bool m_backend_initialized;

    // ===== Phase 2: model singleton state =====
    // The triple (loaded_model_path, loaded_mmproj_path, loaded_ngl) is
    // the cache key. A mismatch on any of the three triggers a full reload
    // (unload old, then load new) inside ensure_model_loaded(). Two paths
    // can be empty simultaneously: LLM inference sets mmproj_path="",
    // MTMD inference sets mmproj_path=<actual_path>. The same is true for
    // a real LLM/MTMD switch.
    //
    // The cache key intentionally uses n_gpu_layers (not the user-facing
    // backend_type enum) because two callers with the same backend type
    // can request different ngl values (LLM with ngl=99 vs MTMD with ngl=0
    // even when both pick the CDSP backend). Caching by ngl is the only
    // way to guarantee that the cached model has the correct backend
    // offload configuration: caching by backend_type alone caused the
    // "MTMD garbage output" regression where a 4GB LLM model already
    // resident in the Hexagon ION pool was reused for a CPU MTMD
    // inference, and the vision encoder compute subsequently failed
    // with "ion-batch: mempool full" because all 4GB were already in
    // ION and there was no room for the vision encoder buffers.
    std::string         loaded_model_path;
    std::string         loaded_mmproj_path;
    int                 loaded_ngl;                 // -1 (auto) / 0 (CPU) / 99 (CDSP offload all)
    llama_model *       loaded_model;              // owned; freed in unload_model()
    mtmd_context *      loaded_mctx;               // owned; freed in unload_model(); nullptr for LLM-only

    // Single-flight gate around load/unload/infer. The UI thread may
    // trigger a model switch while a previous inference is still
    // running (e.g. user picks a different model mid-generation); the
    // inference thread holds this mutex during ensure_model_loaded so
    // the unload+reload pair is atomic with respect to the inference.
    std::mutex          m_model_mutex;
};

// The single global reference to the JNI context singleton. Defined in
// ggml-jni-context.cpp (where it is bound to ggml_jni_context::get_instance());
// declared here with `extern` so that other translation units
// (mtmd-inference.cpp, llm-inference.cpp, realtime-video-recognition.cpp)
// can use it to drive the model singleton. The Meyers-singleton in
// get_instance() guarantees there is only one underlying object even
// though the reference is now visible from multiple translation units.
extern ggml_jni_context & g_jni_ctx;