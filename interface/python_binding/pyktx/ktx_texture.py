# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from ctypes import *
from .ktx_error_code import KtxErrorCode, KtxError
from .ktx_hash_list import KtxHashList
from pyktx.native import ffi, lib
from typing import Dict, Literal, Optional


class KtxVersionMismatchError(Exception):
    """Error thrown when reading a file with wrong KTX version."""

    pass


class KtxTexture:
    """
    Base class representing a texture.

    ktxTextures should be created only by one of the static factory methods,
    and these fields should be considered read-only.
    """

    def __init__(self, ptr: c_uint64):
        self._ptr = ptr
        self.__kv_data_head = KtxHashList(lib.PY_ktxTexture_get_kvDataHead(ptr))

    def __del__(self):
        lib.free(self._ptr)
        self._ptr = ffi.NULL

    @property
    def class_id(self) -> int:
        """Identify the class type. 1 for KtxTexture1, 2 for KtxTexture2."""

        return lib.PY_ktxTexture_get_classId(self._ptr)

    @property
    def is_array(self) -> bool:
        """true if the texture is an array texture, i.e, a GL_TEXTURE_*_ARRAY target is to be used."""

        return lib.PY_ktxTexture_get_isArray(self._ptr)

    @property
    def is_compressed(self) -> bool:
        """If the texture's format is a block compressed format."""

        return lib.PY_ktxTexture_get_isCompressed(self._ptr)

    @property
    def is_cubemap(self) -> bool:
        """
        True if the texture is a cubemap or cubemap array.
        """
        return lib.PY_ktxTexture_get_isCubemap(self._ptr)

    @property
    def generate_mipmaps(self) -> bool:
        """If mipmaps should be generated for the texture when uploading to graphics APIs."""

        return lib.PY_ktxTexture_get_generateMipmaps(self._ptr)

    @property
    def base_width(self) -> int:
        """Width of the texture's base level."""

        return lib.PY_ktxTexture_get_baseWidth(self._ptr)

    @property
    def base_height(self) -> int:
        """Height of the texture's base level."""

        return lib.PY_ktxTexture_get_baseHeight(self._ptr)

    @property
    def base_depth(self) -> int:
        """Depth of the texture's base level."""

        return lib.PY_ktxTexture_get_baseDepth(self._ptr)

    @property
    def num_dimensions(self) -> int:
        """Number of dimensions in the texture: 1, 2 or 3."""

        return lib.PY_ktxTexture_get_numDimensions(self._ptr)

    @property
    def num_levels(self) -> int:
        """Number of mip levels in the texture."""

        return lib.PY_ktxTexture_get_numLevels(self._ptr)

    @property
    def num_faces(self) -> int:
        """Number of faces: 6 for cube maps, 1 otherwise."""

        return lib.PY_ktxTexture_get_numFaces(self._ptr)

    @property
    def element_size(self) -> int:
        """The element size of the texture's images."""

        return lib.ktxTexture_GetElementSize(self._ptr)

    @property
    def kv_data_raw(self) -> Optional[c_buffer]:
        """
        The raw KV data buffer.

        This is available only if RAW_KVDATA_BIT was used in create-flag bits.
        """

        buffer = lib.PY_ktxTexture_get_kvData(self._ptr)

        if buffer == ffi.NULL:
            return None

        return ffi.buffer(buffer, lib.PY_ktxTexture_get_kvDataLen(self._ptr))

    @property
    def kv_data(self) -> KtxHashList:
        """
        The metadata stored in the texture as a hash-list.

        This is not available if SKIP_KVDATA_BIT was used in the create-flag bits.
        """

        return self.__kv_data_head

    @property
    def data_size(self) -> int:
        """The total size of the texture image data in bytes."""

        return lib.ktxTexture_GetDataSize(self._ptr)

    @property
    def data_size_uncompressed(self) -> int:
        """Byte length of the texture's uncompressed image data."""

        return lib.ktxTexture_GetDataSizeUncompressed(self._ptr)

    def row_pitch(self, level: int) -> int:
        """
        Return pitch between rows of a texture image level in bytes.

        For uncompressed textures the pitch is the number of bytes between
        rows of texels. For compressed textures it is the number of bytes
        between rows of blocks. The value is padded to GL_UNPACK_ALIGNMENT,
        if necessary. For all currently known compressed formats padding will
        not be necessary.
        """

        return lib.ktxTexture_GetRowPitch(self._ptr, level)

    def image_size(self, level: int) -> int:
        """
        Calculate & return the size in bytes of an image at the specified mip level.

        For arrays, this is the size of layer, for cubemaps, the size of a face and
        for 3D textures, the size of a depth slice.

        The size reflects the padding of each row to KTX_GL_UNPACK_ALIGNMENT.
        """

        return lib.ktxTexture_GetImageSize(self._ptr, level)

    def image_offset(self, level: int, layer: int, face_slice: int) -> int:
        """
        Find the offset of an image within a ktxTexture's image data.

        As there is no such thing as a 3D cubemap we make the 3rd location parameter
        do double duty.
        """

        data = lib.PY_ktxTexture_GetImageOffset(self._ptr, level, layer, face_slice)

        if int(data.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_GetImageOffset', KtxErrorCode(data.error))

        return data.offset

    def data(self) -> bytes:
        """Return a buffer holding the texture image data."""

        return ffi.buffer(lib.ktxTexture_GetData(self._ptr), self.data_size)

    def set_image_from_memory(self, level: int, layer: int, face_slice: int, data: bytes):
        """
        Set image for level, layer, faceSlice from an image in memory.

        Uncompressed images in memory are expected to have their rows tightly packed
        as is the norm for most image file formats. The copied image is padded as
        necessary to achieve the KTX-specified row alignment. No padding is done if the
        ktxTexture's is_compressed field is true. Level, layer, face_slice rather than offset
        are specified to enable some validation.
        """

        error = KtxErrorCode(lib.ktxTexture_SetImageFromMemory(self._ptr, level, layer, face_slice, data, len(data)))

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_SetImageFromMemory', KtxErrorCode(int(error)))

    def write_to_named_file(self, dst_name: str) -> None:
        """Save this texture to a named file in KTX format."""

        error = KtxErrorCode(lib.ktxTexture_WriteToNamedFile(self._ptr, dst_name.encode('ascii')))

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_WriteToNamedFile', KtxErrorCode(error))

    def write_to_native_memory(self) -> Dict[Literal["bytes", "size"], int]:
        """Write this KTX file to block of memory in KTX format. Recommended to use write_to_memory instead."""

        data = lib.PY_ktxTexture_WriteToMemory(self._ptr)

        if int(data.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_WriteToMemory', KtxErrorCode(data.error))

        return {"bytes": data.bytes, "size": data.size}

    def write_to_memory(self) -> bytes:
        """Write this KTX file to a buffer in KTX format."""

        data = self.write_to_native_memory()
        native_buffer = ffi.buffer(data.get('bytes'), data.get('size'))
        buffer = native_buffer[:]

        lib.free(data.get('bytes'))
        return buffer
