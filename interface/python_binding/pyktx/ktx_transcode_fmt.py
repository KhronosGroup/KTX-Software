# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxTranscodeFmt(IntEnum):
    """Enumerators for specifying the transcode target format."""

    ETC1_RGB = 0
    ETC2_RGBA = 1
    BC1_RGB = 2
    BC3_RGBA = 3
    BC4_R = 4
    BC5_RG = 5
    BC7_RGBA = 6
    PVRTC1_4_RGB = 8
    PVRTC1_4_RGBA = 9
    ASTC_4x4_RGBA = 10
    PVRTC2_4_RGB = 18
    PVRTC2_4_RGBA = 19
    ETC2_EAC_R11 = 20
    ETC2_EAC_RG11 = 21
    RGBA32 = 13
    RGB565 = 14
    BGR565 = 15
    RGBA4444 = 16
    ETC = 22
    BC1_OR_3 = 23
    NO_SELECTION = 0x7fffffff
