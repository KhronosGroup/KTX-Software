Copyright (c) 2021, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Java bindings for libktx, made with love by [Shukant Pal](https://github.com/ShukantPal) originally for the [Texture Compression Tool](https://compressor.shukantpal.com).

## Build and install libktx-jni

You must have `libktx` installed on your system already. The build was
tested on macOS / Linux - Windows build needs contribution :)!

```
mkdir build && cd build
cmake ../src/cpp && make
cmake --install .
```

## Run tests on macOS

It's tricky - I know:

```
 _JAVA_OPTIONS=-Djava.library.path=/usr/local/lib mvn test
```

## Things to do

* Tests for memory leaks!
* Full coverage of KTX api
