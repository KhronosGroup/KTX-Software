# Copyright 2015-2021 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project file for building KTX loadtests.
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
  }, # variables, level 1
  'targets': [
    {
      'target_name': 'streamtests',
      'type': '<(executable)',
      'mac_bundle': 0,
      # To quiet warnings about the anon structs and unions in Basisu.
      'cflags_cc': [ '-Wno-pedantic' ],
      'dependencies': [
        #'appfwSDL',
        'gtest',
        'libktx.gyp:libktx',
      ],
      'include_dirs': [
        '../../utils',
        '../gtest/include',
      ],
      'defines': [ ],
      'sources': [
        'streamtests.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:CONSOLE
          'SubSystem': '1',
        },
      },
      'xcode_settings': {
        'USE_HEADERMAP': 'NO',
        # The BasisU transcoder uses anon typs and structs. They compile ok in
        # Visual Studio (2015+) and on Linux so quiet the clang warnings.
        'WARNING_CFLAGS': [
          '-Wno-nested-anon-types',
          '-Wno-gnu-anonymous-struct',
        ],
      },
    }, # transcodetests
  ] # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
