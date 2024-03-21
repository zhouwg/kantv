/*
 * H.266 / VVC Parameter Set decoding
 *
 * Copyright (c) 2022, Thomas Siedel
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

#include "libavutil/imgutils.h"
#include "golomb.h"
#include "vvc_paramset.h"

static void remove_sps(H266ParamSets *s, int id)
{
    if (s->sps_list[id]) {
        if (s->sps == (const H266SPS *)s->sps_list[id]->data)
            s->sps = NULL;

        av_assert0(!(s->sps_list[id] &&
                     s->sps == (H266SPS *)s->sps_list[id]->data));
    }
    av_buffer_unref(&s->sps_list[id]);
}

static int decode_general_constraints_info(GetBitContext *gb,
                                           AVCodecContext *avctx,
                                           H266GeneralConstraintsInfo *gci)
{
    int i;
    gci->gci_present_flag = get_bits1(gb);

    if (gci->gci_present_flag) {
        /* general */
        gci->gci_intra_only_constraint_flag = get_bits1(gb);
        gci->gci_all_layers_independent_constraint_flag = get_bits1(gb);
        gci->gci_one_au_only_constraint_flag = get_bits1(gb);

        /* picture format */
        gci->gci_sixteen_minus_max_bitdepth_constraint_idc = get_bits(gb, 4);
        gci->gci_three_minus_max_chroma_format_constraint_idc = get_bits(gb, 2);

        /* NAL unit type related */
        gci->gci_no_mixed_nalu_types_in_pic_constraint_flag = get_bits1(gb);
        gci->gci_no_trail_constraint_flag = get_bits1(gb);
        gci->gci_no_stsa_constraint_flag  = get_bits1(gb);
        gci->gci_no_rasl_constraint_flag  = get_bits1(gb);
        gci->gci_no_radl_constraint_flag  = get_bits1(gb);
        gci->gci_no_idr_constraint_flag   = get_bits1(gb);
        gci->gci_no_cra_constraint_flag   = get_bits1(gb);
        gci->gci_no_gdr_constraint_flag   = get_bits1(gb);
        gci->gci_no_aps_constraint_flag   = get_bits1(gb);
        gci->gci_no_idr_rpl_constraint_flag = get_bits1(gb);

        /* tile, slice, subpicture partitioning */
        gci->gci_one_tile_per_pic_constraint_flag     = get_bits1(gb);
        gci->gci_pic_header_in_slice_header_constraint_flag = get_bits1(gb);
        gci->gci_one_slice_per_pic_constraint_flag    = get_bits1(gb);
        gci->gci_no_rectangular_slice_constraint_flag = get_bits1(gb);
        gci->gci_one_slice_per_subpic_constraint_flag = get_bits1(gb);
        gci->gci_no_subpic_info_constraint_flag = get_bits1(gb);

        /* CTU and block partitioning */
        gci->gci_three_minus_max_log2_ctu_size_constraint_idc = get_bits(gb, 2);
        gci->gci_no_partition_constraints_override_constraint_flag =
            get_bits1(gb);
        gci->gci_no_mtt_constraint_flag = get_bits1(gb);
        gci->gci_no_qtbtt_dual_tree_intra_constraint_flag = get_bits1(gb);

        /* intra */
        gci->gci_no_palette_constraint_flag = get_bits1(gb);
        gci->gci_no_ibc_constraint_flag = get_bits1(gb);
        gci->gci_no_isp_constraint_flag = get_bits1(gb);
        gci->gci_no_mrl_constraint_flag = get_bits1(gb);
        gci->gci_no_mip_constraint_flag = get_bits1(gb);
        gci->gci_no_cclm_constraint_flag = get_bits1(gb);

        /* inter */
        gci->gci_no_ref_pic_resampling_constraint_flag  = get_bits1(gb);
        gci->gci_no_res_change_in_clvs_constraint_flag  = get_bits1(gb);
        gci->gci_no_weighted_prediction_constraint_flag = get_bits1(gb);
        gci->gci_no_ref_wraparound_constraint_flag = get_bits1(gb);
        gci->gci_no_temporal_mvp_constraint_flag   = get_bits1(gb);
        gci->gci_no_sbtmvp_constraint_flag = get_bits1(gb);
        gci->gci_no_amvr_constraint_flag   = get_bits1(gb);
        gci->gci_no_bdof_constraint_flag   = get_bits1(gb);
        gci->gci_no_smvd_constraint_flag   = get_bits1(gb);
        gci->gci_no_dmvr_constraint_flag   = get_bits1(gb);
        gci->gci_no_mmvd_constraint_flag   = get_bits1(gb);
        gci->gci_no_affine_motion_constraint_flag = get_bits1(gb);
        gci->gci_no_prof_constraint_flag   = get_bits1(gb);
        gci->gci_no_bcw_constraint_flag    = get_bits1(gb);
        gci->gci_no_ciip_constraint_flag   = get_bits1(gb);
        gci->gci_no_gpm_constraint_flag    = get_bits1(gb);

        /* transform, quantization, residual */
        gci->gci_no_luma_transform_size_64_constraint_flag = get_bits1(gb);
        gci->gci_no_transform_skip_constraint_flag         = get_bits1(gb);
        gci->gci_no_bdpcm_constraint_flag      = get_bits1(gb);
        gci->gci_no_mts_constraint_flag        = get_bits1(gb);
        gci->gci_no_lfnst_constraint_flag      = get_bits1(gb);
        gci->gci_no_joint_cbcr_constraint_flag = get_bits1(gb);
        gci->gci_no_sbt_constraint_flag        = get_bits1(gb);
        gci->gci_no_act_constraint_flag        = get_bits1(gb);
        gci->gci_no_explicit_scaling_list_constraint_flag = get_bits1(gb);
        gci->gci_no_dep_quant_constraint_flag        = get_bits1(gb);
        gci->gci_no_sign_data_hiding_constraint_flag = get_bits1(gb);
        gci->gci_no_cu_qp_delta_constraint_flag      = get_bits1(gb);
        gci->gci_no_chroma_qp_offset_constraint_flag = get_bits1(gb);

        /* loop filter */
        gci->gci_no_sao_constraint_flag   = get_bits1(gb);
        gci->gci_no_alf_constraint_flag   = get_bits1(gb);
        gci->gci_no_ccalf_constraint_flag = get_bits1(gb);
        gci->gci_no_lmcs_constraint_flag  = get_bits1(gb);
        gci->gci_no_ladf_constraint_flag  = get_bits1(gb);
        gci->gci_no_virtual_boundaries_constraint_flag  = get_bits1(gb);
        gci->gci_num_additional_bits  = get_bits(gb,8);
        for (i = 0; i < gci->gci_num_additional_bits; i++) {
            gci->gci_reserved_bit[i] = get_bits1(gb);
        }
    }

    align_get_bits(gb);

    return 0;
}


