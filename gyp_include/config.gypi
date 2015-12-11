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
      'droidolib_dir': '<(otherlibroot_dir)$(BUILDTYPE)/$(TARGET_ABI)',
      # $CONFIGURATION is either Debug or Release. PLATFORM_NAME
      # is either iphoneos or iphonesimulator. Set by xcode during build.
      'iosolib_dir': '<(otherlibroot_dir)/$CONFIGURATION-$PLATFORM_NAME',
      'macolib_dir': '<(otherlibroot_dir)/$CONFIGURATION',
      # $(ConfigurationName) is Debug or Release. $(PlatformName) is
      # Win32 or x64. $(PlatformName) is used instead of $(Platform) for
      # compatibility with VS2005.
      'winolib_dir': '<(otherlibroot_dir)/$(ConfigurationName)-$(PlatformName)',

    }, # variables level 2
    'otherlibroot_dir%': '<(otherlibroot_dir)',
    'droidolib_dir%': '<(droidolib_dir)',
    'iosolib_dir%': '<(iosolib_dir)',
    'macolib_dir%': '<(macolib_dir)',
    'winolib_dir%': '<(winolib_dir)',

    'gl_includes_parent_dir': '../other_include',

    'conditions': [
      ['OS == "ios"', {
        # On iOS libSDL2.a is statically linked and is found in <(iosolib_dir).
      },
      'OS == "mac"', {
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
        'sdl2_lib_dir': '<(macolib_dir)',
      },
      'OS == "win"', {
        # An empty string in a DLL location means an installed dll
        # is expected to be found at run time so no dll will be
        # copied to <(PRODUCT_DIR).
        # Location of SDL2.lib and SDL2main.lib
        'sdl2_lib_dir': '<(winolib_dir)',
        # Location of SDL2.dll
        'sdl2_dll_dir': '<(winolib_dir)',

        # Location of glew32.lib.
        'glew_lib_dir': '<(winolib_dir)',
        # Location of glew32.dll.
        'glew_dll_dir': '<(winolib_dir)',

      }],
    ] # conditions
  },
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
    # PVR has some implementation bugs that cause some
    # of the loadtests to misbehave.
    #
    # Mali has been chosen as default as it will cause the least
    # issues for building and running on both Win32 and x64
    # platforms.

    #'adrenoemu.gypi',
    #'angle.gypi',
    'maliemu.gypi',
    #'pvremu.gypi',
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
