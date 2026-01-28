# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from pyktx import *
from test_config import __test_images__
import unittest


class TestKtxTexture2(unittest.TestCase):
    def test_create_from_named_file(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/alpha_complex_straight.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_levels, 1)
        self.assertEqual(texture.num_faces, 1)
        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_R8G8B8A8_SRGB)
        self.assertEqual(texture.base_width, 256)
        self.assertEqual(texture.base_height, 256)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

    def test_create_from_named_file_mipmapped(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/ktx_app_astc_8x8.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.NO_FLAGS)

        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
        self.assertEqual(texture.num_levels, 11)
        self.assertEqual(texture.base_width, 1024)
        self.assertEqual(texture.base_height, 1024)

    def test_get_image_size(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/ktx_app_astc_8x8.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.image_size(0), 262144)

    def test_get_image_offset(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/ktx_app_astc_8x8.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        level10_offset = texture.image_offset(10, 0, 0)
        level0_offset = texture.image_offset(0, 0, 0)

        self.assertEqual(level10_offset, 0)
        self.assertEqual(level0_offset - level10_offset, 0x15750 -  0x1d0)

    def test_get_size(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/ktx_app_astc_8x8.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_levels, 11)

        data_size = texture.data_size
        total_size = 0

        for i in range(0, 11):
            total_size += texture.image_size(i)

        self.assertEqual(total_size, data_size)

        data = texture.data()
        self.assertEqual(len(data), data_size)

    def test_get_data(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/ktx_app_astc_8x8.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_levels, 11)

        level0_length = texture.image_size(0)

        with open(test_ktx_file, 'rb') as file:
            data = texture.data()
            self.assertEqual(file.read()[-level0_length - 1:], data[-level0_length - 1:])

    def test_compress_basis(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/r8g8b8a8_srgb_array_7_mip.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)

        self.assertEqual(texture.is_compressed, False)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

        texture.compress_basis(KtxBasisParams(quality_level=1))

        self.assertEqual(texture.is_compressed, True)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.BASIS_LZ)

    def test_transcode_basis(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/color_grid_blze.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)

        texture.transcode_basis(KtxTranscodeFmt.ASTC_4x4_RGBA, 0)

        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK)

    def test_create(self):
        info = KtxTextureCreateInfo(
            gl_internal_format=None,  # ignored
            base_width=10,
            base_height=10,
            base_depth=1,
            vk_format=VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK)

        texture = KtxTexture2.create(info, KtxTextureCreateStorage.ALLOC)
        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
        texture.set_image_from_memory(0, 0, 0, bytes(texture.data_size))
