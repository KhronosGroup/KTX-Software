# Copyright (c) 2026, Mark Callow
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum

class KhrDfModel(IntEnum):
    """Model in which the color coordinate space is defined."""

    UNSPECIFIED  = 0
    """No interpretation of color channels defined."""

    RGBSDA       = 1
    """Color primaries (red, green, blue) + alpha, depth and stencil."""

    YUVSDA       = 2
    """Color differences (Y', Cb, Cr) + alpha, depth and stencil."""

    YIQSDA       = 3
    """Color differences (Y', I, Q) + alpha, depth and stencil."""

    LABSDA       = 4
    """Perceptual color (CIE L*a*b*) + alpha, depth and stencil."""

    CMYKA        = 5
    """Subtractive colors (cyan, magenta, yellow, black) + alpha."""

    XYZW         = 6
    """Non-color coordinate data (X, Y, Z, W)."""

    HSVA_ANG     = 7
    """Hue, saturation, value, hue angle on color circle, plus alpha."""

    HSLA_ANG     = 8
    """Hue, saturation, lightness, hue angle on color circle, plus alpha."""

    HSVA_HEX     = 9
    """Hue, saturation, value, hue on color hexagon, plus alpha."""

    HSLA_HEX     = 10
    """Hue, saturation, lightness, hue on color hexagon, plus alpha."""

    YCGCOA       = 11
    """Lightweight approximate color difference (luma, orange, green)."""

    YCCBCCRC     = 12
    """ITU BT.2020 constant luminance YcCbcCrc."""

    ICTCP        = 13
    """ITU BT.2100 constant intensity ICtCp."""

    CIEXYZ       = 14
    """CIE 1931 XYZ color coordinates (X, Y, Z)."""

    CIEXYY       = 15
    """CIE 1931 xyY color coordinates (X, Y, Y)."""


    DXT1A         = 128
    BC1A          = 128
    """Compressed formats start at 128."""
    """
    Direct3D (and S3) compressed formats.
        DXT1 "channels" are RGB (0), Alpha (1).
        DXT1/BC1 with one channel is opaque.
        DXT1/BC1 with a cosited alpha sample is transparent.
    """

    DXT2          = 129
    DXT3          = 129
    BC2           = 129
    """DXT2/DXT3/BC2, with explicit 4-bit alpha."""

    DXT4          = 130
    DXT5          = 130
    BC3           = 130
    """DXT4/DXT5/BC3, with interpolated alpha."""

    ATI1N         = 131
    DXT5A         = 131
    BC4           = 131
    """ATI1n/DXT5A/BC4 - single channel interpolated 8-bit data.
       (The UNORM/SNORM variation is recorded in the channel data)."""

    ATI2N_XY      = 132
    DXN           = 132
    BC5           = 132
    """ATI2n_XY/DXN/BC5 - two channel interpolated 8-bit data.
       (The UNORM/SNORM variation is recorded in the channel data)."""

    BC6H          = 133
    """BC6H - DX11 format for 16-bit float channels."""

    BC7           = 134
    """BC7 - DX11 format."""

    ETC1          = 160
    """A format of ETC1 indicates that the format shall be decodable
       by an ETC1-compliant decoder and not rely on ETC2 features."""

    ETC2          = 161
    """A format of ETC2 is permitted to use ETC2 encodings on top of
       the baseline ETC1 specification.
       The ETC2 format has channels "red", "green", "RGB" and "alpha",
       which should be cosited samples.
       Punch-through alpha can be distinguished from full alpha by
       the plane size in bytes required for the texel block."""

    ASTC          = 162
    """Adaptive Scalable Texture Compression."""
    """ASTC HDR vs LDR is determined by the float flag in the channel."""
    """ASTC block size can be distinguished by texel block size."""

    ETC1S         = 163
    """ETC1S is a simplified subset of ETC1."""

    PVRTC         = 164
    PVRTC2        = 165
    """PowerVR Texture Compression."""

    UASTC         = 166
    UASTC_LDR_4x4 = 166
    UASTC_HDR_4x4 = 167
    UASTC_HDR_6x6 = 168
    """UASTC for BASIS supercompression."""

    MAX = 0xFF
