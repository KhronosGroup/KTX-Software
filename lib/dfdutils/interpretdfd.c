#include <stdint.h>
#include <stdio.h>
#include <KHR/khr_df.h>
#include "dfd.h"

enum ReturnType {
    LITTLE_ENDIAN_FORMAT_BIT = 0, /* N.B. Default for 8bpc */
    BIG_ENDIAN_FORMAT_BIT = 1,
    PACKED_FORMAT_BIT = 2,
    SRGB_FORMAT_BIT = 4,
    NORMALIZED_FORMAT_BIT = 8,
    SIGNED_FORMAT_BIT = 16,
    FLOAT_FORMAT_BIT = 32,
    UNSUPPORTED_ERROR_BIT = 64,
    /* "NONTRIVIAL_ENDIANNESS" means not big-endian, not little-endian */
    /* (a channel has bits that are not consecutive in either order). */
    UNSUPPORTED_NONTRIVIAL_ENDIANNESS = UNSUPPORTED_ERROR_BIT,
    /* "MULTIPLE_SAMPLE_LOCATIONS" is an error because only single-sample */
    /* texel blocks (with coordinates 0,0,0,0 for all samples) are supported. */
    UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS = UNSUPPORTED_ERROR_BIT + 1,
    /* "MULTIPLE_PLANES" is an error because only contiguous data is supported. */
    UNSUPPORTED_MULTIPLE_PLANES = UNSUPPORTED_ERROR_BIT + 2,
    /* Only channels R, G, B and A are supported. */
    UNSUPPORTED_CHANNEL_TYPES = UNSUPPORTED_ERROR_BIT + 3,
    /* Only channels with the same flags are supported */
    /* (e.g. we don't support float red with integer green) */
    UNSUPPORTED_MIXED_CHANNELS = UNSUPPORTED_ERROR_BIT + 4
};

typedef struct {
    unsigned int offset; /* Bits for packed, bytes for unpacked */
    unsigned int size; /* Bits for packed, bytes for unpacked */
} Channel;

