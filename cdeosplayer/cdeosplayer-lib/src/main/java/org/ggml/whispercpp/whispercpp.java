//TODO: re-write entire whispercpp.java with standard Android JNI specification
// interaction between KANTVMgr.java and whispercpp.java

package org.ggml.whispercpp;

public class whispercpp {
  private static final String TAG = whispercpp.class.getName();

  public static native String get_systeminfo();

  public static native void   set_benchmark_status(int bExitBenchmark);

  /**
   *
   * @param modelPath
   * @param audioFile
   * @param nBenchType  0: memcpy 1: mulmat 2: asr 3: full/whisper_encoder
   * @param nThreadCounts
   * @return
   */
  public static native String bench(String modelPath, String audioFile, int nBenchType, int nThreadCounts);
}