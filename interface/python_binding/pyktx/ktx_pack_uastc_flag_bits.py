# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackUastcFlagBits(IntEnum):
    FASTEST = 0
    FASTER = 1
    DEFAULT = 2
    SLOWER = 3
    VERY_SLOW = 4
    MAX_LEVEL = VERY_SLOW

    LEVEL_MASK = 0xF
    FAVOR_UASTC_ERROR = 8
    FAVOR_BC7_ERROR = 16
    ETC1_FASTER_HINTS = 64
    ETC1_FASTEST_HINTS = 128
    ETC1_DISABLE_FLIP_AND_INDIVIDUAL = 256
