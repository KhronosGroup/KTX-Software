package org.khronos.ktx;

public abstract class KTXTexture {
    private final long instance;
    private long buffers;

    protected KTXTexture(long instance) {
        this.instance = instance;
        this.buffers = 0;
    }

    /* DEBUG */
    public boolean isDestroyed() {
        return this.instance == 0;
    }
    public native int getBufferListSize();
    /* DEBUG */

    public native boolean isArray();
    public native boolean isCubemap();
    public native boolean isCompressed();
    public native boolean getGenerateMipmaps();
    public native int getBaseWidth();
    public native int getBaseHeight();
    public native int getBaseDepth();
    public native int getNumDimensions();
    public native int getNumFaces();

    /**
     * Gets the image data of the KTX file (the data is copied into a Java array)
     */
    public native byte[] getData();
    public native long getDataSize();
    public native long getDataSizeUncompressed();
    public native int getElementSize();
    public native int getRowPitch(int level);
    public native int getImageSize(int level);
    public native long getImageOffset(int level, int layer, int faceSlice);

    /**
     * Destroy the KTX texture and free memory image resources
     *
     * NOTE: If you try to use a {@link KTXTexture} after it's destroyed, that will cause a segmentation
     * fault (the texture pointer is set to NULL).
     */
    public native void destroy();

    /**
     * Set the image data - the passed data should be not modified after passing it! The bytes
     * will be kept in memory until {@link KTXTexture#destroy} is called.
     *
     * @param level - The image level, should be 0 for non-mipmapped textures
     * @param layer - The texture layer, should be 0 for non-arrays
     * @param faceSlice - The face slice, should be 0 for non-cubemaps
     * @param src - The image data
     */
    public native int setImageFromMemory(int level, int layer, int faceSlice, byte[] src);

    /**
     * Write the KTX image to the given destination file in KTX format
     *
     * @param dstFilename - The name of the destination file.
     */
    public native int writeToNamedFile(String dstFilename);
}
