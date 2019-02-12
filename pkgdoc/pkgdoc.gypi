##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project files for building KTX package documentation.
#
{
  'conditions': [
    # Can only build doc on desktops.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      'targets': [
        {
          'target_name': 'ktxpkg.doc',
          'type': 'none',
          'dependencies': [
            'libktx.gyp:libktx.doc',
            'ktxtools.gyp:toktx.doc',
          ],
          'variables': { # level 1
            'variables': { # level 2
              'output_dir': '../build/docs',
            },
            'output_dir': '<(output_dir)',
            'doxyConfig': 'ktxpkg.doxy',
            'timestamp': '<(output_dir)/.package_gentimestamp',
          },
          # It is not possible to chain commands in an action with
          # && because the generators will quote such strings.
          # Instead we use an external script.
          'actions': [
            {
              'action_name': 'buildPackageDoc',
              'message': 'Generating package documentation with Doxygen',
              'inputs': [
                '../<(doxyConfig)',
                '../LICENSE.md',
                '../runDoxygen',
                '../TODO.md',
                'mainpage.md',
              ],
              # See ../../lib/libktx.gypi for comment about why only
              # timestamp is in this list.
              'outputs': [ '<(timestamp)' ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that directory will be removed
              # from paths displayed in the documentation. That is also
              # the directory where the .doxy and .gyp files are stored.
              #
              # See ../lib/libktxdoc.gypi for further comments.
              'msvs_cygwin_shell': 1,
              'action': [
                './runDoxygen',
                '-t', '<(timestamp)',
                '-o', '<(output_dir)/html',
                '<(doxyConfig)',
              ],
            }, # buildToolsDoc action
          ], # actions
        }, # ktxpkg.doc
      ], # targets
    }], # 'OS == "linux" or 'OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
