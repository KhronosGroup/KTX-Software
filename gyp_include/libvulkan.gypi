##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on Vulkan.
#
{
  'variables': { # level 1
    'variables': { # level 2
      'variables': { # level 3
        # NB for XCODE: Due to difficulties passing env. vars to Xcode, set
        # VULKAN_SDK in Xcode Preferences, Locations tab, Custom Paths. It
        # should point to whereever you have the Vulkan SDK installed.
        'moltenvk': '$(VULKAN_INSTALL_DIR)/MoltenVK',
      }, # end level 3
      'moltenvk': '<(moltenvk)',
      'conditions': [
        ['OS == "ios"', {
          'mvklib': '<(moltenvk)/iOS',
          'vksdk': '<(moltenvk)' # Until there's an official SDK.
        }, 'OS == "mac"', {
          'mvklib': '<(moltenvk)/macOS',
          'vksdk': '$(VULKAN_SDK)',
        }]
      ], # conditions
    }, # end level 2
    'moltenvk': '<(moltenvk)',
    'mvklib': '<(mvklib)',
    'vksdk': '<(vksdk)',
    'conditions': [
      ['OS == "ios"', {
        'fwdir': '<(mvklib)',
      }, 'OS == "mac"', {
        'fwdir': '<(vksdk)/Frameworks',
      }]
    ], # conditions
  }, # variables
  'targets': [{
    'target_name': 'vulkan_headers',
    'type': 'none',
    # $VULKAN_SDK points to the location of an installed Vulkan SDK.
    # It must be in the includes list on ios, mac & win. On Linux
    # it can be omitted, if the libvulkan_dev package is installed.
    # Otherwise it can point to the location of a more recent version
    # of the standard SDK from LunarG.
    'direct_dependent_settings': {
      'conditions': [
        ['GENERATOR == "cmake"', {
          'include_dirs': [
            '/$ENV{VULKAN_SDK}/include',
          ],
        }, 'GENERATOR == "xcode"', {
          'include_dirs': [
            '<(vksdk)/include'
          ],
          'mac_framework_dirs': [
            '<(fwdir)'
          ],
        }, {
          'include_dirs': [
            '$(VULKAN_SDK)/include',
          ],
       }],
      ], # conditions
    } # direct_dependent_settings
  }, # vulkan_headers target
  {
    'target_name': 'libvulkan',
    'type': 'none',
    'dependencies': [ 'vulkan_headers', ],
    'export_dependent_settings': [ 'vulkan_headers' ],
    'conditions': [
      ['OS == "ios" or OS == "mac"', {
        'link_settings': {
          'libraries': [
            # items in libraries are relativized unless prefixed with /,-,$,<,>,
            # or ^. Both '-framework foo' and '-lfoo' confuse Xcode. It seems
            # these values are being put into an Xcode list that expects only
            # framework names, full or relative path.
            '<(fwdir)/MoltenVK.framework',
            '<(fwdir)/vulkan.framework',
            '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
            '$(SDKROOT)/System/Library/Frameworks/Metal.framework',
            '$(SDKROOT)/System/Library/Frameworks/IOSurface.framework',
            '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
            # Xcode 7 & 8 do not properly handle the new lib*.tbd files. They
            # warn they are ignoring the file because of "unexpected file type
            # 'text'". The workaround is to use OTHER_LDFLAGS as below.
            #'$(SDKROOT)/usr/lib/libc++.tbd',
          ],
          'library_dirs': [ ],
          'mac_framework_dirs': [
            '<(fwdir)'
          ],
          # Not necessary because CLANG_CXX_LIBRARY is being set in
          # default.gypi, if needed for the specified DEPLOYMENT_TARGET.
          #'xcode_settings': {
          #  'OTHER_LDFLAGS': '-lc++',
          #}
        }, # link_settings
        'conditions': [
          ['OS == "mac"', {
            'link_settings': {
              'libraries!': [
                '<(fwdir)/MoltenVK.framework',
                '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
                '$(SDKROOT)/System/Library/Frameworks/Metal.framework',
                '$(SDKROOT)/System/Library/Frameworks/IOSurface.framework',
                '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
              ],
            },
            # dds is needed because each app target gets its own directory.
            'direct_dependent_settings': {
              'xcode_settings': {
                'LD_RUNPATH_SEARCH_PATHS': [ '@executable_path/../Frameworks' ],
              },
              'target_conditions': [
                ['_mac_bundle == 1', {
                  # Can't use mac_bundle_resources because that puts the files
                  # into $(UNLOCALIZED_RESOURCES_FOLDER_PATH).
                  'copies': [{
                    'xcode_code_sign': 1,
                    'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
                    'files': [
                      '<(fwdir)/vulkan.framework',
                      '<(mvklib)/libMoltenVK.dylib',
                    ],
                  },
                  {
                    'xcode_code_sign': 1,
                    'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/vulkan/icd.d',
                    'files': [ '<(otherlibroot_dir)/resources/MoltenVK_icd.json' ],
                  }], # copies
                }], # mac_bundle
              ], # target_conditions
            }, # direct_dependent_settings
          }, 'OS == "ios"', {
            'link_settings': {
              'libraries!': [ '<(fwdir)/vulkan.framework' ],
            },
          }]
        ]
      }, 'OS == "win"', {
        'link_settings': {
          'libraries': [ '-lvulkan-1' ],
          'conditions': [
            ['WIN_PLATFORM == "x64"', {
              'library_dirs': [ '$(VULKAN_SDK)/Lib' ],
            }, {
              'library_dirs': [ '$(VULKAN_SDK)/Lib32' ],
            }]
          ],
        },
      }, {
        'link_settings': {
         'libraries': [ '-lvulkan' ],
        },
      }] # OS == 'ios' or OS == "mac", etc
    ], # conditions
  }, # libvulkan target
  {
    'target_name': 'libvulkan.lazy',
    'type': 'none',
    'conditions': [
      ['OS == "ios"', {
        # Shared library, & therefore this target, not used on iOS.
      }, 'OS == "mac"', {
        'direct_dependent_settings': {
          'xcode_settings': {
            'OTHER_LDFLAGS': [
              '-lazy_library',
              '<(fwdir)/vulkan.framework/vulkan',
            ],
          },
        },
      }, 'OS == "win"', {
        'msvs_settings': {
          'VCLinkerTool': {
            'DelayLoadDLLs': 'vulkan-1',
          }
        }
      }, {
        # Boo! Hiss! GCC & therefore linux does not support delay
        # loading. Have to use dlopen/dlsym.
     }], # OS == "ios", etc.
    ],
  }], # vulkan.framework target & targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
