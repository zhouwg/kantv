/*
 some codes in this file were referenced by codes in original FFmpeg

 Copyright (c) 2000-2003 Fabrice Bellard
 Copyright (c) 2000-2024 FFmpeg Authors

 Copyright (c) 2021-2023, zhouwg2000@gmail.com

 Copyright (c) 2021-2023, Project KanTV

 Copyright (c) 2024- KanTV Authors

 Description: study how to implement kantvrecord in Project KanTV, don't care memory leak currently

              step by step:

              step-1: x86,      if ok, then move to  (done)
              step-2: android,  if ok, then move to  (done)
              step-3: ios,      if ok, then move to  (partially done)
              step-4: wasm                           (partially done)
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavcodec/avcodec.h"
#include "libavutil/cde_log.h"
#include "libavutil/vkey.h"


//attention: this value is depend on machine's performance heavily
#define STREAM_FRAME_RATE   1

#define STREAM_DURATION     10.0

#define SCALE_FLAGS         SWS_BICUBIC

#define STREAM_PIX_FMT      AV_PIX_FMT_YUV420P /* default pix_fmt */

static int                  g_pattern       = 0;
static int                  g_codec         = 0;  //default codec is h264

static struct SwsContext    *g_sws_context  = NULL;

typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt) {

    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

#if 0
    LOGGV("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
#endif
}


static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt) {
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    log_packet(fmt_ctx, pkt);

    return av_interleaved_write_frame(fmt_ctx, pkt);
}


static void add_stream_by_codecid(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id) {
    int i;
    AVCodecContext *c;

    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        LOGGD( "Could not find encoder for '%s'\n", avcodec_get_name(codec_id));
        exit(1);
    } else {
        LOGGD("Using codec %s (%s).", (*codec)->name, (*codec)->long_name);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        LOGGD( "Could not allocate stream\n");
        exit(1);
    }

    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOGGD( "Could not alloc an encoding context\n");
        exit(1);
    }

    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;

        c->width    = 352;
        c->height   = 288;

        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size        = 1;
        c->pix_fmt         = STREAM_PIX_FMT;

        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            c->max_b_frames = 2;
        }

        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            c->mb_decision = 2;
        }
        break;

    default:
        break;
    }

    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}


static void add_stream_by_codecname(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       char *codec_name)
{
    AVCodecContext *c;
    int i;

    *codec = (AVCodec*) avcodec_find_encoder_by_name(codec_name);
    if (!(*codec)) {
        LOGGD( "Could not find encoder for '%s'\n", codec_name);
        exit(1);
    } else {
        LOGGD("Using codec %s (%s)", (*codec)->name, (*codec)->long_name);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        LOGGD( "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        LOGGD( "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 44100;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 44100)
                    c->sample_rate = 44100;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = (*codec)->id;

        c->bit_rate = 400000;

        c->width    = 352;
        c->height   = 288;

        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size      = 1;

        if (2 == g_codec) //make H266 encoder happy
            c->pix_fmt   = AV_PIX_FMT_YUV420P10LE;
        else
            c->pix_fmt   = AV_PIX_FMT_YUV420P;

        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            c->max_b_frames = 2;
        }

        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            c->mb_decision = 2;
        }

        if (4 == g_codec) //make Intel-AV1 encoder happy
            av_opt_set(c->priv_data, "crf", "35", 0);

        break;

    default:
        break;
    }

    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}


static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples) {
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        LOGGD( "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            LOGGD( "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}


static void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        LOGGD( "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    ost->t      = 0;
    ost->tincr  = 2 * M_PI * 110.0 / c->sample_rate;
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        LOGGD( "Could not copy the stream parameters\n");
        exit(1);
    }

    ost->swr_ctx = swr_alloc();
    if (!ost->swr_ctx) {
        LOGGD( "Could not allocate resampler context\n");
        exit(1);
    }

    av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    if ((ret = swr_init(ost->swr_ctx)) < 0) {
        LOGGD( "Failed to initialize the resampling context\n");
        exit(1);
    }
}


static AVFrame *get_audio_frame(OutputStream *ost) {
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}


static int write_audio_frame(AVFormatContext *oc, OutputStream *ost) {
    AVCodecContext *c;
    AVPacket pkt = { 0 };
    AVFrame *frame;
    int ret;
    int got_packet;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) {
        dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);
        av_assert0(dst_nb_samples == frame->nb_samples);

        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

            ret = swr_convert(ost->swr_ctx,
                              ost->frame->data, dst_nb_samples,
                              (const uint8_t **)frame->data, frame->nb_samples);
            if (ret < 0) {
                LOGGD( "Error while converting\n");
                exit(1);
            }
            frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        LOGGD( "Error encoding audio frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            LOGGD( "Error while writing audio frame: %s\n",
                    av_err2str(ret));
            exit(1);
        }
    }

    return (frame || got_packet) ? 0 : 1;
}



