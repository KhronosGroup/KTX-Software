/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

public class KtxAstcParams {
    private boolean verbose;
    private int threadCount;

    /**
     * The {@link KtxPackAstcBlockDimension} constant
     */
    private int blockDimension;

    /**
     * The {@link KtxPackAstcEncoderMode} constant
     */
    private int mode;
    private int qualityLevel;
    private boolean normalMap;
    private boolean perceptual;
    private char[] inputSwizzle = new char[4];

    public boolean isVerbose() {
        return verbose;
    }

    public void setVerbose(boolean verbose) {
        this.verbose = verbose;
    }

    public int getThreadCount() {
        return threadCount;
    }

    public void setThreadCount(int threadCount) {
        this.threadCount = threadCount;
    }

    /**
     * Returns the {@link KtxPackAstcBlockDimension} constant
     *
     * @return The block dimension constant
     */
    public int getBlockDimension() {
        return blockDimension;
    }

    /**
     * Set the specified {@link KtxPackAstcBlockDimension} constant
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
     * Set the {@link KtxPackAstcQualityLevel} constant
     *
     * Note that while this appears to be a simple integer value in the range [0, 100],
     * it does not seem to be guaranteed that all values are supposed. Only the
     * constants from the {@link KtxPackAstcQualityLevel} are guaranteed to be
     * supported.
     *
     * @param qualityLevel The quality level
     */
    public void setQualityLevel(int qualityLevel) {
        this.qualityLevel = qualityLevel;
    }

    public boolean isNormalMap() {
        return normalMap;
    }

    public void setNormalMap(boolean normalMap) {
        this.normalMap = normalMap;
    }

    public boolean isPerceptual() {
        return perceptual;
    }

    public void setPerceptual(boolean perceptual) {
        this.perceptual = perceptual;
    }

    public char[] getInputSwizzle() {
        return inputSwizzle;
    }

    public void setInputSwizzle(char[] inputSwizzle) {
        if (inputSwizzle.length != 4) {
            throw new IllegalArgumentException("inputSwizzle must be 4 bytes!");
        }

        this.inputSwizzle = inputSwizzle;
    }
}
