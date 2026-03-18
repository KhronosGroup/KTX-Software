# Copyright (c) 2026, Mark Callow
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum

class KhrDfPrimaries(IntEnum):
    """The primary colors of an image."""

    KHR_DF_PRIMARIES_UNSPECIFIED = 0
    """No color primaries defined"""

    KHR_DF_PRIMARIES_BT709       = 1
    """Color primaries of ITU-R BT.709 and sRGB"""

    KHR_DF_PRIMARIES_SRGB        = 1
    """Synonym for KHR_DF_PRIMARIES_BT709"""

    KHR_DF_PRIMARIES_BT601_EBU   = 2
    """Color primaries of ITU-R BT.601 (625-line EBU variant)"""

    KHR_DF_PRIMARIES_BT601_SMPTE = 3
    """Color primaries of ITU-R BT.601 (525-line SMPTE C variant)"""

    KHR_DF_PRIMARIES_BT2020      = 4
    """Color primaries of ITU-R BT.2020"""

    KHR_DF_PRIMARIES_BT2100      = 4
    """ITU-R BT.2100 uses the same primaries as BT.2020"""

    KHR_DF_PRIMARIES_CIEXYZ      = 5
    """CIE theoretical color coordinate space"""

    KHR_DF_PRIMARIES_ACES        = 6
    """Academy Color Encoding System primaries"""

    KHR_DF_PRIMARIES_ACESCC      = 7
    """Color primaries of ACEScc"""

    KHR_DF_PRIMARIES_NTSC1953    = 8
    """Legacy NTSC 1953 primaries"""

    KHR_DF_PRIMARIES_PAL525      = 9
    """Legacy PAL 525-line primaries"""

    KHR_DF_PRIMARIES_DISPLAYP3   = 10
    """Color primaries of Display P3"""

    KHR_DF_PRIMARIES_ADOBERGB    = 11
    """Color primaries of Adobe RGB (1998)"""

    KHR_DF_PRIMARIES_MAX         = 0xFF
