/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Enumerators identifying the supercompression scheme.<br>
 * <br>
 * These are the values that are returned by
 * {@link KtxTexture2#getSupercompressionScheme()}.
 */
public class KtxSupercmpScheme {

    /**
     * No supercompression
     */
    public static final int KTX_SS_NONE = 0;

    /**
     * Basis LZ supercompression
     */
    public static final int KTX_SS_BASIS_LZ = 1;

    /**
     * ZStd supercompression
     */
    public static final int KTX_SS_ZSTD = 2;

    /**
     * ZLIB supercompression
     */
    public static final int KTX_SS_ZLIB = 3;

	/**
	 * Returns a string representation of the given supercompression scheme
	 *
	 * @param n The supercompression scheme
	 * @return A string representation of the given supercompression scheme
	 */
	public static String stringFor(int n) {
		switch (n) {
		case KTX_SS_NONE:
			return "KTX_SS_NONE";
		case KTX_SS_BASIS_LZ:
			return "KTX_SS_BASIS_LZ";
		case KTX_SS_ZSTD:
			return "KTX_SS_ZSTD";
		case KTX_SS_ZLIB:
			return "KTX_SS_ZLIB";
		}
		return "[Unknown KtxSupercmpScheme]";
	}

}
