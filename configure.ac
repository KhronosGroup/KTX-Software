AC_INIT([ktx],[2.0])
PKG_PROG_PKG_CONFIG
BUILD_ROOT=$(dirname $(realpath $0))
echo $BUILD_ROOT
AC_SUBST([BUILD_ROOT])
AC_ARG_WITH([openglesv1],[AS_HELP_STRING([--with-openglesv1], [use OpenGL ES v1])],[
    if test -v GL_SPECIFIED; then 
        AC_MSG_ERROR([Please specify only one gl/gles library1])
    fi
    AC_SUBST([USEDGL],[-D KTX_OPENGL_ES1=1])
    GL_SPECIFIED="glesv1"
    PKG_CHECK_MODULES([glesv1], [glesv1_cm],[
        KTX_CFLAGS="-DKTX_OPENGL_ES1=1"
        AC_SUBST([KTX_CFLAGS])
        AC_SUBST([USEDGL_CFLAGS],[$glesv1_CFLAGS])
        AC_SUBST([USEDGL_LDFLAGS],[$glesv1_LIBS])
    ],[
        AC_MSG_ERROR([the specified module glesv1 could not be found. Please check whether it is installed and the .pc file was placed somewhere in \$PKG_CONFIG_PATH.])
    ])
])
AC_ARG_WITH([openglesv2],[AS_HELP_STRING([--with-openglesv2], [use OpenGL ES v2])],[
    if test -v GL_SPECIFIED; then 
        AC_MSG_ERROR([Please specify only one gl/gles library2])
    fi
    AC_SUBST([USEDGL],[-D KTX_OPENGL_ES2=1])
    GL_SPECIFIED="glesv2"
    PKG_CHECK_MODULES([glesv2], [glesv2],[
        KTX_CFLAGS="-DKTX_OPENGL_ES2=1"
        AC_SUBST([KTX_CFLAGS])
        AC_SUBST([USEDGL_CFLAGS],[$glesv2_CFLAGS])
        AC_SUBST([USEDGL_LDFLAGS],[$glesv2_LIBS])
    ],[
        AC_MSG_ERROR([the specified module glesv2 could not be found. Please check whether it is installed and the .pc file was placed somewhere in \$PKG_CONFIG_PATH.])
    ])
])
AC_ARG_WITH([openglesv3],[AS_HELP_STRING([--with-openglesv3], [use OpenGL ES v3])],[
    if test -v GL_SPECIFIED; then 
        AC_MSG_ERROR([Please specify only one gl/gles library3])
    fi
    GL_SPECIFIED="glesv3"
    PKG_CHECK_MODULES([glesv3], [glesv3],[
        KTX_CFLAGS="-DKTX_OPENGL_ES3=1"
        AC_SUBST([KTX_CFLAGS])
        AC_SUBST([USEDGL_CFLAGS],[$glesv3_CFLAGS])
        AC_SUBST([USEDGL_LDFLAGS],[$glesv3_LIBS])
    ],[
        AC_MSG_ERROR([the specified module glesv3 could not be found. Please check whether it is installed and the .pc file was placed somewhere in \$PKG_CONFIG_PATH.])
    ])
])
AC_ARG_WITH([opengl],[AS_HELP_STRING([--with-opengl], [use OpenGL])],[
    if test -v GL_SPECIFIED; then 
        AC_MSG_ERROR([Please specify only one gl/gles library])
    fi
    GL_SPECIFIED="ogl"
    PKG_CHECK_MODULES([gl], [gl],[
        KTX_CFLAGS="-DKTX_OPENGL=1"
        AC_SUBST([KTX_CFLAGS])
        AC_SUBST([USEDGL_CFLAGS],[$gl_CFLAGS])
        AC_SUBST([USEDGL_LDFLAGS],[$gl_LIBS])
    ],[
        AC_MSG_ERROR([the specified module gl could not be found. Please check whether it is installed and the .pc file was placed somewhere in \$PKG_CONFIG_PATH.])
    ])
])
AC_ARG_WITH([glew], [AS_HELP_STRING([--with-glew], [do not use glew as GL helper library (only checked when opengl is used)])],[
    if test ! $GL_SPECIFIED = "ogl"; then
        AC_MSG_WARN([disabling glew only affects gl. Ignoring it...])
    else
        PKG_CHECK_MODULES([glew], [glew], [
            glew_CFLAGS="$glew_CFLAGS -DUSE_GLEW"
            AC_SUBST([GLEW_CFLAGS], [$glew_CFLAGS])
            AC_SUBST([GLEW_LDFLAGS], [$glew_LIBS])
        ])
    fi
],[
    if test $GL_SPECIFIED = "ogl"; then
        GLCORE_LTLIBS=$PWD/glcore/libglcore30.la
        GLCORE_CFLAGS="-I$BUILD_ROOT/glcore -DUSE_GLCORE"
        AC_SUBST([GLCORE_LTLIBS])
        AC_SUBST([GLCORE_CFLAGS])
    fi
])
AC_PROG_CC
AC_PROG_CXX
AM_INIT_AUTOMAKE
LT_INIT
AC_OUTPUT([Makefile\
	  lib/Makefile\
	  include/Makefile\
	  glcore/Makefile\
	  tools/toktx/Makefile])
