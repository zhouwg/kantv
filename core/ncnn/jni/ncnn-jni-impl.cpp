// ref&author:https://github.com/nihui/ncnn-android-scrfd
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

#include <android/asset_manager_jni.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <android/bitmap.h>

#include <android/log.h>

#include <jni.h>

#include <string>
#include <vector>
#include <cmath>

#include "platform.h"
#include "benchmark.h"
#include "net.h"

#include "scrfd.h"
#include "res.id.h"

#include "ndkcamera.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "ncnn-jni.h"

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

static SCRFD *g_scrfd = 0;
static ncnn::Mutex lock;

static ncnn::UnlockedPoolAllocator g_blob_pool_allocator;
static ncnn::PoolAllocator g_workspace_pool_allocator;
static ncnn::Net squeezenet;

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat &rgb) const;
};

void MyNdkCamera::on_image_render(cv::Mat &rgb) const {
    // scrfd
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

static MyNdkCamera *g_camera = 0;

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
        g_scrfd = 0;
    }

    ncnn::destroy_gpu_instance();

    delete g_camera;
    g_camera = 0;
}

// public native boolean loadModel(AssetManager mgr, int modelid, int cpugpu);
JNIEXPORT jboolean JNICALL
Java_org_ncnn_ncnnjava_loadModel(JNIEnv *env, jobject thiz, jobject assetManager, jint netid,
                                 jint modelid, jint cpugpu) {
    LOGGD("netid %d", netid);
    LOGGD("modelid %d", modelid);
    LOGGD("cpugpu %d", cpugpu);
    if (0 == netid) { // face detection
        if (modelid < 0 || modelid > 7 || cpugpu < 0 || cpugpu > 1) {
            return JNI_FALSE;
        }

        AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

        LOGGD("ncnn load Model %p", mgr);

        const char *modeltypes[] =
                {
                        "500m",
                        "500m_kps",
                        "1g",
                        "2.5g",
                        "2.5g_kps",
                        "10g",
                        "10g_kps",
                        "34g"
                };

        const char *modeltype = modeltypes[(int) modelid];
        bool use_gpu = (int) cpugpu == 1;

        // reload
        {
            ncnn::MutexLockGuard g(lock);

            if (use_gpu && ncnn::get_gpu_count() == 0) {
                // no gpu
                delete g_scrfd;
                g_scrfd = 0;
            } else {
                if (!g_scrfd)
                    g_scrfd = new SCRFD;
                g_scrfd->load(mgr, modeltype, use_gpu);
            }
        }
    }

    if (1 == netid) { //ResNet
        ncnn::Option opt;
        opt.lightmode = true;
        opt.num_threads = 4;
        opt.blob_allocator = &g_blob_pool_allocator;
        opt.workspace_allocator = &g_workspace_pool_allocator;

        // use vulkan compute
        if (ncnn::get_gpu_count() != 0)
            opt.use_vulkan_compute = true;

        AAssetManager *mgr = AAssetManager_fromJava(env, assetManager);

        squeezenet.opt = opt;

        // init param
        {
            int ret = squeezenet.load_param_bin(mgr, "resnet.param.bin");
            if (ret != 0) {
                LOGGW("SqueezeNcnn: load_param_bin failed");
                return JNI_FALSE;
            }
        }

        // init bin
        {
            int ret = squeezenet.load_model(mgr, "resnet.bin");
            if (ret != 0) {
                LOGGW("SqueezeNcnn:load_model failed");
                return JNI_FALSE;
            }
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


JNIEXPORT jstring JNICALL
Java_org_ncnn_ncnnjava_detectSqueeze(JNIEnv *env, jobject thiz, jobject bitmap, jboolean use_gpu) {
    if (use_gpu == JNI_TRUE && ncnn::get_gpu_count() == 0) {
        LOGGW("gpu not supported");
        NCNN_JNI_NOTIFY("gpu not supported");
        return env->NewStringUTF("no vulkan capable gpu");
    }

    double start_time = ncnn::get_current_time();

    AndroidBitmapInfo info;
    AndroidBitmap_getInfo(env, bitmap, &info);

    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888)
        return NULL;

    ncnn::Mat in = ncnn::Mat::from_android_bitmap_resize(env, bitmap, ncnn::Mat::PIXEL_RGB, 224,
                                                         224);
    std::vector<float> cls_scores;
    {
        const float mean_vals[3] = {123.675f, 116.28f, 103.53f};
        const float std_vals[3] = {1 / 58.395f, 1 / 57.12f, 1 / 57.375f};

        in.substract_mean_normalize(mean_vals, std_vals);

        ncnn::Extractor ex = squeezenet.create_extractor();

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

}
