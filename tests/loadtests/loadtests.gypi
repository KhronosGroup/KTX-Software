# Copyright 2015-2020 Mark Callow.
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Generate project files for building KTX loadtests.
#
{
  # For consistency between the apps and appfwSDL
  'variables': { # level 1
    'variables': { # level 2
      'conditions': [
        ['OS == "android"', {
          'datadest': '<(android_assets_dir)',
        }, 'OS == "ios" or OS == "mac"', {
          'datadest': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)',
        }, 'OS == "linux" or OS == "win"', {
          'datadest': '<(PRODUCT_DIR)',
        }, 'OS == "web"', {
          'datadest': '<(PRODUCT_DIR)',
        }], # OS == "android" and else clauses
      ], # conditions
    }, # variables level 2
    'model_dest': '<(datadest)/models',
    'shader_dest': '<(datadest)/shaders',
  }, # variables, level 1

  # Modify target_defaults for the loadtests. Inclusion of VkAppSDL in
  # appfwSDL means these settings are needed for all interactive tests.
  'target_defaults': {
    'xcode_settings': {
      'ONLY_ACTIVE_ARCH': 'YES',
      # Minimum targets for Metal/MoltenVK.
      'conditions': [
        ['OS == "ios"', {
          'IPHONEOS_DEPLOYMENT_TARGET': '11.0',
        }, 'OS == "mac"', {
          'MACOSX_DEPLOYMENT_TARGET': '10.11',
        }],
      ], # conditions
    },
  },
  'includes': [
    'appfwSDL/appfwSDL.gypi',
    '../../gyp_include/libassimp.gypi',
    'glloadtests/glloadtests.gypi',
  ],
  'conditions': [
    # Only these versions support C++11 which is needed by vkloadtests.
    ['OS != "web" and (GENERATOR != "msvs" or MSVS_VERSION == "2015" or MSVS_VERSION == "2017" or MSVS_VERSION == "2019")', {
      'includes': [ 'vkloadtests/vkloadtests.gypi' ],
    }],
  ],
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
