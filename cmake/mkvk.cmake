# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Code generation scripts that require a Vulkan SDK installation

# TODO: Rewrite scripts to generate the files from the Vulkan
# registry, vk.xml, in the Vulkan-Docs repo.

if (NOT IOS AND NOT ANDROID)
# Not needed as local custom vulkan_core.h is used. Keeping
# in case we go back to the standard one.
#    # find_package doesn't find the Vulkan SDK when building for IOS.
#    # I haven't investigated why.
#    find_package(Vulkan REQUIRED)
    set(Vulkan_INCLUDE_DIR lib/dfdutils/vulkan)
else()
    # Skip mkvk. We don't need to run it when building for iOS or Android.
    return()
endif()

if(CMAKE_HOST_WIN32 AND NOT CYGWIN_INSTALL_PATH)
    # Git for Windows comes with Perl
    # Trick FindPerl into considering default Git location
    set(CYGWIN_INSTALL_PATH "C:\\Program Files\\Git\\usr")
endif()

find_package(Perl REQUIRED)

list(APPEND mkvkformatfiles_input
    "${Vulkan_INCLUDE_DIR}/vulkan_core.h"
    lib/mkvkformatfiles)
list(APPEND mkvkformatfiles_output
    "${PROJECT_SOURCE_DIR}/lib/vkformat_enum.h"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_check.c"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_str.c")
list(APPEND mkvkformatfiles_command
    Vulkan_INCLUDE_DIR="${Vulkan_INCLUDE_DIR}" lib/mkvkformatfiles lib)
if(CMAKE_HOST_WIN32)
    list(JOIN mkvkformatfiles_command " " mffc_string)
    set(mkvkformatfiles_command "${BASH_EXECUTABLE}" -c "${mffc_string}")
endif()

add_custom_command(OUTPUT ${mkvkformatfiles_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib
    COMMAND ${mkvkformatfiles_command}
    DEPENDS ${mkvkformatfiles_input}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating VkFormat-related source files"
    VERBATIM
)

add_custom_target(mkvkformatfiles
    DEPENDS ${mkvkformatfiles_output}
    SOURCES ${mkvkformatfiles_input}
)

list(APPEND makevk2dfd_input
    "${Vulkan_INCLUDE_DIR}/vulkan_core.h"
    lib/dfdutils/makevk2dfd.pl)
set(makevk2dfd_output
    "${PROJECT_SOURCE_DIR}/lib/dfdutils/vk2dfd.inl")

add_custom_command(
    OUTPUT ${makevk2dfd_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
    COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makevk2dfd.pl ${Vulkan_INCLUDE_DIR}/vulkan_core.h lib/dfdutils/vk2dfd.inl
    DEPENDS ${makevk2dfd_input}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating VkFormat/DFD switch body"
    VERBATIM
)

add_custom_target(makevk2dfd
    DEPENDS ${makevk2dfd_output}
    SOURCES ${makevk2dfd_input}
)


list(APPEND makedfd2vk_input
#    "lib/vkformat_enum.h"
    "${Vulkan_INCLUDE_DIR}/vulkan_core.h"
    "lib/dfdutils/makedfd2vk.pl")
list(APPEND makedfd2vk_output
    "${PROJECT_SOURCE_DIR}/lib/dfdutils/dfd2vk.inl")

add_custom_command(
    OUTPUT ${makedfd2vk_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
    COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makedfd2vk.pl ${Vulkan_INCLUDE_DIR}/vulkan_core.h lib/dfdutils/dfd2vk.inl
    DEPENDS ${makedfd2vk_input}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating DFD/VkFormat switch body"
    VERBATIM
)

add_custom_target(makedfd2vk
    DEPENDS ${makedfd2vk_output}
    SOURCES ${makedfd2vk_input}
)

add_custom_target(mkvk SOURCES ${CMAKE_CURRENT_LIST_FILE})

add_dependencies(mkvk
    mkvkformatfiles
    makevk2dfd
    makedfd2vk
)
