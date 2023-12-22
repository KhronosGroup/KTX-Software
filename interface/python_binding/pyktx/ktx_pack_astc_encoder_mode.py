# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackAstcEncoderMode(IntEnum):
    """
    Options specifying ASTC encoder profile mode.

    This and function is used later to derive the profile.
    """

    DEFAULT = 0
    LDR = 1
    HDR = 2
    MAX = HDR
