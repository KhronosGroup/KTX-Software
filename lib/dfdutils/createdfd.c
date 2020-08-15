/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @~English
 * @brief Utilities for creating data format descriptors.
 */

/*
 * Author: Andrew Garrard
 */

#include <stdlib.h>
#include <KHR/khr_df.h>

#include "dfd.h"

static uint32_t *writeHeader(int numSamples, int bytes, int suffix)
{
    uint32_t *DFD = (uint32_t *) malloc(sizeof(uint32_t) *
                                        (1 + KHR_DF_WORD_SAMPLESTART +
                                         numSamples * KHR_DF_WORD_SAMPLEWORDS));
    uint32_t* BDFD = DFD+1;
    DFD[0] = sizeof(uint32_t) *
        (1 + KHR_DF_WORD_SAMPLESTART +
         numSamples * KHR_DF_WORD_SAMPLEWORDS);
    BDFD[KHR_DF_WORD_VENDORID] =
        (KHR_DF_VENDORID_KHRONOS << KHR_DF_SHIFT_VENDORID) |
        (KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT << KHR_DF_SHIFT_DESCRIPTORTYPE);
    BDFD[KHR_DF_WORD_VERSIONNUMBER] =
        (KHR_DF_VERSIONNUMBER_LATEST << KHR_DF_SHIFT_VERSIONNUMBER) |
        (((uint32_t)sizeof(uint32_t) *
          (KHR_DF_WORD_SAMPLESTART +
           numSamples * KHR_DF_WORD_SAMPLEWORDS)
          << KHR_DF_SHIFT_DESCRIPTORBLOCKSIZE));
    BDFD[KHR_DF_WORD_MODEL] =
        ((KHR_DF_MODEL_RGBSDA << KHR_DF_SHIFT_MODEL) | /* Only supported model */
         (KHR_DF_PRIMARIES_BT709 << KHR_DF_SHIFT_PRIMARIES) | /* Assumed */
         (KHR_DF_FLAG_ALPHA_STRAIGHT << KHR_DF_SHIFT_FLAGS));
    if (suffix == s_SRGB) {
        BDFD[KHR_DF_WORD_TRANSFER] |= KHR_DF_TRANSFER_SRGB << KHR_DF_SHIFT_TRANSFER;
    } else {
        BDFD[KHR_DF_WORD_TRANSFER] |= KHR_DF_TRANSFER_LINEAR << KHR_DF_SHIFT_TRANSFER;
    }
    BDFD[KHR_DF_WORD_TEXELBLOCKDIMENSION0] = 0; /* Only 1x1x1x1 texel blocks supported */
    BDFD[KHR_DF_WORD_BYTESPLANE0] = bytes; /* bytesPlane0 = bytes, bytesPlane3..1 = 0 */
    BDFD[KHR_DF_WORD_BYTESPLANE4] = 0; /* bytesPlane7..5 = 0 */
    return DFD;
}

static uint32_t setChannelFlags(uint32_t channel, enum VkSuffix suffix)
{
    switch (suffix) {
    case s_UNORM: break;
    case s_SNORM:
        channel |=
            KHR_DF_SAMPLE_DATATYPE_SIGNED;
        break;
    case s_USCALED: break;
    case s_SSCALED:
        channel |=
            KHR_DF_SAMPLE_DATATYPE_SIGNED;
        break;
    case s_UINT: break;
    case s_SINT:
        channel |=
            KHR_DF_SAMPLE_DATATYPE_SIGNED;
        break;
    case s_SFLOAT:
        channel |=
            KHR_DF_SAMPLE_DATATYPE_FLOAT |
            KHR_DF_SAMPLE_DATATYPE_SIGNED;
        break;
    case s_UFLOAT:
        channel |=
            KHR_DF_SAMPLE_DATATYPE_FLOAT;
        break;
    case s_SRGB:
        if (channel == KHR_DF_CHANNEL_RGBSDA_ALPHA) {
            channel |= KHR_DF_SAMPLE_DATATYPE_LINEAR;
        }
        break;
    }
    return channel;
}

