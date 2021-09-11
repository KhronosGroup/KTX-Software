package org.khronos.ktx;

public class KTXTextureCreateInfo {
    private int glInternalformat;
    private int baseWidth;
    private int baseHeight;
    private int baseDepth;
    private int numDimensions;
    private int numLevels;
    private int numLayers;
    private int numFaces;
    private boolean isArray;
    private boolean generateMipmaps;

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
}
