#! /usr/bin/env cmake -P

# Copyright 2025 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Run the program, given by the 5th argument, the number of times given by the 4th argument.
# Pass the remaining arguments, if any, to the program.
# The first 3 arguments are cmake, -P, and launch.cmake.

if( NOT ${CMAKE_ARGC} GREATER_EQUAL 5 )
    message( FATAL_ERROR "Usage: ${CMAKE_ARGV0} ${CMAKE_ARGV1} ${CMAKE_ARGV2} <runs> <program>" )
endif()

foreach( n RANGE 5 ${CMAKE_ARGC} )
    list(APPEND args ${CMAKE_ARGV${n}})
endforeach()

foreach( i RANGE 1 ${CMAKE_ARGV3} )
    execute_process(
        COMMAND ${CMAKE_ARGV4} ${args}
        RESULT_VARIABLE result
        ENCODING NONE
        COMMAND_ERROR_IS_FATAL ANY
    )
endforeach()
