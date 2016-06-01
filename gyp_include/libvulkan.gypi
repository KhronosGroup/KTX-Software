##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Target for adding dependency on Vulkan.
#
{
  'targets': [{
    'target_name': 'libvulkan',
    'type': 'none',
    'conditions': [
      ['OS == "ios"', {
        'direct_dependent_settings': {
          'include_dirs': [
            'somewhere',
          ]
        },
        'link_settings': {
         'libraries': [ 'somewhere' ],
         'library_dirs': [ ],
        },
      }, 'OS == "mac"', {
        'direct_dependent_settings': {
          'include_dirs': [
            'somewhere',
          ]
        },
        'link_settings': {
         'libraries': [ 'somewhere' ],
         'library_dirs': [ ],
        },
      }, 'OS == "win"', {
        'direct_dependent_settings': {
          'copies': [{
            'destination': '<(PRODUCT_DIR)',
            'files': [
              'libvulkan.dll',
            ],
          }],
          'include_dirs': [
            'somewhere',
          ],
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
  }], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
