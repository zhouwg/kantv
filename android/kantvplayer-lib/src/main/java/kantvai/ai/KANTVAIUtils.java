/*
 * Copyright (c) 2024- KanTV Authors
 */
 package kantvai.ai;

 import android.app.Activity;
 import android.app.ActivityManager;
 import android.content.Context;
 import android.os.Build;
 import android.os.Debug;

 import java.util.concurrent.atomic.AtomicBoolean;

 import kantvai.media.player.KANTVLog;

 public final class KANTVAIUtils {
     private final static String TAG                        = KANTVAIUtils.class.getName();

     /**
        whether ASR subsystem is initialized
      */
     private static AtomicBoolean mASRSubsystemInit         = new AtomicBoolean(false);

     /**
      * ASR mode
      */
     public static final int  ASR_MODE_NORMAL               = 0;     // transcription
     public static final int  ASR_MODE_PRESURETEST          = 1;     // pressure test of asr subsystem
     public static final int  ASR_MODE_BECHMARK             = 2;     // asr performance benchmark
     public static final int  ASR_MODE_TRANSCRIPTION_RECORD = 3;     // transcription + audio record
     private static int       mASRMode   = ASR_MODE_NORMAL;          // default ASR mode

     //TODO: merge with AI bench_type
     public static final int INFERENCE_ASR                  = 0;
     public static final int INFERENCE_LLM                  = 1;
     public static final  int INFERENCE_LLMAVA              = 2;
     public static final  int INFERENCE_STABLEDIFUSSION     = 3;


    //this is experimental value to check whether a specified AI model has downloaded successfully
    // naive algorithm at the moment:
    // if (1 == (real size of AI model - size of downloaded file))
    //     download ok
    // if (real size of AI model - size of downloaded file > DOWNLOAD_SIZE_CHECK_RANGE)
    //     download failure
    public static final long DOWNLOAD_SIZE_CHECK_RANGE      = 700 * 1024 * 1024L;


     //=============================================================================================
     //add new AI benchmark type / new backend / new realtime inference type for GGML/NCNN here
     //
     //keep sync with ggml_jni_bench_type in ggml-jni.h & ncnn_jni_bench_type in ncnn-jni.h
     public enum bench_type {
         GGML_BENCHMARK_MEMCPY,                    //memcpy  benchmark
         GGML_BENCHMARK_MULMAT,                    //mulmat  benchmark
         GGML_BENCHMARK_ASR,                       //ASR(whisper.cpp) benchmark using GGML
         GGML_BENCHMARK_LLM,                       //LLM(llama.cpp) benchmark using GGML
         GGML_BENCHMARK_TEXT2IMAGE,                //TEXT2IMAGE(stablediffusion.cpp) benchmark using GGML
         GGML_BENCHMARK_LLM_V,                     //A GPT-4V style Multimodal LLM benchmark using llama.cpp based on GGML
         GGML_BENCHMARK_LLM_O,                     //A GPT-4o style Multimodal LLM benchmark using llama.cpp based on GGML

         GGML_BENCHMARK_CV_MNIST,                  //MNIST inference using GGML
         GGML_BENCHMARK_TTS,                       //TTS(bark.cpp) benchmark using GGML
         GGML_BENCHMARK_MAX,                       //used for separate GGML and NCNN
         NCNN_BENCHMARK_RESNET,
         NCNN_BENCHMARK_SQUEEZENET,
         NCNN_BENCHMARK_MNIST,
         NCNN_BENCHMARK_ASR,
         NCNN_BENCHMARK_TTS,
         NCNN_BENCHARK_YOLOV5,
         NCNN_BENCHARK_YOLOV10,
         NCNN_BENCHMARK_MAX,
     };

     //keep sync with ncnn-jni.h, realtime inference with live camera / online TV using NCNN
     public enum ncnn_realtimeinference_type {
         NCNN_REALTIMEINFERENCE_FACEDETECT,
         NCNN_REALTIMEINFERENCE_NANODAT,
         NCNN_REALTIMEINFERENCE_YOLOV10
     };
     //keep sync with ncnn-jni.h, ncnn backend
     public static final int NCNN_BACKEND_CPU           = 0;
     public static final int NCNN_BACKEND_GPU           = 1;
     //=============================================================================================

     public static String getDeviceInfo(Activity activity, int inference_type) {
         ActivityManager am = (ActivityManager) activity.getSystemService(Context.ACTIVITY_SERVICE);
         ActivityManager.MemoryInfo memoryInfo = new ActivityManager.MemoryInfo();
         am.getMemoryInfo(memoryInfo);
         long totalMem = memoryInfo.totalMem;
         long availMem = memoryInfo.availMem;
         boolean isLowMemory = memoryInfo.lowMemory;
         long threshold = memoryInfo.threshold;
         Debug.MemoryInfo debugInfo = new Debug.MemoryInfo();
         Debug.getMemoryInfo(debugInfo);
         int totalPrivateClean = debugInfo.getTotalPrivateClean();
         int totalPrivateDirty = debugInfo.getTotalPrivateDirty();
         int totalPss = debugInfo.getTotalPss();
         int totalSharedClean = debugInfo.getTotalSharedClean();
         int totalSharedDirty = debugInfo.getTotalSharedDirty();
         int totalSwappablePss = debugInfo.getTotalSwappablePss();
         int totalUsageMemory = totalPrivateClean + totalPrivateDirty + totalPss + totalSharedClean + totalSharedDirty + totalSwappablePss;
         int Bytes2MBytes = (1 << 20);
         //VSS - Virtual Set Size
         //RSS - Resident Set Size
         //PSS - Proportional Set Size
         //USS - Unique Set Size
         String memoryInfoString =
                 "total mem              ：" + (totalMem >> 20) + "MB" + "  "
                         + "isLowMeory:" + isLowMemory + " "
                         + "threshold of low mem：" + threshold / Bytes2MBytes + "MB" + "  "
                         + "available mem：" + availMem / Bytes2MBytes + "MB" + " "
                         + "NativeHeapSize：" + (Debug.getNativeHeapSize() >> 20) + "MB" + "  "
                         + "NativeHeapAllocatedSize：" + (Debug.getNativeHeapAllocatedSize() >> 20) + "MB" + " "
                         + "NativeHeapFreeSize：" + (Debug.getNativeHeapFreeSize() >> 20) + "MB " + " "
                         + "total private dirty memory：" + totalPrivateDirty / 1024 + "MB" + " "
                         + "total shared  dirty memory：" + totalSharedDirty / 1024 + "MB" + " "
                         + "total PSS memory               ：" + totalPss / 1024 + "MB" + " "
                         + "total swappable memory  ：" + totalSwappablePss / 1024 + "MB" + " "
                         + "total usage memory           ：" + totalUsageMemory / 1024 + "MB" + " ";
         KANTVLog.j(TAG, "memory info: " + memoryInfoString);

         String systemInfo;
         if (inference_type == INFERENCE_ASR)
             systemInfo = ggmljava.asr_get_systeminfo();
         else if (inference_type == INFERENCE_LLM)
             systemInfo = ggmljava.llm_get_systeminfo();
         else
             systemInfo = "unknown system info of unsupported inference type:" + Integer.toString(inference_type) + " ";

         String deviceInfo = "Device info:" + " "
                 + Build.BRAND + " "
                 + Build.HARDWARE + " "
                 + "Android " + android.os.Build.VERSION.RELEASE + " "
                 + "Arch:" + Build.CPU_ABI + " "
                 + "(" + systemInfo + ")" + " "
                 + "Mem:total " + (totalMem >> 20) + "MB"  + " "
                 + "available " + (availMem >> 20) + "MB"   + " "
                 + "usage " + (totalUsageMemory >> 10) + "MB";

         return deviceInfo;
     }

     public static String getBenchmarkDesc(int benchmarkIndex) {
         bench_type[] benchTypes = bench_type.values();
         for (bench_type item : benchTypes) {
             if (benchmarkIndex < bench_type.GGML_BENCHMARK_MAX.ordinal()) {
                 if (item.ordinal() == benchmarkIndex) {
                     return item.name();
                 }
             } else {
                 if (item.ordinal() == benchmarkIndex + 1) {
                     return item.name();
                 }
             }
         }

         return "unknown";
     }

     public static String getGGMLBackendDesc(int n_backend_type) {
         switch (n_backend_type) {
             case ggmljava.HEXAGON_BACKEND_QNNCPU:
                 return "QNN-CPU";
             case ggmljava.HEXAGON_BACKEND_QNNGPU:
                 return "QNN-GPU";
             case ggmljava.HEXAGON_BACKEND_QNNNPU:
                 return "QNN-NPU";
             case ggmljava.HEXAGON_BACKEND_CDSP:
                 return "Hexagon-CDSP";
             case ggmljava.HEXAGON_BACKEND_GGML:
                 return "ggml";      //fake backend, just used to compare performance between Hexagon and original GGML
             default:
                 return "unknown";
         }
     }

     public static String getNCNNBackendDesc(int n_backend_type) {
         switch (n_backend_type) {
             case 0:
                 return "CPU";
             case 1:
                 return "GPU";
             default:
                 return "unknown";
         }
     }

     public static String getASRModelString(int asrModelType) {
         switch (asrModelType) {
             case 0:
                 return "tiny";
             case 1:
                 return "tiny.en";
             case 2:
                 return "tiny.en-q5_1";
             case 3:
                 return "tiny.en-q8_0";
             case 4:
                 return "tiny-q5_1";
             case 5:
                 return "base";
             case 6:
                 return "base.en";
             case 7:
                 return "base-q5_1";
             case 8:
                 return "small";
             case 9:
                 return "small.en";
             case 10:
                 return "small.en-q5_1";
             case 11:
                 return "small-q5_1";
             case 12:
                 return "medium";
             case 13:
                 return "medium.en";
             case 14:
                 return "medium.en-q5_0";
             case 15:
                 return "large";

             default:
                 return "unknown";
         }
     }

     public static int getASRMode() {
         return mASRMode;
     }

     public static String getASRModeString( int asrMode) {
         switch (asrMode) {
             case ASR_MODE_NORMAL:
                 return "ASR_MODE_NORMAL";
             case ASR_MODE_PRESURETEST:
                 return "ASR_MODE_PRESURETEST";
             case ASR_MODE_BECHMARK:
                 return "ASR_MODE_BENCHAMRK";
             case ASR_MODE_TRANSCRIPTION_RECORD:
                 return "ASR_MODE_TRANSCRIPTION_RECORD";
             default:
                 return "unknown";
         }
     }

     public static void setASRSubsystemInit(boolean bEnabled) {
         mASRSubsystemInit.set(bEnabled);
     }

     public static boolean getASRSubsystemInit() {
         return mASRSubsystemInit.get();
     }

    //FIXME: should I move these helper functions to KANTVAIModelMgr.java?
    public static boolean isASRModel(String name) {
        String[] asrModels = {
                "tiny",
                "tiny.en",
                "tiny.en-q5_1",
                "tiny.en-q8_0",
                "tiny-q5_1",
                "base",
                "base.en",
                "base-q5_1",
                "small",
                "small.en",
                "small.en-q5_1",
                "small-q5_1",
                "medium",
                "medium.en",
                "medium.en-q5_0",
                "large"
        };
        for (int i = 0; i < asrModels.length; i++) {
            if (name.contains(asrModels[i])) {
                return true;
            }
        }
        return false;
    }

    public static boolean isLLMVModel(String name) {
        String[] llmModels = {
                "gemma-3",
                "Qwen2.5-VL-3B",
        };
        for (int i = 0; i < llmModels.length; i++) {
            if (name.contains(llmModels[i])) {
                return true;
            }
        }
        return false;
    }
}
