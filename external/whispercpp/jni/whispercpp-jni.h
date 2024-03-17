#ifndef WHISPER_JNI_H
#define WHISPER_JNI_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#ifdef __cplusplus
extern "C" {
#endif

    // =================================================================================================
    //
    // the following is for PoC(https://github.com/cdeos/kantv/issues/64) in project KanTV
    //
    // =================================================================================================
    // JNI helper function for benchmark
    int          whisper_get_cpu_core_counts(void);
    void         whisper_set_benchmark_status(int b_exit_benchmark);
    void         whisper_bench(const char *model_path, const char *audio_path, int bench_type, int num_threads);

    // JNI helper function for ASR
    void         whisper_asr_init(const char *model_path, int num_threads, int n_devmode);
    void         whisper_asr_finalize(void);


#ifdef __cplusplus
}
#endif


#endif
