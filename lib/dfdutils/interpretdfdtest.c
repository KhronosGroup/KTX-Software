/* Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stdio.h>
#include <KHR/khr_df.h>
#include "dfd.h"

uint32_t dfd1[] = {
    4U + 4U * (KHR_DF_WORD_SAMPLESTART + (4U * KHR_DF_WORD_SAMPLEWORDS)),
    ((KHR_DF_VENDORID_KHRONOS << KHR_DF_SHIFT_VENDORID) |
     (KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT << KHR_DF_SHIFT_DESCRIPTORTYPE)),
    ((KHR_DF_VERSIONNUMBER_LATEST << KHR_DF_SHIFT_VERSIONNUMBER) |
     ((4U * (KHR_DF_WORD_SAMPLESTART + (4U * KHR_DF_WORD_SAMPLEWORDS)))
      << KHR_DF_SHIFT_DESCRIPTORBLOCKSIZE)),
    ((KHR_DF_MODEL_RGBSDA << KHR_DF_SHIFT_MODEL) |
     (KHR_DF_PRIMARIES_BT709 << KHR_DF_SHIFT_PRIMARIES) |
     (KHR_DF_TRANSFER_SRGB << KHR_DF_SHIFT_TRANSFER) |
     (KHR_DF_FLAG_ALPHA_PREMULTIPLIED << KHR_DF_SHIFT_FLAGS)),
    0U, /* Dimensions */
    4U, /* bytesPlane0 = 4 */
    0U, /* bytesPlane7..4 = 0 */
    /* Sample 0 */
    ((0U << KHR_DF_SAMPLESHIFT_BITOFFSET) |
     (7U << KHR_DF_SAMPLESHIFT_BITLENGTH) | /* Store length - 1 */
     (KHR_DF_CHANNEL_RGBSDA_RED << KHR_DF_SAMPLESHIFT_CHANNELID)),
    0U,   /* samplePosition */
    0U,   /* sampleLower */
    255U, /* sampleUpper */
    /* Sample 1 */
    ((8U << KHR_DF_SAMPLESHIFT_BITOFFSET) |
     (7U << KHR_DF_SAMPLESHIFT_BITLENGTH) | /* Store length - 1 */
     (KHR_DF_CHANNEL_RGBSDA_GREEN << KHR_DF_SAMPLESHIFT_CHANNELID)),
    0U,   /* samplePosition */
    0U,   /* sampleLower */
    255U, /* sampleUpper */
    /* Sample 2 */
    ((16U << KHR_DF_SAMPLESHIFT_BITOFFSET) |
     (7U << KHR_DF_SAMPLESHIFT_BITLENGTH) | /* Store length - 1 */
     (KHR_DF_CHANNEL_RGBSDA_BLUE << KHR_DF_SAMPLESHIFT_CHANNELID)),
    0U,   /* samplePosition */
    0U,   /* sampleLower */
    255U, /* sampleUpper */
    /* Sample 3 */
    ((24U << KHR_DF_SAMPLESHIFT_BITOFFSET) |
     (7U << KHR_DF_SAMPLESHIFT_BITLENGTH) | /* Store length - 1 */
     ((KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR) << KHR_DF_SAMPLESHIFT_CHANNELID))
};

