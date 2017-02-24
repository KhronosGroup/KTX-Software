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
	  # Very irritating. Can only have 1 extension but glslangValidator
	  # requires an extension appropriate to the shader type so
	  # have to duplicate most of this.
	  'extension': '.frag',
	  'message': 'Compiling <(RULE_INPUT_NAME).<(RULE_INPUT_EXT).',
	  # For if & when we use glslc -mfmt=num so can #include the
	  # compiled files.
	  #'outputs': [ '<(INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
	  'outputs': [
		'<(shader_dir)/<(RULE_INPUT_NAME).spv'
	  ],
	  'action': [
		'glslangValidator', '-V',
		'-o', '<@(_outputs)',
		'<(RULE_INPUT_PATH)'
	  ],
	},
	{
	  'rule_name': 'vert2spirv',
	  'extension': '.vert',
	  'message': 'Compiling <(RULE_INPUT_NAME).<(RULE_INPUT_EXT).',
	  #'outputs': [ '<(INTERMEDIATE_DIR)/<(RULE_INPUT_NAME).spv' ],
	  'outputs': [
		'<(shader_dir)/<(RULE_INPUT_NAME).spv'
	  ],
	  'action': [
		'glslangValidator', '-V',
		'-o', '<@(_outputs)',
		'<(RULE_INPUT_PATH)'
	  ],
	}
  ],
} 

# vim:ai:ts=4:sts=4:sw=2:expandtab:textwidth=70
