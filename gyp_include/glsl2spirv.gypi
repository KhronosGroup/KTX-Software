##
# @internal
# @copyright © 2016, Mark Callow. For license see LICENSE.md.
#
# @brief Rules for compiling GLSL to SPIR-V.
#
{
  'rules': [
	{
	  'rule_name': 'frag2spirv',
	  # Very irritating! A rule can have only 1 extension but glslangValidator
      # & MoltenShaderConverter use the extension to determine the shader type
      # so have to duplicate everything. (MSC has a -t option to specify type
      # but glslV does not so ...)
	  'extension': '.frag',
	  'message': 'Compiling <(RULE_INPUT_NAME).',
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          # This causes the output to be copied to the "Resources" folder but
          # we don't want to pollute that with all the shaders. We want them in
          # a subdir but there is no way to specify that. Instead we have
          # explicit copy steps in the users of the shaders.
          #'process_outputs_as_mac_bundle_resources': 1,
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
          'action': [
            '$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter',
            '-gi', '<(RULE_INPUT_PATH)',
            '-so', '<@(_outputs)',
          ], # action
        }, {
          'outputs': [ '<(shader_dest)/<(RULE_INPUT_NAME).spv' ],
          'action': [
            #'glslangValidator', '-V',
            'glslc', '-fshader-stage=fragment',
            '-o', '<@(_outputs)',
            '<(RULE_INPUT_PATH)'
          ], # action
        }], # OS == "mac" or OS == "ios"
      ], # conditions
    }, # frag2spirv
	{
	  'rule_name': 'vert2spirv',
	  'extension': '.vert',
	  'message': 'Compiling <(RULE_INPUT_NAME).',
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          #'process_outputs_as_mac_bundle_resources': 1,
          'outputs': [ '<(SHARED_INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
          'action': [
            '$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter',
            '-gi', '<(RULE_INPUT_PATH)',
            '-so', '<@(_outputs)',
          ], # action
        }, {
          'outputs': [ '<(shader_dest)/<(RULE_INPUT_NAME).spv' ],
          'action': [
            #'glslangValidator', '-V',
            'glslc', '-fshader-stage=vertex',
            '-o', '<@(_outputs)',
            '<(RULE_INPUT_PATH)'
          ], # action
        }], # OS == "mac" or OS == "ios"
      ], # conditions
    } # vert2spirv
  ], # rules
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
