# Create a single file zstd decoder source file

set( zstddeclib_output ${PROJECT_SOURCE_DIR}/lib/zstd/contrib/single_file_libs/zstddeclib.c )


# What a shame! We have to duplicate most of the build commands because
# if(WIN32) can't appear inside add_custom_command.
if(WIN32)
    add_custom_command(OUTPUT ${zstddeclib_output}
        COMMAND "${BASH_EXECUTABLE}" -c "./create_single_file_decoder.sh"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/zstd/contrib/single_file_libs
        COMMENT "Generating zstddeclib.c"
    )
else()
    add_custom_command(OUTPUT ${zstddeclib_output}
        COMMAND ./create_single_file_decoder.sh
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/lib/zstd/contrib/single_file_libs
        COMMENT "Generating zstddeclib.c"
    )
endif()
