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
    public static final int NONE = 0;

    /**
     * Basis LZ supercompression
     */
    public static final int BASIS_LZ = 1;

    /**
     * ZStd supercompression
     */
    public static final int ZSTD = 2;

    /**
     * ZLIB supercompression
     */
    public static final int ZLIB = 3;

	/**
	 * Returns a string representation of the given supercompression scheme
	 *
	 * @param n The supercompression scheme
	 * @return A string representation of the given supercompression scheme
	 */
	public static String stringFor(int n) {
		switch (n) {
		case NONE:
			return "NONE";
		case BASIS_LZ:
			return "BASIS_LZ";
		case ZSTD:
			return "ZSTD";
		case ZLIB:
			return "ZLIB";
		}
		return "[Unknown KtxSupercmpScheme]";
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxSupercmpScheme() {
		// Prevent instantiation
	}


}
