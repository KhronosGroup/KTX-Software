##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project files for building ktxcheck.
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
          'target_name': 'ktxcheck',
          'type': '<(executable)',
          'include_dirs' : [ '../../utils' ],
          'mac_bundle': 0,
          'dependencies': [ 'libktx.gyp:libktx.gl' ],
          'include_dirs': [ '../../lib' ],
          'sources': [
            '../../utils/argparser.cpp',
            '../../utils/argparser.h',
            #'../../utils/ktxapp.cpp',
            '../../utils/ktxapp.h',
            'ktxcheck.cpp',
            'stdafx.h',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:CONSOLE
              'SubSystem': '1',
            },
          },
        }, # ktxcheck target
#        {
#          'target_name': 'ktxcheck-tests',
#          'type': 'none',
#          'dependencies': [ 'ktxcheck' ],
#          'actions': [
#            {
#              'action_name': 'ktxcheck-tests',
#              'message': 'Running ktxcheck tests',
#              'inputs': [ '../../tests/ktxcheck-tests' ],
#              'outputs': [ 'testsrun' ],
#              'action': [
#                '<(_inputs)', '<(PRODUCT_DIR)/ktxcheck',
#              ],
#            }, # ktxcheck-tests action
#          ], # actions
#        }, # toktx-tests target
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
