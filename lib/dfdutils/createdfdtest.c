/* Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "dfd.h"

#define TESTRGB565 1

int main()
{
#ifdef TESTRGBA8888
    uint32_t *DFD = createDFDUnpacked(0, 4, 1, 1, s_UNORM);
#endif
#ifdef TESTRG16
    uint32_t *DFD = createDFDUnpacked(1, 2, 2, 0, s_UNORM);
#endif
#ifdef TESTRG64
    uint32_t *DFD = createDFDUnpacked(0, 2, 8, 0, s_UNORM);
#endif
#ifdef TESTRGBA4444
    int bits[] = {4,4,4,4};
    int channels[] = {3,2,1,0};
    uint32_t *DFD = createDFDPacked(1, 2, bits, channels, s_UNORM);
#endif
#ifdef TESTRGBA1010102
    int bits[] = {10,10,10,2};
    int channels[] = {0,1,2,3};
    /* 31 30 29 28 27 26 25 24   23 22 21 20 19 18 17 16   15 14 13 12 11 10  9  8    7  6  5  4  3  2  1  0 */
    /* A1 A0 B9 B8 B7 B6 B5 B4 | B3 B2 B1 B0 G9 G8 G7 G6 | G5 G4 G3 G2 G1 G0 R9 R8 | R7 R6 R5 R4 R3 R2 R1 R0 */
    /*  7  6  5  4  3  2  1  0   15 14 13 12 11 10  9  8   23 22 21 20 19 18 17 16   31 30 29 28 27 26 25 24 */
    uint32_t *DFD = createDFDPacked(0, 4, bits, channels, s_UNORM);
#endif
#ifdef TESTRGB565
    int bits[] = {5,6,5};
    int channels[] = {0,1,2};
    uint32_t *DFD = createDFDPacked(1, 3, bits, channels, s_UNORM);
#endif
    printDFD(DFD);
    return 0;
}
