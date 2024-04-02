/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Base class representing a texture.<br>
 * <br>
 * KTX textures should be created only by one of the provided functions and these
 * fields should be considered read-only.<br>
 * <br>
 * Trying to use a KTX texture after its {@link #destroy()} method was called
 * will result in an <code>IllegalStateException</code>.<br>
 * <br>
 * Unless explicitly noted, none of the parameters passed to any function
 * may be <code>null</code>.
 */
public abstract class KtxTexture {
    private final long instance;

    protected KtxTexture(long instance) {
        this.instance = instance;
    }

    /* DEBUG */
    public boolean isDestroyed() {
        return this.instance == 0;
    }
    /* DEBUG */

    public native boolean isArray();
    public native boolean isCubemap();
    public native boolean isCompressed();
    public native boolean getGenerateMipmaps();
    public native int getBaseWidth();
    public native int getBaseHeight();
    public native int getBaseDepth();
    public native int getNumDimensions();
    public native int getNumLevels();
    public native int getNumFaces();

    /**
     * Gets the image data of the KTX file.<br>
     * <br>
     * The the data is copied into a Java array.
     *
     * @return The data
     * @throws UnsupportedOperationException If the resulting array would
     * be larger than the maximum size of a Java array (i.e. more than 2GB)
     */
    public native byte[] getData();
    public native long getDataSize();
    public native long getDataSizeUncompressed();
    public native int getElementSize();
    public native int getRowPitch(int level);
    public native int getImageSize(int level);
    public native long getImageOffset(int level, int layer, int faceSlice);

    /**
     * Destroy the KTX texture and free memory image resources.<br>
     * <br>
     * Trying to use a {@link KtxTexture} after it was destroyed will cause a
     * an <code>IllegalStateException</code> to be thrown. This method is
     * idempotent: Calling it on an already destroyed texture will have
     * no effect.
     */
    public native void destroy();

    /**
     * Set image for level, layer, faceSlice from an image in memory.
     * <br>
     * Uncompressed images in memory are expected to have their rows tightly packed
     * as is the norm for most image file formats. KTX 2 also requires tight packing
     * this function does not add any padding.<br>
     * <br>
     * Level, layer, faceSlice rather than offset are specified to enable some
     * validation.<br>
     *
     * @param level     The image level, should be 0 for non-mipmapped textures
     * @param layer     The texture layer, should be 0 for non-arrays
     * @param faceSlice The face slice, should be 0 for non-cubemaps
     * @param src       The image data
     * @return a {@link KtxErrorCode} value
     */
    public native int setImageFromMemory(int level, int layer, int faceSlice, byte[] src);

    /**
     * Write the KTX image to the given destination file in KTX format.
     *
     * @param dstFilename The name of the destination file.
     * @return a {@link KtxErrorCode} value
     */
    public native int writeToNamedFile(String dstFilename);

    /**
     * Write this texture to memory.
     *
     * @return The memory containing the texture data.
     * @throws KtxException If the attempt to write the texture to memory caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     * @throws UnsupportedOperationException If the resulting array would
     * be larger than the maximum size of a Java array (i.e. more than 2GB)
     */
    public native byte[] writeToMemory();
}
