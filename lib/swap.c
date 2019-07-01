/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* $Id$ */

/*
 * Copyright (c) 2010 The Khronos Group Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <KHR/khrplatform.h>
#include "ktx.h"

/*
 * SwapEndian16: Swaps endianness in an array of 16-bit values
 */
void
_ktxSwapEndian16(khronos_uint16_t* pData16, ktx_size_t count)
{
    for (ktx_size_t i = 0; i < count; ++i)
    {
        khronos_uint16_t x = *pData16;
        *pData16++ = (x << 8) | (x >> 8);
    }
}

/*
 * SwapEndian32: Swaps endianness in an array of 32-bit values
 */
void
_ktxSwapEndian32(khronos_uint32_t* pData32, ktx_size_t count)
{
    for (ktx_size_t i = 0; i < count; ++i)
    {
        khronos_uint32_t x = *pData32;
        *pData32++ = (x << 24) | ((x & 0xFF00) << 8) | ((x & 0xFF0000) >> 8) | (x >> 24);
    }
}

/*
 * SwapEndian364: Swaps endianness in an array of 32-bit values
 */
void
_ktxSwapEndian64(khronos_uint64_t* pData64, ktx_size_t count)
{
    for (ktx_size_t i = 0; i < count; ++i)
    {
        khronos_uint64_t x = *pData64;
        *pData64++ = (x << 56) | ((x & 0xFF00) << 40) | ((x & 0xFF0000) << 24)
                     | ((x & 0xFF000000) << 8 ) | ((x & 0xFF00000000) >> 8)
                     | ((x & 0xFF0000000000) >> 24)
                     | ((x & 0xFF000000000000) << 40) | (x >> 56);
    }
}



