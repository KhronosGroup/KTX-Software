/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.khronos.ktx.KtxBasisParams;
import org.khronos.ktx.KtxTextureCreateStorage;
import org.khronos.ktx.KtxErrorCode;
import org.khronos.ktx.KtxException;
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
    public void testCreateWithInvalidParameters() {

        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(128);
        info.setBaseHeight(128);
        info.setNumDimensions(-123); // Invalid!
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);

        assertThrows(KtxException.class,
                () -> KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE),
                "Expected to throw NullPointerException");
    }

    @Test
    public void testCreateFromMemoryBasic() {

        // Create a texture, and write it to memory
        int sizeX = 512;
        int sizeY = 512;
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(sizeX);
        info.setBaseHeight(sizeY);
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
        KtxTexture2 input = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);
        byte[] inputMemoryArray = input.writeToMemory();

        // Create the texture from the exact memory
        int createFlags = KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT;
        KtxTexture2 t = KtxTexture2.createFromMemory(ByteBuffer.wrap(inputMemoryArray), createFlags);

        // Ensure that the image has the same basic properties
        // as the one that it was created from
        assertEquals(t.getBaseWidth(), 512);
        assertEquals(t.getBaseHeight(), 512);
    }

    @Test
    public void testCreateFromMemoryWithPositionAndLimit() {

        // Create a texture, and write it to memory
        int sizeX = 512;
        int sizeY = 512;
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(sizeX);
        info.setBaseHeight(sizeY);
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
        KtxTexture2 input = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);
        byte[] inputMemoryArray = input.writeToMemory();

        // Create a byte buffer that is a bit larger than
        // the input memory, and put the input memory
        // into it, at position 50
        ByteBuffer largeBuffer = ByteBuffer.wrap(new byte[inputMemoryArray.length + 100]);
        largeBuffer.position(50);
        largeBuffer.put(inputMemoryArray);

        // Set the position and limit of the buffer to
        // reflect the range that actually contains
        // the real input data
        largeBuffer.position(50);
        largeBuffer.limit(50 + inputMemoryArray.length);

        // Create the texture from the exact memory
        int createFlags = KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT;
        KtxTexture2 t = KtxTexture2.createFromMemory(largeBuffer, createFlags);

        // Ensure that the image has the same basic properties
        // as the one that it was created from
        assertEquals(t.getBaseWidth(), 512);
        assertEquals(t.getBaseHeight(), 512);
    }

    @Test
    public void testCreateFromDirectMemoryWithPositionAndLimit() {

        // Create a texture, and write it to memory
        int sizeX = 512;
        int sizeY = 512;
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(sizeX);
        info.setBaseHeight(sizeY);
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
        KtxTexture2 input = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);
        byte[] inputMemoryArray = input.writeToMemory();

        // Create a DIRECT byte buffer that is a bit larger than
        // the input memory, and put the input memory
        // into it, at position 50
        ByteBuffer largeBuffer = ByteBuffer.allocateDirect(inputMemoryArray.length + 100);
        largeBuffer.position(50);
        largeBuffer.put(inputMemoryArray);

        // Set the position and limit of the buffer to
        // reflect the range that actually contains
        // the real input data
        largeBuffer.position(50);
        largeBuffer.limit(50 + inputMemoryArray.length);

        // Create the texture from the exact memory
        int createFlags = KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT;
        KtxTexture2 t = KtxTexture2.createFromMemory(largeBuffer, createFlags);

        // Ensure that the image has the same basic properties
        // as the one that it was created from
        assertEquals(t.getBaseWidth(), 512);
        assertEquals(t.getBaseHeight(), 512);
    }



    @Test
    public void testCreateFromNamedFileWithNull() {

        assertThrows(NullPointerException.class,
                () -> KtxTexture2.createFromNamedFile(null, KtxTextureCreateFlagBits.NO_FLAGS),
                "Expected to throw NullPointerException");
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
    public void testUsingAfterDestroy() {
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(16);
        info.setBaseHeight(16);
        info.setVkFormat(VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
        KtxTexture2 texture = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);

        // Call destroy, and then try to call a function.
        // It should throw for ALL functions, and there
        // should be a test for EACH function, but...
        // I got stuff to do, you know...
        texture.destroy();
        assertThrows(IllegalStateException.class,
                () -> texture.getBaseDepth(),
                "Expected to throw IllegalStateException");

    }

    @Test
    public void testDestroyMultipleTimes() {
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(16);
        info.setBaseHeight(16);
        info.setVkFormat(VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
        KtxTexture2 texture = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);

        texture.destroy();
        texture.destroy();
        texture.destroy();

        assertTrue(true, "Should be able to call destroy() multiple times");
    }


    @Test
    public void testCompressBasisExWithNull() {
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(16);
        info.setBaseHeight(16);
        info.setVkFormat(VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK);
        KtxTexture2 texture = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);

        assertThrows(NullPointerException.class,
                () -> texture.compressBasisEx(null),
                "Expected to throw NullPointerException");

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

        KtxTexture2 texture = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);
        assertNotNull(texture);

        byte[] imageData = new byte[10 * 10];
        texture.setImageFromMemory(0, 0, 0, imageData);

        texture.destroy();
    }

    @Test
    public void testCreateWithNull() {
        assertThrows(NullPointerException.class,
                () -> KtxTexture2.create(null, KtxTextureCreateStorage.ALLOC_STORAGE),
                "Expected to throw NullPointerException");
    }

    @Test
    void testSetImageFromMemoryWithNull() {
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(16);
        info.setBaseHeight(16);
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
        KtxTexture2 t = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);

        assertThrows(NullPointerException.class,
                () -> t.setImageFromMemory(0, 0, 0, null),
                "Expected to throw NullPointerException");
    }

    @Test
    void testCreateFromMemoryWithInvalidMemory() {

        byte[] invalidMemory = new byte[1000];
        int createFlags = KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT;

        assertThrows(KtxException.class,
                () -> KtxTexture2.createFromMemory(ByteBuffer.wrap(invalidMemory), createFlags),
                "Expected to throw NullPointerException");
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
        KtxTexture2 inputTexture = KtxTexture2.create(inputInfo,
        		KtxTextureCreateStorage.ALLOC_STORAGE);
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
        KtxTexture2 goldTexture = KtxTexture2.create(goldInfo,
        		KtxTextureCreateStorage.ALLOC_STORAGE);
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

    @Test
    public void testSupercompressionZstd() throws IOException {

        int sizeX = 32;
        int sizeY = 32;

        // Create a dummy texture
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(sizeX);
        info.setBaseHeight(sizeY);
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
        KtxTexture2 t = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);
        byte[] rgba = new byte[sizeX * sizeY * 4];
        t.setImageFromMemory(0, 0, 0, rgba);

        // Apply default UASTC compression
        KtxBasisParams p = new KtxBasisParams();
        p.setUastc(true);
        t.compressBasisEx(p);

        // The supercompression scheme should be NONE here
        int scBefore = t.getSupercompressionScheme();
        assertEquals(KtxSupercmpScheme.NONE, scBefore);

        // Apply Zstd compression
        t.deflateZstd(10);

        // The supercompression scheme should now be ZSTD
        int scAfter = t.getSupercompressionScheme();
        assertEquals(KtxSupercmpScheme.ZSTD, scAfter);

        t.destroy();
    }

    @Test
    public void testSupercompressionZLIB() throws IOException {

        int sizeX = 32;
        int sizeY = 32;

        // Create a dummy texture
        KtxTextureCreateInfo info = new KtxTextureCreateInfo();
        info.setBaseWidth(sizeX);
        info.setBaseHeight(sizeY);
        info.setVkFormat(VkFormat.VK_FORMAT_R8G8B8A8_SRGB);
        KtxTexture2 t = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC_STORAGE);
        byte[] rgba = new byte[sizeX * sizeY * 4];
        t.setImageFromMemory(0, 0, 0, rgba);

        // Apply default UASTC compression
        KtxBasisParams p = new KtxBasisParams();
        p.setUastc(true);
        t.compressBasisEx(p);

        // The supercompression scheme should be NONE here
        int scBefore = t.getSupercompressionScheme();
        assertEquals(KtxSupercmpScheme.NONE, scBefore);

        // Apply ZLIB compression
        t.deflateZLIB(10);

        // The supercompression scheme should now be ZLIB
        int scAfter = t.getSupercompressionScheme();
        assertEquals(KtxSupercmpScheme.ZLIB, scAfter);

        t.destroy();
    }

    @Test
    public void testBindings() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_ldr_4x4_FlightHelmet_baseColor.ktx2")
                .toAbsolutePath()
                .normalize();

        // The purpose of this test is to check the bindings for the 'native'
        // functions that only return a value. When the binding for one of
        // these functions is not implemented properly, then trying to call
        // it will cause an 'UnsatisfiedLinkError'.
        // This does not cover all 'native' functions: Some of them can only
        // sensibly be called in the context of the other tests.

        KtxTexture2 texture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.NO_FLAGS);

        // Native getter methods from the 'KtxTexture2' class
        texture.getOETF();
        texture.getPremultipliedAlpha();
        texture.needsTranscoding();
        texture.getSupercompressionScheme();
        texture.getVkFormat();

        // Native getter methods from the 'KtxTexture' class
        texture.isArray();
        texture.isCubemap();
        texture.isCompressed();
        texture.getGenerateMipmaps();
        texture.getBaseWidth();
        texture.getBaseHeight();
        texture.getBaseDepth();
        texture.getNumDimensions();
        texture.getNumLevels();
        texture.getNumFaces();
        texture.getDataSize();
        texture.getDataSizeUncompressed();
        texture.getElementSize();
        texture.getRowPitch(0);
        texture.getImageSize(0);
    }

    @Test
    public void testGlUpload() {
        Path testKtxFile = Paths.get("")
                .resolve("../../tests/testimages/astc_ldr_4x4_FlightHelmet_baseColor.ktx2")
                .toAbsolutePath()
                .normalize();

        KtxTexture2 ktxTexture = KtxTexture2.createFromNamedFile(testKtxFile.toString(),
                KtxTextureCreateFlagBits.NO_FLAGS);
        ktxTexture.transcodeBasis(KtxTranscodeFormat.BC1_RGB, 0);

        // This test checks the error conditions that are supposed
        // to be handled by the JNI layer by throwing exceptions.
        // The test can NOT perform an actual, "valid" call that
        // causes ktxTexture_GLUpload to be called internally,
        // because that would require a GL context to be current.
        int texture0[] = { };
        int target0[] = { };
        int glError0[] = { };
        int texture1[] = { 0 };
        int target1[] = { 0 };
        int glError1[] = { 0 };

        // Expect NullPointerException when target is null
        assertThrows(NullPointerException.class,
            () -> {
                ktxTexture.glUpload(texture1, null, glError1);
            },
            "Expected to throw NullPointerException");

        // Expect IllegalArgumentException when texture length is 0
        assertThrows(IllegalArgumentException.class,
            () -> {
                ktxTexture.glUpload(texture0, target1, glError1);
            },
            "Expected to throw NullPointerException");

        // Expect IllegalArgumentException when target length is 0
        assertThrows(IllegalArgumentException.class,
            () -> {
                ktxTexture.glUpload(texture1, target0, glError1);
            },
            "Expected to throw NullPointerException");

        // Expect IllegalArgumentException when glError length is 0
        assertThrows(IllegalArgumentException.class,
            () -> {
                ktxTexture.glUpload(texture1, target1, glError0);
            },
            "Expected to throw NullPointerException");

        ktxTexture.destroy();
    }

}
