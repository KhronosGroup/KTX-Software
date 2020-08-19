// Copyright 2017-2020 Mark Callow
// SPDX-License-Identifier: Apache-2.0

#version 450 core

layout (location = 0) in vec2 inUV;

layout (binding = 0) uniform sampler2D samplerFont;

layout (location = 0) out vec4 outFragColor;

void main(void)
{
	float color = texture(samplerFont, inUV).r;
	outFragColor = vec4(vec3(color), 1.0);
}
