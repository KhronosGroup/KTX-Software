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
	public static final int FASTEST = 0;

	/**
	 * Fast compression
	 */
	public static final int FAST = 10;

	/**
	 * Medium compression
	 */
	public static final int MEDIUM = 60;

	/**
	 * Slower compression
	 */
	public static final int THOROUGH = 98;

	/**
	 * Very slow compression
	 */
	public static final int EXHAUSTIVE = 100;

	/**
	 * Returns a string representation of the given quality level
	 *
	 * @param n The quality level
	 * @return A string representation of the given quality level
	 */
	public static String stringFor(int n) {
		switch (n) {
		case FASTEST: return "FASTEST";
		case FAST: return "FAST";
		case MEDIUM: return "MEDIUM";
		case THOROUGH: return "THOROUGH";
		case EXHAUSTIVE: return "EXHAUSTIVE";
		}
		return "[Unknown KtxPackAstcQualityLevel]";
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxPackAstcQualityLevel() {
		// Prevent instantiation
	}


}
