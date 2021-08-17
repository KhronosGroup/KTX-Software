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
};

// Keep the default value small to avoid MoltenVK issue 1420.
// https://github.com/KhronosGroup/MoltenVK/issues/1420
//layout(constant_id = 1) const int instanceCount = 1;
// Sadly the above does not work on iOS, only macOS so declare
// roughly the max size expected and in the app allocate the size
// as declared.
layout(constant_id = 1) const int instanceCount = 30;

layout (binding = 0, std140) uniform UBO
{
	mat4 projection;
	mat4 view;
	Instance instance[instanceCount];
} ubo;

// Workaround MoltenVK issue 1421 by passing instanceCount as a push constant.
// https://github.com/KhronosGroup/MoltenVK/issues/1421.
layout(push_constant) uniform PushConstants
{
	uint instanceCount;
} constants;

layout (location = 0) out vec3 outUVW;

void main() 
{
    float divisor = max(1.0, constants.instanceCount - 1);
    //float divisor = max(1.0, instanceCount - 1);
	outUVW = vec3(inUV, gl_InstanceIndex / divisor);
	mat4 modelView = ubo.view * ubo.instance[gl_InstanceIndex].model;
	gl_Position = ubo.projection * modelView * inPos;
}
