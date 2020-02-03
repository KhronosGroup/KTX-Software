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
      # Emscripten "vs-tool" VS integration only supports MSVS 2010;
#      ['GENERATOR=="make" or GENERATOR=="cmake" or (OS=="win" and GENERATOR=="msvs" and MSVS_VERSION=="2010")', {
#        'emit_emscripten_configs': 'true',
#      }, {
#        'emit_emscripten_configs': 'false',
#      }],
      ['OS == "android"', {
        'executable': 'shared_library',
      }],
      ['OS == "win" and GENERATOR == "msvs"', {
        # For now, we'll retain the possiblity of generating multi-platform
        # solutions so just use WIN_PLATFORM to set the existing
        # variables.
        'conditions': [
          ['WIN_PLATFORM == "Win32"', {
            'emit_vs_win32_configs': 'true',
          }, 'MSVS_VERSION != "2010e" and MSVS_VERSION != "2008e" and MSVS_VERSION != "2005e"', {
            # Don't generate x64 configs in certain MSVS Express Edition
            # projects. Note: 2012e and 2013e support x64.
            'emit_vs_x64_configs': 'true',
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
    # Because libassimp is built with this disabled. It's not important unless
    # submitting to the App Store and currently bitcode is optional.
    'ENABLE_BITCODE': 'NO',
    # These have to be project-wide. If in target_defaults', and
    # therefore set in each target, Xcode 8 will warn that the project
    # settings are not the recommended settings and suggest it turns
    # all these on, even though they *will* all be turned on. Xcode
    # bug?
    'CLANG_ENABLE_OBJC_WEAK': 'YES',
    'CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING': 'YES',
    'CLANG_WARN_BOOL_CONVERSION': 'YES',
    'CLANG_WARN_COMMA': 'YES',
    'CLANG_WARN_CONSTANT_CONVERSION': 'YES',
    'CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS': 'YES',
    'CLANG_WARN_EMPTY_BODY': 'YES',
    'CLANG_WARN_ENUM_CONVERSION': 'YES',
    'CLANG_WARN_INFINITE_RECURSION': 'YES',
    'CLANG_WARN_INT_CONVERSION': 'YES',
    'CLANG_WARN_SUSPICIOUS_MOVE': 'YES',
    'CLANG_WARN_UNREACHABLE_CODE': 'YES',
    'CLANG_WARN__DUPLICATE_METHOD_MATCH': 'YES',
    'CLANG_WARN_NON_LITERAL_NULL_CONVERSION': 'YES',
    'CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF': 'YES',
    'CLANG_WARN_OBJC_LITERAL_CONVERSION': 'YES',
    'CLANG_WARN_RANGE_LOOP_ANALYSIS': 'YES',
    'CLANG_WARN_STRICT_PROTOTYPES': 'YES',
    'ENABLE_STRICT_OBJC_MSGSEND': 'YES',
    'GCC_NO_COMMON_BLOCKS': 'YES',
    'GCC_WARN_64_TO_32_BIT_CONVERSION': 'YES',
    'GCC_WARN_ABOUT_RETURN_TYPE': 'YES',
    'GCC_WARN_PEDANTIC': 'YES',
    'GCC_WARN_UNDECLARED_SELECTOR': 'YES',
    'GCC_WARN_UNINITIALIZED_AUTOS': 'YES',
    'GCC_WARN_UNUSED_FUNCTION': 'YES',
    'GCC_WARN_UNUSED_VARIABLE': 'YES',
    #-----------------------------------------------------
  }, # xcode_settings
  # This has to be project-wide too. If in target_defaults' Debug config
  # Xcode 7+ will warn that this recommended value is not set.
  'configurations': {
    'Debug': {
      'xcode_settings': {
        'ENABLE_TESTABILITY': 'YES',
      },
    },
    'Release': {
      'xcode_settings': {
        'ONLY_ACTIVE_ARCH': 'NO',
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
    'cflags': [ '-pedantic' ],
    'msvs_configuration_attributes': {
      # When generating multi-platform solutions & projects these
      # directories must be augmented with $(PlatformName).
      #'OutputDirectory': '$(SolutionDir)$(PlatformName)/$(ConfigurationName)',
      # Must have $(ProjectName) here to avoid conflicts between the
      # various projects' .tlog files in MSBuild/VS2010 that cause,
      # among other problems, all projects to be cleaned when
      # Project Only -> Clean is selected.
      #'IntermediateDirectory': '$(PlatformName)/$(ConfigurationName)/obj/$(ProjectName)'
      'IntermediateDirectory': '$(ConfigurationName)/obj/$(ProjectName)'
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
        # GYP defaults this to level 1 !!!
        'WarningLevel': 3,
      },
    },
    'xcode_settings': {
      'COPY_PHASE_STRIP': 'NO',
      # Avoid linker warnings about "Direct access in function'. These need
      # to be NO or YES everywhere. When not set here, for some reason, appfwSDL
      # had them set to NO while vkloadtests had them set to YES.
      'GCC_INLINES_ARE_PRIVATE_EXTERN': 'NO',
      'GCC_SYMBOLS_PRIVATE_EXTERN': 'NO',
      # Use C99 for maximum portability. Sigh!
      'GCC_C_LANGUAGE_STANDARD': 'c99',
      # Avoid Xcode 10 warning: "Traditional headermap style is no
      # longer supported".
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'conditions': [
        ['OS == "ios"', {
          # 1 = iPhone/iPod Touch; 2 = iPad
          'CODE_SIGN_IDENTITY': 'iPhone Developer',
          'IPHONEOS_DEPLOYMENT_TARGET': '8.0',
          'TARGETED_DEVICE_FAMILY': '1,2',
        }, 'OS == "mac"', {
          # Need 10.9 for GL 4.1 or ARB_ES2_compatibility, 10.11 for Metal
          # compatibility.
          'MACOSX_DEPLOYMENT_TARGET': '10.11',
          # Comment this out if deployment target >= 10.9
          #'CLANG_CXX_LIBRARY': 'libc++',
          'CODE_SIGN_IDENTITY': 'Mac Developer',
          'COMBINE_HIDPI_IMAGES': 'YES',
        }],
      ],
      'target_conditions': [
        ['_type == "executable" or _type == "shared_library"', {
          'target_conditions': [
            ['_mac_bundle == 1', {
              # Don't add a default value because this variable gets exported
              # as is to CMake and ${PRODUCT_NAME:-identifier} is invalid
              # syntax.
              'PRODUCT_BUNDLE_IDENTIFIER': 'org.khronos.ktx.${PRODUCT_NAME}',
            }, {
              # Needed to override a PROVISIONING_PROFILE_SPECIFIER that may
              # be set in Xcode preferences (Locations / Custom Paths) by
              # those with provisioning profiles. Such profiles can't be set
              # here as they are unique to each user or organization. When such
              # a PROVISIONING_PROFILE_SPECIFIER is set, absent the following
              # settings, macOS builds will insist on BUNDLE_IDENTIFIERs for
              # libs and tools but, as they aren't bundles, it isn't possible
              # to set them. iOS builds don't have this problem probably because
              # there are no tools and libktx is put into the app bundles.
              'CODE_SIGN_STYLE': 'Automatic',
              'PROVISIONING_PROFILE_SPECIFIER': '',
            }],
          ], # target_conditions, _mac_bundle
          # Starting with Xcode 8, DEVELOPMENT_TEAM must be specified
          # to successfully build a project. Since it will be different
          # for each user of this project, do not specify it here. See
          # ../BUILDING.md for instructions on how to set it in your
          # Xcode preferences.
        }],
      ], # target_conditions
    }, # xcode_settings
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
        # If this isn't set, GYP defaults to Win32 so both platforms
        # get included when generating x64 configs.
        'msvs_configuration_platform': '<(WIN_PLATFORM)',
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
        'msvs_configuration_platform': '<(WIN_PLATFORM)',
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
            'defines': [ 'VULKAN_HPP_TYPESAFE_CONVERSION' ],
            'msvs_configuration_platform': 'Win32',
            'msvs_settings': {
              'VCLinkerTool': {
                # Disable because it prevents E&C and is unnecessary in
                # debug configurations.
                'AdditionalOptions': '/SAFESEH:NO',
              },
            },
          },
          'Release_Win32': {
            'inherit_from': ['Release'],
            'defines': [ 'VULKAN_HPP_TYPESAFE_CONVERSION' ],
            'msvs_configuration_platform': 'Win32',
          },
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
