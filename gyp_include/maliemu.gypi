# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Set configuration variables for ARM Mali emulator.
#
# Note that this emulator does not support OpenGL ES 1.X.
# Also note that a downloaded emulator only contains the dlls for the
# download target, e.g., 64-bit in the 64-bit download. So a build
# of the project for the other platform will fail. If you do install
# both 64- and 32-bit packages, there is no suitable VS macro to use
# in *_{bin,lib}_dir so VS can find the appropriate files at build
# time. The files are installed in C:/Program Files/ARM/... and
# C:/Program Files (x86)/ARM/... Use the solution appropriate to
# the emulator version you have installed.
#

{
  'variables': { # level 1
    # The ARM MALI emulator's installer adds OPENGLES_LIBDIR to
    # the environment pointing at its libs and dlls.
    'gles1_bin_dir%': 'nowhere',
    'gles1_lib_dir%': 'nowhere',
    'gles2_bin_dir%': '$(OPENGLES_LIBDIR)',
    'gles2_lib_dir%': '$(OPENGLES_LIBDIR)',
    'gles3_bin_dir%': '$(OPENGLES_LIBDIR)',
    'gles3_lib_dir%': '$(OPENGLES_LIBDIR)',

    # The installer also adds the value $OPENGLES_LIBDIR to the
    # PATH so the dlls will be found. No need to copy anything.
    'gles1_dlls': [ ],
    'gles2_dlls': [ ],
    'gles3_dlls': [ ], 

    'es1support': 'false',
  },
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
