// Copyright 2017 Mark Callow
// SPDX-License-Identifier: Apache-2.0

#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;

struct Instance
{
	mat4 model;
	vec4 arrayIndex;
};

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 view;
	Instance instance[20];
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out flat float lambda;

void main() 
{
	outUV = inUV;
    lambda = gl_InstanceIndex+0.5;
	mat4 modelView = ubo.view * ubo.instance[gl_InstanceIndex].model;
	gl_Position = ubo.projection * modelView * inPos;
}
