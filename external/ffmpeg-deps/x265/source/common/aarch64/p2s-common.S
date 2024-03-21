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

#if HIGH_BIT_DEPTH
# if BIT_DEPTH == 10
#  define P2S_SHIFT 4
# elif BIT_DEPTH == 12
#  define P2S_SHIFT 2
# endif
.macro p2s_start
    add             x3, x3, x3
    add             x1, x1, x1
    movi            v31.8h, #0xe0, lsl #8
.endm

#else // if !HIGH_BIT_DEPTH
# define P2S_SHIFT 6
.macro p2s_start
    add             x3, x3, x3
    movi            v31.8h, #0xe0, lsl #8
.endm
#endif // HIGH_BIT_DEPTH

.macro p2s_2x2
#if HIGH_BIT_DEPTH
    ld1             {v0.s}[0], [x0], x1
    ld1             {v0.s}[1], [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
#else
    ldrh            w10, [x0]
    add             x0, x0, x1
    ldrh            w11, [x0]
    orr             w10, w10, w11, lsl #16
    add             x0, x0, x1
    dup             v0.4s, w10
    ushll           v3.8h, v0.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    st1             {v3.s}[0], [x2], x3
    st1             {v3.s}[1], [x2], x3
.endm

.macro p2s_6x2
#if HIGH_BIT_DEPTH
    ld1             {v0.d}[0], [x0], #8
    ld1             {v1.s}[0], [x0], x1
    ld1             {v0.d}[1], [x0], #8
    ld1             {v1.s}[1], [x0], x1
    shl             v3.8h, v0.8h, #P2S_SHIFT
    shl             v4.8h, v1.8h, #P2S_SHIFT
#else
    ldr             s0, [x0]
    ldrh            w10, [x0, #4]
    add             x0, x0, x1
    ld1             {v0.s}[1], [x0]
    ldrh            w11, [x0, #4]
    add             x0, x0, x1
    orr             w10, w10, w11, lsl #16
    dup             v1.4s, w10
    ushll           v3.8h, v0.8b, #P2S_SHIFT
    ushll           v4.8h, v1.8b, #P2S_SHIFT
#endif
    add             v3.8h, v3.8h, v31.8h
    add             v4.8h, v4.8h, v31.8h
    st1             {v3.d}[0], [x2], #8
    st1             {v4.s}[0], [x2], x3
    st1             {v3.d}[1], [x2], #8
    st1             {v4.s}[1], [x2], x3
.endm