static void writeSample(uint32_t *DFD, int sampleNo, int channel,
                        int bits, int offset,
                        int topSample, int bottomSample, enum VkSuffix suffix)
{
    uint32_t lower, upper;
    uint32_t *sample = DFD + 1 + KHR_DF_WORD_SAMPLESTART + sampleNo * KHR_DF_WORD_SAMPLEWORDS;
    if (channel == 3) channel = KHR_DF_CHANNEL_RGBSDA_ALPHA;
    channel = setChannelFlags(channel, suffix);

    sample[KHR_DF_SAMPLEWORD_BITOFFSET] =
        (offset << KHR_DF_SAMPLESHIFT_BITOFFSET) |
        ((bits - 1) << KHR_DF_SAMPLESHIFT_BITLENGTH) |
        (channel << KHR_DF_SAMPLESHIFT_CHANNELID);

    sample[KHR_DF_SAMPLEWORD_SAMPLEPOSITION_ALL] = 0;

    switch (suffix) {
    case s_UNORM:
    case s_SRGB:
    default:
        if (bits > 32) {
            upper = 0xFFFFFFFFU;
        } else {
            upper = (uint32_t)((1U << bits) - 1U);
        }
        lower = 0U;
        break;
    case s_SNORM:
        if (bits > 32) {
            upper = 0x7FFFFFFF;
        } else {
            upper = topSample ? (1U << (bits - 1)) - 1 : (1U << bits) - 1;
        }
        lower = ~upper;
        if (bottomSample) lower += 1;
        break;
    case s_USCALED:
    case s_UINT:
        upper = bottomSample ? 1U : 0U;
        lower = 0U;
        break;
    case s_SSCALED:
    case s_SINT:
        upper = bottomSample ? 1U : 0U;
        lower = ~0U;
        break;
    case s_SFLOAT:
        *((float*)(void*)&upper) = 1.0f;
        *((float*)(void*)&lower) = -1.0f;
        break;
    case s_UFLOAT:
        *((float*)(void*)&upper) = 1.0f;
        *((float*)(void*)&lower) = 0.0f;
        break;
    }
    sample[KHR_DF_SAMPLEWORD_SAMPLELOWER] = lower;
    sample[KHR_DF_SAMPLEWORD_SAMPLEUPPER] = upper;
}

/**
 * @~English
 * @brief Create a Data Format Descriptor for an unpacked format.
 *
 * @param bigEndian Set to 1 for big-endian byte ordering and
                    0 for little-endian byte ordering.
 * @param numChannels The number of color channels.
 * @param bytes The number of bytes per channel.
 * @param redBlueSwap Normally channels appear in consecutive R, G, B, A order
 *                    in memory; redBlueSwap inverts red and blue, allowing
 *                    B, G, R, A.
 * @param suffix Indicates the format suffix for the type.
 *
 * @return A data format descriptor in malloc'd data. The caller is responsible
 *         for freeing the descriptor.
 **/
uint32_t *createDFDUnpacked(int bigEndian, int numChannels, int bytes,
                            int redBlueSwap, enum VkSuffix suffix)
{
    uint32_t *DFD;
    if (bigEndian) {
        int channelCounter, channelByte;
        /* Number of samples = number of channels * bytes per channel */
        DFD = writeHeader(numChannels * bytes, numChannels * bytes, suffix);
        /* First loop over the channels */
        for (channelCounter = 0; channelCounter < numChannels; ++channelCounter) {
            int channel = channelCounter;
            if (redBlueSwap && (channel == 0 || channel == 2)) {
                channel ^= 2;
            }
            /* Loop over the bytes that constitute a channel */
            for (channelByte = 0; channelByte < bytes; ++channelByte) {
                writeSample(DFD, channelCounter * bytes + channelByte, channel,
                            8, 8 * (channelCounter * bytes + bytes - channelByte - 1),
                            channelByte == bytes-1, channelByte == 0, suffix);
            }
        }

    } else { /* Little-endian */

        int sampleCounter;
        /* One sample per channel */
        DFD = writeHeader(numChannels, numChannels * bytes, suffix);
        for (sampleCounter = 0; sampleCounter < numChannels; ++sampleCounter) {
            int channel = sampleCounter;
            if (redBlueSwap && (channel == 0 || channel == 2)) {
                channel ^= 2;
            }
            writeSample(DFD, sampleCounter, channel,
                        8 * bytes, 8 * sampleCounter * bytes,
                        1, 1, suffix);
        }
    }
    return DFD;
}

