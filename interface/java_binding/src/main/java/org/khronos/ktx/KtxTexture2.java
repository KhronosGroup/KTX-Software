/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

import java.nio.ByteBuffer;

/**
 * Class representing a KTX version 2 format texture.
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
public class KtxTexture2 extends KtxTexture {
    protected KtxTexture2(long instance) {
        super(instance);
    }

    public native int getOETF();
    public native boolean getPremultipliedAlpha();
    public native boolean needsTranscoding();
    public native int getVkFormat();

    /**
     * Returns the supercompression scheme that has been applied to
     * this texture.
     *
     * This is a constant from the {@link KtxSupercmpScheme} class.
     *
     * @return The supercompression scheme.
     */
    public native int getSupercompressionScheme();

    public native int compressAstcEx(KtxAstcParams params);
    public native int compressAstc(int quality);
    public native int compressBasisEx(KtxBasisParams params);
    public native int compressBasis(int quality);
    public native int transcodeBasis(int outputFormat, int transcodeFlags);

    /**
     * Create a fresh {@link KTXTexture2}
     *
     * @param createInfo The {@link KtxTextureCreateInfo} parameters for the texture
     * @param storageAllocation The storage allocation. Pass {@link KTXCreateStorage.ALLOC} if you will write image data.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input parameters have been invalid and caused
     * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture2 create(KtxTextureCreateInfo createInfo,
                                            int storageAllocation);

    /**
     * Create a {@link KTXTexture2} from a file.
     *
     * @param filename The name of the file to read.
     * @param createFlags Pass {@link KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT} if you
     *                   want to read image data! Otherwise, {@link KTXTexture.getData()} will
     *                    return null.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input data was invalid and caused
     * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture2 createFromNamedFile(String filename,
                                                         int createFlags);

    /**
     * Create a {@link KTXTexture2} from a file.
     *
     * @param filename The name of the file to read.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input data was invalid and caused
     * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
     * implementation.
     */
    public static KtxTexture2 createFromNamedFile(String filename) {
        return createFromNamedFile(filename, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
    }

    /**
     * Create a {@link KtxTexture2} from KTX-formatted data in memory.
     *
     * The create flag {@link KtxTextureCreateFlagBits#LOAD_IMAGE_DATA_BIT}
     * should not be set if the ktxTexture is ultimately to be uploaded to
     * OpenGL or Vulkan. This will minimize memory usage by allowing, for
     * example, loading the images directly from the source into a Vulkan
     * staging buffer.
     *
     * The create flag {@link KtxTextureCreateFlagBits#RAW_KVDATA_BIT} should
     * not be used. It is provided solely to enable implementation of the
     * libktx v1 API on top of ktxTexture.
     *
     * @param byteBuffer The buffer containing the serialized KTX data.
     * @param createFlags bitmask requesting specific actions during creation.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input data was invalid and caused
     * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture2 createFromMemory(
    		ByteBuffer byteBuffer, int createFlags);

    /**
     * Deflate the data in a {@link KtxTexture2} object using Zstandard.
     *
     * The texture's levelIndex, dataSize, DFD  and supercompressionScheme will
     * all be updated after successful deflation to reflect the deflated data.
     *
     * @param level Set speed vs compression ratio trade-off. Values
     * between 1 and 22 are accepted. The lower the level the faster. Values
     * above 20 should be used with caution as they require more memory.
     * @return A {@link KtxErrorCode} value
     */
    public native int deflateZstd(int level);

    /**
     * Deflate the data in a {@link KtxTexture2} object using miniz (ZLIB)
     *
     * The texture's levelIndex, dataSize, DFD  and supercompressionScheme will
     * all be updated after successful deflation to reflect the deflated data.
     *
     * @param level Set speed vs compression ratio trade-off. Values
     * between 1 and 9 are accepted. The lower the level the faster.
     * @return A {@link KtxErrorCode} value
     */
    public native int deflateZLIB(int level);

}
