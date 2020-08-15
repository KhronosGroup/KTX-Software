# Copyright 2016-2020 Mark Callow.
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Target for including test images in KTX tests.
#
{
  'variables': { # level 1
    'variables': { # level 2
      'datadir': 'testimages',
    },
    'conditions': [
      ['OS == "android"', {
        'imagedest': '<(android_assets_dir)/<(datadir)',
      }, 'OS == "ios" or OS == "mac"', {
        'imagedest': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)/<(datadir)',
      }, 'OS == "linux" or OS == "win" or OS == "web"', {
        'imagedest': '<(PRODUCT_DIR)/<(datadir)',
      }], # OS == "android" and else clauses
    ], # conditions
     # Hack to get the directory relativized
    'testimages_dir': '.',
  },
  'targets': [
    {
      'target_name': 'testimages',
      'type': 'none',
      'conditions': [
        ['OS == "ios" or OS == "mac"', {
          # dds is needed on these platforms because each app target gets
          # its own directory.
          'direct_dependent_settings': {
            'copies': [{
             'destination': '<(imagedest)',
             'files': [
                '<!@(ls <(testimages_dir)/*.ktx)',
                '<!@(ls <(testimages_dir)/*.ktx2)',
              ],
            }], # copies
          }, # direct_dependent_settings
        }, {
          'copies': [{
           'destination': '<(imagedest)',
           'files': [
                '<!@(ls <(testimages_dir)/*.ktx)',
                '<!@(ls <(testimages_dir)/*.ktx2)',
            ],
          }], # copies
        }],
      ],
    }, # testimages target
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
