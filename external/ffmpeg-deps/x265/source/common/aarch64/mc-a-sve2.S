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
#include "mc-a-common.S"

.arch armv8-a+sve2

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

function PFX(pixel_avg_pp_12x16_sve2)
    sub             x1, x1, #4
    sub             x3, x3, #4
    sub             x5, x5, #4
    ptrue           p0.s, vl1
    ptrue           p1.b, vl8
    mov             x11, #4
.rept 16
    ld1w            {z0.s}, p0/z, [x2]
    ld1b            {z1.b}, p1/z, [x2, x11]
    ld1w            {z2.s}, p0/z, [x4]
    ld1b            {z3.b}, p1/z, [x4, x11]
    add             x2, x2, #4
    add             x2, x2, x3
    add             x4, x4, #4
    add             x4, x4, x5
    urhadd          z0.b, p1/m, z0.b, z2.b
    urhadd          z1.b, p1/m, z1.b, z3.b
    st1b            {z0.b}, p1, [x0]
    st1b            {z1.b}, p1, [x0, x11]
    add             x0, x0, #4
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(pixel_avg_pp_24x32_sve2)
    mov             w12, #4
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_avg_pp_24x32
    sub             x1, x1, #16
    sub             x3, x3, #16
    sub             x5, x5, #16
.lpavg_24x32_sve2:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b}, [x2], #16
    ld1             {v1.8b}, [x2], x3
    ld1             {v2.16b}, [x4], #16
    ld1             {v3.8b}, [x4], x5
    urhadd          v0.16b, v0.16b, v2.16b
    urhadd          v1.8b, v1.8b, v3.8b
    st1             {v0.16b}, [x0], #16
    st1             {v1.8b}, [x0], x1
.endr
    cbnz            w12, .lpavg_24x32_sve2
    ret
.vl_gt_16_pixel_avg_pp_24x32:
    mov             x10, #24
    mov             x11, #0
    whilelt         p0.b, x11, x10
.vl_gt_16_loop_pixel_avg_pp_24x32:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z2.b}, p0/z, [x4]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    st1b            {z0.b}, p0, [x0]
    add             x0, x0, x1
.endr
    cbnz            w12, .vl_gt_16_loop_pixel_avg_pp_24x32
    ret
endfunc

.macro pixel_avg_pp_32xN_sve2 h
function PFX(pixel_avg_pp_32x\h\()_sve2)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_avg_pp_32_\h
.rept \h
    ld1             {v0.16b-v1.16b}, [x2], x3
    ld1             {v2.16b-v3.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v2.16b
    urhadd          v1.16b, v1.16b, v3.16b
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    ret
.vl_gt_16_pixel_avg_pp_32_\h:
    ptrue           p0.b, vl32
.rept \h
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z2.b}, p0/z, [x4]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    st1b            {z0.b}, p0, [x0]
    add             x0, x0, x1
.endr
    ret
endfunc
.endm

pixel_avg_pp_32xN_sve2 8
pixel_avg_pp_32xN_sve2 16
pixel_avg_pp_32xN_sve2 24

.macro pixel_avg_pp_32xN1_sve2 h
function PFX(pixel_avg_pp_32x\h\()_sve2)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_avg_pp_32xN1_\h
    mov             w12, #\h / 8
.lpavg_sve2_32x\h\():
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v1.16b}, [x2], x3
    ld1             {v2.16b-v3.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v2.16b
    urhadd          v1.16b, v1.16b, v3.16b
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_sve2_32x\h
    ret
.vl_gt_16_pixel_avg_pp_32xN1_\h:
    ptrue           p0.b, vl32
    mov             w12, #\h / 8
.eq_32_loop_pixel_avg_pp_32xN1_\h\():
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z2.b}, p0/z, [x4]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    st1b            {z0.b}, p0, [x0]
    add             x0, x0, x1
.endr
    cbnz            w12, .eq_32_loop_pixel_avg_pp_32xN1_\h
    ret
endfunc
.endm