static int decode_profile_tier_level(GetBitContext *gb, AVCodecContext *avctx,
                                     H266RawProfileTierLevel *ptl,
                                     int profile_tier_present_flag,
                                     int max_num_sub_layers_minus1)
{
    int i;

    if (profile_tier_present_flag) {
        ptl->general_profile_idc = get_bits(gb, 7);
        ptl->general_tier_flag   = get_bits1(gb);
    }
    ptl->general_level_idc              = get_bits(gb, 8);
    ptl->ptl_frame_only_constraint_flag = get_bits1(gb);
    ptl->ptl_multilayer_enabled_flag    = get_bits1(gb);

    if (profile_tier_present_flag) {
        decode_general_constraints_info(gb, avctx,
                                        &ptl->general_constraints_info);
    }

    for (i = max_num_sub_layers_minus1 - 1; i >= 0; i--) {
        ptl->ptl_sublayer_level_present_flag[i] = get_bits1(gb);
    }

    align_get_bits(gb);

    for (i = max_num_sub_layers_minus1 - 1; i >= 0; i--) {
        if (ptl->ptl_sublayer_level_present_flag[i])
            ptl->sublayer_level_idc[i] = get_bits(gb, 8);
    }

    if (profile_tier_present_flag) {
        ptl->ptl_num_sub_profiles = get_bits(gb, 8);
        for (i = 0; i < ptl->ptl_num_sub_profiles; i++)
            ptl->general_sub_profile_idc[i] = get_bits_long(gb, 32);
    }

    return 0;
}

static int decode_dpb_parameters(GetBitContext *gb, AVCodecContext *avctx,
                                 H266DpbParameters *dpb,
                                 uint8_t max_sublayers_minus1,
                                 uint8_t sublayer_info_flag)
{
    int i;
    for (i = (sublayer_info_flag ? 0 : max_sublayers_minus1);
         i <= max_sublayers_minus1; i++) {
        dpb->dpb_max_dec_pic_buffering_minus1[i] = get_ue_golomb(gb);
        dpb->dpb_max_num_reorder_pics[i]         = get_ue_golomb(gb);
        dpb->dpb_max_latency_increase_plus1[i]   = get_ue_golomb(gb);
    }
    return 0;
}

static int decode_ref_pic_list_struct(GetBitContext *gb, AVCodecContext *avctx,
                                      H266RefPicListStruct *current,
                                      uint8_t list_idx, uint8_t rpls_idx,
                                      const H266SPS *sps)
{
    int i, j, num_direct_ref_layers = 0;

    current->num_ref_entries = get_ue_golomb(gb);
    if (sps->long_term_ref_pics_flag &&
        rpls_idx < sps->num_ref_pic_lists[list_idx] &&
        current->num_ref_entries > 0)
        current->ltrp_in_header_flag = get_bits1(gb);
    if (sps->long_term_ref_pics_flag &&
        rpls_idx == sps->num_ref_pic_lists[list_idx])
        current->ltrp_in_header_flag = 1;
    for (i = 0, j = 0; i < current->num_ref_entries; i++) {
        if (sps->inter_layer_prediction_enabled_flag)
            current->inter_layer_ref_pic_flag[i] = get_bits1(gb);
        else
            current->inter_layer_ref_pic_flag[i] = 0;

        if (!current->inter_layer_ref_pic_flag[i]) {
            if (sps->long_term_ref_pics_flag)
                current->st_ref_pic_flag[i] = get_bits1(gb);
            else
                current->st_ref_pic_flag[i] = 1;
            if (current->st_ref_pic_flag[i]) {
                int abs_delta_poc_st;
                current->abs_delta_poc_st[i] = get_ue_golomb(gb);
                if ((sps->weighted_pred_flag ||
                     sps->weighted_bipred_flag) && i != 0)
                    abs_delta_poc_st = current->abs_delta_poc_st[i];
                else
                    abs_delta_poc_st = current->abs_delta_poc_st[i] + 1;
                if (abs_delta_poc_st > 0)
                    current->strp_entry_sign_flag[i] = get_bits1(gb);
            } else {
                if (!current->ltrp_in_header_flag) {
                    uint8_t bits = sps->log2_max_pic_order_cnt_lsb_minus4 + 4;
                    current->rpls_poc_lsb_lt[j] = get_bits(gb, bits);
                    j++;
                }
            }
        } else {
            if (num_direct_ref_layers == 0) {
                av_log(avctx, AV_LOG_ERROR,
                       "num_direct_ref_layers needs > 0.\n");
                return AVERROR_INVALIDDATA;
            }
            current->ilrp_idx[i] = get_ue_golomb(gb);
        }
    }
    return 0;
}

