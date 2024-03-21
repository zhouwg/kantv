/*****************************************************************************
 * Copyright (C) 2022-2023 MulticoreWare, Inc
 *
 * Authors: David Chen <david.chen@myais.com.cn>
 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
 *
 * This program is also available under a commercial proprietary license.
 * For more information, contact us at license @ x265.com.
 *****************************************************************************/

#include "asm-sve.S"
#include "blockcopy8-common.S"

.arch armv8-a+sve

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

/* void blockcopy_sp(pixel* a, intptr_t stridea, const int16_t* b, intptr_t strideb)
 *
 * r0   - a
 * r1   - stridea
 * r2   - b
 * r3   - strideb */

function PFX(blockcopy_sp_4x4_sve)
    ptrue           p0.h, vl4
.rept 2
    ld1h            {z0.h}, p0/z, [x2]
    add             x2, x2, x3, lsl #1
    st1b            {z0.h}, p0, [x0]
    add             x0, x0, x1
    ld1h            {z1.h}, p0/z, [x2]
    add             x2, x2, x3, lsl #1
    st1b            {z1.h}, p0, [x0]
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_8x8_sve)
    ptrue           p0.h, vl8
.rept 4
    ld1h            {z0.h}, p0/z, [x2]
    add             x2, x2, x3, lsl #1
    st1b            {z0.h}, p0, [x0]
    add            x0, x0, x1
    ld1h            {z1.h}, p0/z, [x2]
    add             x2, x2, x3, lsl #1
    st1b            {z1.h}, p0, [x0]
    add            x0, x0, x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_16x16_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_sp_16_16
    lsl             x3, x3, #1
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.rept 8
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    st1             {v0.16b}, [x0], x1
    st1             {v1.16b}, [x0], x1
.endr
    ret
.vl_gt_16_blockcopy_sp_16_16:
    ptrue           p0.h, vl16
.rept 8
    ld1h            {z0.h}, p0/z, [x2]
    st1b            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
    ld1h            {z1.h}, p0/z, [x2]
    st1b            {z1.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_32x32_sve)
    mov             w12, #4
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_sp_32_32
    lsl             x3, x3, #1
    movrel          x11, xtn_xtn2_table
    ld1             {v31.16b}, [x11]
.loop_csp32_sve:
    sub             w12, w12, #1
