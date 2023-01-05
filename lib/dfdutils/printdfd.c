/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/* Copyright 2019-2020 The Khronos Group Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @~English
 * @brief Utilities for printing data format descriptors.
 */

/*
 * Author: Andrew Garrard
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <KHR/khr_df.h>
#include "dfd.h"


const char* dfdToStringVendorID(khr_df_vendorid_e value) {
    switch (value) {
    case KHR_DF_VENDORID_KHRONOS:
        return "KHR_DF_VENDORID_KHRONOS";

    case KHR_DF_VENDORID_MAX:
        // These enum values are not meant for string representation. Ignore
        break;
    }
    return NULL;
}

const char* dfdToStringDescriptorType(khr_df_khr_descriptortype_e value) {
    switch (value) {
    case KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT:
        return "KHR_DF_KHR_DESCRIPTORTYPE_BASICFORMAT";
    case KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_PLANES:
        return "KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_PLANES";
    case KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_DIMENSIONS:
        return "KHR_DF_KHR_DESCRIPTORTYPE_ADDITIONAL_DIMENSIONS";

    case KHR_DF_KHR_DESCRIPTORTYPE_NEEDED_FOR_WRITE_BIT:
    case KHR_DF_KHR_DESCRIPTORTYPE_NEEDED_FOR_DECODE_BIT:
    case KHR_DF_KHR_DESCRIPTORTYPE_MAX:
        // These enum values are not meant for string representation. Ignore
        break;
    }
    return NULL;
}

const char* dfdToStringVersionNumber(khr_df_versionnumber_e value) {
    switch (value) {
    case KHR_DF_VERSIONNUMBER_1_1:
    // case KHR_DF_VERSIONNUMBER_1_0: // Fallthrough, Matching values
        return "KHR_DF_VERSIONNUMBER_1_1";
    case KHR_DF_VERSIONNUMBER_1_2:
        return "KHR_DF_VERSIONNUMBER_1_2";
    case KHR_DF_VERSIONNUMBER_1_3:
        return "KHR_DF_VERSIONNUMBER_1_3";

    // case KHR_DF_VERSIONNUMBER_LATEST: // Fallthrough, Matching values
    case KHR_DF_VERSIONNUMBER_MAX:
        // These enum values are not meant for string representation. Ignore
        break;
    }
    return NULL;
}

const char* dfdToStringFlagsBit(uint32_t bit_index, bool bit_value) {
    switch (bit_index) {
    case 0:
        return bit_value ? "KHR_DF_FLAG_ALPHA_PREMULTIPLIED" : "KHR_DF_FLAG_ALPHA_STRAIGHT";
    default:
        return NULL;
    }
}

const char* dfdToStringTransferFunction(khr_df_transfer_e value) {
    switch (value) {
    case KHR_DF_TRANSFER_UNSPECIFIED:
        return "KHR_DF_TRANSFER_UNSPECIFIED";
    case KHR_DF_TRANSFER_LINEAR:
        return "KHR_DF_TRANSFER_LINEAR";
    case KHR_DF_TRANSFER_SRGB:
        return "KHR_DF_TRANSFER_SRGB";
    case KHR_DF_TRANSFER_ITU:
        return "KHR_DF_TRANSFER_ITU";
    case KHR_DF_TRANSFER_NTSC:
    // case KHR_DF_TRANSFER_SMTPE170M: // Fallthrough, Matching values
        return "KHR_DF_TRANSFER_NTSC";
    case KHR_DF_TRANSFER_SLOG:
        return "KHR_DF_TRANSFER_SLOG";
    case KHR_DF_TRANSFER_SLOG2:
        return "KHR_DF_TRANSFER_SLOG2";
    case KHR_DF_TRANSFER_BT1886:
        return "KHR_DF_TRANSFER_BT1886";
    case KHR_DF_TRANSFER_HLG_OETF:
        return "KHR_DF_TRANSFER_HLG_OETF";
    case KHR_DF_TRANSFER_HLG_EOTF:
        return "KHR_DF_TRANSFER_HLG_EOTF";
    case KHR_DF_TRANSFER_PQ_EOTF:
        return "KHR_DF_TRANSFER_PQ_EOTF";
    case KHR_DF_TRANSFER_PQ_OETF:
        return "KHR_DF_TRANSFER_PQ_OETF";
    case KHR_DF_TRANSFER_DCIP3:
        return "KHR_DF_TRANSFER_DCIP3";
    case KHR_DF_TRANSFER_PAL_OETF:
        return "KHR_DF_TRANSFER_PAL_OETF";
    case KHR_DF_TRANSFER_PAL625_EOTF:
        return "KHR_DF_TRANSFER_PAL625_EOTF";
    case KHR_DF_TRANSFER_ST240:
        return "KHR_DF_TRANSFER_ST240";
    case KHR_DF_TRANSFER_ACESCC:
        return "KHR_DF_TRANSFER_ACESCC";
    case KHR_DF_TRANSFER_ACESCCT:
        return "KHR_DF_TRANSFER_ACESCCT";
    case KHR_DF_TRANSFER_ADOBERGB:
        return "KHR_DF_TRANSFER_ADOBERGB";

    case KHR_DF_TRANSFER_MAX:
        // These enum values are not meant for string representation. Ignore
        break;
    }
    return NULL;
}

const char* dfdToStringColorPrimaries(khr_df_primaries_e value) {
    switch (value) {
    case KHR_DF_PRIMARIES_UNSPECIFIED:
        return "KHR_DF_PRIMARIES_UNSPECIFIED";
    case KHR_DF_PRIMARIES_BT709:
    // case KHR_DF_PRIMARIES_SRGB: // Fallthrough, Matching values
        return "KHR_DF_PRIMARIES_BT709";
    case KHR_DF_PRIMARIES_BT601_EBU:
        return "KHR_DF_PRIMARIES_BT601_EBU";
    case KHR_DF_PRIMARIES_BT601_SMPTE:
        return "KHR_DF_PRIMARIES_BT601_SMPTE";
    case KHR_DF_PRIMARIES_BT2020:
        return "KHR_DF_PRIMARIES_BT2020";
    case KHR_DF_PRIMARIES_CIEXYZ:
        return "KHR_DF_PRIMARIES_CIEXYZ";
    case KHR_DF_PRIMARIES_ACES:
        return "KHR_DF_PRIMARIES_ACES";
    case KHR_DF_PRIMARIES_ACESCC:
        return "KHR_DF_PRIMARIES_ACESCC";
    case KHR_DF_PRIMARIES_NTSC1953:
        return "KHR_DF_PRIMARIES_NTSC1953";
    case KHR_DF_PRIMARIES_PAL525:
        return "KHR_DF_PRIMARIES_PAL525";
    case KHR_DF_PRIMARIES_DISPLAYP3:
        return "KHR_DF_PRIMARIES_DISPLAYP3";
    case KHR_DF_PRIMARIES_ADOBERGB:
        return "KHR_DF_PRIMARIES_ADOBERGB";

    case KHR_DF_PRIMARIES_MAX:
        // These enum values are not meant for string representation. Ignore
        break;
    }
    return NULL;
}

const char* dfdToStringColorModel(khr_df_model_e value) {
    switch (value) {
    case KHR_DF_MODEL_UNSPECIFIED:
        return "KHR_DF_MODEL_UNSPECIFIED";
    case KHR_DF_MODEL_RGBSDA:
        return "KHR_DF_MODEL_RGBSDA";
    case KHR_DF_MODEL_YUVSDA:
        return "KHR_DF_MODEL_YUVSDA";
    case KHR_DF_MODEL_YIQSDA:
        return "KHR_DF_MODEL_YIQSDA";
    case KHR_DF_MODEL_LABSDA:
        return "KHR_DF_MODEL_LABSDA";
    case KHR_DF_MODEL_CMYKA:
        return "KHR_DF_MODEL_CMYKA";
    case KHR_DF_MODEL_XYZW:
        return "KHR_DF_MODEL_XYZW";
    case KHR_DF_MODEL_HSVA_ANG:
        return "KHR_DF_MODEL_HSVA_ANG";
    case KHR_DF_MODEL_HSLA_ANG:
        return "KHR_DF_MODEL_HSLA_ANG";
    case KHR_DF_MODEL_HSVA_HEX:
        return "KHR_DF_MODEL_HSVA_HEX";
    case KHR_DF_MODEL_HSLA_HEX:
        return "KHR_DF_MODEL_HSLA_HEX";
    case KHR_DF_MODEL_YCGCOA:
        return "KHR_DF_MODEL_YCGCOA";
    case KHR_DF_MODEL_YCCBCCRC:
        return "KHR_DF_MODEL_YCCBCCRC";
    case KHR_DF_MODEL_ICTCP:
        return "KHR_DF_MODEL_ICTCP";
    case KHR_DF_MODEL_CIEXYZ:
        return "KHR_DF_MODEL_CIEXYZ";
    case KHR_DF_MODEL_CIEXYY:
        return "KHR_DF_MODEL_CIEXYY";
    case KHR_DF_MODEL_BC1A:
    // case KHR_DF_MODEL_DXT1A: // Fallthrough, Matching values
        return "KHR_DF_MODEL_BC1A";
    case KHR_DF_MODEL_BC2:
    // case KHR_DF_MODEL_DXT2: // Fallthrough, Matching values
    // case KHR_DF_MODEL_DXT3: // Fallthrough, Matching values
        return "KHR_DF_MODEL_BC2";
    case KHR_DF_MODEL_BC3:
    // case KHR_DF_MODEL_DXT4: // Fallthrough, Matching values
    // case KHR_DF_MODEL_DXT5: // Fallthrough, Matching values
        return "KHR_DF_MODEL_BC3";
    case KHR_DF_MODEL_BC4:
        return "KHR_DF_MODEL_BC4";
    case KHR_DF_MODEL_BC5:
        return "KHR_DF_MODEL_BC5";
    case KHR_DF_MODEL_BC6H:
        return "KHR_DF_MODEL_BC6H";
    case KHR_DF_MODEL_BC7:
        return "KHR_DF_MODEL_BC7";
    case KHR_DF_MODEL_ETC1:
        return "KHR_DF_MODEL_ETC1";
    case KHR_DF_MODEL_ETC2:
        return "KHR_DF_MODEL_ETC2";
    case KHR_DF_MODEL_ASTC:
        return "KHR_DF_MODEL_ASTC";
    case KHR_DF_MODEL_ETC1S:
        return "KHR_DF_MODEL_ETC1S";
    case KHR_DF_MODEL_PVRTC:
        return "KHR_DF_MODEL_PVRTC";
    case KHR_DF_MODEL_PVRTC2:
        return "KHR_DF_MODEL_PVRTC2";
    case KHR_DF_MODEL_UASTC:
        return "KHR_DF_MODEL_UASTC";

    case KHR_DF_MODEL_MAX:
        // These enum values are not meant for string representation. Ignore
        break;
    }
    return NULL;
}

const char* dfdToStringSampleDatatypeQualifiers(uint32_t bit_index, bool bit_value) {
    if (!bit_value)
        return NULL;

    switch (1u << bit_index) {
    case KHR_DF_SAMPLE_DATATYPE_LINEAR:
        return "KHR_DF_SAMPLE_DATATYPE_LINEAR";
    case KHR_DF_SAMPLE_DATATYPE_EXPONENT:
        return "KHR_DF_SAMPLE_DATATYPE_EXPONENT";
    case KHR_DF_SAMPLE_DATATYPE_SIGNED:
        return "KHR_DF_SAMPLE_DATATYPE_SIGNED";
    case KHR_DF_SAMPLE_DATATYPE_FLOAT:
        return "KHR_DF_SAMPLE_DATATYPE_FLOAT";
    }
    return NULL;
}

const char* dfdToStringChannelId(khr_df_model_e model, khr_df_model_channels_e value) {
    switch (model) {
    case KHR_DF_MODEL_RGBSDA:
        switch (value) {
        case KHR_DF_CHANNEL_RGBSDA_RED:
            return "KHR_DF_CHANNEL_RGBSDA_RED";
        case KHR_DF_CHANNEL_RGBSDA_GREEN:
            return "KHR_DF_CHANNEL_RGBSDA_GREEN";
        case KHR_DF_CHANNEL_RGBSDA_BLUE:
            return "KHR_DF_CHANNEL_RGBSDA_BLUE";
        case KHR_DF_CHANNEL_RGBSDA_STENCIL:
            return "KHR_DF_CHANNEL_RGBSDA_STENCIL";
        case KHR_DF_CHANNEL_RGBSDA_DEPTH:
            return "KHR_DF_CHANNEL_RGBSDA_DEPTH";
        case KHR_DF_CHANNEL_RGBSDA_ALPHA:
            return "KHR_DF_CHANNEL_RGBSDA_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_YUVSDA:
        switch (value) {
        case KHR_DF_CHANNEL_YUVSDA_Y:
            return "KHR_DF_CHANNEL_YUVSDA_Y";
        case KHR_DF_CHANNEL_YUVSDA_U:
            return "KHR_DF_CHANNEL_YUVSDA_U";
        case KHR_DF_CHANNEL_YUVSDA_V:
            return "KHR_DF_CHANNEL_YUVSDA_V";
        case KHR_DF_CHANNEL_YUVSDA_STENCIL:
            return "KHR_DF_CHANNEL_YUVSDA_STENCIL";
        case KHR_DF_CHANNEL_YUVSDA_DEPTH:
            return "KHR_DF_CHANNEL_YUVSDA_DEPTH";
        case KHR_DF_CHANNEL_YUVSDA_ALPHA:
            return "KHR_DF_CHANNEL_YUVSDA_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_YIQSDA:
        switch (value) {
        case KHR_DF_CHANNEL_YIQSDA_Y:
            return "KHR_DF_CHANNEL_YIQSDA_Y";
        case KHR_DF_CHANNEL_YIQSDA_I:
            return "KHR_DF_CHANNEL_YIQSDA_I";
        case KHR_DF_CHANNEL_YIQSDA_Q:
            return "KHR_DF_CHANNEL_YIQSDA_Q";
        case KHR_DF_CHANNEL_YIQSDA_STENCIL:
            return "KHR_DF_CHANNEL_YIQSDA_STENCIL";
        case KHR_DF_CHANNEL_YIQSDA_DEPTH:
            return "KHR_DF_CHANNEL_YIQSDA_DEPTH";
        case KHR_DF_CHANNEL_YIQSDA_ALPHA:
            return "KHR_DF_CHANNEL_YIQSDA_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_LABSDA:
        switch (value) {
        case KHR_DF_CHANNEL_LABSDA_L:
            return "KHR_DF_CHANNEL_LABSDA_L";
        case KHR_DF_CHANNEL_LABSDA_A:
            return "KHR_DF_CHANNEL_LABSDA_A";
        case KHR_DF_CHANNEL_LABSDA_B:
            return "KHR_DF_CHANNEL_LABSDA_B";
        case KHR_DF_CHANNEL_LABSDA_STENCIL:
            return "KHR_DF_CHANNEL_LABSDA_STENCIL";
        case KHR_DF_CHANNEL_LABSDA_DEPTH:
            return "KHR_DF_CHANNEL_LABSDA_DEPTH";
        case KHR_DF_CHANNEL_LABSDA_ALPHA:
            return "KHR_DF_CHANNEL_LABSDA_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_CMYKA:
        switch (value) {
        case KHR_DF_CHANNEL_CMYKSDA_CYAN:
            return "KHR_DF_CHANNEL_CMYKSDA_CYAN";
        case KHR_DF_CHANNEL_CMYKSDA_MAGENTA:
            return "KHR_DF_CHANNEL_CMYKSDA_MAGENTA";
        case KHR_DF_CHANNEL_CMYKSDA_YELLOW:
            return "KHR_DF_CHANNEL_CMYKSDA_YELLOW";
        case KHR_DF_CHANNEL_CMYKSDA_BLACK:
            return "KHR_DF_CHANNEL_CMYKSDA_BLACK";
        case KHR_DF_CHANNEL_CMYKSDA_ALPHA:
            return "KHR_DF_CHANNEL_CMYKSDA_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_XYZW:
        switch (value) {
        case KHR_DF_CHANNEL_XYZW_X:
            return "KHR_DF_CHANNEL_XYZW_X";
        case KHR_DF_CHANNEL_XYZW_Y:
            return "KHR_DF_CHANNEL_XYZW_Y";
        case KHR_DF_CHANNEL_XYZW_Z:
            return "KHR_DF_CHANNEL_XYZW_Z";
        case KHR_DF_CHANNEL_XYZW_W:
            return "KHR_DF_CHANNEL_XYZW_W";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_HSVA_ANG:
        switch (value) {
        case KHR_DF_CHANNEL_HSVA_ANG_VALUE:
            return "KHR_DF_CHANNEL_HSVA_ANG_VALUE";
        case KHR_DF_CHANNEL_HSVA_ANG_SATURATION:
            return "KHR_DF_CHANNEL_HSVA_ANG_SATURATION";
        case KHR_DF_CHANNEL_HSVA_ANG_HUE:
            return "KHR_DF_CHANNEL_HSVA_ANG_HUE";
        case KHR_DF_CHANNEL_HSVA_ANG_ALPHA:
            return "KHR_DF_CHANNEL_HSVA_ANG_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_HSLA_ANG:
        switch (value) {
        case KHR_DF_CHANNEL_HSLA_ANG_LIGHTNESS:
            return "KHR_DF_CHANNEL_HSLA_ANG_LIGHTNESS";
        case KHR_DF_CHANNEL_HSLA_ANG_SATURATION:
            return "KHR_DF_CHANNEL_HSLA_ANG_SATURATION";
        case KHR_DF_CHANNEL_HSLA_ANG_HUE:
            return "KHR_DF_CHANNEL_HSLA_ANG_HUE";
        case KHR_DF_CHANNEL_HSLA_ANG_ALPHA:
            return "KHR_DF_CHANNEL_HSLA_ANG_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_HSVA_HEX:
        switch (value) {
        case KHR_DF_CHANNEL_HSVA_HEX_VALUE:
            return "KHR_DF_CHANNEL_HSVA_HEX_VALUE";
        case KHR_DF_CHANNEL_HSVA_HEX_SATURATION:
            return "KHR_DF_CHANNEL_HSVA_HEX_SATURATION";
        case KHR_DF_CHANNEL_HSVA_HEX_HUE:
            return "KHR_DF_CHANNEL_HSVA_HEX_HUE";
        case KHR_DF_CHANNEL_HSVA_HEX_ALPHA:
            return "KHR_DF_CHANNEL_HSVA_HEX_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_HSLA_HEX:
        switch (value) {
        case KHR_DF_CHANNEL_HSLA_HEX_LIGHTNESS:
            return "KHR_DF_CHANNEL_HSLA_HEX_LIGHTNESS";
        case KHR_DF_CHANNEL_HSLA_HEX_SATURATION:
            return "KHR_DF_CHANNEL_HSLA_HEX_SATURATION";
        case KHR_DF_CHANNEL_HSLA_HEX_HUE:
            return "KHR_DF_CHANNEL_HSLA_HEX_HUE";
        case KHR_DF_CHANNEL_HSLA_HEX_ALPHA:
            return "KHR_DF_CHANNEL_HSLA_HEX_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_YCGCOA:
        switch (value) {
        case KHR_DF_CHANNEL_YCGCOA_Y:
            return "KHR_DF_CHANNEL_YCGCOA_Y";
        case KHR_DF_CHANNEL_YCGCOA_CG:
            return "KHR_DF_CHANNEL_YCGCOA_CG";
        case KHR_DF_CHANNEL_YCGCOA_CO:
            return "KHR_DF_CHANNEL_YCGCOA_CO";
        case KHR_DF_CHANNEL_YCGCOA_ALPHA:
            return "KHR_DF_CHANNEL_YCGCOA_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_CIEXYZ:
        switch (value) {
        case KHR_DF_CHANNEL_CIEXYZ_X:
            return "KHR_DF_CHANNEL_CIEXYZ_X";
        case KHR_DF_CHANNEL_CIEXYZ_Y:
            return "KHR_DF_CHANNEL_CIEXYZ_Y";
        case KHR_DF_CHANNEL_CIEXYZ_Z:
            return "KHR_DF_CHANNEL_CIEXYZ_Z";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_CIEXYY:
        switch (value) {
        case KHR_DF_CHANNEL_CIEXYY_X:
            return "KHR_DF_CHANNEL_CIEXYY_X";
        case KHR_DF_CHANNEL_CIEXYY_YCHROMA:
            return "KHR_DF_CHANNEL_CIEXYY_YCHROMA";
        case KHR_DF_CHANNEL_CIEXYY_YLUMA:
            return "KHR_DF_CHANNEL_CIEXYY_YLUMA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC1A:
        switch (value) {
        case KHR_DF_CHANNEL_BC1A_COLOR:
            return "KHR_DF_CHANNEL_BC1A_COLOR";
        case KHR_DF_CHANNEL_BC1A_ALPHA:
            return "KHR_DF_CHANNEL_BC1A_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC2:
        switch (value) {
        case KHR_DF_CHANNEL_BC2_COLOR:
            return "KHR_DF_CHANNEL_BC2_COLOR";
        case KHR_DF_CHANNEL_BC2_ALPHA:
            return "KHR_DF_CHANNEL_BC2_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC3:
        switch (value) {
        case KHR_DF_CHANNEL_BC3_COLOR:
            return "KHR_DF_CHANNEL_BC3_COLOR";
        case KHR_DF_CHANNEL_BC3_ALPHA:
            return "KHR_DF_CHANNEL_BC3_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC4:
        switch (value) {
        case KHR_DF_CHANNEL_BC4_DATA:
            return "KHR_DF_CHANNEL_BC4_DATA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC5:
        switch (value) {
        case KHR_DF_CHANNEL_BC5_RED:
            return "KHR_DF_CHANNEL_BC5_RED";
        case KHR_DF_CHANNEL_BC5_GREEN:
            return "KHR_DF_CHANNEL_BC5_GREEN";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC6H:
        switch (value) {
        case KHR_DF_CHANNEL_BC6H_COLOR:
            return "KHR_DF_CHANNEL_BC6H_COLOR";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_BC7:
        switch (value) {
        case KHR_DF_CHANNEL_BC7_COLOR:
            return "KHR_DF_CHANNEL_BC7_COLOR";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_ETC1:
        switch (value) {
        case KHR_DF_CHANNEL_ETC1_COLOR:
            return "KHR_DF_CHANNEL_ETC1_COLOR";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_ETC2:
        switch (value) {
        case KHR_DF_CHANNEL_ETC2_RED:
            return "KHR_DF_CHANNEL_ETC2_RED";
        case KHR_DF_CHANNEL_ETC2_GREEN:
            return "KHR_DF_CHANNEL_ETC2_GREEN";
        case KHR_DF_CHANNEL_ETC2_COLOR:
            return "KHR_DF_CHANNEL_ETC2_COLOR";
        case KHR_DF_CHANNEL_ETC2_ALPHA:
            return "KHR_DF_CHANNEL_ETC2_ALPHA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_ASTC:
        switch (value) {
        case KHR_DF_CHANNEL_ASTC_DATA:
            return "KHR_DF_CHANNEL_ASTC_DATA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_ETC1S:
        switch (value) {
        case KHR_DF_CHANNEL_ETC1S_RGB:
            return "KHR_DF_CHANNEL_ETC1S_RGB";
        case KHR_DF_CHANNEL_ETC1S_RRR:
            return "KHR_DF_CHANNEL_ETC1S_RRR";
        case KHR_DF_CHANNEL_ETC1S_GGG:
            return "KHR_DF_CHANNEL_ETC1S_GGG";
        case KHR_DF_CHANNEL_ETC1S_AAA:
            return "KHR_DF_CHANNEL_ETC1S_AAA";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_PVRTC:
        switch (value) {
        case KHR_DF_CHANNEL_PVRTC_COLOR:
            return "KHR_DF_CHANNEL_PVRTC_COLOR";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_PVRTC2:
        switch (value) {
        case KHR_DF_CHANNEL_PVRTC2_COLOR:
            return "KHR_DF_CHANNEL_PVRTC2_COLOR";
        default:
            return NULL;
        }

    case KHR_DF_MODEL_UASTC:
        switch (value) {
        case KHR_DF_CHANNEL_UASTC_RGB:
            return "KHR_DF_CHANNEL_UASTC_RGB";
        case KHR_DF_CHANNEL_UASTC_RGBA:
            return "KHR_DF_CHANNEL_UASTC_RGBA";
        case KHR_DF_CHANNEL_UASTC_RRR:
            return "KHR_DF_CHANNEL_UASTC_RRR";
        case KHR_DF_CHANNEL_UASTC_RRRG:
            return "KHR_DF_CHANNEL_UASTC_RRRG";
        case KHR_DF_CHANNEL_UASTC_RG:
            return "KHR_DF_CHANNEL_UASTC_RG";
        default:
            return NULL;
        }

    default:
        break;
    }

    switch (value) {
    case KHR_DF_CHANNEL_UNSPECIFIED_0:
        return "KHR_DF_CHANNEL_UNSPECIFIED_0";
    case KHR_DF_CHANNEL_UNSPECIFIED_1:
        return "KHR_DF_CHANNEL_UNSPECIFIED_1";
    case KHR_DF_CHANNEL_UNSPECIFIED_2:
        return "KHR_DF_CHANNEL_UNSPECIFIED_2";
    case KHR_DF_CHANNEL_UNSPECIFIED_3:
        return "KHR_DF_CHANNEL_UNSPECIFIED_3";
    case KHR_DF_CHANNEL_UNSPECIFIED_4:
        return "KHR_DF_CHANNEL_UNSPECIFIED_4";
    case KHR_DF_CHANNEL_UNSPECIFIED_5:
        return "KHR_DF_CHANNEL_UNSPECIFIED_5";
    case KHR_DF_CHANNEL_UNSPECIFIED_6:
        return "KHR_DF_CHANNEL_UNSPECIFIED_6";
    case KHR_DF_CHANNEL_UNSPECIFIED_7:
        return "KHR_DF_CHANNEL_UNSPECIFIED_7";
    case KHR_DF_CHANNEL_UNSPECIFIED_8:
        return "KHR_DF_CHANNEL_UNSPECIFIED_8";
    case KHR_DF_CHANNEL_UNSPECIFIED_9:
        return "KHR_DF_CHANNEL_UNSPECIFIED_9";
    case KHR_DF_CHANNEL_UNSPECIFIED_10:
        return "KHR_DF_CHANNEL_UNSPECIFIED_10";
    case KHR_DF_CHANNEL_UNSPECIFIED_11:
        return "KHR_DF_CHANNEL_UNSPECIFIED_11";
    case KHR_DF_CHANNEL_UNSPECIFIED_12:
        return "KHR_DF_CHANNEL_UNSPECIFIED_12";
    case KHR_DF_CHANNEL_UNSPECIFIED_13:
        return "KHR_DF_CHANNEL_UNSPECIFIED_13";
    case KHR_DF_CHANNEL_UNSPECIFIED_14:
        return "KHR_DF_CHANNEL_UNSPECIFIED_14";
    case KHR_DF_CHANNEL_UNSPECIFIED_15:
        return "KHR_DF_CHANNEL_UNSPECIFIED_15";
    default:
        break;
    }

    return NULL;
}

/**
 * @internal
 */
