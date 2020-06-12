add_executable( ktx2ktx2
    $<TARGET_OBJECTS:objUtil>
    ktx2ktx2.cpp
    stdafx.h
)
create_version_header( tools/ktx2ktx2 ktx2ktx2 )

target_include_directories(
    ktx2ktx2
PRIVATE
    .
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/other_include
)

target_link_libraries(
    ktx2ktx2
    ktx
)

install(TARGETS ktx2ktx2
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Tools
)
