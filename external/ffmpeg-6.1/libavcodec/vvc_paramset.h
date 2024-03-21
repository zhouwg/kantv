/*
 * H.266 / VVC parameter set parsing
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

#ifndef AVCODEC_H266_PARAMSET_H
#define AVCODEC_H266_PARAMSET_H

#include <stdint.h>

#include "libavutil/buffer.h"
#include "libavutil/pixfmt.h"
#include "libavutil/rational.h"

#include "avcodec.h"
#include "cbs_h266.h"
#include "get_bits.h"
#include "vvc.h"

typedef struct H266GeneralTimingHrdParameters {
    uint32_t num_units_in_tick;
    uint32_t time_scale;
    uint8_t  general_nal_hrd_params_present_flag;
    uint8_t  general_vcl_hrd_params_present_flag;
    uint8_t  general_same_pic_timing_in_all_ols_flag;
    uint8_t  general_du_hrd_params_present_flag;
    uint8_t  tick_divisor_minus2;
    uint8_t  bit_rate_scale;
    uint8_t  cpb_size_scale;
    uint8_t  cpb_size_du_scale;
    uint8_t  hrd_cpb_cnt_minus1;
} H266GeneralTimingHrdParameters;

typedef struct H266SubLayerHRDParameters {
    uint32_t bit_rate_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
    uint32_t cpb_size_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
    uint32_t cpb_size_du_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
    uint32_t bit_rate_du_value_minus1[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
    uint8_t  cbr_flag[VVC_MAX_SUBLAYERS][VVC_MAX_CPB_CNT];
} H266SubLayerHRDParameters;

typedef struct H266OlsTimingHrdParameters {
    uint8_t  fixed_pic_rate_general_flag[VVC_MAX_SUBLAYERS];
    uint8_t  fixed_pic_rate_within_cvs_flag[VVC_MAX_SUBLAYERS];
    uint16_t elemental_duration_in_tc_minus1[VVC_MAX_SUBLAYERS];
    uint8_t  low_delay_hrd_flag[VVC_MAX_SUBLAYERS];
    H266SubLayerHRDParameters nal_sub_layer_hrd_parameters;
    H266SubLayerHRDParameters vcl_sub_layer_hrd_parameters;
} H266OlsTimingHrdParameters;


typedef struct H266VUI {
    uint8_t  progressive_source_flag;
    uint8_t  interlaced_source_flag;
    uint8_t  non_packed_constraint_flag;
    uint8_t  non_projected_constraint_flag;

    uint8_t  aspect_ratio_info_present_flag;
    uint8_t  aspect_ratio_constant_flag;
    uint8_t  aspect_ratio_idc;

    uint16_t sar_width;
    uint16_t sar_height;

    uint8_t  overscan_info_present_flag;
    uint8_t  overscan_appropriate_flag;

    uint8_t  colour_description_present_flag;
    uint8_t  colour_primaries;

    uint8_t  transfer_characteristics;
    uint8_t  matrix_coeffs;
    uint8_t  full_range_flag;

    uint8_t  chroma_loc_info_present_flag;
    uint8_t  chroma_sample_loc_type_frame;
    uint8_t  chroma_sample_loc_type_top_field;
    uint8_t  chroma_sample_loc_type_bottom_field;
    //H266ExtensionData extension_data;
} H266VUI;

typedef struct H266SPS {
    uint8_t  sps_id;
    uint8_t  vps_id;
    uint8_t  max_sublayers;
    uint8_t  chroma_format_idc;
    uint8_t  log2_ctu_size;
    uint8_t  ptl_dpb_hrd_params_present_flag;
    H266RawProfileTierLevel profile_tier_level;
    uint8_t  gdr_enabled_flag;
    uint8_t  ref_pic_resampling_enabled_flag;
    uint8_t  res_change_in_clvs_allowed_flag;

    uint16_t pic_width_max_in_luma_samples;
    uint16_t pic_height_max_in_luma_samples;

    uint8_t  conformance_window_flag;
    uint16_t conf_win_left_offset;
    uint16_t conf_win_right_offset;
    uint16_t conf_win_top_offset;
    uint16_t conf_win_bottom_offset;

    uint8_t  subpic_info_present_flag;
    uint16_t num_subpics_minus1;
    uint8_t  independent_subpics_flag;
    uint8_t  subpic_same_size_flag;
    uint16_t subpic_ctu_top_left_x[VVC_MAX_SLICES];
    uint16_t subpic_ctu_top_left_y[VVC_MAX_SLICES];
    uint16_t subpic_width_minus1[VVC_MAX_SLICES];
    uint16_t subpic_height_minus1[VVC_MAX_SLICES];
    uint8_t  subpic_treated_as_pic_flag[VVC_MAX_SLICES];
    uint8_t  loop_filter_across_subpic_enabled_flag[VVC_MAX_SLICES];
    uint8_t  subpic_id_len_minus1;
    uint8_t  subpic_id_mapping_explicitly_signalled_flag;
    uint8_t  subpic_id_mapping_present_flag;
    uint32_t subpic_id[VVC_MAX_SLICES];


    uint8_t  bit_depth;
    uint8_t  entropy_coding_sync_enabled_flag;
    uint8_t  entry_point_offsets_present_flag;

    uint8_t  log2_max_pic_order_cnt_lsb_minus4;
    uint8_t  poc_msb_cycle_flag;
    uint8_t  poc_msb_cycle_len_minus1;

    uint8_t  num_extra_ph_bytes;
    uint8_t  extra_ph_bit_present_flag[16];

    uint8_t  num_extra_sh_bytes;
    uint8_t  extra_sh_bit_present_flag[16];

    uint8_t  sublayer_dpb_params_flag;
    H266DpbParameters dpb_params;

    uint8_t  log2_min_luma_coding_block_size_minus2;
    uint8_t  partition_constraints_override_enabled_flag;
    uint8_t  log2_diff_min_qt_min_cb_intra_slice_luma;
    uint8_t  max_mtt_hierarchy_depth_intra_slice_luma;
    uint8_t  log2_diff_max_bt_min_qt_intra_slice_luma;
    uint8_t  log2_diff_max_tt_min_qt_intra_slice_luma;

    uint8_t  qtbtt_dual_tree_intra_flag;
    uint8_t  log2_diff_min_qt_min_cb_intra_slice_chroma;
    uint8_t  max_mtt_hierarchy_depth_intra_slice_chroma;
    uint8_t  log2_diff_max_bt_min_qt_intra_slice_chroma;
    uint8_t  log2_diff_max_tt_min_qt_intra_slice_chroma;

    uint8_t  log2_diff_min_qt_min_cb_inter_slice;
    uint8_t  max_mtt_hierarchy_depth_inter_slice;
    uint8_t  log2_diff_max_bt_min_qt_inter_slice;
    uint8_t  log2_diff_max_tt_min_qt_inter_slice;

    uint8_t  max_luma_transform_size_64_flag;

    uint8_t  transform_skip_enabled_flag;
    uint8_t  log2_transform_skip_max_size_minus2;
    uint8_t  bdpcm_enabled_flag;

    uint8_t  mts_enabled_flag;
    uint8_t  explicit_mts_intra_enabled_flag;
    uint8_t  explicit_mts_inter_enabled_flag;

    uint8_t  lfnst_enabled_flag;

    uint8_t  joint_cbcr_enabled_flag;
    uint8_t  same_qp_table_for_chroma_flag;

    int8_t   qp_table_start_minus26[VVC_MAX_SAMPLE_ARRAYS];
    uint8_t  num_points_in_qp_table_minus1[VVC_MAX_SAMPLE_ARRAYS];
    uint8_t  delta_qp_in_val_minus1[VVC_MAX_SAMPLE_ARRAYS][VVC_MAX_POINTS_IN_QP_TABLE];
    uint8_t  delta_qp_diff_val[VVC_MAX_SAMPLE_ARRAYS][VVC_MAX_POINTS_IN_QP_TABLE];

    uint8_t  sao_enabled_flag;
    uint8_t  alf_enabled_flag;
    uint8_t  ccalf_enabled_flag;
    uint8_t  lmcs_enabled_flag;
    uint8_t  weighted_pred_flag;
    uint8_t  weighted_bipred_flag;
    uint8_t  long_term_ref_pics_flag;
    uint8_t  inter_layer_prediction_enabled_flag;
    uint8_t  idr_rpl_present_flag;
    uint8_t  rpl1_same_as_rpl0_flag;

    uint8_t  num_ref_pic_lists[2];
    H266RefPicListStruct ref_pic_list_struct[2][VVC_MAX_REF_PIC_LISTS];

    uint8_t  ref_wraparound_enabled_flag;
    uint8_t  temporal_mvp_enabled_flag;
    uint8_t  sbtmvp_enabled_flag;
    uint8_t  amvr_enabled_flag;
    uint8_t  bdof_enabled_flag;
    uint8_t  bdof_control_present_in_ph_flag;
    uint8_t  smvd_enabled_flag;
    uint8_t  dmvr_enabled_flag;
    uint8_t  dmvr_control_present_in_ph_flag;
    uint8_t  mmvd_enabled_flag;
    uint8_t  mmvd_fullpel_only_enabled_flag;
    uint8_t  six_minus_max_num_merge_cand;
    uint8_t  sbt_enabled_flag;
    uint8_t  affine_enabled_flag;
    uint8_t  five_minus_max_num_subblock_merge_cand;
    uint8_t  param_affine_enabled_flag;
    uint8_t  affine_amvr_enabled_flag;
    uint8_t  affine_prof_enabled_flag;
    uint8_t  prof_control_present_in_ph_flag;
    uint8_t  bcw_enabled_flag;
    uint8_t  ciip_enabled_flag;
    uint8_t  gpm_enabled_flag;
    uint8_t  max_num_merge_cand_minus_max_num_gpm_cand;
    uint8_t  log2_parallel_merge_level_minus2;
    uint8_t  isp_enabled_flag;
    uint8_t  mrl_enabled_flag;
    uint8_t  mip_enabled_flag;
    uint8_t  cclm_enabled_flag;
    uint8_t  chroma_horizontal_collocated_flag;
    uint8_t  chroma_vertical_collocated_flag;
    uint8_t  palette_enabled_flag;
    uint8_t  act_enabled_flag;
    uint8_t  min_qp_prime_ts;
    uint8_t  ibc_enabled_flag;
    uint8_t  six_minus_max_num_ibc_merge_cand;
    uint8_t  ladf_enabled_flag;
    uint8_t  num_ladf_intervals_minus2;
    int8_t   ladf_lowest_interval_qp_offset;
    int8_t   ladf_qp_offset[4];
    uint16_t ladf_delta_threshold_minus1[4];

    uint8_t  explicit_scaling_list_enabled_flag;
    uint8_t  scaling_matrix_for_lfnst_disabled_flag;
    uint8_t  scaling_matrix_for_alternative_colour_space_disabled_flag;
    uint8_t  scaling_matrix_designated_colour_space_flag;
    uint8_t  dep_quant_enabled_flag;
    uint8_t  sign_data_hiding_enabled_flag;

    uint8_t  virtual_boundaries_enabled_flag;
    uint8_t  virtual_boundaries_present_flag;
    uint8_t  num_ver_virtual_boundaries;
    uint16_t virtual_boundary_pos_x_minus1[3];
    uint8_t  num_hor_virtual_boundaries;
    uint16_t virtual_boundary_pos_y_minus1[3];

    uint8_t  timing_hrd_params_present_flag;
    uint8_t  sublayer_cpb_params_present_flag;
    H266GeneralTimingHrdParameters general_timing_hrd_parameters;
    H266OlsTimingHrdParameters ols_timing_hrd_parameters;

    uint8_t  field_seq_flag;
    uint8_t  vui_parameters_present_flag;
    uint16_t vui_payload_size_minus1;
    H266VUI   vui;

    uint8_t  extension_flag;
    //H266RawExtensionData extension_data;   /* TODO: read extension flag and data*/

    enum AVPixelFormat pix_fmt;
} H266SPS;


