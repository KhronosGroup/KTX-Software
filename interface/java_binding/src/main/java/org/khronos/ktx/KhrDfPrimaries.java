/*
 * Copyright (c) 2026, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

/**
 * Definition of the primary colors in color coordinates.
 * 
 * This is implicitly responsible for defining the conversion between RGB an YUV
 * color spaces. LAB and related absolute color models should use
 * {@link KhrDfPrimaries#CIEXYZ}.
 */
public class KhrDfPrimaries {
    /**
     * No color primaries defined
     */
    public static final int UNSPECIFIED = 0;

    /**
     * Color primaries of ITU-R BT.709 and sRGB
     */
    public static final int BT709 = 1;

    /**
     * Synonym for BT709
     */
    public static final int SRGB = 1;

    /**
     * Color primaries of ITU-R BT.601 (625-line EBU variant)
     */
    public static final int BT601_EBU = 2;

    /**
     * Color primaries of ITU-R BT.601 (525-line SMPTE C variant)
     */
    public static final int BT601_SMPTE = 3;

    /**
     * Color primaries of ITU-R BT.2020
     */
    public static final int BT2020 = 4;

    /**
     * ITU-R BT.2100 uses the same primaries as BT.2020
     */
    public static final int BT2100 = 4;

    /**
     * CIE theoretical color coordinate space
     */
    public static final int CIEXYZ = 5;

    /**
     * Academy Color Encoding System primaries
     */
    public static final int ACES = 6;

    /**
     * Color primaries of ACEScc
     */
    public static final int ACESCC = 7;

    /**
     * Legacy NTSC 1953 primaries
     */
    public static final int NTSC1953 = 8;

    /**
     * Legacy PAL 525-line primaries
     */
    public static final int PAL525 = 9;

    /**
     * Color primaries of Display P3
     */
    public static final int DISPLAYP3 = 10;

    /**
     * Color primaries of Adobe RGB (1998)
     */
    public static final int ADOBERGB = 11;

    /**
     * Returns a string representation of the given primary
     *
     * @param n The primary
     * @return A string representation of the given primary
     */
    public static String stringFor(int n) {
	switch (n) {
	case UNSPECIFIED:
	    return "UNSPECIFIED";
	case BT709:
	    return "BT709 (sRGB)";
	case BT601_EBU:
	    return "BT601_EBU";
	case BT601_SMPTE:
	    return "BT601_SMPTE";
	case BT2100:
	    return "BT2100 (BT2020)";
	case CIEXYZ:
	    return "CIEXYZ";
	case ACES:
	    return "ACES";
	case ACESCC:
	    return "ACESCC";
	case NTSC1953:
	    return "NTSC1953";
	case PAL525:
	    return "PAL525";
	case DISPLAYP3:
	    return "DISPLAYP3";
	case ADOBERGB:
	    return "ADOBERGB";

	}
	return "[Unknown KhrDfPrimaries]";
    }

    /**
     * Private constructor to prevent instantiation
     */
    private KhrDfPrimaries() {
	// Prevent instantiation
    }

}
