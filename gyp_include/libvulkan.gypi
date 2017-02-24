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
        'fwdir': 'OSX'
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
    #   The leading '/' is to workaround bugs(?) in the generators
    # that they relativize paths even when they begin with an
    # environment variable.
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
            #'/Users/mark/Molten-0.12.0/MoltenVK/include',
            '/$VULKAN_SDK/include'
          ],
          'mac_framework_dirs': [
            #'/Users/mark/Molten-0.12.0/MoltenVK/<(fwdir)'
          ],
        }, {
          'include_dirs': [
            '/$VULKAN_SDK/include',
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
            'MoltenVK.framework',
            'libc++.dylib',
            'Metal.framework',
            'QuartzCore.framework'
          ],
          'library_dirs': [ ],
          'mac_framework_dirs': [
            '/Users/mark/Molten-0.12.0/MoltenVK/<(fwdir)'
          ],
        },
      }, 'OS == "win"', {
        'direct_dependent_settings': {
          'copies': [{
            'destination': '<(PRODUCT_DIR)',
            'files': [
              'libvulkan.dll',
            ],
          }],
        },
        'link_settings': {
         'libraries': [ 'somewhere' ],
         'library_dirs': [ ],
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
