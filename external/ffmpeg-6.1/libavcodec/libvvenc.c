/*
 * H.266 encoding using the VVenC library
 *
 * Copyright (C) 2022, Thomas Siedel
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config_components.h"

#include <vvenc/vvenc.h>
#include <vvenc/vvencCfg.h>
#include <vvenc/version.h>

#include "avcodec.h"
#include "codec_internal.h"
#include "encode.h"
#include "internal.h"
#include "packet_internal.h"
#include "profiles.h"

#include "libavutil/avutil.h"
#include "libavutil/pixdesc.h"
#include "libavutil/opt.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/frame.h"
#include "libavutil/log.h"

typedef struct VVenCOptions {
    int preset;                 // preset 0: faster  4: slower
    int qp;                     // quantization parameter 0-63
    int subjectiveOptimization; // perceptually motivated QP adaptation, XPSNR based
    int flag8bitCoding;         // encode in 8bit instead of 10bit
    int intraRefreshSec;        // intra period/refresh in seconds
    int levelIdc;               // vvc level_idc
    int tier;                   // vvc tier
    AVDictionary *vvenc_opts;
} VVenCOptions;

typedef struct VVenCContext {
    AVClass         *av_class;
    VVenCOptions    options;      // encoder options
    vvencEncoder    *vvencEnc;
    vvencAccessUnit *pAU;
    bool            encodeDone;
} VVenCContext;


static av_cold void ff_vvenc_log_callback(void *avctx, int level,
                                          const char *fmt, va_list args)
{
    vfprintf(level == 1 ? stderr : stdout, fmt, args);
}

static void ff_vvenc_internalLog(void *ctx, int level, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    ff_vvenc_log_callback(ctx, level, fmt, args);
    va_end(args);
}

static av_cold int ff_vvenc_encode_init(AVCodecContext *avctx)
{
    int ret;
    int framerate, qp, parse_ret;
    VVenCContext *s;
    vvenc_config params;
    vvencPresetMode preset;
    AVDictionaryEntry *en;
    char statsfile[1024] = "vvenc-rcstats.json";

    LOGGD("H266 encoder init\n");
    s = (VVenCContext *) avctx->priv_data;
    qp = (vvencPresetMode) s->options.qp;
    preset = (vvencPresetMode) s->options.preset;

    if (avctx->flags & AV_CODEC_FLAG_INTERLACED_DCT) {
        av_log(avctx, AV_LOG_ERROR,
               "ff_vvenc_encode_init::init() interlaced encoding not supported yet\n");
        return AVERROR_INVALIDDATA;
    }

    vvenc_config_default(&params);

    // set desired encoding options
    framerate = avctx->time_base.den / avctx->time_base.num;
    vvenc_init_default(&params, avctx->width, avctx->height, framerate,
                       (qp >= 0) ? 0 : avctx->bit_rate, (qp < 0) ? 32 : qp, preset);
    params.m_FrameRate = avctx->time_base.den;
    params.m_FrameScale = avctx->time_base.num;

    params.m_verbosity = VVENC_VERBOSE;
    if (av_log_get_level() >= AV_LOG_DEBUG)
        params.m_verbosity = VVENC_DETAILS;
    else if (av_log_get_level() >= AV_LOG_VERBOSE)
        params.m_verbosity = VVENC_NOTICE;      // output per picture info
    else if (av_log_get_level() >= AV_LOG_INFO)
        params.m_verbosity = VVENC_WARNING;     // ffmpeg default ffmpeg loglevel
    else
        params.m_verbosity = VVENC_SILENT;

FF_DISABLE_DEPRECATION_WARNINGS

#if FF_API_TICKS_PER_FRAME
    if (avctx->ticks_per_frame == 1) {
#endif
        params.m_TicksPerSecond = -1;   // auto mode for ticks per frame = 1
#if FF_API_TICKS_PER_FRAME
    } else {
        params.m_TicksPerSecond =
            ceil((avctx->time_base.den / (double) avctx->time_base.num) *
                 (double) avctx->ticks_per_frame);
    }
#endif
FF_ENABLE_DEPRECATION_WARNINGS

    if (avctx->thread_count > 0)
        params.m_numThreads = avctx->thread_count;

    // GOP settings (IDR/CRA)
    if (avctx->flags & AV_CODEC_FLAG_CLOSED_GOP)
        params.m_DecodingRefreshType = VVENC_DRT_IDR;

    if (avctx->gop_size == 1) {
        params.m_GOPSize = 1;
        params.m_IntraPeriod = 1;
    } else {
        params.m_IntraPeriodSec = s->options.intraRefreshSec;
    }

    params.m_usePerceptQPA = s->options.subjectiveOptimization;
    params.m_level         = (vvencLevel) s->options.levelIdc;
    params.m_levelTier     = (vvencTier) s->options.tier;

    params.m_AccessUnitDelimiter = true;

    params.m_internChromaFormat = VVENC_CHROMA_420;
    params.m_inputBitDepth[0]   = 10;

    if ( avctx->pix_fmt != AV_PIX_FMT_YUV420P10LE ){
        av_log(avctx, AV_LOG_ERROR,
               "unsupported pixel format %s, currently only support for yuv420p10le\n",
                av_get_pix_fmt_name(avctx->pix_fmt));
        return AVERROR(EINVAL);
    }

    if ( s->options.flag8bitCoding ) {
#if VVENC_VERSION_MAJOR > 1 || (VVENC_VERSION_MAJOR == 1 && VVENC_VERSION_MINOR > 9) || (VVENC_VERSION_MAJOR == 1 && VVENC_VERSION_MINOR >= 9 && VVENC_VERSION_PATCH >= 1)
        params.m_internalBitDepth[0] = 8;
#else
        av_log(avctx, AV_LOG_ERROR,
                "unsupported 8bit coding mode. 8bit coding needs at least vvenc version >= 1.9.1\n",
                av_get_pix_fmt_name(avctx->pix_fmt));
        return AVERROR(EINVAL);
#endif
    }

    if (avctx->color_primaries != AVCOL_PRI_UNSPECIFIED)
        params.m_colourPrimaries = (int) avctx->color_primaries;
    if (avctx->colorspace != AVCOL_SPC_UNSPECIFIED)
        params.m_matrixCoefficients = (int) avctx->colorspace;
    if (avctx->color_trc != AVCOL_TRC_UNSPECIFIED) {
        params.m_transferCharacteristics = (int) avctx->color_trc;

        if (avctx->color_trc == AVCOL_TRC_SMPTE2084)
            params.m_HdrMode = (avctx->color_primaries == AVCOL_PRI_BT2020) ?
                VVENC_HDR_PQ_BT2020 : VVENC_HDR_PQ;
        else if (avctx->color_trc == AVCOL_TRC_BT2020_10
                 || avctx->color_trc == AVCOL_TRC_ARIB_STD_B67)
            params.m_HdrMode = (avctx->color_trc == AVCOL_TRC_BT2020_10 ||
                                avctx->color_primaries == AVCOL_PRI_BT2020 ||
                                avctx->colorspace == AVCOL_SPC_BT2020_NCL ||
                                avctx->colorspace == AVCOL_SPC_BT2020_CL) ?
                               VVENC_HDR_HLG_BT2020 : VVENC_HDR_HLG;
    }

    if (params.m_HdrMode == VVENC_HDR_OFF
        && (avctx->color_primaries != AVCOL_PRI_UNSPECIFIED
            || avctx->colorspace != AVCOL_SPC_UNSPECIFIED)) {
        params.m_vuiParametersPresent = 1;
        params.m_colourDescriptionPresent = true;
    }

    params.m_RCNumPasses = 1;
    en = NULL;
    while ((en = av_dict_get(s->options.vvenc_opts, "", en,
                             AV_DICT_IGNORE_SUFFIX))) {
        av_log(avctx, AV_LOG_DEBUG, "vvenc_set_param: '%s:%s'\n", en->key,
               en->value);
        parse_ret = vvenc_set_param(&params, en->key, en->value);
        switch (parse_ret) {
        case VVENC_PARAM_BAD_NAME:
            av_log(avctx, AV_LOG_WARNING, "Unknown vvenc option: %s.\n",
                   en->key);
            break;
        case VVENC_PARAM_BAD_VALUE:
            av_log(avctx, AV_LOG_WARNING,
                   "Invalid vvenc value for %s: %s.\n", en->key, en->value);
            break;
        default:
            break;
        }

        if (memcmp(en->key, "rcstatsfile", 11) == 0 ||
            memcmp(en->key, "RCStatsFile", 11) == 0) {
            strncpy(statsfile, en->value, sizeof(statsfile) - 1);
            statsfile[sizeof(statsfile) - 1] = '\0';
        }
    }

    if (params.m_RCPass != -1 && params.m_RCNumPasses == 1)
        params.m_RCNumPasses = 2;       // enable 2pass mode

#if VVENC_VERSION_MAJOR > 1 || (VVENC_VERSION_MAJOR == 1 && VVENC_VERSION_MINOR > 8)
    if(avctx->rc_max_rate) {
        if(!avctx->bit_rate) {
            av_log( avctx, AV_LOG_ERROR, "Rate control parameters set without a bitrate\n");
            return AVERROR(EINVAL);
        }
        else
            params.m_RCMaxBitrate = avctx->rc_max_rate;
    }
#endif

    s->vvencEnc = vvenc_encoder_create();
    if (NULL == s->vvencEnc) {
        av_log(avctx, AV_LOG_ERROR, "cannot create vvc encoder (vvenc)\n");
        return AVERROR(ENOMEM);
    }

    vvenc_set_msg_callback(&params, s->vvencEnc, ff_vvenc_log_callback);
    ret = vvenc_encoder_open(s->vvencEnc, &params);
    if (0 != ret) {
        av_log(avctx, AV_LOG_ERROR, "cannot open vvc encoder (vvenc): %s\n",
               vvenc_get_last_error(s->vvencEnc));
        vvenc_encoder_close(s->vvencEnc);
        return AVERROR(EINVAL);
    }

    vvenc_get_config(s->vvencEnc, &params);     // get the adapted config

    if (params.m_verbosity >= VVENC_INFO
        && av_log_get_level() <= AV_LOG_INFO) {
        ff_vvenc_internalLog(avctx, params.m_verbosity, "vvenc version: %s\n",
                             vvenc_get_version());
        ff_vvenc_internalLog(avctx, params.m_verbosity, "%s\n",
                             vvenc_get_config_as_string(&params,
                                                        params.m_verbosity));
    } else {
        vvencMsgLevel loglvl = VVENC_INFO;
        if (av_log_get_level() >= AV_LOG_DEBUG)
            loglvl = VVENC_DETAILS;
        else if (av_log_get_level() >= AV_LOG_VERBOSE)
            loglvl = VVENC_VERBOSE;

        av_log(avctx, av_log_get_level(), "vvenc version: %s\n",
              vvenc_get_version());
        av_log(avctx, av_log_get_level(), "%s\n",
              vvenc_get_config_as_string(&params, loglvl ));
    }

    if (params.m_RCNumPasses == 2) {
        ret = vvenc_init_pass(s->vvencEnc, params.m_RCPass - 1, &statsfile[0]);
        if (0 != ret) {
            av_log(avctx, AV_LOG_ERROR,
                   "cannot init pass %d for vvc encoder (vvenc): %s\n",
                   params.m_RCPass, vvenc_get_last_error(s->vvencEnc));
            vvenc_encoder_close(s->vvencEnc);
            return AVERROR(EINVAL);
        }
    }

    s->pAU = vvenc_accessUnit_alloc();
    vvenc_accessUnit_alloc_payload(s->pAU, avctx->width * avctx->height);

    if (avctx->flags & AV_CODEC_FLAG_GLOBAL_HEADER) {
        ret = vvenc_get_headers(s->vvencEnc, s->pAU);
        if (0 != ret) {
            av_log(avctx, AV_LOG_ERROR,
                   "cannot get headers (SPS,PPS) from vvc encoder(vvenc): %s\n",
                   vvenc_get_last_error(s->vvencEnc));
            vvenc_encoder_close(s->vvencEnc);
            return AVERROR(EINVAL);
        }

        if (s->pAU->payloadUsedSize <= 0) {
            vvenc_encoder_close(s->vvencEnc);
            return AVERROR_INVALIDDATA;
        }

        avctx->extradata_size = s->pAU->payloadUsedSize;
        avctx->extradata =
            av_malloc(avctx->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE);
        if (!avctx->extradata) {
            av_log(avctx, AV_LOG_ERROR,
                   "Cannot allocate VVC header of size %d.\n",
                   avctx->extradata_size);
            vvenc_encoder_close(s->vvencEnc);
            return AVERROR(ENOMEM);
        }

        memcpy(avctx->extradata, s->pAU->payload, avctx->extradata_size);
        memset(avctx->extradata + avctx->extradata_size, 0,
               AV_INPUT_BUFFER_PADDING_SIZE);
    }
    s->encodeDone = false;
    return 0;
}

static av_cold int ff_vvenc_encode_close(AVCodecContext * avctx)
{
    VVenCContext *s = (VVenCContext *) avctx->priv_data;
    LOGGD("enter H266 encoder deinit\n");
    if (s->vvencEnc) {
        if (av_log_get_level() >= AV_LOG_VERBOSE)
            vvenc_print_summary(s->vvencEnc);

        if (0 != vvenc_encoder_close(s->vvencEnc)) {
            av_log(avctx, AV_LOG_ERROR, "cannot close vvenc\n");
            return -1;
        }
    }

    vvenc_accessUnit_free(s->pAU, true);
    LOGGD("leave H266 encoder deinit\n");

    return 0;
}

static av_cold int ff_vvenc_encode_frame(AVCodecContext *avctx, AVPacket *pkt,
                                         const AVFrame *frame, int *got_packet)
{
    VVenCContext *s = (VVenCContext *) avctx->priv_data;
    vvencYUVBuffer *pyuvbuf;
    vvencYUVBuffer yuvbuf;
    int pict_type;
    int ret;

    pyuvbuf = NULL;
    if (frame) {
        if (avctx->pix_fmt == AV_PIX_FMT_YUV420P10LE) {
            vvenc_YUVBuffer_default(&yuvbuf);
            yuvbuf.planes[0].ptr = (int16_t *) frame->data[0];
            yuvbuf.planes[1].ptr = (int16_t *) frame->data[1];
            yuvbuf.planes[2].ptr = (int16_t *) frame->data[2];

            yuvbuf.planes[0].width = frame->width;
            yuvbuf.planes[0].height = frame->height;
            yuvbuf.planes[0].stride = frame->linesize[0] >> 1;  // stride is used in samples (16bit) in vvenc, ffmpeg uses stride in bytes

            yuvbuf.planes[1].width = frame->width >> 1;
            yuvbuf.planes[1].height = frame->height >> 1;
            yuvbuf.planes[1].stride = frame->linesize[1] >> 1;

            yuvbuf.planes[2].width = frame->width >> 1;
            yuvbuf.planes[2].height = frame->height >> 1;
            yuvbuf.planes[2].stride = frame->linesize[2] >> 1;

            yuvbuf.cts = frame->pts;
            yuvbuf.ctsValid = true;
            pyuvbuf = &yuvbuf;
        } else {
            av_log(avctx, AV_LOG_ERROR,
                   "unsupported input colorspace! input must be yuv420p10le");
            return AVERROR(EINVAL);
        }
    }

    if (!s->encodeDone) {
        ret = vvenc_encode(s->vvencEnc, pyuvbuf, s->pAU, &s->encodeDone);
        if (ret != 0) {
            av_log(avctx, AV_LOG_ERROR, "error in vvenc::encode - ret:%d\n",
                   ret);
            return AVERROR(EINVAL);
        }
    } else {
        *got_packet = 0;
        return 0;
    }

    if (s->pAU->payloadUsedSize > 0) {
        ret = ff_get_encode_buffer(avctx, pkt, s->pAU->payloadUsedSize, 0);
        if (ret < 0) {
            av_log(avctx, AV_LOG_ERROR, "Error getting output packet.\n");
            return ret;
        }

        memcpy(pkt->data, s->pAU->payload, s->pAU->payloadUsedSize);

        if (s->pAU->ctsValid)
            pkt->pts = s->pAU->cts;
        if (s->pAU->dtsValid)
            pkt->dts = s->pAU->dts;
        pkt->flags |= AV_PKT_FLAG_KEY * s->pAU->rap;

        switch (s->pAU->sliceType) {
        case VVENC_I_SLICE:
            pict_type = AV_PICTURE_TYPE_I;
            break;
        case VVENC_P_SLICE:
            pict_type = AV_PICTURE_TYPE_P;
            break;
        case VVENC_B_SLICE:
            pict_type = AV_PICTURE_TYPE_B;
            break;
        default:
            av_log(avctx, AV_LOG_ERROR, "Unknown picture type encountered.\n");
            return AVERROR_EXTERNAL;
        }

        ff_side_data_set_encoder_stats(pkt, 0, NULL, 0, pict_type);

        *got_packet = 1;

        return 0;
    } else {
        *got_packet = 0;
        return 0;
    }

    return 0;
}

static const enum AVPixelFormat pix_fmts_vvenc[] = {
    AV_PIX_FMT_YUV420P10LE,
    AV_PIX_FMT_NONE
};

#define OFFSET(x) offsetof(VVenCContext, x)
#define VE AV_OPT_FLAG_VIDEO_PARAM | AV_OPT_FLAG_ENCODING_PARAM
static const AVOption libvvenc_options[] = {
    {"preset", "set encoding preset(0: faster - 4: slower", OFFSET( options.preset), AV_OPT_TYPE_INT, {.i64 = 2} , 0 , 4 , VE, "preset"},
        { "faster", "0", 0, AV_OPT_TYPE_CONST, {.i64 = VVENC_FASTER}, INT_MIN, INT_MAX, VE, "preset" },
        { "fast",   "1", 0, AV_OPT_TYPE_CONST, {.i64 = VVENC_FAST},   INT_MIN, INT_MAX, VE, "preset" },
        { "medium", "2", 0, AV_OPT_TYPE_CONST, {.i64 = VVENC_MEDIUM}, INT_MIN, INT_MAX, VE, "preset" },
        { "slow",   "3", 0, AV_OPT_TYPE_CONST, {.i64 = VVENC_SLOW},   INT_MIN, INT_MAX, VE, "preset" },
        { "slower", "4", 0, AV_OPT_TYPE_CONST, {.i64 = VVENC_SLOWER}, INT_MIN, INT_MAX, VE, "preset" },
    { "qp"     , "set quantization", OFFSET(options.qp), AV_OPT_TYPE_INT,  {.i64 = -1}, -1 , 63 ,VE, "qp_mode" },
    { "period" , "set (intra) refresh period in seconds", OFFSET(options.intraRefreshSec), AV_OPT_TYPE_INT,  {.i64 = 1},  1 , INT_MAX ,VE,"irefreshsec" },
    { "subjopt", "set subjective (perceptually motivated) optimization", OFFSET(options.subjectiveOptimization), AV_OPT_TYPE_BOOL, {.i64 = 1},  0 , 1, VE},
    { "bitdepth8", "set 8bit coding mode", OFFSET(options.flag8bitCoding), AV_OPT_TYPE_BOOL, {.i64 = 0},  0 , 1, VE},
    { "vvenc-params", "set the vvenc configuration using a :-separated list of key=value parameters", OFFSET(options.vvenc_opts), AV_OPT_TYPE_DICT, { 0 }, 0, 0, VE },
    { "levelidc", "vvc level_idc", OFFSET( options.levelIdc), AV_OPT_TYPE_INT, {.i64 = 0}, 0, 105, VE, "levelidc"},
        { "0",   "auto", 0, AV_OPT_TYPE_CONST, {.i64 = 0},  INT_MIN, INT_MAX, VE, "levelidc"},
        { "1",   "1"   , 0, AV_OPT_TYPE_CONST, {.i64 = 16}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "2",   "2"   , 0, AV_OPT_TYPE_CONST, {.i64 = 32}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "2.1", "2.1" , 0, AV_OPT_TYPE_CONST, {.i64 = 35}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "3",   "3"   , 0, AV_OPT_TYPE_CONST, {.i64 = 48}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "3.1", "3.1" , 0, AV_OPT_TYPE_CONST, {.i64 = 51}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "4",   "4"   , 0, AV_OPT_TYPE_CONST, {.i64 = 64}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "4.1", "4.1" , 0, AV_OPT_TYPE_CONST, {.i64 = 67}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "5",   "5"   , 0, AV_OPT_TYPE_CONST, {.i64 = 80}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "5.1", "5.1" , 0, AV_OPT_TYPE_CONST, {.i64 = 83}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "5.2", "5.2" , 0, AV_OPT_TYPE_CONST, {.i64 = 86}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "6",   "6"   , 0, AV_OPT_TYPE_CONST, {.i64 = 96}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "6.1", "6.1" , 0, AV_OPT_TYPE_CONST, {.i64 = 99}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "6.2", "6.2" , 0, AV_OPT_TYPE_CONST, {.i64 = 102}, INT_MIN, INT_MAX, VE, "levelidc"},
        { "6.3", "6.3" , 0, AV_OPT_TYPE_CONST, {.i64 = 105}, INT_MIN, INT_MAX, VE, "levelidc"},
    { "tier", "set vvc tier", OFFSET( options.tier), AV_OPT_TYPE_INT, {.i64 = 0},  0 , 1 , VE, "tier"},
        { "main", "main", 0, AV_OPT_TYPE_CONST, {.i64 = 0}, INT_MIN, INT_MAX, VE, "tier"},
        { "high", "high", 0, AV_OPT_TYPE_CONST, {.i64 = 1}, INT_MIN, INT_MAX, VE, "tier"},
    {NULL}
};

static const AVClass class_libvvenc = {
    .class_name = "libvvenc-vvc encoder",
    .item_name  = av_default_item_name,
    .option     = libvvenc_options,
    .version    = LIBAVUTIL_VERSION_INT,
};

FFCodec ff_libvvenc_encoder = {
    .p.name         = "libvvenc",
    CODEC_LONG_NAME("H.266 / VVC Encoder VVenC"),
    .p.type         = AVMEDIA_TYPE_VIDEO,
    .p.id           = AV_CODEC_ID_VVC,
    .p.capabilities = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_OTHER_THREADS,
    .p.profiles     = NULL_IF_CONFIG_SMALL(ff_vvc_profiles),
    .p.priv_class   = &class_libvvenc,
    .p.wrapper_name = "libvvenc",
    .priv_data_size = sizeof(VVenCContext),
    .p.pix_fmts     = pix_fmts_vvenc,
    .init           = ff_vvenc_encode_init,
    FF_CODEC_ENCODE_CB(ff_vvenc_encode_frame),
    .close          = ff_vvenc_encode_close,
    .caps_internal  = FF_CODEC_CAP_AUTO_THREADS,
};
