# Copyright (c) 2026, Mark Callow
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum

class KhrDfPrimaries(IntEnum):
    """The primary colors of an image."""

    UNSPECIFIED = 0
    """No color primaries defined"""

    BT709       = 1
    """Color primaries of ITU-R BT.709 and sRGB"""

    SRGB        = 1
    """Synonym for BT709"""

    BT601_EBU   = 2
    """Color primaries of ITU-R BT.601 (625-line EBU variant)"""

    BT601_SMPTE = 3
    """Color primaries of ITU-R BT.601 (525-line SMPTE C variant)"""

    BT2020      = 4
    """Color primaries of ITU-R BT.2020"""

    BT2100      = 4
    """ITU-R BT.2100 uses the same primaries as BT.2020"""

    CIEXYZ      = 5
    """CIE theoretical color coordinate space"""

    ACES        = 6
    """Academy Color Encoding System primaries"""

    ACESCC      = 7
    """Color primaries of ACEScc"""

    NTSC1953    = 8
    """Legacy NTSC 1953 primaries"""

    PAL525      = 9
    """Legacy PAL 525-line primaries"""

    DISPLAYP3   = 10
    """Color primaries of Display P3"""

    ADOBERGB    = 11
    """Color primaries of Adobe RGB (1998)"""

    MAX         = 0xFF
