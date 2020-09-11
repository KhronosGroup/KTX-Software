# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project files for building the toktx tool.
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
          'target_name': 'toktx',
          'type': '<(executable)',
          # To quiet warnings about the anon structs and unions.
          'cflags_cc': [ '-Wno-pedantic' ],
          'include_dirs': [
            '../../utils',
            '../../lib/basisu',
          ],
          'mac_bundle': 0,
          'dependencies': [ 'libktx.gyp:libktx' ],
          'sources': [
            '../../utils/argparser.cpp',
            '../../utils/argparser.h',
            'image.cc',
            'image.hpp',
            'jpgimage.cc',
            #'lodepng.cc',
            #'lodepng.h',
            'npbmimage.cc',
            'pngimage.cc',
            'stdafx.h',
            'targetver.h',
            'toktx.cc',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              # /SUBSYSTEM:CONSOLE
              'SubSystem': '1',
            },
          },
          'xcode_settings': {
            # toktx uses anon types and structs. They compile ok in
            # Visual Studio (2015+) and on Linux so quiet the clang warnings.
            'WARNING_CFLAGS': [
              '-Wno-nested-anon-types',
              '-Wno-gnu-anonymous-struct',
            ],
          },
         'actions': [{
           'action_name': 'mkversion',
           'inputs': [
             '../../mkversion',
             '../../.git'
           ],
           'outputs': [ 'version.h' ],
           'msvs_cygwin_shell': 1,
           'action': [ './mkversion', '-o', 'version.h', 'tools/toktx' ],
         }],
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
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