/* We treat the DFD as 32-bit words in native endianness. */
/* This means a DFD stored in a file should be swizzled to native */
/* endianness before use with this function. */
/* The DFD is a data format descriptor, not just the descriptor block. */
enum ReturnType ProcessDFD(const uint32_t *DFD,
                           Channel *R, Channel *G, Channel *B, Channel *A,
                           uint32_t *wordBytes)
{
    /* We specifically handle "simple" cases that can be translated */
    /* to things a GPU can access. For simplicity, we also ignore */
    /* the compressed formats, which are generally a single sample */
    /* (and I believe are all defined to be little-endian in their */
    /* in-memory layout, even if some documentation confuses this). */
    /* We also just worry about layout and ignore sRGB, since that's */
    /* trivial to extract anyway. */

    /* DFD points to the whole descriptor, not the basic descriptor block. */
    /* Make everything else relative to the basic descriptor block. */
    const uint32_t *BDFDB = DFD+1;

    /* BDFDB size in bytes in BDFDB[1] >> 16, 24 byte header, 16 bytes/sample. */
    /* We assume the compiler can treat "/16" sensibly. */
    uint32_t numSamples = (KHR_DFDVAL(BDFDB, DESCRIPTORBLOCKSIZE) - 24U)
        / (4 * KHR_DF_WORD_SAMPLEWORDS);

    uint32_t sampleCounter;
    int determinedEndianness = 0;
    int determinedNormalizedness = 0;
    int determinedSignedness = 0;
    int determinedFloatness = 0;
    enum ReturnType result = 0; /* Build this up incrementally. */

    /* First rule out the multiple planes case (trivially) */
    /* - that is, we check that only bytesPlane0 is non-zero. */
    /* This means we don't handle YUV even if the API could. */
    /* (We rely on KHR_DF_WORD_BYTESPLANE0..3 being the same and */
    /* KHR_DF_WORD_BYTESPLANE4..7 being the same as a short cut.) */
    if ((BDFDB[KHR_DF_WORD_BYTESPLANE0] & ~KHR_DF_MASK_BYTESPLANE0)
        || BDFDB[KHR_DF_WORD_BYTESPLANE4]) return UNSUPPORTED_MULTIPLE_PLANES;

    /* Only support the RGB color model. */
    /* We could expand this to allow "UNSPECIFIED" as well. */
    if (KHR_DFDVAL(BDFDB, MODEL) != KHR_DF_MODEL_RGBSDA) return UNSUPPORTED_CHANNEL_TYPES;

    /* We only pay attention to sRGB. */
    if (KHR_DFDVAL(BDFDB, TRANSFER) == KHR_DF_TRANSFER_SRGB) result |= SRGB_FORMAT_BIT;

    /* We only support samples at coordinate 0,0,0,0. */
    /* (We could confirm this from texel_block_dimensions in 1.2, but */
    /* the interpretation might change in later versions.) */
    for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
        if (KHR_DFDSVAL(BDFDB, sampleCounter, SAMPLEPOSITION_ALL))
            return UNSUPPORTED_MULTIPLE_SAMPLE_LOCATIONS;
    }

    /* Set flags and check for consistency. */
    for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
        /* Note: We're ignoring 9995, which is weird and worth special-casing */
        /* rather than trying to generalise to all float formats. */
        if (!determinedFloatness) {
            if (KHR_DFDSVAL(BDFDB, sampleCounter, QUALIFIERS)
                 & KHR_DF_SAMPLE_DATATYPE_FLOAT) {
                result |= FLOAT_FORMAT_BIT;
                determinedFloatness = 1;
            }
        } else {
            /* Check whether we disagree with our predetermined floatness. */
            /* Note that this could justifiably happen with (say) D24S8. */
            if (KHR_DFDSVAL(BDFDB, sampleCounter, QUALIFIERS)
                 & KHR_DF_SAMPLE_DATATYPE_FLOAT) {
                if (!(result & FLOAT_FORMAT_BIT)) return UNSUPPORTED_MIXED_CHANNELS;
            } else {
                if ((result & FLOAT_FORMAT_BIT)) return UNSUPPORTED_MIXED_CHANNELS;
            }
        }
        if (!determinedSignedness) {
            if (KHR_DFDSVAL(BDFDB, sampleCounter, QUALIFIERS)
                 & KHR_DF_SAMPLE_DATATYPE_SIGNED) {
                result |= SIGNED_FORMAT_BIT;
                determinedSignedness = 1;
            }
        } else {
            /* Check whether we disagree with our predetermined signedness. */
            if (KHR_DFDSVAL(BDFDB, sampleCounter, QUALIFIERS)
                 & KHR_DF_SAMPLE_DATATYPE_SIGNED) {
                if (!(result & SIGNED_FORMAT_BIT)) return UNSUPPORTED_MIXED_CHANNELS;
            } else {
                if ((result & SIGNED_FORMAT_BIT)) return UNSUPPORTED_MIXED_CHANNELS;
            }
        }
        /* We define "unnormalized" as "sample_upper = 1". */
        /* We don't check whether any non-1 normalization value is correct */
        /* (i.e. set to the maximum bit value, and check min value) on */
        /* the assumption that we're looking at a format which *came* from */
        /* an API we can support. */
        if (!determinedNormalizedness) {
            /* The ambiguity here is if the bottom bit is a single-bit value, */
            /* as in RGBA 5:5:5:1, so we defer the decision if the channel only has one bit. */
            if (KHR_DFDSVAL(BDFDB, sampleCounter, BITLENGTH) > 0) {
                if ((result & FLOAT_FORMAT_BIT)) {
                    if (*(float *)(void *)&BDFDB[KHR_DF_WORD_SAMPLESTART +
                                                 KHR_DF_WORD_SAMPLEWORDS * sampleCounter +
                                                 KHR_DF_SAMPLEWORD_SAMPLEUPPER] != 1.0f) {
                        result |= NORMALIZED_FORMAT_BIT;
                    }
                } else {
                    if (KHR_DFDSVAL(BDFDB, sampleCounter, SAMPLEUPPER) != 1U) {
                        result |= NORMALIZED_FORMAT_BIT;
                    }
                }
                determinedNormalizedness = 1;
            }
        }
        /* Note: We don't check for inconsistent normalization, because */
        /* channels composed of multiple samples will have 0 in the */
        /* lower/upper range. */
        /* This heuristic should handle 64-bit integers, too. */
    }

    /* If this is a packed format, we work out our offsets differently. */
    /* We assume a packed format has channels that aren't byte-aligned. */
    /* If we have a format in which every channel is byte-aligned *and* packed, */
    /* we have the RGBA/ABGR ambiguity; we *probably* don't want the packed */
    /* version in this case, and if hardware has to pack it and swizzle, */
    /* that's up to the hardware to special-case. */
    for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
        if (KHR_DFDSVAL(BDFDB, sampleCounter, BITOFFSET) & 0x7U) {
            result |= PACKED_FORMAT_BIT;
            /* Once we're packed, we're packed, no need to keep checking. */
            break;
        }
    }

    /* Remember: the canonical ordering of samples is to start with */
    /* the lowest bit of the channel/location which touches bit 0 of */
    /* the data, when the latter is concatenated in little-endian order, */
    /* and then progress until all the bits of that channel/location */
    /* have been processed. Multiple channels sharing the same source */
    /* bits are processed in channel ID order. (I should clarify this */
    /* for partially-shared data, but it doesn't really matter so long */
    /* as everything is consecutive, except to make things canonical.) */
    /* Note: For standard formats we could determine big/little-endianness */
    /* simply from whether the first sample starts in bit 0; technically */
    /* it's possible to have a format with unaligned channels wherein the */
    /* first channel starts at bit 0 and is one byte, yet other channels */
    /* take more bytes or aren't aligned (e.g. D24S8), but this should be */
    /* irrelevant for the formats that we support. */
    if ((result & PACKED_FORMAT_BIT)) {
        /* A packed format. */
        uint32_t currentChannel = ~0U; /* Don't start matched. */
        uint32_t currentBitOffset = 0;
        uint32_t currentByteOffset = 0;
        uint32_t currentBitLength = 0;
        *wordBytes = (BDFDB[4] & 0xFFU);
        for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
            uint32_t sampleBitOffset = KHR_DFDSVAL(BDFDB, sampleCounter, BITOFFSET);
            uint32_t sampleByteOffset = sampleBitOffset >> 3U;
            /* The sample bitLength field stores the bit length - 1. */
            uint32_t sampleBitLength = KHR_DFDSVAL(BDFDB, sampleCounter, BITLENGTH) + 1;
            uint32_t sampleChannel = KHR_DFDSVAL(BDFDB, sampleCounter, CHANNELID);
            Channel *sampleChannelPtr;
            switch (sampleChannel) {
            case KHR_DF_CHANNEL_RGBSDA_RED:
                sampleChannelPtr = R;
                break;
            case KHR_DF_CHANNEL_RGBSDA_GREEN:
                sampleChannelPtr = G;
                break;
            case KHR_DF_CHANNEL_RGBSDA_BLUE:
                sampleChannelPtr = B;
                break;
            case KHR_DF_CHANNEL_RGBSDA_ALPHA:
                sampleChannelPtr = A;
                break;
            default:
                return UNSUPPORTED_CHANNEL_TYPES;
            }
            if (sampleChannel == currentChannel) {
                /* Continuation of the same channel. */
                /* Since a big (>32-bit) channel isn't "packed", */
                /* this should only happen in big-endian, or if */
                /* we have a wacky format that we won't support. */
                if (sampleByteOffset == currentByteOffset - 1U && /* One byte earlier */
                    ((currentBitOffset + currentBitLength) & 7U) == 0 && /* Already at the end of a byte */
                    (sampleBitOffset & 7U) == 0) { /* Start at the beginning of the byte */
                    /* All is good, continue big-endian. */
                    /* N.B. We shouldn't be here if we decided we were little-endian, */
                    /* so we don't bother to check that disagreement. */
                    result |= BIG_ENDIAN_FORMAT_BIT;
                    determinedEndianness = 1;
                } else {
                    /* Oh dear. */
                    /* We could be little-endian, but not with any standard format. */
                    /* More likely we've got something weird that we can't support. */
                    return UNSUPPORTED_NONTRIVIAL_ENDIANNESS;
                }
                /* Remember where we are. */
                currentBitOffset = sampleBitOffset;
                currentByteOffset = sampleByteOffset;
                currentBitLength = sampleBitLength;
                /* Accumulate the bit length. */
                sampleChannelPtr->size += sampleBitLength;
            } else {
                /* Everything is new. Hopefully. */
                currentChannel = sampleChannel;
                currentBitOffset = sampleBitOffset;
                currentByteOffset = sampleByteOffset;
                currentBitLength = sampleBitLength;
                if (sampleChannelPtr->size) {
                    /* Uh-oh, we've seen this channel before. */
                    return UNSUPPORTED_NONTRIVIAL_ENDIANNESS;
                }
                /* For now, record the bit offset in little-endian terms, */
                /* because we may not know to reverse it yet. */
                sampleChannelPtr->offset = sampleBitOffset;
                sampleChannelPtr->size = sampleBitLength;
            }
        }
        if ((result & BIG_ENDIAN_FORMAT_BIT)) {
            /* Our bit offsets to bit 0 of each channel are in little-endian terms. */
            /* We need to do a byte swap to work out where they should be. */
            /* We assume, for sanity, that byte sizes are a power of two for this. */
            uint32_t offsetMask = (*wordBytes - 1U) << 3U;
            R->offset ^= offsetMask;
            G->offset ^= offsetMask;
            B->offset ^= offsetMask;
            A->offset ^= offsetMask;
        }
    } else {
        /* Not a packed format. */
        /* Everything is byte-aligned. */
        /* Question is whether there multiple samples per channel. */
        uint32_t currentChannel = ~0U; /* Don't start matched. */
        uint32_t currentByteOffset = 0;
        uint32_t currentByteLength = 0;
        for (sampleCounter = 0; sampleCounter < numSamples; ++sampleCounter) {
            uint32_t sampleByteOffset = KHR_DFDSVAL(BDFDB, sampleCounter, BITOFFSET) >> 3U;
            uint32_t sampleByteLength = (KHR_DFDSVAL(BDFDB, sampleCounter, BITLENGTH) + 1) >> 3U;
            uint32_t sampleChannel = KHR_DFDSVAL(BDFDB, sampleCounter, CHANNELID);
            Channel *sampleChannelPtr;
            switch (sampleChannel) {
            case KHR_DF_CHANNEL_RGBSDA_RED:
                sampleChannelPtr = R;
                break;
            case KHR_DF_CHANNEL_RGBSDA_GREEN:
                sampleChannelPtr = G;
                break;
            case KHR_DF_CHANNEL_RGBSDA_BLUE:
                sampleChannelPtr = B;
                break;
            case KHR_DF_CHANNEL_RGBSDA_ALPHA:
                sampleChannelPtr = A;
                break;
            default:
                return UNSUPPORTED_CHANNEL_TYPES;
            }
            if (sampleChannel == currentChannel) {
                /* Continuation of the same channel. */
                /* Either big-endian, or little-endian with a very large channel. */
                if (sampleByteOffset == currentByteOffset - 1) { /* One byte earlier */
                    if (determinedEndianness && !(result & BIG_ENDIAN_FORMAT_BIT)) {
                        return UNSUPPORTED_NONTRIVIAL_ENDIANNESS;
                    }
                    /* All is good, continue big-endian. */
                    result |= BIG_ENDIAN_FORMAT_BIT;
                    determinedEndianness = 1;
                    /* Update the start */
                    sampleChannelPtr->offset = sampleByteOffset;
                } else if (sampleByteOffset == currentByteOffset + currentByteLength) {
                    if (determinedEndianness && (result & BIG_ENDIAN_FORMAT_BIT)) {
                        return UNSUPPORTED_NONTRIVIAL_ENDIANNESS;
                    }
                    /* All is good, continue little-endian. */
                    determinedEndianness = 1;
                } else {
                    /* Oh dear. */
                    /* We could be little-endian, but not with any standard format. */
                    /* More likely we've got something weird that we can't support. */
                    return UNSUPPORTED_NONTRIVIAL_ENDIANNESS;
                }
                /* Remember where we are. */
                currentByteOffset = sampleByteOffset;
                currentByteLength = sampleByteLength;
                /* Accumulate the byte length. */
                sampleChannelPtr->size += sampleByteLength;
                /* Assume these are all the same. */
                *wordBytes = sampleChannelPtr->size;
            } else {
                /* Everything is new. Hopefully. */
                currentChannel = sampleChannel;
                currentByteOffset = sampleByteOffset;
                currentByteLength = sampleByteLength;
                if (sampleChannelPtr->size) {
                    /* Uh-oh, we've seen this channel before. */
                    return UNSUPPORTED_NONTRIVIAL_ENDIANNESS;
                }
                /* For now, record the byte offset in little-endian terms, */
                /* because we may not know to reverse it yet. */
                sampleChannelPtr->offset = sampleByteOffset;
                sampleChannelPtr->size = sampleByteLength;
                /* Assume these are all the same. */
                *wordBytes = sampleByteLength;
            }
        }
    }
    return result;
}

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

    Channel R,G,B,A;
    uint32_t wordSize;

