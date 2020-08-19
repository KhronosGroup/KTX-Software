# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Set configuration variables for Qualcomm Adreno emulator.
#
# Note that a downloaded emulator only contains the dlls for the
# download target, e.g., 64-bit in the 64-bit download. This leads
# to a warning about a missing file when generating projects. The
# warning can be ignored.  This also causes a project built for
# the other platform to fail to run.

{
  'variables': { # level 1
    'variables': { # level 2 defines variables to be used in level 1
      'variables': { # level 3 defines variables to be used in level 2
        # Default install location
        'conditions': [
          ['OS == "win"', {
            # Default installation location.
            'adrenosdk_dir': '$(USERPROFILE)/Desktop/AdrenoSDK',
          }, {
            'adrenosdk_dir': 'somewhere', # TO DO
          }]
        ] # conditions
      }, # variables, level 3
      'gles1_bin_dir%': 'nowhere',
      'gles1_lib_dir%': 'nowhere',
      'gles2_bin_dir': '<(adrenosdk_dir)/Bin/$(PlatformName)',
      'gles2_lib_dir': '<(adrenosdk_dir)/Development/Lib/$(PlatformName)',
      'gles3_bin_dir': '<(adrenosdk_dir)/Bin/$(PlatformName)',
      'gles3_lib_dir': '<(adrenosdk_dir)/Development/Lib/$(PlatformName)',
    }, # variables, level 2 
    'gles1_bin_dir%': '<(gles1_bin_dir)',
    'gles1_lib_dir%': '<(gles1_lib_dir)',
    'gles2_bin_dir%': '<(gles2_bin_dir)',
    'gles2_lib_dir%': '<(gles2_lib_dir)',
    'gles3_bin_dir%': '<(gles3_bin_dir)',
    'gles3_lib_dir%': '<(gles3_lib_dir)',

    'conditions': [
      ['OS == "win"', {
        'gles1_dlls': [ ],
        'gles3_dlls': [
           '<(gles2_bin_dir)/libEGL.dll',
           '<(gles2_bin_dir)/libGLESv2.dll',
           '<(gles2_bin_dir)/TextureConverter.dll',
        ], 
        'gles3_dlls': [
           '<(gles3_bin_dir)/libEGL.dll',
           '<(gles3_bin_dir)/libGLESv2.dll',
           '<(gles3_bin_dir)/TextureConverter.dll',
        ], 
      }, {
        'gles1_dlls': [ ],
        'gles2_dlls': [ ],
        'gles3_dlls': [ ],
      }],
    ], # conditions

    'es1support': 'false',
  },
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
