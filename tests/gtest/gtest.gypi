##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on gtest libraries.
#
{
  'targets': [
    {
      'target_name': 'gtest',
      'type': 'static_library',
      'defines': [
        'GTEST_HAS_PTHREAD=0',
        '_HAS_EXCEPTIONS=1',
      ],
      'direct_dependent_settings': {
        'include_dirs': [ 'include' ],
        'xcode_settings': {
          # For variadic macros
          'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
        },
      },
      'include_dirs': [
        'include',
        '.',
      ],
      'sources': [
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
        'xcode_settings': {
          'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
        },
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
      'xcode_settings' : {
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
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
