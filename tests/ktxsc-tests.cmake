# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4 expandtab:

# Copyright 2022 Mark Callow
# SPDX-License-Identifier: Apache-2.0

# toktx share a common scapp class with ktxsc so the toktx tests suffice
# for testing actual compression.

add_test( NAME ktxsc-test-help
    COMMAND ktxsc --help
)
set_tests_properties(
    ktxsc-test-help
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktxsc"
)

add_test( NAME ktxsc-test-version
    COMMAND ktxsc --version
)
set_tests_properties(
    ktxsc-test-version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^ktxsc v[0-9][0-9\\.]+"
)

# The near duplication of this and other tests below is due to a "limitation"
# (i.e. a bug) in ctest which checks for neither a zero error code when a
# PASS_REGULAR_EXPRESSION is specified nor a non-zero error code when a
# FAIL_REGULAR_EXPRESSION is specified but only for matches to the REs.
add_test( NAME ktxsc-test-foobar
    COMMAND ktxsc --foobar
)
set_tests_properties(
    ktxsc-test-foobar
PROPERTIES
    WILL_FAIL TRUE
    FAIL_REGULAR_EXPRESSION "^Usage: ktxsc"
)
add_test( NAME ktxsc-test-foobar-exit-code
    COMMAND ktxsc --foobar
)
set_tests_properties(
    ktxsc-test-foobar-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

add_test( NAME ktxsc-test-many-in-one-out
    COMMAND ktxsc -o foo a.ktx2 b.ktx2 c.ktx2
)
set_tests_properties(
    ktxsc-test-many-in-one-out
PROPERTIES
    WILL_FAIL TRUE
    FAIL_REGULAR_EXPRESSION "^Can't use -o when there are multiple infiles."
)

add_test( NAME ktxsc-test-many-in-one-out-exit-code
    COMMAND ktxsc -o foo a.ktx2 b.ktx2 c.ktx2
)
set_tests_properties(
    ktxsc-test-many-in-one-out-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

set( IMG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/testimages" )

function( gencmpktx test_name reference source args inplace )
    if (NOT inplace)
        set( workfile ktxsc.${reference} )
        add_test( NAME ktxsc-cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktxsc> --test ${args} -o ${workfile} ${source} && diff ${reference} ${workfile} && rm ${workfile}"
            WORKING_DIRECTORY ${IMG_DIR}
        )
    elseif(${inplace} STREQUAL "cur-dir")
        set( workfile ktxsc.ip1.${reference} )
        add_test( NAME ktxsc-cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "cp ${source} ${workfile} && $<TARGET_FILE:ktxsc> --test ${args} ${workfile} && diff ${reference} ${workfile} && rm ${workfile}"
            WORKING_DIRECTORY ${IMG_DIR}
        )
    elseif(${inplace} STREQUAL "different-dir")
        set( workfile ktxsc.ip2.${reference} )
        add_test( NAME ktxsc-cmp-${test_name}
            COMMAND ${BASH_EXECUTABLE} -c "cp ${source} ${workfile} && pushd ../.. && $<TARGET_FILE:ktxsc> --test ${args} ${IMG_DIR}/${workfile} && popd && diff ${reference} ${workfile} && rm ${workfile}"
            WORKING_DIRECTORY ${IMG_DIR}
        )
    endif()
endfunction()

gencmpktx( compress-explicit-output skybox_zstd.ktx2 skybox.ktx2 "--zcmp 5" "" "" )
gencmpktx( compress-in-place-cur-dir skybox_zstd.ktx2 skybox.ktx2 "--zcmp 5" "cur-dir" )
gencmpktx( compress-in-place-different-dir skybox_zstd.ktx2 skybox.ktx2 "--zcmp 5" "different-dir" )
