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

	/**
	 * The native pointer to the <code>ktxTexture</code> instance,
	 * converted to a 'long' value.
	 *
	 * This is maintained by native methods - and it may be modified,
	 * even though it is marked as 'final' here!
	 */
	private final long instance;

	/**
	 * Default constructor
	 *
	 * @param instance The native pointer to the texture instance
	 */
	protected KtxTexture(long instance) {
		this.instance = instance;
	}

	/**
	 * Returns whether {@link #destroy()} was already called on this texture.
	 *
	 * @return Whether the texture is destroyed
	 * @deprecated Only for debugging
	 */
	public boolean isDestroyed() {
		return this.instance == 0;
	}

	/**
	 * Returns <code>true</code> if the texture is an array texture.<br>
	 * <br>
	 * This means that a <code>GL_TEXTURE_*_ARRAY</code> target is to be used.
	 *
	 * @return The flag
	 */
	public native boolean isArray();

	/**
	 * Returns <code>true</code> if the texture is a cubemap or cubemap array
	 *
	 * @return The flag
	 */
	public native boolean isCubemap();

	/**
	 * Returns <code>true</code> if the texture's format is a block compressed format.
	 *
	 * @return The flag
	 */
	public native boolean isCompressed();

	/**
	 * Returns <code>true</code> if mipmaps should be generated for the texture.
	 *
	 * @return The flag
	 */
	public native boolean getGenerateMipmaps();

	/**
	 * Returns the width of the texture's base level.
	 *
	 * @return The width
	 */
	public native int getBaseWidth();

	/**
	 * Returns the height of the texture's base level.
	 *
	 * @return The height
	 */
	public native int getBaseHeight();

	/**
	 * Returns the depth of the texture's base level.
	 *
	 * @return The depth
	 */
	public native int getBaseDepth();

	/**
	 * Returns the number of dimensions in the texture - 1, 2 or 3.
	 *
	 * @return The number of dimensions
	 */
	public native int getNumDimensions();

	/**
	 * Returns the number of MIP levels in the texture.
	 *
	 * @return The number of levels
	 */
	public native int getNumLevels();

	/**
	 * Returns the number of faces: 6 for cube maps, 1 otherwise.
	 *
	 * @return The number of faces.
	 */
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

	/**
	 * Return the total size of the texture image data in bytes.
	 *
	 * For a {@link KtxTexture2} with {@link KtxTexture2#getSupercompressionScheme()} not
	 * being {@link KtxSupercmpScheme#NONE}, this will return the deflated size of the data.
	 *
	 * @return The data size
	 */
	public native long getDataSize();

	/**
	 * Return the total size in bytes of the uncompressed data of a texture.<br>
	 * <br>
	 * For a {@link KtxTexture1}, this always returns the value of {@link #getDataSize()}.<br>
	 * <br>
	 * For a {@link KtxTexture2}, if the {@link KtxTexture2#getSupercompressionScheme()} is
	 * {@link KtxSupercmpScheme#NONE} or {@link KtxSupercmpScheme#BASIS_LZ},
	 * returns the value of {@link #getDataSize()}.<br>
	 * <br>
	 * Otherwise, if the supercompression scheme is {@link KtxSupercmpScheme#ZSTD} or
	 * {@link KtxSupercmpScheme#ZLIB}, it returns the sum of the uncompressed sizes
	 * of each mip level plus space for the level padding.<br>
	 * <br>
	 * With no supercompression the data size and uncompressed data size are the same.
	 * For Basis supercompression the uncompressed size cannot be known until the data
	 * is transcoded so the compressed size is returned.
	 *
	 * @return The uncompressed data size
	 */
	public native long getDataSizeUncompressed();

	/**
	 * Return the size in bytes of an elements of a texture's images.<br>
	 * <br>
	 * For uncompressed textures an element is one texel. For compressed textures it is one block.
	 *
	 * @return The element size
	 */
	public native int getElementSize();

	/**
	 * Return pitch between rows of a texture image level in bytes.<br>
	 * <br>
	 * For uncompressed textures the pitch is the number of bytes between rows of texels.
	 * For compressed textures it is the number of bytes between rows of blocks. The value
	 * is padded to <code>GL_UNPACK_ALIGNMENT</code>, if necessary. For all currently known
	 * compressed formats padding will not be necessary.
	 *
	 * @param level The level
	 * @return The row pitch in bytes
	 */
	public native int getRowPitch(int level);

	/**
	 * Calculate and return the size in bytes of an image at the specified mip level.<br>
	 * <br>
	 * For arrays, this is the size of layer, for cubemaps, the size of a face and for
	 * 3D textures, the size of a depth slice.<br>
	 * <br>
	 * The size reflects the padding of each row to <code>KTX_GL_UNPACK_ALIGNMENT</code>.
	 *
	 * @param level The level
	 * @return THe size in bytes
	 */
	public native int getImageSize(int level);

	/**
	 * Find the offset of an image within a texture's image data.<br>
	 * <br>
	 * As there is no such thing as a 3D cubemap we make the 3rd location parameter
	 * do double duty. Only works for non-supercompressed textures as there is no
	 * way to tell where an image is for a supercompressed one.<br>
	 *
	 * @param level The MIP level of the image
	 * @param layer The array layer of the image
	 * @param faceSlice The cube map face or depth slice of the image.
	 * @return The offset, or -1 if the call internally caused a
	 * {@link KtxErrorCode#INVALID_OPERATION}. This means that the level,
	 * layer or faceSlice exceeded the dimensions of the texture,
	 * or that the texture is supercompressed.
	 */
	public native long getImageOffset(int level, int layer, int faceSlice);

	/**
	 * Create a GL texture object from a {@link KtxTexture} object.<br>
	 * <br>
	 * This may only be called when a GL context is current.<br>
	 * <br>
	 * In order to ensure that the GL uploader is not linked into an application 
	 * unless explicitly called, this is not a virtual function. It determines 
	 * the texture type then dispatches to the correct function.<br>
	 * <br>
	 * Sets the texture object's <code>GL_TEXTURE_MAX_LEVEL</code> parameter 
	 * according to the number of levels in the KTX data, provided the 
	 * context supports this feature.<br>
	 * <br>
	 * Unpacks compressed {@link KtxInternalformat#GL_ETC1_RGB8_OES} and 
	 * <code>GL_ETC2_*</code> format textures in software when the format 
	 * is not supported by the GL context, provided the library has been 
	 * compiled with <code>SUPPORT_SOFTWARE_ETC_UNPACK</code> defined as 1.
	 * 
	 * It will also convert textures with legacy formats to their modern 
	 * equivalents when the format is not supported by the GL context, 
	 * provided the library has been compiled with 
	 * <code>SUPPORT_LEGACY_FORMAT_CONVERSION</code> defined as 1.
	 * 
	 * @param texture An array that is either <code>null</code>, or 
	 * has a length of at least 1. It contains the name of the GL texture 
	 * object to load. If it is <code>null</code> or contains 0, the 
	 * function will generate a texture name. The function binds either 
	 * the generated name or the name given in <code>texture</code> to 
	 * the texture target returned in <code>target</code>, before 
	 * loading the texture data. If pTexture is not <code>null</code> 
	 * and a name was generated, the generated name will be returned 
	 * in <code>texture</code>.
	 * @param target An array with a length of at least 1, where
	 * element 0 will receive the GL target value
	 * @param glError An array with a length of at least 1, where
	 * element 0 will receive any GL error information
	 * @return {@link KtxErrorCode#SUCCESS} on sucess.
	 * Returns {@link KtxErrorCode#GL_ERROR} when GL error was raised by 
	 * <code>glBindTexture</code>, <code>glGenTextures</code> or 
	 * <code>gl*TexImage*</code> The GL error will be returned in 
	 * <code>glError</code> if <code>glError</code> is not 
	 * <code>null</code>. Returns {@link KtxErrorCode#INVALID_VALUE}
	 * when target is <code>null</code> or the size of a mip level 
	 * is greater than the size of the preceding level. Returns
	 * {@link KtxErrorCode#NOT_FOUND} when a dynamically loaded 
	 * OpenGL function required by the loader was not found.
	 * Returns {@link KtxErrorCode#UNSUPPORTED_TEXTURE_TYPE} when
	 * the type of texture is not supported by the current OpenGL context.
	 * @throws NullPointerException If the given <code>target</code>
	 * array is <code>null</code>
	 * @throws IllegalArgumentException Any array that is not 
	 * <code>null</code> has a length of 0
	 */
	public native int glUpload(int texture[], int target[], int glError[]);

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
	 * an error code that was not {@link KtxErrorCode#SUCCESS} in the underlying
	 * implementation.
	 * @throws UnsupportedOperationException If the resulting array would
	 * be larger than the maximum size of a Java array (i.e. more than 2GB)
	 */
	public native byte[] writeToMemory();
}
