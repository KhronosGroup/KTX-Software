# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4 expandtab:

# Copyright 2022 Mark Callow
# SPDX-License-Identifier: Apache-2.0

add_test( NAME ktx2ktx2-test-help
    COMMAND ktx2ktx2 --help
)
set_tests_properties(
    ktx2ktx2-test-help
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktx2ktx2"
)

add_test( NAME ktx2ktx2-test-version
    COMMAND ktx2ktx2 --version
)
set_tests_properties(
    ktx2ktx2-test-version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^ktx2ktx2 v[0-9][0-9\\.]+"
)

# The near duplication of this and other tests below is due to a "limitation"
# (i.e. a bug) in ctest which checks for neither a zero error code when a
# PASS_REGULAR_EXPRESSION is specified nor a non-zero error code when a
# FAIL_REGULAR_EXPRESSION is specified but only for matches to the REs.
add_test( NAME ktx2ktx2-test-foobar
    COMMAND ktx2ktx2 --foobar
)
set_tests_properties(
    ktx2ktx2-test-foobar
PROPERTIES
    WILL_FAIL TRUE
    FAIL_REGULAR_EXPRESSION "^Usage: ktx2ktx2"
)
add_test( NAME ktx2ktx2-test-foobar-exit-code
    COMMAND ktx2ktx2 --foobar
)
set_tests_properties(
    ktx2ktx2-test-foobar-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

add_test( NAME ktx2ktx2-test-many-in-one-out
    COMMAND ktx2ktx2 -o foo a.ktx b.ktx c.ktx
)
set_tests_properties(
    ktx2ktx2-test-many-in-one-out
PROPERTIES
    WILL_FAIL TRUE
    FAIL_REGULAR_EXPRESSION "^Can't use -o when there are multiple infiles."
)

add_test( NAME ktx2ktx2-test-many-in-one-out-exit-code
    COMMAND ktx2ktx2 -o foo a.ktx b.ktx c.ktx
)
set_tests_properties(
    ktx2ktx2-test-many-in-one-out-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

set( IMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testimages" )

function( cnvrtcmpktx test_name reference source args )
    set( workfile ktx2ktx2.${reference} )
    add_test( NAME ktx2ktx2-cnvrt-${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2ktx2> --test ${args} -o ${workfile} ${source} && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

function( cnvrtcmpktx_implied_out test_name base_source args )
    set( source ${base_source}.ktx )
    set( reference ${base_source}.ktx2 )
    set( worksource ktx2ktx2.ip.${source} )
    set( workfile ktx2ktx2.ip.${reference} )
    add_test( NAME ktx2ktx2-cnvrt-implied-out-${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "cp ${source} ${worksource} && $<TARGET_FILE:ktx2ktx2> --test ${args} ${worksource} && rm ${worksource} && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

cnvrtcmpktx( 2d-uncompressed orient-down-metadata-u.ktx2 orient-down-metadata.ktx "-f" )
cnvrtcmpktx( 2d-bc2 pattern_02_bc2.ktx2 pattern_02_bc2.ktx "-f" )
cnvrtcmpktx( 2d-array-astc texturearray_astc_8x8_unorm.ktx2 texturearray_astc_8x8_unorm.ktx "-f" )

cnvrtcmpktx_implied_out( 2d-bc2 pattern_02_bc2 "-f" )