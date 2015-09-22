##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on SDL 2.
#
{
  'includes': [
    'config.gypi',
  ],
  'targets': [
    {
      'target_name': 'libsdl',
      'type': 'none',
      'direct_dependent_settings': {
        'include_dirs': [
          '../other_include',
        ],
      },
      # Neither 'copies' nor 'link_settings' can appear inside
      # configurations hence use of $(PlatformName) and $CONFIGURATION.
      # An error is emitted when 'link_settings' is so used. No error
      # is emitted when 'copies' is so used.
      'conditions': [
        ['OS == "win"', {
          'link_settings': {
            'libraries=': [ '-lSDL2', '-lSDL2main' ],
            'library_dirs': [ '<(sdl2_lib_dir)' ],
          },
          'conditions': [
            ['sdl2_dll_dir != ""', {
              'copies': [{
                'destination': '<(PRODUCT_DIR)',
                # This results in <CustomBuildTool include=".../$(PlatformName)/SDL2.dll">
                # which works, but VS2010 typically will show the custom
                # copy command in properties for only one configuration.
                # Nevertheless copy will be performed in all.
                'files': [ '<(sdl2_dll_dir)/SDL2.dll' ],
              }], # copies
            }],
          ], # conditions
        }],
        ['OS == "ios"', {
          'link_settings': {
            'libraries=': [
              '<@(otherlibroot_dir)/$CONFIGURATION-$(PLATFORM_NAME)/libSDL2.a',
              '$(SDKROOT)/System/Library/Frameworks/UIKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreMotion.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreGraphics.framework',
              '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
             ],
          },
        }],
        ['OS == "mac"', {
         'link_settings': {
            'conditions': [
              ['sdl_to_use == "installed_framework"', {
                'libraries=': [
                  '<@(sdl2.framework_dir)/SDL2.framework',
                ],
              }, 'sdl_to_use == "built_framework"', {
                'libraries=': [
                  '<@(sdl2_lib_dir)/SDL2.framework',
                ],
              }, {
                'libraries=': [
                  '<@(sdl2_lib_dir)/libSDL2.dylib',
                ],
                 'library_dirs=': [ '<@(sdl2_lib_dir)' ],
              }],
            ], # conditions
            'libraries=': [
              '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
              '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework',
            ]
          }, # link settings
          'direct_dependent_settings': {
            'conditions': [
              ['sdl_to_use == "installed_framework"', {
                'mac_framework_dirs': [ '<@(sdl2.framework_dir)' ],
              }, 'sdl_to_use == "built_framework"', {
#                'include_dirs': [
#                  '<(sdl2_lib_dir)/SDL2.framework/Headers',
#                ],
                'mac_framework_dirs': [ '<@(sdl2_lib_dir)' ],
                # Do "man dyld" for information about @executable_path.
                'xcode_settings': {
                  'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path/../Frameworks' ],
                },
                'copies': [{
                  # A small change to GYP was required to use
                  # FRAMEWORKS_FOLDER_PATH
                  'destination': '$(FRAMEWORKS_FOLDER_PATH)',
                  'files': [ '<(sdl2_lib_dir)/SDL2.framework' ],
                }],
              }, 'sdl_to_use == "dylib"', {
                'xcode_settings': {
                  'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path/.' ],
                },
                'copies': [{
                  # A small change to GYP was required to use
                  # EXECUTABLE_FOLDER_PATH.
                  'destination': '$(EXECUTABLE_FOLDER_PATH)',
                  'files': [ '<@(sdl2_lib_dir)/libSDL2.dylib' ],
                }],
              }],
            ],
          },
        }],
        ['OS == "ios" or OS == "mac"', {
          'link_settings': {
            'libraries=': [
              '$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework',
              '$(SDKROOT)/System/Library/Frameworks/CoreAudio.framework',
              '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/GameController.framework',
            ],
          },
        }],
        ['OS == "android"', {
          'variables': {
             'droidolib_dir': '<(otherlibroot_dir)/$(BUILDTYPE)/$(TARGET_ABI)',
          },
          'link_settings': {
            'libraries': [ '-lSDL2' ],
            'library_dirs': [ '<(droidolib_dir)' ],
          },
          'direct_dependent_settings': {
            'sources': [
              # Include the .o here instead of as -lSDL2main, in
              # libraries above, to force its inclusion in the
              # output .so. This is necessary because the contained
              # functions are only referenced from Java.
              '<(droidolib_dir)/SDL_android_main.o',
            ],
            # Copy the .so to where the APK builder will find it.
            # Strip the symbols to reduce download time to the
            # Android device/emulator. A gdb.setup file will be
            # created which will enable the debugger to find the
            # source unstripped .so on the host to get the symbols.
            #
            # Put in direct_dependent_settings as we need to copy to a
            # place named for the dependent target.
            'actions': [{
              'action_name': 'copyandstripSDL2',
              'message': 'Copying and stripping libSDL2.so to',
              'inputs': [ '<(droidolib_dir)/libSDL2.so' ],
              'outputs': [
                '<(android_projects_dir)/>(_target_name)/libs/$(TARGET_ABI)/libSDL2.so',
              ],
              'action': [
                '$(TARGET_STRIP)', '--strip-unneeded', '-o', '<@(_outputs)', '<@(_inputs)',
              ],
            }], # actions
          } # direct_dependent_settings
        }], # OS == "Android"
      ], # conditions
    }, # libsdl
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
