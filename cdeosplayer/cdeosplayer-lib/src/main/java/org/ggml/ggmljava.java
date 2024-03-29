// TODO: 03-05-2024, re-write entire whispercpp.java with standard Android JNI specification
// TODO: 03-26-2024, rename this file to ggmljni to unify the JNI of whisper.cpp and llama.cpp, as these projects are all based on ggml

package org.ggml;

public class ggmljava {
    private static final String TAG = ggmljava.class.getName();

    public static final int WHISPER_ASR_MODE_NORMAL         = 0;
    public static final int WHISPER_ASR_MODE_PRESURETEST    = 1;
    public static final int WHISPER_ASR_MODE_BECHMARK       = 2;

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
     * @param modelPath     /sdcard/kantv/ggml-xxxxx.bin
     * @param audioPath     /sdcard/kantv/jfk.wav
     * @param nBenchType    0: asr(transcription) 1: memcpy 2: mulmat  3: full/whisper_encode 4: matrix  5: LLAMA 6: QNN
     * @param nThreadCounts 1 - 8
     * @return
     */
    public static native String asr_bench(String modelPath, String audioPath, int nBenchType, int nThreadCounts);


    public static native String llm_get_systeminfo();


    public static native String llm_inference(String modelPath, String prompt, int nBenchType, int nThreadCounts);
}
