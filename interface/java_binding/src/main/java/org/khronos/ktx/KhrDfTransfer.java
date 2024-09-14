/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

/**
 * Definition of the optical to digital transfer function ("gamma correction").<br>
 * <br>
 * Most transfer functions are not a pure power function and also include a
 * linear element. LAB and related absolute color representations should use
 * {@link #UNSPECIFIED}.<br>
 * <br>
 * These constants are returned by {@link KtxTexture2#getOETF()}.
 */
public class KhrDfTransfer {
	/**
	 * No transfer function defined
	 */
	public static final int UNSPECIFIED = 0;

	/**
	 * Linear transfer function (value proportional to intensity)
	 */
	public static final int LINEAR = 1;

	/**
	 * Perceptually-linear transfer function of sRGH (~2.4)
	 */
	public static final int SRGB = 2;

	/**
	 * Perceptually-linear transfer function of ITU BT.601, BT.709 and BT.2020
	 * (~1/.45)
	 */
	public static final int ITU = 3;

	/**
	 * SMTPE170M (digital NTSC) defines an alias for the ITU transfer function
	 * (~1/.45)
	 */
	public static final int SMTPE170M = 3;

	/**
	 * Perceptually-linear gamma function of original NTSC (simple 2.2 gamma)
	 */
	public static final int NTSC = 4;

	/**
	 * Sony S-log used by Sony video cameras
	 */
	public static final int SLOG = 5;

	/**
	 * Sony S-log 2 used by Sony video cameras
	 */
	public static final int SLOG2 = 6;

	/**
	 * ITU BT.1886 EOTF
	 */
	public static final int BT1886 = 7;

	/**
	 * ITU BT.2100 HLG OETF
	 */
	public static final int HLG_OETF = 8;

	/**
	 * ITU BT.2100 HLG EOTF
	 */
	public static final int HLG_EOTF = 9;

	/**
	 * ITU BT.2100 PQ EOTF
	 */
	public static final int PQ_EOTF = 10;

	/**
	 * ITU BT.2100 PQ OETF
	 */
	public static final int PQ_OETF = 11;

	/**
	 * DCI P3 transfer function
	 */
	public static final int DCIP3 = 12;

	/**
	 * Legacy PAL OETF
	 */
	public static final int PAL_OETF = 13;

	/**
	 * Legacy PAL 625-line EOTF
	 */
	public static final int PAL625_EOTF = 14;

	/**
	 * Legacy ST240 transfer function
	 */
	public static final int ST240 = 15;

	/**
	 * ACEScc transfer function
	 */
	public static final int ACESCC = 16;

	/**
	 * ACEScct transfer function
	 */
	public static final int ACESCCT = 17;

	/**
	 * Adobe RGB (1998) transfer function
	 */
	public static final int ADOBERGB = 18;

	/**
	 * Returns a string representation of the given transfer function
	 *
	 * @param n The transfer function
	 * @return A string representation of the given transfer function
	 */
	public static String stringFor(int n) {
		switch (n) {
		case UNSPECIFIED:
			return "UNSPECIFIED";
		case LINEAR:
			return "LINEAR";
		case SRGB:
			return "SRGB";
		case SMTPE170M:
			return "[ITU/SMTPE170M]";
		case NTSC:
			return "NTSC";
		case SLOG:
			return "SLOG";
		case SLOG2:
			return "SLOG2";
		case BT1886:
			return "BT1886";
		case HLG_OETF:
			return "HLG_OETF";
		case HLG_EOTF:
			return "HLG_EOTF";
		case PQ_EOTF:
			return "PQ_EOTF";
		case PQ_OETF:
			return "PQ_OETF";
		case DCIP3:
			return "DCIP3";
		case PAL_OETF:
			return "PAL_OETF";
		case PAL625_EOTF:
			return "PAL625_EOTF";
		case ST240:
			return "ST240";
		case ACESCC:
			return "ACESCC";
		case ACESCCT:
			return "ACESCCT";
		case ADOBERGB:
			return "ADOBERGB";
		}
		return "[Unknown KhrDfTransfer]";
	}

	/**
	 * Private constructor to prevent instantiation
	 */
	private KhrDfTransfer() {
		// Prevent instantiation
	}


}
