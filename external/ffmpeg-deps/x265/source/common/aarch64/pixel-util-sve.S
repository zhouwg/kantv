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
#include "pixel-util-common.S"

.arch armv8-a+sve

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

function PFX(pixel_sub_ps_8x16_sve)
    lsl             x1, x1, #1
    ptrue           p0.h, vl8
.rept 8
    ld1b            {z0.h}, p0/z, [x2]
    ld1b            {z1.h}, p0/z, [x3]
    add             x2, x2, x4
    add             x3, x3, x5
    ld1b            {z2.h}, p0/z, [x2]
    ld1b            {z3.h}, p0/z, [x3]
    add             x2, x2, x4
    add             x3, x3, x5
    sub             z4.h, z0.h, z1.h
    sub             z5.h, z2.h, z3.h
    st1             {v4.8h}, [x0], x1
    st1             {v5.8h}, [x0], x1
.endr
    ret
endfunc

//******* satd *******
.macro satd_4x4_sve
    ld1b            {z0.h}, p0/z, [x0]
    ld1b            {z2.h}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    ld1b            {z1.h}, p0/z, [x0]
    ld1b            {z3.h}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    ld1b            {z4.h}, p0/z, [x0]
    ld1b            {z6.h}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    ld1b            {z5.h}, p0/z, [x0]
    ld1b            {z7.h}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3

    sub             z0.h, z0.h, z2.h
    sub             z1.h, z1.h, z3.h
    sub             z2.h, z4.h, z6.h
    sub             z3.h, z5.h, z7.h

    add             z4.h, z0.h, z2.h
    add             z5.h, z1.h, z3.h
    sub             z6.h, z0.h, z2.h
    sub             z7.h, z1.h, z3.h

    add             z0.h, z4.h, z5.h
    sub             z1.h, z4.h, z5.h

    add             z2.h, z6.h, z7.h
    sub             z3.h, z6.h, z7.h

    trn1            z4.h, z0.h, z2.h
    trn2            z5.h, z0.h, z2.h

    trn1            z6.h, z1.h, z3.h
    trn2            z7.h, z1.h, z3.h

    add             z0.h, z4.h, z5.h
    sub             z1.h, z4.h, z5.h

    add             z2.h, z6.h, z7.h
    sub             z3.h, z6.h, z7.h

    trn1            z4.s, z0.s, z1.s
    trn2            z5.s, z0.s, z1.s

    trn1            z6.s, z2.s, z3.s
    trn2            z7.s, z2.s, z3.s

    abs             z4.h, p0/m, z4.h
    abs             z5.h, p0/m, z5.h
    abs             z6.h, p0/m, z6.h
    abs             z7.h, p0/m, z7.h

    smax            z4.h, p0/m, z4.h, z5.h
    smax            z6.h, p0/m, z6.h, z7.h

    add             z0.h, z4.h, z6.h

    uaddlp          v0.2s, v0.4h
    uaddlp          v0.1d, v0.2s
.endm

// int satd_4x4(const pixel* pix1, intptr_t stride_pix1, const pixel* pix2, intptr_t stride_pix2)
function PFX(pixel_satd_4x4_sve)
    ptrue           p0.h, vl4
    satd_4x4_sve
    fmov            x0, d0
    ret
endfunc

function PFX(pixel_satd_8x4_sve)
    ptrue           p0.h, vl4
    mov             x4, x0
    mov             x5, x2
    satd_4x4_sve
    add             x0, x4, #4
    add             x2, x5, #4
    umov            x6, v0.d[0]
    satd_4x4_sve
    umov            x0, v0.d[0]
    add             x0, x0, x6
    ret
endfunc

function PFX(pixel_satd_8x12_sve)
    ptrue           p0.h, vl4
    mov             x4, x0
    mov             x5, x2
    mov             x7, #0
    satd_4x4_sve
    umov            x6, v0.d[0]
    add             x7, x7, x6
    add             x0, x4, #4
    add             x2, x5, #4
    satd_4x4_sve
    umov            x6, v0.d[0]
    add             x7, x7, x6
