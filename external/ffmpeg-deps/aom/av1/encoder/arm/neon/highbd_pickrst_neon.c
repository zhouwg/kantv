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

#include <arm_neon.h>
#include <assert.h>
#include <stdint.h>

#include "aom_dsp/arm/mem_neon.h"
#include "aom_dsp/arm/sum_neon.h"
#include "aom_dsp/arm/transpose_neon.h"
#include "av1/encoder/arm/neon/pickrst_neon.h"
#include "av1/encoder/pickrst.h"

static INLINE void highbd_calc_proj_params_r0_r1_neon(
    const uint8_t *src8, int width, int height, int src_stride,
    const uint8_t *dat8, int dat_stride, int32_t *flt0, int flt0_stride,
    int32_t *flt1, int flt1_stride, int64_t H[2][2], int64_t C[2]) {
  assert(width % 8 == 0);
  const int size = width * height;
  const uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  const uint16_t *dat = CONVERT_TO_SHORTPTR(dat8);

  int64x2_t h00_lo = vdupq_n_s64(0);
  int64x2_t h00_hi = vdupq_n_s64(0);
  int64x2_t h11_lo = vdupq_n_s64(0);
  int64x2_t h11_hi = vdupq_n_s64(0);
  int64x2_t h01_lo = vdupq_n_s64(0);
  int64x2_t h01_hi = vdupq_n_s64(0);
  int64x2_t c0_lo = vdupq_n_s64(0);
  int64x2_t c0_hi = vdupq_n_s64(0);
  int64x2_t c1_lo = vdupq_n_s64(0);
  int64x2_t c1_hi = vdupq_n_s64(0);

  do {
    const uint16_t *src_ptr = src;
    const uint16_t *dat_ptr = dat;
    int32_t *flt0_ptr = flt0;
    int32_t *flt1_ptr = flt1;
    int w = width;

    do {
      uint16x8_t s = vld1q_u16(src_ptr);
      uint16x8_t d = vld1q_u16(dat_ptr);
      int32x4_t f0_lo = vld1q_s32(flt0_ptr);
      int32x4_t f0_hi = vld1q_s32(flt0_ptr + 4);
      int32x4_t f1_lo = vld1q_s32(flt1_ptr);
      int32x4_t f1_hi = vld1q_s32(flt1_ptr + 4);

      int32x4_t u_lo =
          vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(d), SGRPROJ_RST_BITS));
      int32x4_t u_hi = vreinterpretq_s32_u32(
          vshll_n_u16(vget_high_u16(d), SGRPROJ_RST_BITS));
      int32x4_t s_lo =
          vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(s), SGRPROJ_RST_BITS));
      int32x4_t s_hi = vreinterpretq_s32_u32(
          vshll_n_u16(vget_high_u16(s), SGRPROJ_RST_BITS));
      s_lo = vsubq_s32(s_lo, u_lo);
      s_hi = vsubq_s32(s_hi, u_hi);

      f0_lo = vsubq_s32(f0_lo, u_lo);
      f0_hi = vsubq_s32(f0_hi, u_hi);
      f1_lo = vsubq_s32(f1_lo, u_lo);
      f1_hi = vsubq_s32(f1_hi, u_hi);

      h00_lo = vmlal_s32(h00_lo, vget_low_s32(f0_lo), vget_low_s32(f0_lo));
      h00_lo = vmlal_s32(h00_lo, vget_high_s32(f0_lo), vget_high_s32(f0_lo));
      h00_hi = vmlal_s32(h00_hi, vget_low_s32(f0_hi), vget_low_s32(f0_hi));
      h00_hi = vmlal_s32(h00_hi, vget_high_s32(f0_hi), vget_high_s32(f0_hi));

      h11_lo = vmlal_s32(h11_lo, vget_low_s32(f1_lo), vget_low_s32(f1_lo));
      h11_lo = vmlal_s32(h11_lo, vget_high_s32(f1_lo), vget_high_s32(f1_lo));
      h11_hi = vmlal_s32(h11_hi, vget_low_s32(f1_hi), vget_low_s32(f1_hi));
      h11_hi = vmlal_s32(h11_hi, vget_high_s32(f1_hi), vget_high_s32(f1_hi));

      h01_lo = vmlal_s32(h01_lo, vget_low_s32(f0_lo), vget_low_s32(f1_lo));
      h01_lo = vmlal_s32(h01_lo, vget_high_s32(f0_lo), vget_high_s32(f1_lo));
      h01_hi = vmlal_s32(h01_hi, vget_low_s32(f0_hi), vget_low_s32(f1_hi));
      h01_hi = vmlal_s32(h01_hi, vget_high_s32(f0_hi), vget_high_s32(f1_hi));

      c0_lo = vmlal_s32(c0_lo, vget_low_s32(f0_lo), vget_low_s32(s_lo));
      c0_lo = vmlal_s32(c0_lo, vget_high_s32(f0_lo), vget_high_s32(s_lo));
      c0_hi = vmlal_s32(c0_hi, vget_low_s32(f0_hi), vget_low_s32(s_hi));
      c0_hi = vmlal_s32(c0_hi, vget_high_s32(f0_hi), vget_high_s32(s_hi));

      c1_lo = vmlal_s32(c1_lo, vget_low_s32(f1_lo), vget_low_s32(s_lo));
      c1_lo = vmlal_s32(c1_lo, vget_high_s32(f1_lo), vget_high_s32(s_lo));
      c1_hi = vmlal_s32(c1_hi, vget_low_s32(f1_hi), vget_low_s32(s_hi));
      c1_hi = vmlal_s32(c1_hi, vget_high_s32(f1_hi), vget_high_s32(s_hi));

      src_ptr += 8;
      dat_ptr += 8;
      flt0_ptr += 8;
      flt1_ptr += 8;
      w -= 8;
    } while (w != 0);

    src += src_stride;
    dat += dat_stride;
    flt0 += flt0_stride;
    flt1 += flt1_stride;
  } while (--height != 0);

  H[0][0] = horizontal_add_s64x2(vaddq_s64(h00_lo, h00_hi)) / size;
  H[0][1] = horizontal_add_s64x2(vaddq_s64(h01_lo, h01_hi)) / size;
  H[1][1] = horizontal_add_s64x2(vaddq_s64(h11_lo, h11_hi)) / size;
  H[1][0] = H[0][1];
  C[0] = horizontal_add_s64x2(vaddq_s64(c0_lo, c0_hi)) / size;
  C[1] = horizontal_add_s64x2(vaddq_s64(c1_lo, c1_hi)) / size;
}

