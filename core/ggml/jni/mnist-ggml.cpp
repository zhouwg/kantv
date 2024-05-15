//author:https://github.com/StudyingLover/ggml-tutorial

#include "ggml.h"

#include "common.h"
#include "ggml-jni.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION


struct mnist_model {
    struct ggml_tensor * fc1_weight;
    struct ggml_tensor * fc1_bias;
    struct ggml_tensor * fc2_weight;
    struct ggml_tensor * fc2_bias;
    struct ggml_context * ctx;
};

struct image_u8 {
    int nx;
    int ny;

    std::vector<uint8_t> data;
};


static bool mnist_model_load(const std::string & fname, mnist_model & model) {
    struct gguf_init_params params = {
        /*.no_alloc   =*/ false,
        /*.ctx        =*/ &model.ctx,
    };
    gguf_context * ctx = gguf_init_from_file(fname.c_str(), params);
    if (!ctx) {
        fprintf(stderr, "%s: gguf_init_from_file() failed\n", __func__);
        return false;
    }
    model.fc1_weight = ggml_get_tensor(model.ctx, "fc1_weights");
    model.fc1_bias = ggml_get_tensor(model.ctx, "fc1_bias");
    model.fc2_weight = ggml_get_tensor(model.ctx, "fc2_weights");
    model.fc2_bias = ggml_get_tensor(model.ctx, "fc2_bias");
    return true;
}

static int mnist_eval(
        const mnist_model & model,
        const int n_threads,
        std::vector<float> digit,
        const char * fname_cgraph
        )
{
    static size_t buf_size = 100000 * sizeof(float) * 4;
    static void * buf = malloc(buf_size);

    struct ggml_init_params params = {
        /*.mem_size   =*/ buf_size,
        /*.mem_buffer =*/ buf,
        /*.no_alloc   =*/ false,
    };

    struct ggml_context * ctx0 = ggml_init(params);
    struct ggml_cgraph * gf = ggml_new_graph(ctx0);

    struct ggml_tensor * input = ggml_new_tensor_4d(ctx0, GGML_TYPE_F32, 28, 28, 1, 1);
    memcpy(input->data, digit.data(), ggml_nbytes(input));
    ggml_set_name(input, "input");
    ggml_tensor * cur = ggml_reshape_2d(ctx0, input, 784, 1);
    cur = ggml_mul_mat(ctx0, model.fc1_weight, cur);
    cur = ggml_add(ctx0, cur, model.fc1_bias);
    cur = ggml_relu(ctx0, cur);
    cur = ggml_mul_mat(ctx0, model.fc2_weight, cur);
    cur = ggml_add(ctx0, cur, model.fc2_bias);

    ggml_tensor * result = cur;
    ggml_set_name(result, "result");
    
    ggml_build_forward_expand(gf, result);
    ggml_graph_compute_with_ctx(ctx0, gf, n_threads);

    //ggml_graph_print(&gf);
    //ggml_graph_dump_dot(gf, NULL, "mnist-cnn.dot");

    if (fname_cgraph) {
        // export the compute graph for later use
        // see the "mnist-cpu" example
        ggml_graph_export(gf, fname_cgraph);

        LOGGD( "%s: exported compute graph to '%s'\n", __func__, fname_cgraph);
    }

    const float * probs_data = ggml_get_data_f32(result);
    const int prediction = std::max_element(probs_data, probs_data + 10) - probs_data;
    ggml_free(ctx0);
    return prediction;
    // return 1;
}



static bool image_load_from_file(const std::string & fname, image_u8 & img) {
    int nx, ny, nc;
    auto data = stbi_load(fname.c_str(), &nx, &ny, &nc, 3);
    if (!data) {
        LOGGD("%s: failed to load '%s'\n", __func__, fname.c_str());
        return false;
    }

    img.nx = nx;
    img.ny = ny;
    img.data.resize(nx * ny * 3);
    memcpy(img.data.data(), data, nx * ny * 3);

    stbi_image_free(data);

    return true;
}

int mnist_ggml(int argc, char ** argv) {
    srand(time(NULL));
    ggml_time_init();

    if (argc != 3) {
        return 1;
    }

    mnist_model model;
    std::vector<float> digit;

    // load the model
    {
        const int64_t t_start_us = ggml_time_us();
        if (!mnist_model_load(argv[1], model)) {
            LOGGE("%s: failed to load model from '%s'\n", __func__, argv[1]);
            return 1;
        }
        

        const int64_t t_load_us = ggml_time_us() - t_start_us;

        LOGGI("%s: loaded model in %8.2f ms\n", __func__, t_load_us / 1000.0f);
    }

    // read a img from a file
    image_u8 img0;
    std::string img_path = argv[2];
    if (!image_load_from_file(img_path, img0)) {
        LOGGI( "%s: failed to load image from '%s'\n", __func__, img_path.c_str());
        return 1;
    }
    LOGGI("%s: loaded image '%s' (%d x %d)\n", __func__, img_path.c_str(), img0.nx, img0.ny);
    

    uint8_t buf[784];

    // convert the image to a digit
    
    const int64_t t_start_us = ggml_time_us();

    for (int i = 0; i < 784; i++) {
        buf[i] = 255 - img0.data[i * 3];
    }

    for (int i = 0; i < 784; i++) {
        digit.push_back(buf[i] / 255.0f);
    }

    const int64_t t_convert_us = ggml_time_us() - t_start_us;

    LOGGI( "%s: converted image to digit in %8.2f ms\n", __func__, t_convert_us / 1000.0f);

    const int prediction = mnist_eval(model, 1, digit, nullptr);
    LOGGI("%s: predicted digit is %d\n", __func__, prediction);
    GGML_JNI_NOTIFY("%s: predicted digit is %d\n", __func__, prediction);
    ggml_free(model.ctx);
    return 0;
}