##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on Vulkan.
#
{
  'variables': {
    'conditions': [
      ['OS == "ios"', {
        'fwdir': 'iOS'
      }, 'OS == "mac"', {
        'fwdir': 'macOS'
      }]
    ], # conditions
  }, # variables
  'targets': [{
    'target_name': 'vulkan_headers',
    'type': 'none',
    #   $VULKAN_SDK points to the location of an installed Vulkan SDK.
    # It must be in the includes list on ios, mac & win. On Linux
    # it can be omitted, if libvulkan_dev is installed. Otherwise it
    # can point to the location of a more recent version of the
    # standard SDK from LunarG.
    'direct_dependent_settings': {
      'conditions': [
        ['GENERATOR == "cmake"', {
          'include_dirs': [
            '/$ENV{VULKAN_SDK}/include',
          ],
        }, 'GENERATOR == "xcode"', {
          # Due to difficulties passing env. vars to Xcode, set VULKAN_SDK
          # in Xcode Preferences, Locations tab, Custom Paths. It
          # should point to whereever you have MoltenVK installed.
          'include_dirs': [
            '$(VULKAN_SDK)/include'
          ],
          'mac_framework_dirs': [
            '$(VULKAN_SDK)/<(fwdir)'
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
    'dependencies': [ 'vulkan_headers' ],
    'export_dependent_settings': [ 'vulkan_headers' ],
    'conditions': [
      ['OS == "ios" or OS == "mac"', {
        'link_settings': {
          'libraries': [
            # items in libraries are relativized unless prefixed with /,-,$,<,>,
            # or ^. Both '-framework foo' and '-lfoo' confuse Xcode. It seems
            # these values are being put into an Xcode list that expects only
            # framework names, full or relative path.
            '$(VULKAN_SDK)/<(fwdir)/MoltenVK.framework',
            '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
            '$(SDKROOT)/System/Library/Frameworks/Metal.framework',
            '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
            # Xcode 7 & 8 do not properly handle the new lib*.tbd files. They
            # warn they are ignoring the file because of "unexpected file type
            # 'text'". The workaround is to use OTHER_LDFLAGS as below.
            #'$(SDKROOT)/usr/lib/libc++.tbd',
          ],
          'library_dirs': [ ],
          'mac_framework_dirs': [
            '$(VULKAN_SDK)/<(fwdir)'
          ],
          # Not necessary because CLANG_CXX_LIBRARY is being set in
          # default.gypi, if needed for the specified DEPLOYMENT_TARGET.
          #'xcode_settings': {
          #  'OTHER_LDFLAGS': '-lc++',
          #}
        },
      }, 'OS == "win"', {
        'link_settings': {
         'libraries': [ '-lvulkan-1' ],
         'library_dirs': [ '$(VULKAN_SDK)/Lib' ],
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
