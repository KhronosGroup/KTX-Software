# Copyright 2025 The Khronos Group Inc.
# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.27) # 3.27 for genex TARGET_IMPORT_FILE artifacts.

option( KTX_PY_USE_VENV
    "Use a Python virtual environment. Needed for externally managed python installations."
    OFF
)

find_package (Python3 COMPONENTS Interpreter)
file(GLOB pyktx_py_src ${CMAKE_CURRENT_SOURCE_DIR}/pyktx/*.py)
list(TRANSFORM pyktx_py_src REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/pyktx/" "${KTX_BUILD_DIR}/interface/python_binding/docs/pyktx." OUTPUT_VARIABLE pyktx_py_rst_filenames)
list(TRANSFORM pyktx_py_rst_filenames REPLACE ".py$" ".rst" OUTPUT_VARIABLE pyktx_py_rst)

set(PYTHON_EXECUTABLE_SYSTEM ${Python3_EXECUTABLE})
if(DEFINED PYTHON AND NOT ${PYTHON} STREQUAL "")
    set(PYTHON_EXECUTABLE_SYSTEM ${PYTHON})
    message(STATUS "Override PYTHON with ${PYTHON}")
endif()
if (LINUX AND NOT Python3_FOUND)
    set(PYTHON_EXECUTABLE_SYSTEM python)
    message(STATUS "CMake failed to find python3. Will continue assuming it's on PATH")
endif()
if(KTX_PY_USE_VENV)
    set(PYTHON_VENV_DIR "${KTX_BUILD_DIR}/interface/python_binding")
    set(PYTHON_EXECUTABLE ${PYTHON_VENV_DIR}/bin/python3)
    message(STATUS "Using virtual environment for Python")
else()
  set(PYTHON_EXECUTABLE ${PYTHON_EXECUTABLE_SYSTEM})
endif()

# Convert Windows path to CMake path
cmake_path(SET PYTHON_PATH ${PYTHON_EXECUTABLE})

set(libktx_include_dir $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>)
# IMPORT_FILE_DIR is typically only set when building a Windows DLL and
# points to the directory with the .lib import file.
set(libktx_import_dir $<IF:$<BOOL:$<TARGET_IMPORT_FILE_DIR:ktx>>,$<TARGET_IMPORT_FILE_DIR:ktx>,$<TARGET_FILE_DIR:ktx>>)
set(libktx_lib_dir $<TARGET_FILE_DIR:ktx>)
#add_custom_target(debug_libktx_dirs COMMAND ${CMAKE_COMMAND} -E echo "libktx_include_dir = ${libktx_include_dir}, libktx_import_dir = ${libktx_import_dir}, libktx_lib_dir = ${libktx_lib_dir}")

if(KTX_PY_USE_VENV)
    add_custom_target( py-venv ALL
        COMMENT
            "Set up virtual environment for Python"
    )
    add_custom_command(
        TARGET py-venv
        PRE_BUILD
        COMMAND
            ${PYTHON_EXECUTABLE_SYSTEM} -m venv ${PYTHON_VENV_DIR}
        COMMENT
            "Create virtual environment for Python"
    )
endif()

add_custom_target( pyktx-deps ALL
    COMMENT
        "Python deps"
)
if(KTX_PY_USE_VENV)
    add_dependencies(pyktx-deps py-venv)
endif()
add_custom_command(
    TARGET pyktx-deps
    PRE_BUILD
    COMMAND
        ${PYTHON_EXECUTABLE} -m pip install --no-warn-script-location -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
    COMMENT
        "Install dependencies for pyktx build"
)

add_custom_target( pyktx ALL
    DEPENDS
        ktx
        ${pyktx_py_src}
        pyktx/ktx_texture.h
        pyktx/ktx_texture1.h
        pyktx/ktx_texture2.h
        pyktx/ktx_texture.c
        pyktx/ktx_texture1.c
        pyktx/ktx_texture2.c
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
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
        ${CMAKE_CURRENT_SOURCE_DIR}
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

cmake_print_variables(BASH_EXECUTABLE)
add_custom_command(
    TARGET pyktx
    POST_BUILD
    BYPRODUCTS
        ${SOURCE_ARCHIVE_BASENAME}.tar.gz
        ${SOURCE_ARCHIVE_BASENAME}.zip
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${libktx_include_dir}
            LIBKTX_IMPORT_DIR=${libktx_import_dir}
            LIBKTX_LIB_DIR=${libktx_lib_dir}
            LIBKTX_VERSION=${KTX_VERSION_NORMALIZED}
        # The build module by default builds in an isolated environment, i.e. it
        # requires a virtual env. I have not found a way via find_package to
        # ensure venv support is installed. This can be turned off with
        # `--no-isolation` but such builds have not been tested.
        ${PYTHON_EXECUTABLE} -m build --sdist --outdir ${DIST_DIR}
    COMMENT
        "Build pyktx source package"
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
    TARGET pyktx
    POST_BUILD
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${libktx_include_dir}
            LIBKTX_IMPORT_DIR=${libktx_import_dir}
            LIBKTX_LIB_DIR=${libktx_lib_dir}
            LIBKTX_VERSION=${KTX_VERSION_NORMALIZED}
        # Ditto with sdist isolated environment comment.
        ${PYTHON_EXECUTABLE} -m build --wheel --outdir ${DIST_DIR}
    COMMENT
        "Build pyktx wheel"
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set(pyktx_egg_info
    ${CMAKE_CURRENT_SOURCE_DIR}/pyktx.egg-info/dependency_links.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/pyktx.egg-info/PKG-INFO
    ${CMAKE_CURRENT_SOURCE_DIR}/pyktx.egg-info/requires.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/pyktx.egg-info/SOURCES.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/pyktx.egg-info/top_level.txt)

add_test(NAME pyktx
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${libktx_include_dir}
            LIBKTX_IMPORT_DIR=${libktx_import_dir}
            LIBKTX_LIB_DIR=${libktx_lib_dir}
            KTX_RUN_TESTS=ON
            DYLD_LIBRARY_PATH=${libktx_lib_dir}:$ENV{DYLD_LIBRARY_PATH}
            LD_LIBRARY_PATH=${libktx_lib_dir}:$ENV{LD_LIBRARY_PATH}
        ${PYTHON_EXECUTABLE} buildscript.py
    WORKING_DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}
)

if(KTX_FEATURE_DOC)
    add_custom_target(pyktx.doc ALL
      DEPENDS
        pyktx-deps
        ktx    # No way to avoid this autodoc needs compiled modules.
      COMMENT
          "Build Python documentation"
    )

    add_custom_command(
        TARGET pyktx.doc
        POST_BUILD
        BYPRODUCTS
            ${KTX_BUILD_DIR}/interface/python_binding/conf.py
            ${KTX_BUILD_DIR}/interface/python_binding/index.rst
            ${pyktx_py_rst}
            ${KTX_BUILD_DIR}/interface/python_binding/docs/pyktx.rst
            ${KTX_BUILD_DIR}/interface/python_binding/docs/pyktx.native.rst
            ${pyktx_egg_info}
        COMMAND
            # This appears to build binaries of the native parts in the source pyktx
            # folder. Without the binaries, autodoc can't import the modules. It
            # prints warnings and fails to include any content in the documentation
            # for the native interfaces in the HTML files it creates.
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${libktx_include_dir}
                LIBKTX_IMPORT_DIR=${libktx_import_dir}
                LIBKTX_LIB_DIR=${libktx_lib_dir}
                DYLD_LIBRARY_PATH=${libktx_lib_dir}:$ENV{DYLD_LIBRARY_PATH}
                LD_LIBRARY_PATH=${libktx_lib_dir}:$ENV{LD_LIBRARY_PATH}
            ${PYTHON_EXECUTABLE} buildscript.py
        COMMAND
            ${CMAKE_COMMAND} -E copy
                index.rst conf.py ${KTX_BUILD_DIR}/interface/python_binding
        # Currently there are no static items to be included in the Sphinx generated
        # documentation. The infrastructure is here in case of future need. If
        # these command are removed, also remove html_static_path from conf.py.
        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            ${KTX_BUILD_DIR}/interface/python_binding/_static
        COMMAND
            ${CMAKE_COMMAND} -E copy_directory
            _static ${KTX_BUILD_DIR}/interface/python_binding/_static
        COMMAND
            # Sphinx Apidoc extracts docstrings from the source files and builds .rst files
            # in the docs directory of the python_binding section of the build directory.
            # `--separate` says to put the documentation for each module on its own page.
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=${libktx_include_dir}
                LIBKTX_LIB_DIR=${libktx_lib_dir}
            ${PYTHON_EXECUTABLE} -m sphinx.ext.apidoc -o ${KTX_BUILD_DIR}/interface/python_binding/docs ./pyktx --separate
        COMMAND
            # Sphinx uses autodoc to generate html files from the .rst files generated above.
            # These are expected to be in a docs subdirectory of the SOURCEDIR hence the
            # build directory appearing in SOURCEDIR. Sphinx invokes autodoc which wants to
            # import the modules hence libktx, must be built for this to fully succeed.
            #
            # Autodoc is configured by the copy of `conf.py` in the build directory (copied
            # there by one of the COMMANDs above). `conf.py` adds the python binding
            # section of the build directory to Python's `sys.path` so Autodoc can find the
            # modules. However they aren't there. The `pyktx` module is the directory
            # ${CMAKE_CURRENT_SOURCE_DIR}/pyktx and the .o files from compiling the native
            # parts are built there by buildscript.py when invoked above.
            # ${CMAKE_CURRENT_SOURCE_DIR} appears in Python's sys.path so autodoc finds them.
            # There is nothing explicit in `conf.py` to do this. It appears that Sphinx or
            # autodoc itself adds its working directory to `sys.path`.
            ${CMAKE_COMMAND} -E env
                LIBKTX_INCLUDE_DIR=$<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
                LIBKTX_LIB_DIR=${libktx_lib_dir}
                SPHINXBUILD=${PYTHON_PATH}\ -m\ sphinx
            make SOURCEDIR="${KTX_BUILD_DIR}/interface/python_binding" BUILDDIR="${KTX_BUILD_DIR}/interface/python_binding/docs/html/pyktx" html
        WORKING_DIRECTORY
            ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()
