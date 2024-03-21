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
#include "sad-a-common.S"

.arch armv8-a+sve2

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

.macro SAD_SVE2_16 h
    mov             z16.d, #0
    ptrue           p0.h, vl16
.rept \h
    ld1b            {z0.h}, p0/z, [x0]
    ld1b            {z2.h}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    uaba            z16.h, z0.h, z2.h
.endr
    uaddv           d0, p0, z16.h
    fmov            w0, s0
    ret
.endm

.macro SAD_SVE2_32 h
    ptrue           p0.b, vl32
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z4.b}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    uabalb          z16.h, z0.b, z4.b
    uabalt          z16.h, z0.b, z4.b
.endr
    uaddv           d0, p0, z16.h
    fmov            w0, s0
    ret
.endm

.macro SAD_SVE2_64 h
    cmp             x9, #48
    bgt             .vl_gt_48_pixel_sad_64x\h
    mov             z16.d, #0
    mov             z17.d, #0
    mov             z18.d, #0
    mov             z19.d, #0
    ptrue           p0.b, vl32
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z4.b}, p0/z, [x2]
    ld1b            {z5.b}, p0/z, [x2, #1, mul vl]
    add             x0, x0, x1
    add             x2, x2, x3
    uabalb          z16.h, z0.b, z4.b
    uabalt          z17.h, z0.b, z4.b
    uabalb          z18.h, z1.b, z5.b
    uabalt          z19.h, z1.b, z5.b
.endr
    add             z16.h, z16.h, z17.h
    add             z17.h, z18.h, z19.h
    add             z16.h, z16.h, z17.h
    uadalp          z24.s, p0/m, z16.h
    uaddv           d5, p0, z24.s
    fmov            x0, d5
    ret
.vl_gt_48_pixel_sad_64x\h\():
    mov             z16.d, #0
    mov             z17.d, #0
    mov             z24.d, #0
    ptrue           p0.b, vl64
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z4.b}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    uabalb          z16.h, z0.b, z4.b
    uabalt          z17.h, z0.b, z4.b
.endr
    add             z16.h, z16.h, z17.h
    uadalp          z24.s, p0/m, z16.h
    uaddv           d5, p0, z24.s
    fmov            x0, d5
    ret
.endm

.macro SAD_SVE2_24 h
    mov             z16.d, #0
    mov             x10, #24
    mov             x11, #0
    whilelt         p0.b, x11, x10
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z8.b}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    uabalb          z16.h, z0.b, z8.b
    uabalt          z16.h, z0.b, z8.b
.endr
    uaddv           d5, p0, z16.h
    fmov            w0, s5
    ret
.endm

