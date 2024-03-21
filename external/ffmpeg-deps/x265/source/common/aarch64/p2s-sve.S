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

#include "asm-sve.S"
#include "p2s-common.S"

.arch armv8-a+sve

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

#if HIGH_BIT_DEPTH
# if BIT_DEPTH == 10
#  define P2S_SHIFT 4
# elif BIT_DEPTH == 12
#  define P2S_SHIFT 2
# endif

.macro p2s_start_sve
    add             x3, x3, x3
    add             x1, x1, x1
    mov             z31.h, #0xe0, lsl #8
.endm

#else // if !HIGH_BIT_DEPTH
# define P2S_SHIFT 6
.macro p2s_start_sve
    add             x3, x3, x3
    mov             z31.h, #0xe0, lsl #8
.endm

#endif // HIGH_BIT_DEPTH

// filterPixelToShort(const pixel* src, intptr_t srcStride, int16_t* dst, intptr_t dstStride)
.macro p2s_2xN_sve h
function PFX(filterPixelToShort_2x\h\()_sve)
    p2s_start_sve
.rept \h / 2
    p2s_2x2
.endr
    ret
endfunc
.endm

p2s_2xN_sve 4
p2s_2xN_sve 8
p2s_2xN_sve 16

.macro p2s_6xN_sve h
function PFX(filterPixelToShort_6x\h\()_sve)
    p2s_start_sve
    sub             x3, x3, #8
#if HIGH_BIT_DEPTH
    sub             x1, x1, #8
#endif
.rept \h / 2
    p2s_6x2
.endr
    ret
endfunc
.endm

p2s_6xN_sve 8
p2s_6xN_sve 16

function PFX(filterPixelToShort_4x2_sve)
    p2s_start_sve
#if HIGH_BIT_DEPTH
    ptrue           p0.h, vl8
    index           z1.d, #0, x1
    index           z2.d, #0, x3
    ld1d            {z3.d}, p0/z, [x0, z1.d]
    lsl             z3.h, p0/m, z3.h, #P2S_SHIFT
    add             z3.h, p0/m, z3.h, z31.h
    st1d            {z3.d}, p0, [x2, z2.d]
#else
    ptrue           p0.h, vl4
    ld1b            {z0.h}, p0/z, [x0]
    add             x0, x0, x1
    ld1b            {z1.h}, p0/z, [x0]
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    st1h            {z0.h}, p0, [x2]
    add             x2, x2, x3
    st1h            {z1.h}, p0, [x2]
#endif
    ret
endfunc


.macro p2s_8xN_sve h
function PFX(filterPixelToShort_8x\h\()_sve)
    p2s_start_sve
    ptrue           p0.h, vl8
.rept \h
#if HIGH_BIT_DEPTH
    ld1d            {z0.d}, p0/z, [x0]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    st1h            {z0.h}, p0, [x2]
    add             x2, x2, x3
#else
    ld1b            {z0.h}, p0/z, [x0]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    st1h            {z0.h}, p0, [x2]
    add             x2, x2, x3
#endif
.endr
    ret
endfunc
.endm

p2s_8xN_sve 2

