Copyright (c) 2023, Shukant Pal and Contributors \
SPDX-License-Identifier: Apache-2.0

Made with love by [Shukant Pal](https://www.shukantpal.com/about) on his way to learning Python.

# pyktx

This Python package provides a Pythonic interface to libktx. It uses CFFI to generate the C bindings.

## Building

To build and test pyktx,

```bash
# Set LIBKTX_INSTALL_DIR if you've installed libktx at the default system location.
# Otherwise set LIBKTX_INCLUDE_DIR, LIBKTX_LIB_DIR to wherever you've built libktx.
cd ${PROJECT_DIR}/interface/python_binding
KTX_RUN_TESTS=true python3 buildscript.py
```