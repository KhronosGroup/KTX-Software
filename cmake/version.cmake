#
# Create a version.h header file using the mkversion shell script
#

function( create_version_header dest_path target )

    set( version_h_output ${dest_path}/version.h)

    if(WIN32)
        add_custom_command(
            OUTPUT ${version_h_output}
            COMMAND call call "setup_env.bat" && bash -c "\"./mkversion\" \"-o\" \"version.h\" \"${dest_path}\""
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT Generate ${version_h_output}
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${version_h_output}
            COMMAND ./mkversion -o version.h ${dest_path}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMENT Generate ${version_h_output}
            VERBATIM
        )
    endif()

    set( version_target ${target}_version )
    add_custom_target( ${version_target} DEPENDS ${version_h_output} )
    add_dependencies( ${target} ${version_target} )
    target_sources( ${target} PRIVATE ${version_h_output} )

endfunction()