static INLINE void highbd_calc_proj_params_r0_neon(
    const uint8_t *src8, int width, int height, int src_stride,
    const uint8_t *dat8, int dat_stride, int32_t *flt0, int flt0_stride,
    int64_t H[2][2], int64_t C[2]) {
  assert(width % 8 == 0);
  const int size = width * height;
  const uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  const uint16_t *dat = CONVERT_TO_SHORTPTR(dat8);

  int64x2_t h00_lo = vdupq_n_s64(0);
  int64x2_t h00_hi = vdupq_n_s64(0);
  int64x2_t c0_lo = vdupq_n_s64(0);
  int64x2_t c0_hi = vdupq_n_s64(0);

  do {
    const uint16_t *src_ptr = src;
    const uint16_t *dat_ptr = dat;
    int32_t *flt0_ptr = flt0;
    int w = width;

    do {
      uint16x8_t s = vld1q_u16(src_ptr);
      uint16x8_t d = vld1q_u16(dat_ptr);
      int32x4_t f0_lo = vld1q_s32(flt0_ptr);
      int32x4_t f0_hi = vld1q_s32(flt0_ptr + 4);

      int32x4_t u_lo =
          vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(d), SGRPROJ_RST_BITS));
      int32x4_t u_hi = vreinterpretq_s32_u32(
          vshll_n_u16(vget_high_u16(d), SGRPROJ_RST_BITS));
      int32x4_t s_lo =
          vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(s), SGRPROJ_RST_BITS));
      int32x4_t s_hi = vreinterpretq_s32_u32(
          vshll_n_u16(vget_high_u16(s), SGRPROJ_RST_BITS));
      s_lo = vsubq_s32(s_lo, u_lo);
      s_hi = vsubq_s32(s_hi, u_hi);

      f0_lo = vsubq_s32(f0_lo, u_lo);
      f0_hi = vsubq_s32(f0_hi, u_hi);

      h00_lo = vmlal_s32(h00_lo, vget_low_s32(f0_lo), vget_low_s32(f0_lo));
      h00_lo = vmlal_s32(h00_lo, vget_high_s32(f0_lo), vget_high_s32(f0_lo));
      h00_hi = vmlal_s32(h00_hi, vget_low_s32(f0_hi), vget_low_s32(f0_hi));
      h00_hi = vmlal_s32(h00_hi, vget_high_s32(f0_hi), vget_high_s32(f0_hi));

      c0_lo = vmlal_s32(c0_lo, vget_low_s32(f0_lo), vget_low_s32(s_lo));
      c0_lo = vmlal_s32(c0_lo, vget_high_s32(f0_lo), vget_high_s32(s_lo));
      c0_hi = vmlal_s32(c0_hi, vget_low_s32(f0_hi), vget_low_s32(s_hi));
      c0_hi = vmlal_s32(c0_hi, vget_high_s32(f0_hi), vget_high_s32(s_hi));

      src_ptr += 8;
      dat_ptr += 8;
      flt0_ptr += 8;
      w -= 8;
    } while (w != 0);

    src += src_stride;
    dat += dat_stride;
    flt0 += flt0_stride;
  } while (--height != 0);

  H[0][0] = horizontal_add_s64x2(vaddq_s64(h00_lo, h00_hi)) / size;
  C[0] = horizontal_add_s64x2(vaddq_s64(c0_lo, c0_hi)) / size;
}

