/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Flags for requesting services during creation.<br>
 * <br>
 * These are passed to the <code>createFrom...</code> functions:<br>
 * {@link KtxTexture2#createFromNamedFile(String, int)}<br>
 * {@link KtxTexture2#createFromMemory(java.nio.ByteBuffer, int)}<br>
 * {@link KtxTexture1#createFromNamedFile(String, int)}<br>
 *
 */
public class KtxTextureCreateFlagBits {

	/**
	 * No flags set
	 */
	public static final int KTX_TEXTURE_CREATE_NO_FLAGS = 0x00;

	/**
	 * Load the images from the KTX source
	 */
	public static final int KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT = 0x01;

	/**
	 * Load the raw key-value data instead of creating a ktxHashList from it.
	 */
	public static final int KTX_TEXTURE_CREATE_RAW_KVDATA_BIT = 0x02;

	/**
	 * Skip any key-value data. This overrides the KTX_TEXTURE_CREATE_RAW_KVDATA_BIT.
	 */
	public static final int KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT = 0x04;

	/**
	 * Returns a string representation of the given flag bits
	 *
	 * @param n The flag bits
	 * @return A string representation of the given flag bits
	 */
	public static String stringFor(int n) {
		if (n == 0)
		{
			return "KTX_TEXTURE_CREATE_NO_FLAGS";
		}
		StringBuilder sb = new StringBuilder();
		if ((n & KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT");
		}
		if ((n & KTX_TEXTURE_CREATE_RAW_KVDATA_BIT) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("KTX_TEXTURE_CREATE_RAW_KVDATA_BIT");
		}
		if ((n & KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT) != 0) {
			if (sb.length() != 0) {
				sb.append("|");
			}
			sb.append("KTX_TEXTURE_CREATE_SKIP_KVDATA_BIT");
		}
		if (sb.length() == 0)
		{
			return "[Unknown KtxTextureCreateFlagBits]";
		}
		return sb.toString();
	}

}
