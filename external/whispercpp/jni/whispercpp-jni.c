//TODO: re-write entire whispercpp-jni.c with standard Android JNI specification
// interaction between kantv-core and whispercpp-jni

#include <jni.h>

#include "whisper.h"
#include "ggml.h"

#include "cde_log.h"
#include "kantv_asr.h"

#define UNUSED(x) (void)(x)

JNIEXPORT jstring JNICALL
Java_org_ggml_whispercpp_whispercpp_getSystemInfo(
        JNIEnv *env, jobject thiz
) {
    UNUSED(thiz);
    LOGGD("enter getSystemInfo");
    const char *sysinfo = whisper_print_system_info();
    jstring string = (*env)->NewStringUTF(env, sysinfo);
    LOGGD("leave getSystemInfo");

    return string;
}

JNIEXPORT jstring JNICALL
Java_org_ggml_whispercpp_whispercpp_benchMemcpy(JNIEnv *env, jobject thiz, jint n_threads) {
    UNUSED(thiz);
    ENTER_JNI_FUNC();
    kantv_asr_notify_c("prepare ggml memcpy benchmark");
    const char *bench_ggml_memcpy = whisper_bench_memcpy_str(n_threads);
    LOGGI("bench_ggml_memcpy:\n%s\n", bench_ggml_memcpy);
    jstring string = (*env)->NewStringUTF(env, bench_ggml_memcpy);
    LEAVE_JNI_FUNC();

    return string;
}

JNIEXPORT jstring JNICALL
Java_org_ggml_whispercpp_whispercpp_benchMulMat(JNIEnv *env, jobject thiz, jint n_threads) {
    UNUSED(thiz);

    kantv_asr_notify_c("prepare ggml mulmat benchmark");
    const char *bench_ggml_mul_mat = whisper_bench_ggml_mul_mat_str(n_threads);
    jstring string = (*env)->NewStringUTF(env, bench_ggml_mul_mat);
    LOGGI("bench_ggml_mulmat:\n%s\n", bench_ggml_mul_mat);

    return string;
}


JNIEXPORT void JNICALL
Java_org_ggml_whispercpp_whispercpp_set_1mulmat_1benchmark_1status(JNIEnv *env, jclass clazz, jint b_exit_benchmark) {
    UNUSED(env);
    UNUSED(clazz);

    whisper_set_ggml_mul_mat_status((int)b_exit_benchmark);
}



JNIEXPORT jstring JNICALL
Java_org_ggml_whispercpp_whispercpp_transcribe_1from_1file(JNIEnv *env, jclass clazz,
                                           jstring model_path, jstring audio_path, jint num_threads) {
    UNUSED(clazz);

    const char *sz_mode_path  = NULL;
    const char *sz_audio_path = NULL;
    const char *sz_asr_result = "unknown";

    sz_mode_path = (*env)->GetStringUTFChars(env, model_path, NULL);
    if (NULL == sz_mode_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    sz_audio_path = (*env)->GetStringUTFChars(env, audio_path, NULL);
    if (NULL == sz_audio_path) {
        LOGGW("JNI failure, pls check why?");
        goto failure;
    }

    LOGGV("mode path:%s\n", sz_mode_path);
    LOGGV("audio file:%s\n", sz_audio_path);

    kantv_asr_notify_c("calling transcribe");
    const char *asr_result = whisper_transcribe_from_file(sz_mode_path, sz_audio_path, num_threads);
    if (NULL == asr_result) {
        LOGGW("failed to get asr result, pls check why?");
        goto failure;
    }
    sz_asr_result = asr_result;
    kantv_asr_notify_c("after calling transcribe");
    LOGGD("asr result:\n%s\n", sz_asr_result);

failure:
    if (NULL != sz_mode_path) {
        (*env)->ReleaseStringUTFChars(env, model_path, sz_mode_path);
    }

    if (NULL != sz_audio_path) {
        (*env)->ReleaseStringUTFChars(env, audio_path, sz_audio_path);
    }

    jstring string = (*env)->NewStringUTF(env, sz_asr_result);
    return string;
}