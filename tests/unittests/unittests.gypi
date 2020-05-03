##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building KTX unittests.
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
    'additional_emcc_options': [ '<@(additional_emcc_options)' ],
    'additional_emlink_options': [
      '<@(additional_emcc_options)',
      '-s', 'USE_SDL=2',
    ],
    # A hack to get the file name relativized for xcode's INFOPLIST_FILE.
    # Keys ending in _file & _dir assumed to be paths and are made relative
    # to the main .gyp file.
#    'conditions': [
#      ['OS == "ios"', {
#        'infoplist_file': 'resources_ios/Info.plist',
#      }, {
#        'infoplist_file': 'resources_mac/Info.plist',
#      }],
#    ],
  }, # variables, level 1
  'targets': [
    {
      'target_name': 'unittests',
      'type': '<(executable)',
      'mac_bundle': 0,
      'dependencies': [
        #'appfwSDL',
        'gtest',
        'gtest_main',
        'libktx.gyp:libktx',
      ],
      'sources': [
        'unittests.cc',
        'wthelper.h',
      ],
      'include_dirs': [
        '../gtest/include',
        '../../lib',
      ],
      'defines': [ ],
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:CONSOLE
          'SubSystem': '1',
        },
      },
#        'xcode_settings': {
#          'INFOPLIST_FILE': '<(infoplist_file)',
#        },
#      'conditions': [
#       ['OS == "mac"', {
#          'sources': [
#            'resources_mac/Info.plist',
#          ],
#        }], # OS == "mac"
#      ], # conditions
    }, # unittests
  ] # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
