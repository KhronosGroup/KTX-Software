##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Master for generating project files for building KTX library,
#        tools and tests.
#
# The command for generating msvs project files from this .gyp file is:
#   gyp -f msvs -G msvs_version=<vername> --generator-output=build/msvs --depth=. ktx.gyp
# where <vername> is one of 2005, 2005e, 2008, 2008e, 2010, 2010e, 2012,
# 2012e, 2013 or 2013e. The suffix 'e' generates a project for the express edition.
#
# Other currently available formats are cmake, make, ninja and xcode.
#
{

  # Is there a way to set --generator-output=DIR and --format=FORMATS a.k.a
  # -f FORMATS from within the GYP file?  I'd like people to just be able
  # to run gyp on this file.

 'variables': { # level 1
    'variables': { # level 2 so GL_VERSION can be used in level 1
      'otherlibroot_dir': 'other_lib/<(OS)',
# Probably not needed.
#      'conditions': [
#        ['OS == "win"', {
#          'GL_PROFILE%': 'gl',
#          'GL_VERSION%': '3.3',
#        }, 'OS == "mac"', {
#          'GL_PROFILE%': 'gl',
#          'GL_VERSION%': '3.3',
#        }, 'OS == "ios"', {
#          'GL_PROFILE%': 'es',
#          'GL_VERSION%': '3.0',
#        }, 'OS == "android"', {
#          'GL_PROFILE%': 'es',
#          'GL_VERSION%': '3.0',
#          'executable': 'shared_library',
#        }],
#      ],
    },
#    'GL_PROFILE%': '<(GL_PROFILE)',
#    'GL_VERSION%': '<(GL_VERSION)',
#    'otherlibroot_dir': 'other_lib/<(OS)',
    'executable': 'executable',
    'emit_x64_configs': 'false',

#    'conditions': [
      # TODO Emscripten support not yet correct or complete. DO NOT
      # TRY TO USE.
      # Emscripten "vs-tool" VS integration only supports certain MSVS versions;
#      ['GL_PROFILE!="gl" and (GENERATOR=="make" or GENERATOR=="cmake" or (OS=="win" and GENERATOR=="msvs" and MSVS_VERSION=="2010"))', {
#        'emit_emscripten_configs': 'true',
#      }, {
        'emit_emscripten_configs': 'false',
#      }],

    'conditions': [
      ['OS == "android"', {
        'executable': 'shared_library',
      }],
      # Don't generate x64 configs in MSVS Express Edition projects.
      # Note: 2012e and 2013e support x64.
      ['OS == "win" and (GENERATOR!="msvs" or (GENERATOR=="msvs" and MSVS_VERSION!="2010e" and MSVS_VERSION!="2008e" and MSVS_VERSION!="2005e"))', {
        'emit_x64_configs': 'true'
      }],
    ],
  },
  'make_global_settings': [
    ['AR.emscripten', 'emar'],

    ['CC.emscripten', 'emcc'],
    ['CXX.emscripten', 'emcc'],

    ['LD.emscripten', 'emcc'],
    ['LINK.emscripten', 'emcc'],
  ],
  'xcode_settings': {
    # Don't add anything new to this block unless you really need it!
    # This block adds *project-wide* configuration settings to each
    # project file. Specify your custom xcode settings in
    # 'target_defaults' or add them to targets.
    'ONLY_ACTIVE_ARCH': 'YES',
    'conditions': [
      ['OS == "ios"', {
        'SDKROOT': 'iphoneos',
      }],
      ['OS == "mac"', {
        'SDKROOT': 'macosx',
      }],
    ],
  },
  'target_defaults': {
    # Sadly variable values cannot be dictionaries. If they could, it
    # would save a lot of duplication such as between configurations.
    'variables': {
      'conditions': [
        ['OS == "win"', {
          'emscripten_run_action': 'cmd.exe /c start $(TargetPath)',
        }, {
        'emscripten_run_action': '',
        }],
      ],
    },
    'msvs_configuration_attributes': {
      # Augment these with $(PlatformName) since we generate
      # multi-platform projects.
      'OutputDirectory': '$(SolutionDir)$(PlatformName)/$(ConfigurationName)',
      # Must have $(ProjectName) here to avoid conflicts between the
      # various projects' .tlog files in MSBuild/VS2010 that cause,
      # among other problems, all projects to be cleaned when
      # Project Only -> Clean is selected.
      'IntermediateDirectory': '$(PlatformName)/$(ConfigurationName)/obj/$(ProjectName)'
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
        # GYP defaults this to level 1 !!!
        'WarningLevel': 3,
      },
    },
    'xcode_settings': {
      'ARCHS': '$(ARCHS_STANDARD_32_64_BIT)',
      'conditions': [
        ['OS == "ios"', {
          # 1 = iPhone/iPod Touch; 2 = iPad
          'TARGETED_DEVICE_FAMILY': '1,2',
          'CODE_SIGN_IDENTITY': 'iPhone Developer',
          'IPHONEOS_DEPLOYMENT_TARGET': '7.0',
        }, 'OS == "mac"', {
          # If need GL 4.1 or ARB_ES2_compatibility, bump to 10.9
          'MACOSX_DEPLOYMENT_TARGET': '10.5',
          'COMBINE_HIDPI_IMAGES': 'YES',
        }],
      ],
    },
    'configurations': {
      'Debug': {
        'cflags': [ '-O0' ],
        'defines': [
          'DEBUG', '_DEBUG',
        ],
        'msvs_configuration_platform': 'Win32',
        'msvs_settings': {
          'VCCLCompilerTool': {
            # EditAndContinue
            'DebugInformationFormat': 4,
            'Optimization': 0,
            # Use MultiThreadedDebugDLL (/MDd) to get extra checking.
            # Default is MultiThreaded (/MD).
            'RuntimeLibrary': 3,
          },
          # Changing OutputFile causes an MSB8012 warning from MSBuild
          # because GYP only ensures that TargetPath matches the
          # OutputFile. TargetName must match as well.
          'VCLinkerTool': {
            'GenerateDebugInformation': 'true',
            # SDLmain.lib is compiled /MD making msvcrt.lib a default
            # link lib. Remove it from default list to quiet the
            # LNK4098 warning that notifies of the conflict with /MDd
            # above.  Alternative fix is to provide a debug version of
            # SDLmain.lib compiled /MDd.
            'IgnoreDefaultLibraryNames': 'msvcrt.lib'
            #'OutputFile': '$(OutDir)$(ProjectName)_g$(TargetExt)'
          },
          #'VCLibrarianTool': {
          #  'OutputFile': '$(OutDir)$(ProjectName)_g$(TargetExt)'
          #},
          'target_conditions': [
            ['_type == "executable"', {
              'VCLinkerTool': {
                'LinkIncremental': 2,
              },
            }],
          ],
        },
        'xcode_settings':  {
          'GCC_OPTIMIZATION_LEVEL': 0,
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES',
          'target_conditions': [
            ['_type == "executable"', {
              'STRIP_INSTALLED_PRODUCT': 'NO',
            }],
          ],
        },
      }, # Debug configuration
      'Release': {
        'cflags': [ '-O3' ],
        'defines': [
          'NDEBUG',
        ],
        'msvs_configuration_platform': 'Win32',
        'msvs_settings': {
          'VCCLCompilerTool': {
            'Optimization': 3,
            # Use MultiThreadDLL (msvcrt) to match the way SDLmain.lib
            # is compiled. Default is MultiThread (libcmt) which causes
            # link errors due to symbols defined in both msvcrt and
            # libcmt.
            'RuntimeLibrary': 2,
          },
        },
        'xcode_settings':  {
          'GCC_OPTIMIZATION_LEVEL': 3,
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          'target_conditions': [
            ['_type == "executable"', {
              'STRIP_INSTALLED_PRODUCT': 'YES',
            }],
          ],
        },
      }, # Release configuration
      # Conditionally add some configs
      'conditions': [  # or 'conditions!':
        ['emit_x64_configs=="true"', { # and Linux?
          # The part after '_' must match the msvs_configuration_platform
          'Debug_x64': {
            'inherit_from': ['Debug'],
            'msvs_configuration_platform': 'x64',
               'msvs_settings': {
              'VCCLCompilerTool': {
                # Program Database. E&C not available in 64-bit
                'DebugInformationFormat': 3,
              },
            },
          },
          'Release_x64': {
            'inherit_from': ['Release'],
            'msvs_configuration_platform': 'x64',
               'msvs_settings': {
              'VCCLCompilerTool': {
                # Program Database. E&C not available in 64-bit
                'DebugInformationFormat': 3,
              },
            },
          },
        }],
        ['emit_emscripten_configs=="true"', {
          # The part after '_' must match the msvs_configuration_platform
          'Debug_Emscripten': {
            'inherit_from': ['Debug'],
            'ldflags': '-O0',
            'msvs_configuration_platform': 'Emscripten',
            'msvs_settings': {
              'VCCLCompilerTool': {
                'OptimizationLevel': 3, # -O0
              },
              'VCLinkerTool': {
                'LinkerOptimizationLevel': 1, # -O0
              },
            },
            # Not working. Investigate later.
            # Variables are not propagated from configurations?
            'OS': 'html5',
            'toolset': 'emscripten',
          }, # Debug_Emscripten
          'Release_Emscripten': {
            'inherit_from': ['Release'],
            'ldflags': '-O3',
            'msvs_configuration_platform': 'Emscripten',
            'msvs_settings': {
              'VCCLCompilerTool': {
                'OptimizationLevel': 6, # -O3
              },
              'VCLinkerTool': {
                'LinkerOptimizationLevel': 4, # -O3
              },
            },
            # ditto
            'OS': 'html5',
            'toolset': 'emscripten',
          }, # Release_Emscripten
        }],
      ], # conditional configurations
    }, # configurations
    # Not having any effect. Fortunately vs-tool sets these so
    # not needed unless want to change the default.
#    'target_conditions': [
#      ['_type=="executable" and emit_emscripten_configs=="true"', {
#        'configurations': {
#          'Debug_Emscripten': {
#            'run_as': {
#              'action': '<(emscripten_run_action)',
#              'working_directory': '$(TargetDir)',
#            },
#          },
#          'Release_Emscripten': {
#            'run_as': {
#              'action': '<(emscripten_run_action)',
#              'working_directory': '$(TargetDir)',
#            },
#          },
#        }
#      }],
#    ], # target_conditions
  }, # target_defaults
  # Caution: variables set here will override any variables set above.
  'includes': [
     'lib/libktx_gl.gypi',
     'lib/libktx_es3.gypi',
     'tests/tests.gypi',
     #'tools/tools.gypi',
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
