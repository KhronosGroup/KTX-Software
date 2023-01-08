# Copyright 2023 Shukant Pal
# SPDX-License-Identifier: Apache-2.0

find_package (Python3 COMPONENTS Interpreter)
file(GLOB pyktx_py_src "pyktx/*.py")

set(PYTHON_EXECUTABLE ${Python3_EXECUTABLE})
if(DEFINED PYTHON AND NOT ${PYTHON} STREQUAL "")
    set(PYTHON_EXECUTABLE ${PYTHON})
    message(STATUS "Override PYTHON with ${PYTHON}")
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
            LIBKTX_LIB_DIR=${KTX_BUILD_DIR}/$<CONFIG>
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

add_custom_command(
    TARGET pyktx
    PRE_BUILD
    COMMAND
       ${PYTHON_EXECUTABLE} -c \"import sys\; print('python: ' + sys.version)\"
    COMMENT
        "Python version"
)
