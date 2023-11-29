# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxPackAstcQualityLevels(IntEnum):
    """Options specifiying ASTC encoding quality levels."""

    FASTEST = 0
    """Fastest compression."""

    FAST = 10
    """Fast compression."""

    MEDIUM = 60
    """Medium compression."""

    THOROUGH = 98
    """Slower compression."""

    EXHAUSTIVE = 100
    """Very slow compression."""

    MAX = EXHAUSTIVE
    """Maximum supported quality level."""