uint32_t dfd2[] = { /* Little-endian unpacked */
    92U,
    0U,
    2U | (88U << 16U),
    KHR_DF_MODEL_RGBSDA | (KHR_DF_PRIMARIES_BT709 << 8) | (KHR_DF_TRANSFER_SRGB << 16) | (KHR_DF_FLAG_ALPHA_PREMULTIPLIED << 24),
    0U,
    8U,
    0U,
    /* Sample 0 */
    0U | (15U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    0xFFFFU,
    /* Sample 1 */
    16U | (15U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    0xFFFFU,
    /* Sample 2 */
    32U | (15U << 16) | (KHR_DF_CHANNEL_RGBSDA_BLUE << 24),
    0U,
    0U,
    0xFFFFU,
    /* Sample 3 */
    48U | (15U << 16) | ((KHR_DF_CHANNEL_RGBSDA_ALPHA | KHR_DF_SAMPLE_DATATYPE_LINEAR) << 24),
    0U,
    0U,
    0xFFFFU
};

uint32_t dfd3[] = { /* Big-endian unpacked */
    92U,
    0U,
    2U | (88U << 16U),
    KHR_DF_MODEL_RGBSDA | (KHR_DF_PRIMARIES_BT709 << 8) | (KHR_DF_TRANSFER_SRGB << 16) | (KHR_DF_FLAG_ALPHA_PREMULTIPLIED << 24),
    0U,
    8U,
    0U,
    /* Sample 0 */
    8U | (7U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    255U,
    /* Sample 1 */
    0U | (7U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    255U,
    /* Sample 2 */
    24U | (7U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    255U,
    /* Sample 3 */
    16U | (7U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    255U
};

uint32_t dfd4[] = { /* Little-endian packed */
    92U,
    0U,
    2U | (88U << 16U),
    KHR_DF_MODEL_RGBSDA | (KHR_DF_PRIMARIES_BT709 << 8) | (KHR_DF_TRANSFER_LINEAR << 16) | (KHR_DF_FLAG_ALPHA_PREMULTIPLIED << 24),
    0U,
    2U,
    0U,
    /* Sample 0 */
    0U | (3U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    7U,
    /* Sample 1 */
    4U | (3U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    7U,
    /* Sample 2 */
    8U | (3U << 16) | (KHR_DF_CHANNEL_RGBSDA_BLUE << 24),
    0U,
    0U,
    7U,
    /* Sample 3 */
    12U | (3U << 16) | (KHR_DF_CHANNEL_RGBSDA_ALPHA << 24),
    0U,
    0U,
    7U
};

uint32_t dfd5[] = { /* Big-endian packed */
    92U,
    0U,
    1U | (88U << 16U),
    KHR_DF_MODEL_RGBSDA | (KHR_DF_PRIMARIES_BT709 << 8) | (KHR_DF_TRANSFER_SRGB << 16) | (KHR_DF_FLAG_ALPHA_PREMULTIPLIED << 24),
    0U,
    2U,
    0U,
    /* Sample 0 (low bits of channel that touches bit 0) */
    13U | (2U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    7U,
    /* Sample 1 (high bits of channel that touches bit 0) */
    0U | (2U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    7U,
    /* Sample 2 */
    3U | (4U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    31U,
    /* Sample 3 */
    8U | (4U << 16) | (KHR_DF_CHANNEL_RGBSDA_BLUE << 24),
    0U,
    0U,
    31U
};

uint32_t dfd6[] = { /* Little-endian unpacked extended (N.B. could be done in two samples) */
    92U,
    0U,
    2U | (88U << 16U),
    KHR_DF_MODEL_RGBSDA | (KHR_DF_PRIMARIES_BT709 << 8) | (KHR_DF_TRANSFER_SRGB << 16) | (KHR_DF_FLAG_ALPHA_PREMULTIPLIED << 24),
    0U,
    16U,
    0U,
    /* Sample 0 */
    0U | (31U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    0xFFFFFFFFU,
    /* Sample 1 */
    32U | (31U << 16) | (KHR_DF_CHANNEL_RGBSDA_RED << 24),
    0U,
    0U,
    0xFFFFFFFFU,
    /* Sample 2 */
    64U | (31U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    0xFFFFFFFFU,
    /* Sample 3 */
    96U | (31U << 16) | (KHR_DF_CHANNEL_RGBSDA_GREEN << 24),
    0U,
    0U,
    0xFFFFFFFFU
};

int main()
{
    const char *errorText[] = {
        "UNSUPPORTED_NONTRIVIAL_ENDIANNESS",
        "UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS",
        "UNSUPPORTED_MULTIPLE_PLANES",
        "UNSUPPORTED_CHANNEL_TYPES",
        "UNSUPPORTED_MIXED_CHANNELS"};

    InterpretedDFDChannel R = {0,0}, G = {0,0}, B = {0,0}, A = {0,0};
    uint32_t wordSize;
    enum InterpretDFDResult t;

#if 0
    t = interpretDFD(dfd1, &R, &G, &B, &A, &wordSize);
#else
#if 0
    uint32_t *d = createDFDUnpacked(1,2,4,0,s_UNORM);
#elif 0
    int channels[] = {0,1,2,3};
    int bits[] = {10,10,10,2};
    uint32_t *d = createDFDPacked(1,4,bits,channels,s_UNORM);
#elif 0
    int channels[] = {0,1,2};
    int bits[] = {5,6,5};
    uint32_t *d = createDFDPacked(1,3,bits,channels,s_UNORM);
#elif 1
    uint32_t *d = createDFDUnpacked(0, 3, 1, 0, s_UNORM);
#endif

    printDFD(d);
    t = interpretDFD(d, &R, &G, &B, &A, &wordSize);
#endif

    if (t & i_UNSUPPORTED_ERROR_BIT) {
        printf("%s\n", errorText[t - i_UNSUPPORTED_ERROR_BIT]);
        return 0;
    } else {
        if (t & i_BIG_ENDIAN_FORMAT_BIT) {
            printf("Big-endian\n");
        } else {
            printf("Little-endian\n");
        }
        if (t & i_PACKED_FORMAT_BIT) {
            printf("Packed\n");
            if (R.size > 0) {
                printf("%u red bit%s starting at %u\n", R.size, R.size>1?"s":"", R.offset);
            }
            if (G.size > 0) {
                printf("%u green bit%s starting at %u\n", G.size, G.size>1?"s":"", G.offset);
            }
            if (B.size > 0) {
                printf("%u blue bit%s starting at %u\n", B.size, B.size>1?"s":"", B.offset);
            }
            if (A.size > 0) {
                printf("%u alpha bit%s starting at %u\n", A.size, A.size>1?"s":"", A.offset);
            }
            printf("Total word size %u\n", wordSize);
        } else {
            printf("Not packed\n");
            if (R.size > 0) {
                printf("%u red byte%s starting at %u\n", R.size, R.size>1?"s":"", R.offset);
            }
            if (G.size > 0) {
                printf("%u green byte%s starting at %u\n", G.size, G.size>1?"s":"", G.offset);
            }
            if (B.size > 0) {
                printf("%u blue byte%s starting at %u\n", B.size, B.size>1?"s":"", B.offset);
            }
            if (A.size > 0) {
                printf("%u alpha byte%s starting at %u\n", A.size, A.size>1?"s":"", A.offset);
            }
        }
        if (t & i_SRGB_FORMAT_BIT) {
            printf("sRGB\n");
        }
        if (t & i_NORMALIZED_FORMAT_BIT) {
            printf("Normalized\n");
        }
        if (t & i_SIGNED_FORMAT_BIT) {
            printf("Signed\n");
        }
        if (t & i_FLOAT_FORMAT_BIT) {
            printf("Float\n");
        }
    }
    return 0;
}
