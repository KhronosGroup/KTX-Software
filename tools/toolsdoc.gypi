##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project files for building tools documentation.
#
{
  'conditions': [
    # Can only build doc on desktops.
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      'targets': [
        {
          'target_name': 'tools.doc',
          'type': 'none',
          'variables': { # level 1
            'variables': { # level 2
              'output_dir': '../build/docs',
            },
            'output_dir': '<(output_dir)',
            'doxyConfig': 'ktxtools.doxy',
            'timestamp': '<(output_dir)/.ktxtools_gentimestamp',
          },
          # It is not possible to chain commands in an action with
          # && because the generators will quote such strings.
          # Instead we use an external script.
          'actions': [
            {
              'action_name': 'buildToolsDoc',
              'message': 'Generating tools documentation with Doxygen',
              'inputs': [
                '../<(doxyConfig)',
                '../runDoxygen',
                'toktx/toktx.cpp',
              ],
              'outputs': [
                '<(output_dir)/html/ktxtools',
                '<(output_dir)/man/ktxtools/man1/toktx.1',
                '<(timestamp)',
              ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that directory will be removed
              # from paths displayed in the documentation. That is also
              # the directory where the .doxy and .gyp files are stored.
              #
              # See ../lib/libktxdoc.gypi for further comments.
              'msvs_cygwin_shell': 1,
              'action': [
                './runDoxygen', '-t', '<(timestamp)', '<(doxyConfig)',
              ],
            }, # buildToolsDoc action
          ], # actions
        }, # tools.doc
      ], # targets
    }], # 'OS == "linux" or 'OS == "mac" or OS == "win"'
  ] # conditions for conditional targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
