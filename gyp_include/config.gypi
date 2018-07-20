##
# @internal
# @copyright c 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Set variables specifying locations from which to link
#        3rd party libraries and which variants to use.
#
# Edit this as desired then regenerate the projects with gyp.
#
{
  'variables': { # level 1
    'variables': { # level 2 defines variables to be used in level 1
      'variables': { # level 3 ditto
        # Build system environment variable names.
        #
        # There should be no need to change these. They are defined here for
        # clarity; this the only gypi where there are used. Defining these
        # variables avoids repeating the tests for all OSes.
        #
        # None of 'copies', 'link_settings', library_dirs or libraries
        # can appear inside a configuration dict hence must use build system
        # environment variables such as $(ConfigurationName).
        #
        # An error is emitted when 'link_settings' is so used. No error
        # is emitted when 'copies' is so used.
        'conditions': [
          ['GENERATOR == "cmake"', {
            'gen_config_var': '${configuration}',
            'gen_platform_var': 'x64', # Default to x64
          }, 'GENERATOR == "make"', {
            'gen_config_var': '$(BUILDTYPE)',
            'gen_platform_var': 'x64', # Default to x64
          }, 'GENERATOR == "msvs"', {
            # $(ConfigurationName) is Debug or Release. $(PlatformName) is
            # Win32 or x64. $(PlatformName) is used instead of $(Platform) for
            # compatibility with VS2005.
            'gen_config_var': '$(ConfigurationName)',
            'gen_platform_var': '$(PlatformName)',
          }, 'GENERATOR == "xcode"', {
            # CONFIGURATION is either Debug or Release. PLATFORM_NAME
            # is either iphoneos or iphonesimulator. Set by xcode during build.
            # Don't be tempted to put () around these names. There appears to
            # be a gyp bug when such a variable is the immediate parent of
            # the source file of a copies operation. Fortunately Xcode
            # recognizes the names without ().
            'gen_config_var': '$CONFIGURATION',
            'gen_platform_var': '$PLATFORM_NAME',
          }],
        ],
        # Base location for 3rd party libraries.
        'otherlibroot_dir': '../other_lib/<(OS)',
      }, # variables level 3
      # Copy variables out one scope.
      'gen_config_var%': '<(gen_config_var)',
      'gen_platform_var%': '<(gen_platform_var)',
      'otherlibroot_dir%': '<(otherlibroot_dir)',

      # Names need _dir (lowercase) suffix to be relativized. Variable
      # expansions are not relativized: use _dir to ensure it happens
      # now.
      #
      # *olib_dir are the OS-specific base locations for 3rd party libraries
      #
      'droidolib_dir': '<(otherlibroot_dir)/<(gen_config_var)/$(TARGET_ABI)',
      'iosolib_dir': '<(otherlibroot_dir)/<(gen_config_var)-<(gen_platform_var)',
      'iosolibr_dir': '<(otherlibroot_dir)/Release-<(gen_platform_var)',
      'linuxolib_dir': '<(otherlibroot_dir)/<(gen_config_var)-<(gen_platform_var)',
      'macolib_dir': '<(otherlibroot_dir)/<(gen_config_var)',
      'winolib_dir': '<(otherlibroot_dir)/<(gen_config_var)-<(gen_platform_var)',
      'winolibr_dir': '<(otherlibroot_dir)/Release-$(PlatformName)',
    }, # variables level 2
    # Copy variables out one scope.
    'otherlibroot_dir%': '<(otherlibroot_dir)',
    'droidolib_dir%': '<(droidolib_dir)',
    'iosolib_dir%': '<(iosolib_dir)',
    'iosolibr_dir%': '<(iosolibr_dir)',
    'linuxolib_dir%': '<(linuxolib_dir)',
    'macolib_dir%': '<(macolib_dir)',
    'winolib_dir%': '<(winolib_dir)',
    'winolibr_dir%': '<(winolibr_dir)',

    # Directory containing EGL, GL{,ES}*, KHR, etc. include directories.
    'gl_includes_parent_dir': '../other_include',

    # Default platform for Windows builds. Multiple platform solution
    # & project files are no longer generated. Limitations in GYP
    # prevent having per-configuration values for link_settings. The
    # Vulkan SDK unfortunately separates its 32- & 64-bit packages
    # into directories, {Lib,Bin}{,32}, whose names differ from
    # the values of the MSVS $(Platform{,Name}) macros and can't be
    # mapped using other MSVS macros because the 64-bit directories
    # have no suffix.
    'WIN_PLATFORM%': 'x64',

    # Possible values for 'sdl_to_use' in the following:
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
    #   static_lib (not Windows)
    #     links to a static library.
    #
    # Possible values for 'library':
    #   static_lib
    #   shared_lib
    'conditions': [
      ['OS == "android"', {
        'sdl_to_use': 'built_dylib', # No other choice
        'sdl2_lib_dir': '<(droidolib_dir)',
        'library': 'static_library',
      }, # OS == "android"
      'OS == "ios"', {
        'sdl_to_use': 'static_lib', # No other choice
        'sdl2_lib_dir': '<(iosolib_dir)',
        'library': 'static_library',
      }, # OS == "ios"
      'OS == "linux"', {
        'sdl_to_use%': 'built_dylib',
        # Location of libSDL2.a, libSDL2main.a and libSDL2_*.so
        'sdl2_lib_dir': '<(linuxolib_dir)',
        # libktx type.
        'library': 'shared_library',
        # Location of glew32.lib.
        'glew_lib_dir': '<(linuxolib_dir)',
        # Location of glew32.dll.
        'glew_dll_dir': '<(linuxolib_dir)',
      }, # OS == "linux"
      'OS == "mac"', {
        'sdl_to_use%': 'built_dylib',
        # Used when sdl_to_use == "installed_framework"
        'sdl2.framework_dir': '/Library/Frameworks',
        # Used when sdl_to_use != "installed_framework". This is the
        # location which will be searched for libSDL2.a,
        # libSDL2.dylib or SDL2.framework as appropriate.
        'sdl2_lib_dir': '<(macolib_dir)',
        # libktx type.
        'library': 'shared_library',
      }, # OS == "mac"
      'OS == "win"', {
        # Location of glew32.lib.
        'glew_lib_dir': '<(winolibr_dir)',
        # Location of glew32.dll.
        'glew_dll_dir': '<(winolibr_dir)',
        'sdl_to_use%': 'built_dylib',
        # Location of SDL2.lib, SDL2main.lib and SDL2.dll
        'sdl2_lib_dir': '<(winolib_dir)',
        # libktx type. Must be static as exports currently not defined.
        'library': 'static_library',
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
    # PVR v2017_R1 runs almost correctly. Previous bugs have been 
    # fixed but there is a new one in the OpenGL ES 1 emulator. It
    # raises a GL_INVALID_ENUM error at line 157 in sample_02_textured.c,
    # `glEnableClientState(GL_TEX_COORD_ARRAY);` the first, and only
    # the first, time a cube test is run. In debug mode this
    # triggers the later assert at line 211, in atRun_02_cube due to
    # the uncollected error. In release mode an error message box is
    # raised during load of the *next* test. After dismissing this,
    # subsequent invocations of the cube test work and you can loop
    # through the tests from the beginning again.
    #
    # As this is a relatively minor bug, provided you run in release
    # mode, the PVR emulator has been chosen as default as we can build
    # and run for both Win32 and x64 platform

    #'adrenoemu.gypi',
    #'angle.gypi',
    #'maliemu.gypi',
    'pvremu.gypi',
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
