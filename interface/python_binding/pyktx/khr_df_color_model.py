# Copyright (c) 2026, Mark Callow
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum

class KhrDfColorModel(IntEnum):
    """Model in which the color coordinate space is defined."""

    KHR_DF_MODEL_UNSPECIFIED  = 0
    """No interpretation of color channels defined."""

    KHR_DF_MODEL_RGBSDA       = 1
    """Color primaries (red, green, blue) + alpha, depth and stencil."""

    KHR_DF_MODEL_YUVSDA       = 2
    """Color differences (Y', Cb, Cr) + alpha, depth and stencil."""

    KHR_DF_MODEL_YIQSDA       = 3
    """Color differences (Y', I, Q) + alpha, depth and stencil."""

    KHR_DF_MODEL_LABSDA       = 4
    """Perceptual color (CIE L*a*b*) + alpha, depth and stencil."""

    KHR_DF_MODEL_CMYKA        = 5
    """Subtractive colors (cyan, magenta, yellow, black) + alpha."""

    KHR_DF_MODEL_XYZW         = 6
    """Non-color coordinate data (X, Y, Z, W)."""

    KHR_DF_MODEL_HSVA_ANG     = 7
    """Hue, saturation, value, hue angle on color circle, plus alpha."""

    KHR_DF_MODEL_HSLA_ANG     = 8
    """Hue, saturation, lightness, hue angle on color circle, plus alpha."""

    KHR_DF_MODEL_HSVA_HEX     = 9
    """Hue, saturation, value, hue on color hexagon, plus alpha."""

    KHR_DF_MODEL_HSLA_HEX     = 10
    """Hue, saturation, lightness, hue on color hexagon, plus alpha."""

    KHR_DF_MODEL_YCGCOA       = 11
    """Lightweight approximate color difference (luma, orange, green)."""

    KHR_DF_MODEL_YCCBCCRC     = 12
    """ITU BT.2020 constant luminance YcCbcCrc."""

    KHR_DF_MODEL_ICTCP        = 13
    """ITU BT.2100 constant intensity ICtCp."""

    KHR_DF_MODEL_CIEXYZ       = 14
    """CIE 1931 XYZ color coordinates (X, Y, Z)."""

    KHR_DF_MODEL_CIEXYY       = 15
    """CIE 1931 xyY color coordinates (X, Y, Y)."""


    KHR_DF_MODEL_DXT1A         = 128
    KHR_DF_MODEL_BC1A          = 128
    """Compressed formats start at 128."""
    """
    Direct3D (and S3) compressed formats.
        DXT1 "channels" are RGB (0), Alpha (1).
        DXT1/BC1 with one channel is opaque.
        DXT1/BC1 with a cosited alpha sample is transparent.
    """

    KHR_DF_MODEL_DXT2          = 129
    KHR_DF_MODEL_DXT3          = 129
    KHR_DF_MODEL_BC2           = 129
    """DXT2/DXT3/BC2, with explicit 4-bit alpha."""

    KHR_DF_MODEL_DXT4          = 130
    KHR_DF_MODEL_DXT5          = 130
    KHR_DF_MODEL_BC3           = 130
    """DXT4/DXT5/BC3, with interpolated alpha."""

    KHR_DF_MODEL_ATI1N         = 131
    KHR_DF_MODEL_DXT5A         = 131
    KHR_DF_MODEL_BC4           = 131
    """ATI1n/DXT5A/BC4 - single channel interpolated 8-bit data.
       (The UNORM/SNORM variation is recorded in the channel data)."""

    KHR_DF_MODEL_ATI2N_XY      = 132
    KHR_DF_MODEL_DXN           = 132
    KHR_DF_MODEL_BC5           = 132
    """ATI2n_XY/DXN/BC5 - two channel interpolated 8-bit data.
       (The UNORM/SNORM variation is recorded in the channel data)."""

    KHR_DF_MODEL_BC6H          = 133
    """BC6H - DX11 format for 16-bit float channels."""

    KHR_DF_MODEL_BC7           = 134
    """BC7 - DX11 format."""

    KHR_DF_MODEL_ETC1          = 160
    """A format of ETC1 indicates that the format shall be decodable
       by an ETC1-compliant decoder and not rely on ETC2 features."""

    KHR_DF_MODEL_ETC2          = 161
    """A format of ETC2 is permitted to use ETC2 encodings on top of
       the baseline ETC1 specification.
       The ETC2 format has channels "red", "green", "RGB" and "alpha",
       which should be cosited samples.
       Punch-through alpha can be distinguished from full alpha by
       the plane size in bytes required for the texel block."""

    KHR_DF_MODEL_ASTC          = 162
    """Adaptive Scalable Texture Compression."""
    """ASTC HDR vs LDR is determined by the float flag in the channel."""
    """ASTC block size can be distinguished by texel block size."""

    KHR_DF_MODEL_ETC1S         = 163
    """ETC1S is a simplified subset of ETC1."""

    KHR_DF_MODEL_PVRTC         = 164
    KHR_DF_MODEL_PVRTC2        = 165
    """PowerVR Texture Compression."""

    KHR_DF_MODEL_UASTC         = 166
    KHR_DF_MODEL_UASTC_LDR_4x4 = 166
    KHR_DF_MODEL_UASTC_HDR_4x4 = 167
    KHR_DF_MODEL_UASTC_HDR_6x6 = 168
    """UASTC for BASIS supercompression."""

    KHR_DF_MODEL_MAX = 0xFF
