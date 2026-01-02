# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from pyktx import *
from test_config import __test_images__
import unittest


class TestKtxTexture2(unittest.TestCase):
    def test_create_from_named_file(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_ldr_4x4_FlightHelmet_baseColor.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_levels, 1)
        self.assertEqual(texture.num_faces, 1)
        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_ASTC_4x4_SRGB_BLOCK)
        self.assertEqual(texture.base_width, 2048)
        self.assertEqual(texture.base_height, 2048)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

    def test_create_from_named_file_mipmapped(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_mipmap_ldr_4x4_posx.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.NO_FLAGS)

        self.assertEqual(texture.num_levels, 12)
        self.assertEqual(texture.base_width, 2048)
        self.assertEqual(texture.base_height, 2048)

    def test_get_image_size(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_mipmap_ldr_4x4_posx.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.image_size(0), 4194304)

    def test_get_image_offset(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_mipmap_ldr_4x4_posx.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        level11_offset = texture.image_offset(11, 0, 0)
        level0_offset = texture.image_offset(0, 0, 0)

        self.assertEqual(level11_offset, 0)
        self.assertEqual(level0_offset - level11_offset, 0x155790 - 0x220)

    def test_get_size(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_mipmap_ldr_4x4_posx.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_levels, 12)

        data_size = texture.data_size
        total_size = 0

        for i in range(0, 12):
            total_size += texture.image_size(i)

        self.assertEqual(total_size, data_size)

        data = texture.data()
        self.assertEqual(len(data), data_size)

    def test_get_data(self):
        test_ktx_file = os.path.join(__test_images__, 'astc_mipmap_ldr_4x4_posx.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_levels, 12)

        level0_length = texture.image_size(0)

        with open(test_ktx_file, 'rb') as file:
            data = texture.data()
            self.assertEqual(file.read()[-level0_length - 1:], data[-level0_length - 1:])

    def test_compress_basis(self):
        test_ktx_file = os.path.join(__test_images__, 'arraytex_7_mipmap_reference_u.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)

        self.assertEqual(texture.is_compressed, False)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

        texture.compress_basis(KtxBasisParams(quality_level=1))

        self.assertEqual(texture.is_compressed, True)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.BASIS_LZ)

    def test_transcode_basis(self):
        test_ktx_file = os.path.join(__test_images__, 'color_grid_basis.ktx2')
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
