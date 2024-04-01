/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

public class KtxBasisParams {
    private boolean uastc;
    private boolean verbose;
    private boolean noSSE;
    private int threadCount;
    private int compressionLevel;
    private int qualityLevel;
    private int maxEndpoints;
    private float endpointRDOThreshold;
    private int maxSelectors;
    private float selectorRDOThreshold;
    private char[] inputSwizzle = new char[4];
    private boolean normalMap;
    private boolean preSwizzle;
    private boolean noEndpointRDO;
    private boolean noSelectorRDO;
    private int uastcFlags;
    private boolean uastcRDO;
    private float uastcRDOQualityScalar;
    private int uastcRDODictSize;
    private float uastcRDOMaxSmoothBlockErrorScale;
    private float uastcRDOMaxSmoothBlockStdDev;
    private boolean uastcRDODontFavorSimplerModes;
    private boolean uastcRDONoMultithreading;

    public boolean isUastc() {
        return uastc;
    }

    public void setUastc(boolean uastc) {
        this.uastc = uastc;
    }

    public boolean isVerbose() {
        return verbose;
    }

    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }

    public boolean isNoSSE() {
        return noSSE;
    }

    public void setNoSSE(boolean noSSE) {
        this.noSSE = noSSE;
    }

    public int getThreadCount() {
        return threadCount;
    }

    public void setThreadCount(int threadCount) {
        this.threadCount = threadCount;
    }

    public int getCompressionLevel() {
        return compressionLevel;
    }

    public void setCompressionLevel(int compressionLevel) {
        this.compressionLevel = compressionLevel;
    }

    public int getQualityLevel() {
        return qualityLevel;
    }

    public void setQualityLevel(int qualityLevel) {
        this.qualityLevel = qualityLevel;
    }

    public int getMaxEndpoints() {
        return maxEndpoints;
    }

    public void setMaxEndpoints(int maxEndpoints) {
        this.maxEndpoints = maxEndpoints;
    }

    public float getEndpointRDOThreshold() {
        return endpointRDOThreshold;
    }

    public void setEndpointRDOThreshold(float endpointRDOThreshold) {
        this.endpointRDOThreshold = endpointRDOThreshold;
    }

    public int getMaxSelectors() {
        return maxSelectors;
    }

    public void setMaxSelectors(int maxSelectors) {
        this.maxSelectors = maxSelectors;
    }

    public float getSelectorRDOThreshold() {
        return selectorRDOThreshold;
    }

    public void setSelectorRDOThreshold(float selectorRDOThreshold) {
        this.selectorRDOThreshold = selectorRDOThreshold;
    }

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

    public boolean isNormalMap() {
        return normalMap;
    }

    public void setNormalMap(boolean normalMap) {
        this.normalMap = normalMap;
    }

    public boolean isPreSwizzle() {
        return preSwizzle;
    }

    public void setPreSwizzle(boolean preSwizzle) {
        this.preSwizzle = preSwizzle;
    }

    public boolean isNoEndpointRDO() {
        return noEndpointRDO;
    }

    public void setNoEndpointRDO(boolean noEndpointRDO) {
        this.noEndpointRDO = noEndpointRDO;
    }

    public boolean isNoSelectorRDO() {
        return noSelectorRDO;
    }

    public void setNoSelectorRDO(boolean noSelectorRDO) {
        this.noSelectorRDO = noSelectorRDO;
    }

    /**
     * Returns the {@link KtxPackUastcFlagBits}
     *
     * @return The flag bits
     */
    public int getUastcFlags() {
        return uastcFlags;
    }

    /**
     * Set the {@link KtxPackUastcFlagBits}
     *
     * @param uastcFlags The flag bits
     */
    public void setUastcFlags(int uastcFlags) {
        this.uastcFlags = uastcFlags;
    }

    public boolean isUastcRDO() {
        return uastcRDO;
    }

    public void setUastcRDO(boolean uastcRDO) {
        this.uastcRDO = uastcRDO;
    }

    public float getUastcRDOQualityScalar() {
        return uastcRDOQualityScalar;
    }

    public void setUastcRDOQualityScalar(float uastcRDOQualityScalar) {
        this.uastcRDOQualityScalar = uastcRDOQualityScalar;
    }

    public int getUastcRDODictSize() {
        return uastcRDODictSize;
    }

    public void setUastcRDODictSize(int uastcRDODictSize) {
        this.uastcRDODictSize = uastcRDODictSize;
    }

    public float getUastcRDOMaxSmoothBlockErrorScale() {
        return uastcRDOMaxSmoothBlockErrorScale;
    }

    public void setUastcRDOMaxSmoothBlockErrorScale(float uastcRDOMaxSmoothBlockErrorScale) {
        this.uastcRDOMaxSmoothBlockErrorScale = uastcRDOMaxSmoothBlockErrorScale;
    }

    public float getUastcRDOMaxSmoothBlockStdDev() {
        return uastcRDOMaxSmoothBlockStdDev;
    }

    public void setUastcRDOMaxSmoothBlockStdDev(float uastcRDOMaxSmoothBlockStdDev) {
        this.uastcRDOMaxSmoothBlockStdDev = uastcRDOMaxSmoothBlockStdDev;
    }

    public boolean isUastcRDODontFavorSimplerModes() {
        return uastcRDODontFavorSimplerModes;
    }

    public void setUastcRDODontFavorSimplerModes(boolean uastcRDODontFavorSimplerModes) {
        this.uastcRDODontFavorSimplerModes = uastcRDODontFavorSimplerModes;
    }

    public boolean isUastcRDONoMultithreading() {
        return uastcRDONoMultithreading;
    }

    public void setUastcRDONoMultithreading(boolean uastcRDONoMultithreading) {
        this.uastcRDONoMultithreading = uastcRDONoMultithreading;
    }
}
