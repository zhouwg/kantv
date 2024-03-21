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

#include "asm.S"

.arch armv8-a+sve

.macro ABS2_SVE a b c
    abs             \a, \c\()/m, \a
    abs             \b, \c\()/m, \b
.endm

.macro ABS8_SVE z0, z1, z2, z3, z4, z5, z6, z7, p0
    ABS2_SVE        \z0, \z1, p0
    ABS2_SVE        \z2, \z3, p0
    ABS2_SVE        \z4, \z5, p0
    ABS2_SVE        \z6, \z7, p0
.endm

