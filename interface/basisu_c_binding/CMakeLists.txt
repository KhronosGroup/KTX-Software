add_library(obj_basisu_cbind OBJECT
    inc/basisu_c_binding.h
    src/basisu_c_binding.cpp
)

target_include_directories(
    obj_basisu_cbind
PUBLIC
    inc
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/utils
)
