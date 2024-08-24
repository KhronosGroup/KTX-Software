/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Options specifying ASTC encoder profile mode.<br>
 * <br>
 * These constants can be passed to the {@link KtxAstcParams#setMode(int)}
 * function.
 */
public class KtxPackAstcEncoderMode {
    public static final int DEFAULT = 0;
    public static final int LDR = 1;
    public static final int HDR = 2;

	/**
	 * Returns a string representation of the given encoder mode
	 *
	 * @param n The encoder mode
	 * @return A string representation of the given encoder mode
	 */
	public static String stringFor(int n) {
		switch (n) {
		case DEFAULT: return "DEFAULT";
		case LDR: return "LDR";
		case HDR: return "HDR";
		}
		return "[Unknown KtxPackAstcEncoderMode]";
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxPackAstcEncoderMode() {
		// Prevent instantiation
	}


}
