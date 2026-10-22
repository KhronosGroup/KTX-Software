/*
 * Copyright (c) 2026, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Options specifying BCn compression format.<br>
 * <br>
 * These constants can be passed to
 * {@link KtxBCnParams#setBCn(int)}.
 */
public class KtxBCnCompression {

  /**
   * None.
   */
  public static final int NONE = 0;

  /**
   * BC1 compression (RGB). Encodes a 4x4 RGB LDR block into 8 bytes.
   */
  public static final int BC1 = 1;

  /**
   * BC1 compression. Encodes a 4x4 RGBA LDR block into 8 bytes. Alpha is
   * encoded just using 1 bit (i.e., fully opaque or fully transparent).
   */
  public static final int BC1A = 2;

  /**
   * BC2 compression. Encodes a 4x4 RGBA LDR block into 16 bytes. RGB block is
   * encoded using BC1 into 8 bytes. Alpha is encoded into 8 bytes by directly
   * storing the 16 alpha values into 8 bytes (i.e., each alpha value is stored
   * using just 4 bits).
   */
  public static final int BC2 = 3;

  /**
   * BC3 compression. Encodes a 4x4 RGBA LDR block into 16 bytes. RGB block is
   * encoded using BC1 into 8 bytes. Alpha is encoded into 8 bytes using two
   * reference alpha points and 16 3-bit alpha indices for interpolation.
   */
  public static final int BC3 = 4;

  /**
   * BC4 compression. Encodes a 4x4 R LDR block into 8 bytes.
   */
  public static final int BC4 = 5;

  /**
   * BC5 compression. Encodes a 4x4 RG LDR block into 16 bytes. Each channel is
   * encoded separately (ideal for 2-channel, non-color data. E.g., normal
   * maps).
   */
  public static final int BC5 = 6;

  /**
   * BC6HU compression. Encodes a 4x4 RGB HDR unsigned block into 16 bytes.
   */
  public static final int BC6HU = 7;

  /**
   * BC6HS compression. Encodes a 4x4 RGB HDR signed block into 16 bytes.
   */
  public static final int BC6HS = 8;

  /**
   * BC7 compression. Encodes a 4x4 RGBA LDR block into 16 bytes.
   */
  public static final int BC7 = 9;

  /**
   * Returns a string representation of the given BCn format
   *
   * @param n The BCn format
   * @return A string representation of the given BCn format
   */
  public static String stringFor(int n) {
    switch (n) {
    case NONE: return "NONE";
    case BC1: return "BC1";
    case BC1A: return "BC1A";
    case BC2: return "BC2";
    case BC3: return "BC3";
    case BC4: return "BC4";
    case BC5: return "BC5";
    case BC6HU: return "BC6HU";
    case BC6HS: return "BC6HS";
    case BC7: return "BC7";
    }
    return "[Unknown KtxBCnCompression]";
  }

  /**
   * Private constructor to prevent instantiation
   */
  private KtxBCnCompression() {
    // Prevent instantiation
  }

}
