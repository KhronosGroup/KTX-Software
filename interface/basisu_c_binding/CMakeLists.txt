# Copyright 2017-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

set(LIB_TYPE STATIC)
# Parent scope variables set in this context are not seen in it and, if there
# is a way to explicitly reference one, I've not found it. So 2-step.
set(BASISU_CBIND_LIBTYPE ${LIB_TYPE} PARENT_SCOPE)

add_library(obj_basisu_cbind ${LIB_TYPE}
    inc/basisu_c_binding.h
    src/basisu_c_binding.cpp
)

# Using cxx_std_17 because VS 2022 raises a set but unused variable warning
# and pre-c++-17 there is no 'unused' attribute. equivalent.
target_compile_features(obj_basisu_cbind PUBLIC c_std_99 cxx_std_17)
# Need this to make VS correctly define __cplusplus to the std in use.
target_compile_options(
    obj_basisu_cbind
PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
)

target_include_directories(
    obj_basisu_cbind
PUBLIC
    inc
PRIVATE
    $<TARGET_PROPERTY:ktx,INCLUDE_DIRECTORIES>
    ${PROJECT_SOURCE_DIR}/utils
)

target_compile_definitions(
    obj_basisu_cbind
PUBLIC
    KTX_BASISU_C_BINDINGS
PRIVATE
    $<TARGET_PROPERTY:ktx,INTERFACE_COMPILE_DEFINITIONS>
    # We're only using this to read .basis files.
    BASISD_SUPPORT_KTX2_ZSTD=0
    BASISD_SUPPORT_KTX2=0
)

if(WIN32)
    target_compile_definitions(
        obj_basisu_cbind
    PRIVATE 
        $<IF:$<STREQUAL:"${LIB_TYPE}","STATIC">,KHRONOS_STATIC,KTX_BASISU_API=__declspec\(dllexport\)>
    )
endif()
