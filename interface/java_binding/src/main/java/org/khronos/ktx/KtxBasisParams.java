/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Structure for passing parameters to {@link KtxTexture2#compressBasisEx(KtxBasisParams)}.<br>
 * <br>
 * At a minimum you must initialize the structure as follows:
 * <pre><code>
 * KtxBasisParams params = new KtxBasisParams();
 * params.setCompressionLevel(KtxBasisParams.ETC1S_DEFAULT_COMPRESSION_LEVEL);
 * </code></pre>
 * The <code>compressionLevel</code> has to be explicitly set, because 0 is a valid
 * <code>compressionLevel</code> but is not the default used by the BasisU encoder when
 * no value is set. Only the other settings that are to be non-default must be non-zero.
 */
public class KtxBasisParams {

	/**
	 * The default compression level
	 */
	public static final int ETC1S_DEFAULT_COMPRESSION_LEVEL = 2;

	/**
	 * Flag to indicate which codec to use. 0 - NONE, 1 - ETC1S, 2 - UASTC_LDR, 3 - UASTC_HDR4x4, 4 - UASTC_HDR6x6i.
	 */
	private int codec;

	/**
	 * Whether encoder operations are printed to standard output
	 */
	private boolean verbose;

	/**
	 * Whether the use of the SSE instruction set should be forbidden.
	 */
	private boolean noSSE;

	/**
	 * Number of threads used for the compression.
	 */
	private int threadCount;

	/**
	 * ETC1S compression effort level. 
	 */
	private int etc1sCompressionLevel;

	/**
	 * Compression quality
	 */
	private int qualityLevel;

	/**
	 * The maximum number of color endpoint clusters
	 */
	private int maxEndpoints;

	/**
	 *  The endpoint RDO quality threshold
	 */
	private float endpointRDOThreshold;

	/**
	 * Maximum number of color selector clusters
	 */
	private int maxSelectors;

	/**
	 * The selector RDO threshold
	 */
	private float selectorRDOThreshold;

	/**
	 * The input swizzle
	 */
	private char[] inputSwizzle = new char[4];

	/**
	 * Whether the codec is tuned for better quality on normal maps
	 */
	private boolean normalMap;

	/**
	 * Whether swizzling metadata should be applied before compressing
	 */
	private boolean preSwizzle;

	/**
	 * Disable endpoint rate distortion optimizations
	 */
	private boolean noEndpointRDO;

	/**
	 * Disable selector rate distortion optimizations
	 */
	private boolean noSelectorRDO;

	/**
	 * The flag bits controlling UASTC encoding
	 */
	private int uastcFlags;

	/**
	 * Whether rate distortion optimization (RDO) post-processing is enabled
	 */
	private boolean uastcRDO;

	/**
	 * The UASTC RDO quality scalar
	 */
	private float uastcRDOQualityScalar;

	/**
	 * The UASTC RDO dictionary size in bytes
	 */
	private int uastcRDODictSize;

	/**
	 * The UASTC RDO maximum smooth block error scale
	 */
	private float uastcRDOMaxSmoothBlockErrorScale;

	/**
	 * The UASTC RDO maximum smooth block standard deviation.
	 */
	private float uastcRDOMaxSmoothBlockStdDev;

	/**
	 * Do not favor simpler UASTC modes in RDO mode.
	 */
	private boolean uastcRDODontFavorSimplerModes;

	/**
	 * Disable RDO multithreading.
	 */
	private boolean uastcRDONoMultithreading;

	/**
	 * The UASTC HDR 4x4 compressor's level
	 */
	private int uastcHDRQuality;

	/**
	 * Allow the UASTC HDR 4x4 encoder to try varying the CEM 11 selectors
	 */
	private boolean uastcHDRUberMode;

	/**
	 * Try to find better quantized CEM 7/11 endpoint values (slower)
	 */
	private boolean uastcHDRUltraQuant;

	/**
	 * Whether ASTC HDR 4x4 quality is favored
	 */
	private boolean uastcHDRFavorAstc;

	/**
	 * Whether the input image's gamut is Rec. 2020 vs. the default Rec. 709
	 */
	private boolean rec2020;

	/**
	 * Enables rate distortion optimization
	 */
	private float uastcHDRLambda;
	
	/**
	 * Controls the 6x6 HDR intermediate mode encoder performance
	 */
	private int uastcHDRLevel;
	
	/**
	 * Returns the used codec.
	 *
	 * See {@link #setCodec(int)}
	 *
	 * @return The codec
	 */
	public int getCodec() {
		return codec;
	}

	/**
	 * Set the codec flag.<br>
	 *
	 * @param codec The codec
	 */
	public void setCodec(int codec) {
		this.codec = codec;
	}

	/**
	 * Returns whether encoder operations are printed to standard output.
	 *
	 * See {@link #setVerbose(boolean)}.
	 *
	 * @return The setting
	 */
	public boolean isVerbose() {
		return verbose;
	}

	/**
	 * Set whether Basis Universal encoder operation details are printed to the
	 * standard output. <br>
	 * <br>
	 * Not recommended for GUI apps.
	 *
	 * @param verbose The setting
	 */
	public void setVerbose(boolean verbose) {
		this.verbose = verbose;
	}

	/**
	 * Returns whether the use of the SSE instruction set is forbidden.
	 *
	 * See {@link #setNoSSE(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isNoSSE() {
		return noSSE;
	}

	/**
	 * Set whether the use of the SSE instruction set should be forbidden.<br>
	 * <br>
	 * Ignored if CPU does not support SSE.
	 *
	 * @param noSSE The setting
	 */
	public void setNoSSE(boolean noSSE) {
		this.noSSE = noSSE;
	}

	/**
	 * Returns the number of threads used for compression.
	 *
	 * See {@link #setThreadCount(int)}
	 *
	 * @return The thread count
	 */
	public int getThreadCount() {
		return threadCount;
	}

	/**
	 * Set the number of threads used for compression.<br>
	 * <br>
	 * Default is 1.
	 *
	 * @param threadCount The thread count
	 */
	public void setThreadCount(int threadCount) {
		this.threadCount = threadCount;
	}

	/**
	 * Returns the ETC1S compression effort level.
	 *
	 * See {@link #setEtc1sCompressionLevel(int)}
	 *
	 * @return The compression level
	 */
	public int getEtc1sCompressionLevel() {
		return etc1sCompressionLevel;
	}

	/**
	 * Set the ETC1S compression effort level.<br>
	 * <br>
	 * Range is [0,6]. Higher values are much slower, but give slightly 
	 * higher quality. Higher levels are intended for video. This 
	 * parameter controls numerous internal encoding speed vs. compression 
	 * efficiency/performance tradeoffs. Note this is NOT the same as the 
	 * ETC1S quality level, and most users shouldn't change this. There 
	 * is no default. Callers must explicitly set this value. Callers
	 * can use {@link #ETC1S_DEFAULT_COMPRESSION_LEVEL} as a default value.
	 * Currently this is 2.
	 *
	 * @param etc1sCompressionLevel The compression level
	 */
	public void setEtc1sCompressionLevel(int etc1sCompressionLevel) {
		this.etc1sCompressionLevel = etc1sCompressionLevel;
	}

	/**
	 * Returns the compression quality.
	 *
	 * See {@link #setQualityLevel(int)}
	 *
	 * @return The compression quality
	 */
	public int getQualityLevel() {
		return qualityLevel;
	}

	/**
	 * Set the compression quality.<br>
	 * <br>
	 * The range is [1,255]. Lower gives better compression/lower quality/faster.
	 * Higher gives less compression/higher quality/slower.<br>
	 * <br>
	 * This automatically determines values for <code>maxEndpoints</code>,
	 * <code>maxSelectors</code>, <code>endpointRDOThreshold</code> and
	 * <code>selectorRDOThreshold</code> for the target quality level.
	 * Setting these parameters overrides the values determined by
	 * <code>qualityLevel</code>, which defaults to 128 if neither it nor
	 * both of <code>maxEndpoints</code> and <code>maxSelectors</code> have been set.<br>
	 * <br>
	 * <b>Note:</b><br>
	 * <br>
	 * Both of <code>maxEndpoints</code> and <code>maxSelectors</code> must be
	 * set for them to have any effect. The <code>qualityLevel</code> will only
	 * determine values for <code>endpointRDOThreshold</code> and
	 * <code>selectorRDOThreshold</code> when its value exceeds 128, otherwise
	 * their defaults will be used.
	 *
	 * @param qualityLevel The compression quality
	 */
	public void setQualityLevel(int qualityLevel) {
		this.qualityLevel = qualityLevel;
	}

	/**
	 * Returns the maximum number of color endpoint clusters.
	 *
	 * See {@link #setMaxEndpoints(int)}
	 *
	 * @return The maximum number of endpoints
	 */
	public int getMaxEndpoints() {
		return maxEndpoints;
	}

	/**
	 * Set the maximum number of color endpoint clusters.<br>
	 * <br>
	 * Range is [1,16128]. Default is 0, unset. If this is set, <code>maxSelectors</code>
	 * must also be set, otherwise the value will be ignored.
	 *
	 * @param maxEndpoints The maximum number of color endpoint clusters
	 */
	public void setMaxEndpoints(int maxEndpoints) {
		this.maxEndpoints = maxEndpoints;
	}

	/**
	 * Returns the endpoint RDO threshold.
	 *
	 * See {@link #setEndpointRDOThreshold(float)}
	 *
	 * @return The threshold
	 */
	public float getEndpointRDOThreshold() {
		return endpointRDOThreshold;
	}

	/**
	 * Set the endpoint RDO quality threshold.<br>
	 * <br>
	 * (RDO stands for "rate distortion optimizations")<br>
	 * <br>
	 * The default is 1.25. Lower is higher quality but less quality per
	 * output bit (try [1.0,3.0]). This will override the value
	 * chosen by <code>qualityLevel</code>.
	 *
	 * @param endpointRDOThreshold The threshold
	 */
	public void setEndpointRDOThreshold(float endpointRDOThreshold) {
		this.endpointRDOThreshold = endpointRDOThreshold;
	}

	/**
	 * Returns the maximum number of color selector clusters.
	 *
	 * See {@link #setMaxEndpoints(int)}
	 *
	 * @return The maximum number of color selector clusters
	 */
	public int getMaxSelectors() {
		return maxSelectors;
	}

	/**
	 * Set the maximum number of color selector clusters.<br>
	 * <br>
	 * Range is [1,16128]. Default is 0, unset. If this is set,
	 * <code>maxEndpoints</code> must also be set, otherwise the value will be ignored.
	 *
	 * @param maxSelectors The maximum number of color selector clusters
	 */
	public void setMaxSelectors(int maxSelectors) {
		this.maxSelectors = maxSelectors;
	}

	/**
	 * Returns the selector RDO threshold.
	 *
	 * See {@link #setSelectorRDOThreshold(float)}
	 *
	 * @return The selector RDO threshold
	 */
	public float getSelectorRDOThreshold() {
		return selectorRDOThreshold;
	}

	/**
	 * Set the selector RDO quality threshold.<br>
	 * <br>
	 * (RDO stands for "rate distortion optimizations")<br>
	 * <br>
	 * The default is 1.5. Lower is higher quality but less quality per output
	 * bit (try [1.0,3.0]). This will override the value chosen by
	 * <code>qualityLevel</code>.
	 *
	 * @param selectorRDOThreshold The selector RDO threshold.
	 */
	public void setSelectorRDOThreshold(float selectorRDOThreshold) {
		this.selectorRDOThreshold = selectorRDOThreshold;
	}

	/**
	 * Returns the swizzle that is applied to the input.<br>
	 * <br>
	 * Note that this will never be <code>null</code>. But it may
	 * be an array containing the value <code>0</code> four times,
	 * to indicate that no swizzling should be applied.<br>
	 * <br>
	 * Callers may not modify the returned array.<br>
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
	 * When the given swizzle is <code>null</code> or all its elements are
	 * <code>0</code>, then no swizzling will be applied to the input.<br>
	 * <br>
	 * Otherwise, this swizzle must match the regular expression 
	 * <code>/^[rgba01]{4}$/</code>.<br>
	 * <br>
	 *
	 * @param inputSwizzle The swizzle
	 */
	public void setInputSwizzle(char[] inputSwizzle) {
	    this.inputSwizzle = KtxUtilities.validateSwizzle(inputSwizzle);
	}

	/**
	 * Returns whether the codec is tuned for better quality on normal maps.
	 *
	 * See {@link #setNormalMap(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isNormalMap() {
		return normalMap;
	}

	/**
	 * Set whether the codec should be tuned for better quality on normal maps.<br>
	 * <br>
	 * When this is <code>true</code>, then it will use no selector RDO, no endpoint RDO,
	 * and set the texture's DFD appropriately. Only valid for linear textures.
	 *
	 * @param normalMap The setting
	 */
	public void setNormalMap(boolean normalMap) {
		this.normalMap = normalMap;
	}

	/**
	 * Whether swizzling metadata should be applied before compressing.
	 *
	 * See {@link #setPreSwizzle(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isPreSwizzle() {
		return preSwizzle;
	}

	/**
	 * Set whether swizzling metadata should be applied before compressing.<br>
	 * <br>
	 * If the texture has <code>KTXswizzle</code> metadata, apply it before
	 * compressing. Swizzling, like <code>rabb</code> may yield drastically
	 * different error metrics if done after supercompression.
	 *
	 * @param preSwizzle The setting
	 */
	public void setPreSwizzle(boolean preSwizzle) {
		this.preSwizzle = preSwizzle;
	}

	/**
	 * Returns whether endpoint rate distortion optimizations are disabled.
	 *
	 * See {@link #setNoEndpointRDO(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isNoEndpointRDO() {
		return noEndpointRDO;
	}

	/**
	 * Set whether endpoint rate distortion optimizations should be disabled.<br>
	 * <br>
	 * Setting this to <code>true</code> will yield slightly faster, less noisy
	 * output, but lower quality per output bit. Default is <code>false</code>.
	 *
	 * @param noEndpointRDO The setting
	 */
	public void setNoEndpointRDO(boolean noEndpointRDO) {
		this.noEndpointRDO = noEndpointRDO;
	}

	/**
	 * Returns whether selector rate distortion optimizations are disabled.
	 *
	 * See {@link #setNoSelectorRDO(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isNoSelectorRDO() {
		return noSelectorRDO;
	}

	/**
	 * Set whether selector rate distortion optimizations are disabled.<br>
	 * <br>
	 * Setting this to <code>true</code> will yield slightly faster, less noisy
	 * output, but lower quality per output bit. Default is <code>false</code>.
	 *
	 * @param noSelectorRDO The setting
	 */
	public void setNoSelectorRDO(boolean noSelectorRDO) {
		this.noSelectorRDO = noSelectorRDO;
	}

	/**
	 * Returns the {@link KtxPackUastcFlagBits}
	 *
	 * See {@link #setUastcFlags(int)}
	 *
	 * @return The flag bits
	 */
	public int getUastcFlags() {
		return uastcFlags;
	}

	/**
	 * Set the {@link KtxPackUastcFlagBits}.<br>
	 * <br>
	 * The most important value is the level given in the least-significant 4 bits
	 * of the respective constant, which selects a speed versus quality tradeoff.
	 *
	 * @param uastcFlags The flag bits
	 */
	public void setUastcFlags(int uastcFlags) {
		this.uastcFlags = uastcFlags;
	}

	/**
	 * Returns whether rate distortion optimization (RDO) post-processing is enabled.
	 *
	 * @return The setting
	 */
	public boolean isUastcRDO() {
		return uastcRDO;
	}

	/**
	 * Set whether rate distortion optimization (RDO) post-processing is enabled.
	 *
	 * @param uastcRDO The setting
	 */
	public void setUastcRDO(boolean uastcRDO) {
		this.uastcRDO = uastcRDO;
	}

	/**
	 * Returns the UASTC quality scalar.
	 *
	 * See {@link #setUastcRDOQualityScalar(float)}
	 *
	 * @return The quality scalar
	 */
	public float getUastcRDOQualityScalar() {
		return uastcRDOQualityScalar;
	}

	/**
	 * Set the UASTC quality scalar.
	 *
	 * This sets the quality scalar ("lambda"). Lower values yield higher quality/larger LZ
	 * compressed files, higher values yield lower quality/smaller LZ compressed files.
	 * A good range to try is [.2,4]. Full range is [.001,50.0]. Default is 1.0.
	 *
	 * @param uastcRDOQualityScalar The quality scalar.
	 */
	public void setUastcRDOQualityScalar(float uastcRDOQualityScalar) {
		this.uastcRDOQualityScalar = uastcRDOQualityScalar;
	}

	/**
	 * Returns the UASTC RDO dictionary size in bytes.
	 *
	 * See {@link #setUastcRDODictSize(int)}
	 *
	 * @return The dictionary size
	 */
	public int getUastcRDODictSize() {
		return uastcRDODictSize;
	}

	/**
	 * Set the UASTC RDO dictionary size in bytes. <br>
	 * <br>
	 * The default is 4096. Lower values=faster, but give less
	 * compression. Range is [64,65536].
	 *
	 * @param uastcRDODictSize The dictionary size
	 */
	public void setUastcRDODictSize(int uastcRDODictSize) {
		this.uastcRDODictSize = uastcRDODictSize;
	}

	/**
	 * Returns the UASTC RDO maximum smooth block error scale.
	 *
	 * See {@link #setUastcRDOMaxSmoothBlockErrorScale(float)}
	 *
	 * @return The error scale
	 */
	public float getUastcRDOMaxSmoothBlockErrorScale() {
		return uastcRDOMaxSmoothBlockErrorScale;
	}

	/**
	 * Set the UASTC RDO maximum smooth block error scale.<br>
	 * <br>
	 * Range is [1,300]. Default is 10.0, 1.0 is disabled. Larger values
	 * suppress more artifacts (and allocate more bits) on smooth blocks.
	 *
	 * @param uastcRDOMaxSmoothBlockErrorScale The error scale
	 */
	public void setUastcRDOMaxSmoothBlockErrorScale(float uastcRDOMaxSmoothBlockErrorScale) {
		this.uastcRDOMaxSmoothBlockErrorScale = uastcRDOMaxSmoothBlockErrorScale;
	}

	/**
	 * Returns the UASTC RDO maximum smooth block standard deviation.
	 *
	 * See {@link #setUastcRDOMaxSmoothBlockStdDev(float)}
	 *
	 * @return The standard deviation
	 */
	public float getUastcRDOMaxSmoothBlockStdDev() {
		return uastcRDOMaxSmoothBlockStdDev;
	}

	/**
	 * Set the UASTC RDO maximum smooth block standard deviation.<br>
	 * <br>
	 * Range is [.01,65536.0]. Default is 18.0. Larger values expand the range
	 * of blocks considered smooth.
	 *
	 * @param uastcRDOMaxSmoothBlockStdDev The standard deviation
	 */
	public void setUastcRDOMaxSmoothBlockStdDev(float uastcRDOMaxSmoothBlockStdDev) {
		this.uastcRDOMaxSmoothBlockStdDev = uastcRDOMaxSmoothBlockStdDev;
	}

	/**
	 * Returns whether to <b>not</b> favor simpler UASTC modes in RDO mode.
	 *
	 * @return The setting
	 */
	public boolean isUastcRDODontFavorSimplerModes() {
		return uastcRDODontFavorSimplerModes;
	}

	/**
	 * Set whether to <b>not</b> favor simpler UASTC modes in RDO mode.
	 *
	 * @param uastcRDODontFavorSimplerModes The setting
	 */
	public void setUastcRDODontFavorSimplerModes(boolean uastcRDODontFavorSimplerModes) {
		this.uastcRDODontFavorSimplerModes = uastcRDODontFavorSimplerModes;
	}

	/**
	 * Returns whether UASTC RDO multithreading is disabled.
	 *
	 * See {@link #setUastcRDONoMultithreading(boolean)}
	 *
	 * @return The setting
	 */
	public boolean isUastcRDONoMultithreading() {
		return uastcRDONoMultithreading;
	}

	/**
	 * Set whether RDO multithreading is disabled.<br>
	 * <br>
	 * Setting this to <code>true</code> will yield slightly higher compression,
	 * and be deterministic.
	 *
	 * @param uastcRDONoMultithreading The setting
	 */
	public void setUastcRDONoMultithreading(boolean uastcRDONoMultithreading) {
		this.uastcRDONoMultithreading = uastcRDONoMultithreading;
	}

	/**
	 * Returns the HRD 4x4 compressor level.
	 * 
	 * See {@link #setUastcHDRQuality(int)}
	 * 
	 * @return The setting
	 */
	public int getUastcHDRQuality() {
	    return uastcHDRQuality;
	}

	/**
	 * Set the HRD 4x4 compressor level.<br>
	 * <br>
	 * UASTC HDR 4x4: Sets the UASTC HDR 4x4 compressor's level. Valid 
	 * range is [0,4] - higher=slower but higher quality. HDR default=1.
	 * Level 0=fastest/lowest quality, 3=highest practical setting, 
	 * 4=exhaustive
	 * 
	 * @param uastcHDRQuality The quality
	 */
	public void setUastcHDRQuality(int uastcHDRQuality) {
	    this.uastcHDRQuality = uastcHDRQuality;
	}

	/**
	 * Returns the UASTC uber mode.
	 * 
	 * See {@link #setUastcHDRUberMode(boolean)}
	 * 
	 * @return The setting
	 */
	public boolean isUastcHDRUberMode() {
	    return uastcHDRUberMode;
	}

	/**
	 * Set the UASTC uber mode.<br>
	 * <br>
	 * UASTC HDR 4x4: Allow the UASTC HDR 4x4 encoder to try varying the 
	 * CEM 11 selectors more for slightly higher quality (slower). 
	 * This may negatively impact BC6H quality, however.
	 * 
	 * @param uastcHDRUberMode The setting
	 */
	public void setUastcHDRUberMode(boolean uastcHDRUberMode) {
	    this.uastcHDRUberMode = uastcHDRUberMode;
	}

	/**
	 * Returns the ultra quantization mode.
	 * 
	 * See {@link #setUastcHDRUltraQuant(boolean)}
	 * 
	 * @return The setting
	 */
	public boolean isUastcHDRUltraQuant() {
	    return uastcHDRUltraQuant;
	}

	/**
	 * Set the ultra quantization mode.<br>
	 * <br>
	 * ASTC HDR 4x4: Try to find better quantized CEM 7/11 endpoint values 
	 * (slower)
	 * 
	 * @param uastcHDRUltraQuant The setting
	 */
	public void setUastcHDRUltraQuant(boolean uastcHDRUltraQuant) {
	    this.uastcHDRUltraQuant = uastcHDRUltraQuant;
	}

	/**
	 * Returns whether to favor ASTC HDR 4x4 quality.
	 * 
	 * See {@link #setUastcHDRFavorAstc(boolean)}
	 * 
	 * @return The setting
	 */
	public boolean isUastcHDRFavorAstc() {
	    return uastcHDRFavorAstc;
	}

	/**
	 * Set whether to favor ASTC HDR 4x4 quality.<br>
	 * <br>
	 * UASTC HDR 4x4: By default the UASTC HDR 4x4 encoder tries to 
	 * strike a balance or even slightly favor BC6H quality. If this 
	 * option is specified, ASTC HDR 4x4 quality is favored instead.
	 * 
	 * @param uastcHDRFavorAstc The setting
	 */
	public void setUastcHDRFavorAstc(boolean uastcHDRFavorAstc) {
	    this.uastcHDRFavorAstc = uastcHDRFavorAstc;
	}

	/**
	 * Returns whether the input image is Rec. 2020.
	 * 
	 * See {@link #setRec2020(boolean)}
	 * 
	 * @return The setting
	 */
	public boolean isRec2020() {
	    return rec2020;
	}

	/**
	 * Set whether the input image is Rec. 2020.<br>
	 * <br>
	 * UASTC HDR 6x6i specific option: The input image's gamut is Rec. 
	 * 2020 vs. the default Rec. 709 - for accurate colorspace error 
	 * calculations.
	 * 
	 * @param rec2020 The setting.
	 */
	public void setRec2020(boolean rec2020) {
	    this.rec2020 = rec2020;
	}

	/**
	 * Returns whether rate distortion optimization (RDO) is enabled.
	 * 
	 * See {@link #setUastcHDRLambda(float)}
	 * 
	 * @return The setting
	 */
	public float getUastcHDRLambda() {
	    return uastcHDRLambda;
	}

	/**
	 * Set whether rate distortion optimization (RDO) is enabled.<br>
	 * <br>
	 *  UASTC HDR 6x6i specific option: Enables rate distortion 
	 *  optimization (RDO). The higher this value, the lower the quality, 
	 *  but the smaller the file size. Try 100-20000, or higher values 
	 *  on some images.
	 * 
	 * @param uastcHDRLambda The setting
	 */
	public void setUastcHDRLambda(float uastcHDRLambda) {
	    this.uastcHDRLambda = uastcHDRLambda;
	}

	/**
	 * Returns the HDR encoder tradeoff.
	 * 
	 * See {@link #setUastcHDRLevel(int)}
	 * 
	 * @return The setting
	 */
	public int getUastcHDRLevel() {
	    return uastcHDRLevel;
	}

	/**
	 * Returns the HDR encoder tradeoff.<br>
	 * <br>
	 * UASTC HDR 6x6i specific option: Controls the 6x6 HDR intermediate 
	 * mode encoder performance vs. max quality tradeoff. X may range 
	 * from [0,12]. Default level is 2.
	 * 
	 * @param uastcHDRLevel The setting
	 */
	public void setUastcHDRLevel(int uastcHDRLevel) {
	    this.uastcHDRLevel = uastcHDRLevel;
	}
	
}
