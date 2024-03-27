/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Class representing a KTX version 1 format texture.
 *
 * KTX textures should be created only by one of the provided functions and these
 * fields should be considered read-only.
 *
 * Trying to use a KTX texture after its {@link #destroy()} method was called
 * will result in an <code>IllegalStateException</code>.
 *
 * Unless explicitly noted, none of the parameters passed to any function
 * may be <code>null</code>.
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
     * Create a fresh {@link KTXTexture1}
     *
     * @param createInfo - Paramaters for the texture
     * @param storageAllocation - Pass {@link KTXCreateStorage.ALLOC} if you will write image data.
     */
    public static native KtxTexture1 create(KtxTextureCreateInfo info,
                                            int storageAllocation);

    /**
     * Create a {@link KTXTexture1} from a file.
     *
     * @param filename - The name of the file to read.
     * @param createFlags - Pass {@link KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT} if you
     *                   want to read image data! Otherwise, {@link KTXTexture.getData()} will
     *                    return null.
     */
    public static native KtxTexture1 createFromNamedFile(String filename,
                                                         int createFlags);

    public static KtxTexture1 createFromNamedFile(String filename) {
        return createFromNamedFile(filename, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
    }
}
