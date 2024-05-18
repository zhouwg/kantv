// TODO: 03-05-2024, re-write entire whispercpp.java with standard Android JNI specification
// TODO: 03-26-2024, rename this file to ggmljni to unify the JNI of whisper.cpp and llama.cpp, as these projects are all based on ggml

package org.ggml;

public class ggmljava {
    private static final String TAG = ggmljava.class.getName();

    //keep sync between here and ggml.h
    // available GGML tensor operations:
   public enum ggml_op {
        GGML_OP_NONE,

        GGML_OP_DUP,
        GGML_OP_ADD,
        GGML_OP_ADD1,
        GGML_OP_ACC,
        GGML_OP_SUB,
        GGML_OP_MUL,
        GGML_OP_DIV,
        GGML_OP_SQR,
        GGML_OP_SQRT,
        GGML_OP_LOG,
        GGML_OP_SUM,
        GGML_OP_SUM_ROWS,
        GGML_OP_MEAN,
        GGML_OP_ARGMAX,
        GGML_OP_REPEAT,
        GGML_OP_REPEAT_BACK,
        GGML_OP_CONCAT,
        GGML_OP_SILU_BACK,
        GGML_OP_NORM, // normalize
        GGML_OP_RMS_NORM,
        GGML_OP_RMS_NORM_BACK,
        GGML_OP_GROUP_NORM,

        GGML_OP_MUL_MAT,
        GGML_OP_MUL_MAT_ID,
        GGML_OP_OUT_PROD,

        GGML_OP_SCALE,
        GGML_OP_SET,
        GGML_OP_CPY,
        GGML_OP_CONT,
        GGML_OP_RESHAPE,
        GGML_OP_VIEW,
        GGML_OP_PERMUTE,
        GGML_OP_TRANSPOSE,
        GGML_OP_GET_ROWS,
        GGML_OP_GET_ROWS_BACK,
        GGML_OP_DIAG,
        GGML_OP_DIAG_MASK_INF,
        GGML_OP_DIAG_MASK_ZERO,
        GGML_OP_SOFT_MAX,
        GGML_OP_SOFT_MAX_BACK,
        GGML_OP_ROPE,
        GGML_OP_ROPE_BACK,
        GGML_OP_ALIBI,
        GGML_OP_CLAMP,
        GGML_OP_CONV_TRANSPOSE_1D,
        GGML_OP_IM2COL,
        GGML_OP_CONV_TRANSPOSE_2D,
        GGML_OP_POOL_1D,
        GGML_OP_POOL_2D,
        GGML_OP_UPSCALE, // nearest interpolate
        GGML_OP_PAD,
        GGML_OP_ARANGE,
        GGML_OP_TIMESTEP_EMBEDDING,
        GGML_OP_ARGSORT,
        GGML_OP_LEAKY_RELU,

        GGML_OP_FLASH_ATTN,
        GGML_OP_FLASH_FF,
        GGML_OP_FLASH_ATTN_BACK,
        GGML_OP_SSM_CONV,
        GGML_OP_SSM_SCAN,
        GGML_OP_WIN_PART,
        GGML_OP_WIN_UNPART,
        GGML_OP_GET_REL_POS,
        GGML_OP_ADD_REL_POS,

        GGML_OP_UNARY,

        GGML_OP_MAP_UNARY,
        GGML_OP_MAP_BINARY,

        GGML_OP_MAP_CUSTOM1_F32,
        GGML_OP_MAP_CUSTOM2_F32,
        GGML_OP_MAP_CUSTOM3_F32,

        GGML_OP_MAP_CUSTOM1,
        GGML_OP_MAP_CUSTOM2,
        GGML_OP_MAP_CUSTOM3,

        GGML_OP_CROSS_ENTROPY_LOSS,
        GGML_OP_CROSS_ENTROPY_LOSS_BACK,

        GGML_OP_COUNT,
    };


    public static native int  asr_init(String strModelPath, int nThreadCounts, int nASRMode, int nBackendType);

    public static native void asr_finalize();

    public static native void asr_start();

    public static native void asr_stop();

    public static native int  asr_reset(String strModelPath, int nThreadCounts, int nASRMode, int nBackendType);

    public static native String asr_get_systeminfo();

    public static native int get_cpu_core_counts();

    //TODO: not work as expected
    public static native void asr_set_benchmark_status(int bExitBenchmark);

    /**
     * @param modelPath     /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/kantv/xxxxxx.gguf or qualcomm's prebuilt dedicated model.so or ""
     * @param userData      ASR: /sdcard/kantv/jfk.wav LLM: user input Text2Image: user input MNIST: image path TTS: user input
     * @param nBenchType    0: whisper asr 1: memcpy 2: mulmat  3: whisper full 4: LLAMA 5: stable diffusion 6: QNN sample 7: QNN saver 8: QNN matrix 9: QNN GGML 10: QNN complex 11: QNN GGML OP(QNN UT) 12: QNN UT automation
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: CPU  1: GPU  2: DSP 3: ggml("fake" QNN backend, just for compare performance)
     * @param nOpType       type of matrix manipulate / GGML OP / type of various complex/complicated computation graph
     * @return
     */
    public static native String ggml_bench(String modelPath, String userData, int nBenchType, int nThreadCounts, int nBackendType, int nOpType);


    public static native String llm_get_systeminfo();


    public static native String llm_inference(String modelPath, String prompt, int nBenchType, int nThreadCounts, int nBackendType);
}
