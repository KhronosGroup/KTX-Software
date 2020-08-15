# Copyright 2015-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for adding dependency on libzstd
#
# For ios or mac install libzstd with port (macports) or brew and set
# ZSTD_HOME accordingly in Xcode's Custom Paths preferences.
#
# For linux install libzstd from your package manager.
#
# For Windows ...?
{
  'targets': [
  {
    'variables': {
      'conditions': [
        ['OS == "web"', {
           'zstd_include': '',
           'zstd_lib': '',
        }, {
          'zstd_include': '../other_include',
          'conditions': [
            # {ios,linux,mac,win}olibr because repo has only a release version.
            ['OS == "ios"', {
              'zstd_lib': '<(iosolibr_dir)',
            }, 'OS == "linux"', {
              # Linux package is rather old plus for some reason does
              # not include libzstd.so, only a .so.1 which makes linking
              # ... difficult. So use one we built.
              'zstd_lib': '<(linuxolibr_dir)',
            }, 'OS == "mac"', {
              'zstd_lib': '<(macolibr_dir)',
            }, {
              'zstd_lib': '<(winolibr_dir)',
            }],
          ], # inner conditions
        }],
      ], # outer conditions
    }, # variables
    'target_name': 'libzstd',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(zstd_include)',
      ],
    }, # direct_dependent_settings
    'link_settings': {
      'library_dirs': [ '<(zstd_lib)' ],
      'conditions': [
        # Use static libs for ios & macOS to avoid having to sign and copy
        # stuff to the app bundle. Note too that if there is a libfoo.dylib
        # in the search path the bloody Apple linker will pick that despite
        # being given the full file name.
        #
        # '-lfoo' here confuses Xcode. It seems these values are being put
        # into an Xcode list that expects only framework names, full or
        # relative paths.
        ['OS == "ios"', {
          'libraries': [ 'libzstd.a' ],
        }, 'OS == "mac"', {
          'libraries': [ 'libzstd.a' ],
        }, 'OS == "win"', {
          'libraries': [ '-lzstd_static' ],
        }, 'GENERATOR != "xcode"', {
          'libraries': [ '-lzstd' ],
        }],
      ], # conditions
    }, # link_settings
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
