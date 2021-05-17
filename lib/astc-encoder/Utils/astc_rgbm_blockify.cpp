// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2019-2020 Arm Limited
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

// This is a utility tool to blockify the M component of an RGBM image, writing
// the result back to a file on disk.
//
// This tool requires stb_image.h and stb_image_write.h single header libraries
// from: https://github.com/nothings/stb.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Force val[N+1] to be contrained to not be less than val[N]*(R/256)
void impose_ratio(uint8_t *data, int length, int stride, uint32_t ratio) {
    uint8_t val = *data;
    for (int i = 1; i < length; i++) {
        data += stride;

        int min_val = (val * ratio) >> 8;
        uint8_t vl2 = *data;
        if (vl2 < min_val) {
            *data = vl2 = min_val;
        }
        val = vl2;
    }
}

int main( int argc, char **argv ) {
    // Parse command line
    if (argc < 5) {
        printf("Usage:\n"
            "   %s <source> <dest> <blocksize> <ratio>\n"
            "where the arguments are:\n"
            "\n"
            "<source>    : Source image in RGBM representation.\n"
            "<dest>      : Destination PNG image file to write.\n"
            "<blocksize> : ASTC block size we are blockifying for; e.g. 6x6.\n"
            "<ratio>     : The maximum permitted ratio of M between two\n"
            "              adjacent blocks. Can be set to any value between\n"
            "              0.01 and 0.99. Recommend 0.7 to start with.\n"
            , argv[0]);
        exit(1);
    }

    int x_blockdim, y_blockdim;
    if (sscanf(argv[3], "%dx%d", &x_blockdim, &y_blockdim) < 2) {
        printf("Blockdim must be of form WxH; e.g. 8x4\n");
        exit(1);
    }

    double ratio_f = atof(argv[4]);
    if (ratio_f < 0.01 || ratio_f > 0.99) {
        printf("Ratio parameter must be in range 0.01 to 0.99\n");
    }

    int ratio_i = (int)floor(ratio_f * 256.0 + 0.5);

    // Load the image
    int xdim, ydim, ncomp;
    uint8_t *data = (uint8_t *)stbi_load(argv[1], &xdim, &ydim, &ncomp, 4);
    if (!data) {
        printf("Failed to load image \"%s\"\n", data );
        exit(1);
    }

    int x_blockcount = (xdim + x_blockdim - 1) / x_blockdim;
    int y_blockcount = (ydim + y_blockdim - 1) / y_blockdim;
    int y_leftover = y_blockdim * y_blockcount - ydim;

    uint8_t *mbuf = (uint8_t *)calloc(x_blockcount * y_blockcount, 1);

    // For each block, obtain the maximum M-value
    for (int y = 0; y < ydim; y++) {
        int ctr = 0;
        uint8_t *s = data + 4 * y * xdim + 3;
        uint8_t *d = mbuf + ((y + y_leftover) / y_blockdim) * x_blockcount;

        int accum = *d;
        for (int x = 0; x < xdim; x++) {
            uint8_t p = s[4*x];
            if (p > accum) {
                accum = p;
            }

            if(++ctr == x_blockdim) {
                *d++ = accum;
                accum = *d;
                ctr = 0;
            }
        }

        if (ctr != 0) {
            *d = accum;
        }
    }

    // Impose ratio restriction on M-values in adjacent blocks.
    for (int y = 0;  y < y_blockcount; y++) {
        impose_ratio(mbuf + y * x_blockcount, x_blockcount, 1, ratio_i);
        impose_ratio(mbuf + ((y + 1) * x_blockcount) - 1, x_blockcount, - 1, ratio_i);
    }

    for (int x = 0; x < x_blockcount; x++) {
        impose_ratio(mbuf + x, y_blockcount, x_blockcount, ratio_i);
        impose_ratio(mbuf + x + (x_blockcount * (y_blockcount - 1)), y_blockcount, -x_blockcount, ratio_i);
    }

    // For each pixel, scale the pixel RGB values based on chosen M
    for (int y = 0; y < ydim; y++) {
        int ctr = 0;
        uint8_t *s = data + 4 * y * xdim;
        uint8_t *d = mbuf + ((y + y_leftover) / y_blockdim) * x_blockcount;
        int mm = *d++;
        for (int x = 0; x < xdim; x++) {
            uint8_t m = s[4 * x + 3];
            if(m != mm) {
                uint8_t r = s[4 * x];
                uint8_t g = s[4 * x + 1];
                uint8_t b = s[4 * x + 2];
                s[4 * x]     = (r * m + (mm >> 1)) / mm;
                s[4 * x + 1] = (g * m + (mm >> 1)) / mm;
                s[4 * x + 2] = (b * m + (mm >> 1)) / mm;
                s[4 * x + 3] = mm;
            }

            if (++ctr == x_blockdim) {
                ctr = 0;
                mm = *d++;
            }
        }
    }

    // Write out the result
    stbi_write_png(argv[2], xdim, ydim, 4, data, 4*xdim);
    return 0;
}
