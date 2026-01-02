# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4 expandtab:

# Copyright 2022 Mark Callow
# SPDX-License-Identifier: Apache-2.0

# toktx shares a common scapp class with ktxsc so the toktx tests suffice
# for testing actual compression.

add_test( NAME ktxsc-test.help
    COMMAND ktxsc --help
)
set_tests_properties(
    ktxsc-test.help
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktxsc"
)

add_test( NAME ktxsc-test.version
    COMMAND ktxsc --version
)
set_tests_properties(
    ktxsc-test.version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^ktxsc v[0-9][0-9\\.]+"
)

# Why are there <test> and matching <test>-exit-code tests
#
# See comment under the same title in ./ktx2check-tests.cmake.

add_test( NAME ktxsc-test.foobar
    COMMAND ktxsc --foobar
)
set_tests_properties(
    ktxsc-test.foobar
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktxsc"
)
add_test( NAME ktxsc-test.foobar-exit-code
    COMMAND ktxsc --foobar
)
set_tests_properties(
    ktxsc-test.foobar-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

add_test( NAME ktxsc-test.many-in-one-out
    COMMAND ktxsc -o foo a.ktx2 b.ktx2 c.ktx2
)
set_tests_properties(
    ktxsc-test.many-in-one-out
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Can't use -o when there are multiple infiles."
)

add_test( NAME ktxsc-test.many-in-one-out-exit-code
    COMMAND ktxsc -o foo a.ktx2 b.ktx2 c.ktx2
)
set_tests_properties(
    ktxsc-test.many-in-one-out-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

set( IMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testimages" )

add_test( NAME ktxsc-test.ktx1-in
    COMMAND ktxsc --zcmp 5 -o foo orient-up-metadata.ktx
    WORKING_DIRECTORY "${IMG_DIR}"
)
set_tests_properties(
    ktxsc-test.ktx1-in
PROPERTIES
    PASS_REGULAR_EXPRESSION ".* is not a KTX v2 file."
)
add_test( NAME ktxsc-test.ktx1-in-exit-code
    COMMAND ktxsc --zcmp 5 -o foo orient-up-metadata.ktx
    WORKING_DIRECTORY ${IMG_DIR}
)
set_tests_properties(
    ktxsc-test.ktx1-in-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

function( sccmpktx test_name reference source args )
    set( workfile ktxsc.${reference} )
    add_test( NAME ktxsc-test.${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktxsc> --test ${args} -o ${workfile} ${source} && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

function( sccmpktxinplacecurdir test_name reference source args )
    set( workfile ktxsc.ip1.${reference} )
    add_test( NAME ktxsc-test.inplace-curdir-${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "cp ${source} ${workfile} && $<TARGET_FILE:ktxsc> --test ${args} ${workfile} && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

function( sccmpktxinplacediffdir test_name reference source args )
    set( workfile ktxsc.ip2.${reference} )
    add_test( NAME ktxsc-test.inplace-diffdir-${test_name}
        COMMAND ${BASH_EXECUTABLE} -c "cp ${source} ${workfile} && pushd ../.. && $<TARGET_FILE:ktxsc> --test ${args} ${IMG_DIR}/${workfile} && popd && diff ${reference} ${workfile} && rm ${workfile}"
        WORKING_DIRECTORY ${IMG_DIR}
    )
endfunction()

sccmpktx( zcmp-cubemap skybox_zstd.ktx2 skybox.ktx2 "--zcmp 5" )
sccmpktxinplacecurdir( zcmp-cubemap skybox_zstd.ktx2 skybox.ktx2 "--zcmp 5" )
sccmpktxinplacediffdir( zcmp_cubemap skybox_zstd.ktx2 skybox.ktx2 "--zcmp 5" )
sccmpktxinplacecurdir( unicode-file-hu hűtő_zstd.ktx2 hűtő.ktx2 "--zcmp 5" )
sccmpktxinplacecurdir( unicode-file-jp テクスチャ_zstd.ktx2 テクスチャ.ktx2 "--zcmp 5" )
sccmpktxinplacecurdir( unicode-file-ar نَسِيج_zstd.ktx2 نَسِيج.ktx2 "--zcmp 5" )
sccmpktxinplacecurdir( unicode-file-zh 质地_zstd.ktx2 质地.ktx2 "--zcmp 5" )
sccmpktxinplacecurdir( unicode-file-ko 조직_zstd.ktx2 조직.ktx2 "--zcmp 5" )
sccmpktxinplacediffdir( unicode-file-hu hűtő_zstd.ktx2 hűtő.ktx2 "--zcmp 5" )
sccmpktxinplacediffdir( unicode-file-jp テクスチャ_zstd.ktx2 テクスチャ.ktx2 "--zcmp 5" )
sccmpktxinplacediffdir( unicode-file-ar نَسِيج_zstd.ktx2 نَسِيج.ktx2 "--zcmp 5" )
sccmpktxinplacediffdir( unicode-file-zh 质地_zstd.ktx2 质地.ktx2 "--zcmp 5" )
sccmpktxinplacediffdir( unicode-file-ko 조직_zstd.ktx2 조직.ktx2 "--zcmp 5" )
