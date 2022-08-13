# Copyright 2015-2020 The Khronos Group Inc.
# SPDX-License-Identifier: Apache-2.0

find_package(Doxygen REQUIRED)

# Global
set( DOXYGEN_PROJECT_LOGO icons/ktx_logo_200.png )
set( DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
set( DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES )
set( DOXYGEN_EXTRACT_LOCAL_CLASSES NO )
set( DOXYGEN_HIDE_UNDOC_MEMBERS YES )
set( DOXYGEN_CASE_SENSE_NAMES YES )
set( DOXYGEN_SHOW_USED_FILES NO )
set( DOXYGEN_VERBATIM_HEADERS NO )
set( DOXYGEN_CLANG_ASSISTED_PARSING NO )
set( DOXYGEN_ALPHABETICAL_INDEX NO )
set( DOXYGEN_HTML_TIMESTAMP YES )
set( DOXYGEN_DISABLE_INDEX YES )
set( DOXYGEN_GENERATE_TREEVIEW YES )
set( DOXYGEN_GENERATE_LATEX NO )
# set( DOXYGEN_PAPER_TYPE a4wide ) # note: invalid value!
set( DOXYGEN_GENERATE_MAN YES )


function( add_docs_cmake target )
    # Make `docs.cmake` show up in IDE/project
    get_target_property( DOC_SOURCES ${target} SOURCES )
    set_target_properties(
        ${target}
    PROPERTIES
        SOURCES "${DOC_SOURCES};cmake/docs.cmake"
    )
endfunction()

# ktx.doc
function( CreateDocLibKTX )
    set( DOXYGEN_PROJECT_NAME "libktx - The KTX Library" )
    set( DOXYGEN_ALIASES error=\"\\par Errors\\n\" )
    set( DOXYGEN_TYPEDEF_HIDES_STRUCT YES )
    set( DOXYGEN_EXCLUDE lib/uthash.h )
    set( DOXYGEN_EXCLUDE_PATTERNS ktxint.h )
    set( DOXYGEN_EXAMPLE_PATH examples lib )
    set( DOXYGEN_HTML_OUTPUT html/libktx )
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
        struct {                                  \\
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
    set( DOXYGEN_GENERATE_TAGFILE ${CMAKE_BINARY_DIR}/docs/libktx.tag )

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/docs )
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/docs/html )

    doxygen_add_docs(
        libktx.doc
        lib/mainpage.md
        LICENSE.md
        include
        lib/astc_encode.cpp
        lib/basis_encode.cpp
        lib/basis_transcode.cpp
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
    add_docs_cmake(libktx.doc)
endfunction()

# ktxtools.doc
function( CreateDocKTXTools )
    set( DOXYGEN_PROJECT_NAME "Khronos Texture Tools" )
    set( DOXYGEN_FULL_PATH_NAMES NO )
    set( DOXYGEN_ALIASES author=\"\\section AUTHOR\\n\" )
    set( DOXYGEN_SHOW_FILES NO )
    set( DOXYGEN_FILE_PATTERNS *.cpp )
    set( DOXYGEN_RECURSIVE YES )
    set( DOXYGEN_EXAMPLE_PATH utils )
    set( DOXYGEN_HTML_OUTPUT html/ktxtools )
    set( DOXYGEN_MAN_EXTENSION .1 )
    set( DOXYGEN_GENERATE_TAGFILE ${CMAKE_BINARY_DIR}/docs/ktxtools.tag )

    doxygen_add_docs(
        ktxtools.doc
        tools/ktxinfo/ktxinfo.cpp
        tools/ktx2check/ktx2check.cpp
        tools/ktx2ktx2/ktx2ktx2.cpp
        tools/ktxsc/ktxsc.cpp
        tools/toktx/toktx.cc
    )
    add_docs_cmake(ktxtools.doc)
endfunction()


# ktxpkg.doc
function( CreateDocKTX )
    set( DOXYGEN_PROJECT_NAME "Khronos Texture Software" )
    set( DOXYGEN_ALIASES pversion=\"\\par Package Version\\n\" )
    set( DOXYGEN_LAYOUT_FILE pkgdoc/packageDoxyLayout.xml )
    set( DOXYGEN_EXCLUDE lib/uthash.h )
    set( DOXYGEN_EXCLUDE_PATTERNS ktxint.h )
    set( DOXYGEN_EXAMPLE_PATH lib )
    set( DOXYGEN_HTML_HEADER pkgdoc/header.html )
    set( DOXYGEN_MAN_OUTPUT man/ktx )
    set( DOXYGEN_MAN_LINKS YES )
    set( DOXYGEN_TAGFILES ${CMAKE_BINARY_DIR}/docs/libktx.tag=libktx ${CMAKE_BINARY_DIR}/docs/ktxtools.tag=ktxtools )

    doxygen_add_docs(
        ktxpkg.doc
        pkgdoc/pages.md
        TODO.md
        interface/js_binding
        ALL
    )
    add_docs_cmake(ktxpkg.doc)
endfunction()

CreateDocLibKTX()
CreateDocKTXTools()
CreateDocKTX()

add_dependencies( libktx.doc ktx_version )
add_dependencies( ktxtools.doc libktx.doc )
add_dependencies( ktxpkg.doc ktxtools.doc )


install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs/html
    DESTINATION "${CMAKE_INSTALL_DOCDIR}"
    COMPONENT dev
)

install( DIRECTORY
    ${CMAKE_CURRENT_BINARY_DIR}/docs/man/man1
    ## Omit those, since at the moment they contain a lot of undesired files generated by Doxygen
    # ${CMAKE_CURRENT_BINARY_DIR}/docs/man/man3
    # ${CMAKE_CURRENT_BINARY_DIR}/docs/man/ktx/man3
DESTINATION
    "${CMAKE_INSTALL_MANDIR}"
COMPONENT tools
)