/*    enum ReturnType t = ProcessDFD(dfd1, &R, &G, &B, &A, &wordSize); */
/*    uint32_t *d = createDFDUnpacked(1,2,4,0,s_UNORM); */
/*    int channels[] = {0,1,2,3} */
/*    int bits[] = {10,10,10,2}; */
/*    uint32_t *d = createDFDPacked(1,4,bits,channels,s_UNORM); */
    int channels[] = {0,1,2};
    int bits[] = {5,6,5};
    uint32_t *d = createDFDPacked(1,3,bits,channels,s_UNORM);
/*    printDFD(d); */
    enum ReturnType t;

    R.offset = G.offset = B.offset = A.offset = 0;
    R.size = G.size = B.size = A.size = 0;
    
    t = ProcessDFD(d, &R, &G, &B, &A, &wordSize);

    if (t & UNSUPPORTED_ERROR_BIT) {
        printf("%s\n", errorText[t - UNSUPPORTED_ERROR_BIT]);
        return 0;
    } else {
        if (t & BIG_ENDIAN_FORMAT_BIT) {
            printf("Big-endian\n");
        } else {
            printf("Little-endian\n");
        }
        if (t & PACKED_FORMAT_BIT) {
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
        if (t & SRGB_FORMAT_BIT) {
            printf("sRGB\n");
        }
        if (t & NORMALIZED_FORMAT_BIT) {
            printf("Normalized\n");
        }
        if (t & SIGNED_FORMAT_BIT) {
            printf("Signed\n");
        }
        if (t & FLOAT_FORMAT_BIT) {
            printf("Float\n");
        }
    }
    return 0;
}
