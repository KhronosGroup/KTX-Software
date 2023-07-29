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

if(LINUX)
    set(LIBKTX_LIB_DIR ${KTX_BUILD_DIR})
else()
    set(LIBKTX_LIB_DIR ${KTX_BUILD_DIR}/$<CONFIG>)
endif()

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
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
            LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            LIBKTX_VERSION=${PROJECT_VERSION}
        ${PYTHON_EXECUTABLE} ./setup.py sdist
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)

add_custom_command(
    DEPENDS
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist/libktx-${PROJECT_VERSION}.tgz
    OUTPUT
        ${CMAKE_SOURCE_DIR}/interface/python_binding/dist
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
            LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            LIBKTX_VERSION=${PROJECT_VERSION}
        ${PYTHON_EXECUTABLE} ./setup.py bdist_wheel
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

add_custom_command(
    TARGET pyktx
    PRE_BUILD
    COMMAND
        ${PYTHON_EXECUTABLE} -m pip install -r ${CMAKE_SOURCE_DIR}/interface/python_binding/requirements.txt
    COMMENT
        "Install dependencies for pyktx build"
)

if (NOT WIN32) # Heap corruption on Windows somehow
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

add_test(NAME pyktx
    COMMAND
        ${CMAKE_COMMAND} -E env
            LIBKTX_INCLUDE_DIR=${CMAKE_SOURCE_DIR}/include
            LIBKTX_LIB_DIR=${LIBKTX_LIB_DIR}
            KTX_RUN_TESTS=ON
            DYLD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{DYLD_LIBRARY_PATH}
            LD_LIBRARY_PATH=${LIBKTX_LIB_DIR}:$ENV{LD_LIBRARY_PATH}
        ${PYTHON_EXECUTABLE} build.py
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}/interface/python_binding
)
endif()
