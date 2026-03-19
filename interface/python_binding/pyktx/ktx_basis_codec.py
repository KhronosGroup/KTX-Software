# Copyright (c) 2026, Mark Callow
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum

class KtxBasisCodec(IntEnum):
    """Options specifiying basis codec."""

    NONE  = 0
    """NONE."""
    ETC1S   = 1
    """BasisLZ."""
    UASTC_LDR_4x4   = 2
    """UASTC."""
    UASTC_HDR_4x4   = 3
    """UASTC_HDR_4x4."""
    UASTC_HDR_6x6_INTERMEDIATE = 4
    """UASTC_HDR_6x6i."""
