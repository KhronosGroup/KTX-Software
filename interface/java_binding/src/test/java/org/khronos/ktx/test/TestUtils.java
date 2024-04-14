/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx.test;

import java.util.Locale;

/**
 * Utilities for the test package
 */
class TestUtils {

	/**
	 * Fill the specified range of rows of the given RGBA pixels array with the
	 * given RGBA components
	 * 
	 * @param rgba   The RGBA pixels array
	 * @param sizeX  The size of the image in x-direction
	 * @param sizeY  The size of the image in y-direction
	 * @param minRow The minimum row, inclusive
	 * @param maxRow The maximum row, exclusive
	 * @param r      The red component (in [0,255])
	 * @param g      The green component (in [0,255])
	 * @param b      The blue component (in [0,255])
	 * @param a      The alpha component (in [0,255])
	 */
	static void fillRows(byte rgba[], int sizeX, int sizeY, 
			int minRow, int maxRow, 
			int r, int g, int b, int a) {
		for (int y = minRow; y < maxRow; y++) {
			for (int x = 0; x < sizeX; x++) {
				int index = (y * sizeX) + x;
				rgba[index * 4 + 0] = (byte) r;
				rgba[index * 4 + 1] = (byte) g;
				rgba[index * 4 + 2] = (byte) b;
				rgba[index * 4 + 3] = (byte) a;
			}
		}
	}

	/**
	 * Create a string representation of the RGBA components of the specified pixel
	 * in the given RGBA pixels array.
	 * 
	 * This is mainly intended for debugging. Some details of the resulting string
	 * are not specified.
	 * 
	 * @param rgba       The RGBA pixels array
	 * @param pixelIndex The pixel index
	 * @return The string
	 */
	static String createRgbaString(byte rgba[], int pixelIndex) {
		byte r = rgba[pixelIndex * 4 + 0];
		byte g = rgba[pixelIndex * 4 + 1];
		byte b = rgba[pixelIndex * 4 + 2];
		byte a = rgba[pixelIndex * 4 + 3];
		int ir = Byte.toUnsignedInt(r);
		int ig = Byte.toUnsignedInt(g);
		int ib = Byte.toUnsignedInt(b);
		int ia = Byte.toUnsignedInt(a);
		String s = String.format(Locale.ENGLISH, "%3d, %3d, %3d, %3d", ir, ig, ib, ia);
		return s;

	}

}
