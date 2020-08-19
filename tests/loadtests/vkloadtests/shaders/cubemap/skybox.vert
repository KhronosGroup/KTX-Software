// Copyright 2017 Mark Callow
// SPDX-License-Identifier: Apache-2.0

#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 modelView;
    mat4 invModelView;
    mat4 uvwTransform;
} ubo;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
    //outUVW = inPos.xyz;
	outUVW = (ubo.uvwTransform * vec4(inPos.xyz, 1.0)).xyz;
    // Override the Z component with the W component to guarantee that the
    // final Z value of the position will be 1.0.
	gl_Position = (ubo.projection * ubo.modelView * vec4(inPos.xyz, 1.0)).xyww;
}