/**
 * @~English
 * @brief Create a Data Format Descriptor for a packed format.
 *
 * @param bigEndian Big-endian flag: Set to 1 for big-endian byte ordering and
 *                  0 for little-endian byte ordering.
 * @param numChannels The number of color channels.
 * @param bits[] An array of length numChannels.
 *               Each entry is the number of bits composing the channel, in
 *               order starting at bit 0 of the packed type.
 * @param channels[] An array of length numChannels.
 *                   Each entry enumerates the channel type: 0 = red, 1 = green,
 *                   2 = blue, 15 = alpha, in order starting at bit 0 of the
 *                   packed type. These values match channel IDs for RGBSDA in
 *                   the Khronos Data Format header. To simplify iteration
 *                   through channels, channel id 3 is a synonym for alpha.
 * @param suffix Indicates the format suffix for the type.
 *
 * @return A data format descriptor in malloc'd data. The caller is responsible
 *         for freeing the descriptor.
 **/
uint32_t *createDFDPacked(int bigEndian, int numChannels,
                          int bits[], int channels[],
                          enum VkSuffix suffix)
{
    uint32_t *DFD = 0;
    if (numChannels == 6) {
        /* Special case E5B9G9R9 */
        DFD = writeHeader(numChannels, 4, s_UFLOAT);
        writeSample(DFD, 0, 0,
                    9, 0,
                    1, 1, s_UNORM);
        KHR_DFDSETSVAL((DFD+1), 0, SAMPLEUPPER, 256);
        writeSample(DFD, 1, 0 | KHR_DF_SAMPLE_DATATYPE_EXPONENT,
                    5, 27,
                    1, 1, s_UNORM);
        KHR_DFDSETSVAL((DFD+1), 1, SAMPLEUPPER, 15);
        writeSample(DFD, 2, 1,
                    9, 9,
                    1, 1, s_UNORM);
        KHR_DFDSETSVAL((DFD+1), 2, SAMPLEUPPER, 256);
        writeSample(DFD, 3, 1 | KHR_DF_SAMPLE_DATATYPE_EXPONENT,
                    5, 27,
                    1, 1, s_UNORM);
        KHR_DFDSETSVAL((DFD+1), 3, SAMPLEUPPER, 15);
        writeSample(DFD, 4, 3,
                    9, 18,
                    1, 1, s_UNORM);
        KHR_DFDSETSVAL((DFD+1), 4, SAMPLEUPPER, 256);
        writeSample(DFD, 5, 3 | KHR_DF_SAMPLE_DATATYPE_EXPONENT,
                    5, 27,
                    1, 1, s_UNORM);
        KHR_DFDSETSVAL((DFD+1), 5, SAMPLEUPPER, 15);
    } else if (bigEndian) {
        /* No packed format is larger than 32 bits. */
        /* No packed channel crosses more than two bytes. */
        int totalBits = 0;
        int bitChannel[32];
        int beChannelStart[4];
        int channelCounter;
        int bitOffset = 0;
        int BEMask;
        int numSamples = numChannels;
        int sampleCounter;
        for (channelCounter = 0; channelCounter < numChannels; ++channelCounter) {
            beChannelStart[channelCounter] = totalBits;
            totalBits += bits[channelCounter];
        }
        BEMask = (totalBits - 1) & 0x18;
        for (channelCounter = 0; channelCounter < numChannels; ++channelCounter) {
            bitChannel[bitOffset ^ BEMask] = channelCounter;
            if (((bitOffset + bits[channelCounter] - 1) & ~7) != (bitOffset & ~7)) {
                /* Continuation sample */
                bitChannel[((bitOffset + bits[channelCounter] - 1) & ~7) ^ BEMask] = channelCounter;
                numSamples++;
            }
            bitOffset += bits[channelCounter];
        }
        DFD = writeHeader(numSamples, totalBits >> 3, suffix);

        sampleCounter = 0;
        for (bitOffset = 0; bitOffset < totalBits;) {
            if (bitChannel[bitOffset] == -1) {
                /* Done this bit, so this is the lower half of something. */
                /* We must therefore jump to the end of the byte and continue. */
                bitOffset = (bitOffset + 8) & ~7;
            } else {
                /* Start of a channel? */
                int thisChannel = bitChannel[bitOffset];
                if ((beChannelStart[thisChannel] ^ BEMask) == bitOffset) {
                    /* Must be just one sample if we hit it first. */
                    writeSample(DFD, sampleCounter++, channels[thisChannel],
                                    bits[thisChannel], bitOffset,
                                    1, 1, suffix);
                    bitOffset += bits[thisChannel];
                } else {
                    /* Two samples. Move to the end of the first one we hit when we're done. */
                    int firstSampleBits = 8 - (beChannelStart[thisChannel] & 0x7); /* Rest of the byte */
                    int secondSampleBits = bits[thisChannel] - firstSampleBits; /* Rest of the bits */
                    writeSample(DFD, sampleCounter++, channels[thisChannel],
                                firstSampleBits, beChannelStart[thisChannel] ^ BEMask,
                                0, 1, suffix);
                    /* Mark that we've already handled this sample */
                    bitChannel[beChannelStart[thisChannel] ^ BEMask] = -1;
                    writeSample(DFD, sampleCounter++, channels[thisChannel],
                                secondSampleBits, bitOffset,
                                1, 0, suffix);
                    bitOffset += secondSampleBits;
                }
            }
        }

    } else { /* Little-endian */

        int sampleCounter;
        int totalBits = 0;
        int bitOffset = 0;
        for (sampleCounter = 0; sampleCounter < numChannels; ++sampleCounter) {
            totalBits += bits[sampleCounter];
        }

        /* One sample per channel */
        DFD = writeHeader(numChannels, totalBits >> 3, suffix);
        for (sampleCounter = 0; sampleCounter < numChannels; ++sampleCounter) {
            writeSample(DFD, sampleCounter, channels[sampleCounter],
                        bits[sampleCounter], bitOffset,
                        1, 1, suffix);
            bitOffset += bits[sampleCounter];
        }
    }
    return DFD;
}

