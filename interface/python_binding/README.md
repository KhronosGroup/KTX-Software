Copyright (c) 2023, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Made with love by [Shukant Pal](https://www.shukantpal.com/about) on his way to learning Python.

# pyktx

This Python package provides a Pythonic interface to libktx. It uses CFFI to generate the C bindings.

## Usage

**You must have libktx installed on your system to use pyktx ordinarily. You can configure where libktx is installed using the `LIBKTX_INCLUDE_DIR` and `LIBKTX_LIB_DIR` environment variables too.**

To install libktx, download and run the appropriate installer from [our releases](https://github.com/KhronosGroup/KTX-Software/releases).

## Building

To build and test pyktx,

```bash
# Set LIBKTX_INSTALL_DIR if you've installed libktx at the default system location.
# Otherwise set LIBKTX_INCLUDE_DIR, LIBKTX_LIB_DIR to wherever you've built libktx.
cd ${PROJECT_DIR}/interface/python_binding
KTX_RUN_TESTS=ON python3 buildscript.py
```

If you are on a POSIX system (macOS or Linux), make sure libktx is on your `DYLD_LIBRARY_PATH` and `LD_LIBRARY_PATH`.