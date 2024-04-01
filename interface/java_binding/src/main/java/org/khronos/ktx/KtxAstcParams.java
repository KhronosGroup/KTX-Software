/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Structure for passing extended parameters to
 * {@link KtxTexture2#compressAstcEx(KtxAstcParams)}.<br>
 * <br>
 * The default settings of an uninitialized structure will be:<br>
 * <br>
 * The <code>blockDimension</code> will be
 * {@link KtxPackAstcBlockDimension#KTX_PACK_ASTC_BLOCK_DIMENSION_D4x4}<br>
 * The <code>mode</code> will be
 * {@link KtxPackAstcEncoderMode#KTX_PACK_ASTC_ENCODER_MODE_LDR}<br>
 * The <code>qualityLevel</code> will be
 * {@link KtxPackAstcQualityLevel#KTX_PACK_ASTC_QUALITY_LEVEL_FASTEST}<br>
 * <br>
 * Setting the <code>qualityLevel</code> to
 * {@link KtxPackAstcQualityLevel#KTX_PACK_ASTC_QUALITY_LEVEL_MEDIUM} is
 * recommended.
 */
public class KtxAstcParams {

	/**
	 * If <code>true</code>, prints ASTC encoder operation details to the standard
	 * output. Not recommended for GUI apps.
	 */
	private boolean verbose;

	/**
	 * Number of threads used for compression. Default is 1.
	 */
	private int threadCount;

	/**
	 * Combinations of block dimensions that astcenc supports, given as a
	 * {@link KtxPackAstcBlockDimension} constant
	 */
	private int blockDimension;

	/**
	 * The {@link KtxPackAstcEncoderMode} constant
	 */
	private int mode;

	/**
	 * The quality level for the ASTC encoder
	 */
	private int qualityLevel;

	/**
	 * Whether to optimize for normal maps
	 */
	private boolean normalMap;

	/**
	 * Whether to optimize for perceptual error
	 */
	private boolean perceptual;

	/**
	 * The input swizzle to apply
	 */
	private char[] inputSwizzle = new char[4];

	/**
	 * Returns whether the ASTC encoder prints operation details to the standard
	 * output
	 *
	 * @return The setting
	 */
	public boolean isVerbose() {
		return verbose;
	}

	/**
	 * If <code>true</code>, prints ASTC encoder operation details to the standard
	 * output. Not recommended for GUI apps.
	 *
	 * @param verbose The setting
	 */
	public void setVerbose(boolean verbose) {
		this.verbose = verbose;
	}

	/**
	 * Returns the number of threads used for compression.
	 *
	 * @return The number of threads
	 */
	public int getThreadCount() {
		return threadCount;
	}

	/**
	 * Set the number of threads used for compression. Default is 1.
	 *
	 * @param threadCount The number of threads
	 */
	public void setThreadCount(int threadCount) {
		this.threadCount = threadCount;
	}

	/**
	 * Returns the current ASTC encoding block dimensions constant, as a value of
	 * {@link KtxPackAstcBlockDimension}.
	 *
	 * @return The block dimension constant
	 */
	public int getBlockDimension() {
		return blockDimension;
	}

	/**
	 * Set the ASTC block dimension constant, as a value of
	 * {@link KtxPackAstcBlockDimension}.
	 *
	 * @param blockDimension The block dimension constant
	 */
	public void setBlockDimension(int blockDimension) {
		this.blockDimension = blockDimension;
	}

	/**
	 * Returns the {@link KtxPackAstcEncoderMode} constant
	 *
	 * @return The encoder mode constant
	 */
	public int getMode() {
		return mode;
	}

	/**
	 * Set the {@link KtxPackAstcEncoderMode} constant
	 *
	 * @param mode The encoder mode constant
	 */
	public void setMode(int mode) {
		this.mode = mode;
	}

	/**
	 * Returns the {@link KtxPackAstcQualityLevel} constant
	 *
	 * @return The quality level constant
	 */
	public int getQualityLevel() {
		return qualityLevel;
	}

	/**
	 * Set the {@link KtxPackAstcQualityLevel} constant.<br>
	 * <br>
	 * Note that while this appears to be a simple integer value in the range [0,
	 * 100], it does not seem to be guaranteed that all values are supposed. Only
	 * the constants from the {@link KtxPackAstcQualityLevel} are guaranteed to be
	 * supported.
	 *
	 * @param qualityLevel The quality level
	 */
	public void setQualityLevel(int qualityLevel) {
		this.qualityLevel = qualityLevel;
	}

	/**
	 * Returns whether the encoder is tuned for normal maps.
	 *
	 * See {@link #setNormalMap(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isNormalMap() {
		return normalMap;
	}

	/**
	 * Set whether the encoder should be tuned for normal maps.<br>
	 * <br>
	 * This tunes codec parameters for better quality on normal maps. In this mode
	 * normals are compressed to X,Y components, discarding Z component. The reader
	 * will need to generate Z component in shaders.
	 *
	 * @param normalMap The setting
	 */
	public void setNormalMap(boolean normalMap) {
		this.normalMap = normalMap;
	}

	/**
	 * Returns whether the encoder optimizes for perceptual error.
	 *
	 * See {@link #setPerceptual(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isPerceptual() {
		return perceptual;
	}

	/**
	 * Set whether the encoder optimizes for perceptual error.<br>
	 * <br>
	 * When this is <code>true</code>, then the codec should optimize for perceptual
	 * error, instead of direct RMS error. This aims to improve perceived image
	 * quality, but typically lowers the measured PSNR score. Perceptual methods are
	 * currently only available for normal maps and RGB color data.
	 *
	 * @param perceptual
	 */
	public void setPerceptual(boolean perceptual) {
		this.perceptual = perceptual;
	}

	/**
	 * Returns the swizzle that is applied to the input.<br>
	 * <br>
	 * See {@link #setInputSwizzle(char[])}
	 *
	 * @return The swizzle
	 */
	public char[] getInputSwizzle() {
		return inputSwizzle;
	}

	/**
	 * Set the swizzle that should be applied to the input.<br>
	 * <br>
	 * This swizzle must match the regular expression /^[rgba01]{4}$/.<br>
	 * <br>
	 * When the given swizzle is <code>null</code>, then no swizzling will be
	 * applied to the input.
	 *
	 * @param inputSwizzle The swizzle
	 */
	public void setInputSwizzle(char[] inputSwizzle) {
		if (inputSwizzle == null) {
			this.inputSwizzle = new char[4];
			return;
		}
		if (inputSwizzle.length != 4) {
			throw new IllegalArgumentException("The inputSwizzle must contain 4 characters");
		}
		String valid = "rgba01";
		for (int i = 0; i < inputSwizzle.length; i++) {
			char c = inputSwizzle[i];
			if (valid.indexOf(c) == -1) {
				throw new IllegalArgumentException("The inputSwizzle may only consist of 'rgba01', but contains " + c);
			}
		}
		this.inputSwizzle = inputSwizzle;
	}
}
