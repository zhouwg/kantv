/*****************************************************************************
 * Copyright (C) 2020-2021 MulticoreWare, Inc
 *
 * Authors: Hongbin Liu <liuhongbin1@huawei.com>
 *          Sebastian Pop <spop@amazon.com>
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

#include "asm.S"
#include "sad-a-common.S"

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

// Fully unrolled.
.macro SAD_FUNC w, h
function PFX(pixel_sad_\w\()x\h\()_neon)
    SAD_START_\w uabdl
    SAD_\w \h
.if \w > 4
    add             v16.8h, v16.8h, v17.8h
.endif
    uaddlv          s0, v16.8h
    fmov            w0, s0
    ret
endfunc
.endm

// Loop unrolled 4.
.macro SAD_FUNC_LOOP w, h
function PFX(pixel_sad_\w\()x\h\()_neon)
    SAD_START_\w

    mov             w9, #\h/8
.loop_\w\()x\h:
    sub             w9, w9, #1
.rept 4
    SAD_\w
.endr
    cbnz            w9, .loop_\w\()x\h

    SAD_END_\w
endfunc
.endm

SAD_FUNC  4,  4
SAD_FUNC  4,  8
SAD_FUNC  4,  16
SAD_FUNC  8,  4
SAD_FUNC  8,  8
SAD_FUNC  8,  16
SAD_FUNC  8,  32
SAD_FUNC  16, 4
SAD_FUNC  16, 8
SAD_FUNC  16, 12
SAD_FUNC  16, 16
SAD_FUNC  16, 32
SAD_FUNC  16, 64

SAD_FUNC_LOOP  32, 8
SAD_FUNC_LOOP  32, 16
SAD_FUNC_LOOP  32, 24
SAD_FUNC_LOOP  32, 32
SAD_FUNC_LOOP  32, 64
SAD_FUNC_LOOP  64, 16
SAD_FUNC_LOOP  64, 32
SAD_FUNC_LOOP  64, 48
SAD_FUNC_LOOP  64, 64
SAD_FUNC_LOOP  12, 16
SAD_FUNC_LOOP  24, 32
SAD_FUNC_LOOP  48, 64

// SAD_X3 and SAD_X4 code start

// static void x264_pixel_sad_x3_##size(pixel *fenc, pixel *pix0, pixel *pix1, pixel *pix2, intptr_t i_stride, int scores[3])
// static void x264_pixel_sad_x4_##size(pixel *fenc, pixel *pix0, pixel *pix1,pixel *pix2, pixel *pix3, intptr_t i_stride, int scores[4])
.macro SAD_X_FUNC x, w, h
function PFX(sad_x\x\()_\w\()x\h\()_neon)
    mov             x9, #FENC_STRIDE

// Make function arguments for x == 3 look like x == 4.
.if \x == 3
    mov             x6, x5
    mov             x5, x4
.endif

.if \w == 12
    movrel          x12, sad12_mask
    ld1             {v31.16b}, [x12]
.endif

    SAD_X_START_\w \h, \x, uabdl
    SAD_X_\w \h, \x
    SAD_X_END_\w \x
endfunc
.endm

.macro SAD_X_LOOP x, w, h
function PFX(sad_x\x\()_\w\()x\h\()_neon)
    mov             x9, #FENC_STRIDE

// Make function arguments for x == 3 look like x == 4.
.if \x == 3
    mov             x6, x5
    mov             x5, x4
.endif
    SAD_X_START_\w \x
    mov             w12, #\h/4
.loop_sad_x\x\()_\w\()x\h:
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
    cbnz            w12, .loop_sad_x\x\()_\w\()x\h
    SAD_X_END_\w \x
endfunc
.endm


SAD_X_FUNC  3, 4,  4
SAD_X_FUNC  3, 4,  8
SAD_X_FUNC  3, 4,  16
SAD_X_FUNC  3, 8,  4
SAD_X_FUNC  3, 8,  8
SAD_X_FUNC  3, 8,  16
SAD_X_FUNC  3, 8,  32
SAD_X_FUNC  3, 12, 16
SAD_X_FUNC  3, 16, 4
SAD_X_FUNC  3, 16, 8
SAD_X_FUNC  3, 16, 12
SAD_X_FUNC  3, 16, 16
SAD_X_FUNC  3, 16, 32
SAD_X_FUNC  3, 16, 64
SAD_X_LOOP  3, 24, 32
SAD_X_LOOP  3, 32, 8
SAD_X_LOOP  3, 32, 16
SAD_X_LOOP  3, 32, 24
SAD_X_LOOP  3, 32, 32
SAD_X_LOOP  3, 32, 64
SAD_X_LOOP  3, 48, 64
SAD_X_LOOP  3, 64, 16
SAD_X_LOOP  3, 64, 32
SAD_X_LOOP  3, 64, 48
SAD_X_LOOP  3, 64, 64

SAD_X_FUNC  4, 4,  4
SAD_X_FUNC  4, 4,  8
SAD_X_FUNC  4, 4,  16
SAD_X_FUNC  4, 8,  4
SAD_X_FUNC  4, 8,  8
SAD_X_FUNC  4, 8,  16
SAD_X_FUNC  4, 8,  32
SAD_X_FUNC  4, 12, 16
SAD_X_FUNC  4, 16, 4
SAD_X_FUNC  4, 16, 8
SAD_X_FUNC  4, 16, 12
SAD_X_FUNC  4, 16, 16
SAD_X_FUNC  4, 16, 32
SAD_X_FUNC  4, 16, 64
SAD_X_LOOP  4, 24, 32
SAD_X_LOOP  4, 32, 8
SAD_X_LOOP  4, 32, 16
SAD_X_LOOP  4, 32, 24
SAD_X_LOOP  4, 32, 32
SAD_X_LOOP  4, 32, 64
SAD_X_LOOP  4, 48, 64
SAD_X_LOOP  4, 64, 16
SAD_X_LOOP  4, 64, 32
SAD_X_LOOP  4, 64, 48
SAD_X_LOOP  4, 64, 64
