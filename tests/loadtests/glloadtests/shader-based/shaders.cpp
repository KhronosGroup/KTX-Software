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
"layout(location = 2) in vec4 texcoord;\n"

"out vec4 v_color;\n"
"out vec2 v_texcoord;\n"

"uniform highp mat4 mvmatrix;\n"
"uniform highp mat4 pmatrix;\n"

"void main(void)\n"
"{\n"
"   mat4 mvpmatrix = pmatrix * mvmatrix;\n"
"   v_color = color;\n"
"   v_texcoord = texcoord.xy;\n"
"   gl_Position = mvpmatrix * position;\n"
"}";

const GLchar* pszDecalFs =
"precision mediump float;\n"

"uniform sampler2D sampler;\n"

"in vec4 v_color;\n"
"in vec2 v_texcoord;\n"
"out vec4 fragcolor;\n"

"void main(void)\n"
"{\n"
"  vec4 color = texture(sampler, v_texcoord);\n"
"  // DECAL\n"
"  fragcolor.rgb = v_color.rgb * (1.0f - color.a) + color.rgb * color.a;\n"
"  fragcolor.a = color.a;\n"
"}";

const GLchar* pszColorFs = "\
precision mediump float;\n\
                        \n\
in vec4 v_color;        \n\
out vec4 fragcolor;     \n\
                        \n\
void main(void)         \n\
{                       \n\
  fragcolor = v_color;\n\
}";

/* ----------------------------------------------------------------------------- */