static INLINE void highbd_calc_proj_params_r1_neon(
    const uint8_t *src8, int width, int height, int src_stride,
    const uint8_t *dat8, int dat_stride, int32_t *flt1, int flt1_stride,
    int64_t H[2][2], int64_t C[2]) {
  assert(width % 8 == 0);
  const int size = width * height;
  const uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  const uint16_t *dat = CONVERT_TO_SHORTPTR(dat8);

  int64x2_t h11_lo = vdupq_n_s64(0);
  int64x2_t h11_hi = vdupq_n_s64(0);
  int64x2_t c1_lo = vdupq_n_s64(0);
  int64x2_t c1_hi = vdupq_n_s64(0);

  do {
    const uint16_t *src_ptr = src;
    const uint16_t *dat_ptr = dat;
    int32_t *flt1_ptr = flt1;
    int w = width;

    do {
      uint16x8_t s = vld1q_u16(src_ptr);
      uint16x8_t d = vld1q_u16(dat_ptr);
      int32x4_t f1_lo = vld1q_s32(flt1_ptr);
      int32x4_t f1_hi = vld1q_s32(flt1_ptr + 4);

      int32x4_t u_lo =
          vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(d), SGRPROJ_RST_BITS));
      int32x4_t u_hi = vreinterpretq_s32_u32(
          vshll_n_u16(vget_high_u16(d), SGRPROJ_RST_BITS));
      int32x4_t s_lo =
          vreinterpretq_s32_u32(vshll_n_u16(vget_low_u16(s), SGRPROJ_RST_BITS));
      int32x4_t s_hi = vreinterpretq_s32_u32(
          vshll_n_u16(vget_high_u16(s), SGRPROJ_RST_BITS));
      s_lo = vsubq_s32(s_lo, u_lo);
      s_hi = vsubq_s32(s_hi, u_hi);

      f1_lo = vsubq_s32(f1_lo, u_lo);
      f1_hi = vsubq_s32(f1_hi, u_hi);

      h11_lo = vmlal_s32(h11_lo, vget_low_s32(f1_lo), vget_low_s32(f1_lo));
      h11_lo = vmlal_s32(h11_lo, vget_high_s32(f1_lo), vget_high_s32(f1_lo));
      h11_hi = vmlal_s32(h11_hi, vget_low_s32(f1_hi), vget_low_s32(f1_hi));
      h11_hi = vmlal_s32(h11_hi, vget_high_s32(f1_hi), vget_high_s32(f1_hi));

      c1_lo = vmlal_s32(c1_lo, vget_low_s32(f1_lo), vget_low_s32(s_lo));
      c1_lo = vmlal_s32(c1_lo, vget_high_s32(f1_lo), vget_high_s32(s_lo));
      c1_hi = vmlal_s32(c1_hi, vget_low_s32(f1_hi), vget_low_s32(s_hi));
      c1_hi = vmlal_s32(c1_hi, vget_high_s32(f1_hi), vget_high_s32(s_hi));

      src_ptr += 8;
      dat_ptr += 8;
      flt1_ptr += 8;
      w -= 8;
    } while (w != 0);

    src += src_stride;
    dat += dat_stride;
    flt1 += flt1_stride;
  } while (--height != 0);

  H[1][1] = horizontal_add_s64x2(vaddq_s64(h11_lo, h11_hi)) / size;
  C[1] = horizontal_add_s64x2(vaddq_s64(c1_lo, c1_hi)) / size;
}

// The function calls 3 subfunctions for the following cases :
// 1) When params->r[0] > 0 and params->r[1] > 0. In this case all elements
//    of C and H need to be computed.
// 2) When only params->r[0] > 0. In this case only H[0][0] and C[0] are
//    non-zero and need to be computed.
// 3) When only params->r[1] > 0. In this case only H[1][1] and C[1] are
//    non-zero and need to be computed.
void av1_calc_proj_params_high_bd_neon(const uint8_t *src8, int width,
                                       int height, int src_stride,
                                       const uint8_t *dat8, int dat_stride,
                                       int32_t *flt0, int flt0_stride,
                                       int32_t *flt1, int flt1_stride,
                                       int64_t H[2][2], int64_t C[2],
                                       const sgr_params_type *params) {
  if ((params->r[0] > 0) && (params->r[1] > 0)) {
    highbd_calc_proj_params_r0_r1_neon(src8, width, height, src_stride, dat8,
                                       dat_stride, flt0, flt0_stride, flt1,
                                       flt1_stride, H, C);
  } else if (params->r[0] > 0) {
    highbd_calc_proj_params_r0_neon(src8, width, height, src_stride, dat8,
                                    dat_stride, flt0, flt0_stride, H, C);
  } else if (params->r[1] > 0) {
    highbd_calc_proj_params_r1_neon(src8, width, height, src_stride, dat8,
                                    dat_stride, flt1, flt1_stride, H, C);
  }
}

