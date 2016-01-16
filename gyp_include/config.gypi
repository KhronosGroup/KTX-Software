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
    'variables': { # level 2
      'variables': { # level 3 defines variables to be used in level 2
        'otherlibroot_dir': '../other_lib/<(OS)',
      },
      'otherlibroot_dir%': '<(otherlibroot_dir)',
      # *olib_dir are the default locations for external libraries
      #
      # None of 'copies', 'link_settings', library_dirs or libraries
      # can appear inside configurations hence use of build system
      # environment variables such as $(ConfigurationName) in some of
      # the paths below.
      #
      # An error is emitted when 'link_settings' is so used. No error
      # is emitted when 'copies' is so used.
      'droidolib_dir': '<(otherlibroot_dir)/$(BUILDTYPE)/$(TARGET_ABI)',
      # $CONFIGURATION is either Debug or Release. PLATFORM_NAME
      # is either iphoneos or iphonesimulator. Set by xcode during build.
      'iosolib_dir': '<(otherlibroot_dir)/$CONFIGURATION-$PLATFORM_NAME',
      'linuxolib_dir': '<(otherlibroot_dir)/$(BUILDTYPE)-x64',
      'macolib_dir': '<(otherlibroot_dir)/$CONFIGURATION',
      # $(ConfigurationName) is Debug or Release. $(PlatformName) is
      # Win32 or x64. $(PlatformName) is used instead of $(Platform) for
      # compatibility with VS2005.
      'winolib_dir': '<(otherlibroot_dir)/$(ConfigurationName)-$(PlatformName)',
    }, # variables level 2
    'otherlibroot_dir%': '<(otherlibroot_dir)',
    'droidolib_dir%': '<(droidolib_dir)',
    'iosolib_dir%': '<(iosolib_dir)',
    'linuxolib_dir%': '<(linuxolib_dir)',
    'macolib_dir%': '<(macolib_dir)',
    'winolib_dir%': '<(winolib_dir)',

    'gl_includes_parent_dir': '../other_include',

    # Possible values for sdl_to_use in the following: 
    #   built_dylib
    #     uses a dynamic SDL2 library you have built yourself and
    #     includes this dylib in the application bundle.
    #   installed_dylib
    #     uses a dynamic SDL2 library installed in your system,
    #     i.e it will be found somewhere on the search path.
    #   installed_framework (mac only)
    #     uses a standard SDL2 binary installed in either
    #     /Library/Frameworks or ~/Library/Frameworks.
    #   built_framework (mac only)
    #     uses a framework you have built yourself and includes
    #     this framework in the application bundle. Note this
    #     means the SDL headers will be included in the
    #     application bundle.
    #   staticlib (not Windows)
    #     links to a static library.
    'conditions': [
      ['OS == "android"', {
        'sdl_to_use': 'built_dylib', # No other choice
        'sdl2_lib_dir': '<(droidolib_dir)',
      }, # OS == "android"
      'OS == "ios"', {
        'sdl_to_use': 'staticlib', # No other choice
        'sdl2_lib_dir': '<(iosolib_dir)',
      }, # OS == "ios"
      'OS == "linux"', {
        'sdl_to_use%': 'built_dylib',

        # Location of libSDL2.a, libSDL2main.a and libSDL2_*.so
        'sdl2_lib_dir': '<(linuxolib_dir)',

        # Location of glew32.lib.
        'glew_lib_dir': '<(linuxolib_dir)',
        # Location of glew32.dll.
        'glew_dll_dir': '<(linuxolib_dir)',
      }, # OS == "linux"
      'OS == "mac"', {
        'sdl_to_use%': 'installed_framework',

        # Used when sdl_to_use == "installed_framework"
        'sdl2.framework_dir': '/Library/Frameworks',

        # Used when sdl_to_use != "installed_framework". This is the
        # location which will be searched for libSDL2.a,
        # libSDL2.dylib or SDL2.framework as appropriate.
        'sdl2_lib_dir': '<(macolib_dir)',
      }, # OS == "mac"
      'OS == "win"', {
        'sdl_to_use%': 'built_dylib',

        # Location of SDL2.lib, SDL2main.lib and SDL2.dll
        'sdl2_lib_dir': '<(winolib_dir)',

        # Location of glew32.lib.
        'glew_lib_dir': '<(winolib_dir)',
        # Location of glew32.dll.
        'glew_dll_dir': '<(winolib_dir)',
      }], # OS == "win"
    ], # conditions
  }, # variables level 1
  'includes': [
    # Pick your poison as regards an OpenGL ES emulator
    # for Windows.
    #
    # Mali, Adreno & ANGLE do not support OpenGL ES 1.X so
    # MSVS projects generated for these do not include
    # es1loadtests.
    #
    # Adreno has bugs. It crashes on eglMakeCurrent when
    # context and suface are EGL_NO_CONTEXT and EGL_NO_SURFACE
    # respectively. Prior to that a message box appears with the
    # message "could not load from Adreno device driver: eglGetError"
    # when eglGetDisplay(EGL_DEFAULT_DISPLAY) is called.
    #
    # With Mali only one of 64- or 32-bit can be built on any given
    # system. See maliemu.gypi for the full explanation.
    #
    # PVR has some implementation bugs that cause some
    # of the loadtests to misbehave. It will not load LUMINANCE8_OES
    # textures even though it advertises support for
    # GL_OES_require_internal_formats causing a pop-up message box
    # on that test.  It fails to raise an INVALID_ENUM error when
    # attempting to load ETC2 formats in the OpenGL ES 1.1 emulator
    # which causes all the ETC2 tests to fail to display so you see
    # a yellow quad instead.
    #
    # Nevertheless PowerVR has been chosen as default as we can build
    # and run for both Win32 and x64 platforms.

    #'adrenoemu.gypi',
    #'angle.gypi',
    #'maliemu.gypi',
    'pvremu.gypi',
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
