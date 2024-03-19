//TODO: re-write entire whispercpp.java with standard Android JNI specification
// interaction between KANTVMgr.java and whispercpp.java

package org.ggml.whispercpp;

public class whispercpp {
  private static final String TAG = whispercpp.class.getName();

  public static final int WHISPER_ASR_MODE_NORMAL       = 0;
  public static final int WHISPER_ASR_MODE_PRESURETEST  = 1;
  public static final int WHISPER_ASR_MODE_BECHMARK     = 2;

  public static native int asr_init(String strModelPath, int nThreadCounts, int nASRMode);

  public static native void asr_finalize();

  public static native void asr_start();
  public static native void asr_stop();
  public static native int asr_reset(String strModelPath, int nThreadCounts, int nASRMode);

  public static native String get_systeminfo();

  public static native int    get_cpu_core_counts();

  //TODO: not work as expected, just skip this during PoC stage
  public static native void   set_benchmark_status(int bExitBenchmark);

  /**
   *
   * @param modelPath         /sdcard/kantv/ggml-xxxxx.bin
   * @param audioPath         /sdcard/kantv/jfk.wav
   * @param nBenchType        0: asr 1: memcpy 2: mulmat  3: full/whisper_encode
   * @param nThreadCounts     1 - 8
   * @return
   */
  public static native String bench(String modelPath, String audioPath, int nBenchType, int nThreadCounts);
}