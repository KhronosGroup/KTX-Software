/*
 * Copyright (c) 2021, Shukant Pal and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

package org.khronos.ktx.test;

import org.junit.jupiter.api.extension.BeforeAllCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

import static org.junit.jupiter.api.extension.ExtensionContext.Namespace.GLOBAL;

public class KTXTestLibraryLoader
        implements BeforeAllCallback, ExtensionContext.Store.CloseableResource {
    private static boolean started = false;

    @Override
    public void beforeAll(final ExtensionContext context) throws Exception {
        if (!started) {
            started = true;

            System.loadLibrary("ktx");
            System.loadLibrary("ktx-jni");
        }
    }

    @Override
    public void close() {
        // NOOP
    }
}
