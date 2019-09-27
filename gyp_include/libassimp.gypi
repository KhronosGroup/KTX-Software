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
        ['OS == "mac"', {
#          'copies': [{
#           'xcode_code_sign': 1,
#            'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
#            'files': [
#              '<(macolibr_dir)/libassimp.5.dylib',
#              '<(macolibr_dir)/libassimp.dylib',
#              '<(macolibr_dir)/libIrrXML.dylib',
#            ],
#          }, {
#            'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
#            'files': [
#              '<(macolibr_dir)/libassimp.dylib',
#              '<(macolibr_dir)/libassimp.5.dylib',
#            ],
#          }], # copies
          'xcode_settings': {
            # Tell DYLD where to search for this dylib.
            # "man ld" for more information. Look for -rpath.
            'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path/../Frameworks' ],
          },
        }, 'OS == "win"', {
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
          'xcode_settings': {
            'OTHER_LDFLAGS': '-lassimp -lz',
          },
        }, 'OS == "mac"', {
          'library_dirs': [ '<(macolibr_dir)' ],
          'xcode_settings': {
            # Use static libs to avoid having to build our own minizip & libz.
            # When I tried to get assimp's CMake to build zlib it reported
            # that the install name wasn't being set with @rpath wasn't which
            # means it wouldn't work even if we copied it into the app bundle.
            # Plus it continued to build a dynamic zlib even when
            # BUILD_SHARED_LIBS was unchecked.
            'OTHER_LDFLAGS': '-lassimp -lIrrXML <(assimp_lib)/libminizip.a <(assimp_lib)/libz.a',
          },
        }, 'OS == "win"', {
          # winolibr because repo has only a release version.
          'library_dirs': [ '<(winolibr_dir)' ],
        }],
        ['GENERATOR != "xcode"', {
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