static int16_t highbd_find_average_neon(const int16_t *src, int src_stride,
                                        int width, int height) {
  assert(width > 0);
  assert(height > 0);

  int64x2_t sum_s64 = vdupq_n_s64(0);
  int64_t sum = 0;

  int h = height;
  do {
    int32x4_t sum_s32[2] = { vdupq_n_s32(0), vdupq_n_s32(0) };

    int w = width;
    const int16_t *row = src;
    while (w >= 32) {
      int16x8_t s0 = vld1q_s16(row + 0);
      int16x8_t s1 = vld1q_s16(row + 8);
      int16x8_t s2 = vld1q_s16(row + 16);
      int16x8_t s3 = vld1q_s16(row + 24);

      s0 = vaddq_s16(s0, s1);
      s2 = vaddq_s16(s2, s3);
      sum_s32[0] = vpadalq_s16(sum_s32[0], s0);
      sum_s32[1] = vpadalq_s16(sum_s32[1], s2);

      row += 32;
      w -= 32;
    }

    if (w >= 16) {
      int16x8_t s0 = vld1q_s16(row + 0);
      int16x8_t s1 = vld1q_s16(row + 8);

      s0 = vaddq_s16(s0, s1);
      sum_s32[0] = vpadalq_s16(sum_s32[0], s0);

      row += 16;
      w -= 16;
    }

    if (w >= 8) {
      int16x8_t s0 = vld1q_s16(row);
      sum_s32[1] = vpadalq_s16(sum_s32[1], s0);

      row += 8;
      w -= 8;
    }

    if (w >= 4) {
      int16x8_t s0 = vcombine_s16(vld1_s16(row), vdup_n_s16(0));
      sum_s32[0] = vpadalq_s16(sum_s32[0], s0);

      row += 4;
      w -= 4;
    }

    while (w-- > 0) {
      sum += *row++;
    }

    sum_s64 = vpadalq_s32(sum_s64, vaddq_s32(sum_s32[0], sum_s32[1]));

    src += src_stride;
  } while (--h != 0);
  return (int16_t)((horizontal_add_s64x2(sum_s64) + sum) / (height * width));
}

static INLINE void compute_H_one_col(int16x8_t *dgd, int col, int64_t *H,
                                     const int wiener_win,
                                     const int wiener_win2) {
  for (int row0 = 0; row0 < wiener_win; row0++) {
    for (int row1 = row0; row1 < wiener_win; row1++) {
      int auto_cov_idx =
          (col * wiener_win + row0) * wiener_win2 + (col * wiener_win) + row1;

      int32x4_t auto_cov =
          vmull_s16(vget_low_s16(dgd[row0]), vget_low_s16(dgd[row1]));
      auto_cov = vmlal_s16(auto_cov, vget_high_s16(dgd[row0]),
                           vget_high_s16(dgd[row1]));

      H[auto_cov_idx] += horizontal_long_add_s32x4(auto_cov);
    }
  }
}

