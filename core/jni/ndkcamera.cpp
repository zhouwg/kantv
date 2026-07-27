// ref:https://github.com/nihui/ncnn-android-scrfd
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

#include "ndkcamera.h"

#include <string>
#include <climits>

#include <android/log.h>

#include <opencv2/core/core.hpp>

#include "image_utils.h"

#include "ggml-jni.h"

static const int NDKCAMERAWINDOW_ID = 233;

static void onDisconnected(void* context, ACameraDevice* device)
{
    LOGGI("NdkCamera: onDisconnected %p", device);
}

static void onError(void* context, ACameraDevice* device, int error)
{
    LOGGW("NdkCamera: onError %p %d", device, error);
}

static void onImageAvailable(void* context, AImageReader* reader)
{
    // NOTE: do NOT check realtimemtmd_is_running_state() here.
    // That flag gates INFERENCE (see do_inference / mtmd_inference), not preview.
    // Checking it here causes black screen when openCamera() resets the flag
    // before surfaceChanged/onSessionActive re-init it.

    AImage* image = 0;
    media_status_t status = AImageReader_acquireLatestImage(reader, &image);

    if (status != AMEDIA_OK)
    {
        LOGGW("onImageAvailable: acquireLatestImage failed status=%d", status);
        return;
    }

    int32_t format;
    AImage_getFormat(image, &format);

    // assert format == AIMAGE_FORMAT_YUV_420_888

    int32_t width = 0;
    int32_t height = 0;
    AImage_getWidth(image, &width);
    AImage_getHeight(image, &height);

    int32_t y_pixelStride = 0;
    int32_t u_pixelStride = 0;
    int32_t v_pixelStride = 0;
    AImage_getPlanePixelStride(image, 0, &y_pixelStride);
    AImage_getPlanePixelStride(image, 1, &u_pixelStride);
    AImage_getPlanePixelStride(image, 2, &v_pixelStride);

    int32_t y_rowStride = 0;
    int32_t u_rowStride = 0;
    int32_t v_rowStride = 0;
    AImage_getPlaneRowStride(image, 0, &y_rowStride);
    AImage_getPlaneRowStride(image, 1, &u_rowStride);
    AImage_getPlaneRowStride(image, 2, &v_rowStride);

    uint8_t* y_data = 0;
    uint8_t* u_data = 0;
    uint8_t* v_data = 0;
    int y_len = 0;
    int u_len = 0;
    int v_len = 0;
    AImage_getPlaneData(image, 0, &y_data, &y_len);
    AImage_getPlaneData(image, 1, &u_data, &u_len);
    AImage_getPlaneData(image, 2, &v_data, &v_len);

    if (u_data == v_data + 1 && v_data == y_data + width * height && y_pixelStride == 1 && u_pixelStride == 2 && v_pixelStride == 2 && y_rowStride == width && u_rowStride == width && v_rowStride == width)
    {
        // already nv21  :)
        ((NdkCamera*)context)->on_image((unsigned char*)y_data, (int)width, (int)height);
    }
    else
    {
        // construct nv21
        unsigned char* nv21 = new unsigned char[width * height + width * height / 2];
        {
            // Y
            unsigned char* yptr = nv21;
            for (int y=0; y<height; y++)
            {
                const unsigned char* y_data_ptr = y_data + y_rowStride * y;
                for (int x=0; x<width; x++)
                {
                    yptr[0] = y_data_ptr[0];
                    yptr++;
                    y_data_ptr += y_pixelStride;
                }
            }

            // UV
            unsigned char* uvptr = nv21 + width * height;
            for (int y=0; y<height/2; y++)
            {
                const unsigned char* v_data_ptr = v_data + v_rowStride * y;
                const unsigned char* u_data_ptr = u_data + u_rowStride * y;
                for (int x=0; x<width/2; x++)
                {
                    uvptr[0] = v_data_ptr[0];
                    uvptr[1] = u_data_ptr[0];
                    uvptr += 2;
                    v_data_ptr += v_pixelStride;
                    u_data_ptr += u_pixelStride;
                }
            }
        }

        ((NdkCamera*)context)->on_image((unsigned char*)nv21, (int)width, (int)height);

        delete[] nv21;
    }

    AImage_delete(image);
}

