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
	public static final int SUCCESS = 0;

	/**
	 * The data in the file is inconsistent with the spec
	 */
	public static final int FILE_DATA_ERROR = 1;

	/**
	 * The file is a pipe or named pipe
	 */
	public static final int FILE_ISPIPE = 2;

	/**
	 * The target file could not be opened
	 */
	public static final int FILE_OPEN_FAILED = 3;

	/**
	 * The operation would exceed the max file size
	 */
	public static final int FILE_OVERFLOW = 4;

	/**
	 * An error occurred while reading from the file
	 */
	public static final int FILE_READ_ERROR = 5;

	/**
	 * An error occurred while seeking in the file
	 */
	public static final int FILE_SEEK_ERROR = 6;

	/**
	 * File does not have enough data to satisfy request
	 */
	public static final int FILE_UNEXPECTED_EOF = 7;

	/**
	 * An error occurred while writing to the file
	 */
	public static final int FILE_WRITE_ERROR = 8;

	/**
	 * GL operations resulted in an error
	 */
	public static final int GL_ERROR = 9;

	/**
	 * The operation is not allowed in the current state
	 */
	public static final int INVALID_OPERATION = 10;

	/**
	 * A parameter value was not valid
	 */
	public static final int INVALID_VALUE = 11;

	/**
	 * Requested metadata key or required dynamically loaded GPU function was not
	 * found
	 */
	public static final int NOT_FOUND = 12;

	/**
	 * Not enough memory to complete the operation
	 */
	public static final int OUT_OF_MEMORY = 13;

	/**
	 * Transcoding of block compressed texture failed
	 */
	public static final int TRANSCODE_FAILED = 14;

	/**
	 * The file not a KTX file
	 */
	public static final int UNKNOWN_FILE_FORMAT = 15;

	/**
	 * The KTX file specifies an unsupported texture type
	 */
	public static final int UNSUPPORTED_TEXTURE_TYPE = 16;

	/**
	 * Feature not included in in-use library or not yet implemented
	 */
	public static final int UNSUPPORTED_FEATURE = 17;

	/**
	 * Library dependency (OpenGL or Vulkan) not linked into application
	 */
	public static final int LIBRARY_NOT_LINKED = 18;

	/**
	 * Decompressed byte count does not match expected byte size
	 */
	public static final int DECOMPRESS_LENGTH_ERROR = 19;

	/**
	 * Checksum mismatch when decompressing
	 */
	public static final int DECOMPRESS_CHECKSUM_ERROR = 20;

	/**
	 * For safety checks
	 */
	public static final int MAX_ENUM = DECOMPRESS_CHECKSUM_ERROR;

	/**
	 * Returns a string representation of the given error code
	 *
	 * @param n The error code
	 * @return A string representation of the given error code
	 */
	public static String stringFor(int n) {
		switch (n) {
		case SUCCESS:
			return "SUCCESS";
		case FILE_DATA_ERROR:
			return "FILE_DATA_ERROR";
		case FILE_ISPIPE:
			return "FILE_ISPIPE";
		case FILE_OPEN_FAILED:
			return "FILE_OPEN_FAILED";
		case FILE_OVERFLOW:
			return "FILE_OVERFLOW";
		case FILE_READ_ERROR:
			return "FILE_READ_ERROR";
		case FILE_SEEK_ERROR:
			return "FILE_SEEK_ERROR";
		case FILE_UNEXPECTED_EOF:
			return "FILE_UNEXPECTED_EOF";
		case FILE_WRITE_ERROR:
			return "FILE_WRITE_ERROR";
		case GL_ERROR:
			return "GL_ERROR";
		case INVALID_OPERATION:
			return "INVALID_OPERATION";
		case INVALID_VALUE:
			return "INVALID_VALUE";
		case NOT_FOUND:
			return "NOT_FOUND";
		case OUT_OF_MEMORY:
			return "OUT_OF_MEMORY";
		case TRANSCODE_FAILED:
			return "TRANSCODE_FAILED";
		case UNKNOWN_FILE_FORMAT:
			return "UNKNOWN_FILE_FORMAT";
		case UNSUPPORTED_TEXTURE_TYPE:
			return "UNSUPPORTED_TEXTURE_TYPE";
		case UNSUPPORTED_FEATURE:
			return "UNSUPPORTED_FEATURE";
		case LIBRARY_NOT_LINKED:
			return "LIBRARY_NOT_LINKED";
		case DECOMPRESS_LENGTH_ERROR:
			return "DECOMPRESS_LENGTH_ERROR";
		case DECOMPRESS_CHECKSUM_ERROR:
			return "DECOMPRESS_CHECKSUM_ERROR";
		}
		return "[Unknown KtxErrorCode]";
	}

	/**
	 * Return a string corresponding to a KTX error code.
	 *
	 * @param error The error code for which to return a string
     * @return The message string.
	 */
	public static native String createString(int error);


	/**
	 * Private constructor to prevent instantiation
	 */
	private KtxErrorCode() {
		// Prevent instantiation
	}


}
