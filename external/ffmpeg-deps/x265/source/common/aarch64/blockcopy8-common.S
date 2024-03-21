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

#include "asm.S"

.arch           armv8-a

// void cpy1Dto2D_shr(int16_t* dst, const int16_t* src, intptr_t dstStride, int shift)
.macro cpy1Dto2D_shr_start
    add             x2, x2, x2
    dup             v0.8h, w3
    cmeq            v1.8h, v1.8h, v1.8h
    sshl            v1.8h, v1.8h, v0.8h
    sri             v1.8h, v1.8h, #1
    neg             v0.8h, v0.8h
.endm

.macro cpy2Dto1D_shr_start
    add             x2, x2, x2
    dup             v0.8h, w3
    cmeq            v1.8h, v1.8h, v1.8h
    sshl            v1.8h, v1.8h, v0.8h
    sri             v1.8h, v1.8h, #1
    neg             v0.8h, v0.8h
.endm

const xtn_xtn2_table, align=4
.byte    0, 2, 4, 6, 8, 10, 12, 14
.byte    16, 18, 20, 22, 24, 26, 28, 30
endconst

