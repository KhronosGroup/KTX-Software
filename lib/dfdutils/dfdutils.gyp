##
# @internal
# @copyright Copyright 2019-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0
#
# @brief Generate project files for building ktxinfo.
#
{
  # All these settings have to be global. If set in target_defaults Xcode
  # warns they aren't set, though they appear to actually be set.
  'xcode_settings': {
    'MACOSX_DEPLOYMENT_TARGET': '10.11',
    'ONLY_ACTIVE_ARCH': 'YES',
    # Xcode recommended target settings.
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
  },
  'configurations': {
    'Debug': {
      'xcode_settings':  {
        'ENABLE_TESTABILITY': 'YES',
      },
    },
  },
  'target_defaults': {
    'xcode_settings': {
      # Needed to override a PROVISIONING_PROFILE_SPECIFIER that may
      # be set in Xcode preferences (Locations / Custom Paths) by
      # those with provisioning profiles. Such profiles can't be set
      # here as they are unique to each user or organization. When such
      # a PROVISIONING_PROFILE_SPECIFIER is set, absent the following
      # settings, macOS builds will insist on BUNDLE_IDENTIFIERs for
      # libs and tools but, as they aren't bundles, it isn't possible
      # to set them.
      'CODE_SIGN_STYLE': 'Automatic',
      'PROVISIONING_PROFILE_SPECIFIER': '',
      'CODE_SIGN_IDENTITY': '-',  # Has to be in target_defaults not global.
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
    },
    'configurations': {
      'Debug': {
        'xcode_settings':  {
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES',
          'GCC_OPTIMIZATION_LEVEL': 0,
          'target_conditions': [
            ['_type == "executable"', {
              'STRIP_INSTALLED_PRODUCT': 'NO',
            }],
          ],
        },
      },
      'Release': {
        'xcode_settings':  {
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          'GCC_OPTIMIZATION_LEVEL': 3,
          'ONLY_ACTIVE_ARCH': 'NO',
          'target_conditions': [
            ['_type == "executable"', {
              'STRIP_INSTALLED_PRODUCT': 'YES',
            }],
          ],
        },
      },
    },
  }, # target_defaults
  'targets': [
    {
      'target_name': 'testbirectionalmapping',
      'type': 'executable',
      'mac_bundle': 0,
      'include_dirs': [
        '.',
      ],
      'sources': [
        'createdfd.c',
        'dfd.h',
        'dfd2vk.inl',
        'interpretdfd.c',
        'KHR/khr_df.h',
        'testbidirectionalmapping.c',
        'vk2dfd.inl',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:CONSOLE
          'SubSystem': '1',
        },
      },
    }, # testbirectionalmapping target
    {
      'target_name': 'testcreatedfd',
      'type': 'executable',
      'mac_bundle': 0,
      'include_dirs': [
        '.',
      ],
      'sources': [
        'createdfd.c',
        'createdfdtest.c',
        'dfd.h',
        'KHR/khr_df.h',
        'printdfd.c',
        'vk2dfd.inl',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:CONSOLE
          'SubSystem': '1',
        },
      },
    }, # testcreatedfd target
    {
      'target_name': 'testinterpretdfd',
      'type': 'executable',
      'mac_bundle': 0,
      'include_dirs': [
        '.',
      ],
      'sources': [
        'createdfd.c',
        'dfd.h',
        'interpretdfd.c',
        'interpretdfdtest.c',
        'KHR/khr_df.h',
        'printdfd.c',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:CONSOLE
          'SubSystem': '1',
        },
      },
    }, # testinterpretdfd target
  ], # targets

}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
