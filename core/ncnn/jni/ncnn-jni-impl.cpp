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
//     https://github.com/nihui/ncnn-android-nanodet
//     https://github.com/nihui/ncnn-android-yolov5
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
#include <iostream>
#include <fstream>
#include <cmath>

//ncnn
#include "platform.h"
#include "benchmark.h"
#include "net.h"
#include "gpu.h"

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
#include "nanodet.h"

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
static NanoDet *g_nanodet = NULL;
static ncnn::Mutex lock;

static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
static ncnn::PoolAllocator g_workspace_pool_allocator;
static std::vector<std::string> squeezenet_words;
static ncnn::Net resnet;
static ncnn::Net squeezenet;
static ncnn::Net squeezenet_gpu;
static ncnn::Net mnist;
static ncnn::Net yolov5;


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
        } else if (g_nanodet) {
            std::vector<NanoObject> objects;
            g_nanodet->detect(rgb, objects);

            g_nanodet->draw(rgb, objects);
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

class YoloV5Focus : public ncnn::Layer {
public:
    YoloV5Focus() {
        one_blob_only = true;
    }

    virtual int
    forward(const ncnn::Mat &bottom_blob, ncnn::Mat &top_blob, const ncnn::Option &opt) const {
        int w = bottom_blob.w;
        int h = bottom_blob.h;
        int channels = bottom_blob.c;

        int outw = w / 2;
        int outh = h / 2;
        int outc = channels * 4;

        top_blob.create(outw, outh, outc, 4u, 1, opt.blob_allocator);
        if (top_blob.empty())
            return -100;

#pragma omp parallel for num_threads(opt.num_threads)
        for (int p = 0; p < outc; p++) {
            const float *ptr = bottom_blob.channel(p % channels).row((p / channels) % 2) +
                               ((p / channels) / 2);
            float *outptr = top_blob.channel(p);

            for (int i = 0; i < outh; i++) {
                for (int j = 0; j < outw; j++) {
                    *outptr = *ptr;

                    outptr += 1;
                    ptr += 2;
                }

                ptr += w;
            }
        }

        return 0;
    }
};

DEFINE_LAYER_CREATOR(YoloV5Focus)

struct YoloV5Object {
    float x;
    float y;
    float w;
    float h;
    int label;
    float prob;
};

static inline float intersection_area(const YoloV5Object &a, const YoloV5Object &b) {
    if (a.x > b.x + b.w || a.x + a.w < b.x || a.y > b.y + b.h || a.y + a.h < b.y) {
        // no intersection
        return 0.f;
    }

    float inter_width = std::min(a.x + a.w, b.x + b.w) - std::max(a.x, b.x);
    float inter_height = std::min(a.y + a.h, b.y + b.h) - std::max(a.y, b.y);

    return inter_width * inter_height;
}

static void qsort_descent_inplace(std::vector<YoloV5Object> &faceobjects, int left, int right) {
    int i = left;
    int j = right;
    float p = faceobjects[(left + right) / 2].prob;

    while (i <= j) {
        while (faceobjects[i].prob > p)
            i++;

        while (faceobjects[j].prob < p)
            j--;

        if (i <= j) {
            // swap
            std::swap(faceobjects[i], faceobjects[j]);

            i++;
            j--;
        }
    }

#pragma omp parallel sections
    {
#pragma omp section
        {
            if (left < j) qsort_descent_inplace(faceobjects, left, j);
        }
#pragma omp section
        {
            if (i < right) qsort_descent_inplace(faceobjects, i, right);
        }
    }
}

static void qsort_descent_inplace(std::vector<YoloV5Object> &faceobjects) {
    if (faceobjects.empty())
        return;

    qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
}

static void
nms_sorted_bboxes(const std::vector<YoloV5Object> &faceobjects, std::vector<int> &picked,
                  float nms_threshold) {
    picked.clear();

    const int n = faceobjects.size();

    std::vector<float> areas(n);
    for (int i = 0; i < n; i++) {
        areas[i] = faceobjects[i].w * faceobjects[i].h;
    }

    for (int i = 0; i < n; i++) {
        const YoloV5Object &a = faceobjects[i];

        int keep = 1;
        for (int j = 0; j < (int) picked.size(); j++) {
            const YoloV5Object &b = faceobjects[picked[j]];

            // intersection over union
            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            // float IoU = inter_area / union_area
            if (inter_area / union_area > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}

static inline float sigmoid(float x) {
    return static_cast<float>(1.f / (1.f + exp(-x)));
}

static void generate_proposals(const ncnn::Mat &anchors, int stride, const ncnn::Mat &in_pad,
                               const ncnn::Mat &feat_blob, float prob_threshold,
                               std::vector<YoloV5Object> &objects) {
    const int num_grid = feat_blob.h;

    int num_grid_x;
    int num_grid_y;
    if (in_pad.w > in_pad.h) {
        num_grid_x = in_pad.w / stride;
        num_grid_y = num_grid / num_grid_x;
    } else {
        num_grid_y = in_pad.h / stride;
        num_grid_x = num_grid / num_grid_y;
    }

    const int num_class = feat_blob.w - 5;

    const int num_anchors = anchors.w / 2;

    for (int q = 0; q < num_anchors; q++) {
        const float anchor_w = anchors[q * 2];
        const float anchor_h = anchors[q * 2 + 1];

        const ncnn::Mat feat = feat_blob.channel(q);

        for (int i = 0; i < num_grid_y; i++) {
            for (int j = 0; j < num_grid_x; j++) {
                const float *featptr = feat.row(i * num_grid_x + j);

                // find class index with max class score
                int class_index = 0;
                float class_score = -FLT_MAX;
                for (int k = 0; k < num_class; k++) {
                    float score = featptr[5 + k];
                    if (score > class_score) {
                        class_index = k;
                        class_score = score;
                    }
                }

                float box_score = featptr[4];

                float confidence = sigmoid(box_score) * sigmoid(class_score);

                if (confidence >= prob_threshold) {
                    // yolov5/models/yolo.py Detect forward
                    // y = x[i].sigmoid()
                    // y[..., 0:2] = (y[..., 0:2] * 2. - 0.5 + self.grid[i].to(x[i].device)) * self.stride[i]  # xy
                    // y[..., 2:4] = (y[..., 2:4] * 2) ** 2 * self.anchor_grid[i]  # wh

                    float dx = sigmoid(featptr[0]);
                    float dy = sigmoid(featptr[1]);
                    float dw = sigmoid(featptr[2]);
                    float dh = sigmoid(featptr[3]);

                    float pb_cx = (dx * 2.f - 0.5f + j) * stride;
                    float pb_cy = (dy * 2.f - 0.5f + i) * stride;

                    float pb_w = pow(dw * 2.f, 2) * anchor_w;
                    float pb_h = pow(dh * 2.f, 2) * anchor_h;

                    float x0 = pb_cx - pb_w * 0.5f;
                    float y0 = pb_cy - pb_h * 0.5f;
                    float x1 = pb_cx + pb_w * 0.5f;
                    float y1 = pb_cy + pb_h * 0.5f;

                    YoloV5Object obj;
                    obj.x = x0;
                    obj.y = y0;
                    obj.w = x1 - x0;
                    obj.h = y1 - y0;
                    obj.label = class_index;
                    obj.prob = confidence;

                    objects.push_back(obj);
                }
            }
        }
    }
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

        if (g_scrfd) {
            delete g_scrfd;
            g_scrfd = NULL;
        }

        if (g_nanodet) {
            delete g_nanodet;
            g_nanodet = NULL;
        }
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
 * @param is_realtime_inference   1: realtime inference with live camera/online TV, 0: not realtime inference
 * @return
 */
JNIEXPORT jboolean JNICALL
Java_org_ncnn_ncnnjava_loadModel(JNIEnv *env, jobject thiz, jobject assetManager, jint netid,
                                 jint modelid, jint backend_type, jboolean is_realtime_inference) {
    LOGGD("netid %d", netid);
    LOGGD("modelid %d", modelid);
    LOGGD("backend_type %d", backend_type);
    LOGGD("is_realtime_inference", is_realtime_inference);

    //AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
    //LOGGD("ncnn load model with AAssetManager %p", mgr);

    if (1 == is_realtime_inference) {
        //sanity check
        if ((NULL != g_scrfd) && (NULL != g_nanodet)) {
            LOGGW("it should not happen, pls check");
        }
        {
            ncnn::MutexLockGuard g(lock);

            if (g_scrfd) {
                delete g_scrfd;
                g_scrfd = NULL;
            }

            if (g_nanodet) {
                delete g_nanodet;
                g_nanodet = NULL;
            }
        }
        //end sanity check

        switch (netid) {
            case NCNN_REALTIMEINFERENCE_FACEDETECT: {
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
                        g_scrfd->load(modeltype, use_gpu);
                    }
                }
            }
                break;

            case NCNN_REALTIMEINFERENCE_NANODAT: {
                if (modelid < 0 || modelid > 6 || backend_type > NCNN_BACKEND_MAX) {
                    LOGGW("invalid params");
                    return JNI_FALSE;
                }

                //AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);
                const char *modeltypes[] =
                        {
                                "m",
                                "m-416",
                                "g",
                                "ELite0_320",
                                "ELite1_416",
                                "ELite2_512",
                                "RepVGG-A0_416"
                        };

                const int target_sizes[] =
                        {
                                320,
                                416,
                                416,
                                320,
                                416,
                                512,
                                416
                        };

                const float mean_vals[][3] =
                        {
                                {103.53f, 116.28f, 123.675f},
                                {103.53f, 116.28f, 123.675f},
                                {103.53f, 116.28f, 123.675f},
                                {127.f,   127.f,   127.f},
                                {127.f,   127.f,   127.f},
                                {127.f,   127.f,   127.f},
                                {103.53f, 116.28f, 123.675f}
                        };

                const float norm_vals[][3] =
                        {
                                {1.f / 57.375f, 1.f / 57.12f, 1.f / 58.395f},
                                {1.f / 57.375f, 1.f / 57.12f, 1.f / 58.395f},
                                {1.f / 57.375f, 1.f / 57.12f, 1.f / 58.395f},
                                {1.f / 128.f,   1.f / 128.f,  1.f / 128.f},
                                {1.f / 128.f,   1.f / 128.f,  1.f / 128.f},
                                {1.f / 128.f,   1.f / 128.f,  1.f / 128.f},
                                {1.f / 57.375f, 1.f / 57.12f, 1.f / 58.395f}
                        };

                const char *modeltype = modeltypes[(int) modelid];
                int target_size = target_sizes[(int) modelid];
                bool use_gpu = (backend_type == NCNN_BACKEND_GPU);

                // reload
                {
                    ncnn::MutexLockGuard g(lock);

                    if (use_gpu && ncnn::get_gpu_count() == 0) {
                        LOGGD("using ncnn gpu but ncnn gpu backend not supported");
                        NCNN_JNI_NOTIFY(
                                "using ncnn gpu backend but ncnn gpu backend not supported");
                        // no gpu
                        delete g_nanodet;
                        g_nanodet = 0;
                    } else {
                        if (!g_nanodet)
                            g_nanodet = new NanoDet;
                        g_nanodet->load(modeltype, target_size, mean_vals[(int) modelid],
                                        norm_vals[(int) modelid], use_gpu);
                    }
                }
            }
                break;

            default:
                LOGGD("netid %d not supported with realtime inference for live camera/online TV",
                      netid);
                break;
        }

    }

    if (0 == is_realtime_inference) {

        if (NCNN_BENCHMARK_RESNET == netid) { //ResNet
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
#if 0
                int ret = resnet.load_param_bin(mgr, "resnet.param.bin");
#else
                int ret = resnet.load_param_bin("/sdcard/kantv/models/resnet.param.bin");
#endif
                if (ret != 0) {
                    LOGGW("ResNet Ncnn: load_param_bin failed");
                    NCNN_JNI_NOTIFY("ResNet Ncnn: load_param_bin failed");
                    return JNI_FALSE;
                }
            }

            // init bin
            {
#if 0
                int ret = resnet.load_model(mgr, "resnet.bin");
#else
                int ret = resnet.load_model("/sdcard/kantv/models/resnet.bin");
#endif
                if (ret != 0) {
                    LOGGW("ResNet Ncnn:load_model failed");
                    NCNN_JNI_NOTIFY("ResNet Ncnn:load_model failed");
                    return JNI_FALSE;
                }
            }

        } else if (NCNN_BENCHMARK_SQUEEZENET == netid) { //SqueezeNet
            // init param
            {
#if 0
                int ret = squeezenet.load_param_bin(mgr, "squeezenet_v1.1.param.bin");
#else
                int ret = squeezenet.load_param_bin("/sdcard/kantv/models/squeezenet_v1.1.param.bin");
#endif
                if (ret != 0) {
                    LOGGW("SqueezeNet Ncnn:load_param_bin failed");
                    NCNN_JNI_NOTIFY("SqueezeNet Ncnn:load_param_bin failed");
                    return JNI_FALSE;
                }
            }

            // init bin
            {
#if 0
                int ret = squeezenet.load_model(mgr, "squeezenet_v1.1.bin");
#else
                int ret = squeezenet.load_model("/sdcard/kantv/models/squeezenet_v1.1.bin");
#endif
                if (ret != 0) {
                    LOGGW("SqueezeNet Ncnn:load_model failed");
                    NCNN_JNI_NOTIFY("SqueezeNet Ncnn:load_model failed");
                    return JNI_FALSE;
                }
            }

            // use vulkan compute
            if (ncnn::get_gpu_count() != 0) {
                LOGGD("using ncnn gpu backend");
                NCNN_JNI_NOTIFY("using ncnn gpu backend");
                squeezenet_gpu.opt.use_vulkan_compute = true;

                {
#if 0
                    int ret = squeezenet_gpu.load_param_bin(mgr, "squeezenet_v1.1.param.bin");
#else
                    int ret = squeezenet_gpu.load_param_bin( "/sdcard/kantv/models/squeezenet_v1.1.param.bin");
#endif
                    if (ret != 0) {
                        LOGGW("SqueezeNet Ncnn: load_param_bin failed");
                        NCNN_JNI_NOTIFY("SqueezeNet Ncnn: load_param_bin failed");
                        return JNI_FALSE;
                    }
                }
                {
#if 0
                    int ret = squeezenet_gpu.load_model(mgr, "squeezenet_v1.1.bin");
#else
                    int ret = squeezenet_gpu.load_model( "/sdcard/kantv/models/squeezenet_v1.1.bin");
#endif
                    if (ret != 0) {
                        LOGGW("SqueezeNet Ncnn:load_model failed");
                        NCNN_JNI_NOTIFY("SqueezeNet Ncnn:load_model failed");
                        return JNI_FALSE;
                    }
                }
            } else {
                LOGGD("ncnn gpu backend not supported");
                NCNN_JNI_NOTIFY("gpu backend not supported");
            }

            // init words
            {
#if 0
                AAsset *asset = AAssetManager_open(mgr, "synset_words.txt", AASSET_MODE_BUFFER);
                if (!asset) {
                    LOGGW("SqueezeNet Ncnn:open synset_words.txt failed");
                    NCNN_JNI_NOTIFY("SqueezeNet Ncnn:open synset_words.txt failed");
                    return JNI_FALSE;
                }

                int len = AAsset_getLength(asset);

                std::string words_buffer;
                words_buffer.resize(len);
                int ret = AAsset_read(asset, (void *) words_buffer.data(), len);

                AAsset_close(asset);
#else
                std::ifstream file("/sdcard/kantv/models/synset_words.txt");
                size_t len = 0;
                size_t ret = 0;
                if (file.is_open()) {
                    std::streampos begin = file.tellg();
                    file.seekg(0, std::ios::end);
                    std::streampos end   = file.tellg();
                    len   = end - begin;
                    file.seekg(0, std::ios::beg);
                    file.close();
                } else {
                    LOGGW("SqueezeNet Ncnn:read synset_words.txt failed");
                    NCNN_JNI_NOTIFY("SqueezeNet Ncnn:read synset_words.txt failed");
                    return JNI_FALSE;
                }
                std::string words_buffer;
                words_buffer.resize(len);

                FILE *fp = fopen("/sdcard/kantv/models/synset_words.txt", "r");
                if (fp) {
                    ret = fread((void *) words_buffer.data(), 1, len, fp);
                    fclose(fp);
                    fp = NULL;
                } else {
                    LOGGW("SqueezeNet Ncnn:read synset_words.txt failed");
                    NCNN_JNI_NOTIFY("SqueezeNet Ncnn:read synset_words.txt failed");
                    return JNI_FALSE;
                }
#endif
                if (ret != len) {
                    LOGGW("SqueezeNet Ncnn:read synset_words.txt failed");
                    NCNN_JNI_NOTIFY("SqueezeNet Ncnn:read synset_words.txt failed");
                    return JNI_FALSE;
                }

                squeezenet_words = split_string(words_buffer, "\n");
            }
        } else if (netid == NCNN_BENCHMARK_MNIST) {
            LOGGD("load ncnn mnist model");
            mnist.opt.use_local_pool_allocator = false;
            mnist.opt.use_sgemm_convolution = false;

            //05-22-2024,all cases(2 + 2*2) works fine/perfectly as expected on Xiaomi 14
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
                    //int ret = mnist.load_param("/sdcard/kantv/models/mnist.param");
                    int ret = mnist.load_param_bin("/sdcard/kantv/models/mnist.param.bin");
                    if (ret != 0) {
                        LOGGW("mnist Ncnn: load_param failed");
                        NCNN_JNI_NOTIFY("mnist Ncnn: load_param failed");
                        return JNI_FALSE;
                    }
                    // init bin
                    ret = mnist.load_model("/sdcard/kantv/models/mnist.bin");
                    if (ret != 0) {
                        LOGGW("mnist Ncnn:load_model failed");
                        NCNN_JNI_NOTIFY("mnist Ncnn:load_model failed");
                        return JNI_FALSE;
                    }
                } else {
                    // init param
                    int ret = mnist.load_param("/sdcard/kantv/models/mnist-int8.param");
                    //int ret = mnist.load_param_bin("/sdcard/kantv/models/mnist-int8.param.bin");
                    if (ret != 0) {
                        LOGGW("mnist Ncnn: load_param failed");
                        NCNN_JNI_NOTIFY("mnist Ncnn: load_param failed");
                        return JNI_FALSE;
                    }
                    // init bin
                    ret = mnist.load_model("/sdcard/kantv/models/mnist-int8.bin");
                    if (ret != 0) {
                        LOGGW("mnist Ncnn:load_model failed");
                        NCNN_JNI_NOTIFY("mnist Ncnn:load_model failed");
                        return JNI_FALSE;
                    }
                }
            }
        } else if (netid == NCNN_BENCHARK_YOLOV5) {
            ncnn::Option opt;
            opt.lightmode = true;
            opt.num_threads = 4;
            opt.blob_allocator = &g_blob_pool_allocator;
            opt.workspace_allocator = &g_workspace_pool_allocator;
            opt.use_packing_layout = true;

            // use vulkan compute
            if (ncnn::get_gpu_count() != 0)
                opt.use_vulkan_compute = true;

            //AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

            yolov5.opt = opt;

            yolov5.register_custom_layer("YoloV5Focus", YoloV5Focus_layer_creator);

            // init param
            {
                int ret = yolov5.load_param("/sdcard/kantv/models/yolov5s.param");
                if (ret != 0) {
                    LOGGD("YoloV5Ncnn:load_param failed");
                    NCNN_JNI_NOTIFY("YoloV5Ncnn:load_param failed");
                    return JNI_FALSE;
                }
            }

            // init bin
            {
                int ret = yolov5.load_model("/sdcard/kantv/models/yolov5s.bin");
                if (ret != 0) {
                    LOGGD("YoloV5Ncnn:load_model failed");
                    NCNN_JNI_NOTIFY("YoloV5Ncnn:load_model failed");
                    return JNI_FALSE;
                }
            }

        } else {
            LOGGW("netid %d not supported using ncnn with non-live inference", netid);
            NCNN_JNI_NOTIFY("netid %d not supported using ncnn with non-live inference", netid);
            return JNI_FALSE;
        }
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


//TODO: remove JNIENV
static void detectResNet(JNIEnv *env, jobject bitmap, bool use_gpu) {
    if (use_gpu && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu backend not supported");
        NCNN_JNI_NOTIFY("gpu backend not supported");
        return;
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGGW("bitmap is not RGBA_8888");
        NCNN_JNI_NOTIFY("bitmap format is not RGBA_8888");
        return;
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
            "elapsed time:" + time;

    LOGGD("ncnn resnet inference result:%s", result_str.c_str());
    NCNN_JNI_NOTIFY("ncnn resnet inference result:%s", result_str.c_str());

    return;
}


//TODO: remove JNIENV
static void detectSqueezeNet(JNIEnv *env, jobject bitmap, bool use_gpu) {
    if (use_gpu && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu backend not supported");
        NCNN_JNI_NOTIFY("gpu backend not supported");
        return;
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    int width = info.width;
    int height = info.height;

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGGW("bitmap format is not RGBA_8888");
        NCNN_JNI_NOTIFY("bitmap format is not RGBA_8888");
        return;
    }
    ncnn::Mat in;
    if (width != 227 || height != 227) {
        LOGGW("width and height is not 227");
        NCNN_JNI_NOTIFY("width and height is not 227");
        in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_BGR, 227, 227);
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
        return;
    }

    // +10 to skip leading n03179701
    std::string result_str = std::string(word.c_str() + 10) + ", probability= " + tmp;
    double elapsed = ncnn::get_current_time() - start_time;
    LOGGD("ncnn squeezenet inference result:%s, elapsed %.2f ms", result_str.c_str(), elapsed);
    NCNN_JNI_NOTIFY("ncnn squeezenet inference result:%s, elapse %.2f ms", result_str.c_str(),
                    elapsed);

    return;
}


//TODO: remove JNIENV
static void detectMnist(JNIEnv *env, jobject bitmap, bool use_gpu) {
    if (use_gpu && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu backend not supported");
        NCNN_JNI_NOTIFY("gpu backend not supported");
        return;
    }

    LOGGD("Preparing input...");
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
            return;
        }

        if (width != 28 || height != 28) {
            LOGGW("width and height are not 28");
            NCNN_JNI_NOTIFY("width and height of input image are not 28, scale it to 28x28");
            //return NULL;
            in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_GRAY, 28, 28);
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

    return;
}


//TODO: remove JNIENV
static void detectYoloV5(JNIEnv *env, jobject bitmap, bool use_gpu) {
    if (use_gpu && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu backend not supported");
        NCNN_JNI_NOTIFY("gpu backend not supported");
        return;
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);
    const int width = info.width;
    const int height = info.height;
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGGW("bitmap is not RGBA_8888");
        NCNN_JNI_NOTIFY("bitmap format is not RGBA_8888");
        return;
    }

    LOGGD("here");
    // ncnn from bitmap
    const int target_size = 640;

    // letterbox pad to multiple of 32
    int w = width;
    int h = height;
    float scale = 1.f;
    if (w > h) {
        scale = (float) target_size / w;
        w = target_size;
        h = h * scale;
    } else {
        scale = (float) target_size / h;
        h = target_size;
        w = w * scale;
    }

    ncnn::Mat in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_RGB, w, h);

    // pad to target_size rectangle
    // yolov5/utils/datasets.py letterbox
    int wpad = (w + 31) / 32 * 32 - w;
    int hpad = (h + 31) / 32 * 32 - h;
    ncnn::Mat in_pad;
    ncnn::copy_make_border(in, in_pad, hpad / 2, hpad - hpad / 2, wpad / 2, wpad - wpad / 2,
                           ncnn::BORDER_CONSTANT, 114.f);

    // yolov5
    std::vector<YoloV5Object> objects;
    {
        const float prob_threshold = 0.25f;
        const float nms_threshold = 0.45f;

        const float norm_vals[3] = {1 / 255.f, 1 / 255.f, 1 / 255.f};
        in_pad.substract_mean_normalize(0, norm_vals);

        ncnn::Extractor ex = yolov5.create_extractor();

        ex.set_vulkan_compute(use_gpu);

        ex.input("images", in_pad);

        std::vector<YoloV5Object> proposals;

        // stride 8
        {
            ncnn::Mat out;
            ex.extract("output", out);

            ncnn::Mat anchors(6);
            anchors[0] = 10.f;
            anchors[1] = 13.f;
            anchors[2] = 16.f;
            anchors[3] = 30.f;
            anchors[4] = 33.f;
            anchors[5] = 23.f;

            std::vector<YoloV5Object> objects8;
            generate_proposals(anchors, 8, in_pad, out, prob_threshold, objects8);

            proposals.insert(proposals.end(), objects8.begin(), objects8.end());
        }

        // stride 16
        {
            ncnn::Mat out;
            ex.extract("781", out);

            ncnn::Mat anchors(6);
            anchors[0] = 30.f;
            anchors[1] = 61.f;
            anchors[2] = 62.f;
            anchors[3] = 45.f;
            anchors[4] = 59.f;
            anchors[5] = 119.f;

            std::vector<YoloV5Object> objects16;
            generate_proposals(anchors, 16, in_pad, out, prob_threshold, objects16);

            proposals.insert(proposals.end(), objects16.begin(), objects16.end());
        }

        // stride 32
        {
            ncnn::Mat out;
            ex.extract("801", out);

            ncnn::Mat anchors(6);
            anchors[0] = 116.f;
            anchors[1] = 90.f;
            anchors[2] = 156.f;
            anchors[3] = 198.f;
            anchors[4] = 373.f;
            anchors[5] = 326.f;

            std::vector<YoloV5Object> objects32;
            generate_proposals(anchors, 32, in_pad, out, prob_threshold, objects32);

            proposals.insert(proposals.end(), objects32.begin(), objects32.end());
        }

        // sort all proposals by score from highest to lowest
        qsort_descent_inplace(proposals);

        // apply nms with nms_threshold
        std::vector<int> picked;
        nms_sorted_bboxes(proposals, picked, nms_threshold);

        int count = picked.size();

        objects.resize(count);
        for (int i = 0; i < count; i++) {
            objects[i] = proposals[picked[i]];

            // adjust offset to original unpadded
            float x0 = (objects[i].x - (wpad / 2)) / scale;
            float y0 = (objects[i].y - (hpad / 2)) / scale;
            float x1 = (objects[i].x + objects[i].w - (wpad / 2)) / scale;
            float y1 = (objects[i].y + objects[i].h - (hpad / 2)) / scale;

            // clip
            x0 = std::max(std::min(x0, (float) (width - 1)), 0.f);
            y0 = std::max(std::min(y0, (float) (height - 1)), 0.f);
            x1 = std::max(std::min(x1, (float) (width - 1)), 0.f);
            y1 = std::max(std::min(y1, (float) (height - 1)), 0.f);

            objects[i].x = x0;
            objects[i].y = y0;
            objects[i].w = x1 - x0;
            objects[i].h = y1 - y0;
        }
    }

    // objects to Obj[]
    static const char *class_names[] = {
            "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
            "traffic light",
            "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse",
            "sheep", "cow",
            "elephant", "bear", "zebra", "giraffe", "backpack", "umbrella", "handbag", "tie",
            "suitcase", "frisbee",
            "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
            "skateboard", "surfboard",
            "tennis racket", "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl",
            "banana", "apple",
            "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
            "chair", "couch",
            "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
            "keyboard", "cell phone",
            "microwave", "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
            "scissors", "teddy bear",
            "hair drier", "toothbrush"
    };

    for (size_t i = 0; i < objects.size(); i++) {
        LOGGD("x=%d, y=%d, w=%d, h=%d, label=%s, probability=%.2f", objects[i].x, objects[i].y, objects[i].w, objects[i].h, class_names[objects[i].label], objects[i].prob);
        NCNN_JNI_NOTIFY("x=%d, y=%d, w=%d, h=%d, label=%s, probability=%.2f", objects[i].x, objects[i].y, objects[i].w, objects[i].h, class_names[objects[i].label], objects[i].prob);
    }

    double elapsed = ncnn::get_current_time() - start_time;
    LOGGD("ncnn YoloV5 inference elapsed %.2f ms", elapsed);
    NCNN_JNI_NOTIFY("ncnn YoloV5 inference elapsed %.2f ms", elapsed);
}


