# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxTextureCreateFlagBits(IntEnum):
    """Flags for requesting services during creation."""

    NO_FLAGS = 0x00

    LOAD_IMAGE_DATA_BIT = 0x01
    """Load the images from the KTX source."""

    RAW_KVDATA_BIT = 0x02
    """Load the raw key-value data instead of creating a KtxHashList from it."""

    SKIP_KVDATA_BIT = 0x04
    """Skip any key-value data. This overrides the RAW_KVDATA_BIT."""
