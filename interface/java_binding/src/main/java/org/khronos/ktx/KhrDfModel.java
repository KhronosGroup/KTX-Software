/*
 * Copyright (c) 2026, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

/**
 * Model in which the color coordinate space is defined.
 * 
 * There is no requirement that a color format use all the channel types that
 * are defined in the color model.
 * 
 * These compressed formats (starting at 128) should generally have a single
 * sample, sited at the 0,0 position of the texel block. Where multiple channels
 * are used to distinguish formats, these should be cosited.
 */
public class KhrDfModel {

    /**
     * No interpretation of color channels defined
     */
    public static final int UNSPECIFIED = 0;

    /**
     * Color primaries (red, green, blue) + alpha, depth and stencil
     */
    public static final int RGBSDA = 1;

    /**
     * Color differences (Y', Cb, Cr) + alpha, depth and stencil
     */
    public static final int YUVSDA = 2;

    /**
     * Color differences (Y', I, Q) + alpha, depth and stencil
     */
    public static final int YIQSDA = 3;

    /**
     * Perceptual color (CIE L*a*b*) + alpha, depth and stencil
     */
    public static final int LABSDA = 4;

    /**
     * Subtractive colors (cyan, magenta, yellow, black) + alpha
     */
    public static final int CMYKA = 5;

    /**
     * Non-color coordinate data (X, Y, Z, W)
     */
    public static final int XYZW = 6;

    /**
     * Hue, saturation, value, hue angle on color circle, plus alpha
     */
    public static final int HSVA_ANG = 7;

    /**
     * Hue, saturation, lightness, hue angle on color circle, plus alpha
     */
    public static final int HSLA_ANG = 8;

    /**
     * Hue, saturation, value, hue on color hexagon, plus alpha
     */
    public static final int HSVA_HEX = 9;

    /**
     * Hue, saturation, lightness, hue on color hexagon, plus alpha
     */
    public static final int HSLA_HEX = 10;

    /**
     * Lightweight approximate color difference (luma, orange, green)
     */
    public static final int YCGCOA = 11;

    /**
     * ITU BT.2020 constant luminance YcCbcCrc
     */
    public static final int YCCBCCRC = 12;

    /**
     * ITU BT.2100 constant intensity ICtCp
     */
    public static final int ICTCP = 13;

    /**
     * CIE 1931 XYZ color coordinates (X, Y, Z)
     */
    public static final int CIEXYZ = 14;

    /**
     * CIE 1931 xyY color coordinates (X, Y, Y)
     */
    public static final int CIEXYY = 15;

    /**
     * DXT. Note that premultiplied status is recorded separately. DXT1 "channels"
     * are RGB (0), Alpha (1). DXT1/BC1 with one channel is opaque. DXT1/BC1 with a
     * cosited alpha sample is transparent.
     */
    public static final int DXT1A = 128;

    /**
     * DXT. Note that premultiplied status is recorded separately. DXT1 "channels"
     * are RGB (0), Alpha (1). DXT1/BC1 with one channel is opaque. DXT1/BC1 with a
     * cosited alpha sample is transparent.
     */
    public static final int BC1A = 128;

    /**
     * DXT2/DXT3/BC2, with explicit 4-bit alpha
     */
    public static final int DXT2 = 129;

    /**
     * DXT2/DXT3/BC2, with explicit 4-bit alpha
     */
    public static final int DXT3 = 129;

    /**
     * DXT2/DXT3/BC2, with explicit 4-bit alpha
     */
    public static final int BC2 = 129;

    /**
     * DXT4/DXT5/BC3, with interpolated alpha
     */
    public static final int DXT4 = 130;

    /**
     * DXT4/DXT5/BC3, with interpolated alpha
     */
    public static final int DXT5 = 130;

    /**
     * DXT4/DXT5/BC3, with interpolated alpha
     */
    public static final int BC3 = 130;

    /**
     * ATI1n/DXT5A/BC4 - single channel interpolated 8-bit data (The UNORM/SNORM
     * variation is recorded in the channel data)
     */
    public static final int ATI1N = 131;

    /**
     * ATI1n/DXT5A/BC4 - single channel interpolated 8-bit data (The UNORM/SNORM
     * variation is recorded in the channel data)
     */
    public static final int DXT5A = 131;

    /**
     * ATI1n/DXT5A/BC4 - single channel interpolated 8-bit data (The UNORM/SNORM
     * variation is recorded in the channel data)
     */
    public static final int BC4 = 131;

