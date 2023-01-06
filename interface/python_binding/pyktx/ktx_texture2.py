# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from .ktx_astc_params import KtxAstcParams
from .ktx_basis_params import KtxBasisParams
from .ktx_error_code import KtxErrorCode, KtxError
from .ktx_supercmp_scheme import KtxSupercmpScheme
from .ktx_texture import KtxTexture, KtxVersionMismatchError
from .ktx_texture_create_flag_bits import KtxTextureCreateFlagBits
from .ktx_texture_create_info import KtxTextureCreateInfo
from .ktx_texture_create_storage import KtxTextureCreateStorage
from .ktx_transcode_fmt import KtxTranscodeFmt
from pyktx.native import ffi, lib
from typing import Union
from .vk_format import VkFormat


class KtxTexture2(KtxTexture):
    @staticmethod
    def create(create_info: KtxTextureCreateInfo, storage_allocation: KtxTextureCreateStorage) -> 'KtxTexture2':
        result = lib.PY_ktxTexture2_Create(create_info.gl_internal_format.value,
                                           create_info.vk_format.value,
                                           ffi.NULL,
                                           create_info.base_width,
                                           create_info.base_height,
                                           create_info.base_depth,
                                           create_info.num_dimensions,
                                           create_info.num_levels,
                                           create_info.num_layers,
                                           create_info.num_faces,
                                           create_info.is_array,
                                           create_info.generate_mipmaps,
                                           storage_allocation.value)

        if int(result.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture2_Create', KtxErrorCode(result.error))

        return KtxTexture2(result.texture)

    @staticmethod
    def create_from_named_file(filename: str,
                               create_flags: int = KtxTextureCreateFlagBits.LOAD_IMAGE_DATA_BIT) -> 'KtxTexture2':
        result = lib.PY_ktxTexture_CreateFromNamedFile(filename.encode("ascii"), int(create_flags))

        if int(result.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture2_CreateFromNamedFile', KtxErrorCode(result.error))

        texture = KtxTexture2(result.texture)

        if texture.class_id != 2:
            raise KtxVersionMismatchError('The provided file ' + filename + ' is not a KTX2 file')

        return texture

    def __init__(self, ptr):
        super().__init__(ptr)

    @property
    def vk_format(self) -> VkFormat:
        return VkFormat(lib.PY_ktxTexture2_get_vkFormat(self._ptr))

    @property
    def supercompression_scheme(self) -> KtxSupercmpScheme:
        return KtxSupercmpScheme(lib.PY_ktxTexture2_get_supercompressionScheme(self._ptr))

    @property
    def oetf(self) -> int:
        return lib.ktxTexture2_GetOETF(self._ptr)

    @property
    def premultipled_alpha(self) -> bool:
        return lib.ktxTexture2_GetPremultipliedAlpha(self._ptr)

    @property
    def needs_transcoding(self) -> bool:
        return lib.ktxTexture2_NeedsTranscoding(self._ptr)

    def compress_astc(self, params: Union[int, KtxAstcParams]) -> None:
        if isinstance(params, int):
            quality = params
            params = KtxAstcParams()
            params.quality_level = quality

        error = lib.PY_ktxTexture2_CompressAstcEx(self._ptr,
                                                  params.verbose,
                                                  params.thread_count,
                                                  int(params.block_dimension),
                                                  int(params.mode),
                                                  params.quality_level,
                                                  params.normal_map,
                                                  params.perceptual,
                                                  params.input_swizzle)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture2_compressAstcEx', KtxErrorCode(error))

    def compress_basis(self, params: Union[int, KtxBasisParams]) -> None:
        if isinstance(params, int):
            quality = params
            params = KtxBasisParams()
            params.quality_level = quality

        error = lib.PY_ktxTexture2_CompressBasisEx(self._ptr,
                                                   params.uastc,
                                                   params.verbose,
                                                   params.no_sse,
                                                   params.thread_count,
                                                   params.compression_level,
                                                   params.quality_level,
                                                   params.max_endpoints,
                                                   params.endpoint_rdo_threshold,
                                                   params.max_selectors,
                                                   params.selector_rdo_threshold,
                                                   params.input_swizzle,
                                                   params.normal_map,
                                                   params.pre_swizzle,
                                                   params.separate_rg_to_rgb_a,
                                                   params.no_endpoint_rdo,
                                                   params.no_selector_rdo,
                                                   params.uastc_flags,
                                                   params.uastc_rdo,
                                                   params.uastc_rdo_quality_scalar,
                                                   params.uastc_rdo_dict_size,
                                                   params.uastc_rdo_max_smooth_block_error_scale,
                                                   params.uastc_rdo_max_smooth_block_std_dev,
                                                   params.uastc_rdo_dont_favor_simpler_modes,
                                                   params.uastc_rdo_no_multithreading)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture2_CompressBasisEx', KtxErrorCode(error))

    def deflate_zstd(self, compression_level: int) -> None:
        if not 1 <= compression_level <= 22:
            raise ValueError("compression_level must be between 1 and 22")

        error = lib.ktxTexture2_DeflateZstd(self._ptr, compression_level)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktx2_DeflateZstd', KtxErrorCode(error))

    def transcode_basis(self, output_format: KtxTranscodeFmt, transcode_flags: int = 0) -> None:
        error = lib.ktxTexture2_TranscodeBasis(self._ptr, output_format, transcode_flags)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktx2_TranscodeBasis', KtxErrorCode(error))
