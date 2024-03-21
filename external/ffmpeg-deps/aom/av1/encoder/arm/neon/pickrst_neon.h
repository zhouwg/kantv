/*
 * Copyright (c) 2023, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_ARM_NEON_PICKRST_NEON_H_
#define AOM_AV1_ENCODER_ARM_NEON_PICKRST_NEON_H_

#include <arm_neon.h>

#include "aom_dsp/arm/sum_neon.h"
#include "aom_dsp/arm/transpose_neon.h"

// When we load 8 values of int16_t type and need less than 8 values for
// processing, the below mask is used to make the extra values zero.
extern const int16_t av1_neon_mask_16bit[16];

static INLINE void copy_upper_triangle(int64_t *H, const int wiener_win2) {
  for (int i = 0; i < wiener_win2 - 2; i = i + 2) {
    // Transpose the first 2x2 square. It needs a special case as the element
    // of the bottom left is on the diagonal.
    int64x2_t row0 = vld1q_s64(H + i * wiener_win2 + i + 1);
    int64x2_t row1 = vld1q_s64(H + (i + 1) * wiener_win2 + i + 1);

    int64x2_t tr_row = aom_vtrn2q_s64(row0, row1);

    vst1_s64(H + (i + 1) * wiener_win2 + i, vget_low_s64(row0));
    vst1q_s64(H + (i + 2) * wiener_win2 + i, tr_row);

    // Transpose and store all the remaining 2x2 squares of the line.
    for (int j = i + 3; j < wiener_win2; j = j + 2) {
      row0 = vld1q_s64(H + i * wiener_win2 + j);
      row1 = vld1q_s64(H + (i + 1) * wiener_win2 + j);

      int64x2_t tr_row0 = aom_vtrn1q_s64(row0, row1);
      int64x2_t tr_row1 = aom_vtrn2q_s64(row0, row1);

      vst1q_s64(H + j * wiener_win2 + i, tr_row0);
      vst1q_s64(H + (j + 1) * wiener_win2 + i, tr_row1);
    }
  }
}

static INLINE void transpose_M_win5(int64_t *M, int64_t *M_trn,
                                    const int wiener_win) {
  // 1st and 2nd rows.
  int64x2_t row00 = vld1q_s64(M_trn);
  int64x2_t row10 = vld1q_s64(M_trn + wiener_win);
  vst1q_s64(M, aom_vtrn1q_s64(row00, row10));
  vst1q_s64(M + wiener_win, aom_vtrn2q_s64(row00, row10));

  int64x2_t row02 = vld1q_s64(M_trn + 2);
  int64x2_t row12 = vld1q_s64(M_trn + wiener_win + 2);
  vst1q_s64(M + 2 * wiener_win, aom_vtrn1q_s64(row02, row12));
  vst1q_s64(M + 3 * wiener_win, aom_vtrn2q_s64(row02, row12));

  // Last column only needs trn2.
  int64x2_t row03 = vld1q_s64(M_trn + 3);
  int64x2_t row13 = vld1q_s64(M_trn + wiener_win + 3);
  vst1q_s64(M + 4 * wiener_win, aom_vtrn2q_s64(row03, row13));

  // 3rd and 4th rows.
  int64x2_t row20 = vld1q_s64(M_trn + 2 * wiener_win);
  int64x2_t row30 = vld1q_s64(M_trn + 3 * wiener_win);
  vst1q_s64(M + 2, aom_vtrn1q_s64(row20, row30));
  vst1q_s64(M + wiener_win + 2, aom_vtrn2q_s64(row20, row30));

  int64x2_t row22 = vld1q_s64(M_trn + 2 * wiener_win + 2);
  int64x2_t row32 = vld1q_s64(M_trn + 3 * wiener_win + 2);
  vst1q_s64(M + 2 * wiener_win + 2, aom_vtrn1q_s64(row22, row32));
  vst1q_s64(M + 3 * wiener_win + 2, aom_vtrn2q_s64(row22, row32));

  // Last column only needs trn2.
  int64x2_t row23 = vld1q_s64(M_trn + 2 * wiener_win + 3);
  int64x2_t row33 = vld1q_s64(M_trn + 3 * wiener_win + 3);
  vst1q_s64(M + 4 * wiener_win + 2, aom_vtrn2q_s64(row23, row33));

  // Last row.
  int64x2_t row40 = vld1q_s64(M_trn + 4 * wiener_win);
  vst1_s64(M + 4, vget_low_s64(row40));
  vst1_s64(M + 1 * wiener_win + 4, vget_high_s64(row40));

  int64x2_t row42 = vld1q_s64(M_trn + 4 * wiener_win + 2);
  vst1_s64(M + 2 * wiener_win + 4, vget_low_s64(row42));
  vst1_s64(M + 3 * wiener_win + 4, vget_high_s64(row42));

  // Element on the bottom right of M_trn is copied as is.
  vst1_s64(M + 4 * wiener_win + 4, vld1_s64(M_trn + 4 * wiener_win + 4));
}

static INLINE void transpose_M_win7(int64_t *M, int64_t *M_trn,
                                    const int wiener_win) {
  // 1st and 2nd rows.
  int64x2_t row00 = vld1q_s64(M_trn);
  int64x2_t row10 = vld1q_s64(M_trn + wiener_win);
  vst1q_s64(M, aom_vtrn1q_s64(row00, row10));
  vst1q_s64(M + wiener_win, aom_vtrn2q_s64(row00, row10));

  int64x2_t row02 = vld1q_s64(M_trn + 2);
  int64x2_t row12 = vld1q_s64(M_trn + wiener_win + 2);
  vst1q_s64(M + 2 * wiener_win, aom_vtrn1q_s64(row02, row12));
  vst1q_s64(M + 3 * wiener_win, aom_vtrn2q_s64(row02, row12));

  int64x2_t row04 = vld1q_s64(M_trn + 4);
  int64x2_t row14 = vld1q_s64(M_trn + wiener_win + 4);
  vst1q_s64(M + 4 * wiener_win, aom_vtrn1q_s64(row04, row14));
  vst1q_s64(M + 5 * wiener_win, aom_vtrn2q_s64(row04, row14));

  // Last column only needs trn2.
  int64x2_t row05 = vld1q_s64(M_trn + 5);
  int64x2_t row15 = vld1q_s64(M_trn + wiener_win + 5);
  vst1q_s64(M + 6 * wiener_win, aom_vtrn2q_s64(row05, row15));

  // 3rd and 4th rows.
  int64x2_t row20 = vld1q_s64(M_trn + 2 * wiener_win);
  int64x2_t row30 = vld1q_s64(M_trn + 3 * wiener_win);
  vst1q_s64(M + 2, aom_vtrn1q_s64(row20, row30));
  vst1q_s64(M + wiener_win + 2, aom_vtrn2q_s64(row20, row30));

  int64x2_t row22 = vld1q_s64(M_trn + 2 * wiener_win + 2);
  int64x2_t row32 = vld1q_s64(M_trn + 3 * wiener_win + 2);
  vst1q_s64(M + 2 * wiener_win + 2, aom_vtrn1q_s64(row22, row32));
  vst1q_s64(M + 3 * wiener_win + 2, aom_vtrn2q_s64(row22, row32));

  int64x2_t row24 = vld1q_s64(M_trn + 2 * wiener_win + 4);
  int64x2_t row34 = vld1q_s64(M_trn + 3 * wiener_win + 4);
  vst1q_s64(M + 4 * wiener_win + 2, aom_vtrn1q_s64(row24, row34));
  vst1q_s64(M + 5 * wiener_win + 2, aom_vtrn2q_s64(row24, row34));

  // Last column only needs trn2.
  int64x2_t row25 = vld1q_s64(M_trn + 2 * wiener_win + 5);
  int64x2_t row35 = vld1q_s64(M_trn + 3 * wiener_win + 5);
  vst1q_s64(M + 6 * wiener_win + 2, aom_vtrn2q_s64(row25, row35));

  // 5th and 6th rows.
  int64x2_t row40 = vld1q_s64(M_trn + 4 * wiener_win);
  int64x2_t row50 = vld1q_s64(M_trn + 5 * wiener_win);
  vst1q_s64(M + 4, aom_vtrn1q_s64(row40, row50));
  vst1q_s64(M + wiener_win + 4, aom_vtrn2q_s64(row40, row50));

  int64x2_t row42 = vld1q_s64(M_trn + 4 * wiener_win + 2);
  int64x2_t row52 = vld1q_s64(M_trn + 5 * wiener_win + 2);
  vst1q_s64(M + 2 * wiener_win + 4, aom_vtrn1q_s64(row42, row52));
  vst1q_s64(M + 3 * wiener_win + 4, aom_vtrn2q_s64(row42, row52));

  int64x2_t row44 = vld1q_s64(M_trn + 4 * wiener_win + 4);
  int64x2_t row54 = vld1q_s64(M_trn + 5 * wiener_win + 4);
  vst1q_s64(M + 4 * wiener_win + 4, aom_vtrn1q_s64(row44, row54));
  vst1q_s64(M + 5 * wiener_win + 4, aom_vtrn2q_s64(row44, row54));

  // Last column only needs trn2.
  int64x2_t row45 = vld1q_s64(M_trn + 4 * wiener_win + 5);
  int64x2_t row55 = vld1q_s64(M_trn + 5 * wiener_win + 5);
  vst1q_s64(M + 6 * wiener_win + 4, aom_vtrn2q_s64(row45, row55));

  // Last row.
  int64x2_t row60 = vld1q_s64(M_trn + 6 * wiener_win);
  vst1_s64(M + 6, vget_low_s64(row60));
  vst1_s64(M + 1 * wiener_win + 6, vget_high_s64(row60));

  int64x2_t row62 = vld1q_s64(M_trn + 6 * wiener_win + 2);
  vst1_s64(M + 2 * wiener_win + 6, vget_low_s64(row62));
  vst1_s64(M + 3 * wiener_win + 6, vget_high_s64(row62));

  int64x2_t row64 = vld1q_s64(M_trn + 6 * wiener_win + 4);
  vst1_s64(M + 4 * wiener_win + 6, vget_low_s64(row64));
  vst1_s64(M + 5 * wiener_win + 6, vget_high_s64(row64));

  // Element on the bottom right of M_trn is copied as is.
  vst1_s64(M + 6 * wiener_win + 6, vld1_s64(M_trn + 6 * wiener_win + 6));
}

static INLINE void compute_M_one_row_win5(int16x8_t src, int16x8_t dgd0,
                                          int16x8_t dgd1, int64_t *M,
                                          const int wiener_win, int row) {
  int64x2_t m_01 = vld1q_s64(M + row * wiener_win + 0);
  int64x2_t m_23 = vld1q_s64(M + row * wiener_win + 2);

  int32x4_t m0 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd0));
  m0 = vmlal_s16(m0, vget_high_s16(src), vget_high_s16(dgd0));

  int16x8_t dgd01 = vextq_s16(dgd0, dgd1, 1);
  int32x4_t m1 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd01));
  m1 = vmlal_s16(m1, vget_high_s16(src), vget_high_s16(dgd01));

  m0 = horizontal_add_2d_s32(m0, m1);
  m_01 = vpadalq_s32(m_01, m0);
  vst1q_s64(M + row * wiener_win + 0, m_01);

  int16x8_t dgd02 = vextq_s16(dgd0, dgd1, 2);
  int32x4_t m2 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd02));
  m2 = vmlal_s16(m2, vget_high_s16(src), vget_high_s16(dgd02));

  int16x8_t dgd03 = vextq_s16(dgd0, dgd1, 3);
  int32x4_t m3 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd03));
  m3 = vmlal_s16(m3, vget_high_s16(src), vget_high_s16(dgd03));

  m2 = horizontal_add_2d_s32(m2, m3);
  m_23 = vpadalq_s32(m_23, m2);
  vst1q_s64(M + row * wiener_win + 2, m_23);

  int16x8_t dgd04 = vextq_s16(dgd0, dgd1, 4);
  int32x4_t m4 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd04));
  m4 = vmlal_s16(m4, vget_high_s16(src), vget_high_s16(dgd04));
  M[row * wiener_win + 4] += horizontal_long_add_s32x4(m4);
}

static INLINE void compute_M_one_row_win7(int16x8_t src, int16x8_t dgd0,
                                          int16x8_t dgd1, int64_t *M,
                                          const int wiener_win, int row) {
  int64x2_t m_01 = vld1q_s64(M + row * wiener_win + 0);
  int64x2_t m_23 = vld1q_s64(M + row * wiener_win + 2);
  int64x2_t m_45 = vld1q_s64(M + row * wiener_win + 4);

  int32x4_t m0 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd0));
  m0 = vmlal_s16(m0, vget_high_s16(src), vget_high_s16(dgd0));

  int16x8_t dgd01 = vextq_s16(dgd0, dgd1, 1);
  int32x4_t m1 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd01));
  m1 = vmlal_s16(m1, vget_high_s16(src), vget_high_s16(dgd01));

  m0 = horizontal_add_2d_s32(m0, m1);
  m_01 = vpadalq_s32(m_01, m0);
  vst1q_s64(M + row * wiener_win + 0, m_01);

  int16x8_t dgd02 = vextq_s16(dgd0, dgd1, 2);
  int32x4_t m2 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd02));
  m2 = vmlal_s16(m2, vget_high_s16(src), vget_high_s16(dgd02));

  int16x8_t dgd03 = vextq_s16(dgd0, dgd1, 3);
  int32x4_t m3 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd03));
  m3 = vmlal_s16(m3, vget_high_s16(src), vget_high_s16(dgd03));

  m2 = horizontal_add_2d_s32(m2, m3);
  m_23 = vpadalq_s32(m_23, m2);
  vst1q_s64(M + row * wiener_win + 2, m_23);

  int16x8_t dgd04 = vextq_s16(dgd0, dgd1, 4);
  int32x4_t m4 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd04));
  m4 = vmlal_s16(m4, vget_high_s16(src), vget_high_s16(dgd04));

  int16x8_t dgd05 = vextq_s16(dgd0, dgd1, 5);
  int32x4_t m5 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd05));
  m5 = vmlal_s16(m5, vget_high_s16(src), vget_high_s16(dgd05));

  m4 = horizontal_add_2d_s32(m4, m5);
  m_45 = vpadalq_s32(m_45, m4);
  vst1q_s64(M + row * wiener_win + 4, m_45);

  int16x8_t dgd06 = vextq_s16(dgd0, dgd1, 6);
  int32x4_t m6 = vmull_s16(vget_low_s16(src), vget_low_s16(dgd06));
  m6 = vmlal_s16(m6, vget_high_s16(src), vget_high_s16(dgd06));
  M[row * wiener_win + 6] += horizontal_long_add_s32x4(m6);
}

static INLINE void compute_H_two_cols(int16x8_t *dgd0, int16x8_t *dgd1,
                                      int col0, int col1, int64_t *H,
                                      const int wiener_win,
                                      const int wiener_win2) {
  for (int row0 = 0; row0 < wiener_win; row0++) {
    for (int row1 = 0; row1 < wiener_win; row1++) {
      int auto_cov_idx =
          (col0 * wiener_win + row0) * wiener_win2 + (col1 * wiener_win) + row1;

      int32x4_t auto_cov =
          vmull_s16(vget_low_s16(dgd0[row0]), vget_low_s16(dgd1[row1]));
      auto_cov = vmlal_s16(auto_cov, vget_high_s16(dgd0[row0]),
                           vget_high_s16(dgd1[row1]));

      H[auto_cov_idx] += horizontal_long_add_s32x4(auto_cov);
    }
  }
}

#endif  // AOM_AV1_ENCODER_ARM_NEON_PICKRST_NEON_H_
