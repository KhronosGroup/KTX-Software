# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackBCnQualityLevels(IntEnum):
    """
    Options specifiying high-level BC1, BC3, and BC7 encoding quality levels

    These enums will be mapped to their corresponding BC1/BC3 or BC7 quality
    values. For BC1/BC3, these are mapped to [0, 19]. For BC7, these are mapped
    to particular bitsets of internal flags.
    """

    FASTEST = 0
    """Fastest compression. For BC1/BC2/BC3, this maps to 0."""

    FASTER = 1
    """Faster compression. For BC1/BC3, this maps to 2."""

    FAST = 2
    """Fast compression. For BC1/BC3, this maps to 5."""

    MEDIUM = 3
    """Medium compression. For BC1/BC3, this maps to 10."""

    THOROUGH = 4
    """Thorough compression. For BC1/BC3, this maps to 15."""

    EXHAUSTIVE = 5
    """Exhaustive compression. For BC1/BC3, this maps to 19."""

    MAX = EXHAUSTIVE
    """Maximum supported quality level."""
