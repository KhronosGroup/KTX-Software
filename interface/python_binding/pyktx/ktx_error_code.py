# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

from enum import IntEnum


class KtxErrorCode(IntEnum):
    """Error codes thrown by library functions."""

    SUCCESS = 0
    """Operation was successful."""

    FILE_DATA_ERROR = 1
    """The data in the file is inconsistent with the spec."""

    FILE_ISPIPE = 2
    """The file is a pipe or named pipe."""

    FILE_OPEN_FAILED = 3
    """The target file could not be opened."""

    FILE_OVERFLOW = 4
    """The operation would exceed the max file size."""

    FILE_READ_ERROR = 5
    """An error occurred while reading from the file."""

    FILE_SEEK_ERROR = 6
    """An error occurred while seeking in the file."""

    FILE_UNEXPECTED_EOF = 7
    """File does not have enough data to satisfy request."""

    FILE_WRITE_ERROR = 8
    """An error occurred while writing to the file."""

    GL_ERROR = 9
    """GL operations resulted in an error."""

    INVALID_OPERATION = 10
    """The operation is not allowed in the current state."""

    INVALID_VALUE = 11
    """A parameter value was not valid."""

    NOT_FOUND = 12
    """Requested key was not found"""

    OUT_OF_MEMORY = 13
    """Not enough memory to complete the operation."""

    TRANSCODE_FAILED = 14
    """Transcoding of block compressed texture failed."""

    UNKNOWN_FILE_FORMAT = 15
    """The file not a KTX file."""

    UNSUPPORTED_TEXTURE_TYPE = 16
    """The KTX file specifies an unsupported texture type."""

    UNSUPPORTED_FEATURE = 17
    """Feature not included in in-use library or not yet implemented."""

    LIBRARY_NOT_LINKED = 18
    """Library dependency (OpenGL or Vulkan) not linked into application."""

    DECOMPRESS_LENGTH_ERROR = 19
    """Decompressed byte count does not match expected byte size."""

    DECOMPRESS_CHECKSUM_ERROR = 20
    """Checksum mismatch when decompressing."""

    ERROR_MAX_ENUM = LIBRARY_NOT_LINKED
    """For safety checks."""


class KtxError(Exception):
    """Error thrown when native operation does not succeed."""

    invocation: str
    """The C library function called."""

    code: KtxErrorCode
    """The error code returned by libktx."""

    def __init__(self, invocation: str, code: KtxErrorCode):
        self.invocation = invocation
        self.code = code

    def __str__(self):
        return str(self.invocation) + " returned with " + str(self.code)