static int decode_general_timing_hrd_parameters(GetBitContext *gb,
                                                H266GeneralTimingHrdParameters *current)
{
    current->num_units_in_tick = get_bits_long(gb, 32);
    current->time_scale = get_bits_long(gb, 32);
    current->general_nal_hrd_params_present_flag = get_bits1(gb);
    current->general_vcl_hrd_params_present_flag = get_bits1(gb);

    if (current->general_nal_hrd_params_present_flag ||
        current->general_vcl_hrd_params_present_flag) {
        current->general_same_pic_timing_in_all_ols_flag = get_bits1(gb);
        current->general_du_hrd_params_present_flag = get_bits1(gb);
        if (current->general_du_hrd_params_present_flag)
            current->tick_divisor_minus2 = get_bits(gb, 8);
        current->bit_rate_scale = get_bits(gb, 4);
        current->cpb_size_scale = get_bits(gb, 4);
        if (current->general_du_hrd_params_present_flag)
            current->cpb_size_du_scale = get_bits(gb, 4);
        current->hrd_cpb_cnt_minus1 = get_ue_golomb_long(gb);
    } else {
        current->general_du_hrd_params_present_flag = 0;
    }
    return 0;
}

static int decode_sublayer_hrd_parameters(GetBitContext *gb,
                                          H266SubLayerHRDParameters *current,
                                          int sublayer_id,
                                          const H266GeneralTimingHrdParameters *general)
{
    int i;
    for (i = 0; i <= general->hrd_cpb_cnt_minus1; i++) {
        current->bit_rate_value_minus1[sublayer_id][i] = get_ue_golomb_long(gb);
        current->cpb_size_value_minus1[sublayer_id][i] = get_ue_golomb_long(gb);
        if (general->general_du_hrd_params_present_flag) {
            current->cpb_size_du_value_minus1[sublayer_id][i] =
                get_ue_golomb_long(gb);
            current->bit_rate_du_value_minus1[sublayer_id][i] =
                get_ue_golomb_long(gb);
        }
        current->cbr_flag[sublayer_id][i] = get_bits1(gb);
    }
    return 0;
}

static int decode_ols_timing_hrd_parameters(GetBitContext *gb,
                                            H266OlsTimingHrdParameters *current,
                                            uint8_t first_sublayer,
                                            uint8_t max_sublayers_minus1,
                                            const H266GeneralTimingHrdParameters * general)
{
    int i;
    for (i = first_sublayer; i <= max_sublayers_minus1; i++) {
        current->fixed_pic_rate_general_flag[i] = get_bits1(gb);
        if (!current->fixed_pic_rate_general_flag[i])
            current->fixed_pic_rate_within_cvs_flag[i] = get_bits1(gb);
        else
            current->fixed_pic_rate_within_cvs_flag[i] = 1;
        if (current->fixed_pic_rate_within_cvs_flag[i]) {
            current->elemental_duration_in_tc_minus1[i] = get_ue_golomb_long(gb);
            current->low_delay_hrd_flag[i] = 0;
        } else if ((general->general_nal_hrd_params_present_flag ||
                    general->general_vcl_hrd_params_present_flag) &&
                   general->hrd_cpb_cnt_minus1 == 0) {
            current->low_delay_hrd_flag[i] = get_bits1(gb);
        } else {
            current->low_delay_hrd_flag[i] = 0;
        }
        if (general->general_nal_hrd_params_present_flag)
            decode_sublayer_hrd_parameters(gb,
                                           &current->
                                           nal_sub_layer_hrd_parameters, i,
                                           general);
        if (general->general_vcl_hrd_params_present_flag)
            decode_sublayer_hrd_parameters(gb,
                                           &current->
                                           nal_sub_layer_hrd_parameters, i,
                                           general);
    }
    return 0;
}

static int decode_vui(GetBitContext *gb, AVCodecContext *avctx,
                      H266VUI* vui, uint8_t chroma_format_idc )
{
    vui->progressive_source_flag        = get_bits1(gb);
    vui->interlaced_source_flag         = get_bits1(gb);
    vui->non_packed_constraint_flag     = get_bits1(gb);
    vui->non_projected_constraint_flag  = get_bits1(gb);
    vui->aspect_ratio_info_present_flag = get_bits1(gb);
    if (vui->aspect_ratio_info_present_flag) {
        vui->aspect_ratio_constant_flag = get_bits1(gb);
        vui->aspect_ratio_idc           = get_bits(gb, 8);
        if (vui->aspect_ratio_idc == 255) {
            vui->sar_width  = get_bits(gb, 16);
            vui->sar_height = get_bits(gb, 16);
        }
    } else {
        vui->aspect_ratio_constant_flag = 0;
        vui->aspect_ratio_idc = 0;
    }
    vui->overscan_info_present_flag = get_bits1(gb);
    if (vui->overscan_info_present_flag)
        vui->overscan_appropriate_flag   = get_bits1(gb);
    vui->colour_description_present_flag = get_bits1(gb);
    if (vui->colour_description_present_flag) {
        vui->colour_primaries         = get_bits(gb, 8);
        vui->transfer_characteristics = get_bits(gb, 8);
        vui->matrix_coeffs            = get_bits(gb, 8);
        av_log(avctx, AV_LOG_DEBUG,
               "colour_primaries: %d transfer_characteristics: %d matrix_coeffs: %d \n",
               vui->colour_primaries, vui->transfer_characteristics,
               vui->matrix_coeffs);

        vui->full_range_flag = get_bits1(gb);
    } else {
        vui->colour_primaries         = 2;
        vui->transfer_characteristics = 2;
        vui->matrix_coeffs            = 2;
        vui->full_range_flag          = 0;
    }
    vui->chroma_loc_info_present_flag = get_bits1(gb);
    if (chroma_format_idc != 1 && vui->chroma_loc_info_present_flag) {
        av_log(avctx, AV_LOG_ERROR, "chroma_format_idc == %d,"
               "chroma_loc_info_present_flag can't not be true",
               chroma_format_idc);
        return AVERROR_INVALIDDATA;
    }
    if (vui->chroma_loc_info_present_flag) {
        if (vui->progressive_source_flag && !vui->interlaced_source_flag) {
            vui->chroma_sample_loc_type_frame = get_ue_golomb(gb);
        } else {
            vui->chroma_sample_loc_type_top_field    = get_ue_golomb(gb);
            vui->chroma_sample_loc_type_bottom_field = get_ue_golomb(gb);
        }
    } else {
        if (chroma_format_idc == 1) {
            vui->chroma_sample_loc_type_frame        = get_ue_golomb(gb);
            vui->chroma_sample_loc_type_top_field    = get_ue_golomb(gb);
            vui->chroma_sample_loc_type_bottom_field = get_ue_golomb(gb);
        }
    }
    return 0;
}