static khr_df_model_e compModelMapping[] = {
    KHR_DF_MODEL_BC1A,   /*!< BC1, aka DXT1, no alpha. */
    KHR_DF_MODEL_BC1A,   /*!< BC1, aka DXT1, punch-through alpha. */
    KHR_DF_MODEL_BC2,    /*!< BC2, aka DXT2 and DXT3. */
    KHR_DF_MODEL_BC3,    /*!< BC3, aka DXT4 and DXT5. */
    KHR_DF_MODEL_BC4,    /*!< BC4. */
    KHR_DF_MODEL_BC5,    /*!< BC5. */
    KHR_DF_MODEL_BC6H,   /*!< BC6h HDR format. */
    KHR_DF_MODEL_BC7,    /*!< BC7. */
    KHR_DF_MODEL_ETC2,   /*!< ETC2 no alpha. */
    KHR_DF_MODEL_ETC2,   /*!< ETC2 punch-through alpha. */
    KHR_DF_MODEL_ETC2,   /*!< ETC2 independent alpha. */
    KHR_DF_MODEL_ETC2,   /*!< R11 ETC2 single-channel. */
    KHR_DF_MODEL_ETC2,   /*!< R11G11 ETC2 dual-channel. */
    KHR_DF_MODEL_ASTC,   /*!< ASTC. */
    KHR_DF_MODEL_ETC1S,  /*!< ETC1S. */
    KHR_DF_MODEL_PVRTC,  /*!< PVRTC(1). */
    KHR_DF_MODEL_PVRTC2  /*!< PVRTC2. */
};

