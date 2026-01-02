Copyright (c) 2021, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Java bindings for [libktx](https://github.com/KhronosGroup/KTX-Software), made with love by [Shukant Pal](https://github.com/ShukantPal) originally for the [Texture Compression Tool](https://compressor.shukantpal.com).

The `libktx-jni` library is built by the CMake project in the repository root. This library glues the `libktx` API with the interfaces provided in this Java library. You'll need to install `libktx` and `libktx-jni` to use the bindings. These, together with the Java archive `libktx.jar` can be installed from the packages found on the [KTX Software Releases](https://github.com/KhronosGroup/KTX-Software/releases) page.

Note: Java does not support arrays with more than 2³² elements so you should not use this library for images larger than four gigabytes in size.

## Usage

The setup is as follows:

```java
import org.khronos.ktx.KtxTexture2;

import java.nio.file.Paths;

public class App {
    static {
        // Load libktx-jni, which provides the JNI stubs for natively
        // implemented Java methods. This should also load libktx
        // automatically! If it doesn't, you may need to load libktx manually.
        System.loadLibrary("ktx-jni");
    }

    public static void main(String[] args) {
        KTXTexture2 texture = KTXTexture2.createFromNamedFile(
                Paths.get("exampleInput.ktx2").toAbsolutePath().toString());
        
        // Do something special with the texture!
        
        texture.writeToNamedFile(
                Paths.get("exampleOutput.ktx2").toAbsolutePath().toString());
    }
}
```

## Build

You must have Maven installed.

Pass `-DKTX_FEATURE_JNI=ON` when configuring the CMake build for `libktx` so that `libktx-jni` and `libktx.jar` are built as well.

This will place the libraries in a sub-directory of the build directory you
configured with CMake corresponding to the configuration you are building, usually `Debug` or `Release` and the JAR in the `target` directory in `interfaces/java_binding`. When building your application, include this JAR in the build.

The installers install the JAR is the same library directory as `libktx` and `libktx-jni`. On GNU/Linux and macOS this is `/usr/local/lib`.

## Manually Build JAR

You must have Maven installed:

```
mvn package
```

The JAR is placed in the location described in the previous section.

## Run tests on macOS

It's tricky - I know.

```
 _JAVA_OPTIONS=-Djava.library.path=/usr/local/lib mvn test
```

The path shown above is for the case when `libktx` and `libktx-jni` have been installed. If you have only built them then use

```
 _JAVA_OPTIONS=-Djava.library.path=/path/to/your/cmake/build/<config> mvn test
```

where `<config>` is your build configuration, usually either `Debug` or `Release`.
