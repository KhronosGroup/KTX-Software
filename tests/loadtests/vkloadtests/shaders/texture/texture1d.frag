// Copyright 2017 Mark Callow
// SPDX-License-Identifier: Apache-2.0

#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (binding = 1) uniform sampler1D samplerColor;

layout (location = 0) in vec2 inUV;
layout (location = 1) in float inLodBias;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;
layout (location = 5) in vec3 inColor;

layout (location = 0) out vec4 outFragColor;

void main()
{
    vec4 fragcolor, texcolor;

    texcolor = texture(samplerColor, inUV.x, inLodBias);
    fragcolor.rgb = inColor.rgb * (1.0f - texcolor.a) + texcolor.rgb * texcolor.a;
    fragcolor.a = texcolor.a;

    vec3 N = normalize(inNormal);
    vec3 L = normalize(inLightVec);
    vec3 V = normalize(inViewVec);
    vec3 R = reflect(-L, N);
    vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
    float specular = pow(max(dot(R, V), 0.0), 16.0) * fragcolor.a;

    outFragColor = vec4(diffuse * fragcolor.rgb + specular, 1.0);
}
