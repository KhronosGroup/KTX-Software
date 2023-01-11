# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from pyktx import *
from tempfile import NamedTemporaryFile
from test_config import __test_images__
import os
import unittest


class TestKtxTexture1(unittest.TestCase):
    def test_create_from_named_file(self):
        test_ktx_file = os.path.join(__test_images__, 'etc1.ktx')
        texture = KtxTexture1.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.gl_internalformat, GlInternalformat.ETC1_RGB8_OES)
        self.assertFalse(texture.is_array)
        self.assertFalse(texture.generate_mipmaps)
        self.assertEqual(texture.num_levels, 1)

    def test_write_to_named_file(self):
        test_ktx_file = os.path.join(__test_images__, 'etc2-rgb.ktx')
        copy_file = NamedTemporaryFile(delete=False)
        copy_file.close()

        texture = KtxTexture1.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)
        texture.write_to_named_file(copy_file.name)

        with open(test_ktx_file, 'rb') as original, open(copy_file.name, 'rb') as copy:
            self.assertEqual(original.read(), copy.read())

        os.unlink(copy_file.name)

    def test_write_to_memory(self):
        test_ktx_file = os.path.join(__test_images__, 'etc2-rgba1.ktx')
        texture = KtxTexture1.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)
        texture_bytes = texture.write_to_memory()

        with open(test_ktx_file, 'rb') as original:
            self.assertEqual(original.read(), texture_bytes)

    def test_get_data(self):
        test_ktx_file = os.path.join(__test_images__, 'etc2-rgba1.ktx')
        texture = KtxTexture1.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)

        data = texture.data()

        with open(test_ktx_file, 'rb') as file:
            level_0_size = texture.image_size(0)
            self.assertEqual(file.read()[-level_0_size:], data[:])

    def test_create(self):
        info = KtxTextureCreateInfo(
            gl_internal_format=GlInternalformat.COMPRESSED_RGBA_ASTC_4x4_KHR,
            base_width=10,
            base_height=10,
            base_depth=1)

        texture = KtxTexture1.create(info, KtxTextureCreateStorage.ALLOC)
        texture.set_image_from_memory(0, 0, 0, bytes(texture.data_size))
