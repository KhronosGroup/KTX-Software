# Copyright (c) 2023, Shukant Pal and Contributors
# SPDX-License-Identifier: Apache-2.0

import os
from setuptools import setup

__name__    = 'pyktx'
__version__ = os.environ['LIBKTX_VERSION']

assert __version__ is not None

setup(
    name=__name__,
    version=__version__,
    description='A Python interface to the libktx library',
    author='Shukant Pal',
    author_email='foss@shukantpal.com',
    cffi_modules=["buildscript.py:ffibuilder"],
    classifiers=[
        "Programming Language :: Python :: 3",
    ],
    include_package_data=True,
    install_requires=["cffi>=1.15.1"],
    license="Apache 2.0",
    license_files=('LICENSE'),
    long_description_content_type="text/markdown",
    long_description="This Python package provides a Pythonic interface to libktx. It uses CFFI to generate the C bindings.",
    packages=['pyktx'],
    package_dir={'pyktx': 'pyktx'},
    setup_requires=["cffi>=1.15.1"],
    url='https://github.com/KhronosGroup/KTX-Software',
)
