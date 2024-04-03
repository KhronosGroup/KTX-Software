# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

find_package(Doxygen REQUIRED)

set(docdest "${CMAKE_CURRENT_BINARY_DIR}/docs")

# The torturous Doxygen output settings are a hack
# to workaround a doxygen limitation where it will
# only create the trailing directory of the path
# given in, e.g DOXYGEN_HTML_OUTPUT while enabling
# us to put each sub-document in its own directory of
# the html output.

# Global
set( DOXYGEN_PROJECT_LOGO icons/ktx_logo_200.png )
set( DOXYGEN_OUTPUT_DIRECTORY ${docdest}/html)
set( DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES )
set( DOXYGEN_EXTRACT_LOCAL_CLASSES NO )
set( DOXYGEN_HIDE_UNDOC_MEMBERS YES )
set( DOXYGEN_CASE_SENSE_NAMES YES )
set( DOXYGEN_SHOW_USED_FILES NO )
set( DOXYGEN_VERBATIM_HEADERS NO )
set( DOXYGEN_CLANG_ASSISTED_PARSING NO )
set( DOXYGEN_ALPHABETICAL_INDEX NO )
set( DOXYGEN_DISABLE_INDEX YES )
set( DOXYGEN_DISABLE_INDEX NO )
set( DOXYGEN_GENERATE_TREEVIEW YES )
set( DOXYGEN_GENERATE_LATEX NO )
set( DOXYGEN_GENERATE_HTML YES )
set( DOXYGEN_GENERATE_MAN YES )
set( DOXYGEN_MAN_OUTPUT ../man )
# This is to get timestamps with older versions of doxygen.
# older
set( DOXYGEN_HTML_TIMESTAMP YES )
set( DOXYGEN_TIMESTAMP YES )

function( add_sources target sources )
    # Make ${sources} show up in IDE/project
    get_target_property( doc_sources ${target} SOURCES )
    if( NOT doc_sources )
        set( doc_sources "" ) # Clear doc_sources-NOTFOUND value.
    endif()
    set_target_properties(
        ${target}
    PROPERTIES
        SOURCES "${doc_sources};${sources}"
    )
endfunction()

function( add_docs_cmake target )
    # Make `docs.cmake` show up in IDE/project
    add_sources( ${target} "cmake/docs.cmake" )
endfunction()

function( add_docs_cmake_plus target sources )
    # Make `docs.cmake` plus ${sources} show up in IDE/project
    add_sources( ${target} "cmake/docs.cmake;${sources}" )
endfunction()

# Note very well
#
# These projects and accompanying DOXYGEN_LAYOUT_FILES are carefully crafted
# to provide the illusion of a consistent GUI across all the projects.
# Likely this will be fragile in the face of Doxygen changes.

