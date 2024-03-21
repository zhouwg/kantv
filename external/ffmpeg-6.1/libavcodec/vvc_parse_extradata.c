/*
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

#include "bytestream.h"
#include "h2645_parse.h"
#include "vvc.h"
#include "vvc_parse_extradata.h"

static int h266_decode_nal_units(const uint8_t *buf, int buf_size,
                                H266ParamSets *ps, int is_nalff,
                                int nal_length_size, int err_recognition,
                                int apply_defdispwin, void *logctx)
{
    int i;
    int ret = 0;
    H2645Packet pkt = { 0 };

    ret = ff_h2645_packet_split(&pkt, buf, buf_size, logctx, is_nalff,
                                nal_length_size, AV_CODEC_ID_VVC, 1, 0);
    if (ret < 0) {
        goto done;
    }

    for (i = 0; i < pkt.nb_nals; i++) {
        H2645NAL *nal = &pkt.nals[i];
        if (nal->nuh_layer_id > 0)
            continue;

        /* ignore everything except parameter sets and VCL NALUs */
        switch (nal->type) {
        case VVC_VPS_NUT:
            // TODO: since not needed yet, not implemented
            //ret = ff_h266_decode_nal_vps(&nal->gb, logctx, ps);
            //if (ret < 0)
            //    goto done;
            break;
        case VVC_SPS_NUT:
            ret = ff_h266_decode_nal_sps(&nal->gb, logctx, ps, apply_defdispwin);
            if (ret < 0)
                goto done;
            break;
        case VVC_PPS_NUT:
            // TODO: since not needed yet, not implemented
            //ret = ff_h266_decode_nal_pps(&nal->gb, logctx, ps);
            //if (ret < 0)
            //    goto done;
            break;
        case VVC_PREFIX_SEI_NUT:
        case VVC_SUFFIX_SEI_NUT:
            // TODO: SEI decoding not implemented yet
            break;
        default:
            av_log(logctx, AV_LOG_VERBOSE,
                   "Ignoring NAL type %d in extradata\n", nal->type);
            break;
        }
    }

  done:
    ff_h2645_packet_uninit(&pkt);
    if (err_recognition & AV_EF_EXPLODE)
        return ret;

    return 0;
}

