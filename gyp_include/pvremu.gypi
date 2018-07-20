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
              # The IMG SDK uses non-standard platform names in its
              # library directory names. Fortunately a suitable
              # MSVS macro (i.e. having values of 32 or 64) was introduced
              # in vs2010: $(PlatformArchitecture). Since we're no longer
              # supporting vs2005 or vs2008 we can use it. In case anyone
              # still needs to use these versions the following condition
              # will set the platform architecture  according to a
              # variable that can be defined on the GYP command.
              'conditions': [
                ['MSVS_VERSION[:4] != "2008" and MSVS_VERSION[:4] != "2005"', {
                  'gen_platform_arch_var': '$(PlatformArchitecture)',
                }, 'WIN_PLATFORM == "x64"', {
                  'gen_platform_arch_var': '64',
                }, {
                  'gen_platform_arch_var': '32',
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
            'C:/Imagination/PowerVR_Graphics/PowerVR_SDK/SDK_2017_R1/Builds/Windows/x86_<(gen_platform_arch_var)/Lib',
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