static void onSessionActive(void* context, ACameraCaptureSession *session)
{
    LOGGI("NdkCamera: onSessionActive %p", session);
    realtimemtmd_init_running_state();
}

static void onSessionReady(void* context, ACameraCaptureSession *session)
{
    LOGGI("NdkCamera: onSessionReady %p", session);
}

static void onSessionClosed(void* context, ACameraCaptureSession *session)
{
    LOGGI("NdkCamera: onSessionClosed %p", session);
    if (0 != realtimemtmd_is_running_state()) {
        LOGGI("it shouldn't happen");
        realtimemtmd_reset_running_state();
    }
}

void onCaptureFailed(void* context, ACameraCaptureSession* session, ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    LOGGI("NdkCamera: onCaptureFailed %p %p %p", session, request, failure);
}

void onCaptureSequenceCompleted(void* context, ACameraCaptureSession* session, int sequenceId, int64_t frameNumber)
{
    LOGGI("NdkCamera: onCaptureSequenceCompleted %p %d %ld", session, sequenceId, frameNumber);
}

void onCaptureSequenceAborted(void* context, ACameraCaptureSession* session, int sequenceId)
{
    LOGGI("NdkCamera: onCaptureSequenceAborted %p %d", session, sequenceId);
}

void onCaptureCompleted(void* context, ACameraCaptureSession* session, ACaptureRequest* request, const ACameraMetadata* result)
{
    //LOGGI("NdkCamera: onCaptureCompleted %p %p %p", session, request, result);
}

NdkCamera::NdkCamera()
{
    camera_facing = 0;
    camera_orientation = 0;

    camera_manager = 0;
    camera_device = 0;
    image_reader = 0;
    image_reader_surface = 0;
    image_reader_target = 0;
    capture_request = 0;
    capture_session_output_container = 0;
    capture_session_output = 0;
    capture_session = 0;
}

NdkCamera::~NdkCamera()
{
    close();

    if (image_reader)
    {
        LOGGD("calling AImageReader_delete");
        AImageReader_delete(image_reader);
        image_reader = 0;
        LOGGD("after calling AImageReader_delete");
    }

    if (image_reader_surface)
    {
        ANativeWindow_release(image_reader_surface);
        image_reader_surface = 0;
    }
    LOGGD("leave %s", __func__);
}

// Pick a YUV_420_888 output size closest to the preferred size.
// The previous hardcoded 640x480 image reader could fail for the rear camera
// on some devices if that size is not published in the camera's available
// stream configurations. Query ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS
// and fall back to the closest supported size.
static bool find_best_yuv_size(ACameraMetadata* camera_metadata, int preferred_w, int preferred_h,
                               int* out_w, int* out_h)
{
    ACameraMetadata_const_entry e = { 0 };
    camera_status_t status = ACameraMetadata_getConstEntry(camera_metadata,
            ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS, &e);
    if (status != ACAMERA_OK || e.count == 0 || e.data.i32 == nullptr) {
        LOGGW("find_best_yuv_size: failed to get stream configs, status=%d count=%u",
              status, e.count);
        return false;
    }

    int best_w = 0;
    int best_h = 0;
    int best_diff = INT_MAX;
    const int preferred_area = preferred_w * preferred_h;

    for (uint32_t i = 0; i + 3 < e.count; i += 4) {
        int32_t format = e.data.i32[i];
        int32_t w = e.data.i32[i + 1];
        int32_t h = e.data.i32[i + 2];
        int32_t input = e.data.i32[i + 3]; // 0 = output, 1 = input

        if (format != AIMAGE_FORMAT_YUV_420_888 || input != ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) {
            continue;
        }

        int diff = w * h - preferred_area;
        if (diff < 0) diff = -diff;
        if (diff < best_diff) {
            best_diff = diff;
            best_w = w;
            best_h = h;
        }
    }

    if (best_w == 0 || best_h == 0) {
        LOGGW("find_best_yuv_size: no YUV_420_888 output config found");
        return false;
    }

    *out_w = best_w;
    *out_h = best_h;
    return true;
}

