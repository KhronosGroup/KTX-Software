# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

file(GLOB pyktx_src
    "pyktx/*.py"
    "pyktx/ktx_texture.h"
    "pyktx/ktx_texture1.h"
    "pyktx/ktx_texture2.h"
    "pyktx/ktx_texture.c"
    "pyktx/ktx_texture1.c"
    "pyktx/ktx_texture2.c")

add_custom_command(
    DEPENDS
        ${pyktx_src}
    OUTPUT
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/libktx-${PROJECT_VERSION}.tgz
    COMMAND
        python ./setup.py sdist
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_command(
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/libktx-${PROJECT_VERSION}.tgz
    COMMAND
        python ./setup.py bdist_wheel
)