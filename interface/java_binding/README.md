Copyright (c) 2021, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Java bindings for [libktx](https://github.com/KhronosGroup/KTX-Software), made with love by [Shukant Pal](https://github.com/ShukantPal) originally for the [Texture Compression Tool](https://compressor.shukantpal.com).

The `libktx-jni` library is built by the CMake project in the repository root. This library glues the `libktx` API with the interfaces provided in this Java library. You'll need to install `libktx`, `libktx-jni` to use the bindings.

Note: Java does not support arrays with more than 2³² elements so you should not use this library for images larger than several gigabytes in size.

## Usage

The setup is as follows:

```java
import org.khronos.ktx.KtxTexture2;

import java.nio.file.Paths;

public class App {
    static {
        // Load libktx-jni, which provides the JNI stubs for natively implemented Java methods
        // This should also load libktx automatically! If it doesn't, you may need to load libktx manually.
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

## Build JAR

You must have Maven installed:

```
mvn package
```

This will place a JAR in the `target` directory in interfaces/java_binding. When building your application, include this JAR in the build.

## Run tests on macOS

It's tricky - I know:

```
 _JAVA_OPTIONS=-Djava.library.path=/usr/local/lib mvn test
```

## Building libktx-jni

You'll need to pass `-DKTX_FEATURE_JNI=ON` when building libktx so that the libktx-jni
library is built as well.
