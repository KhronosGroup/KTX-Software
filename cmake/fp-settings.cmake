# Copyright 2025 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0


function(get_fp_compile_options var)
    include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/compiler_query_genexs.cmake)

    # To improve output determinism, enable precise floating point operations globally
    # This code was based on external/astc-encoder/Source/cmake_core.cmake

    # For Visual Studio prior to 2022 (compiler < 19.30) /fp:strict
    # For Visual Studio 2022 (compiler >= 19.30) /fp:precise
    # For Visual Studio 2022 ClangCL has enabled contraction by default,
    # which is the same as standard clang, so behaves differently to
    # CL.exe. Use the -Xclang argument to access GNU-style switch to
    # control contraction and force disable.

    set(${var}
        $<$<AND:${is_msvccl},$<VERSION_LESS:$<CXX_COMPILER_VERSION>,19.30>>:/fp:strict>
        $<$<AND:${is_msvccl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,19.30>>:/fp:precise>
        $<${is_clangcl}:/fp:precise>
        $<$<AND:${is_clangcl},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,14.0.0>>:-Xclang$<SEMICOLON>-ffp-contract=off>
        $<$<AND:${is_clang},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,10.0.0>>:-ffp-model=precise>
        $<${is_gnu_fe}:-ffp-contract=off>
        # Hide noise from clang and clangcl 20 warning the 2nd fp option changes
        # one of the settings made the first.
        $<$<AND:${is_clang},$<VERSION_GREATER_EQUAL:$<CXX_COMPILER_VERSION>,20.0.0>>:-Wno-overriding-option>
        PARENT_SCOPE
    )
    #if(NOT TARGET debug_gnufe_ffpcontract)
    #    add_custom_target(debug_gnufe_ffpcontract
    #        COMMAND ${CMAKE_COMMAND} -E echo "ffp_contract = $<${is_gnu_fe}:-ffp-contract=off>"
    #    )
    #endif()
endfunction()
