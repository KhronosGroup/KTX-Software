/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

/**
 * An exception that may be thrown due to a KTX error. <br>
 * <br>
 * This will be thrown from methods like the <code>KtxTexture2#create...</code>
 * methods or {@link KtxTexture2#writeToMemory()} in the case that the
 * underlying implementation caused an error code that was not
 * {@link KtxErrorCode#SUCCESS}.
 */
public class KtxException extends RuntimeException
{
	/**
	 * Serial UID
	 */
	private static final long serialVersionUID = 313517353875035541L;

	/**
	 * Creates a new KtxException with the given error message.
	 *
	 * @param message The error message for this KtxException
	 */
	public KtxException(String message)
	{
		super(message);
	}

	/**
	 * Creates a new KtxException with the given error message
	 * and the given Throwable as the cause.
	 *
	 * @param message The error message for this KtxException
	 * @param cause The reason for this KtxException
	 */
	public KtxException(String message, Throwable cause)
	{
		super(message, cause);
	}
}