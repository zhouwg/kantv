//borrow from: https://github.com/ggerganov/whisper.cpp/blob/master/examples/whisper.android.java/app/src/main/java/com/whispercpp/java/whisper/WhisperCpuConfig.java

package org.ggml.whispercpp;

import android.os.Build;

import androidx.annotation.RequiresApi;

public class WhisperCpuConfig {
  @RequiresApi(api = Build.VERSION_CODES.N)
  public static int getPreferredThreadCount() {
    return Math.max(CpuInfo.getHighPerfCpuCount(), 2);
  }
}
