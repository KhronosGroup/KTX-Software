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

	/**
	 * Creates a new instance.<br>
	 * <br>
	 * This is only called from native methods.
	 *
	 * @param instance The pointer to the native instance,
	 * converted to a 'long' value.
	 */
	protected KtxTexture1(long instance) {
		super(instance);
	}

	/**
	 * Returns the format of the texture data.<br>
	 * <br>
	 * This will be a constant like <code>GL_RGB</code>.
	 *
	 * @return The format of the texture data
	 */
	public native int getGlFormat();

	/**
	 * Returns the internal format of the texture data.<br>
	 * <br>
	 * This will be a constant like <code>GL_RGB8</code>.
	 *
	 * @return The internal format of the texture data
	 */
	public native int getGlInternalformat();

	/**
	 * Returns the base format of the texture data.<br>
	 * <br>
	 * This will be a constant like <code>GL_RGB</code>.
	 *
	 * @return The base internal format of the texture data
	 */
	public native int getGlBaseInternalformat();

	/**
	 * Returns the type of the texture data.<br>
	 * <br>
	 * This will be a constant like <code>GL_UNSIGNED_BYTE</code>.
	 *
	 * @return The type of the texture data
	 */
	public native int getGlType();

	/**
	 * Create a fresh {@link KtxTexture1}
	 *
	 * @param info The {@link KtxTextureCreateInfo} parameters for the texture
	 * @param storageAllocation The storage allocation. Pass {@link KtxTextureCreateStorage#ALLOC_STORAGE} if you will write image data.
	 * @return The {@link KtxTexture1}
	 * @throws KtxException If the input parameters have been invalid and caused
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
	 * implementation.
	 */
	public static native KtxTexture1 create(KtxTextureCreateInfo info,
			int storageAllocation);

	/**
	 * Create a {@link KtxTexture1} from a file.
	 *
	 * @param filename The name of the file to read.
	 * @param createFlags The creation flag bits. Pass {@link KtxTextureCreateFlagBits#LOAD_IMAGE_DATA_BIT} if you
	 *                   want to read image data! Otherwise, {@link KtxTexture#getData()} will
	 *                    return null.
	 * @return The {@link KtxTexture1}
	 * @throws KtxException If the input file was invalid and caused
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
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
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
	 * implementation.
	 */
	public static KtxTexture1 createFromNamedFile(String filename) {
		return createFromNamedFile(filename, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
	}
}