static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        LOGGD( "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}


static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg) {
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        LOGGD( "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        LOGGD( "Could not allocate video frame\n");
        exit(1);
    }

    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            LOGGD( "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        LOGGD( "Could not copy the stream parameters\n");
        exit(1);
    }
}


static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height) {
    int x, y, i, ret;

    ret = av_frame_make_writable(pict);
    if (ret < 0)
        exit(1);

    i = frame_index;

    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}


static void ffmpeg_encoder_set_frame_yuv_from_rgb(uint8_t *rgb, int width, int height, AVFrame *frame) {
    const int in_linesize[1] = { 3 * width };
    g_sws_context = sws_getCachedContext(g_sws_context,
            width, height, AV_PIX_FMT_RGB24,
            width, height, AV_PIX_FMT_YUV420P,
            0, 0, 0, 0);
    sws_scale(g_sws_context, (const uint8_t * const *)&rgb, in_linesize, 0,
            height, frame->data, frame->linesize);
}


static uint8_t* generate_rgb(AVFrame *frame, int width, int height, int pts, uint8_t *rgb) {
    int x, y, cur;
    rgb = realloc(rgb, 3 * sizeof(uint8_t) * height * width);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            cur = 3 * (y * width + x);
            rgb[cur + 0] = 0;
            rgb[cur + 1] = 0;
            rgb[cur + 2] = 0;
            if ((frame->pts / 25) % 2 == 0) {
                if (y < height / 2) {
                    if (x < width / 2) {
                    } else {
                        rgb[cur + 0] = 255;
                    }
                } else {
                    if (x < width / 2) {
                        rgb[cur + 1] = 255;
                    } else {
                        rgb[cur + 2] = 255;
                    }
                }
            } else {
                if (y < height / 2) {
                    rgb[cur + 0] = 255;
                    if (x < width / 2) {
                        rgb[cur + 1] = 255;
                    } else {
                        rgb[cur + 2] = 255;
                    }
                } else {
                    if (x < width / 2) {
                        rgb[cur + 1] = 255;
                        rgb[cur + 2] = 255;
                    } else {
                        rgb[cur + 0] = 255;
                        rgb[cur + 1] = 255;
                        rgb[cur + 2] = 255;
                    }
                }
            }
        }
    }

    return rgb;
}


static void fill_yuv_image2(AVFrame *pict, int frame_index, int width, int height) {
        pict->pts = frame_index;
        uint8_t *rgb = NULL;
        rgb = generate_rgb(pict, width, height, pict->pts, rgb);
        ffmpeg_encoder_set_frame_yuv_from_rgb(rgb, width, height, pict);
}


static AVFrame *get_video_frame(OutputStream *ost) {
    AVCodecContext *c = ost->enc;

    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                LOGGD("Could not initialize the conversion context\n");
                exit(1);
            }
        }

        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx,
                  (const uint8_t * const *)ost->tmp_frame->data, ost->tmp_frame->linesize,
                  0, c->height, ost->frame->data, ost->frame->linesize);
    } else {
        if (0 == g_pattern) {
            fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
        } else {
            fill_yuv_image2(ost->frame, ost->next_pts, c->width, c->height);
        }
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}


static int write_video_frame(AVFormatContext *oc, OutputStream *ost) {
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };

    c = ost->enc;

    frame = get_video_frame(ost);

    av_init_packet(&pkt);

    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        LOGGD( "Error encoding video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    } else {
        ret = 0;
    }

    if (ret < 0) {
        LOGGD( "Error while writing video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
}


static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}


static void showUsage()
{
    printf(" " \
        "\nUsage: ff_encode [options]\n" \
        "\n" \
        "Options:\n" \
        " -n <filename>   base file name of encoded file\n" \
        " -f <format>     container name(ts/mp4, codec webp is only valid for ts currently)\n" \
        " -c <0/1/2/3/4>  codec: 0 - h264 1 - h265 2 - h266 3 - AOM AV1 4 - Intel AV1  5 - webp\n" \
        " -p <0/1>        pattern: 0 1\n" \
        " ?/h print usage infomation\n\n"
    );
}


