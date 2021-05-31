# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Code generation scripts that require a Vulkan SDK installation

set(skip_mkvk_message "-> skipping mkvk target (this is harmless; only needed when re-generating of vulkan headers and dfdutils is required)")

if (NOT IOS)
    # find_package doesn't find the Vulkan SDK when building for IOS.
    # I haven't investigated why.
    find_package(Vulkan)
    if(NOT Vulkan_FOUND)
        message(STATUS "Vulkan SDK not found ${skip_mkvk_message}")
        return()
    endif()
else()
    # Skip mkvk. We don't need to run it when building for iOS.
    return()
endif()

if(CMAKE_HOST_WIN32 AND NOT CYGWIN_INSTALL_PATH)
    # Git for Windows comes with Perl
    # Trick FindPerl into considering default Git location
    set(CYGWIN_INSTALL_PATH "C:\\Program Files\\Git\\usr")
endif()

find_package(Perl)

if(NOT PERL_FOUND)
    message(STATUS "Perl not found ${skip_mkvk_message}")
    return()
endif()

list(APPEND mkvkformatfiles_input
    "lib/dfdutils/vulkan/vulkan_core.h"
    "lib/mkvkformatfiles")
list(APPEND mkvkformatfiles_output
    "${PROJECT_SOURCE_DIR}/lib/vkformat_enum.h"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_check.c"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_str.c")

# What a shame! We have to duplicate most of the build commands because
# if(CMAKE_HOST_WIN32) can't appear inside add_custom_command.
if(CMAKE_HOST_WIN32)
    add_custom_command(OUTPUT ${mkvkformatfiles_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib
        COMMAND "${BASH_EXECUTABLE}" -c "Vulkan_INCLUDE_DIR=${Vulkan_INCLUDE_DIR} lib/mkvkformatfiles lib"
        COMMAND "${BASH_EXECUTABLE}" -c "unix2dos ${PROJECT_SOURCE_DIR}/lib/vkformat_enum.h"
        COMMAND "${BASH_EXECUTABLE}" -c "unix2dos ${PROJECT_SOURCE_DIR}/lib/vkformat_check.c"
        COMMAND "${BASH_EXECUTABLE}" -c "unix2dos ${PROJECT_SOURCE_DIR}/lib/vkformat_str.c"
        DEPENDS ${mkvkformatfiles_input}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating VkFormat-related source files"
        VERBATIM
    )
else()
    add_custom_command(OUTPUT ${mkvkformatfiles_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib
        COMMAND Vulkan_INCLUDE_DIR=${Vulkan_INCLUDE_DIR} lib/mkvkformatfiles lib
        DEPENDS ${mkvkformatfiles_input}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating VkFormat-related source files"
        VERBATIM
    )
endif()

add_custom_target(mkvkformatfiles
    DEPENDS ${mkvkformatfiles_output}
    SOURCES ${mkvkformatfiles_input}
)

list(APPEND makevkswitch_input
    "lib/vkformat_enum.h"
    "lib/dfdutils/makevkswitch.pl")
set(makevkswitch_output
    "${PROJECT_SOURCE_DIR}/lib/dfdutils/vk2dfd.inl")
if(CMAKE_HOST_WIN32)
    add_custom_command(
        OUTPUT ${makevkswitch_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
        COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makevkswitch.pl lib/vkformat_enum.h lib/dfdutils/vk2dfd.inl
        COMMAND "${BASH_EXECUTABLE}" -c "unix2dos ${PROJECT_SOURCE_DIR}/lib/dfdutils/vk2dfd.inl"
        DEPENDS ${makevkswitch_input}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating VkFormat/DFD switch body"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${makevkswitch_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
        COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makevkswitch.pl lib/vkformat_enum.h lib/dfdutils/vk2dfd.inl
        DEPENDS ${makevkswitch_input}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating VkFormat/DFD switch body"
        VERBATIM
    )
endif()

add_custom_target(makevkswitch
    DEPENDS ${makevkswitch_output}
    SOURCES ${makevkswitch_input}
)


list(APPEND makedfd2vk_input
    "lib/vkformat_enum.h"
    "lib/dfdutils/makedfd2vk.pl")
list(APPEND makedfd2vk_output
    "${PROJECT_SOURCE_DIR}/lib/dfdutils/dfd2vk.inl")

if(CMAKE_HOST_WIN32)
    add_custom_command(
        OUTPUT ${makedfd2vk_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
        COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makedfd2vk.pl lib/vkformat_enum.h lib/dfdutils/dfd2vk.inl
        COMMAND "${BASH_EXECUTABLE}" -c "unix2dos ${PROJECT_SOURCE_DIR}/lib/dfdutils/dfd2vk.inl"
        DEPENDS ${makedfd2vk_input}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating DFD/VkFormat switch body"
        VERBATIM
    )
else()
    add_custom_command(
        OUTPUT ${makedfd2vk_output}
        COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
        COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makedfd2vk.pl lib/vkformat_enum.h lib/dfdutils/dfd2vk.inl
        DEPENDS ${makedfd2vk_input}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating DFD/VkFormat switch body"
        VERBATIM
    )
endif()

add_custom_target(makedfd2vk
    DEPENDS ${makedfd2vk_output}
    SOURCES ${makedfd2vk_input}
)

add_custom_target(mkvk SOURCES ${CMAKE_CURRENT_LIST_FILE})

add_dependencies(mkvk
    mkvkformatfiles
    makevkswitch
    makedfd2vk
)
