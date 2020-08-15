# Copyright 2019-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project files for building KTX tools & docs.
#
{
  'xcode_settings': {
    # These are Xcode's defaults here for documentation.
    #'DSTROOT': '/tmp/$(PROJECT_NAME).dst',
    #'INSTALL_PATH': '/usr/local/bin',
    # Override DSTROOT to use same place for lib & tools.
    'DSTROOT': '/tmp/ktx.dst',
  },
  'includes': [
    'ktx2ktx2/ktx2ktx2.gypi',
    'ktx2check/ktx2check.gypi',
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
                'ktx2check/ktx2check.cpp',
                'ktx2ktx2/ktx2ktx2.cpp',
                'ktxinfo/ktxinfo.cpp',
                'ktxsc/ktxsc.cpp',
                'toktx/toktx.cc',
                '../utils/ktxapp.h',
                '../utils/scapp.h',
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
          'dependencies': [
            'ktx2ktx2',
            'ktx2check',
            'ktxinfo',
            'ktxsc',
            'ktxtools.doc',
            'libktx.gyp:install.lib',
            'toktx',
          ],
          'xcode_settings': {
            'INSTALL_PATH': '/usr/local',
          },
          'copies': [{
            'xcode_code_sign': 1,
            'destination': '<(dstroot)/<(installpath)/bin',
            'files': [
              '<(PRODUCT_DIR)/ktx2ktx2<(EXECUTABLE_SUFFIX)',
              '<(PRODUCT_DIR)/ktx2check<(EXECUTABLE_SUFFIX)',
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
