#pragma once

#include "ndkcamera.h"

//opencv-android
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"

class MyNdkCamera : public NdkCameraWindow {
public:
    virtual void on_image_render(cv::Mat & rgb) const;
};
