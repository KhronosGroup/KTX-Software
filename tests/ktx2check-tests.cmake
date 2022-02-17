# -*- tab-width: 4; -*-
# vi: set sw=2 ts=4 expandtab:

# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

add_test( NAME ktx2check-test-help
    COMMAND ktx2check --help
)
set_tests_properties(
    ktx2check-test-help
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktx2check"
)

add_test( NAME ktx2check-test-version
    COMMAND ktx2check --version
)
set_tests_properties(
    ktx2check-test-version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^ktx2check v[0-9][0-9\\.]+"
)

# The near duplication of this and other tests below is due to a "limitation"
# (i.e. a bug) in ctest which checks for neither a zero error code when a
# PASS_REGULAR_EXPRESSION is specified nor a non-zero error code when a
# FAIL_REGULAR_EXPRESSION is specified but only for matches to the REs.
add_test( NAME ktx2check-test-foobar
    COMMAND ktx2check --foobar
)
set_tests_properties(
    ktx2check-test-foobar
PROPERTIES
    WILL_FAIL TRUE
    FAIL_REGULAR_EXPRESSION "^Usage: ktx2check"
)
add_test( NAME ktx2check-test-foobar-exit-code
    COMMAND ktx2check --foobar
)
set_tests_properties(
    ktx2check-test-foobar-exit-code
PROPERTIES
    WILL_FAIL TRUE
)

add_test( NAME ktx2check-test-all
    # Invoke via sh workaround, since CMake puts asterisk in quotes
    # otherwise ( "*.ktx2" )
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2check> *.ktx2"
    COMMAND_EXPAND_LISTS
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-all-quiet
    # Invoke via sh workaround, since CMake puts asterisk in quotes
    # otherwise ( "*.ktx2" )
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2check> --quiet *.ktx2"
    COMMAND_EXPAND_LISTS
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)
set_tests_properties(
    ktx2check-test-all-quiet
PROPERTIES
    PASS_REGULAR_EXPRESSION "^$"
)

add_test( NAME ktx2check-test-stdin-read
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2check> < color_grid_uastc_zstd.ktx2
"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-pipe-read
    COMMAND ${BASH_EXECUTABLE} -c "cat color_grid_uastc_zstd.ktx2 | $<TARGET_FILE:ktx2check>"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-invalid-face-count
    COMMAND ktx2check invalid_face_count.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)

add_test( NAME ktx2check-test-invalid-face-count-quiet
    COMMAND ktx2check --quiet invalid_face_count.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)
add_test( NAME ktx2check-test-invalid-face-count-quiet-exit-code
    COMMAND ktx2check --quiet invalid_face_count.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)

add_test( NAME ktx2check-test-incorrect-mip-layout-and-padding
    COMMAND ktx2check incorrect_mip_layout_and_padding.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)

add_test( NAME ktx2check-test-incorrect-mip-layout-and-padding-quiet
    COMMAND ktx2check --quiet incorrect_mip_layout_and_padding.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)
add_test( NAME ktx2check-test-incorrect-mip-layout-and-padding-quiet-exit-code
    COMMAND ktx2check incorrect_mip_layout_and_padding.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)

add_test( NAME ktx2check-test-bad-typesize
    COMMAND ktx2check bad_typesize.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)
set_tests_properties(
    ktx2check-test-bad-typesize
PROPERTIES
    FAIL_REGULAR_EXPRESSION "ERROR: typeSize, 1, does not match data described by the DFD."
)
add_test( NAME ktx2check-test-bad-typesize-exit-code
    COMMAND ktx2check bad_typesize.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)

add_test( NAME ktx2check-test-no-nul-on-value
    COMMAND ktx2check no_nul_on_kvd_val.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)
set_tests_properties(
    ktx2check-test-no-nul-on-value
PROPERTIES
    PASS_REGULAR_EXPRESSION "WARNING: KTXswizzle value missing encouraged NUL termination."
)
add_test( NAME ktx2check-test-no-nul-on-value-exit-code
    COMMAND ktx2check no_nul_on_kvd_val.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)
add_test( NAME ktx2check-test-no-nul-on-value-warn-as-error-exit-code
    COMMAND ktx2check -w no_nul_on_kvd_val.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/badktx2
)

set_tests_properties(
    ktx2check-test-invalid-face-count-quiet
    ktx2check-test-incorrect-mip-layout-and-padding-quiet
PROPERTIES
    FAIL_REGULAR_EXPRESSION "^$"
)

set_tests_properties(
    ktx2check-test-bad-typesize
    ktx2check-test-bad-typesize-exit-code
    ktx2check-test-invalid-face-count
    ktx2check-test-invalid-face-count-quiet
    ktx2check-test-invalid-face-count-quiet-exit-code
    ktx2check-test-incorrect-mip-layout-and-padding
    ktx2check-test-incorrect-mip-layout-and-padding-quiet
    ktx2check-test-incorrect-mip-layout-and-padding-quiet-exit-code
    ktx2check-test-no-nul-on-value-warn-as-error-exit-code
PROPERTIES
    WILL_FAIL TRUE
)