typedef struct H266ParamSets {
    AVBufferRef *sps_list[VVC_MAX_SPS_COUNT];
    //AVBufferRef *vps_list[VVC_MAX_VPS_COUNT]; // TODO: since not needed, not implemented yet
    //AVBufferRef *pps_list[VVC_MAX_PPS_COUNT]; // TODO: since not needed, not implemented yet

    /* currently active parameter sets */
    const H266SPS *sps;
    //const H266VPS *vps; // TODO: since not needed, not implemented yet
    //const H266PPS *pps; // TODO: since not needed, not implemented yet
} H266ParamSets;

/**
 * Parse the SPS from the bitstream into the provided HEVCSPS struct.
 *
 * @param sps_id the SPS id will be written here
 * @param apply_defdispwin if set 1, the default display window from the VUI
 *                         will be applied to the video dimensions
 */
int ff_h266_parse_sps(H266SPS *sps, GetBitContext *gb, unsigned int *sps_id,
                      int apply_defdispwin, AVCodecContext *avctx);

int ff_h266_decode_nal_sps(GetBitContext *gb, AVCodecContext *avctx,
                          H266ParamSets *ps, int apply_defdispwin);

// TODO: since not needed, not implemented yet
//int ff_h266_decode_nal_vps(GetBitContext *gb, AVCodecContext *avctx,
//                          H266ParamSets *ps);
//int ff_h266_decode_nal_pps(GetBitContext *gb, AVCodecContext *avctx,
//                          H266ParamSets *ps);

void ff_h266_ps_uninit(H266ParamSets *ps);

#endif /* AVCODEC_H266_PARAMSET_H */