static void printFlagBits(uint32_t flags, const char*(*toStringFn)(uint32_t, bool)) {
    for (uint32_t bit_index = 0; bit_index < 32; ++bit_index) {
        uint32_t bit_mask = 1u << bit_index;
        bool bit_value = (bit_mask & (uint32_t) flags) != 0;

        const char* comma = (uint32_t) flags >= (bit_mask << 1u) ? ", " : "";
        const char* str = toStringFn(bit_index, bit_value);
        if (str)
            printf("%s%s", str, comma);
        else if (bit_value)
            printf("%d%s", bit_mask, comma);
    }
}

/**
 * @internal
 */
static void printFlagBitsJSON(uint32_t indent, const char* nl, uint32_t flags, const char*(*toStringFn)(uint32_t, bool)) {
    for (uint32_t bit_index = 0; bit_index < 32; ++bit_index) {
        uint32_t bit_mask = 1u << bit_index;
        bool bit_value = (bit_mask & (uint32_t) flags) != 0;

        const char* comma = (uint32_t) flags >= (bit_mask << 1u) ? ", " : "";
        const char* str = toStringFn(bit_index, bit_value);
        if (str)
            printf("%*s\"%s\"%s%s", indent, "", str, comma, nl);
        else if (bit_value)
            printf("%*s%d%s%s", indent, "", bit_mask, comma, nl);
    }
}