static int map_pixel_format(AVCodecContext *avctx, H266SPS *sps)
{
    const AVPixFmtDescriptor *desc;
    switch (sps->bit_depth) {
    case 8:
        if (sps->chroma_format_idc == 0)
            sps->pix_fmt = AV_PIX_FMT_GRAY8;
        if (sps->chroma_format_idc == 1)
            sps->pix_fmt = AV_PIX_FMT_YUV420P;
        if (sps->chroma_format_idc == 2)
            sps->pix_fmt = AV_PIX_FMT_YUV422P;
        if (sps->chroma_format_idc == 3)
            sps->pix_fmt = AV_PIX_FMT_YUV444P;
        break;
    case 9:
        if (sps->chroma_format_idc == 0)
            sps->pix_fmt = AV_PIX_FMT_GRAY9;
        if (sps->chroma_format_idc == 1)
            sps->pix_fmt = AV_PIX_FMT_YUV420P9;
        if (sps->chroma_format_idc == 2)
            sps->pix_fmt = AV_PIX_FMT_YUV422P9;
        if (sps->chroma_format_idc == 3)
            sps->pix_fmt = AV_PIX_FMT_YUV444P9;
        break;
    case 10:
        if (sps->chroma_format_idc == 0)
            sps->pix_fmt = AV_PIX_FMT_GRAY10;
        if (sps->chroma_format_idc == 1)
            sps->pix_fmt = AV_PIX_FMT_YUV420P10;
        if (sps->chroma_format_idc == 2)
            sps->pix_fmt = AV_PIX_FMT_YUV422P10;
        if (sps->chroma_format_idc == 3)
            sps->pix_fmt = AV_PIX_FMT_YUV444P10;
        break;
    case 12:
        if (sps->chroma_format_idc == 0)
            sps->pix_fmt = AV_PIX_FMT_GRAY12;
        if (sps->chroma_format_idc == 1)
            sps->pix_fmt = AV_PIX_FMT_YUV420P12;
        if (sps->chroma_format_idc == 2)
            sps->pix_fmt = AV_PIX_FMT_YUV422P12;
        if (sps->chroma_format_idc == 3)
            sps->pix_fmt = AV_PIX_FMT_YUV444P12;
        break;
    default:
        av_log(avctx, AV_LOG_ERROR,
               "The following bit-depths are currently specified: 8, 9, 10 and 12 bits, "
               "chroma_format_idc is %d, depth is %d\n",
               sps->chroma_format_idc, sps->bit_depth);
        return AVERROR_INVALIDDATA;
    }

    desc = av_pix_fmt_desc_get(sps->pix_fmt);
    if (!desc)
        return AVERROR(EINVAL);

    return 0;
}

