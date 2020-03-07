##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on libassimp.
#
# For ios or mac install libassimp with port (macports) or brew and set
# ASSIMP_HOME accordingly in Xcode's Custom Paths preferences.
#
# For linux install libassimp from your package manager.
#
# For Windows ...?
{
  'targets': [
  {
    'variables': {
      'conditions': [
        ['OS != "linux"', {
          'assimp_include': '../other_include',
          'conditions': [
            # {ios,mac,win}olibr because repo has only a release version.
            ['OS == "ios"', {
              'assimp_lib': '<(iosolibr_dir)',
            }, 'OS == "mac"', {
              'assimp_lib': '<(macolibr_dir)',
            }, {
              'assimp_lib': '<(winolibr_dir)',
            }],
          ], # inner conditions
        }, 'OS == "web"', {
           'assimp_include': '',
           'assimp_lib': '',
        }, 'GENERATOR == "cmake"', {
          'assimp_include': '$ENV{ASSIMP_HOME}/include',
          'assimp_lib': '$ENV{ASSIMP_HOME}/lib',
        }, {
          'assimp_include': '$(ASSIMP_HOME)/include',
          'assimp_lib': '$(ASSIMP_HOME)/lib',
        }],
      ], # outer conditions
    }, # variables
    'target_name': 'libassimp',
    'type': 'none',
    'direct_dependent_settings': {
      'include_dirs': [
        '<(assimp_include)',
      ],
      'conditions': [
        ['OS == "win"', {
          'copies': [{
            'destination': '<(PRODUCT_DIR)',
            # This results in
            #  <CustomBuildTool include=".../$(PlatformName)/assimp.dll">
            # which works, but VS2010 typically will show the
            # custom copy command in properties for only one
            # configuration. Nevertheless copy will be performed
            # in all.
            'files': [ '<(winolibr_dir)/assimp.dll' ],
          }], # copies
        }],
      ], # conditions
    }, # direct_dependent_settings
    'link_settings': {
      'library_dirs': [ '<(assimp_lib)' ],
      'conditions': [
        ['OS == "ios"', {
          'xcode_settings': {
            'OTHER_LDFLAGS': '-lassimp -lz',
          },
        }, 'OS == "mac"', {
          'xcode_settings': {
            # Use static libs to avoid having to copy stuff to app bundle.
            # Use macOS installed libz. Caution that if <(ASSIMP_HOME)/lib
            # is in library_dirs, then that libz.dylib will be found which
            # will not work with the hardened runtime.
            'OTHER_LDFLAGS': '-lassimp -lIrrXML -lminizip -lz',
          },
        }, 'GENERATOR != "xcode"', {
          # '-lfoo' here confuses Xcode. It seems these values are
          # being put into an Xcode list that expects only framework
          # names, full or relative paths. The workaround is to use
          # OTHER_LDFLAGS as above.
          'libraries': [ '-lassimp' ],
        }],
      ], # conditions
    }, # link_settings
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