/**
 * @~English
 * @brief Print a human-readable interpretation of a data format descriptor.
 *
 * @param DFD Pointer to a data format descriptor.
 **/
void printDFD(uint32_t *DFD)
{
    uint32_t *BDB = DFD+1;
    int samples = (KHR_DFDVAL(BDB, DESCRIPTORBLOCKSIZE) - 4 * KHR_DF_WORD_SAMPLESTART) / (4 * KHR_DF_WORD_SAMPLEWORDS);
    int sample;
    int model = KHR_DFDVAL(BDB, MODEL);

#define PRINT_ENUM(VALUE, TO_STRING_FN) {                        \
        int value = VALUE;                                       \
        const char* str = TO_STRING_FN(value);                   \
        if (str)                                                 \
            printf("%s", str);                                   \
        else                                                     \
            printf("%d", value);                                 \
    }

    printf("DFD total bytes: %d\n", DFD[0]);

    printf("BDB descriptor type: ");
    PRINT_ENUM(KHR_DFDVAL(BDB, DESCRIPTORTYPE), dfdToStringDescriptorType);
    printf("\nVendor ID: ");
    PRINT_ENUM(KHR_DFDVAL(BDB, VENDORID), dfdToStringVendorID);
    printf("\n");

    printf("Descriptor block size: %d (%d sample%s)\nVersionNumber: ",
            KHR_DFDVAL(BDB, DESCRIPTORBLOCKSIZE),
            samples,
            samples != 1 ? "s" : "");
    PRINT_ENUM(KHR_DFDVAL(BDB, VERSIONNUMBER), dfdToStringVersionNumber);
    printf("\n");

    khr_df_flags_e flags = KHR_DFDVAL(BDB, FLAGS);
    printf("Flags: 0x%x (", flags);
    printFlagBits(flags, dfdToStringFlagsBit);
    printf(")\nTransfer: ");
    PRINT_ENUM(KHR_DFDVAL(BDB, TRANSFER), dfdToStringTransferFunction);
    printf("\nPrimaries: ");
    PRINT_ENUM(KHR_DFDVAL(BDB, PRIMARIES), dfdToStringColorPrimaries);
    printf("\nModel: ");
    PRINT_ENUM(model, dfdToStringColorModel);
    printf("\n");

    printf("Dimensions: %d, %d, %d, %d\n",
           KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION0) + 1,
           KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION1) + 1,
           KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION2) + 1,
           KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION3) + 1);
    printf("Plane bytes: %d, %d, %d, %d, %d, %d, %d, %d\n",
           KHR_DFDVAL(BDB, BYTESPLANE0),
           KHR_DFDVAL(BDB, BYTESPLANE1),
           KHR_DFDVAL(BDB, BYTESPLANE2),
           KHR_DFDVAL(BDB, BYTESPLANE3),
           KHR_DFDVAL(BDB, BYTESPLANE4),
           KHR_DFDVAL(BDB, BYTESPLANE5),
           KHR_DFDVAL(BDB, BYTESPLANE6),
           KHR_DFDVAL(BDB, BYTESPLANE7));
    for (sample = 0; sample < samples; ++sample) {
        khr_df_model_channels_e channelType = KHR_DFDSVAL(BDB, sample, CHANNELID);
        printf("Sample %d:\n", sample);

        khr_df_sample_datatype_qualifiers_e qualifiers = KHR_DFDSVAL(BDB, sample, QUALIFIERS) >> 4;
        printf("    Qualifiers: 0x%x (", qualifiers);
        printFlagBits(qualifiers, dfdToStringSampleDatatypeQualifiers);
        printf(")\n");
        printf("    Channel Type: 0x%x", channelType);
        {
            const char* str = dfdToStringChannelId(model, channelType);
            if (str)
                printf(" (%s)\n", str);
            else
                printf(" (%d)\n", channelType);
        }
        printf("    Length: %d bits Offset: %d\n",
               KHR_DFDSVAL(BDB, sample, BITLENGTH) + 1,
               KHR_DFDSVAL(BDB, sample, BITOFFSET));
        printf("    Position: %d, %d, %d, %d\n",
               KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION0),
               KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION1),
               KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION2),
               KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION3));
        printf("    Lower: 0x%08x\n    Upper: 0x%08x\n",
               KHR_DFDSVAL(BDB, sample, SAMPLELOWER),
               KHR_DFDSVAL(BDB, sample, SAMPLEUPPER));
    }

