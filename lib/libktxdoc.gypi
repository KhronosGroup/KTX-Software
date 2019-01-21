##
# @internal
# @copyright © 2019, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project to build KTX library for OpenGL.
#
{
  'includes': [
      'sources.gypi',
  ],
  'conditions': [
    ['OS == "linux" or OS == "mac" or OS == "win"', {
      # Can only build doc on desktops
      'targets': [
        {
          'target_name': 'libktx.doc',
          'type': 'none',
          'variables': {
            'variables': { # level 2
              'output_dir': '../build/docs',
            },
            'output_dir': '<(output_dir)',
            'doxyConfig': 'libktx.doxy',
            'timestamp': '<(output_dir)/.libktx_gentimestamp',
          },
          'actions': [
            {
              'action_name': 'buildDoc',
              'message': 'Generating documentation with Doxygen',
              'inputs': [
                '../<(doxyConfig)',
                '../runDoxygen',
                '../lib/mainpage.md',
                '../LICENSE.md',
                '../TODO.md',
                '<@(sources)',
                '<@(vksource_files)',
              ],
              'outputs': [
                '<(output_dir)/html/libktx',
                '<(output_dir)/man/libktx',
              ],
              # doxygen must be run in the top-level project directory
              # so that ancestors of that directory will be removed
              # from paths displayed in the documentation. That is
              # the directory where the .doxy and .gyp files are stored.
              #
              # With Xcode, the current directory during project
              # build is one we need so we're good to go. However
              # we need to spawn another shell with -l so the
              # startup (.bashrc, etc) files will be read.
              #
              # With MSVS the working directory will be the
              # location of the vcxproj file. However when the
              # action is using bash ('msvs_cygwin_shell': '1',
              # the default, is set) no path relativization is
              # performed on any command arguments. If forced, by
              # using variable names such as '*_dir', paths will be
              # made relative to the location of the .gyp file.
              #
              # A setup_env.bat file is run before the command.
              # Apparently that .bat file is expected to be in the
              # same location as the .gyp and to cd to
              # its directory. That makes things work.
              #
              # Note that the same setup_env.bat is run by
              # rules but rules relativize paths to the vcxproj
              # location so cd to the .gyp home breaks rules.
              # Therefore in rules set 'msvs_cygwin_shell': '0.
              #
              # If using cmd.exe ('msvs_cygwin_shell': '0')
              # the MSVS generator will relativize to the vcxproj
              # location *all* command arguments, that do not look
              # like options.
              #
              # With `make`, cmake, etc, like Xcode,  the current
              # directory during project build is the one we need.
              'msvs_cygwin_shell': 1,
              'action': [
                './runDoxygen', '-t', '<(timestamp)', '<(doxyConfig)',
              ],
            }, # buildDoc action
          ], # actions
        }, # libktx.doc
      ], # targets
    }], # 'OS == "linux" or OS == "mac" or OS == "win"'
  ], # conditions
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
