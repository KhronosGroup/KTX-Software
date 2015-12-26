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
      # 'testimages_dir': '../../testimages',
     }, # variables, level 2
     #'data_files': [
     # '<!@(ls <(testimages_dir)/*.ktx)',
     #],
    #'datadir': 'testimages',
    'additional_emcc_options': [ '<@(additional_emcc_options)' ],
    'additional_emlink_options': [
      '<@(additional_emcc_options)',
    ],
  },
  'conditions': [
    # No point in building this command line utility for iOS or
    # Android.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      #'includes': [ '../../gyp_include/libgl.gypi' ],
      'targets': [
        {
          'target_name': 'toktx',
          'type': '<(executable)',
          'mac_bundle': 0,
          'dependencies': [
            'libktx.gl',
          ],
          'sources': [
            'image.cpp',
            'image.h',
            'stdafx.h',
            'targetver.h',
            'toktx.cpp',
          ],
          'defines': [
           'KTX_OPENGL=1',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:CONSOLE
              'SubSystem': '1',
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
                    '<(additional_emlink_options)',
                  ],
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
                  'ldflags': [
                    '<(additional_emlink_options)',
                  ],
                  'msvs_settings': {
                    'VCCLCompilerTool': {
                      'AdditionalOptions': '<(additional_emcc_options)',
                    },
                    'VCLinkerTool': {
                      'AdditionalOptions': '<(additional_emlink_options)',
                    },
                  },
                },
              },
            }], # emit_emscripten_configs=="true"
          ], # conditions
        }, # toktx
      ], # 'OS == "mac" or OS == "win"' targets
    }], # 'OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
