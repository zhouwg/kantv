#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "libavutil/cde_log.h"
#include "ggml-jni.h"

#pragma pack(push, 1)

typedef unsigned char  U8;
typedef unsigned short U16;
typedef unsigned int   U32;

typedef struct tagBITMAPFILEHEADER {
    U16 bfType;
    U32 bfSize;
    U16 bfReserved1;
    U16 bfReserved2;
    U32 bfOffBits;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER {
    U32 biSize;
    U32 biWidth;
    U32 biHeight;
    U16 biPlanes;
    U16 biBitCount;
    U32 biCompression;
    U32 biSizeImage;
    U32 biXPelsPerMeter;
    U32 biYPelsPerMeter;
    U32 biClrUsed;
    U32 biClrImportant;
} BITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    U8 rgbBlue;
    U8 rgbGreen;
    U8 rgbRed;
    U8 rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[1];
} BITMAPINFO;


typedef struct tagBITMAP {
    BITMAPFILEHEADER bfHeader;
    BITMAPINFO biInfo;
}BITMAPFILE;

#pragma pack(pop)

int write_bmp(const char * filename, int width, int height, int bpp, const unsigned char * data) {
    FILE * fp = fopen(filename, "wb");
    if (!fp) {
        LOGGD("can't open file %s to write, reason %s\n", filename, strerror(errno));
        return 1;
    }

    int h = 0;
    int w = 0;
    U32 line_pitch  = ((width * bpp + 31) >> 5) << 2;
    U32 file_size   = line_pitch * height;

    BITMAPFILE bmpfile;

    bmpfile.bfHeader.bfType                 = 0x4D42;
    bmpfile.bfHeader.bfSize                 = file_size + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmpfile.bfHeader.bfReserved1            = 0;
    bmpfile.bfHeader.bfReserved2            = 0;
    bmpfile.bfHeader.bfOffBits              = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmpfile.biInfo.bmiHeader.biSize         = sizeof(BITMAPINFOHEADER);
    bmpfile.biInfo.bmiHeader.biWidth        = width;
    bmpfile.biInfo.bmiHeader.biHeight       = height;
    bmpfile.biInfo.bmiHeader.biPlanes       = 1;
    bmpfile.biInfo.bmiHeader.biBitCount     = bpp;
    bmpfile.biInfo.bmiHeader.biCompression  = 0;
    bmpfile.biInfo.bmiHeader.biSizeImage    = 0;
    bmpfile.biInfo.bmiHeader.biXPelsPerMeter= 0;
    bmpfile.biInfo.bmiHeader.biYPelsPerMeter= 0;
    bmpfile.biInfo.bmiHeader.biClrUsed      = 0;
    bmpfile.biInfo.bmiHeader.biClrImportant = 0;

    fwrite(&(bmpfile.bfHeader), sizeof(BITMAPFILEHEADER), 1, fp);
    fwrite(&(bmpfile.biInfo.bmiHeader), sizeof(BITMAPINFOHEADER), 1, fp);

    U8 * p_linebuf = (U8*)malloc(line_pitch);
    if (NULL == p_linebuf) {
        LOGGD("malloc failed");
        fclose(fp);
        return 2;
    }

    memset(p_linebuf, 0, line_pitch);
    U8 byte_per_pixel   = bpp >> 3;
    U32 pitch           = width * byte_per_pixel;


    for (h = height - 1; h >= 0; h--) {
        for (w = 0; w < width; w++) {
            p_linebuf[w * byte_per_pixel + 0] = data[h * pitch + w * byte_per_pixel + 0];
            p_linebuf[w * byte_per_pixel + 1] = data[h * pitch + w * byte_per_pixel + 1];
            p_linebuf[w * byte_per_pixel + 2] = data[h * pitch + w * byte_per_pixel + 2];
        }
        fwrite(p_linebuf, line_pitch, 1, fp);
    }

    free(p_linebuf);
    fclose(fp);

    return 0;
}