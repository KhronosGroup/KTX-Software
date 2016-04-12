##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building KTX loadtests.
#
{
  'includes': [
    'appfwSDL/appfwSDL.gypi',
  ],
  'variables': { # level 1
    'variables': { # level 2 so can use in level 1
       # This is a list to avoid a very wide line.
       # -s is separate because '-s foo' in a list results
       # in "-s foo" on output.
       'additional_emcc_options': [
         '-s', 'ERROR_ON_UNDEFINED_SYMBOLS=1',
         '-s', 'TOTAL_MEMORY=52000000',
         '-s', 'NO_EXIT_RUNTIME=1',
       ],
       'testimages_dir': '../../testimages',
     }, # variables, level 2
     'data_files': [
        '<!@(ls <(testimages_dir)/*.ktx)',
     ],
    'datadir': 'testimages',
    'additional_emcc_options': [ '<@(additional_emcc_options)' ],
    'additional_emlink_options': [
      '<@(additional_emcc_options)',
      '-s', 'USE_SDL=2',
    ],
    # A hack to get the file name relativized for xcode's INFOPLIST_FILE.
    # Keys ending in _file & _dir assumed to be paths and are made relative
    # to the main .gyp file.
    'conditions': [
      ['OS == "ios"', {
        'infoplist_file': 'resources_ios/Info.plist',
      }, {
        'infoplist_file': 'resources_mac/Info.plist',
      }],
    ],
    'common_source_files': [
      'common/at.h',
      'common/at.c',
      'common/LoadTests.cpp',
      'common/LoadTests.h',
      'data/cube.h',
      'data/frame.h',
    ],
    'gl3_source_files': [
       # .h files are included so they will appear in IDEs' file lists.
      'shader-based/LoadTestsGL3.cpp',
      'shader-based/sample_01_draw_texture.c',
      'shader-based/sample_02_cube_textured.c',
      'shader-based/shaderfuncs.c',
      'shader-based/shaders.c',
    ],
  }, # variables, level 1

  'conditions': [
    ['OS == "mac" or OS == "win" or OS == "linux"', {
      'targets': [
        {
          'target_name': 'gl3loadtests',
          'type': '<(executable)',
          'mac_bundle': 1,
          'dependencies': [
            'appfwSDL',
            'libktx.gyp:libktx.gl',
            'libktx.gyp:libgl',
          ],
          'sources': [
            '<@(common_source_files)',
            '<@(gl3_source_files)',
          ],
          'include_dirs': [
            'common',
          ],
          'defines': [
           'GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_CORE',
           'GL_CONTEXT_MAJOR_VERSION=3',
           'GL_CONTEXT_MINOR_VERSION=3',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
            },
          },
          'xcode_settings': {
            'INFOPLIST_FILE': '<(infoplist_file)',
          },
          'conditions': [
            ['emit_emscripten_configs=="true"', {
              'configurations': {
                'Debug_Emscripten': {
                  'cflags': [ '<(additional_emcc_options)' ],
                  'ldflags': [
                    '--preload-files <(PRODUCT_DIR)/(datadir)@/<(datadir)',
                    '<(additional_emlink_options)',
                  ],
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      'AdditionalOptions': '<(additional_emcc_options)',
                    },
                    'VCLinkerTool': {
                      'PreloadFile': '<(PRODUCT_DIR)/<(datadir)@/<(datadir)',
                      'AdditionalOptions': '<(additional_emlink_options)',
                    },
                  },
                },
                'Release_Emscripten': {
                  'cflags': [ '<(additional_emcc_options)' ],
                  'ldflags': [
                    '--preload-files <(PRODUCT_DIR)/(datadir)@/<(datadir)',
                    '<(additional_emlink_options)',
                  ],
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      'AdditionalOptions': '<(additional_emcc_options)',
                    },
                    'VCLinkerTool': {
                      'PreloadFile': '<(PRODUCT_DIR)/<(datadir)@/<(datadir)',
                      'AdditionalOptions': '<(additional_emlink_options)',
                    },
                  },
                },
              },
            }], # emit_emscripten_configs=="true"
            ['OS == "win" or OS == "linux"', {
              'copies': [{
                'destination': '<(PRODUCT_DIR)/<(datadir)',
                'files': [ '<@(data_files)' ],
              }],
            }], # OS == "win"
            ['OS == "mac"', {
              'sources': [
                'resources_mac/Info.plist',
              ],
              'copies': [{
                'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/<(datadir)',
                'files': [ '<@(data_files)' ],
              }],
            }],
            ['OS == "android"', {
              #'includes': [ '../android_app_common.gypi' ],
              'copies': [{
                'destination': '<(android_assets_dir)/<(datadir)',
                'files': [ '<@(data_files)' ],
              }], # copies
            }], # OS == "android"
          ], # conditions
        }, # gl3loadtests
      ], # 'OS == "mac" or OS == "win"' targets
    }], # 'OS == "mac" or OS == "win"'
    ['OS == "ios" or OS == "win"', {
      'includes': [
        '../../gyp_include/libgles3.gypi',
      ],
      'targets': [
        {
          'target_name': 'es3loadtests',
          'type': '<(executable)',
          'mac_bundle': 1,
          'dependencies': [
            'appfwSDL',
            'libktx.gyp:libktx.es3',
            'libgles3',
          ],
          #'toolsets': [target', 'emscripten'],
          'sources': [
            '<@(common_source_files)',
            'data/quad.h',
            '<@(gl3_source_files)',
          ], # sources
          'include_dirs': [
            'common',
          ],
          'defines': [
           'GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_ES',
           'GL_CONTEXT_MAJOR_VERSION=3',
           'GL_CONTEXT_MINOR_VERSION=0',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
            },
          },
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'AppIcon',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(infoplist_file)',
          },
          'conditions': [
            ['OS == "ios"', {
              'sources': [
                'resources_ios/Info.plist',
              ],
              'mac_bundle_resources': [
                'resources_ios/Images.xcassets',
                'resources_ios/LaunchScreen.storyboard',
              ],
              'copies': [{
                'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/<(datadir)',
                'files': [ '<@(data_files)' ],
              }],
            }], # OS == "ios"
            ['OS == "win"', {
              'copies': [{
                'destination': '<(PRODUCT_DIR)/<(datadir)',
                'files': [ '<@(data_files)' ],
              }],
            }], # OS == "win"
          ],
        }, # es3loadtests
      ], # 'OS == "ios" or OS == "win"' targets
    }], # 'OS == "ios" or OS == "win"'
    ['OS == "ios" or (OS == "win" and es1support == "true")', {
      'includes': [
        '../../gyp_include/libgles1.gypi'
      ],
      'targets': [
        {
          'target_name': 'es1loadtests',
          'type': '<(executable)',
          'mac_bundle': 1,
          'dependencies': [
            'appfwSDL',
            'libktx.gyp:libktx.es1',
            'libgles1',
          ],
          #'toolsets': [target', 'emscripten'],
          'sources': [
            '<@(common_source_files)',
            'gles1/LoadTestsES1.cpp',
            'gles1/sample_01_draw_texture.c',
            'gles1/sample_02_cube_textured.c',
          ], # sources
          'include_dirs': [
            'common',
          ],
          'defines': [
            'GL_CONTEXT_PROFILE=SDL_GL_CONTEXT_PROFILE_ES',
            'GL_CONTEXT_MAJOR_VERSION=1',
            'GL_CONTEXT_MINOR_VERSION=1',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
            },
          },
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'AppIcon',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(infoplist_file)',
          },
          'conditions': [
            ['OS == "ios"', {
              'sources': [
                'resources_ios/Info.plist',
              ],
              'mac_bundle_resources': [
                'resources_ios/Images.xcassets',
                'resources_ios/LaunchScreen.storyboard',
              ],
              'copies': [{
                'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/<(datadir)',
                'files': [ '<@(data_files)' ],
              }],
            }], # OS == "ios"
            ], # conditions
        } # es1loadtests
      ], # 'OS == "ios" or OS == "win"' targets
    }] #'OS == "ios or OS == "win"'
  ], # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
