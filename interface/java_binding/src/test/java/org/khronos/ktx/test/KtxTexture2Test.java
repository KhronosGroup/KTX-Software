/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.khronos.ktx.KtxBasisParams;
import org.khronos.ktx.KtxCreateStorage;
import org.khronos.ktx.KtxErrorCode;
import org.khronos.ktx.KtxSupercmpScheme;
import org.khronos.ktx.KtxTexture2;
import org.khronos.ktx.KtxTextureCreateFlagBits;
import org.khronos.ktx.KtxTextureCreateInfo;
import org.khronos.ktx.KtxTranscodeFormat;
import org.khronos.ktx.VkFormat;

@ExtendWith({ KtxTestLibraryLoader.class })
public class KtxTexture2Test {

    @Test
    public void testCreateFromNamedFile() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_ldr_4x4_FlightHelmet_baseColor.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                                                            KtxTextureCreateFlagBits.NO_FLAGS);

        assertNotNull(texture);
        assertEquals(texture.getNumLevels(), 1);
        assertEquals(texture.getNumFaces(), 1);
        assertEquals(texture.getVkFormat(), VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
        assertEquals(texture.getBaseWidth(), 2048);
        assertEquals(texture.getBaseHeight(), 2048);
        assertEquals(texture.getSupercompressionScheme(), KtxSupercmpScheme.NONE);

        texture.destroy();
    }

    @Test
    public void testCreateFromNamedFileMipmapped() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_mipmap_ldr_4x4_posx.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.NO_FLAGS);

        assertNotNull(texture);
        assertEquals(texture.getNumLevels(), 12);
        assertEquals(texture.getBaseWidth(), 2048);
        assertEquals(texture.getBaseHeight(), 2048);

        texture.destroy();
    }

    @Test
    public void testGetImageSize() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_mipmap_ldr_4x4_posx.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.NO_FLAGS);

        assertNotNull(texture);
        assertEquals( 4194304, texture.getImageSize(0));

        texture.destroy();
    }

    @Test
    public void testGetImageOffset() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_mipmap_ldr_4x4_posx.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.NO_FLAGS);

        assertNotNull(texture);

        long level11Offset = texture.getImageOffset(11, 0, 0);
        long level0Offset = texture.getImageOffset(0, 0, 0);

        assertEquals(level11Offset, 0);
        // ktxinfo offsets are from start of file :)
        assertEquals(level0Offset - level11Offset, 0x155790 -  0x220);

        texture.destroy();
    }

    @Test
    public void testGetSize() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_mipmap_ldr_4x4_posx.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);

        assertNotNull(texture);
        assertEquals(texture.getNumLevels(), 12);

        long dataSize = texture.getDataSize();
        long totalSize = 0;

        for (int i = 0; i < 12; i++) {
            totalSize += texture.getImageSize(i);
        }

        assertEquals(totalSize, dataSize);

        byte[] data = texture.getData();

        assertEquals(data.length, dataSize);

        texture.destroy();
    }

    @Test
    public void testGetData() throws IOException {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_mipmap_ldr_4x4_posx.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);

        assertNotNull(texture);
        assertEquals(texture.getNumLevels(), 12);

        byte[] file = Files.readAllBytes(testKtxFile);
        byte[] data = texture.getData();
        int level0Length = texture.getImageSize(0);

        for (int i = 0; i < level0Length; i++) {
            assertEquals(file[file.length - i - 1], data[data.length - i - 1]);
        }

        texture.destroy();
    }

    @Test
    public void testCompressBasis() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/arraytex_7_mipmap_reference_u.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);

        assertNotNull(texture);
        assertEquals(false, texture.isCompressed());
        assertEquals(KtxSupercmpScheme.NONE, texture.getSupercompressionScheme());

        assertEquals(KtxErrorCode.SUCCESS, texture.compressBasis(1));

        assertEquals(true, texture.isCompressed());
        assertEquals(KtxSupercmpScheme.BASIS_LZ, texture.getSupercompressionScheme());

        texture.destroy();
    }

    @Test
    public void testCompressBasisEx() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/arraytex_7_mipmap_reference_u.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);

        assertNotNull(texture);
        assertEquals(false, texture.isCompressed());
        assertEquals(KtxSupercmpScheme.NONE, texture.getSupercompressionScheme());

        assertEquals(KtxErrorCode.SUCCESS, texture.compressBasisEx(new KtxBasisParams()));

        assertEquals(true, texture.isCompressed());
        assertEquals(KtxSupercmpScheme.BASIS_LZ, texture.getSupercompressionScheme());

        texture.destroy();
    }

    @Test
    public void testTranscodeBasis() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/color_grid_basis.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);

        assertNotNull(texture);

        texture.transcodeBasis(KtxTranscodeFormat.ASTC_4x4_RGBA, 0);

        assertEquals(VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK, texture.getVkFormat());
    }

    @Test
    public void testCreate() {
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();

        info.setBaseWidth(10);
        info.setBaseHeight(10);
        info.setVkFormat(VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK);

        KtxTexture2 texture = KtxTexture2.create(info, KtxCreateStorage.ALLOC);
        assertNotNull(texture);

        byte[] imageData = new byte[10 * 10];
        texture.setImageFromMemory(0, 0, 0, imageData);

        texture.destroy();
    }

	@Test
	public void testInputSwizzleBasisEx() throws IOException {

		int sizeX = 32;
		int sizeY = 32;
		int outputFormat = KtxTranscodeFormat.RGBA32;
		int transcodeFlags = 0;

		// Create the actual texture data:
		// - create RGBA pixels
		// - create texture
		// - compress with BRGA input swizzling
		// - obtain resulting RGBA values

		// Create a RGBA pixels for an image filled with
		// 8 rows of red pixels
		// 8 rows of green pixels
		// 8 rows of blue pixels
		// 8 rows of white pixels
		byte[] input = new byte[sizeX * sizeY * 4];
		TestUtils.fillRows(input, sizeX, sizeY, 0, 8, 255, 0, 0, 255); // Red
		TestUtils.fillRows(input, sizeX, sizeY, 8, 16, 0, 255, 0, 255); // Green
		TestUtils.fillRows(input, sizeX, sizeY, 16, 24, 0, 0, 255, 255); // Blue
		TestUtils.fillRows(input, sizeX, sizeY, 24, 32, 255, 255, 255, 255); // White

		// Create the input texture from the pixels
		KtxTextureCreateInfo inputInfo = new KtxTextureCreateInfo();
		inputInfo.setBaseWidth(sizeX);
		inputInfo.setBaseHeight(sizeY);
		inputInfo.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
		KtxTexture2 inputTexture = KtxTexture2.create(inputInfo, KtxCreateStorage.ALLOC);
		inputTexture.setImageFromMemory(0, 0, 0, input);

		// Apply basis compression to the input, with an input swizzle BRGA,
		// so that
		// the former B channel becomes the R channel
		// the former R channel becomes the G channel
		// the former G channel becomes the B channel
		// the former A channel remains the A channel
		KtxBasisParams inputParams = new KtxBasisParams();
		inputParams.setUastc(false);
		inputParams.setInputSwizzle(new char[] { 'b', 'r', 'g', 'a' });
		inputTexture.compressBasisEx(inputParams);

		// Transcode the input texture to RGBA32
		inputTexture.transcodeBasis(outputFormat, transcodeFlags);
		byte[] actualRgba = inputTexture.getData();

		// Create the expected reference data:
		// - create RGBA pixels, swizzled with BRGA
		// - create texture
		// - compress without input swizzling
		// - obtain resulting RGBA values

		// Create "golden" reference pixels, where a BRGA
		// swizzling was already applied
		byte[] gold = new byte[sizeX * sizeY * 4];
		TestUtils.fillRows(gold, sizeX, sizeY, 0, 8, 0, 255, 0, 255); // Green
		TestUtils.fillRows(gold, sizeX, sizeY, 8, 16, 0, 0, 255, 255); // Blue
		TestUtils.fillRows(gold, sizeX, sizeY, 16, 24, 255, 0, 0, 255); // Red
		TestUtils.fillRows(gold, sizeX, sizeY, 24, 32, 255, 255, 255, 255); // White

		// Create the reference texture from the swizzled pixels
		KtxTextureCreateInfo goldInfo = new KtxTextureCreateInfo();
		goldInfo.setBaseWidth(sizeX);
		goldInfo.setBaseHeight(sizeY);
		goldInfo.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
		KtxTexture2 goldTexture = KtxTexture2.create(goldInfo, KtxCreateStorage.ALLOC);
		goldTexture.setImageFromMemory(0, 0, 0, gold);

		// Apply basis compression to the reference, without swizzling
		KtxBasisParams goldParams = new KtxBasisParams();
		goldParams.setUastc(false);
		goldTexture.compressBasisEx(goldParams);

		// Transcode the reference texture to RGBA32
		goldTexture.transcodeBasis(outputFormat, transcodeFlags);
		byte[] expectedRgba = goldTexture.getData();

		// Compare the resulting data to the expected RGBA values.
		assertArrayEquals(expectedRgba, actualRgba);

		inputTexture.destroy();
		goldTexture.destroy();
	}
}

