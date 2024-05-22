/*
 * the following codes just for:
 *
 * study internal mechanism of NCNN(Nihui Convolutional Neural Network, https://github.com/Tencent/ncnn)
 *
 * study various open source pure C/C++ AI projects based on NCNN(such as https://github.com/k2-fsa/sherpa-ncnn)
 *
 * preparation for implementation of PoC https://github.com/zhouwg/kantv/issues/176
 *
 */

// ref:https://github.com/Tencent/ncnn
//     https://github.com/nihui/opencv-mobile
//     https://github.com/nihui/ncnn_on_esp32
//     https://github.com/nihui/ncnn-android-squeezenet
//     https://github.com/yaoyi30/ResNet_ncnn_android
//     https://github.com/k2-fsa/sherpa-ncnn
//     https://github.com/WongKinYiu/yolov9
//     ...
//
// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2021 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.


/*
 * Copyright (c) 2024- KanTV Authors
 *
 * JNI implementation of ncnn-jni for Project KanTV
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <android/bitmap.h>
#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>
#include <cmath>

//ncnn
#include "platform.h"
#include "benchmark.h"
#include "net.h"

//opencv-android
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//ncnn-jni
#include "ncnn-jni.h"

#include "ndkcamera.h"
#include "scrfd.h"
#include "res.id.h"
#include "squeezenet_v1.1.id.h"

//for mnist inference using ncnn
#include "res.img.h"            //mnist digit image -> array
#include "mnist.mem.h"          //ncnn model
#include "mnist-int8.mem.h"     //ncnn model

#if __ARM_NEON

#include <arm_neon.h>

#endif // __ARM_NEON

static int draw_unsupported(cv::Mat &rgb) {
    const char text[] = "unsupported";

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 1.0, 1, &baseLine);

    int y = (rgb.rows - label_size.height) / 2;
    int x = (rgb.cols - label_size.width) / 2;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0));

    return 0;
}

static int draw_fps(cv::Mat &rgb) {
    // resolve moving average
    float avg_fps = 0.f;
    {
        static double t0 = 0.f;
        static float fps_history[10] = {0.f};

        double t1 = ncnn::get_current_time();
        if (t0 == 0.f) {
            t0 = t1;
            return 0;
        }

        float fps = 1000.f / (t1 - t0);
        t0 = t1;

        for (int i = 9; i >= 1; i--) {
            fps_history[i] = fps_history[i - 1];
        }
        fps_history[0] = fps;

        if (fps_history[9] == 0.f) {
            return 0;
        }

        for (int i = 0; i < 10; i++) {
            avg_fps += fps_history[i];
        }
        avg_fps /= 10.f;
    }

    char text[32];
    sprintf(text, "FPS=%.2f", avg_fps);

    int baseLine = 0;
    cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    int y = 0;
    int x = rgb.cols - label_size.width;

    cv::rectangle(rgb, cv::Rect(cv::Point(x, y),
                                cv::Size(label_size.width, label_size.height + baseLine)),
                  cv::Scalar(255, 255, 255), -1);

    cv::putText(rgb, text, cv::Point(x, y + label_size.height),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));

    return 0;
}

static SCRFD *g_scrfd = NULL;
static ncnn::Mutex lock;

static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
static ncnn::PoolAllocator g_workspace_pool_allocator;
static ncnn::Net resnet;


static std::vector<std::string> squeezenet_words;
static ncnn::Net squeezenet;
static ncnn::Net squeezenet_gpu;

static ncnn::Net mnist;

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat &rgb) const {
    {
        ncnn::MutexLockGuard g(lock);

        if (g_scrfd) {
            std::vector<FaceObject> faceobjects;
            g_scrfd->detect(rgb, faceobjects);

            g_scrfd->draw(rgb, faceobjects);
        } else {
            draw_unsupported(rgb);
        }
    }

    draw_fps(rgb);
}


static std::vector<std::string> split_string(const std::string &str, const std::string &delimiter) {
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + 1;
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}


static MyNdkCamera *g_camera = NULL;

extern "C" {

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    LOGGD("ncnn JNI_OnLoad");

    ncnn::create_gpu_instance();

    g_camera = new MyNdkCamera;


    return JNI_VERSION_1_4;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOGGD("ncnn JNI_OnUnload");

    {
        ncnn::MutexLockGuard g(lock);

        delete g_scrfd;
        g_scrfd = NULL;
    }

    ncnn::destroy_gpu_instance();

    delete g_camera;
    g_camera = NULL;
}


/**
 *
 * @param env
 * @param thiz
 * @param assetManager
 * @param netid
 * @param modelid
 * @param backend_type 0: NCNN_BACKEND_CPU, 1: NCNN_BACKEND_GPU
 * @return
 */
