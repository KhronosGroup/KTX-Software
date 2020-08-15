# Copyright 2016-2020 Mark Callow.
# SPDX-License-Identifier: Apache-2.0

# @internal
#
# @brief Generate project file for building KTX loadtests for Vulkan.
#
{
  'includes': [
    # The following is already included by appfwSDL.gypi, which
    # itself is included by this file's parent.
    #  '../../../gyp_include/libvulkan.gypi',
  ],
  'variables': { # level 1
    # A hack to get the file name relativized for xcode's INFOPLIST_FILE.
    # Keys ending in _file & _dir assumed to be paths and are made relative
    # to the main .gyp file.
     'conditions': [
      ['OS == "ios"', {
        'vkinfoplist_file': 'resources/ios/Info.plist',
      }, {
        'vkinfoplist_file': 'resources/mac/Info.plist',
      }],
    ] # conditions
  }, # variables, level 1
  'targets': [
    {
      'target_name': 'vkloadtests',
      'type': '<(executable)',
      'mac_bundle': 1,
      'defines': [ ],
      'dependencies': [
        'appfwSDL',
        'libassimp',
        'libktx.gyp:libvulkan',
        'libktx.gyp:libktx',
        'testimages',
      ],
      'includes': [
        '../../../gyp_include/glsl2spirv.gypi',
      ],
      'include_dirs': [
        # Uncomment if we #include .spv files in the c++ files.
        #'<(SHARED_INTERMEDIATE_DIR)',
        '../../../utils',
        '../common',
        '../geom',
        'utils',
      ],
      'sources': [
        '../../../utils/argparser.h',
        '../../../utils/argparser.cpp',
        '../common/LoadTestSample.cpp',
        '../common/LoadTestSample.h',
        '../common/SwipeDetector.cpp',
        '../common/SwipeDetector.h',
        '../common/ltexceptions.h',
        '../common/vecmath.hpp',
        '<(vkinfoplist_file)',
        'shaders/cube/cube.frag',
        'shaders/cube/cube.vert',
        'shaders/cubemap/reflect.frag',
        'shaders/cubemap/reflect.vert',
        'shaders/cubemap/skybox.frag',
        'shaders/cubemap/skybox.vert',
        'shaders/texture/texture.frag',
        'shaders/texture/texture.vert',
        'shaders/texturearray/instancing.frag',
        'shaders/texturearray/instancing.vert',
        'shaders/texturemipmap/instancinglod.frag',
        'shaders/texturemipmap/instancinglod.vert',
        'InstancedSampleBase.cpp',
        'InstancedSampleBase.h',
        'Texture.cpp',
        'Texture.h',
        'TextureArray.cpp',
        'TextureArray.h',
        'TextureCubemap.cpp',
        'TextureCubemap.h',
        'TexturedCube.cpp',
        'TexturedCube.h',
        'TextureMipmap.cpp',
        'TextureMipmap.h',
        'shaders/cube/cube.frag',
        'shaders/cube/cube.vert',
        'shaders/cubemap/reflect.frag',
        'shaders/cubemap/reflect.vert',
        'shaders/cubemap/skybox.frag',
        'shaders/cubemap/skybox.vert',
        'shaders/texture/texture.frag',
        'shaders/texture/texture.vert',
        'shaders/texturearray/instancing.frag',
        'shaders/texturearray/instancing.vert',
        'utils/VulkanTextureTranscoder.hpp',
        'utils/VulkanMeshLoader.hpp',
        'VulkanLoadTests.cpp',
        'VulkanLoadTests.h',
        'VulkanLoadTestSample.cpp',
        'VulkanLoadTestSample.h',
      ],
      'copies': [{
        'destination': '<(model_dest)',
        'files': [
          '../common/models/cube.obj',
          '../common/models/sphere.obj',
          '../common/models/teapot.dae',
          '../common/models/torusknot.obj',
        ],
      }],
      'link_settings': {
        'conditions': [
          ['OS == "linux"', {
            'libraries': [ '-lpthread' ],
          }],
        ], # conditions
      },
      'msvs_settings': {
        'VCLinkerTool': {
          # /SUBSYSTEM:WINDOWS.
          'SubSystem': '2',
        },
      },
      'xcode_settings': {
        'INFOPLIST_FILE': '<(vkinfoplist_file)',
      }, # xcode_settings
      'conditions': [
        ['OS == "ios"', {
          'mac_bundle_resources': [
            '../../../icons/ios/CommonIcons.xcassets',
            'resources/ios/LaunchImages.xcassets',
            'resources/ios/LaunchScreen.storyboard',
          ],
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'ktx_app',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
          },
        }, 'OS == "linux"', {
        }, 'OS == "mac"', {
          'mac_bundle_resources': [
            '../../../icons/mac/ktx_app.icns',
          ],
        }, 'OS == "win"', {
          'sources!': [ '<(vkinfoplist_file)' ],
          'sources': [
             '../../../icons/win/ktx_app.ico',
             'resources/win/vkloadtests.rc',
             'resources/win/resource.h',
          ],

        }], # OS == "ios"
        ['OS == "mac" or OS == "ios"', {
          # This copies the shaders to "Resources/shaders". With the simpler
          # choice of setting 'process_outputs_as_mac_bundle_resources' in the
          # glsl2spirv rules there is no way to set a subdir of "Resources" as
          # the destination thus polluting "Resources" with all the .spv
          # files. As I certainly want to avoid polluting the output directories
          # on other platforms and don't want to have a platform dependent path
          # for loading the shaders, this is preferable.
          'copies': [{
            'destination': '<(shader_dest)',
            'files': [
            '<(SHARED_INTERMEDIATE_DIR)/cube.frag.spv',
            '<(SHARED_INTERMEDIATE_DIR)/cube.vert.spv',
            '<(SHARED_INTERMEDIATE_DIR)/reflect.frag.spv',
            '<(SHARED_INTERMEDIATE_DIR)/reflect.vert.spv',
            '<(SHARED_INTERMEDIATE_DIR)/skybox.frag.spv',
            '<(SHARED_INTERMEDIATE_DIR)/skybox.vert.spv',
            '<(SHARED_INTERMEDIATE_DIR)/texture.frag.spv',
            '<(SHARED_INTERMEDIATE_DIR)/texture.vert.spv',
            '<(SHARED_INTERMEDIATE_DIR)/instancing.frag.spv',
            '<(SHARED_INTERMEDIATE_DIR)/instancing.vert.spv',
            '<(SHARED_INTERMEDIATE_DIR)/instancinglod.frag.spv',
            '<(SHARED_INTERMEDIATE_DIR)/instancinglod.vert.spv',
            ],
          }], # copies
        }],
      ], # conditions
    }, # vkloadtests
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
