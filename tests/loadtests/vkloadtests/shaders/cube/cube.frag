// Copyright 2017 Mark Callow
// SPDX-License-Identifier: Apache-2.0
 
// Fragment shader for single texture
 
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#if 0
//layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec4 inColor;
#if 0
layout (location = 2) in float inLodBias;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inViewVec;
layout (location = 5) in vec3 inLightVec;
#endif

layout (location = 0) out vec4 outFragColor;

void main() 
{
#if 0
	vec4 color = texture(samplerColor, inUV, inLodBias);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);	
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
	float specular = pow(max(dot(R, V), 0.0), 16.0) * color.a;

	outFragColor = vec4(diffuse * color.rgb + specular, 1.0);
#endif
	outFragColor = vec4(inColor.rgb, 1.0);
}
#endif

#if 0
//layout (binding = 1) uniform sampler2D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float inLodBias;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 0) out vec4 outFragColor;

void main() 
{
	//vec4 color = texture(samplerColor, inUV, inLodBias);
	vec4 color = vec4(1.0, 0.0, 0.0, 1.0);

	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
	float specular = pow(max(dot(R, V), 0.0), 16.0) * color.a;

	outFragColor = vec4(diffuse * color.rgb + specular, 1.0);	
}
#endif

//layout (location = 0) in vec4 color;
layout (location = 0) out vec4 outColor;

void main() {
    //outColor = color;
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}

