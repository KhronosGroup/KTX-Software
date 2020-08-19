# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

#
# Retrieving the current version from GIT tags
#

include(GetGitRevisionDescription)

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
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	if(NOT res EQUAL 0)
		set(out "${out}-${res}-NOTFOUND")
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

# Get latest tag
git_describe_raw(KTX_VERSION_FULL --abbrev=0 --match v[0-9]*)
#message("KTX full version: ${KTX_VERSION_FULL}")

# generate_version(TOKTX_VERSION tools/toktx)
# message("TOKTX_VERSION: ${TOKTX_VERSION}")

# First try a full regex ( vMARJOR.MINOR.PATCH-TWEAK )
string(REGEX MATCH "^v([0-9]*)\.([0-9]*)\.([0-9]*)(-[^\.]*)"
       KTX_VERSION ${KTX_VERSION_FULL})

if(KTX_VERSION)
    set(KTX_VERSION_MAJOR ${CMAKE_MATCH_1})
    set(KTX_VERSION_MINOR ${CMAKE_MATCH_2})
    set(KTX_VERSION_PATCH ${CMAKE_MATCH_3})
    set(KTX_VERSION_TWEAK ${CMAKE_MATCH_4})
else()
    # If full regex failed, go for vMARJOR.MINOR.PATCH
    string(REGEX MATCH "^v([0-9]*)\.([0-9]*)\.([^\.]*)"
            KTX_VERSION ${KTX_VERSION_FULL})

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

# message("KTX version: ${KTX_VERSION}  major: ${KTX_VERSION_MAJOR} minor:${KTX_VERSION_MINOR} patch:${KTX_VERSION_PATCH} tweak:${KTX_VERSION_TWEAK}")


#
# Create a version.h header file using the mkversion shell script
#

function( create_version_header dest_path target )

    set( version_h_output ${dest_path}/version.h)

    if(CMAKE_HOST_WIN32)
        add_custom_command(
            OUTPUT ${version_h_output}
            # On Windows this command has to be invoked by a shell in order to work
            COMMAND ${BASH_EXECUTABLE} -c "\"./mkversion\" \"-o\" \"version.h\" \"${dest_path}\""
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT "Generate ${version_h_output}"
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${version_h_output}
            COMMAND ./mkversion -o version.h ${dest_path}
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
