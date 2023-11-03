# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

find_package (Python3 COMPONENTS Interpreter)
file(GLOB pyktx_py_src "pyktx/*.py")

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

add_custom_target(pyktx-deps
    COMMENT "pip dependencies")
add_custom_command(
    TARGET pyktx-deps
    COMMAND
        ${PYTHON_EXECUTABLE} -m pip install --no-warn-script-location -r ${CMAKE_SOURCE_DIR}/interface/python_binding/requirements.txt
    COMMENT
        "Install dependencies for pyktx build"
)
add_custom_command(
    TARGET pyktx-deps
    COMMAND
        ${PYTHON_EXECUTABLE} clean.py
    COMMENT
        "Clean up pyktx build artifacts"
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_target( pyktx ALL
    DEPENDS
        ${pyktx_py_src}
        pyktx/ktx_texture.h
        pyktx/ktx_texture1.h
        pyktx/ktx_texture2.h
        pyktx/ktx_texture.c
        pyktx/ktx_texture1.c
        pyktx/ktx_texture2.c
        ${LIBKTX_LIB_DIR}
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
    COMMENT
        "Python distributions"
)
add_dependencies(pyktx ktx pyktx-deps)

add_custom_command(
    TARGET pyktx
    POST_BUILD
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
    TARGET pyktx
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E env
        LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
        LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
        LIBKTX_VERSION=${PROJECT_VERSION}
        ${PYTHON_EXECUTABLE} -m build --wheel
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_target(pyktx-dev
    DEPENDS
        ${pyktx_py_src}
        pyktx/ktx_texture.h
        pyktx/ktx_texture1.h
        pyktx/ktx_texture2.h
        pyktx/ktx_texture.c
        pyktx/ktx_texture1.c
        pyktx/ktx_texture2.c
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
    COMMENT
        "Unbundled Python object files"
)

add_dependencies(pyktx-dev ktx pyktx-deps pyktx)

add_custom_command(
    TARGET pyktx-dev
    BYPRODUCTS
        pyktx/ktx_texture1.o
        pyktx/ktx_texture2.o
        pyktx/ktx_texture.o
    COMMAND
        ${CMAKE_COMMAND} -E env
        LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
        LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
        KTX_RUN_TESTS=${KTX_FEATURE_TESTS}
        DYLD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{DYLD_LIBRARY_PATH}
        LD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{LD_LIBRARY_PATH}
        PATH=${LIBKTX_LIB_DIR}:$ENV{PATH}
        ${PYTHON_EXECUTABLE} buildscript.py
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

if(KTX_FEATURE_DOC)
    add_custom_command(
        TARGET pyktx-dev
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
                DYLD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{DYLD_LIBRARY_PATH}
                LD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{LD_LIBRARY_PATH}
            ${PYTHON_EXECUTABLE} -m sphinx.ext.apidoc -o docs ./pyktx --separate
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
                SPHINXBUILD=$<IF:$<PLATFORM_ID:Windows>,"${PYTHON_PATH} -m sphinx","sphinx-build">
            make BUILDDIR="${CMAKE_CURRENT_BINARY_DIR}/docs/html/pyktx" html
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}/interface/python_binding
    )
endif()
