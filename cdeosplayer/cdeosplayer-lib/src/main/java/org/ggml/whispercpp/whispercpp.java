//TODO: re-write entire whispercpp.java with standard Android JNI specification
// interaction between KANTVMgr.java and whispercpp.java

package org.ggml.whispercpp;

public class whispercpp {
  private static final String TAG = whispercpp.class.getName();

  public static native String getSystemInfo();

  public static native String benchMemcpy(int nThreadCounts);

  public static native String benchMulMat(int nThreadCounts);

  public static native void set_mulmat_benchmark_status(int bExitBenchmark);

  public static native String transcribe_from_file(String modelPath, String audioFile, int nThreadCounts);
}