/**
*
* @param sz_ncnnmodel_param   param file of ncnn model
* @param sz_ncnnmodel_bin     bin   file of ncnn model
* @param sz_user_data         ASR: /sdcard/kantv/jfk.wav / LLM: user input / TEXT2IMAGE: user input / ResNet&SqueezeNet&MNIST: image path / TTS: user input
* @param bitmap
* @param n_bench_type         1: NCNN RESNET 2: NCNN SQUEEZENET 3: NCNN MNIST
* @param n_threads            1 - 8
* @param n_backend_type       0: NCNN_BACKEND_CPU  1: NCNN_BACKEND_GPU
* @param n_op_type            type of NCNN OP
* @return
*/
//TODO: remove JNIEnv
void ncnn_jni_bench(JNIEnv *env, const char *sz_ncnnmodel_param, const char *sz_ncnnmodel_bin,
                    const char *sz_user_data, jobject bitmap, int n_bench_type, int n_threads,
                    int n_backend_type, int n_op_type) {
    LOGGD("model param:%s\n", sz_ncnnmodel_param);
    LOGGD("model bin:%s\n", sz_ncnnmodel_bin);
    LOGGD("user data: %s\n", sz_user_data);
    LOGGD("bench type:%d\n", n_bench_type);
    LOGGD("backend type:%d\n", n_backend_type);
    LOGGD("op type:%d\n", n_op_type);

    bool use_gpu = (n_backend_type == NCNN_BACKEND_GPU ? true : false);

    switch (n_bench_type) {
        case NCNN_BENCHMARK_RESNET:
            detectResNet(env, bitmap, use_gpu);
            break;
        case NCNN_BENCHMARK_SQUEEZENET:
            detectSqueezeNet(env, bitmap, use_gpu);
            break;
        case NCNN_BENCHMARK_MNIST:
            detectMnist(env, bitmap, use_gpu);
            break;
        case NCNN_BENCHARK_YOLOV5:
            detectYoloV5(env, bitmap, use_gpu);
            break;
            //=============================================================================================
            //add new benchmark type for NCNN here
            //=============================================================================================
        default:
            break;

    }
}

}



