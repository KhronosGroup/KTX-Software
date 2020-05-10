
set( DOXYGEN_PROJECT_NAME "libktx - The KTX Library" )

set( DOXYGEN_PROJECT_LOGO icons/ktx_document_small.png )
set( DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/docs)
set( DOXYGEN_ALIASES error=\"\\par Errors\\n\" )
set( DOXYGEN_OPTIMIZE_OUTPUT_FOR_C YES )
set( DOXYGEN_TYPEDEF_HIDES_STRUCT YES )
set( DOXYGEN_EXTRACT_LOCAL_CLASSES NO )
set( DOXYGEN_HIDE_UNDOC_MEMBERS YES )
set( DOXYGEN_CASE_SENSE_NAMES YES )
set( DOXYGEN_SHOW_USED_FILES NO )
set( DOXYGEN_EXCLUDE lib/uthash.h )
set( DOXYGEN_EXCLUDE_PATTERNS ktxint.h )
set( DOXYGEN_EXAMPLE_PATH examples lib )
set( DOXYGEN_VERBATIM_HEADERS NO )
set( DOXYGEN_CLANG_ASSISTED_PARSING NO )
set( DOXYGEN_ALPHABETICAL_INDEX NO )
set( DOXYGEN_HTML_OUTPUT html/libktx )
set( DOXYGEN_HTML_TIMESTAMP YES )
set( DOXYGEN_DISABLE_INDEX YES )
set( DOXYGEN_GENERATE_TREEVIEW YES )
set( DOXYGEN_GENERATE_LATEX NO )
# set( DOXYGEN_PAPER_TYPE a4wide ) # note: invalid value!
set( DOXYGEN_GENERATE_MAN YES )
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
    ktx.doc
    LICENSE.md
    TODO.md
    include
    lib/basis_encode.cpp
    lib/basisu_image_transcoders.h
    lib/basis_transcode.cpp
    lib/strings.c
    lib/mainpage.md
    lib/glloader.c
    lib/hashlist.c
    lib/texture.c
    lib/texture1.c
    lib/texture2.c
    lib/vkloader.c
    lib/writer1.c
    lib/writer2.c
)