pixel_avg_pp_32xN1_sve2 32
pixel_avg_pp_32xN1_sve2 64

function PFX(pixel_avg_pp_48x64_sve2)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_avg_pp_48x64
    mov             w12, #8
.lpavg_48x64_sve2:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v2.16b}, [x2], x3
    ld1             {v3.16b-v5.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v3.16b
    urhadd          v1.16b, v1.16b, v4.16b
    urhadd          v2.16b, v2.16b, v5.16b
    st1             {v0.16b-v2.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_48x64_sve2
    ret
.vl_gt_16_pixel_avg_pp_48x64:
    cmp             x9, #32
    bgt             .vl_gt_32_pixel_avg_pp_48x64
    ptrue           p0.b, vl32
    ptrue           p1.b, vl16
    mov             w12, #8
.vl_eq_32_pixel_avg_pp_48x64:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z1.b}, p1/z, [x2, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x4]
    ld1b            {z3.b}, p1/z, [x4, #1, mul vl]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    urhadd          z1.b, p1/m, z1.b, z3.b
    st1b            {z0.b}, p0, [x0]
    st1b            {z1.b}, p1, [x0, #1, mul vl]
    add             x0, x0, x1
.endr
    cbnz            w12, .vl_eq_32_pixel_avg_pp_48x64
    ret
.vl_gt_32_pixel_avg_pp_48x64:
    mov             x10, #48
    mov             x11, #0
    whilelt         p0.b, x11, x10
    mov             w12, #8
.loop_gt_32_pixel_avg_pp_48x64:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z2.b}, p0/z, [x4]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    st1b            {z0.b}, p0, [x0]
    add             x0, x0, x1
.endr
    cbnz            w12, .loop_gt_32_pixel_avg_pp_48x64
    ret
endfunc

.macro pixel_avg_pp_64xN_sve2 h
function PFX(pixel_avg_pp_64x\h\()_sve2)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_pixel_avg_pp_64x\h
    mov             w12, #\h / 4
.lpavg_sve2_64x\h\():
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v3.16b}, [x2], x3
    ld1             {v4.16b-v7.16b}, [x4], x5
    urhadd          v0.16b, v0.16b, v4.16b
    urhadd          v1.16b, v1.16b, v5.16b
    urhadd          v2.16b, v2.16b, v6.16b
    urhadd          v3.16b, v3.16b, v7.16b
    st1             {v0.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .lpavg_sve2_64x\h
    ret
.vl_gt_16_pixel_avg_pp_64x\h\():
    cmp             x9, #48
    bgt             .vl_gt_48_pixel_avg_pp_64x\h
    ptrue           p0.b, vl32
    mov             w12, #\h / 4
.vl_eq_32_pixel_avg_pp_64x\h\():
    sub             w12, w12, #1
.rept 4
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z1.b}, p0/z, [x2, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x4]
    ld1b            {z3.b}, p0/z, [x4, #1, mul vl]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    urhadd          z1.b, p0/m, z1.b, z3.b
    st1b            {z0.b}, p0, [x0]
    st1b            {z1.b}, p0, [x0, #1, mul vl]
    add             x0, x0, x1
.endr
    cbnz            w12, .vl_eq_32_pixel_avg_pp_64x\h
    ret
.vl_gt_48_pixel_avg_pp_64x\h\():
    ptrue           p0.b, vl64
    mov             w12, #\h / 4
.vl_eq_64_pixel_avg_pp_64x\h\():
    sub             w12, w12, #1
.rept 4
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z2.b}, p0/z, [x4]
    add             x2, x2, x3
    add             x4, x4, x5
    urhadd          z0.b, p0/m, z0.b, z2.b
    st1b            {z0.b}, p0, [x0]
    add             x0, x0, x1
.endr
    cbnz            w12, .vl_eq_64_pixel_avg_pp_64x\h
    ret
endfunc
.endm

pixel_avg_pp_64xN_sve2 16
pixel_avg_pp_64xN_sve2 32
pixel_avg_pp_64xN_sve2 48
pixel_avg_pp_64xN_sve2 64

// void addAvg(const int16_t* src0, const int16_t* src1, pixel* dst, intptr_t src0Stride, intptr_t src1Stride, intptr_t dstStride)

.macro addAvg_2xN_sve2 h
function PFX(addAvg_2x\h\()_sve2)
    ptrue           p0.s, vl2
    ptrue           p1.h, vl4
    ptrue           p2.h, vl2
.rept \h / 2
    ld1rw           {z0.s}, p0/z, [x0]
    ld1rw           {z1.s}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    ld1rw           {z2.s}, p0/z, [x0]
    ld1rw           {z3.s}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p1/m, z0.h, z1.h
    add             z2.h, p1/m, z2.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p2, [x2]
    add             x2, x2, x5
    st1b            {z2.h}, p2, [x2]
    add             x2, x2, x5
.endr
    ret
endfunc
.endm

addAvg_2xN_sve2 4
addAvg_2xN_sve2 8
addAvg_2xN_sve2 16

.macro addAvg_6xN_sve2 h
function PFX(addAvg_6x\h\()_sve2)
    mov             w12, #\h / 2
    ptrue           p0.b, vl16
    ptrue           p2.h, vl6
.loop_sve2_addavg_6x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    ld1b            {z2.b}, p0/z, [x0]
    ld1b            {z3.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    add             z2.h, p0/m, z2.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    sqrshrnb        z2.b, z2.h, #7
    add             z0.b, z0.b, #0x80
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p2, [x2]
    add             x2, x2, x5
    st1b            {z2.h}, p2, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_sve2_addavg_6x\h
    ret
endfunc
.endm

addAvg_6xN_sve2 8
addAvg_6xN_sve2 16

.macro addAvg_8xN_sve2 h
function PFX(addAvg_8x\h\()_sve2)
    ptrue           p0.b, vl16
.rept \h / 2
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    ld1b            {z2.b}, p0/z, [x0]
    ld1b            {z3.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    add             z2.h, p0/m, z2.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    st1b            {z2.h}, p0, [x2]
    add             x2, x2, x5
.endr
    ret
endfunc
.endm

.macro addAvg_8xN1_sve2 h
function PFX(addAvg_8x\h\()_sve2)
    mov             w12, #\h / 2
    ptrue           p0.b, vl16
.loop_sve2_addavg_8x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    ld1b            {z2.b}, p0/z, [x0]
    ld1b            {z3.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    add             z2.h, p0/m, z2.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    st1b            {z2.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_sve2_addavg_8x\h
    ret
endfunc
.endm

addAvg_8xN_sve2 2
addAvg_8xN_sve2 4
addAvg_8xN_sve2 6
addAvg_8xN_sve2 8
addAvg_8xN_sve2 12
addAvg_8xN_sve2 16
addAvg_8xN1_sve2 32
addAvg_8xN1_sve2 64

.macro addAvg_12xN_sve2 h
function PFX(addAvg_12x\h\()_sve2)
    mov             w12, #\h
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_addAvg_12x\h
    ptrue           p0.b, vl16
    ptrue           p1.b, vl8
.loop_sve2_addavg_12x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    ld1b            {z2.b}, p1/z, [x0, #1, mul vl]
    ld1b            {z3.b}, p1/z, [x1, #1, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    add             z2.h, p1/m, z2.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z2.h}, p1, [x2, #1, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_sve2_addavg_12x\h
    ret
.vl_gt_16_addAvg_12x\h\():
    mov             x10, #24
    mov             x11, #0
    whilelt         p0.b, x11, x10
.loop_sve2_gt_16_addavg_12x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_sve2_gt_16_addavg_12x\h
    ret
endfunc
.endm

addAvg_12xN_sve2 16
addAvg_12xN_sve2 32

.macro addAvg_16xN_sve2 h
function PFX(addAvg_16x\h\()_sve2)
    mov             w12, #\h
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_addAvg_16x\h
    ptrue           p0.b, vl16
.loop_eq_16_sve2_addavg_16x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    ld1b            {z2.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z3.b}, p0/z, [x1, #1, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    add             z2.h, p0/m, z2.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z2.h}, p0, [x2, #1, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_eq_16_sve2_addavg_16x\h
    ret
.vl_gt_16_addAvg_16x\h\():
    cmp             x9, #32
    bgt             .vl_gt_32_addAvg_16x\h
    ptrue           p0.b, vl32
.loop_gt_16_sve2_addavg_16x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p1, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_16_sve2_addavg_16x\h
    ret
.vl_gt_32_addAvg_16x\h\():
    mov             x10, #48
    mov             x11, #0
    whilelt         p0.b, x11, x10
.loop_gt_32_sve2_addavg_16x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_32_sve2_addavg_16x\h
    ret
endfunc
.endm

addAvg_16xN_sve2 4
addAvg_16xN_sve2 8
addAvg_16xN_sve2 12
addAvg_16xN_sve2 16
addAvg_16xN_sve2 24
addAvg_16xN_sve2 32
addAvg_16xN_sve2 64

.macro addAvg_24xN_sve2 h
function PFX(addAvg_24x\h\()_sve2)
    mov             w12, #\h
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_addAvg_24x\h
    addAvg_start
.loop_eq_16_sve2_addavg_24x\h\():
    sub             w12, w12, #1
    ld1             {v0.16b-v2.16b}, [x0], x3
    ld1             {v3.16b-v5.16b}, [x1], x4
    addavg_1        v0, v3
    addavg_1        v1, v4
    addavg_1        v2, v5
    sqxtun          v0.8b, v0.8h
    sqxtun          v1.8b, v1.8h
    sqxtun          v2.8b, v2.8h
    st1             {v0.8b-v2.8b}, [x2], x5
    cbnz            w12, .loop_eq_16_sve2_addavg_24x\h
    ret
.vl_gt_16_addAvg_24x\h\():
    cmp             x9, #48
    bgt             .vl_gt_48_addAvg_24x\h
    ptrue           p0.b, vl32
    ptrue           p1.b, vl16
.loop_gt_16_sve2_addavg_24x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p1/z, [x0, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x1]
    ld1b            {z3.b}, p1/z, [x1, #1, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z2.h
    add             z1.h, p1/m, z1.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z1.b, z1.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p1, [x2, #1, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_16_sve2_addavg_24x\h
    ret
.vl_gt_48_addAvg_24x\h\():
    mov             x10, #48
    mov             x11, #0
    whilelt         p0.b, x11, x10
.loop_gt_48_sve2_addavg_24x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z2.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z2.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_48_sve2_addavg_24x\h
    ret
endfunc
.endm

addAvg_24xN_sve2 32
addAvg_24xN_sve2 64

.macro addAvg_32xN_sve2 h
function PFX(addAvg_32x\h\()_sve2)
    mov             w12, #\h
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_addAvg_32x\h
    ptrue           p0.b, vl16
.loop_eq_16_sve2_addavg_32x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x0, #2, mul vl]
    ld1b            {z3.b}, p0/z, [x0, #3, mul vl]
    ld1b            {z4.b}, p0/z, [x1]
    ld1b            {z5.b}, p0/z, [x1, #1, mul vl]
    ld1b            {z6.b}, p0/z, [x1, #2, mul vl]
    ld1b            {z7.b}, p0/z, [x1, #3, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    add             z1.h, p0/m, z1.h, z5.h
    add             z2.h, p0/m, z2.h, z6.h
    add             z3.h, p0/m, z3.h, z7.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z1.b, z1.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    sqrshrnb        z3.b, z3.h, #7
    add             z3.b, z3.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p0, [x2, #1, mul vl]
    st1b            {z2.h}, p0, [x2, #2, mul vl]
    st1b            {z3.h}, p0, [x2, #3, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_eq_16_sve2_addavg_32x\h
    ret
.vl_gt_16_addAvg_32x\h\():
    cmp             x9, #48
    bgt             .vl_gt_48_addAvg_32x\h
    ptrue           p0.b, vl32
.loop_gt_eq_32_sve2_addavg_32x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x1]
    ld1b            {z3.b}, p0/z, [x1, #1, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z2.h
    add             z1.h, p0/m, z1.h, z3.h
    sqrshrnb        z0.b, z0.h, #7
    add             z1.b, z1.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p0, [x2, #1, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_eq_32_sve2_addavg_32x\h
    ret
.vl_gt_48_addAvg_32x\h\():
    ptrue           p0.b, vl64
.loop_eq_64_sve2_addavg_32x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z1.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_eq_64_sve2_addavg_32x\h
    ret
endfunc
.endm

addAvg_32xN_sve2 8
addAvg_32xN_sve2 16
addAvg_32xN_sve2 24
addAvg_32xN_sve2 32
addAvg_32xN_sve2 48
addAvg_32xN_sve2 64

function PFX(addAvg_48x64_sve2)
    mov             w12, #64
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_addAvg_48x64
    addAvg_start
    sub             x3, x3, #64
    sub             x4, x4, #64
.loop_eq_16_sve2_addavg_48x64:
    sub             w12, w12, #1
    ld1             {v0.8h-v3.8h}, [x0], #64
    ld1             {v4.8h-v7.8h}, [x1], #64
    ld1             {v20.8h-v21.8h}, [x0], x3
    ld1             {v22.8h-v23.8h}, [x1], x4
    addavg_1        v0, v4
    addavg_1        v1, v5
    addavg_1        v2, v6
    addavg_1        v3, v7
    addavg_1        v20, v22
    addavg_1        v21, v23
    sqxtun          v0.8b, v0.8h
    sqxtun2         v0.16b, v1.8h
    sqxtun          v1.8b, v2.8h
    sqxtun2         v1.16b, v3.8h
    sqxtun          v2.8b, v20.8h
    sqxtun2         v2.16b, v21.8h
    st1             {v0.16b-v2.16b}, [x2], x5
    cbnz            w12, .loop_eq_16_sve2_addavg_48x64
    ret
.vl_gt_16_addAvg_48x64:
    cmp             x9, #48
    bgt             .vl_gt_48_addAvg_48x64
    ptrue           p0.b, vl32
.loop_gt_eq_32_sve2_addavg_48x64:
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x0, #2, mul vl]
    ld1b            {z4.b}, p0/z, [x1]
    ld1b            {z5.b}, p0/z, [x1, #1, mul vl]
    ld1b            {z6.b}, p0/z, [x1, #2, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    add             z1.h, p0/m, z1.h, z5.h
    add             z2.h, p0/m, z2.h, z6.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z1.b, z1.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p0, [x2, #1, mul vl]
    st1b            {z2.h}, p0, [x2, #2, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_eq_32_sve2_addavg_48x64
    ret
.vl_gt_48_addAvg_48x64:
    cmp             x9, #112
    bgt             .vl_gt_112_addAvg_48x64
    ptrue           p0.b, vl64
    ptrue           p1.b, vl32
.loop_gt_48_sve2_addavg_48x64:
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p1/z, [x0, #1, mul vl]
    ld1b            {z4.b}, p0/z, [x1]
    ld1b            {z5.b}, p1/z, [x1, #1, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    add             z1.h, p1/m, z1.h, z5.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z1.b, z1.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p1, [x2, #1, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_48_sve2_addavg_48x64
    ret
.vl_gt_112_addAvg_48x64:
    mov             x10, #96
    mov             x11, #0
    whilelt         p0.b, x11, x10
.loop_gt_112_sve2_addavg_48x64:
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z4.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_112_sve2_addavg_48x64
    ret
endfunc

.macro addAvg_64xN_sve2 h
function PFX(addAvg_64x\h\()_sve2)
    mov             w12, #\h
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_addAvg_64x\h
    addAvg_start
    sub             x3, x3, #64
    sub             x4, x4, #64
.loop_eq_16_sve2_addavg_64x\h\():
    sub             w12, w12, #1
    ld1             {v0.8h-v3.8h}, [x0], #64
    ld1             {v4.8h-v7.8h}, [x1], #64
    ld1             {v20.8h-v23.8h}, [x0], x3
    ld1             {v24.8h-v27.8h}, [x1], x4
    addavg_1        v0, v4
    addavg_1        v1, v5
    addavg_1        v2, v6
    addavg_1        v3, v7
    addavg_1        v20, v24
    addavg_1        v21, v25
    addavg_1        v22, v26
    addavg_1        v23, v27
    sqxtun          v0.8b, v0.8h
    sqxtun2         v0.16b, v1.8h
    sqxtun          v1.8b, v2.8h
    sqxtun2         v1.16b, v3.8h
    sqxtun          v2.8b, v20.8h
    sqxtun2         v2.16b, v21.8h
    sqxtun          v3.8b, v22.8h
    sqxtun2         v3.16b, v23.8h
    st1             {v0.16b-v3.16b}, [x2], x5
    cbnz            w12, .loop_eq_16_sve2_addavg_64x\h
    ret
.vl_gt_16_addAvg_64x\h\():
    cmp             x9, #48
    bgt             .vl_gt_48_addAvg_64x\h
    ptrue           p0.b, vl32
.loop_gt_eq_32_sve2_addavg_64x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x0, #2, mul vl]
    ld1b            {z3.b}, p0/z, [x0, #3, mul vl]
    ld1b            {z4.b}, p0/z, [x1]
    ld1b            {z5.b}, p0/z, [x1, #1, mul vl]
    ld1b            {z6.b}, p0/z, [x1, #2, mul vl]
    ld1b            {z7.b}, p0/z, [x1, #3, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    add             z1.h, p0/m, z1.h, z5.h
    add             z2.h, p0/m, z2.h, z6.h
    add             z3.h, p0/m, z3.h, z7.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z1.b, z1.b, #0x80
    sqrshrnb        z2.b, z2.h, #7
    add             z2.b, z2.b, #0x80
    sqrshrnb        z3.b, z3.h, #7
    add             z3.b, z3.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p0, [x2, #1, mul vl]
    st1b            {z2.h}, p0, [x2, #2, mul vl]
    st1b            {z3.h}, p0, [x2, #3, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_eq_32_sve2_addavg_64x\h
    ret
.vl_gt_48_addAvg_64x\h\():
    cmp             x9, #112
    bgt             .vl_gt_112_addAvg_64x\h
    ptrue           p0.b, vl64
.loop_gt_eq_48_sve2_addavg_64x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z1.b}, p0/z, [x0, #1, mul vl]
    ld1b            {z4.b}, p0/z, [x1]
    ld1b            {z5.b}, p0/z, [x1, #1, mul vl]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    add             z1.h, p0/m, z1.h, z5.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    sqrshrnb        z1.b, z1.h, #7
    add             z1.b, z1.b, #0x80
    st1b            {z0.h}, p0, [x2]
    st1b            {z1.h}, p0, [x2, #1, mul vl]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_eq_48_sve2_addavg_64x\h
    ret
.vl_gt_112_addAvg_64x\h\():
    ptrue           p0.b, vl128
.loop_gt_eq_128_sve2_addavg_64x\h\():
    sub             w12, w12, #1
    ld1b            {z0.b}, p0/z, [x0]
    ld1b            {z4.b}, p0/z, [x1]
    add             x0, x0, x3, lsl #1
    add             x1, x1, x4, lsl #1
    add             z0.h, p0/m, z0.h, z4.h
    sqrshrnb        z0.b, z0.h, #7
    add             z0.b, z0.b, #0x80
    st1b            {z0.h}, p0, [x2]
    add             x2, x2, x5
    cbnz            w12, .loop_gt_eq_128_sve2_addavg_64x\h
    ret
endfunc
.endm

addAvg_64xN_sve2 16
addAvg_64xN_sve2 32
addAvg_64xN_sve2 48
addAvg_64xN_sve2 64
