# Copyright 2016-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for adding dependency on Vulkan.
#
{
  'variables': { # level 1
    'variables': { # level 2
      # NB for XCODE: Due to difficulties passing env. vars to Xcode, set
      # the following in Xcode Preferences | Locations tab | Custom Paths.
      #
      # - VULKAN_INSTALL_DIR to the directory where you have installed
      #                      the Khronos Vulkan SDK.
      # - VULKAN_SDK as directed by the SDK instructions, like $VULKAN_SDK, to
      #              the macOS folder of $(VULKAN_INSTALL_DIR). We can't just
      #              create this here because it is needed in the environment
      #              by some of the build commands.
      #
      # Note we can't just set VULKAN_INSTALL_DIR here to $(VULKAN_SDK)/..
      # because gyp generators, at least the xcode one, will remove any leading
      # component of a path that includes a '..'.
      'conditions': [
        ['OS == "ios"', {
          'vksdk': '$(VULKAN_SDK)',  # Until there is an official iOS SDK.
          'mvklib': '$(VULKAN_INSTALL_DIR)/MoltenVK/iOS',
        }, 'OS == "mac"', {
          'vksdk': '$(VULKAN_SDK)',
          'vklib': '$(VULKAN_SDK)/lib',
          'mvklib': '$(VULKAN_INSTALL_DIR)/MoltenVK/macOS',
        }]
      ], # conditions
    }, # end level 2
    'mvklib': '<(mvklib)',
    'vklib': '<(vklib)',
    'vksdk': '<(vksdk)',
    'conditions': [
      ['OS == "ios"', {
        'fwdir': '<(mvklib)/dynamic',
        'runpath': '@executable_path/Frameworks',
      }, 'OS == "mac"', {
        'fwdir': '<(vksdk)/Frameworks',
        'runpath': '@executable_path/../Frameworks',
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
            #'<(fwdir)/MoltenVK.framework',
            '<(fwdir)/libMoltenVK.dylib',
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
        # dds is needed because each app target gets its own directory.
        'direct_dependent_settings': {
          'xcode_settings': {
            'LD_RUNPATH_SEARCH_PATHS': [ '<(runpath)' ],
          },
          'target_conditions': [
            ['_type == "executable" and _mac_bundle == 1', {
              # Can't use mac_bundle_resources because that puts the files
              # into $(UNLOCALIZED_RESOURCES_FOLDER_PATH).
              'conditions': [
                ['OS == "ios"', {
                  'copies': [{
                    'xcode_code_sign': 1,
                    'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
                    'files': [
                      '<(mvklib)/dynamic/libMoltenVK.dylib',
                    ],
                  }], # ios copies
                }, 'OS == "mac"', {
                  'copies': [{
                    'xcode_code_sign': 1,
                    'destination': '<(PRODUCT_DIR)/$(FRAMEWORKS_FOLDER_PATH)',
                    'files': [
                      '<(fwdir)/vulkan.framework',
                      '<(vklib)/libMoltenVK.dylib',
                      # Copy the layer dylibs so they can be signed, meeting
                      # requirements for hardened runtime and notarization.
                      # If we try to use layers from the Vulkan SDK via an
                      # environment variable, the layers won't be loaded.
                      # Layers are only needed for debug config but I don't
                      # think there is any way to do a copy only for certain
                      # configs.
                      '<(vklib)/libVkLayer_api_dump.dylib',
                      '<(vklib)/libVkLayer_khronos_validation.dylib',
                    ],
                  },
                  {
                    'xcode_code_sign': 1,
                    'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/vulkan/icd.d',
                    'files': [ '<(otherlibroot_dir)/resources/MoltenVK_icd.json' ],
                  },
                  {
                    'xcode_code_sign': 1,
                    'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/vulkan/explicit_layer.d',
                    'files': [
                      # Need these to tell Vulkan loader where the layers are.
                      '<(otherlibroot_dir)/resources/VkLayer_api_dump.json',
                      '<(otherlibroot_dir)/resources/VkLayer_khronos_validation.json',
                    ],
                  }], # copies mac
                }], # OS == "ios"
              ], # conditions
            }], # mac_bundle
          ], # target_conditions
        }, # direct_dependent_settings
        'conditions': [
          ['OS == "mac"', {
            'link_settings': {
              'libraries!': [
                #'<(fwdir)/MoltenVK.framework',
                '<(fwdir)/libMoltenVK.dylib',
                '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
                '$(SDKROOT)/System/Library/Frameworks/Metal.framework',
                '$(SDKROOT)/System/Library/Frameworks/IOSurface.framework',
                '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
              ],
            },
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
  }], # libvulkan target & targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