int ff_h266_parse_sps(H266SPS *sps, GetBitContext *gb, unsigned int *sps_id,
                     int apply_defdispwin, AVCodecContext *avctx)
{
    int i, j;
    unsigned int ctb_log2_size_y, ctb_size_y, max_num_merge_cand,
                 tmp_width_val, tmp_height_val;

    sps->sps_id = get_bits(gb, 4);
    sps->vps_id = get_bits(gb, 4);

    *sps_id = sps->sps_id;

    sps->max_sublayers = get_bits(gb, 3) + 1;
    sps->chroma_format_idc = get_bits(gb, 2);
    sps->log2_ctu_size = get_bits(gb, 2) + 5;

    ctb_log2_size_y = sps->log2_ctu_size;
    ctb_size_y = 1 << ctb_log2_size_y;

    sps->ptl_dpb_hrd_params_present_flag = get_bits1(gb);
    if (sps->ptl_dpb_hrd_params_present_flag)
        decode_profile_tier_level(gb, avctx, &sps->profile_tier_level, 1,
                                  sps->max_sublayers - 1);

    sps->gdr_enabled_flag = get_bits1(gb);
    sps->ref_pic_resampling_enabled_flag = get_bits1(gb);

    if (sps->ref_pic_resampling_enabled_flag)
        sps->res_change_in_clvs_allowed_flag = get_bits1(gb);
    else
        sps->res_change_in_clvs_allowed_flag = 0;

    sps->pic_width_max_in_luma_samples  = get_ue_golomb(gb);
    sps->pic_height_max_in_luma_samples = get_ue_golomb(gb);

    sps->conformance_window_flag = get_bits1(gb);

    if (sps->conformance_window_flag) {
        sps->conf_win_left_offset   = get_ue_golomb(gb);
        sps->conf_win_right_offset  = get_ue_golomb(gb);
        sps->conf_win_top_offset    = get_ue_golomb(gb);
        sps->conf_win_bottom_offset = get_ue_golomb(gb);
    } else {
        sps->conf_win_left_offset   = 0;
        sps->conf_win_right_offset  = 0;
        sps->conf_win_top_offset    = 0;
        sps->conf_win_bottom_offset = 0;
    }

    tmp_width_val =
        (sps->pic_width_max_in_luma_samples + ctb_size_y - 1) / ctb_size_y;
    tmp_height_val =
        (sps->pic_height_max_in_luma_samples + ctb_size_y - 1) / ctb_size_y;

    sps->subpic_info_present_flag = get_bits1(gb);
    if (sps->subpic_info_present_flag) {
        sps->num_subpics_minus1 = get_ue_golomb(gb);
        if (sps->num_subpics_minus1 > 0) {
            sps->independent_subpics_flag = get_bits1(gb);
            sps->subpic_same_size_flag = get_bits1(gb);
        }
    }

    if (sps->num_subpics_minus1 > 0) {
        int wlen = av_ceil_log2(tmp_width_val);
        int hlen = av_ceil_log2(tmp_height_val);
        if (sps->pic_width_max_in_luma_samples > ctb_size_y)
            sps->subpic_width_minus1[0] = get_bits(gb, wlen);
        else
            sps->subpic_width_minus1[0] = tmp_width_val - 1;

        if (sps->pic_height_max_in_luma_samples > ctb_size_y)
            sps->subpic_height_minus1[0] = get_bits(gb, hlen);
        else
            sps->subpic_height_minus1[0] = tmp_height_val;

        if (!sps->independent_subpics_flag) {
            sps->subpic_treated_as_pic_flag[0] = get_bits1(gb);
            sps->loop_filter_across_subpic_enabled_flag[0] = get_bits1(gb);
        } else {
            sps->subpic_treated_as_pic_flag[0] = 1;
            sps->loop_filter_across_subpic_enabled_flag[0] = 1;
        }

        for (i = 1; i <= sps->num_subpics_minus1; i++) {
            if (!sps->subpic_same_size_flag) {
                if (sps->pic_width_max_in_luma_samples > ctb_size_y)
                    sps->subpic_ctu_top_left_x[i] = get_bits(gb, wlen);
                else
                    sps->subpic_ctu_top_left_x[i] = 0;

                if (sps->pic_height_max_in_luma_samples > ctb_size_y)
                    sps->subpic_ctu_top_left_y[i] = get_bits(gb, hlen);
                else
                    sps->subpic_ctu_top_left_y[i] = 0;

                if (i < sps->num_subpics_minus1 &&
                    sps->pic_width_max_in_luma_samples > ctb_size_y) {
                    sps->subpic_width_minus1[i] = get_bits(gb, wlen);
                } else {
                    sps->subpic_width_minus1[i] =
                        tmp_width_val - sps->subpic_ctu_top_left_x[i] - 1;
                }
                if (i < sps->num_subpics_minus1 &&
                    sps->pic_height_max_in_luma_samples > ctb_size_y) {
                    sps->subpic_height_minus1[i] = get_bits(gb, hlen);
                } else {
                    sps->subpic_height_minus1[i] =
                        tmp_height_val - sps->subpic_ctu_top_left_y[i] - 1;
                }
            } else {
                int num_subpic_cols =
                    tmp_width_val / (sps->subpic_width_minus1[0] + 1);
                sps->subpic_ctu_top_left_x[i] = (i % num_subpic_cols) *(sps->subpic_width_minus1[0] + 1);
                sps->subpic_ctu_top_left_y[i] = (i / num_subpic_cols) *(sps->subpic_height_minus1[0] + 1);
                sps->subpic_width_minus1[i]   =  sps->subpic_width_minus1[0];
                sps->subpic_height_minus1[i]  = sps->subpic_height_minus1[0];
            }
            if (!sps->independent_subpics_flag) {
                sps->subpic_treated_as_pic_flag[i] = get_bits1(gb);
                sps->loop_filter_across_subpic_enabled_flag[i] = get_bits1(gb);
            } else {
                sps->subpic_treated_as_pic_flag[i] = 1;
                sps->loop_filter_across_subpic_enabled_flag[i] = 0;
            }
        }
        sps->subpic_id_len_minus1 = get_ue_golomb(gb);

        if ((1 << (sps->subpic_id_len_minus1 + 1)) <
            sps->num_subpics_minus1 + 1) {
            av_log(avctx, AV_LOG_ERROR,
                   "sps->subpic_id_len_minus1(%d) is too small\n",
                   sps->subpic_id_len_minus1);
            return AVERROR_INVALIDDATA;
        }
        sps->subpic_id_mapping_explicitly_signalled_flag = get_bits1(gb);
        if (sps->subpic_id_mapping_explicitly_signalled_flag) {
            sps->subpic_id_mapping_present_flag = get_bits1(gb);
            if (sps->subpic_id_mapping_present_flag) {
                for (i = 0; i <= sps->num_subpics_minus1; i++)
                    sps->subpic_id[i] =
                        get_bits(gb, sps->subpic_id_len_minus1 + 1);
            }
        } else {
            sps->subpic_ctu_top_left_x[0] = 0;
            sps->subpic_ctu_top_left_y[0] = 0;
            sps->subpic_width_minus1[0] = tmp_width_val - 1;
            sps->subpic_height_minus1[0] = tmp_height_val - 1;
        }
    } else {
        sps->num_subpics_minus1 = 0;
        sps->independent_subpics_flag = 1;
        sps->subpic_same_size_flag = 0;
        sps->subpic_id_mapping_explicitly_signalled_flag = 0;
        sps->subpic_ctu_top_left_x[0] = 0;
        sps->subpic_ctu_top_left_y[0] = 0;
        sps->subpic_width_minus1[0] = tmp_width_val - 1;
        sps->subpic_height_minus1[0] = tmp_height_val - 1;
    }

    sps->bit_depth = get_ue_golomb(gb) + 8;

    sps->entropy_coding_sync_enabled_flag = get_bits1(gb);
    sps->entry_point_offsets_present_flag = get_bits1(gb);

    sps->log2_max_pic_order_cnt_lsb_minus4 = get_bits(gb, 4);

    sps->poc_msb_cycle_flag = get_bits1(gb);
    if (sps->poc_msb_cycle_flag)
        sps->poc_msb_cycle_len_minus1 = get_ue_golomb(gb);

    sps->num_extra_ph_bytes = get_bits(gb, 2);

    for (i = 0; i < FFMIN(16, (sps->num_extra_ph_bytes * 8)); i++) {
        sps->extra_ph_bit_present_flag[i] = get_bits1(gb);
    }

    sps->num_extra_sh_bytes = get_bits(gb, 2);
    for (i = 0; i < FFMIN(16, (sps->num_extra_sh_bytes * 8)); i++) {
        sps->extra_sh_bit_present_flag[i] = get_bits1(gb);
    }

    if (sps->ptl_dpb_hrd_params_present_flag) {
        if (sps->max_sublayers > 1)
            sps->sublayer_dpb_params_flag = get_bits1(gb);
        else
            sps->sublayer_dpb_params_flag = 0;

        decode_dpb_parameters(gb, avctx, &sps->dpb_params,
                              sps->max_sublayers - 1,
                              sps->sublayer_dpb_params_flag);
    }

    sps->log2_min_luma_coding_block_size_minus2 = get_ue_golomb(gb);
    sps->partition_constraints_override_enabled_flag = get_bits1(gb);
    sps->log2_diff_min_qt_min_cb_intra_slice_luma = get_ue_golomb(gb);
    sps->max_mtt_hierarchy_depth_intra_slice_luma = get_ue_golomb(gb);

    if (sps->max_mtt_hierarchy_depth_intra_slice_luma != 0) {
        sps->log2_diff_max_bt_min_qt_intra_slice_luma = get_ue_golomb(gb);
        sps->log2_diff_max_tt_min_qt_intra_slice_luma = get_ue_golomb(gb);
    } else {
        sps->log2_diff_max_bt_min_qt_intra_slice_luma = 0;
        sps->log2_diff_max_tt_min_qt_intra_slice_luma = 0;
    }

    if (sps->chroma_format_idc != 0) {
        sps->qtbtt_dual_tree_intra_flag = get_bits1(gb);
    } else {
        sps->qtbtt_dual_tree_intra_flag = 0;
    }

    if (sps->qtbtt_dual_tree_intra_flag) {
        sps->log2_diff_min_qt_min_cb_intra_slice_chroma = get_ue_golomb(gb);
        sps->max_mtt_hierarchy_depth_intra_slice_chroma = get_ue_golomb(gb);
        if (sps->max_mtt_hierarchy_depth_intra_slice_chroma != 0) {
            sps->log2_diff_max_bt_min_qt_intra_slice_chroma = get_ue_golomb(gb);
            sps->log2_diff_max_tt_min_qt_intra_slice_chroma = get_ue_golomb(gb);
        }
    } else {
        sps->log2_diff_min_qt_min_cb_intra_slice_chroma = 0;
        sps->max_mtt_hierarchy_depth_intra_slice_chroma = 0;
    }
    if (sps->max_mtt_hierarchy_depth_intra_slice_chroma == 0) {
        sps->log2_diff_max_bt_min_qt_intra_slice_chroma = 0;
        sps->log2_diff_max_tt_min_qt_intra_slice_chroma = 0;
    }

    sps->log2_diff_min_qt_min_cb_inter_slice = get_ue_golomb(gb);

    sps->max_mtt_hierarchy_depth_inter_slice = get_ue_golomb(gb);
    if (sps->max_mtt_hierarchy_depth_inter_slice != 0) {
        sps->log2_diff_max_bt_min_qt_inter_slice = get_ue_golomb(gb);
        sps->log2_diff_max_tt_min_qt_inter_slice = get_ue_golomb(gb);
    } else {
        sps->log2_diff_max_bt_min_qt_inter_slice = 0;
        sps->log2_diff_max_tt_min_qt_inter_slice = 0;
    }

    if (ctb_size_y > 32)
        sps->max_luma_transform_size_64_flag = get_bits1(gb);
    else
        sps->max_luma_transform_size_64_flag = 0;

    sps->transform_skip_enabled_flag = get_bits1(gb);
    if (sps->transform_skip_enabled_flag) {
        sps->log2_transform_skip_max_size_minus2 = get_ue_golomb(gb);
        sps->bdpcm_enabled_flag = get_bits1(gb);
    }

    sps->mts_enabled_flag = get_bits1(gb);
    if (sps->mts_enabled_flag) {
        sps->explicit_mts_intra_enabled_flag = get_bits1(gb);
        sps->explicit_mts_inter_enabled_flag = get_bits1(gb);
    } else {
        sps->explicit_mts_intra_enabled_flag = 0;
        sps->explicit_mts_inter_enabled_flag = 0;
    }

    sps->lfnst_enabled_flag = get_bits1(gb);

    if (sps->chroma_format_idc != 0) {
        uint8_t num_qp_tables;
        sps->joint_cbcr_enabled_flag = get_bits1(gb);
        sps->same_qp_table_for_chroma_flag = get_bits1(gb);
        num_qp_tables = sps->same_qp_table_for_chroma_flag ?
            1 : (sps->joint_cbcr_enabled_flag ? 3 : 2);
        for (i = 0; i < num_qp_tables; i++) {
            sps->qp_table_start_minus26[i] = get_se_golomb(gb);
            sps->num_points_in_qp_table_minus1[i] = get_ue_golomb(gb);
            for (j = 0; j <= sps->num_points_in_qp_table_minus1[i]; j++) {
                sps->delta_qp_in_val_minus1[i][j] = get_ue_golomb(gb);
                sps->delta_qp_diff_val[i][j] = get_ue_golomb(gb);
            }
        }
    } else {
        sps->joint_cbcr_enabled_flag = 0;
        sps->same_qp_table_for_chroma_flag = 0;
    }

    sps->sao_enabled_flag = get_bits1(gb);
    sps->alf_enabled_flag = get_bits1(gb);
    if (sps->alf_enabled_flag && sps->chroma_format_idc)
        sps->ccalf_enabled_flag = get_bits1(gb);
    else
        sps->ccalf_enabled_flag = 0;

    sps->lmcs_enabled_flag = get_bits1(gb);
    sps->weighted_pred_flag = get_bits1(gb);
    sps->weighted_bipred_flag = get_bits1(gb);
    sps->long_term_ref_pics_flag = get_bits1(gb);
    if (sps->vps_id > 0)
        sps->inter_layer_prediction_enabled_flag = get_bits1(gb);
    else
        sps->inter_layer_prediction_enabled_flag = 0;
    sps->idr_rpl_present_flag = get_bits1(gb);
    sps->rpl1_same_as_rpl0_flag = get_bits1(gb);

    for (i = 0; i < (sps->rpl1_same_as_rpl0_flag ? 1 : 2); i++) {
        sps->num_ref_pic_lists[i] = get_ue_golomb(gb);
        for (j = 0; j < sps->num_ref_pic_lists[i]; j++)
            decode_ref_pic_list_struct(gb, avctx,
                                       &sps->ref_pic_list_struct[i][j],
                                       i, j, sps);
    }

    if (sps->rpl1_same_as_rpl0_flag) {
        sps->num_ref_pic_lists[1] = sps->num_ref_pic_lists[0];
        for (j = 0; j < sps->num_ref_pic_lists[0]; j++)
            memcpy(&sps->ref_pic_list_struct[1][j],
                   &sps->ref_pic_list_struct[0][j],
                   sizeof(sps->ref_pic_list_struct[0][j]));
    }

    sps->ref_wraparound_enabled_flag = get_bits1(gb);

    sps->temporal_mvp_enabled_flag = get_bits1(gb);
    if (sps->temporal_mvp_enabled_flag)
        sps->sbtmvp_enabled_flag = get_bits1(gb);
    else
        sps->sbtmvp_enabled_flag = 0;

    sps->amvr_enabled_flag = get_bits1(gb);
    sps->bdof_enabled_flag = get_bits1(gb);
    if (sps->bdof_enabled_flag)
        sps->bdof_control_present_in_ph_flag = get_bits1(gb);
    else
        sps->bdof_control_present_in_ph_flag = 0;

    sps->smvd_enabled_flag = get_bits1(gb);
    sps->dmvr_enabled_flag = get_bits1(gb);
    if (sps->dmvr_enabled_flag)
        sps->dmvr_control_present_in_ph_flag = get_bits1(gb);
    else
        sps->dmvr_control_present_in_ph_flag = 0;

    sps->mmvd_enabled_flag = get_bits1(gb);
    if (sps->mmvd_enabled_flag)
        sps->mmvd_fullpel_only_enabled_flag = get_bits1(gb);
    else
        sps->mmvd_fullpel_only_enabled_flag = 0;

    sps->six_minus_max_num_merge_cand = get_ue_golomb(gb);
    max_num_merge_cand = 6 - sps->six_minus_max_num_merge_cand;

    sps->sbt_enabled_flag = get_bits1(gb);

    sps->affine_enabled_flag = get_bits1(gb);
    if (sps->affine_enabled_flag) {
        sps->five_minus_max_num_subblock_merge_cand = get_ue_golomb(gb);
        sps->param_affine_enabled_flag = get_bits1(gb);
        if (sps->amvr_enabled_flag)
            sps->affine_amvr_enabled_flag = get_bits1(gb);
        else
            sps->affine_amvr_enabled_flag = 0;
        sps->affine_prof_enabled_flag = get_bits1(gb);
        if (sps->affine_prof_enabled_flag)
            sps->prof_control_present_in_ph_flag = get_bits1(gb);
        else
            sps->prof_control_present_in_ph_flag = 0;
    } else {
        sps->param_affine_enabled_flag = 0;
        sps->affine_amvr_enabled_flag = 0;
        sps->affine_prof_enabled_flag = 0;
        sps->prof_control_present_in_ph_flag = 0;
    }

    sps->bcw_enabled_flag = get_bits1(gb);
    sps->ciip_enabled_flag = get_bits1(gb);

    if (max_num_merge_cand >= 2) {
        sps->gpm_enabled_flag = get_bits1(gb);
        if (sps->gpm_enabled_flag && max_num_merge_cand >= 3)
            sps->max_num_merge_cand_minus_max_num_gpm_cand = get_ue_golomb(gb);
    } else {
        sps->gpm_enabled_flag = 0;
    }

    sps->log2_parallel_merge_level_minus2 = get_ue_golomb(gb);

    sps->isp_enabled_flag = get_bits1(gb);
    sps->mrl_enabled_flag = get_bits1(gb);
    sps->mip_enabled_flag = get_bits1(gb);

    if (sps->chroma_format_idc != 0)
        sps->cclm_enabled_flag = get_bits1(gb);
    else
        sps->cclm_enabled_flag = 0;
    if (sps->chroma_format_idc == 1) {
        sps->chroma_horizontal_collocated_flag = get_bits1(gb);
        sps->chroma_vertical_collocated_flag = get_bits1(gb);
    } else {
        sps->chroma_horizontal_collocated_flag = 0;
        sps->chroma_vertical_collocated_flag = 0;
    }

    sps->palette_enabled_flag = get_bits1(gb);
    if (sps->chroma_format_idc == 3 && !sps->max_luma_transform_size_64_flag)
        sps->act_enabled_flag = get_bits1(gb);
    else
        sps->act_enabled_flag = 0;
    if (sps->transform_skip_enabled_flag || sps->palette_enabled_flag)
        sps->min_qp_prime_ts = get_ue_golomb(gb);

    sps->ibc_enabled_flag = get_bits1(gb);
    if (sps->ibc_enabled_flag)
        sps->six_minus_max_num_ibc_merge_cand = get_ue_golomb(gb);

    sps->ladf_enabled_flag = get_bits1(gb);
    if (sps->ladf_enabled_flag) {
        sps->num_ladf_intervals_minus2 = get_bits(gb, 2);
        sps->ladf_lowest_interval_qp_offset = get_se_golomb(gb);
        for (i = 0; i < sps->num_ladf_intervals_minus2 + 1; i++) {
            sps->ladf_qp_offset[i] = get_se_golomb(gb);
            sps->ladf_delta_threshold_minus1[i] = get_ue_golomb(gb);
        }
    }

    sps->explicit_scaling_list_enabled_flag = get_bits1(gb);
    if (sps->lfnst_enabled_flag && sps->explicit_scaling_list_enabled_flag)
        sps->scaling_matrix_for_lfnst_disabled_flag = get_bits1(gb);

    if (sps->act_enabled_flag && sps->explicit_scaling_list_enabled_flag)
        sps->scaling_matrix_for_alternative_colour_space_disabled_flag =
            get_bits1(gb);
    else
        sps->scaling_matrix_for_alternative_colour_space_disabled_flag = 0;
    if (sps->scaling_matrix_for_alternative_colour_space_disabled_flag)
        sps->scaling_matrix_designated_colour_space_flag = get_bits1(gb);

    sps->dep_quant_enabled_flag = get_bits1(gb);
    sps->sign_data_hiding_enabled_flag = get_bits1(gb);

    sps->virtual_boundaries_enabled_flag = get_bits1(gb);
    if (sps->virtual_boundaries_enabled_flag) {
        sps->virtual_boundaries_present_flag = get_bits1(gb);
        if (sps->virtual_boundaries_present_flag) {
            sps->num_ver_virtual_boundaries = get_ue_golomb(gb);
            for (i = 0; i < sps->num_ver_virtual_boundaries; i++)
                sps->virtual_boundary_pos_x_minus1[i] = get_ue_golomb(gb);
            for (i = 0; i < sps->num_hor_virtual_boundaries; i++)
                sps->virtual_boundary_pos_y_minus1[i] = get_ue_golomb(gb);
        }
    } else {
        sps->virtual_boundaries_present_flag = 0;
        sps->num_ver_virtual_boundaries = 0;
        sps->num_hor_virtual_boundaries = 0;
    }

    if (sps->ptl_dpb_hrd_params_present_flag) {
        sps->timing_hrd_params_present_flag = get_bits1(gb);
        if (sps->timing_hrd_params_present_flag) {
            uint8_t first_sublayer;
            decode_general_timing_hrd_parameters(gb,
                                          &sps->general_timing_hrd_parameters);

            if (sps->max_sublayers > 1)
                sps->sublayer_cpb_params_present_flag = get_bits1(gb);
            else
                sps->sublayer_cpb_params_present_flag = 0;

            first_sublayer = sps->sublayer_cpb_params_present_flag ?
                0 : sps->max_sublayers - 1;

            decode_ols_timing_hrd_parameters(gb,
                                             &sps->ols_timing_hrd_parameters,
                                             first_sublayer,
                                             sps->max_sublayers - 1,
                                             &sps->
                                             general_timing_hrd_parameters);
        }
    }

    sps->field_seq_flag = get_bits1(gb);
    sps->vui_parameters_present_flag = get_bits1(gb);
    if (sps->vui_parameters_present_flag) {
        sps->vui_payload_size_minus1 = get_ue_golomb(gb);
        align_get_bits(gb);
        decode_vui(gb, avctx, &sps->vui, sps->chroma_format_idc);
    }

    sps->extension_flag = get_bits1(gb);
    // TODO: parse sps extension flag and read extension data

    map_pixel_format(avctx, sps);

    return 0;
}

