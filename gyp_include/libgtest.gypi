# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for adding dependency on gtest libraries.
#
{
  'targets': [
    {
      'target_name': 'libgtest',
      'type': 'none',
      'direct_dependent_settings': {
        'include_dirs': [
          '../other_include',
        ],
      },
      # Neither 'copies' nor 'link_settings' can appear inside
      # configurations hence source folders for copies are
      # specified using configuraton variables such as $(PlatformName)
      # and $CONFIGURATION. An error is emitted when 'link_settings'
      # is so used. No error is emitted when 'copies' is so used.
      'conditions': [
        ['OS == "android"', {
          'link_settings': {
            'libraries': [ '-lgtest_main', '-lgtest' ],
            'library_dirs': [ '<(gtest_lib_dir)' ],
          },
        }], # OS == "android"
        ['OS == "ios"', {
          'link_settings': {
            'libraries': [ 'libgtest_main.a', 'libgtest.a' ],
            'library_dirs': [ '<(gtest_lib_dir)' ],
          },
        }], # OS == "ios"
        ['OS == "linux"', {
          'link_settings': {
            'libraries': [ '-lgtest_main', '-lgtest' ],
            'library_dirs': [ '<(gtest_lib_dir)' ],
          },
        }], # OS == "linux"
        ['OS == "mac"', {
         'link_settings': {
            'libraries': [ 'libgtest_main.a', 'libgtest.a' ],
            'library_dirs': [ '<@(gtest_lib_dir)' ],
          }, # link settings
        }], # OS == "mac"
        ['OS == "win"', {
          'link_settings': {
            'libraries': [ '-lgtest_main', '-lgtest' ],
            'library_dirs': [ '<(gtest_lib_dir)' ],
          },
        }], # OS == "win"
      ], # conditions
    }, # libgtest
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
