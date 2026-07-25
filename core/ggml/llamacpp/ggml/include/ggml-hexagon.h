#pragma once

#include "ggml.h"
#include "ggml-backend.h"

#ifdef  __cplusplus
extern "C" {
#endif

// backend API
GGML_BACKEND_API ggml_backend_t ggml_backend_hexagon_init(void);

GGML_BACKEND_API bool ggml_backend_is_hexagon(ggml_backend_t backend);

GGML_BACKEND_API ggml_backend_reg_t ggml_backend_hexagon_reg(void);

// Set the runtime library path where DSP skeleton .so files (libggmldsp-skel-v*.so)
// and ggml-hexagon.cfg are located. Must be called before any hexagon backend
// registration (i.e. before llama_backend_init / asr_init) to take effect.
// On Android, this should point to the app's data directory (e.g. /data/data/<pkg>/)
// where the APK assets are copied at startup.
GGML_BACKEND_API void ggml_hexagon_set_runtime_libpath(const char * path);

#ifdef  __cplusplus
}
#endif
