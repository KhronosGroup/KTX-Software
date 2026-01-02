/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Options specifying ASTC encoding block dimensions
 */
public class KtxPackAstcBlockDimension {

	/**
	 * 8.00 bpp
	 */
    public static final int D4x4 = 0;

    /**
     * 6.40 bpp
     */
    public static final int D5x4 = 1;

    /**
     * 5.12 bpp
     */
    public static final int D5x5 = 2;

    /**
     * 4.27 bpp
     */
    public static final int D6x5 = 3;

    /**
     * 3.56 bpp
     */
    public static final int D6x6 = 4;

    /**
     * 3.20 bpp
     */
    public static final int D8x5 = 5;

    /**
     * 2.67 bpp
     */
    public static final int D8x6 = 6;

    /**
     * 2.56 bpp
     */
    public static final int D10x5 = 7;

    /**
     * 2.13 bpp
     */
    public static final int D10x6 = 8;

    /**
     * 2.00 bpp
     */
    public static final int D8x8 = 9;

    /**
     * 1.60 bpp
     */
    public static final int D10x8 = 10;

    /**
     * 1.28 bpp
     */
    public static final int D10x10 = 11;

    /**
     * 1.07 bpp
     */
    public static final int D12x10 = 12;

    /**
     * 0.89 bpp
     */
    public static final int D12x12 = 13;

    /**
     * 4.74 bpp
     */
    public static final int D3x3x3 = 14;

    /**
     * 3.56 bpp
     */
    public static final int D4x3x3 = 15;

    /**
     * 2.67 bpp
     */
    public static final int D4x4x3 = 16;

    /**
     * 2.00 bpp
     */
    public static final int D4x4x4 = 17;

    /**
     * 1.60 bpp
     */
    public static final int D5x4x4 = 18;

    /**
     * 1.28 bpp
     */
    public static final int D5x5x4 = 19;

    /**
     * 1.02 bpp
     */
    public static final int D5x5x5 = 20;

    /**
     * 0.85 bpp
     */
    public static final int D6x5x5 = 21;

    /**
     * 0.71 bpp
     */
    public static final int D6x6x5 = 22;

    /**
     *  0.59 bpp
     */
    public static final int D6x6x6 = 23;

	/**
	 * Returns a string representation of the given block dimensions
	 *
	 * @param n The block dimensions
	 * @return A string representation of the given block dimensions
	 */
	public static String stringFor(int n) {
		switch (n) {
		case D4x4: return "D4x4";
		case D5x4: return "D5x4";
		case D5x5: return "D5x5";
		case D6x5: return "D6x5";
		case D6x6: return "D6x6";
		case D8x5: return "D8x5";
		case D8x6: return "D8x6";
		case D10x5: return "D10x5";
		case D10x6: return "D10x6";
		case D8x8: return "D8x8";
		case D10x8: return "D10x8";
		case D10x10: return "D10x10";
		case D12x10: return "D12x10";
		case D12x12: return "D12x12";

		case D3x3x3: return "D3x3x3";
		case D4x3x3: return "D4x3x3";
		case D4x4x3: return "D4x4x3";
		case D4x4x4: return "D4x4x4";
		case D5x4x4: return "D5x4x4";
		case D5x5x4: return "D5x5x4";
		case D5x5x5: return "D5x5x5";
		case D6x5x5: return "D6x5x5";
		case D6x6x5: return "D6x6x5";
		case D6x6x6: return "D6x6x6";
		}
		return "[Unknown KtxPackAstcBlockDimension]";
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxPackAstcBlockDimension() {
		// Prevent instantiation
	}


}
