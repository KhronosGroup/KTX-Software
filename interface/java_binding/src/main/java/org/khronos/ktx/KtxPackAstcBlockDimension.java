/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Options specifiying ASTC encoding block dimensions
 * @author User
 *
 */
public class KtxPackAstcBlockDimension {

	/**
	 * 8.00 bpp
	 */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4 = 0;

    /**
     * 6.40 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D5x4 = 1;

    /**
     * 5.12 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5 = 2;

    /**
     * 4.27 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D6x4 = 3;

    /**
     * 3.56 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6 = 4;

    /**
     * 3.20 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D8x5 = 5;

    /**
     * 2.67 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D8x6 = 6;

    /**
     * 2.56 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D10x5 = 7;

    /**
     * 2.13 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D10x6 = 8;

    /**
     * 2.00 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D8x8 = 9;

    /**
     * 1.60 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D10x8 = 10;

    /**
     * 1.28 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D10x10 = 11;

    /**
     * 1.07 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D12x10 = 12;

    /**
     * 0.89 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D12x12 = 13;

    /**
     * 4.74 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D3x3x3 = 14;

    /**
     * 3.56 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D4x3x3 = 15;

    /**
     * 2.67 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4x3 = 16;

    /**
     * 2.00 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4x4 = 17;

    /**
     * 1.60 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D5x4x4 = 18;

    /**
     * 1.28 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5x4 = 19;

    /**
     * 1.02 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5x5 = 20;

    /**
     * 0.85 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D6x5x5 = 21;

    /**
     * 0.71 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6x5 = 22;

    /**
     *  0.59 bpp
     */
    public static final int KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6x6 = 23;

	/**
	 * Returns a string representation of the given block dimensions
	 *
	 * @param n The block dimensions
	 * @return A string representation of the given block dimensions
	 */
	public static String stringFor(int n) {
		switch (n) {
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D5x4: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D5x4";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D6x4: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D6x4";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D8x5: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D8x5";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D8x6: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D8x6";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D10x5: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D10x5";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D10x6: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D10x6";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D8x8: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D8x8";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D10x8: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D10x8";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D10x10: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D10x10";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D12x10: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D12x10";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D12x12: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D12x12";

		case KTX_PACK_ASTC_BLOCK_DIMENSION_D3x3x3: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D3x3x3";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D4x3x3: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D4x3x3";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4x3: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4x3";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4x4: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4x4";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D5x4x4: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D5x4x4";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5x4: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5x4";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5x5: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D5x5x5";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D6x5x5: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D6x5x5";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6x5: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6x5";
		case KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6x6: return "KTX_PACK_ASTC_BLOCK_DIMENSION_D6x6x6";
		}
		return "[Unknown KtxTextureCreateStorage]";
	}

}