.macro SAD_SVE2_48 h
    cmp             x9, #48
    bgt             .vl_gt_48_pixel_sad_48x\h
    mov             z16.d, #0
    mov             z17.d, #0
    mov             z18.d, #0
    mov             z19.d, #0
    ptrue           p0.b, vl32
    ptrue           p1.b, vl16
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p1/z, [x0, #1, mul vl]
    ld1b            {z8.b}, p0/z, [x2]
    ld1b            {z9.b}, p1/z, [x2, #1, mul vl]
    add             x0, x0, x1
    add             x2, x2, x3
    uabalb          z16.h, z0.b, z8.b
    uabalt          z17.h, z0.b, z8.b
    uabalb          z18.h, z1.b, z9.b
    uabalt          z19.h, z1.b, z9.b
.endr
    add             z16.h, z16.h, z17.h
    add             z17.h, z18.h, z19.h
    add             z16.h, z16.h, z17.h
    uaddv           d5, p0, z16.h
    fmov            w0, s5
    ret
.vl_gt_48_pixel_sad_48x\h\():
    mov             z16.d, #0
    mov             z17.d, #0
    mov             x10, #48
    mov             x11, #0
    whilelt         p0.b, x11, x10
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z8.b}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    uabalb          z16.h, z0.b, z8.b
    uabalt          z17.h, z0.b, z8.b
.endr
    add             z16.h, z16.h, z17.h
    uaddv           d5, p0, z16.h
    fmov            w0, s5
    ret
.endm

// Fully unrolled.
.macro SAD_FUNC_SVE2 w, h
function PFX(pixel_sad_\w\()x\h\()_sve2)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_sad_\w\()x\h
    SAD_START_\w uabdl
    SAD_\w \h
.if \w > 4
    add             v16.8h, v16.8h, v17.8h
.endif
    uaddlv          s0, v16.8h
    fmov            w0, s0
    ret
.vl_gt_16_pixel_sad_\w\()x\h\():
.if \w == 4 || \w == 8 || \w == 12
    SAD_START_\w uabdl
    SAD_\w \h
.if \w > 4
    add             v16.8h, v16.8h, v17.8h
.endif
    uaddlv          s0, v16.8h
    fmov            w0, s0
    ret
.else
    SAD_SVE2_\w \h
.endif
endfunc
.endm

// Loop unrolled 4.
.macro SAD_FUNC_LOOP_SVE2 w, h
function PFX(pixel_sad_\w\()x\h\()_sve2)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_sad_loop_\w\()x\h
    SAD_START_\w

    mov             w9, #\h/8
.loop_sve2_\w\()x\h:
    sub             w9, w9, #1
.rept 4
    SAD_\w
.endr
    cbnz            w9, .loop_sve2_\w\()x\h

    SAD_END_\w

.vl_gt_16_pixel_sad_loop_\w\()x\h\():
.if \w == 4 || \w == 8 || \w == 12
    SAD_START_\w

    mov             w9, #\h/8
.loop_sve2_loop_\w\()x\h:
    sub             w9, w9, #1
.rept 4
    SAD_\w
.endr
    cbnz            w9, .loop_sve2_loop_\w\()x\h

    SAD_END_\w
.else
    SAD_SVE2_\w \h
.endif
endfunc
.endm

SAD_FUNC_SVE2  4,  4
SAD_FUNC_SVE2  4,  8
SAD_FUNC_SVE2  4,  16
SAD_FUNC_SVE2  8,  4
SAD_FUNC_SVE2  8,  8
SAD_FUNC_SVE2  8,  16
SAD_FUNC_SVE2  8,  32
SAD_FUNC_SVE2  16, 4
SAD_FUNC_SVE2  16, 8
SAD_FUNC_SVE2  16, 12
SAD_FUNC_SVE2  16, 16
SAD_FUNC_SVE2  16, 32
SAD_FUNC_SVE2  16, 64

SAD_FUNC_LOOP_SVE2  32, 8
SAD_FUNC_LOOP_SVE2  32, 16
SAD_FUNC_LOOP_SVE2  32, 24
SAD_FUNC_LOOP_SVE2  32, 32
SAD_FUNC_LOOP_SVE2  32, 64
SAD_FUNC_LOOP_SVE2  64, 16
SAD_FUNC_LOOP_SVE2  64, 32
SAD_FUNC_LOOP_SVE2  64, 48
SAD_FUNC_LOOP_SVE2  64, 64
SAD_FUNC_LOOP_SVE2  12, 16
SAD_FUNC_LOOP_SVE2  24, 32
SAD_FUNC_LOOP_SVE2  48, 64

// SAD_X3 and SAD_X4 code start

.macro SAD_X_SVE2_24_INNER_GT_16 base z
    ld1b            {z4.b}, p0/z, [ \base ]
    add             \base, \base, x5
    uabalb          \z\().h, z4.b, z0.b
    uabalt          \z\().h, z4.b, z0.b
.endm

.macro SAD_X_SVE2_24 h x
    mov             z20.d, #0
    mov             z21.d, #0
    mov             z22.d, #0
    mov             z23.d, #0
    mov             x10, #24
    mov             x11, #0
    whilelt         p0.b, x11, x10
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    add             x0, x0, x9
    SAD_X_SVE2_24_INNER_GT_16 x1, z20
    SAD_X_SVE2_24_INNER_GT_16 x2, z21
    SAD_X_SVE2_24_INNER_GT_16 x3, z22
.if \x == 4
    SAD_X_SVE2_24_INNER_GT_16 x4, z23
.endif
.endr
    uaddlv          s0, v20.8h
    uaddlv          s1, v21.8h
    uaddlv          s2, v22.8h
    stp             s0, s1, [x6]
.if \x == 3
    str             s2, [x6, #8]
.elseif \x == 4
    uaddv           d0, p0, z20.h
    uaddv           d1, p0, z21.h
    uaddv           d2, p0, z22.h
    stp             s2, s3, [x6, #8]
.endif
    ret
.endm

.macro SAD_X_SVE2_32_INNER_GT_16 base z
    ld1b            {z4.b}, p0/z, [ \base ]
    add             \base, \base, x5
    uabalb          \z\().h, z4.b, z0.b
    uabalt          \z\().h, z4.b, z0.b
.endm

.macro SAD_X_SVE2_32 h x
    mov             z20.d, #0
    mov             z21.d, #0
    mov             z22.d, #0
    mov             z23.d, #0
    ptrue           p0.b, vl32
.rept \h
    ld1b            {z0.b}, p0/z, [x0]
    add             x0, x0, x9
    SAD_X_SVE2_32_INNER_GT_16 x1, z20
    SAD_X_SVE2_32_INNER_GT_16 x2, z21
    SAD_X_SVE2_32_INNER_GT_16 x3, z22
.if \x == 4
    SAD_X_SVE2_32_INNER_GT_16 x4, z23
.endif
.endr
    uaddv           d0, p0, z20.h
    uaddv           d1, p0, z21.h
    uaddv           d2, p0, z22.h
    stp             s0, s1, [x6]
.if \x == 3
    str             s2, [x6, #8]
.elseif \x == 4
    uaddv           d3, p0, z23.h
    stp             s2, s3, [x6, #8]
.endif
    ret
.endm

// static void x264_pixel_sad_x3_##size(pixel *fenc, pixel *pix0, pixel *pix1, pixel *pix2, intptr_t i_stride, int scores[3])
// static void x264_pixel_sad_x4_##size(pixel *fenc, pixel *pix0, pixel *pix1,pixel *pix2, pixel *pix3, intptr_t i_stride, int scores[4])
.macro SAD_X_FUNC_SVE2 x, w, h
function PFX(sad_x\x\()_\w\()x\h\()_sve2)
    mov             x9, #FENC_STRIDE

// Make function arguments for x == 3 look like x == 4.
.if \x == 3
    mov             x6, x5
    mov             x5, x4
.endif
    rdvl            x11, #1
    cmp             x11, #16
    bgt             .vl_gt_16_sad_x\x\()_\w\()x\h
.if \w == 12
    movrel          x12, sad12_mask
    ld1             {v31.16b}, [x12]
.endif

    SAD_X_START_\w \h, \x, uabdl
    SAD_X_\w \h, \x
    SAD_X_END_\w \x
.vl_gt_16_sad_x\x\()_\w\()x\h\():
.if \w == 24 || \w == 32
    SAD_X_SVE2_\w \h, \x
.else
.if \w == 12
    movrel          x12, sad12_mask
    ld1             {v31.16b}, [x12]
.endif

    SAD_X_START_\w \h, \x, uabdl
    SAD_X_\w \h, \x
    SAD_X_END_\w \x
.endif
endfunc
.endm

.macro SAD_X_LOOP_SVE2 x, w, h
function PFX(sad_x\x\()_\w\()x\h\()_sve2)
    mov             x9, #FENC_STRIDE

// Make function arguments for x == 3 look like x == 4.
.if \x == 3
    mov             x6, x5
    mov             x5, x4
.endif
    rdvl            x11, #1
    cmp             x11, #16
    bgt             .vl_gt_16_sad_x_loop_\x\()_\w\()x\h
    SAD_X_START_\w \x
    mov             w12, #\h/4
.loop_sad_sve2_x\x\()_\w\()x\h:
    sub             w12, w12, #1
 .rept 4
  .if \w == 24
    ld1             {v6.16b}, [x0], #16
    ld1             {v7.8b}, [x0], x9
  .elseif \w == 32
    ld1             {v6.16b-v7.16b}, [x0], x9
  .elseif \w == 48
    ld1             {v4.16b-v6.16b}, [x0], x9
  .elseif \w == 64
    ld1             {v4.16b-v7.16b}, [x0], x9
  .endif
    SAD_X_\w x1, v16, v20
    SAD_X_\w x2, v17, v21
    SAD_X_\w x3, v18, v22
  .if \x == 4
    SAD_X_\w x4, v19, v23
  .endif
 .endr
    cbnz            w12, .loop_sad_sve2_x\x\()_\w\()x\h
    SAD_X_END_\w \x
.vl_gt_16_sad_x_loop_\x\()_\w\()x\h\():
.if \w == 24 || \w == 32
    SAD_X_SVE2_\w \h, \x
    ret
.else
    SAD_X_START_\w \x
    mov             w12, #\h/4
.loop_sad_sve2_gt_16_x\x\()_\w\()x\h:
    sub             w12, w12, #1
 .rept 4
  .if \w == 24
    ld1             {v6.16b}, [x0], #16
    ld1             {v7.8b}, [x0], x9
  .elseif \w == 32
    ld1             {v6.16b-v7.16b}, [x0], x9
  .elseif \w == 48
    ld1             {v4.16b-v6.16b}, [x0], x9
  .elseif \w == 64
    ld1             {v4.16b-v7.16b}, [x0], x9
  .endif
    SAD_X_\w x1, v16, v20
    SAD_X_\w x2, v17, v21
    SAD_X_\w x3, v18, v22
  .if \x == 4
    SAD_X_\w x4, v19, v23
  .endif
 .endr
    cbnz            w12, .loop_sad_sve2_gt_16_x\x\()_\w\()x\h
    SAD_X_END_\w \x
.endif
endfunc
.endm


SAD_X_FUNC_SVE2  3, 4,  4
SAD_X_FUNC_SVE2  3, 4,  8
SAD_X_FUNC_SVE2  3, 4,  16
SAD_X_FUNC_SVE2  3, 8,  4
SAD_X_FUNC_SVE2  3, 8,  8
SAD_X_FUNC_SVE2  3, 8,  16
SAD_X_FUNC_SVE2  3, 8,  32
SAD_X_FUNC_SVE2  3, 12, 16
SAD_X_FUNC_SVE2  3, 16, 4
SAD_X_FUNC_SVE2  3, 16, 8
SAD_X_FUNC_SVE2  3, 16, 12
SAD_X_FUNC_SVE2  3, 16, 16
SAD_X_FUNC_SVE2  3, 16, 32
SAD_X_FUNC_SVE2  3, 16, 64
SAD_X_LOOP_SVE2  3, 24, 32
SAD_X_LOOP_SVE2  3, 32, 8
SAD_X_LOOP_SVE2  3, 32, 16
SAD_X_LOOP_SVE2  3, 32, 24
SAD_X_LOOP_SVE2  3, 32, 32
SAD_X_LOOP_SVE2  3, 32, 64
SAD_X_LOOP_SVE2  3, 48, 64
SAD_X_LOOP_SVE2  3, 64, 16
SAD_X_LOOP_SVE2  3, 64, 32
SAD_X_LOOP_SVE2  3, 64, 48
SAD_X_LOOP_SVE2  3, 64, 64

SAD_X_FUNC_SVE2  4, 4,  4
SAD_X_FUNC_SVE2  4, 4,  8
SAD_X_FUNC_SVE2  4, 4,  16
SAD_X_FUNC_SVE2  4, 8,  4
SAD_X_FUNC_SVE2  4, 8,  8
SAD_X_FUNC_SVE2  4, 8,  16
SAD_X_FUNC_SVE2  4, 8,  32
SAD_X_FUNC_SVE2  4, 12, 16
SAD_X_FUNC_SVE2  4, 16, 4
SAD_X_FUNC_SVE2  4, 16, 8
SAD_X_FUNC_SVE2  4, 16, 12
SAD_X_FUNC_SVE2  4, 16, 16
SAD_X_FUNC_SVE2  4, 16, 32
SAD_X_FUNC_SVE2  4, 16, 64
SAD_X_LOOP_SVE2  4, 24, 32
SAD_X_LOOP_SVE2  4, 32, 8
SAD_X_LOOP_SVE2  4, 32, 16
SAD_X_LOOP_SVE2  4, 32, 24
SAD_X_LOOP_SVE2  4, 32, 32
SAD_X_LOOP_SVE2  4, 32, 64
SAD_X_LOOP_SVE2  4, 48, 64
SAD_X_LOOP_SVE2  4, 64, 16
SAD_X_LOOP_SVE2  4, 64, 32
SAD_X_LOOP_SVE2  4, 64, 48
SAD_X_LOOP_SVE2  4, 64, 64
