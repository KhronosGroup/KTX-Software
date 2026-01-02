# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from .gl_internalformat import GlInternalformat
from .ktx_error_code import KtxErrorCode, KtxError
from .ktx_texture import KtxTexture, KtxVersionMismatchError
from .ktx_texture_create_flag_bits import KtxTextureCreateFlagBits
from .ktx_texture_create_info import KtxTextureCreateInfo
from .ktx_texture_create_storage import KtxTextureCreateStorage
from pyktx.native import ffi, lib


class KtxTexture1(KtxTexture):
    """Class representing a KTX version 1 format texture."""

    @staticmethod
    def create(create_info: KtxTextureCreateInfo, storage_allocation: KtxTextureCreateStorage) -> 'KtxTexture1':
        """Create a new empty KtxTexture1."""

        result = lib.PY_ktxTexture1_Create(create_info.gl_internal_format.value,
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
            raise KtxError('ktxTexture1_Create', KtxErrorCode(result.error))

        return KtxTexture1(result.texture)

    @staticmethod
    def create_from_named_file(filename: str, create_flags: int = KtxTextureCreateFlagBits.NO_FLAGS) -> 'KtxTexture1':
        """Create a KtxTexture1 from a named KTX file according to the file contents."""

        result = lib.PY_ktxTexture_CreateFromNamedFile(filename.encode("ascii"), int(create_flags))

        if int(result.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture1_CreateFromNamedFile', KtxErrorCode(result.error))

        texture = KtxTexture1(result.texture)

        if texture.class_id != 1:
            raise KtxVersionMismatchError('The provided file ' + filename + ' is not a KTX1 file')

        return texture

    @property
    def gl_format(self) -> int:
        """
        Format of the texture data, e.g. GL_RGB.

        You can find all OpenGL formats here: <https://registry.khronos.org/OpenGL/api/GL/glcorearb.h>
        """
        return lib.PY_ktxTexture1_get_glFormat(self._ptr)

    @property
    def gl_internalformat(self) -> int:
        """
        Internal format of the texture data. See GlInternalformat.

        You can find all OpenGL internal formats here: <https://registry.khronos.org/OpenGL/api/GL/glext.h>
        """

        return GlInternalformat(lib.PY_ktxTexture1_get_glInternalformat(self._ptr))

    @property
    def gl_baseinternalformat(self) -> int:
        """
        Base format of the texture data, e.g., GL_RGB.

        You can find all OpenGL formats here: <https://registry.khronos.org/OpenGL/api/GL/glcorearb.h>
        """

        return lib.PY_ktxTexture1_get_glBaseInternalformat(self._ptr)

    @property
    def gl_type(self) -> int:
        """
        Type of the texture data, e.g, GL_UNSIGNED_BYTE.

        You can find all OpenGL data types here: <https://registry.khronos.org/OpenGL/api/GL/glcorearb.h>
        """
        return lib.PY_ktxTexture1_get_glType(self._ptr)