int NdkCamera::open(int _camera_facing)
{
    LOGGI("NdkCamera: open camera %d", _camera_facing);

    // skip if already opened with the same facing — repeated open() calls
    // (e.g. from multiple UI events during slow model_init) cause camera device conflict
    // return 1 so the caller knows it was a no-op and must NOT reset the running state
    if (camera_device && camera_facing == _camera_facing) {
        LOGGI("NdkCamera: camera already opened with facing %d, skip", _camera_facing);
        return 1;
    }

    // close any existing camera device/session/image reader before opening a new one.
    close();

    camera_facing = _camera_facing;

    camera_manager = ACameraManager_create();

    // find camera with requested facing
    std::string camera_id;
    {
        ACameraIdList* camera_id_list = 0;
        camera_status_t id_status = ACameraManager_getCameraIdList(camera_manager, &camera_id_list);
        if (id_status != ACAMERA_OK || !camera_id_list) {
            LOGGW("NdkCamera: getCameraIdList failed status=%d", id_status);
            return -1;
        }

        for (int i = 0; i < camera_id_list->numCameras; ++i)
        {
            const char* id = camera_id_list->cameraIds[i];
            ACameraMetadata* camera_metadata = 0;
            camera_status_t c_status = ACameraManager_getCameraCharacteristics(camera_manager, id, &camera_metadata);
            if (c_status != ACAMERA_OK || !camera_metadata) {
                LOGGW("NdkCamera: getCameraCharacteristics failed for %s status=%d", id, c_status);
                continue;
            }

            // query facing
            acamera_metadata_enum_android_lens_facing_t facing = ACAMERA_LENS_FACING_FRONT;
            {
                ACameraMetadata_const_entry e = { 0 };
                camera_status_t f_status = ACameraMetadata_getConstEntry(camera_metadata, ACAMERA_LENS_FACING, &e);
                if (f_status == ACAMERA_OK && e.count > 0 && e.data.u8) {
                    facing = (acamera_metadata_enum_android_lens_facing_t)e.data.u8[0];
                } else {
                    LOGGW("NdkCamera: get LENS_FACING failed for %s status=%d", id, f_status);
                    ACameraMetadata_free(camera_metadata);
                    continue;
                }
            }

            if (camera_facing == 0 && facing != ACAMERA_LENS_FACING_FRONT)
            {
                ACameraMetadata_free(camera_metadata);
                continue;
            }

            if (camera_facing == 1 && facing != ACAMERA_LENS_FACING_BACK)
            {
                ACameraMetadata_free(camera_metadata);
                continue;
            }

            camera_id = id;

            // query orientation
            int orientation = 0;
            {
                ACameraMetadata_const_entry e = { 0 };
                camera_status_t o_status = ACameraMetadata_getConstEntry(camera_metadata, ACAMERA_SENSOR_ORIENTATION, &e);
                if (o_status == ACAMERA_OK && e.count > 0 && e.data.i32) {
                    orientation = (int)e.data.i32[0];
                } else {
                    LOGGW("NdkCamera: get SENSOR_ORIENTATION failed for %s status=%d", id, o_status);
                }
            }

            // query supported stream configurations and create image reader
            // with a size that is guaranteed to be supported by this camera.
            int image_reader_w = 640;
            int image_reader_h = 480;
            if (!find_best_yuv_size(camera_metadata, 640, 480, &image_reader_w, &image_reader_h)) {
                LOGGW("NdkCamera: cannot query stream configs for %s, fallback to 640x480", id);
                image_reader_w = 640;
                image_reader_h = 480;
            }

            camera_orientation = orientation;

            ACameraMetadata_free(camera_metadata);

            // create image reader and its surface for this camera
            LOGGI("NdkCamera: selected image reader size %dx%d for %s", image_reader_w, image_reader_h, id);
            media_status_t mr_status = AImageReader_new(image_reader_w, image_reader_h,
                    AIMAGE_FORMAT_YUV_420_888, /*maxImages*/2, &image_reader);
            if (mr_status != AMEDIA_OK || !image_reader) {
                LOGGW("NdkCamera: AImageReader_new(%d,%d) failed status=%d reader=%p",
                      image_reader_w, image_reader_h, mr_status, image_reader);
                camera_id.clear();
                break;
            }

            AImageReader_ImageListener listener;
            listener.context = this;
            listener.onImageAvailable = onImageAvailable;
            AImageReader_setImageListener(image_reader, &listener);

            AImageReader_getWindow(image_reader, &image_reader_surface);
            if (!image_reader_surface) {
                LOGGW("NdkCamera: AImageReader_getWindow returned null");
                AImageReader_delete(image_reader);
                image_reader = 0;
                camera_id.clear();
                break;
            }
            ANativeWindow_acquire(image_reader_surface);

            break;
        }

        ACameraManager_deleteCameraIdList(camera_id_list);
    }

    if (camera_id.empty()) {
        LOGGW("NdkCamera: no matching camera found for facing %d", camera_facing);
        return -1;
    }

    LOGGI("NdkCamera: open %s %d", camera_id.c_str(), camera_orientation);

    // open camera
    {
        ACameraDevice_StateCallbacks camera_device_state_callbacks;
        camera_device_state_callbacks.context = this;
        camera_device_state_callbacks.onDisconnected = onDisconnected;
        camera_device_state_callbacks.onError = onError;

        camera_status_t open_status = ACameraManager_openCamera(camera_manager, camera_id.c_str(),
                                                                &camera_device_state_callbacks, &camera_device);
        if (open_status != ACAMERA_OK || !camera_device) {
            LOGGW("NdkCamera: openCamera failed status=%d device=%p", open_status, camera_device);
            return -1;
        }
    }

    // capture request
    {
        camera_status_t cr_status = ACameraDevice_createCaptureRequest(camera_device, TEMPLATE_PREVIEW, &capture_request);
        if (cr_status != ACAMERA_OK || !capture_request) {
            LOGGW("NdkCamera: createCaptureRequest failed status=%d request=%p", cr_status, capture_request);
            return -1;
        }

        camera_status_t ot_status = ACameraOutputTarget_create(image_reader_surface, &image_reader_target);
        if (ot_status != ACAMERA_OK || !image_reader_target) {
            LOGGW("NdkCamera: createOutputTarget failed status=%d target=%p", ot_status, image_reader_target);
            return -1;
        }

        camera_status_t at_status = ACaptureRequest_addTarget(capture_request, image_reader_target);
        if (at_status != ACAMERA_OK) {
            LOGGW("NdkCamera: addTarget failed status=%d", at_status);
            return -1;
        }
    }

    // capture session
    {
        ACameraCaptureSession_stateCallbacks camera_capture_session_state_callbacks;
        camera_capture_session_state_callbacks.context = this;
        camera_capture_session_state_callbacks.onActive = onSessionActive;
        camera_capture_session_state_callbacks.onReady = onSessionReady;
        camera_capture_session_state_callbacks.onClosed = onSessionClosed;

        ACaptureSessionOutputContainer_create(&capture_session_output_container);

        ACaptureSessionOutput_create(image_reader_surface, &capture_session_output);

        ACaptureSessionOutputContainer_add(capture_session_output_container, capture_session_output);

        camera_status_t cs_status = ACameraDevice_createCaptureSession(camera_device, capture_session_output_container, &camera_capture_session_state_callbacks, &capture_session);
        LOGGI("NdkCamera: createCaptureSession status=%d session=%p", cs_status, capture_session);
        if (cs_status != ACAMERA_OK || !capture_session) {
            LOGGW("NdkCamera: createCaptureSession failed");
            return -1;
        }

        ACameraCaptureSession_captureCallbacks camera_capture_session_capture_callbacks;
        camera_capture_session_capture_callbacks.context = this;
        camera_capture_session_capture_callbacks.onCaptureStarted = 0;
        camera_capture_session_capture_callbacks.onCaptureProgressed = 0;
        camera_capture_session_capture_callbacks.onCaptureCompleted = onCaptureCompleted;
        camera_capture_session_capture_callbacks.onCaptureFailed = onCaptureFailed;
        camera_capture_session_capture_callbacks.onCaptureSequenceCompleted = onCaptureSequenceCompleted;
        camera_capture_session_capture_callbacks.onCaptureSequenceAborted = onCaptureSequenceAborted;
        camera_capture_session_capture_callbacks.onCaptureBufferLost = 0;

        int seqId = -1;
        camera_status_t sr_status = ACameraCaptureSession_setRepeatingRequest(capture_session, &camera_capture_session_capture_callbacks, 1, &capture_request, &seqId);
        LOGGI("NdkCamera: setRepeatingRequest status=%d seqId=%d", sr_status, seqId);
        if (sr_status != ACAMERA_OK) {
            LOGGW("NdkCamera: setRepeatingRequest failed");
            return -1;
        }
    }

    return 0;
}

