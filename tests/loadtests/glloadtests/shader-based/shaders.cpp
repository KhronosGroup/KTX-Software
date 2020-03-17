/* -*- tab-iWidth: 4; -*- */
/* vi: set sw=2 ts=4 expandtab: */

/*
 * ©2008 HI Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file    shaders.c
 * @brief   Shaders used by the DrawTexture and TexturedCube samples.
 *
 * @author Mark Callow, www.edgewise-consulting.com
 */

#include "GL/glcorearb.h"
#include "ktx.h"

const GLchar* pszVs =
"layout(location = 0) in vec4 position;\n"
"layout(location = 1) in vec4 color;\n"
"layout(location = 2) in vec4 texcoord;\n\n"

"out vec4 v_color;\n"
"out vec2 v_texcoord;\n\n"

"uniform highp mat4 mvmatrix;\n"
"uniform highp mat4 pmatrix;\n\n"

"void main(void)\n"
"{\n"
"   mat4 mvpmatrix = pmatrix * mvmatrix;\n"
"   v_color = color;\n"
"   v_texcoord = texcoord.xy;\n"
"   gl_Position = mvpmatrix * position;\n"
"}";

const GLchar* pszDecalFs =
"precision mediump float;\n\n"

"uniform sampler2D sampler;\n\n"

"in vec4 v_color;\n"
"in vec2 v_texcoord;\n"
"out vec4 fragcolor;\n\n"

"void main(void)\n"
"{\n"
"  vec4 color = texture(sampler, v_texcoord);\n"
"  // DECAL\n"
"  fragcolor.rgb = v_color.rgb * (1.0f - color.a) + color.rgb * color.a;\n"
"  fragcolor.a = color.a;\n"
"}";

const GLchar* pszColorFs =
"precision mediump float;\n\n"

"in vec4 v_color;\n"
"out vec4 fragcolor;\n\n"

"void main(void)\n"
"{\n"
"  fragcolor = v_color;\n"
"}";

// For use when sRGB rendering is not available. Note this is really a
// workaround for broken GL implementations (most) where "linear" framebuffers
// are really sRGB framebuffers but since they are "linear" GL does not encode
// the output of the fs to sRGB so we do it ourselves.
const GLchar* pszDecalSrgbEncodeFs =
"precision mediump float;\n\n"

"uniform sampler2D sampler;\n\n"

"in vec4 v_color;\n"
"in vec2 v_texcoord;\n"
"out vec4 fragcolor;\n\n"

"vec3 srgb_encode(vec3 color) {\n"
"   float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;\n"
"   float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;\n"
"   float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;\n"
"   return vec3(r, g, b);\n"
"}\n\n"

"void main(void)\n"
"{\n"
"  vec4 t_color = texture(sampler, v_texcoord);\n"
"  vec3 lin_fragcolor;\n"
"  // DECAL\n"
"  lin_fragcolor = v_color.rgb * (1.0f - t_color.a) + t_color.rgb * t_color.a;\n"
"  fragcolor.rgb = srgb_encode(lin_fragcolor);\n"
"  fragcolor.a = t_color.a;\n"
"}";

const GLchar* pszColorSrgbEncodeFs =
"precision mediump float;\n\n"

"in vec4 v_color;\n"
"out vec4 fragcolor;\n\n"

"vec3 srgb_encode(vec3 color) {\n"
"   float r = color.r < 0.0031308 ? 12.92 * color.r : 1.055 * pow(color.r, 1.0/2.4) - 0.055;\n"
"   float g = color.g < 0.0031308 ? 12.92 * color.g : 1.055 * pow(color.g, 1.0/2.4) - 0.055;\n"
"   float b = color.b < 0.0031308 ? 12.92 * color.b : 1.055 * pow(color.b, 1.0/2.4) - 0.055;\n"
"   return vec3(r, g, b);\n"
"}\n\n"

"void main(void)\n"
"{\n"
"  fragcolor.rgb = srgb_encode(v_color.rgb);\n"
"  fragcolor.a = v_color.a;\n"
"}";

/* ----------------------------------------------------------------------------- */
