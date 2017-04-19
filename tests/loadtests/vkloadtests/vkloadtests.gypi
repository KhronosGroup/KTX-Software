##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building KTX loadtests for Vulkan.
#
{
  'includes': [
    # The following is already included by appfwSDL.gypi, which
    # itself is included by this file's parent.
    #  '../../../gyp_include/libvulkan.gypi',
    '../../../gyp_include/libassimp.gypi',
  ],
  'variables': { # level 1
    # A hack to get the file name relativized for xcode's INFOPLIST_FILE.
    # Keys ending in _file & _dir assumed to be paths and are made relative
    # to the main .gyp file.
     'conditions': [
      ['OS == "ios"', {
        'vkinfoplist_file': 'resources_ios/Info.plist',
      }, 'OS == "mac"', {
        'vkinfoplist_file': 'resources_mac/Info.plist',
      }],
    ] # conditions
  }, # variables, level 1
  'targets': [
    {
      'target_name': 'vkloadtests',
      'type': '<(executable)',
      'mac_bundle': 1,
      'cflags': [ '-std=c++11' ],
      'defines': [ ],
      'dependencies': [
        'appfwSDL',
        'libassimp',
        'libktx.gyp:libvulkan',
        'testimages',
      ],
      'includes': [
        '../../../gyp_include/glsl2spirv.gypi',
      ],
      'include_dirs': [
        # Uncomment if we #include .spv files in the c++ files.
        #'<(SHARED_INTERMEDIATE_DIR)',
        '../common',
        '../geom',
      ],
      'sources': [
        '../common/vecmath.hpp',
        'Texture.cpp',
        'Texture.h',
        'TextureArray.cpp',
        'TextureArray.h',
        'TextureCubemap.cpp',
        'TextureCubemap.h',
        'TexturedCube.cpp',
        'TexturedCube.h',
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
        'utils/VulkanMeshLoader.hpp',
        'VulkanLoadTests.cpp',
        'VulkanLoadTests.h',
        'VulkanLoadTestSample.cpp',
        'VulkanLoadTestSample.h',
      ],
      'copies': [{
        'destination': '<(model_dest)',
        'files': [
          'models/cube.obj',
          'models/sphere.obj',
          'models/teapot.dae',
          'models/torusknot.obj',
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
        'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
        'GCC_C_LANGUAGE_STANDARD': 'c99',
        'INFOPLIST_FILE': '<(vkinfoplist_file)',
        # Minimum targets for Metal/MoltenVK.
        'conditions': [
          ['OS == "ios"', {
            'IPHONEOS_DEPLOYMENT_TARGET': '9.0',
          }, 'OS == "mac"', {
            'MACOSX_DEPLOYMENT_TARGET': '10.11',
          }],
        ], # conditions
      }, # xcode_settings
      'conditions': [
        ['OS == "ios"', {
          'dependencies': [ 'libktx.gyp:libktx.es3' ],
          'sources': [ 'resources_ios/Info.plist' ],
          'mac_bundle_resources': [
            'resources_ios/Images.xcassets',
            'resources_ios/LaunchScreen.storyboard',
          ],
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'AppIcon',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(vkinfoplist_file)',
          },
        }, 'OS == "mac"', {
          'dependencies': [ 'libktx.gyp:libktx.gl' ],
          'mac_bundle_resources': [
            'resources_mac/KTXAppIcons.icns',
          ],
          'sources': [ 'resources_mac/Info.plist' ],
        }, {
          'dependencies': [ 'libktx.gyp:libktx.gl' ],
        }], # OS == "ios"
        ['OS == "mac" or OS == "ios"', {
          # This copies the shaders to "Resources/shaders" thus avoiding
          # polluting "Resources" with all the .spv files and avoiding a
          # platform dependent path for loading the shaders, as I certainly
          # don't want to pollute the output directories on other platforms.
          # With the simpler choice of setting
          # 'process_outputs_as_mac_bundle_resources' in the glsl2spirv rules
          # there is no way to set a subdir of "Resources" as the destination.
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
            ],
          }], # copies
        }],
      ], # conditions
    }, # vkloadtests
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