void NdkCamera::close()
{
    LOGGD("NdkCamera:close");

    if (capture_session)
    {
        ACameraCaptureSession_stopRepeating(capture_session);
        ACameraCaptureSession_close(capture_session);
        capture_session = 0;
    }

    if (camera_device)
    {
        ACameraDevice_close(camera_device);
        camera_device = 0;
    }

    if (capture_session_output_container)
    {
        ACaptureSessionOutputContainer_remove(capture_session_output_container, capture_session_output);
        ACaptureSessionOutputContainer_free(capture_session_output_container);
        capture_session_output_container = 0;
    }

    if (capture_session_output)
    {
        ACaptureSessionOutput_free(capture_session_output);
        capture_session_output = 0;
    }

    if (capture_request)
    {
        ACaptureRequest_removeTarget(capture_request, image_reader_target);
        ACaptureRequest_free(capture_request);
        capture_request = 0;
    }

    if (image_reader_target)
    {
        ACameraOutputTarget_free(image_reader_target);
        image_reader_target = 0;
    }

    if (image_reader)
    {
        LOGGD("calling AImageReader_delete");
        //attention: deadlock here if used improperly
        //source code of NDKCamera API AImageReader_delete refer to:
        //https://github.com/yuchuangu85/Android-framework-code/blob/master/av/media/ndk/NdkImageReader.cpp#L750
        AImageReader_delete(image_reader);
        image_reader = 0;
        LOGGD("after calling AImageReader_delete");
    }

    if (image_reader_surface)
    {
        ANativeWindow_release(image_reader_surface);
        image_reader_surface = 0;
    }

    if (camera_manager)
    {
        ACameraManager_delete(camera_manager);
        camera_manager = 0;
    }
}

