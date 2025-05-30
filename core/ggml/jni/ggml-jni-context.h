#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <set>
#include <tuple>
#include <queue>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <memory>
#include <regex>
#include <random>
#include <functional>
#include <unordered_map>
#include <condition_variable>
#include <cassert>
#include <unordered_set>
#include <utility>

//ggml-jni
#include "ggml-jni.h"

class ggml_jni_context {
public:
    static ggml_jni_context & get_instance() {
        static ggml_jni_context instance;
        return instance;
    }

    void    init();

    void    finalize();

    void    set_top_p(float value) { llm_temperature  = value; }
    float   get_top_p()  { return llm_top_p; }
    void    set_temperature(float value) { llm_temperature = value; }
    float   get_temperature() { return llm_temperature; }

    void llm_init_running_state() {
        llm_inference_is_running.store(1);
    }

    void llm_reset_running_state() {
        llm_inference_is_running.store(0);
    }

    int llm_is_running_state() {
        return llm_inference_is_running.load();
    }

    void realtimemtmd_init_running_state() {
        realtimemtmd_inference_is_running.store(1);
    }

    void realtimemtmd_reset_running_state() {
        realtimemtmd_inference_is_running.store(0);
    }

    int realtimemtmd_is_running_state() {
        return realtimemtmd_inference_is_running.load();
    }

    void sd_init_running_state() {
        sd_inference_is_running.store(1);
    }

    void sd_reset_running_state() {
        sd_inference_is_running.store(0);
    }

    int sd_is_running_state() {
        return sd_inference_is_running.load();
    }

private:
    ggml_jni_context();

    ggml_jni_context(const ggml_jni_context &)              = delete;
    ggml_jni_context(const ggml_jni_context &&)             = delete;
    ggml_jni_context & operator= (const ggml_jni_context &) = delete;

    ~ggml_jni_context();

private:
    float llm_temperature;
    float llm_top_p;
    //TODO:add other LLM parameters

    std::atomic<uint32_t> llm_inference_is_running;
    std::atomic<uint32_t> realtimemtmd_inference_is_running;
    std::atomic<uint32_t> sd_inference_is_running;

    bool initialized;
};