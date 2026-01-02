// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2023 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

// astcenc doesn't use the top 8 integer bits directly for sRGB RGB components
// or when using the decode_unorm8 decode mode. An alterantive is used which
// allows a common code path to be used. This test program shows that the two
// produce equivalent output once rounded to a decode_unorm8 output.

// Compile with e.g. clang++ astcenc_u8_test_bench.cpp -o astcenc_u8_test_bench -mavx2 -mf16c

#define ASTCENC_AVX 2
#define ASTCENC_F16C 1
#define ASTCENC_SSE 41

#include "../Source/astcenc_mathlib.cpp"
#include "../Source/astcenc_color_unquantize.cpp"
#include "../Source/astcenc_decompress_symbolic.cpp"

int main()
{
    printf("Decode mode test bench\n");

    for (int ep0 = 0; ep0 < 256; ep0++)
    {
        for (int ep1 = 0; ep1 < 256; ep1++)
        {
            for (int wt1 = 0; wt1 < 65; wt1++)
            {
                // Validate linear data with decode_unorm8 mode
                {
                    // Expand 8 bit to 16 bit
                    vint4 weights(wt1);
                    int ep0_v0 = ep0 * 257;
                    int ep1_v0 = ep1 * 257;

                    // Linear with decode_u8 handling
                    vmask4 decode_u8_v0(true, true, true, true);
                    vint4 ep0v0(ep0_v0, ep0_v0, ep0_v0, ep0_v0);
                    vint4 ep1v0(ep1_v0, ep1_v0, ep1_v0, ep1_v0);

                    // Linear without decode_u8 handling
                    vmask4 decode_u8_v1(false, false, false, false);
                    vint4 ep0v1(ep0_v0, ep0_v0, ep0_v0, ep0_v0);
                    vint4 ep1v1(ep1_v0, ep1_v0, ep1_v0, ep1_v0);

                    // Lerp both styles
                    vint4 colorv0 = lerp_color_int(decode_u8_v0, ep0v0, ep1v0, weights);
                    vint4 colorv1 = lerp_color_int(decode_u8_v1, ep0v1, ep1v1, weights);

                    // Validate top 8 integer bits match in both cases
                    //  - Shows that astcenc-style U8 doesn't differ from Khronos-style U8
                    vint4 cs0 = lsr<8>(colorv0);
                    vint4 cs1 = lsr<8>(colorv1);
                    assert(cs0.lane<0>() == cs1.lane<0>());
                    assert(cs0.lane<3>() == cs1.lane<3>());

                    // Validate that astcenc output matches the top 8 integer bits
                    vfloat4 colorv0f = decode_texel(colorv0, vmask4(false));
                    vint4 colorv0_out = float_to_int_rtn(colorv0f * 255.0f);
                    assert(colorv0_out.lane<0>() == cs0.lane<0>());
                }

                // Validate sRGB data with decode_unorm8 mode
                {
                    // Expand 8 bit to 16 bit
                    vint4 weights(wt1);
                    int ep0_v0s = (ep0 << 8) | 0x80;
                    int ep1_v0s = (ep1 << 8) | 0x80;
                    int ep0_v0 = ep0 * 257;
                    int ep1_v0 = ep1 * 257;

                    // sRGB RGB and linear A with decode_u8 handling
                    vmask4 decode_u8_v0(true, true, true, true);
                    vint4 ep0v0(ep0_v0s, ep0_v0s, ep0_v0s, ep0_v0);
                    vint4 ep1v0(ep1_v0s, ep1_v0s, ep1_v0s, ep1_v0);

                    // sRGB RGB and linear A without decode_u8 handling
                    vmask4 decode_u8_v1(false, false, false, false);
                    vint4 ep0v1(ep0_v0s, ep0_v0s, ep0_v0s, ep0_v0);
                    vint4 ep1v1(ep1_v0s, ep1_v0s, ep1_v0s, ep1_v0);

                    // Lerp both styles
                    vint4 colorv0 = lerp_color_int(decode_u8_v0, ep0v0, ep1v0, weights);
                    vint4 colorv1 = lerp_color_int(decode_u8_v1, ep0v1, ep1v1, weights);

                    // Validate top 8 integer bits match in both cases
                    //  - Shows that astcenc-style U8 doesn't differ from Khronos-style U8
                    vint4 cs0 = lsr<8>(colorv0);
                    vint4 cs1 = lsr<8>(colorv1);
                    assert(cs0.lane<0>() == cs1.lane<0>());
                    assert(cs0.lane<3>() == cs1.lane<3>());

                    // Validate that astcenc output matches the top 8 integer bits
                    vfloat4 colorv0f = decode_texel(colorv0, vmask4(false));
                    vint4 colorv0_out = float_to_int_rtn(colorv0f * 255.0f);
                    assert(colorv0_out.lane<0>() == cs0.lane<0>());
                }
            }
        }
    }

    return 0;
}
