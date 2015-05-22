##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Configuration variables specifying file locations
#        and other choices.
#
{
  # Names need _dir (lowercase) suffix to be relativized. Variable
  # expansions are not relativized: use _dir to ensure it happens
  # now.
  'variables': { # level 1
    'variables': { # level 2 defines variables to be used in level 1
      'otherlibroot_dir': '../other_lib/<(OS)',
    },
    'otherlibroot_dir%': '<(otherlibroot_dir)',
    'gl_includes_parent_dir': '../other_include',
    'gles1_lib_dir': 'nowhere',
    'gles1_bin_dir': 'nowhere',
    'gles2_lib_dir': 'nowhere',
    'gles2_bin_dir': 'nowhere',
    'gles3_lib_dir': 'nowhere',
    'gles3_bin_dir': 'nowhere',

    # None of 'copies', 'link_settings', library_dirs or libraries
    # can appear inside configurations hence use of build system
    # environment variables such as $(ConfigurationName).
    #
    # An error is emitted when 'link_settings' is so used. No error
    # is emitted when 'copies' is so used.
    'conditions': [
      ['OS == "ios"', {
        # Location of other libraries for iOS. Used to find libSDL2.a
        # $CONFIGURATION is either Debug or Release. PLATFORM_NAME
        # is either iphoneos or iphonesimulator. Both are set by
        # xcode during build.
        'iosolib_dir': '<(otherlibroot_dir)/$CONFIGURATION-$PLATFORM_NAME',
      }, 'OS == "mac"', {
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
        # SDL2.framework as appropriate. See iOS above for info
        # about $CONFIGURATION.
        'macolib_dir': '<(otherlibroot_dir)/$CONFIGURATION',
      }, 'OS == "win"', {
        # Location of other libraries for Windows. Used to find
        # libs and dlls for SDL2. $(PlatformName) is used instead
        # of $(Platform) for compatibility with VS2005.
        'winolib_dir': '<(otherlibroot_dir)/$(ConfigurationName)-$(PlatformName)',
      }],
    ] # conditions
  }
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
