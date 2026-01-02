# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxTranscodeFlagBits(IntEnum):
    """Flags guiding transcoding of Basis Universal compressed textures."""

    PVRTC_DECODE_TO_NEXT_POW2 = 2
    """
    PVRTC1: decode non-pow2 ETC1S texture level to the next larger
    power of 2 (not implemented yet, but we're going to support it).
    Ignored if the slice's dimensions are already a power of 2.
    """

    TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS = 4
    """
    When decoding to an opaque texture format, if the Basis data has
    alpha, decode the alpha slice instead of the color slice to the
    output texture format. Has no effect if there is no alpha data.
    """

    HIGH_QUALITY = 32
    """
    Request higher quality transcode of UASTC to BC1, BC3, ETC2_EAC_R11 and
    ETC2_EAC_RG11. The flag is unused by other UASTC transcoders.
    """
