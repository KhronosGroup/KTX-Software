# Copyright 2016-2020 Mark Callow
# SPDX-License-Identifier: Apache-2.0

##
# @internal
#
# @brief Rules for compiling GLSL to SPIR-V.
#
{
  'variables': {
    'conditions': [
      ['OS == "mac" or OS == "ios"', {
        'bin': '$(VULKAN_SDK)/bin/',
      }, {
        'bin': '',
      }],
    ],
  },
  'rules': [
    {
      'rule_name': 'frag2spirv',
      # Very irritating! A rule can have only 1 extension. Common practice is
      # to use .frag and .vert, so we have to duplicate this rule. Some tools,
      # though not glslc, distinguish shader type by the extension too.
      'extension': '.frag',
      'message': 'Compiling <(RULE_INPUT_NAME).',
      # This causes the output to be copied to the "Resources" folder but
      # we don't want to pollute that with all the shaders. We want them in
      # a subdir but there is no way to specify that. Instead we have
      # explicit copy steps in the users of the shaders.
      #'process_outputs_as_mac_bundle_resources': 1,
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
        }, {
          'outputs': [ '<(shader_dest)/<(RULE_INPUT_NAME).spv' ],
        }]
      ],
      # Using a "cygwin" shell results in the rule reading the
      # setup_env.bat file in the .gyp file directory. That .bat
      # file cd's to its location so actions will work. That
      # breaks rules because they relativize inputs and outputs
      # to the location of the vcxproj directory.
      'msvs_cygwin_shell': 0,
      'action': [
        #'glslangValidator', '-V',
        '<(bin)glslc', '-fshader-stage=fragment',
        '-o', '<@(_outputs)',
        '<(RULE_INPUT_PATH)'
      ], # action
    }, # frag2spirv
    {
      'rule_name': 'vert2spirv',
      'extension': '.vert',
      'message': 'Compiling <(RULE_INPUT_NAME).',
      #'process_outputs_as_mac_bundle_resources': 1,
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
        }, {
          'outputs': [ '<(shader_dest)/<(RULE_INPUT_NAME).spv' ],
        }]
      ],
      'msvs_cygwin_shell': 0,
      'action': [
        #'glslangValidator', '-V',
        '<(bin)glslc', '-fshader-stage=vertex',
        '-o', '<@(_outputs)',
        '<(RULE_INPUT_PATH)'
      ], # action
    } # vert2spirv
  ], # rules
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