.macro p2s_32xN_sve h
function PFX(filterPixelToShort_32x\h\()_sve)
#if HIGH_BIT_DEPTH
    p2s_start_sve
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_filterPixelToShort_high_32x\h
    ptrue           p0.h, vl8
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    ld1h            {z2.h}, p0/z, [x0, #2, mul vl]
    ld1h            {z3.h}, p0/z, [x0, #3, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    lsl             z2.h, p0/m, z2.h, #P2S_SHIFT
    lsl             z3.h, p0/m, z3.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    add             z2.h, p0/m, z2.h, z31.h
    add             z3.h, p0/m, z3.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x2, #2, mul vl]
    st1h            {z3.h}, p0, [x2, #3, mul vl]
    add             x2, x2, x3
.endr
    ret
.vl_gt_16_filterPixelToShort_high_32x\h\():
    cmp             x9, #48
    bgt             .vl_gt_48_filterPixelToShort_high_32x\h
    ptrue           p0.h, vl16
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    add             x2, x2, x3
.endr
    ret
.vl_gt_48_filterPixelToShort_high_32x\h\():
    ptrue           p0.h, vl32
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    st1h            {z0.h}, p0, [x2]
    add             x2, x2, x3
.endr
    ret
#else
    p2s_start
    mov             x9, #\h
.loop_filter_sve_P2S_32x\h:
    sub             x9, x9, #1
    ld1             {v0.16b-v1.16b}, [x0], x1
    ushll           v22.8h, v0.8b,  #P2S_SHIFT
    ushll2          v23.8h, v0.16b, #P2S_SHIFT
    ushll           v24.8h, v1.8b,  #P2S_SHIFT
    ushll2          v25.8h, v1.16b, #P2S_SHIFT
    add             v22.8h, v22.8h, v31.8h
    add             v23.8h, v23.8h, v31.8h
    add             v24.8h, v24.8h, v31.8h
    add             v25.8h, v25.8h, v31.8h
    st1             {v22.16b-v25.16b}, [x2], x3
    cbnz            x9, .loop_filter_sve_P2S_32x\h
    ret
#endif
endfunc
.endm

p2s_32xN_sve 8
p2s_32xN_sve 16
p2s_32xN_sve 24
p2s_32xN_sve 32
p2s_32xN_sve 48
p2s_32xN_sve 64

.macro p2s_64xN_sve h
function PFX(filterPixelToShort_64x\h\()_sve)
#if HIGH_BIT_DEPTH
    p2s_start_sve
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_filterPixelToShort_high_64x\h
    ptrue           p0.h, vl8
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    ld1h            {z2.h}, p0/z, [x0, #2, mul vl]
    ld1h            {z3.h}, p0/z, [x0, #3, mul vl]
    ld1h            {z4.h}, p0/z, [x0, #4, mul vl]
    ld1h            {z5.h}, p0/z, [x0, #5, mul vl]
    ld1h            {z6.h}, p0/z, [x0, #6, mul vl]
    ld1h            {z7.h}, p0/z, [x0, #7, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    lsl             z2.h, p0/m, z2.h, #P2S_SHIFT
    lsl             z3.h, p0/m, z3.h, #P2S_SHIFT
    lsl             z4.h, p0/m, z4.h, #P2S_SHIFT
    lsl             z5.h, p0/m, z5.h, #P2S_SHIFT
    lsl             z6.h, p0/m, z6.h, #P2S_SHIFT
    lsl             z7.h, p0/m, z7.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    add             z2.h, p0/m, z2.h, z31.h
    add             z3.h, p0/m, z3.h, z31.h
    add             z4.h, p0/m, z4.h, z31.h
    add             z5.h, p0/m, z5.h, z31.h
    add             z6.h, p0/m, z6.h, z31.h
    add             z7.h, p0/m, z7.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x2, #2, mul vl]
    st1h            {z3.h}, p0, [x2, #3, mul vl]
    st1h            {z4.h}, p0, [x2, #4, mul vl]
    st1h            {z5.h}, p0, [x2, #5, mul vl]
    st1h            {z6.h}, p0, [x2, #6, mul vl]
    st1h            {z7.h}, p0, [x2, #7, mul vl]
    add             x2, x2, x3
.endr
    ret
.vl_gt_16_filterPixelToShort_high_64x\h\():
    cmp             x9, #48
    bgt             .vl_gt_48_filterPixelToShort_high_64x\h
    ptrue           p0.h, vl16
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    ld1h            {z2.h}, p0/z, [x0, #2, mul vl]
    ld1h            {z3.h}, p0/z, [x0, #3, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    lsl             z2.h, p0/m, z2.h, #P2S_SHIFT
    lsl             z3.h, p0/m, z3.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    add             z2.h, p0/m, z2.h, z31.h
    add             z3.h, p0/m, z3.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x2, #2, mul vl]
    st1h            {z3.h}, p0, [x2, #3, mul vl]
    add             x2, x2, x3
.endr
    ret
.vl_gt_48_filterPixelToShort_high_64x\h\():
    cmp             x9, #112
    bgt             .vl_gt_112_filterPixelToShort_high_64x\h
    ptrue           p0.h, vl32
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    add             x2, x2, x3
.endr
    ret
.vl_gt_112_filterPixelToShort_high_64x\h\():
    ptrue           p0.h, vl64
.rept \h
    ld1h            {z0.h}, p0/z, [x0]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    st1h            {z0.h}, p0, [x2]
    add             x2, x2, x3
.endr
    ret
#else
    p2s_start
    sub             x3, x3, #64
    mov             x9, #\h
.loop_filter_sve_P2S_64x\h:
    sub             x9, x9, #1
    ld1             {v0.16b-v3.16b}, [x0], x1
    ushll           v16.8h, v0.8b,  #P2S_SHIFT
    ushll2          v17.8h, v0.16b, #P2S_SHIFT
    ushll           v18.8h, v1.8b,  #P2S_SHIFT
    ushll2          v19.8h, v1.16b, #P2S_SHIFT
    ushll           v20.8h, v2.8b,  #P2S_SHIFT
    ushll2          v21.8h, v2.16b, #P2S_SHIFT
    ushll           v22.8h, v3.8b,  #P2S_SHIFT
    ushll2          v23.8h, v3.16b, #P2S_SHIFT
    add             v16.8h, v16.8h, v31.8h
    add             v17.8h, v17.8h, v31.8h
    add             v18.8h, v18.8h, v31.8h
    add             v19.8h, v19.8h, v31.8h
    add             v20.8h, v20.8h, v31.8h
    add             v21.8h, v21.8h, v31.8h
    add             v22.8h, v22.8h, v31.8h
    add             v23.8h, v23.8h, v31.8h
    st1             {v16.16b-v19.16b}, [x2], #64
    st1             {v20.16b-v23.16b}, [x2], x3
    cbnz            x9, .loop_filter_sve_P2S_64x\h
    ret
#endif
endfunc
.endm

p2s_64xN_sve 16
p2s_64xN_sve 32
p2s_64xN_sve 48
p2s_64xN_sve 64

function PFX(filterPixelToShort_48x64_sve)
#if HIGH_BIT_DEPTH
    p2s_start_sve
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_filterPixelToShort_high_48x64
    ptrue           p0.h, vl8
.rept 64
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    ld1h            {z2.h}, p0/z, [x0, #2, mul vl]
    ld1h            {z3.h}, p0/z, [x0, #3, mul vl]
    ld1h            {z4.h}, p0/z, [x0, #4, mul vl]
    ld1h            {z5.h}, p0/z, [x0, #5, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    lsl             z2.h, p0/m, z2.h, #P2S_SHIFT
    lsl             z3.h, p0/m, z3.h, #P2S_SHIFT
    lsl             z4.h, p0/m, z4.h, #P2S_SHIFT
    lsl             z5.h, p0/m, z5.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    add             z2.h, p0/m, z2.h, z31.h
    add             z3.h, p0/m, z3.h, z31.h
    add             z4.h, p0/m, z4.h, z31.h
    add             z5.h, p0/m, z5.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x2, #2, mul vl]
    st1h            {z3.h}, p0, [x2, #3, mul vl]
    st1h            {z4.h}, p0, [x2, #4, mul vl]
    st1h            {z5.h}, p0, [x2, #5, mul vl]
    add             x2, x2, x3
.endr
    ret
.vl_gt_16_filterPixelToShort_high_48x64:
    ptrue           p0.h, vl16
.rept 64
    ld1h            {z0.h}, p0/z, [x0]
    ld1h            {z1.h}, p0/z, [x0, #1, mul vl]
    ld1h            {z2.h}, p0/z, [x0, #2, mul vl]
    add             x0, x0, x1
    lsl             z0.h, p0/m, z0.h, #P2S_SHIFT
    lsl             z1.h, p0/m, z1.h, #P2S_SHIFT
    lsl             z2.h, p0/m, z2.h, #P2S_SHIFT
    add             z0.h, p0/m, z0.h, z31.h
    add             z1.h, p0/m, z1.h, z31.h
    add             z2.h, p0/m, z2.h, z31.h
    st1h            {z0.h}, p0, [x2]
    st1h            {z1.h}, p0, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x2, #2, mul vl]
    add             x2, x2, x3
.endr
    ret
#else
    p2s_start
    sub             x3, x3, #64
    mov             x9, #64
.loop_filterP2S_sve_48x64:
    sub            x9, x9, #1
    ld1             {v0.16b-v2.16b}, [x0], x1
    ushll           v16.8h, v0.8b,  #P2S_SHIFT
    ushll2          v17.8h, v0.16b, #P2S_SHIFT
    ushll           v18.8h, v1.8b,  #P2S_SHIFT
    ushll2          v19.8h, v1.16b, #P2S_SHIFT
    ushll           v20.8h, v2.8b,  #P2S_SHIFT
    ushll2          v21.8h, v2.16b, #P2S_SHIFT
    add             v16.8h, v16.8h, v31.8h
    add             v17.8h, v17.8h, v31.8h
    add             v18.8h, v18.8h, v31.8h
    add             v19.8h, v19.8h, v31.8h
    add             v20.8h, v20.8h, v31.8h
    add             v21.8h, v21.8h, v31.8h
    st1             {v16.16b-v19.16b}, [x2], #64
    st1             {v20.16b-v21.16b}, [x2], x3
    cbnz            x9, .loop_filterP2S_sve_48x64
    ret
#endif
endfunc
