/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Options specifying ASTC encoding quality levels.<br>
 * <br>
 * These constants can be passed to {@link KtxAstcParams#setQualityLevel(int)}.
 */
public class KtxPackAstcQualityLevel {

	/**
	 * Fastest compression
	 */
	public static final int KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST = 0;

	/**
	 * Fast compression
	 */
	public static final int KTX_PACK_ASTC_QUALITY_LEVEL_FAST = 10;

	/**
	 * Medium compression
	 */
	public static final int KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM = 60;

	/**
	 * Slower compression
	 */
	public static final int KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH = 98;

	/**
	 * Very slow compression
	 */
	public static final int KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE = 100;

	/**
	 * Returns a string representation of the given quality level
	 *
	 * @param n The quality level
	 * @return A string representation of the given quality level
	 */
	public static String stringFor(int n) {
		switch (n) {
		case KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST: return "KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST";
		case KTX_PACK_ASTC_QUALITY_LEVEL_FAST: return "KTX_PACK_ASTC_QUALITY_LEVEL_FAST";
		case KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM: return "KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM";
		case KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH: return "KTX_PACK_ASTC_QUALITY_LEVEL_THOROUGH";
		case KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE: return "KTX_PACK_ASTC_QUALITY_LEVEL_EXHAUSTIVE";
		}
		return "[Unknown KtxPackAstcQualityLevel]";
	}

}