static uint32_t compSampleCount[] = {
    1U, /*!< BC1, aka DXT1, no alpha. */
    1U, /*!< BC1, aka DXT1, punch-through alpha. */
    2U, /*!< BC2, aka DXT2 and DXT3. */
    2U, /*!< BC3, aka DXT4 and DXT5. */
    1U, /*!< BC4. */
    2U, /*!< BC5. */
    1U, /*!< BC6h HDR format. */
    1U, /*!< BC7. */
    1U, /*!< ETC2 no alpha. */
    2U, /*!< ETC2 punch-through alpha. */
    2U, /*!< ETC2 independent alpha. */
    1U, /*!< R11 ETC2 single-channel. */
    2U, /*!< R11G11 ETC2 dual-channel. */
    1U, /*!< ASTC. */
    1U, /*!< ETC1S. */
    1U, /*!< PVRTC. */
    1U  /*!< PVRTC2. */
};

static khr_df_model_channels_e compFirstChannel[] = {
    KHR_DF_CHANNEL_BC1A_COLOR,        /*!< BC1, aka DXT1, no alpha. */
    KHR_DF_CHANNEL_BC1A_ALPHAPRESENT, /*!< BC1, aka DXT1, punch-through alpha. */
    KHR_DF_CHANNEL_BC2_ALPHA,         /*!< BC2, aka DXT2 and DXT3. */
    KHR_DF_CHANNEL_BC3_ALPHA,         /*!< BC3, aka DXT4 and DXT5. */
    KHR_DF_CHANNEL_BC4_DATA,          /*!< BC4. */
    KHR_DF_CHANNEL_BC5_RED,           /*!< BC5. */
    KHR_DF_CHANNEL_BC6H_COLOR,        /*!< BC6h HDR format. */
    KHR_DF_CHANNEL_BC7_COLOR,         /*!< BC7. */
    KHR_DF_CHANNEL_ETC2_COLOR,        /*!< ETC2 no alpha. */
    KHR_DF_CHANNEL_ETC2_COLOR,        /*!< ETC2 punch-through alpha. */
    KHR_DF_CHANNEL_ETC2_ALPHA,        /*!< ETC2 independent alpha. */
    KHR_DF_CHANNEL_ETC2_RED,          /*!< R11 ETC2 single-channel. */
    KHR_DF_CHANNEL_ETC2_RED,          /*!< R11G11 ETC2 dual-channel. */
    KHR_DF_CHANNEL_ASTC_DATA,         /*!< ASTC. */
    KHR_DF_CHANNEL_ETC1S_RGB,         /*!< ETC1S. */
    KHR_DF_CHANNEL_PVRTC_COLOR,       /*!< PVRTC. */
    KHR_DF_CHANNEL_PVRTC2_COLOR       /*!< PVRTC2. */
};

