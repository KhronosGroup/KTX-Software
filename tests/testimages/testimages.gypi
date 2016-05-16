##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Target for including test images in KTX tests.
#
{
  'variables': { # level 1
    'datadir': 'testimages',
    # Hack to get the directory relativized
    'testimages_dir': '.',
  },
  'targets': [
    {
      'target_name': 'testimages',
      'type': 'none',
      'direct_dependent_settings': {
        'copies': [{
          'conditions': [
            ['OS == "android"', {
              'destination': '<(android_assets_dir)/<(datadir)',
            }, 'OS == "ios" or OS == "mac"', {
              'destination': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/<(datadir)',
            }, 'OS == "linux" or OS == "win"', {
                'destination': '<(PRODUCT_DIR)/<(datadir)',
            }], # OS == "android" and else clauses
          ], # conditions
          'files': [ '<!@(ls <(testimages_dir)/*.ktx)' ],
        }], # copies
      } # direct_dependent_settings
    } # testimages target
  ] # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
