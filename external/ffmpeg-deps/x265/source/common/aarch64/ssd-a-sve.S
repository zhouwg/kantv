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

.arch armv8-a+sve

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.text

function PFX(pixel_sse_pp_4x4_sve)
    ptrue           p0.s, vl4
    ld1b            {z0.s}, p0/z, [x0]
    ld1b            {z17.s}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    sub             z0.s, p0/m, z0.s, z17.s
    mul             z0.s, p0/m, z0.s, z0.s
.rept 3
    ld1b            {z16.s}, p0/z, [x0]
    ld1b            {z17.s}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    sub             z16.s, p0/m, z16.s, z17.s
    mla             z0.s, p0/m, z16.s, z16.s
.endr
    uaddv           d0, p0, z0.s
    fmov            w0, s0
    ret
endfunc

function PFX(pixel_sse_pp_4x8_sve)
    ptrue           p0.s, vl4
    ld1b            {z0.s}, p0/z, [x0]
    ld1b            {z17.s}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    sub             z0.s, p0/m, z0.s, z17.s
    mul             z0.s, p0/m, z0.s, z0.s
.rept 7
    ld1b            {z16.s}, p0/z, [x0]
    ld1b            {z17.s}, p0/z, [x2]
    add             x0, x0, x1
    add             x2, x2, x3
    sub             z16.s, p0/m, z16.s, z17.s
    mla             z0.s, p0/m, z16.s, z16.s
.endr
    uaddv           d0, p0, z0.s
    fmov            w0, s0
    ret
endfunc
