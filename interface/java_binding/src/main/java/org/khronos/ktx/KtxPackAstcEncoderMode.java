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
    public static final int KTX_PACK_ASTC_ENCODER_MODE_DEFAULT = 0;
    public static final int KTX_PACK_ASTC_ENCODER_MODE_LDR = 1;
    public static final int KTX_PACK_ASTC_ENCODER_MODE_HDR = 2;

	/**
	 * Returns a string representation of the given encoder mode
	 *
	 * @param n The encoder mode
	 * @return A string representation of the given encoder mode
	 */
	public static String stringFor(int n) {
		switch (n) {
		case KTX_PACK_ASTC_ENCODER_MODE_DEFAULT: return "KTX_PACK_ASTC_ENCODER_MODE_DEFAULT";
		case KTX_PACK_ASTC_ENCODER_MODE_LDR: return "KTX_PACK_ASTC_ENCODER_MODE_LDR";
		case KTX_PACK_ASTC_ENCODER_MODE_HDR: return "KTX_PACK_ASTC_ENCODER_MODE_HDR";
		}
		return "[Unknown KtxPackAstcEncoderMode]";
	}

}
