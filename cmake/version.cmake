# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

#
# Retrieving the current version from GIT tags
#

include(GetGitRevisionDescription)
include(CMakePrintHelpers)

find_package(Bash REQUIRED)

function(git_update_index)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()
    execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        update-index -q --refresh
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        )
    if(NOT res EQUAL 0)
        message(SEND_ERROR "git update-index not successful")
    endif()
endfunction()

# `man 7 gitrevisions` for the meaning of the output from git describe.
function(git_describe_raw _var)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()
    if(NOT GIT_FOUND)
        set(${_var} "GIT-NOTFOUND" PARENT_SCOPE)
        return()
    endif()

    execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        describe
        ${ARGN}
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        out
        #ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT res EQUAL 0)
        set(out "exitcode-${res}-NOTFOUND")
    endif()

    set(${_var} "${out}" PARENT_SCOPE)
endfunction()

function(git_rev_list target_path _var)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()
    execute_process(COMMAND
        "${GIT_EXECUTABLE}"
        rev-list -1 HEAD ${target_path}
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
        OUTPUT_VARIABLE
        out
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT res EQUAL 0)
        message(SEND_ERROR "git update-index not successful")
    endif()
    set(${_var} "${out}" PARENT_SCOPE)
endfunction()

function(git_dirty _var)
    if(NOT GIT_FOUND)
        find_package(Git QUIET)
    endif()
    execute_process(COMMAND
        diff-index --name-only HEAD --
        WORKING_DIRECTORY
        "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE
        res
    )
    set(${_var} "${res}" PARENT_SCOPE)
endfunction()


function(generate_version _var )
    if(${ARGC} GREATER 1)
        set(target_path ${ARGN})
        git_rev_list(${target_path} KTX_REV)
        git_describe_raw(KTX_VERSION --contains --match v[0-9]* ${KTX_REV})
        if(NOT KTX_VERSION)
            git_describe_raw(KTX_VERSION "--match" "v[0-9]*" ${KTX_REV})
        endif()
    else()
        git_describe_raw(KTX_VERSION "--match" "v[0-9]*" "HEAD" )
    endif()

    git_update_index()
    git_dirty(GIT_DIRTY)
    if(GIT_DIRTY)
        set(KTX_VERSION ${KTX_VERSION}-dirty)
    endif()
    set(${_var} "${KTX_VERSION}" PARENT_SCOPE)
endfunction()

# Get latest tag from git if not passed to cmake.
# This property can be passed to cmake when building from tar.gz
# This value will only change when a config is run. While not ideal,
# this limitation is acceptable for a value that is used only to label
# installer packages, etc. as they are built by CI which always starts
# with a clean slate and configuration of the project.
if(NOT KTX_GIT_VERSION_FULL)
    git_describe_raw(KTX_GIT_VERSION_FULL --abbrev=0 --match v[0-9]*)
    # No MKV_VERSION_OPT so mkversion will run git describe at build
    # time.
else()
    # Set up option to pass version on to mkversion when generating
    # version.h files.
    set(MKV_VERSION_OPT -v ${KTX_GIT_VERSION_FULL})
endif()
#message("KTX git full version: ${KTX_GIT_VERSION_FULL}")
#cmake_print_variables(MKV_VERSION_OPT)

# First try a full regex ( vMAJOR.MINOR.PATCH-TWEAK )
string(REGEX MATCH "^v([0-9]*)\.([0-9]*)\.([0-9]*)(-[^\.]*)"
       KTX_VERSION ${KTX_GIT_VERSION_FULL})

if(KTX_VERSION)
    set(KTX_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(KTX_VERSION_MINOR ${CMAKE_MATCH_2})
    set(KTX_VERSION_PATCH ${CMAKE_MATCH_3})
    set(KTX_VERSION_TWEAK ${CMAKE_MATCH_4})
else()
    # If full regex failed, go for vMAJOR.MINOR.PATCH
    string(REGEX MATCH "^v([0-9]*)\.([0-9]*)\.([^\.]*)"
            KTX_VERSION ${KTX_GIT_VERSION_FULL})

    if(KTX_VERSION)
        set(KTX_VERSION_MAJOR ${CMAKE_MATCH_1})
        set(KTX_VERSION_MINOR ${CMAKE_MATCH_2})
        set(KTX_VERSION_PATCH ${CMAKE_MATCH_3})

        string(REGEX MATCH "^[0-9]*$"
            KTX_VERSION_PATCH_INT ${KTX_VERSION_PATCH})

        if(KTX_VERSION_PATCH_INT)
            set(KTX_VERSION_TWEAK "")
        else()
            if(KTX_VERSION_PATCH)
                set(KTX_VERSION_TWEAK "-${KTX_VERSION_PATCH}")
            else()
                set(KTX_VERSION_TWEAK "")
            endif()
            set(KTX_VERSION_PATCH "0")
        endif()
    else()
        message(WARNING "Error retrieving version from GIT tag. Falling back to 0.0.0-noversion ")
        set(KTX_VERSION_MAJOR "0" )
        set(KTX_VERSION_MINOR "0" )
        set(KTX_VERSION_PATCH "0" )
        set(KTX_VERSION_TWEAK "-noversion" )
    endif()
endif()

set(KTX_VERSION ${KTX_VERSION_MAJOR}.${KTX_VERSION_MINOR}.${KTX_VERSION_PATCH})
set(KTX_VERSION_FULL ${KTX_VERSION}${KTX_VERSION_TWEAK})

#cmake_print_variables(KTX_VERSION KTX_VERSION_FULL)
#cmake_print_variables(KTX_VERSION_MAJOR KTX_VERSION_MINOR KTX_VERSION_PATCH KTX_VERSION_TWEAK)

function( create_version_header dest_path target )

    # N.B. cmake_print_variables(CMAKE_CURRENT_FUNCTION_LIST_DIR) will
    # print the location of the cmake_print_variables function.
    set( mkversion "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../scripts/mkversion" )
    set( version_h_output ${PROJECT_SOURCE_DIR}/${dest_path}/version.h)
    if(CMAKE_HOST_WIN32)
        add_custom_command(
            OUTPUT ${version_h_output}
            # On Windows this command has to be invoked by a shell in order to work
            COMMAND "${BASH_EXECUTABLE}" -- "${mkversion}" ${MKV_VERSION_OPT} -o version.h "${dest_path}"
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Generate ${version_h_output}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${version_h_output}
            COMMAND ${mkversion} ${MKV_VERSION_OPT} -o version.h ${dest_path}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Generate ${version_h_output}"
            VERBATIM
        )
    endif()

    set( version_target ${target}_version )
    add_custom_target( ${version_target} DEPENDS ${version_h_output} )
    add_dependencies( ${target} ${version_target} )
    target_sources( ${target} PRIVATE ${version_h_output} )

endfunction()

function( create_version_file )
    file(WRITE ${PROJECT_BINARY_DIR}/ktx.version "${KTX_VERSION_FULL}")
endfunction()

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