void NdkCamera::on_image(const cv::Mat& rgb) const
{
}

void NdkCamera::on_image(const unsigned char* nv21, int nv21_width, int nv21_height) const
{
    // rotate nv21
    int w = 0;
    int h = 0;
    int rotate_type = 0;
    {
        if (camera_orientation == 0)
        {
            w = nv21_width;
            h = nv21_height;
            rotate_type = camera_facing == 0 ? 2 : 1;
        }
        if (camera_orientation == 90)
        {
            w = nv21_height;
            h = nv21_width;
            rotate_type = camera_facing == 0 ? 5 : 6;
        }
        if (camera_orientation == 180)
        {
            w = nv21_width;
            h = nv21_height;
            rotate_type = camera_facing == 0 ? 4 : 3;
        }
        if (camera_orientation == 270)
        {
            w = nv21_height;
            h = nv21_width;
            rotate_type = camera_facing == 0 ? 7 : 8;
        }
    }

    cv::Mat nv21_rotated(h + h / 2, w, CV_8UC1);
    image_utils::kanna_rotate_yuv420sp(nv21, nv21_width, nv21_height, nv21_rotated.data, w, h, rotate_type);

    // nv21_rotated to rgb
    cv::Mat rgb(h, w, CV_8UC3);
    image_utils::yuv420sp2rgb(nv21_rotated.data, w, h, rgb.data);

    on_image(rgb);
}


