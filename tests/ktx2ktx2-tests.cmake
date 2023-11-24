# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4 expandtab:

# Copyright 2022 Mark Callow
# SPDX-License-Identifier: Apache-2.0

add_test( NAME ktx2ktx2-test.help
    COMMAND ktx2ktx2 --help
)
set_tests_properties(
    ktx2ktx2-test.help
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktx2ktx2"
)

add_test( NAME ktx2ktx2-test.version
    COMMAND ktx2ktx2 --version
)
set_tests_properties(
    ktx2ktx2-test.version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^ktx2ktx2 v[0-9][0-9\\.]+"
)

# Why are there <test> and matching <test>-exit-code tests
#
# See comment under the same title in ./ktx2check-tests.cmake.

add_test( NAME ktx2ktx2-test.foobar
    COMMAND ktx2ktx2 --foobar
)
set_tests_properties(
    ktx2ktx2-test.foobar
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktx2ktx2"
)
add_test( NAME ktx2ktx2-test.foobar-exit-code
    COMMAND ktx2ktx2 --foobar
)
set_tests_properties(
    ktx2ktx2-test.foobar-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

add_test( NAME ktx2ktx2-test.many-in-one-out
    COMMAND ktx2ktx2 -o foo a.ktx b.ktx c.ktx
)
set_tests_properties(
    ktx2ktx2-test.many-in-one-out
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Can't use -o when there are multiple infiles."
)

add_test( NAME ktx2ktx2-test.many-in-one-out-exit-code
    COMMAND ktx2ktx2 -o foo a.ktx b.ktx c.ktx
)
set_tests_properties(
    ktx2ktx2-test.many-in-one-out-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

set( IMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testimages" )

add_test( NAME ktx2ktx2-test.ktx2-in
    COMMAND ktx2ktx2 -o foo CesiumLogoFlat.ktx2
    WORKING_DIRECTORY ${IMG_DIR}
)
set_tests_properties(
    ktx2ktx2-test.ktx2-in
PROPERTIES
    PASS_REGULAR_EXPRESSION ".* is not a KTX v1 file."
)
add_test( NAME ktx2ktx2-test.ktx2-in-exit-code
    COMMAND ktx2ktx2 -o foo CesiumLogoFlat.ktx2
    WORKING_DIRECTORY ${IMG_DIR}
)
set_tests_properties(
    ktx2ktx2-test.ktx2-in-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

function( cnvrtcmpktx test_name reference source args )
    set( workfile ktx2ktx2.${reference} )
    add_test( NAME ktx2ktx2-test.cnvrt-${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2ktx2> --test ${args} -o ${workfile} ${source} && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

function( cnvrtcmpktx_implied_out test_name base_source args )
    set( source ${base_source}.ktx )
    set( reference ${base_source}.ktx2 )
    set( worksource ktx2ktx2.ip.${source} )
    set( workfile ktx2ktx2.ip.${reference} )
    add_test( NAME ktx2ktx2-test.cnvrt-implied-out-${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "cp ${source} ${worksource} && $<TARGET_FILE:ktx2ktx2> --test ${args} ${worksource} && rm ${worksource} && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

cnvrtcmpktx( 2d-uncompressed orient-down-metadata-u.ktx2 orient-down-metadata.ktx "-f" )
cnvrtcmpktx( 2d-bc2 pattern_02_bc2.ktx2 pattern_02_bc2.ktx "-f" )
cnvrtcmpktx( 2d-array-astc texturearray_astc_8x8_unorm.ktx2 texturearray_astc_8x8_unorm.ktx "-f" )

cnvrtcmpktx_implied_out( 2d-bc2 pattern_02_bc2 "-f" )

cnvrtcmpktx( unicode-file-hu hűtő.ktx2 hűtő.ktx "-f" )
cnvrtcmpktx( unicode-file-jp テクスチャ.ktx2 テクスチャ.ktx "-f" )
cnvrtcmpktx( unicode-file-ar نَسِيج.ktx2 نَسِيج.ktx "-f" )
cnvrtcmpktx( unicode-file-zh 质地.ktx2 质地.ktx "-f" )
cnvrtcmpktx( unicode-file-ko 조직.ktx2 조직.ktx "-f" )

cnvrtcmpktx_implied_out( unicode-file-hu hűtő "-f" )
cnvrtcmpktx_implied_out( unicode-file-jp テクスチャ "-f" )
cnvrtcmpktx_implied_out( unicode-file-ar نَسِيج "-f" )
cnvrtcmpktx_implied_out( unicode-file-zh 质地 "-f" )
cnvrtcmpktx_implied_out( unicode-file-ko 조직 "-f" )
