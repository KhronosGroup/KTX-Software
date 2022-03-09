/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

public class KtxAstcParams {
    private boolean verbose;
    private int threadCount;
    private int blockDimension;
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

    public int getBlockDimension() {
        return blockDimension;
    }

    public void setBlockDimension(int blockDimension) {
        this.blockDimension = blockDimension;
    }

    public int getMode() {
        return mode;
    }

    public void setMode(int mode) {
        this.mode = mode;
    }

    public int getQualityLevel() {
        return qualityLevel;
    }

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
