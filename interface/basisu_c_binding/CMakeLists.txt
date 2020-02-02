add_library(obj_basisu_cbind OBJECT
    src/basisu_c_binding.cpp
    )

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)
get_property(basisu_include_dirs TARGET objBasisu PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(
    obj_basisu_cbind
    PUBLIC
    inc
    PRIVATE
    ${objKtxRead_include_dirs}
    ${basisu_include_dirs}
    ${PROJECT_SOURCE_DIR}/utils
    )