JNIEXPORT jboolean JNICALL
Java_org_ncnn_ncnnjava_loadModel(JNIEnv *env, jobject thiz, jobject assetManager, jint netid,
                                 jint modelid, jint backend_type) {
    LOGGD("netid %d", netid);
    LOGGD("modelid %d", modelid);
    LOGGD("backend_type %d", backend_type);

    AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    LOGGD("ncnn load model with AAssetManager %p", mgr);

    if (NCNN_FACEDETECT ==
        netid) { // reserved for multimodal poc(CV, NLP, LLM, TTS... with live camera)
        if (modelid < 0 || modelid > 6 || backend_type > NCNN_BACKEND_MAX) {
            LOGGW("invalid params");
            return JNI_FALSE;
        }

        const char *modeltypes[] =
                {
                        "500m",
                        "500m_kps",
                        "1g",
                        "2.5g",
                        "2.5g_kps",
                        "10g",
                        "10g_kps"
                };

        const char *modeltype = modeltypes[(int) modelid];
        bool use_gpu = (backend_type == NCNN_BACKEND_GPU);

        // reload
        {
            ncnn::MutexLockGuard g(lock);

            if (use_gpu && ncnn::get_gpu_count() == 0) {
                LOGGW("ncnn gpu backend not supported");
                NCNN_JNI_NOTIFY("ncnn gpu backend not supported");
                // no gpu
                delete g_scrfd;
                g_scrfd = 0;
            } else {
                if (!g_scrfd)
                    g_scrfd = new SCRFD;
                g_scrfd->load(mgr, modeltype, use_gpu);
            }
        }
    } else if (NCNN_RESNET == netid) { //ResNet
        ncnn::Option opt;
        opt.lightmode = true;
        opt.num_threads = 4;
        opt.blob_allocator = &g_blob_pool_allocator;
        opt.workspace_allocator = &g_workspace_pool_allocator;

        // use vulkan compute
        if (ncnn::get_gpu_count() != 0)
            opt.use_vulkan_compute = true;

        resnet.opt = opt;

        // init param
        {
            int ret = resnet.load_param_bin(mgr, "resnet.param.bin");
            if (ret != 0) {
                LOGGW("ResNet Ncnn: load_param_bin failed");
                return JNI_FALSE;
            }
        }

        // init bin
        {
            int ret = resnet.load_model(mgr, "resnet.bin");
            if (ret != 0) {
                LOGGW("ResNet Ncnn:load_model failed");
                return JNI_FALSE;
            }
        }

    } else if (NCNN_SQUEEZENET == netid) { //SqueezeNet
        // init param
        {
            int ret = squeezenet.load_param_bin(mgr, "squeezenet_v1.1.param.bin");
            if (ret != 0) {
                LOGGW("SqueezeNet Ncnn:load_param_bin failed");
                return JNI_FALSE;
            }
        }

        // init bin
        {
            int ret = squeezenet.load_model(mgr, "squeezenet_v1.1.bin");
            if (ret != 0) {
                LOGGW("SqueezeNet Ncnn:load_model failed");
                return JNI_FALSE;
            }
        }

        // use vulkan compute
        if (ncnn::get_gpu_count() != 0) {
            LOGGD("using ncnn gpu backend");
            NCNN_JNI_NOTIFY("using ncnn gpu backend");
            squeezenet_gpu.opt.use_vulkan_compute = true;

            {
                int ret = squeezenet_gpu.load_param_bin(mgr, "squeezenet_v1.1.param.bin");
                if (ret != 0) {
                    LOGGW("SqueezeNet Ncnn: load_param_bin failed");
                    return JNI_FALSE;
                }
            }
            {
                int ret = squeezenet_gpu.load_model(mgr, "squeezenet_v1.1.bin");
                if (ret != 0) {
                    LOGGW("SqueezeNet Ncnn:load_model failed");
                    return JNI_FALSE;
                }
            }
        } else {
            LOGGD("ncnn gpu backend not supported");
            NCNN_JNI_NOTIFY("using ncnn gpu backend not supported");
        }

        // init words
        {
            AAsset *asset = AAssetManager_open(mgr, "synset_words.txt", AASSET_MODE_BUFFER);
            if (!asset) {
                LOGGW("SqueezeNet Ncnn:open synset_words.txt failed");
                return JNI_FALSE;
            }

            int len = AAsset_getLength(asset);

            std::string words_buffer;
            words_buffer.resize(len);
            int ret = AAsset_read(asset, (void *) words_buffer.data(), len);

            AAsset_close(asset);

            if (ret != len) {
                LOGGW("SqueezeNet Ncnn:read synset_words.txt failed");
                return JNI_FALSE;
            }

            squeezenet_words = split_string(words_buffer, "\n");
        }
    } else if (netid == NCNN_MNIST) {
        LOGGD("load ncnn mnist model");
        mnist.opt.use_local_pool_allocator = false;
        mnist.opt.use_sgemm_convolution = false;

        //05-22-2024,all cases(1 + 2*2) works fine/perfectly as expected on Xiaomi 14
        if (0) {
            //load ncnn model from memory
            if (0) {
                mnist.load_param(mnist_param_bin);
                mnist.load_model(mnist_bin);
            } else {
                mnist.load_param(mnist_int8_param_bin);
                mnist.load_model(mnist_int8_bin);
            }
        } else {
            //load ncnn model from file
            if (0) {
                // init param
                //int ret = mnist.load_param(mgr, "mnist.param");
                int ret = mnist.load_param_bin(mgr, "mnist.param.bin");
                if (ret != 0) {
                    LOGGW("mnist Ncnn: load_param failed");
                    NCNN_JNI_NOTIFY("mnist Ncnn: load_param failed");
                    return JNI_FALSE;
                }
                // init bin
                ret = mnist.load_model(mgr, "mnist.bin");
                if (ret != 0) {
                    LOGGW("mnist Ncnn:load_model failed");
                    NCNN_JNI_NOTIFY("mnist Ncnn:load_model failed");
                    return JNI_FALSE;
                }
            } else {
                // init param
                int ret = mnist.load_param(mgr, "mnist-int8.param");
                //int ret = mnist.load_param_bin(mgr, "mnist-int8.param.bin");
                if (ret != 0) {
                    LOGGW("mnist Ncnn: load_param failed");
                    NCNN_JNI_NOTIFY("mnist Ncnn: load_param failed");
                    return JNI_FALSE;
                }
                // init bin
                ret = mnist.load_model(mgr, "mnist-int8.bin");
                if (ret != 0) {
                    LOGGW("mnist Ncnn:load_model failed");
                    NCNN_JNI_NOTIFY("mnist Ncnn:load_model failed");
                    return JNI_FALSE;
                }
            }
        }

    } else {
        LOGGW("netid %d not supported using ncnn", netid);
        NCNN_JNI_NOTIFY("netid %d not supported using ncnn", netid);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

// public native boolean openCamera(int facing);
JNIEXPORT jboolean JNICALL
Java_org_ncnn_ncnnjava_openCamera(JNIEnv *env, jobject thiz, jint facing) {
    if (facing < 0 || facing > 1)
        return JNI_FALSE;

    LOGGD("ncnn openCamera %d", facing);

    g_camera->open((int) facing);

    return JNI_TRUE;
}

// public native boolean closeCamera();
JNIEXPORT jboolean JNICALL Java_org_ncnn_ncnnjava_closeCamera(JNIEnv *env, jobject thiz) {
    LOGGD("ncnn closeCamera");

    g_camera->close();

    return JNI_TRUE;
}

// public native boolean setOutputWindow(Surface surface);
JNIEXPORT jboolean JNICALL
Java_org_ncnn_ncnnjava_setOutputWindow(JNIEnv *env, jobject thiz, jobject surface) {
    ANativeWindow *win = ANativeWindow_fromSurface(env, surface);

    LOGGD("ncnn setOutputWindow %p", win);

    g_camera->set_window(win);

    return JNI_TRUE;
}


JNIEXPORT jstring JNICALL
Java_org_ncnn_ncnnjava_detectResNet(JNIEnv *env, jobject thiz, jobject bitmap, jboolean use_gpu) {
    if (use_gpu == JNI_TRUE && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu not supported");
        NCNN_JNI_NOTIFY("gpu not supported");
        return env->NewStringUTF("no vulkan capable gpu");
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGGW("bitmap is not RGBA_8888");
        NCNN_JNI_NOTIFY("bitmap format is not RGBA_8888");
        return NULL;
    }

    ncnn::Mat in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_RGB, 224,
                                                         224);
    std::vector<float> cls_scores;
    {
        const float mean_vals[3] = {123.675f, 116.28f, 103.53f};
        const float std_vals[3] = {1 / 58.395f, 1 / 57.12f, 1 / 57.375f};

        in.substract_mean_normalize(mean_vals, std_vals);

        ncnn::Extractor ex = resnet.create_extractor();

        ex.set_vulkan_compute(use_gpu);

        ex.input(res_param_id::LAYER_input, in);

        ncnn::Mat out;
        ex.extract(res_param_id::BLOB_output, out);


        cls_scores.resize(out.w);
        for (int j = 0; j < out.w; j++) {
            cls_scores[j] = out[j];
        }
    }

    static const char *class_names[] = {
            "n0", "n1", "n2", "n3", "n4", "n5", "n6", "n7", "n8", "n9"
    };

    float sum = 0.0f;
    for (int j = 0; j < cls_scores.size(); j++) {
        float s_l = cls_scores[j];
        cls_scores[j] = std::exp(s_l);
        //scores[i] = std::exp(scores[i]);
        sum += cls_scores[j];
    }

    for (int a = 0; a < cls_scores.size(); a++) {
        cls_scores[a] /= sum;
    }

    int size = cls_scores.size();
    std::vector<std::pair<float, std::string>> vec;
    vec.resize(size);
    for (int i = 0; i < size; i++) {
        vec[i] = std::make_pair(cls_scores[i], class_names[i]);
    }

    std::sort(vec.begin(), vec.end(), std::greater<std::pair<float, std::string>>());

    char tmp[32];
    snprintf(tmp, 32, "%.1f", vec[0].first * 100);

    double elasped = ncnn::get_current_time() - start_time;

    char time[32];
    snprintf(time, 32, "%.1fms", elasped);

    std::string result_str =
            "classification:" + vec[0].second + "  " + "probability:" + tmp + "%" + "  " +
            "elapse time:" + time;

    LOGGD("ncnn resnet inference result:%s", result_str.c_str());
    NCNN_JNI_NOTIFY("ncnn resnet inference result:%s", result_str.c_str());

    jstring result = env->NewStringUTF(result_str.c_str());

    return result;
}


JNIEXPORT jstring JNICALL
Java_org_ncnn_ncnnjava_detectSqueezeNet(JNIEnv *env, jobject thiz, jobject bitmap,
                                        jboolean use_gpu) {
    if (use_gpu == JNI_TRUE && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu not supported");
        NCNN_JNI_NOTIFY("gpu not supported");
        return env->NewStringUTF("no vulkan capable gpu");
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    int width = info.width;
    int height = info.height;

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGGW("bitmap format is not RGBA_8888");
        NCNN_JNI_NOTIFY("bitmap format is not RGBA_8888");
        return NULL;
    }
    ncnn::Mat in;
    if (width != 227 || height != 227) {
        LOGGW("width and height is not 227");
        NCNN_JNI_NOTIFY("width and height is not 227");
        in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_BGR, 227,227);
        //return NULL;
    } else {
        in = ncnn::Mat::from_android_bitmap(env, bitmap, ncnn::Mat::PIXEL_BGR);
    }

    std::vector<float> cls_scores;
    {
        const float mean_vals[3] = {104.f, 117.f, 123.f};
        in.substract_mean_normalize(mean_vals, 0);

        ncnn::Extractor ex = use_gpu ? squeezenet_gpu.create_extractor()
                                     : squeezenet.create_extractor();

        ex.input(squeezenet_v1_1_param_id::BLOB_data, in);

        ncnn::Mat out;
        ex.extract(squeezenet_v1_1_param_id::BLOB_prob, out);

        cls_scores.resize(out.w);
        for (int j = 0; j < out.w; j++) {
            cls_scores[j] = out[j];
        }
    }

    // return top class
    int top_class = 0;
    float max_score = 0.f;
    for (size_t i = 0; i < cls_scores.size(); i++) {
        float s = cls_scores[i];
        //LOGGD("SqueezeNcnn %d %f", i, s);
        if (s > max_score) {
            top_class = i;
            max_score = s;
        }
    }

    const std::string &word = squeezenet_words[top_class];
    char tmp[32];
    snprintf(tmp, 32, "%.3f", max_score);
    LOGGD("max_score %s", tmp);
    if (is_zero_floatvalue(max_score)) {
        LOGGD("ncnn squeezenet inference failure");
        NCNN_JNI_NOTIFY("ncnn squeezenet inference failure");
        return env->NewStringUTF("ncnn squeezenet inference failure");
    }
    // +10 to skip leading n03179701
    std::string result_str = std::string(word.c_str() + 10) + ", probability= " + tmp;
    jstring result = env->NewStringUTF(result_str.c_str());

    double elapse = ncnn::get_current_time() - start_time;
    LOGGD("ncnn squeezenet inference result:%s, elapse %.2f ms", result_str.c_str(), elapse);
    NCNN_JNI_NOTIFY("ncnn squeezenet inference result:%s, elapse %.2f ms", result_str.c_str(),
                    elapse);

    return result;
}