# ktx.doc
function( CreateDocLibKTX )
    set( DOXYGEN_PROJECT_NAME "libktx Reference" )
    set( DOXYGEN_ALIASES error=\"\\par Errors\\n\" )
    set( DOXYGEN_LAYOUT_FILE pkgdoc/libktxDoxyLayout.xml )
    set( DOXYGEN_TYPEDEF_HIDES_STRUCT NO )
    set( DOXYGEN_EXCLUDE lib/uthash.h )
    set( DOXYGEN_EXCLUDE_PATTERNS ktxint.h )
    set( DOXYGEN_EXAMPLE_PATH examples lib )
    # This does not hide the scope (class) names in the Modules list
    # in the ToC. See https://github.com/doxygen/doxygen/issues/9921.
    set( DOXYGEN_HIDE_SCOPE_NAMES YES )
    set( DOXYGEN_HTML_OUTPUT libktx )
    # Order is important here. '_' suffixed prefices must come first
    # otherwise the non-suffixed is stripped first leaving just '_'.
    set( DOXYGEN_IGNORE_PREFIX KTX_;ktx_;KTX;ktx )
    set( DOXYGEN_MAN_LINKS YES )
    set( DOXYGEN_MACRO_EXPANSION YES )
    set( DOXYGEN_EXPAND_ONLY_PREDEF YES )

    set( DOXYGEN_PREDEFINED
    "KTXTEXTURECLASSDEFN=class_id classId\; \\
        struct ktxTexture_vtbl* vtbl\;             \\
        struct ktxTexture_vvtbl* vvtbl\;           \\
        struct ktxTexture_protected* _protected\;  \\
        ktx_bool_t   isArray\;                     \\
        ktx_bool_t   isCubemap\;                   \\
        ktx_bool_t   isCompressed\;                \\
        ktx_bool_t   generateMipmaps\;             \\
        ktx_uint32_t baseWidth\;                   \\
        ktx_uint32_t baseHeight\;                  \\
        ktx_uint32_t baseDepth\;                   \\
        ktx_uint32_t numDimensions\;               \\
        ktx_uint32_t numLevels\;                   \\
        ktx_uint32_t numLayers\;                   \\
        ktx_uint32_t numFaces\;                    \\
        struct {                                   \\
            ktxOrientationX x\;                    \\
            ktxOrientationY y\;                    \\
            ktxOrientationZ z\;                    \\
        } orientation\;                            \\
        ktxHashList  kvDataHead\;                  \\
        ktx_uint32_t kvDataLen\;                   \\
        ktx_uint8_t* kvData\;                      \\
        ktx_size_t dataSize\;                      \\
        ktx_uint8_t* pData\;"
    )
    #set( DOXYGEN_GENERATE_TAGFILE ${docdest}/libktx.tag )

    doxygen_add_docs(
        libktx.doc
        lib/libktx_mainpage.md
        include
        lib/astc_encode.cpp
        lib/basis_encode.cpp
        lib/basis_transcode.cpp
        lib/miniz_wrapper.cpp
        lib/strings.c
        lib/glloader.c
        lib/hashlist.c
        lib/filestream.c
        lib/memstream.c
        lib/texture.c
        lib/texture1.c
        lib/texture2.c
        lib/vkloader.c
        lib/writer1.c
        lib/writer2.c
    )
    add_docs_cmake_plus( libktx.doc pkgdoc/libktxDoxyLayout.xml )
endfunction()

# ktxtools.doc
function( CreateDocTools )
    set( DOXYGEN_PROJECT_NAME "KTX Tools Reference" )
    set( DOXYGEN_FULL_PATH_NAMES NO )
    set( DOXYGEN_ALIASES author=\"\\section AUTHOR\\n\" )
    set( DOXYGEN_LAYOUT_FILE pkgdoc/toolsDoxyLayout.xml )
    set( DOXYGEN_SHOW_FILES NO )
    set( DOXYGEN_FILE_PATTERNS *.cpp )
    set( DOXYGEN_RECURSIVE YES )
    set( DOXYGEN_EXAMPLE_PATH utils tools )
    set( DOXYGEN_HTML_OUTPUT ktxtools )
    set( DOXYGEN_MAN_EXTENSION .1 )
    #set( DOXYGEN_GENERATE_TAGFILE ${docdest}/ktxtools.tag )
    set( DOXYGEN_TAGFILES ${docdest}/ktxpkg.tag=.. )

    doxygen_add_docs(
        tools.doc
        tools/ktx/ktx_main.cpp
        tools/ktx/command_compare.cpp
        tools/ktx/command_create.cpp
        tools/ktx/command_encode.cpp
        tools/ktx/command_extract.cpp
        tools/ktx/command_help.cpp
        tools/ktx/command_info.cpp
        tools/ktx/command_transcode.cpp
        tools/ktx/command_validate.cpp
        tools/ktx2check/ktx2check.cpp
        tools/ktx2ktx2/ktx2ktx2.cpp
        tools/ktxinfo/ktxinfo.cpp
        tools/ktxsc/ktxsc.cpp
        tools/ktxtools_mainpage.md
        tools/toktx/toktx.cc
    )
    add_docs_cmake_plus( tools.doc pkgdoc/toolsDoxyLayout.xml )
endfunction()

# ktxjswrappers.doc
function( CreateDocJSWrappers )
    set( DOXYGEN_PROJECT_NAME "KTX Javascript Wrappers Reference" )
    set( DOXYGEN_FULL_PATH_NAMES NO )
    set( DOXYGEN_ALIASES author=\"\\section AUTHOR\\n\" )
    set( DOXYGEN_LAYOUT_FILE pkgdoc/jswrappersDoxyLayout.xml )
    set( DOXYGEN_SHOW_FILES NO )
    set( DOXYGEN_HTML_OUTPUT ktxjswrappers )
    #set( DOXYGEN_GENERATE_TAGFILE ${docdest}/ktxjswrappers.tag )
    set( DOXYGEN_TAGFILES ${docdest}/ktxpkg.tag=.. )

    doxygen_add_docs(
        jswrappers.doc
        interface/js_binding
    )
    add_docs_cmake_plus( jswrappers.doc pkgdoc/jswrappersDoxyLayout.xml )
endfunction()

# pyktxwrappers.doc
function( CreateDocPyktxWrappers )
    add_custom_command(
        TARGET libktx.doc
        POST_BUILD
        COMMAND
            ${CMAKE_COMMAND} -E copy_directory ${KTX_BUILD_DIR}/interface/python_binding/docs/html/pyktx/html ${KTX_BUILD_DIR}/docs/html/pyktx
    )
    add_dependencies( libktx.doc pyktx )
endfunction()

# ktxpkg.doc
function( CreateDocKTX )
    set( DOXYGEN_PROJECT_NAME "Khronos Texture Software" )
    set( DOXYGEN_ALIASES pversion=\"\\par Package Version\\n\" )
    set( DOXYGEN_LAYOUT_FILE pkgdoc/packageDoxyLayout.xml )
    set( DOXYGEN_EXCLUDE lib/uthash.h )
    set( DOXYGEN_EXCLUDE_PATTERNS ktxint.h )
    set( DOXYGEN_EXAMPLE_PATH lib )
    set( DOXYGEN_GENERATE_TAGFILE ${docdest}/ktxpkg.tag )
    set( DOXYGEN_HTML_HEADER pkgdoc/header.html )
    set( DOXYGEN_HTML_OUTPUT . )
    set( DOXYGEN_MAN_LINKS YES )
    #set( DOXYGEN_TAGFILES ${docdest}/libktx.tag=libktx ${docdest}/ktxtools.tag=ktxtools )

    doxygen_add_docs(
        ktxpkg.doc
        pkgdoc/pages.md
        LICENSE.md
        #RELEASE_NOTES.md
    )
    add_docs_cmake_plus( ktxpkg.doc pkgdoc/packageDoxyLayout.xml )
endfunction()

CreateDocLibKTX()
CreateDocTools()
CreateDocJSWrappers()
if (KTX_FEATURE_PY)
    CreateDocPyktxWrappers()
endif()
CreateDocKTX()

add_dependencies( libktx.doc ktxpkg.doc ktx_version )
add_dependencies( jswrappers.doc ktxpkg.doc )
add_dependencies( tools.doc ktxpkg.doc )

# I want to add a dependency on the "package" built-in target.
# Unfortunately CMake does not support adding dependencies to
# built-in targets. See https://gitlab.kitware.com/cmake/cmake/-/issues/8438.
#
# There also seems to be no way to add a dependency for the install commands
# below. Presumably "install(TARGETS ...)" adds a dependency on each target
# but the rest of that command is simply not appropriate for this case.
add_custom_target( all.doc ALL
    DEPENDS tools.doc libktx.doc jswrappers.doc
)

install(
    DIRECTORY ${docdest}/html
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    COMPONENT tools
)

install(
    DIRECTORY ${docdest}/man/man1
    ## Omit those, since at the moment they contain a lot of undesired files generated by Doxygen
    # ${docdest}/man/man3
    # ${docdest}/man/ktx/man3
    DESTINATION "${CMAKE_INSTALL_MANDIR}"
    COMPONENT tools
)