.rept 4
    ld1             {v0.8h-v3.8h}, [x2], x3
    ld1             {v4.8h-v7.8h}, [x2], x3
    tbl             v0.16b, {v0.16b,v1.16b}, v31.16b
    tbl             v1.16b, {v2.16b,v3.16b}, v31.16b
    tbl             v2.16b, {v4.16b,v5.16b}, v31.16b
    tbl             v3.16b, {v6.16b,v7.16b}, v31.16b
    st1             {v0.16b-v1.16b}, [x0], x1
    st1             {v2.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_csp32_sve
    ret
.vl_gt_16_blockcopy_sp_32_32:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_sp_32_32
    ptrue           p0.h, vl16
.vl_gt_16_loop_csp32_sve:
    sub             w12, w12, #1
.rept 4
    ld1h            {z0.h}, p0/z, [x2]
    ld1h            {z1.h}, p0/z, [x2, #1, mul vl]
    st1b            {z0.h}, p0, [x0]
    st1b            {z1.h}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
    ld1h            {z2.h}, p0/z, [x2]
    ld1h            {z3.h}, p0/z, [x2, #1, mul vl]
    st1b            {z2.h}, p0, [x0]
    st1b            {z3.h}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    cbnz            w12, .vl_gt_16_loop_csp32_sve
    ret
.vl_gt_48_blockcopy_sp_32_32:
    ptrue           p0.h, vl32
.vl_gt_48_loop_csp32_sve:
    sub             w12, w12, #1
.rept 4
    ld1h            {z0.h}, p0/z, [x2]
    st1b            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
    ld1h            {z1.h}, p0/z, [x2]
    st1b            {z1.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    cbnz            w12, .vl_gt_48_loop_csp32_sve
    ret
endfunc

function PFX(blockcopy_ps_16x16_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ps_16_16
    lsl             x1, x1, #1
.rept 8
    ld1             {v4.16b}, [x2], x3
    ld1             {v5.16b}, [x2], x3
    uxtl            v0.8h, v4.8b
    uxtl2           v1.8h, v4.16b
    uxtl            v2.8h, v5.8b
    uxtl2           v3.8h, v5.16b
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
.vl_gt_16_blockcopy_ps_16_16:
    ptrue           p0.b, vl32
.rept 16
    ld1b            {z1.h}, p0/z, [x2]
    st1h            {z1.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
endfunc

function PFX(blockcopy_ps_32x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ps_32_32
    lsl             x1, x1, #1
    mov             w12, #4
.loop_cps32_sve:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v17.16b}, [x2], x3
    ld1             {v18.16b-v19.16b}, [x2], x3
    uxtl            v0.8h, v16.8b
    uxtl2           v1.8h, v16.16b
    uxtl            v2.8h, v17.8b
    uxtl2           v3.8h, v17.16b
    uxtl            v4.8h, v18.8b
    uxtl2           v5.8h, v18.16b
    uxtl            v6.8h, v19.8b
    uxtl2           v7.8h, v19.16b
    st1             {v0.8h-v3.8h}, [x0], x1
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_cps32_sve
    ret
.vl_gt_16_blockcopy_ps_32_32:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_ps_32_32
    ptrue           p0.b, vl32
.rept 32
    ld1b            {z2.h}, p0/z, [x2]
    ld1b            {z3.h}, p0/z, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x0]
    st1h            {z3.h}, p0, [x0, #1, mul vl]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
.vl_gt_48_blockcopy_ps_32_32:
    ptrue           p0.b, vl64
.rept 32
    ld1b            {z2.h}, p0/z, [x2]
    st1h            {z2.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
endfunc

function PFX(blockcopy_ps_64x64_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ps_64_64
    lsl             x1, x1, #1
    sub             x1, x1, #64
    mov             w12, #16
.loop_cps64_sve:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v19.16b}, [x2], x3
    uxtl            v0.8h, v16.8b
    uxtl2           v1.8h, v16.16b
    uxtl            v2.8h, v17.8b
    uxtl2           v3.8h, v17.16b
    uxtl            v4.8h, v18.8b
    uxtl2           v5.8h, v18.16b
    uxtl            v6.8h, v19.8b
    uxtl2           v7.8h, v19.16b
    st1             {v0.8h-v3.8h}, [x0], #64
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_cps64_sve
    ret
.vl_gt_16_blockcopy_ps_64_64:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_ps_64_64
    ptrue           p0.b, vl32
.rept 64
    ld1b            {z4.h}, p0/z, [x2]
    ld1b            {z5.h}, p0/z, [x2, #1, mul vl]
    ld1b            {z6.h}, p0/z, [x2, #2, mul vl]
    ld1b            {z7.h}, p0/z, [x2, #3, mul vl]
    st1h            {z4.h}, p0, [x0]
    st1h            {z5.h}, p0, [x0, #1, mul vl]
    st1h            {z6.h}, p0, [x0, #2, mul vl]
    st1h            {z7.h}, p0, [x0, #3, mul vl]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
.vl_gt_48_blockcopy_ps_64_64:
    cmp             x9, #112
    bgt             .vl_gt_112_blockcopy_ps_64_64
    ptrue           p0.b, vl64
.rept 64
    ld1b            {z4.h}, p0/z, [x2]
    ld1b            {z5.h}, p0/z, [x2, #1, mul vl]
    st1h            {z4.h}, p0, [x0]
    st1h            {z5.h}, p0, [x0, #1, mul vl]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
.vl_gt_112_blockcopy_ps_64_64:
    ptrue           p0.b, vl128
.rept 64
    ld1b            {z4.h}, p0/z, [x2]
    st1h            {z4.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret

endfunc

function PFX(blockcopy_ss_16x16_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ss_16_16
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 8
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
.vl_gt_16_blockcopy_ss_16_16:
    ptrue           p0.h, vl16
.rept 16
    ld1h            {z0.h}, p0/z, [x2]
    st1h            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    ret
endfunc

function PFX(blockcopy_ss_32x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ss_32_32
    lsl             x1, x1, #1
    lsl             x3, x3, #1
    mov             w12, #4
.loop_css32_sve:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    cbnz            w12, .loop_css32_sve
    ret
.vl_gt_16_blockcopy_ss_32_32:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_ss_32_32
    ptrue           p0.h, vl16
.rept 32
    ld1h            {z0.h}, p0/z, [x2]
    ld1h            {z1.h}, p0/z, [x2, #1, mul vl]
    st1h            {z0.h}, p0, [x0]
    st1h            {z1.h}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    ret
.vl_gt_48_blockcopy_ss_32_32:
    ptrue           p0.h, vl32
.rept 32
    ld1h            {z0.h}, p0/z, [x2]
    st1h            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    ret
endfunc

function PFX(blockcopy_ss_64x64_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ss_64_64
    lsl             x1, x1, #1
    sub             x1, x1, #64
    lsl             x3, x3, #1
    sub             x3, x3, #64
    mov             w12, #8
.loop_css64_sve:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8h-v3.8h}, [x2], #64
    ld1             {v4.8h-v7.8h}, [x2], x3
    st1             {v0.8h-v3.8h}, [x0], #64
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_css64_sve
    ret
.vl_gt_16_blockcopy_ss_64_64:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_ss_64_64
    mov             w12, #8
    ptrue           p0.b, vl32
.vl_gt_16_loop_css64_sve:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z1.b}, p0/z, [x2, #1, mul vl]
    ld1b            {z2.b}, p0/z, [x2, #2, mul vl]
    ld1b            {z3.b}, p0/z, [x2, #3, mul vl]
    st1b            {z0.b}, p0, [x0]
    st1b            {z1.b}, p0, [x0, #1, mul vl]
    st1b            {z2.b}, p0, [x0, #2, mul vl]
    st1b            {z3.b}, p0, [x0, #3, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    cbnz            w12, .vl_gt_16_loop_css64_sve
    ret
.vl_gt_48_blockcopy_ss_64_64:
    cmp             x9, #112
    bgt             .vl_gt_112_blockcopy_ss_64_64
    mov             w12, #8
    ptrue           p0.b, vl64
.vl_gt_48_loop_css64_sve:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z1.b}, p0/z, [x2, #1, mul vl]
    st1b            {z0.b}, p0, [x0]
    st1b            {z1.b}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    cbnz            w12, .vl_gt_48_loop_css64_sve
    ret
.vl_gt_112_blockcopy_ss_64_64:
    mov             w12, #8
    ptrue           p0.b, vl128
.vl_gt_112_loop_css64_sve:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    st1b            {z0.b}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    cbnz            w12, .vl_gt_112_loop_css64_sve
    ret
endfunc

/******** Chroma blockcopy********/
function PFX(blockcopy_ss_16x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ss_16_32
    lsl             x1, x1, #1
    lsl             x3, x3, #1
.rept 16
    ld1             {v0.8h-v1.8h}, [x2], x3
    ld1             {v2.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
.vl_gt_16_blockcopy_ss_16_32:
    ptrue           p0.h, vl16
.rept 32
    ld1h            {z0.h}, p0/z, [x2]
    st1h            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    ret
endfunc

function PFX(blockcopy_ss_32x64_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ss_32_64
    lsl             x1, x1, #1
    lsl             x3, x3, #1
    mov             w12, #8
.loop_css32x64_sve:
    sub             w12, w12, #1
.rept 8
    ld1             {v0.8h-v3.8h}, [x2], x3
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    cbnz            w12, .loop_css32x64_sve
    ret
.vl_gt_16_blockcopy_ss_32_64:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_ss_32_64
    mov             w12, #8
    ptrue           p0.b, vl32
.vl_gt_32_loop_css32x64_sve:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z1.b}, p0/z, [x2, #1, mul vl]
    st1b            {z0.b}, p0, [x0]
    st1b            {z1.b}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    cbnz            w12, .vl_gt_32_loop_css32x64_sve
    ret
.vl_gt_48_blockcopy_ss_32_64:
    mov             w12, #8
    ptrue           p0.b, vl64
.vl_gt_48_loop_css32x64_sve:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    st1b            {z0.b}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1, lsl #1
.endr
    cbnz            w12, .vl_gt_48_loop_css32x64_sve
    ret
endfunc

// chroma blockcopy_ps
function PFX(blockcopy_ps_4x8_sve)
    ptrue           p0.h, vl4
.rept 8
    ld1b            {z0.h}, p0/z, [x2]
    st1h            {z0.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
endfunc

function PFX(blockcopy_ps_8x16_sve)
    ptrue           p0.h, vl8
.rept 16
    ld1b            {z0.h}, p0/z, [x2]
    st1h            {z0.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
endfunc

function PFX(blockcopy_ps_16x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ps_16_32
    lsl             x1, x1, #1
.rept 16
    ld1             {v4.16b}, [x2], x3
    ld1             {v5.16b}, [x2], x3
    uxtl            v0.8h, v4.8b
    uxtl2           v1.8h, v4.16b
    uxtl            v2.8h, v5.8b
    uxtl2           v3.8h, v5.16b
    st1             {v0.8h-v1.8h}, [x0], x1
    st1             {v2.8h-v3.8h}, [x0], x1
.endr
    ret
.vl_gt_16_blockcopy_ps_16_32:
    ptrue           p0.b, vl32
.rept 32
    ld1b            {z1.h}, p0/z, [x2]
    st1h            {z1.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
endfunc

function PFX(blockcopy_ps_32x64_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_ps_32_64
    lsl             x1, x1, #1
    mov             w12, #8
.loop_cps32x64_sve:
    sub             w12, w12, #1
.rept 4
    ld1             {v16.16b-v17.16b}, [x2], x3
    ld1             {v18.16b-v19.16b}, [x2], x3
    uxtl            v0.8h, v16.8b
    uxtl2           v1.8h, v16.16b
    uxtl            v2.8h, v17.8b
    uxtl2           v3.8h, v17.16b
    uxtl            v4.8h, v18.8b
    uxtl2           v5.8h, v18.16b
    uxtl            v6.8h, v19.8b
    uxtl2           v7.8h, v19.16b
    st1             {v0.8h-v3.8h}, [x0], x1
    st1             {v4.8h-v7.8h}, [x0], x1
.endr
    cbnz            w12, .loop_cps32x64_sve
    ret
.vl_gt_16_blockcopy_ps_32_64:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_ps_32_64
    ptrue           p0.b, vl32
.rept 64
    ld1b            {z2.h}, p0/z, [x2]
    ld1b            {z3.h}, p0/z, [x2, #1, mul vl]
    st1h            {z2.h}, p0, [x0]
    st1h            {z3.h}, p0, [x0, #1, mul vl]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
.vl_gt_48_blockcopy_ps_32_64:
    ptrue           p0.b, vl64
.rept 64
    ld1b            {z2.h}, p0/z, [x2]
    st1h            {z2.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
    add             x2, x2, x3
.endr
    ret
endfunc

// chroma blockcopy_sp
function PFX(blockcopy_sp_4x8_sve)
    ptrue           p0.h, vl4
.rept 8
    ld1h            {z0.h}, p0/z, [x2]
    st1b            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_8x16_sve)
    ptrue           p0.h, vl8
.rept 16
    ld1h            {z0.h}, p0/z, [x2]
    st1b            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_16x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_sp_16_32
    ptrue           p0.h, vl8
.rept 32
    ld1h            {z0.h}, p0/z, [x2]
    ld1h            {z1.h}, p0/z, [x2, #1, mul vl]
    st1b            {z0.h}, p0, [x0]
    st1b            {z1.h}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
.vl_gt_16_blockcopy_sp_16_32:
    ptrue           p0.h, vl16
.rept 32
    ld1h            {z0.h}, p0/z, [x2]
    st1b            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
endfunc

function PFX(blockcopy_sp_32x64_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_sp_32_64
    ptrue           p0.h, vl8
.rept 64
    ld1h            {z0.h}, p0/z, [x2]
    ld1h            {z1.h}, p0/z, [x2, #1, mul vl]
    ld1h            {z2.h}, p0/z, [x2, #2, mul vl]
    ld1h            {z3.h}, p0/z, [x2, #3, mul vl]
    st1b            {z0.h}, p0, [x0]
    st1b            {z1.h}, p0, [x0, #1, mul vl]
    st1b            {z2.h}, p0, [x0, #2, mul vl]
    st1b            {z3.h}, p0, [x0, #3, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
.vl_gt_16_blockcopy_sp_32_64:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_sp_32_64
    ptrue           p0.h, vl16
.rept 64
    ld1h            {z0.h}, p0/z, [x2]
    ld1h            {z1.h}, p0/z, [x2, #1, mul vl]
    st1b            {z0.h}, p0, [x0]
    st1b            {z1.h}, p0, [x0, #1, mul vl]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
.vl_gt_48_blockcopy_sp_32_64:
    ptrue           p0.h, vl32
.rept 64
    ld1h            {z0.h}, p0/z, [x2]
    st1b            {z0.h}, p0, [x0]
    add             x2, x2, x3, lsl #1
    add             x0, x0, x1
.endr
    ret
endfunc

/* blockcopy_pp(pixel* dst, intptr_t dstStride, const pixel* src, intptr_t srcStride) */

function PFX(blockcopy_pp_32x8_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_pp_32_8
.rept 8
    ld1             {v0.16b-v1.16b}, [x2], x3
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    ret
.vl_gt_16_blockcopy_pp_32_8:
    ptrue           p0.b, vl32
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    st1b            {z0.b}, p0, [x0]
    add             x2, x2, x3
    add             x0, x0, x1
.endr
    ret
endfunc

.macro blockcopy_pp_32xN_sve h
function PFX(blockcopy_pp_32x\h\()_sve)
    mov             w12, #\h / 8
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_pp_32xN_\h
.loop_sve_32x\h\():
    sub             w12, w12, #1
.rept 8
    ld1             {v0.16b-v1.16b}, [x2], x3
    st1             {v0.16b-v1.16b}, [x0], x1
.endr
    cbnz            w12, .loop_sve_32x\h
    ret
.vl_gt_16_blockcopy_pp_32xN_\h:
    ptrue           p0.b, vl32
.L_gt_16_blockcopy_pp_32xN_\h:
    sub             w12, w12, #1
.rept 8
    ld1b            {z0.b}, p0/z, [x2]
    st1b            {z0.b}, p0, [x0]
    add             x2, x2, x3
    add             x0, x0, x1
.endr
    cbnz            w12, .L_gt_16_blockcopy_pp_32xN_\h
    ret
endfunc
.endm

blockcopy_pp_32xN_sve 16
blockcopy_pp_32xN_sve 24
blockcopy_pp_32xN_sve 32
blockcopy_pp_32xN_sve 64
blockcopy_pp_32xN_sve 48

.macro blockcopy_pp_64xN_sve h
function PFX(blockcopy_pp_64x\h\()_sve)
    mov             w12, #\h / 4
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockcopy_pp_64xN_\h
.loop_sve_64x\h\():
    sub             w12, w12, #1
.rept 4
    ld1             {v0.16b-v3.16b}, [x2], x3
    st1             {v0.16b-v3.16b}, [x0], x1
.endr
    cbnz            w12, .loop_sve_64x\h
    ret
.vl_gt_16_blockcopy_pp_64xN_\h:
    cmp             x9, #48
    bgt             .vl_gt_48_blockcopy_pp_64xN_\h
    ptrue           p0.b, vl32
.L_le_32_blockcopy_pp_64xN_\h:
    sub             w12, w12, #1
.rept 4
    ld1b            {z0.b}, p0/z, [x2]
    ld1b            {z1.b}, p0/z, [x2, #1, mul vl]
    st1b            {z0.b}, p0, [x0]
    st1b            {z1.b}, p0, [x0, #1, mul vl]
    add             x2, x2, x3
    add             x0, x0, x1
.endr
    cbnz            w12, .L_le_32_blockcopy_pp_64xN_\h
    ret
.vl_gt_48_blockcopy_pp_64xN_\h:
    ptrue           p0.b, vl64
.L_blockcopy_pp_64xN_\h:
    sub             w12, w12, #1
.rept 4
    ld1b            {z0.b}, p0/z, [x2]
    st1b            {z0.b}, p0, [x0]
    add             x2, x2, x3
    add             x0, x0, x1
.endr
    cbnz            w12, .L_blockcopy_pp_64xN_\h
    ret
endfunc
.endm

blockcopy_pp_64xN_sve 16
blockcopy_pp_64xN_sve 32
blockcopy_pp_64xN_sve 48
blockcopy_pp_64xN_sve 64

function PFX(blockfill_s_32x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_blockfill_s_32_32
    dup             v0.8h, w2
    mov             v1.16b, v0.16b
    mov             v2.16b, v0.16b
    mov             v3.16b, v0.16b
    lsl             x1, x1, #1
.rept 32
    st1             {v0.8h-v3.8h}, [x0], x1
.endr
    ret
.vl_gt_16_blockfill_s_32_32:
    cmp             x9, #48
    bgt             .vl_gt_48_blockfill_s_32_32
    dup             z0.h, w2
    ptrue           p0.h, vl16
.rept 32
    st1h            {z0.h}, p0, [x0]
    st1h            {z0.h}, p0, [x0, #1, mul vl]
    add             x0, x0, x1, lsl #1
.endr
    ret
.vl_gt_48_blockfill_s_32_32:
    dup             z0.h, w2
    ptrue           p0.h, vl32
.rept 32
    st1h            {z0.h}, p0, [x0]
    add             x0, x0, x1, lsl #1
.endr
    ret
endfunc

// void cpy2Dto1D_shl(int16_t* dst, const int16_t* src, intptr_t srcStride, int shift)
.macro cpy2Dto1D_shl_start_sve
    add             x2, x2, x2
    mov             z0.h, w3
.endm

function PFX(cpy2Dto1D_shl_16x16_sve)
    dup             z0.h, w3
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy2Dto1D_shl_16x16
    cpy2Dto1D_shl_start_sve
    mov             w12, #4
.loop_cpy2Dto1D_shl_16_sve:
    sub             w12, w12, #1
.rept 4
    ld1             {v2.16b-v3.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.16b-v3.16b}, [x0], #32
.endr
    cbnz            w12, .loop_cpy2Dto1D_shl_16_sve
    ret
.vl_gt_16_cpy2Dto1D_shl_16x16:
    ptrue           p0.h, vl16
.rept 16
    ld1h            {z1.h}, p0/z, [x1]
    lsl             z1.h, p0/m, z1.h, z0.h
    st1h            {z1.h}, p0, [x0]
    add             x1, x1, x2, lsl #1
    add             x0, x0, #32
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shl_32x32_sve)
    dup             z0.h, w3
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy2Dto1D_shl_32x32
    cpy2Dto1D_shl_start_sve
    mov             w12, #16
.loop_cpy2Dto1D_shl_32_sve:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], #64
.endr
    cbnz            w12, .loop_cpy2Dto1D_shl_32_sve
    ret
.vl_gt_16_cpy2Dto1D_shl_32x32:
    cmp             x9, #48
    bgt             .vl_gt_48_cpy2Dto1D_shl_32x32
    ptrue           p0.h, vl16
.rept 32
    ld1h            {z1.h}, p0/z, [x1]
    ld1h            {z2.h}, p0/z, [x1, #1, mul vl]
    lsl             z1.h, p0/m, z1.h, z0.h
    lsl             z2.h, p0/m, z2.h, z0.h
    st1h            {z1.h}, p0, [x0]
    st1h            {z2.h}, p0, [x0, #1, mul vl]
    add             x1, x1, x2, lsl #1
    add             x0, x0, #64
.endr
    ret
.vl_gt_48_cpy2Dto1D_shl_32x32:
    ptrue           p0.h, vl32
.rept 32
    ld1h            {z1.h}, p0/z, [x1]
    lsl             z1.h, p0/m, z1.h, z0.h
    st1h            {z1.h}, p0, [x0]
    add             x1, x1, x2, lsl #1
    add             x0, x0, #64
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shl_64x64_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy2Dto1D_shl_64x64
    cpy2Dto1D_shl_start_sve
    mov             w12, #32
    sub             x2, x2, #64
.loop_cpy2Dto1D_shl_64_sve:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    ld1             {v16.16b-v19.16b}, [x1], x2
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    sshl            v16.8h, v16.8h, v0.8h
    sshl            v17.8h, v17.8h, v0.8h
    sshl            v18.8h, v18.8h, v0.8h
    sshl            v19.8h, v19.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], #64
    st1             {v16.16b-v19.16b}, [x0], #64
.endr
    cbnz            w12, .loop_cpy2Dto1D_shl_64_sve
    ret
.vl_gt_16_cpy2Dto1D_shl_64x64:
    dup             z0.h, w3
    mov             x8, #64
    mov             w12, #64
.L_init_cpy2Dto1D_shl_64x64:
    sub             w12, w12, 1
    mov             x9, #0
    whilelt         p0.h, x9, x8
.L_cpy2Dto1D_shl_64x64:
    ld1h            {z1.h}, p0/z, [x1, x9, lsl #1]
    lsl             z1.h, p0/m, z1.h, z0.h
    st1h            {z1.h}, p0, [x0, x9, lsl #1]
    inch            x9
    whilelt         p0.h, x9, x8
    b.first         .L_cpy2Dto1D_shl_64x64
    add             x1, x1, x2, lsl #1
    addvl           x0, x0, #1
    cbnz            w12, .L_init_cpy2Dto1D_shl_64x64
    ret
endfunc

// void cpy2Dto1D_shr(int16_t* dst, const int16_t* src, intptr_t srcStride, int shift)

function PFX(cpy2Dto1D_shr_4x4_sve)
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    ptrue           p0.h, vl8
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
    lsl             x2, x2, #1
    index           z3.d, #0, x2
    index           z4.d, #0, #8
.rept 2
    ld1d            {z5.d}, p0/z, [x1, z3.d]
    add             x1, x1, x2, lsl #1
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0, z4.d]
    add             x0, x0, #16
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shr_8x8_sve)
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    ptrue           p0.h, vl8
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 8
    ld1d            {z5.d}, p0/z, [x1]
    add             x1, x1, x2, lsl #1
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0]
    add             x0, x0, #16
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shr_16x16_sve)
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy2Dto1D_shr_16x16
    ptrue           p0.h, vl8
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 16
    ld1d            {z5.d}, p0/z, [x1]
    ld1d            {z6.d}, p0/z, [x1, #1, mul vl]
    add             x1, x1, x2, lsl #1
    add             z5.h, p0/m, z5.h, z2.h
    add             z6.h, p0/m, z6.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    asr             z6.h, p0/m, z6.h, z0.h
    st1d            {z5.d}, p0, [x0]
    st1d            {z6.d}, p0, [x0, #1, mul vl]
    add             x0, x0, #32
.endr
    ret
.vl_gt_16_cpy2Dto1D_shr_16x16:
    ptrue           p0.h, vl16
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 16
    ld1d            {z5.d}, p0/z, [x1]
    add             x1, x1, x2, lsl #1
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0]
    add             x0, x0, #32
.endr
    ret
endfunc

function PFX(cpy2Dto1D_shr_32x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy2Dto1D_shr_32x32
    cpy2Dto1D_shr_start
    mov             w12, #16
.loop_cpy2Dto1D_shr_32_sve:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.8h-v5.8h}, [x1], x2
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sub             v4.8h, v4.8h, v1.8h
    sub             v5.8h, v5.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.8h-v5.8h}, [x0], #64
.endr
    cbnz            w12, .loop_cpy2Dto1D_shr_32_sve
    ret
.vl_gt_16_cpy2Dto1D_shr_32x32:
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    cmp             x9, #48
    bgt             .vl_gt_48_cpy2Dto1D_shr_32x32
    ptrue           p0.h, vl16
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 32
    ld1d            {z5.d}, p0/z, [x1]
    ld1d            {z6.d}, p0/z, [x1, #1, mul vl]
    add             x1, x1, x2, lsl #1
    add             z5.h, p0/m, z5.h, z2.h
    add             z6.h, p0/m, z6.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    asr             z6.h, p0/m, z6.h, z0.h
    st1d            {z5.d}, p0, [x0]
    st1d            {z6.d}, p0, [x0, #1, mul vl]
    add             x0, x0, #64
.endr
    ret
.vl_gt_48_cpy2Dto1D_shr_32x32:
    ptrue           p0.h, vl32
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 32
    ld1d            {z5.d}, p0/z, [x1]
    add             x1, x1, x2, lsl #1
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0]
    add             x0, x0, #64
.endr
    ret
endfunc

// void cpy1Dto2D_shl(int16_t* dst, const int16_t* src, intptr_t dstStride, int shift)

function PFX(cpy1Dto2D_shl_16x16_sve)
    dup             z0.h, w3
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy1Dto2D_shl_16x16
    ptrue           p0.h, vl8
.rept 16
    ld1h            {z1.h}, p0/z, [x1]
    ld1h            {z2.h}, p0/z, [x1, #1, mul vl]
    lsl             z1.h, p0/m, z1.h, z0.h
    lsl             z2.h, p0/m, z2.h, z0.h
    st1h            {z1.h}, p0, [x0]
    st1h            {z2.h}, p0, [x0, #1, mul vl]
    add             x1, x1, #32
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_16_cpy1Dto2D_shl_16x16:
    ptrue           p0.h, vl16
.rept 16
    ld1h            {z1.h}, p0/z, [x1]
    lsl             z1.h, p0/m, z1.h, z0.h
    st1h            {z1.h}, p0, [x0]
    add             x1, x1, #32
    add             x0, x0, x2, lsl #1
.endr
    ret
endfunc

function PFX(cpy1Dto2D_shl_32x32_sve)
    dup             z0.h, w3
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy1Dto2D_shl_32x32
    ptrue           p0.h, vl8
.rept 32
    ld1h            {z1.h}, p0/z, [x1]
    ld1h            {z2.h}, p0/z, [x1, #1, mul vl]
    ld1h            {z3.h}, p0/z, [x1, #2, mul vl]
    ld1h            {z4.h}, p0/z, [x1, #3, mul vl]
    lsl             z1.h, p0/m, z1.h, z0.h
    lsl             z2.h, p0/m, z2.h, z0.h
    lsl             z3.h, p0/m, z3.h, z0.h
    lsl             z4.h, p0/m, z4.h, z0.h
    st1h            {z1.h}, p0, [x0]
    st1h            {z2.h}, p0, [x0, #1, mul vl]
    st1h            {z3.h}, p0, [x0, #2, mul vl]
    st1h            {z4.h}, p0, [x0, #3, mul vl]
    add             x1, x1, #64
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_16_cpy1Dto2D_shl_32x32:
    cmp             x9, #48
    bgt             .vl_gt_48_cpy1Dto2D_shl_32x32
    ptrue           p0.h, vl16
.rept 32
    ld1h            {z1.h}, p0/z, [x1]
    ld1h            {z2.h}, p0/z, [x1, #1, mul vl]
    lsl             z1.h, p0/m, z1.h, z0.h
    lsl             z2.h, p0/m, z2.h, z0.h
    st1h            {z1.h}, p0, [x0]
    st1h            {z2.h}, p0, [x0, #1, mul vl]
    add             x1, x1, #64
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_48_cpy1Dto2D_shl_32x32:
    ptrue           p0.h, vl32
.rept 32
    ld1h            {z1.h}, p0/z, [x1]
    lsl             z1.h, p0/m, z1.h, z0.h
    st1h            {z1.h}, p0, [x0]
    add             x1, x1, #64
    add             x0, x0, x2, lsl #1
.endr
    ret
endfunc

function PFX(cpy1Dto2D_shl_64x64_sve)
    dup             z0.h, w3
    mov             x8, #64
    mov             w12, #64
.L_init_cpy1Dto2D_shl_64x64:
    sub             w12, w12, 1
    mov             x9, #0
    whilelt         p0.h, x9, x8
.L_cpy1Dto2D_shl_64x64:
    ld1h            {z1.h}, p0/z, [x1, x9, lsl #1]
    lsl             z1.h, p0/m, z1.h, z0.h
    st1h            {z1.h}, p0, [x0, x9, lsl #1]
    inch            x9
    whilelt         p0.h, x9, x8
    b.first         .L_cpy1Dto2D_shl_64x64
    addvl           x1, x1, #1
    add             x0, x0, x2, lsl #1
    cbnz            w12, .L_init_cpy1Dto2D_shl_64x64
    ret
endfunc

// void cpy1Dto2D_shr(int16_t* dst, const int16_t* src, intptr_t dstStride, int shift)

function PFX(cpy1Dto2D_shr_16x16_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy1Dto2D_shr_16x16
    cpy1Dto2D_shr_start
    mov             w12, #4
.loop_cpy1Dto2D_shr_16:
    sub             w12, w12, #1
.rept 4
    ld1             {v2.8h-v3.8h}, [x1], #32
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    st1             {v2.8h-v3.8h}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shr_16
    ret
.vl_gt_16_cpy1Dto2D_shr_16x16:
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    ptrue           p0.h, vl16
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 16
    ld1d            {z5.d}, p0/z, [x1]
    add             x1, x1, #32
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0]
    add             x0, x0, x2, lsl #1
.endr
    ret
endfunc

function PFX(cpy1Dto2D_shr_32x32_sve)
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy1Dto2D_shr_32x32
    cpy1Dto2D_shr_start
    mov             w12, #16
.loop_cpy1Dto2D_shr_32_sve:
    sub             w12, w12, #1
.rept 2
    ld1             {v2.16b-v5.16b}, [x1], #64
    sub             v2.8h, v2.8h, v1.8h
    sub             v3.8h, v3.8h, v1.8h
    sub             v4.8h, v4.8h, v1.8h
    sub             v5.8h, v5.8h, v1.8h
    sshl            v2.8h, v2.8h, v0.8h
    sshl            v3.8h, v3.8h, v0.8h
    sshl            v4.8h, v4.8h, v0.8h
    sshl            v5.8h, v5.8h, v0.8h
    st1             {v2.16b-v5.16b}, [x0], x2
.endr
    cbnz            w12, .loop_cpy1Dto2D_shr_32_sve
    ret
.vl_gt_16_cpy1Dto2D_shr_32x32:
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    cmp             x9, #48
    bgt             .vl_gt_48_cpy2Dto1D_shr_32x32
    ptrue           p0.h, vl16
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 32
    ld1d            {z5.d}, p0/z, [x1]
    ld1d            {z6.d}, p0/z, [x1, #1, mul vl]
    add             x1, x1, #64
    add             z5.h, p0/m, z5.h, z2.h
    add             z6.h, p0/m, z6.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    asr             z6.h, p0/m, z6.h, z0.h
    st1d            {z5.d}, p0, [x0]
    st1d            {z6.d}, p0, [x0, #1, mul vl]
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_48_cpy1Dto2D_shr_32x32:
    ptrue           p0.h, vl32
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 32
    ld1d            {z5.d}, p0/z, [x1]
    add             x1, x1, #64
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0]
    add             x0, x0, x2, lsl #1
.endr
    ret
endfunc

function PFX(cpy1Dto2D_shr_64x64_sve)
    dup             z0.h, w3
    sub             w4, w3, #1
    dup             z1.h, w4
    rdvl            x9, #1
    cmp             x9, #16
    bgt             .vl_gt_16_cpy1Dto2D_shr_64x64
    ptrue           p0.h, vl8
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 128
    ld1d            {z5.d}, p0/z, [x1]
    ld1d            {z6.d}, p0/z, [x1, #1, mul vl]
    ld1d            {z7.d}, p0/z, [x1, #2, mul vl]
    ld1d            {z8.d}, p0/z, [x1, #3, mul vl]
    ld1d            {z9.d}, p0/z, [x1, #4, mul vl]
    ld1d            {z10.d}, p0/z, [x1, #5, mul vl]
    ld1d            {z11.d}, p0/z, [x1, #6, mul vl]
    ld1d            {z12.d}, p0/z, [x1, #7, mul vl]
    add             x1, x1, #128
    add             z5.h, p0/m, z5.h, z2.h
    add             z6.h, p0/m, z6.h, z2.h
    add             z7.h, p0/m, z7.h, z2.h
    add             z8.h, p0/m, z8.h, z2.h
    add             z9.h, p0/m, z9.h, z2.h
    add             z10.h, p0/m, z10.h, z2.h
    add             z11.h, p0/m, z11.h, z2.h
    add             z12.h, p0/m, z12.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    asr             z6.h, p0/m, z6.h, z0.h
    asr             z7.h, p0/m, z7.h, z0.h
    asr             z8.h, p0/m, z8.h, z0.h
    asr             z9.h, p0/m, z9.h, z0.h
    asr             z10.h, p0/m, z10.h, z0.h
    asr             z11.h, p0/m, z11.h, z0.h
    asr             z12.h, p0/m, z12.h, z0.h
    st1d            {z5.d}, p0, [x0]
    st1d            {z6.d}, p0, [x0, #1, mul vl]
    st1d            {z7.d}, p0, [x0, #2, mul vl]
    st1d            {z8.d}, p0, [x0, #3, mul vl]
    st1d            {z9.d}, p0, [x0, #4, mul vl]
    st1d            {z10.d}, p0, [x0, #5, mul vl]
    st1d            {z11.d}, p0, [x0, #6, mul vl]
    st1d            {z12.d}, p0, [x0, #7, mul vl]
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_16_cpy1Dto2D_shr_64x64:
    cmp             x9, #48
    bgt             .vl_gt_48_cpy1Dto2D_shr_64x64
    ptrue           p0.h, vl16
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 128
    ld1d            {z5.d}, p0/z, [x1]
    ld1d            {z6.d}, p0/z, [x1, #1, mul vl]
    ld1d            {z7.d}, p0/z, [x1, #2, mul vl]
    ld1d            {z8.d}, p0/z, [x1, #3, mul vl]
    add             x1, x1, #128
    add             z5.h, p0/m, z5.h, z2.h
    add             z6.h, p0/m, z6.h, z2.h
    add             z7.h, p0/m, z7.h, z2.h
    add             z8.h, p0/m, z8.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    asr             z6.h, p0/m, z6.h, z0.h
    asr             z7.h, p0/m, z7.h, z0.h
    asr             z8.h, p0/m, z8.h, z0.h
    st1d            {z5.d}, p0, [x0]
    st1d            {z6.d}, p0, [x0, #1, mul vl]
    st1d            {z7.d}, p0, [x0, #2, mul vl]
    st1d            {z8.d}, p0, [x0, #3, mul vl]
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_48_cpy1Dto2D_shr_64x64:
    cmp             x9, #112
    bgt             .vl_gt_112_cpy1Dto2D_shr_64x64
    ptrue           p0.h, vl32
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 128
    ld1d            {z5.d}, p0/z, [x1]
    ld1d            {z6.d}, p0/z, [x1, #1, mul vl]
    add             x1, x1, #128
    add             z5.h, p0/m, z5.h, z2.h
    add             z6.h, p0/m, z6.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    asr             z6.h, p0/m, z6.h, z0.h
    st1d            {z5.d}, p0, [x0]
    st1d            {z6.d}, p0, [x0, #1, mul vl]
    add             x0, x0, x2, lsl #1
.endr
    ret
.vl_gt_112_cpy1Dto2D_shr_64x64:
    ptrue           p0.h, vl64
    mov             z2.h, #1
    lsl             z2.h, p0/m, z2.h, z1.h
.rept 128
    ld1d            {z5.d}, p0/z, [x1]
    add             x1, x1, #128
    add             z5.h, p0/m, z5.h, z2.h
    asr             z5.h, p0/m, z5.h, z0.h
    st1d            {z5.d}, p0, [x0]
    add             x0, x0, x2, lsl #1
.endr
    ret
endfunc
