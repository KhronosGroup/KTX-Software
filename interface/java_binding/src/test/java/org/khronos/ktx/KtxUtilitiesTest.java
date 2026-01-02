/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.io.IOException;

import org.junit.jupiter.api.Test;

public class KtxUtilitiesTest {
    @Test
    public void testValidSwizzle() {
	char swizzle[] = new char[] { 'a', '1', 'r', '0' };
	char expected[] = swizzle;
	char actual[] = KtxUtilities.validateSwizzle(swizzle);
	assertArrayEquals(expected, actual, "Accepts valid swizzle and returns it");
    }

    @Test
    public void testNullSwizzle() {
	char expected[] = new char[] { 0, 0, 0, 0 };
	char actual[] = KtxUtilities.validateSwizzle(null);
	assertArrayEquals(expected, actual, "Accepts null swizzle (to apply no swizzle), and returns a default");
    }

    @Test
    public void testDefaultSwizzle() {
	char swizzle[] = new char[] { 0, 0, 0, 0 };
	char expected[] = swizzle;
	char actual[] = KtxUtilities.validateSwizzle(swizzle);
	assertArrayEquals(expected, actual, "Accepts default swizzle (all zeros)");
    }

    @Test
    public void testInvalidSwizzleLength() throws IOException {
	char swizzle[] = new char[] { 'a', 'b', 'r', 'g', 'r', 'g' };
	assertThrows(IllegalArgumentException.class, () -> KtxUtilities.validateSwizzle(swizzle),
		"Swizzle length != 4 expected to throw IllegalArgumentException");
    }

    @Test
    public void testInvalidSwizzleChar() throws IOException {

	char swizzle[] = new char[] { 'a', 'b', 'X', 'g' };
	assertThrows(IllegalArgumentException.class, () -> KtxUtilities.validateSwizzle(swizzle),
		"Invalid swizzle character expected to throw IllegalArgumentException");
    }
}
