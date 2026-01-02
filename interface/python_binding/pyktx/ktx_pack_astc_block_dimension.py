# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackAstcBlockDimension(IntEnum):
    """Options specifiying ASTC encoding block dimensions."""

    D4x4 = 0
    """8.00 bpp"""

    D5x4 = 1
    """6.40 bpp"""

    D5x5 = 2
    """5.12 bpp"""

    D6x5 = 3
    """4.27 bpp"""

    D6x6 = 4
    """3.56 bpp"""

    D8x5 = 5
    """3.20 bpp"""

    D8x6 = 6
    """2.67 bpp"""

    D10x5 = 7
    """2.56 bpp"""

    D10x6 = 8
    """2.13 bpp"""

    D8x8 = 9
    """2.00 bpp"""

    D10x8 = 10
    """1.60 bpp"""

    D10x10 = 11
    """1.28 bpp"""

    D12x10 = 12
    """1.07 bpp"""

    D12x12 = 13
    """0.89 bpp"""


    D3x3x3 = 14
    """4.74 bpp"""

    D4x3x3 = 15
    """3.56 bpp"""

    D4x4x3 = 16
    """2.67 bpp"""

    D4x4x4 = 17
    """2.00 bpp"""

    D5x4x4 = 18
    """1.60 bpp"""

    D5x5x4 = 19
    """1.28 bpp"""

    D5x5x5 = 20
    """1.02 bpp"""

    D6x5x5 = 21
    """0.85 bpp"""

    D6x6x5 = 22
    """0.71 bpp"""

    D6x6x6 = 23
    """0.59 bpp"""


    DMAX = D6x6x6
    "Maximum supported blocks."
