##
# @internal
# @copyright © 2015, Mark Callow. For license see LICENSE.md.
#
# @brief Default settings for building KTX library, tools and tests.
#
{
  'variables': { # level 1
    'executable': 'executable',
    'emit_vs_x64_configs': 'false',
    'emit_vs_win32_configs': 'false',
    'emit_emscripten_configs': 'false',

    'conditions': [
      # TODO Emscripten support not yet correct or complete. DO NOT
      # TRY TO USE.
      # Emscripten "vs-tool" VS integration only supports certain MSVS versions;
#      ['GENERATOR=="make" or GENERATOR=="cmake" or (OS=="win" and GENERATOR=="msvs" and MSVS_VERSION=="2010")', {
#        'emit_emscripten_configs': 'true',
#      }, {
#        'emit_emscripten_configs': 'false',
#      }],
      ['OS == "android"', {
        'executable': 'shared_library',
      }],
      ['OS == "win" and GENERATOR == "msvs"', {
        'emit_vs_win32_configs': 'true',
        'conditions': [
          # Don't generate x64 configs in MSVS Express Edition projects.
          # Note: 2012e and 2013e support x64.
          ['MSVS_VERSION != "2010e" and MSVS_VERSION != "2008e" and MSVS_VERSION != "2005e"', {
            'emit_vs_x64_configs': 'true'
          }],
        ],
      }], # OS == "win" and GENERATOR == "msvs"
    ], # conditions
  }, # variables
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
    # These have to be here. If in target_defaults', Xcode 8 will
    # warn that the project settings are not the recommended settings
    # and suggest it turns all these on. However, if they are set in
    # target_defaults, the warnings *will* be turned on. Xcode bug?
    'CLANG_WARN_BOOL_CONVERSION': 'YES',
    'CLANG_WARN_CONSTANT_CONVERSION': 'YES',
    'CLANG_WARN_EMPTY_BODY': 'YES',
    'CLANG_WARN_ENUM_CONVERSION': 'YES',
    'CLANG_WARN_INFINITE_RECURSION': 'YES',
    'CLANG_WARN_INT_CONVERSION': 'YES',
    'CLANG_WARN_SUSPICIOUS_MOVE': 'YES',
    'CLANG_WARN_UNREACHABLE_CODE': 'YES',
    'CLANG_WARN__DUPLICATE_METHOD_MATCH': 'YES',
    'ENABLE_STRICT_OBJC_MSGSEND': 'YES',
    'GCC_NO_COMMON_BLOCKS': 'YES',
    'GCC_WARN_64_TO_32_BIT_CONVERSION': 'YES',
    'GCC_WARN_ABOUT_RETURN_TYPE': 'YES',
    'GCC_WARN_PEDANTIC': 'YES',
    'GCC_WARN_UNDECLARED_SELECTOR': 'YES',
    'GCC_WARN_UNINITIALIZED_AUTOS': 'YES',
    'GCC_WARN_UNUSED_FUNCTION': 'YES',
    'GCC_WARN_UNUSED_VARIABLE': 'YES',
  }, # xcode_settings
  # This has to be here. If in target_defaults' Debug config
  # Xcode 7+ will warn that this recommended value is not set.
  'configurations': {
    'Debug': {
      'xcode_settings': {
        'ENABLE_TESTABILITY': 'YES',
      },
    },
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
      'COPY_PHASE_STRIP': 'NO',
      'conditions': [
        ['OS == "ios"', {
          # 1 = iPhone/iPod Touch; 2 = iPad
          'TARGETED_DEVICE_FAMILY': '1,2',
          'CODE_SIGN_IDENTITY': 'iPhone Developer',
          'PROVISIONING_PROFILE': '',
          'IPHONEOS_DEPLOYMENT_TARGET': '8.0',
        }, 'OS == "mac"', {
          'MACOSX_DEPLOYMENT_TARGET': '10.9',
          'CODE_SIGN_IDENTITY': 'Mac Developer',
          'PROVISIONING_PROFILE': '',
          'COMBINE_HIDPI_IMAGES': 'YES',
        }],
      ],
      'target_conditions': [
        ['_type == "executable"', {
            # Don't add a default value because this variable gets exported
            # as is to CMake and ${PRODUCT_NAME:-identifier} is invalid
            # syntax.
          'PRODUCT_BUNDLE_IDENTIFIER': 'org.khronos.${PRODUCT_NAME}',
        }],
      ],
    },
    'configurations': {
      'Debug': {
        'target_conditions': [
          ['OS == "linux" and _type == "shared_library"', {
            'cflags': [ '-fPIC' ],
          }], # OS == "linux" and library == "shared_library"
        ],

        'cflags': [ '-Og', '-g' ],
        'defines': [ 'DEBUG', '_DEBUG', ],
        'ldflags': [ '-g' ],
        'msvs_configuration_platform': 'Win32',
        'msvs_settings': {
          'VCCLCompilerTool': {
            # EditAndContinue
            'DebugInformationFormat': 4,
            'Optimization': 0,
            # Use MultiThreadedDebugDLL (/MDd) to get extra checking.
            # Default in msvs is probably version dependent. In
            # VS2010 it is MultiThreaded (/MT).
            'RuntimeLibrary': 3,
          },
          # Changing OutputFile causes an MSB8012 warning from MSBuild
          # because GYP only ensures that TargetPath matches the
          # OutputFile. TargetName must match as well.
          'VCLinkerTool': {
            'GenerateDebugInformation': 'true',
            # Uncomment the following if using a non-debug
            # SDLmain.lib. This will be compiled /MD making msvcrt.lib
            # a default link lib. This removes it from default list to
            # quiet the LNK4098 warning that notifies of the conflict
            # with /MDd above.
            # SDLmain.lib compiled /MDd.
            #'IgnoreDefaultLibraryNames': 'msvcrt.lib'

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
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES',
          'GCC_OPTIMIZATION_LEVEL': 0,
          'target_conditions': [
            ['_type == "executable"', {
              'STRIP_INSTALLED_PRODUCT': 'NO',
            }],
          ],
        },
      }, # Debug configuration
      'Release': {
        'target_conditions': [
          ['OS == "linux" and _type == "shared_library"', {
            'cflags': [ '-fPIC' ],
          }], # OS == "linux" and library == "shared_library"
        ],
        'cflags': [ '-O3' ],
        'defines': [ 'NDEBUG' ],
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
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          'GCC_OPTIMIZATION_LEVEL': 3,
           'target_conditions': [
            ['_type == "executable"', {
              'STRIP_INSTALLED_PRODUCT': 'YES',
            }],
          ],
        },
      }, # Release configuration
      # Conditionally add some configs
      'conditions': [  # or 'conditions!':
        ['emit_vs_win32_configs=="true"', {
          # The part after '_' must match the msvs_configuration_platform
          'Debug_Win32': {
            'inherit_from': ['Debug'],
            'msvs_configuration_platform': 'Win32',
            'msvs_settings': {
              'VCLinkerTool': {
                # Disable because it prevents E&C and is unnecessary in
                # debug configurations.
                'AdditionalOptions': '/SAFESEH:NO',
              },
            },
          },
          # No need for Release_Win32 as the standard Release settings apply and
          # we can include msvs_configuration_platform there without problem.
        }], # emit_vs_win32_configs
        ['emit_vs_x64_configs=="true"', {
          'Debug_x64': {
            'inherit_from': ['Debug'],
            'msvs_configuration_platform': 'x64',
            'msvs_settings': {
              'VCCLCompilerTool': {
                # Program Database. E&C not available in 64-bit
                'DebugInformationFormat': 3,
              },
            },
          }, # Debug_x64
          'Release_x64': {
            'inherit_from': ['Release'],
            'msvs_configuration_platform': 'x64',
            'msvs_settings': {
              'VCCLCompilerTool': {
                # Program Database. E&C not available in 64-bit
                'DebugInformationFormat': 3,
              },
            },
          }, # Release_x64
        }], # emit_vs_x64_configs
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
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
