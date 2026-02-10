# Copyright 2019-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.10)
project(dfdutils C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

option( DFDUTILS_TESTAPPS "Enable DFD Utils tests." ON)
option( DFDUTILS_EMBED "Embed DFD Utils library via object library" OFF)

# Output directory used for all build artifacts
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Optional VULKAN_SDK include directory
if(DEFINED ENV{VULKAN_SDK})
    include_directories($ENV{VULKAN_SDK}/include)
endif()

#
# --- CHECK FOR PERL ---
#
find_package(Perl)
if (Perl_FOUND)
    message(STATUS "Perl found: ${PERL_EXECUTABLE}")


    #
    # --- GENERATE dfd2vk.inl ---
    #
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dfd2vk.inl
            COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/makedfd2vk.pl
            ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_core.h ${CMAKE_CURRENT_SOURCE_DIR}/dfd2vk.inl
            DEPENDS makedfd2vk.pl vulkan/vulkan_core.h
            COMMENT "Generating dfd2vk.inl using makedfd2vk.pl"
    )

    #
    # --- GENERATE vk2dfd.inl ---
    #
    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/vk2dfd.inl
            COMMAND ${PERL_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/makevk2dfd.pl
            ${CMAKE_CURRENT_SOURCE_DIR}/vulkan/vulkan_core.h ${CMAKE_CURRENT_SOURCE_DIR}/vk2dfd.inl
            DEPENDS makevk2dfd.pl vulkan/vulkan_core.h
            COMMENT "Generating vk2dfd.inl using makevk2dfd.pl"
    )
else()
    message(STATUS "Perl NOT found — using pre-existing inline switches.")
endif()

add_custom_target(dfd2vk_inl
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/dfd2vk.inl
)

add_custom_target(vk2dfd_inl
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vk2dfd.inl
)

set(DFDUTILS_LINK_TYPE STATIC)
if(DFDUTILS_EMBED)
    set(DFDUTILS_LINK_TYPE OBJECT)
endif()

#
# --- DFD Utils library ---
#
add_library(dfdutils ${DFDUTILS_LINK_TYPE}
        createdfd.c
        colourspaces.c
        dfd.h
        interpretdfd.c
        KHR/khr_df.h
        printdfd.c
        queries.c
        vk2dfd.c
        dfd2vk.c
        vulkan/vk_platform.h
        vulkan/vulkan_core.h)
set_target_properties(dfdutils PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_dependencies(dfdutils dfd2vk_inl)
add_dependencies(dfdutils vk2dfd_inl)

# ALSO install dfdutils headers
install(DIRECTORY
        ${CMAKE_CURRENT_SOURCE_DIR}/KHR/
        DESTINATION include/KHR
)

if(DFDUTILS_TESTAPPS)

    #
    # --- Build testcreatedfd ---
    #
    add_executable(testcreatedfd
        createdfdtest.c
    )
    target_link_libraries(testcreatedfd PRIVATE dfdutils)

    #
    # --- Build testinterpretdfd ---
    #
    add_executable(testinterpretdfd
        interpretdfdtest.c
    )
    target_link_libraries(testinterpretdfd PRIVATE dfdutils)

    #
    # --- Build testbidirectionalmapping ---
    #
    add_executable(testbidirectionalmapping
        testbidirectionalmapping.c
    )
    target_link_libraries(testbidirectionalmapping PRIVATE dfdutils)
endif()

#
# --- DOXYGEN TARGET ---
#
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # Global
    set( DOXYGEN_OUTPUT_DIRECTORY doc)
    set( DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES )
    set( DOXYGEN_EXTRACT_LOCAL_CLASSES NO )
    set( DOXYGEN_HIDE_UNDOC_MEMBERS YES )
    set( DOXYGEN_CASE_SENSE_NAMES YES )
    set( DOXYGEN_SHOW_USED_FILES NO )
    set( DOXYGEN_VERBATIM_HEADERS NO )
    set( DOXYGEN_CLANG_ASSISTED_PARSING NO )
    set( DOXYGEN_ALPHABETICAL_INDEX NO )
    set( DOXYGEN_DISABLE_INDEX YES )
    set( DOXYGEN_DISABLE_INDEX NO )
    set( DOXYGEN_GENERATE_TREEVIEW YES )
    set( DOXYGEN_GENERATE_LATEX NO )
    set( DOXYGEN_GENERATE_HTML YES )
    set( DOXYGEN_GENERATE_MAN YES )
    set( DOXYGEN_MAN_OUTPUT man )
    # This is to get timestamps with older versions of doxygen.
    # older
    set( DOXYGEN_HTML_TIMESTAMP YES )
    set( DOXYGEN_TIMESTAMP YES )

    doxygen_add_docs(
            docs
            dfd.h
            colourspaces.c
            createdfd.c
            interpretdfd.c
            printdfd.c
            queries.c
            endswap.c
            dfd2vk.c
            vk2dfd.c
            COMMENT "Running Doxygen to build documentation"
    )
else()
    message(WARNING "Doxygen not found. 'doc' target will not be available.")
endif()
