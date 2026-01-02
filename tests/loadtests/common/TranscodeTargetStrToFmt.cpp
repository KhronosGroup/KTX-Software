/* -*- tab-width: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * Copyright 2017-2020 Mark Callow, <khronos at callow dot im>.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <ktx.h>
#include "TranscodeTargetStrToFmt.h"

ktx_transcode_fmt_e
TranscodeTargetStrToFmt(std::string format)
{
    if (!format.compare("ETC1_RGB"))
        return KTX_TTF_ETC1_RGB;
    else if (!format.compare("ETC2_RGBA"))
        return KTX_TTF_ETC2_RGBA;
    else if (!format.compare("BC1_RGB"))
        return KTX_TTF_BC1_RGB;
    else if (!format.compare("BC3_RGBA"))
        return KTX_TTF_BC3_RGBA;
    else if (!format.compare("BC4_R"))
        return KTX_TTF_BC4_R;
    else if (!format.compare("BC5_RG"))
        return KTX_TTF_BC5_RG;
    else if (!format.compare("BC7_M6_RGB"))
        return KTX_TTF_BC7_M6_RGB;
    else if (!format.compare("BC7_M5_RGBA"))
        return KTX_TTF_BC7_M5_RGBA;
    else if (!format.compare("PVRTC1_4_RGB"))
        return KTX_TTF_PVRTC1_4_RGB;
    else if (!format.compare("PVRTC1_4_RGBA"))
        return KTX_TTF_PVRTC1_4_RGBA;
    else if (!format.compare("ASTC_4x4_RGBA"))
        return KTX_TTF_ASTC_4x4_RGBA;
    else if (!format.compare("PVRTC2_4_RGB"))
        return KTX_TTF_PVRTC2_4_RGB;
    else if (!format.compare("PVRTC2_4_RGBA"))
        return KTX_TTF_PVRTC2_4_RGBA;
    else if (!format.compare("ETC2_EAC_R11"))
        return KTX_TTF_ETC2_EAC_R11;
    else if (!format.compare("ETC2_EAC_RG11"))
        return KTX_TTF_ETC2_EAC_RG11;
    else if (!format.compare("RGBA32"))
        return KTX_TTF_RGBA32;
    else if (!format.compare("RGB565"))
        return KTX_TTF_RGB565;
    else if (!format.compare("BGR565"))
        return KTX_TTF_BGR565;
    else if (!format.compare("RGBA4444"))
        return KTX_TTF_RGBA4444;
    else if (!format.compare("ETC"))
        return KTX_TTF_ETC;
    else if (!format.compare("BC1_OR_3"))
        return KTX_TTF_BC1_OR_3;
    assert(false); // Error in args in sample table.
    return static_cast<ktx_transcode_fmt_e>(-1); // To keep compilers happy.
}

