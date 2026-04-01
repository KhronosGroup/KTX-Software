# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from pyktx import *
from test_config import __test_images__
import unittest


class TestKtxTexture2(unittest.TestCase):
    def test_create_from_named_file(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/alpha_complex_straight.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file)

        self.assertEqual(texture.num_faces, 1)
        self.assertEqual(texture.num_layers, 1)
        self.assertEqual(texture.num_levels, 1)
        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_R8G8B8A8_SRGB)
        self.assertEqual(texture.base_width, 256)
        self.assertEqual(texture.base_height, 256)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

    def test_create_from_named_file_mipmapped(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/ktx_app_astc_8x8.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.NO_FLAGS)

        self.assertEqual(texture.vk_format, VkFormat.VK_FORMAT_ASTC_8x8_SRGB_BLOCK)
        self.assertEqual(texture.num_layers, 1)
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

        self.assertEqual(texture.color_model, KhrDfModel.RGBSDA)
        self.assertEqual(texture.is_compressed, False)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

        texture.compress_basis(1)

        self.assertEqual(texture.color_model, KhrDfModel.ETC1S)
        self.assertEqual(texture.is_compressed, True)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.BASIS_LZ)

    def test_compress_basis_with_params(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/r8g8b8a8_srgb_array_7_mip.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)

        self.assertEqual(texture.color_model, KhrDfModel.RGBSDA)
        self.assertEqual(texture.is_compressed, False)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

        texture.compress_basis(KtxBasisParams(codec=KtxBasisCodec.UASTC_LDR_4x4,uastc_rdo=True))

        self.assertEqual(texture.color_model, KhrDfModel.UASTC)
        self.assertEqual(texture.color_model, KhrDfModel.UASTC_LDR_4x4)
        self.assertEqual(texture.is_compressed, True)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

    def test_compress_basis_hdr(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/Desk_small_zstd_15.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)

        self.assertEqual(texture.color_model, KhrDfModel.RGBSDA)
        self.assertEqual(texture.is_compressed, False)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.NONE)

        texture.compress_basis(KtxBasisParams(codec=KtxBasisCodec.UASTC_HDR_6x6_INTERMEDIATE,
                                              uastc_hdr_lambda=100.,
                                              uastc_hdr_level=3))

        self.assertEqual(texture.color_model, KhrDfModel.UASTC_HDR_6x6)
        self.assertEqual(texture.is_compressed, True)
        self.assertEqual(texture.supercompression_scheme, KtxSupercmpScheme.UASTC_HDR_6x6_INTERMEDIATE)

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

    def test_queries(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/alpha_simple_blze.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)
        self.assertTrue(texture.needs_transcoding)
        self.assertTrue(texture.is_transcodable)
        self.assertFalse(texture.is_hdr);
        self.assertFalse(texture.premultipled_alpha)
        self.assertEqual(texture.color_model, KhrDfModel.ETC1S)
        self.assertEqual(texture.primaries, KhrDfPrimaries.BT709)
        self.assertEqual(texture.transfer_function, KhrDfTransfer.SRGB)

    def test_compress_astc(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/r8g8b8a8_srgb_array_7_mip.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)
        self.assertFalse(texture.is_compressed)
        self.assertEqual(texture.num_layers, 7)
        self.assertEqual(texture.transfer_function, KhrDfTransfer.SRGB)
        texture.compress_astc(KtxPackAstcQualityLevels.FAST)
        self.assertTrue(texture.is_compressed)
        self.assertEqual(texture.color_model, KhrDfModel.ASTC)
        self.assertEqual(texture.num_layers, 7)
        self.assertEqual(texture.transfer_function, KhrDfTransfer.SRGB)

    def test_compress_astc_with_params(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/r8g8b8a8_srgb_array_7_mip.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)
        self.assertFalse(texture.is_compressed)
        self.assertEqual(texture.num_layers, 7)
        self.assertEqual(texture.transfer_function, KhrDfTransfer.SRGB)
        texture.compress_astc(KtxAstcParams(quality_level=KtxPackAstcQualityLevels.FAST,
                                            block_dimension=KtxPackAstcBlockDimension.D8x8))
        self.assertTrue(texture.is_compressed)
        self.assertEqual(texture.color_model, KhrDfModel.ASTC)
        self.assertEqual(texture.num_layers, 7)
        self.assertEqual(texture.transfer_function, KhrDfTransfer.SRGB)

    def test_decode_astc(self):
        test_ktx_file = os.path.join(__test_images__, 'ktx2/astc_8x8_unorm_array_7.ktx2')
        texture = KtxTexture2.create_from_named_file(test_ktx_file, KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT)
        self.assertEqual(texture.color_model, KhrDfModel.ASTC)
        self.assertTrue(texture.is_compressed)
        self.assertTrue(texture.is_array)
        self.assertFalse(texture.is_hdr);
        self.assertFalse(texture.premultipled_alpha)
        self.assertEqual(texture.num_layers, 7)
        self.assertFalse(texture.needs_transcoding)
        texture.decode_astc();
        self.assertFalse(texture.is_compressed)
        self.assertEqual(texture.color_model, KhrDfModel.RGBSDA)
        self.assertEqual(texture.transfer_function, KhrDfTransfer.LINEAR)
        self.assertTrue(texture.is_array)
        self.assertFalse(texture.is_hdr);
        self.assertFalse(texture.premultipled_alpha)
        self.assertEqual(texture.num_layers, 7)
        self.assertFalse(texture.needs_transcoding)
