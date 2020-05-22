add_executable( ktx2check
    $<TARGET_OBJECTS:objUtil>
    ktx2check.cpp
    stdafx.h
    )

target_include_directories(
    ktx2check
    PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
    )

target_link_libraries(
    ktx2check
    ktx
    )

install(TARGETS ktx2check)
