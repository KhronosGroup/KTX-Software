# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

find_package (Python3 COMPONENTS Interpreter)
file(GLOB pyktx_py_src "pyktx/*.py")

add_custom_command(
    DEPENDS
        ${pyktx_py_src}
        pyktx/ktx_texture.h
        pyktx/ktx_texture1.h
        pyktx/ktx_texture2.h
        pyktx/ktx_texture.c
        pyktx/ktx_texture1.c
        pyktx/ktx_texture2.c
    OUTPUT
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/libktx-${PROJECT_VERSION}.tgz
    COMMAND
        ${Python3_EXECUTABLE} ./setup.py sdist
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

message(STATUS DEBUG_BUILD "${CMAKE_BINARY_DIR}")

add_custom_command(
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/libktx-${PROJECT_VERSION}.tgz
    OUTPUT
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist
    COMMAND
        ${CMAKE_COMMAND} -E env
            C_INCLUDE_PATH=${CMAKE_SOURCE_DIR}/include
            LDFLAGS=-L${CMAKE_BINARY_DIR}
        ${Python3_EXECUTABLE} ./setup.py bdist_wheel
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_target( pyktx ALL
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist
        ktx
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
    COMMENT
        "Python distributions"
)