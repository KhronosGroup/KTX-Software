/* -*- tab-iWidth: 4; -*- */
/* vi: set sw=2 ts=4: */

/**
 * @file	sample_01_draw_texture.c
 * @brief	Tests DrawTexture functionality to see if the implementation
 *          applies the viewport transform to the supplied coordinates.
 *
 * @author	Mark Callow
 *
 * $Revision: 21082 $
 * $Date:: 2013-04-09 14:52:45 +0900 #$
 */

/*
 * Copyright (c) 2008 HI Corporation.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of HI Corporation."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#include "ktx.h"

const GLchar* pszESLangVer = "#version 300 es\n";
// location layout qualifier did not appear until version 330.
const GLchar* pszGLLangVer = "#version 330 core\n";

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
"	v_color = color;\n"
"	v_texcoord = texcoord.xy;\n"
"	gl_Position = mvpmatrix * position;\n"
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