JNIEXPORT jstring JNICALL
Java_org_ncnn_ncnnjava_detectMnist(JNIEnv *env, jobject thiz, jobject bitmap, jboolean use_gpu) {
    if (use_gpu == JNI_TRUE && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu not supported");
        NCNN_JNI_NOTIFY("gpu not supported");
        return env->NewStringUTF("no vulkan capable gpu");
    }

    LOGGD("Preparing input...");
    LOGGD("bitmap is %p", bitmap);
    ncnn::Mat in;
    if (1) {
        //load image from bitmap which loaded from /sdcard/kantv/(mnist-4.png,mnist-5.png,mnist-7.png,minist-8.png, mnist-9.png...)
        //works fine as expected with /sdcard/kantv/(mnist-4.png,mnist-5.png,mnist-7.png,minist-8.png, mnist-9.png...)
        AndroidBitmapInfo info;
        AndroidBitmap_getInfo(env, bitmap, &info);
        int width = info.width;
        int height = info.height;
        LOGGD("width %d, height %d", width, height);
        if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
            LOGGW("bitmap format is not RGBA_8888");
            NCNN_JNI_NOTIFY("bitmap format is not RGBA_8888");
            return NULL;
        }

        if (width != 28 || height != 28) {
            LOGGW("width and height are not 28");
            NCNN_JNI_NOTIFY("width and height of input image are not 28, scale it to 28x28");
            //return NULL;
            in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_GRAY, 28,28);
        } else {
            in = ncnn::Mat::from_android_bitmap(env, bitmap, ncnn::Mat::PIXEL_GRAY);
        }
    } else {
        //load image from memory(internal array IN_IMG)
        in = ncnn::Mat::from_pixels(IN_IMG, ncnn::Mat::PIXEL_GRAY, IMAGE_W, IMAGE_H);
    }
    ncnn::Mat out;

    const float mean_vals[1] = {0.f};
    const float norm_vals[1] = {1 / 255.f};
    in.substract_mean_normalize(mean_vals, norm_vals);

    LOGGD("Start mnist inference using ncnn\n");

    double total_latency = 0;
    float max = -1;
    float min = 10000000000000000;

    for (int i = 0; i < 10; i++) {
        double start = ncnn::get_current_time();

        ncnn::Extractor ex = mnist.create_extractor();
        ex.input(0, in);
        ex.extract(5, out);

        double end = ncnn::get_current_time();

        float lat = (end - start) / 1000.0;
        total_latency += lat;
        if (lat > max) {
            max = lat;
        }
        if (lat < min) {
            min = lat;
        }
    }

    LOGGD("mnist inference done\n");

    const float *ptr = out;
    int gussed = -1;
    float guss_exp = -10000000;
    for (int i = 0; i < 10; i++) {
        LOGGD("%d: %.2f\n", i, ptr[i]);
        if (guss_exp < ptr[i]) {
            gussed = i;
            guss_exp = ptr[i];
        }
    }
    std::string result_str = "predicted digit is: " + std::to_string(gussed);
    LOGGD("predicted digit is: %d\n", gussed);
    NCNN_JNI_NOTIFY("mnist inference using ncnn: predicted digit is %d\n", gussed);

    LOGGD("Latency, avg: %.2fms, max: %.2f, min: %.2f. Avg Flops: %.2fMFlops\n",
          total_latency / 10.0, max, min, 0.78 / (total_latency / 10.0 / 1000.0));
    NCNN_JNI_NOTIFY("Latency, avg: %.2fms, max: %.2f, min: %.2f. Avg Flops: %.2fMFlops\n",
                    total_latency / 10.0, max, min, 0.78 / (total_latency / 10.0 / 1000.0));

    jstring result = env->NewStringUTF(result_str.c_str());
    return result;
}

}


