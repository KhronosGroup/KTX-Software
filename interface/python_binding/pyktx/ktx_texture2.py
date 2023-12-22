# Copyright (c) 2023, Shukant Pal and Contributors
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
    """Class representing a KTX version 2 format texture."""

    @staticmethod
    def create(create_info: KtxTextureCreateInfo, storage_allocation: KtxTextureCreateStorage) -> 'KtxTexture2':
        """Create a new empty KtxTexture2."""

        result = lib.PY_ktxTexture2_Create(0,
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
        """Create a KtxTexture2 from a named KTX file."""

        result = lib.PY_ktxTexture_CreateFromNamedFile(filename.encode("ascii"), int(create_flags))

        if int(result.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture2_CreateFromNamedFile', KtxErrorCode(result.error))

        texture = KtxTexture2(result.texture)

        if texture.class_id != 2:
            raise KtxVersionMismatchError('The provided file ' + filename + ' is not a KTX2 file')

        return texture

    @property
    def vk_format(self) -> VkFormat:
        """VkFormat for texture."""

        return VkFormat(lib.PY_ktxTexture2_get_vkFormat(self._ptr))

    @property
    def supercompression_scheme(self) -> KtxSupercmpScheme:
        """The supercompression scheme used to compress the texture data."""

        return KtxSupercmpScheme(lib.PY_ktxTexture2_get_supercompressionScheme(self._ptr))

    @property
    def oetf(self) -> int:
        """The opto-electrical transfer function of the images."""

        return lib.ktxTexture2_GetOETF(self._ptr)

    @property
    def premultipled_alpha(self) -> bool:
        """Whether the RGB components have been premultiplied by the alpha component."""

        return lib.ktxTexture2_GetPremultipliedAlpha(self._ptr)

    @property
    def needs_transcoding(self) -> bool:
        """If the images are in a transcodable format."""

        return lib.ktxTexture2_NeedsTranscoding(self._ptr)

    def compress_astc(self, params: Union[int, KtxAstcParams]) -> None:
        """
        Encode and compress a ktx texture with uncompressed images to ASTC.

        The images are either encoded to ASTC block-compressed format. The
        encoded images replace the original images and the texture's fields
        including the dfd are modified to reflect the new state.

        Such textures can be directly uploaded to a GPU via a graphics API.
        """

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
        """
        Supercompress a KTX2 texture with uncompressed images.

        The images are either encoded to ETC1S block-compressed format and supercompressed
        with Basis LZ or they are encoded to UASTC block-compressed format. UASTC format is
        selected by setting the uastc field of params to true. The encoded images replace
        the original images and the texture's fields including the DFD are modified to reflect
        the new state.  Such textures must be transcoded to a desired target block compressed
        format before they can be uploaded to a GPU via a graphics API.
        """

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
        """
        Deflate the data in a ktxTexture2 object using Zstandard.

        The texture's level_index, data_size, dfd and supercompression_scheme will all
        be updated after successful deflation to reflect the deflated data.
        """

        if not 1 <= compression_level <= 22:
            raise ValueError("compression_level must be between 1 and 22")

        error = lib.ktxTexture2_DeflateZstd(self._ptr, compression_level)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktx2_DeflateZstd', KtxErrorCode(error))

    def transcode_basis(self, output_format: KtxTranscodeFmt, transcode_flags: int = 0) -> None:
        """
        Transcode a KTX2 texture with BasisLZ/ETC1S or UASTC images.

        If the texture contains BasisLZ supercompressed images, inflates them from back to
        ETC1S then transcodes them to the specified block-compressed format. If the texture
        contains UASTC images, inflates them, if they have been supercompressed with zstd, then
        transcodes then to the specified format, The transcoded images replace the original images
        and the texture's fields including the dfd are modified to reflect the new format.

        These types of textures must be transcoded to a desired target block-compressed format
        before they can be uploaded to a GPU via a graphics API.

        The following block compressed transcode targets (KtxTranscodeFmt) are available: ETC1_RGB,
        ETC2_RGBA, BC1_RGB, BC3_RGBA, BC4_R, BC5_RG, BC7_RGBA, PVRTC1_4_RGB, PVRTC1_4_RGBA,
        PVRTC2_4_RGB, PVRTC2_4_RGBA, ASTC_4x4_RGBA, ETC2_EAC_R11, ETC2_EAC_RG11, ETC and BC1_OR_3.

        ETC automatically selects between ETC1_RGB and ETC2_RGBA according to whether an alpha
        channel is available. BC1_OR_3 does likewise between BC1_RGB and BC3_RGBA. Note that if
        PVRTC1_4_RGBA or PVRTC2_4_RGBA is specified and there is no alpha channel PVRTC1_4_RGB
        or PVRTC2_4_RGB respectively will be selected.

        Transcoding to ATC & FXT1 formats is not supported by libktx as there are no equivalent Vulkan formats.

        The following uncompressed transcode targets are also available: RGBA32, RGB565, BGR565 and RGBA4444.
        """

        error = lib.ktxTexture2_TranscodeBasis(self._ptr, output_format, transcode_flags)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktx2_TranscodeBasis', KtxErrorCode(error))
