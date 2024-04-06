//==============================================================================
//
//  Copyright (c) 2020, 2022 Qualcomm Technologies, Inc.
//  All Rights Reserved.
//  Confidential and Proprietary - Qualcomm Technologies, Inc.
//
//==============================================================================

#include "LogUtils.hpp"
#include "ggml-jni.h"

void qnn::log::utils::logStdoutCallback(const char* fmt,
                                        QnnLog_Level_t level,
                                        uint64_t timestamp,
                                        va_list argp) {
  const char* levelStr = "";
  switch (level) {
    case QNN_LOG_LEVEL_ERROR:
      levelStr = " ERROR ";
      break;
    case QNN_LOG_LEVEL_WARN:
      levelStr = "WARNING";
      break;
    case QNN_LOG_LEVEL_INFO:
      levelStr = "  INFO ";
      break;
    case QNN_LOG_LEVEL_DEBUG:
      levelStr = " DEBUG ";
      break;
    case QNN_LOG_LEVEL_VERBOSE:
      levelStr = "VERBOSE";
      break;
    case QNN_LOG_LEVEL_MAX:
      levelStr = "UNKNOWN";
      break;
  }

  double ms = (double)timestamp / 1000000.0;
  // To avoid interleaved messages
  {
    std::lock_guard<std::mutex> lock(sg_logUtilMutex);

    static unsigned char s_qnn_jni_buf[JNI_BUF_LEN];
    int len_content = 0;
    memset(s_qnn_jni_buf, 0, JNI_BUF_LEN);
    len_content = vsnprintf(reinterpret_cast<char *const>(s_qnn_jni_buf), JNI_BUF_LEN, fmt, argp);
    snprintf(reinterpret_cast<char *const>(s_qnn_jni_buf + len_content), JNI_BUF_LEN - len_content, "\n");
    LOGGD("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
    if (level <= QNN_LOG_LEVEL_INFO) {
      GGML_JNI_NOTIFY("%8.1fms [%-7s] %s ", ms, levelStr, s_qnn_jni_buf);
    }
  }
}
