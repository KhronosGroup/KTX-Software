# Copyright 2015-2025 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Useful macros

macro(list_contains list item result)
    list(FIND ${list} ${item} tmp_list_index)
    if(${tmp_list_index} GREATER_EQUAL 0)
        set(${result} ON)
    else()
        set(${result} OFF)
    endif()
endmacro()
