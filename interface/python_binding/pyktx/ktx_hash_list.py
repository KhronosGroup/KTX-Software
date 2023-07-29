# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from ctypes import c_buffer
from .ktx_error_code import KtxErrorCode, KtxError
from pyktx.native import ffi, lib
from typing import Dict, Optional


class KtxHashList:
    """Opaque handle to a ktxHashList implemented in C."""

    def __init__(self, ptr):
        self._ptr = ptr

    def add_kv_pair(self, key: str, value: bytes) -> None:
        """Add a key value pair to a hash list."""

        error = lib.ktxHashList_AddKVPair(self._ptr, key.encode('ascii'), len(value), value)

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxHashList_AddKVPair', KtxErrorCode(error))

    def delete_kv_pair(self, key: str) -> None:
        """Delete a key value pair in a hash list."""

        error = lib.ktxHashList_DeleteKVPair(self._ptr, key.encode('ascii'))

        if int(error) != KtxErrorCode.SUCCESS:
            raise KtxError('ktxHashList_DeleteKVPair', KtxErrorCode(error))

    def find_value(self, key: str) -> Optional[c_buffer]:
        """Looks up a key in a hash list and returns the value."""

        data = lib.PY_ktxHashList_FindValue(self._ptr, key.encode('ascii'))

        if data.error == KtxErrorCode.NOT_FOUND:
            return None
        if data.error != KtxErrorCode.SUCCESS:
            raise KtxError('ktxHashList_FindValue', KtxErrorCode(data.error))

        return ffi.buffer(data.bytes, data.size)

    def copy(self) -> Dict[str, bytes]:
        """Copy the hash list into a dict. This is recommended if you reading the hash list."""

        kv_data = {}
        entry = lib.PY_ktxHashList_get_listHead(self._ptr)

        while entry != ffi.NULL:
            key_data = lib.PY_ktxHashListEntry_GetKey(entry)
            value_data = lib.PY_ktxHashListEntry_GetValue(entry)

            if int(key_data.error) != KtxErrorCode.SUCCESS:
                raise KtxError('ktxHashListEntry_GetKey', KtxErrorCode(key_data.error))
            if int(value_data.error) != KtxErrorCode.SUCCESS:
                raise KtxError('ktxHashListEntry_GetValue', KtxErrorCode(value_data.error))

            key: str = ffi.buffer(key_data.bytes, key_data.size)[:].decode(encoding='utf-8')
            value = ffi.buffer(value_data.bytes, value_data.size)[:]

            if key.endswith('\x00'):
                key = key[:-1]

            kv_data[key] = value

            entry = lib.ktxHashList_Next(entry)

        return kv_data