NdkCameraWindow::NdkCameraWindow() : NdkCamera()
{
    sensor_manager = 0;
    sensor_event_queue = 0;
    accelerometer_sensor = 0;
    win = 0;

    accelerometer_orientation = 0;

    // sensor
    sensor_manager = ASensorManager_getInstance();

    accelerometer_sensor = ASensorManager_getDefaultSensor(sensor_manager, ASENSOR_TYPE_ACCELEROMETER);
}

NdkCameraWindow::~NdkCameraWindow()
{
    if (accelerometer_sensor)
    {
        ASensorEventQueue_disableSensor(sensor_event_queue, accelerometer_sensor);
        accelerometer_sensor = 0;
    }

    if (sensor_event_queue)
    {
        ASensorManager_destroyEventQueue(sensor_manager, sensor_event_queue);
        sensor_event_queue = 0;
    }

    if (win)
    {
        ANativeWindow_release(win);
    }
}

void NdkCameraWindow::set_window(ANativeWindow* _win)
{
    LOGGI("set_window: old win=%p new win=%p", win, _win);
    if (win)
    {
        ANativeWindow_release(win);
    }

    win = _win;
    if (win) {
        ANativeWindow_acquire(win);
        LOGGI("set_window: acquired win=%p", win);
    }
}

void NdkCameraWindow::on_image_render(cv::Mat& rgb) const
{
}

