/*
 * Copyright (c) 2026, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Options specifying BC1, BC2, BC3, and BC7 encoding quality levels.<br>
 * <br>
 * These constants can be passed to
 * {@link KtxBCnParams#setBCnCompressionQuality(int)}.
 */
public class KtxPackBCnQualityLevels {

  /**
   * Fastest compression. For BC1/BC2/BC3, this maps to 0.
   */
  public static final int FASTEST = 0;

  /**
   * Faster compression. For BC1/BC2/BC3, this maps to 2.
   */
  public static final int FASTER = 1;

  /**
   * Fast compression. For BC1/BC2/BC3, this maps to 5.
   */
  public static final int FAST = 2;

  /**
   * Medium compression. For BC1/BC2/BC3, this maps to 10.
   */
  public static final int MEDIUM = 3;

  /**
   * Thorough compression. For BC1/BC2/BC3, this maps to 15.
   */
  public static final int THOROUGH = 4;

  /**
   * Exhaustive compression. For BC1/BC2/BC3, this maps to 19.
   */
  public static final int EXHAUSTIVE = 5;

  /**
   * Returns a string representation of the given quality level
   *
   * @param n The quality level
   * @return A string representation of the given quality level
   */
  public static String stringFor(int n) {
    switch (n) {
    case FASTEST: return "FASTEST";
    case FASTER: return "FASTER";
    case FAST: return "FAST";
    case MEDIUM: return "MEDIUM";
    case THOROUGH: return "THOROUGH";
    case EXHAUSTIVE: return "EXHAUSTIVE";
    }
    return "[Unknown KtxPackBCnQualityLevel]";
  }

  /**
   * Private constructor to prevent instantiation
   */
  private KtxPackBCnQualityLevels() {
    // Prevent instantiation
  }

}
