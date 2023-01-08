# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackAstcEncoderMode(IntEnum):
    DEFAULT = 0
    LDR = 1
    HDR = 2
    MAX = HDR
