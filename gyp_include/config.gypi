##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Configuration variables specifying file locations
#        and other choices.
#
{
  # Names need _dir (lowercase) suffix to be relativized. Variable
  # expansions are not relativized so must ensure it happens now.
  'variables': {
    'gl_includes_parent_dir': '../other_include',
    'gles1_lib_dir': 'nowhere',
    'gles1_bin_dir': 'nowhere',
    'gles2_lib_dir': 'nowhere',
    'gles2_bin_dir': 'nowhere',
    'gles3_lib_dir': 'nowhere',
    'gles3_bin_dir': 'nowhere',

    'conditions': [
      ['OS == "mac"', {
        'sdl_to_use%': 'installed_framework',
        # Possible values for the preceding: 
        #   installed_framework
        #     uses a standard SDL2 binary installed in either
        #     /Library/Frameworks or ~/Library/Frameworks.
        #   built_framework
        #     uses a framework you have built yourself and includes
        #     this framework in the application bundle. Note this
        #     means the SDL headers will be included in the
        #     application bundle.
        #   dylib
        #     uses a dynamic library you have built yourself and
        #     includes this dylib in the application bundle.

        # Used when sdl_to_use == "installed_framework"
        'sdl2.framework_dir': '/Library/Frameworks',

        # Used when sdl_to_use != "installed_framework". This is the
        # location which will be searched for libSDL2.dylib or
        # SDL2.framework as appropriate. $CONFIGURATION is either
        # Debug or Release.
        'macolib_dir': '<(otherlibroot_dir)/$CONFIGURATION',
      }],
    ] # conditions
  }
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
