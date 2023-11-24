# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

find_package (Python3 COMPONENTS Interpreter)
file(GLOB pyktx_py_src ${CMAKE_SOURCE_DIR}/interface/python_binding/pyktx/*.py)
list(TRANSFORM pyktx_py_src REPLACE "/python_binding/pyktx/" "/python_binding/docs/" OUTPUT_VARIABLE pyktx_py_rst_filenames)
list(TRANSFORM pyktx_py_rst_filenames REPLACE ".py$" ".rst" OUTPUT_VARIABLE pyktx_py_rst)

message(STATUS "pyktx_py_rst is ${pyktx_py_rst}")

set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
if(DEFINED PYTHON AND NOT ${PYTHON} STREQUAL "")
    set(PYTHON_EXECUTABLE ${PYTHON})
    message(STATUS "Override PYTHON with ${PYTHON}")
endif()
if (LINUX AND NOT Python3_FOUND)
    set(PYTHON_EXECUTABLE python)
    message(STATUS "CMake failed to find python3. Will continue assuming it's on PATH")
endif()

# Convert Windows path to CMake path
cmake_path(SET PYTHON_PATH ${PYTHON_EXECUTABLE})

if(LINUX)
    set(LIBKTX_LIB_DIR ${KTX_BUILD_DIR})
else()
    set(LIBKTX_LIB_DIR ${KTX_BUILD_DIR}/$<CONFIG>)
endif()

add_custom_target( pyktx-deps ALL
    COMMENT
        "Python deps")

add_custom_command(
    TARGET pyktx-deps
    COMMAND
        ${PYTHON_EXECUTABLE} -m pip install --no-warn-script-location -r ${CMAKE_SOURCE_DIR}/interface/python_binding/requirements.txt
    COMMENT
        "Install dependencies for pyktx build"
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
add_dependencies(pyktx pyktx-deps)

add_custom_command(
    TARGET pyktx
    PRE_BUILD
    COMMAND
        ${PYTHON_EXECUTABLE} clean.py
    COMMENT
        "Clean up pyktx build artifacts"
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_command(
    TARGET
        pyktx
    BYPRODUCTS
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/pyktx-${KTX_VERSION}.tar.gz
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/pyktx-${KTX_VERSION}.zip
    DEPENDS
        ${pyktx_py_src}
        pyktx/ktx_texture.h
        pyktx/ktx_texture1.h
        pyktx/ktx_texture2.h
        pyktx/ktx_texture.c
        pyktx/ktx_texture1.c
        pyktx/ktx_texture2.c
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
            LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            LIBKTX_VERSION=${PROJECT_VERSION}
        ${PYTHON_EXECUTABLE} -m build --sdist
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_command(
    TARGET
        pyktx
    DEPENDS
        ${PYKTX_SRC_DIST}
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
            LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            LIBKTX_VERSION=${PROJECT_VERSION}
        ${PYTHON_EXECUTABLE} -m build --wheel
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_test(NAME pyktx
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
            LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            KTX_RUN_TESTS=ON
            DYLD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{DYLD_LIBRARY_PATH}
            LD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{LD_LIBRARY_PATH}
        ${PYTHON_EXECUTABLE} buildscript.py
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

if(KTX_FEATURE_DOC)
    add_custom_command(
        TARGET pyktx
        POST_BUILD
        BYPRODUCTS
            ${pyktx_py_rst}
            ${CMAKE_SOURCE_DIR}/interface/python_binding/docs/modules.rst
            ${CMAKE_SOURCE_DIR}/interface/python_binding/docs/native.rst
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
                DYLD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{DYLD_LIBRARY_PATH}
                LD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{LD_LIBRARY_PATH}
            ${PYTHON_EXECUTABLE} buildscript.py
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            ${PYTHON_EXECUTABLE} -m sphinx.ext.apidoc -o docs ./pyktx --separate
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
                SPHINXBUILD=${PYTHON_PATH}\ -m\ sphinx
            make BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}/docs/html/pyktx" html
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}/interface/python_binding
    )
endif()