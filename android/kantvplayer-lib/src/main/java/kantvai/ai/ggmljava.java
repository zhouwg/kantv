 /*
  * Copyright (c) 2024- KanTV Authors
  */
package kantvai.ai;

 import android.view.Surface;

 import kantvai.media.player.KANTVLibraryLoader;

 public final class ggmljava {
    private static final String TAG = ggmljava.class.getName();

    // Backend type constants kept for backwards compatibility with existing code
    // that may still reference them. Backend is now decided at build time (GGML_USE_HEXAGON).
    public static final int HEXAGON_BACKEND_CDSP   = 3;
    public static final int HEXAGON_BACKEND_GGML   = 4;


     static {
       KANTVLibraryLoader.load("ggml-jni");
    }

    public static native int asr_init(String strModelPath, int nASRMode);

    public static native void asr_finalize();

    public static native void asr_start();

    public static native void asr_stop();

    public static native int asr_reset(String strModelPath, int nASRMode);

    public static native String asr_get_systeminfo();

    public static native int get_cpu_core_counts();

    /**
     * @param bExitBenchmark 0: reset internal status  1: exit/abort time-consuming bench task(such as LLM inference)
     */
    public static native void ggml_set_benchmark_status(int bExitBenchmark);

    /**
     * @param modelPath     /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/xxxxxx.gguf
     * @param userData      ASR: /sdcard/kantv/jfk.wav or LLM: user input from UI
     * @param nBenchType    0: ASR(whisper.cpp) 1: LLM(llama.cpp)
     * @return
     */
    public static native String ggml_bench(String modelPath, String userData, int nBenchType);

    public static native String llm_get_systeminfo();


    /**
     * @param modelPath     /sdcard/xxxxxx.gguf
     * @param prompt        user input from UI
     * @param nLLMType      not used currently
     * @return
     */
    public static native String llm_inference(String modelPath, String prompt, int nLLMType);

    public static native void    llm_init_running_state();
    public static native void    llm_reset_running_state();
    public static native boolean llm_is_running_state();

    public static native void    realtimemtmd_init_running_state();
    public static native void    realtimemtmd_reset_running_state();
    public static native boolean realtimemtmd_is_running_state();

    public static native void    sd_init_running_state();
    public static native void    sd_reset_running_state();
    public static native boolean sd_is_running_state();


    /**
     * @param modelPath            /sdcard/xxxxxx.gguf
     * @param mmprojModelPath      /sdcard/xxxxxx.gguf
     * @param mediaPath
     * @param prompt        user input from UI
     * @param nLLMType      1: MTMD image, 2: MTMD audio
     * @return
     */
    public static native String mtmd_inference(String modelPath, String mmprojModelPath, String mediaPath, String prompt, int nLLMType);

    /**
     * @param modelPath     /sdcard/xxxxxx.ckpt or /sdcard/safetensors or other name of SD model
     * @param auxModePath
     * @param prompt        user input from UI
     * @param nLLMType      not used currently
     * @return
     */
    public static native String stablediffusion_inference(String modelPath, String auxModePath, String prompt, int nLLMType);

    public static native byte[] jni_text2image(String text);

    public static native boolean openCamera(int facing);

    public static native void closeCamera();

    public static native void setOutputWindow(Surface surface);

    public static native boolean isStableDiffusionHexagonEnabled();

    public static native boolean isGGMLHexagonEnabled();

    public static native void setLLMTemperature(float temperature);

    public static native float getLLMTemperature();

    public static native void setLLMTopP(float top_p);

    public static native float getLLMTopP();

    public static native void  llm_finalize();
}