void NdkCameraWindow::on_image(const unsigned char* nv21, int nv21_width, int nv21_height) const
{
    // win may be NULL if frames arrive before set_window() is called
    // (e.g. during reload() in initView before surfaceChanged). Skip silently.
    if (!win)
    {
        return;
    }

    // resolve orientation from camera_orientation and accelerometer_sensor
    {
        if (!sensor_event_queue)
        {
            sensor_event_queue = ASensorManager_createEventQueue(sensor_manager, ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS), NDKCAMERAWINDOW_ID, 0, 0);

            ASensorEventQueue_enableSensor(sensor_event_queue, accelerometer_sensor);
        }

        int id = ALooper_pollOnce(0, 0, 0, 0);
        if (id == NDKCAMERAWINDOW_ID)
        {
            ASensorEvent e[8];
            ssize_t num_event = 0;
            while (ASensorEventQueue_hasEvents(sensor_event_queue) == 1)
            {
                num_event = ASensorEventQueue_getEvents(sensor_event_queue, e, 8);
                if (num_event < 0)
                    break;
            }

            if (num_event > 0)
            {
                float acceleration_x = e[num_event - 1].acceleration.x;
                float acceleration_y = e[num_event - 1].acceleration.y;
                float acceleration_z = e[num_event - 1].acceleration.z;
                //LOGGD("NdkCameraWindow", "x = %f, y = %f, z = %f", x, y, z);

                if (acceleration_y > 7)
                {
                    accelerometer_orientation = 0;
                }
                if (acceleration_x < -7)
                {
                    accelerometer_orientation = 90;
                }
                if (acceleration_y < -7)
                {
                    accelerometer_orientation = 180;
                }
                if (acceleration_x > 7)
                {
                    accelerometer_orientation = 270;
                }
            }
        }
    }

    // roi crop and rotate nv21
    int nv21_roi_x = 0;
    int nv21_roi_y = 0;
    int nv21_roi_w = 0;
    int nv21_roi_h = 0;
    int roi_x = 0;
    int roi_y = 0;
    int roi_w = 0;
    int roi_h = 0;
    int rotate_type = 0;
    int render_w = 0;
    int render_h = 0;
    int render_rotate_type = 0;
    {
        int win_w = ANativeWindow_getWidth(win);
        int win_h = ANativeWindow_getHeight(win);

        if (accelerometer_orientation == 90 || accelerometer_orientation == 270)
        {
            std::swap(win_w, win_h);
        }

        const int final_orientation = (camera_orientation + accelerometer_orientation) % 360;

        if (final_orientation == 0 || final_orientation == 180)
        {
            if (win_w * nv21_height > win_h * nv21_width)
            {
                roi_w = nv21_width;
                roi_h = (nv21_width * win_h / win_w) / 2 * 2;
                roi_x = 0;
                roi_y = ((nv21_height - roi_h) / 2) / 2 * 2;
            }
            else
            {
                roi_h = nv21_height;
                roi_w = (nv21_height * win_w / win_h) / 2 * 2;
                roi_x = ((nv21_width - roi_w) / 2) / 2 * 2;
                roi_y = 0;
            }

            nv21_roi_x = roi_x;
            nv21_roi_y = roi_y;
            nv21_roi_w = roi_w;
            nv21_roi_h = roi_h;
        }
        if (final_orientation == 90 || final_orientation == 270)
        {
            if (win_w * nv21_width > win_h * nv21_height)
            {
                roi_w = nv21_height;
                roi_h = (nv21_height * win_h / win_w) / 2 * 2;
                roi_x = 0;
                roi_y = ((nv21_width - roi_h) / 2) / 2 * 2;
            }
            else
            {
                roi_h = nv21_width;
                roi_w = (nv21_width * win_w / win_h) / 2 * 2;
                roi_x = ((nv21_height - roi_w) / 2) / 2 * 2;
                roi_y = 0;
            }

            nv21_roi_x = roi_y;
            nv21_roi_y = roi_x;
            nv21_roi_w = roi_h;
            nv21_roi_h = roi_w;
        }

        if (camera_facing == 0)
        {
            if (camera_orientation == 0 && accelerometer_orientation == 0)
            {
                rotate_type = 2;
            }
            if (camera_orientation == 0 && accelerometer_orientation == 90)
            {
                rotate_type = 7;
            }
            if (camera_orientation == 0 && accelerometer_orientation == 180)
            {
                rotate_type = 4;
            }
            if (camera_orientation == 0 && accelerometer_orientation == 270)
            {
                rotate_type = 5;
            }
            if (camera_orientation == 90 && accelerometer_orientation == 0)
            {
                rotate_type = 5;
            }
            if (camera_orientation == 90 && accelerometer_orientation == 90)
            {
                rotate_type = 2;
            }
            if (camera_orientation == 90 && accelerometer_orientation == 180)
            {
                rotate_type = 7;
            }
            if (camera_orientation == 90 && accelerometer_orientation == 270)
            {
                rotate_type = 4;
            }
            if (camera_orientation == 180 && accelerometer_orientation == 0)
            {
                rotate_type = 4;
            }
            if (camera_orientation == 180 && accelerometer_orientation == 90)
            {
                rotate_type = 5;
            }
            if (camera_orientation == 180 && accelerometer_orientation == 180)
            {
                rotate_type = 2;
            }
            if (camera_orientation == 180 && accelerometer_orientation == 270)
            {
                rotate_type = 7;
            }
            if (camera_orientation == 270 && accelerometer_orientation == 0)
            {
                rotate_type = 7;
            }
            if (camera_orientation == 270 && accelerometer_orientation == 90)
            {
                rotate_type = 4;
            }
            if (camera_orientation == 270 && accelerometer_orientation == 180)
            {
                rotate_type = 5;
            }
            if (camera_orientation == 270 && accelerometer_orientation == 270)
            {
                rotate_type = 2;
            }
        }
        else
        {
            if (final_orientation == 0)
            {
                rotate_type = 1;
            }
            if (final_orientation == 90)
            {
                rotate_type = 6;
            }
            if (final_orientation == 180)
            {
                rotate_type = 3;
            }
            if (final_orientation == 270)
            {
                rotate_type = 8;
            }
        }

        if (accelerometer_orientation == 0)
        {
            render_w = roi_w;
            render_h = roi_h;
            render_rotate_type = 1;
        }
        if (accelerometer_orientation == 90)
        {
            render_w = roi_h;
            render_h = roi_w;
            render_rotate_type = 8;
        }
        if (accelerometer_orientation == 180)
        {
            render_w = roi_w;
            render_h = roi_h;
            render_rotate_type = 3;
        }
        if (accelerometer_orientation == 270)
        {
            render_w = roi_h;
            render_h = roi_w;
            render_rotate_type = 6;
        }
    }

    // crop and rotate nv21
    cv::Mat nv21_croprotated(roi_h + roi_h / 2, roi_w, CV_8UC1);
    {
        const unsigned char* srcY = nv21 + nv21_roi_y * nv21_width + nv21_roi_x;
        unsigned char* dstY = nv21_croprotated.data;
        image_utils::kanna_rotate_c1(srcY, nv21_roi_w, nv21_roi_h, nv21_width, dstY, roi_w, roi_h, roi_w, rotate_type);

        const unsigned char* srcUV = nv21 + nv21_width * nv21_height + nv21_roi_y * nv21_width / 2 + nv21_roi_x;
        unsigned char* dstUV = nv21_croprotated.data + roi_w * roi_h;
        image_utils::kanna_rotate_c2(srcUV, nv21_roi_w / 2, nv21_roi_h / 2, nv21_width, dstUV, roi_w / 2, roi_h / 2, roi_w, rotate_type);
    }

    // nv21_croprotated to rgb
    cv::Mat rgb(roi_h, roi_w, CV_8UC3);
    image_utils::yuv420sp2rgb(nv21_croprotated.data, roi_w, roi_h, rgb.data);

    on_image_render(rgb);

    // rotate to native window orientation
    cv::Mat rgb_render(render_h, render_w, CV_8UC3);
    image_utils::kanna_rotate_c3(rgb.data, roi_w, roi_h, rgb_render.data, render_w, render_h, render_rotate_type);

    ANativeWindow_setBuffersGeometry(win, render_w, render_h, AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM);

    ANativeWindow_Buffer buf;
    int lock_ret = ANativeWindow_lock(win, &buf, NULL);
    if (lock_ret != 0) {
        LOGGW("on_image: ANativeWindow_lock failed ret=%d win=%p", lock_ret, win);
        return;
    }

    // scale to target size
    if (buf.format == AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM || buf.format == AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM)
    {
        for (int y = 0; y < render_h; y++)
        {
            const unsigned char* ptr = rgb_render.ptr<const unsigned char>(y);
            unsigned char* outptr = (unsigned char*)buf.bits + buf.stride * 4 * y;

            int x = 0;
#if __ARM_NEON
            for (; x + 7 < render_w; x += 8)
            {
                uint8x8x3_t _rgb = vld3_u8(ptr);
                uint8x8x4_t _rgba;
                _rgba.val[0] = _rgb.val[0];
                _rgba.val[1] = _rgb.val[1];
                _rgba.val[2] = _rgb.val[2];
                _rgba.val[3] = vdup_n_u8(255);
                vst4_u8(outptr, _rgba);

                ptr += 24;
                outptr += 32;
            }
#endif // __ARM_NEON
            for (; x < render_w; x++)
            {
                outptr[0] = ptr[0];
                outptr[1] = ptr[1];
                outptr[2] = ptr[2];
                outptr[3] = 255;

                ptr += 3;
                outptr += 4;
            }
        }
    }

    ANativeWindow_unlockAndPost(win);

    static int render_count = 0;
    render_count++;
    if (render_count <= 5 || render_count % 100 == 0) {
        LOGGD("on_image: render ok frame %d win=%p %dx%d", render_count, win, render_w, render_h);
    }
}
