add_library(obj_ktx_cbind OBJECT
    src/basisu_c_binding.cpp
    src/ktx_c_binding.cpp
    )

get_property(objKtxRead_include_dirs TARGET objKtxRead PROPERTY INCLUDE_DIRECTORIES)
get_property(basisu_include_dirs TARGET objBasisu PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(
    obj_ktx_cbind
    PUBLIC
    inc
    PRIVATE
    ${objKtxRead_include_dirs}
    ${basisu_include_dirs}
    )