// This function computes two matrices: the cross-correlation between the src
// buffer and dgd buffer (M), and the auto-covariance of the dgd buffer (H).
//
// M is of size 7 * 7. It needs to be filled such that multiplying one element
// from src with each element of a row of the wiener window will fill one
// column of M. However this is not very convenient in terms of memory
// accesses, as it means we do contiguous loads of dgd but strided stores to M.
// As a result, we use an intermediate matrix M_trn which is instead filled
// such that one row of the wiener window gives one row of M_trn. Once fully
// computed, M_trn is then transposed to return M.
//
// H is of size 49 * 49. It is filled by multiplying every pair of elements of
// the wiener window together. Since it is a symmetric matrix, we only compute
// the upper triangle, and then copy it down to the lower one. Here we fill it
// by taking each different pair of columns, and multiplying all the elements of
// the first one with all the elements of the second one, with a special case
// when multiplying a column by itself.
static INLINE void highbd_compute_stats_win7_neon(
    const int16_t *dgd, int dgd_stride, const int16_t *src, int src_stride,
    int width, int height, int64_t *M, int64_t *H, int16_t avg, int bit_depth) {
  const int wiener_win = 7;
  const int wiener_win2 = wiener_win * wiener_win;
  const int16x8_t mask = vld1q_s16(&av1_neon_mask_16bit[8] - (width % 8));

  // We use an intermediate matrix that will be transposed to get M.
  int64_t M_trn[49];
  memset(M_trn, 0, sizeof(M_trn));

  int16x8_t vavg = vdupq_n_s16(avg);
  do {
    // Cross-correlation (M).
    for (int row = 0; row < wiener_win; row++) {
      int16x8_t dgd0 = vsubq_s16(vld1q_s16(dgd + row * dgd_stride), vavg);
      int j = 0;
      while (j <= width - 8) {
        int16x8_t dgd1 =
            vsubq_s16(vld1q_s16(dgd + row * dgd_stride + j + 8), vavg);
        int16x8_t s = vsubq_s16(vld1q_s16(src + j), vavg);

        // Compute all the elements of one row of M.
        compute_M_one_row_win7(s, dgd0, dgd1, M_trn, wiener_win, row);

        dgd0 = dgd1;
        j += 8;
      }
      // Process remaining elements without Neon.
      while (j < width) {
        int16_t s = src[j] - avg;
        int16_t d0 = dgd[row * dgd_stride + 0 + j] - avg;
        int16_t d1 = dgd[row * dgd_stride + 1 + j] - avg;
        int16_t d2 = dgd[row * dgd_stride + 2 + j] - avg;
        int16_t d3 = dgd[row * dgd_stride + 3 + j] - avg;
        int16_t d4 = dgd[row * dgd_stride + 4 + j] - avg;
        int16_t d5 = dgd[row * dgd_stride + 5 + j] - avg;
        int16_t d6 = dgd[row * dgd_stride + 6 + j] - avg;

        M_trn[row * wiener_win + 0] += d0 * s;
        M_trn[row * wiener_win + 1] += d1 * s;
        M_trn[row * wiener_win + 2] += d2 * s;
        M_trn[row * wiener_win + 3] += d3 * s;
        M_trn[row * wiener_win + 4] += d4 * s;
        M_trn[row * wiener_win + 5] += d5 * s;
        M_trn[row * wiener_win + 6] += d6 * s;

        j++;
      }
    }

    // Auto-covariance (H).
    int j = 0;
    while (j <= width - 8) {
      for (int col0 = 0; col0 < wiener_win; col0++) {
        int16x8_t dgd0[7];
        dgd0[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col0), vavg);
        dgd0[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col0), vavg);
        dgd0[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col0), vavg);
        dgd0[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col0), vavg);
        dgd0[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col0), vavg);
        dgd0[5] = vsubq_s16(vld1q_s16(dgd + 5 * dgd_stride + j + col0), vavg);
        dgd0[6] = vsubq_s16(vld1q_s16(dgd + 6 * dgd_stride + j + col0), vavg);

        // Perform computation of the first column with itself (28 elements).
        // For the first column this will fill the upper triangle of the 7x7
        // matrix at the top left of the H matrix. For the next columns this
        // will fill the upper triangle of the other 7x7 matrices around H's
        // diagonal.
        compute_H_one_col(dgd0, col0, H, wiener_win, wiener_win2);

        // All computation next to the matrix diagonal has already been done.
        for (int col1 = col0 + 1; col1 < wiener_win; col1++) {
          // Load second column.
          int16x8_t dgd1[7];
          dgd1[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col1), vavg);
          dgd1[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col1), vavg);
          dgd1[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col1), vavg);
          dgd1[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col1), vavg);
          dgd1[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col1), vavg);
          dgd1[5] = vsubq_s16(vld1q_s16(dgd + 5 * dgd_stride + j + col1), vavg);
          dgd1[6] = vsubq_s16(vld1q_s16(dgd + 6 * dgd_stride + j + col1), vavg);

          // Compute all elements from the combination of both columns (49
          // elements).
          compute_H_two_cols(dgd0, dgd1, col0, col1, H, wiener_win,
                             wiener_win2);
        }
      }
      j += 8;
    }

    if (j < width) {
      // Process remaining columns using a mask to discard excess elements.
      for (int col0 = 0; col0 < wiener_win; col0++) {
        // Load first column.
        int16x8_t dgd0[7];
        dgd0[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col0), vavg);
        dgd0[0] = vandq_s16(dgd0[0], mask);
        dgd0[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col0), vavg);
        dgd0[1] = vandq_s16(dgd0[1], mask);
        dgd0[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col0), vavg);
        dgd0[2] = vandq_s16(dgd0[2], mask);
        dgd0[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col0), vavg);
        dgd0[3] = vandq_s16(dgd0[3], mask);
        dgd0[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col0), vavg);
        dgd0[4] = vandq_s16(dgd0[4], mask);
        dgd0[5] = vsubq_s16(vld1q_s16(dgd + 5 * dgd_stride + j + col0), vavg);
        dgd0[5] = vandq_s16(dgd0[5], mask);
        dgd0[6] = vsubq_s16(vld1q_s16(dgd + 6 * dgd_stride + j + col0), vavg);
        dgd0[6] = vandq_s16(dgd0[6], mask);

        // Perform computation of the first column with itself (28 elements).
        // For the first column this will fill the upper triangle of the 7x7
        // matrix at the top left of the H matrix. For the next columns this
        // will fill the upper triangle of the other 7x7 matrices around H's
        // diagonal.
        compute_H_one_col(dgd0, col0, H, wiener_win, wiener_win2);

        // All computation next to the matrix diagonal has already been done.
        for (int col1 = col0 + 1; col1 < wiener_win; col1++) {
          // Load second column.
          int16x8_t dgd1[7];
          dgd1[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col1), vavg);
          dgd1[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col1), vavg);
          dgd1[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col1), vavg);
          dgd1[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col1), vavg);
          dgd1[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col1), vavg);
          dgd1[5] = vsubq_s16(vld1q_s16(dgd + 5 * dgd_stride + j + col1), vavg);
          dgd1[6] = vsubq_s16(vld1q_s16(dgd + 6 * dgd_stride + j + col1), vavg);

          // Compute all elements from the combination of both columns (49
          // elements).
          compute_H_two_cols(dgd0, dgd1, col0, col1, H, wiener_win,
                             wiener_win2);
        }
      }
    }
    dgd += dgd_stride;
    src += src_stride;
  } while (--height != 0);

  // Transpose M_trn.
  transpose_M_win7(M, M_trn, 7);

  // Copy upper triangle of H in the lower one.
  copy_upper_triangle(H, wiener_win2);

  // Scaling the results.
  uint8_t bit_depth_divider = 1;
  if (bit_depth == AOM_BITS_12) {
    bit_depth_divider = 16;
  } else if (bit_depth == AOM_BITS_10) {
    bit_depth_divider = 4;
  }

  for (int i = 0; i < wiener_win2; ++i) {
    M[i] /= bit_depth_divider;
    for (int j = 0; j < wiener_win2; ++j) {
      H[i * wiener_win2 + j] /= bit_depth_divider;
    }
  }
}

