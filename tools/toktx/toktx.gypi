##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project files for building KTX tools.
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
          'target_name': 'toktx',
          'type': '<(executable)',
          'include_dirs' : [ '../../utils' ],
          'mac_bundle': 0,
          'dependencies': [ 'libktx.gyp:libktx.gl' ],
          'sources': [
            '../../utils/argparser.cpp',
            '../../utils/argparser.h',
            'image.cpp',
            'image.h',
            'stdafx.h',
            'targetver.h',
            'toktx.cpp',
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
        }, # toktx target
        {
          'target_name': 'toktx-tests',
          'type': 'none',
          'dependencies': [ 'toktx' ],
          'actions': [
            {
              'action_name': 'toktx-tests',
              'message': 'Running toktx tests',
              'inputs': [ '../../tests/toktx-tests' ],
              'outputs': [ 'testsrun' ],
              'action': [
                '<(_inputs)', '<(PRODUCT_DIR)/toktx',
              ],
            }, # toktx-tests action
          ], # actions
        }, # toktx-tests target
        {
          'target_name': 'toktx.doc',
          'type': 'none',
          'variables': { # level 1
            'variables': { # level 2
              'output_dir': '../../build/docs',
            },
            'output_dir': '<(output_dir)',
            'doxyConfig': 'ktxtools.doxy',
            'timestamp': '<(output_dir)/.toktxdoc_gentimestamp',
          },
          # It is not possible to chain commands in an action with
          # && because the generators will quote such strings.
          # Instead we use an external script.
          # these actions
          'actions': [
            {
              'action_name': 'buildToktxDoc',
              'message': 'Generating toktx documentation with Doxygen',
              'inputs': [
                '../../<(doxyConfig)',
                '../../runDoxygen',
                'toktx.cpp',
              ],
              # See ../../lib/libktx.gypi for comment about why only
              # timestamp is in this list.
              'outputs': [ '<(timestamp)' ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that directory will be removed
              # from paths displayed in the documentation. That is also
              # the directory where the .doxy and .gyp files are stored.
              #
              # See ../../lib/libktx.gypi for further comments.
              'msvs_cygwin_shell': 1,
              'action': [
                './runDoxygen',
                '-t', '<(timestamp)',
                '-o', '<(output_dir)/html',
                '<(doxyConfig)',
              ],
            }, # buildToktxDoc action
          ], # actions
        }, # toktx.doc
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
