### input data and extract output
```cpp
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "net.h"

int main()
{
    cv::Mat img = cv::imread("image.ppm", CV_LOAD_IMAGE_GRAYSCALE);
    int w = img.cols;
    int h = img.rows;

    // subtract 128, norm to -1 ~ 1
    ncnn::Mat in = ncnn::Mat::from_pixels_resize(img.data, ncnn::Mat::PIXEL_GRAY, w, h, 60, 60);
    float mean[1] = { 128.f };
    float norm[1] = { 1/128.f };
    in.substract_mean_normalize(mean, norm);

    ncnn::Net net;
    net.load_param("model.param");
    net.load_model("model.bin");

    ncnn::Extractor ex = net.create_extractor();

    ex.input("data", in);

    ncnn::Mat feat;
    ex.extract("output", feat);

    return 0;
}

```

### print Mat content
```cpp
void pretty_print(const ncnn::Mat& m)
{
    for (int q=0; q<m.c; q++)
    {
        const float* ptr = m.channel(q);
        for (int z=0; z<m.d; z++)
        {
            for (int y=0; y<m.h; y++)
            {
                for (int x=0; x<m.w; x++)
                {
                    printf("%f ", ptr[x]);
                }
                ptr += m.w;
                printf("\n");
            }
            printf("\n");
        }
        printf("------------------------\n");
    }
}
```

### visualize Mat content
```cpp
void visualize(const char* title, const ncnn::Mat& m)
{
    std::vector<cv::Mat> normed_feats(m.c);

    for (int i=0; i<m.c; i++)
    {
        cv::Mat tmp(m.h, m.w, CV_32FC1, (void*)(const float*)m.channel(i));

        cv::normalize(tmp, normed_feats[i], 0, 255, cv::NORM_MINMAX, CV_8U);

        cv::cvtColor(normed_feats[i], normed_feats[i], cv::COLOR_GRAY2BGR);

        // check NaN
        for (int y=0; y<m.h; y++)
        {
            const float* tp = tmp.ptr<float>(y);
            uchar* sp = normed_feats[i].ptr<uchar>(y);
            for (int x=0; x<m.w; x++)
            {
                float v = tp[x];
                if (v != v)
                {
                    sp[0] = 0;
                    sp[1] = 0;
                    sp[2] = 255;
                }

                sp += 3;
            }
        }
    }

    int tw = m.w < 10 ? 32 : m.w < 20 ? 16 : m.w < 40 ? 8 : m.w < 80 ? 4 : m.w < 160 ? 2 : 1;
    int th = (m.c - 1) / tw + 1;

    cv::Mat show_map(m.h * th, m.w * tw, CV_8UC3);
    show_map = cv::Scalar(127);

    // tile
    for (int i=0; i<m.c; i++)
    {
        int ty = i / tw;
        int tx = i % tw;

        normed_feats[i].copyTo(show_map(cv::Rect(tx * m.w, ty * m.h, m.w, m.h)));
    }

    cv::resize(show_map, show_map, cv::Size(0,0), 2, 2, cv::INTER_NEAREST);
    cv::imshow(title, show_map);
}
```