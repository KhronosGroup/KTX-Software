# Copyright (c) 2026, Mark Callow
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum

class KhrDfTransfer(IntEnum):
    """The transfer function of an image."""

    UNSPECIFIED = 0
    """No transfer function defined."""

    LINEAR      = 1
    """Linear transfer function (value proportional to intensity."""

    SRGB        = 2
    SRGB_EOTF   = 2
    SCRGB       = 2
    SCRGB_EOTF  = 2
    """Perceptually-linear transfer function of sRGB (~2.2); also used for scRGB."""

    ITU         = 3
    ITU_OETF    = 3
    BT601       = 3
    BT601_OETF  = 3
    BT709       = 3
    BT709_OETF  = 3
    BT2020      = 3
    BT2020_OETF = 3
    """Perceptually-linear transfer function of ITU BT.601, BT.709 and BT.2020 (~1/.45)."""

    SMTPE170M      = 3
    SMTPE170M_OETF = 3
    SMTPE170M_EOTF = 3
    """SMTPE170M (digital NTSC) defines an alias for the ITU transfer function (~1/.45) and a linear OOTF."""

    NTSC        = 4
    NTSC_EOTF   = 4
    """Perceptually-linear gamma function of original NTSC (simple 2.2 gamma)."""

    SLOG        = 5
    SLOG_OETF   = 5
    """Sony S-log used by Sony video cameras."""

    SLOG2       = 6
    SLOG2_OETF  = 6
    """Sony S-log 2 used by Sony video cameras."""

    BT1886      = 7
    BT1886_EOTF = 7
    """ITU BT.1886 EOTF."""

    HLG_OETF    = 8
    """ITU BT.2100 HLG OETF (typical scene-referred content), linear light normalized 0..1."""

    HLG_EOTF    = 9
    """ITU BT.2100 HLG EOTF (nominal HDR display of HLG content), linear light normalized 0..1."""

    PQ_EOTF     = 10
    """ITU BT.2100 PQ EOTF (typical HDR display-referred PQ content)."""

    PQ_OETF     = 11
    """ITU BT.2100 PQ OETF (nominal scene described by PQ HDR content)."""

    DCIP3       = 12
    DCIP3_EOTF  = 12
    """DCI P3 transfer function."""

    PAL_OETF    = 13
    """Legacy PAL OETF."""

    PAL625_EOTF = 14
    """Legacy PAL 625-line EOTF."""

    ST240       = 15
    ST240_OETF  = 15
    ST240_EOTF  = 15
    """Legacy ST240 transfer function."""

    ACESCC      = 16
    ACESCC_OETF = 16
    """ACEScc transfer function."""

    ACESCCT      = 17
    ACESCCT_OETF = 17
    """ACEScct transfer function."""

    ADOBERGB      = 18
    ADOBERGB_EOTF = 18
    """Adobe RGB (1998) transfer function."""

    HLG_UNNORMALIZED_OETF = 19
    """Legacy ITU BT.2100 HLG OETF (typical scene-referred content), linear light normalized 0..12."""

    MAX                   = 0xFF
