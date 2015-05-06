bin_PROGRAMS=toktx
toktx_SOURCES=image.cpp toktx.cpp @BUILD_ROOT@/glcore/gl_core_3_0.c
toktx_CXXFLAGS=@USEDGL@ @KTX_CFLAGS@ @GLEW_CFLAGS@ -I@BUILD_ROOT@/include -I@BUILD_ROOT@/glcore
toktx_LDFLAGS=@USEDGL_LDFLAGS@ @GLEW_LDFLAGS@
toktx_LDADD=../../lib/libktx.la
