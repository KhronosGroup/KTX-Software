/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.khronos.ktx.*;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;

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
}
