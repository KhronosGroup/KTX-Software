# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxTextureCreateFlagBits(IntEnum):
    NO_FLAGS = 0x00
    LOAD_IMAGE_DATA_BIT = 0x01
    RAW_KVDATA_BIT = 0x02
    SKIP_KVDATA_BIT = 0x04
