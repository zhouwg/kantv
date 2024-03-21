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

// This file contains the macros written using NEON instruction set
// that are also used by the SVE2 functions

.arch           armv8-a

#ifdef __APPLE__
.section __RODATA,__rodata
#else
.section .rodata
#endif

.align 4

.macro pixel_var_start
    movi            v0.16b, #0
    movi            v1.16b, #0
    movi            v2.16b, #0
    movi            v3.16b, #0
.endm

.macro pixel_var_1 v
    uaddw           v0.8h, v0.8h, \v\().8b
    umull           v30.8h, \v\().8b, \v\().8b
    uaddw2          v1.8h, v1.8h, \v\().16b
    umull2          v31.8h, \v\().16b, \v\().16b
    uadalp          v2.4s, v30.8h
    uadalp          v3.4s, v31.8h
.endm

.macro pixel_var_end
    uaddlv          s0, v0.8h
    uaddlv          s1, v1.8h
    add             v2.4s, v2.4s, v3.4s
    fadd            s0, s0, s1
    uaddlv          d2, v2.4s
    fmov            w0, s0
    fmov            x2, d2
    orr             x0, x0, x2, lsl #32
.endm

.macro ssimDist_start
    movi            v0.16b, #0
    movi            v1.16b, #0
.endm

.macro ssimDist_end
    uaddlv          d0, v0.4s
    uaddlv          d1, v1.4s
    str             d0, [x6]
    str             d1, [x4]
.endm

.macro normFact_start
    movi            v0.16b, #0
.endm

.macro normFact_end
    uaddlv          d0, v0.4s
    str             d0, [x3]
.endm

