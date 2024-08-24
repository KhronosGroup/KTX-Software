/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Flag bits that determine UASTC compression.<br>
 * <br>
 * These bits can be passed to {@link KtxBasisParams#setUastcFlags(int)}.<br>
 * <br>
 * The most important value is the level given in the least-significant
 * 4 bits which selects a speed vs quality tradeoff.
 */
public class KtxPackUastcFlagBits {

	/**
	 * Fastest compression. 43.45dB.
	 */
    public static final int FASTEST = 0;

    /**
     * Faster compression. 46.49dB.
     */
    public static final int FASTER = 1;

    /**
     * Default compression. 47.47dB.
     */
    public static final int DEFAULT = 2;

    /**
     * Slower compression. 48.01dB.
     */
    public static final int SLOWER = 3;

    /**
     * Very slow compression. 48.24dB.
     */
    public static final int VERYSLOW = 4;

    /**
     * The maximum quality level
     */
    public static final int MAX_LEVEL = VERYSLOW;

    /**
     * Mask to extract the level from the other bits
     */
    public static final int MASK = 0xF;

    /**
     * Optimize for lowest UASTC error
     */
    public static final int FAVOR_UASTC_ERROR = 8;

    /**
     * Optimize for lowest BC7 error
     */
    public static final int FAVOR_BC7_ERROR = 16;

    /**
     * Optimize for faster transcoding to ETC1
     */
    public static final int ETC1_FASTER_HINTS = 64;

    /**
     * Optimize for fastest transcoding to ETC1
     */
    public static final int ETC1_FASTEST_HINTS = 128;

    /**
     * Not documented in BasisU code
     */
    public static final int ETC1_DISABLE_FLIP_AND_INDIVIDUAL = 256;


	/**
	 * Returns a string representation of the given flag bits
	 *
	 * @param n The flag bits
	 * @return A string representation of the given flag bits
	 */
	public static String stringFor(int n) {
		StringBuilder sb = new StringBuilder();
		int level = n & MASK;

		switch (level) {
		case FASTEST: sb.append("FASTEST"); break;
		case FASTER : sb.append("FASTER"); break;
		case DEFAULT: sb.append("DEFAULT"); break;
		case SLOWER: sb.append("SLOWER"); break;
		case VERYSLOW: sb.append("VERYSLOW"); break;
		}

		if ((n & FAVOR_UASTC_ERROR) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("FAVOR_UASTC_ERROR");
		}
		if ((n & FAVOR_BC7_ERROR) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("FAVOR_BC7_ERROR");
		}
		if ((n & ETC1_FASTER_HINTS) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("ETC1_FASTER_HINTS");
		}
		if ((n & ETC1_FASTEST_HINTS) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("ETC1_FASTEST_HINTS");
		}
		if ((n & ETC1_DISABLE_FLIP_AND_INDIVIDUAL) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("ETC1_DISABLE_FLIP_AND_INDIVIDUAL");
		}
		return sb.toString();
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxPackUastcFlagBits() {
		// Prevent instantiation
	}


}
