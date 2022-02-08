/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

public class KtxTextureCreateInfo {
    private int glInternalformat;
    private int baseWidth = 0;
    private int baseHeight = 0;
    private int baseDepth = 1;
    private int numDimensions = 2;
    private int numLevels = 1;
    private int numLayers = 1;
    private int numFaces = 1;
    private boolean isArray = false;
    private boolean generateMipmaps = false;
    private int vkFormat;

    public int getGlInternalformat() {
        return this.glInternalformat;
    }

    public void setGlInternalformat(int glInternalformat) {
        this.glInternalformat = glInternalformat;
    }

    public int getBaseWidth() {
        return baseWidth;
    }

    public void setBaseWidth(int baseWidth) {
        this.baseWidth = baseWidth;
    }

    public int getBaseHeight() {
        return baseHeight;
    }

    public void setBaseHeight(int baseHeight) {
        this.baseHeight = baseHeight;
    }

    public int getBaseDepth() {
        return baseDepth;
    }

    public void setBaseDepth(int baseDepth) {
        this.baseDepth = baseDepth;
    }

    public int getNumDimensions() {
        return numDimensions;
    }

    public void setNumDimensions(int numDimensions) {
        this.numDimensions = numDimensions;
    }

    public int getNumLevels() {
        return numLevels;
    }

    public void setNumLevels(int numLevels) {
        this.numLevels = numLevels;
    }

    public int getNumLayers() {
        return numLayers;
    }

    public void setNumLayers(int numLayers) {
        this.numLayers = numLayers;
    }

    public int getNumFaces() {
        return numFaces;
    }

    public void setNumFaces(int numFaces) {
        this.numFaces = numFaces;
    }

    public boolean isArray() {
        return isArray;
    }

    public void setArray(boolean array) {
        isArray = array;
    }

    public boolean isGenerateMipmaps() {
        return generateMipmaps;
    }

    public void setGenerateMipmaps(boolean generateMipmaps) {
        this.generateMipmaps = generateMipmaps;
    }

    public int getVkFormat() {
        return this.vkFormat;
    }

    public void setVkFormat(int vkFormat) {
        this.vkFormat = vkFormat;
    }
}
