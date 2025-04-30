/*
 * Copyright (c) 2024- KanTV Authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <jni.h>

#include "whispercpp/whisper.h"

#include "llamacpp/include/llama.h"

#include "kantv-asr.h"
#include "ggml-jni.h"

#include "llamacpp/ggml/include/ggml-hexagon.h"

#define UNUSED(x)       (void)(x)

JNIEXPORT jstring JNICALL
Java_kantvai_ai_ggmljava_asr_1get_1systeminfo(JNIEnv *env, jclass clazz) {
    UNUSED(env);

    LOGGD("enter getSystemInfo");
    const char *sysinfo = whisper_print_system_info();
    jstring string = (*env)->NewStringUTF(env, sysinfo);
    LOGGD("leave getSystemInfo");

    return string;
}


JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_ggml_1set_1benchmark_1status(JNIEnv *env, jclass clazz,
                                                    jint b_exit_benchmark) {
    UNUSED(env);
    UNUSED(clazz);

    ggml_jni_set_abortbenchmark_flag((int) b_exit_benchmark);
}


JNIEXPORT jstring JNICALL
Java_kantvai_ai_ggmljava_ggml_1bench(JNIEnv *env, jclass clazz, jstring model_path,
                                       jstring user_data, jint bench_type, jint num_threads, jint backend_type, jint accel_type) {
    UNUSED(clazz);

    const char *sz_model_path = NULL;
    const char *sz_user_data = NULL;
    const char *sz_bench_result = "unknown";
    const char *bench_result = NULL;

    sz_model_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_model_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_user_data = (*env)->GetStringUTFChars(env, user_data, NULL);
    if (NULL == sz_user_data) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("user_data:%s\n", sz_user_data);
    LOGGV("bench type: %d\n", bench_type);
    LOGGV("thread counts:%d\n", num_threads);
    LOGGV("backend type:%d\n", backend_type);
    LOGGV("accel type:%d\n", accel_type);

    if (bench_type >= GGML_BENCHMARK_MAX) {
        LOGGW("pls check bench type\n");
        GGML_JNI_NOTIFY("ggml benchmark type %d not supported currently", bench_type);
        goto failure;
    }

    if (backend_type > HEXAGON_BACKEND_GGML) {
        LOGGW("pls check backend type\n");
        goto failure;
    }

    //FIXME: cDSP doesn't works with whisper.cpp at the moment
    if ((GGML_BENCHMARK_ASR == bench_type) && (HEXAGON_BACKEND_CDSP == backend_type)) {
        LOGGW("whisper.cpp through cDSP not supported currently");
        //GGML_JNI_NOTIFY("whisper.cpp through cDSP not supported currently");
        //goto failure;
    }

#if !defined GGML_USE_HEXAGON
    if (HEXAGON_BACKEND_GGML != backend_type) {
        LOGGW("ggml-hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(backend_type));
        GGML_JNI_NOTIFY("ggml-hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(backend_type));
        goto failure;
    }
#endif

    if (0 == num_threads)
        num_threads = 1;

    ggml_jni_bench(sz_model_path, sz_user_data, bench_type, num_threads, backend_type, accel_type);

    if (GGML_BENCHMARK_ASR == bench_type) { // asr
        //just return "asr_result" even get correct asr result because I'll try to do everything in native layer
        sz_bench_result = "asr_result";
    }

failure:
    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_model_path);
    }

    if (NULL != sz_user_data) {
        (*env)->ReleaseStringUTFChars(env, user_data, sz_user_data);
    }

    jstring string = (*env)->NewStringUTF(env, sz_bench_result);
    return string;
}


JNIEXPORT jint JNICALL
Java_kantvai_ai_ggmljava_get_1cpu_1core_1counts(JNIEnv *env, jclass clazz) {
    UNUSED(env);
    UNUSED(clazz);

    return ggml_jni_get_cpu_core_counts();
}


JNIEXPORT jint JNICALL
Java_kantvai_ai_ggmljava_asr_1init(JNIEnv *env, jclass clazz, jstring model_path, jint n_threads, jint n_asrmode, jint n_backend) {
    UNUSED(clazz);

    int result  = 0;
    const char *sz_model_path = NULL;

    sz_model_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_model_path) {
        result = 1;
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("thread counts:%d\n", n_threads);
    LOGGV("asr mode:%d\n", n_asrmode);
    LOGGV("backend type: %d\n", n_backend);

    result = whisper_asr_init(sz_model_path, n_threads, n_asrmode, n_backend);

failure:
    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_model_path);
    }

    return result;
}


JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_asr_1finalize(JNIEnv *env, jclass clazz) {
    UNUSED(env);
    UNUSED(clazz);

    whisper_asr_finalize();
}

JNIEXPORT jint JNICALL
Java_kantvai_ai_ggmljava_asr_1reset(JNIEnv *env, jclass clazz, jstring str_model_path,
                                               jint n_thread_counts, jint n_asrmode, jint n_backend) {
    UNUSED(clazz);

    int result  = 0;
    const char *sz_model_path = NULL;

    sz_model_path = (*env)->GetStringUTFChars(env, str_model_path, NULL);
    if (NULL == sz_model_path) {
        result = 1;
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("thread counts:%d\n", n_thread_counts);
    LOGGV("asr mode:%d\n", n_asrmode);
    LOGGV("backend type: %d\n", n_backend);

    result = whisper_asr_reset(sz_model_path, n_thread_counts, n_asrmode, n_backend);

failure:
    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, str_model_path, sz_model_path);
    }

    return result;
}


JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_asr_1start(JNIEnv *env, jclass clazz) {
    UNUSED(env);
    UNUSED(clazz);

    whisper_asr_start();
}

JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_asr_1stop(JNIEnv *env, jclass clazz) {
    UNUSED(env);
    UNUSED(clazz);

    whisper_asr_stop();
}


JNIEXPORT jstring JNICALL
Java_kantvai_ai_ggmljava_llm_1get_1systeminfo(JNIEnv *env, jclass clazz) {
    UNUSED(env);

    LOGGD("enter getSystemInfo");
    const char *sysinfo = llama_print_system_info();
    jstring string = (*env)->NewStringUTF(env, sysinfo);
    LOGGD("leave getSystemInfo");

    return string;
}


JNIEXPORT jstring  JNICALL
Java_kantvai_ai_ggmljava_llm_1inference(JNIEnv *env, jclass clazz, jstring model_path, jstring prompt,
                                               jint n_llm_type, jint n_thread_counts, jint n_backend, jint n_hwaccel_type) {
    UNUSED(clazz);

    const char *sz_model_path = NULL;
    const char *sz_prompt = NULL;
    const char *sz_bench_result = "unknown";
    const char *bench_result = NULL;
    int result = 0;

    sz_model_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_model_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_prompt = (*env)->GetStringUTFChars(env, prompt, NULL);
    if (NULL == sz_prompt) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("prompt:%s\n", sz_prompt);
    LOGGV("llm type: %d\n", n_llm_type);
    LOGGV("thread counts:%d\n", n_thread_counts);
    LOGGV("backend type:%d\n", n_backend);
    LOGGV("accel type:%d\n", n_hwaccel_type);

#if !defined GGML_USE_HEXAGON
    if (n_backend != HEXAGON_BACKEND_GGML) {
        LOGGW("ggml-hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(n_backend));
        GGML_JNI_NOTIFY("ggml-hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(n_backend));
        goto failure;
    }
#endif

    if (0 == n_thread_counts)
        n_thread_counts = 1;

    result = llama_inference_ng(sz_model_path, sz_prompt, n_llm_type, n_thread_counts, n_backend, n_hwaccel_type);
    LOGGD("result %d", result);
    if (0 != result) {
        if (result != LLM_INFERENCE_INTERRUPTED) {
            GGML_JNI_NOTIFY("LLM inference with backend %d failure", n_backend);
        }
    }

failure:
    if (NULL != sz_prompt) {
        (*env)->ReleaseStringUTFChars(env, prompt, sz_prompt);
    }

    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_model_path);
    }

    jstring string = (*env)->NewStringUTF(env, sz_bench_result);

    return string;
}


void  ggml_jni_notify_c_impl(const char * format,  ...) {
    static unsigned char s_ggml_jni_buf[JNI_BUF_LEN];

    va_list va;
    int len_content = 0;

    memset(s_ggml_jni_buf, 0, JNI_BUF_LEN);

    va_start(va, format);

    len_content = vsnprintf(s_ggml_jni_buf, JNI_BUF_LEN, format, va);
    snprintf(s_ggml_jni_buf + len_content, JNI_BUF_LEN - len_content, "\n");
    va_end(va);

    kantv_asr_notify_benchmark_c(s_ggml_jni_buf);
}

JNIEXPORT void JNICALL
Java_kantvai_ai_ggmljava_llm_1stop_1inference(JNIEnv *env, jclass clazz) {
    llama_reset_running_state();
}

JNIEXPORT jboolean JNICALL
Java_kantvai_ai_ggmljava_llm_1is_1running(JNIEnv *env, jclass clazz) {
    if (1 == llama_is_running_state()) {
        return JNI_TRUE;
    } else {
        return JNI_FALSE;
    }
}

JNIEXPORT jstring JNICALL
Java_kantvai_ai_ggmljava_llava_1inference(JNIEnv *env, jclass clazz, jstring model_path,
                                          jstring mmproj_model_path, jstring img_path,
                                          jstring prompt, jint n_llmtype, jint n_thread_counts,
                                          jint n_backend_type, jint n_hwaccel_type) {
    const char *sz_model_path = NULL;
    const char *sz_mmproj_path = NULL;
    const char *sz_img_path = NULL;
    const char *sz_prompt = NULL;
    const char *sz_bench_result = "unknown";
    const char *bench_result = NULL;
    int result = 0;

    sz_model_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_model_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_mmproj_path = (*env)->GetStringUTFChars(env, mmproj_model_path, NULL);
    if (NULL == sz_mmproj_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_img_path = (*env)->GetStringUTFChars(env, img_path, NULL);
    if (NULL == sz_img_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_prompt = (*env)->GetStringUTFChars(env, prompt, NULL);
    if (NULL == sz_prompt) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("model path:%s\n", sz_model_path);
    LOGGV("mmproj model path:%s\n", sz_mmproj_path);
    LOGGV("img path:%s\n", sz_img_path);
    LOGGV("prompt:%s\n", sz_prompt);
    LOGGV("llm type: %d\n", n_llmtype);
    LOGGV("thread counts:%d\n", n_thread_counts);
    LOGGV("backend type:%d\n", n_backend_type);
    LOGGV("accel type:%d\n", n_hwaccel_type);


#if !defined GGML_USE_HEXAGON
    if (n_backend != HEXAGON_BACKEND_GGML) {
        LOGGW("ggml-hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(n_backend));
        GGML_JNI_NOTIFY("ggml-hexagon backend %s is disabled and only ggml backend is supported\n", ggml_backend_hexagon_get_devname(n_backend));
        goto failure;
    }
#endif

    if (0 == n_thread_counts)
        n_thread_counts = 1;

    result = llava_inference(sz_model_path, sz_mmproj_path, sz_img_path, sz_prompt, n_llmtype, n_thread_counts, n_backend_type, n_hwaccel_type);
    LOGGD("result %d", result);
    if (0 != result) {
        if (result != LLM_INFERENCE_INTERRUPTED) {
            GGML_JNI_NOTIFY("LLM inference with backend %d failure", n_backend_type);
        }
    }

    failure:
    if (NULL != sz_prompt) {
        (*env)->ReleaseStringUTFChars(env, prompt, sz_prompt);
    }

    if (NULL != sz_img_path) {
        (*env)->ReleaseStringUTFChars(env, img_path, sz_img_path);
    }

    if (NULL != sz_mmproj_path) {
        (*env)->ReleaseStringUTFChars(env, mmproj_model_path, sz_mmproj_path);
    }

    if (NULL != sz_model_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_model_path);
    }

    jstring string = (*env)->NewStringUTF(env, sz_bench_result);

    return string;
}