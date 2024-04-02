/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Class representing a KTX version 1 format texture.<br>
 * <br>
 * KTX textures should be created only by one of the provided functions and these
 * fields should be considered read-only.<br>
 * <br>
 * Trying to use a KTX texture after its {@link #destroy()} method was called
 * will result in an <code>IllegalStateException</code>.<br>
 * <br>
 * Unless explicitly noted, none of the parameters passed to any function
 * may be <code>null</code>.<br>
 */
public class KtxTexture1 extends KtxTexture {
    protected KtxTexture1(long instance) {
        super(instance);
    }

    public native int getGlFormat();
    public native int getGlInternalformat();
    public native int getGlBaseInternalformat();
    public native int getGlType();

    /**
     * Create a fresh {@link KtxTexture1}
     *
     * @param info The {@link KtxTextureCreateInfo} parameters for the texture
     * @param storageAllocation The storage allocation. Pass {@link KtxTextureCreateStorage#KTX_TEXTURE_CREATE_ALLOC_STORAGE} if you will write image data.
     * @return The {@link KtxTexture1}
     * @throws KtxException If the input parameters have been invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture1 create(KtxTextureCreateInfo info,
                                            int storageAllocation);

    /**
     * Create a {@link KtxTexture1} from a file.
     *
     * @param filename The name of the file to read.
     * @param createFlags The creation flag bits. Pass {@link KtxTextureCreateFlagBits#KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT} if you
     *                   want to read image data! Otherwise, {@link KtxTexture#getData()} will
     *                    return null.
     * @return The {@link KtxTexture1}
     * @throws KtxException If the input file was invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture1 createFromNamedFile(String filename,
                                                         int createFlags);

    /**
     * Create a {@link KtxTexture1} from a file.
     *
     * @param filename The name of the file to read.
     * @return The {@link KtxTexture1}
     * @throws KtxException If the input file was invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static KtxTexture1 createFromNamedFile(String filename) {
        return createFromNamedFile(filename, KtxTextureCreateFlagBits.KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT);
    }
}
