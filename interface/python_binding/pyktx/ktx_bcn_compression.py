# Copyright (c) 2026, Khronos Group
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxBCnCompression(IntEnum):
    """BCn compression formats."""

    NONE = 0
    """None."""
    
    BC1 = 1
    """BC1 compression (RGB). Encodes a 4x4 RGB LDR block into 8 bytes."""

    BC1A = 2
    """
    BC1 compression. Encodes a 4x4 RGBA LDR block into 8 bytes. Alpha is encoded
    just using 1 bit (i.e., fully opaque or fully transparent).
    """

    BC2 = 3
    """
    BC2 compression. Encodes a 4x4 RGBA LDR block into 16 bytes. RGB block is
    encoded using BC1 into 8 bytes. Alpha is encoded into 8 bytes by directly
    storing the 16 alpha values into 8 bytes (i.e., each alpha value is stored
    using just 4 bits).
    """

    BC3 = 4
    """
    BC3 compression. Encodes a 4x4 RGBA LDR block into 16 bytes. RGB block is
    encoded using BC1 into 8 bytes. Alpha is encoded into 8 bytes using two
    reference alpha points and 16 3-bit alpha indices for interpolation.
    """

    BC4 = 5
    """BC4 compression. Encodes a 4x4 R LDR block into 8 bytes."""

    BC5 = 6
    """
    BC5 compression. Encodes a 4x4 RG LDR block into 16 bytes. Each channel is
    encoded separately (ideal for 2-channel, non-color data. E.g., normal maps).
    """

    BC6HU = 7
    """BC6HU compression. Encodes a 4x4 RGB HDR unsigned block into 16 bytes."""

    BC6HS = 8
    """BC6HS compression. Encodes a 4x4 RGB HDR signed block into 16 bytes."""

    BC7 = 9
    """BC7 compression. Encodes a 4x4 RGBA LDR block into 16 bytes."""
