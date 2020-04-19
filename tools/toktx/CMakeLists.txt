add_executable( toktx
    image.cc
    jpgimage.cc
    npbmimage.cc
    pngimage.cc
    toktx.cc
    $<TARGET_OBJECTS:objUtil>
    )

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)
get_property(basisu_include_dirs TARGET objBasisuWrite PROPERTY INCLUDE_DIRECTORIES)
get_property(objUtil_include_dirs TARGET objUtil PROPERTY INCLUDE_DIRECTORIES)


target_include_directories(
    toktx
    PRIVATE
    ${objKtxRead_include_dirs}
    ${basisu_include_dirs}
    ${objUtil_include_dirs}
    ${PROJECT_SOURCE_DIR}/other_include
    )

target_link_libraries(
    toktx
    ktx
    )

install(TARGETS toktx)
