add_library(obj_basisu_cbind OBJECT
    src/basisu_c_binding.cpp
    )

get_property(ktx_include_dirs TARGET ktx PROPERTY INCLUDE_DIRECTORIES)

target_include_directories(
    obj_basisu_cbind
    PUBLIC
    inc
    PRIVATE
    ${ktx_include_dirs}
    ${PROJECT_SOURCE_DIR}/utils
    )
