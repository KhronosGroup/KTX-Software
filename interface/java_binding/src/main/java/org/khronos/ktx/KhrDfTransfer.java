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
 * {@link #KHR_DF_TRANSFER_UNSPECIFIED}.<br>
 * <br>
 * These constants are returned by {@link KtxTexture2#getOETF()}.
 */
public class KhrDfTransfer {
	/**
	 * No transfer function defined
	 */
	public static final int KHR_DF_TRANSFER_UNSPECIFIED = 0;

	/**
	 * Linear transfer function (value proportional to intensity)
	 */
	public static final int KHR_DF_TRANSFER_LINEAR = 1;

	/**
	 * Perceptually-linear transfer function of sRGH (~2.4)
	 */
	public static final int KHR_DF_TRANSFER_SRGB = 2;

	/**
	 * Perceptually-linear transfer function of ITU BT.601, BT.709 and BT.2020
	 * (~1/.45)
	 */
	public static final int KHR_DF_TRANSFER_ITU = 3;

	/**
	 * SMTPE170M (digital NTSC) defines an alias for the ITU transfer function
	 * (~1/.45)
	 */
	public static final int KHR_DF_TRANSFER_SMTPE170M = 3;

	/**
	 * Perceptually-linear gamma function of original NTSC (simple 2.2 gamma)
	 */
	public static final int KHR_DF_TRANSFER_NTSC = 4;

	/**
	 * Sony S-log used by Sony video cameras
	 */
	public static final int KHR_DF_TRANSFER_SLOG = 5;

	/**
	 * Sony S-log 2 used by Sony video cameras
	 */
	public static final int KHR_DF_TRANSFER_SLOG2 = 6;

	/**
	 * ITU BT.1886 EOTF
	 */
	public static final int KHR_DF_TRANSFER_BT1886 = 7;

	/**
	 * ITU BT.2100 HLG OETF
	 */
	public static final int KHR_DF_TRANSFER_HLG_OETF = 8;

	/**
	 * ITU BT.2100 HLG EOTF
	 */
	public static final int KHR_DF_TRANSFER_HLG_EOTF = 9;

	/**
	 * ITU BT.2100 PQ EOTF
	 */
	public static final int KHR_DF_TRANSFER_PQ_EOTF = 10;

	/**
	 * ITU BT.2100 PQ OETF
	 */
	public static final int KHR_DF_TRANSFER_PQ_OETF = 11;

	/**
	 * DCI P3 transfer function
	 */
	public static final int KHR_DF_TRANSFER_DCIP3 = 12;

	/**
	 * Legacy PAL OETF
	 */
	public static final int KHR_DF_TRANSFER_PAL_OETF = 13;

	/**
	 * Legacy PAL 625-line EOTF
	 */
	public static final int KHR_DF_TRANSFER_PAL625_EOTF = 14;

	/**
	 * Legacy ST240 transfer function
	 */
	public static final int KHR_DF_TRANSFER_ST240 = 15;

	/**
	 * ACEScc transfer function
	 */
	public static final int KHR_DF_TRANSFER_ACESCC = 16;

	/**
	 * ACEScct transfer function
	 */
	public static final int KHR_DF_TRANSFER_ACESCCT = 17;

	/**
	 * Adobe RGB (1998) transfer function
	 */
	public static final int KHR_DF_TRANSFER_ADOBERGB = 18;

	/**
	 * Returns a string representation of the given transfer function
	 *
	 * @param n The transfer function
	 * @return A string representation of the given transfer function
	 */
	public static String stringFor(int n) {
		switch (n) {
		case KHR_DF_TRANSFER_UNSPECIFIED:
			return "KHR_DF_TRANSFER_UNSPECIFIED";
		case KHR_DF_TRANSFER_LINEAR:
			return "KHR_DF_TRANSFER_LINEAR";
		case KHR_DF_TRANSFER_SRGB:
			return "KHR_DF_TRANSFER_SRGB";
		case KHR_DF_TRANSFER_SMTPE170M:
			return "KHR_DF_TRANSFER_[ITU/SMTPE170M]";
		case KHR_DF_TRANSFER_NTSC:
			return "KHR_DF_TRANSFER_NTSC";
		case KHR_DF_TRANSFER_SLOG:
			return "KHR_DF_TRANSFER_SLOG";
		case KHR_DF_TRANSFER_SLOG2:
			return "KHR_DF_TRANSFER_SLOG2";
		case KHR_DF_TRANSFER_BT1886:
			return "KHR_DF_TRANSFER_BT1886";
		case KHR_DF_TRANSFER_HLG_OETF:
			return "KHR_DF_TRANSFER_HLG_OETF";
		case KHR_DF_TRANSFER_HLG_EOTF:
			return "KHR_DF_TRANSFER_HLG_EOTF";
		case KHR_DF_TRANSFER_PQ_EOTF:
			return "KHR_DF_TRANSFER_PQ_EOTF";
		case KHR_DF_TRANSFER_PQ_OETF:
			return "KHR_DF_TRANSFER_PQ_OETF";
		case KHR_DF_TRANSFER_DCIP3:
			return "KHR_DF_TRANSFER_DCIP3";
		case KHR_DF_TRANSFER_PAL_OETF:
			return "KHR_DF_TRANSFER_PAL_OETF";
		case KHR_DF_TRANSFER_PAL625_EOTF:
			return "KHR_DF_TRANSFER_PAL625_EOTF";
		case KHR_DF_TRANSFER_ST240:
			return "KHR_DF_TRANSFER_ST240";
		case KHR_DF_TRANSFER_ACESCC:
			return "KHR_DF_TRANSFER_ACESCC";
		case KHR_DF_TRANSFER_ACESCCT:
			return "KHR_DF_TRANSFER_ACESCCT";
		case KHR_DF_TRANSFER_ADOBERGB:
			return "KHR_DF_TRANSFER_ADOBERGB";
		}
		return "[Unknown KhrDfTransfer]";
	}


}
