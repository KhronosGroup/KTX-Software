/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/** Error codes returned from KTX functions as "int". */
public class KtxErrorCode {
    public static final int SUCCESS = 0;
    public static final int FILE_DATA_ERROR = 1;
    public static final int FILE_ISPIPE = 2;
    public static final int FILE_OPEN_FAILED = 3;
    public static final int FILE_OVERFLOW = 4;
    public static final int FILE_READ_ERROR = 5;
    public static final int FILE_SEEK_ERROR = 6;
    public static final int FILE_UNEXPECTED_EOF = 7;
    public static final int FILE_WRITE_ERROR = 8;
    public static final int GL_ERROR = 9;
    public static final int INVALID_OPERATION = 10;
    public static final int INVALID_VALUE = 11;
    public static final int NOT_FOUND = 12;
    public static final int OUT_OF_MEMORY = 13;
    public static final int TRANSCODE_FAILED = 14;
    public static final int UNKNOWN_FILE_FORMAT = 15;
    public static final int UNSUPPORTED_TEXTURE_TYPE = 16;
    public static final int UNSUPPORTED_FEATURE = 17;
    public static final int LIBRARY_NOT_LINKED = 18;
    public static final int DECOMPRESS_LENGTH_ERROR = 19;
    public static final int DECOMPRESS_CHECKSUM_ERROR = 20;
    public static final int ERROR_MAX_ENUM = DECOMPRESS_CHECKSUM_ERROR;
}
