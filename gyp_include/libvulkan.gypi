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
    'conditions': [
      ['OS == "ios" or OS == "mac"', {
        'direct_dependent_settings': {
          'include_dirs': [
            # For the Vulkan includes
            '/Users/mark/Molten-0.12.0/MoltenVK/include',
          ],
          'mac_framework_dirs': [
            '/Users/mark/Molten-0.12.0/MoltenVK/<(fwdir)'
          ],
        },
      #}, {
        # In /usr/include on Linux so nothing needed.
      }] # OS == 'ios' or OS == 'mac', etc
    ], # conditions
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
