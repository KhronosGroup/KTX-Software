##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building KTX loadtests for Vulkan.
#
{
  'includes': [
#    '../../../gyp_include/libvulkan.gypi',
  ],
  'variables': { # level 1
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
  }, # variables, level 1
  'targets': [
    {
      'target_name': 'vkloadtests',
      'type': '<(executable)',
      'mac_bundle': 1,
      'dependencies': [
        'appfwSDL',
        'libktx.gyp:libktx.gl',
        'libvulkan',
        'testimages'
      ],
      'sources': [
        '../common/at.c',
        '../common/at.h',
        'VkLoadTests.cpp',
        'VkLoadTests.h',
        'vksample_02_cube_textured.c',
      ],
      'cflags': [ '-std=c++11' ],
      'defines': [ ],
      'include_dirs': [
        '../common',
        '../geom',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:WINDOWS.
          'SubSystem': '2',
        },
      },
      'xcode_settings': {
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
        'GCC_C_LANGUAGE_STANDARD': 'c99',
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
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'AppIcon',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(infoplist_file)',
          },
        }, 'OS == "mac"', {
          'sources': [
            'resources_mac/Info.plist',
          ],
        }], # OS == "ios", etc
      ], # conditions
    }, # vkloadtests
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
