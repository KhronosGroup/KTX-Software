# Copyright 2015-2020 Mark Callow.
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project file for building KTX loadtests for OpenGL {,ES}.
#
{
  'variables': { # level 1
    'datadir': 'testimages',
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
      '../../../utils/argparser.h',
      '../../../utils/argparser.cpp',
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
      'shader-based/BasisuTest.cpp',
      'shader-based/BasisuTest.h',
      'shader-based/DrawTexture.cpp',
      'shader-based/DrawTexture.h',
      'shader-based/GL3LoadTests.cpp',
      'shader-based/GL3LoadTestSample.cpp',
      'shader-based/GL3LoadTestSample.h',
      'shader-based/TextureArray.cpp',
      'shader-based/TextureArray.h',
      'shader-based/TextureCubemap.cpp',
      'shader-based/TextureCubemap.h',
      'shader-based/TexturedCube.cpp',
      'shader-based/TexturedCube.h',
      'shader-based/mygl.h',
      'shader-based/shaders.cpp',
      'utils/GLMeshLoader.hpp',
      'utils/GLTextureTranscoder.hpp',
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
      'includes': [
        '../../../gyp_include/libgl.gypi'
      ],
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
            'libassimp',
            'libktx.gyp:libktx',
            'libgl',
            'testimages',
          ],
          'sources': [
            '<@(common_source_files)',
            '<@(gl3_source_files)',
          ],
          'copies': [{
            'destination': '<(model_dest)',
            'files': [
              '../common/models/cube.obj',
              '../common/models/sphere.obj',
              '../common/models/teapot.dae',
              '../common/models/torusknot.obj',
            ],
          }],
          'include_dirs': [
            '.',
            '../common',
            '../geom',
            '../../../utils',
            'utils',
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
            ['OS == "mac"', {
              'sources': [ '<(glinfoplist_file)' ],
            }, 'OS == "win"', {
              'sources': [ '<@(win_resource_files)' ],
            }],
          ], # conditions
        }, # gl3loadtests
      ], # 'OS == "mac" or OS == "win"' targets
    }], # 'OS == "mac" or OS == "win"'
    ['OS == "ios" or OS == "win" or OS == "web"', {
      'variables': {
        # Putting this condition within the target causes a GYP error.
        # I've not been able to find a way to override EXECUTABLE_SUFFIX so...
        'conditions': [
          ['OS == "web"', {
            'target_name': 'es3loadtests.html',
          }, {
            'target_name': 'es3loadtests',
          }],
        ],
      },
      'includes': [
        '../../../gyp_include/libgles3.gypi'
      ],
      'targets': [
        {
          'target_name': '<(target_name)',
          'type': '<(executable)',
          'mac_bundle': 1,
          'dependencies': [
            'appfwSDL',
            'libassimp',
            'libktx.gyp:libktx',
            'libgles3',
            'testimages',
          ],
         'sources': [
            '../geom/quad.h',
            '<@(common_source_files)',
            '<@(gl3_source_files)',
          ], # sources
          'include_dirs': [
            '.',
            '../common',
            '../geom',
            '../../../utils',
            'utils',
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
            }, 'OS == "web"', {
              'cflags': [
                '-s', 'DISABLE_EXCEPTION_CATCHING=0',
              ],
              'defines': [ 'TEST_BASIS_COMPRESSION=0' ],
              'dependencies!': [ 'libassimp' ],
              'ldflags': [
                '--source-map-base', './',
                #'--preload-file', '../common/models',
                '--preload-file', 'testimages',
                '--exclude-file', 'testimages/genref',
                '--exclude-file', 'testimages/*.pgm',
                '--exclude-file', 'testimages/*.ppm',
                '--exclude-file', 'testimages/*.pam',
                '--exclude-file', 'testimages/*.pspimage',
                '-s', 'ALLOW_MEMORY_GROWTH=1',
                '-s', 'DISABLE_EXCEPTION_CATCHING=0',
              ],
              'sources!': [
                'shader-based/TextureCubemap.cpp',
                'shader-based/TextureCubemap.h',
              ],
            }], # OS == "ios" else "win" else "web"
            ['OS != "web"', {
              'copies': [{
                'destination': '<(model_dest)',
                'files': [
                  '../common/models/cube.obj',
                  '../common/models/sphere.obj',
                  '../common/models/teapot.dae',
                  '../common/models/torusknot.obj',
                ],
              }],
            }], # OS != "web"
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
            'libktx.gyp:libktx',
            'libgles1',
            'testimages',
          ],
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
