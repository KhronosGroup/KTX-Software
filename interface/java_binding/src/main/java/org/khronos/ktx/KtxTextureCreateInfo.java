/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Structure for passing texture information to {@link KtxTexture1#create(KtxTextureCreateInfo, int)}
 * and {@link KtxTexture2#create(KtxTextureCreateInfo, int)}.
 */
public class KtxTextureCreateInfo {

	/**
	 * The internal format for the texture
	 */
	private int glInternalformat;

	/**
	 * Width of the base level of the texture.
	 */
	private int baseWidth = 0;

	/**
	 * Height of the base level of the texture.
	 */
	private int baseHeight = 0;

	/**
	 * Depth of the base level of the texture.
	 */
	private int baseDepth = 1;

	/**
	 * Number of dimensions in the texture.
	 */
	private int numDimensions = 2;

	/**
	 * Number of MIP levels in the texture.
	 */
	private int numLevels = 1;

	/**
	 * Number of array layers in the texture.
	 */
	private int numLayers = 1;

	/**
	 * The number of faces.
	 */
	private int numFaces = 1;

	/**
	 * Whether the texture is an array texture
	 */
	private boolean isArray = false;

	/**
	 * Whether mipmaps should be generated
	 */
	private boolean generateMipmaps = false;

	/**
	 * VkFormat for the texture
	 */
	private int vkFormat;

	/**
	 * Default constructor
	 */
	public KtxTextureCreateInfo() {
		// Nothing to do here
	}

	/**
	 * Returns the internal format for the texture
	 *
	 * See {@link #setGlInternalformat(int)}
	 *
	 * @return The internal format
	 */
	public int getGlInternalformat() {
		return this.glInternalformat;
	}

	/**
	 * Set the internal format for the texture.<br>
	 * <br>
	 * This is one of the constants from {@link KtxInternalformat}, for example
	 * {@link KtxInternalformat#GL_RGB8}. Ignored when creating a {@link KtxTexture2}.
	 *
	 * @param glInternalformat The internal format
	 */
	public void setGlInternalformat(int glInternalformat) {
		this.glInternalformat = glInternalformat;
	}

	/**
	 * Returns the width of the base level of the texture.
	 *
	 * @return The base width
	 */
	public int getBaseWidth() {
		return baseWidth;
	}

	/**
	 * Set the width of the base level of the texture.
	 *
	 * @param baseWidth The base width
	 */
	public void setBaseWidth(int baseWidth) {
		this.baseWidth = baseWidth;
	}

	/**
	 * Returns the height of the base level of the texture.
	 *
	 * @return The base height
	 */
	public int getBaseHeight() {
		return baseHeight;
	}

	/**
	 * Set the height of the base level of the texture.
	 *
	 * @param baseHeight The base height
	 */
	public void setBaseHeight(int baseHeight) {
		this.baseHeight = baseHeight;
	}

	/**
	 * Returns the depth of the base level of the texture.
	 *
	 * @return The base depth
	 */
	public int getBaseDepth() {
		return baseDepth;
	}

	/**
	 * Set the depth of the base level of the texture.
	 *
	 * @param baseDepth The base depth
	 */
	public void setBaseDepth(int baseDepth) {
		this.baseDepth = baseDepth;
	}

	/**
	 * Returns the number of dimensions in the texture.
	 *
	 * See {@link #setNumDimensions(int)}
	 *
	 * @return The number of dimensions
	 */
	public int getNumDimensions() {
		return numDimensions;
	}

	/**
	 * Set the number of dimensions in the texture.<br>
	 * <br>
	 * This may be 1, 2, or 3.
	 *
	 * @param numDimensions The number of dimensions
	 */
	public void setNumDimensions(int numDimensions) {
		this.numDimensions = numDimensions;
	}

	/**
	 * Returns the number of MIP levels in the texture.
	 *
	 * See {@link #setNumLevels(int)}
	 *
	 * @return The number of levels
	 */
	public int getNumLevels() {
		return numLevels;
	}

	/**
	 * Set the number of MIP levels in the texture.<br>
	 * <br>
	 * This should be 1 if {@link #isGenerateMipmaps()} is <code>true</code>.
	 *
	 * @param numLevels The number of levels
	 */
	public void setNumLevels(int numLevels) {
		this.numLevels = numLevels;
	}

	/**
	 * Returns the number of array layers in the texture.
	 *
	 * @return The number of layers
	 */
	public int getNumLayers() {
		return numLayers;
	}

	/**
	 * Set the number of array layers in the texture.
	 *
	 * @param numLayers The number of layers
	 */
	public void setNumLayers(int numLayers) {
		this.numLayers = numLayers;
	}

	/**
	 * Returns the number of faces.
	 *
	 * See {@link #setNumFaces(int)}
	 *
	 * @return The number of faces
	 */
	public int getNumFaces() {
		return numFaces;
	}

	/**
	 * Set the number of faces.<br>
	 * <br>
	 * This should be 6 for cube maps, 1 otherwise.
	 *
	 * @param numFaces The number of faces
	 */
	public void setNumFaces(int numFaces) {
		this.numFaces = numFaces;
	}

	/**
	 * Returns whether the texture is an array texture.
	 *
	 * See {@link #setArray(boolean)}
	 *
	 * @return Whether the texture is an array texture
	 */
	public boolean isArray() {
		return isArray;
	}

	/**
	 * Set whether the texture is an array texture.
	 *
	 * This means that OpenGL will use a <code>GL_TEXTURE_*_ARRAY</code> target.
	 *
	 * @param array Whether the texture is an array texture
	 */
	public void setArray(boolean array) {
		isArray = array;
	}

	/**
	 * Returns whether mipmaps should be generated.
	 *
	 * See {@link #setGenerateMipmaps(boolean)}
	 *
	 * @return Whether mipmaps should be generated
	 */
	public boolean isGenerateMipmaps() {
		return generateMipmaps;
	}

	/**
	 * Set whether mipmaps should be generated.<br>
	 * <br>
	 * This indicates whether mipmaps should be generated for the texture when loading into a 3D API.
	 *
	 * @param generateMipmaps Whether mipmaps should be generated
	 */
	public void setGenerateMipmaps(boolean generateMipmaps) {
		this.generateMipmaps = generateMipmaps;
	}

	/**
	 * Returns the {@link VkFormat} for the texture.
	 *
	 * See {@link #setVkFormat(int)}
	 *
	 * @return The {@link VkFormat}
	 */
	public int getVkFormat() {
		return this.vkFormat;
	}

	/**
	 * Set the {@link VkFormat} for the texture.<br>
	 * <br>
	 * Ignored when creating a {@link KtxTexture1}.
	 *
	 * @param vkFormat The {@link VkFormat}
	 */
	public void setVkFormat(int vkFormat) {
		this.vkFormat = vkFormat;
	}
}
