# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackAstcQualityLevels(IntEnum):
    FASTEST = 0
    FAST = 10
    MEDIUM = 60
    THOROUGH = 98
    EXHAUSTIVE = 100
    MAX = EXHAUSTIVE
