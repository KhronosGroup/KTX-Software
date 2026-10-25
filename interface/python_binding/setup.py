# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

import os
import sysconfig
from setuptools import setup

__name__    = 'pyktx'
__version__ = os.environ['LIBKTX_VERSION']

assert __version__ is not None

# The native module is built with the limited API, so one abi3 wheel
# serves CPython 3.10 and later. Free-threaded builds can't use it.
options = {}
if not sysconfig.get_config_var('Py_GIL_DISABLED'):
    options['bdist_wheel'] = {'py_limited_api': 'cp310'}

setup(
    name=__name__,
    version=__version__,
    description='A Python interface to the libktx library',
    author='Shukant Pal, Mark Callow',
    author_email='ktx-sw-maintainer@khronosgroup.org',
    cffi_modules=["buildscript.py:ffibuilder"],
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    include_package_data=True,
    install_requires=["cffi>=2.0.0"],
    license="Apache 2.0",
    license_files=['LICENSE'],
    long_description_content_type="text/markdown",
    long_description="This Python package provides a Pythonic interface to libktx. It uses CFFI to generate the C bindings.",
    options=options,
    packages=['pyktx'],
    package_dir={'pyktx': 'pyktx'},
    setup_requires=["cffi>=2.0.0"],
    url='https://github.com/KhronosGroup/KTX-Software',
)
