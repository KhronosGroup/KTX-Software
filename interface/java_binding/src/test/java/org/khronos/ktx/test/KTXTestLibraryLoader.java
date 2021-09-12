package org.khronos.ktx.test;

import org.junit.jupiter.api.extension.BeforeAllCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

import static org.junit.jupiter.api.extension.ExtensionContext.Namespace.GLOBAL;

public class KTXTestLibraryLoader
        implements BeforeAllCallback, ExtensionContext.Store.CloseableResource {
    private static boolean started = false;

    @Override
    public void beforeAll(final ExtensionContext context) throws Exception {
        // lock the access so only one Thread has access to it
        if (!started) {
            started = true;
            // Your "before all tests" startup logic goes here
            // The following line registers a callback hook when the root test context is
            // shut down
            context.getRoot().getStore(GLOBAL).put("any unique name", this);

            System.loadLibrary("ktx");
            System.loadLibrary("ktx-jni");
        }
    }

    @Override
    public void close() {
        // Your "after all tests" logic goes here
    }
}
