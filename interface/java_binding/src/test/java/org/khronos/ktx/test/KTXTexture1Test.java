/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.khronos.ktx.*;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.*;

@ExtendWith({ KTXTestLibraryLoader.class })
public class KTXTexture1Test {
    @Test
    public void testCreateFromNamedFile() {
        Path testKTXFile = Paths.get("")
                .resolve("../../tests/testimages/etc1.ktx")
                .toAbsolutePath()
                .normalize();

        KTXTexture1 texture = KTXTexture1.createFromNamedFile(testKTXFile.toString(),
                                                            KTXTextureCreateFlagBits.NO_FLAGS);

        assertNotNull(texture);
        assertEquals(texture.getGlInternalformat(), KTXInternalformat.GL_ETC1_RGB8_OES);
        assertEquals(texture.isArray(), false);
        assertEquals(texture.isCompressed(), true);
        assertEquals(texture.getGenerateMipmaps(), false);
        assertEquals(texture.getNumLevels(), 1);

        texture.destroy();
    }

    @Test
    public void testWriteToNamedFile() throws IOException {
        Path testKTXFile = Paths.get("")
                .resolve("../../tests/testimages/etc2-rgb.ktx")
                .toAbsolutePath()
                .normalize();
        File copyFile = File.createTempFile("copyktx", ".ktx");

        KTXTexture1 texture = KTXTexture1.createFromNamedFile(testKTXFile.toString(), KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
        assertNotNull(texture);

        int result = texture.writeToNamedFile(copyFile.getAbsolutePath().toString());
        assertEquals(result, KTXErrorCode.SUCCESS);

        byte[] original = Files.readAllBytes(testKTXFile);
        byte[] copy = Files.readAllBytes(copyFile.toPath());

        assertArrayEquals(copy, original);

        texture.destroy();
    }

    @Test
    public void testWriteToMemory() throws IOException {
        Path testKTXFile = Paths.get("")
                .resolve("../../tests/testimages/etc2-rgba1.ktx")
                .toAbsolutePath()
                .normalize();

        KTXTexture1 texture = KTXTexture1.createFromNamedFile(testKTXFile.toString(), KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
        assertNotNull(texture);

        byte[] file = Files.readAllBytes(testKTXFile);
        byte[] data = texture.writeToMemory();

        assertArrayEquals(file, data);

        texture.destroy();
    }

    @Test
    public void testGetData() throws IOException {
        Path testKTXFile = Paths.get("")
                .resolve("../../tests/testimages/etc2-rgba1.ktx")
                .toAbsolutePath()
                .normalize();

        KTXTexture1 texture = KTXTexture1.createFromNamedFile(testKTXFile.toString(), KTXTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT);
        assertNotNull(texture);

        byte[] data = texture.getData();
        byte[] file = Files.readAllBytes(testKTXFile);
        int level0Size = texture.getImageSize(0);

        for (int i = 0; i < level0Size; i++) {
            assertEquals(data[data.length - 1 - i], file[file.length - 1 - i]);
        }

        texture.destroy();
    }

    @Test
    public void testCreate() {
        KTXTextureCreateInfo info = new KTXTextureCreateInfo();

        info.setGlInternalformat(KTXInternalformat.GL_COMPRESSED_RGBA_ASTC_4x4_KHR);
        info.setBaseWidth(10);
        info.setBaseHeight(10);

        KTXTexture1 texture = KTXTexture1.create(info, KTXCreateStorage.ALLOC);
        assertNotNull(texture);

        byte[] imageData = new byte[10 * 10];
        texture.setImageFromMemory(0, 0, 0, imageData);

        texture.destroy();
    }
}
