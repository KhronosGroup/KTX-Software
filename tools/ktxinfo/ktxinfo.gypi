##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
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
    #  # No point in building this command line utility for Android
    # or iOS.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      'targets': [
        {
          'target_name': 'ktxinfo',
          'type': '<(executable)',
          'include_dirs' : [ '../../utils' ],
          'mac_bundle': 0,
          'dependencies': [ 'libktx.gyp:libktx.gl' ],
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
          'conditions': [
            ['emit_emscripten_configs=="true"', {
              'configurations': {
                'Debug_Emscripten': {
                  'cflags': [ '<(additional_emcc_options)' ],
                  'ldflags': [ '<(additional_emlink_options)' ],
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      'AdditionalOptions': '<(additional_emcc_options)',
                    },
                    'VCLinkerTool': {
                      'AdditionalOptions': '<(additional_emlink_options)',
                    },
                  },
                },
                'Release_Emscripten': {
                  'cflags': [ '<(additional_emcc_options)' ],
                  'ldflags': [ '<(additional_emlink_options)' ],
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      'AdditionalOptions': '<(additional_emcc_options)',
                    },
                    'VCLinkerTool': {
                      'AdditionalOptions': '<(additional_emlink_options)',
                    },
                  },
                },
              }, # configurations
            }], # emit_emscripten_configs=="true"
          ], # conditions
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
