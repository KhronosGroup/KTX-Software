/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx;

/**
 * Error codes returned from KTX functions as "int"
 */
public class KtxErrorCode {

	/**
	 * Operation was successful
	 */
	public static final int KTX_SUCCESS = 0;

	/**
	 * The data in the file is inconsistent with the spec
	 */
	public static final int KTX_FILE_DATA_ERROR = 1;

	/**
	 * The file is a pipe or named pipe
	 */
	public static final int KTX_FILE_ISPIPE = 2;

	/**
	 * The target file could not be opened
	 */
	public static final int KTX_FILE_OPEN_FAILED = 3;

	/**
	 * The operation would exceed the max file size
	 */
	public static final int KTX_FILE_OVERFLOW = 4;

	/**
	 * An error occurred while reading from the file
	 */
	public static final int KTX_FILE_READ_ERROR = 5;

	/**
	 * An error occurred while seeking in the file
	 */
	public static final int KTX_FILE_SEEK_ERROR = 6;

	/**
	 * File does not have enough data to satisfy request
	 */
	public static final int KTX_FILE_UNEXPECTED_EOF = 7;

	/**
	 * An error occurred while writing to the file
	 */
	public static final int KTX_FILE_WRITE_ERROR = 8;

	/**
	 * GL operations resulted in an error
	 */
	public static final int KTX_GL_ERROR = 9;

	/**
	 * The operation is not allowed in the current state
	 */
	public static final int KTX_INVALID_OPERATION = 10;

	/**
	 * A parameter value was not valid
	 */
	public static final int KTX_INVALID_VALUE = 11;

	/**
	 * Requested metadata key or required dynamically loaded GPU function was not
	 * found
	 */
	public static final int KTX_NOT_FOUND = 12;

	/**
	 * Not enough memory to complete the operation
	 */
	public static final int KTX_OUT_OF_MEMORY = 13;

	/**
	 * Transcoding of block compressed texture failed
	 */
	public static final int KTX_TRANSCODE_FAILED = 14;

	/**
	 * The file not a KTX file
	 */
	public static final int KTX_UNKNOWN_FILE_FORMAT = 15;

	/**
	 * The KTX file specifies an unsupported texture type
	 */
	public static final int KTX_UNSUPPORTED_TEXTURE_TYPE = 16;

	/**
	 * Feature not included in in-use library or not yet implemented
	 */
	public static final int KTX_UNSUPPORTED_FEATURE = 17;

	/**
	 * Library dependency (OpenGL or Vulkan) not linked into application
	 */
	public static final int KTX_LIBRARY_NOT_LINKED = 18;

	/**
	 * Decompressed byte count does not match expected byte size
	 */
	public static final int KTX_DECOMPRESS_LENGTH_ERROR = 19;

	/**
	 * Checksum mismatch when decompressing
	 */
	public static final int KTX_DECOMPRESS_CHECKSUM_ERROR = 20;

	/**
	 * For safety checks
	 */
	public static final int KTX_ERROR_MAX_ENUM = KTX_DECOMPRESS_CHECKSUM_ERROR;

	/**
	 * Returns a string representation of the given error code
	 *
	 * @param n The error code
	 * @return A string representation of the given error code
	 */
	public static String stringFor(int n) {
		switch (n) {
		case KTX_SUCCESS:
			return "KTX_SUCCESS";
		case KTX_FILE_DATA_ERROR:
			return "KTX_FILE_DATA_ERROR";
		case KTX_FILE_ISPIPE:
			return "KTX_FILE_ISPIPE";
		case KTX_FILE_OPEN_FAILED:
			return "KTX_FILE_OPEN_FAILED";
		case KTX_FILE_OVERFLOW:
			return "KTX_FILE_OVERFLOW";
		case KTX_FILE_READ_ERROR:
			return "KTX_FILE_READ_ERROR";
		case KTX_FILE_SEEK_ERROR:
			return "KTX_FILE_SEEK_ERROR";
		case KTX_FILE_UNEXPECTED_EOF:
			return "KTX_FILE_UNEXPECTED_EOF";
		case KTX_FILE_WRITE_ERROR:
			return "KTX_FILE_WRITE_ERROR";
		case KTX_GL_ERROR:
			return "KTX_GL_ERROR";
		case KTX_INVALID_OPERATION:
			return "KTX_INVALID_OPERATION";
		case KTX_INVALID_VALUE:
			return "KTX_INVALID_VALUE";
		case KTX_NOT_FOUND:
			return "KTX_NOT_FOUND";
		case KTX_OUT_OF_MEMORY:
			return "KTX_OUT_OF_MEMORY";
		case KTX_TRANSCODE_FAILED:
			return "KTX_TRANSCODE_FAILED";
		case KTX_UNKNOWN_FILE_FORMAT:
			return "KTX_UNKNOWN_FILE_FORMAT";
		case KTX_UNSUPPORTED_TEXTURE_TYPE:
			return "KTX_UNSUPPORTED_TEXTURE_TYPE";
		case KTX_UNSUPPORTED_FEATURE:
			return "KTX_UNSUPPORTED_FEATURE";
		case KTX_LIBRARY_NOT_LINKED:
			return "KTX_LIBRARY_NOT_LINKED";
		case KTX_DECOMPRESS_LENGTH_ERROR:
			return "KTX_DECOMPRESS_LENGTH_ERROR";
		case KTX_DECOMPRESS_CHECKSUM_ERROR:
			return "KTX_DECOMPRESS_CHECKSUM_ERROR";
		}
		return "[Unknown KtxErrorCode]";
	}
}