int main(int argc, char **argv) {
    OutputStream video_st   = { 0 };
    OutputStream audio_st   = { 0 };

    AVOutputFormat *fmt     = NULL;
    AVFormatContext *oc     = NULL;
    AVCodec *audio_codec    = NULL;
    AVCodec *video_codec    = NULL;
    AVDictionary *opt       = NULL;

    int have_video          = 0;
    int have_audio          = 0;
    int encode_video        = 0;
    int encode_audio        = 0;

    int i;
    int ret;

    int optArg              = 0;
    int codecid             = 0;
    char *format            = NULL;
    char *mainfilename      = NULL;
    char *codec_name        = "libx264";
    char filename[256]      = { 0 };

    av_register_all();
    av_log_set_level(AV_LOG_TRACE);

    //TODO: add -fflags +genpts to fix below issue
    //first pts value must be set av_interleaved_write_frame(): Invalid data found when processing input
    while (-1 != (optArg = getopt(argc, argv, "n:f:c:p:?h"))) {
        switch (optArg) {
            case 'n':
                mainfilename = optarg;
                LOGGD("mainfilename %s\n", mainfilename);
                break;

            case 'f':
                format = optarg;
                LOGGD("format %s\n", format);
                break;

            case 'p':
                g_pattern = atoi(optarg);
                if (g_pattern >= 2) {
                    g_pattern = 0;
                }
                LOGGD("g_pattern %d\n", g_pattern);
                break;

            case 'c':
                codecid  = atoi(optarg);
                g_codec  = codecid;

                LOGGD("codecid %d\n", codecid);
                switch (codecid) {
                    case 0:
                        codec_name = "libx264";
                        break;

                    case 1:
                        codec_name = "libx265";
                        break;

                    case 2:
                        codec_name = "libvvenc";
                        break;

                    case 3:
                        codec_name = "libaom-av1";
                        break;

                    case 4:
                        codec_name = "libsvtav1";
                        break;

                    case 5:
                        codec_name = "libwebp";
                        break;

                    default:
                        LOGGD("invalid codec id, default is libx264\n");
                        break;
                }
                LOGGD("codec_name %s\n", codec_name);
                break;

            case '?':
            case 'h':
                showUsage(); return 0;
            default:
                showUsage(); return -1;
        }
    }

    if ((NULL == mainfilename) || (NULL == format)) {
        showUsage();
        return 1;
    }

    memset(filename, 0, 256);
    snprintf(filename, 256, "%s.%s", mainfilename, format);
    LOGGD("encoded filename %s\n", filename);

    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        LOGGD("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    } else {
        LOGGD("Using format %s (%s)\n", oc->oformat->name, oc->oformat->long_name);
    }

    if (!oc) {
        LOGGD("error\n");
        return 1;
    }

    AVCodec *codec = (AVCodec*) avcodec_find_encoder_by_name(codec_name);
    if (NULL == codec) {
        LOGGD("can't find encoder, pls check why?\n");

        return 1;
    } else {
        LOGGD("find codec %s\n", codec_name);
        LOGGD("Using codec %s (%s,0x%x)", codec->name, codec->long_name, codec->id);
        AVCodecContext* codec_context =avcodec_alloc_context3(codec);
        LOGGD("codec_context=%p\n", codec_context);
    }

    fmt = oc->oformat;

    LOGGD("fmt->video_codec 0x%x\n", fmt->video_codec);
    LOGGD("video_codec=%p\n", video_codec);
    LOGGD("codec->id %d\n", codec->id);

    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream_by_codecname(&video_st, oc, &video_codec, codec_name);
        have_video = 1;
        encode_video = 1;
    }

    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        //don't care audio currently
        //add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio      = 0;
        encode_audio    = 0;
    }

    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    av_dump_format(oc, 0, filename, 1);

    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGGD("Could not open '%s': %s\n", filename, av_err2str(ret));
            return 1;
        }
    }

    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        LOGGD( "Error occurred when opening output file: %s\n", av_err2str(ret));
        return 1;
    }

    while (encode_video || encode_audio) {
        if (encode_video &&
            (!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,
                                            audio_st.next_pts, audio_st.enc->time_base) <= 0)) {
            encode_video = !write_video_frame(oc, &video_st);
        } else {
            encode_audio = !write_audio_frame(oc, &audio_st);
        }
    }

    av_write_trailer(oc);

    if (have_video)
        close_stream(oc, &video_st);

    if (have_audio)
        close_stream(oc, &audio_st);

    if (!(fmt->flags & AVFMT_NOFILE))
        avio_closep(&oc->pb);

    avformat_free_context(oc);

    return 0;
}
