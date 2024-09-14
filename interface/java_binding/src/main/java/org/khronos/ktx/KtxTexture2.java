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
	/**
	 * Creates a new instance.<br>
	 * <br>
	 * This is only called from native methods.
	 *
	 * @param instance The pointer to the native instance,
	 * converted to a 'long' value.
	 */
	protected KtxTexture2(long instance) {
		super(instance);
	}

	/**
	 * Returns the the opto-electrical transfer function of the images.<br>
	 * <br>
	 * This is one of the constants in {@link KhrDfTransfer}.
	 *
	 * @return The transfer function.
	 */
	public native int getOETF();

	/**
	 * Returns whether the RGB components have been premultiplied by the alpha component.
	 *
	 * @return The premultiplied flag
	 */
	public native boolean getPremultipliedAlpha();

	/**
	 * Returns whether the images are in a transcodable format.
	 *
	 * @return Whether the images are transcodable
	 */
	public native boolean needsTranscoding();

	/**
	 * Return the {@link VkFormat} of this texture
	 *
	 * @return The {@link VkFormat}
	 */
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

	/**
	 * Encode and compress a ktx texture with uncompressed images to astc.<br>
	 * <br>
	 * The images are encoded to ASTC block-compressed format. The encoded
	 * images replace the original images and the texture's fields including
	 * the DFD are modified to reflect the new state.<br>
	 * <br>
	 * Such textures can be directly uploaded to a GPU via a graphics API.
	 *
	 * @param params The {@link KtxAstcParams}
	 * @return A {@link KtxErrorCode} constant:<br>
	 * {@link KtxErrorCode#SUCCESS} on success<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's images are supercompressed.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's images are in a block compressed format.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture image's format is a packed format (e.g. RGB565).<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture image format's component size is not 8-bits.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's images are 1D. Only 2D images can be supercompressed.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} ASTC compressor failed to compress image for any reason.<br>
	 * {@link KtxErrorCode#OUT_OF_MEMORY} Not enough memory to carry out compression.
	 */
	public native int compressAstcEx(KtxAstcParams params);

	/**
	 * Encode and compress a ktx texture with uncompressed images to astc.<br>
	 * <br>
	 * The images are either encoded to ASTC block-compressed format. The encoded images
	 * replace the original images and the texture's fields including the DFD are
	 * modified to reflect the new state.<br>
	 * <br>
	 * Such textures can be directly uploaded to a GPU via a graphics API.
	 *
	 * @param quality The compression quality, a value from 0 - 100.
	 * Higher=higher quality/slower speed. Lower=lower quality/faster speed.
	 * Negative values for quality are considered to be greater than 100.
	 * @return A {@link KtxErrorCode} constant:<br>
	 * {@link KtxErrorCode#SUCCESS} on success<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture is already supercompressed.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's image are in a block compressed format.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture image's format is a packed format (e.g. RGB565).<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture image format's component size is not 8-bits.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's images are 1D. Only 2D images can be supercompressed.<br>
	 * {@link KtxErrorCode#OUT_OF_MEMORY} Not enough memory to carry out supercompression.<br>
	 */
	public native int compressAstc(int quality);

	/**
	 * Encode and possibly Supercompress a KTX2 texture with uncompressed images.<br>
	 * <br>
	 * The images are either encoded to ETC1S block-compressed format and supercompressed
	 * with Basis LZ or they are encoded to UASTC block-compressed format. UASTC format
	 * is selected by setting the {@link KtxBasisParams#setUastc(boolean)} to <code>true</code>.
	 * The encoded images replace the original images and the texture's fields including the
	 * DFD are modified to reflect the new state.<br>
	 * <br>
	 * Such textures must be transcoded to a desired target block compressed format before
	 * they can be uploaded to a GPU via a graphics API.
	 *
	 * @param params The {@link KtxBasisParams}
	 * @return A {@link KtxErrorCode} constant:<br>
	 * {@link KtxErrorCode#SUCCESS} on success<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's images are supercompressed.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's images are in a block compressed format.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture image's format is a packed format (e.g. RGB565).<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture image format's component size is not 8-bits.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} normalMode is specified but the texture has only one component.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} Both preSwizzle and and inputSwizzle are specified in params.<br>
	 * {@link KtxErrorCode#OUT_OF_MEMORY} Not enough memory to carry out compression.<br>
	 */
	public native int compressBasisEx(KtxBasisParams params);

	/**
	 * Supercompress a KTX2 texture with uncompressed images.<br>
	 * <br>
	 * The images are encoded to ETC1S block-compressed format and supercompressed
	 * with Basis Universal. The encoded images replace the original images and
	 * the texture's fields including the DFD are modified to reflect the new state.<br>
	 * <br>
	 * Such textures must be transcoded to a desired target block compressed
	 * format before they can be uploaded to a GPU via a graphics API.<br>
	 * <br>
	 * @param quality Compression quality, a value from 1 - 255. Default is 128 which
	 * is selected if quality is 0. Lower=better compression/lower quality/faster.
	 * Higher=less compression/higher quality/slower.
	 * @return A {@link KtxErrorCode} constant:<br>
	 * {@link KtxErrorCode#SUCCESS} on success<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture is already supercompressed.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The texture's image are in a block compressed format.<br>
	 * {@link KtxErrorCode#OUT_OF_MEMORY} Not enough memory to carry out supercompression.
	 */
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
	 * {@link KtxTranscodeFormat#ETC1_RGB}<br>
	 * {@link KtxTranscodeFormat#ETC2_RGBA}<br>
	 * {@link KtxTranscodeFormat#BC1_RGB}<br>
	 * {@link KtxTranscodeFormat#BC3_RGBA}<br>
	 * {@link KtxTranscodeFormat#BC4_R}<br>
	 * {@link KtxTranscodeFormat#BC5_RG}<br>
	 * {@link KtxTranscodeFormat#BC7_RGBA}<br>
	 * {@link KtxTranscodeFormat#PVRTC1_4_RGB}<br>
	 * {@link KtxTranscodeFormat#PVRTC1_4_RGBA}<br>
	 * {@link KtxTranscodeFormat#PVRTC2_4_RGB}<br>
	 * {@link KtxTranscodeFormat#PVRTC2_4_RGBA}<br>
	 * {@link KtxTranscodeFormat#ASTC_4x4_RGBA}<br>
	 * {@link KtxTranscodeFormat#ETC2_EAC_R11}<br>
	 * {@link KtxTranscodeFormat#ETC2_EAC_RG11}<br>
	 * {@link KtxTranscodeFormat#ETC}<br>
	 * {@link KtxTranscodeFormat#BC1_OR_3}<br>
	 * <br>
	 * <code>ETC</code> automatically selects between
	 * <code>ETC1_RGB</code> and <code>ETC2_RGBA</code> according to
	 * whether an alpha channel is available. <code>BC1_OR_3</code> does
	 * likewise between <code>BC1_RGB</code> and
	 * <code>BC3_RGBA</code>. Note that if
	 * <code>PVRTC1_4_RGBA</code> or <code>PVRTC2_4_RGBA</code> is
	 * specified and there is no alpha channel <code>PVRTC1_4_RGB</code> or
	 * <code>PVRTC2_4_RGB</code> respectively will be selected. <br>
	 * Transcoding to ATC and FXT1 formats is not supported by libktx as there are no
	 * equivalent Vulkan formats.<br>
	 * <br>
	 * The following uncompressed transcode targets are also available: <br>
	 * {@link KtxTranscodeFormat#RGBA32}<br>
	 * {@link KtxTranscodeFormat#RGB565}<br>
	 * {@link KtxTranscodeFormat#BGR565}<br>
	 * {@link KtxTranscodeFormat#RGBA4444}<br>
	 * <br>
	 * The following transcodeFlags are available:<br>
	 * <br>
	 * {@link KtxTranscodeFlagBits#PVRTC_DECODE_TO_NEXT_POW2}<br>
	 * {@link KtxTranscodeFlagBits#TRANSCODE_ALPHA_DATA_TO_OPAQUE_FORMATS}<br>
	 * {@link KtxTranscodeFlagBits#HIGH_QUALITY}<br>
	 *
	 * @param outputFormat   A value from the {@link KtxTranscodeFormat} specifying
	 *                       the target format.
	 * @param transcodeFlags A bitfield of flags from {@link KtxTranscodeFlagBits}
	 *                       modifying the transcode
	 * @return {@link KtxErrorCode#SUCCESS} on success, or a code indicating the error:<br>
	 * {@link KtxErrorCode#FILE_DATA_ERROR} Supercompression global data
	 * is corrupted.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} The
	 * texture's format is not transcodable (not ETC1S/BasisLZ or UASTC).<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} Supercompression global
	 * data is missing, i.e., the texture object is invalid.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} Image data is missing,
	 * i.e., the texture object is invalid.<br>
	 * {@link KtxErrorCode#INVALID_OPERATION} outputFormat is PVRTC1 but the texture
	 * does does not have power-of-two dimensions.<br>
	 * {@link KtxErrorCode#INVALID_VALUE} outputFormat is invalid.<br>
	 * {@link KtxErrorCode#TRANSCODE_FAILED} Something went wrong during transcoding.<br>
	 * {@link KtxErrorCode#UNSUPPORTED_FEATURE} PVRTC_DECODE_TO_NEXT_POW2 was
	 * requested or the specified transcode target has not been included in the library
	 * being used.<br>
	 * {@link KtxErrorCode#OUT_OF_MEMORY} Not enough memory to carry out transcoding.<br>
	 */
	public native int transcodeBasis(int outputFormat, int transcodeFlags);

	/**
	 * Create a fresh {@link KtxTexture2}
	 *
	 * @param createInfo The {@link KtxTextureCreateInfo} parameters for the texture
	 * @param storageAllocation The storage allocation. Pass {@link KtxTextureCreateStorage#ALLOC_STORAGE} if you will write image data.
	 * @return The {@link KtxTexture2}
	 * @throws KtxException If the input parameters have been invalid and caused
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
	 * implementation.
	 */
	public static native KtxTexture2 create(KtxTextureCreateInfo createInfo,
			int storageAllocation);

	/**
	 * Create a {@link KtxTexture2} from a file.
	 *
	 * @param filename The name of the file to read.
	 * @param createFlags Pass {@link KtxTextureCreateFlagBits#LOAD_IMAGE_DATA_BIT} if you
	 *                   want to read image data! Otherwise, {@link KtxTexture#getData()} will
	 *                    return null.
	 * @return The {@link KtxTexture2}
	 * @throws KtxException If the input data was invalid and caused
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
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
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
	 * implementation.
	 */
	public static KtxTexture2 createFromNamedFile(String filename) {
		return createFromNamedFile(filename, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
	}

	/**
	 * Create a {@link KtxTexture2} from KTX-formatted data in memory.<br>
	 * <br>
	 * The create flag {@link KtxTextureCreateFlagBits#LOAD_IMAGE_DATA_BIT}
	 * should not be set if the ktxTexture is ultimately to be uploaded to
	 * OpenGL or Vulkan. This will minimize memory usage by allowing, for
	 * example, loading the images directly from the source into a Vulkan
	 * staging buffer.<br>
	 * <br>
	 * The create flag {@link KtxTextureCreateFlagBits#RAW_KVDATA_BIT} should
	 * not be used. It is provided solely to enable implementation of the
	 * libktx v1 API on top of ktxTexture.<br>
	 * <br>
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
	 * Deflate the data in a {@link KtxTexture2} object using Zstandard.<br>
	 * <br>
	 * The texture's levelIndex, dataSize, DFD, data pointer, and supercompressionScheme will
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
	 * The texture's levelIndex, dataSize, DFD, data pointer, and supercompressionScheme will
	 * all be updated after successful deflation to reflect the deflated data.<br>
	 *
	 * @param level Set speed vs compression ratio trade-off. Values
	 * between 1 and 9 are accepted. The lower the level the faster.
	 * @return A {@link KtxErrorCode} value
	 */
	public native int deflateZLIB(int level);

}
