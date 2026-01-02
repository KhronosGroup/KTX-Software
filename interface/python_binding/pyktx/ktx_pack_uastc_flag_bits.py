# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackUastcFlagBits(IntEnum):
    """Flags specifiying UASTC encoding options."""

    FASTEST = 0
    """Fastest compression. 43.45dB."""

    FASTER = 1
    """Faster compression. 46.49dB."""

    DEFAULT = 2
    """Default compression. 47.47dB."""

    SLOWER = 3
    """Slower compression. 48.01dB."""

    VERY_SLOW = 4
    """Very slow compression. 48.24dB."""

    MAX_LEVEL = VERY_SLOW
    """Maximum supported quality level."""

    LEVEL_MASK = 0xF
    """Mask to extract the level from the other bits."""

    FAVOR_UASTC_ERROR = 8
    """Optimize for lowest UASTC error."""

    FAVOR_BC7_ERROR = 16
    """Optimize for lowest BC7 error."""

    ETC1_FASTER_HINTS = 64
    """Optimize for faster transcoding to ETC1."""

    ETC1_FASTEST_HINTS = 128
    """Optimize for fastest transcoding to ETC1."""

    ETC1_DISABLE_FLIP_AND_INDIVIDUAL = 256
    """Not documented in BasisU code."""
