# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

# Code generation scripts that require a Vulkan SDK installation

# TODO: Rewrite scripts to generate the files from the Vulkan
# registry, vk.xml, in the Vulkan-Docs repo.

# NOTE: Since this must be explicitly included by setting an option,
# require sought packages.
#
# CAUTION: Outputs of custom commands are deleted during a clean
# operation so these targets result in clean deleting what are normally
# considered source files. There appears to be no easy way to avoid
# this. Since only project developers need to use these targets, and
# only occasionally, this misfeature can be tolerated.

if (NOT IOS AND NOT ANDROID)
# Not needed as local custom vulkan_core.h is used. Keeping
# in case we go back to the standard one.
#    # find_package doesn't find the Vulkan SDK when building for IOS.
#    # I haven't investigated why.
#    find_package(Vulkan REQUIRED)

    # This cmake file is included from its parent so has the same scope as
    # the including file. If we change Vulkan_INCLUDE_DIR, other parts will
    # be affected.
    set(mkvk_vulkan_include_dir lib/dfdutils)
else()
    # Skip mkvk. There is no need to use iOS or Android to regenerate
    # the files.
    return()
endif()

set(vulkan_header "${mkvk_vulkan_include_dir}/vulkan/vulkan_core.h")

# CAUTION: On Windows use a version of Perl built for Windows, i.e. not
# one found in Cygwin or MSYS (Git for Windows). This is needed so the
# generated files have the correct the correct CRLF line endings. The ones
# mentioned write LF line endings (possibly related to some Cygwin or MSYS
# installation setting for handling of text files). The Perl scripts,
# unlike the Awk scripts, have not been modified to always write CRLF
# on Windows.
#
# Strawberry Perl via Chocolatey is recommended.
#    choco install strawberryperl

#if(CMAKE_HOST_WIN32 AND NOT CYGWIN_INSTALL_PATH)
#    # Git for Windows comes with Perl
#    # Trick FindPerl into considering default Git location
#    set(CYGWIN_INSTALL_PATH "C:\\Program Files\\Git\\usr")
#endif()

find_package(Perl REQUIRED)

list(APPEND mkvkformatfiles_input
    ${vulkan_header}
    ci_scripts/mkvkformatfiles)
list(APPEND mkvkformatfiles_output
    "${PROJECT_SOURCE_DIR}/interface/java_binding/src/main/java/org/khronos/ktx/VkFormat.java"
    "${PROJECT_SOURCE_DIR}/interface/python_binding/pyktx/vk_format.py"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_enum.h"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_typesize.c"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_check.c"
    "${PROJECT_SOURCE_DIR}/lib/vkformat_str.c"
    "${PROJECT_SOURCE_DIR}tests/unittests/vkformat_list.inl")

# CAUTION: When a COMMAND contains VAR="Value" CMake messes up the escaping
# for Bash. With or without VERBATIM, if Value has no spaces CMake changes it
# to VAR=\"Value\". If it has spaces CMake changes it to "VAR=\"Value\"".
# The first causes the quotes to leak into the command that is reading VAR
# breaking, e.g. opening a file that has VAR's value as part of its name.
# The second causes Bash to look for the command 'VAR="Value"' causing it
# to exit with error.
#
# The only workaround I've found is to put the command in a string and invoke
# it with bash -c. This is what we'd have to do on Windows anyway as COMMAND
# defaults to cmd or powershell (not sure which). In this case CMake passes
# to bash a string of the form '"VAR=\"Value\" command arg ..."' which bash
# parses successfully.

list(APPEND mvffc_as_list
    ci_scripts/mkvkformatfiles ./ ${vulkan_header})
list(JOIN mvffc_as_list " " mvffc_as_string)
    set(mkvkformatfiles_command "${BASH_EXECUTABLE}" -c "${mvffc_as_string}")

add_custom_command(OUTPUT ${mkvkformatfiles_output}
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
    ${vulkan_header}
    lib/dfdutils/makevk2dfd.pl)
set(makevk2dfd_output
    "${PROJECT_SOURCE_DIR}/lib/dfdutils/vk2dfd.inl")

add_custom_command(
    OUTPUT ${makevk2dfd_output}
    COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makevk2dfd.pl ${vulkan_header} lib/dfdutils/vk2dfd.inl
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
    ${vulkan_header}
    lib/dfdutils/makedfd2vk.pl)
list(APPEND makedfd2vk_output
    "${PROJECT_SOURCE_DIR}/lib/dfdutils/dfd2vk.inl")

add_custom_command(
    OUTPUT ${makedfd2vk_output}
    COMMAND ${CMAKE_COMMAND} -E make_directory lib/dfdutils
    COMMAND "${PERL_EXECUTABLE}" lib/dfdutils/makedfd2vk.pl ${vulkan_header} lib/dfdutils/dfd2vk.inl
    DEPENDS ${makedfd2vk_input}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating DFD/VkFormat switch body"
    VERBATIM
)

add_custom_target(makedfd2vk
    DEPENDS ${makedfd2vk_output}
    SOURCES ${makedfd2vk_input}
)

find_program(RUBY_EXECUTABLE
    NAMES ruby ruby3.1 ruby31 ruby3.0 ruby30
    # N.B. Must use HINTS. PATHS are searched after default paths.
    HINTS $ENV{MY_RUBY_HOME}/bin
    REQUIRED)
find_path(KTX_SPECIFICATION
    NAME formats.json
    PATHS ${PROJECT_SOURCE_DIR}/../KTX-Specification
    NO_DEFAULT_PATH
    REQUIRED)

list(APPEND makevk2gl_input
    ${KTX_SPECIFICATION}/generate_format_switches.rb
    ${KTX_SPECIFICATION}/formats.json)
list(APPEND makevk2gl_output
    "${PROJECT_SOURCE_DIR}/lib/vkFormat2glFormat.inl"
    "${PROJECT_SOURCE_DIR}/lib/vkFormat2glInternalFormat.inl"
    "${PROJECT_SOURCE_DIR}/lib/vkFormat2glType.inl")
# Until we have D3D or Metal loaders these outputs of
# generate_format_switches.rb are unneeded.
list(APPEND makevk2gl_extraneous_files
    "${PROJECT_SOURCE_DIR}/lib/vkFormat2dxgiFormat.inl"
    "${PROJECT_SOURCE_DIR}/lib/vkFormat2mtlFormat.inl"
)

add_custom_command(
    OUTPUT ${makevk2gl_output}
    COMMAND "${RUBY_EXECUTABLE}" ${KTX_SPECIFICATION}/generate_format_switches.rb ${PROJECT_SOURCE_DIR}/lib
    COMMAND ${CMAKE_COMMAND} -E rm -f ${makevk2gl_extraneous_files}
    DEPENDS ${makevk2gl_input}
    WORKING_DIRECTORY ${KTX_SPECIFICATION}
    COMMENT "Generating VkFormat to OpenGL internal format, format and type switches"
    VERBATIM
)

add_custom_target(makevk2gl
    DEPENDS ${makevk2gl_output}
    SOURCES ${makedvk2gl_input}
)

add_custom_target(mkvk SOURCES ${CMAKE_CURRENT_LIST_FILE})

add_dependencies(mkvk
    mkvkformatfiles
    makevk2dfd
    makedfd2vk
    makevk2gl
)
