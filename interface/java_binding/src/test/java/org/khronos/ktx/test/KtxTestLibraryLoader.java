/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import org.junit.jupiter.api.extension.BeforeAllCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

import static org.junit.jupiter.api.extension.ExtensionContext.Namespace.GLOBAL;

public class KtxTestLibraryLoader
        implements BeforeAllCallback, ExtensionContext.Store.CloseableResource {
    private static boolean started = false;

    @Override
    public void beforeAll(final ExtensionContext context) throws Exception {
        if (!started) {
            started = true;

            String ktxDir = System.getenv("LIBKTX_BINARY_DIR");
            String ktxLibrary = null;
            String ktxJNILibrary = null;

            if (ktxDir != null &&
                    Path.of(ktxDir).isAbsolute() &&
                    Files.exists(Path.of(ktxDir)) &&
                    Files.isDirectory(Path.of(ktxDir))) {
                System.out.println("KTXTestLibraryLoader is loading libktx, libktx-jni from " + ktxDir);

                File ktxDirFile = new File(ktxDir);

                for (File file : ktxDirFile.listFiles()) {
                    if (!file.isFile()) continue;
                    if (ktxLibrary != null && ktxJNILibrary != null) break;

                    String[] tokens = file.getName().split("\\.");

                    if (ktxLibrary == null &&
                            tokens.length == 2 &&
                            tokens[0].contentEquals("libktx")) {

                        ktxLibrary = file.getAbsolutePath();
                        System.out.println("KTXTestLibraryLoader found libktx at " + ktxLibrary);
                    } else if (ktxJNILibrary == null &&
                            tokens.length == 2 &&
                            tokens[0].contentEquals("libktx-jni")) {

                        ktxJNILibrary = file.getAbsolutePath();
                        System.out.println("KTXTestLibraryLoader found libktx-jni at " + ktxJNILibrary);
                    }
                }
            }

            if (ktxLibrary != null)
                System.load(ktxLibrary);
            else
                System.loadLibrary("ktx");

            if (ktxJNILibrary != null)
                System.load(ktxJNILibrary);
            else
                System.loadLibrary("ktx-jni");
        }
    }

    @Override
    public void close() {
        // NOOP
    }
}
