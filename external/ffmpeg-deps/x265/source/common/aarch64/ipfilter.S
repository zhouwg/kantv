/*****************************************************************************
 * Copyright (C) 2021 MulticoreWare, Inc
 *
 * Authors: Sebastian Pop <spop@amazon.com>
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

#include "asm.S"
#include "ipfilter-common.S"

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

// ***** luma_vpp *****
// void interp_vert_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPP_4xN h
function x265_interp_8tap_vert_pp_4x\h\()_neon
    movrel          x10, g_luma_s16
    sub             x0, x0, x1
    sub             x0, x0, x1, lsl #1         // src -= 3 * srcStride
    lsl             x4, x4, #4
    ldr             q0, [x10, x4]              // q0 = luma interpolate coeff
    dup             v24.8h, v0.h[0]
    dup             v25.8h, v0.h[1]
    trn1            v24.2d, v24.2d, v25.2d
    dup             v26.8h, v0.h[2]
    dup             v27.8h, v0.h[3]
    trn1            v26.2d, v26.2d, v27.2d
    dup             v28.8h, v0.h[4]
    dup             v29.8h, v0.h[5]
    trn1            v28.2d, v28.2d, v29.2d
    dup             v30.8h, v0.h[6]
    dup             v31.8h, v0.h[7]
    trn1            v30.2d, v30.2d, v31.2d

    // prepare to load 8 lines
    ld1             {v0.s}[0], [x0], x1
    ld1             {v0.s}[1], [x0], x1
    ushll           v0.8h, v0.8b, #0
    ld1             {v1.s}[0], [x0], x1
    ld1             {v1.s}[1], [x0], x1
    ushll           v1.8h, v1.8b, #0
    ld1             {v2.s}[0], [x0], x1
    ld1             {v2.s}[1], [x0], x1
    ushll           v2.8h, v2.8b, #0
    ld1             {v3.s}[0], [x0], x1
    ld1             {v3.s}[1], [x0], x1
    ushll           v3.8h, v3.8b, #0

    mov             x9, #\h
.loop_4x\h:
    ld1             {v4.s}[0], [x0], x1
    ld1             {v4.s}[1], [x0], x1
    ushll           v4.8h, v4.8b, #0

    // row[0-1]
    mul             v16.8h, v0.8h, v24.8h
    ext             v21.16b, v0.16b, v1.16b, #8
    mul             v17.8h, v21.8h, v24.8h
    mov             v0.16b, v1.16b

    // row[2-3]
    mla             v16.8h, v1.8h, v26.8h
    ext             v21.16b, v1.16b, v2.16b, #8
    mla             v17.8h, v21.8h, v26.8h
    mov             v1.16b, v2.16b

    // row[4-5]
    mla             v16.8h, v2.8h, v28.8h
    ext             v21.16b, v2.16b, v3.16b, #8
    mla             v17.8h, v21.8h, v28.8h
    mov             v2.16b, v3.16b

    // row[6-7]
    mla             v16.8h, v3.8h, v30.8h
    ext             v21.16b, v3.16b, v4.16b, #8
    mla             v17.8h, v21.8h, v30.8h
    mov             v3.16b, v4.16b

    // sum row[0-7]
    trn1            v20.2d, v16.2d, v17.2d
    trn2            v21.2d, v16.2d, v17.2d
    add             v16.8h, v20.8h, v21.8h

    sqrshrun        v16.8b,  v16.8h,  #6
    st1             {v16.s}[0], [x2], x3
    st1             {v16.s}[1], [x2], x3

    sub             x9, x9, #2
    cbnz            x9, .loop_4x\h
    ret
endfunc
.endm

LUMA_VPP_4xN 4
LUMA_VPP_4xN 8
LUMA_VPP_4xN 16

// void interp_vert_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPP w, h
function x265_interp_8tap_vert_pp_\w\()x\h\()_neon
    cmp             x4, #0
    b.eq            0f
    cmp             x4, #1
    b.eq            1f
    cmp             x4, #2
    b.eq            2f
    cmp             x4, #3
    b.eq            3f
0:
    FILTER_LUMA_VPP \w, \h, 0
1:
    FILTER_LUMA_VPP \w, \h, 1
2:
    FILTER_LUMA_VPP \w, \h, 2
3:
    FILTER_LUMA_VPP \w, \h, 3
endfunc
.endm

LUMA_VPP 8, 4
LUMA_VPP 8, 8
LUMA_VPP 8, 16
LUMA_VPP 8, 32
LUMA_VPP 12, 16
LUMA_VPP 16, 4
LUMA_VPP 16, 8
LUMA_VPP 16, 16
LUMA_VPP 16, 32
LUMA_VPP 16, 64
LUMA_VPP 16, 12
LUMA_VPP 24, 32
LUMA_VPP 32, 8
LUMA_VPP 32, 16
LUMA_VPP 32, 32
LUMA_VPP 32, 64
LUMA_VPP 32, 24
LUMA_VPP 48, 64
LUMA_VPP 64, 16
LUMA_VPP 64, 32
LUMA_VPP 64, 64
LUMA_VPP 64, 48

// ***** luma_vps *****
// void interp_vert_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPS_4xN h
function x265_interp_8tap_vert_ps_4x\h\()_neon
    lsl             x3, x3, #1
    lsl             x5, x4, #6
    lsl             x4, x1, #2
    sub             x4, x4, x1
    sub             x0, x0, x4

    mov             w6, #8192
    dup             v28.4s, w6
    mov             x4, #\h
    movrel          x12, g_lumaFilter
    add             x12, x12, x5
    ld1r            {v16.2d}, [x12], #8
    ld1r            {v17.2d}, [x12], #8
    ld1r            {v18.2d}, [x12], #8
    ld1r            {v19.2d}, [x12], #8
    ld1r            {v20.2d}, [x12], #8
    ld1r            {v21.2d}, [x12], #8
    ld1r            {v22.2d}, [x12], #8
    ld1r            {v23.2d}, [x12], #8

.loop_vps_4x\h:
    mov             x6, x0

    ld1             {v0.s}[0], [x6], x1
    ld1             {v1.s}[0], [x6], x1
    ld1             {v2.s}[0], [x6], x1
    ld1             {v3.s}[0], [x6], x1
    ld1             {v4.s}[0], [x6], x1
    ld1             {v5.s}[0], [x6], x1
    ld1             {v6.s}[0], [x6], x1
    ld1             {v7.s}[0], [x6], x1
    uxtl            v0.8h, v0.8b
    uxtl            v0.4s, v0.4h

    uxtl            v1.8h, v1.8b
    uxtl            v1.4s, v1.4h
    mul             v0.4s, v0.4s, v16.4s

    uxtl            v2.8h, v2.8b
    uxtl            v2.4s, v2.4h
    mla             v0.4s, v1.4s, v17.4s

    uxtl            v3.8h, v3.8b
    uxtl            v3.4s, v3.4h
    mla             v0.4s, v2.4s, v18.4s

    uxtl            v4.8h, v4.8b
    uxtl            v4.4s, v4.4h
    mla             v0.4s, v3.4s, v19.4s

    uxtl            v5.8h, v5.8b
    uxtl            v5.4s, v5.4h
    mla             v0.4s, v4.4s, v20.4s

    uxtl            v6.8h, v6.8b
    uxtl            v6.4s, v6.4h
    mla             v0.4s, v5.4s, v21.4s

    uxtl            v7.8h, v7.8b
    uxtl            v7.4s, v7.4h
    mla             v0.4s, v6.4s, v22.4s

    mla             v0.4s, v7.4s, v23.4s

    sub             v0.4s, v0.4s, v28.4s
    sqxtn           v0.4h, v0.4s
    st1             {v0.8b}, [x2], x3

    add             x0, x0, x1
    sub             x4, x4, #1
    cbnz            x4, .loop_vps_4x\h
    ret
endfunc
.endm

LUMA_VPS_4xN 4
LUMA_VPS_4xN 8
LUMA_VPS_4xN 16

// void interp_vert_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VPS w, h
function x265_interp_8tap_vert_ps_\w\()x\h\()_neon
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
0:
    FILTER_VPS \w, \h, 0
1:
    FILTER_VPS \w, \h, 1
2:
    FILTER_VPS \w, \h, 2
3:
    FILTER_VPS \w, \h, 3
endfunc
.endm

LUMA_VPS 8, 4
LUMA_VPS 8, 8
LUMA_VPS 8, 16
LUMA_VPS 8, 32
LUMA_VPS 12, 16
LUMA_VPS 16, 4
LUMA_VPS 16, 8
LUMA_VPS 16, 16
LUMA_VPS 16, 32
LUMA_VPS 16, 64
LUMA_VPS 16, 12
LUMA_VPS 24, 32
LUMA_VPS 32, 8
LUMA_VPS 32, 16
LUMA_VPS 32, 32
LUMA_VPS 32, 64
LUMA_VPS 32, 24
LUMA_VPS 48, 64
LUMA_VPS 64, 16
LUMA_VPS 64, 32
LUMA_VPS 64, 64
LUMA_VPS 64, 48

// ***** luma_vsp *****
// void interp_vert_sp_c(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VSP_4xN h
function x265_interp_8tap_vert_sp_4x\h\()_neon
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
    ld1r            {v16.2d}, [x12], #8
    ld1r            {v17.2d}, [x12], #8
    ld1r            {v18.2d}, [x12], #8
    ld1r            {v19.2d}, [x12], #8
    ld1r            {v20.2d}, [x12], #8
    ld1r            {v21.2d}, [x12], #8
    ld1r            {v22.2d}, [x12], #8
    ld1r            {v23.2d}, [x12], #8
.loop_vsp_4x\h:
    mov             x6, x0

    ld1             {v0.8b}, [x6], x1
    ld1             {v1.8b}, [x6], x1
    ld1             {v2.8b}, [x6], x1
    ld1             {v3.8b}, [x6], x1
    ld1             {v4.8b}, [x6], x1
    ld1             {v5.8b}, [x6], x1
    ld1             {v6.8b}, [x6], x1
    ld1             {v7.8b}, [x6], x1

    sshll           v0.4s, v0.4h, #0
    sshll           v1.4s, v1.4h, #0
    mul             v0.4s, v0.4s, v16.4s
    sshll           v2.4s, v2.4h, #0
    mla             v0.4s, v1.4s, v17.4s
    sshll           v3.4s, v3.4h, #0
    mla             v0.4s, v2.4s, v18.4s
    sshll           v4.4s, v4.4h, #0
    mla             v0.4s, v3.4s, v19.4s
    sshll           v5.4s, v5.4h, #0
    mla             v0.4s, v4.4s, v20.4s
    sshll           v6.4s, v6.4h, #0
    mla             v0.4s, v5.4s, v21.4s
    sshll           v7.4s, v7.4h, #0
    mla             v0.4s, v6.4s, v22.4s

    mla             v0.4s, v7.4s, v23.4s

    add             v0.4s, v0.4s, v24.4s
    sqshrun         v0.4h, v0.4s, #12
    sqxtun          v0.8b, v0.8h
    st1             {v0.s}[0], [x2], x3

    add             x0, x0, x1
    sub             x4, x4, #1
    cbnz            x4, .loop_vsp_4x\h
    ret
endfunc
.endm

LUMA_VSP_4xN 4
LUMA_VSP_4xN 8
LUMA_VSP_4xN 16

// void interp_vert_sp_c(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VSP w, h
function x265_interp_8tap_vert_sp_\w\()x\h\()_neon
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
0:
    FILTER_VSP \w, \h, 0
1:
    FILTER_VSP \w, \h, 1
2:
    FILTER_VSP \w, \h, 2
3:
    FILTER_VSP \w, \h, 3
endfunc
.endm

LUMA_VSP 8, 4
LUMA_VSP 8, 8
LUMA_VSP 8, 16
LUMA_VSP 8, 32
LUMA_VSP 12, 16
LUMA_VSP 16, 4
LUMA_VSP 16, 8
LUMA_VSP 16, 16
LUMA_VSP 16, 32
LUMA_VSP 16, 64
LUMA_VSP 16, 12
LUMA_VSP 32, 8
LUMA_VSP 32, 16
LUMA_VSP 32, 32
LUMA_VSP 32, 64
LUMA_VSP 32, 24
LUMA_VSP 64, 16
LUMA_VSP 64, 32
LUMA_VSP 64, 64
LUMA_VSP 64, 48
LUMA_VSP 24, 32
LUMA_VSP 48, 64

// ***** luma_vss *****
// void interp_vert_ss_c(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_VSS w, h
function x265_interp_8tap_vert_ss_\w\()x\h\()_neon
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
0:
    FILTER_VSS \w, \h, 0
1:
    FILTER_VSS \w, \h, 1
2:
    FILTER_VSS \w, \h, 2
3:
    FILTER_VSS \w, \h, 3
endfunc
.endm

LUMA_VSS 4, 4
LUMA_VSS 4, 8
LUMA_VSS 4, 16
LUMA_VSS 8, 4
LUMA_VSS 8, 8
LUMA_VSS 8, 16
LUMA_VSS 8, 32
LUMA_VSS 12, 16
LUMA_VSS 16, 4
LUMA_VSS 16, 8
LUMA_VSS 16, 16
LUMA_VSS 16, 32
LUMA_VSS 16, 64
LUMA_VSS 16, 12
LUMA_VSS 32, 8
LUMA_VSS 32, 16
LUMA_VSS 32, 32
LUMA_VSS 32, 64
LUMA_VSS 32, 24
LUMA_VSS 64, 16
LUMA_VSS 64, 32
LUMA_VSS 64, 64
LUMA_VSS 64, 48
LUMA_VSS 24, 32
LUMA_VSS 48, 64

// ***** luma_hpp *****
// void interp_horiz_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro LUMA_HPP w, h
function x265_interp_horiz_pp_\w\()x\h\()_neon
    cmp             x4, #0
    beq             0f
    cmp             x4, #1
    beq             1f
    cmp             x4, #2
    beq             2f
    cmp             x4, #3
    beq             3f
0:
    FILTER_HPP \w, \h, 0
1:
    FILTER_HPP \w, \h, 1
2:
    FILTER_HPP \w, \h, 2
3:
    FILTER_HPP \w, \h, 3
endfunc
.endm

LUMA_HPP 4, 4
LUMA_HPP 4, 8
LUMA_HPP 4, 16
LUMA_HPP 8, 4
LUMA_HPP 8, 8
LUMA_HPP 8, 16
LUMA_HPP 8, 32
LUMA_HPP 12, 16
LUMA_HPP 16, 4
LUMA_HPP 16, 8
LUMA_HPP 16, 12
LUMA_HPP 16, 16
LUMA_HPP 16, 32
LUMA_HPP 16, 64
LUMA_HPP 24, 32
LUMA_HPP 32, 8
LUMA_HPP 32, 16
LUMA_HPP 32, 24
LUMA_HPP 32, 32
LUMA_HPP 32, 64
LUMA_HPP 48, 64
LUMA_HPP 64, 16
LUMA_HPP 64, 32
LUMA_HPP 64, 48
LUMA_HPP 64, 64

// ***** luma_hps *****
// void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
.macro LUMA_HPS w, h
function x265_interp_horiz_ps_\w\()x\h\()_neon
    mov             w10, #\h
    cmp             w5, #0
    b.eq            6f
    sub             x0, x0, x1, lsl #2
    add             x0, x0, x1
    add             w10, w10, #7
6:
    mov             w6, w10
    cmp             w4, #0
    b.eq            0f
    cmp             w4, #1
    b.eq            1f
    cmp             w4, #2
    b.eq            2f
    cmp             w4, #3
    b.eq            3f
0:
    FILTER_HPS \w, \h, 0
1:
    FILTER_HPS \w, \h, 1
2:
    FILTER_HPS \w, \h, 2
3:
    FILTER_HPS \w, \h, 3
endfunc
.endm

LUMA_HPS 4, 4
LUMA_HPS 4, 8
LUMA_HPS 4, 16
LUMA_HPS 8, 4
LUMA_HPS 8, 8
LUMA_HPS 8, 16
LUMA_HPS 8, 32
LUMA_HPS 12, 16
LUMA_HPS 16, 4
LUMA_HPS 16, 8
LUMA_HPS 16, 12
LUMA_HPS 16, 16
LUMA_HPS 16, 32
LUMA_HPS 16, 64
LUMA_HPS 24, 32
LUMA_HPS 32, 8
LUMA_HPS 32, 16
LUMA_HPS 32, 24
LUMA_HPS 32, 32
LUMA_HPS 32, 64
LUMA_HPS 48, 64
LUMA_HPS 64, 16
LUMA_HPS 64, 32
LUMA_HPS 64, 48
LUMA_HPS 64, 64

// ***** chroma_vpp *****
// void interp_vert_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VPP w, h
function x265_interp_4tap_vert_pp_\w\()x\h\()_neon
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
    FILTER_CHROMA_VPP  \w, \h, 0
1:
    FILTER_CHROMA_VPP  \w, \h, 1
2:
    FILTER_CHROMA_VPP  \w, \h, 2
3:
    FILTER_CHROMA_VPP  \w, \h, 3
4:
    FILTER_CHROMA_VPP  \w, \h, 4
5:
    FILTER_CHROMA_VPP  \w, \h, 5
6:
    FILTER_CHROMA_VPP  \w, \h, 6
7:
    FILTER_CHROMA_VPP  \w, \h, 7
endfunc
.endm

CHROMA_VPP 2, 4
CHROMA_VPP 2, 8
CHROMA_VPP 2, 16
CHROMA_VPP 4, 2
CHROMA_VPP 4, 4
CHROMA_VPP 4, 8
CHROMA_VPP 4, 16
CHROMA_VPP 4, 32
CHROMA_VPP 6, 8
CHROMA_VPP 6, 16
CHROMA_VPP 8, 2
CHROMA_VPP 8, 4
CHROMA_VPP 8, 6
CHROMA_VPP 8, 8
CHROMA_VPP 8, 16
CHROMA_VPP 8, 32
CHROMA_VPP 8, 12
CHROMA_VPP 8, 64
CHROMA_VPP 12, 16
CHROMA_VPP 12, 32
CHROMA_VPP 16, 4
CHROMA_VPP 16, 8
CHROMA_VPP 16, 12
CHROMA_VPP 16, 16
CHROMA_VPP 16, 32
CHROMA_VPP 16, 64
CHROMA_VPP 16, 24
CHROMA_VPP 32, 8
CHROMA_VPP 32, 16
CHROMA_VPP 32, 24
CHROMA_VPP 32, 32
CHROMA_VPP 32, 64
CHROMA_VPP 32, 48
CHROMA_VPP 24, 32
CHROMA_VPP 24, 64
CHROMA_VPP 64, 16
CHROMA_VPP 64, 32
CHROMA_VPP 64, 48
CHROMA_VPP 64, 64
CHROMA_VPP 48, 64

// ***** chroma_vps *****
// void interp_vert_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VPS w, h
function x265_interp_4tap_vert_ps_\w\()x\h\()_neon
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
    FILTER_CHROMA_VPS  \w, \h, 0
1:
    FILTER_CHROMA_VPS  \w, \h, 1
2:
    FILTER_CHROMA_VPS  \w, \h, 2
3:
    FILTER_CHROMA_VPS  \w, \h, 3
4:
    FILTER_CHROMA_VPS  \w, \h, 4
5:
    FILTER_CHROMA_VPS  \w, \h, 5
6:
    FILTER_CHROMA_VPS  \w, \h, 6
7:
    FILTER_CHROMA_VPS  \w, \h, 7
endfunc
.endm

CHROMA_VPS 2, 4
CHROMA_VPS 2, 8
CHROMA_VPS 2, 16
CHROMA_VPS 4, 2
CHROMA_VPS 4, 4
CHROMA_VPS 4, 8
CHROMA_VPS 4, 16
CHROMA_VPS 4, 32
CHROMA_VPS 6, 8
CHROMA_VPS 6, 16
CHROMA_VPS 8, 2
CHROMA_VPS 8, 4
CHROMA_VPS 8, 6
CHROMA_VPS 8, 8
CHROMA_VPS 8, 16
CHROMA_VPS 8, 32
CHROMA_VPS 8, 12
CHROMA_VPS 8, 64
CHROMA_VPS 12, 16
CHROMA_VPS 12, 32
CHROMA_VPS 16, 4
CHROMA_VPS 16, 8
CHROMA_VPS 16, 12
CHROMA_VPS 16, 16
CHROMA_VPS 16, 32
CHROMA_VPS 16, 64
CHROMA_VPS 16, 24
CHROMA_VPS 32, 8
CHROMA_VPS 32, 16
CHROMA_VPS 32, 24
CHROMA_VPS 32, 32
CHROMA_VPS 32, 64
CHROMA_VPS 32, 48
CHROMA_VPS 24, 32
CHROMA_VPS 24, 64
CHROMA_VPS 64, 16
CHROMA_VPS 64, 32
CHROMA_VPS 64, 48
CHROMA_VPS 64, 64
CHROMA_VPS 48, 64

// ***** chroma_vsp *****
// void interp_vert_sp_c(const int16_t* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VSP w, h
function x265_interp_4tap_vert_sp_\w\()x\h\()_neon
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
    FILTER_CHROMA_VSP  \w, \h, 0
1:
    FILTER_CHROMA_VSP  \w, \h, 1
2:
    FILTER_CHROMA_VSP  \w, \h, 2
3:
    FILTER_CHROMA_VSP  \w, \h, 3
4:
    FILTER_CHROMA_VSP  \w, \h, 4
5:
    FILTER_CHROMA_VSP  \w, \h, 5
6:
    FILTER_CHROMA_VSP  \w, \h, 6
7:
    FILTER_CHROMA_VSP  \w, \h, 7
endfunc
.endm

CHROMA_VSP 4, 4
CHROMA_VSP 4, 8
CHROMA_VSP 4, 16
CHROMA_VSP 4, 32
CHROMA_VSP 8, 2
CHROMA_VSP 8, 4
CHROMA_VSP 8, 6
CHROMA_VSP 8, 8
CHROMA_VSP 8, 16
CHROMA_VSP 8, 32
CHROMA_VSP 8, 12
CHROMA_VSP 8, 64
CHROMA_VSP 12, 16
CHROMA_VSP 12, 32
CHROMA_VSP 16, 4
CHROMA_VSP 16, 8
CHROMA_VSP 16, 12
CHROMA_VSP 16, 16
CHROMA_VSP 16, 32
CHROMA_VSP 16, 64
CHROMA_VSP 16, 24
CHROMA_VSP 32, 8
CHROMA_VSP 32, 16
CHROMA_VSP 32, 24
CHROMA_VSP 32, 32
CHROMA_VSP 32, 64
CHROMA_VSP 32, 48
CHROMA_VSP 24, 32
CHROMA_VSP 24, 64
CHROMA_VSP 64, 16
CHROMA_VSP 64, 32
CHROMA_VSP 64, 48
CHROMA_VSP 64, 64
CHROMA_VSP 48, 64

// ***** chroma_vss *****
// void interp_vert_ss_c(const int16_t* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_VSS w, h
function x265_interp_4tap_vert_ss_\w\()x\h\()_neon
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
    FILTER_CHROMA_VSS  \w, \h, 0
1:
    FILTER_CHROMA_VSS  \w, \h, 1
2:
    FILTER_CHROMA_VSS  \w, \h, 2
3:
    FILTER_CHROMA_VSS  \w, \h, 3
4:
    FILTER_CHROMA_VSS  \w, \h, 4
5:
    FILTER_CHROMA_VSS  \w, \h, 5
6:
    FILTER_CHROMA_VSS  \w, \h, 6
7:
    FILTER_CHROMA_VSS  \w, \h, 7
endfunc
.endm

CHROMA_VSS 4, 4
CHROMA_VSS 4, 8
CHROMA_VSS 4, 16
CHROMA_VSS 4, 32
CHROMA_VSS 8, 2
CHROMA_VSS 8, 4
CHROMA_VSS 8, 6
CHROMA_VSS 8, 8
CHROMA_VSS 8, 16
CHROMA_VSS 8, 32
CHROMA_VSS 8, 12
CHROMA_VSS 8, 64
CHROMA_VSS 12, 16
CHROMA_VSS 12, 32
CHROMA_VSS 16, 4
CHROMA_VSS 16, 8
CHROMA_VSS 16, 12
CHROMA_VSS 16, 16
CHROMA_VSS 16, 32
CHROMA_VSS 16, 64
CHROMA_VSS 16, 24
CHROMA_VSS 32, 8
CHROMA_VSS 32, 16
CHROMA_VSS 32, 24
CHROMA_VSS 32, 32
CHROMA_VSS 32, 64
CHROMA_VSS 32, 48
CHROMA_VSS 24, 32
CHROMA_VSS 24, 64
CHROMA_VSS 64, 16
CHROMA_VSS 64, 32
CHROMA_VSS 64, 48
CHROMA_VSS 64, 64
CHROMA_VSS 48, 64

// ***** chroma_hpp *****
// void interp_horiz_pp_c(const pixel* src, intptr_t srcStride, pixel* dst, intptr_t dstStride, int coeffIdx)
.macro CHROMA_HPP w, h
function x265_interp_4tap_horiz_pp_\w\()x\h\()_neon
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
    FILTER_CHROMA_HPP  \w, \h, 0
1:
    FILTER_CHROMA_HPP  \w, \h, 1
2:
    FILTER_CHROMA_HPP  \w, \h, 2
3:
    FILTER_CHROMA_HPP  \w, \h, 3
4:
    FILTER_CHROMA_HPP  \w, \h, 4
5:
    FILTER_CHROMA_HPP  \w, \h, 5
6:
    FILTER_CHROMA_HPP  \w, \h, 6
7:
    FILTER_CHROMA_HPP  \w, \h, 7
endfunc
.endm

CHROMA_HPP 2, 4
CHROMA_HPP 2, 8
CHROMA_HPP 2, 16
CHROMA_HPP 4, 2
CHROMA_HPP 4, 4
CHROMA_HPP 4, 8
CHROMA_HPP 4, 16
CHROMA_HPP 4, 32
CHROMA_HPP 6, 8
CHROMA_HPP 6, 16
CHROMA_HPP 8, 2
CHROMA_HPP 8, 4
CHROMA_HPP 8, 6
CHROMA_HPP 8, 8
CHROMA_HPP 8, 12
CHROMA_HPP 8, 16
CHROMA_HPP 8, 32
CHROMA_HPP 8, 64
CHROMA_HPP 12, 16
CHROMA_HPP 12, 32
CHROMA_HPP 16, 4
CHROMA_HPP 16, 8
CHROMA_HPP 16, 12
CHROMA_HPP 16, 16
CHROMA_HPP 16, 24
CHROMA_HPP 16, 32
CHROMA_HPP 16, 64
CHROMA_HPP 24, 32
CHROMA_HPP 24, 64
CHROMA_HPP 32, 8
CHROMA_HPP 32, 16
CHROMA_HPP 32, 24
CHROMA_HPP 32, 32
CHROMA_HPP 32, 48
CHROMA_HPP 32, 64
CHROMA_HPP 48, 64
CHROMA_HPP 64, 16
CHROMA_HPP 64, 32
CHROMA_HPP 64, 48
CHROMA_HPP 64, 64

// ***** chroma_hps *****
// void interp_horiz_ps_c(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride, int coeffIdx, int isRowExt)
.macro CHROMA_HPS w, h
function x265_interp_4tap_horiz_ps_\w\()x\h\()_neon
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
    FILTER_CHROMA_HPS  \w, \h, 0
1:
    FILTER_CHROMA_HPS  \w, \h, 1
2:
    FILTER_CHROMA_HPS  \w, \h, 2
3:
    FILTER_CHROMA_HPS  \w, \h, 3
4:
    FILTER_CHROMA_HPS  \w, \h, 4
5:
    FILTER_CHROMA_HPS  \w, \h, 5
6:
    FILTER_CHROMA_HPS  \w, \h, 6
7:
    FILTER_CHROMA_HPS  \w, \h, 7
endfunc
.endm

CHROMA_HPS 2, 4
CHROMA_HPS 2, 8
CHROMA_HPS 2, 16
CHROMA_HPS 4, 2
CHROMA_HPS 4, 4
CHROMA_HPS 4, 8
CHROMA_HPS 4, 16
CHROMA_HPS 4, 32
CHROMA_HPS 6, 8
CHROMA_HPS 6, 16
CHROMA_HPS 8, 2
CHROMA_HPS 8, 4
CHROMA_HPS 8, 6
CHROMA_HPS 8, 8
CHROMA_HPS 8, 12
CHROMA_HPS 8, 16
CHROMA_HPS 8, 32
CHROMA_HPS 8, 64
CHROMA_HPS 12, 16
CHROMA_HPS 12, 32
CHROMA_HPS 16, 4
CHROMA_HPS 16, 8
CHROMA_HPS 16, 12
CHROMA_HPS 16, 16
CHROMA_HPS 16, 24
CHROMA_HPS 16, 32
CHROMA_HPS 16, 64
CHROMA_HPS 24, 32
CHROMA_HPS 24, 64
CHROMA_HPS 32, 8
CHROMA_HPS 32, 16
CHROMA_HPS 32, 24
CHROMA_HPS 32, 32
CHROMA_HPS 32, 48
CHROMA_HPS 32, 64
CHROMA_HPS 48, 64
CHROMA_HPS 64, 16
CHROMA_HPS 64, 32
CHROMA_HPS 64, 48
CHROMA_HPS 64, 64

const g_luma_s16, align=8
//       a, b,   c,  d,  e,   f, g,  h
.hword   0, 0,   0, 64,  0,   0, 0,  0
.hword  -1, 4, -10, 58, 17,  -5, 1,  0
.hword  -1, 4, -11, 40, 40, -11, 4, -1
.hword   0, 1,  -5, 17, 58, -10, 4, -1
endconst
