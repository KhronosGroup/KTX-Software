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
	  # For if & when we use glslc -mfmt=num so can #include the
	  # compiled files.
	  #'outputs': [ '<(INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
	  'outputs': [
		'<(shader_dir)/<(RULE_INPUT_NAME).spv'
	  ],
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          'action': [
            '$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter',
            '-gi', '<(RULE_INPUT_PATH)',
            '-so', '<@(_outputs)',
            ], # action
        }, {
          'action': [
            'glslangValidator', '-V',
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
	  #'outputs': [ '<(INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
	  'outputs': [
		'<(shader_dir)/<(RULE_INPUT_NAME).spv'
	  ],
      'conditions': [
        ['OS == "mac" or OS == "ios"', {
          'action': [
          '$(VULKAN_SDK)/../MoltenShaderConverter/Tools/MoltenShaderConverter',
          '-gi', '<(RULE_INPUT_PATH)',
          '-so', '<@(_outputs)',
          ], # action
        }, {
          'action': [
          'glslangValidator', '-V',
          '-o', '<@(_outputs)',
          '<(RULE_INPUT_PATH)'
          ], # action
        }], # OS == "mac" or OS == "ios"
      ], # conditions
    } # vert2spirv
  ], # rules
}

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
