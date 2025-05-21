 /*
  * Copyright (c) 2024- KanTV Authors
  */
package kantvai.ai;

 import android.view.Surface;

 import kantvai.media.player.KANTVLibraryLoader;

 public final class ggmljava {
    private static final String TAG = ggmljava.class.getName();

    //keep sync with ggml-hexagon.cpp
    public static final int HEXAGON_BACKEND_QNNCPU = 0;
    public static final int HEXAGON_BACKEND_QNNGPU = 1;
    public static final int HEXAGON_BACKEND_QNNNPU = 2;
    public static final int HEXAGON_BACKEND_CDSP   = 3;
    public static final int HEXAGON_BACKEND_GGML   = 4;//"fake" HEXAGON backend for compare performance between HEXAGON backend and ggml backend

    //keep sync with ggml-hexagon.cpp
    public static final int HWACCEL_QNN = 0;
    public static final int HWACCEL_QNN_SINGLEGRAPH = 1;
    public static final int HWACCEL_CDSP = 2;


     static {
       KANTVLibraryLoader.load("ggml-jni");
    }

    public static native int asr_init(String strModelPath, int nThreadCounts, int nASRMode, int nBackendType);

    public static native void asr_finalize();

    public static native void asr_start();

    public static native void asr_stop();

    public static native int asr_reset(String strModelPath, int nThreadCounts, int nASRMode, int nBackendType);

    public static native String asr_get_systeminfo();

    public static native int get_cpu_core_counts();

    /**
     * @param bExitBenchmark 0: reset internal status  1: exit/abort time-consuming bench task(such as LLM inference)
     */
    public static native void ggml_set_benchmark_status(int bExitBenchmark);

    /**
     * @param modelPath     /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/xxxxxx.gguf
     * @param userData      ASR: /sdcard/kantv/jfk.wav or LLM: user input from UI
     * @param nBenchType    0: memcpy 1: mulmat 2: ASR(whisper.cpp) 3: LLM(llama.cpp)
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
     * @param nHWAccelType  0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
     * @return
     */
    public static native String ggml_bench(String modelPath, String userData, int nBenchType, int nThreadCounts, int nBackendType, int nHWAccelType);

    public static native String llm_get_systeminfo();


    /**
     * @param modelPath     /sdcard/xxxxxx.gguf
     * @param prompt        user input from UI
     * @param nLLMType      not used currently
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
     * @param nHWAccelType  0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
     * @return
     */
    public static native String llm_inference(String modelPath, String prompt, int nLLMType, int nThreadCounts, int nBackendType, int nHWAccelType);

    public static native void   inference_init_inference();
    /**
     * stop AI inference
     */
    public static native void    inference_stop_inference();

    /**
     * check whether AI inference is running
     */
    public static native boolean inference_is_running();

    /**
     * @param modelPath     /sdcard/xxxxxx.gguf
     * @param mmprojModelPath
     * @param imgPath
     * @param prompt        user input from UI
     * @param nLLMType      not used currently
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
     * @param nHWAccelType  0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
     * @return
     */
    public static native String llava_inference(String modelPath, String mmprojModelPath, String imgPath, String prompt, int nLLMType, int nThreadCounts, int nBackendType, int nHWAccelType);

    /**
     * @param modelPath     /sdcard/xxxxxx.ckpt or /sdcard/safetensors or other name of SD model
     * @param auxModePath
     * @param prompt        user input from UI
     * @param nLLMType      not used currently
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: HEXAGON_BACKEND_QNNCPU 1: HEXAGON_BACKEND_QNNGPU 2: HEXAGON_BACKEND_QNNNPU, 3: HEXAGON_BACKEND_CDSP 4: ggml
     * @param nHWAccelType  0: HWACCEL_QNN 1: HWACCEL_QNN_SINGLEGRAPH 2: HWACCEL_CDSP
     * @return
     */
    public static native String stablediffusion_inference(String modelPath, String auxModePath, String prompt, int nLLMType, int nThreadCounts, int nBackendType, int nHWAccelType);

    public static native byte[] jni_text2image(String text);

    public static native boolean openCamera(int facing);

    public static native void closeCamera();

    public static native void setOutputWindow(Surface surface);

    public static native boolean isStableDiffusionEnabled();

    public static native boolean isGGMLHexagonEnabled();

    public static native void setLLMTemperature(float temperature);

    public static native float getLLMTemperature();

    public static native void setLLMTopP(float top_p);

    public static native float getLLMTopP();
}
