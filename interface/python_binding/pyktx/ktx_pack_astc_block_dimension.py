# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackAstcBlockDimension(IntEnum):
    D4x4 = 0
    D5x4 = 1
    D5x5 = 2
    D6x5 = 3
    D6x6 = 4
    D8x5 = 5
    D8x6 = 6
    D10x5 = 7
    D10x6 = 8
    D8x8 = 9
    D10x8 = 10
    D10x10 = 11
    D12x10 = 12
    D12x12 = 13

    D3x3x3 = 14
    D4x3x3 = 15
    D4x4x3 = 16
    D4x4x4 = 17
    D5x4x4 = 18
    D5x5x4 = 19
    D5x5x5 = 20
    D6x5x5 = 21
    D6x6x5 = 22
    D6x6x6 = 23
    DMAX = D6x6x6
