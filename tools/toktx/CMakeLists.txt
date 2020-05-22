add_executable( toktx
    $<TARGET_OBJECTS:objUtil>
    image.cc
    jpgimage.cc
    npbmimage.cc
    pngimage.cc
    stdafx.h
    targetver.h
    toktx.cc
    )

target_include_directories(
    toktx
    PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:objUtil,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/lib
    ${PROJECT_SOURCE_DIR}/lib/basisu
    ${PROJECT_SOURCE_DIR}/other_include
    )

target_link_libraries(
    toktx
    ktx
    )

target_compile_definitions(
    toktx
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
)

install(TARGETS toktx)
