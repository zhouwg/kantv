//TODO: re-write entire whispercpp.java with standard Android JNI specification
// interaction between KANTVMgr.java and whispercpp.java

package org.ggml.whispercpp;

public class whispercpp {
  private static final String TAG = whispercpp.class.getName();

  public static native String get_systeminfo();

  public static native int    get_cpu_core_counts();

  public static native void   set_benchmark_status(int bExitBenchmark);

  /**
   *
   * @param modelPath
   * @param audioFile
   * @param nBenchType  0: asr 1: memcpy 2: mulmat  3: full/whisper_encode
   * @param nThreadCounts
   * @return
   */
  public static native String bench(String modelPath, String audioFile, int nBenchType, int nThreadCounts);
}