static khr_df_model_channels_e compSecondChannel[] = {
    KHR_DF_CHANNEL_BC1A_COLOR,        /*!< BC1, aka DXT1, no alpha. */
    KHR_DF_CHANNEL_BC1A_ALPHAPRESENT, /*!< BC1, aka DXT1, punch-through alpha. */
    KHR_DF_CHANNEL_BC2_COLOR,         /*!< BC2, aka DXT2 and DXT3. */
    KHR_DF_CHANNEL_BC3_COLOR,         /*!< BC3, aka DXT4 and DXT5. */
    KHR_DF_CHANNEL_BC4_DATA,          /*!< BC4. */
    KHR_DF_CHANNEL_BC5_GREEN,         /*!< BC5. */
    KHR_DF_CHANNEL_BC6H_COLOR,        /*!< BC6h HDR format. */
    KHR_DF_CHANNEL_BC7_COLOR,         /*!< BC7. */
    KHR_DF_CHANNEL_ETC2_COLOR,        /*!< ETC2 no alpha. */
    KHR_DF_CHANNEL_ETC2_ALPHA,        /*!< ETC2 punch-through alpha. */
    KHR_DF_CHANNEL_ETC2_COLOR,        /*!< ETC2 independent alpha. */
    KHR_DF_CHANNEL_ETC2_RED,          /*!< R11 ETC2 single-channel. */
    KHR_DF_CHANNEL_ETC2_GREEN,        /*!< R11G11 ETC2 dual-channel. */
    KHR_DF_CHANNEL_ASTC_DATA,         /*!< ASTC. */
    KHR_DF_CHANNEL_ETC1S_RGB,         /*!< ETC1S. */
    KHR_DF_CHANNEL_PVRTC_COLOR,       /*!< PVRTC. */
    KHR_DF_CHANNEL_PVRTC2_COLOR       /*!< PVRTC2. */
};

static uint32_t compSecondChannelOffset[] = {
    0U,  /*!< BC1, aka DXT1, no alpha. */
    0U,  /*!< BC1, aka DXT1, punch-through alpha. */
    64U, /*!< BC2, aka DXT2 and DXT3. */
    64U, /*!< BC3, aka DXT4 and DXT5. */
    0U,  /*!< BC4. */
    64U, /*!< BC5. */
    0U,  /*!< BC6h HDR format. */
    0U,  /*!< BC7. */
    0U,  /*!< ETC2 no alpha. */
    0U,  /*!< ETC2 punch-through alpha. */
    64U, /*!< ETC2 independent alpha. */
    0U,  /*!< R11 ETC2 single-channel. */
    64U, /*!< R11G11 ETC2 dual-channel. */
    0U,  /*!< ASTC. */
    0U,  /*!< ETC1S. */
    0U,  /*!< PVRTC. */
    0U   /*!< PVRTC2. */
};

static uint32_t compChannelBits[] = {
    64U,  /*!< BC1, aka DXT1, no alpha. */
    64U,  /*!< BC1, aka DXT1, punch-through alpha. */
    64U,  /*!< BC2, aka DXT2 and DXT3. */
    64U,  /*!< BC3, aka DXT4 and DXT5. */
    64U,  /*!< BC4. */
    64U,  /*!< BC5. */
    128U, /*!< BC6h HDR format. */
    128U, /*!< BC7. */
    64U,  /*!< ETC2 no alpha. */
    64U,  /*!< ETC2 punch-through alpha. */
    64U,  /*!< ETC2 independent alpha. */
    64U,  /*!< R11 ETC2 single-channel. */
    64U,  /*!< R11G11 ETC2 dual-channel. */
    128U, /*!< ASTC. */
    64U,  /*!< ETC1S. */
    64U,  /*!< PVRTC. */
    64U   /*!< PVRTC2. */
};

static uint32_t compBytes[] = {
    8U,  /*!< BC1, aka DXT1, no alpha. */
    8U,  /*!< BC1, aka DXT1, punch-through alpha. */
    16U, /*!< BC2, aka DXT2 and DXT3. */
    16U, /*!< BC3, aka DXT4 and DXT5. */
    8U,  /*!< BC4. */
    16U, /*!< BC5. */
    16U, /*!< BC6h HDR format. */
    16U, /*!< BC7. */
    8U,  /*!< ETC2 no alpha. */
    8U,  /*!< ETC2 punch-through alpha. */
    16U, /*!< ETC2 independent alpha. */
    8U,  /*!< R11 ETC2 single-channel. */
    16U, /*!< R11G11 ETC2 dual-channel. */
    16U, /*!< ASTC. */
    8U,  /*!< ETC1S. */
    8U,  /*!< PVRTC. */
    8U   /*!< PVRTC2. */
};

