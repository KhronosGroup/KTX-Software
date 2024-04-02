/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

import java.nio.ByteBuffer;

/**
 * Class representing a KTX version 2 format texture.<br>
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
     * this texture.<br>
     * <br>
     * This is a constant from the {@link KtxSupercmpScheme} class.
     *
     * @return The supercompression scheme.
     */
    public native int getSupercompressionScheme();

    public native int compressAstcEx(KtxAstcParams params);
    public native int compressAstc(int quality);
    public native int compressBasisEx(KtxBasisParams params);
    public native int compressBasis(int quality);

    /**
	 * Transcode a KTX2 texture with BasisLZ/ETC1S or UASTC images.<br>
	 * <br>
	 * If the texture contains BasisLZ supercompressed images, Inflates them from
	 * back to ETC1S then transcodes them to the specified block-compressed format.
	 * If the texture contains UASTC images, inflates them, if they have been
	 * supercompressed with zstd, then transcodes then to the specified format, The
	 * transcoded images replace the original images and the texture's fields
	 * including the DFD are modified to reflect the new format.<br>
	 * <br>
	 * These types of textures must be transcoded to a desired target
	 * block-compressed format before they can be uploaded to a GPU via a graphics
	 * API.<br>
	 * <br>
	 * The following block compressed transcode targets are available: <br>
	 * {@link KtxTranscodeFormat#KTX_TTF_ETC1_RGB}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_ETC2_RGBA}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BC1_RGB}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BC3_RGBA}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BC4_R}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BC5_RG}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BC7_RGBA}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_PVRTC1_4_RGB}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_PVRTC1_4_RGBA}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_PVRTC2_4_RGB}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_PVRTC2_4_RGBA}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_ASTC_4x4_RGBA}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_ETC2_EAC_R11}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_ETC2_EAC_RG11}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_ETC}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BC1_OR_3}<br>
	 * <br>
	 * <code>KTX_TTF_ETC</code> automatically selects between
	 * <code>KTX_TTF_ETC1_RGB</code> and <code>KTX_TTF_ETC2_RGBA</code> according to
	 * whether an alpha channel is available. <code>KTX_TTF_BC1_OR_3</code> does
	 * likewise between <code>KTX_TTF_BC1_RGB</code> and
	 * <code>KTX_TTF_BC3_RGBA</code>. Note that if
	 * <code>KTX_TTF_PVRTC1_4_RGBA</code> or <code>KTX_TTF_PVRTC2_4_RGBA</code> is
	 * specified and there is no alpha channel <code>KTX_TTF_PVRTC1_4_RGB</code> or
	 * <code>KTX_TTF_PVRTC2_4_RGB</code> respectively will be selected. <br>
	 * Transcoding to ATC and FXT1 formats is not supported by libktx as there are no
	 * equivalent Vulkan formats.<br>
	 * <br>
	 * The following uncompressed transcode targets are also available: <br>
	 * {@link KtxTranscodeFormat#KTX_TTF_RGBA32}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_RGB565}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_BGR565}<br>
	 * {@link KtxTranscodeFormat#KTX_TTF_RGBA4444}<br>
	 * <br>
	 * The following transcodeFlags are available:<br>
	 * <br>
	 * {@link KtxTranscodeFlagBits#KTX_TF_PVRTC_DECODE_TO_NEXT_POW2}<br>
	 * {@link KtxTranscodeFlagBits#KTX_TF_TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS}<br>
	 * {@link KtxTranscodeFlagBits#KTX_TF_HIGH_QUALITY}<br>
	 *
	 * @param outputFormat   A value from the {@link KtxTranscodeFormat} specifying
	 *                       the target format.
	 * @param transcodeFlags A bitfield of flags from {@link KtxTranscodeFlagBits}
	 *                       modifying the transcode
	 * @return {@link KtxErrorCode#KTX_SUCCESS} on success, or a code indicating the error:<br>
	 * {@link KtxErrorCode#KTX_FILE_DATA_ERROR} Supercompression global data
	 * is corrupted.<br>
	 * {@link KtxErrorCode#KTX_INVALID_OPERATION} The
	 * texture's format is not transcodable (not ETC1S/BasisLZ or UASTC).<br>
	 * {@link KtxErrorCode#KTX_INVALID_OPERATION} Supercompression global
	 * data is missing, i.e., the texture object is invalid.<br>
	 * {@link KtxErrorCode#KTX_INVALID_OPERATION} Image data is missing,
	 * i.e., the texture object is invalid.<br>
	 * {@link KtxErrorCode#KTX_INVALID_OPERATION} outputFormat is PVRTC1 but the texture
	 * does does not have power-of-two dimensions.<br>
	 * {@link KtxErrorCode#KTX_INVALID_VALUE} outputFormat is invalid.<br>
	 * {@link KtxErrorCode#KTX_TRANSCODE_FAILED} Something went wrong during transcoding.<br>
	 * {@link KtxErrorCode#KTX_UNSUPPORTED_FEATURE} KTX_TF_PVRTC_DECODE_TO_NEXT_POW2 was
	 * requested or the specified transcode target has not been included in the library
	 * being used.<br>
	 * {@link KtxErrorCode#KTX_OUT_OF_MEMORY} Not enough memory to carry out transcoding.<br>
	 */
    public native int transcodeBasis(int outputFormat, int transcodeFlags);

    /**
     * Create a fresh {@link KtxTexture2}
     *
     * @param createInfo The {@link KtxTextureCreateInfo} parameters for the texture
     * @param storageAllocation The storage allocation. Pass {@link KtxTextureCreateStorage#KTX_TEXTURE_CREATE_ALLOC_STORAGE} if you will write image data.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input parameters have been invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture2 create(KtxTextureCreateInfo createInfo,
                                            int storageAllocation);

    /**
     * Create a {@link KtxTexture2} from a file.
     *
     * @param filename The name of the file to read.
     * @param createFlags Pass {@link KtxTextureCreateFlagBits#KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT} if you
     *                   want to read image data! Otherwise, {@link KtxTexture#getData()} will
     *                    return null.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input data was invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture2 createFromNamedFile(String filename,
                                                         int createFlags);

    /**
     * Create a {@link KtxTexture2} from a file.
     *
     * @param filename The name of the file to read.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input data was invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static KtxTexture2 createFromNamedFile(String filename) {
        return createFromNamedFile(filename, KtxTextureCreateFlagBits.KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT);
    }

    /**
     * Create a {@link KtxTexture2} from KTX-formatted data in memory.<br>
     * <br>
     * The create flag {@link KtxTextureCreateFlagBits#KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT}
     * should not be set if the ktxTexture is ultimately to be uploaded to
     * OpenGL or Vulkan. This will minimize memory usage by allowing, for
     * example, loading the images directly from the source into a Vulkan
     * staging buffer.<br>
     * <br>
     * The create flag {@link KtxTextureCreateFlagBits#KTX_TEXTURE_CREATE_RAW_KVDATA_BIT} should
     * not be used. It is provided solely to enable implementation of the
     * libktx v1 API on top of ktxTexture.<br>
     * <br>
     * @param byteBuffer The buffer containing the serialized KTX data.
     * @param createFlags bitmask requesting specific actions during creation.
     * @return The {@link KtxTexture2}
     * @throws KtxException If the input data was invalid and caused
     * an error code that was not {@link KtxErrorCode#KTX_SUCCESS} in the underlying
     * implementation.
     */
    public static native KtxTexture2 createFromMemory(
    		ByteBuffer byteBuffer, int createFlags);

    /**
     * Deflate the data in a {@link KtxTexture2} object using Zstandard.<br>
     * <br>
     * The texture's levelIndex, dataSize, DFD  and supercompressionScheme will
     * all be updated after successful deflation to reflect the deflated data.<br>
     *
     * @param level Set speed vs compression ratio trade-off. Values
     * between 1 and 22 are accepted. The lower the level the faster. Values
     * above 20 should be used with caution as they require more memory.
     * @return A {@link KtxErrorCode} value
     */
    public native int deflateZstd(int level);

    /**
     * Deflate the data in a {@link KtxTexture2} object using miniz (ZLIB).<br>
     * <br>
     * The texture's levelIndex, dataSize, DFD  and supercompressionScheme will
     * all be updated after successful deflation to reflect the deflated data.<br>
     *
     * @param level Set speed vs compression ratio trade-off. Values
     * between 1 and 9 are accepted. The lower the level the faster.
     * @return A {@link KtxErrorCode} value
     */
    public native int deflateZLIB(int level);

}
