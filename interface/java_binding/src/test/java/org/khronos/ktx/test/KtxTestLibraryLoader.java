/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Locale;

import org.junit.jupiter.api.extension.BeforeAllCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

/**
 * A class that will be used for extending the unit tests, and allow using the
 * native <code>ktx</code> and <code>ktx-jni</code> libraries from <i>any</i>
 * directory for the tests.
 *
 * (Usually, this will be a local build output directory)
 *
 * It will check the <code>LIBKTX_BINARY_DIR<code> environment variable. If this
 * environment variable is a directory that contains the KTX JNI library, then
 * this library will be loaded.
 *
 * Otherwise, it will load the KTX JNI library that was installed globally with
 * the usual installation procedure.
 */
public class KtxTestLibraryLoader implements BeforeAllCallback, ExtensionContext.Store.CloseableResource {
	private static boolean started = false;

	@Override
	public void beforeAll(final ExtensionContext context) throws Exception {

		if (started) {
			return;
		}
		started = true;

		String ktxJniLibrary = findKtxJniLibraryName();
		if (ktxJniLibrary != null) {
			System.load(ktxJniLibrary);
		} else {
			System.loadLibrary("ktx-jni");
		}
	}

	/**
	 * Try to find the name (full, absolute path) of the KTX JNI library that should
	 * be loaded.
	 *
	 * This method will search for the library in the directory that is defined via
	 * the <code>LIBKTX_BINARY_DIR</code> environment variable. If this variable is
	 * not defined, or no suitable library can be found, then <code>null</code> is
	 * returned.
	 *
	 * @return The KTX JNI library name
	 */
	private static String findKtxJniLibraryName() {
		String ktxDir = System.getenv("LIBKTX_BINARY_DIR");
		if (ktxDir == null) {
			return null;
		}

		Path ktxPath = Path.of(ktxDir);
		if (!ktxPath.isAbsolute() || !Files.exists(ktxPath) || !Files.isDirectory(ktxPath)) {
			System.out.println(
					"KTXTestLibraryLoader: The value of the LIBKTX_BINARY_DIR environment variable is invalid: "
							+ ktxDir);
			return null;
		}

		String expectedKtxJniLibraryName = isRunningOnWindows() ? "ktx-jni" : "libktx-jni";

		System.out.println("KTXTestLibraryLoader: Loading KTX libraries from " + ktxDir);
		File ktxDirFile = new File(ktxDir);
		for (File file : ktxDirFile.listFiles()) {
			if (!file.isFile()) {
				continue;
			}
			String[] tokens = file.getName().split("\\.");
			if (tokens.length == 2 && tokens[0].equals(expectedKtxJniLibraryName)) {

				String ktxJniLibrary = file.getAbsolutePath();
				System.out.println("KTXTestLibraryLoader: Found " + expectedKtxJniLibraryName + " at " + ktxJniLibrary);
				return ktxJniLibrary;
			}
		}
		System.out
				.println("KTXTestLibraryLoader: Could not find " + expectedKtxJniLibraryName + " in given directory");
		return null;
	}

	/**
	 * Returns whether the <code>os.name</code> system property indicates that the
	 * operating system is Windows.
	 *
	 * @return Whether the operating system is Windows
	 */
	private static boolean isRunningOnWindows() {
		String osName = System.getProperty("os.name");
		osName = osName.toLowerCase(Locale.ENGLISH);
		return osName.startsWith("windows");
	}

	@Override
	public void close() {
		// NOOP
	}
}