// This function computes two matrices: the cross-correlation between the src
// buffer and dgd buffer (M), and the auto-covariance of the dgd buffer (H).
//
// M is of size 5 * 5. It needs to be filled such that multiplying one element
// from src with each element of a row of the wiener window will fill one
// column of M. However this is not very convenient in terms of memory
// accesses, as it means we do contiguous loads of dgd but strided stores to M.
// As a result, we use an intermediate matrix M_trn which is instead filled
// such that one row of the wiener window gives one row of M_trn. Once fully
// computed, M_trn is then transposed to return M.
//
// H is of size 25 * 25. It is filled by multiplying every pair of elements of
// the wiener window together. Since it is a symmetric matrix, we only compute
// the upper triangle, and then copy it down to the lower one. Here we fill it
// by taking each different pair of columns, and multiplying all the elements of
// the first one with all the elements of the second one, with a special case
// when multiplying a column by itself.
static INLINE void highbd_compute_stats_win5_neon(
    const int16_t *dgd, int dgd_stride, const int16_t *src, int src_stride,
    int width, int height, int64_t *M, int64_t *H, int16_t avg, int bit_depth) {
  const int wiener_win = 5;
  const int wiener_win2 = wiener_win * wiener_win;
  const int16x8_t mask = vld1q_s16(&av1_neon_mask_16bit[8] - (width % 8));

  // We use an intermediate matrix that will be transposed to get M.
  int64_t M_trn[25];
  memset(M_trn, 0, sizeof(M_trn));

  int16x8_t vavg = vdupq_n_s16(avg);
  do {
    // Cross-correlation (M).
    for (int row = 0; row < wiener_win; row++) {
      int16x8_t dgd0 = vsubq_s16(vld1q_s16(dgd + row * dgd_stride), vavg);
      int j = 0;
      while (j <= width - 8) {
        int16x8_t dgd1 =
            vsubq_s16(vld1q_s16(dgd + row * dgd_stride + j + 8), vavg);
        int16x8_t s = vsubq_s16(vld1q_s16(src + j), vavg);

        // Compute all the elements of one row of M.
        compute_M_one_row_win5(s, dgd0, dgd1, M_trn, wiener_win, row);

        dgd0 = dgd1;
        j += 8;
      }
      // Process remaining elements without Neon.
      while (j < width) {
        int16_t s = src[j] - avg;
        int16_t d0 = dgd[row * dgd_stride + 0 + j] - avg;
        int16_t d1 = dgd[row * dgd_stride + 1 + j] - avg;
        int16_t d2 = dgd[row * dgd_stride + 2 + j] - avg;
        int16_t d3 = dgd[row * dgd_stride + 3 + j] - avg;
        int16_t d4 = dgd[row * dgd_stride + 4 + j] - avg;

        M_trn[row * wiener_win + 0] += d0 * s;
        M_trn[row * wiener_win + 1] += d1 * s;
        M_trn[row * wiener_win + 2] += d2 * s;
        M_trn[row * wiener_win + 3] += d3 * s;
        M_trn[row * wiener_win + 4] += d4 * s;

        j++;
      }
    }

    // Auto-covariance (H).
    int j = 0;
    while (j <= width - 8) {
      for (int col0 = 0; col0 < wiener_win; col0++) {
        // Load first column.
        int16x8_t dgd0[5];
        dgd0[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col0), vavg);
        dgd0[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col0), vavg);
        dgd0[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col0), vavg);
        dgd0[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col0), vavg);
        dgd0[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col0), vavg);

        // Perform computation of the first column with itself (15 elements).
        // For the first column this will fill the upper triangle of the 5x5
        // matrix at the top left of the H matrix. For the next columns this
        // will fill the upper triangle of the other 5x5 matrices around H's
        // diagonal.
        compute_H_one_col(dgd0, col0, H, wiener_win, wiener_win2);

        // All computation next to the matrix diagonal has already been done.
        for (int col1 = col0 + 1; col1 < wiener_win; col1++) {
          // Load second column.
          int16x8_t dgd1[5];
          dgd1[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col1), vavg);
          dgd1[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col1), vavg);
          dgd1[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col1), vavg);
          dgd1[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col1), vavg);
          dgd1[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col1), vavg);

          // Compute all elements from the combination of both columns (25
          // elements).
          compute_H_two_cols(dgd0, dgd1, col0, col1, H, wiener_win,
                             wiener_win2);
        }
      }
      j += 8;
    }

    if (j < width) {
      // Process remaining columns using a mask to discard excess elements.
      for (int col0 = 0; col0 < wiener_win; col0++) {
        // Load first column.
        int16x8_t dgd0[5];
        dgd0[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col0), vavg);
        dgd0[0] = vandq_s16(dgd0[0], mask);
        dgd0[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col0), vavg);
        dgd0[1] = vandq_s16(dgd0[1], mask);
        dgd0[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col0), vavg);
        dgd0[2] = vandq_s16(dgd0[2], mask);
        dgd0[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col0), vavg);
        dgd0[3] = vandq_s16(dgd0[3], mask);
        dgd0[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col0), vavg);
        dgd0[4] = vandq_s16(dgd0[4], mask);

        // Perform computation of the first column with itself (15 elements).
        // For the first column this will fill the upper triangle of the 5x5
        // matrix at the top left of the H matrix. For the next columns this
        // will fill the upper triangle of the other 5x5 matrices around H's
        // diagonal.
        compute_H_one_col(dgd0, col0, H, wiener_win, wiener_win2);

        // All computation next to the matrix diagonal has already been done.
        for (int col1 = col0 + 1; col1 < wiener_win; col1++) {
          // Load second column.
          int16x8_t dgd1[5];
          dgd1[0] = vsubq_s16(vld1q_s16(dgd + 0 * dgd_stride + j + col1), vavg);
          dgd1[1] = vsubq_s16(vld1q_s16(dgd + 1 * dgd_stride + j + col1), vavg);
          dgd1[2] = vsubq_s16(vld1q_s16(dgd + 2 * dgd_stride + j + col1), vavg);
          dgd1[3] = vsubq_s16(vld1q_s16(dgd + 3 * dgd_stride + j + col1), vavg);
          dgd1[4] = vsubq_s16(vld1q_s16(dgd + 4 * dgd_stride + j + col1), vavg);

          // Compute all elements from the combination of both columns (25
          // elements).
          compute_H_two_cols(dgd0, dgd1, col0, col1, H, wiener_win,
                             wiener_win2);
        }
      }
    }
    dgd += dgd_stride;
    src += src_stride;
  } while (--height != 0);

  // Transpose M_trn.
  transpose_M_win5(M, M_trn, 5);

  // Copy upper triangle of H in the lower one.
  copy_upper_triangle(H, wiener_win2);

  // Scaling the results.
  uint8_t bit_depth_divider = 1;
  if (bit_depth == AOM_BITS_12) {
    bit_depth_divider = 16;
  } else if (bit_depth == AOM_BITS_10) {
    bit_depth_divider = 4;
  }

  for (int i = 0; i < wiener_win2; ++i) {
    M[i] /= bit_depth_divider;
    for (int j = 0; j < wiener_win2; ++j) {
      H[i * wiener_win2 + j] /= bit_depth_divider;
    }
  }
}