.rept 2
    sub             x0, x0, #4
    sub             x2, x2, #4
    mov             x4, x0
    mov             x5, x2
    satd_4x4_sve
    umov            x6, v0.d[0]
    add             x7, x7, x6
    add             x0, x4, #4
    add             x2, x5, #4
    satd_4x4_sve
    umov            x6, v0.d[0]
    add             x7, x7, x6
.endr
    mov             x0, x7
    ret
endfunc

.macro LOAD_DIFF_16x4_sve v0 v1 v2 v3 v4 v5 v6 v7
    mov             x11, #8 // in order to consider CPUs whose vector size is greater than 128 bits
    ld1b            {z0.h}, p0/z, [x0]
    ld1b            {z1.h}, p0/z, [x0, x11]
    ld1b            {z2.h}, p0/z, [x2]
    ld1b            {z3.h}, p0/z, [x2, x11]
    add             x0, x0, x1
    add             x2, x2, x3
    ld1b            {z4.h}, p0/z, [x0]
    ld1b            {z5.h}, p0/z, [x0, x11]
    ld1b            {z6.h}, p0/z, [x2]
    ld1b            {z7.h}, p0/z, [x2, x11]
    add             x0, x0, x1
    add             x2, x2, x3
    ld1b            {z29.h}, p0/z, [x0]
    ld1b            {z9.h}, p0/z, [x0, x11]
    ld1b            {z10.h}, p0/z, [x2]
    ld1b            {z11.h}, p0/z, [x2, x11]
    add             x0, x0, x1
    add             x2, x2, x3
    ld1b            {z12.h}, p0/z, [x0]
    ld1b            {z13.h}, p0/z, [x0, x11]
    ld1b            {z14.h}, p0/z, [x2]
    ld1b            {z15.h}, p0/z, [x2, x11]
    add             x0, x0, x1
    add             x2, x2, x3

    sub             \v0\().h, z0.h, z2.h
    sub             \v4\().h, z1.h, z3.h
    sub             \v1\().h, z4.h, z6.h
    sub             \v5\().h, z5.h, z7.h
    sub             \v2\().h, z29.h, z10.h
    sub             \v6\().h, z9.h, z11.h
    sub             \v3\().h, z12.h, z14.h
    sub             \v7\().h, z13.h, z15.h
.endm

// one vertical hadamard pass and two horizontal
function PFX(satd_8x4v_8x8h_sve), export=0
    HADAMARD4_V     z16.h, z18.h, z17.h, z19.h, z0.h, z2.h, z1.h, z3.h
    HADAMARD4_V     z20.h, z21.h, z22.h, z23.h, z0.h, z1.h, z2.h, z3.h
    trn4            z0.h, z1.h, z2.h, z3.h, z16.h, z17.h, z18.h, z19.h
    trn4            z4.h, z5.h, z6.h, z7.h, z20.h, z21.h, z22.h, z23.h
    SUMSUB_ABCD     z16.h, z17.h, z18.h, z19.h, z0.h, z1.h, z2.h, z3.h
    SUMSUB_ABCD     z20.h, z21.h, z22.h, z23.h, z4.h, z5.h, z6.h, z7.h
    trn4            z0.s, z2.s, z1.s, z3.s, z16.s, z18.s, z17.s, z19.s
    trn4            z4.s, z6.s, z5.s, z7.s, z20.s, z22.s, z21.s, z23.s
    ABS8_SVE        z0.h, z1.h, z2.h, z3.h, z4.h, z5.h, z6.h, z7.h, p0
    smax            z0.h, p0/m, z0.h, z2.h
    smax            z1.h, p0/m, z1.h, z3.h
    smax            z4.h, p0/m, z4.h, z6.h
    smax            z5.h, p0/m, z5.h, z7.h
    ret
endfunc

function PFX(satd_16x4_sve), export=0
    LOAD_DIFF_16x4_sve  z16, z17, z18, z19, z20, z21, z22, z23
    b                    PFX(satd_8x4v_8x8h_sve)
endfunc

