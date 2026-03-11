/*
 * Copyright (c) 2026, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

/**
 * Options specifying basis codec.<br>
 * <br>
 * These constants can be passed to the {@link KtxBasisParams#setCodec(int)}
 * function.
 */
public class KtxBasisCodec {
    /**
     * None
     */
    public static final int NONE = 0;

    /**
     * BasisLZ
     */
    public static final int ETC1S = 1;

    /**
     * UASTC
     */
    public static final int UASTC_LDR = 2;

    /**
     * UASTC_HDR_4X4
     */
    public static final int UASTC_HDR_4X4 = 3;

    /**
     * UASTC_HDR_6X6i
     */
    public static final int UASTC_HDR_6X6_INTERMEDIATE = 4;

    /**
     * Returns a string representation of the given basis codec
     *
     * @param n The basis codec
     * @return A string representation of the given basis codec
     */
    public static String stringFor(int n) {
	switch (n) {
	case NONE:
	    return "NONE";
	case ETC1S:
	    return "ETC1S";
	case UASTC_LDR:
	    return "UASTC_LDR";
	case UASTC_HDR_4X4:
	    return "UASTC_HDR_4X4";
	case UASTC_HDR_6X6_INTERMEDIATE:
	    return "UASTC_HDR_6X6_INTERMEDIATE";
	}
	return "[Unknown KtxBasisCodec]";
    }

    /**
     * Private constructor to prevent instantiation
     */
    private KtxBasisCodec() {
	// Prevent instantiation
    }

}
