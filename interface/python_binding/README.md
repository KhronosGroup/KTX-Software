Copyright (c) 2023, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Made with love by [Shukant Pal](https://www.shukantpal.com/about) on his way to learning Python.

# pyktx

This Python package provides a Pythonic interface to _libktx_. It uses CFFI to generate the C bindings.

## Usage

**You must have _libktx_ installed on your system to use pyktx ordinarily. If not
installed in the default location, you can configure where _libktx_ is installed
using the `LIBKTX_INCLUDE_DIR` and `LIBKTX_LIB_DIR` environment variables.**

To install libktx, download and run the appropriate installer from [our releases](https://github.com/KhronosGroup/KTX-Software/releases).

## Building

To build and test pyktx,

Note: the following examples assume the environment variable PROJECT_DIR contains
the full path to your KTX project. The self-built examples further assume that your
build is in the `build` subdirectory of the project.

Set `LIBKTX_INSTALL_DIR` to point to your `libktx` installation. If you've
installed `libktx` at the default location there is no need to set this.

```bash
cd $PROJECT_DIR/interface/python_binding
KTX_RUN_TESTS=ON python3 buildscript.py
# or
LIBKTX_INSTALL_DIR=<path_to_your_installation> KTX_RUN_TESTS=ON python3 buildscript.py
```

On Windows.

```powershell
cd $env:PROJECT_DIR/interface/python_binding
pwsh -Command { $env:LIBKTX_INSTALL_DIR='<path to installation>'; python buildscript.py }
# or
pwsh -Command { $env:LIBKTX_INSTALL_DIR='<path to installation>'; $env:KTX_RUN_TESTS='ON'; python buildscript.py }
```

If you want to test with a self-built libktx, set `LIBKTX_INCLUDE_DIR` and
`LIBKTX_LIB_DIR` to wherever your build is located. E.g.

```bash
cd $PROJECT_DIR/interface/python_binding
LIBKTX_INCLUDE_DIR=../../lib/include LIBKTX_LIB_DIR=../../build/Debug KTX_RUN_TESTS=ON python3 buildscript.py
```

On Windows, you must also set `LIBKTX_IMPORT_DIR`. It must be the path to the
directory where `ktx.lib` can be found. `LIBKTX_LIB_DIR` is the path to
the directory where `ktx.dll` can be found.

```powershell
cd $env:PROJECT_DIR/interface/python_binding
pwsh -Command { $env:LIBKTX_INCLUDE_DIR='../../lib/include'; $env:LIBKTX_IMPORT_DIR='../../build/lib/Debug'; $env:LIBKTX_LIB_DIR = '../../build/Debug>'; python buildscript.py }
# or
pwsh -Command { $env:LIBKTX_INCLUDE_DIR='../../lib/include'; $env:LIBKTX_IMPORT_DIR='../../build/lib/Debug'; $env:LIBKTX_LIB_DIR = '../../build/Debug>'; $env:KTX_RUN_TESTS='ON'; python buildscript.py }
```

### Note
> When building on macOS against a universal CPython binary, such as that installed with the Xcode command-line tools (/usr/bin/python3), ld will issue a warning
>
> ```
> ld: warning: ignoring file '/usr/local/lib/libktx.4.3.0.dylib': found architecture 'arm64', required architecture 'x86_64'
> ```
>
> 'arm64' and 'x86_64' may be reversed depending on the build machine architecture. This happens because libktx is not a universal binary so only supports the current platform architecture. The message can be ignored.
