//borrow from:https://github.com/ggerganov/whisper.cpp/blob/master/examples/whisper.android.java/app/src/main/java/com/whispercpp/java/whisper/WhisperLib.java

package org.ggml.whispercpp;

import android.content.res.AssetManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;

import java.io.InputStream;

import cdeos.media.player.CDELog;

@RequiresApi(api = Build.VERSION_CODES.O)
public class WhisperLib {
  private static final String TAG = WhisperLib.class.getName();

  public static native long initContextFromInputStream(InputStream inputStream);

  public static native long initContextFromAsset(AssetManager assetManager, String assetPath);

  public static native long initContext(String modelPath);

  public static native void freeContext(long contextPtr);

  public static native void fullTranscribe(long contextPtr, int numThreads, float[] audioData);

  public static native int getTextSegmentCount(long contextPtr);

  public static native String getTextSegment(long contextPtr, int index);

  public static native long getTextSegmentT0(long contextPtr, int index);

  public static native long getTextSegmentT1(long contextPtr, int index);

  public static native String getSystemInfo();

  public static native String benchMemcpy(int nthread);

  public static native String benchGgmlMulMat(int nthread);

  public static native void set_mulmat_benchmark_status(int bExit);
}