/**
 * @~English
 * @brief Create a Data Format Descriptor for a compressed format.
 *
 * @param compScheme Vulkan-style compression scheme enumeration.
 * @param bwidth Block width in texel coordinates.
 * @param bheight Block height in texel coordinates.
 * @param bdepth Block depth in texel coordinates.
 * @author Mark Callow, Edgewise Consulting.
 * @param suffix Indicates the format suffix for the type.
 *
 * @return A data format descriptor in malloc'd data. The caller is responsible
 *         for freeing the descriptor.
 **/
uint32_t *createDFDCompressed(enum VkCompScheme compScheme, int bwidth, int bheight, int bdepth,
                              enum VkSuffix suffix)
{
    uint32_t *DFD = 0;
    uint32_t numSamples = compSampleCount[compScheme];
    uint32_t* BDFD;
    uint32_t *sample;
    uint32_t channel;
    uint32_t lower;
    uint32_t upper;

    DFD = (uint32_t *) malloc(sizeof(uint32_t) *
                              (1 + KHR_DF_WORD_SAMPLESTART +
                               numSamples * KHR_DF_WORD_SAMPLEWORDS));
    BDFD = DFD+1;
    DFD[0] = sizeof(uint32_t) *
        (1 + KHR_DF_WORD_SAMPLESTART +
         numSamples * KHR_DF_WORD_SAMPLEWORDS);
    BDFD[KHR_DF_WORD_VENDORID] =
        (KHR_DF_VENDORID_KHRONOS << KHR_DF_SHIFT_VENDORID) |
        (KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT << KHR_DF_SHIFT_DESCRIPTORTYPE);
    BDFD[KHR_DF_WORD_VERSIONNUMBER] =
        (KHR_DF_VERSIONNUMBER_LATEST << KHR_DF_SHIFT_VERSIONNUMBER) |
        (((uint32_t)sizeof(uint32_t) *
          (KHR_DF_WORD_SAMPLESTART +
           numSamples * KHR_DF_WORD_SAMPLEWORDS)
          << KHR_DF_SHIFT_DESCRIPTORBLOCKSIZE));
    BDFD[KHR_DF_WORD_MODEL] =
        ((compModelMapping[compScheme] << KHR_DF_SHIFT_MODEL) |
         (KHR_DF_PRIMARIES_BT709 << KHR_DF_SHIFT_PRIMARIES) | /* Assumed */
         (KHR_DF_FLAG_ALPHA_STRAIGHT << KHR_DF_SHIFT_FLAGS));

    if (suffix == s_SRGB) {
        BDFD[KHR_DF_WORD_TRANSFER] |= KHR_DF_TRANSFER_SRGB << KHR_DF_SHIFT_TRANSFER;
    } else {
        BDFD[KHR_DF_WORD_TRANSFER] |= KHR_DF_TRANSFER_LINEAR << KHR_DF_SHIFT_TRANSFER;
    }
    BDFD[KHR_DF_WORD_TEXELBLOCKDIMENSION0] =
        (bwidth - 1) | ((bheight - 1) << KHR_DF_SHIFT_TEXELBLOCKDIMENSION1) | ((bdepth - 1) << KHR_DF_SHIFT_TEXELBLOCKDIMENSION2);
    /* bytesPlane0 = bytes, bytesPlane3..1 = 0 */
    BDFD[KHR_DF_WORD_BYTESPLANE0] = compBytes[compScheme];
    BDFD[KHR_DF_WORD_BYTESPLANE4] = 0; /* bytesPlane7..5 = 0 */

    sample = BDFD + KHR_DF_WORD_SAMPLESTART;
    channel = compFirstChannel[compScheme];
    channel = setChannelFlags(channel, suffix);

