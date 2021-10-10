Copyright (c) 2021, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Java bindings for [libktx](https://github.com/KhronosGroup/KTX-Software), made with love by [Shukant Pal](https://github.com/ShukantPal) originally for the [Texture Compression Tool](https://compressor.shukantpal.com).

The `libktx-jni` library is built by the CMake project in the repository root. This library glues the `libktx` API with the interfaces provided in this Java library. You'll need to install `libktx`, `libktx-jni` to use the bindings.

## Usage

```java
import org.khronos.ktx.KtxTexture2;

import java.nio.file.Paths;

public class App {
    static {
        // Load libktx
        System.loadLibrary("ktx");

        // Load libktx-jni, which provides the JNI stubs for natively implemented Java methods
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
maven package
```

This will place a JAR in the `target` directory in interfaces/java_binding. When building your application, include this JAR in the build.

## Run tests on macOS

It's tricky - I know:

```
 _JAVA_OPTIONS=-Djava.library.path=/usr/local/lib mvn test
```
