# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxSupercmpScheme(IntEnum):
    """Enumerators identifying the supercompression scheme."""

    NONE = 0
    """No supercompression."""

    BASIS_LZ = 1
    """Basis LZ supercompression."""

    ZSTD = 2
    """ZStd supercompression."""

    ZLIB = 3
    """ZLIB supercompression."""
