##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Set configuration variables for Imagination Technology
#        PowerVR emulator
#
# Note that version 3.5 of the emulator has some bugs that cause
# errors during both the OpenGL ES 1 and OpenGL ES 3 tests. The
# former don't load the ETC2 compressed textures and the latter
# raises an error message box when loading the luminance texture
# with sized internal format.

{
  'variables': { # level 1
    'variables': { # level 2 defines variables to be used in level 1
      'variables': { # level 3 ditto
        'variables': { # level 4 ditto
          'gen_platform_arch_var': '64',
          'conditions': [
            ['GENERATOR == "msvs"', {
              # XXX Crap! The IMG directory names are not easy to map to
              # using MSVS macros. The only suitable macro (i.e. having
              # values of 32 or 64) is $(PlatformArchitecture) but it
              # does not exist in VS2005, 2008 and possibly 2010. For
              # these, default is chosen according to whether the
              # version supports x64. 32-bit builds will therefore
              # not work on those versions.  Users wishing to support
              # both platforms will need to do something like rename
              # the IMG folders to Win32 and x64 and use $(PlatformName)
              # here.
              'conditions': [
                #['int(MSVS_VERSION[:4]) >= 2010', {
                ['MSVS_VERSION[:4] != "2010" and MSVS_VERSION[:4] != "2008" and MSVS_VERSION[:4] != "2005"', {
                  'gen_platform_arch_var': '$(PlatformArchitecture)',
                }, 'MSVS_VERSION[-1] == "e"', {
                  # Express ediions of 2005 ~ 2010 do not support
                  # 64-bit.
                  'gen_platform_arch_var': '32',
                }, {
                  'gen_platform_arch_var': '64',
                }],
              ],
            }],
          ],
        },
        'gen_platform_arch_var': '<(gen_platform_arch_var)',
        # Default install location
        'conditions': [
          ['OS == "win"', {            
            'pvrsdk_dir':
            'C:/Imagination/PowerVR_Graphics/PowerVR_SDK/SDK_4.0/Builds/Windows/x86_<(gen_platform_arch_var)/Lib',
          }, {
            'pvrsdk_dir': 'somewhere', # TO DO
          }]
        ] # conditions
      }, # variables, level 3
      'gles1_bin_dir': '<(pvrsdk_dir)',
      'gles1_lib_dir': '<(pvrsdk_dir)',
      'gles2_bin_dir': '<(pvrsdk_dir)',
      'gles2_lib_dir': '<(pvrsdk_dir)',
      'gles3_bin_dir': '<(pvrsdk_dir)',
      'gles3_lib_dir': '<(pvrsdk_dir)',
    }, # variables, level 2 
    'gles1_bin_dir%': '<(gles1_bin_dir)',
    'gles1_lib_dir%': '<(gles1_lib_dir)',
    'gles2_bin_dir%': '<(gles2_bin_dir)',
    'gles2_lib_dir%': '<(gles2_lib_dir)',
    'gles3_bin_dir%': '<(gles3_bin_dir)',
    'gles3_lib_dir%': '<(gles3_lib_dir)',

    'conditions': [
      ['OS == "win"', {
        'gles1_dlls': [
           '<(gles1_bin_dir)/libEGL.dll',
           '<(gles1_bin_dir)/libGLES_CM.dll',
        ],
        'gles2_dlls': [
           '<(gles2_bin_dir)/libEGL.dll',
           '<(gles2_bin_dir)/libGLESv2.dll',
        ], 
        'gles3_dlls': [
           '<(gles3_bin_dir)/libEGL.dll',
           '<(gles3_bin_dir)/libGLESv2.dll',
        ], 
      }, {
        'gles1_dlls': [ ],
        'gles2_dlls': [ ],
        'gles3_dlls': [ ],
      }],
    ], # conditions

    'es1support': 'true',
  },
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
