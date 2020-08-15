# Copyright 2015-2020 Mark Callow.
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for adding dependency on gtest libraries.
#
{
  'variables': {
    # Hack to get the directory relativized
    'include_dir': 'include',
    'common_defines': [
      'GTEST_HAS_PTHREAD=0',
      '_HAS_EXCEPTIONS=1',
    ],
    'common_include_dirs': [ 'include' ],
  },
  'targets': [
    {
      'target_name': 'gtest',
      'type': 'static_library',
      'defines': [ '<@(common_defines)' ],
      'direct_dependent_settings': {
        'defines': [ '<@(common_defines)' ],
        'include_dirs': [ '<@(common_include_dirs)' ],
        'xcode_settings': {
          # For variadic macros
          'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
        },
      },
      'include_dirs': [
        '<@(common_include_dirs)',
        '.',
      ],
      'sources': [
        '<!@(ls <(include_dir)/gtest/*.h)',
        '<!@(ls <(include_dir)/gtest/internal/*.h)',
        'src/gtest-all.cc'
      ],
      'msvs_settings': {
        'VCCLCompilerTool': {
          'WarningLevel': 4,
          'WarnAsError': 'true',
        },
      },
      'xcode_settings': {
        # For variadic macros
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
      },
      'configurations': {
        'Debug': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              # EnableFastChecks
              'BasicRuntimeChecks': 3,
            }
          }
        }
      },
    }, # gtest
    {
      'target_name': 'gtest_main',
      'type': 'static_library',
      'direct_dependent_settings': {
        'include_dirs': [ 'include' ],
      },
      'include_dirs': [
        'include',
        '.'
      ],
      'sources': [
        'src/gtest_main.cc'
      ],
      'msvs_settings': {
        'VCCLCompilerTool': {
          'WarningLevel': 4,
          'WarnAsError': 'true',
        },
      },
      'configurations': {
        'Debug': {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'BasicRuntimeChecks': 3,
            }
          }
        }
      }
    }, # gtest_main
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
