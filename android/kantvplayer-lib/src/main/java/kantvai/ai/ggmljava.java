// TODO: 03-05-2024, re-write entire whispercpp.java with standard Android JNI specification
// TODO: 03-26-2024, rename this file to ggmljni to unify the JNI of whisper.cpp and llama.cpp, as these projects are all based on ggml

package kantvai.ai;

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
         GGML_OP_SIN,
         GGML_OP_COS,
         GGML_OP_SUM,
         GGML_OP_SUM_ROWS,
         GGML_OP_MEAN,
         GGML_OP_ARGMAX,
         GGML_OP_COUNT_EQUAL,
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
         GGML_OP_CLAMP,
         GGML_OP_CONV_TRANSPOSE_1D,
         GGML_OP_IM2COL,
         GGML_OP_IM2COL_BACK,
         GGML_OP_CONV_TRANSPOSE_2D,
         GGML_OP_POOL_1D,
         GGML_OP_POOL_2D,
         GGML_OP_POOL_2D_BACK,
         GGML_OP_UPSCALE, // nearest interpolate
         GGML_OP_PAD,
         GGML_OP_PAD_REFLECT_1D,
         GGML_OP_ARANGE,
         GGML_OP_TIMESTEP_EMBEDDING,
         GGML_OP_ARGSORT,
         GGML_OP_LEAKY_RELU,

         GGML_OP_FLASH_ATTN_EXT,
         GGML_OP_FLASH_ATTN_BACK,
         GGML_OP_SSM_CONV,
         GGML_OP_SSM_SCAN,
         GGML_OP_WIN_PART,
         GGML_OP_WIN_UNPART,
         GGML_OP_GET_REL_POS,
         GGML_OP_ADD_REL_POS,
         GGML_OP_RWKV_WKV6,
         GGML_OP_GATED_LINEAR_ATTN,

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
         GGML_OP_OPT_STEP_ADAMW,

         GGML_OP_COUNT,
    };


     public static native int  asr_init(String strModelPath, int nThreadCounts, int nASRMode, int nBackendType);

    public static native void asr_finalize();

    public static native void asr_start();

    public static native void asr_stop();

    public static native int  asr_reset(String strModelPath, int nThreadCounts, int nASRMode, int nBackendType);

    public static native String asr_get_systeminfo();

    public static native int get_cpu_core_counts();


     /**
      * @param bExitBenchmark  0: reset internal status  1: exit/abort time-consuming bench task(such as LLM inference)
      */
    public static native void ggml_set_benchmark_status(int bExitBenchmark);

    /**
     * @param modelPath     /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/kantv/xxxxxx.gguf or qualcomm's prebuilt dedicated model.so or ""
     * @param userData      ASR: /sdcard/kantv/jfk.wav / LLM: user input / TEXT2IMAGE: user input / MNIST: image path / TTS: user input
     * @param nBenchType    0: memcpy 1: mulmat 2: ASR(whisper.cpp) 3: LLM(llama.cpp) 6: MNIST
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: ggml 1: NPU
     * @param nOpType       type of matrix manipulate / GGML OP / type of various complex/complicated computation graph
     * @return
     */
    public static native String ggml_bench(String modelPath, String userData, int nBenchType, int nThreadCounts, int nBackendType, int nOpType);

    //05-25-2024, add for MiniCPM-V(A GPT-4V Level Multimodal LLM, https://github.com/OpenBMB/MiniCPM-V) or other GPT-4o style Multimodal LLM)
    //"m" for "multimodal"
    public static native String ggml_bench_m(String modelPath, String imgPath, String userData, int nBenchType, int nThreadCounts, int nBackendType);


    public static native String llm_get_systeminfo();


    public static native String llm_inference(String modelPath, String prompt, int nBenchType, int nThreadCounts, int nBackendType);
}
