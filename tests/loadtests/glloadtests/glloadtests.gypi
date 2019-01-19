##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building KTX loadtests for OpenGL {,ES}.
#
{
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
     }, # variables, level 2
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
        'glinfoplist_file': 'resources/ios/Info.plist',
      }, {
        'glinfoplist_file': 'resources/mac/Info.plist',
      }],
    ],
    'common_source_files': [
      '../common/LoadTestSample.cpp',
      '../common/LoadTestSample.h',
      '../common/SwipeDetector.cpp',
      '../common/SwipeDetector.h',
      '../geom/cube.h',
      '../geom/frame.h',
      'GLLoadTests.cpp',
      'GLLoadTests.h',
    ],
    'gl3_source_files': [
       # .h files are included so they will appear in IDEs' file lists.
      '../common/ltexceptions.h',
      'shader-based/DrawTexture.cpp',
      'shader-based/DrawTexture.h',
      'shader-based/GL3LoadTests.cpp',
      'shader-based/GL3LoadTestSample.cpp',
      'shader-based/GL3LoadTestSample.h',
      'shader-based/TextureArray.cpp',
      'shader-based/TextureArray.h',
      'shader-based/TexturedCube.cpp',
      'shader-based/TexturedCube.h',
      'shader-based/mygl.h',
      'shader-based/shaders.cpp',
    ],
    'ios_resource_files': [
      '../../../icons/ios/CommonIcons.xcassets',
      'resources/ios/LaunchImages.xcassets',
      'resources/ios/LaunchScreen.storyboard',
    ],
    'win_resource_files': [
      '../../../icons/win/ktx_app.ico',
      'resources/win/glloadtests.rc',
      'resources/win/resource.h',
    ]
  }, # variables, level 1

  'conditions': [
    ['OS == "mac" or OS == "win" or OS == "linux"', {
      'targets': [
        {
          'target_name': 'gl3loadtests',
          'type': '<(executable)',
          'mac_bundle': 1,
          'mac_bundle_resources': [
            '../../../icons/mac/ktx_app.icns',
          ],
          'dependencies': [
            'appfwSDL',
            'libktx.gyp:libktx.gl',
            'libktx.gyp:libgl',
            'testimages',
          ],
          'sources': [
            '<@(common_source_files)',
            '<@(gl3_source_files)',
          ],
          'include_dirs': [
            '.',
            '../common',
            '../geom',
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
              # Needed for loading this app's own icon
              'AdditionalDependencies': [ 'user32.lib' ],
            },
          },
          'xcode_settings': {
            'INFOPLIST_FILE': '<(glinfoplist_file)',
          },
          'conditions': [
            ['emit_emscripten_configs=="true"', {
              'configurations': {
                'Debug_Emscripten': {
                  'cflags': [ '<(additional_emcc_options)' ],
                  'ldflags': [
                    '--preload-files <(PRODUCT_DIR)/<(datadir)@/<(datadir)',
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
                    '--preload-files <(PRODUCT_DIR)/<(datadir)@/<(datadir)',
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
            ['OS == "mac"', {
              'sources': [ '<(glinfoplist_file)' ],
            }, 'OS == "win"', {
              'sources': [ '<@(win_resource_files)' ],
            }],
          ], # conditions
        }, # gl3loadtests
      ], # 'OS == "mac" or OS == "win"' targets
    }], # 'OS == "mac" or OS == "win"'
    ['OS == "ios" or OS == "win"', {
      'includes': [
        '../../../gyp_include/libgles3.gypi',
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
            'testimages',
          ],
          #'toolsets': [target', 'emscripten'],
          'sources': [
            '../geom/quad.h',
            '<@(common_source_files)',
            '<@(gl3_source_files)',
          ], # sources
          'include_dirs': [
            '.',
            '../common',
            '../geom',
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
              # Needed for loading this app's own icon
              'AdditionalDependencies': [ 'user32.lib' ],
            },
          },
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'ktx_app',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(glinfoplist_file)',
          },
          'conditions': [
            ['OS == "ios"', {
              'sources': [
                '<(glinfoplist_file)',
              ],
              'mac_bundle_resources': [ '<@(ios_resource_files)' ],
            }, 'OS == "win"', {
              'sources': [ '<@(win_resource_files)' ],
            }], # OS == "ios" else OS = "win"
          ],
        }, # es3loadtests
      ], # 'OS == "ios" or OS == "win"' targets
    }], # 'OS == "ios" or OS == "win"'
    ['OS == "ios" or (OS == "win" and es1support == "true")', {
      'includes': [
        '../../../gyp_include/libgles1.gypi'
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
            'testimages',
          ],
          #'toolsets': [target', 'emscripten'],
          'sources': [
            '<@(common_source_files)',
            'gles1/ES1LoadTests.cpp',
            'gles1/DrawTexture.cpp',
            'gles1/DrawTexture.h',
            'gles1/TexturedCube.cpp',
            'gles1/TexturedCube.h',
          ], # sources
          'include_dirs': [
            '.',
            '../common',
            '../geom',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:WINDOWS.
              'SubSystem': '2',
              # Needed for loading this app's own icon
              'AdditionalDependencies': [ 'user32.lib' ],
            },
          },
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'ktx_app',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(glinfoplist_file)',
          },
          'conditions': [
            ['OS == "ios"', {
              'sources': [
                '<(glinfoplist_file)',
              ],
              'mac_bundle_resources': [ '<@(ios_resource_files)' ],
            }, 'OS == "win"', {
                'sources': [ '<@(win_resource_files)' ],
            }], # OS == "ios"
          ], # conditions
        } # es1loadtests
      ], # 'OS == "ios" or OS == "win"' targets
    }] #'OS == "ios or OS == "win"'
  ], # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
