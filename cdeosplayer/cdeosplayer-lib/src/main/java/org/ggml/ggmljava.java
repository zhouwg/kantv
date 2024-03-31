// TODO: 03-05-2024, re-write entire whispercpp.java with standard Android JNI specification
// TODO: 03-26-2024, rename this file to ggmljni to unify the JNI of whisper.cpp and llama.cpp, as these projects are all based on ggml

package org.ggml;

public class ggmljava {
    private static final String TAG = ggmljava.class.getName();

    // keep sync with ggml_jni_op in ggml-jni.h
    public static final int    GGML_JNI_OP_NONE     = 0;
    public static final int    GGML_JNI_OP_ADD      = 1;
    public static final int    GGML_JNI_OP_SUB      = 2;
    public static final int    GGML_JNI_OP_MUL      = 3;
    public static final int    GGML_JNI_OP_DIV      = 4;
    public static final int    GGML_JNI_OP_SUM      = 5;
    public static final int    GGML_JNI_OP_MUL_MAT  = 6;

    public static native int  asr_init(String strModelPath, int nThreadCounts, int nASRMode);

    public static native void asr_finalize();

    public static native void asr_start();

    public static native void asr_stop();

    public static native int  asr_reset(String strModelPath, int nThreadCounts, int nASRMode);

    public static native String asr_get_systeminfo();

    public static native int get_cpu_core_counts();

    //TODO: not work as expected
    public static native void asr_set_benchmark_status(int bExitBenchmark);

    /**
     * @param modelPath     /sdcard/kantv/ggml-xxxxxx.bin or  /sdcard/kantv/xxxxxx.gguf or qualcomm's dedicated model
     * @param audioPath     /sdcard/kantv/jfk.wav
     * @param nBenchType    0: asr(transcription) 1: memcpy 2: mulmat  3: full/whisper_encode 4: matrix  5: LLAMA 6: QNN sample 7: QNN matrix 8: QNN GGML
     * @param nThreadCounts 1 - 8
     * @param nBackendType  0: CPU  1: GPU  2: DSP
     * @param nOpType       type of matrix manipulate / GGML OP
     * @return
     */
    public static native String ggml_bench(String modelPath, String audioPath, int nBenchType, int nThreadCounts, int nBackendType, int nOpType);


    public static native String llm_get_systeminfo();


    public static native String llm_inference(String modelPath, String prompt, int nBenchType, int nThreadCounts);
}
