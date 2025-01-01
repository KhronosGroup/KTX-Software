/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;

import org.junit.jupiter.api.Test;
import org.khronos.ktx.KtxBasisParams;

public class KtxBasisParamsTest {
    @Test
    public void testValidSwizzle() {
	KtxBasisParams p = new KtxBasisParams();
	p.setInputSwizzle(new char[] { 'a', '1', 'r', '0' });
	assertTrue(true, "Accepts valid swizzle");
    }

    @Test
    public void testNullSwizzle() {
	KtxBasisParams p = new KtxBasisParams();
	p.setInputSwizzle(null);
	assertTrue(true, "Accepts null swizzle (to apply no swizzle)");
    }

    @Test
    public void testDefaultSwizzle() {
	KtxBasisParams p = new KtxBasisParams();
	p.setInputSwizzle(new char[4]);
	assertTrue(true, "Accepts default swizzle (all zeros)");
    }

    @Test
    public void testInvalidSwizzleLength() throws IOException {
	KtxBasisParams p = new KtxBasisParams();
	assertThrows(IllegalArgumentException.class,
		() -> p.setInputSwizzle(new char[] { 'a', 'b', 'r', 'g', 'r', 'g' }),
		"Swizzle length != 4 expected to throw IllegalArgumentException");
    }

    @Test
    public void testInvalidSwizzleChar() throws IOException {

	KtxBasisParams p = new KtxBasisParams();
	assertThrows(IllegalArgumentException.class, () -> p.setInputSwizzle(new char[] { 'a', 'b', 'X', 'g' }),
		"Invalid swizzle character expected to throw IllegalArgumentException");
    }
}
