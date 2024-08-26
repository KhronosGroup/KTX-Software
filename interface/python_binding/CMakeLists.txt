# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

find_package (Python3 COMPONENTS Interpreter)
set(SOURCE_DIR ${CMAKE_SOURCE_DIR}/interface/python_binding)
file(GLOB pyktx_py_src ${SOURCE_DIR}/pyktx/*.py)
list(TRANSFORM pyktx_py_src REPLACE "${SOURCE_DIR}/pyktx/" "${KTX_BUILD_DIR}/interface/python_binding/docs/pyktx." OUTPUT_VARIABLE pyktx_py_rst_filenames)
list(TRANSFORM pyktx_py_rst_filenames REPLACE ".py$" ".rst" OUTPUT_VARIABLE pyktx_py_rst)

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

set(LIBKTX_LIB_DIR ${KTX_BUILD_DIR}/$<CONFIG>)

add_custom_target( pyktx-deps ALL
    COMMENT
        "Python deps")

add_custom_command(
    TARGET pyktx-deps
    COMMAND
        ${PYTHON_EXECUTABLE} -m pip install --no-warn-script-location -r ${SOURCE_DIR}/requirements.txt
    COMMENT
        "Install dependencies for pyktx build"
)

add_custom_target( pyktx ALL
    DEPENDS
        ktx
    WORKING_DIRECTORY
        ${SOURCE_DIR}
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
        ${SOURCE_DIR}
)

# Normalize version number as the python toolchain does. Tweaks are reduced
# to a, b or rc immediately following the patch number. We do because names
# of the BYPRODUCTS in the following custom_command need to match the
# names with normalized version numbers the python tools would produce.
function(normalize_version _var fullver)
    string(REPLACE -alpha a normalized ${fullver})
    string(REPLACE -beta b normalized ${normalized})
    set(${_var} "${normalized}" PARENT_SCOPE)
endfunction()

normalize_version(KTX_VERSION_NORMALIZED ${KTX_VERSION_FULL})
set(DIST_DIR ${KTX_BUILD_DIR}/interface/python_binding/dist)
set(SOURCE_ARCHIVE_BASENAME ${DIST_DIR}/pyktx-${KTX_VERSION_NORMALIZED})

add_custom_command(
    TARGET
        pyktx
    BYPRODUCTS
        ${SOURCE_ARCHIVE_BASENAME}.tar.gz
        ${SOURCE_ARCHIVE_BASENAME}.zip
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
            LIBKTX_VERSION=${KTX_VERSION_NORMALIZED}
        ${PYTHON_EXECUTABLE} -m build --sdist --outdir ${DIST_DIR}
    WORKING_DIRECTORY
        ${SOURCE_DIR}
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
            LIBKTX_VERSION=${KTX_VERSION_NORMALIZED}
        ${PYTHON_EXECUTABLE} -m build --wheel --outdir ${DIST_DIR}
    WORKING_DIRECTORY
        ${SOURCE_DIR}
)

set(pyktx_egg_info
    ${SOURCE_DIR}/pyktx.egg-info/dependency_links.txt
    ${SOURCE_DIR}/pyktx.egg-info/PKG-INFO
    ${SOURCE_DIR}/pyktx.egg-info/requires.txt
    ${SOURCE_DIR}/pyktx.egg-info/SOURCES.txt
    ${SOURCE_DIR}/pyktx.egg-info/top_level.txt)

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
        ${SOURCE_DIR}
)

if(KTX_FEATURE_DOC)
    add_custom_command(
        TARGET pyktx
        POST_BUILD
        BYPRODUCTS
            ${KTX_BUILD_DIR}/interface/python_binding/conf.py
            ${KTX_BUILD_DIR}/interface/python_binding/index.rst
            ${pyktx_py_rst}
            ${KTX_BUILD_DIR}/interface/python_binding/docs/pyktx.rst
            ${KTX_BUILD_DIR}/interface/python_binding/docs/pyktx.native.rst
            ${pyktx_egg_info}
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
                DYLD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{DYLD_LIBRARY_PATH}
                LD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{LD_LIBRARY_PATH}
            ${PYTHON_EXECUTABLE} buildscript.py
        COMMAND
            ${CMAKE_COMMAND} -E copy
                index.rst conf.py ${KTX_BUILD_DIR}/interface/python_binding
        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            ${KTX_BUILD_DIR}/interface/python_binding/_static
        COMMAND
            ${CMAKE_COMMAND} -E copy_directory
            _static ${KTX_BUILD_DIR}/interface/python_binding/_static
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            ${PYTHON_EXECUTABLE} -m sphinx.ext.apidoc -o ${KTX_BUILD_DIR}/interface/python_binding/docs ./pyktx --separate
        COMMAND
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
                LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
                SPHINXBUILD=${PYTHON_PATH}\ -m\ sphinx
            make SOURCEDIR="${KTX_BUILD_DIR}/interface/python_binding" BUILDDIR="${KTX_BUILD_DIR}/interface/python_binding/docs/html/pyktx" html
        WORKING_DIRECTORY
            ${SOURCE_DIR}
    )
endif()
