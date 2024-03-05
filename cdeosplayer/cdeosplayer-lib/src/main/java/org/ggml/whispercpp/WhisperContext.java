//borrow from: https://github.com/ggerganov/whisper.cpp/blob/master/examples/whisper.android.java/app/src/main/java/com/whispercpp/java/whisper/WhisperContext.java

package org.ggml.whispercpp;

import android.content.res.AssetManager;
import android.os.Build;
import android.util.Log;

import androidx.annotation.RequiresApi;


import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

import cdeos.media.player.CDELog;

public class WhisperContext {

  private static final String TAG = WhisperContext.class.getName();
  private long ptr;
  private final ExecutorService executorService;

  private WhisperContext(long ptr) {
    this.ptr = ptr;
    this.executorService = Executors.newSingleThreadExecutor();
  }

  public String transcribeData(float[] data) throws ExecutionException, InterruptedException {
    return executorService.submit(new Callable<String>() {
      @RequiresApi(api = Build.VERSION_CODES.O)
      @Override
      public String call() throws Exception {
        if (ptr == 0L) {
          throw new IllegalStateException();
        }
        int numThreads = WhisperCpuConfig.getPreferredThreadCount();
        CDELog.j(TAG, "Selecting " + numThreads + " threads");

        StringBuilder result = new StringBuilder();
        synchronized (this) {

          WhisperLib.fullTranscribe(ptr, numThreads, data);
          int textCount = WhisperLib.getTextSegmentCount(ptr);
          for (int i = 0; i < textCount; i++) {
            String sentence = WhisperLib.getTextSegment(ptr, i);
            result.append(sentence);
          }
        }
        return result.toString();
      }
    }).get();
  }



  @RequiresApi(api = Build.VERSION_CODES.O)
  public String benchMemory(int nthreads) throws ExecutionException, InterruptedException {
    return executorService.submit(() -> WhisperLib.benchMemcpy(nthreads)).get();
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public String benchGgmlMulMat(int nthreads) throws ExecutionException, InterruptedException {
    return executorService.submit(() -> WhisperLib.benchGgmlMulMat(nthreads)).get();
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public void release() throws ExecutionException, InterruptedException {
    executorService.submit(() -> {
      if (ptr != 0L) {
        WhisperLib.freeContext(ptr);
        ptr = 0;
      }
    }).get();
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public static WhisperContext createContextFromFile(String filePath) {
    long ptr = WhisperLib.initContext(filePath);
    if (ptr == 0L) {
      throw new RuntimeException("Couldn't create context with path " + filePath);
    }
    return new WhisperContext(ptr);
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public static WhisperContext createContextFromInputStream(InputStream stream) {
    long ptr = WhisperLib.initContextFromInputStream(stream);
    if (ptr == 0L) {
      throw new RuntimeException("Couldn't create context from input stream");
    }
    return new WhisperContext(ptr);
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public static WhisperContext createContextFromAsset(AssetManager assetManager, String assetPath) {
    long ptr = WhisperLib.initContextFromAsset(assetManager, assetPath);
    if (ptr == 0L) {
      throw new RuntimeException("Couldn't create context from asset " + assetPath);
    }
    return new WhisperContext(ptr);
  }

  @RequiresApi(api = Build.VERSION_CODES.O)
  public static String getSystemInfo() {
    return WhisperLib.getSystemInfo();
  }
}