    sample[KHR_DF_SAMPLEWORD_BITOFFSET] =
        (0 << KHR_DF_SAMPLESHIFT_BITOFFSET) |
        ((compChannelBits[compScheme] - 1) << KHR_DF_SAMPLESHIFT_BITLENGTH) |
        (channel << KHR_DF_SAMPLESHIFT_CHANNELID);

    sample[KHR_DF_SAMPLEWORD_SAMPLEPOSITION_ALL] = 0;
    switch (suffix) {
    case s_UNORM:
    case s_SRGB:
    default:
        upper = 0xFFFFFFFFU;
        lower = 0U;
        break;
    case s_SNORM:
        upper = 0x7FFFFFFF;
        lower = ~upper;
        break;
    case s_USCALED:
    case s_UINT:
        upper = 1U;
        lower = 0U;
        break;
    case s_SSCALED:
    case s_SINT:
        upper = 1U;
        lower = ~0U;
        break;
    case s_SFLOAT:
        *((float*)(void*)&upper) = 1.0f;
        *((float*)(void*)&lower) = -1.0f;
        break;
    case s_UFLOAT:
        *((float*)(void*)&upper) = 1.0f;
        *((float*)(void*)&lower) = 0.0f;
        break;
    }
    sample[KHR_DF_SAMPLEWORD_SAMPLELOWER] = lower;
    sample[KHR_DF_SAMPLEWORD_SAMPLEUPPER] = upper;

    if (compSampleCount[compScheme] > 1) {
        sample += KHR_DF_WORD_SAMPLEWORDS;
        channel = compSecondChannel[compScheme];
        channel = setChannelFlags(channel, suffix);

        sample[KHR_DF_SAMPLEWORD_BITOFFSET] =
            (compSecondChannelOffset[compScheme] << KHR_DF_SAMPLESHIFT_BITOFFSET) |
            ((compChannelBits[compScheme] - 1) << KHR_DF_SAMPLESHIFT_BITLENGTH) |
            (channel << KHR_DF_SAMPLESHIFT_CHANNELID);

        sample[KHR_DF_SAMPLEWORD_SAMPLEPOSITION_ALL] = 0;

        sample[KHR_DF_SAMPLEWORD_SAMPLELOWER] = lower;
        sample[KHR_DF_SAMPLEWORD_SAMPLEUPPER] = upper;
    }
    return DFD;
}

/**
 * @~English
 * @brief Create a Data Format Descriptor for a depth-stencil format.
 *
 * @param depthBits   The numeber of bits in the depth channel.
 * @param stencilBits The numeber of bits in the stencil channel.
 * @param sizeBytes   The total byte size of the texel.
 *
 * @return A data format descriptor in malloc'd data. The caller is responsible
 *         for freeing the descriptor.
 **/
uint32_t *createDFDDepthStencil(int depthBits,
                                int stencilBits,
                                int sizeBytes)
{
    /* N.B. Little-endian is assumed. */
    uint32_t *DFD = 0;
    DFD = writeHeader((depthBits > 0) + (stencilBits > 0),
                      sizeBytes, s_UNORM);
    if (depthBits == 32) {
        writeSample(DFD, 0, KHR_DF_CHANNEL_RGBSDA_DEPTH,
                    32, 0,
                    1, 1, s_SFLOAT);
    } else if (depthBits > 0) {
        writeSample(DFD, 0, KHR_DF_CHANNEL_RGBSDA_DEPTH,
                    depthBits, 0,
                    1, 1, s_UNORM);
    }
    if (stencilBits > 0) {
        if (depthBits > 0) {
            writeSample(DFD, 1, KHR_DF_CHANNEL_RGBSDA_STENCIL,
                        stencilBits, depthBits,
                        1, 1, s_UINT);
        } else {
            writeSample(DFD, 0, KHR_DF_CHANNEL_RGBSDA_STENCIL,
                        stencilBits, 0,
                        1, 1, s_UINT);
        }
    }
    return DFD;
}
