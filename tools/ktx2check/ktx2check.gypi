# Copyright 2019-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project files for building ktx2check.
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
    # No point in building this command line utility for Android
    # or iOS.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      'targets': [
        {
          'target_name': 'ktx2check',
          'type': '<(executable)',
          'include_dirs': [
            '../../lib',
            '../../utils',
          ],
          'mac_bundle': 0,
          'dependencies': [ 'libktx.gyp:libktx' ],
          'sources': [
            '../../utils/argparser.cpp',
            '../../utils/argparser.h',
            '../../utils/ktxapp.h',
            'ktx2check.cpp',
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
            'action': [ './mkversion', '-o', 'version.h', 'tools/ktx2check' ],
          }],
        }, # ktx2check target
        {
          'target_name': 'ktx2check-tests',
          'type': 'none',
          'dependencies': [ 'ktx2check' ],
          'actions': [
            {
              'action_name': 'ktx2check-tests',
              'message': 'Running ktx2check tests',
              'inputs': [ '../../tests/ktx2check-tests' ],
              'outputs': [ 'testsrun' ],
              'action': [
                '<(_inputs)', '<(PRODUCT_DIR)/ktx2check',
              ],
            }, # ktx2check-tests action
          ], # actions
        }, # ktx2check-tests target
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
