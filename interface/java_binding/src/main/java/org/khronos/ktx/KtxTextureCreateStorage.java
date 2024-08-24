/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Enum for requesting, or not, allocation of storage for images.<br>
 * <br>
 * These are passed to the {@link KtxTexture1#create(KtxTextureCreateInfo, int)}
 * and {@link KtxTexture2#create(KtxTextureCreateInfo, int)} methods.
 */
public class KtxTextureCreateStorage {
	/**
	 * Don't allocate any image storage
	 */
    public static final int NO_STORAGE = 0;

    /**
     * Allocate image storage.
     */
    public static final int ALLOC_STORAGE = 1;

	/**
	 * Returns a string representation of the given storage mode
	 *
	 * @param n The storage mode
	 * @return A string representation of the given storage mode
	 */
	public static String stringFor(int n) {
		switch (n) {
		case NO_STORAGE:
			return "NO_STORAGE";
		case ALLOC_STORAGE:
			return "ALLOC_STORAGE";
		}
		return "[Unknown KtxTextureCreateStorage]";
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxTextureCreateStorage() {
		// Prevent instantiation
	}
}