#undef PRINT_ENUM
}

/**
 * @~English
 * @brief Print a JSON interpretation of a data format descriptor.
 *
 * @param DFD Pointer to a data format descriptor.
 * @param base_indent The number of indentations to include at the front of every line
 * @param indent_width The number of spaces to add with each nested scope
 * @param minified Specifies whether the JSON output should be minified
 **/
void printDFDJSON(uint32_t* DFD, uint32_t base_indent, uint32_t indent_width, bool minified)
{
    if (minified) {
        base_indent = 0;
        indent_width = 0;
    }
    const char* space = minified ? "" : " ";
    const char* nl = minified ? "" : "\n";

    uint32_t dfdTotalSize = DFD[0];
    uint32_t dfdTotalSizeRemaining = dfdTotalSize - 4;
    uint32_t* BDB = DFD + 1;

#define LENGTH_OF_INDENT(INDENT) ((base_indent + INDENT) * indent_width)

/** Prints an enum as string or number */
#define PRINT_ENUM(INDENT, NAME, VALUE, TO_STRING_FN, COMMA) {             \
        int value = VALUE;                                                 \
        printf("%*s\"" NAME "\":%s", LENGTH_OF_INDENT(INDENT), "", space); \
        const char* str = TO_STRING_FN(value);                             \
        if (str)                                                           \
            printf("\"%s\"", str);                                         \
        else                                                               \
            printf("%d", value);                                           \
        printf(COMMA "%s", nl);                                            \
    }

