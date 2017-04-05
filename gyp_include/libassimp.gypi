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
        ['GENERATOR == "cmake"', {
          'assimp_include': '$ENV{ASSIMP_HOME}/include',
          'assimp_lib': '$ENV{ASSIMP_HOME}/lib',
        }, {
          'assimp_include': '$(ASSIMP_HOME)/include',
          'assimp_lib': '$(ASSIMP_HOME)/lib',
        }],
      ],
    }, # variables
    'target_name': 'libassimp',
    'type': 'none',
    'direct_dependent_settings': {
      'conditions': [
        ['OS != "linux"', {
          'include_dirs': [
            '<(assimp_include)',
          ]
        }],
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
      'conditions': [
        ['OS == "ios"', {
          'library_dirs': [ '<(iosolibr_dir)' ],
        }, 'OS == "mac"', {
          'library_dirs': [ '<(assimp_lib)' ],
        }, 'OS == "win"', {
          # winolibr because repo has only a release version.
          'library_dirs': [ '<(winolibr_dir)' ],
        }],
        ['GENERATOR != "xcode"', {
          # '-lfoo' here confuses Xcode. It seems these values are
          # being put into an Xcode list that expects only framework
          # names, full or relative paths. The workaround is to use
          # OTHER_LDFLAGS as below.
          'libraries': [ '-lassimp' ],
        }],
      ], # conditions
      'xcode_settings': {
        'OTHER_LDFLAGS': '-lassimp -lz',
      },
    }, # link_settings
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
