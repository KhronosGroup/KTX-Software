/*
 * Copyright (c) 2024, Khronos Group and Contributors
 * SPDX-License-Identifier: Apache-2.0
 */
package org.khronos.ktx;

/**
 * Utility class for loading the native KTX library.
 */
class KtxLibraryLoader
{
    /**
     * Whether the native library was loaded
     */
    private static boolean loaded = false;

    /**
     * Try to load the native KTX library.<br>
     * <br>
     * This should be called in a static initializer block of any class
     * that contains <code>native</code> methods that rely on the native
     * library. When such a class is loaded by the ClassLoader, then this
     * function will be executed and try to load the native library.<br>
     * <br>
     * If the native library cannot be loaded due to an 
     * <code>UnsatisfiedLinkError</code>, an error message will be printed.
     */
    static void load()
    {
        if (loaded)
        {
            return;
        }
        loaded = true;

        // TODO Allow this... ?
        // String ktxDir = System.getenv("LIBKTX_BINARY_DIR");
        // if (ktxDir == null) {
        // System.out.println("KtxTestLibraryLoader: USING FIXED PATH!");
        // ktxDir =
        // "C:/Develop/KhronosGroup/KTX-Software/interface/java_binding/nativeLibraries";
        // //return null;
        // }

        try
        {
            LibUtils.loadLibrary("ktx-jni", "ktx");
        }
        catch (UnsatisfiedLinkError e)
        {
            e.printStackTrace();
        }
    }
}
