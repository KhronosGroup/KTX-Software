/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Flags guiding transcoding of Basis Universal compressed textures.<br>
 * <br>
 * These are passed to {@link KtxTexture2#transcodeBasis(int, int)}.
 */
public class KtxTranscodeFlagBits {

	/**
	 * PVRTC1: decode non-pow2 ETC1S texture level to the next larger
	 * power of 2 (not implemented yet, but we're going to support it).
	 * Ignored if the slice's dimensions are already a power of 2.
	 */
	public static final int PVRTC_DECODE_TO_NEXT_POW2 = 2;

	/**
	 * When decoding to an opaque texture format, if the Basis data has
	 * alpha, decode the alpha slice instead of the color slice to the
	 * output texture format. Has no effect if there is no alpha data.
	 */
	public static final int TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS = 4;

	/**
	 * Request higher quality transcode of UASTC to BC1, BC3, ETC2_EAC_R11 and
	 * ETC2_EAC_RG11. The flag is unused by other UASTC transcoders.
	 */
	public static final int HIGH_QUALITY = 32;

	/**
	 * Returns a string representation of the given flag bits
	 *
	 * @param n The flag bits
	 * @return A string representation of the given flag bits
	 */
	public static String stringFor(int n) {
		if (n == 0)
		{
			return "(none)";
		}
		StringBuilder sb = new StringBuilder();
		if ((n & PVRTC_DECODE_TO_NEXT_POW2) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("PVRTC_DECODE_TO_NEXT_POW2");
		}
		if ((n & TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS");
		}
		if ((n & HIGH_QUALITY) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("HIGH_QUALITY");
		}
		if (sb.length() == 0)
		{
			return "[Unknown KtxTranscodeFlagBits]";
		}
		return sb.toString();
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxTranscodeFlagBits() {
		// Prevent instantiation
	}


}
