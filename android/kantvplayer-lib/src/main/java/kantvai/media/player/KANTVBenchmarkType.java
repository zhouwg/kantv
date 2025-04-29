 /*
  * Copyright (c) Project KanTV. 2021-2023
  *
  * Copyright (c) 2024- KanTV Authors
  *
  * Permission is hereby granted, free of charge, to any person obtaining a copy
  * of this software and associated documentation files (the "Software"), to
  * deal in the Software without restriction, including without limitation the
  * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
  * sell copies of the Software, and to permit persons to whom the Software is
  * furnished to do so, subject to the following conditions:
  *
  * The above copyright notice and this permission notice shall be included in
  * all copies or substantial portions of the Software.
  *
  * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  * IN THE SOFTWARE.
  */
package kantvai.media.player;


public class KANTVBenchmarkType
{
    public static final int BENCHMARK_H264     = 0;
    public static final int BENCHMARK_H265     = 1;
    public static final int BENCHMARK_H266     = 2;
    public static final int BENCHMARK_AOM_AV1  = 3;
    public static final int BENCHMARK_SVT_AV1  = 4;


    //borrow from FFmpeg
    public static final int AV_PIX_FMT_YUV420P = 0;   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
    public static final int AV_PIX_FMT_YUYV422 = 1;   ///< packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
    public static final int AV_PIX_FMT_RGB24 = 2;     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
    public static final int AV_PIX_FMT_BGR24 = 3;     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    public static final int AV_PIX_FMT_YUV422P = 4;   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
    public static final int AV_PIX_FMT_YUV444P = 5;   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
    public static final int AV_PIX_FMT_YUV410P = 6;  ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
    public static final int AV_PIX_FMT_YUV411P =7;   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
    public static final int AV_PIX_FMT_GRAY8 =8;     ///<        Y        ,  8bpp
    public static final int AV_PIX_FMT_MONOWHITE=9; ///<        Y        ,  1bpp, 0 is white, 1 is black, in each byte pixels are ordered from the msb to the lsb
    public static final int AV_PIX_FMT_MONOBLACK=10; ///<        Y        ,  1bpp, 0 is black, 1 is white, in each byte pixels are ordered from the msb to the lsb
    public static final int AV_PIX_FMT_PAL8=11;    ///< 8 bits with AV_PIX_FMT_RGB32 palette
    public static final int AV_PIX_FMT_YUVJ420P=12; ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV420P and setting color_range
    public static final int AV_PIX_FMT_YUVJ422P=13;///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV422P and setting color_range
    public static final int AV_PIX_FMT_YUVJ444P=14;  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of AV_PIX_FMT_YUV444P and setting color_range
    public static final int AV_PIX_FMT_UYVY422=15;   ///< packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
    public static final int AV_PIX_FMT_UYYVYY411=16;///< packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
    public static final int AV_PIX_FMT_BGR8=17;      ///< packed RGB 3:3:2,  8bpp, (msb)2B 3G 3R(lsb)
    public static final int AV_PIX_FMT_BGR4=18;   ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1B 2G 1R(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    public static final int AV_PIX_FMT_BGR4_BYTE=19; ///< packed RGB 1:2:1,  8bpp, (msb)1B 2G 1R(lsb)
    public static final int AV_PIX_FMT_RGB8=20;      ///< packed RGB 3:3:2,  8bpp, (msb)2R 3G 3B(lsb)
    public static final int AV_PIX_FMT_RGB4=21;   ///< packed RGB 1:2:1 bitstream,  4bpp, (msb)1R 2G 1B(lsb), a byte contains two pixels, the first pixel in the byte is the one composed by the 4 msb bits
    public static final int AV_PIX_FMT_RGB4_BYTE=22; ///< packed RGB 1:2:1,  8bpp, (msb)1R 2G 1B(lsb)
    public static final int AV_PIX_FMT_NV12=23;      ///< planar YUV 4:2:0, 12bpp, 1 plane for Y and 1 plane for the UV components, which are interleaved (first byte U and the following byte V)
    public static final int AV_PIX_FMT_NV21=24;      ///< as above, but U and V bytes are swapped

    public static final int AV_PIX_FMT_ARGB=25;     ///< packed ARGB 8:8:8:8, 32bpp, ARGBARGB...
    public static final int AV_PIX_FMT_RGBA=26;      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
    public static final int AV_PIX_FMT_ABGR=27;      ///< packed ABGR 8:8:8:8, 32bpp, ABGRABGR...
    public static final int AV_PIX_FMT_BGRA=28;      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...

    public static native int kantv_anti_remove_rename_this_file();
}
