# Copyright 2019-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project files for building ktxinfo.
#
{
  'variables': { # level 1
    'variables': { # level 2 so can use in level 1
      # This is a list to avoid a very wide line.
      # -s is separate because '-s foo' in a list results
      # in "-s foo" on output.
      'additional_emcc_options': [
        '-s', 'ERROR_ON_UNDEFINED_SYMBOLS=1',
      ],
    }, # variables, level 2
    'additional_emcc_options': [ '<@(additional_emcc_options)' ],
    'additional_emlink_options': [
      '<@(additional_emcc_options)',
    ],
  },
  'conditions': [
    # No point in building this command line utility for Android or iOS.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      'targets': [
        {
          'target_name': 'ktxinfo',
          'type': '<(executable)',
          'include_dirs' : [ '../../utils' ],
          'mac_bundle': 0,
          'dependencies': [ 'libktx.gyp:libktx' ],
          'sources': [
            '../../utils/argparser.cpp',
            '../../utils/argparser.h',
            'ktxinfo.cpp',
            'stdafx.h',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:CONSOLE
              'SubSystem': '1',
            },
          },
          'actions': [{
            'action_name': 'mkversion',
            'inputs': [
              '../../mkversion',
              '../../.git'
            ],
            'outputs': [ 'version.h' ],
            'msvs_cygwin_shell': 1,
            'action': [ './mkversion', '-o', 'version.h', 'tools/ktxinfo' ],
          }],
        }, # ktxinfo target
#        {
#          'target_name': 'ktxinfo-tests',
#          'type': 'none',
#          'dependencies': [ 'ktxinfo' ],
#          'actions': [
#            {
#              'action_name': 'ktxinfo-tests',
#              'message': 'Running ktxinfo tests',
#              'inputs': [ '../../tests/ktxinfo-tests' ],
#              'outputs': [ 'testsrun' ],
#              'action': [
#                '<(_inputs)', '<(PRODUCT_DIR)/ktxinfo',
#              ],
#            }, # ktxinfo-tests action
#          ], # actions
#        }, # ktxinfo-tests target
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