int ff_h266_decode_nal_sps( GetBitContext *gb, AVCodecContext *avctx,
                           H266ParamSets *ps, int apply_defdispwin)
{
    unsigned int sps_id;
    int ret;
    H266SPS *sps;
    AVBufferRef *sps_buf = av_buffer_allocz(sizeof(*sps));

    if (!sps_buf)
        return AVERROR(ENOMEM);
    sps = (H266SPS *) sps_buf->data;

    ret = ff_h266_parse_sps(sps, gb, &sps_id, apply_defdispwin, avctx);
    if (ret < 0) {
        av_buffer_unref(&sps_buf);
        return ret;
    }

    if (avctx->debug & FF_DEBUG_BITSTREAM) {
        av_log(avctx, AV_LOG_DEBUG,
               "Parsed SPS: id %d; coded wxh: %dx%d; "
               "pix_fmt: %s.\n",
               sps->sps_id, sps->pic_width_max_in_luma_samples,
               sps->pic_height_max_in_luma_samples,
               av_get_pix_fmt_name(sps->pix_fmt));
    }

    /* check if this is a repeat of an already parsed SPS, then keep the
     * original one otherwise drop all PPSes that depend on it (PPS,VPS not implemented yet) */
    if (ps->sps_list[sps_id] &&
        !memcmp(ps->sps_list[sps_id]->data, sps_buf->data, sps_buf->size)) {
        av_buffer_unref(&sps_buf);
    } else {
        remove_sps(ps, sps_id);
        ps->sps_list[sps_id] = sps_buf;
        ps->sps = (H266SPS *) ps->sps_list[sps_id]->data;
    }

    // TODO: read PPS flag and data

    return 0;
}

void ff_h266_ps_uninit(H266ParamSets *ps)
{
    int i;

    for (i = 0; i < FF_ARRAY_ELEMS(ps->sps_list); i++)
        av_buffer_unref(&ps->sps_list[i]);

    // TODO: if PPS, VPS is implemended it must be unrefed as well
    // for (i = 0; i < FF_ARRAY_ELEMS(ps->vps_list); i++)
    //     av_buffer_unref(&ps->vps_list[i]);
    // for (i = 0; i < FF_ARRAY_ELEMS(ps->pps_list); i++)
    //     av_buffer_unref(&ps->pps_list[i]);

    ps->sps = NULL;
    //ps->pps = NULL;
    //ps->vps = NULL;
}
