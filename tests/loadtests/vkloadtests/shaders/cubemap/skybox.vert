#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelView;
} ubo;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	// Compensate for original images in texture being opposite to reality.
	outUVW.x = -outUVW.x;
	gl_Position = ubo.projection * ubo.modelView * vec4(inPos.xyz, 1.0);
}
