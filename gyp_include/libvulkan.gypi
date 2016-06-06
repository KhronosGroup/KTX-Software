##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on Vulkan.
#
{
  'targets': [{
    'target_name': 'vulkan_headers',
    'type': 'none',
    'conditions': [
      ['OS == "ios"', {
        'direct_dependent_settings': {
          'include_dirs': [
            # For the Vulkan includes
            '/Users/mark/Molten-0.12.0/MoltenVK/include',
          ],
            'mac_framework_dirs': [
            '/Users/mark/Molten-0.12.0/MoltenVK/iOS'
          ],
        },
      }, 'OS == "mac"', {
        'direct_dependent_settings': {
          'include_dirs': [
            # For the Vulkan includes
            '/Users/mark/Molten-0.12.0/MoltenVK/include',
          ],
          'mac_framework_dirs': [
            '/Users/mark/Molten-0.12.0/MoltenVK/iOS'
          ],
        },
      }, 'OS == "win"', {
        'direct_dependent_settings': {
          'include_dirs': [
            'somewhere',
          ],
        },
      #}, {
        # In /usr/include on Linux so nothing needed.
      }] # OS == 'ios', etc
    ], # conditions
  }, # vulkan_headers target
  {
    'target_name': 'libvulkan',
    'type': 'none',
    'dependencies': [ 'vulkan_headers' ],
    'export_dependent_settings': [ 'vulkan_headers' ],
    'conditions': [
      ['OS == "ios"', {
        'link_settings': {
          'libraries': [
            'MoltenVK.framework',
            'libc++.tbd',
            'Metal.framework',
          ],
          'library_dirs': [ ],
          'mac_framework_dirs': [
            '/Users/mark/Molten-0.12.0/MoltenVK/iOS'
          ],
        },
      }, 'OS == "mac"', {
        'link_settings': {
          'libraries': [
            'MoltenVK.framework',
            'libc++.tbd',
            'Metal.framework',
          ],
          'library_dirs': [ ],
          'mac_framework_dirs': [
            '/Users/mark/Molten-0.12.0/MoltenVK/iOS'
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
      }] # OS == 'ios', etc
    ], # conditions
  }], # libvulkan target & targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