void av1_compute_stats_highbd_neon(int wiener_win, const uint8_t *dgd8,
                                   const uint8_t *src8, int h_start, int h_end,
                                   int v_start, int v_end, int dgd_stride,
                                   int src_stride, int64_t *M, int64_t *H,
                                   aom_bit_depth_t bit_depth) {
  assert(wiener_win == WIENER_WIN || wiener_win == WIENER_WIN_REDUCED);

  const int wiener_halfwin = wiener_win >> 1;
  const int wiener_win2 = wiener_win * wiener_win;
  memset(H, 0, sizeof(*H) * wiener_win2 * wiener_win2);

  const int16_t *src = (const int16_t *)CONVERT_TO_SHORTPTR(src8);
  const int16_t *dgd = (const int16_t *)CONVERT_TO_SHORTPTR(dgd8);
  const int height = v_end - v_start;
  const int width = h_end - h_start;
  const int vert_offset = v_start - wiener_halfwin;
  const int horiz_offset = h_start - wiener_halfwin;

  int16_t avg = highbd_find_average_neon(dgd + v_start * dgd_stride + h_start,
                                         dgd_stride, width, height);

  src += v_start * src_stride + h_start;
  dgd += vert_offset * dgd_stride + horiz_offset;

  if (wiener_win == WIENER_WIN) {
    highbd_compute_stats_win7_neon(dgd, dgd_stride, src, src_stride, width,
                                   height, M, H, avg, bit_depth);
  } else {
    highbd_compute_stats_win5_neon(dgd, dgd_stride, src, src_stride, width,
                                   height, M, H, avg, bit_depth);
  }
}