.macro pixel_satd_32x8_sve
    mov             x4, x0
    mov             x5, x2
.rept 2
    bl              PFX(satd_16x4_sve)
    add             z30.h, z30.h, z0.h
    add             z31.h, z31.h, z1.h
    add             z30.h, z30.h, z4.h
    add             z31.h, z31.h, z5.h
.endr
    add             x0, x4, #16
    add             x2, x5, #16
.rept 2
    bl              PFX(satd_16x4_sve)
    add             z30.h, z30.h, z0.h
    add             z31.h, z31.h, z1.h
    add             z30.h, z30.h, z4.h
    add             z31.h, z31.h, z5.h
.endr
.endm

.macro satd_32x16_sve
    movi            v30.2d, #0
    movi            v31.2d, #0
    pixel_satd_32x8_sve
    sub             x0, x0, #16
    sub             x2, x2, #16
    pixel_satd_32x8_sve
    add             z0.h, z30.h, z31.h
    uaddlv          s0, v0.8h
    mov             w6, v0.s[0]
.endm

function PFX(pixel_satd_32x16_sve)
    ptrue           p0.h, vl8
    mov             x10, x30
    satd_32x16_sve
    mov             x0, x6
    ret             x10
endfunc

function PFX(pixel_satd_32x32_sve)
    ptrue           p0.h, vl8
    mov             x10, x30
    mov             x7, #0
    satd_32x16_sve
    sub             x0, x0, #16
    sub             x2, x2, #16
    add             x7, x7, x6
    satd_32x16_sve
    add             x0, x7, x6
    ret             x10
endfunc

.macro satd_64x16_sve
    mov             x8, x0
    mov             x9, x2
    satd_32x16_sve
    add             x7, x7, x6
    add             x0, x8, #32
    add             x2, x9, #32
    satd_32x16_sve
    add             x7, x7, x6
.endm

function PFX(pixel_satd_64x48_sve)
    ptrue           p0.h, vl8
    mov             x10, x30
    mov             x7, #0
.rept 2
    satd_64x16_sve
    sub             x0, x0, #48
    sub             x2, x2, #48
.endr
    satd_64x16_sve
    mov             x0, x7
    ret             x10
endfunc

/********* ssim ***********/
// uint32_t quant_c(const int16_t* coef, const int32_t* quantCoeff, int32_t* deltaU, int16_t* qCoef, int qBits, int add, int numCoeff)
// No need to fully use sve instructions for this function
function PFX(quant_sve)
    mov             w9, #1
    lsl             w9, w9, w4
    mov             z0.s, w9
    neg             w9, w4
    mov             z1.s, w9
    add             w9, w9, #8
    mov             z2.s, w9
    mov             z3.s, w5

    lsr             w6, w6, #2
    eor             z4.d, z4.d, z4.d
    eor             w10, w10, w10
    eor             z17.d, z17.d, z17.d

.loop_quant_sve:
    ld1             {v18.4h}, [x0], #8
    ld1             {v7.4s}, [x1], #16
    sxtl            v6.4s, v18.4h

    cmlt            v5.4s, v6.4s, #0

    abs             v6.4s, v6.4s


    mul             v6.4s, v6.4s, v7.4s

    add             v7.4s, v6.4s, v3.4s
    sshl            v7.4s, v7.4s, v1.4s

    mls             v6.4s, v7.4s, v0.s[0]
    sshl            v16.4s, v6.4s, v2.4s
    st1             {v16.4s}, [x2], #16

    // numsig
    cmeq            v16.4s, v7.4s, v17.4s
    add             v4.4s, v4.4s, v16.4s
    add             w10, w10, #4

    // level *= sign
    eor             z16.d, z7.d, z5.d
    sub             v16.4s, v16.4s, v5.4s
    sqxtn           v5.4h, v16.4s
    st1             {v5.4h}, [x3], #8

    subs            w6, w6, #1
    b.ne             .loop_quant_sve

    addv            s4, v4.4s
    mov             w9, v4.s[0]
    add             w0, w10, w9
    ret
endfunc
