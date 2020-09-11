# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

add_test( NAME ktx2check-test-help
    COMMAND ktx2check --help
)

add_test( NAME ktx2check-test-version
    COMMAND ktx2check --version
)
set_tests_properties(
    ktx2check-test-version
PROPERTIES
    PASS_REGULAR_EXPRESSION "^ktx2check v[0-9\\.]+"
)

add_test( NAME ktx2check-test-foobar
    COMMAND ktx2check --foobar
)
set_tests_properties(
    ktx2check-test-foobar
PROPERTIES
    FAIL_REGULAR_EXPRESSION "^Usage: ktx2check"
)

add_test( NAME ktx2check-test-all
    # Invoke via sh workaround, since CMake puts asterisk in quotes otherwise ( "*.ktx2" )
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2check> *.ktx2"
    COMMAND_EXPAND_LISTS
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-all-quiet
    # Invoke via sh workaround, since CMake puts asterisk in quotes otherwise ( "*.ktx2" )
    COMMAND ${BASH_EXECUTABLE} -c "$<TARGET_FILE:ktx2check> --quiet *.ktx2"
    COMMAND_EXPAND_LISTS
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-invalid-face-count
    COMMAND ktx2check invalid_face_count.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-invalid-face-count-quiet
    COMMAND ktx2check --quiet invalid_face_count.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-incorrect-mip-layout-and-padding
    COMMAND ktx2check incorrect_mip_layout_and_padding.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

add_test( NAME ktx2check-test-incorrect-mip-layout-and-padding-quiet
    COMMAND ktx2check --quiet incorrect_mip_layout_and_padding.ktx2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testimages
)

set_tests_properties(
    ktx2check-test-help
PROPERTIES
    PASS_REGULAR_EXPRESSION "^Usage: ktx2check"
)

set_tests_properties(
    ktx2check-test-all-quiet
PROPERTIES
    PASS_REGULAR_EXPRESSION "^$"
)

set_tests_properties(
    ktx2check-test-invalid-face-count-quiet
    ktx2check-test-incorrect-mip-layout-and-padding-quiet
PROPERTIES
    FAIL_REGULAR_EXPRESSION "^$"
)

set_tests_properties(
    ktx2check-test-foobar
    ktx2check-test-invalid-face-count
    ktx2check-test-invalid-face-count-quiet
    ktx2check-test-incorrect-mip-layout-and-padding
    ktx2check-test-incorrect-mip-layout-and-padding-quiet
PROPERTIES
    WILL_FAIL TRUE
)