int64_t av1_highbd_pixel_proj_error_neon(
    const uint8_t *src8, int width, int height, int src_stride,
    const uint8_t *dat8, int dat_stride, int32_t *flt0, int flt0_stride,
    int32_t *flt1, int flt1_stride, int xq[2], const sgr_params_type *params) {
  const uint16_t *src = CONVERT_TO_SHORTPTR(src8);
  const uint16_t *dat = CONVERT_TO_SHORTPTR(dat8);
  int64_t sse = 0;
  int64x2_t sse_s64 = vdupq_n_s64(0);

  if (params->r[0] > 0 && params->r[1] > 0) {
    int32x2_t xq_v = vld1_s32(xq);
    int32x2_t xq_sum_v = vshl_n_s32(vpadd_s32(xq_v, xq_v), 4);

    do {
      int j = 0;
      int32x4_t sse_s32 = vdupq_n_s32(0);

      do {
        const uint16x8_t d = vld1q_u16(&dat[j]);
        const uint16x8_t s = vld1q_u16(&src[j]);
        int32x4_t flt0_0 = vld1q_s32(&flt0[j]);
        int32x4_t flt0_1 = vld1q_s32(&flt0[j + 4]);
        int32x4_t flt1_0 = vld1q_s32(&flt1[j]);
        int32x4_t flt1_1 = vld1q_s32(&flt1[j + 4]);

        int32x4_t d_s32_lo = vreinterpretq_s32_u32(
            vmull_lane_u16(vget_low_u16(d), vreinterpret_u16_s32(xq_sum_v), 0));
        int32x4_t d_s32_hi = vreinterpretq_s32_u32(vmull_lane_u16(
            vget_high_u16(d), vreinterpret_u16_s32(xq_sum_v), 0));

        int32x4_t v0 = vsubq_s32(
            vdupq_n_s32(1 << (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS - 1)),
            d_s32_lo);
        int32x4_t v1 = vsubq_s32(
            vdupq_n_s32(1 << (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS - 1)),
            d_s32_hi);

        v0 = vmlaq_lane_s32(v0, flt0_0, xq_v, 0);
        v1 = vmlaq_lane_s32(v1, flt0_1, xq_v, 0);
        v0 = vmlaq_lane_s32(v0, flt1_0, xq_v, 1);
        v1 = vmlaq_lane_s32(v1, flt1_1, xq_v, 1);

        int16x4_t vr0 = vshrn_n_s32(v0, SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS);
        int16x4_t vr1 = vshrn_n_s32(v1, SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS);

        int16x8_t e = vaddq_s16(vcombine_s16(vr0, vr1),
                                vreinterpretq_s16_u16(vsubq_u16(d, s)));
        int16x4_t e_lo = vget_low_s16(e);
        int16x4_t e_hi = vget_high_s16(e);

        sse_s32 = vmlal_s16(sse_s32, e_lo, e_lo);
        sse_s32 = vmlal_s16(sse_s32, e_hi, e_hi);

        j += 8;
      } while (j <= width - 8);

      for (int k = j; k < width; ++k) {
        int32_t v = 1 << (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS - 1);
        v += xq[0] * (flt0[k]) + xq[1] * (flt1[k]);
        v -= (xq[1] + xq[0]) * (int32_t)(dat[k] << 4);
        int32_t e =
            (v >> (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS)) + dat[k] - src[k];
        sse += ((int64_t)e * e);
      }

      sse_s64 = vpadalq_s32(sse_s64, sse_s32);

      dat += dat_stride;
      src += src_stride;
      flt0 += flt0_stride;
      flt1 += flt1_stride;
    } while (--height != 0);
  } else if (params->r[0] > 0 || params->r[1] > 0) {
    int xq_active = (params->r[0] > 0) ? xq[0] : xq[1];
    int32_t *flt = (params->r[0] > 0) ? flt0 : flt1;
    int flt_stride = (params->r[0] > 0) ? flt0_stride : flt1_stride;
    int32x4_t xq_v = vdupq_n_s32(xq_active);

    do {
      int j = 0;
      int32x4_t sse_s32 = vdupq_n_s32(0);
      do {
        const uint16x8_t d0 = vld1q_u16(&dat[j]);
        const uint16x8_t s0 = vld1q_u16(&src[j]);
        int32x4_t flt0_0 = vld1q_s32(&flt[j]);
        int32x4_t flt0_1 = vld1q_s32(&flt[j + 4]);

        uint16x8_t d_u16 = vshlq_n_u16(d0, 4);
        int32x4_t sub0 = vreinterpretq_s32_u32(
            vsubw_u16(vreinterpretq_u32_s32(flt0_0), vget_low_u16(d_u16)));
        int32x4_t sub1 = vreinterpretq_s32_u32(
            vsubw_u16(vreinterpretq_u32_s32(flt0_1), vget_high_u16(d_u16)));

        int32x4_t v0 = vmlaq_s32(
            vdupq_n_s32(1 << (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS - 1)), sub0,
            xq_v);
        int32x4_t v1 = vmlaq_s32(
            vdupq_n_s32(1 << (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS - 1)), sub1,
            xq_v);

        int16x4_t vr0 = vshrn_n_s32(v0, SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS);
        int16x4_t vr1 = vshrn_n_s32(v1, SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS);

        int16x8_t e = vaddq_s16(vcombine_s16(vr0, vr1),
                                vreinterpretq_s16_u16(vsubq_u16(d0, s0)));
        int16x4_t e_lo = vget_low_s16(e);
        int16x4_t e_hi = vget_high_s16(e);

        sse_s32 = vmlal_s16(sse_s32, e_lo, e_lo);
        sse_s32 = vmlal_s16(sse_s32, e_hi, e_hi);

        j += 8;
      } while (j <= width - 8);

      for (int k = j; k < width; ++k) {
        int32_t v = 1 << (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS - 1);
        v += xq_active * (int32_t)((uint32_t)flt[j] - (uint16_t)(dat[k] << 4));
        const int32_t e =
            (v >> (SGRPROJ_RST_BITS + SGRPROJ_PRJ_BITS)) + dat[k] - src[k];
        sse += ((int64_t)e * e);
      }

      sse_s64 = vpadalq_s32(sse_s64, sse_s32);

      dat += dat_stride;
      flt += flt_stride;
      src += src_stride;
    } while (--height != 0);
  } else {
    do {
      int j = 0;

      do {
        const uint16x8_t d = vld1q_u16(&dat[j]);
        const uint16x8_t s = vld1q_u16(&src[j]);

        uint16x8_t diff = vabdq_u16(d, s);
        uint16x4_t diff_lo = vget_low_u16(diff);
        uint16x4_t diff_hi = vget_high_u16(diff);

        uint32x4_t sqr_lo = vmull_u16(diff_lo, diff_lo);
        uint32x4_t sqr_hi = vmull_u16(diff_hi, diff_hi);

        sse_s64 = vpadalq_s32(sse_s64, vreinterpretq_s32_u32(sqr_lo));
        sse_s64 = vpadalq_s32(sse_s64, vreinterpretq_s32_u32(sqr_hi));

        j += 8;
      } while (j <= width - 8);

      for (int k = j; k < width; ++k) {
        int32_t e = dat[k] - src[k];
        sse += e * e;
      }

      dat += dat_stride;
      src += src_stride;
    } while (--height != 0);
  }

  sse += horizontal_add_s64x2(sse_s64);
  return sse;
}
