##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project files for building KTX tools & docs.
#
{
  'xcode_settings': {
    # These actually Xcode's defaults here for documentation.
    #'DSTROOT': '/tmp/$(PROJECT_NAME).dst',
    #'INSTALL_PATH': '/usr/local/bin',
    #'DSTROOT': '<(dstroot)',
  },
  'includes': [
    'ktx2ktx2/ktx2ktx2.gypi',
    'ktxcheck/ktxcheck.gypi',
    'ktxinfo/ktxinfo.gypi',
    'ktxsc/ktxsc.gypi',
    'toktx/toktx.gypi',
  ],
  'conditions': [
    # Can't build the docs on Android or iOS.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      'targets': [
        {
          'target_name': 'ktxtools.doc',
          'type': 'none',
          'variables': { # level 1
            'variables': { # level 2
              'output_dir': '../build/docs',
            },
            'output_dir': '<(output_dir)',
            'doxyConfig': 'ktxtools.doxy',
            'timestamp': '<(output_dir)/.ktxtoolsdoc_gentimestamp',
          },
          'xcode_settings': {
              'INSTALL_PATH': '/usr/local/share/man/man1',
          },
          # It is not possible to chain commands in an action with
          # && because the generators will quote such strings.
          # Instead we use an external script.
          'actions': [
            {
              'action_name': 'buildKtxtoolsDoc',
              'message': 'Generating KTX Tools documentation with Doxygen',
              'inputs': [
                '../<(doxyConfig)',
                '../runDoxygen',
                'ktxcheck/ktxcheck.cpp',
                'ktx2ktx2/ktx2ktx2.cpp',
                'ktxinfo/ktxinfo.cpp',
                'ktxsc.cpp',
                'toktx/toktx.cpp',
              ],
              # See ../lib/libktx.gypi for comment about why only
              # timestamp is in this list.
              'outputs': [ '<(timestamp)' ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that directory will be removed
              # from paths displayed in the documentation. That is also
              # the directory where the .doxy and .gyp files are stored.
              #
              # See ../lib/libktx.gypi for further comments.
              'msvs_cygwin_shell': 1,
              'action': [
                './runDoxygen',
                '-t', '<(timestamp)',
                '-o', '<(output_dir)/html',
                '<(doxyConfig)',
              ],
            }, # buildKtxtoolsDoc action
          ], # actions
        }, # ktxtools.doc
        {
          'target_name': 'install.tools',
          'type': 'none',
          # These variables duplicate those in libktx.gyp:install.lib. The
          # default DSTROOT set by Xcode is is /tmp/$(PRODUCT_NAME).dst. For
          # this case there is therefore no need for duplication. But we'll keep
          # separate settings at least until we have this working on other
          # platforms.
          'variables': {
            'conditions': [
              ['GENERATOR == "xcode"', {
                # This weird path is because Xcode ignores its DSTROOT setting
                # when the path is an absolute path. WRAPPER_NAME defaults to
                # /Applications/$(PRODUCT_NAME).app. Use DSTROOT so that
                # xcodebuild ... install will put the .dylib in the same place.
                'dstroot': '$(WRAPPER_NAME)/../../$(DSTROOT)',
                'installpath': '$(INSTALL_PATH)',
              }, 'OS == "win"', {
                'dstroot': '$(TMP)/ktxtools.dst',
                'installpath': '/usr/local',
              }, {
                # XXX Need to figure out how to set & propagate DSTROOT to the
                # environment. See comment in ../lib/libktx.gypi.
                'dstroot': '/tmp/ktxtools.dst',
                'installpath': '/usr/local',
              }],
              ['GENERATOR == "cmake"', {
                'libktx_dir': '<(PRODUCT_DIR)/lib.target',
              }, {
                'libktx_dir': '<(PRODUCT_DIR)',
              }],
            ], # conditions
          }, # variables
          'dependencies': [
            'ktx2ktx2',
            'ktxcheck',
            'ktxinfo',
            'ktxsc',
            'ktxtools.doc',
            'libktx.gyp:libktx.gl',
            'toktx',
          ],
          'xcode_settings': {
            'INSTALL_PATH': '/usr/local',
          },
          'copies': [{
            # Do our own copy of the library because libktx:install.lib
            # is building a developers distribution.
            'xcode_code_sign': 1,
            'destination': '<(dstroot)/<(installpath)/lib',
            'conditions': [
              ['"<(library)" == "shared_library"', {
                'files': [ '<(libktx_dir)/libktx.gl<(SHARED_LIB_SUFFIX)' ],
              }],
            ], # conditions
          }, {
            'xcode_code_sign': 1,
            'destination': '<(dstroot)/<(installpath)/bin',
            'files': [
              '<(PRODUCT_DIR)/ktx2ktx2<(EXECUTABLE_SUFFIX)',
              '<(PRODUCT_DIR)/ktxcheck<(EXECUTABLE_SUFFIX)',
              '<(PRODUCT_DIR)/ktxinfo<(EXECUTABLE_SUFFIX)',
              '<(PRODUCT_DIR)/ktxsc<(EXECUTABLE_SUFFIX)',
              '<(PRODUCT_DIR)/toktx<(EXECUTABLE_SUFFIX)',
            ],
          }, {
            'destination': '<(dstroot)/<(installpath)/share/man',
            'files': [ '../build/docs/man/man1/' ],
          }]
        }, # install.tools target
        {
          'target_name': 'package.tools',
          'type': 'none',
          'dependencies': [ 'install.tools' ],
          'conditions': [
            # No packages for other systems yet.
            ['OS == "mac"', {
              'actions': [{
                'action_name': 'buildToolsPackage',
                'message': 'Assembling distribution package',
                'inputs': [
                  'package/mac/ktxtools.pkgproj',
                ],
                'outputs': [ '../build/packages/mac/ktxtools.pkg' ],
                'action': [
                  'packagesbuild', '<@(_inputs)',
                ],
              }], # actions
            }], # OS == mac
          ], # conditions
        }, # package target
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
