/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

import java.util.Arrays;

/**
 * Package-private utility methods
 */
class KtxUtilities {

    /**
     * Validates the given value as an input swizzle to be set as
     * {@link KtxBasisParams#setInputSwizzle(char[])} or
     * {@link KtxAstcParams#setInputSwizzle(char[])}.<br>
     * <br>
     * When the given swizzle is <code>null</code>, then a default swizzle is
     * returned, which is a 4-element <code>char</code> array with all elements
     * being 0. <br>
     * <br>
     * Otherwise, if the given array does not have a length of 4, then an
     * <code>IllegalArgumentException</code> is thrown.<br>
     * <br>
     * If the given swizzle is equal to the default swizzle, then it is returned
     * directly. <br>
     * <br>
     * Otherwise, this swizzle must match the regular expression
     * <code>/^[rgba01]{4}$/</code>.<br>
     * 
     * @param inputSwizzle The input swizzle
     * @return The validated input swizzle, or a new default swizzle if the input
     *         was <code>null</code>
     * @throws IllegalArgumentException If the given swizzle is not valid
     */
    static char[] validateSwizzle(char inputSwizzle[]) {
	char defaultSwizzle[] = new char[4];
	if (inputSwizzle == null) {
	    return defaultSwizzle;
	}
	if (inputSwizzle.length != 4) {
	    throw new IllegalArgumentException("The inputSwizzle must contain 4 characters");
	}
	if (Arrays.equals(inputSwizzle, defaultSwizzle)) {
	    return inputSwizzle;
	}
	String valid = "rgba01";
	for (int i = 0; i < inputSwizzle.length; i++) {
	    char c = inputSwizzle[i];
	    if (valid.indexOf(c) == -1) {
		throw new IllegalArgumentException("The inputSwizzle may only consist of 'rgba01', but contains " + c);
	    }
	}
	return inputSwizzle;

    }

    /**
     * Private constructor to prevent instantiation
     */
    private KtxUtilities() {
	// Private constructor to prevent instantiation
    }
}
