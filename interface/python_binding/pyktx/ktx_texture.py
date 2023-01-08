# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from ctypes import *
from .ktx_error_code import KtxErrorCode, KtxError
from .ktx_hash_list import KtxHashList
from pyktx.native import ffi, lib
from typing import Dict, Literal, Optional


class KtxVersionMismatchError(Exception):
    pass


class KtxTexture:
    def __init__(self, ptr: c_uint64):
        self._ptr = ptr
        self.__kv_data_head = KtxHashList(lib.PY_ktxTexture_get_kvDataHead(ptr))

    def __del__(self):
        lib.free(self._ptr)
        self._ptr = ffi.NULL

    @property
    def class_id(self) -> int:
        return lib.PY_ktxTexture_get_classId(self._ptr)

    @property
    def is_array(self) -> bool:
        return lib.PY_ktxTexture_get_isArray(self._ptr)

    @property
    def is_cubemap(self) -> bool:
        return lib.PY_ktxTexture_get_isCubemap(self._ptr)

    @property
    def generate_mipmaps(self) -> bool:
        return lib.PY_ktxTexture_get_generateMipmaps(self._ptr)

    @property
    def base_width(self) -> int:
        return lib.PY_ktxTexture_get_baseWidth(self._ptr)

    @property
    def base_height(self) -> int:
        return lib.PY_ktxTexture_get_baseHeight(self._ptr)

    @property
    def base_depth(self) -> int:
        return lib.PY_ktxTexture_get_baseDepth(self._ptr)

    @property
    def num_dimensions(self) -> int:
        return lib.PY_ktxTexture_get_numDimensions(self._ptr)

    @property
    def num_levels(self) -> int:
        return lib.PY_ktxTexture_get_numLevels(self._ptr)

    @property
    def num_faces(self) -> int:
        return lib.PY_ktxTexture_get_numFaces(self._ptr)

    @property
    def element_size(self) -> int:
        return lib.ktxTexture_GetElementSize(self._ptr)

    @property
    def kv_data_raw(self) -> Optional[c_buffer]:
        buffer = lib.PY_ktxTexture_get_kvData(self._ptr)

        if buffer == ffi.NULL:
            return None

        return ffi.buffer(buffer, lib.PY_ktxTexture_get_kvDataLen(self._ptr))

    @property
    def kv_data(self) -> KtxHashList:
        return self.__kv_data_head

    @property
    def data_size(self) -> int:
        return lib.ktxTexture_GetDataSize(self._ptr)

    @property
    def data_size_uncompressed(self) -> int:
        return lib.ktxTexture_GetDataSizeUncompressed(self._ptr)

    def row_pitch(self, level: int) -> int:
        return lib.ktxTexture_GetRowPitch(self._ptr, level)

    def image_size(self, level: int) -> int:
        return lib.ktxTexture_GetImageSize(self._ptr, level)

    def image_offset(self, level: int, layer: int, face_slice: int) -> int:
        data = lib.PY_ktxTexture_GetImageOffset(self._ptr, level, layer, face_slice)

        if int(data.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_GetImageOffset', KtxErrorCode(data.error))

        return data.offset

    def data(self) -> bytes:
        return ffi.buffer(lib.ktxTexture_GetData(self._ptr), self.data_size)

    def set_image_from_memory(self, level: int, layer: int, face_slice: int, data: bytes):
        error = KtxErrorCode(lib.ktxTexture_SetImageFromMemory(self._ptr, level, layer, face_slice, data, len(data)))

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_SetImageFromMemory', KtxErrorCode(int(error)))

    def write_to_named_file(self, dst_name: str) -> None:
        error = KtxErrorCode(lib.ktxTexture_WriteToNamedFile(self._ptr, dst_name.encode('ascii')))

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_WriteToNamedFile', KtxErrorCode(error))

    def write_to_native_memory(self) -> Dict[Literal["bytes", "size"], int]:
        data = lib.PY_ktxTexture_WriteToMemory(self._ptr)

        if int(data.error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxTexture_WriteToMemory', KtxErrorCode(data.error))

        return {"bytes": data.bytes, "size": data.size}

    def write_to_memory(self) -> bytes:
        data = self.write_to_native_memory()
        native_buffer = ffi.buffer(data.get('bytes'), data.get('size'))
        buffer = native_buffer[:]

        lib.free(data.get('bytes'))
        return buffer
