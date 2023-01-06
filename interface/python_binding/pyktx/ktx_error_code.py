# Copyright (c) 2021, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxErrorCode(IntEnum):
    SUCCESS = 0
    FILE_DATA_ERROR = 1
    FILE_ISPIPE = 2
    FILE_OPEN_FAILED = 3
    FILE_OVERFLOW = 4
    FILE_READ_ERROR = 5
    FILE_SEEK_ERROR = 6
    FILE_UNEXPECTED_EOF = 7
    FILE_WRITE_ERROR = 8
    GL_ERROR = 9
    INVALID_OPERATION = 10
    INVALID_VALUE = 11
    NOT_FOUND = 12
    OUT_OF_MEMORY = 13
    TRANSCODE_FAILED = 14
    UNKNOWN_FILE_FORMAT = 15
    UNSUPPORTED_TEXTURE_TYPE = 16
    UNSUPPORTED_FEATURE = 17
    LIBRARY_NOT_LINKED = 18
    ERROR_MAX_ENUM = LIBRARY_NOT_LINKED


class KtxError(Exception):
    invocation: str
    code: KtxErrorCode

    def __init__(self, invocation: str, code: KtxErrorCode):
        self.invocation = invocation
        self.code = code

    def __str__(self):
        return str(self.invocation) + " returned with " + str(self.code)
