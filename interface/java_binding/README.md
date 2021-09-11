Java bindings for libktx, made by [Shukant Pal](https://github.com/ShukantPal) originally for the [Texture Compression Tool](https://compressor.shukantpal.com).

## Build and install libktx-jni

You must have `libktx` installed on your system already. The build was
tested on macOS / Linux - Windows build needs contribution :)!

```
mkdir build && cd build
cmake ../src/cpp && make
cmake --install .
```
