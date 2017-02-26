##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Generate project file for building KTX loadtests for Vulkan.
#
{
  # The following is already included by appfwSDL.gypi, which
  # itself is included by this file's parent.
  #'includes': [
  #  '../../../gyp_include/libvulkan.gypi',
  #],
  'variables': { # level 1
    'variables': { # level 2
      'conditions': [
        ['OS == "android"', {
          'datadest': '<(android_assets_dir)',
        }, 'OS == "ios" or OS == "mac"', {
          'datadest': '<(PRODUCT_DIR)/$(UNLOCALIZED_RESOURCES_FOLDER_PATH)',
        }, 'OS == "linux" or OS == "win"', {
          'datadest': '<(PRODUCT_DIR)',
        }], # OS == "android" and else clauses
      ], # conditions
    }, # variables level 2
    'model_dir': '<(datadest)/models',
    'shader_dir': '<(datadest)/shaders',
    # A hack to get the file name relativized for xcode's INFOPLIST_FILE.
    # Keys ending in _file & _dir assumed to be paths and are made relative
    # to the main .gyp file.
     'conditions': [
      ['OS == "ios"', {
        'infoplist_file': 'resources_ios/Info.plist',
      }, 'OS == "mac"', {
        'infoplist_file': 'resources_mac/Info.plist',
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
        'libktx.gyp:libktx.gl',
        'libktx.gyp:libvulkan',
        'testimages',
      ],
      'includes': [
        '../../../gyp_include/glsl2spirv.gypi',
      ],
      'include_dirs': [
        '<(INTERMEDIATE_DIR)',
        '../common',
        '../geom',
        '$(ASSIMP_HOME)/include',
      ],
      'conditions': [
        ['OS != "mac"', {
          'include_dirs!': [ '$(ASSIMP_HOME)/include' ],
        }],
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
        'destination': '<(model_dir)',
        'files': [ 
          'models/cube.obj',
          'models/sphere.obj',
          'models/teapot.dae',
          'models/torusknot.obj',
        ],
      }], # copies      
      'link_settings': {
        'conditions': [
          ['OS == "linux"', {
            'libraries': [ '-lassimp', '-lpthread' ],
          }],
          ['OS == "mac"', {
            'library_dirs': [ '$(ASSIMP_HOME)/lib' ],
            'xcode_settings': {
              'OTHER_LDFLAGS': '-lassimp',
            },
          }],
        ],
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
        'INFOPLIST_FILE': '<(infoplist_file)',
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
          'sources': [
            'resources_ios/Info.plist',
          ],
          'mac_bundle_resources': [
            'resources_ios/Images.xcassets',
            'resources_ios/LaunchScreen.storyboard',
          ],
          'xcode_settings': {
            'ASSETCATALOG_COMPILER_APPICON_NAME': 'AppIcon',
            'ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME': 'LaunchImage',
            'INFOPLIST_FILE': '<(infoplist_file)',
          },
        }, 'OS == "mac"', {
          'sources': [
            'resources_mac/Info.plist',
          ],
        }], # OS == "ios", etc
      ], # conditions
    }, # vkloadtests
  ], # targets
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
