/*****************************************************************************
 * Copyright (C) 2022-2023 MulticoreWare, Inc
 *
 * Authors: David Chen <david.chen@myais.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

// Functions in this file:
// ***** luma_vpp *****
// ***** luma_vps *****
// ***** luma_vsp *****
// ***** luma_vss *****
// ***** luma_hpp *****
// ***** luma_hps *****
// ***** chroma_vpp *****
// ***** chroma_vps *****
// ***** chroma_vsp *****
// ***** chroma_vss *****
// ***** chroma_hpp *****
// ***** chroma_hps *****

#include "asm-sve.S"
#include "ipfilter-common.S"

.arch armv8-a+sve2

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

.macro qpel_load_32b_sve2 v
.if \v == 0
    add             x6, x6, x11       // do not load 3 values that are not used in qpel_filter_0
    ld1b            {z3.h}, p0/z, [x6]
    add             x6, x6, x1
.elseif \v == 1 || \v == 2 || \v == 3
.if \v != 3                           // not used in qpel_filter_3
    ld1b            {z0.h}, p0/z, [x6]
    add             x6, x6, x1
.else
    add             x6, x6, x1
.endif
    ld1b            {z1.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z2.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z3.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z4.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z5.h}, p0/z, [x6]
    add             x6, x6, x1
.if \v != 1                           // not used in qpel_filter_1
    ld1b            {z6.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z7.h}, p0/z, [x6]
.else
    ld1b            {z6.h}, p0/z, [x6]
.endif
.endif
.endm

.macro qpel_load_64b_sve2_gt_16 v
.if \v == 0
    add             x6, x6, x11       // do not load 3 values that are not used in qpel_filter_0
    ld1b            {z3.h}, p2/z, [x6]
    add             x6, x6, x1
.elseif \v == 1 || \v == 2 || \v == 3
.if \v != 3                           // not used in qpel_filter_3
    ld1b            {z0.h}, p2/z, [x6]
    add             x6, x6, x1
.else
    add             x6, x6, x1
.endif
    ld1b            {z1.h}, p2/z, [x6]
    add             x6, x6, x1
    ld1b            {z2.h}, p2/z, [x6]
    add             x6, x6, x1
    ld1b            {z3.h}, p2/z, [x6]
    add             x6, x6, x1
    ld1b            {z4.h}, p2/z, [x6]
    add             x6, x6, x1
    ld1b            {z5.h}, p2/z, [x6]
    add             x6, x6, x1
.if \v != 1                           // not used in qpel_filter_1
    ld1b            {z6.h}, p2/z, [x6]
    add             x6, x6, x1
    ld1b            {z7.h}, p2/z, [x6]
.else
    ld1b            {z6.h}, p2/z, [x6]
.endif
.endif
.endm

.macro qpel_chroma_load_32b_sve2 v
.if \v == 0
    // qpel_filter_chroma_0 only uses values in v1
    add             x6, x6, x1
    ld1b            {z1.h}, p0/z, [x6]
.else
    ld1b            {z0.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z1.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z2.h}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z3.h}, p0/z, [x6]
.endif
.endm

.macro qpel_start_sve2_0
    mov             z24.h, #64
.endm

.macro qpel_filter_sve2_0_32b
    mul             z17.h, z3.h, z24.h    // 64*d
.endm

.macro qpel_filter_sve2_0_64b
    qpel_filter_sve2_0_32b
    mul             z18.h, z11.h, z24.h
.endm

.macro qpel_start_sve2_1
    mov             z24.h, #58
    mov             z25.h, #10
    mov             z26.h, #17
    mov             z27.h, #5
.endm

.macro qpel_filter_sve2_1_32b
    mul             z19.h, z2.h, z25.h  // c*10
    mul             z17.h, z3.h, z24.h  // d*58
    mul             z21.h, z4.h, z26.h  // e*17
    mul             z23.h, z5.h, z27.h  // f*5
    sub             z17.h, z17.h, z19.h // d*58 - c*10
    lsl             z18.h, z1.h, #2      // b*4
    add             z17.h, z17.h, z21.h // d*58 - c*10 + e*17
    sub             z21.h, z6.h, z0.h   // g - a
    add             z17.h, z17.h, z18.h // d*58 - c*10 + e*17 + b*4
    sub             z21.h, z21.h, z23.h // g - a - f*5
    add             z17.h, z17.h, z21.h // d*58 - c*10 + e*17 + b*4 + g - a - f*5
.endm

.macro qpel_filter_sve2_1_64b
    qpel_filter_sve2_1_32b
    mul             z20.h, z10.h, z25.h  // c*10
    mul             z18.h, z11.h, z24.h  // d*58
    mul             z21.h, z12.h, z26.h  // e*17
    mul             z23.h, z13.h, z27.h  // f*5
    sub             z18.h, z18.h, z20.h   // d*58 - c*10
    lsl             z28.h, z30.h, #2       // b*4
    add             z18.h, z18.h, z21.h   // d*58 - c*10 + e*17
    sub             z21.h, z14.h, z29.h   // g - a
    add             z18.h, z18.h, z28.h   // d*58 - c*10 + e*17 + b*4
    sub             z21.h, z21.h, z23.h   // g - a - f*5
    add             z18.h, z18.h, z21.h   // d*58 - c*10 + e*17 + b*4 + g - a - f*5
.endm

.macro qpel_start_sve2_2
    mov             z24.h, #11
    mov             z25.h, #40
.endm

.macro qpel_filter_sve2_2_32b
    add             z17.h, z3.h, z4.h     // d + e
    add             z19.h, z2.h, z5.h     // c + f
    add             z23.h, z1.h, z6.h     // b + g
    add             z21.h, z0.h, z7.h     // a + h
    mul             z17.h, z17.h, z25.h   // 40 * (d + e)
    mul             z19.h, z19.h, z24.h   // 11 * (c + f)
    lsl             z23.h, z23.h, #2       // (b + g) * 4
    add             z19.h, z19.h, z21.h   // 11 * (c + f) + a + h
    add             z17.h, z17.h, z23.h   // 40 * (d + e) + (b + g) * 4
    sub             z17.h, z17.h, z19.h   // 40 * (d + e) + (b + g) * 4 - 11 * (c + f) - a - h
.endm

.macro qpel_filter_sve2_2_64b
    qpel_filter_sve2_2_32b
    add             z27.h, z11.h, z12.h   // d + e
    add             z16.h, z10.h, z13.h   // c + f
    add             z23.h, z30.h, z14.h   // b + g
    add             z21.h, z29.h, z15.h   // a + h
    mul             z27.h, z27.h, z25.h   // 40 * (d + e)
    mul             z16.h, z16.h, z24.h   // 11 * (c + f)
    lsl             z23.h, z23.h, #2       // (b + g) * 4
    add             z16.h, z16.h, z21.h   // 11 * (c + f) + a + h
    add             z27.h, z27.h, z23.h   // 40 * (d + e) + (b + g) * 4
    sub             z18.h, z27.h, z16.h   // 40 * (d + e) + (b + g) * 4 - 11 * (c + f) - a - h
.endm

.macro qpel_start_sve2_3
    mov             z24.h, #17
    mov             z25.h, #5
    mov             z26.h, #58
    mov             z27.h, #10
.endm

.macro qpel_filter_sve2_3_32b
    mul             z19.h, z2.h, z25.h    // c * 5
    mul             z17.h, z3.h, z24.h    // d * 17
    mul             z21.h, z4.h, z26.h    // e * 58
    mul             z23.h, z5.h, z27.h    // f * 10
    sub             z17.h, z17.h, z19.h   // d * 17 - c * 5
    lsl             z19.h, z6.h, #2        // g * 4
    add             z17.h, z17.h, z21.h   // d * 17 - c * 5 + e * 58
    sub             z21.h, z1.h, z7.h     // b - h
    add             z17.h, z17.h, z19.h   // d * 17 - c * 5 + e * 58 + g * 4
    sub             z21.h, z21.h, z23.h   // b - h - f * 10
    add             z17.h, z17.h, z21.h   // d * 17 - c * 5 + e * 58 + g * 4 + b - h - f * 10
.endm

.macro qpel_filter_sve2_3_64b
    qpel_filter_sve2_3_32b
    mul             z16.h, z10.h, z25.h  // c * 5
    mul             z18.h, z11.h, z24.h  // d * 17
    mul             z21.h, z12.h, z26.h  // e * 58
    mul             z23.h, z13.h, z27.h  // f * 10
    sub             z18.h, z18.h, z16.h   // d * 17 - c * 5
    lsl             z16.h, z14.h, #2       // g * 4
    add             z18.h, z18.h, z21.h   // d * 17 - c * 5 + e * 58
    sub             z21.h, z30.h, z15.h   // b - h
    add             z18.h, z18.h, z16.h   // d * 17 - c * 5 + e * 58 + g * 4
    sub             z21.h, z21.h, z23.h   // b - h - f * 10
    add             z18.h, z18.h, z21.h   // d * 17 - c * 5 + e * 58 + g * 4 + b - h - f * 10
.endm

.macro qpel_start_chroma_sve2_0
    mov             z29.h, #64
.endm

.macro qpel_filter_chroma_sve2_0_32b
    mul             z17.h, z1.h, z29.h    // 64*b
.endm

.macro qpel_start_chroma_sve2_1
    mov             z29.h, #58
    mov             z30.h, #10
.endm

.macro qpel_filter_chroma_sve2_1_32b
    mul             z17.h, z1.h, z29.h    // 58 * b
    mul             z19.h, z2.h, z30.h    // 10 * c
    add             z22.h, z0.h, z3.h     // a + d
    lsl             z22.h, z22.h, #1       // 2 * (a+d)
    sub             z17.h, z17.h, z22.h   // 58*b - 2*(a+d)
    add             z17.h, z17.h, z19.h   // 58*b-2*(a+d) + 10*c
.endm

.macro qpel_start_chroma_sve2_2
    mov             z30.h, #54
.endm

.macro qpel_filter_chroma_sve2_2_32b
    mul             z17.h, z1.h, z30.h    // 54 * b
    lsl             z19.h, z0.h, #2        // 4 * a
    lsl             z21.h, z2.h, #4        // 16 * c
    lsl             z23.h, z3.h, #1        // 2 * d
    add             z17.h, z17.h, z21.h   // 54*b + 16*c
    add             z19.h, z19.h, z23.h   // 4*a + 2*d
    sub             z17.h, z17.h, z19.h   // 54*b+16*c - (4*a+2*d)
.endm

.macro qpel_start_chroma_sve2_3
    mov             z28.h, #46
    mov             z29.h, #28
    mov             z30.h, #6
.endm

.macro qpel_filter_chroma_sve2_3_32b
    mul             z17.h, z1.h, z28.h    // 46 * b
    mul             z19.h, z2.h, z29.h    // 28 * c
    lsl             z21.h, z3.h, #2        // 4 * d
    mul             z23.h, z0.h, z30.h    // 6 * a
    add             z17.h, z17.h, z19.h   // 46*b + 28*c
    add             z21.h, z21.h, z23.h   // 4*d + 6*a
    sub             z17.h, z17.h, z21.h   // 46*b+28*c - (4*d+6*a)
.endm

.macro qpel_start_chroma_sve2_4
    mov             z29.h, #36
.endm

.macro qpel_filter_chroma_sve2_4_32b
    add             z20.h, z0.h, z3.h     // a + d
    add             z17.h, z1.h, z2.h     // b + c
    lsl             z20.h, z20.h, #2       // 4 * (a+d)
    mul             z17.h, z17.h, z29.h   // 36 * (b+c)
    sub             z17.h, z17.h, z20.h   // 36*(b+c) - 4*(a+d)
.endm

.macro qpel_start_chroma_sve2_5
    mov             z28.h, #28
    mov             z29.h, #46
    mov             z30.h, #6
.endm

.macro qpel_filter_chroma_sve2_5_32b
    mul             z17.h, z1.h, z28.h    // 28 * b
    mul             z19.h, z2.h, z29.h    // 46 * c
    lsl             z21.h, z0.h, #2        // 4 * a
    mul             z23.h, z3.h, z30.h    // 6 * d
    add             z17.h, z17.h, z19.h   // 28*b + 46*c
    add             z21.h, z21.h, z23.h   // 4*a + 6*d
    sub             z17.h, z17.h, z21.h   // 28*b+46*c - (4*a+6*d)
.endm

.macro qpel_start_chroma_sve2_6
    mov             z30.h, #54
.endm

.macro qpel_filter_chroma_sve2_6_32b
    mul             z17.h, z2.h, z30.h    // 54 * c
    lsl             z19.h, z0.h, #1        // 2 * a
    lsl             z21.h, z1.h, #4        // 16 * b
    lsl             z23.h, z3.h, #2        // 4 * d
    add             z17.h, z17.h, z21.h   // 54*c + 16*b
    add             z19.h, z19.h, z23.h   // 2*a + 4*d
    sub             z17.h, z17.h, z19.h   // 54*c+16*b - (2*a+4*d)
.endm

.macro qpel_start_chroma_sve2_7
    mov             z29.h, #58
    mov             z30.h, #10
.endm

.macro qpel_filter_chroma_sve2_7_32b
    add             z20.h, z0.h, z3.h     // a + d
    mul             z17.h, z2.h, z29.h    // 58 * c
    lsl             z20.h, z20.h, #1       // 2 * (a+d)
    mul             z19.h, z1.h, z30.h    // 10 * b
    sub             z17.h, z17.h, z20.h   // 58*c - 2*(a+d)
    add             z17.h, z17.h, z19.h   // 58*c-2*(a+d) + 10*b
.endm

.macro vpp_end_sve2
    add             z17.h, z17.h, z31.h
    sqshrun         v17.8b, v17.8h, #6
.endm

.macro FILTER_LUMA_VPP_SVE2 w, h, v
    lsl             x10, x1, #2      // x10 = 4 * x1
    sub             x11, x10, x1     // x11 = 3 * x1
    sub             x0, x0, x11      // src -= (8 / 2 - 1) * srcStride
    mov             x5, #\h
    mov             z31.h, #32
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_FILTER_LUMA_VPP_\v\()_\w\()x\h
    qpel_start_\v
.loop_luma_vpp_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.loop_luma_vpp_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
.if \w == 8 || \w == 24
    qpel_load_32b \v
    qpel_filter_\v\()_32b
    vpp_end
    str             d17, [x7], #8
    add             x9, x9, #8
.elseif \w == 12
    qpel_load_32b \v
    qpel_filter_\v\()_32b
    vpp_end
    str             d17, [x7], #8
    add             x6, x0, #8
    qpel_load_32b \v
    qpel_filter_\v\()_32b
    vpp_end
    fmov            w6, s17
    str             w6, [x7], #4
    add             x9, x9, #12
.else
    qpel_load_64b \v
    qpel_filter_\v\()_64b
    vpp_end
    add             v18.8h, v18.8h, v31.8h
    sqshrun2        v17.16b, v18.8h, #6
    str             q17, [x7], #16
    add             x9, x9, #16
.endif
    cmp             x9, #\w
    blt             .loop_luma_vpp_w8_sve2_\v\()_\w\()x\h
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_luma_vpp_sve2_\v\()_\w\()x\h
    ret
.vl_gt_16_FILTER_LUMA_VPP_\v\()_\w\()x\h:
    ptrue           p0.h, vl8
    ptrue           p2.h, vl16
    qpel_start_sve2_\v
.gt_16_loop_luma_vpp_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.gt_16_loop_luma_vpp_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
.if \w == 8 || \w == 24
    qpel_load_32b_sve2 \v
    qpel_filter_sve2_\v\()_32b
    vpp_end_sve2
    str             d17, [x7], #8
    add             x9, x9, #8
.elseif \w == 12
    qpel_load_32b_sve2 \v
    qpel_filter_sve2_\v\()_32b
    vpp_end_sve2
    str             d17, [x7], #8
    add             x6, x0, #8
    qpel_load_32b_sve2 \v
    qpel_filter_sve2_\v\()_32b
    vpp_end_sve2
    fmov            w6, s17
    str             w6, [x7], #4
    add             x9, x9, #12
.else
    qpel_load_64b_sve2_gt_16 \v
    qpel_filter_sve2_\v\()_32b
    vpp_end_sve2
    add             z18.h, z18.h, z31.h
    sqshrun2        v17.16b, v18.8h, #6
    str             q17, [x7], #16
    add             x9, x9, #16
.endif
    cmp             x9, #\w
    blt             .gt_16_loop_luma_vpp_w8_sve2_\v\()_\w\()x\h
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .gt_16_loop_luma_vpp_sve2_\v\()_\w\()x\h
    ret
.endm

// void interp_vert_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPP_SVE2 w, h
function x265_interp_8tap_vert_pp_\w\()x\h\()_sve2
    cmp             x4, #0
    b.eq            0f
    cmp             x4, #1
    b.eq            1f
    cmp             x4, #2
    b.eq            2f
    cmp             x4, #3
    b.eq            3f
0:
    FILTER_LUMA_VPP_SVE2 \w, \h, 0
1:
    FILTER_LUMA_VPP_SVE2 \w, \h, 1
2:
    FILTER_LUMA_VPP_SVE2 \w, \h, 2
3:
    FILTER_LUMA_VPP_SVE2 \w, \h, 3
endfunc
.endm

LUMA_VPP_SVE2 8, 4
LUMA_VPP_SVE2 8, 8
LUMA_VPP_SVE2 8, 16
LUMA_VPP_SVE2 8, 32
LUMA_VPP_SVE2 12, 16
LUMA_VPP_SVE2 16, 4
LUMA_VPP_SVE2 16, 8
LUMA_VPP_SVE2 16, 16
LUMA_VPP_SVE2 16, 32
LUMA_VPP_SVE2 16, 64
LUMA_VPP_SVE2 16, 12
LUMA_VPP_SVE2 24, 32
LUMA_VPP_SVE2 32, 8
LUMA_VPP_SVE2 32, 16
LUMA_VPP_SVE2 32, 32
LUMA_VPP_SVE2 32, 64
LUMA_VPP_SVE2 32, 24
LUMA_VPP_SVE2 48, 64
LUMA_VPP_SVE2 64, 16
LUMA_VPP_SVE2 64, 32
LUMA_VPP_SVE2 64, 64
LUMA_VPP_SVE2 64, 48

// void interp_vert_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPS_4xN_SVE2 h
function x265_interp_8tap_vert_ps_4x\h\()_sve2
    lsl             x3, x3, #1
    lsl             x5, x4, #6
    lsl             x4, x1, #2
    sub             x4, x4, x1
    sub             x0, x0, x4

    mov             z28.s, #8192
    mov             x4, #\h
    movrel          x12, g_lumaFilter
    add             x12, x12, x5
    ptrue           p0.s, vl4
    ld1rd           {z16.d}, p0/z, [x12]
    ld1rd           {z17.d}, p0/z, [x12, #8]
    ld1rd           {z18.d}, p0/z, [x12, #16]
    ld1rd           {z19.d}, p0/z, [x12, #24]
    ld1rd           {z20.d}, p0/z, [x12, #32]
    ld1rd           {z21.d}, p0/z, [x12, #40]
    ld1rd           {z22.d}, p0/z, [x12, #48]
    ld1rd           {z23.d}, p0/z, [x12, #56]

.loop_vps_sve2_4x\h:
    mov             x6, x0

    ld1b            {z0.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z1.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z2.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z3.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z4.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z5.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z6.s}, p0/z, [x6]
    add             x6, x6, x1
    ld1b            {z7.s}, p0/z, [x6]
    add             x6, x6, x1

    mul             z0.s, z0.s, z16.s
    mla             z0.s, p0/m, z1.s, z17.s
    mla             z0.s, p0/m, z2.s, z18.s
    mla             z0.s, p0/m, z3.s, z19.s
    mla             z0.s, p0/m, z4.s, z20.s
    mla             z0.s, p0/m, z5.s, z21.s
    mla             z0.s, p0/m, z6.s, z22.s
    mla             z0.s, p0/m, z7.s, z23.s

    sub             z0.s, z0.s, z28.s
    sqxtn           v0.4h, v0.4s
    st1             {v0.8b}, [x2], x3

    add             x0, x0, x1
    sub             x4, x4, #1
    cbnz            x4, .loop_vps_sve2_4x\h
    ret
endfunc
.endm

LUMA_VPS_4xN_SVE2 4
LUMA_VPS_4xN_SVE2 8
LUMA_VPS_4xN_SVE2 16

// void interp_vert_sp_c(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VSP_4xN_SVE2 h
function x265_interp_8tap_vert_sp_4x\h\()_sve2
    lsl             x5, x4, #6
    lsl             x1, x1, #1
    lsl             x4, x1, #2
    sub             x4, x4, x1
    sub             x0, x0, x4

    mov             w12, #1
    lsl             w12, w12, #19
    add             w12, w12, #2048
    dup             v24.4s, w12
    mov             x4, #\h
    movrel          x12, g_lumaFilter
    add             x12, x12, x5

    ptrue           p0.s, vl4
    ld1rd           {z16.d}, p0/z, [x12]
    ld1rd           {z17.d}, p0/z, [x12, #8]
    ld1rd           {z18.d}, p0/z, [x12, #16]
    ld1rd           {z19.d}, p0/z, [x12, #24]
    ld1rd           {z20.d}, p0/z, [x12, #32]
    ld1rd           {z21.d}, p0/z, [x12, #40]
    ld1rd           {z22.d}, p0/z, [x12, #48]
    ld1rd           {z23.d}, p0/z, [x12, #56]

.loop_vsp_sve2_4x\h:
    mov             x6, x0

    ld1             {v0.8b}, [x6], x1
    ld1             {v1.8b}, [x6], x1
    ld1             {v2.8b}, [x6], x1
    ld1             {v3.8b}, [x6], x1
    ld1             {v4.8b}, [x6], x1
    ld1             {v5.8b}, [x6], x1
    ld1             {v6.8b}, [x6], x1
    ld1             {v7.8b}, [x6], x1

    sunpklo         z0.s, z0.h
    sunpklo         z1.s, z1.h
    mul             z0.s, z0.s, z16.s
    sunpklo         z2.s, z2.h
    mla             z0.s, p0/m, z1.s, z17.s
    sunpklo         z3.s, z3.h
    mla             z0.s, p0/m, z2.s, z18.s
    sunpklo         z4.s, z4.h
    mla             z0.s, p0/m, z3.s, z19.s
    sunpklo         z5.s, z5.h
    mla             z0.s, p0/m, z4.s, z20.s
    sunpklo         z6.s, z6.h
    mla             z0.s, p0/m, z5.s, z21.s
    sunpklo         z7.s, z7.h
    mla             z0.s, p0/m, z6.s, z22.s

    mla             z0.s, p0/m, z7.s, z23.s

    add             z0.s, z0.s, z24.s
    sqshrun         v0.4h, v0.4s, #12
    sqxtun          v0.8b, v0.8h
    st1             {v0.s}[0], [x2], x3

    add             x0, x0, x1
    sub             x4, x4, #1
    cbnz            x4, .loop_vsp_sve2_4x\h
    ret
endfunc
.endm

LUMA_VSP_4xN_SVE2 4
LUMA_VSP_4xN_SVE2 8
LUMA_VSP_4xN_SVE2 16

.macro vps_end_sve2
    sub             z17.h, z17.h, z31.h
.endm

.macro FILTER_VPS_SVE2 w, h, v
    lsl             x3, x3, #1
    lsl             x10, x1, #2      // x10 = 4 * x1
    sub             x11, x10, x1     // x11 = 3 * x1
    sub             x0, x0, x11      // src -= (8 / 2 - 1) * srcStride
    mov             x5, #\h
    mov             z31.h, #8192
    rdvl            x14, #1
    cmp             x14, #16
    bgt             .vl_gt_16_FILTER_VPS_\v\()_\w\()x\h
    qpel_start_\v
.loop_ps_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.loop_ps_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
.if \w == 8 || \w == 24
    qpel_load_32b \v
    qpel_filter_\v\()_32b
    vps_end
    str             q17, [x7], #16
    add             x9, x9, #8
.elseif \w == 12
    qpel_load_32b \v
    qpel_filter_\v\()_32b
    vps_end
    str             q17, [x7], #16
    add             x6, x0, #8
    qpel_load_32b \v
    qpel_filter_\v\()_32b
    vps_end
    str             d17, [x7], #8
    add             x9, x9, #12
.else
    qpel_load_64b \v
    qpel_filter_\v\()_64b
    vps_end
    sub             v18.8h, v18.8h, v31.8h
    stp             q17, q18, [x7], #32
    add             x9, x9, #16
.endif
    cmp             x9, #\w
    blt             .loop_ps_w8_sve2_\v\()_\w\()x\h
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_ps_sve2_\v\()_\w\()x\h
    ret
.vl_gt_16_FILTER_VPS_\v\()_\w\()x\h:
    ptrue           p0.h, vl8
    ptrue           p2.h, vl16
    qpel_start_sve2_\v
.gt_16_loop_ps_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.gt_16_loop_ps_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
.if \w == 8 || \w == 24
    qpel_load_32b_sve2 \v
    qpel_filter_sve2_\v\()_32b
    vps_end_sve2
    str             q17, [x7], #16
    add             x9, x9, #8
.elseif \w == 12
    qpel_load_32b_sve2 \v
    qpel_filter_sve2_\v\()_32b
    vps_end_sve2
    str             q17, [x7], #16
    add             x6, x0, #8
    qpel_load_32b_sve2 \v
    qpel_filter_sve2_\v\()_32b
    vps_end_sve2
    str             d17, [x7], #8
    add             x9, x9, #12
.else
    qpel_load_64b_sve2_gt_16 \v
    qpel_filter_sve2_\v\()_32b
    vps_end_sve2
    sub             z18.h, z18.h, z31.h
    stp             q17, q18, [x7], #32
    add             x9, x9, #16
.endif
    cmp             x9, #\w
    blt             .gt_16_loop_ps_w8_sve2_\v\()_\w\()x\h
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .gt_16_loop_ps_sve2_\v\()_\w\()x\h
    ret
.endm

// void interp_vert_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPS_SVE2 w, h
function x265_interp_8tap_vert_ps_\w\()x\h\()_sve2
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
0:
    FILTER_VPS_SVE2 \w, \h, 0
1:
    FILTER_VPS_SVE2 \w, \h, 1
2:
    FILTER_VPS_SVE2 \w, \h, 2
3:
    FILTER_VPS_SVE2 \w, \h, 3
endfunc
.endm

LUMA_VPS_SVE2 8, 4
LUMA_VPS_SVE2 8, 8
LUMA_VPS_SVE2 8, 16
LUMA_VPS_SVE2 8, 32
LUMA_VPS_SVE2 12, 16
LUMA_VPS_SVE2 16, 4
LUMA_VPS_SVE2 16, 8
LUMA_VPS_SVE2 16, 16
LUMA_VPS_SVE2 16, 32
LUMA_VPS_SVE2 16, 64
LUMA_VPS_SVE2 16, 12
LUMA_VPS_SVE2 24, 32
LUMA_VPS_SVE2 32, 8
LUMA_VPS_SVE2 32, 16
LUMA_VPS_SVE2 32, 32
LUMA_VPS_SVE2 32, 64
LUMA_VPS_SVE2 32, 24
LUMA_VPS_SVE2 48, 64
LUMA_VPS_SVE2 64, 16
LUMA_VPS_SVE2 64, 32
LUMA_VPS_SVE2 64, 64
LUMA_VPS_SVE2 64, 48

// ***** luma_vss *****
.macro vss_end_sve2
    asr             z17.s, z17.s, #6
    asr             z18.s, z18.s, #6
    uzp1            v17.8h, v17.8h, v18.8h
.endm

.macro FILTER_VSS_SVE2 w, h, v
    lsl             x1, x1, #1
    lsl             x10, x1, #2      // x10 = 4 * x1
    sub             x11, x10, x1     // x11 = 3 * x1
    sub             x0, x0, x11
    lsl             x3, x3, #1
    mov             x5, #\h
    mov             x12, #\w
    lsl             x12, x12, #1
    qpel_start_\v\()_1
.loop_luma_vss_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.loop_luma_vss_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
    qpel_load_64b \v
    qpel_filter_\v\()_32b_1
    vss_end_sve2
.if \w == 4
    str             s17, [x7], #4
    add             x9, x9, #4
.else
    str             q17, [x7], #16
    add             x9, x9, #16
.if \w == 12
    add             x6, x0, x9
    qpel_load_64b \v
    qpel_filter_\v\()_32b_1
    vss_end_sve2
    str             d17, [x7], #8
    add             x9, x9, #8
.endif
.endif
    cmp             x9, x12
    blt             .loop_luma_vss_w8_sve2_\v\()_\w\()x\h
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_luma_vss_sve2_\v\()_\w\()x\h
    ret
.endm

// void interp_vert_ss_c(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VSS_SVE2 w, h
function x265_interp_8tap_vert_ss_\w\()x\h\()_sve2
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
0:
    FILTER_VSS_SVE2 \w, \h, 0
1:
    FILTER_VSS_SVE2 \w, \h, 1
2:
    FILTER_VSS_SVE2 \w, \h, 2
3:
    FILTER_VSS_SVE2 \w, \h, 3
endfunc
.endm

LUMA_VSS_SVE2 4, 4
LUMA_VSS_SVE2 4, 8
LUMA_VSS_SVE2 4, 16
LUMA_VSS_SVE2 8, 4
LUMA_VSS_SVE2 8, 8
LUMA_VSS_SVE2 8, 16
LUMA_VSS_SVE2 8, 32
LUMA_VSS_SVE2 12, 16
LUMA_VSS_SVE2 16, 4
LUMA_VSS_SVE2 16, 8
LUMA_VSS_SVE2 16, 16
LUMA_VSS_SVE2 16, 32
LUMA_VSS_SVE2 16, 64
LUMA_VSS_SVE2 16, 12
LUMA_VSS_SVE2 32, 8
LUMA_VSS_SVE2 32, 16
LUMA_VSS_SVE2 32, 32
LUMA_VSS_SVE2 32, 64
LUMA_VSS_SVE2 32, 24
LUMA_VSS_SVE2 64, 16
LUMA_VSS_SVE2 64, 32
LUMA_VSS_SVE2 64, 64
LUMA_VSS_SVE2 64, 48
LUMA_VSS_SVE2 24, 32
LUMA_VSS_SVE2 48, 64

// ***** luma_hps *****

.macro FILTER_CHROMA_VPP_SVE2 w, h, v
    ptrue           p0.h, vl8
    qpel_start_chroma_sve2_\v
    mov             z31.h, #32
    sub             x0, x0, x1
    mov             x5, #\h
.loop_chroma_vpp_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.loop_chroma_vpp_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
    qpel_chroma_load_32b_sve2 \v
    qpel_filter_chroma_sve2_\v\()_32b
    vpp_end_sve2
    add             x9, x9, #8
.if \w == 2
    fmov            w12, s17
    strh            w12, [x7], #2
.elseif \w == 4
    str             s17, [x7], #4
.elseif \w == 6
    str             s17, [x7], #4
    umov            w12, v17.h[2]
    strh            w12, [x7], #2
.elseif \w == 12
    str             d17, [x7], #8
    add             x6, x0, x9
    qpel_chroma_load_32b_sve2 \v
    qpel_filter_chroma_sve2_\v\()_32b
    vpp_end_sve2
    str             s17, [x7], #4
    add             x9, x9, #8
.else
    str             d17, [x7], #8
.endif
    cmp             x9, #\w
    blt             .loop_chroma_vpp_w8_sve2_\v\()_\w\()x\h
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_chroma_vpp_sve2_\v\()_\w\()x\h
    ret
.endm

// void interp_vert_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VPP_SVE2 w, h
function x265_interp_4tap_vert_pp_\w\()x\h\()_sve2
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
    cmp             x4, #4
    beq             4f
    cmp             x4, #5
    beq             5f
    cmp             x4, #6
    beq             6f
    cmp             x4, #7
    beq             7f
0:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 0
1:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 1
2:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 2
3:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 3
4:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 4
5:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 5
6:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 6
7:
    FILTER_CHROMA_VPP_SVE2  \w, \h, 7
endfunc
.endm

CHROMA_VPP_SVE2 2, 4
CHROMA_VPP_SVE2 2, 8
CHROMA_VPP_SVE2 2, 16
CHROMA_VPP_SVE2 4, 2
CHROMA_VPP_SVE2 4, 4
CHROMA_VPP_SVE2 4, 8
CHROMA_VPP_SVE2 4, 16
CHROMA_VPP_SVE2 4, 32
CHROMA_VPP_SVE2 6, 8
CHROMA_VPP_SVE2 6, 16
CHROMA_VPP_SVE2 8, 2
CHROMA_VPP_SVE2 8, 4
CHROMA_VPP_SVE2 8, 6
CHROMA_VPP_SVE2 8, 8
CHROMA_VPP_SVE2 8, 16
CHROMA_VPP_SVE2 8, 32
CHROMA_VPP_SVE2 8, 12
CHROMA_VPP_SVE2 8, 64
CHROMA_VPP_SVE2 12, 16
CHROMA_VPP_SVE2 12, 32
CHROMA_VPP_SVE2 16, 4
CHROMA_VPP_SVE2 16, 8
CHROMA_VPP_SVE2 16, 12
CHROMA_VPP_SVE2 16, 16
CHROMA_VPP_SVE2 16, 32
CHROMA_VPP_SVE2 16, 64
CHROMA_VPP_SVE2 16, 24
CHROMA_VPP_SVE2 32, 8
CHROMA_VPP_SVE2 32, 16
CHROMA_VPP_SVE2 32, 24
CHROMA_VPP_SVE2 32, 32
CHROMA_VPP_SVE2 32, 64
CHROMA_VPP_SVE2 32, 48
CHROMA_VPP_SVE2 24, 32
CHROMA_VPP_SVE2 24, 64
CHROMA_VPP_SVE2 64, 16
CHROMA_VPP_SVE2 64, 32
CHROMA_VPP_SVE2 64, 48
CHROMA_VPP_SVE2 64, 64
CHROMA_VPP_SVE2 48, 64

.macro FILTER_CHROMA_VPS_SVE2 w, h, v
    ptrue           p0.h, vl8
    qpel_start_chroma_sve2_\v
    mov             z31.h, #8192
    lsl             x3, x3, #1
    sub             x0, x0, x1
    mov             x5, #\h
.loop_vps_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.loop_vps_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
    qpel_chroma_load_32b_sve2 \v
    qpel_filter_chroma_sve2_\v\()_32b
    vps_end_sve2
    add             x9, x9, #8
.if \w == 2
    str             s17, [x7], #4
.elseif \w == 4
    str             d17, [x7], #8
.elseif \w == 6
    str             d17, [x7], #8
    st1             {v17.s}[2], [x7], #4
.elseif \w == 12
    str             q17, [x7], #16
    add             x6, x0, x9
    qpel_chroma_load_32b_sve2 \v
    qpel_filter_chroma_sve2_\v\()_32b
    vps_end_sve2
    str             d17, [x7], #8
    add             x9, x9, #8
.else
    str             q17, [x7], #16
.endif
    cmp             x9, #\w
    blt             .loop_vps_w8_sve2_\v\()_\w\()x\h

    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_vps_sve2_\v\()_\w\()x\h
    ret
.endm

// void interp_vert_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VPS_SVE2 w, h
function x265_interp_4tap_vert_ps_\w\()x\h\()_sve2
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
    cmp             x4, #4
    beq             4f
    cmp             x4, #5
    beq             5f
    cmp             x4, #6
    beq             6f
    cmp             x4, #7
    beq             7f
0:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 0
1:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 1
2:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 2
3:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 3
4:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 4
5:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 5
6:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 6
7:
    FILTER_CHROMA_VPS_SVE2  \w, \h, 7
endfunc
.endm

CHROMA_VPS_SVE2 2, 4
CHROMA_VPS_SVE2 2, 8
CHROMA_VPS_SVE2 2, 16
CHROMA_VPS_SVE2 4, 2
CHROMA_VPS_SVE2 4, 4
CHROMA_VPS_SVE2 4, 8
CHROMA_VPS_SVE2 4, 16
CHROMA_VPS_SVE2 4, 32
CHROMA_VPS_SVE2 6, 8
CHROMA_VPS_SVE2 6, 16
CHROMA_VPS_SVE2 8, 2
CHROMA_VPS_SVE2 8, 4
CHROMA_VPS_SVE2 8, 6
CHROMA_VPS_SVE2 8, 8
CHROMA_VPS_SVE2 8, 16
CHROMA_VPS_SVE2 8, 32
CHROMA_VPS_SVE2 8, 12
CHROMA_VPS_SVE2 8, 64
CHROMA_VPS_SVE2 12, 16
CHROMA_VPS_SVE2 12, 32
CHROMA_VPS_SVE2 16, 4
CHROMA_VPS_SVE2 16, 8
CHROMA_VPS_SVE2 16, 12
CHROMA_VPS_SVE2 16, 16
CHROMA_VPS_SVE2 16, 32
CHROMA_VPS_SVE2 16, 64
CHROMA_VPS_SVE2 16, 24
CHROMA_VPS_SVE2 32, 8
CHROMA_VPS_SVE2 32, 16
CHROMA_VPS_SVE2 32, 24
CHROMA_VPS_SVE2 32, 32
CHROMA_VPS_SVE2 32, 64
CHROMA_VPS_SVE2 32, 48
CHROMA_VPS_SVE2 24, 32
CHROMA_VPS_SVE2 24, 64
CHROMA_VPS_SVE2 64, 16
CHROMA_VPS_SVE2 64, 32
CHROMA_VPS_SVE2 64, 48
CHROMA_VPS_SVE2 64, 64
CHROMA_VPS_SVE2 48, 64

.macro qpel_start_chroma_sve2_0_1
    mov             z24.h, #64
.endm

.macro qpel_start_chroma_sve2_1_1
    mov             z24.h, #58
    mov             z25.h, #10
.endm

.macro qpel_start_chroma_sve2_2_1
    mov             z25.h, #54
.endm

.macro qpel_start_chroma_sve2_3_1
    mov             z25.h, #46
    mov             z26.h, #28
    mov             z27.h, #6
.endm

.macro qpel_start_chroma_sve2_4_1
    mov             z24.h, #36
.endm

.macro qpel_start_chroma_sve2_5_1
    mov             z25.h, #28
    mov             z26.h, #46
    mov             z27.h, #6
.endm

.macro qpel_start_chroma_sve2_6_1
    mov             z25.h, #54
.endm

.macro qpel_start_chroma_sve2_7_1
    mov             z24.h, #58
    mov             z25.h, #10
.endm

.macro FILTER_CHROMA_VSS_SVE2 w, h, v
    lsl             x1, x1, #1
    sub             x0, x0, x1
    lsl             x3, x3, #1
    mov             x5, #\h
    mov             x12, #\w
    lsl             x12, x12, #1
    qpel_start_chroma_sve2_\v\()_1
.loop_vss_sve2_\v\()_\w\()x\h:
    mov             x7, x2
    mov             x9, #0
.if \w == 4
.rept 2
    add             x6, x0, x9
    qpel_chroma_load_64b \v
    qpel_filter_chroma_\v\()_32b_1
    vss_end_sve2
    str             s17, [x7], #4
    add             x9, x9, #4
.endr
.else
.loop_vss_w8_sve2_\v\()_\w\()x\h:
    add             x6, x0, x9
    qpel_chroma_load_64b \v
    qpel_filter_chroma_\v\()_32b_1
    vss_end_sve2
    str             q17, [x7], #16
    add             x9, x9, #16
.if \w == 12
    add             x6, x0, x9
    qpel_chroma_load_64b \v
    qpel_filter_chroma_\v\()_32b_1
    vss_end_sve2
    str             d17, [x7], #8
    add             x9, x9, #8
.endif
    cmp             x9, x12
    blt             .loop_vss_w8_sve2_\v\()_\w\()x\h
.endif
    add             x0, x0, x1
    add             x2, x2, x3
    sub             x5, x5, #1
    cbnz            x5, .loop_vss_sve2_\v\()_\w\()x\h
    ret
.endm

// void interp_vert_ss_c(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VSS_SVE2 w, h
function x265_interp_4tap_vert_ss_\w\()x\h\()_sve2
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
    cmp             x4, #4
    beq             4f
    cmp             x4, #5
    beq             5f
    cmp             x4, #6
    beq             6f
    cmp             x4, #7
    beq             7f
0:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 0
1:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 1
2:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 2
3:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 3
4:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 4
5:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 5
6:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 6
7:
    FILTER_CHROMA_VSS_SVE2  \w, \h, 7
endfunc
.endm

CHROMA_VSS_SVE2 4, 4
CHROMA_VSS_SVE2 4, 8
CHROMA_VSS_SVE2 4, 16
CHROMA_VSS_SVE2 4, 32
CHROMA_VSS_SVE2 8, 2
CHROMA_VSS_SVE2 8, 4
CHROMA_VSS_SVE2 8, 6
CHROMA_VSS_SVE2 8, 8
CHROMA_VSS_SVE2 8, 16
CHROMA_VSS_SVE2 8, 32
CHROMA_VSS_SVE2 8, 12
CHROMA_VSS_SVE2 8, 64
CHROMA_VSS_SVE2 12, 16
CHROMA_VSS_SVE2 12, 32
CHROMA_VSS_SVE2 16, 4
CHROMA_VSS_SVE2 16, 8
CHROMA_VSS_SVE2 16, 12
CHROMA_VSS_SVE2 16, 16
CHROMA_VSS_SVE2 16, 32
CHROMA_VSS_SVE2 16, 64
CHROMA_VSS_SVE2 16, 24
CHROMA_VSS_SVE2 32, 8
CHROMA_VSS_SVE2 32, 16
CHROMA_VSS_SVE2 32, 24
CHROMA_VSS_SVE2 32, 32
CHROMA_VSS_SVE2 32, 64
CHROMA_VSS_SVE2 32, 48
CHROMA_VSS_SVE2 24, 32
CHROMA_VSS_SVE2 24, 64
CHROMA_VSS_SVE2 64, 16
CHROMA_VSS_SVE2 64, 32
CHROMA_VSS_SVE2 64, 48
CHROMA_VSS_SVE2 64, 64
CHROMA_VSS_SVE2 48, 64