    /**
     * ATI2n_XY/DXN/BC5 - two channel interpolated 8-bit data (The UNORM/SNORM
     * variation is recorded in the channel data)
     */
    public static final int ATI2N_XY = 132;

    /**
     * ATI2n_XY/DXN/BC5 - two channel interpolated 8-bit data (The UNORM/SNORM
     * variation is recorded in the channel data)
     */
    public static final int DXN = 132;

    /**
     * ATI2n_XY/DXN/BC5 - two channel interpolated 8-bit data (The UNORM/SNORM
     * variation is recorded in the channel data)
     */
    public static final int BC5 = 132;

    /**
     * BC6H - DX11 format for 16-bit float channels
     */
    public static final int BC6H = 133;

    /**
     * BC7 - DX11 format
     */
    public static final int BC7 = 134;

    /**
     * A format of ETC1 indicates that the format shall be decodable by an
     * ETC1-compliant decoder and not rely on ETC2 features
     */
    public static final int ETC1 = 160;

    /**
     * A format of ETC2 is permitted to use ETC2 encodings on top of the baseline
     * ETC1 specification. The ETC2 format has channels "red", "green", "RGB" and
     * "alpha", which should be cosited samples. Punch-through alpha can be
     * distinguished from full alpha by the plane size in bytes required for the
     * texel block
     */
    public static final int ETC2 = 161;

    /**
     * Adaptive Scalable Texture Compression: ASTC HDR vs LDR is determined by the
     * float flag in the channel. ASTC block size can be distinguished by texel
     * block size
     */
    public static final int ASTC = 162;

    /**
     * ETC1S is a simplified subset of ETC1
     */
    public static final int ETC1S = 163;

    /**
     * PowerVR Texture Compression
     */
    public static final int PVRTC = 164;

    /**
     * PowerVR Texture Compression
     */
    public static final int PVRTC2 = 165;

    /**
     * UASTC for BASIS supercompression
     */
    public static final int UASTC = 166;

    /**
     * UASTC for BASIS supercompression
     */
    public static final int UASTC_LDR_4x4 = 166;

    /**
     * UASTC for BASIS supercompression
     */
    public static final int UASTC_HDR_4x4 = 167;

    /**
     * UASTC for BASIS supercompression
     */
    public static final int UASTC_HDR_6x6 = 168;

    /**
     * Returns a string representation of the given model
     *
     * @param n The model
     * @return A string representation of the given model
     */
    public static String stringFor(int n) {
	switch (n) {
	case UNSPECIFIED:
	    return "UNSPECIFIED";
	case RGBSDA:
	    return "RGBSDA";
	case YUVSDA:
	    return "YUVSDA";
	case YIQSDA:
	    return "YIQSDA";
	case LABSDA:
	    return "LABSDA";
	case CMYKA:
	    return "CMYKA";
	case XYZW:
	    return "XYZW";
	case HSVA_ANG:
	    return "HSVA_ANG";
	case HSLA_ANG:
	    return "HSLA_ANG";
	case HSVA_HEX:
	    return "HSVA_HEX";
	case HSLA_HEX:
	    return "HSLA_HEX";
	case YCGCOA:
	    return "YCGCOA";
	case YCCBCCRC:
	    return "YCCBCCRC";
	case ICTCP:
	    return "ICTCP";
	case CIEXYZ:
	    return "CIEXYZ";
	case CIEXYY:
	    return "CIEXYY";
	case BC1A:
	    return "BC1A (DXT1A)";
	case BC2:
	    return "BC2 (DXT2, DXT3";
	case BC3:
	    return "BC3 (DXT4, DXT5)";
	case BC4:
	    return "BC4 (ATI1N, DXT5A)";
	case BC5:
	    return "BC5 (ATI2N_XY, DXN)";
	case BC6H:
	    return "BC6H";
	case BC7:
	    return "BC7";
	case ETC1:
	    return "ETC1";
	case ETC2:
	    return "ETC2";
	case ASTC:
	    return "ASTC";
	case ETC1S:
	    return "ETC1S";
	case PVRTC:
	    return "PVRTC";
	case PVRTC2:
	    return "PVRTC2";
	case UASTC_LDR_4x4:
	    return "UASTC_LDR_4x4 (UASTC)";
	case UASTC_HDR_4x4:
	    return "UASTC_HDR_4x4";
	case UASTC_HDR_6x6:
	    return "UASTC_HDR_6x6";

	}
	return "[Unknown (prorietary) KhrDfModel]";
    }

    /**
     * Private constructor to prevent instantiation
     */
    private KhrDfModel() {
	// Prevent instantiation
    }

}
