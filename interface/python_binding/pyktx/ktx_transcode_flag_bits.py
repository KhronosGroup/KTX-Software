# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxTranscodeFlagBits(IntEnum):
    PVRTC_DECODE_TO_NEXT_POW2 = 2
    TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS = 4
    HIGH_QUALITY = 32
