# Copyright 2020 Andreas Atteneder
# SPDX-License-Identifier: Apache-2.0

function(compile_shader shader_target shader_name shader_src_path shader_path)

    set(vert_name "${shader_name}.vert")
    set(vert2spirv_in "${shader_src_path}/${vert_name}")
    set(vert2spirv_out "${CMAKE_CURRENT_BINARY_DIR}/${shader_path}/${vert_name}.spv")

    add_custom_command(OUTPUT
    ${vert2spirv_out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${shader_path}
    COMMAND glslc "-fshader-stage=vertex" -o "${vert2spirv_out}" "${vert2spirv_in}"
    DEPENDS ${vert2spirv_in}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Compiling ${vert_name}."
    VERBATIM
    )

    set(frag_name "${shader_name}.frag")
    set(frag2spirv_in "${shader_src_path}/${frag_name}")
    set(frag2spirv_out "${CMAKE_CURRENT_BINARY_DIR}/${shader_path}/${frag_name}.spv")

    add_custom_command(OUTPUT
    ${frag2spirv_out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${shader_path}
    COMMAND glslc "-fshader-stage=fragment" -o "${frag2spirv_out}" "${frag2spirv_in}"
    DEPENDS ${frag2spirv_in}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMENT "Compiling ${frag_name}."
    VERBATIM
    )

    add_custom_target(
        ${shader_target}
        DEPENDS
        ${vert2spirv_out}
        ${frag2spirv_out}
        SOURCES
        ${vert2spirv_in}
        ${frag2spirv_in}
    )

    set_target_properties(${shader_target} PROPERTIES EXCLUDE_FROM_ALL "FALSE")

    set(SHADER_SOURCES ${SHADER_SOURCES} ${frag2spirv_out} ${vert2spirv_out} PARENT_SCOPE)

endfunction(compile_shader)

function(compile_shader_list shader_target shader_src_path shader_path)

    foreach(shader ${ARGN})
        set(spirv_in  "${shader_src_path}/${shader}")
        set(spirv_out "${CMAKE_CURRENT_BINARY_DIR}/${shader_path}/${shader}.spv")

        add_custom_command(OUTPUT
        ${spirv_out}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/${shader_path}
        COMMAND glslc -o "${spirv_out}" "${spirv_in}"
        DEPENDS ${spirv_in}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        COMMENT "Compiling ${shader}."
        VERBATIM
        )

        list(APPEND inputs ${spirv_in})
        list(APPEND outputs ${spirv_out})
     endforeach()

    add_custom_target(
        ${shader_target}
        DEPENDS ${outputs}
        SOURCES ${inputs}
    )

    set_target_properties(${shader_target} PROPERTIES EXCLUDE_FROM_ALL "FALSE")

    set(SHADER_SOURCES ${SHADER_SOURCES} ${outputs} PARENT_SCOPE)

endfunction()
