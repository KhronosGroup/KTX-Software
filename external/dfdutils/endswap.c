/* Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <khr_df.h>

uint32_t *endSwapDFD(const uint32_t *DFD, size_t wordByteSize)
{
    /* Provides a DFD for what the buffer described by BDFD would be if it were end-swapped. */
    /* The DFD itself is described in uint32_t terms, so you're on your own for that. */
    /* This does assume that there's only a single location for each channel, and a single plane. */

    /* Build a representation of the bits of each channel in order, recording whence they came. */
    /* Then end-swap the locations and write a DFD for the result. */
    /* Worst case channel size is the number of bytes in the plane. */
    const uint32_t *BDFD = DFD+1;
    const uint32_t worstCaseBits = 8 * KHR_DFDVAL(BDFD, BYTESPLANE0);
    /* Treat each channel and flag separately. */
    uint32_t *virtualSampleBitLocations[256] = {};
    uint32_t numBitsPerVirtualSample[256] = {};
    uint32_t *lowerValues[256] = {};
    uint32_t *upperValues[256] = {};
    uint32_t lowerSign[256] = {};
    uint32_t upperSign[256] = {};
    uint32_t *samplesPerBit[4] = {};
    uint32_t sampleCounter;
    uint32_t previousChannel = 0x100; /* Impossible value */
    uint32_t channelOffset = 0;
    uint32_t swapMask = ((1U << wordByteSize) - 1U) << 3;
    uint32_t previousSampleStart = 0;
    uint32_t *outdfd;
    uint32_t *outbdb;
    /* Note: We assume that all samples of the same channel
       are the same virtual sample; to be fully general-purpose
       we should extend this to support distinguishing virtual
       samples by qualifier and position. */
    for (sampleCounter = 0; sampleCounter < KHR_DFDSAMPLECOUNT(BDFD); ++sampleCounter) {
        uint32_t thisSampleChannelId =
            KHR_DFDSVAL(BDFD, sampleCounter, CHANNELID) |
            KHR_DFDSVAL(BDFD, sampleCounter, QUALIFIERS);
        uint32_t bitCounter;
        uint32_t sampleBitBase = KHR_DFDSVAL(BDFD, sampleCounter, BITOFFSET);
        if (numBitsPerVirtualSample[thiSampleChannelId] == 0) {
            /* Need to allocate storage for this virtual sample */
        }
        switch (thisChannel) {
        case KHR_DF_CHANNEL_RGBSDA_RED:
            targetChannelArray = channelR;
            targetBitCount = &channelRBits;
            targetLower = lowerR;
            targetLowerSign = &lowerRSign;
            targetUpper = upperR;
            targetUpperSign = &upperRSign;
            /* Assume all qualifiers are the same. */
            /* N.B. This will break for, e.g., explicit exponent formats. */
            redQualifiers = KHR_DFDSVAL(BDFD, sampleCounter, QUALIFIERS);
            break;
        case KHR_DF_CHANNEL_RGBSDA_GREEN:
            targetChannelArray = channelG;
            targetBitCount = &channelGBits;
            targetLower = lowerG;
            targetLowerSign = &lowerGSign;
            targetUpper = upperG;
            targetUpperSign = &upperGSign;
            /* Assume all qualifiers are the same. */
            /* N.B. This will break for, e.g., explicit exponent formats. */
            greenQualifiers = KHR_DFDSVAL(BDFD, sampleCounter, QUALIFIERS);
            break;
        case KHR_DF_CHANNEL_RGBSDA_BLUE:
            targetChannelArray = channelB;
            targetBitCount = &channelBBits;
            targetLower = lowerB;
            targetLowerSign = &lowerBSign;
            targetUpper = upperB;
            targetUpperSign = &upperBSign;
            /* Assume all qualifiers are the same. */
            /* N.B. This will break for, e.g., explicit exponent formats. */
            blueQualifiers = KHR_DFDSVAL(BDFD, sampleCounter, QUALIFIERS);
            break;
        case KHR_DF_CHANNEL_RGBSDA_ALPHA:
            targetChannelArray = channelA;
            targetBitCount = &channelABits;
            targetLower = lowerA;
            targetLowerSign = &lowerASign;
            targetUpper = upperA;
            targetUpperSign = &upperASign;
            /* Assume all qualifiers are the same. */
            /* N.B. This will break for, e.g., explicit exponent formats. */
            alphaQualifiers = KHR_DFDSVAL(BDFD, sampleCounter, QUALIFIERS);
            break;
        default: return 0; /* Unknown/unexpected channel */
        }
        *targetBitCount += KHR_DFDSVAL(BDFD, sampleCounter, BITLENGTH) + 1;
        if (thisChannel != previousChannel) channelOffset = 0;
        for (bitCounter = 0; bitCounter < *targetBitCount; ++bitCounter, ++channelOffset) {
            /* Record the bit offset whence this came */
            targetChannelArray[channelOffset] = sampleBitBase + bitCounter;
            /* Also need to build up min and max values. */
            targetLower[channelOffset >> 5] |= (1U << (channelOffset & 0x1F)) * ((KHR_DFDSVAL(BDFD, sampleCounter, SAMPLELOWER) & (1 << bitCounter)) != 0U);
            targetUpper[channelOffset >> 5] |= (1U << (channelOffset & 0x1F)) * ((KHR_DFDSVAL(BDFD, sampleCounter, SAMPLEUPPER) & (1 << bitCounter)) != 0U);
        }
        /* The last sample of a channel ends with any sign bit. */
        /* Note: this does not properly handle exponents. */
        if (KHR_DFDSVAL(BDFD, sampleCounter, QUALIFIERS) & KHR_DF_DATATYPE_SIGNED) {
            *targetLowerSign = ((KHR_DFDSVAL(BDFD, sampleCounter, SAMPLELOWER) & (1 << *targetBitCount-1)) != 0U);
            *targetUpperSign = ((KHR_DFDSVAL(BDFD, sampleCounter, SAMPLEUPPER) & (1 << *targetBitCount-1)) != 0U);
        }
    }
    /* Now we have the target bit corresponding to each channel bit. */
    /* Do a big-endian swap on them and fill in bitChannels. */
    sampleCounter = 0;
    for (previousSampleStart = 0, channelOffset = 0; channelOffset < channelRBits; ++channelOffset) {
        channelR[channelOffset] ^= swapMask;
        bitChannels[channelR[channelOffset]] |= 1;
        /* We're going to need another sample. */
        if (((channelOffset - previousSampleStart) & 0x1F) == 0 || channelR[channelOffset] != channelR[channelOffset-1] + 1) {
            ++sampleCounter;
            previousSampleStart = channelOffset;
        }
    }
    for (channelOffset = 0; channelOffset < channelGBits; ++channelOffset) {
        channelG[channelOffset] ^= swapMask;
        bitChannels[channelG[channelOffset]] |= 2;
        /* We're going to need another sample. */
        if (((channelOffset - previousSampleStart) & 0x1F) == 0 || channelG[channelOffset] != channelG[channelOffset-1] + 1) {
            ++sampleCounter;
            previousSampleStart = channelOffset;
        }
    }
    for (channelOffset = 0; channelOffset < channelBBits; ++channelOffset) {
        channelB[channelOffset] ^= swapMask;
        bitChannels[channelB[channelOffset]] |= 4;
        /* We're going to need another sample. */
        if (((channelOffset - previousSampleStart) & 0x1F) == 0 || channelB[channelOffset] != channelB[channelOffset-1] + 1) {
            ++sampleCounter;
            previousSampleStart = channelOffset;
        }
    }
    for (channelOffset = 0; channelOffset < channelABits; ++channelOffset) {
        channelA[channelOffset] ^= swapMask;
        bitChannels[channelA[channelOffset]] |= 8;
        /* We're going to need another sample. */
        if (((channelOffset - previousSampleStart) & 0x1F) == 0 || channelA[channelOffset] != channelA[channelOffset-1] + 1) {
            ++sampleCounter;
            previousSampleStart = channelOffset;
        }
    }

    /* Now we can create the DFD and populate its header. */
    outdfd = (uint32_t *)malloc(sizeof(uint32_t) * (1 + KHR_DFDSIZEWORDS(sampleCounter)));
    outbdb = outdfd + 1;
    *dfd = sizeof(uint32_t) * (1 + KHR_DFDSIZEWORDS(sampleCounter));
    /* Note that this is a fairly inefficient way of doing things unless */
    /* the compiler is really good at removing redundant bit masking, */
    /* but in the interests of readability it probably doesn't matter. */
    KHR_DFDSETVAL(outbdb, VENDORID, KHR_DF_VENDORID_KHRONOS);
    KHR_DFDSETVAL(outbdb, DESCRIPTORTYPE, KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT);
    KHR_DFDSETVAL(outbdb, VERSIONNUMBER, KHR_DF_VERSIONNUMBER_1_3);
    KHR_DFDSETVAL(outbdb, DESCRIPTORBLOCKSIZE, (sizeof(uint32_t) * KHR_DFDSIZEWORDS(sampleCounter)));
    KHR_DFDSETVAL(outbdb, MODEL, KHR_DF_MODEL_RGBSDA); /* Or this function won't work. */
    KHR_DFDSETVAL(outbdb, PRIMARIES, KHR_DFDVAL(BDFD, PRIMARIES));
    KHR_DFDSETVAL(outbdb, TRANSFER, KHR_DFDVAL(BDFD, TRANSFER));
    KHR_DFDSETVAL(outbdb, FLAGS, KHR_DFDVAL(BDFD, FLAGS));
    outbdb[KHR_DF_WORD_TEXELBLOCKDIMENSION0] = KHR_DFDVAL(BDFD, TEXELBLOCKDIMENSION0);
    outbdb[KHR_DF_WORD_BYTESPLANE0] = KHR_DFDVAL(BDFD, BYTESPLANE0);
    outbdb[KHR_DF_WORD_BYTESPLANE4] = 0;
    /* Now iterate through bitChannels, outputting the bits of that channel in order. */
    {
        uint32_t bitCounter;
        sampleCounter = 0;
        for (bitCounter = 0; bitCounter < worstCase; ++bitCounter) {
            /* Note: This is a simplification assuming no overlapping channels. */
            /* Ideally we should determine which channel has the lowes unique bit */
            /* and output that first, rather than relying on channel id. */
            if ((bitChannels[bitCounter] & 1) && channelRBits) {
                /* Output channel R in order */
                uint32_t redBit = 0;
                uint32_t sampleBit = 0;
                while (redBit < channelRBits) {
                    if (sampleBit == 0) {
                        KHR_DFDSETSVAL(outbdb, sampleCounter, BITOFFSET, channelR[0]);
                        /* Come back to BITLENGTH */
                        KHR_DFDSETSVAL(outbdb, sampleCounter, CHANNELID, KHR_DF_CHANNEL_RGBSDA_RED);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, QUALIFIERS, redQualifiers);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, SAMPLEPOSITION_ALL, 0);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, SAMPLELOWER, 0);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, SAMPLEUPPER, 0);
                        inSample = 1;
                    }
                    outbdb[KHR_DF_WORD_SAMPLESTART +
                           sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                           KHR_DF_SAMPLEWORD_SAMPLELOWER] |=
                        ((lowerR[redBit >> 5] & (1U << (redBit & 0x1F))) != 0) << sampleBit;
                    outbdb[KHR_DF_WORD_SAMPLESTART +
                           sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                           KHR_DF_SAMPLEWORD_SAMPLEUPPER] |=
                        ((upperR[redBit >> 5] & (1U << (redBit & 0x1F))) != 0) << sampleBit;
                    sampleBit += 1;
                    /* End of sample? */
                    /* This over-simplifies large samples: */
                    /* If a sample is greater than 32 bits and we can describe the lower
                       and upper values in fewer samples using the rules for extension,
                       we should do so. */
                    if (redBit == channelRBits - 1 ||
                        channelR[redBit] != channelR[redBit+1] - 1 ||
                        sampleBit == 32) {
                        KHR_DFDSETSVAL(outbdb, sampleCounter, BITLENGTH, sampleBit - 1);
                        if (sampleBit < 32 && (redQualifiers & KHR_DF_DATATYPE_SIGNED)) {
                            /* Sign extend lower and upper */
                            uint32_t lowerSign = lowerR[channelRBits >> 5] & 1U << (channelRBits & 0x1F);
                                (KHR_DFDSVAL(outbdb, sampleCounter, SAMPLELOWER) & 0x80000000U) != 0;
                            uint32_t upperSign =
                                (KHR_DFDSVAL(outbdb, sampleCounter, SAMPLEUPPER) & 0x80000000U) != 0;
                            if (lowerSign) lowerSign = ~((1U << sampleBit) - 1);
                            if (upperSign) upperSign = ~((1U << sampleBit) - 1);
                            outbdb[KHR_DF_WORD_SAMPLESTART +
                                   sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                                   KHR_DF_SAMPLEWORD_SAMPLELOWER] |=
                                lowerSign;
                            outbdb[KHR_DF_WORD_SAMPLESTART +
                                   sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                                   KHR_DF_SAMPLEWORD_SAMPLEUPPER] |=
                                upperSign;
                        }
                        sampleBit = 0;
                        sampleCounter += 1;
                    }
                    redBit += 1;
                }
                /* Don't do this channel again */
                channelRBits = 0;
            }
            if ((bitChannels[bitCounter] & 2) && channelGBits) {
                /* Output channel G in order */
                uint32_t greenBit = 0;
                uint32_t sampleBit = 0;
                while (greenBit < channelGBits) {
                    if (sampleBit == 0) {
                        KHR_DFDSETSVAL(outbdb, sampleCounter, BITOFFSET, channelR[0]);
                        /* Come back to BITLENGTH */
                        KHR_DFDSETSVAL(outbdb, sampleCounter, CHANNELID, KHR_DF_CHANNEL_RGBSDA_RED);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, QUALIFIERS, greenQualifiers);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, SAMPLEPOSITION_ALL, 0);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, SAMPLELOWER, 0);
                        KHR_DFDSETSVAL(outbdb, sampleCounter, SAMPLEUPPER, 0);
                        inSample = 1;
                    }
                    outbdb[KHR_DF_WORD_SAMPLESTART +
                           sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                           KHR_DF_SAMPLEWORD_SAMPLELOWER] |=
                        ((lowerG[redBit >> 5] & (1U << (redBit & 0x1F))) != 0) << sampleBit;
                    outbdb[KHR_DF_WORD_SAMPLESTART +
                           sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                           KHR_DF_SAMPLEWORD_SAMPLEUPPER] |=
                        ((upperG[redBit >> 5] & (1U << (redBit & 0x1F))) != 0) << sampleBit;
                    sampleBit += 1;
                    /* End of sample? */
                    /* This over-simplifies large samples: */
                    /* If a sample is greater than 32 bits and we can describe the lower
                       and upper values in fewer samples using the rules for extension,
                       we should do so. */
                    if (greenBit == channelGBits - 1 ||
                        channelG[greenBit] != channelG[greenBit+1] - 1 ||
                        sampleBit == 32) {
                        KHR_DFDSETSVAL(outbdb, sampleCounter, BITLENGTH, sampleBit - 1);
                        if (sampleBit < 32 && (greenQualifiers & KHR_DF_DATATYPE_SIGNED)) {
                            /* Sign extend lower and upper */
                            uint32_t lowerSign =
                                (KHR_DFDSVAL(outbdb, sampleCounter, SAMPLELOWER) & 0x80000000U) != 0;
                            uint32_t upperSign =
                                (KHR_DFDSVAL(outbdb, sampleCounter, SAMPLEUPPER) & 0x80000000U) != 0;
                            if (lowerSign) lowerSign = ~((1U << sampleBit) - 1);
                            if (upperSign) upperSign = ~((1U << sampleBit) - 1);
                            outbdb[KHR_DF_WORD_SAMPLESTART +
                                   sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                                   KHR_DF_SAMPLEWORD_SAMPLELOWER] |=
                                lowerSign;
                            outbdb[KHR_DF_WORD_SAMPLESTART +
                                   sampleCounter * KHR_DF_WORD_SAMPLEWORDS +
                                   KHR_DF_SAMPLEWORD_SAMPLEUPPER] |=
                                upperSign;
                        }
                        sampleBit = 0;
                        sampleCounter += 1;
                    }
                    redBit += 1;
                }
                /* Don't do this channel again */
                channelGBits = 0;
            }
            if ((bitChannels[bitCounter] & 4) && channelBBits) {
                /* Output channel B in order */
                channelBBits = 0;
            }
            if ((bitChannels[bitCounter] & 8) && channelABits) {
                /* Output channel A in order */
                channelABits = 0;
            }
        }
    }
    /* Clean up allocations */
    free(channelR);
    free(channelG);
    free(channelB);
    free(channelA);
    free(lowerR);
    free(upperR);
    free(lowerG);
    free(upperG);
    free(lowerB);
    free(upperB);
    free(lowerA);
    free(upperA);
    free(bitChannels);
    return dfd;
}
