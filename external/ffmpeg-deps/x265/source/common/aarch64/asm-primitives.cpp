/*****************************************************************************
 * Copyright (C) 2020 MulticoreWare, Inc
 *
 * Authors: Hongbin Liu <liuhongbin1@huawei.com>
 *          Yimeng Su <yimeng.su@huawei.com>
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


#include "common.h"
#include "primitives.h"
#include "x265.h"
#include "cpu.h"

extern "C" {
#include "fun-decls.h"
}

#define ALL_LUMA_TU_TYPED(prim, fncdef, fname, cpu) \
    p.cu[BLOCK_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.cu[BLOCK_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.cu[BLOCK_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.cu[BLOCK_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.cu[BLOCK_64x64].prim = fncdef PFX(fname ## _64x64_ ## cpu)
#define LUMA_TU_TYPED_NEON(prim, fncdef, fname) \
    p.cu[BLOCK_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.cu[BLOCK_8x8].prim   = fncdef PFX(fname ## _8x8_ ## neon); \
    p.cu[BLOCK_16x16].prim = fncdef PFX(fname ## _16x16_ ## neon); \
    p.cu[BLOCK_64x64].prim = fncdef PFX(fname ## _64x64_ ## neon)
#define LUMA_TU_TYPED_CAN_USE_SVE(prim, fncdef, fname) \
    p.cu[BLOCK_32x32].prim = fncdef PFX(fname ## _32x32_ ## sve)
#define ALL_LUMA_TU(prim, fname, cpu)      ALL_LUMA_TU_TYPED(prim, , fname, cpu)
#define LUMA_TU_NEON(prim, fname)      LUMA_TU_TYPED_NEON(prim, , fname)
#define LUMA_TU_CAN_USE_SVE(prim, fname)      LUMA_TU_TYPED_CAN_USE_SVE(prim, , fname)

#define ALL_LUMA_PU_TYPED(prim, fncdef, fname, cpu) \
    p.pu[LUMA_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.pu[LUMA_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.pu[LUMA_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.pu[LUMA_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.pu[LUMA_64x64].prim = fncdef PFX(fname ## _64x64_ ## cpu); \
    p.pu[LUMA_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.pu[LUMA_4x8].prim   = fncdef PFX(fname ## _4x8_ ## cpu); \
    p.pu[LUMA_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.pu[LUMA_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.pu[LUMA_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.pu[LUMA_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.pu[LUMA_64x32].prim = fncdef PFX(fname ## _64x32_ ## cpu); \
    p.pu[LUMA_32x64].prim = fncdef PFX(fname ## _32x64_ ## cpu); \
    p.pu[LUMA_16x12].prim = fncdef PFX(fname ## _16x12_ ## cpu); \
    p.pu[LUMA_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.pu[LUMA_16x4].prim  = fncdef PFX(fname ## _16x4_ ## cpu); \
    p.pu[LUMA_4x16].prim  = fncdef PFX(fname ## _4x16_ ## cpu); \
    p.pu[LUMA_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.pu[LUMA_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.pu[LUMA_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.pu[LUMA_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu); \
    p.pu[LUMA_64x48].prim = fncdef PFX(fname ## _64x48_ ## cpu); \
    p.pu[LUMA_48x64].prim = fncdef PFX(fname ## _48x64_ ## cpu); \
    p.pu[LUMA_64x16].prim = fncdef PFX(fname ## _64x16_ ## cpu); \
    p.pu[LUMA_16x64].prim = fncdef PFX(fname ## _16x64_ ## cpu)
#define LUMA_PU_TYPED_MULTIPLE_ARCHS_1(prim, fncdef, fname, cpu) \
    p.pu[LUMA_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.pu[LUMA_4x8].prim   = fncdef PFX(fname ## _4x8_ ## cpu); \
    p.pu[LUMA_4x16].prim  = fncdef PFX(fname ## _4x16_ ## cpu)
#define LUMA_PU_TYPED_MULTIPLE_ARCHS_2(prim, fncdef, fname, cpu) \
    p.pu[LUMA_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.pu[LUMA_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.pu[LUMA_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.pu[LUMA_64x64].prim = fncdef PFX(fname ## _64x64_ ## cpu); \
    p.pu[LUMA_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.pu[LUMA_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.pu[LUMA_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.pu[LUMA_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.pu[LUMA_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.pu[LUMA_64x32].prim = fncdef PFX(fname ## _64x32_ ## cpu); \
    p.pu[LUMA_32x64].prim = fncdef PFX(fname ## _32x64_ ## cpu); \
    p.pu[LUMA_16x12].prim = fncdef PFX(fname ## _16x12_ ## cpu); \
    p.pu[LUMA_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.pu[LUMA_16x4].prim  = fncdef PFX(fname ## _16x4_ ## cpu); \
    p.pu[LUMA_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.pu[LUMA_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.pu[LUMA_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.pu[LUMA_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu); \
    p.pu[LUMA_64x48].prim = fncdef PFX(fname ## _64x48_ ## cpu); \
    p.pu[LUMA_48x64].prim = fncdef PFX(fname ## _48x64_ ## cpu); \
    p.pu[LUMA_64x16].prim = fncdef PFX(fname ## _64x16_ ## cpu); \
    p.pu[LUMA_16x64].prim = fncdef PFX(fname ## _16x64_ ## cpu)
#define LUMA_PU_TYPED_NEON_1(prim, fncdef, fname) \
    p.pu[LUMA_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.pu[LUMA_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.pu[LUMA_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon); \
    p.pu[LUMA_12x16].prim = fncdef PFX(fname ## _12x16_ ## neon); \
    p.pu[LUMA_8x8].prim   = fncdef PFX(fname ## _8x8_ ## neon); \
    p.pu[LUMA_16x16].prim = fncdef PFX(fname ## _16x16_ ## neon); \
    p.pu[LUMA_8x4].prim   = fncdef PFX(fname ## _8x4_ ## neon); \
    p.pu[LUMA_16x8].prim  = fncdef PFX(fname ## _16x8_ ## neon); \
    p.pu[LUMA_8x16].prim  = fncdef PFX(fname ## _8x16_ ## neon); \
    p.pu[LUMA_16x12].prim = fncdef PFX(fname ## _16x12_ ## neon); \
    p.pu[LUMA_16x32].prim = fncdef PFX(fname ## _16x32_ ## neon); \
    p.pu[LUMA_16x4].prim  = fncdef PFX(fname ## _16x4_ ## neon); \
    p.pu[LUMA_24x32].prim = fncdef PFX(fname ## _24x32_ ## neon); \
    p.pu[LUMA_8x32].prim  = fncdef PFX(fname ## _8x32_ ## neon); \
    p.pu[LUMA_48x64].prim = fncdef PFX(fname ## _48x64_ ## neon); \
    p.pu[LUMA_16x64].prim = fncdef PFX(fname ## _16x64_ ## neon)
#define LUMA_PU_TYPED_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, fncdef, fname) \
    p.pu[LUMA_32x32].prim = fncdef PFX(fname ## _32x32_ ## sve); \
    p.pu[LUMA_64x64].prim = fncdef PFX(fname ## _64x64_ ## sve); \
    p.pu[LUMA_32x16].prim = fncdef PFX(fname ## _32x16_ ## sve); \
    p.pu[LUMA_64x32].prim = fncdef PFX(fname ## _64x32_ ## sve); \
    p.pu[LUMA_32x64].prim = fncdef PFX(fname ## _32x64_ ## sve); \
    p.pu[LUMA_32x24].prim = fncdef PFX(fname ## _32x24_ ## sve); \
    p.pu[LUMA_32x8].prim  = fncdef PFX(fname ## _32x8_ ## sve); \
    p.pu[LUMA_64x48].prim = fncdef PFX(fname ## _64x48_ ## sve); \
    p.pu[LUMA_64x16].prim = fncdef PFX(fname ## _64x16_ ## sve)
#define LUMA_PU_TYPED_NEON_2(prim, fncdef, fname) \
    p.pu[LUMA_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.pu[LUMA_8x4].prim   = fncdef PFX(fname ## _8x4_ ## neon); \
    p.pu[LUMA_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.pu[LUMA_8x8].prim   = fncdef PFX(fname ## _8x8_ ## neon); \
    p.pu[LUMA_16x8].prim  = fncdef PFX(fname ## _16x8_ ## neon); \
    p.pu[LUMA_8x16].prim  = fncdef PFX(fname ## _8x16_ ## neon); \
    p.pu[LUMA_16x16].prim = fncdef PFX(fname ## _16x16_ ## neon); \
    p.pu[LUMA_16x32].prim = fncdef PFX(fname ## _16x32_ ## neon); \
    p.pu[LUMA_16x12].prim = fncdef PFX(fname ## _16x12_ ## neon); \
    p.pu[LUMA_16x4].prim  = fncdef PFX(fname ## _16x4_ ## neon); \
    p.pu[LUMA_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon); \
    p.pu[LUMA_8x32].prim  = fncdef PFX(fname ## _8x32_ ## neon); \
    p.pu[LUMA_16x64].prim = fncdef PFX(fname ## _16x64_ ## neon)
#define LUMA_PU_TYPED_MULTIPLE_ARCHS_3(prim, fncdef, fname, cpu) \
    p.pu[LUMA_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.pu[LUMA_64x64].prim = fncdef PFX(fname ## _64x64_ ## cpu); \
    p.pu[LUMA_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.pu[LUMA_64x32].prim = fncdef PFX(fname ## _64x32_ ## cpu); \
    p.pu[LUMA_32x64].prim = fncdef PFX(fname ## _32x64_ ## cpu); \
    p.pu[LUMA_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.pu[LUMA_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.pu[LUMA_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.pu[LUMA_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.pu[LUMA_64x48].prim = fncdef PFX(fname ## _64x48_ ## cpu); \
    p.pu[LUMA_48x64].prim = fncdef PFX(fname ## _48x64_ ## cpu); \
    p.pu[LUMA_64x16].prim = fncdef PFX(fname ## _64x16_ ## cpu)
#define LUMA_PU_TYPED_NEON_3(prim, fncdef, fname) \
    p.pu[LUMA_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.pu[LUMA_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.pu[LUMA_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon)
#define LUMA_PU_TYPED_CAN_USE_SVE2(prim, fncdef, fname) \
    p.pu[LUMA_8x8].prim   = fncdef PFX(fname ## _8x8_ ## sve2); \
    p.pu[LUMA_16x16].prim = fncdef PFX(fname ## _16x16_ ## sve2); \
    p.pu[LUMA_32x32].prim = fncdef PFX(fname ## _32x32_ ## sve2); \
    p.pu[LUMA_64x64].prim = fncdef PFX(fname ## _64x64_ ## sve2); \
    p.pu[LUMA_8x4].prim   = fncdef PFX(fname ## _8x4_ ## sve2); \
    p.pu[LUMA_16x8].prim  = fncdef PFX(fname ## _16x8_ ## sve2); \
    p.pu[LUMA_8x16].prim  = fncdef PFX(fname ## _8x16_ ## sve2); \
    p.pu[LUMA_16x32].prim = fncdef PFX(fname ## _16x32_ ## sve2); \
    p.pu[LUMA_32x16].prim = fncdef PFX(fname ## _32x16_ ## sve2); \
    p.pu[LUMA_64x32].prim = fncdef PFX(fname ## _64x32_ ## sve2); \
    p.pu[LUMA_32x64].prim = fncdef PFX(fname ## _32x64_ ## sve2); \
    p.pu[LUMA_16x12].prim = fncdef PFX(fname ## _16x12_ ## sve2); \
    p.pu[LUMA_12x16].prim = fncdef PFX(fname ## _12x16_ ## sve2); \
    p.pu[LUMA_16x4].prim  = fncdef PFX(fname ## _16x4_ ## sve2); \
    p.pu[LUMA_32x24].prim = fncdef PFX(fname ## _32x24_ ## sve2); \
    p.pu[LUMA_24x32].prim = fncdef PFX(fname ## _24x32_ ## sve2); \
    p.pu[LUMA_32x8].prim  = fncdef PFX(fname ## _32x8_ ## sve2); \
    p.pu[LUMA_8x32].prim  = fncdef PFX(fname ## _8x32_ ## sve2); \
    p.pu[LUMA_64x48].prim = fncdef PFX(fname ## _64x48_ ## sve2); \
    p.pu[LUMA_48x64].prim = fncdef PFX(fname ## _48x64_ ## sve2); \
    p.pu[LUMA_64x16].prim = fncdef PFX(fname ## _64x16_ ## sve2); \
    p.pu[LUMA_16x64].prim = fncdef PFX(fname ## _16x64_ ## sve2)
#define LUMA_PU_TYPED_NEON_FILTER_PIXEL_TO_SHORT(prim, fncdef) \
    p.pu[LUMA_4x4].prim   = fncdef PFX(filterPixelToShort ## _4x4_ ## neon); \
    p.pu[LUMA_8x8].prim   = fncdef PFX(filterPixelToShort ## _8x8_ ## neon); \
    p.pu[LUMA_16x16].prim = fncdef PFX(filterPixelToShort ## _16x16_ ## neon); \
    p.pu[LUMA_8x4].prim   = fncdef PFX(filterPixelToShort ## _8x4_ ## neon); \
    p.pu[LUMA_4x8].prim   = fncdef PFX(filterPixelToShort ## _4x8_ ## neon); \
    p.pu[LUMA_16x8].prim  = fncdef PFX(filterPixelToShort ## _16x8_ ## neon); \
    p.pu[LUMA_8x16].prim  = fncdef PFX(filterPixelToShort ## _8x16_ ## neon); \
    p.pu[LUMA_16x32].prim = fncdef PFX(filterPixelToShort ## _16x32_ ## neon); \
    p.pu[LUMA_16x12].prim = fncdef PFX(filterPixelToShort ## _16x12_ ## neon); \
    p.pu[LUMA_12x16].prim = fncdef PFX(filterPixelToShort ## _12x16_ ## neon); \
    p.pu[LUMA_16x4].prim  = fncdef PFX(filterPixelToShort ## _16x4_ ## neon); \
    p.pu[LUMA_4x16].prim  = fncdef PFX(filterPixelToShort ## _4x16_ ## neon); \
    p.pu[LUMA_24x32].prim = fncdef PFX(filterPixelToShort ## _24x32_ ## neon); \
    p.pu[LUMA_8x32].prim  = fncdef PFX(filterPixelToShort ## _8x32_ ## neon); \
    p.pu[LUMA_16x64].prim = fncdef PFX(filterPixelToShort ## _16x64_ ## neon)
#define LUMA_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, fncdef) \
    p.pu[LUMA_32x32].prim = fncdef PFX(filterPixelToShort ## _32x32_ ## sve); \
    p.pu[LUMA_32x16].prim = fncdef PFX(filterPixelToShort ## _32x16_ ## sve); \
    p.pu[LUMA_32x64].prim = fncdef PFX(filterPixelToShort ## _32x64_ ## sve); \
    p.pu[LUMA_32x24].prim = fncdef PFX(filterPixelToShort ## _32x24_ ## sve); \
    p.pu[LUMA_32x8].prim  = fncdef PFX(filterPixelToShort ## _32x8_ ## sve); \
    p.pu[LUMA_64x64].prim = fncdef PFX(filterPixelToShort ## _64x64_ ## sve); \
    p.pu[LUMA_64x32].prim = fncdef PFX(filterPixelToShort ## _64x32_ ## sve); \
    p.pu[LUMA_64x48].prim = fncdef PFX(filterPixelToShort ## _64x48_ ## sve); \
    p.pu[LUMA_64x16].prim = fncdef PFX(filterPixelToShort ## _64x16_ ## sve); \
    p.pu[LUMA_48x64].prim = fncdef PFX(filterPixelToShort ## _48x64_ ## sve)
#define ALL_LUMA_PU(prim, fname, cpu) ALL_LUMA_PU_TYPED(prim, , fname, cpu)
#define LUMA_PU_MULTIPLE_ARCHS_1(prim, fname, cpu) LUMA_PU_TYPED_MULTIPLE_ARCHS_1(prim, , fname, cpu)
#define LUMA_PU_MULTIPLE_ARCHS_2(prim, fname, cpu) LUMA_PU_TYPED_MULTIPLE_ARCHS_2(prim, , fname, cpu)
#define LUMA_PU_NEON_1(prim, fname) LUMA_PU_TYPED_NEON_1(prim, , fname)
#define LUMA_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, fname) LUMA_PU_TYPED_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, , fname)
#define LUMA_PU_NEON_2(prim, fname) LUMA_PU_TYPED_NEON_2(prim, , fname)
#define LUMA_PU_MULTIPLE_ARCHS_3(prim, fname, cpu) LUMA_PU_TYPED_MULTIPLE_ARCHS_3(prim, , fname, cpu)
#define LUMA_PU_NEON_3(prim, fname) LUMA_PU_TYPED_NEON_3(prim, , fname)
#define LUMA_PU_CAN_USE_SVE2(prim, fname) LUMA_PU_TYPED_CAN_USE_SVE2(prim, , fname)
#define LUMA_PU_NEON_FILTER_PIXEL_TO_SHORT(prim) LUMA_PU_TYPED_NEON_FILTER_PIXEL_TO_SHORT(prim, )
#define LUMA_PU_SVE_FILTER_PIXEL_TO_SHORT(prim) LUMA_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, )


#define ALL_LUMA_PU_T(prim, fname) \
    p.pu[LUMA_4x4].prim   = fname<LUMA_4x4>; \
    p.pu[LUMA_8x8].prim   = fname<LUMA_8x8>; \
    p.pu[LUMA_16x16].prim = fname<LUMA_16x16>; \
    p.pu[LUMA_32x32].prim = fname<LUMA_32x32>; \
    p.pu[LUMA_64x64].prim = fname<LUMA_64x64>; \
    p.pu[LUMA_8x4].prim   = fname<LUMA_8x4>; \
    p.pu[LUMA_4x8].prim   = fname<LUMA_4x8>; \
    p.pu[LUMA_16x8].prim  = fname<LUMA_16x8>; \
    p.pu[LUMA_8x16].prim  = fname<LUMA_8x16>; \
    p.pu[LUMA_16x32].prim = fname<LUMA_16x32>; \
    p.pu[LUMA_32x16].prim = fname<LUMA_32x16>; \
    p.pu[LUMA_64x32].prim = fname<LUMA_64x32>; \
    p.pu[LUMA_32x64].prim = fname<LUMA_32x64>; \
    p.pu[LUMA_16x12].prim = fname<LUMA_16x12>; \
    p.pu[LUMA_12x16].prim = fname<LUMA_12x16>; \
    p.pu[LUMA_16x4].prim  = fname<LUMA_16x4>; \
    p.pu[LUMA_4x16].prim  = fname<LUMA_4x16>; \
    p.pu[LUMA_32x24].prim = fname<LUMA_32x24>; \
    p.pu[LUMA_24x32].prim = fname<LUMA_24x32>; \
    p.pu[LUMA_32x8].prim  = fname<LUMA_32x8>; \
    p.pu[LUMA_8x32].prim  = fname<LUMA_8x32>; \
    p.pu[LUMA_64x48].prim = fname<LUMA_64x48>; \
    p.pu[LUMA_48x64].prim = fname<LUMA_48x64>; \
    p.pu[LUMA_64x16].prim = fname<LUMA_64x16>; \
    p.pu[LUMA_16x64].prim = fname<LUMA_16x64>

#define ALL_CHROMA_420_PU_TYPED(prim, fncdef, fname, cpu)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x2].prim   = fncdef PFX(fname ## _4x2_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x4].prim   = fncdef PFX(fname ## _2x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].prim   = fncdef PFX(fname ## _4x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x6].prim   = fncdef PFX(fname ## _8x6_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_6x8].prim   = fncdef PFX(fname ## _6x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x2].prim   = fncdef PFX(fname ## _8x2_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x8].prim   = fncdef PFX(fname ## _2x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].prim = fncdef PFX(fname ## _16x12_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].prim  = fncdef PFX(fname ## _16x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].prim  = fncdef PFX(fname ## _4x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu)
#define CHROMA_420_PU_TYPED_NEON_1(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x2].prim   = fncdef PFX(fname ## _4x2_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_6x8].prim   = fncdef PFX(fname ## _6x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].prim = fncdef PFX(fname ## _12x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].prim = fncdef PFX(fname ## _32x24_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].prim = fncdef PFX(fname ## _24x32_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].prim  = fncdef PFX(fname ## _32x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].prim  = fncdef PFX(fname ## _8x32_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].prim   = fncdef PFX(fname ## _8x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].prim = fncdef PFX(fname ## _16x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x4].prim   = fncdef PFX(fname ## _2x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].prim   = fncdef PFX(fname ## _8x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].prim  = fncdef PFX(fname ## _16x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].prim  = fncdef PFX(fname ## _8x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].prim = fncdef PFX(fname ## _16x32_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x6].prim   = fncdef PFX(fname ## _8x6_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x2].prim   = fncdef PFX(fname ## _8x2_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x8].prim   = fncdef PFX(fname ## _2x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].prim = fncdef PFX(fname ## _16x12_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].prim  = fncdef PFX(fname ## _16x4_ ## neon)
#define CHROMA_420_PU_TYPED_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].prim = fncdef PFX(fname ## _32x32_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].prim = fncdef PFX(fname ## _32x16_ ## sve)
#define CHROMA_420_PU_TYPED_NEON_2(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x2].prim   = fncdef PFX(fname ## _4x2_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon)
#define CHROMA_420_PU_TYPED_MULTIPLE_ARCHS(prim, fncdef, fname, cpu)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x4].prim   = fncdef PFX(fname ## _2x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x6].prim   = fncdef PFX(fname ## _8x6_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_6x8].prim   = fncdef PFX(fname ## _6x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x2].prim   = fncdef PFX(fname ## _8x2_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x8].prim   = fncdef PFX(fname ## _2x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].prim = fncdef PFX(fname ## _16x12_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].prim  = fncdef PFX(fname ## _16x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu)
#define CHROMA_420_PU_TYPED_FILTER_PIXEL_TO_SHORT_NEON(prim, fncdef)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].prim   = fncdef PFX(filterPixelToShort ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].prim   = fncdef PFX(filterPixelToShort ## _8x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].prim = fncdef PFX(filterPixelToShort ## _16x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].prim   = fncdef PFX(filterPixelToShort ## _8x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].prim   = fncdef PFX(filterPixelToShort ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].prim  = fncdef PFX(filterPixelToShort ## _16x8_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].prim  = fncdef PFX(filterPixelToShort ## _8x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].prim = fncdef PFX(filterPixelToShort ## _16x32_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x6].prim   = fncdef PFX(filterPixelToShort ## _8x6_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x2].prim   = fncdef PFX(filterPixelToShort ## _8x2_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].prim = fncdef PFX(filterPixelToShort ## _16x12_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].prim = fncdef PFX(filterPixelToShort ## _12x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].prim  = fncdef PFX(filterPixelToShort ## _16x4_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].prim  = fncdef PFX(filterPixelToShort ## _4x16_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].prim = fncdef PFX(filterPixelToShort ## _24x32_ ## neon); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].prim  = fncdef PFX(filterPixelToShort ## _8x32_ ## neon)
#define CHROMA_420_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, fncdef)               \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x4].prim   = fncdef PFX(filterPixelToShort ## _2x4_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_2x8].prim   = fncdef PFX(filterPixelToShort ## _2x8_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_6x8].prim   = fncdef PFX(filterPixelToShort ## _6x8_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x2].prim   = fncdef PFX(filterPixelToShort ## _4x2_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].prim = fncdef PFX(filterPixelToShort ## _32x32_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].prim = fncdef PFX(filterPixelToShort ## _32x16_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].prim = fncdef PFX(filterPixelToShort ## _32x24_ ## sve); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].prim  = fncdef PFX(filterPixelToShort ## _32x8_ ## sve)
#define ALL_CHROMA_420_PU(prim, fname, cpu) ALL_CHROMA_420_PU_TYPED(prim, , fname, cpu)
#define CHROMA_420_PU_NEON_1(prim, fname) CHROMA_420_PU_TYPED_NEON_1(prim, , fname)
#define CHROMA_420_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, fname) CHROMA_420_PU_TYPED_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, , fname)
#define CHROMA_420_PU_NEON_2(prim, fname) CHROMA_420_PU_TYPED_NEON_2(prim, , fname)
#define CHROMA_420_PU_MULTIPLE_ARCHS(prim, fname, cpu) CHROMA_420_PU_TYPED_MULTIPLE_ARCHS(prim, , fname, cpu)
#define CHROMA_420_PU_FILTER_PIXEL_TO_SHORT_NEON(prim) CHROMA_420_PU_TYPED_FILTER_PIXEL_TO_SHORT_NEON(prim, )
#define CHROMA_420_PU_SVE_FILTER_PIXEL_TO_SHORT(prim) CHROMA_420_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, )


#define ALL_CHROMA_420_4x4_PU_TYPED(prim, fncdef, fname, cpu) \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x2].prim   = fncdef PFX(fname ## _8x2_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x6].prim   = fncdef PFX(fname ## _8x6_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].prim   = fncdef PFX(fname ## _4x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].prim = fncdef PFX(fname ## _16x12_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].prim  = fncdef PFX(fname ## _16x4_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].prim  = fncdef PFX(fname ## _4x16_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu)
#define ALL_CHROMA_420_4x4_PU(prim, fname, cpu) ALL_CHROMA_420_4x4_PU_TYPED(prim, , fname, cpu)

#define ALL_CHROMA_422_PU_TYPED(prim, fncdef, fname, cpu)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].prim   = fncdef PFX(fname ## _4x8_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].prim = fncdef PFX(fname ## _32x64_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x8].prim   = fncdef PFX(fname ## _2x8_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].prim  = fncdef PFX(fname ## _4x16_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].prim = fncdef PFX(fname ## _16x64_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].prim  = fncdef PFX(fname ## _8x12_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_6x16].prim  = fncdef PFX(fname ## _6x16_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x16].prim  = fncdef PFX(fname ## _2x16_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].prim = fncdef PFX(fname ## _16x24_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].prim = fncdef PFX(fname ## _12x32_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].prim  = fncdef PFX(fname ## _4x32_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].prim = fncdef PFX(fname ## _32x48_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].prim = fncdef PFX(fname ## _24x64_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].prim  = fncdef PFX(fname ## _8x64_ ## cpu)
#define CHROMA_422_PU_TYPED_NEON_1(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_6x16].prim  = fncdef PFX(fname ## _6x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].prim = fncdef PFX(fname ## _12x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].prim  = fncdef PFX(fname ## _4x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].prim  = fncdef PFX(fname ## _8x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].prim = fncdef PFX(fname ## _16x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x8].prim   = fncdef PFX(fname ## _2x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].prim   = fncdef PFX(fname ## _8x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].prim = fncdef PFX(fname ## _16x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].prim  = fncdef PFX(fname ## _8x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].prim = fncdef PFX(fname ## _16x64_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].prim  = fncdef PFX(fname ## _8x12_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].prim   = fncdef PFX(fname ## _8x4_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x16].prim  = fncdef PFX(fname ## _2x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].prim = fncdef PFX(fname ## _16x24_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].prim  = fncdef PFX(fname ## _16x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].prim = fncdef PFX(fname ## _24x64_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].prim  = fncdef PFX(fname ## _8x64_ ## neon)
#define CHROMA_422_PU_TYPED_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].prim = fncdef PFX(fname ## _32x64_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].prim = fncdef PFX(fname ## _32x32_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].prim = fncdef PFX(fname ## _32x48_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].prim = fncdef PFX(fname ## _32x16_ ## sve)
#define CHROMA_422_PU_TYPED_NEON_2(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].prim   = fncdef PFX(fname ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].prim   = fncdef PFX(fname ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].prim  = fncdef PFX(fname ## _4x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].prim  = fncdef PFX(fname ## _4x32_ ## neon)
#define CHROMA_422_PU_TYPED_CAN_USE_SVE2(prim, fncdef, fname)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].prim  = fncdef PFX(fname ## _8x16_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].prim = fncdef PFX(fname ## _16x32_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].prim = fncdef PFX(fname ## _32x64_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x8].prim   = fncdef PFX(fname ## _2x8_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].prim   = fncdef PFX(fname ## _8x8_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].prim = fncdef PFX(fname ## _16x16_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].prim  = fncdef PFX(fname ## _8x32_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].prim = fncdef PFX(fname ## _32x32_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].prim = fncdef PFX(fname ## _16x64_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].prim  = fncdef PFX(fname ## _8x12_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_6x16].prim  = fncdef PFX(fname ## _6x16_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].prim   = fncdef PFX(fname ## _8x4_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x16].prim  = fncdef PFX(fname ## _2x16_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].prim = fncdef PFX(fname ## _16x24_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].prim = fncdef PFX(fname ## _12x32_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].prim  = fncdef PFX(fname ## _16x8_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].prim = fncdef PFX(fname ## _32x48_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].prim = fncdef PFX(fname ## _24x64_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].prim = fncdef PFX(fname ## _32x16_ ## sve2); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].prim  = fncdef PFX(fname ## _8x64_ ## sve2)
#define CHROMA_422_PU_TYPED_NEON_FILTER_PIXEL_TO_SHORT(prim, fncdef)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].prim   = fncdef PFX(filterPixelToShort ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].prim  = fncdef PFX(filterPixelToShort ## _8x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].prim = fncdef PFX(filterPixelToShort ## _16x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].prim   = fncdef PFX(filterPixelToShort ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].prim   = fncdef PFX(filterPixelToShort ## _8x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].prim  = fncdef PFX(filterPixelToShort ## _4x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].prim = fncdef PFX(filterPixelToShort ## _16x16_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].prim  = fncdef PFX(filterPixelToShort ## _8x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].prim = fncdef PFX(filterPixelToShort ## _16x64_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].prim  = fncdef PFX(filterPixelToShort ## _8x12_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].prim   = fncdef PFX(filterPixelToShort ## _8x4_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].prim = fncdef PFX(filterPixelToShort ## _16x24_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].prim = fncdef PFX(filterPixelToShort ## _12x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].prim  = fncdef PFX(filterPixelToShort ## _16x8_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].prim  = fncdef PFX(filterPixelToShort ## _4x32_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].prim = fncdef PFX(filterPixelToShort ## _24x64_ ## neon); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].prim  = fncdef PFX(filterPixelToShort ## _8x64_ ## neon)
#define CHROMA_422_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, fncdef)               \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x8].prim   = fncdef PFX(filterPixelToShort ## _2x8_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_2x16].prim  = fncdef PFX(filterPixelToShort ## _2x16_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_6x16].prim  = fncdef PFX(filterPixelToShort ## _6x16_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].prim = fncdef PFX(filterPixelToShort ## _32x64_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].prim = fncdef PFX(filterPixelToShort ## _32x32_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].prim = fncdef PFX(filterPixelToShort ## _32x48_ ## sve); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].prim = fncdef PFX(filterPixelToShort ## _32x16_ ## sve)
#define ALL_CHROMA_422_PU(prim, fname, cpu) ALL_CHROMA_422_PU_TYPED(prim, , fname, cpu)
#define CHROMA_422_PU_NEON_1(prim, fname) CHROMA_422_PU_TYPED_NEON_1(prim, , fname)
#define CHROMA_422_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, fname) CHROMA_422_PU_TYPED_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(prim, , fname)
#define CHROMA_422_PU_NEON_2(prim, fname) CHROMA_422_PU_TYPED_NEON_2(prim, , fname)
#define CHROMA_422_PU_CAN_USE_SVE2(prim, fname) CHROMA_422_PU_TYPED_CAN_USE_SVE2(prim, , fname)
#define CHROMA_422_PU_NEON_FILTER_PIXEL_TO_SHORT(prim) CHROMA_422_PU_TYPED_NEON_FILTER_PIXEL_TO_SHORT(prim, )
#define CHROMA_422_PU_SVE_FILTER_PIXEL_TO_SHORT(prim) CHROMA_422_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, )

#define ALL_CHROMA_444_PU_TYPED(prim, fncdef, fname, cpu) \
    p.chroma[X265_CSP_I444].pu[LUMA_4x4].prim   = fncdef PFX(fname ## _4x4_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x8].prim   = fncdef PFX(fname ## _8x8_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x16].prim = fncdef PFX(fname ## _16x16_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x32].prim = fncdef PFX(fname ## _32x32_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x64].prim = fncdef PFX(fname ## _64x64_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x4].prim   = fncdef PFX(fname ## _8x4_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_4x8].prim   = fncdef PFX(fname ## _4x8_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x8].prim  = fncdef PFX(fname ## _16x8_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x16].prim  = fncdef PFX(fname ## _8x16_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x32].prim = fncdef PFX(fname ## _16x32_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x16].prim = fncdef PFX(fname ## _32x16_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x32].prim = fncdef PFX(fname ## _64x32_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x64].prim = fncdef PFX(fname ## _32x64_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x12].prim = fncdef PFX(fname ## _16x12_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_12x16].prim = fncdef PFX(fname ## _12x16_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x4].prim  = fncdef PFX(fname ## _16x4_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_4x16].prim  = fncdef PFX(fname ## _4x16_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x24].prim = fncdef PFX(fname ## _32x24_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_24x32].prim = fncdef PFX(fname ## _24x32_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x8].prim  = fncdef PFX(fname ## _32x8_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x32].prim  = fncdef PFX(fname ## _8x32_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x48].prim = fncdef PFX(fname ## _64x48_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_48x64].prim = fncdef PFX(fname ## _48x64_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x16].prim = fncdef PFX(fname ## _64x16_ ## cpu); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x64].prim = fncdef PFX(fname ## _16x64_ ## cpu)
#define CHROMA_444_PU_TYPED_NEON_FILTER_PIXEL_TO_SHORT(prim, fncdef) \
    p.chroma[X265_CSP_I444].pu[LUMA_4x4].prim   = fncdef PFX(filterPixelToShort ## _4x4_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x8].prim   = fncdef PFX(filterPixelToShort ## _8x8_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x16].prim = fncdef PFX(filterPixelToShort ## _16x16_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x4].prim   = fncdef PFX(filterPixelToShort ## _8x4_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_4x8].prim   = fncdef PFX(filterPixelToShort ## _4x8_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x8].prim  = fncdef PFX(filterPixelToShort ## _16x8_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x16].prim  = fncdef PFX(filterPixelToShort ## _8x16_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x32].prim = fncdef PFX(filterPixelToShort ## _16x32_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x12].prim = fncdef PFX(filterPixelToShort ## _16x12_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_12x16].prim = fncdef PFX(filterPixelToShort ## _12x16_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x4].prim  = fncdef PFX(filterPixelToShort ## _16x4_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_4x16].prim  = fncdef PFX(filterPixelToShort ## _4x16_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_24x32].prim = fncdef PFX(filterPixelToShort ## _24x32_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_8x32].prim  = fncdef PFX(filterPixelToShort ## _8x32_ ## neon); \
    p.chroma[X265_CSP_I444].pu[LUMA_16x64].prim = fncdef PFX(filterPixelToShort ## _16x64_ ## neon)
#define CHROMA_444_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, fncdef) \
    p.chroma[X265_CSP_I444].pu[LUMA_32x32].prim = fncdef PFX(filterPixelToShort ## _32x32_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x16].prim = fncdef PFX(filterPixelToShort ## _32x16_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x64].prim = fncdef PFX(filterPixelToShort ## _32x64_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x24].prim = fncdef PFX(filterPixelToShort ## _32x24_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_32x8].prim  = fncdef PFX(filterPixelToShort ## _32x8_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x64].prim = fncdef PFX(filterPixelToShort ## _64x64_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x32].prim = fncdef PFX(filterPixelToShort ## _64x32_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x48].prim = fncdef PFX(filterPixelToShort ## _64x48_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_64x16].prim = fncdef PFX(filterPixelToShort ## _64x16_ ## sve); \
    p.chroma[X265_CSP_I444].pu[LUMA_48x64].prim = fncdef PFX(filterPixelToShort ## _48x64_ ## sve)
#define ALL_CHROMA_444_PU(prim, fname, cpu) ALL_CHROMA_444_PU_TYPED(prim, , fname, cpu)
#define CHROMA_444_PU_NEON_FILTER_PIXEL_TO_SHORT(prim) CHROMA_444_PU_TYPED_NEON_FILTER_PIXEL_TO_SHORT(prim, )
#define CHROMA_444_PU_SVE_FILTER_PIXEL_TO_SHORT(prim) CHROMA_444_PU_TYPED_SVE_FILTER_PIXEL_TO_SHORT(prim, )

#define ALL_CHROMA_420_VERT_FILTERS(cpu)                             \
    ALL_CHROMA_420_4x4_PU(filter_vpp, interp_4tap_vert_pp, cpu); \
    ALL_CHROMA_420_4x4_PU(filter_vps, interp_4tap_vert_ps, cpu); \
    ALL_CHROMA_420_4x4_PU(filter_vsp, interp_4tap_vert_sp, cpu); \
    ALL_CHROMA_420_4x4_PU(filter_vss, interp_4tap_vert_ss, cpu)

#define CHROMA_420_VERT_FILTERS_NEON()                             \
    ALL_CHROMA_420_4x4_PU(filter_vsp, interp_4tap_vert_sp, neon)

#define CHROMA_420_VERT_FILTERS_CAN_USE_SVE2()                             \
    ALL_CHROMA_420_4x4_PU(filter_vpp, interp_4tap_vert_pp, sve2); \
    ALL_CHROMA_420_4x4_PU(filter_vps, interp_4tap_vert_ps, sve2); \
    ALL_CHROMA_420_4x4_PU(filter_vss, interp_4tap_vert_ss, sve2)

#define SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(W, H) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vsp = PFX(interp_4tap_vert_sp_ ## W ## x ## H ## _ ## neon)

#define SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(W, H, cpu) \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vpp = PFX(interp_4tap_vert_pp_ ## W ## x ## H ## _ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vps = PFX(interp_4tap_vert_ps_ ## W ## x ## H ## _ ## cpu); \
    p.chroma[X265_CSP_I422].pu[CHROMA_422_ ## W ## x ## H].filter_vss = PFX(interp_4tap_vert_ss_ ## W ## x ## H ## _ ## cpu)

#define CHROMA_422_VERT_FILTERS_NEON() \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(4, 8); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(8, 16); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(8, 8); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(4, 16); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(8, 12); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(8, 4); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(16, 32); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(16, 16); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(8, 32); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(16, 24); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(12, 32); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(16, 8); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(4, 32); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(32, 64); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(32, 32); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(16, 64); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(32, 48); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(24, 64); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(32, 16); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_NEON(8, 64)

#define CHROMA_422_VERT_FILTERS_CAN_USE_SVE2(cpu) \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(4, 8, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(8, 16, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(8, 8, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(4, 16, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(8, 12, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(8, 4, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(16, 32, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(16, 16, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(8, 32, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(16, 24, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(12, 32, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(16, 8, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(4, 32, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(32, 64, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(32, 32, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(16, 64, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(32, 48, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(24, 64, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(32, 16, cpu); \
    SETUP_CHROMA_422_VERT_FUNC_DEF_CAN_USE_SVE2(8, 64, cpu)

#define ALL_CHROMA_444_VERT_FILTERS(cpu) \
    ALL_CHROMA_444_PU(filter_vpp, interp_4tap_vert_pp, cpu); \
    ALL_CHROMA_444_PU(filter_vps, interp_4tap_vert_ps, cpu); \
    ALL_CHROMA_444_PU(filter_vsp, interp_4tap_vert_sp, cpu); \
    ALL_CHROMA_444_PU(filter_vss, interp_4tap_vert_ss, cpu)

#define CHROMA_444_VERT_FILTERS_NEON() \
    ALL_CHROMA_444_PU(filter_vsp, interp_4tap_vert_sp, neon)

#define CHROMA_444_VERT_FILTERS_CAN_USE_SVE2() \
    ALL_CHROMA_444_PU(filter_vpp, interp_4tap_vert_pp, sve2); \
    ALL_CHROMA_444_PU(filter_vps, interp_4tap_vert_ps, sve2); \
    ALL_CHROMA_444_PU(filter_vss, interp_4tap_vert_ss, sve2)

#define ALL_CHROMA_420_FILTERS(cpu)                               \
    ALL_CHROMA_420_PU(filter_hpp, interp_4tap_horiz_pp, cpu); \
    ALL_CHROMA_420_PU(filter_hps, interp_4tap_horiz_ps, cpu); \
    ALL_CHROMA_420_PU(filter_vpp, interp_4tap_vert_pp, cpu);  \
    ALL_CHROMA_420_PU(filter_vps, interp_4tap_vert_ps, cpu)

#define CHROMA_420_FILTERS_NEON()                               \
    ALL_CHROMA_420_PU(filter_hpp, interp_4tap_horiz_pp, neon); \
    ALL_CHROMA_420_PU(filter_hps, interp_4tap_horiz_ps, neon)

#define CHROMA_420_FILTERS_CAN_USE_SVE2()                               \
    ALL_CHROMA_420_PU(filter_vpp, interp_4tap_vert_pp, sve2);  \
    ALL_CHROMA_420_PU(filter_vps, interp_4tap_vert_ps, sve2)

#define ALL_CHROMA_422_FILTERS(cpu) \
    ALL_CHROMA_422_PU(filter_hpp, interp_4tap_horiz_pp, cpu); \
    ALL_CHROMA_422_PU(filter_hps, interp_4tap_horiz_ps, cpu); \
    ALL_CHROMA_422_PU(filter_vpp, interp_4tap_vert_pp, cpu);  \
    ALL_CHROMA_422_PU(filter_vps, interp_4tap_vert_ps, cpu)

#define CHROMA_422_FILTERS_NEON() \
    ALL_CHROMA_422_PU(filter_hpp, interp_4tap_horiz_pp, neon); \
    ALL_CHROMA_422_PU(filter_hps, interp_4tap_horiz_ps, neon)

#define CHROMA_422_FILTERS_CAN_USE_SVE2() \
    ALL_CHROMA_422_PU(filter_vpp, interp_4tap_vert_pp, sve2);  \
    ALL_CHROMA_422_PU(filter_vps, interp_4tap_vert_ps, sve2)

#define ALL_CHROMA_444_FILTERS(cpu) \
    ALL_CHROMA_444_PU(filter_hpp, interp_4tap_horiz_pp, cpu); \
    ALL_CHROMA_444_PU(filter_hps, interp_4tap_horiz_ps, cpu); \
    ALL_CHROMA_444_PU(filter_vpp, interp_4tap_vert_pp, cpu);  \
    ALL_CHROMA_444_PU(filter_vps, interp_4tap_vert_ps, cpu)

#define CHROMA_444_FILTERS_NEON() \
    ALL_CHROMA_444_PU(filter_hpp, interp_4tap_horiz_pp, neon); \
    ALL_CHROMA_444_PU(filter_hps, interp_4tap_horiz_ps, neon)

#define CHROMA_444_FILTERS_CAN_USE_SVE2() \
    ALL_CHROMA_444_PU(filter_vpp, interp_4tap_vert_pp, sve2);  \
    ALL_CHROMA_444_PU(filter_vps, interp_4tap_vert_ps, sve2)


#if defined(__GNUC__)
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#endif

#define GCC_4_9_0 40900
#define GCC_5_1_0 50100

#include "pixel-prim.h"
#include "filter-prim.h"
#include "dct-prim.h"
#include "loopfilter-prim.h"
#include "intrapred-prim.h"

namespace X265_NS
{
// private x265 namespace


template<int size>
void interp_8tap_hv_pp_cpu(const pixel *src, intptr_t srcStride, pixel *dst, intptr_t dstStride, int idxX, int idxY)
{
    ALIGN_VAR_32(int16_t, immed[MAX_CU_SIZE * (MAX_CU_SIZE + NTAPS_LUMA - 1)]);
    const int halfFilterSize = NTAPS_LUMA >> 1;
    const int immedStride = MAX_CU_SIZE;

    primitives.pu[size].luma_hps(src, srcStride, immed, immedStride, idxX, 1);
    primitives.pu[size].luma_vsp(immed + (halfFilterSize - 1) * immedStride, immedStride, dst, dstStride, idxY);
}

void setupNeonPrimitives(EncoderPrimitives &p)
{
    setupPixelPrimitives_neon(p);
    setupFilterPrimitives_neon(p);
    setupDCTPrimitives_neon(p);
    setupLoopFilterPrimitives_neon(p);
    setupIntraPrimitives_neon(p);

    ALL_CHROMA_420_PU(p2s[NONALIGNED], filterPixelToShort, neon);
    ALL_CHROMA_422_PU(p2s[ALIGNED], filterPixelToShort, neon);
    ALL_CHROMA_444_PU(p2s[ALIGNED], filterPixelToShort, neon);
    ALL_LUMA_PU(convert_p2s[ALIGNED], filterPixelToShort, neon);
    ALL_CHROMA_420_PU(p2s[ALIGNED], filterPixelToShort, neon);
    ALL_CHROMA_422_PU(p2s[NONALIGNED], filterPixelToShort, neon);
    ALL_CHROMA_444_PU(p2s[NONALIGNED], filterPixelToShort, neon);
    ALL_LUMA_PU(convert_p2s[NONALIGNED], filterPixelToShort, neon);

#if !HIGH_BIT_DEPTH
    ALL_LUMA_PU(luma_vpp, interp_8tap_vert_pp, neon);
    ALL_LUMA_PU(luma_vsp, interp_8tap_vert_sp, neon);
    ALL_LUMA_PU(luma_vps, interp_8tap_vert_ps, neon);
    ALL_LUMA_PU(luma_hpp, interp_horiz_pp, neon);
    ALL_LUMA_PU(luma_hps, interp_horiz_ps, neon);
    ALL_LUMA_PU(luma_vss, interp_8tap_vert_ss, neon);
    ALL_LUMA_PU_T(luma_hvpp, interp_8tap_hv_pp_cpu);
    ALL_CHROMA_420_VERT_FILTERS(neon);
    CHROMA_422_VERT_FILTERS_NEON();
    CHROMA_422_VERT_FILTERS_CAN_USE_SVE2(neon);
    ALL_CHROMA_444_VERT_FILTERS(neon);
    ALL_CHROMA_420_FILTERS(neon);
    ALL_CHROMA_422_FILTERS(neon);
    ALL_CHROMA_444_FILTERS(neon);

    // Blockcopy_pp
    ALL_LUMA_PU(copy_pp, blockcopy_pp, neon);
    ALL_CHROMA_420_PU(copy_pp, blockcopy_pp, neon);
    ALL_CHROMA_422_PU(copy_pp, blockcopy_pp, neon);
    p.cu[BLOCK_4x4].copy_pp   = PFX(blockcopy_pp_4x4_neon);
    p.cu[BLOCK_8x8].copy_pp   = PFX(blockcopy_pp_8x8_neon);
    p.cu[BLOCK_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    p.cu[BLOCK_32x32].copy_pp = PFX(blockcopy_pp_32x32_neon);
    p.cu[BLOCK_64x64].copy_pp = PFX(blockcopy_pp_64x64_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_pp = PFX(blockcopy_pp_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_pp = PFX(blockcopy_pp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_pp = PFX(blockcopy_pp_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_pp = PFX(blockcopy_pp_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_pp = PFX(blockcopy_pp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_pp = PFX(blockcopy_pp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_pp = PFX(blockcopy_pp_32x64_neon);

#endif // !HIGH_BIT_DEPTH

    // Blockcopy_ss
    p.cu[BLOCK_4x4].copy_ss   = PFX(blockcopy_ss_4x4_neon);
    p.cu[BLOCK_8x8].copy_ss   = PFX(blockcopy_ss_8x8_neon);
    p.cu[BLOCK_16x16].copy_ss = PFX(blockcopy_ss_16x16_neon);
    p.cu[BLOCK_32x32].copy_ss = PFX(blockcopy_ss_32x32_neon);
    p.cu[BLOCK_64x64].copy_ss = PFX(blockcopy_ss_64x64_neon);

    // Blockcopy_ps
    p.cu[BLOCK_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
    p.cu[BLOCK_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
    p.cu[BLOCK_16x16].copy_ps = PFX(blockcopy_ps_16x16_neon);
    p.cu[BLOCK_32x32].copy_ps = PFX(blockcopy_ps_32x32_neon);
    p.cu[BLOCK_64x64].copy_ps = PFX(blockcopy_ps_64x64_neon);

    // Blockcopy_sp
    p.cu[BLOCK_4x4].copy_sp   = PFX(blockcopy_sp_4x4_neon);
    p.cu[BLOCK_8x8].copy_sp   = PFX(blockcopy_sp_8x8_neon);
    p.cu[BLOCK_16x16].copy_sp = PFX(blockcopy_sp_16x16_neon);
    p.cu[BLOCK_32x32].copy_sp = PFX(blockcopy_sp_32x32_neon);
    p.cu[BLOCK_64x64].copy_sp = PFX(blockcopy_sp_64x64_neon);

    // chroma blockcopy_ss
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_ss   = PFX(blockcopy_ss_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_ss   = PFX(blockcopy_ss_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_ss = PFX(blockcopy_ss_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_ss = PFX(blockcopy_ss_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_ss   = PFX(blockcopy_ss_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_ss  = PFX(blockcopy_ss_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_ss = PFX(blockcopy_ss_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_ss = PFX(blockcopy_ss_32x64_neon);

    // chroma blockcopy_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_ps = PFX(blockcopy_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_ps = PFX(blockcopy_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_ps   = PFX(blockcopy_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_ps  = PFX(blockcopy_ps_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_ps = PFX(blockcopy_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_ps = PFX(blockcopy_ps_32x64_neon);

    // chroma blockcopy_sp
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_sp   = PFX(blockcopy_sp_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_sp   = PFX(blockcopy_sp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_sp = PFX(blockcopy_sp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_sp = PFX(blockcopy_sp_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_sp   = PFX(blockcopy_sp_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_sp  = PFX(blockcopy_sp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_sp = PFX(blockcopy_sp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_sp = PFX(blockcopy_sp_32x64_neon);

    // Block_fill
    ALL_LUMA_TU(blockfill_s[ALIGNED], blockfill_s, neon);
    ALL_LUMA_TU(blockfill_s[NONALIGNED], blockfill_s, neon);

    // copy_count
    p.cu[BLOCK_4x4].copy_cnt     = PFX(copy_cnt_4_neon);
    p.cu[BLOCK_8x8].copy_cnt     = PFX(copy_cnt_8_neon);
    p.cu[BLOCK_16x16].copy_cnt   = PFX(copy_cnt_16_neon);
    p.cu[BLOCK_32x32].copy_cnt   = PFX(copy_cnt_32_neon);

    // count nonzero
    p.cu[BLOCK_4x4].count_nonzero     = PFX(count_nonzero_4_neon);
    p.cu[BLOCK_8x8].count_nonzero     = PFX(count_nonzero_8_neon);
    p.cu[BLOCK_16x16].count_nonzero   = PFX(count_nonzero_16_neon);
    p.cu[BLOCK_32x32].count_nonzero   = PFX(count_nonzero_32_neon);

    // cpy2Dto1D_shl
    p.cu[BLOCK_4x4].cpy2Dto1D_shl   = PFX(cpy2Dto1D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy2Dto1D_shl   = PFX(cpy2Dto1D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_16x16_neon);
    p.cu[BLOCK_32x32].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_32x32_neon);
    p.cu[BLOCK_64x64].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_64x64_neon);

    // cpy2Dto1D_shr
    p.cu[BLOCK_4x4].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_4x4_neon);
    p.cu[BLOCK_8x8].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_8x8_neon);
    p.cu[BLOCK_16x16].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_16x16_neon);
    p.cu[BLOCK_32x32].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_32x32_neon);

    // cpy1Dto2D_shl
    p.cu[BLOCK_4x4].cpy1Dto2D_shl[ALIGNED]      = PFX(cpy1Dto2D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shl[ALIGNED]      = PFX(cpy1Dto2D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_16x16_neon);
    p.cu[BLOCK_32x32].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_32x32_neon);
    p.cu[BLOCK_64x64].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_64x64_neon);

    p.cu[BLOCK_4x4].cpy1Dto2D_shl[NONALIGNED]   = PFX(cpy1Dto2D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shl[NONALIGNED]   = PFX(cpy1Dto2D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_16x16_neon);
    p.cu[BLOCK_32x32].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_32x32_neon);
    p.cu[BLOCK_64x64].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_64x64_neon);

    // cpy1Dto2D_shr
    p.cu[BLOCK_4x4].cpy1Dto2D_shr   = PFX(cpy1Dto2D_shr_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shr   = PFX(cpy1Dto2D_shr_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_16x16_neon);
    p.cu[BLOCK_32x32].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_32x32_neon);
    p.cu[BLOCK_64x64].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_64x64_neon);

#if !HIGH_BIT_DEPTH
    // pixel_avg_pp
    ALL_LUMA_PU(pixelavg_pp[NONALIGNED], pixel_avg_pp, neon);
    ALL_LUMA_PU(pixelavg_pp[ALIGNED], pixel_avg_pp, neon);

    // addAvg
    ALL_LUMA_PU(addAvg[NONALIGNED], addAvg, neon);
    ALL_LUMA_PU(addAvg[ALIGNED], addAvg, neon);
    ALL_CHROMA_420_PU(addAvg[NONALIGNED], addAvg, neon);
    ALL_CHROMA_422_PU(addAvg[NONALIGNED], addAvg, neon);
    ALL_CHROMA_420_PU(addAvg[ALIGNED], addAvg, neon);
    ALL_CHROMA_422_PU(addAvg[ALIGNED], addAvg, neon);

    // sad
    ALL_LUMA_PU(sad, pixel_sad, neon);
    ALL_LUMA_PU(sad_x3, sad_x3, neon);
    ALL_LUMA_PU(sad_x4, sad_x4, neon);

    // sse_pp
    p.cu[BLOCK_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_neon);
    p.cu[BLOCK_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
    p.cu[BLOCK_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
    p.cu[BLOCK_32x32].sse_pp = PFX(pixel_sse_pp_32x32_neon);
    p.cu[BLOCK_64x64].sse_pp = PFX(pixel_sse_pp_64x64_neon);

    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].sse_pp = PFX(pixel_sse_pp_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].sse_pp   = PFX(pixel_sse_pp_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sse_pp  = PFX(pixel_sse_pp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sse_pp = PFX(pixel_sse_pp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sse_pp = PFX(pixel_sse_pp_32x64_neon);

    // sse_ss
    p.cu[BLOCK_4x4].sse_ss   = PFX(pixel_sse_ss_4x4_neon);
    p.cu[BLOCK_8x8].sse_ss   = PFX(pixel_sse_ss_8x8_neon);
    p.cu[BLOCK_16x16].sse_ss = PFX(pixel_sse_ss_16x16_neon);
    p.cu[BLOCK_32x32].sse_ss = PFX(pixel_sse_ss_32x32_neon);
    p.cu[BLOCK_64x64].sse_ss = PFX(pixel_sse_ss_64x64_neon);

    // ssd_s
    p.cu[BLOCK_4x4].ssd_s[NONALIGNED]   = PFX(pixel_ssd_s_4x4_neon);
    p.cu[BLOCK_8x8].ssd_s[NONALIGNED]   = PFX(pixel_ssd_s_8x8_neon);
    p.cu[BLOCK_16x16].ssd_s[NONALIGNED] = PFX(pixel_ssd_s_16x16_neon);
    p.cu[BLOCK_32x32].ssd_s[NONALIGNED] = PFX(pixel_ssd_s_32x32_neon);

    p.cu[BLOCK_4x4].ssd_s[ALIGNED]   = PFX(pixel_ssd_s_4x4_neon);
    p.cu[BLOCK_8x8].ssd_s[ALIGNED]   = PFX(pixel_ssd_s_8x8_neon);
    p.cu[BLOCK_16x16].ssd_s[ALIGNED] = PFX(pixel_ssd_s_16x16_neon);
    p.cu[BLOCK_32x32].ssd_s[ALIGNED] = PFX(pixel_ssd_s_32x32_neon);

    // pixel_var
    p.cu[BLOCK_8x8].var   = PFX(pixel_var_8x8_neon);
    p.cu[BLOCK_16x16].var = PFX(pixel_var_16x16_neon);
    p.cu[BLOCK_32x32].var = PFX(pixel_var_32x32_neon);
    p.cu[BLOCK_64x64].var = PFX(pixel_var_64x64_neon);

    // calc_Residual
    p.cu[BLOCK_4x4].calcresidual[NONALIGNED]   = PFX(getResidual4_neon);
    p.cu[BLOCK_8x8].calcresidual[NONALIGNED]   = PFX(getResidual8_neon);
    p.cu[BLOCK_16x16].calcresidual[NONALIGNED] = PFX(getResidual16_neon);
    p.cu[BLOCK_32x32].calcresidual[NONALIGNED] = PFX(getResidual32_neon);

    p.cu[BLOCK_4x4].calcresidual[ALIGNED]   = PFX(getResidual4_neon);
    p.cu[BLOCK_8x8].calcresidual[ALIGNED]   = PFX(getResidual8_neon);
    p.cu[BLOCK_16x16].calcresidual[ALIGNED] = PFX(getResidual16_neon);
    p.cu[BLOCK_32x32].calcresidual[ALIGNED] = PFX(getResidual32_neon);

    // pixel_sub_ps
    p.cu[BLOCK_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
    p.cu[BLOCK_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
    p.cu[BLOCK_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
    p.cu[BLOCK_32x32].sub_ps = PFX(pixel_sub_ps_32x32_neon);
    p.cu[BLOCK_64x64].sub_ps = PFX(pixel_sub_ps_64x64_neon);

    // chroma sub_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].sub_ps = PFX(pixel_sub_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].sub_ps   = PFX(pixel_sub_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sub_ps  = PFX(pixel_sub_ps_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sub_ps = PFX(pixel_sub_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sub_ps = PFX(pixel_sub_ps_32x64_neon);

    // pixel_add_ps
    p.cu[BLOCK_4x4].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.cu[BLOCK_8x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.cu[BLOCK_16x16].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.cu[BLOCK_32x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.cu[BLOCK_64x64].add_ps[NONALIGNED] = PFX(pixel_add_ps_64x64_neon);

    p.cu[BLOCK_4x4].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.cu[BLOCK_8x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.cu[BLOCK_16x16].add_ps[ALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.cu[BLOCK_32x32].add_ps[ALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.cu[BLOCK_64x64].add_ps[ALIGNED] = PFX(pixel_add_ps_64x64_neon);

    // chroma add_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].add_ps[NONALIGNED]  = PFX(pixel_add_ps_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x64_neon);

    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].add_ps[ALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].add_ps[ALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].add_ps[ALIGNED]  = PFX(pixel_add_ps_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].add_ps[ALIGNED] = PFX(pixel_add_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].add_ps[ALIGNED] = PFX(pixel_add_ps_32x64_neon);

    //scale2D_64to32
    p.scale2D_64to32  = PFX(scale2D_64to32_neon);

    // scale1D_128to64
    p.scale1D_128to64[NONALIGNED] = PFX(scale1D_128to64_neon);
    p.scale1D_128to64[ALIGNED] = PFX(scale1D_128to64_neon);

    // planecopy
    p.planecopy_cp = PFX(pixel_planecopy_cp_neon);

    // satd
    ALL_LUMA_PU(satd, pixel_satd, neon);

    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].satd   = PFX(pixel_satd_4x4_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].satd = PFX(pixel_satd_32x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].satd   = PFX(pixel_satd_8x4_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].satd = PFX(pixel_satd_32x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].satd = PFX(pixel_satd_16x12_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].satd = PFX(pixel_satd_12x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].satd  = PFX(pixel_satd_16x4_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].satd = PFX(pixel_satd_32x24_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].satd = PFX(pixel_satd_24x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].satd  = PFX(pixel_satd_32x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].satd  = PFX(pixel_satd_8x32_neon);

    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].satd = PFX(pixel_satd_32x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].satd   = PFX(pixel_satd_4x4_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].satd  = PFX(pixel_satd_8x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].satd = PFX(pixel_satd_32x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].satd = PFX(pixel_satd_16x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].satd  = PFX(pixel_satd_8x12_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].satd   = PFX(pixel_satd_8x4_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].satd = PFX(pixel_satd_16x24_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].satd = PFX(pixel_satd_12x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].satd  = PFX(pixel_satd_4x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].satd = PFX(pixel_satd_32x48_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].satd = PFX(pixel_satd_24x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].satd = PFX(pixel_satd_32x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].satd  = PFX(pixel_satd_8x64_neon);

    // sa8d
    p.cu[BLOCK_4x4].sa8d   = PFX(pixel_satd_4x4_neon);
    p.cu[BLOCK_8x8].sa8d   = PFX(pixel_sa8d_8x8_neon);
    p.cu[BLOCK_16x16].sa8d = PFX(pixel_sa8d_16x16_neon);
    p.cu[BLOCK_32x32].sa8d = PFX(pixel_sa8d_32x32_neon);
    p.cu[BLOCK_64x64].sa8d = PFX(pixel_sa8d_64x64_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_8x8].sa8d = PFX(pixel_satd_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_16x16].sa8d = PFX(pixel_sa8d_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_32x32].sa8d = PFX(pixel_sa8d_32x32_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_64x64].sa8d = PFX(pixel_sa8d_64x64_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sa8d = PFX(pixel_sa8d_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sa8d = PFX(pixel_sa8d_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sa8d = PFX(pixel_sa8d_32x64_neon);

    // dequant_scaling
    p.dequant_scaling = PFX(dequant_scaling_neon);
    p.dequant_normal  = PFX(dequant_normal_neon);

    // ssim_4x4x2_core
    p.ssim_4x4x2_core = PFX(ssim_4x4x2_core_neon);

    // ssimDist
    p.cu[BLOCK_4x4].ssimDist = PFX(ssimDist4_neon);
    p.cu[BLOCK_8x8].ssimDist = PFX(ssimDist8_neon);
    p.cu[BLOCK_16x16].ssimDist = PFX(ssimDist16_neon);
    p.cu[BLOCK_32x32].ssimDist = PFX(ssimDist32_neon);
    p.cu[BLOCK_64x64].ssimDist = PFX(ssimDist64_neon);

    // normFact
    p.cu[BLOCK_8x8].normFact = PFX(normFact8_neon);
    p.cu[BLOCK_16x16].normFact = PFX(normFact16_neon);
    p.cu[BLOCK_32x32].normFact = PFX(normFact32_neon);
    p.cu[BLOCK_64x64].normFact = PFX(normFact64_neon);

    // psy_cost_pp
    p.cu[BLOCK_4x4].psy_cost_pp = PFX(psyCost_4x4_neon);

    p.weight_pp = PFX(weight_pp_neon);
#if !defined(__APPLE__)
    p.scanPosLast = PFX(scanPosLast_neon);
#endif
    p.costCoeffNxN = PFX(costCoeffNxN_neon);
#endif

    // quant
    p.quant = PFX(quant_neon);
    p.nquant = PFX(nquant_neon);
}

#if defined(HAVE_SVE2) || defined(HAVE_SVE)
void setupSvePrimitives(EncoderPrimitives &p)
{
    // When these primitives will use SVE/SVE2 instructions set,
    // change the following definitions to point to the SVE/SVE2 implementation
    setupPixelPrimitives_neon(p);
    setupFilterPrimitives_neon(p);
    setupDCTPrimitives_neon(p);
    setupLoopFilterPrimitives_neon(p);
    setupIntraPrimitives_neon(p);

    CHROMA_420_PU_FILTER_PIXEL_TO_SHORT_NEON(p2s[NONALIGNED]);
    CHROMA_420_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_422_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_422_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_444_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_444_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    LUMA_PU_NEON_FILTER_PIXEL_TO_SHORT(convert_p2s[ALIGNED]);
    LUMA_PU_SVE_FILTER_PIXEL_TO_SHORT(convert_p2s[ALIGNED]);
    CHROMA_420_PU_FILTER_PIXEL_TO_SHORT_NEON(p2s[ALIGNED]);
    CHROMA_420_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_422_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_422_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_444_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_444_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    LUMA_PU_NEON_FILTER_PIXEL_TO_SHORT(convert_p2s[NONALIGNED]);
    LUMA_PU_SVE_FILTER_PIXEL_TO_SHORT(convert_p2s[NONALIGNED]);

#if !HIGH_BIT_DEPTH
    ALL_LUMA_PU(luma_vpp, interp_8tap_vert_pp, neon);
    ALL_LUMA_PU(luma_vsp, interp_8tap_vert_sp, neon);
    ALL_LUMA_PU(luma_vps, interp_8tap_vert_ps, neon);
    ALL_LUMA_PU(luma_hpp, interp_horiz_pp, neon);
    ALL_LUMA_PU(luma_hps, interp_horiz_ps, neon);
    ALL_LUMA_PU(luma_vss, interp_8tap_vert_ss, neon);
    ALL_LUMA_PU_T(luma_hvpp, interp_8tap_hv_pp_cpu);
    ALL_CHROMA_420_VERT_FILTERS(neon);
    CHROMA_422_VERT_FILTERS_NEON();
    CHROMA_422_VERT_FILTERS_CAN_USE_SVE2(neon);
    ALL_CHROMA_444_VERT_FILTERS(neon);
    ALL_CHROMA_420_FILTERS(neon);
    ALL_CHROMA_422_FILTERS(neon);
    ALL_CHROMA_444_FILTERS(neon);


    // Blockcopy_pp
    LUMA_PU_NEON_1(copy_pp, blockcopy_pp);
    LUMA_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(copy_pp, blockcopy_pp);
    CHROMA_420_PU_NEON_1(copy_pp, blockcopy_pp);
    CHROMA_420_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(copy_pp, blockcopy_pp);
    CHROMA_422_PU_NEON_1(copy_pp, blockcopy_pp);
    CHROMA_422_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(copy_pp, blockcopy_pp);
    p.cu[BLOCK_4x4].copy_pp   = PFX(blockcopy_pp_4x4_neon);
    p.cu[BLOCK_8x8].copy_pp   = PFX(blockcopy_pp_8x8_neon);
    p.cu[BLOCK_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    p.cu[BLOCK_32x32].copy_pp = PFX(blockcopy_pp_32x32_sve);
    p.cu[BLOCK_64x64].copy_pp = PFX(blockcopy_pp_64x64_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_pp = PFX(blockcopy_pp_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_pp = PFX(blockcopy_pp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_pp = PFX(blockcopy_pp_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_pp = PFX(blockcopy_pp_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_pp = PFX(blockcopy_pp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_pp = PFX(blockcopy_pp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_pp = PFX(blockcopy_pp_32x64_sve);

#endif // !HIGH_BIT_DEPTH

    // Blockcopy_ss
    p.cu[BLOCK_4x4].copy_ss   = PFX(blockcopy_ss_4x4_neon);
    p.cu[BLOCK_8x8].copy_ss   = PFX(blockcopy_ss_8x8_neon);
    p.cu[BLOCK_16x16].copy_ss = PFX(blockcopy_ss_16x16_sve);
    p.cu[BLOCK_32x32].copy_ss = PFX(blockcopy_ss_32x32_sve);
    p.cu[BLOCK_64x64].copy_ss = PFX(blockcopy_ss_64x64_sve);

    // Blockcopy_ps
    p.cu[BLOCK_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
    p.cu[BLOCK_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
    p.cu[BLOCK_16x16].copy_ps = PFX(blockcopy_ps_16x16_sve);
    p.cu[BLOCK_32x32].copy_ps = PFX(blockcopy_ps_32x32_sve);
    p.cu[BLOCK_64x64].copy_ps = PFX(blockcopy_ps_64x64_sve);

    // Blockcopy_sp
    p.cu[BLOCK_4x4].copy_sp   = PFX(blockcopy_sp_4x4_sve);
    p.cu[BLOCK_8x8].copy_sp   = PFX(blockcopy_sp_8x8_sve);
    p.cu[BLOCK_16x16].copy_sp = PFX(blockcopy_sp_16x16_sve);
    p.cu[BLOCK_32x32].copy_sp = PFX(blockcopy_sp_32x32_sve);
    p.cu[BLOCK_64x64].copy_sp = PFX(blockcopy_sp_64x64_neon);

    // chroma blockcopy_ss
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_ss   = PFX(blockcopy_ss_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_ss   = PFX(blockcopy_ss_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_ss = PFX(blockcopy_ss_16x16_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_ss = PFX(blockcopy_ss_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_ss   = PFX(blockcopy_ss_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_ss  = PFX(blockcopy_ss_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_ss = PFX(blockcopy_ss_16x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_ss = PFX(blockcopy_ss_32x64_sve);

    // chroma blockcopy_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_ps = PFX(blockcopy_ps_16x16_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_ps = PFX(blockcopy_ps_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_ps   = PFX(blockcopy_ps_4x8_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_ps  = PFX(blockcopy_ps_8x16_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_ps = PFX(blockcopy_ps_16x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_ps = PFX(blockcopy_ps_32x64_sve);

    // chroma blockcopy_sp
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_sp   = PFX(blockcopy_sp_4x4_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_sp   = PFX(blockcopy_sp_8x8_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_sp = PFX(blockcopy_sp_16x16_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_sp = PFX(blockcopy_sp_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_sp   = PFX(blockcopy_sp_4x8_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_sp  = PFX(blockcopy_sp_8x16_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_sp = PFX(blockcopy_sp_16x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_sp = PFX(blockcopy_sp_32x64_sve);

    // Block_fill
    LUMA_TU_NEON(blockfill_s[ALIGNED], blockfill_s);
    LUMA_TU_CAN_USE_SVE(blockfill_s[ALIGNED], blockfill_s);
    LUMA_TU_NEON(blockfill_s[NONALIGNED], blockfill_s);
    LUMA_TU_CAN_USE_SVE(blockfill_s[NONALIGNED], blockfill_s);

    // copy_count
    p.cu[BLOCK_4x4].copy_cnt     = PFX(copy_cnt_4_neon);
    p.cu[BLOCK_8x8].copy_cnt     = PFX(copy_cnt_8_neon);
    p.cu[BLOCK_16x16].copy_cnt   = PFX(copy_cnt_16_neon);
    p.cu[BLOCK_32x32].copy_cnt   = PFX(copy_cnt_32_neon);

    // count nonzero
    p.cu[BLOCK_4x4].count_nonzero     = PFX(count_nonzero_4_neon);
    p.cu[BLOCK_8x8].count_nonzero     = PFX(count_nonzero_8_neon);
    p.cu[BLOCK_16x16].count_nonzero   = PFX(count_nonzero_16_neon);
    p.cu[BLOCK_32x32].count_nonzero   = PFX(count_nonzero_32_neon);

    // cpy2Dto1D_shl
    p.cu[BLOCK_4x4].cpy2Dto1D_shl   = PFX(cpy2Dto1D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy2Dto1D_shl   = PFX(cpy2Dto1D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_16x16_sve);
    p.cu[BLOCK_32x32].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_32x32_sve);
    p.cu[BLOCK_64x64].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_64x64_sve);

    // cpy2Dto1D_shr
    p.cu[BLOCK_4x4].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_4x4_neon);
    p.cu[BLOCK_8x8].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_8x8_neon);
    p.cu[BLOCK_16x16].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_16x16_sve);
    p.cu[BLOCK_32x32].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_32x32_sve);

    // cpy1Dto2D_shl
    p.cu[BLOCK_4x4].cpy1Dto2D_shl[ALIGNED]      = PFX(cpy1Dto2D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shl[ALIGNED]      = PFX(cpy1Dto2D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_16x16_sve);
    p.cu[BLOCK_32x32].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_32x32_sve);
    p.cu[BLOCK_64x64].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_64x64_sve);

    p.cu[BLOCK_4x4].cpy1Dto2D_shl[NONALIGNED]   = PFX(cpy1Dto2D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shl[NONALIGNED]   = PFX(cpy1Dto2D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_16x16_sve);
    p.cu[BLOCK_32x32].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_32x32_sve);
    p.cu[BLOCK_64x64].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_64x64_sve);

    // cpy1Dto2D_shr
    p.cu[BLOCK_4x4].cpy1Dto2D_shr   = PFX(cpy1Dto2D_shr_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shr   = PFX(cpy1Dto2D_shr_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_16x16_sve);
    p.cu[BLOCK_32x32].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_32x32_sve);
    p.cu[BLOCK_64x64].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_64x64_sve);

#if !HIGH_BIT_DEPTH
    // pixel_avg_pp
    ALL_LUMA_PU(pixelavg_pp[NONALIGNED], pixel_avg_pp, neon);
    ALL_LUMA_PU(pixelavg_pp[ALIGNED], pixel_avg_pp, neon);

    // addAvg
    ALL_LUMA_PU(addAvg[NONALIGNED], addAvg, neon);
    ALL_LUMA_PU(addAvg[ALIGNED], addAvg, neon);
    ALL_CHROMA_420_PU(addAvg[NONALIGNED], addAvg, neon);
    ALL_CHROMA_422_PU(addAvg[NONALIGNED], addAvg, neon);
    ALL_CHROMA_420_PU(addAvg[ALIGNED], addAvg, neon);
    ALL_CHROMA_422_PU(addAvg[ALIGNED], addAvg, neon);

    // sad
    ALL_LUMA_PU(sad, pixel_sad, neon);
    ALL_LUMA_PU(sad_x3, sad_x3, neon);
    ALL_LUMA_PU(sad_x4, sad_x4, neon);

    // sse_pp
    p.cu[BLOCK_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_sve);
    p.cu[BLOCK_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
    p.cu[BLOCK_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
    p.cu[BLOCK_32x32].sse_pp = PFX(pixel_sse_pp_32x32_neon);
    p.cu[BLOCK_64x64].sse_pp = PFX(pixel_sse_pp_64x64_neon);

    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].sse_pp = PFX(pixel_sse_pp_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].sse_pp   = PFX(pixel_sse_pp_4x8_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sse_pp  = PFX(pixel_sse_pp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sse_pp = PFX(pixel_sse_pp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sse_pp = PFX(pixel_sse_pp_32x64_neon);

    // sse_ss
    p.cu[BLOCK_4x4].sse_ss   = PFX(pixel_sse_ss_4x4_neon);
    p.cu[BLOCK_8x8].sse_ss   = PFX(pixel_sse_ss_8x8_neon);
    p.cu[BLOCK_16x16].sse_ss = PFX(pixel_sse_ss_16x16_neon);
    p.cu[BLOCK_32x32].sse_ss = PFX(pixel_sse_ss_32x32_neon);
    p.cu[BLOCK_64x64].sse_ss = PFX(pixel_sse_ss_64x64_neon);

    // ssd_s
    p.cu[BLOCK_4x4].ssd_s[NONALIGNED]   = PFX(pixel_ssd_s_4x4_neon);
    p.cu[BLOCK_8x8].ssd_s[NONALIGNED]   = PFX(pixel_ssd_s_8x8_neon);
    p.cu[BLOCK_16x16].ssd_s[NONALIGNED] = PFX(pixel_ssd_s_16x16_neon);
    p.cu[BLOCK_32x32].ssd_s[NONALIGNED] = PFX(pixel_ssd_s_32x32_neon);

    p.cu[BLOCK_4x4].ssd_s[ALIGNED]   = PFX(pixel_ssd_s_4x4_neon);
    p.cu[BLOCK_8x8].ssd_s[ALIGNED]   = PFX(pixel_ssd_s_8x8_neon);
    p.cu[BLOCK_16x16].ssd_s[ALIGNED] = PFX(pixel_ssd_s_16x16_neon);
    p.cu[BLOCK_32x32].ssd_s[ALIGNED] = PFX(pixel_ssd_s_32x32_neon);

    // pixel_var
    p.cu[BLOCK_8x8].var   = PFX(pixel_var_8x8_neon);
    p.cu[BLOCK_16x16].var = PFX(pixel_var_16x16_neon);
    p.cu[BLOCK_32x32].var = PFX(pixel_var_32x32_neon);
    p.cu[BLOCK_64x64].var = PFX(pixel_var_64x64_neon);

    // calc_Residual
    p.cu[BLOCK_4x4].calcresidual[NONALIGNED]   = PFX(getResidual4_neon);
    p.cu[BLOCK_8x8].calcresidual[NONALIGNED]   = PFX(getResidual8_neon);
    p.cu[BLOCK_16x16].calcresidual[NONALIGNED] = PFX(getResidual16_neon);
    p.cu[BLOCK_32x32].calcresidual[NONALIGNED] = PFX(getResidual32_neon);

    p.cu[BLOCK_4x4].calcresidual[ALIGNED]   = PFX(getResidual4_neon);
    p.cu[BLOCK_8x8].calcresidual[ALIGNED]   = PFX(getResidual8_neon);
    p.cu[BLOCK_16x16].calcresidual[ALIGNED] = PFX(getResidual16_neon);
    p.cu[BLOCK_32x32].calcresidual[ALIGNED] = PFX(getResidual32_neon);

    // pixel_sub_ps
    p.cu[BLOCK_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
    p.cu[BLOCK_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
    p.cu[BLOCK_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
    p.cu[BLOCK_32x32].sub_ps = PFX(pixel_sub_ps_32x32_neon);
    p.cu[BLOCK_64x64].sub_ps = PFX(pixel_sub_ps_64x64_neon);

    // chroma sub_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].sub_ps = PFX(pixel_sub_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].sub_ps   = PFX(pixel_sub_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sub_ps  = PFX(pixel_sub_ps_8x16_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sub_ps = PFX(pixel_sub_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sub_ps = PFX(pixel_sub_ps_32x64_neon);

    // pixel_add_ps
    p.cu[BLOCK_4x4].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.cu[BLOCK_8x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.cu[BLOCK_16x16].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.cu[BLOCK_32x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.cu[BLOCK_64x64].add_ps[NONALIGNED] = PFX(pixel_add_ps_64x64_neon);

    p.cu[BLOCK_4x4].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.cu[BLOCK_8x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.cu[BLOCK_16x16].add_ps[ALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.cu[BLOCK_32x32].add_ps[ALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.cu[BLOCK_64x64].add_ps[ALIGNED] = PFX(pixel_add_ps_64x64_neon);

    // chroma add_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].add_ps[NONALIGNED]  = PFX(pixel_add_ps_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x64_neon);

    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].add_ps[ALIGNED] = PFX(pixel_add_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].add_ps[ALIGNED] = PFX(pixel_add_ps_32x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].add_ps[ALIGNED]  = PFX(pixel_add_ps_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].add_ps[ALIGNED] = PFX(pixel_add_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].add_ps[ALIGNED] = PFX(pixel_add_ps_32x64_neon);

    //scale2D_64to32
    p.scale2D_64to32  = PFX(scale2D_64to32_neon);

    // scale1D_128to64
    p.scale1D_128to64[NONALIGNED] = PFX(scale1D_128to64_neon);
    p.scale1D_128to64[ALIGNED] = PFX(scale1D_128to64_neon);

    // planecopy
    p.planecopy_cp = PFX(pixel_planecopy_cp_neon);

    // satd
    p.pu[LUMA_4x4].satd   = PFX(pixel_satd_4x4_sve);
    p.pu[LUMA_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.pu[LUMA_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.pu[LUMA_32x32].satd = PFX(pixel_satd_32x32_sve);
    p.pu[LUMA_64x64].satd = PFX(pixel_satd_64x64_neon);
    p.pu[LUMA_8x4].satd   = PFX(pixel_satd_8x4_sve);
    p.pu[LUMA_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.pu[LUMA_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.pu[LUMA_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.pu[LUMA_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.pu[LUMA_32x16].satd = PFX(pixel_satd_32x16_sve);
    p.pu[LUMA_64x32].satd = PFX(pixel_satd_64x32_neon);
    p.pu[LUMA_32x64].satd = PFX(pixel_satd_32x64_neon);
    p.pu[LUMA_16x12].satd = PFX(pixel_satd_16x12_neon);
    p.pu[LUMA_12x16].satd = PFX(pixel_satd_12x16_neon);
    p.pu[LUMA_16x4].satd  = PFX(pixel_satd_16x4_neon);
    p.pu[LUMA_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.pu[LUMA_32x24].satd = PFX(pixel_satd_32x24_neon);
    p.pu[LUMA_24x32].satd = PFX(pixel_satd_24x32_neon);
    p.pu[LUMA_32x8].satd  = PFX(pixel_satd_32x8_neon);
    p.pu[LUMA_8x32].satd  = PFX(pixel_satd_8x32_neon);
    p.pu[LUMA_64x48].satd = PFX(pixel_satd_64x48_sve);
    p.pu[LUMA_48x64].satd = PFX(pixel_satd_48x64_neon);
    p.pu[LUMA_64x16].satd = PFX(pixel_satd_64x16_neon);
    p.pu[LUMA_16x64].satd = PFX(pixel_satd_16x64_neon);

    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].satd   = PFX(pixel_satd_4x4_sve);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].satd = PFX(pixel_satd_32x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].satd   = PFX(pixel_satd_8x4_sve);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].satd = PFX(pixel_satd_32x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].satd = PFX(pixel_satd_16x12_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].satd = PFX(pixel_satd_12x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].satd  = PFX(pixel_satd_16x4_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].satd = PFX(pixel_satd_32x24_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].satd = PFX(pixel_satd_24x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].satd  = PFX(pixel_satd_32x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].satd  = PFX(pixel_satd_8x32_neon);

    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].satd = PFX(pixel_satd_32x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].satd   = PFX(pixel_satd_4x4_sve);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].satd  = PFX(pixel_satd_8x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].satd = PFX(pixel_satd_32x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].satd = PFX(pixel_satd_16x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].satd  = PFX(pixel_satd_8x12_sve);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].satd   = PFX(pixel_satd_8x4_sve);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].satd = PFX(pixel_satd_16x24_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].satd = PFX(pixel_satd_12x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].satd  = PFX(pixel_satd_4x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].satd = PFX(pixel_satd_32x48_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].satd = PFX(pixel_satd_24x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].satd = PFX(pixel_satd_32x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].satd  = PFX(pixel_satd_8x64_neon);

    // sa8d
    p.cu[BLOCK_4x4].sa8d   = PFX(pixel_satd_4x4_sve);
    p.cu[BLOCK_8x8].sa8d   = PFX(pixel_sa8d_8x8_neon);
    p.cu[BLOCK_16x16].sa8d = PFX(pixel_sa8d_16x16_neon);
    p.cu[BLOCK_32x32].sa8d = PFX(pixel_sa8d_32x32_neon);
    p.cu[BLOCK_64x64].sa8d = PFX(pixel_sa8d_64x64_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_8x8].sa8d = PFX(pixel_satd_4x4_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_16x16].sa8d = PFX(pixel_sa8d_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_32x32].sa8d = PFX(pixel_sa8d_32x32_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_64x64].sa8d = PFX(pixel_sa8d_64x64_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sa8d = PFX(pixel_sa8d_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sa8d = PFX(pixel_sa8d_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sa8d = PFX(pixel_sa8d_32x64_neon);

    // dequant_scaling
    p.dequant_scaling = PFX(dequant_scaling_neon);
    p.dequant_normal  = PFX(dequant_normal_neon);

    // ssim_4x4x2_core
    p.ssim_4x4x2_core = PFX(ssim_4x4x2_core_neon);

    // ssimDist
    p.cu[BLOCK_4x4].ssimDist = PFX(ssimDist4_neon);
    p.cu[BLOCK_8x8].ssimDist = PFX(ssimDist8_neon);
    p.cu[BLOCK_16x16].ssimDist = PFX(ssimDist16_neon);
    p.cu[BLOCK_32x32].ssimDist = PFX(ssimDist32_neon);
    p.cu[BLOCK_64x64].ssimDist = PFX(ssimDist64_neon);

    // normFact
    p.cu[BLOCK_8x8].normFact = PFX(normFact8_neon);
    p.cu[BLOCK_16x16].normFact = PFX(normFact16_neon);
    p.cu[BLOCK_32x32].normFact = PFX(normFact32_neon);
    p.cu[BLOCK_64x64].normFact = PFX(normFact64_neon);

    // psy_cost_pp
    p.cu[BLOCK_4x4].psy_cost_pp = PFX(psyCost_4x4_neon);

    p.weight_pp = PFX(weight_pp_neon);
#if !defined(__APPLE__)
    p.scanPosLast = PFX(scanPosLast_neon);
#endif
    p.costCoeffNxN = PFX(costCoeffNxN_neon);
#endif

    // quant
    p.quant = PFX(quant_sve);
    p.nquant = PFX(nquant_neon);
}
#endif

#if defined(HAVE_SVE2)
void setupSve2Primitives(EncoderPrimitives &p)
{
    // When these primitives will use SVE/SVE2 instructions set,
    // change the following definitions to point to the SVE/SVE2 implementation
    setupPixelPrimitives_neon(p);
    setupFilterPrimitives_neon(p);
    setupDCTPrimitives_neon(p);
    setupLoopFilterPrimitives_neon(p);
    setupIntraPrimitives_neon(p);

    CHROMA_420_PU_FILTER_PIXEL_TO_SHORT_NEON(p2s[NONALIGNED]);
    CHROMA_420_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_422_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_422_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_444_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_444_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    LUMA_PU_NEON_FILTER_PIXEL_TO_SHORT(convert_p2s[ALIGNED]);
    LUMA_PU_SVE_FILTER_PIXEL_TO_SHORT(convert_p2s[ALIGNED]);
    CHROMA_420_PU_FILTER_PIXEL_TO_SHORT_NEON(p2s[ALIGNED]);
    CHROMA_420_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[ALIGNED]);
    CHROMA_422_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_422_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_444_PU_NEON_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    CHROMA_444_PU_SVE_FILTER_PIXEL_TO_SHORT(p2s[NONALIGNED]);
    LUMA_PU_NEON_FILTER_PIXEL_TO_SHORT(convert_p2s[NONALIGNED]);
    LUMA_PU_SVE_FILTER_PIXEL_TO_SHORT(convert_p2s[NONALIGNED]);

#if !HIGH_BIT_DEPTH
    LUMA_PU_MULTIPLE_ARCHS_1(luma_vpp, interp_8tap_vert_pp, neon);
    LUMA_PU_MULTIPLE_ARCHS_2(luma_vpp, interp_8tap_vert_pp, sve2);
    LUMA_PU_MULTIPLE_ARCHS_1(luma_vsp, interp_8tap_vert_sp, sve2);
    LUMA_PU_MULTIPLE_ARCHS_2(luma_vsp, interp_8tap_vert_sp, neon);
    ALL_LUMA_PU(luma_vps, interp_8tap_vert_ps, sve2);
    ALL_LUMA_PU(luma_hpp, interp_horiz_pp, neon);
    ALL_LUMA_PU(luma_hps, interp_horiz_ps, neon);
    ALL_LUMA_PU(luma_vss, interp_8tap_vert_ss, sve2);
    ALL_LUMA_PU_T(luma_hvpp, interp_8tap_hv_pp_cpu);
    CHROMA_420_VERT_FILTERS_NEON();
    CHROMA_420_VERT_FILTERS_CAN_USE_SVE2();
    CHROMA_422_VERT_FILTERS_NEON();
    CHROMA_422_VERT_FILTERS_CAN_USE_SVE2(sve2);
    CHROMA_444_VERT_FILTERS_NEON();
    CHROMA_444_VERT_FILTERS_CAN_USE_SVE2();
    CHROMA_420_FILTERS_NEON();
    CHROMA_420_FILTERS_CAN_USE_SVE2();
    CHROMA_422_FILTERS_NEON();
    CHROMA_422_FILTERS_CAN_USE_SVE2();
    CHROMA_444_FILTERS_NEON();
    CHROMA_444_FILTERS_CAN_USE_SVE2();

    // Blockcopy_pp
    LUMA_PU_NEON_1(copy_pp, blockcopy_pp);
    LUMA_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(copy_pp, blockcopy_pp);
    CHROMA_420_PU_NEON_1(copy_pp, blockcopy_pp);
    CHROMA_420_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(copy_pp, blockcopy_pp);
    CHROMA_422_PU_NEON_1(copy_pp, blockcopy_pp);
    CHROMA_422_PU_CAN_USE_SVE_EXCEPT_FILTER_PIXEL_TO_SHORT(copy_pp, blockcopy_pp);
    p.cu[BLOCK_4x4].copy_pp   = PFX(blockcopy_pp_4x4_neon);
    p.cu[BLOCK_8x8].copy_pp   = PFX(blockcopy_pp_8x8_neon);
    p.cu[BLOCK_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    p.cu[BLOCK_32x32].copy_pp = PFX(blockcopy_pp_32x32_sve);
    p.cu[BLOCK_64x64].copy_pp = PFX(blockcopy_pp_64x64_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_pp = PFX(blockcopy_pp_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_pp = PFX(blockcopy_pp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_pp = PFX(blockcopy_pp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_pp = PFX(blockcopy_pp_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_pp = PFX(blockcopy_pp_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_pp = PFX(blockcopy_pp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_pp = PFX(blockcopy_pp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_pp = PFX(blockcopy_pp_32x64_sve);

#endif // !HIGH_BIT_DEPTH

    // Blockcopy_ss
    p.cu[BLOCK_4x4].copy_ss   = PFX(blockcopy_ss_4x4_neon);
    p.cu[BLOCK_8x8].copy_ss   = PFX(blockcopy_ss_8x8_neon);
    p.cu[BLOCK_16x16].copy_ss = PFX(blockcopy_ss_16x16_sve);
    p.cu[BLOCK_32x32].copy_ss = PFX(blockcopy_ss_32x32_sve);
    p.cu[BLOCK_64x64].copy_ss = PFX(blockcopy_ss_64x64_sve);

    // Blockcopy_ps
    p.cu[BLOCK_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
    p.cu[BLOCK_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
    p.cu[BLOCK_16x16].copy_ps = PFX(blockcopy_ps_16x16_sve);
    p.cu[BLOCK_32x32].copy_ps = PFX(blockcopy_ps_32x32_sve);
    p.cu[BLOCK_64x64].copy_ps = PFX(blockcopy_ps_64x64_sve);

    // Blockcopy_sp
    p.cu[BLOCK_4x4].copy_sp   = PFX(blockcopy_sp_4x4_sve);
    p.cu[BLOCK_8x8].copy_sp   = PFX(blockcopy_sp_8x8_sve);
    p.cu[BLOCK_16x16].copy_sp = PFX(blockcopy_sp_16x16_sve);
    p.cu[BLOCK_32x32].copy_sp = PFX(blockcopy_sp_32x32_sve);
    p.cu[BLOCK_64x64].copy_sp = PFX(blockcopy_sp_64x64_neon);

    // chroma blockcopy_ss
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_ss   = PFX(blockcopy_ss_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_ss   = PFX(blockcopy_ss_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_ss = PFX(blockcopy_ss_16x16_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_ss = PFX(blockcopy_ss_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_ss   = PFX(blockcopy_ss_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_ss  = PFX(blockcopy_ss_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_ss = PFX(blockcopy_ss_16x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_ss = PFX(blockcopy_ss_32x64_sve);

    // chroma blockcopy_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_ps   = PFX(blockcopy_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_ps   = PFX(blockcopy_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_ps = PFX(blockcopy_ps_16x16_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_ps = PFX(blockcopy_ps_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_ps   = PFX(blockcopy_ps_4x8_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_ps  = PFX(blockcopy_ps_8x16_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_ps = PFX(blockcopy_ps_16x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_ps = PFX(blockcopy_ps_32x64_sve);

    // chroma blockcopy_sp
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].copy_sp   = PFX(blockcopy_sp_4x4_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].copy_sp   = PFX(blockcopy_sp_8x8_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].copy_sp = PFX(blockcopy_sp_16x16_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].copy_sp = PFX(blockcopy_sp_32x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].copy_sp   = PFX(blockcopy_sp_4x8_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].copy_sp  = PFX(blockcopy_sp_8x16_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].copy_sp = PFX(blockcopy_sp_16x32_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].copy_sp = PFX(blockcopy_sp_32x64_sve);

    // Block_fill
    LUMA_TU_NEON(blockfill_s[ALIGNED], blockfill_s);
    LUMA_TU_CAN_USE_SVE(blockfill_s[ALIGNED], blockfill_s);
    LUMA_TU_NEON(blockfill_s[NONALIGNED], blockfill_s);
    LUMA_TU_CAN_USE_SVE(blockfill_s[NONALIGNED], blockfill_s);

    // copy_count
    p.cu[BLOCK_4x4].copy_cnt     = PFX(copy_cnt_4_neon);
    p.cu[BLOCK_8x8].copy_cnt     = PFX(copy_cnt_8_neon);
    p.cu[BLOCK_16x16].copy_cnt   = PFX(copy_cnt_16_neon);
    p.cu[BLOCK_32x32].copy_cnt   = PFX(copy_cnt_32_neon);

    // count nonzero
    p.cu[BLOCK_4x4].count_nonzero     = PFX(count_nonzero_4_neon);
    p.cu[BLOCK_8x8].count_nonzero     = PFX(count_nonzero_8_neon);
    p.cu[BLOCK_16x16].count_nonzero   = PFX(count_nonzero_16_neon);
    p.cu[BLOCK_32x32].count_nonzero   = PFX(count_nonzero_32_neon);

    // cpy2Dto1D_shl
    p.cu[BLOCK_4x4].cpy2Dto1D_shl   = PFX(cpy2Dto1D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy2Dto1D_shl   = PFX(cpy2Dto1D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_16x16_sve);
    p.cu[BLOCK_32x32].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_32x32_sve);
    p.cu[BLOCK_64x64].cpy2Dto1D_shl = PFX(cpy2Dto1D_shl_64x64_sve);

    // cpy2Dto1D_shr
    p.cu[BLOCK_4x4].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_4x4_neon);
    p.cu[BLOCK_8x8].cpy2Dto1D_shr   = PFX(cpy2Dto1D_shr_8x8_neon);
    p.cu[BLOCK_16x16].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_16x16_sve);
    p.cu[BLOCK_32x32].cpy2Dto1D_shr = PFX(cpy2Dto1D_shr_32x32_sve);

    // cpy1Dto2D_shl
    p.cu[BLOCK_4x4].cpy1Dto2D_shl[ALIGNED]      = PFX(cpy1Dto2D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shl[ALIGNED]      = PFX(cpy1Dto2D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_16x16_sve);
    p.cu[BLOCK_32x32].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_32x32_sve);
    p.cu[BLOCK_64x64].cpy1Dto2D_shl[ALIGNED]    = PFX(cpy1Dto2D_shl_64x64_sve);

    p.cu[BLOCK_4x4].cpy1Dto2D_shl[NONALIGNED]   = PFX(cpy1Dto2D_shl_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shl[NONALIGNED]   = PFX(cpy1Dto2D_shl_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_16x16_sve);
    p.cu[BLOCK_32x32].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_32x32_sve);
    p.cu[BLOCK_64x64].cpy1Dto2D_shl[NONALIGNED] = PFX(cpy1Dto2D_shl_64x64_sve);

    // cpy1Dto2D_shr
    p.cu[BLOCK_4x4].cpy1Dto2D_shr   = PFX(cpy1Dto2D_shr_4x4_neon);
    p.cu[BLOCK_8x8].cpy1Dto2D_shr   = PFX(cpy1Dto2D_shr_8x8_neon);
    p.cu[BLOCK_16x16].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_16x16_sve);
    p.cu[BLOCK_32x32].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_32x32_sve);
    p.cu[BLOCK_64x64].cpy1Dto2D_shr = PFX(cpy1Dto2D_shr_64x64_sve);

#if !HIGH_BIT_DEPTH
    // pixel_avg_pp
    LUMA_PU_NEON_2(pixelavg_pp[NONALIGNED], pixel_avg_pp);
    LUMA_PU_MULTIPLE_ARCHS_3(pixelavg_pp[NONALIGNED], pixel_avg_pp, sve2);
    LUMA_PU_NEON_2(pixelavg_pp[ALIGNED], pixel_avg_pp);
    LUMA_PU_MULTIPLE_ARCHS_3(pixelavg_pp[ALIGNED], pixel_avg_pp, sve2);

    // addAvg
    LUMA_PU_NEON_3(addAvg[NONALIGNED], addAvg);
    LUMA_PU_CAN_USE_SVE2(addAvg[NONALIGNED], addAvg);
    LUMA_PU_NEON_3(addAvg[ALIGNED], addAvg);
    LUMA_PU_CAN_USE_SVE2(addAvg[ALIGNED], addAvg);
    CHROMA_420_PU_NEON_2(addAvg[NONALIGNED], addAvg);
    CHROMA_420_PU_MULTIPLE_ARCHS(addAvg[NONALIGNED], addAvg, sve2);
    CHROMA_420_PU_NEON_2(addAvg[ALIGNED], addAvg);
    CHROMA_420_PU_MULTIPLE_ARCHS(addAvg[ALIGNED], addAvg, sve2);
    CHROMA_422_PU_NEON_2(addAvg[NONALIGNED], addAvg);
    CHROMA_422_PU_CAN_USE_SVE2(addAvg[NONALIGNED], addAvg);
    CHROMA_422_PU_NEON_2(addAvg[ALIGNED], addAvg);
    CHROMA_422_PU_CAN_USE_SVE2(addAvg[ALIGNED], addAvg);

    // sad
    ALL_LUMA_PU(sad, pixel_sad, sve2);
    ALL_LUMA_PU(sad_x3, sad_x3, sve2);
    ALL_LUMA_PU(sad_x4, sad_x4, sve2);

    // sse_pp
    p.cu[BLOCK_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_sve);
    p.cu[BLOCK_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
    p.cu[BLOCK_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
    p.cu[BLOCK_32x32].sse_pp = PFX(pixel_sse_pp_32x32_sve2);
    p.cu[BLOCK_64x64].sse_pp = PFX(pixel_sse_pp_64x64_sve2);

    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].sse_pp   = PFX(pixel_sse_pp_4x4_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].sse_pp   = PFX(pixel_sse_pp_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].sse_pp = PFX(pixel_sse_pp_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].sse_pp = PFX(pixel_sse_pp_32x32_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].sse_pp   = PFX(pixel_sse_pp_4x8_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sse_pp  = PFX(pixel_sse_pp_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sse_pp = PFX(pixel_sse_pp_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sse_pp = PFX(pixel_sse_pp_32x64_sve2);

    // sse_ss
    p.cu[BLOCK_4x4].sse_ss   = PFX(pixel_sse_ss_4x4_sve2);
    p.cu[BLOCK_8x8].sse_ss   = PFX(pixel_sse_ss_8x8_sve2);
    p.cu[BLOCK_16x16].sse_ss = PFX(pixel_sse_ss_16x16_sve2);
    p.cu[BLOCK_32x32].sse_ss = PFX(pixel_sse_ss_32x32_sve2);
    p.cu[BLOCK_64x64].sse_ss = PFX(pixel_sse_ss_64x64_sve2);

    // ssd_s
    p.cu[BLOCK_4x4].ssd_s[NONALIGNED]   = PFX(pixel_ssd_s_4x4_sve2);
    p.cu[BLOCK_8x8].ssd_s[NONALIGNED]   = PFX(pixel_ssd_s_8x8_sve2);
    p.cu[BLOCK_16x16].ssd_s[NONALIGNED] = PFX(pixel_ssd_s_16x16_sve2);
    p.cu[BLOCK_32x32].ssd_s[NONALIGNED] = PFX(pixel_ssd_s_32x32_sve2);

    p.cu[BLOCK_4x4].ssd_s[ALIGNED]   = PFX(pixel_ssd_s_4x4_sve2);
    p.cu[BLOCK_8x8].ssd_s[ALIGNED]   = PFX(pixel_ssd_s_8x8_sve2);
    p.cu[BLOCK_16x16].ssd_s[ALIGNED] = PFX(pixel_ssd_s_16x16_sve2);
    p.cu[BLOCK_32x32].ssd_s[ALIGNED] = PFX(pixel_ssd_s_32x32_sve2);

    // pixel_var
    p.cu[BLOCK_8x8].var   = PFX(pixel_var_8x8_sve2);
    p.cu[BLOCK_16x16].var = PFX(pixel_var_16x16_sve2);
    p.cu[BLOCK_32x32].var = PFX(pixel_var_32x32_sve2);
    p.cu[BLOCK_64x64].var = PFX(pixel_var_64x64_sve2);

    // calc_Residual
    p.cu[BLOCK_4x4].calcresidual[NONALIGNED]   = PFX(getResidual4_neon);
    p.cu[BLOCK_8x8].calcresidual[NONALIGNED]   = PFX(getResidual8_neon);
    p.cu[BLOCK_16x16].calcresidual[NONALIGNED] = PFX(getResidual16_sve2);
    p.cu[BLOCK_32x32].calcresidual[NONALIGNED] = PFX(getResidual32_sve2);

    p.cu[BLOCK_4x4].calcresidual[ALIGNED]   = PFX(getResidual4_neon);
    p.cu[BLOCK_8x8].calcresidual[ALIGNED]   = PFX(getResidual8_neon);
    p.cu[BLOCK_16x16].calcresidual[ALIGNED] = PFX(getResidual16_sve2);
    p.cu[BLOCK_32x32].calcresidual[ALIGNED] = PFX(getResidual32_sve2);

    // pixel_sub_ps
    p.cu[BLOCK_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
    p.cu[BLOCK_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
    p.cu[BLOCK_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
    p.cu[BLOCK_32x32].sub_ps = PFX(pixel_sub_ps_32x32_sve2);
    p.cu[BLOCK_64x64].sub_ps = PFX(pixel_sub_ps_64x64_sve2);

    // chroma sub_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].sub_ps   = PFX(pixel_sub_ps_4x4_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].sub_ps   = PFX(pixel_sub_ps_8x8_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].sub_ps = PFX(pixel_sub_ps_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].sub_ps = PFX(pixel_sub_ps_32x32_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].sub_ps   = PFX(pixel_sub_ps_4x8_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sub_ps  = PFX(pixel_sub_ps_8x16_sve);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sub_ps = PFX(pixel_sub_ps_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sub_ps = PFX(pixel_sub_ps_32x64_sve2);

    // pixel_add_ps
    p.cu[BLOCK_4x4].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x4_sve2);
    p.cu[BLOCK_8x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_8x8_sve2);
    p.cu[BLOCK_16x16].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x16_sve2);
    p.cu[BLOCK_32x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x32_sve2);
    p.cu[BLOCK_64x64].add_ps[NONALIGNED] = PFX(pixel_add_ps_64x64_sve2);

    p.cu[BLOCK_4x4].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x4_sve2);
    p.cu[BLOCK_8x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_8x8_sve2);
    p.cu[BLOCK_16x16].add_ps[ALIGNED] = PFX(pixel_add_ps_16x16_sve2);
    p.cu[BLOCK_32x32].add_ps[ALIGNED] = PFX(pixel_add_ps_32x32_sve2);
    p.cu[BLOCK_64x64].add_ps[ALIGNED] = PFX(pixel_add_ps_64x64_sve2);

    // chroma add_ps
    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x4_sve2);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_8x8_sve2);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x16_sve2);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x32_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].add_ps[NONALIGNED]   = PFX(pixel_add_ps_4x8_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].add_ps[NONALIGNED]  = PFX(pixel_add_ps_8x16_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].add_ps[NONALIGNED] = PFX(pixel_add_ps_16x32_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].add_ps[NONALIGNED] = PFX(pixel_add_ps_32x64_sve2);

    p.chroma[X265_CSP_I420].cu[BLOCK_420_4x4].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x4_sve2);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_8x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_8x8_sve2);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_16x16].add_ps[ALIGNED] = PFX(pixel_add_ps_16x16_sve2);
    p.chroma[X265_CSP_I420].cu[BLOCK_420_32x32].add_ps[ALIGNED] = PFX(pixel_add_ps_32x32_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_4x8].add_ps[ALIGNED]   = PFX(pixel_add_ps_4x8_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].add_ps[ALIGNED]  = PFX(pixel_add_ps_8x16_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].add_ps[ALIGNED] = PFX(pixel_add_ps_16x32_sve2);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].add_ps[ALIGNED] = PFX(pixel_add_ps_32x64_sve2);

    //scale2D_64to32
    p.scale2D_64to32  = PFX(scale2D_64to32_neon);

    // scale1D_128to64
    p.scale1D_128to64[NONALIGNED] = PFX(scale1D_128to64_sve2);
    p.scale1D_128to64[ALIGNED] = PFX(scale1D_128to64_sve2);

    // planecopy
    p.planecopy_cp = PFX(pixel_planecopy_cp_neon);

    // satd
    p.pu[LUMA_4x4].satd   = PFX(pixel_satd_4x4_sve);
    p.pu[LUMA_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.pu[LUMA_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.pu[LUMA_32x32].satd = PFX(pixel_satd_32x32_sve);
    p.pu[LUMA_64x64].satd = PFX(pixel_satd_64x64_neon);
    p.pu[LUMA_8x4].satd   = PFX(pixel_satd_8x4_sve);
    p.pu[LUMA_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.pu[LUMA_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.pu[LUMA_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.pu[LUMA_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.pu[LUMA_32x16].satd = PFX(pixel_satd_32x16_sve);
    p.pu[LUMA_64x32].satd = PFX(pixel_satd_64x32_neon);
    p.pu[LUMA_32x64].satd = PFX(pixel_satd_32x64_neon);
    p.pu[LUMA_16x12].satd = PFX(pixel_satd_16x12_neon);
    p.pu[LUMA_12x16].satd = PFX(pixel_satd_12x16_neon);
    p.pu[LUMA_16x4].satd  = PFX(pixel_satd_16x4_neon);
    p.pu[LUMA_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.pu[LUMA_32x24].satd = PFX(pixel_satd_32x24_neon);
    p.pu[LUMA_24x32].satd = PFX(pixel_satd_24x32_neon);
    p.pu[LUMA_32x8].satd  = PFX(pixel_satd_32x8_neon);
    p.pu[LUMA_8x32].satd  = PFX(pixel_satd_8x32_neon);
    p.pu[LUMA_64x48].satd = PFX(pixel_satd_64x48_sve);
    p.pu[LUMA_48x64].satd = PFX(pixel_satd_48x64_neon);
    p.pu[LUMA_64x16].satd = PFX(pixel_satd_64x16_neon);
    p.pu[LUMA_16x64].satd = PFX(pixel_satd_16x64_neon);

    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x4].satd   = PFX(pixel_satd_4x4_sve);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x32].satd = PFX(pixel_satd_32x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x4].satd   = PFX(pixel_satd_8x4_sve);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x16].satd = PFX(pixel_satd_32x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x12].satd = PFX(pixel_satd_16x12_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_12x16].satd = PFX(pixel_satd_12x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_16x4].satd  = PFX(pixel_satd_16x4_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x24].satd = PFX(pixel_satd_32x24_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_24x32].satd = PFX(pixel_satd_24x32_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_32x8].satd  = PFX(pixel_satd_32x8_neon);
    p.chroma[X265_CSP_I420].pu[CHROMA_420_8x32].satd  = PFX(pixel_satd_8x32_neon);

    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x8].satd   = PFX(pixel_satd_4x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x16].satd  = PFX(pixel_satd_8x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x32].satd = PFX(pixel_satd_16x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x64].satd = PFX(pixel_satd_32x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x4].satd   = PFX(pixel_satd_4x4_sve);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x8].satd   = PFX(pixel_satd_8x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x16].satd  = PFX(pixel_satd_4x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x16].satd = PFX(pixel_satd_16x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x32].satd  = PFX(pixel_satd_8x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x32].satd = PFX(pixel_satd_32x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x64].satd = PFX(pixel_satd_16x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x12].satd  = PFX(pixel_satd_8x12_sve);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x4].satd   = PFX(pixel_satd_8x4_sve);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x24].satd = PFX(pixel_satd_16x24_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_12x32].satd = PFX(pixel_satd_12x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_16x8].satd  = PFX(pixel_satd_16x8_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_4x32].satd  = PFX(pixel_satd_4x32_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x48].satd = PFX(pixel_satd_32x48_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_24x64].satd = PFX(pixel_satd_24x64_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_32x16].satd = PFX(pixel_satd_32x16_neon);
    p.chroma[X265_CSP_I422].pu[CHROMA_422_8x64].satd  = PFX(pixel_satd_8x64_neon);

    // sa8d
    p.cu[BLOCK_4x4].sa8d   = PFX(pixel_satd_4x4_sve);
    p.cu[BLOCK_8x8].sa8d   = PFX(pixel_sa8d_8x8_neon);
    p.cu[BLOCK_16x16].sa8d = PFX(pixel_sa8d_16x16_neon);
    p.cu[BLOCK_32x32].sa8d = PFX(pixel_sa8d_32x32_neon);
    p.cu[BLOCK_64x64].sa8d = PFX(pixel_sa8d_64x64_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_8x8].sa8d = PFX(pixel_satd_4x4_sve);
    p.chroma[X265_CSP_I420].cu[BLOCK_16x16].sa8d = PFX(pixel_sa8d_16x16_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_32x32].sa8d = PFX(pixel_sa8d_32x32_neon);
    p.chroma[X265_CSP_I420].cu[BLOCK_64x64].sa8d = PFX(pixel_sa8d_64x64_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_8x16].sa8d = PFX(pixel_sa8d_8x16_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_16x32].sa8d = PFX(pixel_sa8d_16x32_neon);
    p.chroma[X265_CSP_I422].cu[BLOCK_422_32x64].sa8d = PFX(pixel_sa8d_32x64_neon);

    // dequant_scaling
    p.dequant_scaling = PFX(dequant_scaling_sve2);
    p.dequant_normal  = PFX(dequant_normal_sve2);

    // ssim_4x4x2_core
    p.ssim_4x4x2_core = PFX(ssim_4x4x2_core_sve2);

    // ssimDist
    p.cu[BLOCK_4x4].ssimDist = PFX(ssimDist4_sve2);
    p.cu[BLOCK_8x8].ssimDist = PFX(ssimDist8_sve2);
    p.cu[BLOCK_16x16].ssimDist = PFX(ssimDist16_sve2);
    p.cu[BLOCK_32x32].ssimDist = PFX(ssimDist32_sve2);
    p.cu[BLOCK_64x64].ssimDist = PFX(ssimDist64_sve2);

    // normFact
    p.cu[BLOCK_8x8].normFact = PFX(normFact8_sve2);
    p.cu[BLOCK_16x16].normFact = PFX(normFact16_sve2);
    p.cu[BLOCK_32x32].normFact = PFX(normFact32_sve2);
    p.cu[BLOCK_64x64].normFact = PFX(normFact64_sve2);

    // psy_cost_pp
    p.cu[BLOCK_4x4].psy_cost_pp = PFX(psyCost_4x4_neon);

    p.weight_pp = PFX(weight_pp_neon);
#if !defined(__APPLE__)
    p.scanPosLast = PFX(scanPosLast_neon);
#endif
    p.costCoeffNxN = PFX(costCoeffNxN_neon);
#endif

    // quant
    p.quant = PFX(quant_sve);
    p.nquant = PFX(nquant_neon);
}
#endif

void setupAssemblyPrimitives(EncoderPrimitives &p, int cpuMask)
{

#ifdef HAVE_SVE2
    if (cpuMask & X265_CPU_SVE2)
    {
        setupSve2Primitives(p);
    }
    else if (cpuMask & X265_CPU_SVE)
    {
        setupSvePrimitives(p);
    }
    else if (cpuMask & X265_CPU_NEON)
    {
        setupNeonPrimitives(p);
    }

#elif defined(HAVE_SVE)
    if (cpuMask & X265_CPU_SVE)
    {
        setupSvePrimitives(p);
    }
    else if (cpuMask & X265_CPU_NEON)
    {
        setupNeonPrimitives(p);
    }

#else
    if (cpuMask & X265_CPU_NEON)
    {
        setupNeonPrimitives(p);
    }
#endif

}
} // namespace X265_NS
