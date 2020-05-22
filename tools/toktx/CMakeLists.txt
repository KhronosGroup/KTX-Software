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

get_property(ktx_include_dirs TARGET ktx PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    toktx
    PRIVATE
    ${ktx_include_dirs}
    ${objUtil_include_dirs}
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
