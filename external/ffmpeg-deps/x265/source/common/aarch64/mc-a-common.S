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

.macro addAvg_start
    lsl             x3, x3, #1
    lsl             x4, x4, #1
    mov             w11, #0x40
    dup             v30.16b, w11
.endm

.macro addavg_1 v0, v1
    add             \v0\().8h, \v0\().8h, \v1\().8h
    saddl           v16.4s, \v0\().4h, v30.4h
    saddl2          v17.4s, \v0\().8h, v30.8h
    shrn            \v0\().4h, v16.4s, #7
    shrn2           \v0\().8h, v17.4s, #7
.endm