int ff_h266_decode_extradata(const uint8_t *data, int size, H266ParamSets *ps,
                            int *is_nalff, int *nal_length_size,
                            int err_recognition, int apply_defdispwin,
                            void *logctx)
{
    int ret = 0;
    GetByteContext gb;

    bytestream2_init(&gb, data, size);

    if (size > 3 && (data[0] || data[1] || data[2] > 1)) {
        /* extradata is encoded as vvcC format. */
        int i, j, b, num_arrays, nal_len_size, has_ptl;

        b = bytestream2_get_byte(&gb);
        nal_len_size  = ((b >> 1) & 3) + 1;
        has_ptl = b & 1;

        if (has_ptl) {
            int max_picture_width = 0;
            int max_picture_height = 0;
            int avg_frame_rate = 0;

            int num_sublayers;
            int num_bytes_constraint_info;
            int general_profile_idc;
            int general_tier_flag;
            int general_level_idc;
            int ptl_frame_only_constraint_flag;
            int ptl_multi_layer_enabled_flag;
            int ptl_num_sub_profiles;
            int ols_idx;
            int constant_frame_rate;
            int chroma_format_idc;
            int bit_depth_minus8;

            b = bytestream2_get_be16(&gb);
            ols_idx = (b >> 7) & 0x1ff;
            num_sublayers = (b >> 4) & 7;
            constant_frame_rate = (b >> 2) & 3;
            chroma_format_idc = b & 0x3;
            bit_depth_minus8 = (bytestream2_get_byte(&gb) >> 5) & 7;
            av_log(logctx, AV_LOG_TRACE,
                   "bit_depth_minus8 %d chroma_format_idc %d\n",
                   bit_depth_minus8, chroma_format_idc);
            av_log(logctx, AV_LOG_TRACE,
                   "constant_frame_rate %d, ols_idx %d\n",
                   constant_frame_rate, ols_idx);
            // VvcPTLRecord(num_sublayers) native_ptl
            b = bytestream2_get_byte(&gb);
            num_bytes_constraint_info = (b) & 0x3f;
            b = bytestream2_get_byte(&gb);
            general_profile_idc = (b >> 1) & 0x7f;
            general_tier_flag = b & 1;
            general_level_idc = bytestream2_get_byte(&gb);
            av_log(logctx, AV_LOG_TRACE,
                   "general_profile_idc %d, general_tier_flag %d, "
                   "general_level_idc %d, num_sublayers %d num_bytes_constraint_info %d\n",
                   general_profile_idc, general_tier_flag, general_level_idc,
                   num_sublayers, num_bytes_constraint_info);

            b = bytestream2_get_byte(&gb);
            ptl_frame_only_constraint_flag = (b >> 7) & 1;
            ptl_multi_layer_enabled_flag = (b >> 6) & 1;
            for (i = 0; i < num_bytes_constraint_info - 1; i++) {
                // unsigned int(8*num_bytes_constraint_info - 2) general_constraint_info;
                bytestream2_get_byte(&gb);
            }

            av_log(logctx, AV_LOG_TRACE,
                   "ptl_multi_layer_enabled_flag %d, ptl_frame_only_constraint_flag %d\n",
                   ptl_multi_layer_enabled_flag,
                   ptl_frame_only_constraint_flag);

            if (num_sublayers > 1) {
                int temp6 = bytestream2_get_byte(&gb);
                uint8_t ptl_sublayer_level_present_flag[8] = { 0 };
                //uint8_t sublayer_level_idc[8] = {0};
                for (i = num_sublayers - 2; i >= 0; i--) {
                    ptl_sublayer_level_present_flag[i] =
                        (temp6 >> (7 - (num_sublayers - 2 - i))) & 0x01;
                }
                for (i = num_sublayers - 2; i >= 0; i--) {
                    if (ptl_sublayer_level_present_flag[i]) {
                        bytestream2_get_byte(&gb);      // sublayer_level_idc[8]
                    }
                }
            }

            ptl_num_sub_profiles = bytestream2_get_byte(&gb);
            for (j = 0; j < ptl_num_sub_profiles; j++) {
                // unsigned int(32) general_sub_profile_idc[j];
                bytestream2_get_be16(&gb);
                bytestream2_get_be16(&gb);
            }

            max_picture_width  = bytestream2_get_be16(&gb);
            max_picture_height = bytestream2_get_be16(&gb);
            avg_frame_rate     = bytestream2_get_be16(&gb);
            av_log(logctx, AV_LOG_TRACE,
                   "max_picture_width %d, max_picture_height %d, avg_frame_rate %d\n",
                   max_picture_width, max_picture_height, avg_frame_rate);
        }

        *is_nalff = 1;

        /* nal units in the vvcC always have length coded with 2 bytes,
         * so set nal_length_size = 2 while parsing them */
        *nal_length_size = 2;

        num_arrays = bytestream2_get_byte(&gb);
        for (i = 0; i < num_arrays; i++) {
            int cnt;
            int type = bytestream2_get_byte(&gb) & 0x1f;

            if (type == VVC_OPI_NUT || type == VVC_DCI_NUT)
                cnt = 1;
            else
                cnt = bytestream2_get_be16(&gb);

            av_log(logctx, AV_LOG_DEBUG, "nalu_type %d cnt %d\n", type, cnt);

            if (!(type == VVC_OPI_NUT || type == VVC_DCI_NUT ||
                  type == VVC_VPS_NUT || type == VVC_SPS_NUT
                  || type == VVC_PPS_NUT || type == VVC_PREFIX_SEI_NUT
                  || type == VVC_SUFFIX_SEI_NUT)) {
                av_log(logctx, AV_LOG_ERROR,
                       "Invalid NAL unit type in extradata: %d\n", type);
                ret = AVERROR_INVALIDDATA;
                return ret;
            }

            for (j = 0; j < cnt; j++) {
                // +2 for the nal size field
                int nalsize = bytestream2_peek_be16(&gb) + 2;
                if (bytestream2_get_bytes_left(&gb) < nalsize) {
                    av_log(logctx, AV_LOG_ERROR,
                           "Invalid NAL unit size in extradata.\n");
                    return AVERROR_INVALIDDATA;
                }

                ret = h266_decode_nal_units(gb.buffer, nalsize, ps, *is_nalff,
                                           *nal_length_size, err_recognition,
                                           apply_defdispwin, logctx);
                if (ret < 0) {
                    av_log(logctx, AV_LOG_ERROR,
                           "Decoding nal unit %d %d from vvcC failed\n", type, i);
                    return ret;
                }
                bytestream2_skip(&gb, nalsize);
            }
        }

        /* store nal length size, that will be used to parse all other nals */
        *nal_length_size = nal_len_size;
    } else {
        *is_nalff = 0;
        ret = h266_decode_nal_units(data, size, ps, *is_nalff, *nal_length_size,
                                   err_recognition, apply_defdispwin, logctx);
        if (ret < 0)
            return ret;
    }

    return ret;
}