/** Prints an enum as string or number if the to string function fails with a trailing comma*/
#define PRINT_ENUM_C(INDENT, NAME, VALUE, TO_STRING_FN) \
        PRINT_ENUM(INDENT, NAME, VALUE, TO_STRING_FN, ",")

/** Prints an enum as string or number if the to string function fails without a trailing comma*/
#define PRINT_ENUM_E(INDENT, NAME, VALUE, TO_STRING_FN) \
        PRINT_ENUM(INDENT, NAME, VALUE, TO_STRING_FN, "")

#define PRINT_INDENT(INDENT, FMT, ...) {                              \
        printf("%*s" FMT, LENGTH_OF_INDENT(INDENT), "", __VA_ARGS__); \
    }

    PRINT_INDENT(0, "\"totalSize\":%s%d,%s", space, dfdTotalSize, nl)
    PRINT_INDENT(0, "\"blocks\":%s[%s", space, nl)

    while (dfdTotalSizeRemaining > 0) {
        int blockSize = KHR_DFDVAL(BDB, DESCRIPTORBLOCKSIZE);
        int samples = (blockSize - 4 * KHR_DF_WORD_SAMPLESTART) / (4 * KHR_DF_WORD_SAMPLEWORDS);
        int model = KHR_DFDVAL(BDB, MODEL);

        PRINT_INDENT(1, "{%s", nl)
        PRINT_ENUM_C(2, "descriptorType", KHR_DFDVAL(BDB, DESCRIPTORTYPE), dfdToStringDescriptorType);
        PRINT_ENUM_C(2, "vendorId", KHR_DFDVAL(BDB, VENDORID), dfdToStringVendorID);
        PRINT_INDENT(2, "\"descriptorBlockSize\":%s%d,%s", space, blockSize, nl)
        PRINT_ENUM_C(2, "versionNumber", KHR_DFDVAL(BDB, VERSIONNUMBER), dfdToStringVersionNumber);

        PRINT_INDENT(2, "\"flags\":%s[%s", space, nl)
        khr_df_flags_e flags = KHR_DFDVAL(BDB, FLAGS);
        printFlagBitsJSON(LENGTH_OF_INDENT(3), nl, flags, dfdToStringFlagsBit);
        PRINT_INDENT(2, "],%s", nl)

        PRINT_ENUM_C(2, "transferFunction", KHR_DFDVAL(BDB, TRANSFER), dfdToStringTransferFunction);
        PRINT_ENUM_C(2, "colorPrimaries", KHR_DFDVAL(BDB, PRIMARIES), dfdToStringColorPrimaries);
        PRINT_ENUM_C(2, "colorModel", model, dfdToStringColorModel);
        PRINT_INDENT(2, "\"texelBlockDimension\":%s[%d,%s%d,%s%d,%s%d],%s", space,
                KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION0) + 1, space,
                KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION1) + 1, space,
                KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION2) + 1, space,
                KHR_DFDVAL(BDB, TEXELBLOCKDIMENSION3) + 1, nl)
        PRINT_INDENT(2, "\"bytesPlane\":%s[%d,%s%d,%s%d,%s%d,%s%d,%s%d,%s%d,%s%d],%s", space,
                KHR_DFDVAL(BDB, BYTESPLANE0), space,
                KHR_DFDVAL(BDB, BYTESPLANE1), space,
                KHR_DFDVAL(BDB, BYTESPLANE2), space,
                KHR_DFDVAL(BDB, BYTESPLANE3), space,
                KHR_DFDVAL(BDB, BYTESPLANE4), space,
                KHR_DFDVAL(BDB, BYTESPLANE5), space,
                KHR_DFDVAL(BDB, BYTESPLANE6), space,
                KHR_DFDVAL(BDB, BYTESPLANE7), nl)

        PRINT_INDENT(2, "\"samples\":%s[%s", space, nl)
        for (int sample = 0; sample < samples; ++sample) {
            PRINT_INDENT(3, "{%s", nl)

            khr_df_sample_datatype_qualifiers_e qualifiers = KHR_DFDSVAL(BDB, sample, QUALIFIERS) >> 4;
            if (qualifiers == 0) {
                PRINT_INDENT(4, "\"qualifiers\":%s[],%s", space, nl)

            } else {
                PRINT_INDENT(4, "\"qualifiers\":%s[%s", space, nl)
                printFlagBitsJSON(LENGTH_OF_INDENT(5), nl, qualifiers, dfdToStringSampleDatatypeQualifiers);
                PRINT_INDENT(4, "],%s", nl)
            }

            khr_df_model_channels_e channelType = KHR_DFDSVAL(BDB, sample, CHANNELID);
            const char* channelStr = dfdToStringChannelId(model, channelType);
            if (channelStr)
                PRINT_INDENT(4, "\"channelType\":%s\"%s\",%s", space, channelStr, nl)
            else
                PRINT_INDENT(4, "\"channelType\":%s%d,%s", space, channelType, nl)

            PRINT_INDENT(4, "\"bitLength\":%s%d,%s", space, KHR_DFDSVAL(BDB, sample, BITLENGTH), nl)
            PRINT_INDENT(4, "\"bitOffset\":%s%d,%s", space, KHR_DFDSVAL(BDB, sample, BITOFFSET), nl)
            PRINT_INDENT(4, "\"samplePosition\":%s[%d,%s%d,%s%d,%s%d],%s", space,
                    KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION0), space,
                    KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION1), space,
                    KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION2), space,
                    KHR_DFDSVAL(BDB, sample, SAMPLEPOSITION3), nl)

            if (qualifiers & KHR_DF_SAMPLE_DATATYPE_SIGNED) {
                PRINT_INDENT(4, "\"sampleLower\":%s%d,%s", space, KHR_DFDSVAL(BDB, sample, SAMPLELOWER), nl)
                PRINT_INDENT(4, "\"sampleUpper\":%s%d%s", space, KHR_DFDSVAL(BDB, sample, SAMPLEUPPER), nl)
            } else {
                PRINT_INDENT(4, "\"sampleLower\":%s%u,%s", space, (unsigned int) KHR_DFDSVAL(BDB, sample, SAMPLELOWER), nl)
                PRINT_INDENT(4, "\"sampleUpper\":%s%u%s", space, (unsigned int) KHR_DFDSVAL(BDB, sample, SAMPLEUPPER), nl)
            }

            if (sample + 1 != samples)
                PRINT_INDENT(3, "},%s", nl)
            else
                PRINT_INDENT(3, "}%s", nl)
        }
        PRINT_INDENT(2, "]%s", nl) // End of samples

        PRINT_INDENT(1, "}%s", nl) // End of block

        assert((uint32_t) blockSize <= dfdTotalSizeRemaining);
        dfdTotalSizeRemaining -= blockSize;
        BDB += blockSize / 4;
    }
    PRINT_INDENT(0, "]%s", nl) // End of blocks

#undef PRINT_ENUM
#undef PRINT_ENUM_C
#undef PRINT_ENUM_E
#undef PRINT_INDENT
}
