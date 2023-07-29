# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxTextureCreateStorage(IntEnum):
    """Enum for requesting, or not, allocation of storage for images."""

    NO = 0
    """Don't allocate any image storage."""

    ALLOC = 1
    """Allocate image storage."""

